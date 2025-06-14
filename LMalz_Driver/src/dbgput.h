#pragma once
#ifndef __NDBG
#define __DBG
#endif // !__NDBG
#if defined(__R0)||defined(__R3) 
//=========================================================R0头文件
#ifdef __R0
#include <ntifs.h>
#define dbgPut(a,...) DbgPrintEx(0,0,a,__VA_ARGS__)
#define dbgStop DbgBreakPoint
#endif // __R0
//=========================================================R3头文件
#ifdef __R3
#include <windows.h>
#include <winnt.h>
#include <stdio.h>
//---------------------------
#define dbgPut printf
#define dbgStop system
#endif
//=========================================================通用代码
#ifdef __DBG
#define ALdbgPut(a,...) dbgPut("[OMRI]-->{" __FUNCTION__ "}\t/%d/:" a "\n",__LINE__,__VA_ARGS__) 
#else 
#define ALdbgPut(...)

#endif
void dbgPut2(const char* a, ...);
#define ALdbg_0_3_put(a,...)  dbgPut2("[OMRI]-->{" __FUNCTION__ "}\t/%d/:" a "\n",__LINE__,__VA_ARGS__)


int MergeString(char* out, const char* format, ...);
inline int ALdbgPrint(char** out, const char* format, void** varg);
struct OR_DBG_ERROR
{
	int errorStringCharNumber;
	char errorString[0x100];
};
void ALdbgEmptyError(struct OR_DBG_ERROR* obj);
char* ALdbgGetError(struct OR_DBG_ERROR* obj, char* out);
int ALdbgAddError(struct OR_DBG_ERROR* obj, const char* error, ...);

//---------------------------
//=========================================================R0专属
#ifdef __R0
#ifdef __DBG
#define ALdbgStop() dbgStop()
#else
#define ALdbgStop() 
#endif
struct OR_DBG_LOG
{
	HANDLE FileHandle;
	char string[0x100];

};
bool ALdbgLogInitialization(OR_DBG_LOG* obj, const WCHAR* logFilePath);
bool ALdbgLogPut(OR_DBG_LOG* obj, const char* aa, ...);


#define ALdbgKill(a,b) KeBugCheckEx(('or'<<16)+__LINE__,(ULONG_PTR)(a),(ULONG_PTR)(b),(ULONG_PTR)__FUNCTION__,(ULONG_PTR)__LINE__)
#endif // __R0
//=========================================================R3专属
#ifdef __R3
#ifdef __DBG
#define ALdbgBox(a) MessageBoxA(0, a, "[OMRI]-->{" __FUNCTION__ "}:", 0) 
#define ALdbgStop() dbgStop("pause")
#define ALdbgKill(a) exit(a)

#else
#define ALdbgStop() 
#define ALdbgBox(a)
#endif
#endif
//---------------------------

#ifdef __DBG
#define ALdbgBoxEx(a)  MessageBoxA(0,  a,"[OMRI]:", 0) 
#define ALdbgStoput(a,...)		 ALdbgPut("[OMRI]-->{" __FUNCTION__ "}\t/%d/:" a "\t",__LINE__,__VA_ARGS__);ALdbgStop()
#define ALdbgPutValue(a)		 ALdbgPut(#a" %p",a)
#define ALdbgPutEx(a,...)		 dbgPut("[OMRI]:" a "\n",__VA_ARGS__)
#define ALdbgStoputEx(a,...)	 ALdbgStoput(a,__VA_ARGS__)
#define ALdbgGetLine()	 __LINE__
#else
#define ALdbgStoput(a,...)		
#define ALdbgPutValue(a)		
#define ALdbgPutEx(a,...)		
#define ALdbgStoputEx(a,...)	
#define ALdbgGetLine()	

#endif
void ALdbgSetMaster(const char* name);

#endif
