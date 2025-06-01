include	vmx.inc
extern ALvmxExitHandler:proc
extern gALvmxHostStackSize:qword		
.code


ALvmxGuestEnter_asm proc
  
  mov r15, rcx
  
  mov rcx, 681Ch ; VMX_VMCS_GUEST_RSP
  mov rdx, [r15 + OR_HV_VMX_CORE.reg.rsp_0]
  vmwrite rcx, rdx
  mov rcx, 681Eh ; VMX_VMCS_GUEST_RIP
  mov rdx, [r15 + OR_HV_VMX_CORE.reg.rip_0]
  vmwrite rcx, rdx
  mov rcx, 6820h ; VMX_VMCS_GUEST_RFLAGS
  mov rdx, [r15 + OR_HV_VMX_CORE.reg.rflags_0]
  vmwrite rcx, rdx
  
  ;movaps xmm0, [r15 + OR_HV_VMX_CORE.reg.xmm0_0]
  ;movaps xmm1, [r15 + OR_HV_VMX_CORE.reg.xmm1_0]
  ;movaps xmm2, [r15 + OR_HV_VMX_CORE.reg.xmm2_0]
  ;movaps xmm3, [r15 + OR_HV_VMX_CORE.reg.xmm3_0]
  ;movaps xmm4, [r15 + OR_HV_VMX_CORE.reg.xmm4_0]
  ;movaps xmm5, [r15 + OR_HV_VMX_CORE.reg.xmm5_0]
  ;movaps xmm6, [r15 + OR_HV_VMX_CORE.reg.xmm6_0]
  ;movaps xmm7, [r15 + OR_HV_VMX_CORE.reg.xmm7_0]
  
  mov rax, [r15 + OR_HV_VMX_CORE.reg.rax_0]
  mov rbx, [r15 + OR_HV_VMX_CORE.reg.rbx_0]
  mov rcx, [r15 + OR_HV_VMX_CORE.reg.rcx_0]
  mov rdx, [r15 + OR_HV_VMX_CORE.reg.rdx_0]
  mov rsi, [r15 + OR_HV_VMX_CORE.reg.rsi_0]
  mov rdi, [r15 + OR_HV_VMX_CORE.reg.rdi_0]
  mov rbp, [r15 + OR_HV_VMX_CORE.reg.rbp_0]
  mov r8, [r15 + OR_HV_VMX_CORE.reg.r8_0] 
  mov r9, [r15 + OR_HV_VMX_CORE.reg.r9_0] 
  mov r10, [r15 + OR_HV_VMX_CORE.reg.r10_0]
  mov r11, [r15 + OR_HV_VMX_CORE.reg.r11_0]
  mov r12, [r15 + OR_HV_VMX_CORE.reg.r12_0]
  mov r13, [r15 + OR_HV_VMX_CORE.reg.r13_0]
  mov r14, [r15 + OR_HV_VMX_CORE.reg.r14_0]
  mov r15, [r15 + OR_HV_VMX_CORE.reg.r15_0]
  
  vmlaunch
  
  ; vmlaunch failed.
  
  ret
ALvmxGuestEnter_asm endp




ALvmxHostEnter_asm proc
  cli

  mov [rsp + OR_HV_VMX_CORE.isRoot], 1
  mov [rsp + OR_HV_VMX_CORE.reg.rax_0], rax
  mov [rsp + OR_HV_VMX_CORE.reg.rbx_0], rbx
  mov [rsp + OR_HV_VMX_CORE.reg.rcx_0], rcx
  mov [rsp + OR_HV_VMX_CORE.reg.rdx_0], rdx
  mov [rsp + OR_HV_VMX_CORE.reg.rsi_0], rsi
  mov [rsp + OR_HV_VMX_CORE.reg.rdi_0], rdi
  mov [rsp + OR_HV_VMX_CORE.reg.rbp_0], rbp
  mov [rsp + OR_HV_VMX_CORE.reg.r8_0], r8
  mov [rsp + OR_HV_VMX_CORE.reg.r9_0], r9
  mov [rsp + OR_HV_VMX_CORE.reg.r10_0], r10
  mov [rsp + OR_HV_VMX_CORE.reg.r11_0], r11
  mov [rsp + OR_HV_VMX_CORE.reg.r12_0], r12
  mov [rsp + OR_HV_VMX_CORE.reg.r13_0], r13
  mov [rsp + OR_HV_VMX_CORE.reg.r14_0], r14
  mov [rsp + OR_HV_VMX_CORE.reg.r15_0], r15

  ; movaps [rsp - 0xFFF0 + OR_HV_VMX_CORE.reg.xmm0_0], xmm0
  ; movaps [rsp - 0xFFF0 + OR_HV_VMX_CORE.reg.xmm1_0], xmm1
  ; movaps [rsp - 0xFFF0 + OR_HV_VMX_CORE.reg.xmm2_0], xmm2
  ; movaps [rsp - 0xFFF0 + OR_HV_VMX_CORE.reg.xmm3_0], xmm3
  ; movaps [rsp - 0xFFF0 + OR_HV_VMX_CORE.reg.xmm4_0], xmm4
  ; movaps [rsp - 0xFFF0 + OR_HV_VMX_CORE.reg.xmm5_0], xmm5
  ; movaps [rsp - 0xFFF0 + OR_HV_VMX_CORE.reg.xmm6_0], xmm6
  ; movaps [rsp - 0xFFF0 + OR_HV_VMX_CORE.reg.xmm7_0], xmm7

  mov rax, 681Ch ; VMX_VMCS_GUEST_RSP
  vmread rax, rax
  mov [rsp + OR_HV_VMX_CORE.reg.rsp_0], rax
  mov rax, 681Eh ; VMX_VMCS_GUEST_RIP
  vmread rax, rax
  mov [rsp + OR_HV_VMX_CORE.reg.rip_0], rax
  mov rax, 6820h ; VMX_VMCS_GUEST_RFLAGS
  vmread rax, rax
  mov [rsp + OR_HV_VMX_CORE.reg.rflags_0], rax

  ;-------------------------------------------------

  ;mov rax, 0
  ;mov rbx, cr8
  ;mov cr8, rax

  ;-------------------------------------------------

  mov rcx, rsp
  add rsp, gALvmxHostStackSize
  sub rsp, 10h
  ;获取tsc
  MFENCE
  rdtsc
  shl rdx,32
  or rdx,rax
  mov r15,rdx 
  ;调用处理
  sub rsp, 100h
  call ALvmxExitHandler
  add rsp, 100h

  ;再次获取tsc
  MFENCE
   rdtsc
  shl rdx,32
  or rdx,rax

  ;设置tsc偏移
  ;sub rdx,r15
  ;mov rcx, 2010h ; VMCS_CTRL_TSC_OFFSET
  ;vmwrite rcx, rdx

  ;-------------------------------------------------

 ; mov cr8, rbx

  ;-------------------------------------------------
  add rsp,10h
  sub rsp,gALvmxHostStackSize

  mov rcx, 681Ch ; VMX_VMCS_GUEST_RSP
  mov rdx, [rsp + OR_HV_VMX_CORE.reg.rsp_0]
  vmwrite rcx, rdx
  mov rcx, 681Eh ; VMX_VMCS_GUEST_RIP
  mov rdx, [rsp + OR_HV_VMX_CORE.reg.rip_0]
  vmwrite rcx, rdx
  mov rcx, 6820h ; VMX_VMCS_GUEST_RFLAGS
  mov rdx, [rsp + OR_HV_VMX_CORE.reg.rflags_0]
  vmwrite rcx, rdx

  mov rax, [rsp + OR_HV_VMX_CORE.reg.rax_0]
  mov rbx, [rsp + OR_HV_VMX_CORE.reg.rbx_0]
  mov rcx, [rsp + OR_HV_VMX_CORE.reg.rcx_0]
  mov rdx, [rsp + OR_HV_VMX_CORE.reg.rdx_0]
  mov rsi, [rsp + OR_HV_VMX_CORE.reg.rsi_0]
  mov rdi, [rsp + OR_HV_VMX_CORE.reg.rdi_0]
  mov rbp, [rsp + OR_HV_VMX_CORE.reg.rbp_0]
  mov r8, [rsp + OR_HV_VMX_CORE.reg.r8_0]
  mov r9, [rsp + OR_HV_VMX_CORE.reg.r9_0]
  mov r10, [rsp + OR_HV_VMX_CORE.reg.r10_0]
  mov r11, [rsp + OR_HV_VMX_CORE.reg.r11_0]
  mov r12, [rsp + OR_HV_VMX_CORE.reg.r12_0]
  mov r13, [rsp + OR_HV_VMX_CORE.reg.r13_0]
  mov r14, [rsp + OR_HV_VMX_CORE.reg.r14_0]
  mov r15, [rsp + OR_HV_VMX_CORE.reg.r15_0]
  ; movaps xmm0, [rsp + OR_HV_VMX_CORE.reg.xmm0]
  ; movaps xmm1, [rsp + OR_HV_VMX_CORE.reg.xmm1]
  ; movaps xmm2, [rsp + OR_HV_VMX_CORE.reg.xmm2]
  ; movaps xmm3, [rsp + OR_HV_VMX_CORE.reg.xmm3]
  ; movaps xmm4, [rsp + OR_HV_VMX_CORE.reg.xmm4]
  ; movaps xmm5, [rsp + OR_HV_VMX_CORE.reg.xmm5]
  ; movaps xmm6, [rsp + OR_HV_VMX_CORE.reg.xmm6]
  ; movaps xmm7, [rsp + OR_HV_VMX_CORE.reg.xmm7]
  sub [rsp + OR_HV_VMX_CORE.isRoot], 1   ;将该值减一,如果需要退出,那么zf标志位不等于0
  jnz exitVm

  add rsp, gALvmxHostStackSize
  sub rsp, 10h
  ;sti
  vmresume
exitVm:
  sub [rsp + OR_HV_VMX_CORE.isRoot], 1
  add rsp, gALvmxHostStackSize
  sub rsp, 10h
  push rax		;保存返回值

  mov rax, 681Eh ; VMX_VMCS_GUEST_RIP
  vmread rax, rax
  push rax ;保存 rip

  mov rax, 681Ch ; VMX_VMCS_GUEST_RSP
  vmread rax, rax	
  push rax;保存rsp

  mov rax, 6820h ; VMX_VMCS_GUEST_RFLAGS
  vmread rax, rax
  push rax ;保存rflags

  vmxoff;关闭vm
  popfq;恢复rflags
  mov rax,rsp
  pop rsp;换栈
  push qword ptr[rax+8];rip入栈
  mov rax,qword ptr[rax+10h];恢复返回值
  sti
  ret;返回







ALvmxHostEnter_asm endp































ALvmxInvept_asm proc
invept rcx, oword ptr [rdx]
ret
ALvmxInvept_asm endp

end