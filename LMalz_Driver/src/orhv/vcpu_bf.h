//#pragma once
//#include <ia32.hpp>
//
//#pragma warning(disable:4201)
//struct vcpu_cached_data {
//	// maximum number of bits in a physical address (MAXPHYSADDR)
//	uint64_t max_phys_addr;
//
//	// reserved bits in CR0/CR4
//	uint64_t vmx_cr0_fixed0;
//	uint64_t vmx_cr0_fixed1;
//	uint64_t vmx_cr4_fixed0;
//	uint64_t vmx_cr4_fixed1;
//
//	// mask of unsupported processor state components for XCR0
//	uint64_t xcr0_unsupported_mask;
//
//	// IA32_FEATURE_CONTROL
//	ia32_feature_control_register feature_control;
//	ia32_feature_control_register guest_feature_control;
//
//	// IA32_VMX_MISC
//	ia32_vmx_misc_register vmx_misc;
//
//	// CPUID 0x01
//	cpuid_eax_01 cpuid_01;
//};
//typedef union OR_VM_VCPU_CORE_
//{
//	struct {
//		union {
//			struct {
//				union {
//					UINT64 rax;
//					UINT32 eax;
//					UINT16 ax;
//					struct {
//						UINT8 al;
//						UINT8 ah;
//					};
//				};
//				union {
//					UINT64 rcx;
//					UINT32 ecx;
//					UINT16 cx;
//					struct {
//						UINT8 cl;
//						UINT8 ch;
//					};
//				};
//				union {
//					UINT64 rdx;
//					UINT32 edx;
//					UINT16 dx;
//					struct {
//						UINT8 dl;
//						UINT8 dh;
//					};
//				};
//				union {
//					UINT64 rbx;
//					UINT32 ebx;
//					UINT16 bx;
//					struct {
//						UINT8 bl;
//						UINT8 bh;
//					};
//				};
//				union {
//					UINT64 rsp;
//					UINT32 esp;
//					UINT16 sp;
//					UINT8  spl;
//				};
//				union {
//					UINT64 rbp;
//					UINT32 ebp;
//					UINT16 bp;
//					UINT8  bpl;
//				};
//				union {
//					UINT64 rsi;
//					UINT32 esi;
//					UINT16 si;
//					UINT8  sil;
//				};
//				union {
//					UINT64 rdi;
//					UINT32 edi;
//					UINT16 di;
//					UINT8  dil;
//				};
//				union {
//					UINT64 r8;
//					UINT32 r8d;
//					UINT16 r8w;
//					UINT8  r8b;
//				};
//				union {
//					UINT64 r9;
//					UINT32 r9d;
//					UINT16 r9w;
//					UINT8  r9b;
//				};
//				union {
//					UINT64 r10;
//					UINT32 r10d;
//					UINT16 r10w;
//					UINT8  r10b;
//				};
//				union {
//					UINT64 r11;
//					UINT32 r11d;
//					UINT16 r11w;
//					UINT8  r11b;
//				};
//				union {
//					UINT64 r12;
//					UINT32 r12d;
//					UINT16 r12w;
//					UINT8  r12b;
//				};
//				union {
//					UINT64 r13;
//					UINT32 r13d;
//					UINT16 r13w;
//					UINT8  r13b;
//				};
//				union {
//					UINT64 r14;
//					UINT32 r14d;
//					UINT16 r14w;
//					UINT8  r14b;
//				};
//				union {
//					UINT64 r15;
//					UINT32 r15d;
//					UINT16 r15w;
//					UINT8  r15b;
//				};
//			};
//			UINT64 gpr[16];
//		};
//
//		union {
//			UINT64 rip;
//			UINT32 eip;
//			UINT16 ip;
//		};
//		union {
//			rflags rflags;
//			eflags eflags;
//		};
//
//		unsigned char xmm0[16];
//		unsigned char xmm1[16];
//		unsigned char xmm2[16];
//		unsigned char xmm3[16];
//		unsigned char xmm4[16];
//		unsigned char xmm5[16];
//		unsigned char xmm6[16];
//		unsigned char xmm7[16];
//		// unsigned char xmm8[16];
//		// unsigned char xmm9[16];
//		// unsigned char xmm10[16];
//		// unsigned char xmm11[16];
//		// unsigned char xmm12[16];
//		// unsigned char xmm13[16];
//		// unsigned char xmm14[16];
//		// unsigned char xmm15[16];
//
//		size_t cr0;
//		size_t cr2;
//		size_t cr3;
//		cr4 cr4;
//		size_t cr8;
//
//		size_t dr0;
//		size_t dr1;
//		size_t dr2;
//		size_t dr3;
//		size_t dr4;
//		size_t dr5;
//		size_t dr6;
//		dr7 dr7;
//
//		UINT64 efer;
//
//		UINT64 Reserved1;
//
//		UINT64 isHost;
//
//		vcpu_cached_data cached;
//
//
//		vmxon* vmxon;
//		vmcs* vmcs;
//		vmx_msr_bitmap* msr_bitmap;
//
//		task_state_segment_64* host_tss;
//		segment_descriptor_32* host_gdt;
//		segment_descriptor_interrupt_gate_64* host_idt;
//		//OR_EXCEPTION_INFO exceptionInfo;
//
//	};
//#define ORVM_STACK_SIZE 0x10000
//	char stack[ORVM_STACK_SIZE];
//}vcpu;
//struct hv_data {
//	// dynamically allocated array of vcpus
//	unsigned long vcpu_count;
//	vcpu* vcpus;
//
//	pml4e_64* host_page_table;
//	cr3 host_cr3;
//
//	pml4e_64* system_page_table;
//	cr3 system_cr3;
//
//	PVOID driver_base;
//	UINT64 driver_size;
//
//	PVOID SystemState;
//
//
//};
//enum OR_VM_CPU_TYPE :UINT32
//{
//	OR_VM_CPU_TYPE_intel = 1,
//	OR_VM_CPU_TYPE_AMD,
//	OR_VM_CPU_TYPE_Other
//};
//typedef struct OR_VMX_VCPU_CORE_
//{
//
//	vmxon* vmxon;
//	vmcs* vmcs;
//
//}OR_VMX_VCPU_CORE;
//typedef struct OR_SVM_VCPU_CORE_
//{
//
//}OR_SVM_VCPU_CORE;
//typedef union OR_HV_CORE_
//{
//	struct
//	{
//		union
//		{
//			OR_VMX_VCPU_CORE vmx;
//			OR_SVM_VCPU_CORE svm;
//		};
//
//		UINT64 isHost;
//
//
//
//
//
//		task_state_segment_64* host_tss;
//		segment_descriptor_32* host_gdt;
//		segment_descriptor_interrupt_gate_64* host_idt;
//
//
//
//
//	};
//#define ORVM_STACK_SIZE 0x10000
//	char stack[ORVM_STACK_SIZE];
//}OR_HV_CORE;
//
//
//
//
//typedef struct OR_HV_
//{
//	OR_VM_CPU_TYPE cpu_type;
//
//	UINT32 core_count;
//	OR_HV_CORE* cores;
//
//
//
//
//	pml4e_64* host_page_table;// host页表	  所有核心共用同一个表		 
//	cr3 host_cr3;
//
//	pml4e_64* system_page_table;// 
//	cr3 system_cr3;
//
//	union {
//		struct {
//			vmx_msr_bitmap* msr_bitmap;
//		}vmx;
//		struct {
//		}svm;
//	};
//
//
//
//}OR_HV;