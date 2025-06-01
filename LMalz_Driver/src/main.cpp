#include "orhv\orhv.h"
#include "dbgput.h"
bool ALhvStart();
int main(UINT64*)
{
	ALdbgPut("驱动加载成功,开始加载VT");

	if (!ALhvStart())
	{
		auto err = ALhvGetErr();
		ALdbgPut("加载出错:%s", err);
	}
	else
	{
		ALdbgPut("加载成功");
	}
	return 0;
}