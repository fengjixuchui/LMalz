#pragma once
#include <ia32.hpp>
#include "..\common.h"

#pragma warning(disable:4201)
#define VMX_HOST_STACK_SIZE 0x10000
struct OR_HV_VMX_REG
{
	struct {
		union {
			struct {
				union {
					UINT64 rax;
					UINT32 eax;
					UINT16 ax;
					struct {
						UINT8 al;
						UINT8 ah;
					};
				};
				union {
					UINT64 rcx;
					UINT32 ecx;
					UINT16 cx;
					struct {
						UINT8 cl;
						UINT8 ch;
					};
				};
				union {
					UINT64 rdx;
					UINT32 edx;
					UINT16 dx;
					struct {
						UINT8 dl;
						UINT8 dh;
					};
				};
				union {
					UINT64 rbx;
					UINT32 ebx;
					UINT16 bx;
					struct {
						UINT8 bl;
						UINT8 bh;
					};
				};
				union {
					UINT64 rsp;
					UINT32 esp;
					UINT16 sp;
					UINT8  spl;
				};
				union {
					UINT64 rbp;
					UINT32 ebp;
					UINT16 bp;
					UINT8  bpl;
				};
				union {
					UINT64 rsi;
					UINT32 esi;
					UINT16 si;
					UINT8  sil;
				};
				union {
					UINT64 rdi;
					UINT32 edi;
					UINT16 di;
					UINT8  dil;
				};
				union {
					UINT64 r8;
					UINT32 r8d;
					UINT16 r8w;
					UINT8  r8b;
				};
				union {
					UINT64 r9;
					UINT32 r9d;
					UINT16 r9w;
					UINT8  r9b;
				};
				union {
					UINT64 r10;
					UINT32 r10d;
					UINT16 r10w;
					UINT8  r10b;
				};
				union {
					UINT64 r11;
					UINT32 r11d;
					UINT16 r11w;
					UINT8  r11b;
				};
				union {
					UINT64 r12;
					UINT32 r12d;
					UINT16 r12w;
					UINT8  r12b;
				};
				union {
					UINT64 r13;
					UINT32 r13d;
					UINT16 r13w;
					UINT8  r13b;
				};
				union {
					UINT64 r14;
					UINT32 r14d;
					UINT16 r14w;
					UINT8  r14b;
				};
				union {
					UINT64 r15;
					UINT32 r15d;
					UINT16 r15w;
					UINT8  r15b;
				};
			};
			UINT64 gpr[16];
		};

		union {
			UINT64 rip;
			UINT32 eip;
			UINT16 ip;
		};
		union {
			rflags rflags;
			eflags eflags;
		};

		unsigned char xmm0[16];
		unsigned char xmm1[16];
		unsigned char xmm2[16];
		unsigned char xmm3[16];
		unsigned char xmm4[16];
		unsigned char xmm5[16];
		unsigned char xmm6[16];
		unsigned char xmm7[16];
		// unsigned char xmm8[16];
		// unsigned char xmm9[16];
		// unsigned char xmm10[16];
		// unsigned char xmm11[16];
		// unsigned char xmm12[16];
		// unsigned char xmm13[16];
		// unsigned char xmm14[16];
		// unsigned char xmm15[16];

		size_t cr0;
		size_t cr2;
		size_t cr3;
		cr4 cr4;
		size_t cr8;

		size_t dr0;
		size_t dr1;
		size_t dr2;
		size_t dr3;
		size_t dr4;
		size_t dr5;
		size_t dr6;
		dr7 dr7;

		UINT64 efer;
	};
};
static_assert (offsetof(OR_HV_VMX_REG, rax) == 0, "");

static_assert (offsetof(OR_HV_VMX_REG, xmm0) % 16 == 0, "");
static_assert (offsetof(OR_HV_VMX_REG, xmm1) % 16 == 0, "");
static_assert (offsetof(OR_HV_VMX_REG, xmm2) % 16 == 0, "");
static_assert (offsetof(OR_HV_VMX_REG, xmm3) % 16 == 0, "");
static_assert (offsetof(OR_HV_VMX_REG, xmm4) % 16 == 0, "");
static_assert (offsetof(OR_HV_VMX_REG, xmm5) % 16 == 0, "");
static_assert (offsetof(OR_HV_VMX_REG, xmm6) % 16 == 0, "");
static_assert (offsetof(OR_HV_VMX_REG, xmm7) % 16 == 0, "");


typedef union OR_HV_VMX_CORE_
{
	struct
	{
		
		OR_HV_VMX_REG reg;//寄存器
		UINT64 isRoot;	//host or guest

		OR_PTR<vmxon> vmxon;
		OR_PTR<vmcs> vmcs;

		//vmx_msr_bitmap* msr_bitmap;			 //先抄hv项目,先不共享,写完能跑了慢慢调

		task_state_segment_64* host_tss;	  //记得初始化
		segment_descriptor_32* host_gdt;
		segment_descriptor_interrupt_gate_64* host_idt;
		UINT64 queuedNmis;

		// vm-exit MSR store area
		struct vmx_msr_entry {
			uint32_t msr_idx;
			uint32_t _reserved;
			uint64_t msr_data;
		};
		struct {
			vmx_msr_entry perf_global_ctrl;
			vmx_msr_entry aperf;
			vmx_msr_entry mperf;
		} msr_store;
		static_assert(sizeof msr_store == 16 * 3, "");




	};

	char stack[VMX_HOST_STACK_SIZE];
}OR_HV_VMX_CORE;
typedef struct OR_HV_VMX_
{

	UINT64 core_count;

	OR_HV_VMX_CORE* cores;

	OR_PTR<ept_pml4e> eptp;

	OR_PTR<vmx_msr_bitmap> msr_bitmap;



	cr3 host_cr3;

	cr3 system_cr3;




}OR_HV_VMX;
