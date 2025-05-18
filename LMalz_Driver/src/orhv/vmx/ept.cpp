//#include <intrin.h>
//#include <wdm.h>
//#include "../orhv.h"
//#include "vmx.h"
#include "ept.h"
// 1 root, 2 pml4e, 3 pdpte,4 pde 5 pte
static UINT64 ALvmxEPTgetPageTableInfo(UINT64 GPA, UINT8 layerNumber, ept_pml4e* ept)
{
	if (layerNumber > 5)
	{
		ALhvSetErr("无效 参数");
		return 0;
	}
	OR_ADDRESS addIndex = { 0 };   addIndex.value = GPA;
	if (ept)
	{
		if (layerNumber == 1)
			return (UINT64)ept;
		ept_pml4e& pml4e = ept[addIndex.pml4_index];
		if (pml4e.page_frame_number)
		{
			if (layerNumber == 2)
				return pml4e.flags;
			auto pdpte = (ept_pml4e*)ALhvMMgetVA((pml4e.page_frame_number << 12));
			pdpte += addIndex.pdpt_index;
			if (layerNumber == 3)
				return pdpte->flags;
			if (pdpte->page_frame_number)
			{
				auto pPde = (ept_pde_2mb*)ALhvMMgetVA((pdpte->page_frame_number << 12));
				pPde += addIndex.pd_index;
				if (layerNumber == 4)
					return pPde->flags;
				if (pPde->large_page)
					return 0;
				ept_pde pde = { 0 }; pde.flags = pPde->flags;
				if (pde.page_frame_number)
				{
					//static auto save = ALvmMMallocateMemory(0x1000);
					//ALvmMMsaveSelfMapPte(save);
					auto pPte = (ept_pte*)ALhvMMgetVA((pde.page_frame_number << 12));
					pPte += addIndex.pt_index;
					return pPte->flags;
				}
				else
				{
					ALhvSetErr("无效 pde");
					return 0;
				}
			}
			else
			{
				ALhvSetErr("无效 pdpte");
				return 0;
			}
		}
		else
		{
			ALhvSetErr("无效 pml4e");
			return 0;
		}
	}
	else
	{
		ALhvSetErr("not have ept root");
		return 0;
	}
}
static UINT64 ALvmxEPTbuildPDE(UINT64 GPA, ept_pte pte_demo)
{
	GPA = GPA & ~(MB_TO_BYTE(2) - 1);
	ept_pde pde = { 0 };
	auto nPdePage = (ept_pte*)ALhvMMallocateMemory(0x1000);
	auto nPdePage_pa = ALhvMMgetPA(nPdePage);
	if (nPdePage_pa)
	{
		pde.read_access = 1;
		pde.write_access = 1;
		pde.execute_access = 1;
		pde.page_frame_number = nPdePage_pa >> 12;
		for (UINT64 i = 0; i < 512; i++)
		{
			nPdePage[i].read_access = pte_demo.read_access;
			nPdePage[i].write_access = pte_demo.write_access;
			nPdePage[i].execute_access = pte_demo.execute_access;
			nPdePage[i].page_frame_number = (GPA >> 12) + i;
			nPdePage[i].memory_type = ALhvMMgetMemoryType(GPA + i * 0x1000, 0x1000, 0);
		}
		return 	pde.flags;
	}
	else
	{
		ALhvAddErr("获取物理地址失败");
		return { 0 };
	}
}
static bool ALvmxEPTsetPDE(UINT64 GPA, UINT64 pde, ept_pml4e* ept)					 //一级二级页表属性给全无需设置demo
{
	if (ept)
	{
		OR_ADDRESS addIndex = { 0 };   addIndex.value = GPA;
		ept_pml4e& pml4e = ept[addIndex.pml4_index];
		if (pml4e.page_frame_number)
		{
			auto pdpte = (ept_pdpte*)ALhvMMgetVA((pml4e.page_frame_number << 12));
			if (pdpte)
			{
				pdpte += addIndex.pdpt_index;
				if (pdpte->page_frame_number)
				{
					auto pPde = (ept_pde*)ALhvMMgetVA((pdpte->page_frame_number << 12));
					if (pPde)
					{
						pPde += addIndex.pd_index;
						pPde->flags = pde;
						return 1;
					}
					else
					{
						ALhvAddErr("获取PDT错误");
						return 0;
					}
				}
				else
				{
					auto newPdePage = (ept_pdpte*)ALhvMMallocateMemory(0x1000);
					auto nPdePage_pa = ALhvMMgetPA(newPdePage);
					if (nPdePage_pa)
					{
						pdpte->flags = 0;
						pdpte->read_access = 1;
						pdpte->write_access = 1;
						pdpte->execute_access = 1;
						pdpte->page_frame_number = nPdePage_pa >> 12;
					}
					else
					{
						ALhvAddErr("获取物理地址失败", 0);
						return 0;
					}

					return ALvmxEPTsetPDE(GPA, pde, ept);
				}
			}
			else
			{
				ALhvAddErr("获取PDPT错误");
				return 0;
			}
		}
		else
		{
			auto newPdptePage = (ept_pdpte*)ALhvMMallocateMemory(0x1000);
			auto nPdptePage_pa = ALhvMMgetPA(newPdptePage);
			if (nPdptePage_pa)
			{
				pml4e.flags = 0;
				pml4e.read_access = 1;
				pml4e.write_access = 1;
				pml4e.execute_access = 1;
				pml4e.page_frame_number = nPdptePage_pa >> 12;
			}
			else
			{
				ALhvAddErr("获取物理地址失败", 0);
				return 0;
			}
			return ALvmxEPTsetPDE(GPA, pde, ept);
		}
	}
	else
	{
		ALhvAddErr("not have ept root");
		return 0;
	}
}
static bool ALvmxEPTsetPTE(UINT64 GPA, UINT64 pte, ept_pml4e* ept)		  //
{
	ept_pde pde_r = { 0 };
	pde_r.flags = ALvmxEPTgetPageTableInfo(GPA, 4, ept);
	if (pde_r.execute_access || pde_r.read_access || pde_r.write_access)
	{
		ept_pde_2mb pde_2m = { 0 }; pde_2m.flags = pde_r.flags;
		if (pde_2m.large_page)
		{
			//继承属性构建pdet
			ept_pte pte_demo = { 0 };
			pte_demo.read_access = pde_2m.read_access;
			pte_demo.write_access = pde_2m.write_access;
			pte_demo.execute_access = pde_2m.execute_access;
			auto pde = ALvmxEPTbuildPDE(GPA, pte_demo);					   //重新构建全属性pde
			if (pde)
			{
				ALvmxEPTsetPDE(GPA, pde, ept);
				return ALvmxEPTsetPTE(GPA, pte, ept);
			}
			else
				ALhvAddErr("构建pde失败");

		}
		OR_ADDRESS addIndex = { 0 };   addIndex.value = GPA;
		auto pPte = (ept_pte*)ALhvMMgetVA((pde_r.page_frame_number << 12));
		if (pPte)
		{
			pPte += addIndex.pt_index;
			pPte->flags = pte;
			return 1;
		}
		else
		{
			ALhvAddErr("获取虚拟地址失败");
			return 0;
		}

	}
	else															  //无效pde克隆要设置的pte属性构造pdet
	{
		ept_pte pte_demo = { 0 };
		pte_demo.flags = pte;
		auto pde = ALvmxEPTbuildPDE(GPA, pte_demo);
		if (pde)
		{
			ALvmxEPTsetPDE(GPA, pde, ept);
			return ALvmxEPTsetPTE(GPA, pte, ept);
		}
		else
		{
			ALhvAddErr("构建pde失败");
			return 0;
		}
	}
}


static bool ALvmxEPTnewTable(__in ept_pde_2mb pde_demo, __in UINT64 size, __out OR_PTR<ept_pml4e>* o_eptp)
{
	o_eptp = {};
	if (!o_eptp)
	{
		ALhvSetErr("参数错误");
		return 0;
	}
	
	auto ept_pml4_mem = (ept_pml4e*)ALhvMMallocateMemory(0x1000);
	auto ept_pml4_mem_pa = ALhvMMgetPA(ept_pml4_mem);
	auto ept = ept_pml4_mem;
	for (UINT64 i = 0; i < MB_TO_BYTE(2); i += 0x1000)
	{
		ept_pte pte = { 0 };
		pte.read_access = pde_demo.read_access;
		pte.write_access = pde_demo.write_access;
		pte.execute_access = pde_demo.execute_access;
		pte.page_frame_number = i >> 12;
		pte.memory_type = ALhvMMgetMemoryType(i, 0x1000, 0);
		if (!ALvmxEPTsetPTE(i, pte.flags, ept))
		{
			ALhvAddErr("设置pte失败");
			return { 0 };
		}
		//ALvmPut("%d,%d", i, pte.memory_type);
	}
	for (UINT64 i = MB_TO_BYTE(2); i < GB_TO_BYTE(size); i += MB_TO_BYTE(2))
	{
		ept_pde_2mb pde = { 0 };
		pde.read_access = pde_demo.read_access;
		pde.write_access = pde_demo.write_access;
		pde.execute_access = pde_demo.execute_access;
		pde.large_page = 1;
		pde.page_frame_number = i >> (12 + 9);
		pde.memory_type = ALhvMMgetMemoryType(i, MB_TO_BYTE(2), 0);
		if (!ALvmxEPTsetPDE(i, pde.flags, ept))
		{
			ALhvAddErr("设置pde失败");
			return { 0 };
		}
	}
	o_eptp->va = ept_pml4_mem;
	o_eptp->pv = ept_pml4_mem_pa;


	return 1;



}


static BOOLEAN EptCheckFeatures()
{
	ia32_vmx_ept_vpid_cap_register VpidRegister;
	ia32_mtrr_def_type_register    MTRRDefType;

	VpidRegister.flags = __readmsr(IA32_VMX_EPT_VPID_CAP);
	MTRRDefType.flags = __readmsr(IA32_MTRR_DEF_TYPE);

	if (!VpidRegister.page_walk_length_4 || !VpidRegister.memory_type_write_back || !VpidRegister.pde_2mb_pages)
	{
		return FALSE;
	}

	if (!VpidRegister.advanced_vmexit_ept_violations_information)
	{
		ALhvSetErr("The processor doesn't report advanced VM-exit information for EPT violations\n");//勉强能用
	}
	if (!VpidRegister.pde_2mb_pages)
	{
		ALhvSetErr("not 2m page\n");
		return FALSE;
	}
	if (!VpidRegister.execute_only_pages)
	{
		return FALSE;
	}
	if (!MTRRDefType.mtrr_enable)
	{
		ALhvSetErr("Mtrr Dynamic Ranges not supported");
		return FALSE;
	}

	return TRUE;
}

bool EPT_CLS::init()
{
	
	if (const_ept.vv)
	{
		ALhvSetErr("重复初始化");
		return 0;
	}
	else
	{
		if (!EptCheckFeatures())
		{
			ALhvSetErr("CPU不支持EPT");
			return 0;
		}
		ept_pde_2mb pde_dome = {};
		pde_dome.read_access = 1;
		pde_dome.write_access = 1;
		pde_dome.execute_access = 1;
		if (!ALvmxEPTnewTable(pde_dome, const_ept_size, &const_ept))
		{
			ALhvAddErr("生成新的ept失败");
			return 0;
		}
		const_eptp_value.flags = 0;
		const_eptp_value.memory_type = MEMORY_TYPE_WRITE_BACK;
		const_eptp_value.page_walk_length = 3;
		const_eptp_value.page_frame_number = const_ept.pv / 0x1000;
		return 1;
	}

}

ept_pointer EPT_CLS::getConstEptp()
{
	if (const_eptp_value.flags)
		return const_eptp_value;
	else
	{
		ALhvSetErr("EPT未初始化");
		return {};
	}
}

//bool EPT_CLS::getCloneTable()
//{
//	/*if (!o_eptp)
//	{
//		ALhvSetErr("参数错误");
//		return 0;
//	}*/
//	
//}

EPT_CLS::EPT_CLS()
{
	this->ept_pml4 = {};
	this->eptp_v = {};
	auto o_eptp = this;
	if (!const_eptp_value.flags)
	{
		ALhvSetErr("ept未初始化");
		return;
	}
	auto new_t_m = (ept_pml4e*)ALhvMMallocateMemory(0x1000);
	for (int i = 0; i < 0x1000 / 8; i++)
	{
		new_t_m->flags = const_ept->flags;
	}
	o_eptp->ept_pml4.vv = (UINT64)new_t_m;
	o_eptp->ept_pml4.pv = ALhvMMgetPA(new_t_m);
	o_eptp->eptp_v = o_eptp->const_eptp_value;
	o_eptp->eptp_v.page_frame_number = o_eptp->ept_pml4.pv >> 12;
	return;
}

ept_pointer EPT_CLS::getEptp()
{
	return this->eptp_v;
}
OR_PTR<ept_pml4e> EPT_CLS::getPml4e()
{
	return this->ept_pml4;
}

bool EPT_CLS::set_pte(UINT64 GPA, ept_pte pte)
{
	return ALvmxEPTsetPTE(GPA, pte.flags, this->ept_pml4.va);
}

bool EPT_CLS::get_pte(UINT64 GPA, ept_pte* pte)
{
	auto r = ALvmxEPTgetPageTableInfo(GPA, 5, this->ept_pml4.va);
	pte->flags = r;
	return r != 0;
}

