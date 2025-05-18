#include "orhv.h"


#define MiGetPxeAddress(BASE, VA) ((UINT64)BASE + ((UINT32)(((UINT64)(VA) >> 39) & 0x1FF)))
#define MiGetPpeAddress(BASE, VA) ((UINT64)(((((UINT64)VA & 0xFFFFFFFFFFFF) >> 30) << 3) + BASE))
#define MiGetPdeAddress(BASE, VA) ((UINT64)(((((UINT64)VA & 0xFFFFFFFFFFFF) >> 21) << 3) + BASE))
#define MiGetPteAddress(BASE, VA) ((UINT64)(((((UINT64)VA & 0xFFFFFFFFFFFF) >> 12) << 3) + BASE))

static void SetPoolTag(UINT32 newTag);
static UINT32 GetPoolTag();
static PVOID AllocateMemory(SIZE_T NumberOfBytes);
static void FreeMemory(PVOID add);
static PVOID GetVA(UINT64 pa);
static NTSTATUS GetPteBase(PUINT8 pBuffer[]);
static NTSTATUS AccessPhysicalMemory(void* PhysicalAddress, void* bufferAddress, UINT64 size, int isWrite);
static NTSTATUS CopyData(PVOID desAdd, PVOID souAdd, UINT64 size);
static UINT64 GetPA(PVOID Vadd);



//内存池
static PUINT8 g_hvMMpool = 0;
//内存池物理地址
static UINT64 g_hvMMpool_pa = 0;
//内存池已用数量
static volatile INT64 g_hvMMpoolUsable = 0;
static const UINT64 g_hvMMpoolSize = MB_TO_BYTE(10) ;	 //设置池大小
static volatile INT64 g_hvMMpoolBigMemEnd = g_hvMMpoolSize;

//static PUINT8 g_guest_pte_base[4];

bool ALhvMMinitPool()
{
	PHYSICAL_ADDRESS maxAddress = { 0 };
	maxAddress.QuadPart = -1;
	g_hvMMpool = (PUCHAR)MmAllocateContiguousMemory( g_hvMMpoolSize, maxAddress);
	UINT64 time = __rdtsc();
	SetPoolTag(time & ((1LL << 32) - 1));
	if (g_hvMMpool)
	{
		memset(g_hvMMpool, 0, g_hvMMpoolSize);
		g_hvMMpool_pa = (UINT64)GetPA(g_hvMMpool);
		/*auto ret = GetPteBase(g_guest_pte_base);
		if (ret)
		{
			ALhvSetErr("获取pte_base失败");
			return 0;
		}*/
		if (g_hvMMpool_pa)
			return 1;
		else
		{
			ALhvSetErr("获取物理地址失败");
			return 0;
		}
	}
	else
	{
		ALhvSetErr("申请内存失败");
		return 0;
	}
}
PUINT8 ALhvMMallocateMemory(UINT64 sizeByByte)
{
	if (!g_hvMMpool)
	{
		ALhvSetErr("vmMM未初始化");
		return 0;
	}
	if (sizeByByte >= 0x1000)//如果大于一个页,按页申请
	{
		auto a_size = ALIGNS_PAGE(sizeByByte);
		auto oldOff = InterlockedExchangeAdd64(&g_hvMMpoolUsable, a_size);
		if (oldOff + a_size <= g_hvMMpoolBigMemEnd)
			return &g_hvMMpool[oldOff];
		else
		{
			ALhvKill("池内存不足", 0);
			//ALhvSetErr("池内存不足");
			//return 0;
		}
	}
	else
	{
		sizeByByte = -sizeByByte;
		auto oldOff = InterlockedExchangeAdd64(&g_hvMMpoolBigMemEnd, sizeByByte);

		if (g_hvMMpoolUsable <= oldOff + sizeByByte)
		{
			return &g_hvMMpool[oldOff + sizeByByte];
		}
		else
		{
			ALhvKill("池内存不足", 0);
		}

	}
}
static PUINT8 hvMMpMemBase = 0;
PUINT8 ALhvMMgetPoolBase()
{
	return hvMMpMemBase;
}
bool ALhvMMsetAllPA(pml4e_64* page_table)
{
	//映射所有物理内存
	for (UINT64 z = 256; z < 512; z++)
	{
		if (page_table[z].flags == 0)
		{
			auto PML4 = (pdpte_64*)ALhvMMallocateMemory(0x1000);
			pml4e_64 pml4e;
			pml4e.flags = 0;
			pml4e.present = 1;
			pml4e.write = 1;
			pml4e.page_frame_number = MmGetPhysicalAddress(PML4).QuadPart >> 12;
			page_table[z].flags = pml4e.flags;
			for (uint64_t i = 0; i < 64; ++i) {
				auto PDPT = (pde_2mb_64*)ALhvMMallocateMemory(0x1000);
				auto& pdpte = PML4[i];
				pdpte.flags = 0;
				pdpte.present = 1;
				pdpte.write = 1;
				pdpte.page_frame_number = MmGetPhysicalAddress(PDPT).QuadPart >> 12;

				for (uint64_t j = 0; j < 512; ++j) {
					auto& pde = PDPT[j];
					pde.flags = 0;
					pde.present = 1;
					pde.write = 1;
					pde.large_page = 1;
					pde.page_frame_number = (i << 9) + j;
				}
			}
			OR_ADDRESS add = { 0 };
			add.pml4_index = z;
			add.reserved = 0xffff;

			hvMMpMemBase = (PUINT8)add.value;
			//ALdbgPutValue(gALvmMMpMemBase);
			return 1;
		}
	}
	ALhvSetErr("未找到合适的映射地址");
	return 0;
}

//
//pml4e_64* ALhvMMgetPML4E(PVOID add)
//{
//	if (g_guest_pte_base[0] == 0 || g_guest_pte_base[1] == 0 || g_guest_pte_base[2] == 0 || g_guest_pte_base[3] == 0)
//	{
//		ALhvSetErr("未初始化pte_base") ;
//		return 0;
//	}
//	pml4e_64* pml4e_p = (GT(pml4e_p)) MiGetPxeAddress(g_guest_pte_base[3], add);
//	return pml4e_p;
//
//}
//pdpte_64* ALhvMMgetPDPTE(PVOID add)
//{
//	auto pml4e_p = ALhvMMgetPML4E(add);
//	if (pml4e_p == 0)
//	{
//		ALhvAddErr("获取PML4E失败");
//		return 0;
//	}
//	if (pml4e_p->present == 0)
//	{
//		ALhvSetErr("无效PML4E");
//		return 0;
//	}
//	pdpte_64* pdpte_p = (GT(pdpte_p))MiGetPpeAddress(g_guest_pte_base[2], add);
//	return pdpte_p;
//}
//pde_64* ALhvMMgetPDE(PVOID add)
//{
//	auto pdpte_p = ALhvMMgetPDPTE(add);
//	if (pdpte_p == 0)
//	{
//		ALhvAddErr("获取PDPTE失败");
//		return 0;
//	}
//	if (pdpte_p->present == 0)
//	{
//		ALhvSetErr("无效PDPTE");
//		return 0;
//	}
//	pde_64* pde_p = (GT(pde_p))MiGetPdeAddress(g_guest_pte_base[1], add);
//	return pde_p;
//}
//pte_64* ALhvMMgetPTE(PVOID add)
//{
//	auto pde_p = ALhvMMgetPDE(add);
//	if (pde_p == 0)
//	{
//		ALhvAddErr("获取PDE失败");
//		return 0;
//	}
//	if (pde_p->present == 0)
//	{
//		ALhvSetErr("无效PDE");
//		return 0;
//	}
//	pte_64* pte_p = (GT(pte_p))MiGetPteAddress(g_guest_pte_base[0], add);
//	return pte_p;
//}
bool ALhvMMaccessPhysicalMemory(UINT64 PhysicalAddress, void* bufferAddress, UINT64 size, int isWrite)
{
	if (ALhvIsHost())
	{
		if (ALhvMMgetPoolBase())
		{
			if (isWrite)
				CopyData(&ALhvMMgetPoolBase()[PhysicalAddress], bufferAddress, size);
			else
				CopyData(bufferAddress, &ALhvMMgetPoolBase()[PhysicalAddress], size);
			return 1;
		}
		ALhvSetErr("未知错误");
		return 0;
	}
	else
	{
		//尝试直接获取虚拟地址
		if (size <= 0x1000 && (((PhysicalAddress + size) & ~0xfffLL) == (PhysicalAddress & ~0xfffLL)))
		{
			



		}

		auto ret = AccessPhysicalMemory((PVOID)PhysicalAddress, bufferAddress, size, isWrite);
		if (ret)
		{
			ALhvSetErr("读取物理内存失败(%d)", ret);
			return 0;
		}
		return 1;
	}
}

bool ALhvMMreadPM(UINT64 pa, PVOID buff, UINT64 size)
{
	return ALhvMMaccessPhysicalMemory(pa, buff, size, 0);
}
 
bool ALhvMMwritePM(UINT64 pa, PVOID buff, UINT64 size)
{
	return ALhvMMaccessPhysicalMemory(pa, buff, size, 1);
}
pml4e_64 ALhvMMgetPML4E(pml4e_64* page_table, PVOID vadd)
{
	if (page_table == 0)
		return { 0 };
	OR_ADDRESS mapAddress; mapAddress.value = (UINT64)vadd;
	return page_table[mapAddress.pml4_index];
}		  
bool ALhvMMsetPML4E(pml4e_64* page_table, PVOID vadd, pml4e_64 v)
{
	if (page_table == 0)
	{
		ALhvSetErr(" ");
		return { 0 };
	}
	OR_ADDRESS mapAddress; mapAddress.value = (UINT64)vadd;
	page_table[mapAddress.pml4_index] = v;
	return 1;
}
pdpte_64 ALhvMMgetPDPTE(pml4e_64* page_table, PVOID vadd)
{
	OR_ADDRESS mapAddress; mapAddress.value = (UINT64)vadd;
	auto pml4e = ALhvMMgetPML4E(page_table, vadd);
	if (!pml4e.present)
		return { 0 };
	auto pdpt_pa = pml4e.page_frame_number << 12;
	pdpte_64 pdpte = { 0 };
	ALhvMMreadPM(pdpt_pa + mapAddress.pdpt_index * 8, &pdpte, sizeof(pdpte));
	return pdpte;
}	   
bool ALhvMMsetPDPTE(pml4e_64* page_table, PVOID vadd, pdpte_64 v)
{
	OR_ADDRESS mapAddress; mapAddress.value = (UINT64)vadd;
	auto pml4e = ALhvMMgetPML4E(page_table, vadd);
	if (!pml4e.present)
	{
		ALhvSetErr(" ");
		return { 0 };
	}
	auto pdpt_pa = pml4e.page_frame_number << 12;
	ALhvMMwritePM(pdpt_pa + mapAddress.pdpt_index * 8, &v, sizeof(pdpte_64));
	return 1;
}
pde_64 ALhvMMgetPDE(pml4e_64* page_table, PVOID vadd)
{
	OR_ADDRESS mapAddress; mapAddress.value = (UINT64)vadd;
	auto pdpte = ALhvMMgetPDPTE(page_table, vadd);
	if (!pdpte.present)
		return { 0 };
	auto pdt_pa = pdpte.page_frame_number << 12;
	pde_64 pde = { 0 };
	ALhvMMreadPM(pdt_pa + mapAddress.pd_index * 8, &pde, sizeof(pde));
	return pde;
}
bool ALhvMMsetPDE(pml4e_64* page_table, PVOID vadd, pde_64 v)
{
	OR_ADDRESS mapAddress; mapAddress.value = (UINT64)vadd;
	auto pdpte = ALhvMMgetPDPTE(page_table, vadd);
	if (!pdpte.present)
	{
		ALhvSetErr(" ");
		return { 0 };
	}
	auto pdt_pa = pdpte.page_frame_number << 12;
	ALhvMMwritePM(pdt_pa + mapAddress.pd_index * 8, &v, sizeof(v));
	return 1;
}
pte_64 ALhvMMgetPTE(pml4e_64* page_table, PVOID vadd)
{
	OR_ADDRESS mapAddress; mapAddress.value = (UINT64)vadd;
	auto pde = ALhvMMgetPDE(page_table, vadd);
	if (!pde.present)
		return { 0 };
	auto ptt_pa = pde.page_frame_number << 12;
	pte_64 pte = { 0 };
	ALhvMMreadPM(ptt_pa + mapAddress.pt_index * 8, &pte, sizeof(pte));
	return pte;
}
bool ALhvMMsetPTE(pml4e_64* page_table, PVOID vadd, pte_64 v)
{
	OR_ADDRESS mapAddress; mapAddress.value = (UINT64)vadd;
	auto pde = ALhvMMgetPDE(page_table, vadd);
	if (!pde.present)
	{
		ALhvSetErr(" ");
		return { 0 };
	}
	auto ptt_pa = pde.page_frame_number << 12;
	ALhvMMwritePM(ptt_pa + mapAddress.pt_index * 8, &v, sizeof(v));
	return 1;
}
PVOID ALhvMMgetVA(UINT64 pa)
{
	if (ALhvIsHost())
	{
		return 	&ALhvMMgetPoolBase()[pa];
	}
	else
	{
		return GetVA(pa);
	}
}
UINT64 ALhvMMgetPA(PVOID add)
{
	if ((UINT64)add >= (UINT64)g_hvMMpool && (UINT64)add < (UINT64)g_hvMMpool + g_hvMMpoolSize)
		return g_hvMMpool_pa + ((UINT64)add - (UINT64)g_hvMMpool);
	if (ALhvIsHost())
	{
		cr3 cr3_v = { 0 };
		cr3_v.flags = __readcr3();
		auto cr3_va = (pml4e_64*)ALhvMMgetVA(cr3_v.address_of_page_directory << 12);
		auto pde = ALhvMMgetPDE(cr3_va, add);
		if (!pde.present)
			return 0;
		if (pde.large_page)
		{
			pde_2mb_64 pde_2m = { 0 };
			pde_2m.flags = pde.flags;
			return (pde_2m.page_frame_number << 21) + ((UINT64)add & 0x1fffff);
		}
		else
		{
			auto pte = ALhvMMgetPTE(cr3_va, add);
			if(!pte.present)
				return 0;
			else
			{
				return (pte.page_frame_number << 12) + _Gpg_off(add);
			}
		}
	}
	else
	{
		return GetPA(add);
	}

}


bool ALhvMMrebuildPath(pml4e_64* page_table_old, pml4e_64* page_table, PVOID vadd)
{
	OR_ADDRESS mapAddress; mapAddress.value = (UINT64)vadd;

	//auto addOfPml4e_p = page_table[mapAddress.pml4_index];
	auto pml4e = page_table[mapAddress.pml4_index];
	auto pml4e_old = page_table_old[mapAddress.pml4_index];
	//判断虚拟地址是否有效
	if (pml4e.present == 0 || pml4e_old.present == 0)
	{
		ALhvSetErr("无效虚拟地址");
		return 0;
	}
	pdpte_64* PDPT_new = 0;
	if (pml4e.page_frame_number == pml4e_old.page_frame_number)	 
	{
		auto pdpt_pa = pml4e.page_frame_number << 12;
		PDPT_new = (GT(PDPT_new))ALhvMMallocateMemory(0x1000);
		ALhvMMreadPM(pdpt_pa, PDPT_new, 0x1000);
	}

	auto pdpte_old = ALhvMMgetPDPTE(page_table_old, vadd);
	auto pdpte = ALhvMMgetPDPTE(page_table, vadd);
	if (!pdpte_old.present || !pdpte.present)
	{
		ALhvSetErr("无效PDPTE");
		return 0;
	}
	pde_64* PDT_new = 0;
	if (pdpte.page_frame_number == pdpte_old.page_frame_number)
	{
		auto pdt_pa = pdpte.page_frame_number << 12;
		PDT_new = (GT(PDT_new))ALhvMMallocateMemory(0x1000);
		ALhvMMreadPM(pdt_pa, PDT_new, 0x1000);
	}

	auto pde_old = ALhvMMgetPDE(page_table_old, vadd);
	auto pde = ALhvMMgetPDE(page_table, vadd);
	if (!pde_old.present || !pde.present)
	{
		ALhvSetErr("无效PDE");
		return 0;
	}
	pte_64* PT_new = 0;
	if (!pde.large_page)	   //非2m页面
	{
		if (pde_old.large_page)
		{
			ALhvSetErr("未知错误");
			return 0;
		}
		if (pde.page_frame_number == pde_old.page_frame_number)
		{
			auto pt_pa = pde_old.page_frame_number << 12;
			PT_new = (GT(PT_new))ALhvMMallocateMemory(0x1000);
			ALhvMMreadPM(pt_pa, PT_new, 0x1000);
			if (PT_new[mapAddress.pt_index].present == 0)
			{
				ALhvSetErr("无效PTE");
				return 0;
			}
		}
	}
	if (PT_new)
	{
		pde.page_frame_number = ALhvMMgetPA(PT_new) >> 12;
		ALhvMMsetPDE(page_table, vadd, pde);
	}
	if (PDT_new)		  //有值说明需要更换
	{
		pdpte.page_frame_number = ALhvMMgetPA(PDT_new) >> 12;
		ALhvMMsetPDPTE(page_table, vadd, pdpte);
	}
	if (PDPT_new)												  //有值说明是新的
		page_table[mapAddress.pml4_index].page_frame_number = ALhvMMgetPA(PDPT_new) >> 12;

	return 1;

}
bool ALhvMMrebuildPath(pml4e_64* page_table_old, pml4e_64* page_table, PVOID vadd, UINT64 size)
{
	size = (size + 0xfff) & ~0xfff;
	for (UINT64 i = (UINT64)vadd; i < size; i += 0x1000)
	{
		auto ret= ALhvMMrebuildPath(page_table_old, page_table, (PVOID)i);
		if (!ret)
		{
			ALhvAddErr("重建映射路径第%d页失败", ((i - (UINT64)vadd) >> 12) + 1);
			return 0;
		}
	}
	return 1;
}
bool ALhvMMrebuildAlllPath(pml4e_64* page_table_old, pml4e_64* page_table)		 //贼鸡儿慢不行
{
	OR_ADDRESS add = { 0 };
	add.reserved = 0xffff;

	for (int i = 256; i < 512; i++)	 //pml4e
	{
		if (page_table[i].page_frame_number == ALhvMMgetPA(page_table) >> 12)		   //自映射不复制
			continue;
		if (page_table_old[i].present != 1 || page_table_old[i].write != 1 || page_table_old[i].page_frame_number == 0)
			continue;
		add.pml4_index = i;

		auto pml4e = page_table_old[i];
		auto pdpt_pa = pml4e.page_frame_number << 12;
		pdpte_64* PDPT_new = (GT(PDPT_new))ALhvMMallocateMemory(0x1000);
		ALhvMMreadPM(pdpt_pa, PDPT_new, 0x1000);
		page_table[i].page_frame_number = ALhvMMgetPA(PDPT_new) >> 12;

		for (int j = 0; j < 512; j++)  //pdpte
		{
			add.pdpt_index = j;
			auto pdpte = ALhvMMgetPDPTE(page_table_old, add.pointer);
			if (pdpte.present != 1 || pdpte.large_page || pdpte.write != 1 || pdpte.page_frame_number == 0)
				continue;

			pde_64* PDT_new = 0;
			auto pdt_pa = pdpte.page_frame_number << 12;
			PDT_new = (GT(PDT_new))ALhvMMallocateMemory(0x1000);
			ALhvMMreadPM(pdt_pa, PDT_new, 0x1000);
			pdpte.page_frame_number = ALhvMMgetPA(PDT_new) >> 12;
			ALhvMMsetPDPTE(page_table, add.pointer, pdpte);

			for (int k = 0; k < 512; k++)  //pdt
			{
				add.pd_index = k;
				auto pde = ALhvMMgetPDE(page_table_old, add.pointer);
				if (pde.present != 1 || pde.large_page || pdpte.write != 1 || pdpte.page_frame_number == 0)
					continue;

				pte_64* PT_new = 0;
				auto pt_pa = pde.page_frame_number << 12;
				PT_new = (GT(PT_new))ALhvMMallocateMemory(0x1000);
				ALhvMMreadPM(pt_pa, PT_new, 0x1000);
				pde.page_frame_number = ALhvMMgetPA(PT_new) >> 12;
				ALhvMMsetPDE(page_table, add.pointer, pde);


				//DbgPrintEx(0, 0, "重新构造映射路径%p\n", add.value);
			}
		}
	}
	return 1;
}
UINT64 ALhvMMgetPoolSize()
{
	return g_hvMMpoolSize;
}
bool ALhvMMrebuildMemPoolPath(pml4e_64* page_table_old, pml4e_64* page_table)
{
	return ALhvMMrebuildPath(page_table_old, page_table, ALhvMMgetPoolBase(), ALhvMMgetPoolSize());
}
cr3 ALhvMMgetCr3ByObject(PEPROCESS process)
{
	KAPC_STATE pr;
	KeStackAttachProcess(process, &pr);
	cr3 a = { 0 };
	a.flags = __readcr3();
	KeUnstackDetachProcess(&pr);
	return a;
}

static UINT32 memPoolTag = 'OmrI';
static void SetPoolTag(UINT32 newTag)
{
	memPoolTag = newTag;
}
static UINT32 GetPoolTag()
{
	return 	  memPoolTag;
}
static PVOID AllocateMemory(SIZE_T NumberOfBytes)
{
	PUINT64 a = (PUINT64)ExAllocatePoolWithTag(NonPagedPool, NumberOfBytes, memPoolTag);
	if (a)
		memset(a, 0, NumberOfBytes);
	return a;
}
static void FreeMemory(PVOID add)
{
	ExFreePoolWithTag(add, memPoolTag);
}
static PVOID GetVA(UINT64 pa)
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

static NTSTATUS GetPteBase(PUINT8 pBuffer[]) 
{

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


static NTSTATUS AccessPhysicalMemory(void* PhysicalAddress, void* bufferAddress, UINT64 size, int isWrite)
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

static NTSTATUS CopyData(PVOID desAdd, PVOID souAdd, UINT64 size)
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
static UINT64 GetPA(PVOID Vadd)
{
	return MmGetPhysicalAddress(Vadd).QuadPart;
}

//浅映射复制cr3
//需要确保送入的cr3参数是真实可用的cr3
bool ALhvMMprepare_fake_page_tables(__in cr3 real_cr3, __out pml4e_64** fake_tabble)
{
	if (!fake_tabble)
	{
		ALhvSetErr("参数错误");
		return 0;
	}
	*fake_tabble = 0;
	//复制system cr3 的 pml4后256个项,之后重新构建自身,idt,gdt的映射路径,还要修复自映射
	auto Hostpml4 = (pml4e_64*)ALhvMMallocateMemory(0x1000);

	PHYSICAL_ADDRESS pml4_address;
	pml4_address.QuadPart = real_cr3.address_of_page_directory << 12;
	auto pt = static_cast<pml4e_64*>(ALhvMMgetVA(pml4_address.QuadPart));
	if (pt == 0)
	{
		ALhvSetErr("获取真实cr3虚拟地址失败");
		return 0;
	}

	// kernel PML4 address
	auto const guest_pml4 = pt;
	memcpy(&Hostpml4[256], &guest_pml4[256], sizeof(pml4e_64) * 256);

	if (!ALhvMMsetAllPA(Hostpml4))
	{
		ALhvAddErr("映射所有物理内存失败");
		return 0;
	}


	auto Hostpml4_pa = ALhvMMgetPA(Hostpml4);
	if (!Hostpml4_pa)
	{
		ALhvSetErr("获取Hostpml4_pa失败");
		return 0;
	}
	//修复自映射
	auto fixselfmap = [guest_pml4, Hostpml4, Hostpml4_pa, real_cr3]()->bool {
		for (int i = 255; i < 512; i++)
		{
			if (guest_pml4[i].page_frame_number == real_cr3.address_of_page_directory)
			{
				Hostpml4[i].page_frame_number = Hostpml4_pa >> 12;
				return 1;
			}
		}
		return 0;
	};

	if (fixselfmap() == 0)
	{
		ALhvSetErr("修复自映射失败");
		return 0;
	}



	auto guestcr3 = real_cr3;
	/*auto hostcr3 = guestcr3;
	hostcr3.address_of_page_directory = Hostpml4_pa >> 12;*/

	*fake_tabble = Hostpml4;
	return 1;
}

bool ALhvMMgetHostCr3(__in cr3 system_cr3, __out cr3* host_cr3_out)
{
	if (!host_cr3_out)
	{
		ALhvSetErr("参数错误");
		return 0;
	}
	*host_cr3_out = {};
	auto a = (UINT64)(ALhvMMgetHostCr3);
	a = _2pg(a);
	while (*(PUINT16)a != (UINT16)'ZM')
		a -= 0x1000;
	PIMAGE_DOS_HEADER DOS = (PIMAGE_DOS_HEADER)a;									   //设置DOS头
	PIMAGE_NT_HEADERS NT = (PIMAGE_NT_HEADERS)(DOS->e_lfanew + (PUCHAR)a);			   //设置NT头
	PIMAGE_NT_HEADERS pNTHeader = (PIMAGE_NT_HEADERS)NT; // PE头
	PIMAGE_SECTION_HEADER pSectionHeader = (PIMAGE_SECTION_HEADER)((ULONG64)pNTHeader + sizeof(IMAGE_NT_HEADERS));
	int nAlign = pNTHeader->OptionalHeader.SectionAlignment; //段对齐字节数
	//// 计算所有头的尺寸。包括dos, coff, pe头 和 段表的大小
	int ImageSize = (pNTHeader->OptionalHeader.SizeOfHeaders + nAlign - 1) / nAlign * nAlign;
	// 计算所有节的大小
	for (int i = 0; i < pNTHeader->FileHeader.NumberOfSections; ++i)
	{
		//得到该节的大小
		int CodeSize = pSectionHeader[i].Misc.VirtualSize;
		int LoadSize = pSectionHeader[i].SizeOfRawData;
		int MaxSize = (LoadSize > CodeSize) ? (LoadSize) : (CodeSize);

		int SectionSize = (pSectionHeader[i].VirtualAddress + MaxSize + nAlign - 1) / nAlign * nAlign;
		if (ImageSize < SectionSize)
			ImageSize = SectionSize; //Use the Max;
	}

	auto driver_base = (PVOID)a;
	auto driver_size = ImageSize;


	auto sys_cr3 = system_cr3;
	//GT(sys_cr3) host_cr3 = {};
	pml4e_64* host_table = 0;
	auto ret = ALhvMMprepare_fake_page_tables(sys_cr3, &host_table);
	if (ret == 0)
	{
		ALhvAddErr("构建host页表失败");
		return 0;
	}

	auto Hostpml4_pa = ALhvMMgetPA(host_table);
	if (!Hostpml4_pa)
	{
		ALhvSetErr("获取Hostpml4_pa失败");
		return 0;
	}
	auto hostcr3 = system_cr3;
	hostcr3.address_of_page_directory = Hostpml4_pa >> 12;

	PHYSICAL_ADDRESS pml4_address;
	pml4_address.QuadPart = sys_cr3.address_of_page_directory << 12;
	auto pt = static_cast<pml4e_64*>(ALhvMMgetVA(pml4_address.QuadPart));
	if (pt == 0)
	{
		ALhvSetErr("获取sys cr3虚拟地址失败");
		return 0;
	}

	ret = ALhvMMrebuildPath(pt, host_table, driver_base, driver_size); //重新构建驱动映射路径
	if (!ret)
	{
		ALhvAddErr("重新构建驱动映射路径失败");
		return 0;
	}

	ret = ALhvMMrebuildMemPoolPath(pt, host_table);
	if (!ret)
	{
		ALhvAddErr("重新构建内存池路径失败");
		return 0;
	}
	*host_cr3_out = hostcr3;
	return 1;

}

struct mtrr_data {
	ia32_mtrr_capabilities_register cap;
	ia32_mtrr_def_type_register def_type;

	// fixed-range MTRRs
	struct {
		// TODO: implement
	} fixed;

	// variable-range MTRRs
	struct {
		ia32_mtrr_physbase_register base;
		ia32_mtrr_physmask_register mask;
	} variable[64];

	// number of valid variable-range MTRRs
	size_t var_count;
};
// read MTRR data into a single structure
static mtrr_data read_mtrr_data() {
	mtrr_data mtrrs;

	mtrrs.cap.flags = __readmsr(IA32_MTRR_CAPABILITIES);
	mtrrs.def_type.flags = __readmsr(IA32_MTRR_DEF_TYPE);
	mtrrs.var_count = 0;

	for (uint32_t i = 0; i < mtrrs.cap.variable_range_count; ++i) {
		ia32_mtrr_physmask_register mask;
		mask.flags = __readmsr(IA32_MTRR_PHYSMASK0 + i * 2);

		if (!mask.valid)
			continue;

		mtrrs.variable[mtrrs.var_count].mask = mask;
		mtrrs.variable[mtrrs.var_count].base.flags =
			__readmsr(IA32_MTRR_PHYSBASE0 + i * 2);

		++mtrrs.var_count;
	}

	return mtrrs;
}

// calculate the MTRR memory type for a single page
static uint8_t calc_mtrr_mem_type(mtrr_data const& mtrrs, uint64_t const pfn) {
	if (!mtrrs.def_type.mtrr_enable)
		return MEMORY_TYPE_UNCACHEABLE;

	// fixed range MTRRs
	if (pfn < 0x100 && mtrrs.cap.fixed_range_supported
		&& mtrrs.def_type.fixed_range_mtrr_enable) {
		// TODO: implement this
		return MEMORY_TYPE_UNCACHEABLE;
	}

	uint8_t curr_mem_type = MEMORY_TYPE_INVALID;

	// variable-range MTRRs
	for (uint32_t i = 0; i < mtrrs.var_count; ++i) {
		auto const base = mtrrs.variable[i].base.page_frame_number;
		auto const mask = mtrrs.variable[i].mask.page_frame_number;

		// 3.11.11.2.3
		// essentially checking if the top part of the address (as specified
		// by the PHYSMASK) is equal to the top part of the PHYSBASE.
		if ((pfn & mask) == (base & mask)) {
			auto const type = static_cast<uint8_t>(mtrrs.variable[i].base.type);

			// UC takes precedence over everything
			if (type == MEMORY_TYPE_UNCACHEABLE)
				return MEMORY_TYPE_UNCACHEABLE;

			// this works for WT and WB, which is the only other "defined" overlap scenario
			if (type < curr_mem_type)
				curr_mem_type = type;
		}
	}

	// no MTRR covers the specified address
	if (curr_mem_type == MEMORY_TYPE_INVALID)
		return mtrrs.def_type.default_memory_type;

	return curr_mem_type;
}

// calculate the MTRR memory type for the given physical memory range
static uint8_t calc_mtrr_mem_type(mtrr_data const& mtrrs, uint64_t address, uint64_t size) {
	// base address must be on atleast a 4KB boundary
	address &= ~0xFFFull;

	// minimum range size is 4KB
	size = (size + 0xFFF) & ~0xFFFull;

	uint8_t curr_mem_type = MEMORY_TYPE_INVALID;

	for (uint64_t curr = address; curr < address + size; curr += 0x1000) {
		auto const type = calc_mtrr_mem_type(mtrrs, curr >> 12);

		if (type == MEMORY_TYPE_UNCACHEABLE)
			return type;

		// use the worse memory type between the two
		if (type < curr_mem_type)
			curr_mem_type = type;
	}

	if (curr_mem_type == MEMORY_TYPE_INVALID)
		return MEMORY_TYPE_UNCACHEABLE;

	return curr_mem_type;
}
UINT8 ALhvMMgetMemoryType(UINT64 address, UINT64 size, bool update)
{
	static mtrr_data mtrrs = read_mtrr_data();
	if (update)
	{
		mtrrs = read_mtrr_data();
		return 0;
	}

	/*if (address < MB_TO_BYTE(1))
		return MEMORY_TYPE_UNCACHEABLE;*/
	return calc_mtrr_mem_type(mtrrs, address, size);
}




