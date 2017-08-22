/*
 * Copyright (c) 2003 NONAKA Kimihiro
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
#include "ia32.mcr"

const char *exception_str[EXCEPTION_NUM] = {
	"DE_EXCEPTION",
	"DB_EXCEPTION",
	"NMI_EXCEPTION",
	"BP_EXCEPTION",
	"OF_EXCEPTION",
	"BR_EXCEPTION",
	"UD_EXCEPTION",
	"NM_EXCEPTION",
	"DF_EXCEPTION",
	"CoProcesser Segment Overrun",
	"TS_EXCEPTION",
	"NP_EXCEPTION",
	"SS_EXCEPTION",
	"GP_EXCEPTION",
	"PF_EXCEPTION",
	"Reserved",
	"MF_EXCEPTION",
	"AC_EXCEPTION",
	"MC_EXCEPTION",
	"XF_EXCEPTION",
};

static const int exctype[EXCEPTION_NUM] = {
	1, 0, 0, 0, 0, 0, 0, 0, 3, 0, 1, 1, 1, 1, 2, 0, 0, 0, 0, 0,
};

static const int dftable[4][4] = {
	{ 0, 0, 0, 1, },
	{ 0, 1, 0, 1, },
	{ 0, 1, 1, 1, },
	{ 1, 1, 1, 1, },
};

void CPUCALL
exception(int num, int error_code)
{
#if defined(DEBUG)
	extern int cpu_debug_rep_cont;
	extern CPU_REGS cpu_debug_rep_regs;
#endif
	int errorp = 0;

	__ASSERT((unsigned int)num < EXCEPTION_NUM);

#if 0
	iptrace_out();
	debugwriteseg("execption.bin", &CPU_CS_DESC, CPU_PREV_EIP & 0xffff0000, 0x10000);
#endif

	VERBOSE(("exception: -------------------------------------------------------------- start"));
	VERBOSE(("exception: %s, error_code = %x at %04x:%08x", exception_str[num], error_code, CPU_CS, CPU_PREV_EIP));
	VERBOSE(("%s", cpu_reg2str()));
	VERBOSE(("code: %dbit(%dbit), address: %dbit(%dbit)", CPU_INST_OP32 ? 32 : 16, CPU_STATSAVE.cpu_inst_default.op_32 ? 32 : 16, CPU_INST_AS32 ? 32 : 16, CPU_STATSAVE.cpu_inst_default.as_32 ? 32 : 16));
#if defined(DEBUG)
	if (cpu_debug_rep_cont) {
		VERBOSE(("rep: original regs: ecx=%08x, esi=%08x, edi=%08x", cpu_debug_rep_regs.reg[CPU_ECX_INDEX].d, cpu_debug_rep_regs.reg[CPU_ESI_INDEX].d, cpu_debug_rep_regs.reg[CPU_EDI_INDEX].d));
	}
	VERBOSE(("%s", cpu_disasm2str(CPU_PREV_EIP)));
#endif

	CPU_STAT_EXCEPTION_COUNTER_INC();
	if ((CPU_STAT_EXCEPTION_COUNTER >= 3) 
	 || (CPU_STAT_EXCEPTION_COUNTER == 2 && CPU_STAT_PREV_EXCEPTION == DF_EXCEPTION)) {
		/* Triple fault */
		ia32_panic("exception: catch triple fault!");
	}

	switch (num) {
	case DE_EXCEPTION:	/* (F) 除算エラー */
	case DB_EXCEPTION:	/* (F/T) デバッグ */
	case BR_EXCEPTION:	/* (F) BOUND の範囲外 */
	case UD_EXCEPTION:	/* (F) 無効オペコード */
	case NM_EXCEPTION:	/* (F) デバイス使用不可 (FPU が無い) */
	case MF_EXCEPTION:	/* (F) 浮動小数点エラー */
		CPU_EIP = CPU_PREV_EIP;
		if (CPU_STATSAVE.cpu_stat.backout_sp)
			CPU_ESP = CPU_PREV_ESP;
		/*FALLTHROUGH*/
	case NMI_EXCEPTION:	/* (I) NMI 割り込み */
	case BP_EXCEPTION:	/* (T) ブレークポイント */
	case OF_EXCEPTION:	/* (T) オーバーフロー */
		errorp = 0;
		break;

	case DF_EXCEPTION:	/* (A) ダブルフォルト (errcode: 0) */
		errorp = 1;
		error_code = 0;
		break;

	case AC_EXCEPTION:	/* (F) アラインメントチェック (errcode: 0) */
		error_code = 0;
		/*FALLTHROUGH*/
	case TS_EXCEPTION:	/* (F) 無効 TSS (errcode) */
	case NP_EXCEPTION:	/* (F) セグメント不在 (errcode) */
	case SS_EXCEPTION:	/* (F) スタックセグメントフォルト (errcode) */
	case GP_EXCEPTION:	/* (F) 一般保護例外 (errcode) */
	case PF_EXCEPTION:	/* (F) ページフォルト (errcode) */
		CPU_EIP = CPU_PREV_EIP;
		if (CPU_STATSAVE.cpu_stat.backout_sp)
			CPU_ESP = CPU_PREV_ESP;
		errorp = 1;
		break;

	default:
		ia32_panic("exception: unknown exception (%d)", num);
		break;
	}

	if (CPU_STAT_EXCEPTION_COUNTER >= 2) {
		if (dftable[exctype[CPU_STAT_PREV_EXCEPTION]][exctype[num]]) {
			num = DF_EXCEPTION;
			errorp = 1;
			error_code = 0;
		}
	}
	CPU_STAT_PREV_EXCEPTION = num;

	VERBOSE(("exception: ---------------------------------------------------------------- end"));

	interrupt(num, INTR_TYPE_EXCEPTION, errorp, error_code);
	CPU_STAT_EXCEPTION_COUNTER_CLEAR();
	siglongjmp(exec_1step_jmpbuf, 1);
}

/*
 * コール・ゲート・ディスクリプタ
 *
 *  31                                16 15 14 13 12       8 7   5 4       0
 * +------------------------------------+--+-----+----------+-----+---------+
 * |         オフセット 31..16          | P| DPL | 0 D 1 0 0|0 0 0|カウント | 4
 * +------------------------------------+--+-----+----------+-----+---------+
 *  31                                16 15                                0
 * +------------------------------------+-----------------------------------+
 * |        セグメント・セレクタ        |          オフセット 15..0         | 0
 * +------------------------------------+-----------------------------------+
 */

/*
 * 割り込みディスクリプタ
 *--
 * タスク・ゲート
 *
 *  31                                16 15 14 13 12       8 7             0
 * +------------------------------------+--+-----+----------+---------------+
 * |              Reserved              | P| DPL | 0 0 1 0 1|   Reserved    | 4
 * +------------------------------------+--+-----+----------+---------------+
 *  31                                16 15                                0
 * +------------------------------------+-----------------------------------+
 * |      TSS セグメント・セレクタ      |              Reserved             | 0
 * +------------------------------------+-----------------------------------+
 *--
 * 割り込み・ゲート
 *
 *  31                                16 15 14 13 12       8 7   5 4       0
 * +------------------------------------+--+-----+----------+-----+---------+
 * |         オフセット 31..16          | P| DPL | 0 D 1 1 0|0 0 0|Reserved | 4
 * +------------------------------------+--+-----+----------+-----+---------+
 *  31                                16 15                                0
 * +------------------------------------+-----------------------------------+
 * |        セグメント・セレクタ        |          オフセット 15..0         | 0
 * +------------------------------------+-----------------------------------+
 *--
 * トラップ・ゲート
 *
 *  31                                16 15 14 13 12       8 7   5 4       0
 * +------------------------------------+--+-----+----------+-----+---------+
 * |         オフセット 31..16          | P| DPL | 0 D 1 1 1|0 0 0|Reserved | 4
 * +------------------------------------+--+-----+----------+-----+---------+
 *  31                                16 15                                0
 * +------------------------------------+-----------------------------------+
 * |        セグメント・セレクタ        |          オフセット 15..0         | 0
 * +------------------------------------+-----------------------------------+
 *--
 * DPL        : ディスクリプタ特権レベル
 * オフセット : プロシージャ・エントリ・ポイントまでのオフセット
 * P          : セグメント存在フラグ
 * セレクタ   : ディスティネーション・コード・セグメントのセグメント・セレクタ
 * D          : ゲートのサイズ．0 = 16 bit, 1 = 32 bit
 */

static void CPUCALL interrupt_task_gate(const descriptor_t *gsdp, int intrtype, int errorp, int error_code);
static void CPUCALL interrupt_intr_or_trap(const descriptor_t *gsdp, int intrtype, int errorp, int error_code);

void CPUCALL
interrupt(int num, int intrtype, int errorp, int error_code)
{
	descriptor_t gsd;
	UINT idt_idx;
	UINT32 new_ip;
	UINT16 new_cs;
	int exc_errcode;

	VERBOSE(("interrupt: num = 0x%02x, intrtype = %s, errorp = %s, error_code = %08x", num, (intrtype == INTR_TYPE_EXTINTR) ? "external" : (intrtype == INTR_TYPE_EXCEPTION ? "exception" : "softint"), errorp ? "on" : "off", error_code));

	CPU_SET_PREV_ESP();

	if (!CPU_STAT_PM) {
		/* real mode */
		CPU_WORKCLOCK(20);

		idt_idx = num * 4;
		if (idt_idx + 3 > CPU_IDTR_LIMIT) {
			VERBOSE(("interrupt: real-mode IDTR limit check failure (idx = 0x%04x, limit = 0x%08x", idt_idx, CPU_IDTR_LIMIT));
			EXCEPTION(GP_EXCEPTION, idt_idx + 2);
		}

		if ((intrtype == INTR_TYPE_EXTINTR) && CPU_STAT_HLT) {
			VERBOSE(("interrupt: reset HTL in real mode"));
			CPU_EIP++;
			CPU_STAT_HLT = 0;
		}

		REGPUSH0(REAL_FLAGREG);
		REGPUSH0(CPU_CS);
		REGPUSH0(CPU_IP);

		CPU_EFLAG &= ~(T_FLAG | I_FLAG | AC_FLAG | RF_FLAG);
		CPU_TRAP = 0;

		new_ip = cpu_memoryread_w(CPU_IDTR_BASE + idt_idx);
		new_cs = cpu_memoryread_w(CPU_IDTR_BASE + idt_idx + 2);
		LOAD_SEGREG(CPU_CS_INDEX, new_cs);
		CPU_EIP = new_ip;
	} else {
		/* protected mode */
		CPU_WORKCLOCK(200);

		VERBOSE(("interrupt: -------------------------------------------------------------- start"));
		VERBOSE(("interrupt: old EIP = %04x:%08x, ESP = %04x:%08x", CPU_CS, CPU_EIP, CPU_SS, CPU_ESP));

#if defined(DEBUG)
		if (num == 0x80) {
			/* Linux, FreeBSD, NetBSD, OpenBSD system call */
			VERBOSE(("interrupt: syscall# = %d\n%s", CPU_EAX, cpu_reg2str()));
		}
#endif

		idt_idx = num * 8;
		exc_errcode = idt_idx + 2;
		if (intrtype == INTR_TYPE_EXTINTR)
			exc_errcode++;

		if (idt_idx + 7 > CPU_IDTR_LIMIT) {
			VERBOSE(("interrupt: IDTR limit check failure (idx = 0x%04x, limit = 0x%08x", idt_idx, CPU_IDTR_LIMIT));
			EXCEPTION(GP_EXCEPTION, exc_errcode);
		}

		/* load a gate descriptor from interrupt descriptor table */
		memset(&gsd, 0, sizeof(gsd));
		load_descriptor(&gsd, CPU_IDTR_BASE + idt_idx);
		if (!SEG_IS_VALID(&gsd)) {
			VERBOSE(("interrupt: gate descripter is invalid."));
			EXCEPTION(GP_EXCEPTION, exc_errcode);
		}
		if (!SEG_IS_SYSTEM(&gsd)) {
			VERBOSE(("interrupt: gate descriptor is not system segment."));
			EXCEPTION(GP_EXCEPTION, exc_errcode);
		}

		switch (gsd.type) {
		case CPU_SYSDESC_TYPE_TASK:
		case CPU_SYSDESC_TYPE_INTR_16:
		case CPU_SYSDESC_TYPE_INTR_32:
		case CPU_SYSDESC_TYPE_TRAP_16:
		case CPU_SYSDESC_TYPE_TRAP_32:
			break;

		default:
			VERBOSE(("interrupt: invalid gate type (%d)", gsd.type));
			EXCEPTION(GP_EXCEPTION, exc_errcode);
			break;
		}

		/* 5.10.1.1. 例外／割り込みハンドラ・プロシージャの保護 */
		if ((intrtype == INTR_TYPE_SOFTINTR) && (gsd.dpl < CPU_STAT_CPL)) {
			VERBOSE(("interrupt: intrtype(softint) && DPL(%d) < CPL(%d)", gsd.dpl, CPU_STAT_CPL));
			EXCEPTION(GP_EXCEPTION, exc_errcode);
		}

		if (!SEG_IS_PRESENT(&gsd)) {
			VERBOSE(("interrupt: gate descriptor is not present."));
			EXCEPTION(NP_EXCEPTION, exc_errcode);
		}

		if ((intrtype == INTR_TYPE_EXTINTR) && CPU_STAT_HLT) {
			VERBOSE(("interrupt: reset HTL in protected mode"));
			CPU_EIP++;
			CPU_STAT_HLT = 0;
		}

		switch (gsd.type) {
		case CPU_SYSDESC_TYPE_TASK:
			interrupt_task_gate(&gsd, intrtype, errorp, error_code);
			break;

		case CPU_SYSDESC_TYPE_INTR_16:
		case CPU_SYSDESC_TYPE_INTR_32:
		case CPU_SYSDESC_TYPE_TRAP_16:
		case CPU_SYSDESC_TYPE_TRAP_32:
			interrupt_intr_or_trap(&gsd, intrtype, errorp, error_code);
			break;

		default:
			EXCEPTION(GP_EXCEPTION, exc_errcode);
			break;
		}

		VERBOSE(("interrupt: ---------------------------------------------------------------- end"));
	}

	CPU_CLEAR_PREV_ESP();
}

static void CPUCALL
interrupt_task_gate(const descriptor_t *gsdp, int intrtype, int errorp, int error_code)
{
	selector_t task_sel;
	int rv;

	VERBOSE(("interrupt: TASK-GATE"));

	rv = parse_selector(&task_sel, gsdp->u.gate.selector);
	if (rv < 0 || task_sel.ldt || !SEG_IS_SYSTEM(&task_sel.desc)) {
		VERBOSE(("interrupt: parse_selector (selector = %04x, rv = %d, %cDT, type = %s)", gsdp->u.gate.selector, rv, task_sel.ldt ? 'L' : 'G', task_sel.desc.s ? "code/data" : "system"));
		EXCEPTION(TS_EXCEPTION, task_sel.idx);
	}

	/* check gate type */
	switch (task_sel.desc.type) {
	case CPU_SYSDESC_TYPE_TSS_16:
	case CPU_SYSDESC_TYPE_TSS_32:
		break;

	case CPU_SYSDESC_TYPE_TSS_BUSY_16:
	case CPU_SYSDESC_TYPE_TSS_BUSY_32:
		VERBOSE(("interrupt: task is busy."));
		/*FALLTHROUGH*/
	default:
		VERBOSE(("interrupt: invalid gate type (%d)", task_sel.desc.type));
		EXCEPTION(TS_EXCEPTION, task_sel.idx);
		break;
	}

	/* not present */
	if (selector_is_not_present(&task_sel)) {
		VERBOSE(("interrupt: selector is not present"));
		EXCEPTION(NP_EXCEPTION, task_sel.idx);
	}

	task_switch(&task_sel, TASK_SWITCH_INTR);

	CPU_SET_PREV_ESP();

	if (errorp) {
		VERBOSE(("interrupt: push error code (%08x)", error_code));
		if (task_sel.desc.type == CPU_SYSDESC_TYPE_TSS_32) {
			PUSH0_32(error_code);
		} else {
			PUSH0_16(error_code);
		}
	}

	/* out of range */
	if (CPU_EIP > CPU_STAT_CS_LIMIT) {
		VERBOSE(("interrupt: new_ip is out of range. new_ip = %08x, limit = %08x", CPU_EIP, CPU_STAT_CS_LIMIT));
		EXCEPTION(GP_EXCEPTION, 0);
	}
}

static void CPUCALL
interrupt_intr_or_trap(const descriptor_t *gsdp, int intrtype, int errorp, int error_code)
{
	selector_t cs_sel, ss_sel;
	UINT stacksize;
	UINT32 old_flags;
	UINT32 new_flags;
	UINT32 mask;
	UINT32 sp;
	UINT32 new_ip, new_sp;
	UINT32 old_ip, old_sp;
	UINT16 old_cs, old_ss, new_ss;
	BOOL is32bit;
	int exc_errcode;
	int rv; 

	new_ip = gsdp->u.gate.offset;
	old_ss = CPU_SS;
	old_cs = CPU_CS;
	old_ip = CPU_EIP;
	old_sp = CPU_ESP;
	old_flags = REAL_EFLAGREG;
	new_flags = REAL_EFLAGREG & ~(T_FLAG|RF_FLAG|NT_FLAG|VM_FLAG);
	mask = T_FLAG|RF_FLAG|NT_FLAG|VM_FLAG;

	switch (gsdp->type) {
	case CPU_SYSDESC_TYPE_INTR_16:
	case CPU_SYSDESC_TYPE_INTR_32:
		VERBOSE(("interrupt: INTERRUPT-GATE"));
		new_flags &= ~I_FLAG;
		mask |= I_FLAG;
		break;

	case CPU_SYSDESC_TYPE_TRAP_16:
	case CPU_SYSDESC_TYPE_TRAP_32:
		VERBOSE(("interrupt: TRAP-GATE"));
		break;

	default:
		ia32_panic("interrupt: gate descriptor type is invalid (type = %d)", gsdp->type);
		break;
	}

	exc_errcode = gsdp->u.gate.selector & ~3;
	if (intrtype == INTR_TYPE_EXTINTR)
		exc_errcode++;

	rv = parse_selector(&cs_sel, gsdp->u.gate.selector);
	if (rv < 0) {
		VERBOSE(("interrupt: parse_selector (selector = %04x, rv = %d)", gsdp->u.gate.selector, rv));
		EXCEPTION(GP_EXCEPTION, exc_errcode);
	}

	/* check segment type */
	if (SEG_IS_SYSTEM(&cs_sel.desc)) {
		VERBOSE(("interrupt: code segment is system segment"));
		EXCEPTION(GP_EXCEPTION, exc_errcode);
	}
	if (SEG_IS_DATA(&cs_sel.desc)) {
		VERBOSE(("interrupt: code segment is data segment"));
		EXCEPTION(GP_EXCEPTION, exc_errcode);
	}

	/* check privilege level */
	if (cs_sel.desc.dpl > CPU_STAT_CPL) {
		VERBOSE(("interrupt: DPL(%d) > CPL(%d)", cs_sel.desc.dpl, CPU_STAT_CPL));
		EXCEPTION(GP_EXCEPTION, exc_errcode);
	}

	/* not present */
	if (selector_is_not_present(&cs_sel)) {
		VERBOSE(("interrupt: selector is not present"));
		EXCEPTION(NP_EXCEPTION, exc_errcode);
	}

	is32bit = gsdp->type & CPU_SYSDESC_TYPE_32BIT;
	if (!SEG_IS_CONFORMING_CODE(&cs_sel.desc) && (cs_sel.desc.dpl < CPU_STAT_CPL)) {
		stacksize = errorp ? 12 : 10;
		if (!CPU_STAT_VM86) {
			VERBOSE(("interrupt: INTER-PRIVILEGE-LEVEL-INTERRUPT"));
		} else {
			/* VM86 */
			VERBOSE(("interrupt: INTERRUPT-FROM-VIRTUAL-8086-MODE"));
			if (cs_sel.desc.dpl != 0) {
				/* 16.3.1.1 */
				VERBOSE(("interrupt: DPL[CS](%d) != 0", cs_sel.desc.dpl));
				EXCEPTION(GP_EXCEPTION, exc_errcode);
			}
			stacksize += 8;
		}
		if (is32bit) {
			stacksize *= 2;
		}

		/* get stack pointer from TSS */
		get_stack_pointer_from_tss(cs_sel.desc.dpl, &new_ss, &new_sp);

		/* parse stack segment descriptor */
		rv = parse_selector(&ss_sel, new_ss);

		/* update exception error code */
		exc_errcode = ss_sel.idx;
		if (intrtype == INTR_TYPE_EXTINTR)
			exc_errcode++;

		if (rv < 0) {
			VERBOSE(("interrupt: parse_selector (selector = %04x, rv = %d)", new_ss, rv));
			EXCEPTION(TS_EXCEPTION, exc_errcode);
		}

		/* check privilege level */
		if (ss_sel.rpl != cs_sel.desc.dpl) {
			VERBOSE(("interrupt: selector RPL[SS](%d) != DPL[CS](%d)", ss_sel.rpl, cs_sel.desc.dpl));
			EXCEPTION(TS_EXCEPTION, exc_errcode);
		}
		if (ss_sel.desc.dpl != cs_sel.desc.dpl) {
			VERBOSE(("interrupt: descriptor DPL[SS](%d) != DPL[CS](%d)", ss_sel.desc.dpl, cs_sel.desc.dpl));
			EXCEPTION(TS_EXCEPTION, exc_errcode);
		}

		/* stack segment must be writable data segment. */
		if (SEG_IS_SYSTEM(&ss_sel.desc)) {
			VERBOSE(("interrupt: stack segment is system segment"));
			EXCEPTION(TS_EXCEPTION, exc_errcode);
		}
		if (SEG_IS_CODE(&ss_sel.desc)) {
			VERBOSE(("interrupt: stack segment is code segment"));
			EXCEPTION(TS_EXCEPTION, exc_errcode);
		}
		if (!SEG_IS_WRITABLE_DATA(&ss_sel.desc)) {
			VERBOSE(("interrupt: stack segment is read-only data segment"));
			EXCEPTION(TS_EXCEPTION, exc_errcode);
		}

		/* not present */
		if (selector_is_not_present(&ss_sel)) {
			VERBOSE(("interrupt: selector is not present"));
			EXCEPTION(SS_EXCEPTION, exc_errcode);
		}

		/* check stack room size */
		cpu_stack_push_check(ss_sel.idx, &ss_sel.desc, new_sp, stacksize, ss_sel.desc.d);

		/* out of range */
		if (new_ip > cs_sel.desc.u.seg.limit) {
			VERBOSE(("interrupt: new_ip is out of range. new_ip = %08x, limit = %08x", new_ip, cs_sel.desc.u.seg.limit));
			EXCEPTION(GP_EXCEPTION, 0);
		}

		load_ss(ss_sel.selector, &ss_sel.desc, cs_sel.desc.dpl);
		CPU_ESP = new_sp;

		load_cs(cs_sel.selector, &cs_sel.desc, cs_sel.desc.dpl);
		CPU_EIP = new_ip;

		if (is32bit) {
			if (CPU_STAT_VM86) {
				PUSH0_32(CPU_GS);
				PUSH0_32(CPU_FS);
				PUSH0_32(CPU_DS);
				PUSH0_32(CPU_ES);

				LOAD_SEGREG(CPU_GS_INDEX, 0);
				CPU_STAT_SREG(CPU_GS_INDEX).valid = 0;
				LOAD_SEGREG(CPU_FS_INDEX, 0);
				CPU_STAT_SREG(CPU_FS_INDEX).valid = 0;
				LOAD_SEGREG(CPU_DS_INDEX, 0);
				CPU_STAT_SREG(CPU_DS_INDEX).valid = 0;
				LOAD_SEGREG(CPU_ES_INDEX, 0);
				CPU_STAT_SREG(CPU_ES_INDEX).valid = 0;
			}
			PUSH0_32(old_ss);
			PUSH0_32(old_sp);
			PUSH0_32(old_flags);
			PUSH0_32(old_cs);
			PUSH0_32(old_ip);
			if (errorp) {
				PUSH0_32(error_code);
			}
		} else {
			if (CPU_STAT_VM86) {
				ia32_panic("interrupt: 16bit gate && VM86");
			}
			PUSH0_16(old_ss);
			PUSH0_16(old_sp);
			PUSH0_16(old_flags);
			PUSH0_16(old_cs);
			PUSH0_16(old_ip);
			if (errorp) {
				PUSH0_16(error_code);
			}
		}
	} else {
		if (CPU_STAT_VM86) {
			VERBOSE(("interrupt: VM86"));
			EXCEPTION(GP_EXCEPTION, exc_errcode);
		}
		if (!SEG_IS_CONFORMING_CODE(&cs_sel.desc) && (cs_sel.desc.dpl != CPU_STAT_CPL)) {
			VERBOSE(("interrupt: %sCONFORMING-CODE-SEGMENT(%d) && DPL[CS](%d) != CPL", SEG_IS_CONFORMING_CODE(&cs_sel.desc) ? "" : "NON-", cs_sel.desc.dpl, CPU_STAT_CPL));
			EXCEPTION(GP_EXCEPTION, exc_errcode);
		}

		VERBOSE(("interrupt: INTRA-PRIVILEGE-LEVEL-INTERRUPT"));

		stacksize = errorp ? 8 : 6;
		if (is32bit) {
			stacksize *= 2;
		}

		/* check stack room size */
		if (CPU_STAT_SS32) {
			sp = CPU_ESP;
		} else {
			sp = CPU_SP;
		}
		/*
		 * 17.1
		 * コールゲート、割り込みゲート、またはトラップゲートを通じて
		 * プログラムの制御を他のコード・セグメントに移行するときは、
		 * 移行中に使用されるオペランド・サイズは使用されるゲートの
		 * タイプ（16 ビットまたは32 ビット）によって決まる（移行命
		 * 令のD フラグ、プリフィックスのいずれにもよらない）。
		 */
		SS_PUSH_CHECK1(sp, stacksize, is32bit);

		/* out of range */
		if (new_ip > cs_sel.desc.u.seg.limit) {
			VERBOSE(("interrupt: new_ip is out of range. new_ip = %08x, limit = %08x", new_ip, cs_sel.desc.u.seg.limit));
			EXCEPTION(GP_EXCEPTION, 0);
		}

		load_cs(cs_sel.selector, &cs_sel.desc, CPU_STAT_CPL);
		CPU_EIP = new_ip;

		if (is32bit) {
			PUSH0_32(old_flags);
			PUSH0_32(old_cs);
			PUSH0_32(old_ip);
			if (errorp) {
				PUSH0_32(error_code);
			}
		} else {
			PUSH0_16(old_flags);
			PUSH0_16(old_cs);
			PUSH0_16(old_ip);
			if (errorp) {
				PUSH0_16(error_code);
			}
		}
	}
	set_eflags(new_flags, mask);

	VERBOSE(("interrupt: new EIP = %04x:%08x, ESP = %04x:%08x", CPU_CS, CPU_EIP, CPU_SS, CPU_ESP));
}
