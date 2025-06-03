#pragma once
#include "../orhv.h"
#include "vmx.h"

//主表用于克隆,副表用于触发违例,所有首次ept违例必须都用副表触发,guest跑在副表上

//使用时创建全局指针指向局部静态对象
class EPT_CLS
{					
	static OR_PTR<ept_pml4e> const_ept;
	static const UINT64 const_ept_size = 512;  //大小(GB)
	static ept_pointer const_eptp_value;//写入vmcs的值


	OR_PTR<ept_pml4e> ept_pml4;
	ept_pointer eptp_v;
	EPT_CLS* clone;			  //克隆的对象,若为0则是克隆的const,不支持新建,仅允许复用
	int need_remap(EPT_CLS* target, UINT64 GPA, __out UINT64* pt_v);
	int need_remap(UINT64 GPA, __out UINT64* pt_v);   //判断是否需要重新映射(为了不影响const_ept)

public:

	static ept_pointer getConstEptp();
	static OR_PTR<ept_pml4e>* getConstEpt() { return &const_ept; };

	static EPT_CLS* getViceEpt();	  //获取副表ept,该表继承自const


	//EPT类初始化
	static bool init();
	//克隆const_ept(只复制pml4e,全属性)
	EPT_CLS();
	//克隆conset_ept(复制pml4e和pdpte,并设置pdpte属性)
	EPT_CLS(__in bool _1gb_R, __in bool _1gb_W, __in bool _1gb_X);
	//克隆参数指向的表
	EPT_CLS(EPT_CLS* clone);

	bool set_pte(UINT64 GPA, ept_pte pte);
	bool set_pte(UINT64 GPA, bool _r, bool _w, bool _x);


	bool get_pte(UINT64 GPA, ept_pte* pte);
	bool get_pde(UINT64 GPA, ept_pde* pde);
	bool get_pdpte(UINT64 GPA, ept_pdpte* pdpte);
	bool get_pml4e(UINT64 GPA, ept_pml4e* pml4e);
	ept_pml4e  get_pml4e(UINT64 GPA);
	ept_pdpte  get_pdpte(UINT64 GPA);
	ept_pde  get_pde(UINT64 GPA);
	ept_pte  get_pte(UINT64 GPA);

		

	ept_pte* pte(UINT64 GPA);
	ept_pde* pde(UINT64 GPA);
	ept_pdpte* pdpte(UINT64 GPA);
	ept_pml4e* pml4e(UINT64 GPA);



	ept_pointer	getEptp();

	OR_PTR<ept_pml4e> getPml4e();


};
