#pragma once
#include "vcpu_vmx.h"
#include "vmx_exitHandler.h"

//vmx.cpp
extern OR_HV_VMX* gALvmxVCPU;
bool ALvmxStart(OR_HV_VMX* vcpu);	  //vmx ��ʼ

bool ALvmxIsRoot();//��ǰ�����Ƿ�Ϊhostģʽ

cr3 ALvmxGetGuestCr3();

OR_HV_VMX_CORE* ALvmxGetCurrVcore();  //��ȡ��ǰ����CPU�ṹ��	  ���δ��ʼ����Ϊ0

//vmxasm.asm
extern "C" void ALvmxInvept_asm(invept_type type, invept_descriptor const& desc);
extern "C" void ALvmxInvvpid_asm(invvpid_type type, invvpid_descriptor const& desc);;

extern "C" void ALvmxHostEnter_asm();
extern "C" void ALvmxGuestEnter_asm(OR_HV_VMX_CORE*);
extern "C" UINT64 ALvmxVmcall_asm(UINT32 key_ecx, UINT64 code_rdx, UINT64 r8 = 0, UINT64 r9 = 0);

//vmx_exitHandler.cpp
bool ALvmxVmexitInit();
inline void vmx_increment_rip(OR_HV_VMX_CORE* vcpu);
VMEXIT_HANDLERS_CLS* ALvmxGetExitHandlers();