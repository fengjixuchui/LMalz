#pragma once

#include "vmx\vcpu_vmx.h"
#include "svm\vcpu_svm.h"
enum OR_HV_CPU_TYPE :UINT32
{
	OR_VM_CPU_TYPE_intel = 1,
	OR_VM_CPU_TYPE_AMD,
	OR_VM_CPU_TYPE_Other
};

typedef struct {
    uint64_t rax;
    uint64_t rbx;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t rbp;
    uint64_t rsp;
    uint64_t rip;
    uint64_t rflags;
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;

    segment_selector cs;
    segment_selector ds;
    segment_selector es;
    segment_selector fs;
    segment_selector gs;
    segment_selector ss;
    segment_selector ldtr;
    segment_selector tr;

    uint64_t cr0;
    uint64_t cr1;
    uint64_t cr2;
    uint64_t cr3;
    uint64_t cr4;
    uint64_t cr8;

    uint64_t dr0;
    uint64_t dr1;
    uint64_t dr2;
    uint64_t dr3;
    uint64_t dr4;
    uint64_t dr5;
    uint64_t dr6;
    uint64_t dr7;

    unsigned char xmm0[16];
    unsigned char xmm1[16];
    unsigned char xmm2[16];
    unsigned char xmm3[16];
    unsigned char xmm4[16];
    unsigned char xmm5[16];
    unsigned char xmm6[16];
    unsigned char xmm7[16];
    unsigned char xmm8[16];
    unsigned char xmm9[16];
    unsigned char xmm10[16];
    unsigned char xmm11[16];
    unsigned char xmm12[16];
    unsigned char xmm13[16];
    unsigned char xmm14[16];
    unsigned char xmm15[16];

} ORVM_CONTEXT_t;
static_assert (offsetof(ORVM_CONTEXT_t, xmm0) % 16 == 0, "");


typedef struct OR_HV_
{
	OR_HV_CPU_TYPE cpu_type;	 //CPU类型
	PVOID SystemState;			 //系统状态,用于恢复可睡眠

	union {
		OR_HV_VMX vmx;
		OR_HV_SVM svm;
	};
}OR_HV;