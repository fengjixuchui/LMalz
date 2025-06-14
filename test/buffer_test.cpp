#include "..\LMalz_Driver\src\Buffer.h"
#define __R3
#include "F:\g_un\ALJT\Code\omri\include\debug\debug.h"


UINT8 aaa[0x1000] = { 0 };
OR_SHA_IO* aa = 0;
void hd(PVOID mm, UINT64 sz)
{
	/*ALdbgPutValue(*(PUINT64)mm);
	
	ALdbgPutValue(*((PUINT64)mm + 1));*/
	ALdbgPut("1");
}
DWORD t(
	LPVOID lpThreadParameter
	)
{
	while (true)
	{
		aa->ReadData(hd);
	}
}
int main()
{
	OR_SHA_IO a(aaa, 0x1000);
	aa = &a;
	CreateThread(0, 0, t, 0, 0, 0);
	UINT64 id = 0;
	while (1)
	{
		auto mm = (PUINT64)a.PrepareData(16);
		ALdbgPutValue(mm);
		if (mm)
		{
			*mm = 0x1122334455667788;
			mm[1] = id++;
			a.SendData(mm);
			Sleep(100);
		}
		a.ReadData(hd);
		Sleep(100);
		mm = (PUINT64)a.PrepareData(16);
		ALdbgPutValue(mm);
		if (mm)
		{
			*mm = 0x1122334455667788;
			mm[1] = id++;
			a.SendData(mm);
		}
		a.ReadData(hd);
	}
	while (1);
}

