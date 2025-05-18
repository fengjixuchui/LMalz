#include "main.h"
//#include <ntddk.h>
#include <intrin.h>
#include <ntimage.h>

static UINT32 memPoolTag = 'OmrI';
void ALmemSetPoolTag(UINT32 newTag)
{
	memPoolTag = newTag;
}
UINT32 ALmemGetPoolTag()
{
	return 	  memPoolTag;
}
PVOID ALmemAllocateMemory(SIZE_T NumberOfBytes)
{
	PUINT64 a = (PUINT64)ExAllocatePoolWithTag(NonPagedPool, NumberOfBytes, memPoolTag);
	if (a)
		memset(a, 0, NumberOfBytes);
	return a;
}
void ALmemFreeMemory(PVOID add)
{
	ExFreePoolWithTag(add, memPoolTag);
}
PVOID ALmemGetVA(UINT64 pa)
{
	PHYSICAL_ADDRESS a;
	a.QuadPart = pa;
	return MmGetVirtualForPhysical(a);
}
typedef struct _MMPTE_HARDWARE {

	UINT64 Valid : 1;
	UINT64 Dirty1 : 1;
	UINT64 Owner : 1;
	UINT64 WriteThrough : 1;
	UINT64 CacheDisable : 1;
	UINT64 Accessed : 1;
	UINT64 Dirty : 1;
	UINT64 LargePage : 1;
	UINT64 Global : 1;
	UINT64 CopyOnWrite : 1;
	UINT64 Unused : 1;
	UINT64 Write : 1;
	UINT64 PageFrameNumber : 36;
	UINT64 ReservedForHardware : 4;
	UINT64 ReservedForSoftware : 4;
	UINT64 WsleAge : 4;
	UINT64 WsleProtection : 3;
	UINT64 NoExecute : 1;
} MMPTE_HARDWARE, * PMMPTE_HARDWARE;

typedef struct _MMPTE {

	union {

		UINT64 Long;
		UINT64 VolatileLong;
		struct _MMPTE_HARDWARE Hard;
	} u;
} MMPTE, * PMMPTE;
typedef union CR3_STRUCT_
{
	UINT64 value;
	struct
	{
		UINT64 ignored_1 : 3;
		UINT64 write_through : 1;
		UINT64 cache_disable : 1;
		UINT64 ignored_2 : 7;
		UINT64 pml4_p : 40;
		UINT64 reserved : 12;
	};
} CR3_STRUCT;

NTSTATUS ALmemGetPteBase(PUINT8 pBuffer[]) {

	NTSTATUS Result = STATUS_SUCCESS;

	{

		PHYSICAL_ADDRESS PML4;

		CR3_STRUCT cr3 = { 0 };
		cr3.value = __readcr3();

		PML4.QuadPart = cr3.pml4_p << 12;

		PVOID VirtualAddress = MmGetVirtualForPhysical(PML4);
		/*{
			get_fun(DbgPrintEx);
			get_fun(MmGetPhysicalAddress);
			UINT8 put[] = { '[','O','M','R','I',']','v','t','p','%','p','\n','\0' };
			DbgPrintEx(0, 0, (char*)put, MmGetPhysicalAddress(MmGetVirtualForPhysical));
		}*/
		if (MmIsAddressValid(VirtualAddress)) {

			PMMPTE PageDirectory = (PMMPTE)(PAGE_ALIGN(VirtualAddress));

			for (SIZE_T Index = 0;; Index++) {

				if (PageDirectory[Index].u.Hard.PageFrameNumber == (UINT64)(PML4.QuadPart >> PAGE_SHIFT)) {

					pBuffer[0] = (PUINT8)((UINT64)(Index << 39) | (UINT64)(0xFFFF'0000'0000'0000));

					pBuffer[1] = (PUINT8)((UINT64)(Index << 30) | (UINT64)(pBuffer[0]));

					pBuffer[2] = (PUINT8)((UINT64)(Index << 21) | (UINT64)(pBuffer[1]));

					pBuffer[3] = (PUINT8)((UINT64)(Index << 12) | (UINT64)(pBuffer[2]));

					Result = STATUS_SUCCESS;

					break;
				}
			}
		}
	}

	return Result;
}


NTSTATUS ALmemAccessPhysicalMemory(void* PhysicalAddress, void* bufferAddress, UINT64 size, int isWrite)
{
	//打开内存驱动句柄
	NTSTATUS status;
	UNICODE_STRING physicalMemoryString;
	OBJECT_ATTRIBUTES attributes;
	HANDLE hPhysicalhandle = 0;
	WCHAR physicalMemoryName[] = L"\\Device\\PhysicalMemory";
	RtlInitUnicodeString(&physicalMemoryString, physicalMemoryName);
	InitializeObjectAttributes(&attributes, &physicalMemoryString, 0, NULL, NULL);
	status = ZwOpenSection(&hPhysicalhandle, SECTION_MAP_READ | SECTION_MAP_WRITE, &attributes);
	if (status != STATUS_SUCCESS)
	{
		DbgPrint("打开内存失败:%X", status);
		return status;
	}
	//映射物理地址
	PHYSICAL_ADDRESS physicalBase;
	physicalBase.QuadPart = (UINT64)PhysicalAddress;// & (~0xfff);
	PVOID virtualBase = 0;
	UINT64 exactAddress = (UINT64)PhysicalAddress & 0xfff;
	SIZE_T mappingSize = /*exactAddress +*/ size;
	/*mappingSize += (mappingSize % 0x1000) ? 0x1000 : 0;
	mappingSize &= (~0xfff); */
	status = ZwMapViewOfSection(
		hPhysicalhandle,
		NtCurrentProcess(),
		(PVOID*)&virtualBase,
		0,
		mappingSize,
		&physicalBase,
		&mappingSize,
		ViewShare,
		MEM_TOP_DOWN,
		PAGE_READWRITE);
	if (status != 0)
	{
		DbgPrint("映射物理地址失败:%x\n", status);
		return status;
	}
	exactAddress += (UINT64)virtualBase;
	if (isWrite == 0)
	{
		memcpy(bufferAddress, (PVOID)exactAddress, size);
	}
	else
	{
		memcpy((PVOID)exactAddress, bufferAddress, size);
	}

	if (hPhysicalhandle != NULL)
	{
		ZwClose(hPhysicalhandle);
	}
	status = ZwUnmapViewOfSection(NtCurrentProcess(), virtualBase);
	return status;
}
NTSTATUS ALmemCopyData(PVOID desAdd, PVOID souAdd, UINT64 size)
{

	for (UINT64 i = 0; i < size / 8; i++)
	{
		((PUINT64)desAdd)[i] = ((PUINT64)souAdd)[i];
	}
	if ((size & 0b111) >= 4)
	{
		auto a = (size & ~0b111);
		((PUINT32)desAdd)[a / 4] = ((PUINT32)souAdd)[a / 4];
	}
	for (UINT64 j = size & ~0b11; j < size; j++)
	{
		((PUINT8)desAdd)[j] = ((PUINT8)souAdd)[j];
	}
	return 0;
}
UINT64 ALmemGetPA(PVOID Vadd)
{
	return MmGetPhysicalAddress(Vadd).QuadPart;
}
