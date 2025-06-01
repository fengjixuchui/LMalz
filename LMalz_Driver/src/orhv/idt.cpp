#include "orhv.h"
//#include "vmx\vmx.inl"
//#include "vmx\vmx.h"
#include "handler_class.h"
#include "idt.h"


static OR_EXCEPTION_INFO* g_idt_infos = 0;


typedef HANDLER_CLS<IDT_HANDLER_T> IDT_HANDLERS;
IDT_HANDLERS* idt_handlers;
//IDT_HANDLER_T IDT_HANDLERS::_null_fun_v = 0;

extern "C" void ALhvIDT_interrupt_handler_00_DE_asm();
extern "C" void ALhvIDT_interrupt_handler_01_DB_asm();
extern "C" void ALhvIDT_interrupt_handler_02_NMI_asm();
extern "C" void ALhvIDT_interrupt_handler_03_BP_asm();
extern "C" void ALhvIDT_interrupt_handler_04_OF_asm();
extern "C" void ALhvIDT_interrupt_handler_05_BR_asm();
extern "C" void ALhvIDT_interrupt_handler_06_UD_asm();
extern "C" void ALhvIDT_interrupt_handler_07_NM_asm();
extern "C" void ALhvIDT_interrupt_handler_08_DF_asm();

extern "C" void ALhvIDT_interrupt_handler_10_TS_asm();
extern "C" void ALhvIDT_interrupt_handler_11_NP_asm();
extern "C" void ALhvIDT_interrupt_handler_12_SS_asm();
extern "C" void ALhvIDT_interrupt_handler_13_GP_asm();
extern "C" void ALhvIDT_interrupt_handler_14_PF_asm();

extern "C" void ALhvIDT_interrupt_handler_16_MF_asm();
extern "C" void ALhvIDT_interrupt_handler_17_AC_asm();
extern "C" void ALhvIDT_interrupt_handler_18_MC_asm();
extern "C" void ALhvIDT_interrupt_handler_19_XF_asm();
extern "C" void ALhvIDT_interrupt_handler_20_VE_asm();

extern "C" void ALhvIDT_interrupt_handler_21_CP_asm();
typedef struct
{
	uint16_t offset_low;
	uint16_t segment_selector;
	union
	{
		struct
		{
			uint32_t interrupt_stack_table : 3;
			uint32_t must_be_zero_0 : 5;
			uint32_t type : 4;
			uint32_t must_be_zero_1 : 1;
			uint32_t descriptor_privilege_level : 2;
			uint32_t present : 1;
			uint32_t offset_middle : 16;
		};

		uint32_t flags;
	};
	uint32_t offset_high;
	uint32_t reserved;
} IDTE;
static void ALvmSetIdteFunAdd(IDTE* idte_, UINT64 FunctionAdd)
{
	/*idte_->interrupt_stack_table = 0;
	idte_->segment_selector = ALvmGetCs();
	idte_->must_be_zero_0 = 0;
	idte_->type = SEGMENT_DESCRIPTOR_TYPE_INTERRUPT_GATE;
	idte_->must_be_zero_1 = 0;
	idte_->descriptor_privilege_level = 0;
	idte_->present = 1;
	idte_->reserved = 0;*/
	IDTE* pIdte = (IDTE*)idte_;
	pIdte->offset_low = (FunctionAdd >> 0) & 0xFFFF;
	pIdte->offset_middle = (FunctionAdd >> 16) & 0xFFFF;
	pIdte->offset_high = (FunctionAdd >> 32) & 0xFFFFFFFF;
	//idteAdd.add1 = pIdte->offset_low ;
	//idteAdd.add2 = pIdte->offset_middle ;
	//idteAdd.add3 = pIdte->offset_high;
	//ALdbgPutValue(idteAdd.add);
}	 
static OR_EXCEPTION_INFO* get_curr_info()
{
	if (g_idt_infos)
		return &g_idt_infos[ALhvGetCurrVcoreIndex()];
	else
		return 0;
}
template <typename RET_V, typename CALL>
static RET_V ALhvIDTread(CALL f, RET_V* add)
{
	auto info = get_curr_info();
	info->monitor = 1;
	info->find = 0;
	RET_V v = f(add);
	info->monitor = 0;
	return v;
}		  

extern"C" UINT64 ALhvIDTreadMemory64_asm(PUINT64 add);
extern"C" UINT32 ALhvIDTreadMemory32_asm(PUINT32 add);
extern"C" UINT16 ALhvIDTreadMemory16_asm(PUINT16 add);
extern"C" UINT8 ALhvIDTreadMemory8_asm(PUINT8 add);
extern"C" UINT64 ALhvIDTwriteMemory64_asm(PUINT64 to, PUINT64 from);
extern"C" UINT32 ALhvIDTwriteMemory32_asm(PUINT32 to, PUINT32 from);
extern"C" UINT16 ALhvIDTwriteMemory16_asm(PUINT16 to, PUINT16 from);
extern"C" UINT8 ALhvIDTwriteMemory8_asm(PUINT8 to, PUINT8 from);


UINT8 ALhvIDTsafeRead8(PUINT8 add)
{
	return ALhvIDTread(ALhvIDTreadMemory8_asm, add);
}
UINT32 ALhvIDTsafeRead32(PUINT32 add)
{
	return ALhvIDTread(ALhvIDTreadMemory32_asm, add);
}
UINT16 ALhvIDTsafeRead16(PUINT16 add)
{
	return ALhvIDTread(ALhvIDTreadMemory16_asm, add);
}
UINT64 ALhvIDTsafeRead64(PUINT64 add)
{
	return ALhvIDTread(ALhvIDTreadMemory64_asm, add);
}

bool ALhvIDTsafeCopy(PVOID desAdd, PVOID souAdd, UINT64 size)
{
	auto info = get_curr_info();
	info->monitor = 1;
	info->find = 0;

	for (UINT64 i = 0; i < size / 8; i++)
	{
		ALhvIDTwriteMemory64_asm(&((PUINT64)desAdd)[i], &((PUINT64)souAdd)[i]);
		if (info->find)
		{
			info->monitor = 0;
			return 0;
		}
	}
	if ((size & 0b111) >= 4)
	{
		auto a = (size & ~0b111);
		ALhvIDTwriteMemory32_asm(&((PUINT32)desAdd)[a / 4], &((PUINT32)souAdd)[a / 4]);
		if (info->find)
		{
			info->monitor = 0;
			return 0;
		}
	}
	for (UINT64 j = size & ~0b11; j < size; j++)
	{
		ALhvIDTwriteMemory8_asm(&((PUINT8)desAdd)[j], &((PUINT8)souAdd)[j]);
		if (info->find)
		{
			info->monitor = 0;
			return 0;
		}
	}
	info->monitor = 0;
	return 1;
}
bool ALhvIDTisMemoryValid(PVOID add)
{
	auto info = get_curr_info();
	info->monitor = 1;
	info->find = 0;
	ALhvIDTreadMemory8_asm((PUINT8)add);
	info->monitor = 0;
	return !info->find;
}

inline bool ALhvIDTexception()
{
	auto info = get_curr_info();
	return info->find;
}

inline UINT64 ALhvIDTgetVector()
{
	auto info = get_curr_info();
	return info->vector;
}
inline UINT64 ALhvIDTgetCode()
{
	auto info = get_curr_info();
	return info->errorCode;
}


extern "C" void ALhvIDTxsetbv_asm(uint32_t idx, uint64_t value);

const OR_EXCEPTION_INFO* ALhvIDTsafeXsetbv(uint32_t idx, uint64_t value)
{
	auto info = get_curr_info();
	info->monitor = 1;
	info->find = 0;
	ALhvIDTxsetbv_asm(idx, value);
	info->monitor = 0;
	return info;
}
//template <typename T, typename... Args>
//static auto call_asm(T fun, Args... args)
//{
//	auto info = get_curr_info();
//	info->monitor = 1;
//	info->find = 0;
//	auto r = fun(args...);
//	info->monitor = 0;
//	return r;
//}		  
//template <typename T, typename... Args>
//static void call_asm(T fun, Args... args)
//{
//	auto info = get_curr_info();
//	info->monitor = 1;
//	info->find = 0;
//	fun(args...);
//	info->monitor = 0;
//	return;
//}
//bool ALhvIDTsafeXsetbv(uint32_t idx, uint64_t value)
//{
//	call_asm(ALhvIDTxsetbv_asm, idx, value);
//	return !ALhvIDTexception();
//}

extern "C" uint64_t ALhvIDTrdmsr_asm(uint32_t msr);
extern "C" void ALhvIDTwrmsr_asm(uint32_t msr, UINT64 v);



const OR_EXCEPTION_INFO* ALhvIDTsafeRdmsr(__in uint32_t msr, __out UINT64* value)
{
	auto info = get_curr_info();
	info->monitor = 1;
	info->find = 0;
	auto v = ALhvIDTrdmsr_asm(msr);
	*value = v;
	info->monitor = 0;
	return info;
}			   
const OR_EXCEPTION_INFO* ALhvIDTsafeWrmsr(__in uint32_t msr, __in UINT64 value)
{
	auto info = get_curr_info();
	info->monitor = 1;
	info->find = 0;
	ALhvIDTwrmsr_asm(msr, value);
	info->monitor = 0;
	return info;
}



static bool safe_handler(trap_frame* frame)
{
	auto info = get_curr_info();
	if (!info->monitor)
		return 0;
	else
	{
		info->find = 1;
		info->vector = (UINT32)frame->vector;
		info->errorCode = (UINT32)frame->error;

		frame->rip = *((PUINT64)frame->rsp);
		frame->rsp += 8;

		return 1;
	}
}
segment_descriptor_interrupt_gate_64* ALhvIDT_prepare_host_idt()
{
	static IDT_HANDLERS s_idt_handlers;
	idt_handlers = &s_idt_handlers;

	g_idt_infos = (GT(g_idt_infos))ALhvMMallocateMemory(ALhvGetCoreCount() * sizeof(GT(*g_idt_infos)));

	//添加安全处理函数
	ALhvIdtAddHandler(safe_handler);

	segment_descriptor_register_64 idtr = { 0 };
	__sidt(&idtr);
	auto ret = ALhvMMallocateMemory(0x1000);
	if (ret && idtr.base_address)
	{
		memcpy(ret, (PVOID)idtr.base_address, 0x1000);

		ALvmSetIdteFunAdd(&((IDTE*)ret)[0], (UINT64)&ALhvIDT_interrupt_handler_00_DE_asm);
		ALvmSetIdteFunAdd(&((IDTE*)ret)[1], (UINT64)&ALhvIDT_interrupt_handler_01_DB_asm);
		ALvmSetIdteFunAdd(&((IDTE*)ret)[2], (UINT64)&ALhvIDT_interrupt_handler_02_NMI_asm);
		ALvmSetIdteFunAdd(&((IDTE*)ret)[3], (UINT64)&ALhvIDT_interrupt_handler_03_BP_asm);
		ALvmSetIdteFunAdd(&((IDTE*)ret)[4], (UINT64)&ALhvIDT_interrupt_handler_04_OF_asm);
		ALvmSetIdteFunAdd(&((IDTE*)ret)[5], (UINT64)&ALhvIDT_interrupt_handler_05_BR_asm);
		ALvmSetIdteFunAdd(&((IDTE*)ret)[6], (UINT64)&ALhvIDT_interrupt_handler_06_UD_asm);
		ALvmSetIdteFunAdd(&((IDTE*)ret)[7], (UINT64)&ALhvIDT_interrupt_handler_07_NM_asm);
		ALvmSetIdteFunAdd(&((IDTE*)ret)[8], (UINT64)&ALhvIDT_interrupt_handler_08_DF_asm);

		ALvmSetIdteFunAdd(&((IDTE*)ret)[10], (UINT64)&ALhvIDT_interrupt_handler_10_TS_asm);
		ALvmSetIdteFunAdd(&((IDTE*)ret)[11], (UINT64)&ALhvIDT_interrupt_handler_11_NP_asm);
		ALvmSetIdteFunAdd(&((IDTE*)ret)[12], (UINT64)&ALhvIDT_interrupt_handler_12_SS_asm);
		ALvmSetIdteFunAdd(&((IDTE*)ret)[13], (UINT64)&ALhvIDT_interrupt_handler_13_GP_asm);
		ALvmSetIdteFunAdd(&((IDTE*)ret)[14], (UINT64)&ALhvIDT_interrupt_handler_14_PF_asm);

		ALvmSetIdteFunAdd(&((IDTE*)ret)[16], (UINT64)&ALhvIDT_interrupt_handler_16_MF_asm);
		ALvmSetIdteFunAdd(&((IDTE*)ret)[17], (UINT64)&ALhvIDT_interrupt_handler_17_AC_asm);
		ALvmSetIdteFunAdd(&((IDTE*)ret)[18], (UINT64)&ALhvIDT_interrupt_handler_18_MC_asm);
		ALvmSetIdteFunAdd(&((IDTE*)ret)[19], (UINT64)&ALhvIDT_interrupt_handler_19_XF_asm);
		ALvmSetIdteFunAdd(&((IDTE*)ret)[20], (UINT64)&ALhvIDT_interrupt_handler_20_VE_asm);

		ALvmSetIdteFunAdd(&((IDTE*)ret)[21], (UINT64)&ALhvIDT_interrupt_handler_21_CP_asm);


		return (segment_descriptor_interrupt_gate_64*)ret;
	}
	else
		return 0;
}
//UINT64 ALhvGetSystemIdt(int vector)
//{
//	segment_descriptor_register_64 idtr = { 0 };
//	__sidt(&idtr);
//	UINT64 idtBase = idtr.base_address;
//	if (idtr.base == gALvmCpuObject.hostIdt.v)
//	{
//		__vmx_vmread(VMCS_GUEST_IDTR_BASE, &idtBase);
//	}
//	PUINT64 idtTP = (PUINT64)idtBase;
//	if (idtTP)
//		return ALvmGetIdteFunAdd(&idtTP[vector * 2]);
//	else
//		ALdbgKill("获取system idt 失败", 0);
//}

extern "C" UINT64 ALhvIdtHandler(trap_frame * frame)
{
	auto h_r = idt_handlers->call_handlers(frame);
	if (h_r)
		return 0;
	else
		ALhvKill(frame->rip, 0);
	/*{

		auto a = ALhvGetSystemIdt(frame->vector);
		return a;
	}*/
}

bool ALhvIdtAddHandler(IDT_HANDLER_T handler, bool toend)
{
	return idt_handlers->add_handler(handler, toend);
}	 
bool ALhvIdtDelHandler(IDT_HANDLER_T handler)
{
	return idt_handlers->sub_handler(handler);
}

