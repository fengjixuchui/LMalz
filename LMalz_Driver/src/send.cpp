#include "main.h"
#include "send.h"
static OR_SHA_IO* sd_pool = 0;
PVOID ALsdGetPool()
{
	if (sd_pool)
		return sd_pool->getBuffer();
	return 0;
}
#define _POOL_SIZE 2*1024*1024
UINT64 ALsdGetPool_sz()
{
	return _POOL_SIZE;
}
bool ALsdInit()
{
	auto pool_va = ALmemAllocateMemory(_POOL_SIZE);
	if (pool_va == 0)
	{
		ALdbgSetErr("ÉêÇëÄÚ´æÊ§°Ü");
		return 0;
	}
	static OR_SHA_IO pool(pool_va, _POOL_SIZE);
	sd_pool = &pool;
	return 1;
}

bool ALsdSendBytes(PVOID add, UINT16 size)
{
	auto pl_add = sd_pool->PrepareData(size);
	if (pl_add == 0)
	{
		ALdbgSetErr("ÉêÇëÊ§°Ü");
		return 0;
	}

	ALmemCopyData(pl_add, add, size);

	sd_pool->SendData(pl_add);
	return 1;
}
//static volatile LONG g_pk_id = 0;
volatile LONG g_pk_id = 0;
template <typename T>
static T* get_pl_add()
{
	auto sz = sizeof T;
	return (T*)sd_pool->PrepareData((UINT16)sz);
}
#define BK_INIT(TYPE)  \
auto id=InterlockedIncrement(&g_pk_id); \
auto pl_add = get_pl_add<SEND_PACKET_##TYPE>();  \
if (pl_add == 0)																  \
{																				  \
	ALdbgSetErr("ÉêÇëÊ§°Ü");													  \
	return 0;																	  \
}																				  \
pl_add->head.packet_id = id;						  \
pl_add->head.packet_type =  SEND_PACKET_TYPE_##TYPE;								  \
pl_add->head.tid = tid;												 \
pl_add->head.rip = rip;

bool ALsdSend_error(UINT32 tid, UINT64 rip, UINT64 errcode)
{
	BK_INIT(error);
	pl_add->errcode = errcode;
	return sd_pool->SendData(pl_add);
}


bool ALsdSend_enter(UINT32 tid, UINT64 rip)
{
	BK_INIT(enter);
	return sd_pool->SendData(pl_add);
}	
bool ALsdSend_exit(UINT32 tid, UINT64 rip, UINT64 from)
{
	BK_INIT(exit);
	pl_add->from_add = from;
	return sd_pool->SendData(pl_add);
}	 
template <typename T>
static T* get_pl_add(int ins_size)
{
	auto sz = sizeof T;
	return (T*)sd_pool->PrepareData((UINT16)(sz + ins_size));
}
#define BK_INIT_INS(TYPE)  \
auto id=InterlockedIncrement(&g_pk_id); \
auto pl_add = get_pl_add<SEND_PACKET_ins<SEND_PACKET_INS_DATA_##TYPE>>(ins_size);  \
 if (pl_add == 0)																  \
{																				  \
	ALdbgSetErr("ÉêÇëÊ§°Ü");													  \
	return 0;																	  \
}																				  \
pl_add->head.packet_id = id;						  \
pl_add->head.packet_type = SEND_PACKET_TYPE_ins;								  \
pl_add->head.tid = tid;												 \
pl_add->head.rip = rip;

bool ALsdSend_ins_mov_to_reg(UINT32 tid, UINT64 rip, PUINT8 ins, UINT32 ins_size, UINT64 new_v, UINT64 old_v)
{
	BK_INIT_INS(mov_to_reg);
	pl_add->ins_data.new_v = new_v;
	pl_add->ins_data.old_v = old_v;
	ALmemCopyData(pl_add->ins, ins, ins_size);
	return sd_pool->SendData(pl_add);
}						 
bool ALsdSend_ins_save_reg(UINT32 tid, UINT64 rip, PUINT8 ins, UINT32 ins_size, UINT64 reg_v)
{
	BK_INIT_INS(save_reg);
	pl_add->ins_data.reg_v = reg_v;
	ALmemCopyData(pl_add->ins, ins, ins_size);
	return sd_pool->SendData(pl_add);
}
bool ALsdSend_ins_rdmsr(UINT32 tid, UINT64 rip, PUINT8 ins, UINT32 ins_size, UINT32 num, UINT64 reg_v)
{
	BK_INIT_INS(rdmsr);
	pl_add->ins_data.num = num;
	pl_add->ins_data._v = reg_v;
	ALmemCopyData(pl_add->ins, ins, ins_size);
	return sd_pool->SendData(pl_add);
}


bool ALsdSend_ins_wrmsr(UINT32 tid, UINT64 rip, PUINT8 ins, UINT32 ins_size, UINT32 num, UINT64 new_v, UINT64 old_v)
{
	BK_INIT_INS(wrmsr);
	pl_add->ins_data.num = num;
	pl_add->ins_data.new_v = new_v;
	pl_add->ins_data.old_v = old_v;
	ALmemCopyData(pl_add->ins, ins, ins_size);
	return sd_pool->SendData(pl_add);
}

bool ALsdSend_ins_r_fs_gs(UINT32 tid, UINT64 rip, PUINT8 ins, UINT32 ins_size, UINT64 base, UINT32 offset, UINT64  v)
{
	BK_INIT_INS(r_fs_gs);
	pl_add->ins_data.base = base;
	pl_add->ins_data.offset = offset;
	pl_add->ins_data.v = v;
	ALmemCopyData(pl_add->ins, ins, ins_size);
	return sd_pool->SendData(pl_add);
}

bool ALsdSend_ins_w_fs_gs(UINT32 tid, UINT64 rip, PUINT8 ins, UINT32 ins_size, UINT64 base, UINT32 offset, UINT64  new_v, UINT64  old_v)
{
	BK_INIT_INS(w_fs_gs);
	pl_add->ins_data.base = base;
	pl_add->ins_data.offset = offset;
	pl_add->ins_data.new_v = new_v;
	pl_add->ins_data.old_v = old_v;
	ALmemCopyData(pl_add->ins, ins, ins_size);
	return sd_pool->SendData(pl_add);
}	

bool ALsdSend_ins_invlpg(UINT32 tid, UINT64 rip, PUINT8 ins, UINT32 ins_size, UINT64 va)
{
	BK_INIT_INS(invlpg);
	pl_add->ins_data.va = va;
	ALmemCopyData(pl_add->ins, ins, ins_size);
	return sd_pool->SendData(pl_add);
}
bool ALsdSend_ins_far(UINT32 tid, UINT64 rip, PUINT8 ins, UINT32 ins_size, UINT8 v[6])
{
	BK_INIT_INS(far);
	ALmemCopyData(pl_add->ins_data.v, v, sizeof(pl_add->ins_data.v));
	ALmemCopyData(pl_add->ins, ins, ins_size);
	return sd_pool->SendData(pl_add);
}

bool ALsdSend_ins_int(UINT32 tid, UINT64 rip, PUINT8 ins, UINT32 ins_size, UINT8 n)
{
	BK_INIT_INS(int);
	pl_add->ins_data.n = n;
	ALmemCopyData(pl_add->ins, ins, ins_size);
	return sd_pool->SendData(pl_add);
}	 

bool ALsdSend_ins_cpuid(UINT32 tid, UINT64 rip, PUINT8 ins, UINT32 ins_size, UINT32 num, UINT64 reg_v)
{
	BK_INIT_INS(cpuid);
	pl_add->ins_data.num = num;
	pl_add->ins_data._v = reg_v;
	ALmemCopyData(pl_add->ins, ins, ins_size);
	return sd_pool->SendData(pl_add);
}
bool ALsdSend_ins_in_out(UINT32 tid, UINT64 rip, PUINT8 ins, UINT32 ins_size, bool i0_o1)
{
	BK_INIT_INS(in_out);
	pl_add->ins_data.i0_o1 = i0_o1;
	ALmemCopyData(pl_add->ins, ins, ins_size);
	return sd_pool->SendData(pl_add);
}


#define DF_NULL_DATA_FUN(a)																	\
bool ALsdSend_ins_##a(UINT32 tid, UINT64 rip, PUINT8 ins, UINT32 ins_size )		\
{																							\
	BK_INIT_INS(a);																			\
	ALmemCopyData(pl_add->ins, ins, ins_size);												\
	return sd_pool->SendData(pl_add);														\
}
DF_NULL_DATA_FUN(vmx)
DF_NULL_DATA_FUN(sys)
DF_NULL_DATA_FUN(ud2_int13o)
DF_NULL_DATA_FUN(invd)
DF_NULL_DATA_FUN(fence)






