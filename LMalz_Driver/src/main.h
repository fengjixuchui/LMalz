#pragma once
#include <ntifs.h>
#include <ntddk.h>
/*
* 命名规范:
* 
* 全局变量:g + 模块缩写 + '_'+ 描述
* 静态全局变量:g_开头作为标识
* 
* 普通函数:AL+ 模块缩写 + 描述
* 汇编函数:AL+ 模块缩写 + 描述 + '_asm'
* 静态函数:描述
* 
* 结构体:'OR_'+描述
* 
* 模块之间尽量不要互相引用,代码复用率低一点也没关系
* 
*/



#pragma warning(disable:4201)

typedef union VIRTUAL_ADDRESS_
{
	UINT64 value;
	void* pointer;
	struct
	{
		UINT64 offset : 12;
		UINT64 pt_index : 9;
		UINT64 pd_index : 9;
		UINT64 pdpt_index : 9;
		UINT64 pml4_index : 9;
		UINT64 reserved : 16;
	};
} OR_ADDRESS;
#define GT(a) decltype(a)

#define _2u64(a) ((UINT64)a)
#define _2p(a) ((PVOID)a)
#define _2pg(a) ((GT(a))(((UINT64)a)&~0xfff))
#define _Gpg_off(a) ((UINT64)a&0xfff)

#define _duiqi
#define _piany


//memory.h
PVOID ALmemAllocateMemory(SIZE_T NumberOfBytes);
void ALmemFreeMemory(PVOID add);
void ALmemSetPoolTag(UINT32 newTag);
UINT64 ALmemGetPA(PVOID Vadd);
PVOID ALmemGetVA(UINT64 pa);
NTSTATUS ALmemAccessPhysicalMemory(void* PhysicalAddress, void* bufferAddress, UINT64 size, int isWrite);
NTSTATUS ALmemGetPteBase(PUINT8 pBuffer[]);
NTSTATUS ALmemCopyData(PVOID desAdd, PVOID souAdd, UINT64 size);
#define MiGetPxeAddress(BASE, VA) ((UINT64)BASE + ((UINT32)(((UINT64)(VA) >> 39) & 0x1FF)))
#define MiGetPpeAddress(BASE, VA) ((UINT64)(((((UINT64)VA & 0xFFFFFFFFFFFF) >> 30) << 3) + BASE))
#define MiGetPdeAddress(BASE, VA) ((UINT64)(((((UINT64)VA & 0xFFFFFFFFFFFF) >> 21) << 3) + BASE))
#define MiGetPteAddress(BASE, VA) ((UINT64)(((((UINT64)VA & 0xFFFFFFFFFFFF) >> 12) << 3) + BASE))