#pragma once
#include "..\vcpu_vmx.h"
enum MONITOR_CALLBACK_TYPE {

	MONITOR_CALLBACK_TYPE_enter,   //����Ŀ��
	MONITOR_CALLBACK_TYPE_exit,	   //����Ŀ��
	MONITOR_CALLBACK_TYPE_mtf,	   //Ŀ���ڵ���

};
typedef void (*MONITOR_CALLBACK)(OR_HV_VMX_CORE*, MONITOR_CALLBACK_TYPE);
UINT64 ALvmxMNTgetLastMTFaddress();
