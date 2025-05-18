#pragma once
#include <wdm.h>
#define HANDLER_LIMIT 10
template <typename T>
class HANDLER_CLS
{
	volatile T handlers[HANDLER_LIMIT];
public:
	bool add_handler(T hansler)
	{
		for (int i = 0; i < HANDLER_LIMIT; i++)
		{
			auto r = _InterlockedCompareExchange64((LONG64 volatile*)&handlers[i], (UINT64)hansler, 0);
			if (!r)
				return 1;
		}
		return 0;
	}
	bool sub_handler(T hansler)
	{
		for (int i = 0; i < HANDLER_LIMIT; i++)
		{
			auto r = _InterlockedCompareExchange64((LONG64 volatile*)&handlers[i], 0, (UINT64)hansler);
			if (!r)
				return 1;
		}
		return 0;
	}
	template <typename... Args>
	bool call_handlers(Args... args)
	{
		for (int i = 0; i < HANDLER_LIMIT; i++)
		{
			auto a = handlers[i];
			if (a)
			{
				auto r = a(args...);
				if (r)
					return 1;
			}
		}
		return 0;
	}


};
