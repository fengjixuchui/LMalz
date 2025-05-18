//#include "orhv.h"
//#include "..\main.h"
//
//
//
//
//
//static bool CheckCPUtype()
//{
//	CPUID_reg data = { 0 };
//	char vendor[0x20] = { 0 };
//	__cpuid((int*)&data, 0);
//	*(int*)(vendor) = data.ebx;
//	*(int*)(vendor + 4) = data.edx;
//	*(int*)(vendor + 8) = data.ecx;
//
//	if (memcmp(vendor, "GenuineIntel", 12) == 0)
//		return 1;
//	if (memcmp(vendor, "AuthenticAMD", 12) == 0)
//	{
//		ALhvSetErr("AMD CPU ,go out");
//		return 0;
//	}
//	ALhvSetErr("??? CPU ,go out");
//	return 0;
//}
//
//hv_data ghv = { 0 };
//
//static cr3 GetCr3ByObject(PEPROCESS process)
//{
//	KAPC_STATE pr;
//	KeStackAttachProcess(process, &pr);
//	cr3 a = { 0 };
//	a.flags = __readcr3();
//	KeUnstackDetachProcess(&pr);
//	return a;
//}
//int ALhvGetCurrentCoreObjectNumber()
//{
//	return KeGetCurrentProcessorNumberEx(0);
//}
//vcpu* ALhvGetVcpu()
//{
//	if (ghv.vcpus == 0)
//	{
//		ALhvSetErr("对象未初始化");
//		return 0;
//	}
//	auto  rsp = (UINT64)_AddressOfReturnAddress();
//	for (UINT32 i = 0; i < ghv.vcpu_count; i++)
//		if (rsp >= (UINT64)(&ghv.vcpus[i]) && rsp < (UINT64)(&ghv.vcpus[i + 1]))
//			return  &ghv.vcpus[i];
//	//如果上面找不到说明不在root模式,直接调用api
//	return  &ghv.vcpus[ALhvGetCurrentCoreObjectNumber()];
//}
//bool ALhvIsHost()
//{
//	auto obj = ALhvGetVcpu();
//	if (obj != 0)
//	{
//		return obj->isHost;
//	}
//	return 0;
//}
//
//// initialize the host page tables
////初始化host cr3
//bool prepare_host_page_tables()
//{
//	//复制system cr3 的 pml4后256个项,之后重新构建自身,idt,gdt的映射路径,还要修复自映射
//	auto Hostpml4 = (pml4e_64*)ALhvMMallocateMemory(0x1000);
//
//
//	// kernel PML4 address
//	auto const guest_pml4 = ghv.system_page_table;
//	memcpy(&Hostpml4[256], &guest_pml4[256], sizeof(pml4e_64) * 256);
//	if (!ALhvMMsetAllPA(Hostpml4))
//	{
//		ALhvAddErr("映射所有物理内存失败");
//		return 0;
//	}
//
//
//	auto Hostpml4_pa = ALmemGetPA(Hostpml4);
//	if (!Hostpml4_pa)
//	{
//		ALhvSetErr("获取Hostpml4_pa失败");
//		return 0;
//	}
//	//修复自映射
//	auto fixselfmap = [guest_pml4, Hostpml4, Hostpml4_pa]()->bool {
//		for (int i = 255; i < 512; i++)
//		{
//			if (guest_pml4[i].page_frame_number == ghv.system_cr3.address_of_page_directory)
//			{
//				Hostpml4[i].page_frame_number = Hostpml4_pa >> 12;
//				return 1;
//			}
//		}
//		return 0;
//	};
//
//	if (fixselfmap() == 0)
//	{
//		ALhvSetErr("修复自映射失败");
//		return 0;
//	}
//
//
//
//	auto guestcr3 = ghv.system_cr3;
//	auto hostcr3 = guestcr3;
//	hostcr3.address_of_page_directory = Hostpml4_pa >> 12;
//	ghv.host_cr3 = hostcr3;
//
//	ghv.host_page_table = Hostpml4;
//
//
//
//
//
//
//	return 1;
//}
////初始化一些必要数据 
//static bool vmInitialization()
//{
//	//禁止睡眠
//	{
//		ghv.SystemState = PoRegisterSystemState(NULL, ES_SYSTEM_REQUIRED | ES_CONTINUOUS);
//
//		//	void PoUnregisterSystemState(
//		//  [in, out] PVOID StateHandle
//		//);
//	}
//	auto ret = ALhvMMinitPool();
//	if (!ret)
//	{
//		ALhvAddErr("初始化内存池失败");
//		return 0;
//	}
//	{
//		ghv.vcpu_count = KeQueryActiveProcessorCountEx(ALL_PROCESSOR_GROUPS);
//
//		auto const arr_size = sizeof(vcpu) * ghv.vcpu_count;
//
//		ghv.vcpus = (vcpu*)ALhvMMallocateMemory(arr_size);
//
//		if (ghv.vcpus == 0)
//		{
//			ALhvAddErr("申请vcpu内存失败");
//			return 0;
//		}
//	}
//
//	{
//
//		auto sys_cr3 = GetCr3ByObject((PEPROCESS)PsInitialSystemProcess);
//		if (sys_cr3.flags == 0)
//		{
//			ALhvSetErr("获取系统cr3失败");
//			return 0;
//		}
//		PHYSICAL_ADDRESS pml4_address;
//		pml4_address.QuadPart = ghv.system_cr3.address_of_page_directory << 12;
//		auto pt = static_cast<pml4e_64*>(ALmemGetVA(pml4_address.QuadPart));
//		if (pt == 0)
//		{
//			ALhvSetErr("获取系统cr3虚拟地址失败");
//			return 0;
//		}
//
//		ghv.system_cr3 = sys_cr3;
//		ghv.system_page_table = pt;
//	}
//
//	{
//		auto a = (UINT64)(vmInitialization);
//		a = _2pg(a);
//		while (*(PUINT16)a != (UINT16)'ZM')
//			a -= 0x1000;
//		PIMAGE_DOS_HEADER DOS = (PIMAGE_DOS_HEADER)a;									   //设置DOS头
//		PIMAGE_NT_HEADERS NT = (PIMAGE_NT_HEADERS)(DOS->e_lfanew + (PUCHAR)a);			   //设置NT头
//		PIMAGE_NT_HEADERS pNTHeader = (PIMAGE_NT_HEADERS)NT; // PE头
//		PIMAGE_SECTION_HEADER pSectionHeader = (PIMAGE_SECTION_HEADER)((ULONG64)pNTHeader + sizeof(IMAGE_NT_HEADERS));
//		int nAlign = pNTHeader->OptionalHeader.SectionAlignment; //段对齐字节数
//		//// 计算所有头的尺寸。包括dos, coff, pe头 和 段表的大小
//		int ImageSize = (pNTHeader->OptionalHeader.SizeOfHeaders + nAlign - 1) / nAlign * nAlign;
//		// 计算所有节的大小
//		for (int i = 0; i < pNTHeader->FileHeader.NumberOfSections; ++i)
//		{
//			//得到该节的大小
//			int CodeSize = pSectionHeader[i].Misc.VirtualSize;
//			int LoadSize = pSectionHeader[i].SizeOfRawData;
//			int MaxSize = (LoadSize > CodeSize) ? (LoadSize) : (CodeSize);
//
//			int SectionSize = (pSectionHeader[i].VirtualAddress + MaxSize + nAlign - 1) / nAlign * nAlign;
//			if (ImageSize < SectionSize)
//				ImageSize = SectionSize; //Use the Max;
//		}
//		ghv.driver_base = (PVOID)a;
//		ghv.driver_size = ImageSize;
//	}
//	//初始化host 页表
//	{
//
//		ret = prepare_host_page_tables();
//		if (ret == 0)
//		{
//			ALhvAddErr("构建host页表失败");
//			return 0;
//		}
//
//		ret = ALhvMMrebuildPath(ghv.system_page_table, ghv.host_page_table, ghv.driver_base, ghv.driver_size); //重新构建驱动映射路径
//		if (!ret)
//		{
//			ALhvAddErr("重新构建驱动映射路径失败");
//			return 0;
//		}
//
//		ret = ALhvMMrebuildMemPoolPath(ghv.system_page_table, ghv.host_page_table);
//		if (!ret)
//		{
//			ALhvAddErr("重新构建内存池路径失败");
//			return 0;
//		}
//	}
//
//
//	return 1;
//
//}
//bool ALvmxStart()
//{
//	if (!CheckCPUtype())
//	{
//		ALhvAddErr("CPU 型号不受支持");
//		return 0;
//	}
//	if (!vmInitialization())
//	{
//		ALhvAddErr("初始化失败");
//		return 0;
//	}
//
//
//	PROCESSOR_NUMBER processorNumber = { 0 };
//	GROUP_AFFINITY   affinity = { 0 }, oldAffinity = { 0 };
//	UINT64 success = 0;
//	ALhvSetErr("");
//	for (ULONG i = 0; i < ghv.vcpu_count; i++)
//	{
//		//切换处理器核心
//		auto status = KeGetProcessorNumberFromIndex(i, &processorNumber);
//		if (!NT_SUCCESS(status)) {
//			ALhvSetErr("切换核心失败%d", i);
//			break;
//		}
//		affinity.Group = processorNumber.Group;
//		affinity.Mask = 1ULL << processorNumber.Number;
//		affinity.Reserved[0] = affinity.Reserved[1] = affinity.Reserved[2] = 0;
//		KeSetSystemGroupAffinityThread(&affinity, &oldAffinity);
//
//		//开始当前核心虚拟化
//		if (ALvmxCoreStart(&ghv.vcpus[i]))
//		{
//			(success++);
//		}
//		else
//		{
//			ALhvAddErr("%d核心虚拟化失败", i);
//		}
//
//		//换回原核心
//		KeRevertToUserGroupAffinityThread(&oldAffinity);
//
//	}
//
//
//	//ALdbgPut("VT Engine has been loaded!\n");
//
//	return success == ghv.vcpu_count;
//
//
//
//}