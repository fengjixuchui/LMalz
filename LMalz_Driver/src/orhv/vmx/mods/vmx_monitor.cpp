#include "..\ept.h"
#include "..\vmx.inl"
#include "..\vmx_exitHandler.h"
#include "vmx_monitor.h"
/*
*  三页表设计，其一全属性，重映射自身模块物理内存，使自身模块在guest层面为0，
*  其二全局全属性目标缺属性用于触发违例，可在此页表再次重映射自身某段用于guest执行，
*  其三复制第一份页表映射数据但全局缺执行目标带执行用于单步控制，此方案可用于驱动调试，
* 
*  注:用于进程内存执行控制时需结合写时复制重映射模块内存，防止影响其他进程
*/
static EPT_CLS* g_target_X_ept = 0;
static MONITOR_CALLBACK g_callback = 0;

struct MNT_INFO
{
	UINT64 mtf_add;//最后一个MTF的RIP地址
};

static MNT_INFO* g_infos = 0;
UINT64 ALvmxMNTgetLastMTFaddress()
{
	return g_infos[ALhvGetCurrVcoreIndex()].mtf_add;
}

static bool monitor_MTF_handler(OR_HV_VMX_CORE* vcpu, bool is2)
{
	is2;
	auto curr_ept = get_ept();
	if (curr_ept.page_frame_number == g_target_X_ept->getEptp().page_frame_number)
	{
		g_infos[ALhvGetCurrVcoreIndex()].mtf_add = vcpu->reg.rip;
		g_callback(vcpu, MONITOR_CALLBACK_TYPE_mtf);
		return 1;
	}
	else
		return 0;
}

static bool monitor_ept_violation_handler(OR_HV_VMX_CORE* vcpu, bool is2)
{
	is2;
	vmx_exit_qualification_ept_violation qualification;
	qualification.flags = vmx_vmread(VMCS_EXIT_QUALIFICATION);

	// guest physical address that caused the ept-violation
	auto const physical_address = vmx_vmread(VMCS_GUEST_PHYSICAL_ADDRESS);
	auto const virtual_address = vmx_vmread(VMCS_EXIT_GUEST_LINEAR_ADDRESS);
	if (!qualification.execute_access)	  //只接管执行访问异常
		return 0;
		//ALhvPutLog("1");
	
		//ALhvPutLog("%p %p %p", virtual_address,physical_address, target_pte.flags);



	auto curr_ept = get_ept();
	if (curr_ept.page_frame_number == EPT_CLS::getViceEpt()->getEptp().page_frame_number)  //副表
	{
		auto target_pte = g_target_X_ept->get_pte(physical_address);
		if (!target_pte.execute_access)//如果在目标执行页表没有访问权限则不是本模块该处理的
			return 0;
		g_callback(vcpu, MONITOR_CALLBACK_TYPE_enter);
		set_ept(g_target_X_ept->getEptp());
		//ALvmxInvept_asm(invept_all_context, {});

		//enable_monitor_trap_flag();		//设置执行后MTF
		//inject_mtf();//悬挂一个MTF事件,让进入后直接退出
		return 1;
		//ALhvKill(0, 0);
	}
	else if (curr_ept.page_frame_number == g_target_X_ept->getEptp().page_frame_number)	  //退出目标模块
	{
		g_callback(vcpu, MONITOR_CALLBACK_TYPE_exit);
		set_ept(EPT_CLS::getViceEpt()->getEptp());
		//disable_monitor_trap_flag();
		return 1;
	}
	//ALhvPutLog("1");

	return 0;
}
#define MNT_VMCALL_KEY 'MNT'
enum MNT_VMCALL_FUNCTION {
	MNT_VMCALL_FUNCTION_INVEPT_ALL,
	MNT_VMCALL_FUNCTION_test,




};
static bool monitor_handler_vmcall(OR_HV_VMX_CORE* vcpu, bool is2)
{
	vcpu;
	is2;
	if (vcpu->reg.ecx == MNT_VMCALL_KEY)
	{
		switch (vcpu->reg.rdx)
		{
		case MNT_VMCALL_FUNCTION_INVEPT_ALL:
		{
			ALvmxInvept_asm(invept_all_context, {});
			/*invvpid_descriptor desc;
			desc.linear_address = 0;
			desc.reserved1 = 0;
			desc.reserved2 = 0;
			desc.vpid = 1;
			ALvmxInvvpid_asm(invvpid_single_context, desc);*/
			break;
		}
		case MNT_VMCALL_FUNCTION_test:
		{
			auto va = vcpu->reg.r8;
			auto size = vcpu->reg.r9;

			auto start_a = (UINT64)va & ~0xfffLL;
			auto end = (((UINT64)va + size) + 0xfff) & ~0xfffLL;

			for (PUINT8 i = (PUINT8)start_a; i < (PUINT8)end; i += 0x1000)
			{
				auto pa = ALhvMMgetPA(i);
				g_target_X_ept->set_pte(pa, 1, 1, 1);	   //仅让目标模块执行

				EPT_CLS::getViceEpt()->set_pte(pa, 1, 1, 0);	   //让目标在副表不可执行
			}
			/*ALvmxInvept_asm(invept_all_context,{});

			invvpid_descriptor desc;
			desc.linear_address = 0;
			desc.reserved1 = 0;
			desc.reserved2 = 0;
			desc.vpid = 1;
			ALvmxInvvpid_asm(invvpid_single_context_retaining_globals, desc);*/

			break;
		}
		default:
			ALhvKill(vcpu->reg.rip, 0);
		}
		vmx_increment_rip(vcpu);
		return 1;
	}
	else
		return 0;
}

static UINT64 monitor_vmcall(UINT64 code_rdx, UINT64 r8 = 0, UINT64 r9 = 0)
{
	return ALvmxVmcall_asm(MNT_VMCALL_KEY, code_rdx, r8, r9);
}
static void  monitor_invept_all()
{
	monitor_vmcall(MNT_VMCALL_FUNCTION_INVEPT_ALL);
}
#pragma warning(disable:4505)


//需要两个EPT,一个仅有目标页表
bool ALvmxMNTinit(MONITOR_CALLBACK callback)
{
	
	g_infos = (GT(g_infos))ALhvMMallocateMemory(ALhvGetCoreCount() * sizeof(GT(*g_infos)));

	static EPT_CLS target_X_ept(1, 1, 0);  //新建一个让目标执行的页表  允许读写
	g_callback = callback;
	if (g_callback == 0)
	{
		ALhvSetErr("参数错误");
		return 0;
	}
	g_target_X_ept = &target_X_ept;
	if (g_target_X_ept->getPml4e().vv == 0)
	{
		ALhvAddErr("新建失败");
		return 0;
	}
	auto b = ALvmxGetExitHandlers()[VMX_EXIT_REASON_EPT_VIOLATION].add_fun(monitor_ept_violation_handler);

	b &= ALvmxGetExitHandlers()[VMX_EXIT_REASON_EXECUTE_VMCALL].add_fun(monitor_handler_vmcall);

	b &= ALvmxGetExitHandlers()[VMX_EXIT_REASON_MONITOR_TRAP_FLAG].add_fun(monitor_MTF_handler);
	if (!b)
	{
		ALhvAddErr("添加处理函数失败");
		return 0;
	}
	return 1;
}
 UINT64 ALvmxEPTgetPageTableInfo(UINT64 GPA, int layerNumber, ept_pml4e* ept);
 //#pragma warning(disable:4702)

bool ALvmxMNTsetTarget(PVOID va, UINT64 size)
{
	/*monitor_vmcall(MNT_VMCALL_FUNCTION_test, (UINT64)va, size);
	__invlpg(va);

	return 1;*/
	/*auto v = EPT_CLS::getViceEpt()->getEptp().flags;
	ALhvPutLog("!!!%p", v);*/
	auto start_a = (UINT64)va & ~0xfffLL;
	auto end = (((UINT64)va + size) + 0xfff) & ~0xfffLL;

	for (PUINT8 i = (PUINT8)start_a; i < (PUINT8)end; i += 0x1000)
	{
		auto pa = ALhvMMgetPA(i);

		/*ALhvPutLog("%p", pa);

		ALhvPutLog("改之前目标执行页1 %p", g_target_X_ept->get_pml4e(pa));
		ALhvPutLog("改之前目标执行页2 %p", g_target_X_ept->get_pdpte(pa));
		ALhvPutLog("改之前目标执行页3 %p", g_target_X_ept->get_pde(pa));
		ALhvPutLog("改之前目标执行页4 %p", g_target_X_ept->get_pte(pa));

		ALhvPutLog("改之前主表1 %p", ALvmxEPTgetPageTableInfo(pa, 1, EPT_CLS::getConstEpt()->va));
		ALhvPutLog("改之前主表1 %p", ALvmxEPTgetPageTableInfo(pa, 2, EPT_CLS::getConstEpt()->va));
		ALhvPutLog("改之前主表1 %p", ALvmxEPTgetPageTableInfo(pa, 3, EPT_CLS::getConstEpt()->va));
		ALhvPutLog("改之前主表1 %p", ALvmxEPTgetPageTableInfo(pa, 4, EPT_CLS::getConstEpt()->va));

		ALhvPutLog("改之前系统执行页1 %p", EPT_CLS::getViceEpt()->get_pml4e(pa));
		ALhvPutLog("改之前系统执行页2 %p", EPT_CLS::getViceEpt()->get_pdpte(pa));
		ALhvPutLog("改之前系统执行页3 %p", EPT_CLS::getViceEpt()->get_pde(pa));
		ALhvPutLog("改之前系统执行页4 %p", EPT_CLS::getViceEpt()->get_pte(pa));*/

		g_target_X_ept->set_pte(pa, 1, 1, 1);	   //仅让目标模块执行

		EPT_CLS::getViceEpt()->set_pte(pa, 1, 1, 0);	   //让目标在副表不可执行

		//pa += 0x1000;

		/*ALhvPutLog("改之后目标执行页1 %p", g_target_X_ept->get_pml4e(pa));
		ALhvPutLog("改之后目标执行页2 %p", g_target_X_ept->get_pdpte(pa));
		ALhvPutLog("改之后目标执行页3 %p", g_target_X_ept->get_pde(pa));
		ALhvPutLog("改之后目标执行页4 %p", g_target_X_ept->get_pte(pa));

		ALhvPutLog("改之后主表1 %p", ALvmxEPTgetPageTableInfo(pa, 1, EPT_CLS::getConstEpt()->va));
		ALhvPutLog("改之后主表1 %p", ALvmxEPTgetPageTableInfo(pa, 2, EPT_CLS::getConstEpt()->va));
		ALhvPutLog("改之后主表1 %p", ALvmxEPTgetPageTableInfo(pa, 3, EPT_CLS::getConstEpt()->va));
		ALhvPutLog("改之后主表1 %p", ALvmxEPTgetPageTableInfo(pa, 4, EPT_CLS::getConstEpt()->va));

		ALhvPutLog("改之后系统执行页1 %p", EPT_CLS::getViceEpt()->get_pml4e(pa));
		ALhvPutLog("改之后系统执行页2 %p", EPT_CLS::getViceEpt()->get_pdpte(pa));
		ALhvPutLog("改之后系统执行页3 %p", EPT_CLS::getViceEpt()->get_pde(pa));
		ALhvPutLog("改之后系统执行页4 %p", EPT_CLS::getViceEpt()->get_pte(pa));*/

		__invlpg(va);
	}
	monitor_invept_all();


	return 1;
}