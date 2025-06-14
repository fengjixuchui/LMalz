#pragma once
#include "..\vcpu_vmx.h"
enum MONITOR_CALLBACK_TYPE {

	MONITOR_CALLBACK_TYPE_enter,   //进入目标
	MONITOR_CALLBACK_TYPE_exit,	   //跳出目标
	MONITOR_CALLBACK_TYPE_mtf,	   //目标内单步

};
typedef void (*MONITOR_CALLBACK)(OR_HV_VMX_CORE*, MONITOR_CALLBACK_TYPE);
UINT64 ALvmxMNTgetLastMTFaddress();
