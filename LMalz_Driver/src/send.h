#pragma once
#include "send_type.h"
bool ALsdInit();
bool ALsdSend_error(UINT32 tid, UINT64 rip, UINT64 errcode);
bool ALsdSend_enter(UINT32 tid, UINT64 rip);
bool ALsdSend_exit(UINT32 tid, UINT64 rip, UINT64 from);
bool ALsdSend_ins_mov_to_reg(UINT32 tid, UINT64 rip, PUINT8 ins, UINT32 ins_size, UINT64 new_v, UINT64 old_v);
bool ALsdSend_ins_save_reg(UINT32 tid, UINT64 rip, PUINT8 ins, UINT32 ins_size, UINT64 reg_v);
bool ALsdSend_ins_rdmsr(UINT32 tid, UINT64 rip, PUINT8 ins, UINT32 ins_size, UINT32 num, UINT64 reg_v);
bool ALsdSend_ins_wrmsr(UINT32 tid, UINT64 rip, PUINT8 ins, UINT32 ins_size, UINT32 num, UINT64 new_v, UINT64 old_v);
bool ALsdSend_ins_r_fs_gs(UINT32 tid, UINT64 rip, PUINT8 ins, UINT32 ins_size, UINT64 base, UINT32 offset, UINT64  v);
bool ALsdSend_ins_w_fs_gs(UINT32 tid, UINT64 rip, PUINT8 ins, UINT32 ins_size, UINT64 base, UINT32 offset, UINT64  new_v, UINT64  old_v);
bool ALsdSend_ins_invlpg(UINT32 tid, UINT64 rip, PUINT8 ins, UINT32 ins_size, UINT64 va);
bool ALsdSend_ins_far(UINT32 tid, UINT64 rip, PUINT8 ins, UINT32 ins_size, UINT8 v[6]);
bool ALsdSend_ins_int(UINT32 tid, UINT64 rip, PUINT8 ins, UINT32 ins_size, UINT8 n);
bool ALsdSend_ins_in_out(UINT32 tid, UINT64 rip, PUINT8 ins, UINT32 ins_size, bool i0_o1);
bool ALsdSend_ins_cpuid(UINT32 tid, UINT64 rip, PUINT8 ins, UINT32 ins_size, UINT32 num, UINT64 reg_v);
#define EX_NULL_DATA_FUN(a)																	\
bool ALsdSend_ins_##a(UINT32 tid, UINT64 rip, PUINT8 ins, UINT32 ins_size );
EX_NULL_DATA_FUN(vmx)
EX_NULL_DATA_FUN(sys)
EX_NULL_DATA_FUN(ud2_int13o)
EX_NULL_DATA_FUN(invd)
EX_NULL_DATA_FUN(fence)

PVOID ALsdGetPool();
UINT64 ALsdGetPool_sz();





