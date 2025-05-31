#pragma once
#include "../orhv.h"
#include "vmx.h"

//使用时创建全局指针指向局部静态对象
class EPT_CLS
{					
	OR_PTR<ept_pml4e> ept_pml4;
	ept_pointer eptp_v;


	static OR_PTR<ept_pml4e> const_ept;
	static const UINT64 const_ept_size = 512;  //大小(GB)
	static ept_pointer const_eptp_value;//写入vmcs的值

	int EPT_CLS::need_remap(UINT64 GPA, __out UINT64* pt_v);   //判断是否需要重新映射(为了不影响const_ept)

public:

	static ept_pointer getConstEptp();
	//EPT类初始化
	static bool init();
	//克隆一张表(只复制pml4e,全属性)
	EPT_CLS();
	//克隆一张表(复制pml4e和pdpte,并设置pdpte属性)
	EPT_CLS(__in bool _1gb_R, __in bool _1gb_W, __in bool _1gb_X);


	bool set_pte(UINT64 GPA, ept_pte pte);
	bool set_pte(UINT64 GPA, bool _r, bool _w, bool _x);
	bool get_pte(UINT64 GPA, ept_pte* pte);

	ept_pointer	getEptp();

	OR_PTR<ept_pml4e> getPml4e();


};
