#include "orhv\orhv.h"
#include "orhv\vmx\mods\vmx_monitor.h"
#include "dbgput.h"
#include "Buffer.h"
#include "send_type.h"
#include "disassemble.h"
bool ALhvStart();
bool ALvmxMNTinit(MONITOR_CALLBACK callback);
bool ALvmxMNTsetTarget(PVOID va, UINT64 size);

void ALmhdCallback(OR_HV_VMX_CORE* vcpu, MONITOR_CALLBACK_TYPE type);
bool ALmhdInit();
PVOID ALmhdGetInfoPool();
UINT64 ALmhdGetInfoPoolSize();

#pragma code_seg(".onon")
__declspec(noinline) void test()
{
	ALdbgPut("测试");
}
#pragma  code_seg()
extern volatile LONG g_pk_id;
VOID
put_func(
	_In_ PVOID StartContext
)
{
	StartContext;
	auto pool = ALmhdGetInfoPool();
	auto pl_sz = ALmhdGetInfoPoolSize();
	auto put = [](PVOID data, UINT64 sz)
	{
		sz;
		data;
		//return;
		auto head = (SEND_PACKET_HEAD*)data;
		switch (head->packet_type)
		{
		case SEND_PACKET_TYPE_enter:
		{
			auto enter = (SEND_PACKET_enter*)head;
			ALdbgPut("<%d>进入,地址:%p", enter->head.packet_id, enter->head.rip);
			break;
		}
		case SEND_PACKET_TYPE_exit:
		{
			auto dt = (SEND_PACKET_exit*)head;
			ALdbgPut("<%d>从地址:%p跳向地址:%p", dt->head.packet_id, dt->from_add, dt->head.rip);
			break;
		}
		case SEND_PACKET_TYPE_error:
		{
			auto dt = (SEND_PACKET_error*)head;
			ALdbgPut("<%d>rip %p 出现错误:%p", dt->head.packet_id, dt->head.rip, dt->errcode);
			break;
		}
		case SEND_PACKET_TYPE_ins:
		{
			auto dt = (SEND_PACKET_ins<int>*)head;
			switch (dt->ins_type)
			{
			case SEND_INS_TYPE_mov_to_reg://设置寄存器的值  
			{
				auto ins = (SEND_PACKET_ins<SEND_PACKET_INS_DATA_mov_to_reg>*)head;
				ASM dsm(ins->head.rip, ins->ins);
				ALdbgPut("执行指令: %s", dsm.get_str());
				break;
			}
			case SEND_INS_TYPE_save_reg://保存寄存器的值  
			{
				auto ins = (SEND_PACKET_ins<SEND_PACKET_INS_DATA_save_reg>*)head;
				ASM dsm(ins->head.rip, ins->ins);
				ALdbgPut("执行指令: %s", dsm.get_str());
				break;
			}
			case SEND_INS_TYPE_rdmsr://访问msr  
			{
				auto ins = (SEND_PACKET_ins<SEND_PACKET_INS_DATA_rdmsr>*)head;
				ASM dsm(ins->head.rip, ins->ins);
				ALdbgPut("执行指令: %s", dsm.get_str());
				break;
			}
			case SEND_INS_TYPE_wrmsr://访问msr  
			{
				auto ins = (SEND_PACKET_ins<SEND_PACKET_INS_DATA_wrmsr>*)head;
				ASM dsm(ins->head.rip, ins->ins);
				ALdbgPut("执行指令: %s", dsm.get_str());
				break;
			}
			case SEND_INS_TYPE_cpuid://   
			{
				auto ins = (SEND_PACKET_ins<SEND_PACKET_INS_DATA_cpuid>*)head;
				ASM dsm(ins->head.rip, ins->ins);
				ALdbgPut("执行指令: %s", dsm.get_str());
				break;
			}
			case SEND_INS_TYPE_r_fs_gs://访问fs_gs  
			{
				auto ins = (SEND_PACKET_ins<SEND_PACKET_INS_DATA_r_fs_gs>*)head;
				ASM dsm(ins->head.rip, ins->ins);
				ALdbgPut("执行指令: %s", dsm.get_str());
				break;
			}
			case SEND_INS_TYPE_w_fs_gs://访问fs_gs  
			{
				auto ins = (SEND_PACKET_ins<SEND_PACKET_INS_DATA_w_fs_gs>*)head;
				ASM dsm(ins->head.rip, ins->ins);
				ALdbgPut("执行指令: %s", dsm.get_str());
				break;
			}
			case SEND_INS_TYPE_vmx:		//虚拟化指令	   vmx__  
			{
				auto ins = (SEND_PACKET_ins<SEND_PACKET_INS_DATA_vmx>*)head;
				ASM dsm(ins->head.rip, ins->ins);
				ALdbgPut("执行指令: %s", dsm.get_str());
				break;
			}
			case SEND_INS_TYPE_sys:		//快速系统调用  SYSENTER/SYSEXIT	SYSCALL/SYSRET  
			{
				auto ins = (SEND_PACKET_ins<SEND_PACKET_INS_DATA_sys>*)head;
				ASM dsm(ins->head.rip, ins->ins);
				ALdbgPut("执行指令: %s", dsm.get_str());
				break;
			}
			case SEND_INS_TYPE_ud2_int13o:		//无效指令异常  ud2  
			{
				auto ins = (SEND_PACKET_ins<SEND_PACKET_INS_DATA_ud2_int13o>*)head;
				ASM dsm(ins->head.rip, ins->ins);
				ALdbgPut("执行指令: %s", dsm.get_str());
				break;
			}
			case SEND_INS_TYPE_invd:		//使缓存失效 WBINVD/INVD  
			{
				auto ins = (SEND_PACKET_ins<SEND_PACKET_INS_DATA_invd>*)head;
				ASM dsm(ins->head.rip, ins->ins);
				ALdbgPut("执行指令: %s", dsm.get_str());
				break;
			}
			case SEND_INS_TYPE_invlpg:		//使缓存失效 INVLPG  
			{
				auto ins = (SEND_PACKET_ins<SEND_PACKET_INS_DATA_invlpg>*)head;
				ASM dsm(ins->head.rip, ins->ins);
				ALdbgPut("执行指令: %s", dsm.get_str());
				break;
			}
			case SEND_INS_TYPE_far:		//jmp far/call far   
			{
				auto ins = (SEND_PACKET_ins<SEND_PACKET_INS_DATA_far>*)head;
				ASM dsm(ins->head.rip, ins->ins);
				ALdbgPut("执行指令: %s", dsm.get_str());
				break;
			}
			case SEND_INS_TYPE_int:
			{
				auto ins = (SEND_PACKET_ins<SEND_PACKET_INS_DATA_int>*)head;
				ASM dsm(ins->head.rip, ins->ins);
				ALdbgPut("执行指令: %s", dsm.get_str());
				break;
			}
			case SEND_INS_TYPE_in_out:
			{
				auto ins = (SEND_PACKET_ins<SEND_PACKET_INS_DATA_in_out>*)head;
				ASM dsm(ins->head.rip, ins->ins);
				ALdbgPut("执行指令: %s", dsm.get_str());
				break;
			}
			case SEND_INS_TYPE_fence:	//内存屏障	   
			{
				auto ins = (SEND_PACKET_ins<SEND_PACKET_INS_DATA_fence>*)head;
				ASM dsm(ins->head.rip, ins->ins);
				ALdbgPut("执行指令: %s", dsm.get_str());
				break;
			}
			default:
				break;
			}

		}
		default:
			break;
		}
	};
	OR_SHA_IO bf(pool, pl_sz);

	while (true)
	{
		ALdbgPutValue(g_pk_id);
		bf.ReadData(put);
	}
}
VOID UnicodeToChar(PUNICODE_STRING dst, char* src)
{
	ANSI_STRING string;
	if (!src || !dst || !dst->Buffer) return;
	if (RtlUnicodeStringToAnsiString(&string, dst, TRUE) == STATUS_SUCCESS) {
		strcpy(src, string.Buffer);
		RtlFreeAnsiString(&string);
	}
}
VOID LoadImageNotifyRoutine
(
	__in_opt PUNICODE_STRING  FullImageName,
	__in HANDLE  ProcessId,
	__in PIMAGE_INFO  ImageInfo
)
{
	char szFullImageName[260] = { 0 };
	if (FullImageName != NULL && MmIsAddressValid(FullImageName))
	{
		if (ProcessId == 0)
		{
			UnicodeToChar(FullImageName, szFullImageName);
			if (strstr(_strlwr(szFullImageName), "ace-game-0"))
			{
				auto b = ALvmxMNTsetTarget((PVOID)ImageInfo->ImageBase, ImageInfo->ImageSize);
				if (!b)
				{
					ALdbgPut("监控设置失败");
					return;
				}
				else
				{
					ALdbgPut("监控设置成功");
				}
				return;
			}
		}
	}
}

int main(UINT64*)
{
	ALdbgPut("驱动加载成功,开始加载VT");

	if (!ALhvStart())
	{
		auto err = ALhvGetErr();
		ALdbgPut("加载出错:%s", err);
		return 0;
	}
	else
	{
		ALdbgPut("加载成功");
		if (!ALmhdInit())
		{
			ALdbgPut("监控信息初始化失败");
			return 0;
		}
		PVOID a = 0;
		PsCreateSystemThread(&a, THREAD_ALL_ACCESS, 0, 0, 0, put_func, 0);

		if (!ALvmxMNTinit(ALmhdCallback))
		{
			ALdbgPut("监控模块初始化失败");
			return 0;
		}
		auto r = PsSetLoadImageNotifyRoutine(LoadImageNotifyRoutine);

		if (r)
		{
			ALdbgPut("设置回调失败 %p", r);
		}

		/*auto b = ALvmxMNTsetTarget((PVOID)((UINT64)test & ~0xfffLL), 0x1000);
		if (!b)
		{
			ALdbgPut("监控设置失败");
			return 0;
		}*/
		/*ALdbgPut("开始调用测试函数");
		test();
		ALdbgPut("调用测试函数完毕");*/

		//KeRaiseIrqlToDpcLevel();
		//_disable();
		//while (1);
	}
	return 0;
}