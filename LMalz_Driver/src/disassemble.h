#pragma once
#include <ntddk.h>
#define ZYDIS_STATIC_BUILD 1
#define ZYCORE_STATIC_BUILD 1
#include <Zydis\Zydis.h>

#define INS(a) ZYDIS_MNEMONIC_##a
#define REG(a) ZYDIS_REGISTER_##a

class ASM
{
	ZydisDisassembledInstruction inst;

public:
	ASM(UINT64 runutime, UINT8 bytecode[16]){
		ZydisDisassembleIntel(
			/* machine_mode:    */ ZYDIS_MACHINE_MODE_LONG_64,
			/* runtime_address: */ runutime,
			/* buffer:          */ bytecode,
			/* length:          */ 16,
			/* instruction:     */ &inst
		);
	};
	inline ZydisMnemonic get_mnemonic() { return inst.info.mnemonic; };
	inline UINT8 get_operand_count() { return inst.info.operand_count; };

	static const ZydisOperandType reg = ZYDIS_OPERAND_TYPE_REGISTER;
	static const ZydisOperandType mem = ZYDIS_OPERAND_TYPE_MEMORY;
	static const ZydisOperandType imm = ZYDIS_OPERAND_TYPE_IMMEDIATE;
	static const ZydisOperandType ptr = ZYDIS_OPERAND_TYPE_POINTER;

	inline ZydisOperandType get_operand_type(int n) { return inst.operands[n - 1].type; };

	inline const ZydisDecodedOperandReg* get_operand_reg(int n) { return &inst.operands[n].reg; };
	inline const ZydisDecodedOperandPtr* get_operand_ptr(int n) { return &inst.operands[n].ptr; };
	inline const ZydisDecodedOperandImm* get_operand_imm(int n) { return &inst.operands[n].imm; };
	inline const ZydisDecodedOperandMem* get_operand_mem(int n) { return &inst.operands[n].mem; };
	UINT8 get_code_size() { return inst.info.length; };
	inline bool is_far() {
		return inst.info.meta.branch_type == ZYDIS_BRANCH_TYPE_FAR;
	};

	inline char* get_str() { return inst.text; };





};
