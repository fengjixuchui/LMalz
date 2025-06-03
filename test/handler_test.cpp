#include <windows.h>
#include <intrin.h>
#include <stdio.h>
#define __R3
#define _CRT_SECURE_NO_WARNINGS

#include "F:\g_un\ALJT\Code\omri\include\driver\driver.h"
#include "F:\g_un\ALJT\Code\omri\include\orvm\orvm.h"
#include "F:\g_un\ALJT\Code\omri\include\debug\debug.h"
#include "F:\g_un\ALJT\Code\omri\include\memory\memory.h"
#include "F:\g_un\ALJT\Code\omri\include\PEanalysis\PEanalysis.h"
#include "F:\g_un\ALJT\Code\omri\include\process\process.h"
#include "F:\g_un\ALJT\Code\omri\include\resource\omri3\resource.h"
//禁止超过128
#define HANDLER_LIMIT 10
template <typename T>
class HANDLER_CLS
{
	volatile T handlers[HANDLER_LIMIT];
	char handler_link[HANDLER_LIMIT]; //单向链表省空间
	char handler_start;				  //链表开始
	T _null_fun_v;
	template <typename... Args>
	static bool _null(Args... args)
	{
		int a[] = { ((args), 0)... };
		a;
		return 0;
	}
	char find_link_last(int n)	 //寻找上一个有效函数n须为有效函数
	{
		if (handler_start == n)
			return -1;
		for (char i = handler_start; handler_link[i] != -1; i = handler_link[i])
		{
			if (handler_link[i] == n)
			{
				return i;
			}
		}
		return -1;
	}
	char find_link_next(int n)	//寻找下一个有效函数n须为有效函数
	{
		if (handler_start == -1)
			return -1;
		return handler_link[n];
	}
	char get_link_end()
	{
		if (handler_start == -1)
			return -1;
		for (char i = handler_start;; i = handler_link[i])
		{
			if (handler_link[i] == -1)
			{
				return i;
			}
		}
	}
	void add_set_link(char n, bool toend)
	{
		if (toend)
		{
			if (handler_start == -1)
			{
				handler_start = n;
			}
			else
			{
				auto end = get_link_end();
				handler_link[end] = n;
			}
		}
		else
		{
			handler_link[n] = handler_start;
			handler_start = n;
		}
	}
	void sub_set_link(char n)
	{
		auto lst = find_link_last(n);
		if (lst == -1)
		{
			handler_start = handler_link[n];
		}
		else
		{
			handler_link[lst] = handler_link[n];
		}

	}
public:
	HANDLER_CLS()
	{
		_null_fun_v = &_null;
		handler_start = -1;
		for (int i = 0; i < HANDLER_LIMIT; i++)
		{
			handlers[i] = _null;
			handler_link[i] = -1;
		}
	}
	bool add_handler(T handler, bool toend)
	{
		if (toend)
			for (char i = HANDLER_LIMIT - 1; i >= 0; i--)
			{
				auto r = _InterlockedCompareExchange64((LONG64 volatile*)&handlers[i], (UINT64)handler, (UINT64)_null_fun_v);
				if (r == (INT64)_null_fun_v)
				{
					add_set_link(i, toend);
					return 1;
				}
			}
		else
			for (char i = 0; i < HANDLER_LIMIT; i++)
			{
				auto r = _InterlockedCompareExchange64((LONG64 volatile*)&handlers[i], (UINT64)handler, (UINT64)_null_fun_v);
				if (r == (INT64)_null_fun_v)
				{
					add_set_link(i, toend);
					return 1;
				}
			}
		return 0;
	}
	bool sub_handler(T handler)
	{
		for (char i = 0; i < HANDLER_LIMIT; i++)
		{
			auto r = _InterlockedCompareExchange64((LONG64 volatile*)&handlers[i], (UINT64)_null_fun_v, (UINT64)handler);
			if (r == (INT64)handler)
			{
				sub_set_link(i);
				return 1;
			}
		}
		return 0;
	}
	template <typename... Args>
	bool call_handlers(Args... args)
	{
		for (char i = handler_start; i != -1; i = handler_link[i])
		{
			auto r = handlers[i](args...);
			if (r)
				return 1;
		}
		return 0;
	}

};
typedef bool(*VMEXIT_HANDLER_T)(int*, bool);

class VMEXIT_HANDLERS_CLS :private HANDLER_CLS<VMEXIT_HANDLER_T>
{
	static bool _default(int* vcpu, bool call_2)
	{
		if (call_2)		//第二次调用给蓝屏
		{
			ALdbgStoput("爆炸");
			return 1;
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
	bool call_all(int* vcpu)		   //调用两次,必须被处理不然就蓝屏
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
bool test1(int* a, bool is2)
{
	ALdbgPut("test1执行%d", (int)is2);
	return 0;
}	  
bool test2(int* a, bool is2)
{
	ALdbgPut("test2执行%d", (int)is2);
	return 0;
}		  
bool test3(int* a, bool is2)
{
	ALdbgPut("test3执行%d", (int)is2);
	return 0;
}

int main()
{
	VMEXIT_HANDLERS_CLS a;
	a.add_fun(test1);
	a.add_fun(test2);
	a.add_fun(test3);
	a.call_all(0);
	return 0;

}