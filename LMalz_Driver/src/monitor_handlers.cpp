#include "monitor_handlers.h"
#include "send.h"
#include "disassemble.h"
//不安全,最好自己获取guest gsbase
static auto get_guest_curr_tid()
{
	return (UINT32)(UINT64)PsGetCurrentThreadId();
}


static void enter_handler(OR_HV_VMX_CORE* vcpu)
{
	auto tid = get_guest_curr_tid();
	ALsdSend_enter(tid, vcpu->reg.rip);
}			
static void exit_handler(OR_HV_VMX_CORE* vcpu)
{
	auto tid = get_guest_curr_tid();
	ALsdSend_exit(tid, vcpu->reg.rip, ALvmxMNTgetLastMTFaddress());
}
#define MH_ERROR_READ_GUEST_MEM	0xc00005

static bool is_special_reg(ZydisRegister reg)
{
	if (reg >= REG(CR0) && reg <= REG(CR15))
		return 1;
	else if (reg >= REG(DR0) && reg <= REG(DR15))
		return 1;
	//else if (reg == REG(TR) || reg == REG(IDTR) || reg == REG(GDTR) || reg == REG(LDTR))
	//	return 1;
	//else if (reg == REG(XCR0))
	//	return 1;
	else if (reg == REG(FS) || reg == REG(GS) || reg == REG(ES) || reg == REG(DS) || reg == REG(CS) || reg == REG(SS))
		return 1;
	else
		return 0;

}



static void mtf_handler(OR_HV_VMX_CORE* vcpu)
{
	auto tid = get_guest_curr_tid();
	auto rip = vcpu->reg.rip;
	UINT8 code[16];
	auto r = ALhvMMreadGuestMemroy(code, (PVOID)rip, sizeof(code));
	if (!r)
	{
		ALsdSend_error(tid, rip, MH_ERROR_READ_GUEST_MEM);
		return;
	}
	ASM a(rip, code);
	auto mne = a.get_mnemonic();
	auto send_mov_to_reg = [tid, rip, &code, &a](UINT64 n, UINT64 o)
	{
		ALsdSend_ins_mov_to_reg(tid, rip, code, a.get_code_size(), n, o);
	};
	auto send_save_reg = [tid, rip, &code, &a](UINT64 v)
	{
		ALsdSend_ins_save_reg(tid, rip, code, a.get_code_size(), v);
	};
	switch (mne)
	{
	case INS(MOV):
	{
		if (a.get_operand_count() >= 2)
		{
			if (a.get_operand_type(1) == ASM::reg && a.get_operand_type(2) == ASM::reg)
			{
				if (is_special_reg(a.get_operand_reg(1)->value))
				{
					send_mov_to_reg(0, 0);
				}
				else if (is_special_reg(a.get_operand_reg(2)->value))
				{
					send_save_reg(0);
				}
			}
			else if (a.get_operand_type(1) == ASM::reg && a.get_operand_type(2) == ASM::mem)
			{
				if (a.get_operand_mem(2)->segment == REG(FS) || a.get_operand_mem(2)->segment == REG(GS))
				{
					ALsdSend_ins_r_fs_gs(tid, rip, code, a.get_code_size(), 0, 0, 0);
				}
			}
			else if (a.get_operand_type(1) == ASM::mem && a.get_operand_type(2) == ASM::reg)
			{
				if (a.get_operand_mem(1)->segment == REG(FS) || a.get_operand_mem(1)->segment == REG(GS))
				{
					ALsdSend_ins_w_fs_gs(tid, rip, code, a.get_code_size(), 0, 0, 0, 0);
				}
			}
		}
		break;
	}
	//case INS(PUSHFQ):
	//case INS(PUSHF):
	case INS(SIDT):
	case INS(SGDT):
	case INS(SLDT):
	case INS(STR):
	case INS(RDPMC):
	case INS(RDTSC):
	case INS(RDTSCP):
	{
		send_save_reg(0);
		break;
	}
	case INS(STI):
	case INS(CLI):
	//case INS(POPF):
	//case INS(POPFQ):
	case INS(LIDT):
	case INS(LGDT):
	case INS(LLDT):
	case INS(LTR):
	{
		send_mov_to_reg(0, 0);
		break;
	}
	case INS(RDMSR):
	{
		ALsdSend_ins_rdmsr(tid, rip, code, a.get_code_size(), 0, 0);
		break;
	}
	case INS(WRMSR):
	{
		ALsdSend_ins_wrmsr(tid, rip, code, a.get_code_size(), 0, 0, 0);
		break;
	}
	case INS(SFENCE):
	case INS(LFENCE):
	case INS(MFENCE):
	{
		ALsdSend_ins_fence(tid, rip, code, a.get_code_size());
		break;
	}
	case INS(JMP):
	case INS(CALL):
	case INS(RET):
	{
		if (a.is_far())
		{
			UINT8 v[6];
			ALsdSend_ins_far(tid, rip, code, a.get_code_size(), v);
		}
		break;
	}
	case INS(SYSENTER):
	case INS(SYSEXIT):
	case INS(SYSCALL):
	case INS(SYSRET):
	{
		ALsdSend_ins_sys(tid, rip, code, a.get_code_size());
		break;
	}
	case INS(UD2):
	case INS(INT1):
	case INS(INT3):
	case INS(INTO):
	{
		ALsdSend_ins_ud2_int13o(tid, rip, code, a.get_code_size());
		break;
	}
	case INS(INVD):
	case INS(WBINVD):
	{
		ALsdSend_ins_invd(tid, rip, code, a.get_code_size());
		break;
	}
	case INS(INVLPG):
	{
		ALsdSend_ins_invlpg(tid, rip, code, a.get_code_size(),0);
		break;
	}
	case INS(INT):
	{
		ALsdSend_ins_int(tid, rip, code, a.get_code_size(), 0);
		break;
	}
	case INS(INVEPT):
	case INS(INVVPID):
	case INS(VMCLEAR):
	case INS(VMLAUNCH):
	case INS(VMPTRLD):
	case INS(VMPTRST):
	case INS(VMREAD):
	case INS(VMRESUME):
	case INS(VMWRITE):
	case INS(VMXOFF):
	case INS(VMFUNC):
	{
		ALsdSend_ins_vmx(tid, rip, code, a.get_code_size());
		break;
	}
	case INS(IN):
	{
		ALsdSend_ins_in_out(tid, rip, code, a.get_code_size(), 1);
		break;
	}
	case INS(OUT):
	{
		ALsdSend_ins_in_out(tid, rip, code, a.get_code_size(), 0);
		break;
	}
	case INS(CPUID):
	{
		ALsdSend_ins_cpuid(tid, rip, code, a.get_code_size(), 0, 0);
		break;
	}
	default:
		break;
	}

}



bool ALmhdInit()
{
	return ALsdInit();
}

PVOID ALmhdGetInfoPool()
{
	return ALsdGetPool();
}
UINT64 ALmhdGetInfoPoolSize()
{
	return ALsdGetPool_sz();
}


void ALmhdCallback(OR_HV_VMX_CORE* vcpu, MONITOR_CALLBACK_TYPE type)
{
	switch (type)
	{
	case MONITOR_CALLBACK_TYPE_enter:
		//ALdbgPut("进入地址:%p", vcpu->reg.rip);
		enter_handler(vcpu);
		break;
	case MONITOR_CALLBACK_TYPE_exit:
		//ALdbgPut("调用函数:%p", vcpu->reg.rip);
		exit_handler(vcpu);
		break;
	case MONITOR_CALLBACK_TYPE_mtf:
		//ALdbgPut("单步:%p", vcpu->reg.rip);
		if (vcpu == 0)
			mtf_handler(vcpu);
		break;
	default:
		ALdbgKill(0, 0);
		break;
	}
}