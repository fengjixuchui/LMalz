//#include <intrin.h>
//#include <wdm.h>
//#include "../orhv.h"
//#include "vmx.h"
#include "ept.h"
OR_PTR<ept_pml4e> EPT_CLS::const_ept = {};
ept_pointer EPT_CLS::const_eptp_value = {};
// 0 null, 1 pml4e(512GB), 2 pdpte(1GB), 3 pde(2MB), 4 pte(4KB)
static PUINT64 ALvmxEPTpageTableInfo(UINT64 GPA, int layerNumber, ept_pml4e* ept)
{

	if (layerNumber > 4 || layerNumber < 1)
	{
		ALhvSetErr("无效 参数");
		return 0;
	}
	OR_ADDRESS addIndex = { 0 };   addIndex.value = GPA;
	if (ept)
	{
		ept_pml4e& pml4e = ept[addIndex.pml4_index];
		if (pml4e.page_frame_number)
		{
			if (layerNumber == 1)
				return &pml4e.flags;
			auto pdpte = (ept_pml4e*)ALhvMMgetVA((pml4e.page_frame_number << 12));
			pdpte += addIndex.pdpt_index;
			if (layerNumber == 2)
				return &pdpte->flags;
			if (pdpte->page_frame_number)
			{
				auto pPde = (ept_pde_2mb*)ALhvMMgetVA((pdpte->page_frame_number << 12));
				pPde += addIndex.pd_index;
				if (layerNumber == 3)
					return &pPde->flags;
				if (pPde->large_page)
					return 0;
				ept_pde pde = { 0 }; pde.flags = pPde->flags;
				if (pde.page_frame_number)
				{
					//static auto save = ALvmMMallocateMemory(0x1000);
					//ALvmMMsaveSelfMapPte(save);
					auto pPte = (ept_pte*)ALhvMMgetVA((pde.page_frame_number << 12));
					pPte += addIndex.pt_index;
					return &pPte->flags;
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

// 0 null, 1 pml4e(512GB), 2 pdpte(1GB), 3 pde(2MB), 4 pte(4KB)
 UINT64 ALvmxEPTgetPageTableInfo(UINT64 GPA, int layerNumber, ept_pml4e* ept)
{
	auto r = ALvmxEPTpageTableInfo(GPA, layerNumber, ept);
	if (r)
		return *r;
	else
		return 0;
}		   
// 0 null, 1 pml4e(512GB), 2 pdpte(1GB), 3 pde(2MB), 4 pte(4KB)
static bool ALvmxEPTsetPageTableInfo(UINT64 GPA, int layerNumber, UINT64 v, ept_pml4e* ept)
{
	auto r = ALvmxEPTpageTableInfo(GPA, layerNumber, ept);
	if (r)
	{
		*r = v;
		return 1;
	}
	else
		return 0;
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
		else	  //设置pml4e
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
	pde_r.flags = ALvmxEPTgetPageTableInfo(GPA, 3, ept);
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
	if (!o_eptp)
	{
		ALhvSetErr("参数错误");
		return 0;
	}
	*o_eptp = {};

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
		ept_pde_2mb pde_demo = {};
		pde_demo.read_access = 1;
		pde_demo.write_access = 1;
		pde_demo.execute_access = 1;
		if (!ALvmxEPTnewTable(pde_demo, const_ept_size, &const_ept))
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
EPT_CLS* EPT_CLS::getViceEpt()
{
	static EPT_CLS vice_ept(0);

	return &vice_ept;

}


//返回构建的页表项
static UINT64 clone_page_table(UINT64 GPA, int layerNumber, ept_pml4e* ept)
{
	if (layerNumber < 1 || layerNumber > 3)
	{
		ALhvSetErr("无效参数");
		return 0;
	}
	auto new_pg_tb = ALhvMMallocateMemory(0x1000);//申请新的pdpt(每个项1GB的表)
	auto new_pg_tb_pa = ALhvMMgetPA(new_pg_tb);
	union
	{
		ept_pml4e cur_e;
		ept_pdpte cur_e2;
		ept_pde cur_e3;
		ept_pde_2mb	cur_e32;
		ept_pte cur_e4;
		UINT64 v;
	} te = {};

	te.v = ALvmxEPTgetPageTableInfo(GPA, layerNumber, ept);	 
	PVOID va = 0;
	if (layerNumber == 1)
	{
		va = ALhvMMgetVA(te.cur_e.page_frame_number << 12);
		te.cur_e.page_frame_number = new_pg_tb_pa>>12;

	}
	else if (layerNumber == 2)
	{
		va = ALhvMMgetVA(te.cur_e2.page_frame_number << 12);
		te.cur_e2.page_frame_number = new_pg_tb_pa>>12;
	}
	else if (layerNumber == 3)
	{
		if (te.cur_e32.large_page)
		{
			ALhvSetErr("终项无法克隆");
			return 0;
		}
		va = ALhvMMgetVA(te.cur_e3.page_frame_number << 12);
		te.cur_e3.page_frame_number = new_pg_tb_pa >> 12;
	}
	if (!new_pg_tb)
	{
		ALhvSetErr("申请内存失败");
		return 0;
	}
	if (!new_pg_tb_pa)
	{
		ALhvSetErr("新页表物理地址获取失败");
		return 0;
	}
	if (!va)
	{
		ALhvSetErr("原页表虚拟地址获取失败");
		return 0;
	}
	ALhvMMCopyData(new_pg_tb, va, 0x1000);
	return te.v;
}


static ept_pdpte* clone_pdpt(ept_pml4e* ept, UINT64 start_GPA)
{

	ept_pml4e cur_e = { 0 };
	cur_e.flags = ALvmxEPTgetPageTableInfo(start_GPA, 1, ept);	//获取pml4e(每个项512GB的表)
	auto va = ALhvMMgetVA(cur_e.page_frame_number << 12);

	auto pdpt_p = (ept_pdpte*)ALhvMMallocateMemory(0x1000);//申请新的pdpt(每个项1GB的表)

	ALhvMMCopyData(pdpt_p, va, 0x1000);

	return pdpt_p;
}
EPT_CLS::EPT_CLS() :EPT_CLS((EPT_CLS*)0)
{
	return;
}
EPT_CLS::EPT_CLS(__in bool _1gb_R, __in bool _1gb_W, __in bool _1gb_X) :EPT_CLS()
{
	auto pdpt_p = clone_pdpt(this->const_ept.va, 0);//克隆新的pdpt(每个项为1GB)表

	for (int i = 0; i < 512; i++)				 //将项设置为需要的属性
	{
		pdpt_p[i].read_access = _1gb_R;
		pdpt_p[i].write_access = _1gb_W;
		pdpt_p[i].execute_access = _1gb_X;
	}
	auto pa = ALhvMMgetPA(pdpt_p);

	this->ept_pml4->page_frame_number = pa >> 12;		  //修改pml4第一个项(0-512GB)
}
EPT_CLS::EPT_CLS(EPT_CLS* clone)
{
	this->ept_pml4 = {};
	this->eptp_v = {};
	this->clone = clone;
	auto o_eptp = this;
	if (!const_eptp_value.flags)
	{
		ALhvSetErr("ept未初始化");
		return;
	}
	OR_PTR<ept_pml4e>* target = {};
	if (clone == 0)
		target = &const_ept;
	else
		target = &clone->ept_pml4;
	auto new_t_m = (ept_pml4e*)ALhvMMallocateMemory(0x1000);
	for (int i = 0; i < 0x1000 / 8; i++)
	{
		new_t_m[i].flags = target->va[i].flags;
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
int EPT_CLS::need_remap(EPT_CLS* target_obj, UINT64 GPA, __out UINT64* pt_v)
{
	*pt_v = 0;
	OR_PTR<ept_pml4e>& target = target_obj == 0 ? const_ept : target_obj->ept_pml4;
	ept_pml4e cur_e = { 0 };
	ept_pml4e cons_e = { 0 };
	cur_e.flags = ALvmxEPTgetPageTableInfo(GPA, 1, this->ept_pml4.va);
	cons_e.flags = ALvmxEPTgetPageTableInfo(GPA, 1, target.va);
	if (cur_e.page_frame_number == cons_e.page_frame_number)
	{
		*pt_v = cur_e.flags;
		return 1;
	}
	else
	{

		ept_pdpte cur_e1 = { 0 };
		ept_pdpte cons_e1 = { 0 };
		cur_e1.flags = ALvmxEPTgetPageTableInfo(GPA, 2, this->ept_pml4.va);
		cons_e1.flags = ALvmxEPTgetPageTableInfo(GPA, 2, target.va);
		if (cur_e1.page_frame_number == cons_e1.page_frame_number)
		{
			*pt_v = cur_e1.flags;
			return 2;
		}
		else
		{
			ept_pde_2mb cons_e_2m = { 0 };
			ept_pde_2mb cur_e_2m = { 0 };
			cur_e_2m.flags = ALvmxEPTgetPageTableInfo(GPA, 3, this->ept_pml4.va);
			cons_e_2m.flags = ALvmxEPTgetPageTableInfo(GPA, 3, target.va);
			if (cons_e_2m.large_page || cur_e_2m.large_page) //终项
				return 0;
			ept_pde cons_e2 = { 0 };
			ept_pde cur_e2 = { 0 };
			cons_e2.flags = cons_e_2m.flags;
			cur_e2.flags = cur_e_2m.flags;
			if (cur_e2.page_frame_number == cons_e2.page_frame_number)
			{
				*pt_v = cur_e2.flags;
				return 3;
			}
			else
				return 0;

		}
	}
}
int EPT_CLS::need_remap(UINT64 GPA, __out UINT64* pt_v)
{
	for (EPT_CLS* i = this->clone;;)
	{
		auto ret = need_remap(i, GPA, pt_v);
		if (ret || i == 0)	//如果需要重新构建路径或者克隆已到达终项
			return ret;
		else
			i = i->clone;
	}		
}
//#pragma warning(disable:4702)

bool EPT_CLS::set_pte(UINT64 GPA, ept_pte pte)
{
	//return ALvmxEPTsetPTE(GPA, pte.flags, this->ept_pml4.va);

	UINT64 pt_v = 0;
	auto n_p = need_remap(GPA, &pt_v);
	if (!n_p)	  //如果不需要重新映射
		return ALvmxEPTsetPTE(GPA, pte.flags, this->ept_pml4.va);
	else
	{
		union
		{
			ept_pml4e pml4e;
			ept_pdpte pdpte;
			ept_pde pde;
			UINT64 v;
		} te[2] = {};
		te[1].v = pt_v;
		te->v = clone_page_table(GPA, n_p, this->ept_pml4.va);
		if (n_p == 1)
		{
			te[1].pml4e.page_frame_number = te->pml4e.page_frame_number;
			auto tb_va = (ept_pdpte*)ALhvMMgetVA(te[1].pml4e.page_frame_number << 12);
			for (int i = 0; i < 512; i++)
			{
				tb_va[i].read_access = te[1].pml4e.read_access;
				tb_va[i].write_access = te[1].pml4e.write_access;
				tb_va[i].execute_access = te[1].pml4e.execute_access;
			}
			te[1].pml4e.execute_access = te[1].pml4e.write_access = te[1].pml4e.read_access = 1;	  //克隆的项继承上一级属性,上一级设为全属性
			ALvmxEPTsetPageTableInfo(GPA, n_p, te[1].v, this->ept_pml4.va);
			return set_pte(GPA, pte);				  //递归继续设置
		}
		else if (n_p == 2)
		{
			te[1].pdpte.page_frame_number = te->pdpte.page_frame_number;
			auto tb_va = (ept_pde*)ALhvMMgetVA(te[1].pdpte.page_frame_number << 12);
			for (int i = 0; i < 512; i++)
			{
				tb_va[i].read_access = te[1].pdpte.read_access;
				tb_va[i].write_access = te[1].pdpte.write_access;
				tb_va[i].execute_access = te[1].pdpte.execute_access;
			}
			te[1].pdpte.execute_access = te[1].pdpte.write_access = te[1].pdpte.read_access = 1;	  //克隆的项继承上一级属性,上一级设为全属性
			ALvmxEPTsetPageTableInfo(GPA, n_p, te[1].v, this->ept_pml4.va);
			return set_pte(GPA, pte);				    //递归继续设置
		}
		else if (n_p == 3)
		{
			te[1].pde.page_frame_number = te->pde.page_frame_number;
			auto tb_va = (ept_pte*)ALhvMMgetVA(te[1].pde.page_frame_number << 12);
			for (int i = 0; i < 512; i++)
			{
				tb_va[i].read_access = te[1].pde.read_access;
				tb_va[i].write_access = te[1].pde.write_access;
				tb_va[i].execute_access = te[1].pde.execute_access;
			}
			te[1].pde.execute_access = te[1].pde.write_access = te[1].pde.read_access = 1;		     //克隆的项继承上一级属性,上一级设为全属性
			ALvmxEPTsetPageTableInfo(GPA, n_p, te[1].v, this->ept_pml4.va);
			return set_pte(GPA, pte);					 //递归继续设置
		}
		else
		{
			ALhvSetErr("未知异常");
			return 0;
		}

	}
}
bool EPT_CLS::set_pte(UINT64 GPA, bool _r, bool _w, bool _x)
{
	ept_pte pte = { 0 };
	this->get_pte(GPA, &pte);
	if (pte.flags == 0)
	{
		pte.page_frame_number = GPA >> 12;
		pte.memory_type = ALhvMMgetMemoryType(GPA, 0x1000, 0);
	}
	pte.read_access = _r;
	pte.write_access = _w;
	pte.execute_access = _x;
	
	return set_pte(GPA, pte);
}

bool EPT_CLS::get_pml4e(UINT64 GPA, ept_pml4e* e)
{
	auto r = ALvmxEPTgetPageTableInfo(GPA, 1, this->ept_pml4.va);
	e->flags = r;
	return r != 0;
}
bool EPT_CLS::get_pdpte(UINT64 GPA, ept_pdpte* e)
{
	auto r = ALvmxEPTgetPageTableInfo(GPA, 2, this->ept_pml4.va);
	e->flags = r;
	return r != 0;
}
bool EPT_CLS::get_pde(UINT64 GPA, ept_pde* e)
{
	auto r = ALvmxEPTgetPageTableInfo(GPA, 3, this->ept_pml4.va);
	e->flags = r;
	return r != 0;
}
bool EPT_CLS::get_pte(UINT64 GPA, ept_pte* e)
{
	auto r = ALvmxEPTgetPageTableInfo(GPA, 4, this->ept_pml4.va);
	e->flags = r;
	return r != 0;
}	  

ept_pml4e EPT_CLS::get_pml4e(UINT64 GPA)
{
	ept_pml4e r = {};
	r.flags	 = ALvmxEPTgetPageTableInfo(GPA, 1, this->ept_pml4.va);
	return r;
}
ept_pdpte EPT_CLS::get_pdpte(UINT64 GPA )
{
	ept_pdpte r = {};
	r.flags = ALvmxEPTgetPageTableInfo(GPA, 2, this->ept_pml4.va);
	return r;
}
ept_pde EPT_CLS::get_pde(UINT64 GPA )
{
	ept_pde r = {};
	r.flags = ALvmxEPTgetPageTableInfo(GPA, 3, this->ept_pml4.va);
	return r;
}
ept_pte EPT_CLS::get_pte(UINT64 GPA )
{
	ept_pte r = {};
	r.flags = ALvmxEPTgetPageTableInfo(GPA, 4, this->ept_pml4.va);
	return r;
}





ept_pte* EPT_CLS::pte(UINT64 GPA)
{
	return (ept_pte*)ALvmxEPTpageTableInfo(GPA, 4, this->ept_pml4.va);
}
ept_pde* EPT_CLS::pde(UINT64 GPA)
{
	return (ept_pde*)ALvmxEPTpageTableInfo(GPA, 3, this->ept_pml4.va);

}
ept_pdpte* EPT_CLS::pdpte(UINT64 GPA)
{
	return (ept_pdpte*)ALvmxEPTpageTableInfo(GPA, 2, this->ept_pml4.va);

}
ept_pml4e* EPT_CLS::pml4e(UINT64 GPA)
{
	return (ept_pml4e*)ALvmxEPTpageTableInfo(GPA, 1, this->ept_pml4.va);
}

