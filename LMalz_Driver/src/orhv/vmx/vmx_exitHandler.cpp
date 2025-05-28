#include "vmx.h"
#include "vmx_exitHandler.h"
#include "vmx.inl"


#pragma  warning( push )
#pragma warning(disable:4100)
static bool vmx_handler_exception_nmi(OR_HV_VMX_CORE* vcpu, bool) {
	vmexit_interrupt_information info = { 0 };
	UINT64 a = 0;
	__vmx_vmread(VMCS_VMEXIT_INTERRUPTION_INFORMATION, &a);
	info.flags = (UINT32)a;
	switch (info.vector)
	{
	case exception_vector::nmi:
	{
		if (info.interruption_type == interruption_type::non_maskable_interrupt)
		{
			vcpu->queuedNmis++;
			auto ctrl = read_ctrl_proc_based();
			ctrl.nmi_window_exiting = 1;
			write_ctrl_proc_based(ctrl);
			return 1;
		}
		break;
	}
	}
	return 0;
}
static bool vmx_handler_triple_fault(OR_HV_VMX_CORE* vcpu, bool) {
	cr3 guestCr3 = { 0 };
	__vmx_vmread(VMCS_GUEST_CR3, &guestCr3.flags);				//读cr3
	auto a = ALhvMMallocateMemory(0x1000);
	ALhvMMaccessPhysicalMemory(guestCr3.address_of_page_directory << 12, a, 0x1000, 0);
	KeBugCheckEx(('or' << 16) + __LINE__, (ULONG_PTR)(vcpu->reg.rip), (ULONG_PTR)(guestCr3.flags), (ULONG_PTR)a, (ULONG_PTR)__LINE__);
	//return 0;
}
// inject an NMI into the guest
inline void vmx_inject_nmi() {
	vmentry_interrupt_information interrupt_info;
	interrupt_info.flags = 0;
	interrupt_info.vector = nmi;
	interrupt_info.interruption_type = non_maskable_interrupt;
	interrupt_info.deliver_error_code = 0;
	interrupt_info.valid = 1;
	vmx_vmwrite(VMCS_CTRL_VMENTRY_INTERRUPTION_INFORMATION_FIELD, interrupt_info.flags);
}
static bool vmx_handler_nmi_window(OR_HV_VMX_CORE* vcpu, bool) {

	--vcpu->queuedNmis;

	vmx_inject_nmi();	  // inject the NMI into the guest

	if (vcpu->queuedNmis == 0) {
		// disable NMI-window exiting since we have no more NMIs to inject
		auto ctrl = read_ctrl_proc_based();
		ctrl.nmi_window_exiting = 0;
		write_ctrl_proc_based(ctrl);
	}

	// there is the possibility that a host NMI occurred right before we
	// disabled NMI-window exiting. make sure to re-enable it if this is the case.            //悟了,是指上面那个判断之后到写入关闭之前,
	if (vcpu->queuedNmis > 0) {
		auto ctrl = read_ctrl_proc_based();
		ctrl.nmi_window_exiting = 1;
		write_ctrl_proc_based(ctrl);
	}
	return 1;
}
// increment the instruction pointer after emulating an instruction
inline void vmx_increment_rip(OR_HV_VMX_CORE* vcpu) {
	// increment RIP
	auto const old_rip = vcpu->reg.rip;
	auto new_rip = old_rip + vmx_vmread(VMCS_VMEXIT_INSTRUCTION_LENGTH);

	// handle wrap-around for 32-bit addresses
	// https://patchwork.kernel.org/project/kvm/patch/20200427165917.31799-1-pbonzini@redhat.com/
	if (old_rip < (1ull << 32) && new_rip >= (1ull << 32)) {
		vmx_segment_access_rights cs_access_rights;
		cs_access_rights.flags = static_cast<uint32_t>(
			vmx_vmread(VMCS_GUEST_CS_ACCESS_RIGHTS));

		// make sure guest is in 32-bit mode
		if (!cs_access_rights.long_mode)
			new_rip &= 0xFFFF'FFFF;
	}
	vcpu->reg.rip = new_rip;

	// if we're currently blocking interrupts (due to mov ss or sti)
	// then we should unblock them since we just emulated an instruction
	auto interrupt_state = read_interruptibility_state();
	interrupt_state.blocking_by_mov_ss = 0;
	interrupt_state.blocking_by_sti = 0;
	write_interruptibility_state(interrupt_state);

	ia32_debugctl_register debugctl;
	debugctl.flags = vmx_vmread(VMCS_GUEST_DEBUGCTL);

	// if we're single-stepping, inject a debug exception
	// just like normal instruction execution would
	if (vcpu->reg.rflags.trap_flag && !debugctl.btf) {
		vmx_pending_debug_exceptions dbg_exception;
		dbg_exception.flags = vmx_vmread(VMCS_GUEST_PENDING_DEBUG_EXCEPTIONS);
		dbg_exception.bs = 1;
		vmx_vmwrite(VMCS_GUEST_PENDING_DEBUG_EXCEPTIONS, dbg_exception.flags);
	}
}
static bool vmx_handler_cpuid(OR_HV_VMX_CORE* vcpu, bool is2)
{
	if (vcpu->reg.rax == HV_IO_KEY)   //只在第二次分发处理
	{
		vcpu->reg.rax = HV_IO_SUCCEED;
		vmx_increment_rip(vcpu);
		return 1;
	}								  
	else
	{
		auto& ctx = vcpu->reg;

		int regs[4];
		__cpuidex(regs, ctx.eax, ctx.ecx);

		ctx.rax = regs[0];
		ctx.rbx = regs[1];
		ctx.rcx = regs[2];
		ctx.rdx = regs[3];

		vmx_increment_rip(vcpu);

		return 1;
	}
}
static bool vmx_handler_getsec(OR_HV_VMX_CORE* vcpu, bool is2) {
	// inject a #GP(0) since SMX is disabled in the IA32_FEATURE_CONTROL MSR
	inject_hw_exception(general_protection, 0);
	return 1;
}		  
static bool vmx_handler_invd(OR_HV_VMX_CORE* vcpu, bool is2) {
	inject_hw_exception(general_protection, 0);
	return 1;
}		  	 

static bool vmx_handler_rdtsc(OR_HV_VMX_CORE* vcpu, bool is2) {

	auto const tsc = __rdtsc();

	// return current TSC
	vcpu->reg.rax = tsc & 0xFFFFFFFF;
	vcpu->reg.rdx = (tsc >> 32) & 0xFFFFFFFF;

	vmx_increment_rip(vcpu);

	return 1;
}
static bool vmx_handler_xsetbv(OR_HV_VMX_CORE* vcpu, bool is2)
{
	// 3.2.6

	// CR4.OSXSAVE must be 1
	if (!read_effective_guest_cr4().os_xsave) {
		inject_hw_exception(invalid_opcode);
		return 1;
	}

	xcr0 new_xcr0;
	new_xcr0.flags = (vcpu->reg.rdx << 32) | vcpu->reg.eax;

	// only XCR0 is supported
	if (vcpu->reg.ecx != 0) {
		inject_hw_exception(general_protection, 0);
		return 1;
	}
	cpuid_eax_0d_ecx_00 cpuid_0d;
	__cpuidex(reinterpret_cast<int*>(&cpuid_0d), 0x0D, 0x00);

	// features in XCR0 that are supported
	auto xcr0_unsupported_mask = ~((static_cast<uint64_t>(
		cpuid_0d.edx.flags) << 32) | cpuid_0d.eax.flags);

	// #GP(0) if trying to set an unsupported bit
	if (new_xcr0.flags & xcr0_unsupported_mask) {
		inject_hw_exception(general_protection, 0);
		return 1;
	}

	// #GP(0) if clearing XCR0.X87
	if (!new_xcr0.x87) {
		inject_hw_exception(general_protection, 0);
		return 1;
	}

	// #GP(0) if XCR0.AVX is 1 while XCRO.SSE is cleared
	if (new_xcr0.avx && !new_xcr0.sse) {
		inject_hw_exception(general_protection, 0);
		return 1;
	}

	// #GP(0) if XCR0.AVX is clear and XCR0.opmask, XCR0.ZMM_Hi256, or XCR0.Hi16_ZMM is set
	if (!new_xcr0.avx && (new_xcr0.opmask || new_xcr0.zmm_hi256 || new_xcr0.zmm_hi16)) {
		inject_hw_exception(general_protection, 0);
		return 1;
	}

	// #GP(0) if setting XCR0.BNDREG or XCR0.BNDCSR while not setting the other
	if (new_xcr0.bndreg != new_xcr0.bndcsr) {
		inject_hw_exception(general_protection, 0);
		return 1;
	}

	// #GP(0) if setting XCR0.opmask, XCR0.ZMM_Hi256, or XCR0.Hi16_ZMM while not setting all of them
	if (new_xcr0.opmask != new_xcr0.zmm_hi256 || new_xcr0.zmm_hi256 != new_xcr0.zmm_hi16) {
		inject_hw_exception(general_protection, 0);
		return 1;
	}

	if (ALhvIDTsafeXsetbv(vcpu->reg.ecx, new_xcr0.flags)->find)
	{
		// TODO: assert that it was a #GP(0) that occurred, although I really
		//       doubt that any other exception could happen (according to manual).
		inject_hw_exception(general_protection, 0);
		return 1;
	}

	vmx_increment_rip(vcpu);

	return 1;
}
static bool vmx_handler_msr_read(OR_HV_VMX_CORE* vcpu, bool is2) {

	UINT64 v = 0;
	auto e = ALhvIDTsafeRdmsr(vcpu->reg.ecx, &v);
	if (e->find)
	{
		inject_hw_exception(e->vector, e->errorCode);
		return 1;
	}
	vcpu->reg.rax = v & 0xFFFF'FFFF;
	vcpu->reg.rdx = v >> 32;

	vmx_increment_rip(vcpu);
	return 1;
}
static bool vmx_handler_msr_write(OR_HV_VMX_CORE* vcpu, bool is2) {

	auto const msr = vcpu->reg.ecx;
	auto const value = (vcpu->reg.rdx << 32) | vcpu->reg.eax;
	auto e = ALhvIDTsafeWrmsr(vcpu->reg.ecx, value);
	if (e->find)
	{
		inject_hw_exception(e->vector, e->errorCode);
		return 1;
	}

	// we need to make sure to update EPT memory types if the guest
	// modifies any of the MTRR registers
	if (msr == IA32_MTRR_DEF_TYPE || msr == IA32_MTRR_FIX64K_00000 ||
		msr == IA32_MTRR_FIX16K_80000 || msr == IA32_MTRR_FIX16K_A0000 ||
		(msr >= IA32_MTRR_FIX4K_C0000 && msr <= IA32_MTRR_FIX4K_F8000) ||
		(msr >= IA32_MTRR_PHYSBASE0 && msr <= IA32_MTRR_PHYSBASE0 + 511)) {
		// update EPT memory types
		if (!read_effective_guest_cr0().cache_disable)
		{
			//太麻烦,暂不考虑,遇到再处理,先给蓝屏
			ALhvKill(vcpu->reg.rip, msr);
			//update_ept_memory_type(cpu->ept);
		}

		ALvmxInvept_asm(invept_all_context, {});
	}



	vmx_increment_rip(vcpu);
	return 1;
}

static bool handle_vmx_instruction(OR_HV_VMX_CORE* vcpu, bool is2) {
	// inject #UD for every VMX instruction since we
	// don't allow the guest to ever enter VMX operation.
	inject_hw_exception(invalid_opcode);
	return 1;
}	
static bool vmx_handler_vmon(OR_HV_VMX_CORE* vcpu, bool is2) {
	// usually a #UD doesn't trigger a vm-exit, but in this case it is possible
// that CR4.VMXE is 1 while guest shadow CR4.VMXE is 0.
	if (!read_effective_guest_cr4().vmx_enable) {
		inject_hw_exception(invalid_opcode);
		return 1;
	}

	// we are spoofing the value of the IA32_FEATURE_CONTROL MSR in
	// order to convince the guest that VMX has been disabled by BIOS.
	inject_hw_exception(general_protection, 0);
	return 1;
}
static bool vmx_handler_vmcall(OR_HV_VMX_CORE* vcpu, bool is2) {
	if (!is2)
		return 0;
	inject_hw_exception(invalid_opcode);
	return 1;
}	  
static bool vmx_handler_preemption_timer(OR_HV_VMX_CORE* vcpu, bool is2) {
	return 1;
}  	  
static bool vmx_handler_monitor_trap_flag(OR_HV_VMX_CORE* vcpu, bool is2) {
	if (!is2)
		return 0;

	disable_monitor_trap_flag();
	return 1;
}  	  
static bool vmx_handler_rdtscp(OR_HV_VMX_CORE* vcpu, bool is2) {

	unsigned int aux = 0;
	auto const tsc = __rdtscp(&aux);

	// return current TSC
	vcpu->reg.rax = tsc & 0xFFFFFFFF;
	vcpu->reg.rdx = (tsc >> 32) & 0xFFFFFFFF;
	vcpu->reg.rcx = aux;

	return 1;
}
static bool vmx_handler_ept_violation(OR_HV_VMX_CORE* vcpu, bool is2) {

	if(!is2)
		return 0;
	//先不修,给蓝屏(按道理不应该出这个)
	ALhvKill(vcpu->reg.rip, 0);
	//host模式尝试访问内存,如果内存无法访问,那么有可能是检测,注入异常进入guest
	//UINT64 hostIdt = 0;
	//__vmx_vmread(VMX_VMCS_HOST_IDTR_BASE, &hostIdt);
	//if (gALvmCpuObject.hostIdt.v == hostIdt)
	//{
	//	auto selfmap = ALvmMMgetVirtualAddress((PVOID)physical_address);
	//	//ALvmPutValue(physical_address);		   
	//	vcpu->exceptionInfo.find = 0;
	//	ALvmIDTaccessMemory((UINT8*)selfmap);
	//	if (vcpu->exceptionInfo.find)
	//	{
	//		if (vmx_inject_exception(vcpu->exceptionInfo.vector, vcpu->exceptionInfo.errorCode))
	//			return 0;
	//		else
	//		{
	//			ALvmPut("注入失败,尝试继续执行");
	//		}
	//	}

	//}
	////上锁一个个来
	//ALvmEPTupdateLock_real();
	//auto ept = ALvmxGetCurrentEpt();
	//EPT_PTE pte = { 0 };
	//pte.flags = ALvmxEPTgetPageTableInfo(physical_address, 5, ept);
	//if (!pte.flags)
	//{

	//	pte.read_access = 1;
	//	pte.write_access = 1;
	//	pte.execute_access = 1;
	//	pte.page_frame_number = physical_address >> (12 + 9);
	//	pte.memory_type = ALvmMMgetMemoryType(physical_address, 0x1000, 0);
	//	if (ALvmxEPTsetPTE(physical_address, pte.flags, ept))
	//	{
	//		ALvmEPTinvGlobal();
	//	}
	//	else
	//		ALdbgKill("设置pte失败", 0);
	//}
	//ALvmEPTupdateUnlock_real();
	//return 1;
}
#pragma  warning(  pop  ) 
static VMEXIT_HANDLERS_CLS* vmeixt_handlers = 0;
static const int vmeixt_handlers_count = 65;
bool ALvmxVmexitInit()
{
	static VMEXIT_HANDLERS_CLS handlers[vmeixt_handlers_count] = {};
	vmeixt_handlers = handlers;
	handlers[VMX_EXIT_REASON_EXCEPTION_OR_NMI].add_fun(vmx_handler_exception_nmi);	 //guest
	handlers[VMX_EXIT_REASON_TRIPLE_FAULT].add_fun(vmx_handler_triple_fault);
	handlers[VMX_EXIT_REASON_NMI_WINDOW].add_fun(vmx_handler_nmi_window);
	handlers[VMX_EXIT_REASON_EXECUTE_CPUID].add_fun(vmx_handler_cpuid);		 //仅用作探测ORHV
	handlers[VMX_EXIT_REASON_EXECUTE_GETSEC].add_fun(vmx_handler_getsec);
	handlers[VMX_EXIT_REASON_EXECUTE_INVD].add_fun(vmx_handler_invd);
	handlers[VMX_EXIT_REASON_EXECUTE_RDTSC].add_fun(vmx_handler_rdtsc);

	handlers[VMX_EXIT_REASON_EXECUTE_RDTSCP].add_fun(vmx_handler_rdtscp);
	handlers[VMX_EXIT_REASON_EXECUTE_XSETBV].add_fun(vmx_handler_xsetbv);
	handlers[VMX_EXIT_REASON_EXECUTE_RDMSR].add_fun(vmx_handler_msr_read);
	handlers[VMX_EXIT_REASON_EXECUTE_WRMSR].add_fun(vmx_handler_msr_write);

	handlers[VMX_EXIT_REASON_MONITOR_TRAP_FLAG].add_fun(vmx_handler_monitor_trap_flag);

	handlers[VMX_EXIT_REASON_EXECUTE_VMXON].add_fun(vmx_handler_vmon);
	handlers[VMX_EXIT_REASON_EXECUTE_VMCALL].add_fun(vmx_handler_vmcall);

	handlers[VMX_EXIT_REASON_VMX_PREEMPTION_TIMER_EXPIRED].add_fun(vmx_handler_preemption_timer);	  //ept

	handlers[VMX_EXIT_REASON_EPT_MISCONFIGURATION].add_fun(vmx_handler_ept_violation);	  //ept

	handlers[VMX_EXIT_REASON_EXECUTE_INVEPT].add_fun(handle_vmx_instruction);
	handlers[VMX_EXIT_REASON_EXECUTE_INVVPID].add_fun(handle_vmx_instruction);
	handlers[VMX_EXIT_REASON_EXECUTE_VMCLEAR].add_fun(handle_vmx_instruction);
	handlers[VMX_EXIT_REASON_EXECUTE_VMLAUNCH].add_fun(handle_vmx_instruction);
	handlers[VMX_EXIT_REASON_EXECUTE_VMPTRLD].add_fun(handle_vmx_instruction);
	handlers[VMX_EXIT_REASON_EXECUTE_VMPTRST].add_fun(handle_vmx_instruction);
	handlers[VMX_EXIT_REASON_EXECUTE_VMREAD].add_fun(handle_vmx_instruction);
	handlers[VMX_EXIT_REASON_EXECUTE_VMRESUME].add_fun(handle_vmx_instruction);
	handlers[VMX_EXIT_REASON_EXECUTE_VMWRITE].add_fun(handle_vmx_instruction);
	handlers[VMX_EXIT_REASON_EXECUTE_VMXOFF].add_fun(handle_vmx_instruction);
	handlers[VMX_EXIT_REASON_EXECUTE_VMFUNC].add_fun(handle_vmx_instruction);

	return 1;
}

extern "C" void ALvmxExitHandler(OR_HV_VMX_CORE * vcpu)
{

	vmx_vmexit_reason reason;
	reason.flags = static_cast<uint32_t>(vmx_vmread(VMCS_EXIT_REASON));

	auto index = reason.basic_exit_reason;

	if (index >= vmeixt_handlers_count)
		ALhvKill(0, 0);
	//一定会被处理,不然自动蓝屏
	vmeixt_handlers[index].call_all(vcpu);

	return;

}