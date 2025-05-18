#pragma once
#include <ia32.hpp>
#pragma warning(disable:4201)

#define SVM_HOST_STACK_SIZE 0x10000

typedef union OR_HV_SVM_CORE_
{
	struct
	{
		UINT64 isRoot;





		task_state_segment_64* host_tss;
		segment_descriptor_32* host_gdt;
		segment_descriptor_interrupt_gate_64* host_idt;
	};

	char stack[SVM_HOST_STACK_SIZE];

}OR_HV_SVM_CORE;

typedef struct OR_HV_SVM_
{
	UINT64 core_count;

	OR_HV_SVM_CORE* cores;


	cr3 host_cr3;

	cr3 system_cr3;


}OR_HV_SVM;