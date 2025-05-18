#pragma once
#include "vcpu_vmx.h"

//vmx.cpp
extern OR_HV_VMX* gALvmxVCPU;
bool ALvmxStart(OR_HV_VMX* vcpu);	  //vmx 开始

OR_HV_VMX_CORE* ALvmxGetVCPU();//获取当前虚拟CPU结构体	  如果未初始化则为0

bool ALvmxIsRoot();//当前环境是否为host模式


OR_HV_VMX_CORE* ALvmxGetCurrVcore();

//vmxasm.asm
extern "C" void ALvmxInvept_asm(invept_type type, invept_descriptor const& desc);