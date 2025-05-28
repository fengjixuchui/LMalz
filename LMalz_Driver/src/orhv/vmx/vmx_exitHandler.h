#pragma once
#include "..\orhv.h"
#include "..\handler_class.h"
typedef bool(*VMEXIT_HANDLER_T)(OR_HV_VMX_CORE*,bool);

class VMEXIT_HANDLERS_CLS :private HANDLER_CLS<VMEXIT_HANDLER_T>
{
	static bool _default(OR_HV_VMX_CORE* vcpu, bool call_2)
	{
		if (call_2)		//第二次调用给蓝屏
		{
			UINT64 v = 0;
			__vmx_vmread(VMCS_EXIT_REASON, &v);
			vmx_vmexit_reason reason;
			reason.flags = (UINT32)v;
			ALhvKill(reason.basic_exit_reason, vcpu->reg.rip);
		}
		else
			return 0;
	}
public:
	VMEXIT_HANDLERS_CLS(VMEXIT_HANDLER_T m, bool toend = 0)	  //默认置顶处理
	{
		add_handler(m, toend);
	};
	VMEXIT_HANDLERS_CLS()
	{
		add_handler(_default, 1);
	};
	bool add_fun(VMEXIT_HANDLER_T m, bool toend = 0)
	{
		sub_handler(_default);//尝试删除末位处理程序
		return add_handler(m, toend);
	}
	bool call_all(OR_HV_VMX_CORE* vcpu)		   //调用两次,必须被处理不然就蓝屏
	{
		auto hd = call_handlers(vcpu, 0);
		if (!hd)
		{
			hd = call_handlers(vcpu, 1);
			if (!hd)
				_default(vcpu, 1);
		}
		return 1;
	}

};
