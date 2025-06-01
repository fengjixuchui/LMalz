#include "orhv.h"
#include "vmx\vmx.h"
#include "svm\svm.h"
#include "..\main.h"

static OR_HV_CPU_TYPE GetCPUtype()
{
	typedef struct _CPUID_reg
	{
		int eax;
		int ebx;
		int ecx;
		int edx;
	} CPUID_reg, * PCPUID_reg;
	CPUID_reg data = { 0 };
	char vendor[0x20] = { 0 };
	__cpuid((int*)&data, 0);
	*(int*)(vendor) = data.ebx;
	*(int*)(vendor + 4) = data.edx;
	*(int*)(vendor + 8) = data.ecx;

	if (memcmp(vendor, "GenuineIntel", 12) == 0)
		return OR_HV_CPU_TYPE::OR_VM_CPU_TYPE_intel;
	if (memcmp(vendor, "AuthenticAMD", 12) == 0)
	{
		return OR_HV_CPU_TYPE::OR_VM_CPU_TYPE_AMD;
	}
	return OR_HV_CPU_TYPE::OR_VM_CPU_TYPE_Other;
}
OR_HV gALhvData = {};
inline bool ALhvIsIntelCPU()
{
	return gALhvData.cpu_type == OR_HV_CPU_TYPE::OR_VM_CPU_TYPE_intel;
}

inline bool ALhvIsAMDCPU()
{
	return gALhvData.cpu_type == OR_HV_CPU_TYPE::OR_VM_CPU_TYPE_AMD;
}

int ALhvGetCoreCount()
{
	return KeQueryActiveProcessorCountEx(ALL_PROCESSOR_GROUPS);
}
static bool MapAPICidToCoreIndex();
template<typename VCPU>
bool ALhvInitialization(VCPU* vcpu)
{
	{
		if (!MapAPICidToCoreIndex())
		{
			ALhvAddErr("核心号映射失败");
			return 0;
		}
		auto ret = ALhvMMinitPool();
		if (!ret)
		{
			ALhvAddErr("初始化内存池失败");
			return 0;
		}
	}
	//申请核心对象数组
	{
		auto core_count = ALhvGetCoreCount();

		auto const arr_size = sizeof(*vcpu->cores) * core_count;

		auto cores = (GT(vcpu->cores))ALhvMMallocateMemory(arr_size);

		if (cores == 0)
		{
			ALhvAddErr("申请vcpu内存失败");
			return 0;
		}

		vcpu->core_count = core_count;
		vcpu->cores = cores;

	}
	//获取系统cr3
	{

		auto sys_cr3 = ALhvMMgetCr3ByObject((PEPROCESS)PsInitialSystemProcess);
		if (sys_cr3.flags == 0)
		{
			ALhvSetErr("获取系统cr3失败");
			return 0;
		}
		PHYSICAL_ADDRESS pml4_address;
		pml4_address.QuadPart = sys_cr3.address_of_page_directory << 12;

		vcpu->system_cr3 = sys_cr3;
	}
	//获取host cr3
	{
		cr3 host_cr3 = {};
		auto ret = ALhvMMgetHostCr3(vcpu->system_cr3, &host_cr3);
		if (!ret)
		{
			ALhvAddErr("制作hostcr3失败");
			return 0;
		}
		vcpu->host_cr3 = host_cr3;
		ALhvPutLog("%p", vcpu->host_cr3);
	}
	//获取host_idt
	{
		auto host_idt = ALhvIDT_prepare_host_idt();
		if (!host_idt)
		{
			ALhvAddErr("制作hostidt失败");
			return 0;
		}
		vcpu->host_idt = host_idt;
		ALhvPutLog("%p", vcpu->host_idt);

	}
	//获取HOST_gdt
	{
		auto host_gdt = ALhvGDTgetHostGDT();
		if (!host_gdt)
		{
			ALhvAddErr("制作hostidt失败");
			return 0;
		}
		vcpu->host_gdt = host_gdt;
		ALhvPutLog("%p", vcpu->host_gdt);

	}

	return 1;
}


bool ALhvStart()
{
	{
		gALhvData.cpu_type = GetCPUtype();
		//禁止睡眠
		gALhvData.SystemState = PoRegisterSystemState(NULL, ES_SYSTEM_REQUIRED | ES_CONTINUOUS);

		//	void PoUnregisterSystemState(
		//  [in, out] PVOID StateHandle
		//);
	}
	if (gALhvData.cpu_type == OR_HV_CPU_TYPE::OR_VM_CPU_TYPE_intel)
	{
		if (!ALhvInitialization(&gALhvData.vmx))
		{
			ALhvAddErr("hv初始化失败");
			return 0;
		}
		return ALvmxStart(&gALhvData.vmx);
	}
	else if (gALhvData.cpu_type == OR_HV_CPU_TYPE::OR_VM_CPU_TYPE_AMD)
	{
		if (!ALhvInitialization(&gALhvData.svm))
		{
			ALhvAddErr("hv初始化失败");
			return 0;
		}
		return ALsvmStart(&gALhvData.svm);
	}
	else
	{
		ALhvSetErr("未知CPU类型");
	}
	return 1;
}


bool ALhvIsHost()
{
	if (ALhvIsIntelCPU())
		return ALvmxIsRoot();
	else
		return ALsvmIsRoot();
}

#define APIC_MAP_SIZE 512
static UINT8 apid_to_coreIndex[APIC_MAP_SIZE] = {};//假设apic最大为511

UINT32 ALhvGetAPIC_id()
{
	// 通过CPUID获取APIC ID（示例：Leaf 0x01的EBX高24位）
	int regs[4];
	__cpuid(regs, 1);
	UINT32 apic_id = (regs[1] >> 24) & 0xFF;

	return 	apic_id;
}
static bool MapAPICidToCoreIndex()
{
	//apid_to_coreIndex
	auto core_count = ALhvGetCoreCount();
	PROCESSOR_NUMBER processorNumber = { 0 };
	GROUP_AFFINITY   affinity = { 0 }, oldAffinity = { 0 };
	if (core_count > (1LL << ((sizeof(GT(*apid_to_coreIndex)) * 8) - 1)))
	{
		ALhvSetErr("核心数量过多,请重新设置映射图颗粒尺寸", core_count);
		return 0;
	}
	for (UINT32 i = 0; i < (UINT32)core_count; i++)
	{
		//切换处理器核心
		auto status = KeGetProcessorNumberFromIndex(i, &processorNumber);
		if (!NT_SUCCESS(status)) {
			ALhvSetErr("切换核心失败%d", i);
			return 0;
		}
		affinity.Group = processorNumber.Group;
		affinity.Mask = 1ULL << processorNumber.Number;
		affinity.Reserved[0] = affinity.Reserved[1] = affinity.Reserved[2] = 0;
		KeSetSystemGroupAffinityThread(&affinity, &oldAffinity);
		//开始当前核心虚拟化
		auto apic_id = ALhvGetAPIC_id();
		if (apic_id >= APIC_MAP_SIZE)
		{
			ALhvSetErr("APIC_id过大%d %d", i, apic_id);
			return 0;
		}
		apid_to_coreIndex[apic_id] = (UINT8)i;
		//换回原核心
		KeRevertToUserGroupAffinityThread(&oldAffinity);

	}
	return 1;
}


int ALhvGetCurrVcoreIndex()	 
{
	return apid_to_coreIndex[ALhvGetAPIC_id()];
}
// calculate a segment's base address
uint64_t ALhvGetSegmentBase(
	segment_descriptor_register_64 const& gdtr,
	segment_selector const selector) {
	// null selector
	if (selector.index == 0)
		return 0;

	// fetch the segment descriptor from the gdtr
	auto const descriptor = reinterpret_cast<segment_descriptor_64*>(
		gdtr.base_address + static_cast<uint64_t>(selector.index) * 8);

	// 3.3.4.5
	// calculate the segment base address
	auto base_address =
		(uint64_t)descriptor->base_address_low |
		((uint64_t)descriptor->base_address_middle << 16) |
		((uint64_t)descriptor->base_address_high << 24);

	// 3.3.5.2
	// system descriptors are expanded to 16 bytes for ia-32e
	if (descriptor->descriptor_type == SEGMENT_DESCRIPTOR_TYPE_SYSTEM)
		base_address |= (uint64_t)descriptor->base_address_upper << 32;

	return base_address;
}


