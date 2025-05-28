#include "../orhv.h"
#include "svm.h"
#include <intrin.h>
#include <wdm.h>

OR_HV_SVM* gALsvmVCPU = 0;
bool ALsvmCoreStart(OR_HV_SVM_CORE* core)
{
	core;
	ALhvSetErr("未实现");
	return 0;
}
bool ALsvmStart(OR_HV_SVM* vcpu)
{

	PROCESSOR_NUMBER processorNumber = { 0 };
	GROUP_AFFINITY   affinity = { 0 }, oldAffinity = { 0 };
	UINT64 success = 0;
	ALhvSetErr("");	  //初始化错误string
	for (ULONG i = 0; i < vcpu->core_count; i++)
	{
		//切换处理器核心
		auto status = KeGetProcessorNumberFromIndex(i, &processorNumber);
		if (!NT_SUCCESS(status)) {
			ALhvSetErr("切换核心失败%d", i);
			break;
		}
		affinity.Group = processorNumber.Group;
		affinity.Mask = 1ULL << processorNumber.Number;
		affinity.Reserved[0] = affinity.Reserved[1] = affinity.Reserved[2] = 0;
		KeSetSystemGroupAffinityThread(&affinity, &oldAffinity);

		//开始当前核心虚拟化
		if (ALsvmCoreStart(&vcpu->cores[i]))
		{
			(success++);
		}
		else
		{
			ALhvAddErr("%d核心虚拟化失败", i);
		}

		//换回原核心
		KeRevertToUserGroupAffinityThread(&oldAffinity);

	}


	//ALdbgPut("VT Engine has been loaded!\n");

	return success == vcpu->core_count;
}


bool ALsvmIsRoot()
{
	auto vcpu = ALsvmGetCurrVcore();
	if (vcpu)
		return vcpu->isRoot;
	return 0;
}


OR_HV_SVM_CORE* ALsvmGetCurrVcore()
{
	return  &gALsvmVCPU->cores[ALhvGetCurrVcoreIndex()];
}
//OR_HV_SVM_CORE* ALsvmGetCurrVcore()
//{
//	if (!gALsvmVCPU)
//		return 0;
//	auto  rsp = (UINT64)_AddressOfReturnAddress();
//	for (int i = 0; i < gALsvmVCPU->core_count; i++)
//		if (rsp >= (UINT64)(&gALsvmVCPU->cores[i]) && rsp < (UINT64)(&gALsvmVCPU->cores[i + 1]))
//			return  &gALsvmVCPU->cores[i];
//	//如果上面找不到说明不在root模式,直接调用api
//	return  &gALsvmVCPU->cores[KeGetCurrentProcessorIndex()];
//}
//int ALsvmGetCurrVcoreIndex()
//{
//	if (!gALsvmVCPU)
//		return KeGetCurrentProcessorIndex();
//	auto  rsp = (UINT64)_AddressOfReturnAddress();
//	for (int i = 0; i < gALsvmVCPU->core_count; i++)
//		if (rsp >= (UINT64)(&gALsvmVCPU->cores[i]) && rsp < (UINT64)(&gALsvmVCPU->cores[i + 1]))
//			return i;
//	//如果上面找不到说明不在root模式,直接调用api
//	return KeGetCurrentProcessorIndex();
//}