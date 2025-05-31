#include "..\ept.h"

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

bool ALvmxMNTsetTarget(PVOID va, UINT64 size)
{
	auto start_a = (UINT64)va & ~0xfffLL;
	auto end = (((UINT64)va + size) + 0xfff) & ~0xfffLL;
	
	for (PUINT8 i = (PUINT8)start_a; i < (PUINT8)end; i += 0x1000)
	{
		auto pa = ALhvMMgetPA(i);
		g_target_X_ept->set_pte()

	}
	return 1;

}