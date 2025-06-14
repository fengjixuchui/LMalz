#pragma once
#include "Buffer.h"
/*
 mov cr
 mov dr
 mov 段选择子寄存器
 vmx指令
 cpuid

 rdtsc
 rdtscp

 XGETBV/XSETBV		xcr0
 XSAVES/XRSTORS		不管了


 rdmsr
 wrmsr

 SFENCE,LFENCE,MFENCE 内存屏障

 RDPMC 读取性能监控计数器

 jmp far
 call far
 ret far

 PUSHF/POPF
 sti	开启中断
 cli	屏蔽中断
 hlt	休眠			 不管

 sidt	保存中断描述符表
 lidt	设置中断描述符表
 sgdt	保存全局段选择表
 lgdt	设置全局段选择表

 sldt	保存局部段选择表
 lldt	设置全局段选择表

 str
 ltr

 LDMXCSR/STMXCSR 浮点异常控制,不管
 RDPKRU/WRPKRU 内存保护key,不管

 快速系统调用
 SYSENTER/SYSEXIT
 SYSCALL/SYSRET

 int x
 in/out

 ud2	 触发无效指令异常（#UD）
 WBINVD 写回并失效缓存
 INVD	失效缓存（不写回）
 INVLPG	失效 TLB 条目


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
	SEND_INS_TYPE_mov_to_reg,//设置寄存器的值
	SEND_INS_TYPE_save_reg,//保存寄存器的值
	SEND_INS_TYPE_rdmsr,//访问msr
	SEND_INS_TYPE_wrmsr,//访问msr
	SEND_INS_TYPE_cpuid,// 
	SEND_INS_TYPE_r_fs_gs,//访问fs_gs
	SEND_INS_TYPE_w_fs_gs,//访问fs_gs
	SEND_INS_TYPE_vmx,		//虚拟化指令	   vmx__
	SEND_INS_TYPE_sys,		//快速系统调用  SYSENTER/SYSEXIT	SYSCALL/SYSRET
	SEND_INS_TYPE_ud2_int13o,		//无效指令异常  ud2
	SEND_INS_TYPE_invd,		//使缓存失效 WBINVD/INVD
	SEND_INS_TYPE_invlpg,		//使缓存失效 INVLPG
	SEND_INS_TYPE_far,		//jmp far/call far 
	SEND_INS_TYPE_int,
	SEND_INS_TYPE_in_out,
	SEND_INS_TYPE_fence,	//内存屏障	 
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
