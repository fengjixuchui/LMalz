

OR_HV_VMX_REG struct
rax_0                    qword ?	 ;0
rcx_0                    qword ?	 ;8
rdx_0                    qword ?	 ;10
rbx_0                    qword ?	 ;18
rsp_0                    qword ?	 ;20
rbp_0                    qword ?	 ;28
rsi_0                    qword ?	 ;30
rdi_0                    qword ?	 ;38
r8_0                     qword ?	 ;40
r9_0                     qword ?	 ;48
r10_0                    qword ?	 ;50
r11_0                    qword ?	 ;58
r12_0                    qword ?	 ;60
r13_0                    qword ?	 ;68
r14_0                    qword ?	 ;70
r15_0                    qword ?	 ;78
									 
rip_0                    qword ?	 ;80
rflags_0                 qword ?	 ;88
									 	  
xmm0_0                   xmmword	?	  ;90
xmm1_0                   xmmword	?	  ;a0
xmm2_0                   xmmword	?	  ;b0
xmm3_0                   xmmword	?	  ;c0
xmm4_0                   xmmword	?	  ;d0
xmm5_0                   xmmword	?	  ;e0
xmm6_0                   xmmword	?	  ;f0
xmm7_0                   xmmword	?	  ;100
										  
cr0_0                    qword ?		  ;110
cr2_0                    qword ?		  ;118
cr3_0                    qword ?		  ;120
cr4_0                    qword ?		  ;128
cr8_0                    qword ?		  ;130
										  
dr0_0                    qword ?		  ;138
dr1_0                    qword ?		  ;140
dr2_0                    qword ?		  ;148
dr3_0                    qword ?		  ;150
dr4_0                    qword ?		  ;158
dr5_0                    qword ?		  ;160
dr6_0                    qword ?		  ;168
dr7_0                    qword ?		  ;170
										  
efer_0                   qword ?		  ;178
										  
OR_HV_VMX_REG ends	  ;180


OR_HV_VMX_CORE union
struct
reg					OR_HV_VMX_REG <?>	;0
isRoot				 qword ?			;0x180

;.............						暂时省略,这些够用了

;.............

;.............


ends


stack db 10000h dup(?)

OR_HV_VMX_CORE ends
