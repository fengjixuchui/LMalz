HVM_CONTEXT_t STRUCT
rax_0                    qword ?
rbx_0                    qword ?
rcx_0                    qword ?
rdx_0                    qword ?
rsi_0                    qword ?
rdi_0                    qword ?
rbp_0                    qword ?
rsp_0                    qword ?
rip_0                    qword ?
rflags_0                 qword ?
r8_0                     qword ?
r9_0                     qword ?
r10_0                    qword ?
r11_0                    qword ?
r12_0                    qword ?
r13_0                    qword ?
r14_0                    qword ?
r15_0                    qword ?
					  
cs_0                     word ?
ds_0                     word ?
es_0                     word ?
fs_0                     word ?
gs_0                     word ?
ss_0                     word ?
ldtr_0                   word ?
tr_0                     word ?
					 
cr0_0                    qword ?
cr1_0                    qword ?
cr2_0                    qword ?
cr3_0                    qword ?
cr4_0                    qword ?
cr8_0                    qword ?
					  	 
dr0_0                    qword ?
dr1_0                    qword ?
dr2_0                    qword ?
dr3_0                    qword ?
dr4_0                    qword ?
dr5_0                    qword ?
dr6_0                    qword ?
dr7_0                    qword ?
					  
xmm0_0                   xmmword	?
xmm1_0                   xmmword	?
xmm2_0                   xmmword	?
xmm3_0                   xmmword	?
xmm4_0                   xmmword	?
xmm5_0                   xmmword	?
xmm6_0                   xmmword	?
xmm7_0                   xmmword	?
xmm8_0                   xmmword	?
xmm9_0                   xmmword	?
xmm10_0                  xmmword	?
xmm11_0                  xmmword	?
xmm12_0                  xmmword	?
xmm13_0                  xmmword	?
xmm14_0                  xmmword	?
xmm15_0                  xmmword	?

HVM_CONTEXT_t ENDS

.CODE
asm_lar proc
lar rax,rcx
ret
asm_lar endp
ALhvGetContext_asm PROC	

pushfq
pop (HVM_CONTEXT_t ptr [rcx]).rflags_0

mov (HVM_CONTEXT_t ptr [rcx]).rax_0, rax
mov (HVM_CONTEXT_t ptr [rcx]).rbx_0, rbx
mov (HVM_CONTEXT_t ptr [rcx]).rcx_0, rcx
mov (HVM_CONTEXT_t ptr [rcx]).rdx_0, rdx
mov (HVM_CONTEXT_t ptr [rcx]).rdi_0, rdi
mov (HVM_CONTEXT_t ptr [rcx]).rsi_0, rsi
mov (HVM_CONTEXT_t ptr [rcx]).rbp_0, rbp
mov (HVM_CONTEXT_t ptr [rcx]).rsp_0, rsp
add (HVM_CONTEXT_t ptr [rcx]).rsp_0, 8
mov rax, [rsp]
mov (HVM_CONTEXT_t ptr [rcx]).rip_0, rax
mov (HVM_CONTEXT_t ptr [rcx]).r8_0, r8
mov (HVM_CONTEXT_t ptr [rcx]).r9_0, r9
mov (HVM_CONTEXT_t ptr [rcx]).r10_0, r10
mov (HVM_CONTEXT_t ptr [rcx]).r11_0, r11
mov (HVM_CONTEXT_t ptr [rcx]).r12_0, r12
mov (HVM_CONTEXT_t ptr [rcx]).r13_0, r13
mov (HVM_CONTEXT_t ptr [rcx]).r14_0, r14
mov (HVM_CONTEXT_t ptr [rcx]).r15_0, r15
mov (HVM_CONTEXT_t ptr [rcx]).cs_0, cs
mov (HVM_CONTEXT_t ptr [rcx]).ds_0, ds
mov (HVM_CONTEXT_t ptr [rcx]).es_0, es
mov (HVM_CONTEXT_t ptr [rcx]).fs_0, fs
mov (HVM_CONTEXT_t ptr [rcx]).gs_0, gs
mov (HVM_CONTEXT_t ptr [rcx]).ss_0, ss
sldt  (HVM_CONTEXT_t ptr [rcx]).ldtr_0
str  (HVM_CONTEXT_t ptr [rcx]).tr_0
xor rax, rax
mov rax, cr0
mov (HVM_CONTEXT_t ptr [rcx]).cr0_0, rax
mov rax, cr2
mov (HVM_CONTEXT_t ptr [rcx]).cr2_0, rax
mov rax, cr3
mov (HVM_CONTEXT_t ptr [rcx]).cr3_0, rax
mov rax, cr4
mov (HVM_CONTEXT_t ptr [rcx]).cr4_0, rax
mov rax, cr8
mov (HVM_CONTEXT_t ptr [rcx]).cr8_0, rax
mov rax, dr0
mov (HVM_CONTEXT_t ptr [rcx]).dr0_0, rax
mov rax, dr1
mov (HVM_CONTEXT_t ptr [rcx]).dr1_0, rax
mov rax, dr2
mov (HVM_CONTEXT_t ptr [rcx]).dr2_0, rax
mov rax, dr3
mov (HVM_CONTEXT_t ptr [rcx]).dr3_0, rax
; mov rax, dr4                            ; #UD
; mov (HVM_CONTEXT_t ptr [rcx]).dr4_0, rax
; mov rax, dr5                            ; #UD
; mov (HVM_CONTEXT_t ptr [rcx]).dr5_0, rax
; mov rax, dr6                            ; #UD
; mov (HVM_CONTEXT_t ptr [rcx]).dr6_0, rax
mov rax, dr7
mov (HVM_CONTEXT_t ptr [rcx]).dr7_0, rax
movaps (HVM_CONTEXT_t ptr [rcx]).xmm0_0, xmm0
movaps (HVM_CONTEXT_t ptr [rcx]).xmm1_0, xmm1
movaps (HVM_CONTEXT_t ptr [rcx]).xmm2_0, xmm2
movaps (HVM_CONTEXT_t ptr [rcx]).xmm3_0, xmm3
movaps (HVM_CONTEXT_t ptr [rcx]).xmm4_0, xmm4
movaps (HVM_CONTEXT_t ptr [rcx]).xmm5_0, xmm5
movaps (HVM_CONTEXT_t ptr [rcx]).xmm6_0, xmm6
movaps (HVM_CONTEXT_t ptr [rcx]).xmm7_0, xmm7
movaps (HVM_CONTEXT_t ptr [rcx]).xmm8_0, xmm8
movaps (HVM_CONTEXT_t ptr [rcx]).xmm9_0, xmm9
movaps (HVM_CONTEXT_t ptr [rcx]).xmm10_0, xmm10
movaps (HVM_CONTEXT_t ptr [rcx]).xmm11_0, xmm11
movaps (HVM_CONTEXT_t ptr [rcx]).xmm12_0, xmm12
movaps (HVM_CONTEXT_t ptr [rcx]).xmm13_0, xmm13
movaps (HVM_CONTEXT_t ptr [rcx]).xmm14_0, xmm14
movaps (HVM_CONTEXT_t ptr [rcx]).xmm15_0, xmm15

xor rax, rax

ret
ALhvGetContext_asm ENDP




ALhvGetCs_asm proc
mov ax,cs
ret
ALhvGetCs_asm endp

ALhvSetCr2_asm proc
mov cr2,rcx
ret
ALhvSetCr2_asm endp
END