#pragma once
#include <intrin.h>
#include <ntifs.h>
#include <ntimage.h>
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

template <typename T = void>
struct OR_PTR
{
	union
	{
		T* va;
		UINT64 vv;
	};
	union
	{
		PVOID pa;
		UINT64 pv;
	};
	T* operator->()	const
	{
		return va;
	};
};
static_assert (sizeof(OR_PTR<>) == 16, "");



#define _2u64(a) ((UINT64)a)
#define _2p(a) ((PVOID)a)
#define _2pg(a) ((GT(a))(((UINT64)a)&~0xfff))
#define _Gpg_off(a) ((UINT64)a&0xfff)

#define GT(a) decltype(a)

#define GB_TO_BYTE(a) ((UINT64)a*1024LL*1024LL*1024LL)
#define MB_TO_BYTE(a) ((UINT64)a*1024LL*1024LL)
#define ALIGNS_MB(value,n_MB) (((UINT64)value + (MB_TO_BYTE(n_MB)-1)) & ~(MB_TO_BYTE(n_MB)-1)) 
#define ALIGNS_PAGE(value) (((UINT64)(value) + 0xfff) & ~0xfffLL)