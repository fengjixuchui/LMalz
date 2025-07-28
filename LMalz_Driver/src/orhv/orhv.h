//r3����ؽӿڿ�һ�鲻��ͷ�ļ����ļ����߹���
//r3����ؽӿڿ�һ�鲻��ͷ�ļ����ļ����߹���
//r3����ؽӿڿ�һ�鲻��ͷ�ļ����ļ����߹���
#pragma once


//#include <ntddk.h>


//#include "common.h"
#include "vcpu.h"
#include "idt.h"
//�����淶
//ȫ�ֱ���g��ͷ��AL��С��ĸ��ʾģ���ӹ���������
//��̬ȫ�ֲ���AL���»��ߴ���
//����������ȫ�ֱ�������g
//����ֵ��bool��������ؼ�һ���ܻ�ȡ������

#define HV_IO_KEY 'orhv'
#define HV_IO_SUCCEED 'ORhv'
#define HV_IO_FAILED 'orHV'






//--------------------
extern "C" void _sgdt(segment_descriptor_register_64* gdtr);
extern "C" void _lgdt(segment_descriptor_register_64* gdtr);
//hverrput.cpp
char* _hvErrSetString(const char* format, ...);
char* _hvErrAddString(const char* format, ...);
char* _hvErrGetString();
#define ALhvSetErr(a,...) _hvErrSetString( __FUNCTION__ "(%d):\t" a,__LINE__, __VA_ARGS__)
#define ALhvAddErr(a,...) _hvErrAddString(" FROM:" __FUNCTION__ "(%d):\t" a,__LINE__, __VA_ARGS__)
#define ALhvGetErr() _hvErrGetString()
#define ALhvKill(a,b) KeBugCheckEx(('or'<<16)+__LINE__,(ULONG_PTR)(a),(ULONG_PTR)(b),(ULONG_PTR)__FUNCTION__,(ULONG_PTR)__LINE__)

#define dbgPut(a,...) DbgPrintEx(0,0,a,__VA_ARGS__)

#define ALhvPutLog(a,...) dbgPut("[OMRI]-->{" __FUNCTION__ "}\t/%d/:" a "\n",__LINE__,__VA_ARGS__) 


//-------------------------orhv.cpp
bool ALhvIsHost();	 //�жϵ�ǰģʽ�Ƿ�����guest
extern OR_HV gALhvData;
//��ʼ��VCPU
template<typename VCPU>
bool ALhvInitialization(VCPU* vcpu);

inline bool ALhvIsIntelCPU();
inline bool ALhvIsAMDCPU();
int ALhvGetCoreCount();
int ALhvGetCurrVcoreIndex();
uint64_t ALhvGetSegmentBase(
	segment_descriptor_register_64 const& gdtr,
	segment_selector const selector);


cr3 ALhvGetGuestCr3();




//---------------------hvmem.cpp
bool ALhvMMinitPool();

bool ALhvMMCopyData(PVOID desAdd, PVOID souAdd, UINT64 size);

PUINT8 ALhvMMallocateMemory(INT64 sizeByByte);
bool ALhvMMaccessPhysicalMemory(UINT64 PhysicalAddress, void* bufferAddress, UINT64 size, int isWrite);
bool ALhvMMsetAllPA(pml4e_64* Hostcr3);
bool ALhvMMrebuildPath(pml4e_64* page_table_old, pml4e_64* page_table, PVOID vadd, UINT64 size);
bool ALhvMMrebuildMemPoolPath(pml4e_64* page_table_old, pml4e_64* page_table);

cr3 ALhvMMgetCr3ByObject(PEPROCESS process);
PVOID ALhvMMgetVA(UINT64 pa);
UINT64 ALhvMMgetPA(PVOID add);
/// <summary>
/// ǳ����cr3
/// </summary>
/// <param name="real_cr3">���뱣֤�ǿ��õ�cr3</param>
/// <param name="fake_table">����</param>
/// <returns></returns>
bool ALhvMMprepare_fake_page_tables(__in cr3 real_cr3, __out pml4e_64** fake_table);
//��ϵͳcr3����һ��hostcr3
bool ALhvMMgetHostCr3(__in cr3 system_cr3, __out cr3* host_cr3_out);

UINT8 ALhvMMgetMemoryType(UINT64 address, UINT64 size, bool update = 0);

bool ALhvMMreadGuestMemroy(PVOID buff, PVOID add, UINT64 size);



//----------------------------hvasm.asm
extern "C" UINT64 ALhvGetContext_asm(ORVM_CONTEXT_t*);


//-------------------------gdt.cpp
segment_descriptor_32* ALhvGDTgetHostGDT();