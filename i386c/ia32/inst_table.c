/*
 * Copyright (c) 2002-2003 NONAKA Kimihiro
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "compiler.h"
#include "cpu.h"

#include "inst_table.h"
#include "groups.h"

#include "ia32/instructions/bin_arith.h"
#include "ia32/instructions/bit_byte.h"
#include "ia32/instructions/ctrl_trans.h"
#include "ia32/instructions/data_trans.h"
#include "ia32/instructions/dec_arith.h"
#include "ia32/instructions/flag_ctrl.h"
#include "ia32/instructions/logic_arith.h"
#include "ia32/instructions/misc_inst.h"
#include "ia32/instructions/seg_reg.h"
#include "ia32/instructions/shift_rotate.h"
#include "ia32/instructions/string_inst.h"
#include "ia32/instructions/system_inst.h"

#include "ia32/instructions/fpu/fp.h"
#include "ia32/instructions/mmx/mmx.h"
#include "ia32/instructions/mmx/3dnow.h"
#include "ia32/instructions/sse/sse.h"
#include "ia32/instructions/sse2/sse2.h"
#include "ia32/instructions/sse3/sse3.h"

/*
 * UNDEF OP
 */
static void
undef_op(void)
{

	EXCEPTION(UD_EXCEPTION, 0);
}

static void CPUCALL
undef_op2(UINT32 v)
{

	EXCEPTION(UD_EXCEPTION, 0);
}


UINT8 insttable_info[256] = {
	0,				/* 00 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,				/* 08 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,

	0,				/* 10 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,				/* 18 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,

	0,				/* 20 */
	0,
	0,
	0,
	0,
	0,
	INST_PREFIX,				/* ES: */
	0,
	0,				/* 28 */
	0,
	0,
	0,
	0,
	0,
	INST_PREFIX,				/* CS: */
	0,

	0,				/* 30 */
	0,
	0,
	0,
	0,
	0,
	INST_PREFIX,				/* SS: */
	0,
	0,				/* 38 */
	0,
	0,
	0,
	0,
	0,
	INST_PREFIX,				/* DS: */
	0,

	0,				/* 40 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,				/* 48 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,

	0,				/* 50 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,				/* 58 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,

	0,				/* 60 */
	0,
	0,
	0,
	INST_PREFIX,				/* FS: */
	INST_PREFIX,				/* GS: */
	INST_PREFIX,				/* OpSize: */
	INST_PREFIX,				/* AddrSize: */
	0,				/* 68 */
	0,
	0,
	0,
	INST_STRING,				/* INSB_YbDX */
	INST_STRING,				/* INSW_YvDX */
	INST_STRING,				/* OUTSB_DXXb */
	INST_STRING,				/* OUTSW_DXXv */

	0,				/* 70 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,				/* 78 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,

	0,				/* 80 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,				/* 88 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,

	0,				/* 90 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,				/* 98 */
	0,
	0,
	INST_PREFIX,				/* FWAIT */
	0,
	0,
	0,
	0,

	0,				/* A0 */
	0,
	0,
	0,
	INST_STRING,				/* MOVSB_XbYb */
	INST_STRING,				/* MOVSW_XvYv */
	INST_STRING | REP_CHECKZF,		/* CMPSB_XbYb */
	INST_STRING | REP_CHECKZF,		/* CMPSW_XvYv */
	0,				/* A8 */
	0,
	INST_STRING,				/* STOSB_YbAL */
	INST_STRING,				/* STOSW_YveAX */
	INST_STRING,				/* LODSB_ALXb */
	INST_STRING,				/* LODSW_eAXXv */
	INST_STRING | REP_CHECKZF,		/* SCASB_ALXb */
	INST_STRING | REP_CHECKZF,		/* SCASW_eAXXv */

	0,				/* B0 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,				/* B8 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,

	0,				/* C0 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,				/* C8 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,

	0,				/* D0 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,				/* D8 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,

	0,				/* E0 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,				/* E8 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,

	INST_PREFIX,			/* F0 *//* LOCK */
	0,
	INST_PREFIX,				/* REPNE */
	INST_PREFIX,				/* REPE */
	0,
	0,
	0,
	0,
	0,				/* F8 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
};

void (*insttable_1byte[2][256])(void) = {
	/* 16bit */
	{
		ADD_EbGb,		/* 00 */
		ADD_EwGw,
		ADD_GbEb,
		ADD_GwEw,
		ADD_ALIb,
		ADD_AXIw,
		PUSH16_ES,
		POP16_ES,
		OR_EbGb,		/* 08 */
		OR_EwGw,
		OR_GbEb,
		OR_GwEw,
		OR_ALIb,
		OR_AXIw,
		PUSH16_CS,
		_2byte_ESC16,

		ADC_EbGb,		/* 10 */
		ADC_EwGw,
		ADC_GbEb,
		ADC_GwEw,
		ADC_ALIb,
		ADC_AXIw,
		PUSH16_SS,
		POP16_SS,
		SBB_EbGb,		/* 18 */
		SBB_EwGw,
		SBB_GbEb,
		SBB_GwEw,
		SBB_ALIb,
		SBB_AXIw,
		PUSH16_DS,
		POP16_DS,

		AND_EbGb,		/* 20 */
		AND_EwGw,
		AND_GbEb,
		AND_GwEw,
		AND_ALIb,
		AND_AXIw,
		Prefix_ES,
		DAA,
		SUB_EbGb,		/* 28 */
		SUB_EwGw,
		SUB_GbEb,
		SUB_GwEw,
		SUB_ALIb,
		SUB_AXIw,
		Prefix_CS,
		DAS,

		XOR_EbGb,		/* 30 */
		XOR_EwGw,
		XOR_GbEb,
		XOR_GwEw,
		XOR_ALIb,
		XOR_AXIw,
		Prefix_SS,
		AAA,
		CMP_EbGb,		/* 38 */
		CMP_EwGw,
		CMP_GbEb,
		CMP_GwEw,
		CMP_ALIb,
		CMP_AXIw,
		Prefix_DS,
		AAS,

		INC_AX,			/* 40 */
		INC_CX,
		INC_DX,
		INC_BX,
		INC_SP,
		INC_BP,
		INC_SI,
		INC_DI,
		DEC_AX,			/* 48 */
		DEC_CX,
		DEC_DX,
		DEC_BX,
		DEC_SP,
		DEC_BP,
		DEC_SI,
		DEC_DI,

		PUSH_AX,		/* 50 */
		PUSH_CX,
		PUSH_DX,
		PUSH_BX,
		PUSH_SP,
		PUSH_BP,
		PUSH_SI,
		PUSH_DI,
		POP_AX,			/* 58 */
		POP_CX,
		POP_DX,
		POP_BX,
		POP_SP,
		POP_BP,
		POP_SI,
		POP_DI,

		PUSHA,			/* 60 */
		POPA,
		BOUND_GwMa,
		ARPL_EwGw,
		Prefix_FS,
		Prefix_GS,
		OpSize,
		AddrSize,
		PUSH_Iw,		/* 68 */
		IMUL_GwEwIw,
		PUSH_Ib,
		IMUL_GwEwIb,
		INSB_YbDX,
		INSW_YwDX,
		OUTSB_DXXb,
		OUTSW_DXXw,

		JO_Jb,			/* 70 */
		JNO_Jb,
		JC_Jb,
		JNC_Jb,
		JZ_Jb,
		JNZ_Jb,
		JNA_Jb,
		JA_Jb,
		JS_Jb,			/* 78 */
		JNS_Jb,
		JP_Jb,
		JNP_Jb,
		JL_Jb,
		JNL_Jb,
		JLE_Jb,
		JNLE_Jb,

		Grp1_EbIb,		/* 80 */
		Grp1_EwIw,
		Grp1_EbIb,
		Grp1_EwIb,
		TEST_EbGb,
		TEST_EwGw,
		XCHG_EbGb,
		XCHG_EwGw,
		MOV_EbGb,		/* 88 */
		MOV_EwGw,
		MOV_GbEb,
		MOV_GwEw,
		MOV_EwSw,
		LEA_GwM,
		MOV_SwEw,
		POP_Ew,

		_NOP,			/* 90 */
		XCHG_CXAX,
		XCHG_DXAX,
		XCHG_BXAX,
		XCHG_SPAX,
		XCHG_BPAX,
		XCHG_SIAX,
		XCHG_DIAX,
		CBW,			/* 98 */
		CWD,
		CALL16_Ap,
		FPU_FWAIT,
		PUSHF_Fw,
		POPF_Fw,
		SAHF,
		LAHF,

		MOV_ALOb,		/* A0 */
		MOV_AXOw,
		MOV_ObAL,
		MOV_OwAX,
		MOVSB_XbYb,
		MOVSW_XwYw,
		CMPSB_XbYb,
		CMPSW_XwYw,
		TEST_ALIb,		/* A8 */
		TEST_AXIw,
		STOSB_YbAL,
		STOSW_YwAX,
		LODSB_ALXb,
		LODSW_AXXw,
		SCASB_ALXb,
		SCASW_AXXw,

		MOV_ALIb,		/* B0 */
		MOV_CLIb,
		MOV_DLIb,
		MOV_BLIb,
		MOV_AHIb,
		MOV_CHIb,
		MOV_DHIb,
		MOV_BHIb,
		MOV_AXIw,		/* B8 */
		MOV_CXIw,
		MOV_DXIw,
		MOV_BXIw,
		MOV_SPIw,
		MOV_BPIw,
		MOV_SIIw,
		MOV_DIIw,

		Grp2_EbIb,		/* C0 */
		Grp2_EwIb,
		RETnear16_Iw,
		RETnear16,
		LES_GwMp,
		LDS_GwMp,
		MOV_EbIb,
		MOV_EwIw,
		ENTER16_IwIb,		/* C8 */
		LEAVE,
		RETfar16_Iw,
		RETfar16,
		INT3,
		INT_Ib,
		INTO,
		IRET,

		Grp2_Eb,		/* D0 */
		Grp2_Ew,
		Grp2_EbCL,
		Grp2_EwCL,
		AAM,
		AAD,
		SALC,				/* undoc(8086) */
		XLAT,
		NOFPU_ESC0,			/* D8 */
		NOFPU_ESC1,
		NOFPU_ESC2,
		NOFPU_ESC3,
		NOFPU_ESC4,
		NOFPU_ESC5,
		NOFPU_ESC6,
		NOFPU_ESC7,

		LOOPNE_Jb,		/* E0 */
		LOOPE_Jb,
		LOOP_Jb,
		JeCXZ_Jb,
		IN_ALIb,
		IN_AXIb,
		OUT_IbAL,
		OUT_IbAX,
		CALL_Aw,		/* E8 */
		JMP_Jw,
		JMP16_Ap,
		JMP_Jb,
		IN_ALDX,
		IN_AXDX,
		OUT_DXAL,
		OUT_DXAX,

		_LOCK,			/* F0 */
		INT1,
		_REPNE,
		_REPE,
		HLT,
		CMC,
		Grp3_Eb,
		Grp3_Ew,
		CLC,			/* F8 */
		STC,
		CLI,
		STI,
		CLD,
		STD,
		Grp4,
		Grp5_Ew,
	},

	/* 32bit */
	{
		ADD_EbGb,		/* 00 */
		ADD_EdGd,
		ADD_GbEb,
		ADD_GdEd,
		ADD_ALIb,
		ADD_EAXId,
		PUSH32_ES,
		POP32_ES,
		OR_EbGb,		/* 08 */
		OR_EdGd,
		OR_GbEb,
		OR_GdEd,
		OR_ALIb,
		OR_EAXId,
		PUSH32_CS,
		_2byte_ESC32,

		ADC_EbGb,		/* 10 */
		ADC_EdGd,
		ADC_GbEb,
		ADC_GdEd,
		ADC_ALIb,
		ADC_EAXId,
		PUSH32_SS,
		POP32_SS,
		SBB_EbGb,		/* 18 */
		SBB_EdGd,
		SBB_GbEb,
		SBB_GdEd,
		SBB_ALIb,
		SBB_EAXId,
		PUSH32_DS,
		POP32_DS,

		AND_EbGb,		/* 20 */
		AND_EdGd,
		AND_GbEb,
		AND_GdEd,
		AND_ALIb,
		AND_EAXId,
		undef_op,			/* Prefix_ES */
		DAA,
		SUB_EbGb,		/* 28 */
		SUB_EdGd,
		SUB_GbEb,
		SUB_GdEd,
		SUB_ALIb,
		SUB_EAXId,
		undef_op,			/* Prefix_CS */
		DAS,

		XOR_EbGb,		/* 30 */
		XOR_EdGd,
		XOR_GbEb,
		XOR_GdEd,
		XOR_ALIb,
		XOR_EAXId,
		undef_op,			/* Prefix_SS */
		AAA,
		CMP_EbGb,		/* 38 */
		CMP_EdGd,
		CMP_GbEb,
		CMP_GdEd,
		CMP_ALIb,
		CMP_EAXId,
		undef_op,			/* Prefix_DS */
		AAS,

		INC_EAX,		/* 40 */
		INC_ECX,
		INC_EDX,
		INC_EBX,
		INC_ESP,
		INC_EBP,
		INC_ESI,
		INC_EDI,
		DEC_EAX,		/* 48 */
		DEC_ECX,
		DEC_EDX,
		DEC_EBX,
		DEC_ESP,
		DEC_EBP,
		DEC_ESI,
		DEC_EDI,

		PUSH_EAX,		/* 50 */
		PUSH_ECX,
		PUSH_EDX,
		PUSH_EBX,
		PUSH_ESP,
		PUSH_EBP,
		PUSH_ESI,
		PUSH_EDI,
		POP_EAX,		/* 58 */
		POP_ECX,
		POP_EDX,
		POP_EBX,
		POP_ESP,
		POP_EBP,
		POP_ESI,
		POP_EDI,

		PUSHAD,			/* 60 */
		POPAD,
		BOUND_GdMa,
		ARPL_EwGw,
		undef_op,			/* Prefix_FS */
		undef_op,			/* Prefix_GS */
		undef_op,			/* OpSize */
		undef_op,			/* AddrSize */
		PUSH_Id,		/* 68 */
		IMUL_GdEdId,
		PUSH_Ib,
		IMUL_GdEdIb,
		INSB_YbDX,
		INSD_YdDX,
		OUTSB_DXXb,
		OUTSD_DXXd,

		JO_Jb,			/* 70 */
		JNO_Jb,
		JC_Jb,
		JNC_Jb,
		JZ_Jb,
		JNZ_Jb,
		JNA_Jb,
		JA_Jb,
		JS_Jb,			/* 78 */
		JNS_Jb,
		JP_Jb,
		JNP_Jb,
		JL_Jb,
		JNL_Jb,
		JLE_Jb,
		JNLE_Jb,

		Grp1_EbIb,		/* 80 */
		Grp1_EdId,
		Grp1_EbIb,
		Grp1_EdIb,
		TEST_EbGb,
		TEST_EdGd,
		XCHG_EbGb,
		XCHG_EdGd,
		MOV_EbGb,		/* 88 */
		MOV_EdGd,
		MOV_GbEb,
		MOV_GdEd,
		MOV_EdSw,
		LEA_GdM,
		MOV_SwEw,
		POP_Ed,

		_NOP,			/* 90 */
		XCHG_ECXEAX,
		XCHG_EDXEAX,
		XCHG_EBXEAX,
		XCHG_ESPEAX,
		XCHG_EBPEAX,
		XCHG_ESIEAX,
		XCHG_EDIEAX,
		CWDE,			/* 98 */
		CDQ,
		CALL32_Ap,
		undef_op,			/* FWAIT */
		PUSHFD_Fd,
		POPFD_Fd,
		SAHF,
		LAHF,

		MOV_ALOb,		/* A0 */
		MOV_EAXOd,
		MOV_ObAL,
		MOV_OdEAX,
		MOVSB_XbYb,
		MOVSD_XdYd,
		CMPSB_XbYb,
		CMPSD_XdYd,
		TEST_ALIb,		/* A8 */
		TEST_EAXId,
		STOSB_YbAL,
		STOSD_YdEAX,
		LODSB_ALXb,
		LODSD_EAXXd,
		SCASB_ALXb,
		SCASD_EAXXd,

		MOV_ALIb,		/* B0 */
		MOV_CLIb,
		MOV_DLIb,
		MOV_BLIb,
		MOV_AHIb,
		MOV_CHIb,
		MOV_DHIb,
		MOV_BHIb,
		MOV_EAXId,		/* B8 */
		MOV_ECXId,
		MOV_EDXId,
		MOV_EBXId,
		MOV_ESPId,
		MOV_EBPId,
		MOV_ESIId,
		MOV_EDIId,

		Grp2_EbIb,		/* C0 */
		Grp2_EdIb,
		RETnear32_Iw,
		RETnear32,
		LES_GdMp,
		LDS_GdMp,
		MOV_EbIb,
		MOV_EdId,
		ENTER32_IwIb,		/* C8 */
		LEAVE,
		RETfar32_Iw,
		RETfar32,
		INT3,
		INT_Ib,
		INTO,
		IRET,

		Grp2_Eb,		/* D0 */
		Grp2_Ed,
		Grp2_EbCL,
		Grp2_EdCL,
		AAM,
		AAD,
		SALC,				/* undoc(8086) */
		XLAT,
		NOFPU_ESC0,			/* D8 */
		NOFPU_ESC1,
		NOFPU_ESC2,
		NOFPU_ESC3,
		NOFPU_ESC4,
		NOFPU_ESC5,
		NOFPU_ESC6,
		NOFPU_ESC7,

		LOOPNE_Jb,		/* E0 */
		LOOPE_Jb,
		LOOP_Jb,
		JeCXZ_Jb,
		IN_ALIb,
		IN_EAXIb,
		OUT_IbAL,
		OUT_IbEAX,
		CALL_Ad,		/* E8 */
		JMP_Jd,
		JMP32_Ap,
		JMP_Jb,
		IN_ALDX,
		IN_EAXDX,
		OUT_DXAL,
		OUT_DXEAX,

		_LOCK,			/* F0 */
		INT1,
		undef_op,			/* repne */
		undef_op,			/* repe */
		HLT,
		CMC,
		Grp3_Eb,
		Grp3_Ed,
		CLC,			/* F8 */
		STC,
		CLI,
		STI,
		CLD,
		STD,
		Grp4,
		Grp5_Ed,
	},
};


void (*insttable_1byte_repfunc[2][256])(int reptype) = {
	/* 16bit */
	{
		NULL,		/* 00 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,		/* 08 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,

		NULL,		/* 10 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,		/* 18 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,

		NULL,		/* 20 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,		/* 28 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,

		NULL,		/* 30 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,		/* 38 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,

		NULL,			/* 40 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,			/* 48 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,

		NULL,		/* 50 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,		/* 58 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,

		NULL,	/* 60 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,	/* 68 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,

		NULL,		/* 70 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,		/* 78 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,

		NULL,		/* 80 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,		/* 88 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,

		NULL,	/* 90 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,	/* 98 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,

		NULL,	/* A0 */
		NULL,
		NULL,
		NULL,
		MOVSB_XbYb_rep,
		MOVSW_XwYw_rep,
		CMPSB_XbYb_rep,
		CMPSW_XwYw_rep,
		NULL,	/* A8 */
		NULL,
		STOSB_YbAL_rep,
		STOSW_YwAX_rep,
		NULL,
		NULL,
		NULL,
		NULL,

		NULL,		/* B0 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,		/* B8 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,

		NULL,	/* C0 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,		/* C8 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,

		NULL,	/* D0 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,		/* undoc(8086) */
		NULL,
		NULL,			/* D8 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,

		NULL,		/* E0 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,	/* E8 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,

		NULL,		/* F0 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,		/* F8 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
	},

	/* 32bit */
	{
		NULL,		/* 00 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,		/* 08 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,

		NULL,		/* 10 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,		/* 18 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,

		NULL,		/* 20 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,		/* 28 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,

		NULL,		/* 30 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,		/* 38 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,

		NULL,			/* 40 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,			/* 48 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,

		NULL,		/* 50 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,		/* 58 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,

		NULL,	/* 60 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,	/* 68 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,

		NULL,		/* 70 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,		/* 78 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,

		NULL,		/* 80 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,		/* 88 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,

		NULL,	/* 90 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,	/* 98 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,

		NULL,	/* A0 */
		NULL,
		NULL,
		NULL,
		MOVSB_XbYb_rep,
		MOVSD_XdYd_rep,
		CMPSB_XbYb_rep,
		CMPSD_XdYd_rep,
		NULL,	/* A8 */
		NULL,
		STOSB_YbAL_rep,
		STOSD_YdEAX_rep,
		NULL,
		NULL,
		NULL,
		NULL,

		NULL,		/* B0 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,		/* B8 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,

		NULL,	/* C0 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,		/* C8 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,

		NULL,	/* D0 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,		/* undoc(8086) */
		NULL,
		NULL,			/* D8 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,

		NULL,		/* E0 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,	/* E8 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,

		NULL,		/* F0 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,		/* F8 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
	},
};

void (*insttable_2byte[2][256])(void) = {
	/* 16bit */
	{
		Grp6,			/* 00 */
		Grp7,
		LAR_GwEw,
		LSL_GwEw,
		undef_op,
		LOADALL286,			/* undoc(286) */
		CLTS,
		LOADALL,
		INVD,			/* 08 */
		WBINVD,
		undef_op,
		UD2,
		undef_op,
		AMD3DNOW_PREFETCH,
		AMD3DNOW_FEMMS,
		undef_op,

		SSE_MOVUPSmem2xmm,		/* 10 */
		SSE_MOVUPSxmm2mem,
		SSE_MOVLPSmem2xmm, // + MOVHLPS
		SSE_MOVLPSxmm2mem,
		SSE_UNPCKLPS,
		SSE_UNPCKHPS,
		SSE_MOVHPSmem2xmm, // + MOVLHPS
		SSE_MOVHPSxmm2mem,
		SSE_PREFETCHTx,		/* 18 */
		SSE_NOPPREFETCH,
		SSE_NOPPREFETCH,
		SSE_NOPPREFETCH,
		SSE_NOPPREFETCH,
		SSE_NOPPREFETCH,
		SSE_NOPPREFETCH,
		SSE_NOPPREFETCH,

		MOV_RdCd,		/* 20 */
		MOV_RdDd,
		MOV_CdRd,
		MOV_DdRd,
		MOV_RdTd,
		undef_op,
		MOV_TdRd,
		undef_op,
		SSE_MOVAPSmem2xmm,		/* 28 */
		SSE_MOVAPSxmm2mem,
		SSE_CVTPI2PS,
		SSE_MOVNTPS,
		SSE_CVTTPS2PI,
		SSE_CVTPS2PI,
		SSE_UCOMISS,
		SSE_COMISS,

		WRMSR,			/* 30 */
		RDTSC,
		RDMSR,
		RDPMC,
		SYSENTER,
		SYSEXIT,
		undef_op,
		undef_op,
		undef_op,		/* 38 */
		undef_op,
		undef_op,
		undef_op,
		undef_op,
		undef_op,
		undef_op,
		undef_op,

		CMOVO_GwEw,		/* 40 */
		CMOVNO_GwEw,
		CMOVC_GwEw,
		CMOVNC_GwEw,
		CMOVZ_GwEw,
		CMOVNZ_GwEw,
		CMOVNA_GwEw,
		CMOVA_GwEw,
		CMOVS_GwEw,		/* 48 */
		CMOVNS_GwEw,
		CMOVP_GwEw,
		CMOVNP_GwEw,
		CMOVL_GwEw,
		CMOVNL_GwEw,
		CMOVLE_GwEw,
		CMOVNLE_GwEw,

		SSE_MOVMSKPS,		/* 50 */
		SSE_SQRTPS,
		SSE_RSQRTPS,
		SSE_RCPPS,
		SSE_ANDPS,
		SSE_ANDNPS,
		SSE_ORPS,
		SSE_XORPS,
		SSE_ADDPS,		/* 58 */
		SSE_MULPS,
		SSE2_CVTPS2PD,
		SSE2_CVTDQ2PS,
		SSE_SUBPS,
		SSE_MINPS,
		SSE_DIVPS,
		SSE_MAXPS,
		
		MMX_PUNPCKLBW,		/* 60 */
		MMX_PUNPCKLWD,
		MMX_PUNPCKLDQ,
		MMX_PACKSSWB,
		MMX_PCMPGTB,
		MMX_PCMPGTW,
		MMX_PCMPGTD,
		MMX_PACKUSWB,
		MMX_PUNPCKHBW,		/* 68 */
		MMX_PUNPCKHWD,
		MMX_PUNPCKHDQ,
		MMX_PACKSSDW,
		undef_op,
		undef_op,
		MMX_MOVD_mm_rm32,
		MMX_MOVQ_mm_mmm64,

		SSE_PSHUFW,		/* 70 */
		MMX_PSxxW_imm8,
		MMX_PSxxD_imm8,
		MMX_PSxxQ_imm8,
		MMX_PCMPEQB,
		MMX_PCMPEQW,
		MMX_PCMPEQD,
		MMX_EMMS,
		undef_op,		/* 78 */
		undef_op,
		undef_op,
		undef_op,
		undef_op,
		undef_op,
		MMX_MOVD_rm32_mm,
		MMX_MOVQ_mmm64_mm,

		JO_Jw,			/* 80 */
		JNO_Jw,
		JC_Jw,
		JNC_Jw,
		JZ_Jw,
		JNZ_Jw,
		JNA_Jw,
		JA_Jw,
		JS_Jw,			/* 88 */
		JNS_Jw,
		JP_Jw,
		JNP_Jw,
		JL_Jw,
		JNL_Jw,
		JLE_Jw,
		JNLE_Jw,

		SETO_Eb,		/* 90 */
		SETNO_Eb,
		SETC_Eb,
		SETNC_Eb,
		SETZ_Eb,
		SETNZ_Eb,
		SETNA_Eb,
		SETA_Eb,
		SETS_Eb,		/* 98 */
		SETNS_Eb,
		SETP_Eb,
		SETNP_Eb,
		SETL_Eb,
		SETNL_Eb,
		SETLE_Eb,
		SETNLE_Eb,

		PUSH16_FS,		/* A0 */
		POP16_FS,
		_CPUID,
		BT_EwGw,
		SHLD_EwGwIb,
		SHLD_EwGwCL,
		CMPXCHG_EbGb,			/* undoc(486) */
		CMPXCHG_EwGw,			/* undoc(486) */
		PUSH16_GS,		/* A8 */
		POP16_GS,
		RSM,
		BTS_EwGw,
		SHRD_EwGwIb,
		SHRD_EwGwCL,
		NOFPU_FPU_FXSAVERSTOR, // + LDMXCSR + STMXCSR + SFENCE + LFENCE + CLFLUSH
		IMUL_GwEw,

		CMPXCHG_EbGb,		/* B0 */
		CMPXCHG_EwGw,
		LSS_GwMp,
		BTR_EwGw,
		LFS_GwMp,
		LGS_GwMp,
		MOVZX_GwEb,
		MOVZX_GwEw,
		undef_op,		/* B8 */
		UD2,
		Grp8_EwIb,
		BTC_EwGw,
		BSF_GwEw,
		BSR_GwEw,
		MOVSX_GwEb,
		MOVSX_GwEw,

		XADD_EbGb,		/* C0 */
		XADD_EwGw,
		SSE_CMPPS,
		undef_op,
		SSE_PINSRW,
		SSE_PEXTRW,
		SSE_SHUFPS,
		Grp9,
		BSWAP_EAX,		/* C8 */
		BSWAP_ECX,
		BSWAP_EDX,
		BSWAP_EBX,
		BSWAP_ESP,
		BSWAP_EBP,
		BSWAP_ESI,
		BSWAP_EDI,

		undef_op,		/* D0 */
		MMX_PSRLW,
		MMX_PSRLD,
		MMX_PSRLQ,
		SSE2_PADDQmm,
		MMX_PMULLW,
		undef_op,
		SSE_PMOVMSKB,
		MMX_PSUBUSB,		/* D8 */
		MMX_PSUBUSW,
		SSE_PMINUB,
		MMX_PAND,
		MMX_PADDUSB,
		MMX_PADDUSW,
		SSE_PMAXUB,
		MMX_PANDN,

		SSE_PAVGB,		/* E0 */
		MMX_PSRAW,
		MMX_PSRAD,
		SSE_PAVGW,
		SSE_PMULHUW,
		MMX_PMULHW,
		undef_op,
		SSE_MOVNTQ,
		MMX_PSUBSB,		/* E8 */
		MMX_PSUBSW,
		SSE_PMINSW,
		MMX_POR,
		MMX_PADDSB,
		MMX_PADDSW,
		SSE_PMAXSW,
		MMX_PXOR,

		AMD3DNOW_F0,		/* F0 */
		MMX_PSLLW,
		MMX_PSLLD,
		MMX_PSLLQ,
		SSE2_PMULUDQmm,
		MMX_PMADDWD,
		SSE_PSADBW,
		SSE_MASKMOVQ,
		MMX_PSUBB,		/* F8 */
		MMX_PSUBW,
		MMX_PSUBD,
		SSE2_PSUBQmm,
		MMX_PADDB,
		MMX_PADDW,
		MMX_PADDD,
		undef_op,
	},

	/* 32bit */
	{
		Grp6,			/* 00 */
		Grp7,
		LAR_GdEw,
		LSL_GdEw,
		undef_op,
		LOADALL286,			/* undoc(286) */
		CLTS,
		LOADALL,
		INVD,			/* 08 */
		WBINVD,
		undef_op,
		UD2,
		undef_op,
		AMD3DNOW_PREFETCH,
		AMD3DNOW_FEMMS,
		undef_op,
		
		SSE_MOVUPSmem2xmm,		/* 10 */
		SSE_MOVUPSxmm2mem,
		SSE_MOVLPSmem2xmm, // + MOVHLPS
		SSE_MOVLPSxmm2mem,
		SSE_UNPCKLPS,
		SSE_UNPCKHPS,
		SSE_MOVHPSmem2xmm, // + MOVLHPS
		SSE_MOVHPSxmm2mem,
		SSE_PREFETCHTx,		/* 18 */
		SSE_NOPPREFETCH,
		SSE_NOPPREFETCH,
		SSE_NOPPREFETCH,
		SSE_NOPPREFETCH,
		SSE_NOPPREFETCH,
		SSE_NOPPREFETCH,
		SSE_NOPPREFETCH,

		MOV_RdCd,		/* 20 */
		MOV_RdDd,
		MOV_CdRd,
		MOV_DdRd,
		MOV_RdTd,
		undef_op,
		MOV_TdRd,
		undef_op,
		SSE_MOVAPSmem2xmm,		/* 28 */
		SSE_MOVAPSxmm2mem,
		SSE_CVTPI2PS,
		SSE_MOVNTPS,
		SSE_CVTTPS2PI,
		SSE_CVTPS2PI,
		SSE_UCOMISS,
		SSE_COMISS,

		WRMSR,			/* 30 */
		RDTSC,
		RDMSR,
		RDPMC,
		SYSENTER,
		SYSEXIT,
		undef_op,
		undef_op,
		undef_op,		/* 38 */
		undef_op,
		undef_op,
		undef_op,
		undef_op,
		undef_op,
		undef_op,
		undef_op,

		CMOVO_GdEd,		/* 40 */
		CMOVNO_GdEd,
		CMOVC_GdEd,
		CMOVNC_GdEd,
		CMOVZ_GdEd,
		CMOVNZ_GdEd,
		CMOVNA_GdEd,
		CMOVA_GdEd,
		CMOVS_GdEd,		/* 48 */
		CMOVNS_GdEd,
		CMOVP_GdEd,
		CMOVNP_GdEd,
		CMOVL_GdEd,
		CMOVNL_GdEd,
		CMOVLE_GdEd,
		CMOVNLE_GdEd,
		
		SSE_MOVMSKPS,		/* 50 */
		SSE_SQRTPS,
		SSE_RSQRTPS,
		SSE_RCPPS,
		SSE_ANDPS,
		SSE_ANDNPS,
		SSE_ORPS,
		SSE_XORPS,
		SSE_ADDPS,		/* 58 */
		SSE_MULPS,
		SSE2_CVTPS2PD,
		SSE2_CVTDQ2PS,
		SSE_SUBPS,
		SSE_MINPS,
		SSE_DIVPS,
		SSE_MAXPS,

		MMX_PUNPCKLBW,		/* 60 */
		MMX_PUNPCKLWD,
		MMX_PUNPCKLDQ,
		MMX_PACKSSWB,
		MMX_PCMPGTB,
		MMX_PCMPGTW,
		MMX_PCMPGTD,
		MMX_PACKUSWB,
		MMX_PUNPCKHBW,		/* 68 */
		MMX_PUNPCKHWD,
		MMX_PUNPCKHDQ,
		MMX_PACKSSDW,
		undef_op,
		undef_op,
		MMX_MOVD_mm_rm32,
		MMX_MOVQ_mm_mmm64,
		
		SSE_PSHUFW,		/* 70 */
		MMX_PSxxW_imm8,
		MMX_PSxxD_imm8,
		MMX_PSxxQ_imm8,
		MMX_PCMPEQB,
		MMX_PCMPEQW,
		MMX_PCMPEQD,
		MMX_EMMS,
		undef_op,		/* 78 */
		undef_op,
		undef_op,
		undef_op,
		undef_op,
		undef_op,
		MMX_MOVD_rm32_mm,
		MMX_MOVQ_mmm64_mm,

		JO_Jd,			/* 80 */
		JNO_Jd,
		JC_Jd,
		JNC_Jd,
		JZ_Jd,
		JNZ_Jd,
		JNA_Jd,
		JA_Jd,
		JS_Jd,			/* 88 */
		JNS_Jd,
		JP_Jd,
		JNP_Jd,
		JL_Jd,
		JNL_Jd,
		JLE_Jd,
		JNLE_Jd,

		SETO_Eb,		/* 90 */
		SETNO_Eb,
		SETC_Eb,
		SETNC_Eb,
		SETZ_Eb,
		SETNZ_Eb,
		SETNA_Eb,
		SETA_Eb,
		SETS_Eb,		/* 98 */
		SETNS_Eb,
		SETP_Eb,
		SETNP_Eb,
		SETL_Eb,
		SETNL_Eb,
		SETLE_Eb,
		SETNLE_Eb,

		PUSH32_FS,		/* A0 */
		POP32_FS,
		_CPUID,
		BT_EdGd,
		SHLD_EdGdIb,
		SHLD_EdGdCL,
		CMPXCHG_EbGb,			/* undoc(486) */
		CMPXCHG_EdGd,			/* undoc(486) */
		PUSH32_GS,		/* A8 */
		POP32_GS,
		RSM,
		BTS_EdGd,
		SHRD_EdGdIb,
		SHRD_EdGdCL,
		NOFPU_FPU_FXSAVERSTOR, // + LDMXCSR + STMXCSR + SFENCE + LFENCE + CLFLUSH
		IMUL_GdEd,

		CMPXCHG_EbGb,		/* B0 */
		CMPXCHG_EdGd,
		LSS_GdMp,
		BTR_EdGd,
		LFS_GdMp,
		LGS_GdMp,
		MOVZX_GdEb,
		MOVZX_GdEw,
		undef_op,		/* B8 */
		UD2,
		Grp8_EdIb,
		BTC_EdGd,
		BSF_GdEd,
		BSR_GdEd,
		MOVSX_GdEb,
		MOVSX_GdEw,

		XADD_EbGb,		/* C0 */
		XADD_EdGd,
		SSE_CMPPS,
		undef_op,
		SSE_PINSRW,
		SSE_PEXTRW,
		SSE_SHUFPS,
		Grp9,
		BSWAP_EAX,		/* C8 */
		BSWAP_ECX,
		BSWAP_EDX,
		BSWAP_EBX,
		BSWAP_ESP,
		BSWAP_EBP,
		BSWAP_ESI,
		BSWAP_EDI,

		undef_op,		/* D0 */
		MMX_PSRLW,
		MMX_PSRLD,
		MMX_PSRLQ,
		SSE2_PADDQmm,
		MMX_PMULLW,
		undef_op,
		SSE_PMOVMSKB,
		MMX_PSUBUSB,		/* D8 */
		MMX_PSUBUSW,
		SSE_PMINUB,
		MMX_PAND,
		MMX_PADDUSB,
		MMX_PADDUSW,
		SSE_PMAXUB,
		MMX_PANDN,
		
		SSE_PAVGB,		/* E0 */
		MMX_PSRAW,
		MMX_PSRAD,
		SSE_PAVGW,
		SSE_PMULHUW,
		MMX_PMULHW,
		undef_op,
		SSE_MOVNTQ,
		MMX_PSUBSB,		/* E8 */
		MMX_PSUBSW,
		SSE_PMINSW,
		MMX_POR,
		MMX_PADDSB,
		MMX_PADDSW,
		SSE_PMAXSW,
		MMX_PXOR,

		AMD3DNOW_F0,		/* F0 */
		MMX_PSLLW,
		MMX_PSLLD,
		MMX_PSLLQ,
		SSE2_PMULUDQmm,
		MMX_PMADDWD,
		SSE_PSADBW,
		SSE_MASKMOVQ,
		MMX_PSUBB,		/* F8 */
		MMX_PSUBW,
		MMX_PSUBD,
		SSE2_PSUBQmm,
		MMX_PADDB,
		MMX_PADDW,
		MMX_PADDD,
		undef_op,
	},
};

void (*insttable_2byte660F_32[256])(void) = {
	NULL,		/* 00 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,		
	NULL,
	NULL,
	NULL,		/* 08 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	SSE2_MOVUPDmem2xmm,		/* 10 */
	SSE2_MOVUPDxmm2mem,
	SSE2_MOVLPDmem2xmm,
	SSE2_MOVLPDxmm2mem,
	SSE2_UNPCKLPD,
	SSE2_UNPCKHPD,
	SSE2_MOVHPDmem2xmm,
	SSE2_MOVHPDxmm2mem,
	NULL,		/* 18 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	NULL,		/* 20 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	SSE2_MOVAPDmem2xmm,		/* 28 */
	SSE2_MOVAPDxmm2mem,
	SSE2_CVTPI2PD,
	SSE2_MOVNTPD,
	SSE2_CVTTPD2PI,
	SSE2_CVTPD2PI,
	SSE2_UCOMISD,
	SSE2_COMISD,

	NULL,		/* 30 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,		/* 38 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	NULL,		/* 40 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,		/* 48 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	SSE2_MOVMSKPD,		/* 50 */
	SSE2_SQRTPD,
	NULL,
	NULL,
	SSE2_ANDPD,
	SSE2_ANDNPD,
	SSE2_ORPD,
	SSE2_XORPD,
	SSE2_ADDPD,		/* 58 */
	SSE2_MULPD,
	SSE2_CVTPD2PS,
	SSE2_CVTPS2DQ,
	SSE2_SUBPD,
	SSE2_MINPD,
	SSE2_DIVPD,
	SSE2_MAXPD,

	SSE2_PUNPCKLBW,		/* 60 */
	SSE2_PUNPCKLWD,
	SSE2_PUNPCKLDQ,
	SSE2_PACKSSWB,
	SSE2_PCMPGTB,
	SSE2_PCMPGTW,
	SSE2_PCMPGTD,
	SSE2_PACKUSWB,
	SSE2_PUNPCKHBW,		/* 68 */
	SSE2_PUNPCKHWD,
	SSE2_PUNPCKHDQ,
	SSE2_PACKSSDW,
	SSE2_PUNPCKLQDQ,
	SSE2_PUNPCKHQDQ,
	SSE2_MOVDrm2xmm,
	SSE2_MOVDQAmem2xmm,

	SSE2_PSHUFD,		/* 70 */
	SSE2_PSxxWimm, // PSLLWimm + PSRAWimm + PSRLWimm
	SSE2_PSxxDimm, // PSLLDimm + PSRADimm + PSRLDimm
	SSE2_PSxxQimm, // PSLLQimm + PSRAQimm + PSRLQimm + PSLLDQ + PSRLDQ
	SSE2_PCMPEQB,
	SSE2_PCMPEQW,
	SSE2_PCMPEQD,
	NULL,
	NULL,		/* 78 */
	NULL,
	NULL,
	NULL,
	SSE3_HADDPD,
	SSE3_HSUBPD,
	SSE2_MOVDxmm2rm,
	SSE2_MOVDQAxmm2mem,

	NULL,		/* 80 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,		/* 88 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	NULL,		/* 90 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,		/* 98 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	NULL,		/* A0 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,		
	NULL,		
	NULL,		/* A8 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	NULL,		/* B0 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,		/* B8 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	NULL,		/* C0 */
	NULL,
	SSE2_CMPPD,
	SSE2_MOVNTI,
	SSE2_PINSRW,
	SSE2_PEXTRW,
	SSE2_SHUFPD,
	NULL,
	NULL,		/* C8 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	SSE3_ADDSUBPD,		/* D0 */
	SSE2_PSRLW,
	SSE2_PSRLD,
	SSE2_PSRLQ,
	SSE2_PADDQxmm,
	SSE2_PMULLW,
	SSE2_MOVQxmm2mem,
	SSE2_PMOVMSKB,
	SSE2_PSUBUSB,		/* D8 */
	SSE2_PSUBUSW,
	SSE2_PMINUB,
	SSE2_PAND,
	SSE2_PADDUSB,
	SSE2_PADDUSW,
	SSE2_PMAXUB,
	SSE2_PANDN,

	SSE2_PAVGB,		/* E0 */
	SSE2_PSRAW,
	SSE2_PSRAD,
	SSE2_PAVGW,
	SSE2_PMULHUW,
	SSE2_PMULHW,
	SSE2_CVTTPD2DQ,
	SSE2_MOVNTDQ,
	SSE2_PSUBSB,		/* E8 */
	SSE2_PSUBSW,
	SSE2_PMINSW,
	SSE2_POR,
	SSE2_PADDSB,
	SSE2_PADDSW,
	SSE2_PMAXSW,
	SSE2_PXOR,

	NULL,		/* F0 */
	SSE2_PSLLW,
	SSE2_PSLLD,
	SSE2_PSLLQ,
	SSE2_PMULUDQxmm,
	SSE2_PMADD,
	SSE2_PSADBW,
	SSE2_MASKMOVDQU,
	SSE2_PSUBB,		/* F8 */
	SSE2_PSUBW,
	SSE2_PSUBD,
	SSE2_PSUBQxmm,
	SSE2_PADDB,
	SSE2_PADDW,
	SSE2_PADDD,
	NULL,
};

void (*insttable_2byteF20F_32[256])(void) = {
	NULL,		/* 00 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,		
	NULL,
	NULL,
	NULL,		/* 08 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	SSE2_MOVSDmem2xmm,		/* 10 */
	SSE2_MOVSDxmm2mem,
	SSE3_MOVDDUP,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,		/* 18 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	NULL,		/* 20 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,		/* 28 */
	NULL,
	SSE2_CVTSI2SD,
	NULL,
	SSE2_CVTTSD2SI,
	SSE2_CVTSD2SI,
	NULL,
	NULL,

	NULL,		/* 30 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,		/* 38 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	NULL,		/* 40 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,		/* 48 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	NULL,		/* 50 */
	SSE2_SQRTSD,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	SSE2_ADDSD,		/* 58 */
	SSE2_MULSD,
	SSE2_CVTSD2SS,
	NULL,
	SSE2_SUBSD,
	SSE2_MINSD,
	SSE2_DIVSD,
	SSE2_MAXSD,

	NULL,		/* 60 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,		/* 68 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	SSE2_PSHUFLW,		/* 70 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,		/* 78 */
	NULL,
	NULL,
	NULL,
	SSE3_HADDPS,
	SSE3_HSUBPS,
	NULL,
	NULL,

	NULL,		/* 80 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,		/* 88 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	NULL,		/* 90 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,		/* 98 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	NULL,		/* A0 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,		
	NULL,		
	NULL,		/* A8 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	NULL,		/* B0 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,		/* B8 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	NULL,		/* C0 */
	NULL,
	SSE2_CMPSD,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,		/* C8 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	SSE3_ADDSUBPS,		/* D0 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	SSE2_MOVDQ2Q,
	NULL,
	NULL,		/* D8 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	NULL,		/* E0 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	SSE2_CVTPD2DQ,
	NULL,
	NULL,		/* E8 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	SSE3_LDDQU,		/* F0 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,		/* F8 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
};

void (*insttable_2byteF30F_32[256])(void) = {
	NULL,		/* 00 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,		
	NULL,
	NULL,
	NULL,		/* 08 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	SSE_MOVSSmem2xmm,		/* 10 */
	SSE_MOVSSxmm2mem,
	SSE3_MOVSLDUP,
	NULL,
	NULL,
	NULL,
	SSE3_MOVSHDUP,
	NULL,
	NULL,		/* 18 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	NULL,		/* 20 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,		/* 28 */
	NULL,
	SSE_CVTSI2SS,
	NULL,
	SSE_CVTTSS2SI,
	SSE_CVTSS2SI,
	NULL,
	NULL,

	NULL,		/* 30 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,		/* 38 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	NULL,		/* 40 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,		/* 48 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	NULL,		/* 50 */
	SSE_SQRTSS,
	SSE_RSQRTSS,
	SSE_RCPSS,
	NULL,
	NULL,
	NULL,
	NULL,
	SSE_ADDSS,		/* 58 */
	SSE_MULSS,
	SSE2_CVTSS2SD,
	SSE2_CVTTPS2DQ,
	SSE_SUBSS,
	SSE_MINSS,
	SSE_DIVSS,
	SSE_MAXSS,

	NULL,		/* 60 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,		/* 68 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	SSE2_MOVDQUmem2xmm,

	SSE2_PSHUFHW,		/* 70 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,		/* 78 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	SSE2_MOVQmem2xmm,
	SSE2_MOVDQUxmm2mem,

	NULL,		/* 80 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,		/* 88 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	NULL,		/* 90 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,		/* 98 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	NULL,		/* A0 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,		
	NULL,		
	NULL,		/* A8 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	NULL,		/* B0 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,		/* B8 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	NULL,		/* C0 */
	NULL,
	SSE_CMPSS,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,		/* C8 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	NULL,		/* D0 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	SSE2_MOVQ2DQ,
	NULL,
	NULL,		/* D8 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	NULL,		/* E0 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	SSE2_CVTDQ2PD,
	NULL,
	NULL,		/* E8 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	NULL,		/* F0 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,		/* F8 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
};



/*
 * for group
 */

/* group 1 */
void (CPUCALL *insttable_G1EbIb[])(UINT8 *, UINT32) = {
	ADD_EbIb,
	OR_EbIb,
	ADC_EbIb,
	SBB_EbIb,
	AND_EbIb,
	SUB_EbIb,
	XOR_EbIb,
	CMP_EbIb,
};
void (CPUCALL *insttable_G1EbIb_ext[])(UINT32, UINT32) = {
	ADD_EbIb_ext,
	OR_EbIb_ext,
	ADC_EbIb_ext,
	SBB_EbIb_ext,
	AND_EbIb_ext,
	SUB_EbIb_ext,
	XOR_EbIb_ext,
	CMP_EbIb_ext,
};

void (CPUCALL *insttable_G1EwIx[])(UINT16 *, UINT32) = {
	ADD_EwIx,
	OR_EwIx,
	ADC_EwIx,
	SBB_EwIx,
	AND_EwIx,
	SUB_EwIx,
	XOR_EwIx,
	CMP_EwIx,
};
void (CPUCALL *insttable_G1EwIx_ext[])(UINT32, UINT32) = {
	ADD_EwIx_ext,
	OR_EwIx_ext,
	ADC_EwIx_ext,
	SBB_EwIx_ext,
	AND_EwIx_ext,
	SUB_EwIx_ext,
	XOR_EwIx_ext,
	CMP_EwIx_ext,
};

void (CPUCALL *insttable_G1EdIx[])(UINT32 *, UINT32) = {
	ADD_EdIx,
	OR_EdIx,
	ADC_EdIx,
	SBB_EdIx,
	AND_EdIx,
	SUB_EdIx,
	XOR_EdIx,
	CMP_EdIx,
};
void (CPUCALL *insttable_G1EdIx_ext[])(UINT32, UINT32) = {
	ADD_EdIx_ext,
	OR_EdIx_ext,
	ADC_EdIx_ext,
	SBB_EdIx_ext,
	AND_EdIx_ext,
	SUB_EdIx_ext,
	XOR_EdIx_ext,
	CMP_EdIx_ext,
};


/* group 2 */
void (CPUCALL *insttable_G2Eb[])(UINT8 *) = {
	ROL_Eb,
	ROR_Eb,
	RCL_Eb,
	RCR_Eb,
	SHL_Eb,
	SHR_Eb,
	SHL_Eb,
	SAR_Eb,
};
void (CPUCALL *insttable_G2Eb_ext[])(UINT32) = {
	ROL_Eb_ext,
	ROR_Eb_ext,
	RCL_Eb_ext,
	RCR_Eb_ext,
	SHL_Eb_ext,
	SHR_Eb_ext,
	SHL_Eb_ext,
	SAR_Eb_ext,
};

void (CPUCALL *insttable_G2Ew[])(UINT16 *) = {
	ROL_Ew,
	ROR_Ew,
	RCL_Ew,
	RCR_Ew,
	SHL_Ew,
	SHR_Ew,
	SHL_Ew,
	SAR_Ew,
};
void (CPUCALL *insttable_G2Ew_ext[])(UINT32) = {
	ROL_Ew_ext,
	ROR_Ew_ext,
	RCL_Ew_ext,
	RCR_Ew_ext,
	SHL_Ew_ext,
	SHR_Ew_ext,
	SHL_Ew_ext,
	SAR_Ew_ext,
};

void (CPUCALL *insttable_G2Ed[])(UINT32 *) = {
	ROL_Ed,
	ROR_Ed,
	RCL_Ed,
	RCR_Ed,
	SHL_Ed,
	SHR_Ed,
	SHL_Ed,
	SAR_Ed,
};
void (CPUCALL *insttable_G2Ed_ext[])(UINT32) = {
	ROL_Ed_ext,
	ROR_Ed_ext,
	RCL_Ed_ext,
	RCR_Ed_ext,
	SHL_Ed_ext,
	SHR_Ed_ext,
	SHL_Ed_ext,
	SAR_Ed_ext,
};

void (CPUCALL *insttable_G2EbCL[])(UINT8 *, UINT) = {
	ROL_EbCL,
	ROR_EbCL,
	RCL_EbCL,
	RCR_EbCL,
	SHL_EbCL,
	SHR_EbCL,
	SHL_EbCL,
	SAR_EbCL,
};
void (CPUCALL *insttable_G2EbCL_ext[])(UINT32, UINT) = {
	ROL_EbCL_ext,
	ROR_EbCL_ext,
	RCL_EbCL_ext,
	RCR_EbCL_ext,
	SHL_EbCL_ext,
	SHR_EbCL_ext,
	SHL_EbCL_ext,
	SAR_EbCL_ext,
};

void (CPUCALL *insttable_G2EwCL[])(UINT16 *, UINT) = {
	ROL_EwCL,
	ROR_EwCL,
	RCL_EwCL,
	RCR_EwCL,
	SHL_EwCL,
	SHR_EwCL,
	SHL_EwCL,
	SAR_EwCL,
};
void (CPUCALL *insttable_G2EwCL_ext[])(UINT32, UINT) = {
	ROL_EwCL_ext,
	ROR_EwCL_ext,
	RCL_EwCL_ext,
	RCR_EwCL_ext,
	SHL_EwCL_ext,
	SHR_EwCL_ext,
	SHL_EwCL_ext,
	SAR_EwCL_ext,
};

void (CPUCALL *insttable_G2EdCL[])(UINT32 *, UINT) = {
	ROL_EdCL,
	ROR_EdCL,
	RCL_EdCL,
	RCR_EdCL,
	SHL_EdCL,
	SHR_EdCL,
	SHL_EdCL,
	SAR_EdCL,
};
void (CPUCALL *insttable_G2EdCL_ext[])(UINT32, UINT) = {
	ROL_EdCL_ext,
	ROR_EdCL_ext,
	RCL_EdCL_ext,
	RCR_EdCL_ext,
	SHL_EdCL_ext,
	SHR_EdCL_ext,
	SHL_EdCL_ext,
	SAR_EdCL_ext,
};

/* group 3 */
void (CPUCALL *insttable_G3Eb[])(UINT32) = {
	TEST_EbIb,
	TEST_EbIb,
	NOT_Eb,
	NEG_Eb,
	MUL_ALEb,
	IMUL_ALEb,
	DIV_ALEb,
	IDIV_ALEb,
};

void (CPUCALL *insttable_G3Ew[])(UINT32) = {
	TEST_EwIw,
	TEST_EwIw,
	NOT_Ew,
	NEG_Ew,
	MUL_AXEw,
	IMUL_AXEw,
	DIV_AXEw,
	IDIV_AXEw,
};

void (CPUCALL *insttable_G3Ed[])(UINT32) = {
	TEST_EdId,
	TEST_EdId,
	NOT_Ed,
	NEG_Ed,
	MUL_EAXEd,
	IMUL_EAXEd,
	DIV_EAXEd,
	IDIV_EAXEd,
};

/* group 4 */
void (CPUCALL *insttable_G4[])(UINT32) = {
	INC_Eb,
	DEC_Eb,
	undef_op2,
	undef_op2,
	undef_op2,
	undef_op2,
	undef_op2,
	undef_op2,
};

/* group 5 */
void (CPUCALL *insttable_G5Ew[])(UINT32) = {
	INC_Ew,
	DEC_Ew,
	CALL_Ew,
	CALL16_Ep,
	JMP_Ew,
	JMP16_Ep,
	PUSH_Ew,
	undef_op2,	/* POP_Ew_G5 */
};

void (CPUCALL *insttable_G5Ed[])(UINT32) = {
	INC_Ed,
	DEC_Ed,
	CALL_Ed,
	CALL32_Ep,
	JMP_Ed,
	JMP32_Ep,
	PUSH_Ed,
	undef_op2,	/* POP_Ed_G5 */
};

/* group 6 */
void (CPUCALL *insttable_G6[])(UINT32) = {
	SLDT_Ew,
	STR_Ew,
	LLDT_Ew,
	LTR_Ew,
	VERR_Ew,
	VERW_Ew,
	undef_op2,
	undef_op2,
};

/* group 7 */
void (CPUCALL *insttable_G7[])(UINT32) = {
	SGDT_Ms,
	SIDT_Ms,
	LGDT_Ms,
	LIDT_Ms,
	SMSW_Ew,
	undef_op2,
	LMSW_Ew,
	INVLPG,
};

/* group 8 */
void (CPUCALL *insttable_G8EwIb[])(UINT32) = {
	undef_op2,
	undef_op2,
	undef_op2,
	undef_op2,
	BT_EwIb,
	BTS_EwIb,
	BTR_EwIb,
	BTC_EwIb,
};

void (CPUCALL *insttable_G8EdIb[])(UINT32) = {
	undef_op2,
	undef_op2,
	undef_op2,
	undef_op2,
	BT_EdIb,
	BTS_EdIb,
	BTR_EdIb,
	BTC_EdIb,
};

/* group 9 */
void (CPUCALL *insttable_G9[])(UINT32) = {
	undef_op2,
	CMPXCHG8B,
	undef_op2,
	undef_op2,
	undef_op2,
	undef_op2,
	undef_op2,
	undef_op2,
};
