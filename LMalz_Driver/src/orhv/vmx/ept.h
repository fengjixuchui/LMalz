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

	////生成一张克隆表(需初始化完毕)
	//bool getCloneTable();
public:

	static ept_pointer getConstEptp();
	//EPT类初始化
	static bool init();
	//克隆一张表
	EPT_CLS();
	bool set_pte(UINT64 GPA, ept_pte pte);
	bool get_pte(UINT64 GPA, ept_pte* pte);

	ept_pointer	getEptp();

	OR_PTR<ept_pml4e> getPml4e();


};
