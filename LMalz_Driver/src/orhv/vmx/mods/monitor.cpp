#include "..\ept.h"
#include "..\vmx.inl"
#include "..\vmx_exitHandler.h"

static EPT_CLS* g_target_X_ept = 0;
static EPT_CLS* g_system_X_ept = 0;

//需要两个EPT,一个仅有目标页表
bool ALvmxMNTinit()
{
	static EPT_CLS target_X_ept(1, 1, 0);  //新建一个让目标执行的页表  允许读写
	static EPT_CLS system_X_ept;  //克隆一个让系统执行的页表   
	g_target_X_ept = &target_X_ept;
	g_system_X_ept = &system_X_ept;
	if (g_target_X_ept->getPml4e().vv == 0)
	{
		ALhvAddErr("新建失败");
		return 0;
	}
	if (g_system_X_ept->getPml4e().vv == 0)
	{
		ALhvAddErr("新建失败");
		return 0;
	}
	return 1;
}
static bool monitor_ept_violation_handler(OR_HV_VMX_CORE* vcpu, bool is2)
{

	vcpu;
	is2;
	return 0;
}
bool ALvmxMNTsetTarget(PVOID va, UINT64 size)
{
	auto start_a = (UINT64)va & ~0xfffLL;
	auto end = (((UINT64)va + size) + 0xfff) & ~0xfffLL;
	
	for (PUINT8 i = (PUINT8)start_a; i < (PUINT8)end; i += 0x1000)
	{
		auto pa = ALhvMMgetPA(i);
		g_target_X_ept->set_pte(pa, 1, 1, 1);	   //仅让目标模块执行

		g_system_X_ept->set_pte(pa, 1, 1, 0);	   //让目标不可执行
	}
	auto b = ALvmxGetExitHandlers()[VMX_EXIT_REASON_EPT_VIOLATION].add_fun(monitor_ept_violation_handler);
	if (!b)
	{
		ALhvAddErr("添加处理函数失败");
		return 0;
	}

	set_ept(g_system_X_ept->getEptp());

	return 1;

}