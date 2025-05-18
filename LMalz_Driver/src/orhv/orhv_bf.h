////r3的相关接口开一组不用头文件的文件两边共享
////r3的相关接口开一组不用头文件的文件两边共享
////r3的相关接口开一组不用头文件的文件两边共享
//#pragma once
//
//
////#include <ntddk.h>
//#include <intrin.h>
//#include <ntifs.h>
//#include <ntimage.h>
//
//#include "vcpu.h"
////命名规范
////全局变量g开头接AL带小字母提示模块后加功能性名称
////静态全局不带AL以下划线代替
////函数名类似全局变量不带g
////返回值是bool的如果返回假一定能获取到错误
////以下所有API均属于Intel CPU,AMD另开文件
//
////--------------------
//
//
//
//#define GT(a) decltype(a)
//
//typedef struct _CPUID_reg
//{
//	int eax;
//	int ebx;
//	int ecx;
//	int edx;
//} CPUID_reg, * PCPUID_reg;
//
//#define GB_TO_BYTE(a) ((UINT64)a*1024LL*1024LL*1024LL)
//#define MB_TO_BYTE(a) ((UINT64)a*1024LL*1024LL)
//#define ALIGNS_MB(value,n_MB) (((UINT64)value + (MB_TO_BYTE(n_MB)-1)) & ~(MB_TO_BYTE(n_MB)-1)) 
//#define ALIGNS_PAGE(value) (((UINT64)(value) + 0xfff) & ~0xfffLL)
//
//
////orhv.cpp
//bool ALhvIsHost();	 //判断当前模式是否属于guest
//
//extern hv_data ghv;
//
//
////errput.cpp
//char* _hvErrSetString(const char* format, ...);
//char* _hvErrAddString(const char* format, ...);
//#define ALhvSetErr(a,...) _hvErrSetString( __FUNCTION__ "(%d):\t" a,__LINE__, __VA_ARGS__)
//#define ALhvAddErr(a,...) _hvErrAddString("\nFROM:" __FUNCTION__ "(%d):\t" a,__LINE__, __VA_ARGS__)
//#define ALhvGetErr() _hvErrGetString()
//#define ALhvKill(a,b) KeBugCheckEx(('or'<<16)+__LINE__,(ULONG_PTR)(a),(ULONG_PTR)(b),(ULONG_PTR)__FUNCTION__,(ULONG_PTR)__LINE__)
//
////hvmem.cpp
//bool ALhvMMinitPool();
//
//PUINT8 ALhvMMallocateMemory(UINT64 sizeByByte);
//
//bool ALhvMMsetAllPA(pml4e_64* Hostcr3);
//bool ALhvMMrebuildPath(pml4e_64* page_table_old, pml4e_64* page_table, PVOID vadd, UINT64 size);
//bool ALhvMMrebuildMemPoolPath(pml4e_64* page_table_old, pml4e_64* page_table);
//
//
//
////vmx.cpp
//bool ALvmxCoreStart(vcpu* vcpu);
//
////idt.cpp
//segment_descriptor_interrupt_gate_64* ALvmx_prepare_host_idt();
//
//
////vmx_.asm
//
//extern "C" segment_selector ALvmx_read_cs_asm();
//extern "C" segment_selector ALvmx_read_ss_asm();
//extern "C" segment_selector ALvmx_read_ds_asm();
//extern "C" segment_selector ALvmx_read_es_asm();
//extern "C" segment_selector ALvmx_read_fs_asm();
//extern "C" segment_selector ALvmx_read_gs_asm();
//extern "C" segment_selector ALvmx_read_tr_asm();
//extern "C" segment_selector ALvmx_read_ldtr_asm();
//
//extern "C" void ALvmx_write_ds_asm(uint16_t selector);
//extern "C" void ALvmx_write_es_asm(uint16_t selector);
//extern "C" void ALvmx_write_fs_asm(uint16_t selector);
//extern "C" void ALvmx_write_gs_asm(uint16_t selector);
//extern "C" void ALvmx_write_tr_asm(uint16_t selector);
//extern "C" void ALvmx_write_ldtr_asm(uint16_t selector);