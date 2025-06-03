#include "../orhv.h"
#include "vmx.h"
#include "ept.h"
#include "vmx.inl"
OR_HV_VMX* gALvmxVCPU = 0;
extern "C" UINT16 ALvmx_guest_vpid = 1;
char* ALvmxGetVmcsError()
{
	size_t out = 0;
	char* errorString = 0;
	auto ret = __vmx_vmread(VMCS_VM_INSTRUCTION_ERROR, &out);
	if (ret)
	{
		if (ret == 1)
		{
			//ALvmPut("get vmcs error fail :%s", ALvmxGetVmcsError());
			errorString = "get vmcs error fail";
			return 0;
		}
		else
		{
			errorString = ("get vmcs error fail:unknown error");
			return 0;
		}
	}
	switch (out) {
	case 1:
		errorString = ("VMCALL executed in VMX root operation");
		break;
	case 2:
		errorString = ("VMCLEAR with invalid physical address");
		break;
	case 3:
		errorString = ("VMCLEAR with VMXON pointer");
		break;
	case 4:
		errorString = ("VMLAUNCH with non-clear VMCS");
		break;
	case 5:
		errorString = ("VMRESUME with non-launched VMCS");
		break;
	case 6:
		errorString = ("VMRESUME after VMXOFF (VMXOFF and VMXON between VMLAUNCH and VMRESUME)");
		break;
	case 7:
		errorString = ("VM entry with invalid control field(s)");
		break;
	case 8:
		errorString = ("VM entry with invalid host-state field(s)");
		break;
	case 9:
		errorString = ("VMPTRLD with invalid physical address");
		break;
	case 10:
		errorString = ("VMPTRLD with VMXON pointer");
		break;
	case 11:
		errorString = ("VMPTRLD with incorrect VMCS revision identifier");
		break;
	case 12:
		errorString = ("VMREAD/VMWRITE from/to unsupported VMCS component");
		break;
	case 13:
		errorString = ("VMWRITE to read-only VMCS component");
		break;
	case 15:
		errorString = ("VMXON executed in VMX root operation");
		break;
	case 16:
		errorString = ("VM entry with invalid executive-VMCS pointer2");
		break;
	case 17:
		errorString = ("VM entry with non-launched executive VMCS2");
		break;
	case 18:
		errorString = ("VM entry with executive-VMCS pointer not VMXON pointer (when attempting to deactivate the dual-monitor treatment of SMIs and SMM)");
		break;
	case 19:
		errorString = ("VMCALL with non-clear VMCS (when attempting to activate the dual-monitor treatment of SMIs and SMM)");
		break;
	case 20:
		errorString = ("VMCALL with invalid VM-exit control fields");
		break;
	case 22:
		errorString = ("VMCALL with incorrect MSEG revision identifier (when attempting to activate the dual-monitor treatment of SMIs and SMM)");
		break;
	case 23:
		errorString = ("VMXOFF under dual-monitor treatment of SMIs and SMM");
		break;
	case 24:
		errorString = ("VMCALL with invalid SMM-monitor features (when attempting to activate the dual-monitor treatment of SMIs and SMM)");
		break;
	case 25:
		errorString = ("VM entry with invalid VM-execution control fields in executive VMCS (when attempting to return from SMM)");
		break;
	case 26:
		errorString = ("VM entry with events blocked by MOV SS.");
		break;
	case 28:
		errorString = ("Invalid operand to INVEPT/INVVPID.");
		break;
	default:
		errorString = ("Unknown error.");
		break;
	}
	return errorString;
}
extern "C" UINT64 gALvmxHostStackSize = VMX_HOST_STACK_SIZE;
// calculate a segment's access rights
vmx_segment_access_rights ALhvGetVMX_segment(
	segment_descriptor_register_64 const& gdtr,
	segment_selector const selector) {
	// fetch the segment descriptor from the gdtr
	auto const descriptor = reinterpret_cast<segment_descriptor_64*>(
		gdtr.base_address + static_cast<uint64_t>(selector.index) * 8);

	vmx_segment_access_rights access;
	access.flags = 0;

	// 3.24.4.1
	access.type = descriptor->type;
	access.descriptor_type = descriptor->descriptor_type;
	access.descriptor_privilege_level = descriptor->descriptor_privilege_level;
	access.present = descriptor->present;
	access.available_bit = descriptor->system;
	access.long_mode = descriptor->long_mode;
	access.default_big = descriptor->default_big;
	access.granularity = descriptor->granularity;
	access.unusable = (selector.index == 0);

	return access;
}
static bool ALvmxSupported() {
	cpuid_eax_01 cpuid_01;
	__cpuid(reinterpret_cast<int*>(&cpuid_01), 0x01);

	// 3.23.6
	if (!cpuid_01.cpuid_feature_information_ecx.virtual_machine_extensions) {
		DbgPrint("[hv] VMX not supported by CPUID.\n");
		return false;
	}
	ia32_feature_control_register feature_control;
	feature_control.flags = __readmsr(IA32_FEATURE_CONTROL);

	// 3.23.7
	if (!feature_control.lock_bit ||
		!feature_control.enable_vmx_outside_smx) {
		DbgPrint("[hv] VMX not enabled outside SMX.\n");
		return false;
	}

	//_disable();

	auto cr0 = __readcr0();
	auto cr4 = __readcr4();

	// 3.23.7
	cr4 |= CR4_VMX_ENABLE_FLAG;

	// reserved bits in CR0/CR4
	uint64_t vmx_cr0_fixed0 = __readmsr(IA32_VMX_CR0_FIXED0);;
	uint64_t vmx_cr0_fixed1 = __readmsr(IA32_VMX_CR0_FIXED1);;
	uint64_t vmx_cr4_fixed0 = __readmsr(IA32_VMX_CR4_FIXED0);;
	uint64_t vmx_cr4_fixed1 = __readmsr(IA32_VMX_CR4_FIXED1);;

	// 3.23.8
	cr0 |= vmx_cr0_fixed0;
	cr0 &= vmx_cr0_fixed1;
	cr4 |= vmx_cr4_fixed0;
	cr4 &= vmx_cr4_fixed1;

	__writecr0(cr0);
	__writecr4(cr4);

	//_enable();

	return true;
}

bool ALvmxCoreStart(OR_HV_VMX_CORE* core)
{
	if (!ALvmxSupported())
	{
		ALhvAddErr("CPU不支持虚拟化");
		return 0;
	}

	ORVM_CONTEXT_t ctx = { 0 };
	if (ALhvGetContext_asm(&ctx) == 1) {

		/*ALdbgPutValue(currCore->vmxInfo.vm_exit_tsc_overhead);
		ALdbgPutValue(currCore->vmxInfo.vm_exit_mperf_overhead);
		ALdbgPutValue(currCore->vmxInfo.vm_exit_ref_tsc_overhead);*/
		///cr04保留位读取的初始值 cr3load(先不动试试) 	修改hostcr3


		//开启完毕
		return 1;
	}
	auto vmxon_mem = ALhvMMallocateMemory(0x1000);
	if (vmxon_mem == NULL)
	{
		ALhvAddErr("申请vmon内存失败");
		return 0;
	}
	auto vmcs_mem = ALhvMMallocateMemory(0x1000);
	if (vmcs_mem == NULL)
	{
		ALhvAddErr("申请vmcs内存失败");
		return 0;
	}
	core->vmxon.vv = (UINT64)vmxon_mem;
	core->vmxon.pv = ALhvMMgetPA(vmxon_mem);
	core->vmcs.vv = (UINT64)vmcs_mem;
	core->vmcs.pv = ALhvMMgetPA(vmcs_mem);

	auto& vmxon = core->vmxon;
	auto& vmcs = core->vmcs;

	ia32_vmx_basic_register vmx_basic;
	vmx_basic.flags = __readmsr(IA32_VMX_BASIC);

	// 3.24.11.5
	vmxon->revision_id = vmx_basic.vmcs_revision_id;
	vmxon->must_be_zero = 0;

	// 启用虚拟化,进入non-root模式
	if (__vmx_on(&vmxon.pv)) {
		ALhvSetErr("[hv] VMXON failed.\n");
		return 0;
	}

	// 3.28.3.3.4
	ALvmxInvept_asm(invept_all_context, {});

	//设置当前cs
	// 3.24.2
	vmcs->revision_id = vmx_basic.vmcs_revision_id;
	vmcs->shadow_vmcs_indicator = 0;

	if (__vmx_vmclear(&vmcs.pv)) {
		ALhvSetErr("[hv] VMCLEAR failed.\n");
		return false;
	}

	if (__vmx_vmptrld(&vmcs.pv)) {
		ALhvSetErr("[hv] VMPTRLD failed.\n");
		return false;
	}
	auto& reg = core->reg;
	//设置guest 寄存器
	{
		reg.rax = 1;                              //
		reg.rbx = ctx.rbx;                        //
		reg.rcx = ctx.rcx;                        //
		reg.rdx = ctx.rdx;                        //
		reg.rsi = ctx.rsi;                        //
		reg.rdi = ctx.rdi;                        //
		reg.rbp = ctx.rbp;                        //
		reg.rsp = ctx.rsp;                        //
		reg.rip = ctx.rip;                        //
		reg.rflags.flags = ctx.rflags;                     //
		reg.r8 = ctx.r8;                         //
		reg.r9 = ctx.r9;                         //
		reg.r10 = ctx.r10;                        //
		reg.r11 = ctx.r11;                        //
		reg.r12 = ctx.r12;                        //
		reg.r13 = ctx.r13;                        //
		reg.r14 = ctx.r14;                        //
		reg.r15 = ctx.r15;                        //
		reg.cr0 = ctx.cr0;                        //
		reg.cr2 = ctx.cr2;                        //
		reg.cr3 = ctx.cr3;                        //
		reg.cr4.flags = ctx.cr4;                        //
		reg.cr8 = ctx.cr8;                        //
		reg.dr0 = ctx.dr0;                        //
		reg.dr1 = ctx.dr1;                        //
		reg.dr2 = ctx.dr2;                        //
		reg.dr3 = ctx.dr3;                        //
		reg.dr4 = ctx.dr4;                        //
		reg.dr5 = ctx.dr5;                        //
		reg.dr6 = ctx.dr6;                        //
		reg.dr7.flags = ctx.dr7;                        //
		memcpy(reg.xmm0, ctx.xmm0, 16);                  //
		memcpy(reg.xmm1, ctx.xmm1, 16);                  //
		memcpy(reg.xmm2, ctx.xmm2, 16);                  //
		memcpy(reg.xmm3, ctx.xmm3, 16);                  //
		memcpy(reg.xmm4, ctx.xmm4, 16);                  //
		memcpy(reg.xmm5, ctx.xmm5, 16);                  //
		memcpy(reg.xmm6, ctx.xmm6, 16);                  //
		memcpy(reg.xmm7, ctx.xmm7, 16);                  //
	}
	{
		ia32_vmx_pinbased_ctls_register pin_based_ctrl;
		pin_based_ctrl.flags = 0;
		pin_based_ctrl.virtual_nmi = 1;				  //开启虚拟nmi
		pin_based_ctrl.nmi_exiting = 1;				  //开启nmi退出
		//pin_based_ctrl.activate_vmx_preemption_timer = 1;	 //定时vmexit,暂不开启
		write_ctrl_pin_based_safe(pin_based_ctrl);
	}


	{
		// 3.24.6.2
		ia32_vmx_procbased_ctls_register proc_based_ctrl;
		proc_based_ctrl.flags = 0;
		//#ifndef NDEBUG
		//proc_based_ctrl.cr3_load_exiting = 1;					 //加载cr3 vmexit
		//proc_based_ctrl.cr3_store_exiting           = 1;
	  //#endif
		proc_based_ctrl.use_msr_bitmaps = 1;		 //msr退出位图
		{
			__vmx_vmwrite(VMCS_CTRL_MSR_BITMAP_ADDRESS, gALvmxVCPU->msr_bitmap.pv);
		}
		proc_based_ctrl.use_tsc_offsetting = 1;		 //tsc偏移
		{
			__vmx_vmwrite(VMCS_CTRL_TSC_OFFSET, 0);
		}
		proc_based_ctrl.activate_secondary_controls = 1;  //启用拓展控制位
		write_ctrl_proc_based_safe(proc_based_ctrl);
	}

	{

		// 3.24.6.2
		ia32_vmx_procbased_ctls2_register proc_based_ctrl2;
		proc_based_ctrl2.flags = 0;
		proc_based_ctrl2.enable_ept = 1;		//启用EPT
		{
			auto v = EPT_CLS::getViceEpt()->getEptp();
			/*auto v = EPT_CLS::getConstEptp();
			v.page_frame_number = 123;
			ALhvPutLog("!!!%p", v.flags);*/
			__vmx_vmwrite(VMCS_CTRL_EPT_POINTER, v.flags);
		}
		proc_based_ctrl2.enable_rdtscp = 1;		//rdtscp指令exit
		proc_based_ctrl2.enable_vpid = 1;		//启用缓存标识用以躲过mov cr3,0这种傻逼东西 "https://www.unknowncheats.me/forum/anti-cheat-bypass/572387-cr3-trashing.html"
		{
			//因为采用虚拟核心独占物理核心的模式,标识可以相同
			__vmx_vmwrite(VMCS_CTRL_VIRTUAL_PROCESSOR_IDENTIFIER, ALvmx_guest_vpid);
		}
		proc_based_ctrl2.enable_invpcid = 1;	//..
		proc_based_ctrl2.enable_xsaves = 1;		//..
		proc_based_ctrl2.enable_user_wait_pause = 1; //启用一些指令
		proc_based_ctrl2.conceal_vmx_from_pt = 1;	//禁止跟踪non-root模式下操作
		write_ctrl_proc_based2_safe(proc_based_ctrl2);
	}
	 //vmexit时行为控制
	{
		// 3.24.7
		ia32_vmx_exit_ctls_register exit_ctrl;
		exit_ctrl.flags = 0;
		exit_ctrl.save_debug_controls = 1;		  //保存guest debug控制寄存器
		exit_ctrl.host_address_space_size = 1;	  //设置host为长模式
		exit_ctrl.save_ia32_pat = 1;			  //保存guest相关寄存器 (内存类型)
		exit_ctrl.load_ia32_pat = 1;			  //加载host相关寄存器
		{
			ia32_pat_register host_pat;
			host_pat.flags = 0;
			host_pat.pa0 = MEMORY_TYPE_WRITE_BACK;
			host_pat.pa1 = MEMORY_TYPE_WRITE_THROUGH;
			host_pat.pa2 = MEMORY_TYPE_UNCACHEABLE_MINUS;
			host_pat.pa3 = MEMORY_TYPE_UNCACHEABLE;
			host_pat.pa4 = MEMORY_TYPE_WRITE_BACK;
			host_pat.pa5 = MEMORY_TYPE_WRITE_THROUGH;
			host_pat.pa6 = MEMORY_TYPE_UNCACHEABLE_MINUS;
			host_pat.pa7 = MEMORY_TYPE_UNCACHEABLE;
			__vmx_vmwrite(VMCS_HOST_PAT, host_pat.flags);
		}
		exit_ctrl.save_ia32_efer = 1;			  //保存guest相关寄存器 (性能计数器)
		//exit_ctrl.load_ia32_efer = 1;			  //加载host相关寄存器	 (不需要,这个无关紧要host可以沿用guest)
		{
			//__vmx_vmwrite(VMCS_HOST_EFER, __readmsr(IA32_EFER));
		}
		exit_ctrl.load_ia32_perf_global_ctrl = 1; //加载host相关寄存器	(某些功能位)
		{
			__vmx_vmwrite(VMCS_HOST_PERF_GLOBAL_CTRL, 0);
		}
		exit_ctrl.conceal_vmx_from_pt = 1;		  //禁止跟踪vmexit
		write_ctrl_exit_safe(exit_ctrl);
		
	}
	//vm进入时行为控制
	{
		// 3.24.8
		ia32_vmx_entry_ctls_register entry_ctrl;
		entry_ctrl.flags = 0;
		entry_ctrl.load_debug_controls = 1;		  //加载guest debug控制寄存器
		entry_ctrl.ia32e_mode_guest = 1;		  //vm进入时进入长模式
		entry_ctrl.load_ia32_pat = 1;			  //加载guest相关寄存器
		{
			__vmx_vmwrite(VMCS_GUEST_PAT, __readmsr(IA32_PAT));
		}
		//entry_ctrl.load_ia32_perf_global_ctrl = 1;	  //加载guest相关寄存器  不在此处加载
		{
			//__vmx_vmwrite(VMCS_GUEST_PERF_GLOBAL_CTRL, __readmsr(IA32_PERF_GLOBAL_CTRL));
		}
		entry_ctrl.load_ia32_efer = 1;			   //加载guest相关寄存器
		{
			__vmx_vmwrite(VMCS_GUEST_EFER, __readmsr(IA32_EFER));
		}
		entry_ctrl.conceal_vmx_from_pt = 1;
		write_ctrl_entry_safe(entry_ctrl);
	}
	//guest msr保存恢复

	{
		auto& msr_store = core->msr_store;
		msr_store.perf_global_ctrl.msr_idx = IA32_PERF_GLOBAL_CTRL;
		msr_store.aperf.msr_idx = IA32_APERF;
		msr_store.mperf.msr_idx = IA32_MPERF;
		msr_store.perf_global_ctrl.msr_data = __readmsr(IA32_PERF_GLOBAL_CTRL);
		msr_store.aperf.msr_data = __readmsr(IA32_APERF);
		msr_store.mperf.msr_data = __readmsr(IA32_MPERF);
		auto pa = ALhvMMgetPA(&msr_store);
		__vmx_vmwrite(VMCS_CTRL_VMEXIT_MSR_STORE_COUNT, 3);
		__vmx_vmwrite(VMCS_CTRL_VMEXIT_MSR_STORE_ADDRESS, pa);
		__vmx_vmwrite(VMCS_CTRL_VMENTRY_MSR_LOAD_COUNT, 3);
		__vmx_vmwrite(VMCS_CTRL_VMENTRY_MSR_LOAD_ADDRESS, pa);
	}
	segment_descriptor_register_64 gdtr, idtr;
	_sgdt(&gdtr);
	__sidt(&idtr);

	 //host上下文
	{
		vmx_vmwrite(VMCS_HOST_CR3, gALvmxVCPU->host_cr3.flags);


		vmx_vmwrite(VMCS_HOST_CR0, ctx.cr0);
		cr4 host_cr4;
		host_cr4.flags = __readcr4();
		host_cr4.fsgsbase_enable = 1;
		host_cr4.os_xsave = 1;
		host_cr4.smap_enable = 0;
		host_cr4.smep_enable = 0;
		vmx_vmwrite(VMCS_HOST_CR4, host_cr4.flags);

		vmx_vmwrite(VMCS_HOST_RSP, (UINT64)core);//直接将VCPU核心对象作为堆栈传入HOST,方便保存寄存器
		vmx_vmwrite(VMCS_HOST_RIP, (UINT64)(ALvmxHostEnter_asm));	//vmxasm.asm

		//沿用guest GDT
		vmx_vmwrite(VMCS_HOST_CS_SELECTOR, ctx.cs.flags & 0xFFF8);  
		vmx_vmwrite(VMCS_HOST_SS_SELECTOR, ctx.ds.flags & 0xFFF8);
		vmx_vmwrite(VMCS_HOST_DS_SELECTOR, ctx.es.flags & 0xFFF8);
		vmx_vmwrite(VMCS_HOST_ES_SELECTOR, ctx.fs.flags & 0xFFF8);
		vmx_vmwrite(VMCS_HOST_FS_SELECTOR, ctx.gs.flags & 0xFFF8);
		vmx_vmwrite(VMCS_HOST_GS_SELECTOR, ctx.ss.flags & 0xFFF8);
		vmx_vmwrite(VMCS_HOST_TR_SELECTOR, ctx.tr.flags & 0xFFF8);

		vmx_vmwrite(VMCS_HOST_FS_BASE, __readmsr(IA32_FS_BASE));
		vmx_vmwrite(VMCS_HOST_GS_BASE, __readmsr(IA32_GS_BASE));
		vmx_vmwrite(VMCS_HOST_TR_BASE, ALhvGetSegmentBase(gdtr, ctx.tr));

		vmx_vmwrite(VMCS_HOST_IDTR_BASE, (UINT64)gALvmxVCPU->host_idt);
		vmx_vmwrite(VMCS_HOST_GDTR_BASE, (UINT64)gALvmxVCPU->host_gdt);


		vmx_vmwrite(VMCS_HOST_SYSENTER_CS, __readmsr(IA32_SYSENTER_CS));
		vmx_vmwrite(VMCS_HOST_SYSENTER_ESP, __readmsr(IA32_SYSENTER_ESP));
		vmx_vmwrite(VMCS_HOST_SYSENTER_EIP, __readmsr(IA32_SYSENTER_EIP));
	}
	//guest 上下文
	{
		vmx_vmwrite(VMCS_GUEST_CR3, ctx.cr3);

		vmx_vmwrite(VMCS_GUEST_CR0, ctx.cr0);
		vmx_vmwrite(VMCS_GUEST_CR4, ctx.cr4);

		vmx_vmwrite(VMCS_GUEST_DR7, ctx.dr7);

		// RIP and RSP are set in vm-launch.asm
		vmx_vmwrite(VMCS_GUEST_RSP, ctx.rsp);
		vmx_vmwrite(VMCS_GUEST_RIP, ctx.rip);

		vmx_vmwrite(VMCS_GUEST_RFLAGS, ctx.rflags);

		vmx_vmwrite(VMCS_GUEST_CS_SELECTOR, ctx.cs.flags);
		vmx_vmwrite(VMCS_GUEST_SS_SELECTOR, ctx.ss.flags);
		vmx_vmwrite(VMCS_GUEST_DS_SELECTOR, ctx.ds.flags);
		vmx_vmwrite(VMCS_GUEST_ES_SELECTOR, ctx.es.flags);
		vmx_vmwrite(VMCS_GUEST_FS_SELECTOR, ctx.fs.flags);
		vmx_vmwrite(VMCS_GUEST_GS_SELECTOR, ctx.gs.flags);
		vmx_vmwrite(VMCS_GUEST_TR_SELECTOR, ctx.tr.flags);
		vmx_vmwrite(VMCS_GUEST_LDTR_SELECTOR, ctx.ldtr.flags);

		vmx_vmwrite(VMCS_GUEST_CS_BASE, ALhvGetSegmentBase(gdtr, ctx.cs));
		vmx_vmwrite(VMCS_GUEST_SS_BASE, ALhvGetSegmentBase(gdtr, ctx.ss));
		vmx_vmwrite(VMCS_GUEST_DS_BASE, ALhvGetSegmentBase(gdtr, ctx.ds));
		vmx_vmwrite(VMCS_GUEST_ES_BASE, ALhvGetSegmentBase(gdtr, ctx.es));
		vmx_vmwrite(VMCS_GUEST_FS_BASE, __readmsr(IA32_FS_BASE));
		vmx_vmwrite(VMCS_GUEST_GS_BASE, __readmsr(IA32_GS_BASE));
		vmx_vmwrite(VMCS_GUEST_TR_BASE, ALhvGetSegmentBase(gdtr, ctx.tr));
		vmx_vmwrite(VMCS_GUEST_LDTR_BASE, ALhvGetSegmentBase(gdtr, ctx.ldtr));

		vmx_vmwrite(VMCS_GUEST_CS_LIMIT, __segmentlimit(ctx.cs.flags));
		vmx_vmwrite(VMCS_GUEST_SS_LIMIT, __segmentlimit(ctx.ss.flags));
		vmx_vmwrite(VMCS_GUEST_DS_LIMIT, __segmentlimit(ctx.ds.flags));
		vmx_vmwrite(VMCS_GUEST_ES_LIMIT, __segmentlimit(ctx.es.flags));
		vmx_vmwrite(VMCS_GUEST_FS_LIMIT, __segmentlimit(ctx.fs.flags));
		vmx_vmwrite(VMCS_GUEST_GS_LIMIT, __segmentlimit(ctx.gs.flags));
		vmx_vmwrite(VMCS_GUEST_TR_LIMIT, __segmentlimit(ctx.tr.flags));
		vmx_vmwrite(VMCS_GUEST_LDTR_LIMIT, __segmentlimit(ctx.ldtr.flags));

		vmx_vmwrite(VMCS_GUEST_CS_ACCESS_RIGHTS, ALhvGetVMX_segment(gdtr, ctx.cs).flags);
		vmx_vmwrite(VMCS_GUEST_SS_ACCESS_RIGHTS, ALhvGetVMX_segment(gdtr, ctx.ss).flags);
		vmx_vmwrite(VMCS_GUEST_DS_ACCESS_RIGHTS, ALhvGetVMX_segment(gdtr, ctx.ds).flags);
		vmx_vmwrite(VMCS_GUEST_ES_ACCESS_RIGHTS, ALhvGetVMX_segment(gdtr, ctx.es).flags);
		vmx_vmwrite(VMCS_GUEST_FS_ACCESS_RIGHTS, ALhvGetVMX_segment(gdtr, ctx.fs).flags);
		vmx_vmwrite(VMCS_GUEST_GS_ACCESS_RIGHTS, ALhvGetVMX_segment(gdtr, ctx.gs).flags);
		vmx_vmwrite(VMCS_GUEST_TR_ACCESS_RIGHTS, ALhvGetVMX_segment(gdtr, ctx.tr).flags);
		vmx_vmwrite(VMCS_GUEST_LDTR_ACCESS_RIGHTS, ALhvGetVMX_segment(gdtr, ctx.ldtr).flags);

		vmx_vmwrite(VMCS_GUEST_GDTR_BASE, gdtr.base_address);
		vmx_vmwrite(VMCS_GUEST_IDTR_BASE, idtr.base_address);

		vmx_vmwrite(VMCS_GUEST_GDTR_LIMIT, gdtr.limit);
		vmx_vmwrite(VMCS_GUEST_IDTR_LIMIT, idtr.limit);

		vmx_vmwrite(VMCS_GUEST_SYSENTER_CS, __readmsr(IA32_SYSENTER_CS));
		vmx_vmwrite(VMCS_GUEST_SYSENTER_ESP, __readmsr(IA32_SYSENTER_ESP));
		vmx_vmwrite(VMCS_GUEST_SYSENTER_EIP, __readmsr(IA32_SYSENTER_EIP));
		vmx_vmwrite(VMCS_GUEST_DEBUGCTL, __readmsr(IA32_DEBUGCTL));
		vmx_vmwrite(VMCS_GUEST_PAT, __readmsr(IA32_PAT));
		vmx_vmwrite(VMCS_GUEST_PERF_GLOBAL_CTRL, __readmsr(IA32_PERF_GLOBAL_CTRL));

		vmx_vmwrite(VMCS_GUEST_ACTIVITY_STATE, vmx_active);

		vmx_vmwrite(VMCS_GUEST_INTERRUPTIBILITY_STATE, 0);

		vmx_vmwrite(VMCS_GUEST_PENDING_DEBUG_EXCEPTIONS, 0);

		vmx_vmwrite(VMCS_GUEST_VMCS_LINK_POINTER, MAXULONG64);

		vmx_vmwrite(VMCS_GUEST_VMX_PREEMPTION_TIMER_VALUE, MAXULONG64);


	}

	ALvmxGuestEnter_asm(core);

	ALhvSetErr("进入出现错误:%s", ALvmxGetVmcsError());
	//ALdbgStop();
	return 0;











}
// enable vm-exits for MTRR MSR writes
static void enable_mtrr_exiting(vmx_msr_bitmap* msr_bitmap) {
	ia32_mtrr_capabilities_register mtrr_cap;
	mtrr_cap.flags = __readmsr(IA32_MTRR_CAPABILITIES);

	enable_exit_for_msr_write(msr_bitmap, IA32_MTRR_DEF_TYPE, true);

	// enable exiting for fixed-range MTRRs
	if (mtrr_cap.fixed_range_supported) {
		enable_exit_for_msr_write(msr_bitmap, IA32_MTRR_FIX64K_00000, true);
		enable_exit_for_msr_write(msr_bitmap, IA32_MTRR_FIX16K_80000, true);
		enable_exit_for_msr_write(msr_bitmap, IA32_MTRR_FIX16K_A0000, true);

		for (uint32_t i = 0; i < 8; ++i)
			enable_exit_for_msr_write(msr_bitmap, IA32_MTRR_FIX4K_C0000 + i, true);
	}

	// enable exiting for variable-range MTRRs
	for (uint32_t i = 0; i < mtrr_cap.variable_range_count; ++i) {
		enable_exit_for_msr_write(msr_bitmap, IA32_MTRR_PHYSBASE0 + i * 2, true);
		enable_exit_for_msr_write(msr_bitmap, IA32_MTRR_PHYSMASK0 + i * 2, true);
	}
}
bool ALvmxInit(OR_HV_VMX* vcpu)
{
	{
		auto bitmap = ALhvMMallocateMemory(0x1000);
		if (!bitmap)
		{
			ALhvAddErr("申请内存失败");
			return 0;
		}

		vcpu->msr_bitmap.vv = (UINT64)bitmap;
		vcpu->msr_bitmap.pv = ALhvMMgetPA(bitmap);

		enable_mtrr_exiting(vcpu->msr_bitmap.va);
	}


	if (!EPT_CLS::init())
	{
		ALhvAddErr("EPT初始化失败");
		return 0;
	}

	if (!ALvmxVmexitInit())
	{
		ALhvAddErr("vmeixt_handler初始化失败");
		return 0;
	}

	return 1;
}



bool ALvmxStart(OR_HV_VMX* vcpu)
{
	//初始化错误string
	ALhvSetErr("");
	if (!ALvmxInit(vcpu))
	{
		ALhvAddErr("vmx初始化失败");
		return 0;
	}
	gALvmxVCPU = vcpu;
	PROCESSOR_NUMBER processorNumber = { 0 };
	GROUP_AFFINITY   affinity = { 0 }, oldAffinity = { 0 };
	UINT64 success = 0;
	
	for (ULONG i = 0; i < vcpu->core_count; i++)
	{

		//切换处理器核心
		auto status = KeGetProcessorNumberFromIndex(i, &processorNumber);
		if (!NT_SUCCESS(status)) {
			ALhvSetErr("切换核心失败%d", i);
			break;
		}
		affinity.Group = processorNumber.Group;
		affinity.Mask = 1ULL << processorNumber.Number;
		affinity.Reserved[0] = affinity.Reserved[1] = affinity.Reserved[2] = 0;
		KeSetSystemGroupAffinityThread(&affinity, &oldAffinity);
		//开始当前核心虚拟化
		if (ALvmxCoreStart(&vcpu->cores[i]))
		{
			(success++);
		}
		else
		{
			ALhvAddErr("%d核心虚拟化失败", i);
			return 0;
		}
		//换回原核心
		KeRevertToUserGroupAffinityThread(&oldAffinity);

	}


	//ALdbgPut("VT Engine has been loaded!\n");

	return success == vcpu->core_count;









}


bool ALvmxIsRoot()
{
	if (gALvmxVCPU == 0 || gALvmxVCPU->cores == 0)
		return 0;
	auto vcpu = ALvmxGetCurrVcore();
	if (vcpu)
		return vcpu->isRoot;
	return 0;
}

OR_HV_VMX_CORE* ALvmxGetCurrVcore()
{
	return &gALvmxVCPU->cores[ALhvGetCurrVcoreIndex()];
}

//OR_HV_VMX_CORE* ALvmxGetCurrVcore()
//{
//	if (!gALvmxVCPU)
//		return 0;
//	auto  rsp = (UINT64)_AddressOfReturnAddress();
//	for (int i = 0; i < gALvmxVCPU->core_count; i++)
//		if (rsp >= (UINT64)(&gALvmxVCPU->cores[i]) && rsp < (UINT64)(&gALvmxVCPU->cores[i + 1]))
//			return  &gALvmxVCPU->cores[i];
//	//如果上面找不到说明不在root模式,直接调用api
//	return  &gALvmxVCPU->cores[KeGetCurrentProcessorIndex()];
//}
//int ALvmxGetCurrVcoreIndex()
//{
//	if (!gALvmxVCPU)
//		return KeGetCurrentProcessorIndex();
//	auto  rsp = (UINT64)_AddressOfReturnAddress();
//	for (int i = 0; i < gALvmxVCPU->core_count; i++)
//		if (rsp >= (UINT64)(&gALvmxVCPU->cores[i]) && rsp < (UINT64)(&gALvmxVCPU->cores[i + 1]))
//			return i;
//	//如果上面找不到说明不在root模式,直接调用api
//	return KeGetCurrentProcessorIndex();
//}

