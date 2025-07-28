#pragma once
#include "Buffer.h"
/*
 mov cr
 mov dr
 mov ��ѡ���ӼĴ���
 vmxָ��
 cpuid

 rdtsc
 rdtscp

 XGETBV/XSETBV		xcr0
 XSAVES/XRSTORS		������


 rdmsr
 wrmsr

 SFENCE,LFENCE,MFENCE �ڴ�����

 RDPMC ��ȡ���ܼ�ؼ�����

 jmp far
 call far
 ret far

 PUSHF/POPF
 sti	�����ж�
 cli	�����ж�
 hlt	����			 ����

 sidt	�����ж���������
 lidt	�����ж���������
 sgdt	����ȫ�ֶ�ѡ���
 lgdt	����ȫ�ֶ�ѡ���

 sldt	����ֲ���ѡ���
 lldt	����ȫ�ֶ�ѡ���

 str
 ltr

 LDMXCSR/STMXCSR �����쳣����,����
 RDPKRU/WRPKRU �ڴ汣��key,����

 ����ϵͳ����
 SYSENTER/SYSEXIT
 SYSCALL/SYSRET

 int x
 in/out

 ud2	 ������Чָ���쳣��#UD��
 WBINVD д�ز�ʧЧ����
 INVD	ʧЧ���棨��д�أ�
 INVLPG	ʧЧ TLB ��Ŀ


*/



enum SEND_PACKET_TYPE :unsigned char
{
	SEND_PACKET_TYPE_ins,
	SEND_PACKET_TYPE_enter,
	SEND_PACKET_TYPE_exit,
	SEND_PACKET_TYPE_error,
};

struct SEND_PACKET_HEAD
{
	SEND_PACKET_TYPE packet_type;
	UINT32 packet_id;
	UINT32 tid;
	UINT64 rip;
};

struct SEND_PACKET_error
{
	SEND_PACKET_HEAD head;
	UINT64 errcode;
};
struct SEND_PACKET_enter
{
	SEND_PACKET_HEAD head;
};
struct SEND_PACKET_exit
{
	SEND_PACKET_HEAD head;
	UINT64 from_add;
};
enum SEND_INS_TYPE :unsigned char
{
	SEND_INS_TYPE_mov_to_reg,//���üĴ�����ֵ
	SEND_INS_TYPE_save_reg,//����Ĵ�����ֵ
	SEND_INS_TYPE_rdmsr,//����msr
	SEND_INS_TYPE_wrmsr,//����msr
	SEND_INS_TYPE_cpuid,// 
	SEND_INS_TYPE_r_fs_gs,//����fs_gs
	SEND_INS_TYPE_w_fs_gs,//����fs_gs
	SEND_INS_TYPE_vmx,		//���⻯ָ��	   vmx__
	SEND_INS_TYPE_sys,		//����ϵͳ����  SYSENTER/SYSEXIT	SYSCALL/SYSRET
	SEND_INS_TYPE_ud2_int13o,		//��Чָ���쳣  ud2
	SEND_INS_TYPE_invd,		//ʹ����ʧЧ WBINVD/INVD
	SEND_INS_TYPE_invlpg,		//ʹ����ʧЧ INVLPG
	SEND_INS_TYPE_far,		//jmp far/call far 
	SEND_INS_TYPE_int,
	SEND_INS_TYPE_in_out,
	SEND_INS_TYPE_fence,	//�ڴ�����	 
	SEND_INS_TYPE_,
};

struct SEND_PACKET_INS_DATA_mov_to_reg { UINT64 old_v; UINT64 new_v; };
struct SEND_PACKET_INS_DATA_save_reg { UINT64 reg_v; };
struct SEND_PACKET_INS_DATA_rdmsr { UINT32 num; UINT64  _v; };
struct SEND_PACKET_INS_DATA_cpuid { UINT32 num; UINT64  _v; };
struct SEND_PACKET_INS_DATA_wrmsr { UINT32 num; UINT64 old_v; UINT64 new_v; };
struct SEND_PACKET_INS_DATA_r_fs_gs { UINT64 base; UINT32 offset; UINT64 v; };
struct SEND_PACKET_INS_DATA_w_fs_gs { UINT64 base; UINT32 offset; UINT64 old_v; UINT64 new_v; };
struct SEND_PACKET_INS_DATA_vmx {  };
struct SEND_PACKET_INS_DATA_sys {   };
struct SEND_PACKET_INS_DATA_ud2_int13o {   };
struct SEND_PACKET_INS_DATA_invd { };
struct SEND_PACKET_INS_DATA_invlpg { UINT64 va; };
struct SEND_PACKET_INS_DATA_far { UINT8 v[6]; };
struct SEND_PACKET_INS_DATA_int { UINT8 n; };
struct SEND_PACKET_INS_DATA_in_out { bool i0_o1; };
struct SEND_PACKET_INS_DATA_fence { };



template <typename T >
struct SEND_PACKET_ins
{
	SEND_PACKET_HEAD head;
	SEND_INS_TYPE ins_type;
	T ins_data;
	UINT8 ins[0];
};
