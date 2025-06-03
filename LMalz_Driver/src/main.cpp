#include "orhv\orhv.h"
#include "dbgput.h"
bool ALhvStart();

#pragma code_seg(".onon")


void test()
{
	ALdbgPut("测试成功");
}



#pragma  code_seg()

bool ALvmxMNTinit();
bool ALvmxMNTsetTarget(PVOID va, UINT64 size);
int main(UINT64*)
{
	ALdbgPut("驱动加载成功,开始加载VT");

	if (!ALhvStart())
	{
		auto err = ALhvGetErr();
		ALdbgPut("加载出错:%s", err);
		return 0;
	}
	else
	{
		ALdbgPut("加载成功");
		if (!ALvmxMNTinit())
		{
			ALdbgPut("监控初始化失败");
			return 0;
		}
		auto b = ALvmxMNTsetTarget((PVOID)((UINT64)test & ~0xfffLL), 0x1000);
		if (!b)
		{
			ALdbgPut("监控设置失败");
			return 0;
		}
		ALdbgPut("开始调用测试函数");
		test();
		ALdbgPut("调用测试函数完毕");

	}
	return 0;
}