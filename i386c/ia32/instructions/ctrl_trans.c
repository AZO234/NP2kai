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

#include <compiler.h>
#include <ia32/cpu.h>
#include "ia32/ia32.mcr"
#include "ia32/ctrlxfer.h"

#include "ctrl_trans.h"

#if defined(ENABLE_TRAP)
#include "trap/inttrap.h"
#endif

#ifdef SUPPORT_IA32_HAXM
#include <bios/bios.h>
#include <sound/soundrom.h>
#endif

static void
current_stack_push16(UINT32 *sp, UINT16 value)
{
	UINT32 mask;
	UINT32 new_sp;

	mask = CPU_STAT_SS32 ? 0xffffffff : 0x0000ffff;
	new_sp = (*sp - 2) & mask;

	cpu_vmemorywrite_w(CPU_SS_INDEX, new_sp, value);

	*sp = new_sp;
}

static void
current_stack_push32(UINT32 *sp, UINT32 value)
{
	UINT32 mask;
	UINT32 new_sp;

	mask = CPU_STAT_SS32 ? 0xffffffff : 0x0000ffff;
	new_sp = (*sp - 4) & mask;

	cpu_vmemorywrite_d(CPU_SS_INDEX, new_sp, value);

	*sp = new_sp;
}

static void
commit_current_stack_pointer(UINT32 sp)
{
	if (CPU_STAT_SS32) {
		CPU_ESP = sp;
	} else {
		CPU_SP = (UINT16)sp;
	}
}

#if defined(USE_VME)
static BOOL CPUCALL
vme_int_redirect_allowed(UINT vect)
{
	UINT32 iobase;
	UINT32 redir_base;
	UINT32 off;
	UINT8 map;

	if (!(CPU_CR4 & CPU_CR4_VME)) {
		return FALSE;
	}
	if (CPU_TR_DESC.type != CPU_SYSDESC_TYPE_TSS_BUSY_32) {
		return FALSE;
	}
	if (CPU_STAT_IOLIMIT == 0) {
		return FALSE;
	}
	if (CPU_STAT_IOADDR < CPU_TR_BASE) {
		return FALSE;
	}

	/*
	 * In a 32-bit TSS used with VME, the 256-bit interrupt-redirection
	 * bitmap is the 32 bytes immediately before the I/O permission bitmap.
	 * A clear bit means emulate the software interrupt in VM86; a set bit
	 * means trap to the VM86 monitor with #GP(0).
	 */
	iobase = CPU_STAT_IOADDR - CPU_TR_BASE;
	if (iobase < 32) {
		return FALSE;
	}
	redir_base = CPU_STAT_IOADDR - 32;
	off = vect >> 3;
	map = cpu_lmemoryread(redir_base + off, CPU_PAGE_READ_DATA | CPU_STAT_USER_MODE);

	return (map & (1 << (vect & 7))) == 0;
}

static void CPUCALL
vme_redirect_int(UINT vect)
{
	UINT32 idt_idx;
	UINT32 new_ip;
	UINT16 new_cs;
	UINT16 flags;

	idt_idx = vect * 4;
	if (idt_idx + 3 > 0xffff) {
		EXCEPTION(GP_EXCEPTION, 0);
	}

	/* The redirected interrupt uses the VM86 real-mode IVT at linear 0. */
	new_ip = cpu_lmemoryread_w(idt_idx, CPU_PAGE_READ_DATA | CPU_STAT_USER_MODE);
	new_cs = cpu_lmemoryread_w(idt_idx + 2, CPU_PAGE_READ_DATA | CPU_STAT_USER_MODE);

	flags = REAL_FLAGREG;
	flags = (flags & ~I_FLAG) | ((CPU_EFLAG & VIF_FLAG) ? I_FLAG : 0);
	flags |= IOPL_FLAG;

	CPU_SET_PREV_ESP();
	PUSH0_16(flags);
	PUSH0_16(CPU_CS);
	PUSH0_16(CPU_IP);

	CPU_EFLAG &= ~(T_FLAG | VIF_FLAG | AC_FLAG | RF_FLAG);
	CPU_TRAP = 0;

	LOAD_SEGREG(CPU_CS_INDEX, new_cs);
	CPU_EIP = new_ip;
	CPU_CLEAR_PREV_ESP();
}
#endif


/*
 * JMP
 */
void
JMP_Jb(void)
{

	JMPSHORT(7);
}

void
JMP_Jw(void)
{

	JMPNEAR(7);
}

void
JMP_Jd(void)
{

	JMPNEAR32(7);
}

void CPUCALL
JMP_Ew(UINT32 op)
{
	UINT32 madr;
	UINT16 new_ip;

	if (op >= 0xc0) {
		CPU_WORKCLOCK(7);
		new_ip = *(CPU_REG16_B20(op));
	} else {
		CPU_WORKCLOCK(11);
		madr = calc_ea_dst(op);
		new_ip = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, madr);
	}
	if (new_ip > CPU_STAT_CS_LIMIT) {
		EXCEPTION(GP_EXCEPTION, 0);
	}
	CPU_EIP = new_ip;
}

void CPUCALL
JMP_Ed(UINT32 op)
{
	UINT32 madr;
	UINT32 new_ip;

	if (op >= 0xc0) {
		CPU_WORKCLOCK(7);
		new_ip = *(CPU_REG32_B20(op));
	} else {
		CPU_WORKCLOCK(11);
		madr = calc_ea_dst(op);
		new_ip = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, madr);
	}
	if (new_ip > CPU_STAT_CS_LIMIT) {
		EXCEPTION(GP_EXCEPTION, 0);
	}
	CPU_EIP = new_ip;
}

void
JMP16_Ap(void)
{
	descriptor_t sd;
	UINT16 new_ip;
	UINT16 new_cs;
	UINT16 sreg;

	CPU_WORKCLOCK(11);
	GET_PCWORD(new_ip);
	GET_PCWORD(new_cs);
	if (!CPU_STAT_PM || CPU_STAT_VM86) {
		/* Real mode or VM86 mode */
		/* check new instrunction pointer with new code segment */
		load_segreg(CPU_CS_INDEX, new_cs, &sreg, &sd, GP_EXCEPTION);
		if (new_ip > sd.u.seg.limit) {
			EXCEPTION(GP_EXCEPTION, 0);
		}
		LOAD_SEGREG(CPU_CS_INDEX, new_cs);
		CPU_EIP = new_ip;
	} else {
		/* Protected mode */
		JMPfar_pm(new_cs, new_ip);
	}
}

void
JMP32_Ap(void)
{
	descriptor_t sd;
	UINT32 new_ip;
	UINT16 new_cs;
	UINT16 sreg;

	CPU_WORKCLOCK(11);
	GET_PCDWORD(new_ip);
	GET_PCWORD(new_cs);
	if (!CPU_STAT_PM || CPU_STAT_VM86) {
		/* Real mode or VM86 mode */
		/* check new instrunction pointer with new code segment */
		load_segreg(CPU_CS_INDEX, new_cs, &sreg, &sd, GP_EXCEPTION);
		if (new_ip > sd.u.seg.limit) {
			EXCEPTION(GP_EXCEPTION, 0);
		}
		LOAD_SEGREG(CPU_CS_INDEX, new_cs);
		CPU_EIP = new_ip;
	} else {
		/* Protected mode */
		JMPfar_pm(new_cs, new_ip);
	}
}

void CPUCALL
JMP16_Ep(UINT32 op)
{
	descriptor_t sd;
	UINT32 madr;
	UINT16 new_ip;
	UINT16 new_cs;
	UINT16 sreg;

	CPU_WORKCLOCK(11);
	if (op < 0xc0) {
		madr = calc_ea_dst(op);
		new_ip = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, madr);
		new_cs = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, madr + 2);
		if (!CPU_STAT_PM || CPU_STAT_VM86) {
			/* Real mode or VM86 mode */
			/* check new instrunction pointer with new code segment */
			load_segreg(CPU_CS_INDEX, new_cs, &sreg, &sd, GP_EXCEPTION);
			if (new_ip > sd.u.seg.limit) {
				EXCEPTION(GP_EXCEPTION, 0);
			}
			LOAD_SEGREG(CPU_CS_INDEX, new_cs);
			CPU_EIP = new_ip;
		} else {
			/* Protected mode */
			JMPfar_pm(new_cs, new_ip);
		}
		return;
	}
	EXCEPTION(UD_EXCEPTION, 0);
}

void CPUCALL
JMP32_Ep(UINT32 op)
{
	descriptor_t sd;
	UINT32 madr;
	UINT32 new_ip;
	UINT16 new_cs;
	UINT16 sreg;

	CPU_WORKCLOCK(11);
	if (op < 0xc0) {
		madr = calc_ea_dst(op);
		new_ip = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, madr);
		new_cs = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, madr + 4);
		if (!CPU_STAT_PM || CPU_STAT_VM86) {
			/* Real mode or VM86 mode */
			/* check new instrunction pointer with new code segment */
			load_segreg(CPU_CS_INDEX, new_cs, &sreg, &sd, GP_EXCEPTION);
			if (new_ip > sd.u.seg.limit) {
				EXCEPTION(GP_EXCEPTION, 0);
			}
			LOAD_SEGREG(CPU_CS_INDEX, new_cs);
			CPU_EIP = new_ip;
		} else {
			/* Protected mode */
			JMPfar_pm(new_cs, new_ip);
		}
		return;
	}
	EXCEPTION(UD_EXCEPTION, 0);
}

/* jo */
void
JO_Jb(void)
{

	if (CC_NO) {
		JMPNOP(2, 1);
	} else {
		JMPSHORT(7);
	}
}

void
JO_Jw(void)
{

	if (CC_NO) {
		JMPNOP(2, 2);
	} else {
		JMPNEAR(7);
	}
}

void
JO_Jd(void)
{

	if (CC_NO) {
		JMPNOP(2, 4);
	} else {
		JMPNEAR32(7);
	}
}

/* jno */
void
JNO_Jb(void)
{

	if (CC_O) {
		JMPNOP(2, 1);
	} else {
		JMPSHORT(7);
	}
}

void
JNO_Jw(void)
{

	if (CC_O) {
		JMPNOP(2, 2);
	} else {
		JMPNEAR(7);
	}
}

void
JNO_Jd(void)
{

	if (CC_O) {
		JMPNOP(2, 4);
	} else {
		JMPNEAR32(7);
	}
}

/* jc */
void
JC_Jb(void)
{

	if (CC_NC) {
		JMPNOP(2, 1);
	} else {
		JMPSHORT(7);
	}
}

void
JC_Jw(void)
{

	if (CC_NC) {
		JMPNOP(2, 2);
	} else {
		JMPNEAR(7);
	}
}

void
JC_Jd(void)
{

	if (CC_NC) {
		JMPNOP(2, 4);
	} else {
		JMPNEAR32(7);
	}
}

/* jnc */
void
JNC_Jb(void)
{

	if (CC_C) {
		JMPNOP(2, 1);
	} else {
		JMPSHORT(7);
	}
}
void
JNC_Jw(void)
{

	if (CC_C) {
		JMPNOP(2, 2);
	} else {
		JMPNEAR(7);
	}
}
void
JNC_Jd(void)
{

	if (CC_C) {
		JMPNOP(2, 4);
	} else {
		JMPNEAR32(7);
	}
}

/* jz */
void
JZ_Jb(void)
{

	if (CC_NZ) {
		JMPNOP(2, 1);
	} else {
		JMPSHORT(7);
	}
}

void
JZ_Jw(void)
{

	if (CC_NZ) {
		JMPNOP(2, 2);
	} else {
		JMPNEAR(7);
	}
}

void
JZ_Jd(void)
{

	if (CC_NZ) {
		JMPNOP(2, 4);
	} else {
		JMPNEAR32(7);
	}
}

/* jnz */
void
JNZ_Jb(void)
{

	if (CC_Z) {
		JMPNOP(2, 1);
	} else {
		JMPSHORT(7);
	}
}

void
JNZ_Jw(void)
{

	if (CC_Z) {
		JMPNOP(2, 2);
	} else {
		JMPNEAR(7);
	}
}

void
JNZ_Jd(void)
{

	if (CC_Z) {
		JMPNOP(2, 4);
	} else {
		JMPNEAR32(7);
	}
}

/* jna */
void
JNA_Jb(void)
{

	if (CC_A) {
		JMPNOP(2, 1);
	} else {
		JMPSHORT(7);
	}
}

void
JNA_Jw(void)
{

	if (CC_A) {
		JMPNOP(2, 2);
	} else {
		JMPNEAR(7);
	}
}

void
JNA_Jd(void)
{

	if (CC_A) {
		JMPNOP(2, 4);
	} else {
		JMPNEAR32(7);
	}
}

/* ja */
void
JA_Jb(void)
{

	if (CC_NA) {
		JMPNOP(2, 1);
	} else {
		JMPSHORT(7);
	}
}

void
JA_Jw(void)
{

	if (CC_NA) {
		JMPNOP(2, 2);
	} else {
		JMPNEAR(7);
	}
}

void
JA_Jd(void)
{

	if (CC_NA) {
		JMPNOP(2, 4);
	} else {
		JMPNEAR32(7);
	}
}

/* js */
void
JS_Jb(void)
{

	if (CC_NS) {
		JMPNOP(2, 1);
	} else {
		JMPSHORT(7);
	}
}

void
JS_Jw(void)
{

	if (CC_NS) {
		JMPNOP(2, 2);
	} else {
		JMPNEAR(7);
	}
}

void
JS_Jd(void)
{

	if (CC_NS) {
		JMPNOP(2, 4);
	} else {
		JMPNEAR32(7);
	}
}

/* jns */
void
JNS_Jb(void)
{

	if (CC_S) {
		JMPNOP(2, 1);
	} else {
		JMPSHORT(7);
	}
}

void
JNS_Jw(void)
{

	if (CC_S) {
		JMPNOP(2, 2);
	} else {
		JMPNEAR(7);
	}
}

void
JNS_Jd(void)
{

	if (CC_S) {
		JMPNOP(2, 4);
	} else {
		JMPNEAR32(7);
	}
}

/* jp */
void
JP_Jb(void)
{

	if (CC_NP) {
		JMPNOP(2, 1);
	} else {
		JMPSHORT(7);
	}
}

void
JP_Jw(void)
{

	if (CC_NP) {
		JMPNOP(2, 2);
	} else {
		JMPNEAR(7);
	}
}

void
JP_Jd(void)
{

	if (CC_NP) {
		JMPNOP(2, 4);
	} else {
		JMPNEAR32(7);
	}
}

/* jnp */
void
JNP_Jb(void)
{

	if (CC_P) {
		JMPNOP(2, 1);
	} else {
		JMPSHORT(7);
	}
}

void
JNP_Jw(void)
{

	if (CC_P) {
		JMPNOP(2, 2);
	} else {
		JMPNEAR(7);
	}
}

void
JNP_Jd(void)
{

	if (CC_P) {
		JMPNOP(2, 4);
	} else {
		JMPNEAR32(7);
	}
}

/* jl */
void
JL_Jb(void)
{

	if (CC_NL) {
		JMPNOP(2, 1);
	} else {
		JMPSHORT(7);
	}
}

void
JL_Jw(void)
{

	if (CC_NL) {
		JMPNOP(2, 2);
	} else {
		JMPNEAR(7);
	}
}

void
JL_Jd(void)
{

	if (CC_NL) {
		JMPNOP(2, 4);
	} else {
		JMPNEAR32(7);
	}
}

/* jnl */
void
JNL_Jb(void)
{

	if (CC_L) {
		JMPNOP(2, 1);
	} else {
		JMPSHORT(7);
	}
}

void
JNL_Jw(void)
{

	if (CC_L) {
		JMPNOP(2, 2);
	} else {
		JMPNEAR(7);
	}
}

void
JNL_Jd(void)
{

	if (CC_L) {
		JMPNOP(2, 4);
	} else {
		JMPNEAR32(7);
	}
}

/* jle */
void
JLE_Jb(void)
{

	if (CC_NLE) {
		JMPNOP(2, 1);
	} else {
		JMPSHORT(7);
	}
}

void
JLE_Jw(void)
{

	if (CC_NLE) {
		JMPNOP(2, 2);
	} else {
		JMPNEAR(7);
	}
}

void
JLE_Jd(void)
{

	if (CC_NLE) {
		JMPNOP(2, 4);
	} else {
		JMPNEAR32(7);
	}
}

/* jnle */
void
JNLE_Jb(void)
{

	if (CC_LE) {
		JMPNOP(2, 1);
	} else {
		JMPSHORT(7);
	}
}

void
JNLE_Jw(void)
{

	if (CC_LE) {
		JMPNOP(2, 2);
	} else {
		JMPNEAR(7);
	}
}

void
JNLE_Jd(void)
{

	if (CC_LE) {
		JMPNOP(2, 4);
	} else {
		JMPNEAR32(7);
	}
}

/* jcxz */
void
JeCXZ_Jb(void)
{

	if (!CPU_INST_AS32) {
		if (CPU_CX == 0) {
			JMPSHORT(8);
		} else {
			JMPNOP(4, 1);
		}
	} else {
		if (CPU_ECX == 0) {
			JMPSHORT(8);
		} else {
			JMPNOP(4, 1);
		}
	}
}

/*
 * LOOPcc
 */
/* loopne */
void
LOOPNE_Jb(void)
{
	UINT32 cx;

	if (!CPU_INST_AS32) {
		cx = CPU_CX;
		if (--cx != 0 && CC_NZ) {
			JMPSHORT(8);
		} else {
			JMPNOP(4, 1);
		}
		CPU_CX--;
	} else {
		cx = CPU_ECX;
		if (--cx != 0 && CC_NZ) {
			JMPSHORT(8);
		} else {
			JMPNOP(4, 1);
		}
		CPU_ECX--;
	}
}

/* loope */
void
LOOPE_Jb(void)
{
	UINT32 cx;

	if (!CPU_INST_AS32) {
		cx = CPU_CX;
		if (--cx != 0 && CC_Z) {
			JMPSHORT(8);
		} else {
			JMPNOP(4, 1);
		}
		CPU_CX--;
	} else {
		cx = CPU_ECX;
		if (--cx != 0 && CC_Z) {
			JMPSHORT(8);
		} else {
			JMPNOP(4, 1);
		}
		CPU_ECX--;
	}
}

/* loop */
void
LOOP_Jb(void)
{
	UINT32 cx;

	if (!CPU_INST_AS32) {
		cx = CPU_CX;
		if (--cx != 0) {
			JMPSHORT(8);
		} else {
			JMPNOP(4, 1);
		}
		CPU_CX--;
	} else {
		cx = CPU_ECX;
		if (--cx != 0) {
			JMPSHORT(8);
		} else {
			JMPNOP(4, 1);
		}
		CPU_ECX--;
	}
}

/*
 * CALL
 */
void
CALL_Aw(void)
{
	UINT16 new_ip;
	SINT16 dest;

	CPU_WORKCLOCK(7);
	CPU_SET_PREV_ESP();
	GET_PCWORDS(dest);
	new_ip = CPU_EIP + dest;
	if (new_ip > CPU_STAT_CS_LIMIT) {
		EXCEPTION(GP_EXCEPTION, 0);
	}
	PUSH0_16(CPU_IP);
	CPU_EIP = new_ip;
	CPU_CLEAR_PREV_ESP();
}

void
CALL_Ad(void)
{
	UINT32 new_ip;
	UINT32 dest;

	CPU_WORKCLOCK(7);
	CPU_SET_PREV_ESP();
	GET_PCDWORD(dest);
	new_ip = CPU_EIP + dest;
	if (new_ip > CPU_STAT_CS_LIMIT) {
		EXCEPTION(GP_EXCEPTION, 0);
	}
	PUSH0_32(CPU_EIP);
	CPU_EIP = new_ip;
	CPU_CLEAR_PREV_ESP();
}

void CPUCALL
CALL_Ew(UINT32 op)
{
	UINT32 madr;
	UINT16 new_ip;

	CPU_SET_PREV_ESP();
	if (op >= 0xc0) {
		CPU_WORKCLOCK(7);
		new_ip = *(CPU_REG16_B20(op));
	} else {
		CPU_WORKCLOCK(11);
		madr = calc_ea_dst(op);
		new_ip = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, madr);
	}
	if (new_ip > CPU_STAT_CS_LIMIT) {
		EXCEPTION(GP_EXCEPTION, 0);
	}
	PUSH0_16(CPU_IP);
	CPU_EIP = new_ip;
	CPU_CLEAR_PREV_ESP();
}

void CPUCALL
CALL_Ed(UINT32 op)
{
	UINT32 madr;
	UINT32 new_ip;

	CPU_SET_PREV_ESP();
	if (op >= 0xc0) {
		CPU_WORKCLOCK(7);
		new_ip = *(CPU_REG32_B20(op));
	} else {
		CPU_WORKCLOCK(11);
		madr = calc_ea_dst(op);
		new_ip = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, madr);
	}
	if (new_ip > CPU_STAT_CS_LIMIT) {
		EXCEPTION(GP_EXCEPTION, 0);
	}
	PUSH0_32(CPU_EIP);
	CPU_EIP = new_ip;
	CPU_CLEAR_PREV_ESP();
}

void
CALL16_Ap(void)
{
	descriptor_t sd;
	UINT16 new_ip;
	UINT16 new_cs;
	UINT16 sreg;
	UINT32 sp;
	UINT32 tmp_sp;

	CPU_WORKCLOCK(13);
	GET_PCWORD(new_ip);
	GET_PCWORD(new_cs);
	if (!CPU_STAT_PM || CPU_STAT_VM86) {
		/* Real mode or VM86 mode */
		load_segreg(CPU_CS_INDEX, new_cs, &sreg, &sd, GP_EXCEPTION);
		if (new_ip > sd.u.seg.limit) {
			EXCEPTION(GP_EXCEPTION, 0);
		}

		sp = CPU_STAT_SS32 ? CPU_ESP : CPU_SP;
		SS_PUSH_CHECK(sp, 4);

		tmp_sp = sp;
		current_stack_push16(&tmp_sp, CPU_CS);
		current_stack_push16(&tmp_sp, CPU_IP);

		/* Commit only after all faultable stack writes succeeded. */
		commit_current_stack_pointer(tmp_sp);
		LOAD_SEGREG(CPU_CS_INDEX, new_cs);
		CPU_EIP = new_ip;
	} else {
		/* Protected mode */
		CALLfar_pm(new_cs, new_ip);
	}
}

void
CALL32_Ap(void)
{
	descriptor_t sd;
	UINT32 new_ip;
	UINT16 new_cs;
	UINT16 sreg;
	UINT32 sp;
	UINT32 tmp_sp;

	CPU_WORKCLOCK(13);
	GET_PCDWORD(new_ip);
	GET_PCWORD(new_cs);
	if (!CPU_STAT_PM || CPU_STAT_VM86) {
		/* Real mode or VM86 mode */
		load_segreg(CPU_CS_INDEX, new_cs, &sreg, &sd, GP_EXCEPTION);
		if (new_ip > sd.u.seg.limit) {
			EXCEPTION(GP_EXCEPTION, 0);
		}

		sp = CPU_STAT_SS32 ? CPU_ESP : CPU_SP;
		SS_PUSH_CHECK(sp, 8);

		tmp_sp = sp;
		current_stack_push32(&tmp_sp, CPU_CS);
		current_stack_push32(&tmp_sp, CPU_EIP);

		/* Commit only after all faultable stack writes succeeded. */
		commit_current_stack_pointer(tmp_sp);
		LOAD_SEGREG(CPU_CS_INDEX, new_cs);
		CPU_EIP = new_ip;
	} else {
		/* Protected mode */
		CALLfar_pm(new_cs, new_ip);
	}
}

void CPUCALL
CALL16_Ep(UINT32 op)
{
	descriptor_t sd;
	UINT32 madr;
	UINT16 new_ip;
	UINT16 new_cs;
	UINT16 sreg;
	UINT32 sp;
	UINT32 tmp_sp;

	CPU_WORKCLOCK(16);
	if (op < 0xc0) {
		madr = calc_ea_dst(op);
		new_ip = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, madr);
		new_cs = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, madr + 2);
		if (!CPU_STAT_PM || CPU_STAT_VM86) {
			/* Real mode or VM86 mode */
			load_segreg(CPU_CS_INDEX, new_cs, &sreg, &sd, GP_EXCEPTION);
			if (new_ip > sd.u.seg.limit) {
				EXCEPTION(GP_EXCEPTION, 0);
			}

			sp = CPU_STAT_SS32 ? CPU_ESP : CPU_SP;
			SS_PUSH_CHECK(sp, 4);

			tmp_sp = sp;
			current_stack_push16(&tmp_sp, CPU_CS);
			current_stack_push16(&tmp_sp, CPU_IP);

			/* Commit only after all faultable stack writes succeeded. */
			commit_current_stack_pointer(tmp_sp);
			LOAD_SEGREG(CPU_CS_INDEX, new_cs);
			CPU_EIP = new_ip;
		} else {
			/* Protected mode */
			CALLfar_pm(new_cs, new_ip);
		}
		return;
	} 
	EXCEPTION(UD_EXCEPTION, 0);
}

void CPUCALL
CALL32_Ep(UINT32 op)
{
	descriptor_t sd;
	UINT32 madr;
	UINT32 new_ip;
	UINT16 new_cs;
	UINT16 sreg;
	UINT32 sp;
	UINT32 tmp_sp;

	CPU_WORKCLOCK(16);
	if (op < 0xc0) {
		madr = calc_ea_dst(op);
		new_ip = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, madr);
		new_cs = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, madr + 4);
		if (!CPU_STAT_PM || CPU_STAT_VM86) {
			/* Real mode or VM86 mode */
			load_segreg(CPU_CS_INDEX, new_cs, &sreg, &sd, GP_EXCEPTION);
			if (new_ip > sd.u.seg.limit) {
				EXCEPTION(GP_EXCEPTION, 0);
			}

			sp = CPU_STAT_SS32 ? CPU_ESP : CPU_SP;
			SS_PUSH_CHECK(sp, 8);

			tmp_sp = sp;
			current_stack_push32(&tmp_sp, CPU_CS);
			current_stack_push32(&tmp_sp, CPU_EIP);

			/* Commit only after all faultable stack writes succeeded. */
			commit_current_stack_pointer(tmp_sp);
			LOAD_SEGREG(CPU_CS_INDEX, new_cs);
			CPU_EIP = new_ip;
		} else {
			/* Protected mode */
			CALLfar_pm(new_cs, new_ip);
		}
		return;
	}
	EXCEPTION(UD_EXCEPTION, 0);
}

/*
 * RET
 */
void
RETnear16(void)
{
	UINT16 new_ip;

	CPU_WORKCLOCK(11);
	CPU_SET_PREV_ESP();
	POP0_16(new_ip);
	if (new_ip > CPU_STAT_CS_LIMIT) {
		EXCEPTION(GP_EXCEPTION, 0);
	}
	CPU_EIP = new_ip;
	CPU_CLEAR_PREV_ESP();
}

void
RETnear32(void)
{
	UINT32 new_ip;

	CPU_WORKCLOCK(11);
	CPU_SET_PREV_ESP();
	POP0_32(new_ip);
	if (new_ip > CPU_STAT_CS_LIMIT) {
		EXCEPTION(GP_EXCEPTION, 0);
	}
	CPU_EIP = new_ip;
	CPU_CLEAR_PREV_ESP();
}

void
RETnear16_Iw(void)
{
	UINT16 new_ip;
	UINT16 size;

	CPU_WORKCLOCK(11);
	CPU_SET_PREV_ESP();
	GET_PCWORD(size);
	POP0_16(new_ip);
	if (new_ip > CPU_STAT_CS_LIMIT) {
		EXCEPTION(GP_EXCEPTION, 0);
	}
	CPU_EIP = new_ip;
	if (!CPU_STAT_SS32) {
		CPU_SP += size;
	} else {
		CPU_ESP += size;
	}
	CPU_CLEAR_PREV_ESP();
}

void
RETnear32_Iw(void)
{
	UINT32 new_ip;
	UINT16 size;

	CPU_WORKCLOCK(11);
	CPU_SET_PREV_ESP();
	GET_PCWORD(size);
	POP0_32(new_ip);
	if (new_ip > CPU_STAT_CS_LIMIT) {
		EXCEPTION(GP_EXCEPTION, 0);
	}
	CPU_EIP = new_ip;
	if (!CPU_STAT_SS32) {
		CPU_SP += size;
	} else {
		CPU_ESP += size;
	}
	CPU_CLEAR_PREV_ESP();
}

void
RETfar16(void)
{
	descriptor_t sd;
	UINT16 new_ip;
	UINT16 new_cs;
	UINT16 sreg;

	CPU_WORKCLOCK(15);
	if (!CPU_STAT_PM || CPU_STAT_VM86) {
		/* Real mode or VM86 mode */
		CPU_SET_PREV_ESP();
		POP0_16(new_ip);
		POP0_16(new_cs);

		/* check new instrunction pointer with new code segment */
		load_segreg(CPU_CS_INDEX, new_cs, &sreg, &sd, GP_EXCEPTION);
		if (new_ip > sd.u.seg.limit) {
			EXCEPTION(GP_EXCEPTION, 0);
		}

		LOAD_SEGREG(CPU_CS_INDEX, new_cs);
		CPU_EIP = new_ip;
		CPU_CLEAR_PREV_ESP();
	} else {
		/* Protected mode */
		RETfar_pm(0);
	}
}

void
RETfar32(void)
{
	descriptor_t sd;
	UINT32 new_ip;
	UINT32 new_cs;
	UINT16 sreg;

	CPU_WORKCLOCK(15);
	if (!CPU_STAT_PM || CPU_STAT_VM86) {
		/* Real mode or VM86 mode */
		CPU_SET_PREV_ESP();
		POP0_32(new_ip);
		POP0_32(new_cs);

		/* check new instrunction pointer with new code segment */
		load_segreg(CPU_CS_INDEX, (UINT16)new_cs, &sreg, &sd, GP_EXCEPTION);
		if (new_ip > sd.u.seg.limit) {
			EXCEPTION(GP_EXCEPTION, 0);
		}

		LOAD_SEGREG(CPU_CS_INDEX, (UINT16)new_cs);
		CPU_EIP = new_ip;
		CPU_CLEAR_PREV_ESP();
	} else {
		/* Protected mode */
		RETfar_pm(0);
	}
}

void
RETfar16_Iw(void)
{
	descriptor_t sd;
	UINT16 new_ip;
	UINT16 new_cs;
	UINT16 sreg;
	UINT16 size;

	CPU_WORKCLOCK(15);
	GET_PCWORD(size);
	if (!CPU_STAT_PM || CPU_STAT_VM86) {
		/* Real mode or VM86 mode */
		CPU_SET_PREV_ESP();
		POP0_16(new_ip);
		POP0_16(new_cs);

		/* check new instrunction pointer with new code segment */
		load_segreg(CPU_CS_INDEX, new_cs, &sreg, &sd, GP_EXCEPTION);
		if (new_ip > sd.u.seg.limit) {
			EXCEPTION(GP_EXCEPTION, 0);
		}

		LOAD_SEGREG(CPU_CS_INDEX, new_cs);
		CPU_EIP = new_ip;
		if (!CPU_STAT_SS32) {
			CPU_SP += size;
		} else {
			CPU_ESP += size;
		}
		CPU_CLEAR_PREV_ESP();
	} else {
		/* Protected mode */
		RETfar_pm(size);
	}
}

void
RETfar32_Iw(void)
{
	descriptor_t sd;
	UINT32 new_ip;
	UINT32 new_cs;
	UINT16 sreg;
	UINT16 size;

	CPU_WORKCLOCK(15);
	GET_PCWORD(size);
	if (!CPU_STAT_PM || CPU_STAT_VM86) {
		/* Real mode or VM86 mode */
		CPU_SET_PREV_ESP();
		POP0_32(new_ip);
		POP0_32(new_cs);

		/* check new instrunction pointer with new code segment */
		load_segreg(CPU_CS_INDEX, (UINT16)new_cs, &sreg, &sd, GP_EXCEPTION);
		if (new_ip > sd.u.seg.limit) {
			EXCEPTION(GP_EXCEPTION, 0);
		}

		LOAD_SEGREG(CPU_CS_INDEX, (UINT16)new_cs);
		CPU_EIP = new_ip;
		if (!CPU_STAT_SS32) {
			CPU_SP += size;
		} else {
			CPU_ESP += size;
		}
		CPU_CLEAR_PREV_ESP();
	} else {
		/* Protected mode */
		RETfar_pm(size);
	}
}

void
IRET(void)
{
	descriptor_t sd;
	UINT32 new_ip;
	UINT32 new_flags;
	UINT32 new_cs;
	UINT32 mask;
	UINT16 sreg;

	CPU_WORKCLOCK(22);
	if (!CPU_STAT_PM) {
		/* Real mode */
		CPU_SET_PREV_ESP();
		mask = I_FLAG|IOPL_FLAG;
		if (!CPU_INST_OP32) {
			POP0_16(new_ip);
			POP0_16(new_cs);
			POP0_16(new_flags);
		} else {
			POP0_32(new_ip);
			POP0_32(new_cs);
			POP0_32(new_flags);
			mask |= RF_FLAG;
		}

		/* check new instrunction pointer with new code segment */
		load_segreg(CPU_CS_INDEX, (UINT16)new_cs, &sreg, &sd, GP_EXCEPTION);
		if (new_ip > sd.u.seg.limit) {
			EXCEPTION(GP_EXCEPTION, 0);
		}

		LOAD_SEGREG(CPU_CS_INDEX, (UINT16)new_cs);
		CPU_EIP = new_ip;

		set_eflags(new_flags, mask);
		CPU_CLEAR_PREV_ESP();
	} else {
		/* Protected mode */
		IRET_pm();
	}
	IRQCHECKTERM();
}

/*
 * INT
 */
void
INT1(void)
{

	CPU_WORKCLOCK(33);
	INTERRUPT(1, INTR_TYPE_SOFTINTR);
}

void
INT3(void)
{
#if defined(SUPPORT_IA32_HAXM)
#if defined(USE_CUSTOM_HOOKINST)
	if(bioshookinfo.hookinst == 0xCC){
		if (!CPU_STAT_PM || CPU_STAT_VM86) {
			UINT32 adrs;
			adrs = CPU_PREV_EIP + (CPU_CS << 4);
			if (
#if defined(SUPPORT_EMU_SOUNDBIOS)
				soundrom_isbiosaddr(adrs) ||
#endif
				((adrs >= 0xf8000) && (adrs < 0x100000))) {
				ia32_bioscall();
				return;
			}
		}
	}
#endif
#endif

	CPU_WORKCLOCK(33);
	INTERRUPT(3, INTR_TYPE_SOFTINTR);
}

void
INTO(void)
{

	if (!CPU_OV) {
		CPU_WORKCLOCK(3);
		return;
	}
	CPU_WORKCLOCK(35);
	INTERRUPT(4, INTR_TYPE_SOFTINTR);
}

void
INT_Ib(void)
{
	UINT8 vect;

	CPU_WORKCLOCK(37);
	if (!CPU_STAT_PM || !CPU_STAT_VM86 || (CPU_STAT_IOPL == CPU_IOPL3)) {
		GET_MODRM_PCBYTE(vect);
#if defined(ENABLE_TRAP)
		softinttrap(CPU_CS, CPU_EIP - 2, vect);
#endif
		INTERRUPT(vect, INTR_TYPE_SOFTINTR);
		return;
	}
	VERBOSE(("INT_Ib: VM86 && IOPL < 3 && INTn"));
#if defined(USE_VME)
	GET_MODRM_PCBYTE(vect);
	if (vme_int_redirect_allowed(vect)) {
		VERBOSE(("INT_Ib: VME redirect INT %02x", vect));
		vme_redirect_int(vect);
		return;
	}
	CPU_EIP = CPU_PREV_EIP;
#endif
	EXCEPTION(GP_EXCEPTION, 0);
}

void
BOUND_GwMa(void)
{
	UINT32 op, madr;
	UINT16 reg;

	CPU_WORKCLOCK(13);
	GET_MODRM_PCBYTE(op);
	if (op < 0xc0) {
		reg = *(CPU_REG16_B53(op));
		madr = calc_ea_dst(op);
		if (reg >= cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, madr) &&
		    reg <= cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, madr + 2)) {
				return;
		}
		EXCEPTION(BR_EXCEPTION, 0);
		return;
	}
	EXCEPTION(UD_EXCEPTION, 0);
}

void
BOUND_GdMa(void)
{
	UINT32 op, madr;
	UINT32 reg;

	CPU_WORKCLOCK(13);
	GET_MODRM_PCBYTE(op);
	if (op < 0xc0) {
		reg = *(CPU_REG32_B53(op));
		madr = calc_ea_dst(op);
		if (reg >= cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, madr) &&
		    reg <= cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, madr + 4)) {
				return;
		}
		EXCEPTION(BR_EXCEPTION, 0);
		return;
	}
	EXCEPTION(UD_EXCEPTION, 0);
}

/*
 * STACK
 */
void
ENTER16_IwIb(void)
{
	UINT32 sp, bp;
	UINT32 val;
	UINT16 dimsize;
	UINT16 new_bp;
	UINT8 level;

	GET_PCWORD(dimsize);
	GET_PCBYTE(level);
	level &= 0x1f;

	CPU_SET_PREV_ESP();
	PUSH0_16(CPU_BP);
	if (level == 0) {			/* enter level=0 */
		CPU_WORKCLOCK(11);
		CPU_BP = CPU_SP;
		if (!CPU_STAT_SS32) {
			CPU_SP -= dimsize;
		} else {
			CPU_ESP -= dimsize;
		}
	} else {
		--level;
		if (level == 0) {		/* enter level=1 */
			CPU_WORKCLOCK(15);
			sp = CPU_SP;
			PUSH0_16(sp);
			CPU_BP = (UINT16)sp;
			if (!CPU_STAT_SS32) {
				CPU_SP -= dimsize;
			} else {
				CPU_ESP -= dimsize;
			}
		} else {			/* enter level=2-31 */
			CPU_WORKCLOCK(12 + level * 4);
			if (!CPU_STAT_SS32) {
				bp = CPU_BP;
				new_bp = CPU_SP;
				while (level--) {
					bp -= 2;
					CPU_SP -= 2;
					val = cpu_vmemoryread_w(CPU_SS_INDEX, bp);
					cpu_vmemorywrite_w(CPU_SS_INDEX, CPU_SP, (UINT16)val);
				}
				REGPUSH0(new_bp);
				CPU_BP = new_bp;
				CPU_SP -= dimsize;
			} else {
				bp = CPU_EBP;
				new_bp = CPU_SP;
				while (level--) {
					bp -= 2;
					CPU_ESP -= 2;
					val = cpu_vmemoryread_w(CPU_SS_INDEX, bp);
					cpu_vmemorywrite_w(CPU_SS_INDEX, CPU_ESP, (UINT16)val);
				}
				REGPUSH0_16_32(new_bp);
				CPU_BP = new_bp;
				CPU_ESP -= dimsize;
			}
		}
	}
	CPU_CLEAR_PREV_ESP();
}

void
ENTER32_IwIb(void)
{
	UINT32 sp, bp;
	UINT32 new_bp;
	UINT32 val;
	UINT16 dimsize;
	UINT8 level;

	GET_PCWORD(dimsize);
	GET_PCBYTE(level);
	level &= 0x1f;

	CPU_SET_PREV_ESP();
	PUSH0_32(CPU_EBP);
	if (level == 0) {			/* enter level=0 */
		CPU_WORKCLOCK(11);
		CPU_EBP = CPU_ESP;
		if (!CPU_STAT_SS32) {
			CPU_SP -= dimsize;
		} else {
			CPU_ESP -= dimsize;
		}
	} else {
		--level;
		if (level == 0) {		/* enter level=1 */
			CPU_WORKCLOCK(15);
			sp = CPU_ESP;
			PUSH0_32(sp);
			CPU_EBP = sp;
			if (CPU_STAT_SS32) {
				CPU_ESP -= dimsize;
			} else {
				CPU_SP -= dimsize;
			}
		} else {			/* enter level=2-31 */
			CPU_WORKCLOCK(12 + level * 4);
			if (CPU_STAT_SS32) {
				bp = CPU_EBP;
				new_bp = CPU_ESP;
				while (level--) {
					bp -= 4;
					CPU_ESP -= 4;
					val = cpu_vmemoryread_d(CPU_SS_INDEX, bp);
					cpu_vmemorywrite_d(CPU_SS_INDEX, CPU_ESP, val);
				}
				REGPUSH0_32(new_bp);
				CPU_EBP = new_bp;
				CPU_ESP -= dimsize;
			} else {
				bp = CPU_BP;
				new_bp = CPU_ESP;
				while (level--) {
					bp -= 4;
					CPU_SP -= 4;
					val = cpu_vmemoryread_d(CPU_SS_INDEX, bp);
					cpu_vmemorywrite_d(CPU_SS_INDEX, CPU_SP, val);
				}
				REGPUSH0_32_16(new_bp);
				CPU_EBP = new_bp;
				CPU_SP -= dimsize;
			}
		}
	}
	CPU_CLEAR_PREV_ESP();
}

void
LEAVE(void)
{

	CPU_WORKCLOCK(4);

	CPU_SET_PREV_ESP();
	if (!CPU_STAT_SS32) {
		CPU_SP = CPU_BP;
	} else {
		CPU_ESP = CPU_EBP;
	}
	if (!CPU_INST_OP32) {
		POP0_16(CPU_BP);
	} else {
		POP0_32(CPU_EBP);
	}
	CPU_CLEAR_PREV_ESP();
}
