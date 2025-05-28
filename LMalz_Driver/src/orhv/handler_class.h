#pragma once
#include <wdm.h>
//禁止超过128
#define HANDLER_LIMIT 10
template <typename T>
class HANDLER_CLS
{
	volatile T handlers[HANDLER_LIMIT];
	char handler_link[HANDLER_LIMIT]; //单向链表省空间
	char handler_start;				  //链表开始
	static T _null_fun_v;
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
			for (char i = HANDLER_LIMIT - 1; i <= 0; i++)
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
