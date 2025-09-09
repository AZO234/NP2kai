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

#include <compiler.h>
#include <ia32/cpu.h>
#include "ia32/ia32.mcr"
#include <pccore.h>

#include "system_inst.h"


void CPUCALL
LGDT_Ms(UINT32 op)
{
	UINT32 madr;
	UINT32 base;
	UINT16 limit;

	if (op < 0xc0) {
		if (!CPU_STAT_PM || (CPU_STAT_CPL == 0 && !CPU_STAT_VM86)) {
			CPU_WORKCLOCK(11);
			madr = calc_ea_dst(op);
			limit = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, madr);
			base = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, madr + 2);
			if (!CPU_INST_OP32) {
				base &= 0x00ffffff;
			}

#if defined(MORE_DEBUG)
			gdtr_dump(base, limit);
#endif

			CPU_GDTR_BASE = base;
			CPU_GDTR_LIMIT = limit;
			return;
		}
		VERBOSE(("LGDT: VM86(%s) or CPL(%d) != 0", CPU_STAT_VM86 ? "true" : "false", CPU_STAT_CPL));
		EXCEPTION(GP_EXCEPTION, 0);
	}
	EXCEPTION(UD_EXCEPTION, 0);
}

void CPUCALL
SGDT_Ms(UINT32 op)
{
	UINT32 madr;
	UINT32 base;
	UINT16 limit;

	if (op < 0xc0) {
		CPU_WORKCLOCK(11);
		limit = CPU_GDTR_LIMIT;
		base = CPU_GDTR_BASE;
		if (!CPU_INST_OP32) {
			base &= 0x00ffffff;
		}
		madr = calc_ea_dst(op);
		cpu_vmemorywrite_w(CPU_INST_SEGREG_INDEX, madr, limit);
		cpu_vmemorywrite_d(CPU_INST_SEGREG_INDEX, madr + 2, base);
		return;
	}
	EXCEPTION(UD_EXCEPTION, 0);
}

void CPUCALL
LLDT_Ew(UINT32 op)
{
	UINT32 madr;
	UINT16 src;

	if (CPU_STAT_PM && !CPU_STAT_VM86) {
		if (CPU_STAT_CPL == 0) {
			if (op >= 0xc0) {
				CPU_WORKCLOCK(5);
				src = *(reg16_b20[op]);
			} else {
				CPU_WORKCLOCK(11);
				madr = calc_ea_dst(op);
				src = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, madr);
			}
			load_ldtr(src, GP_EXCEPTION);
			return;
		}
		VERBOSE(("LLDT: CPL(%d) != 0", CPU_STAT_CPL));
		EXCEPTION(GP_EXCEPTION, 0);
	}
	VERBOSE(("LLDT: real-mode or VM86"));
	EXCEPTION(UD_EXCEPTION, 0);
}

void CPUCALL
SLDT_Ew(UINT32 op)
{
	UINT32 madr;
	UINT16 ldtr;

	if (CPU_STAT_PM && !CPU_STAT_VM86) {
		ldtr = CPU_LDTR;
		if (op >= 0xc0) {
			CPU_WORKCLOCK(5);
			if (CPU_INST_OP32) {
				*(reg32_b20[op]) = ldtr;
			} else {
				*(reg16_b20[op]) = ldtr;
			}
		} else {
			CPU_WORKCLOCK(11);
			madr = calc_ea_dst(op);
			cpu_vmemorywrite_w(CPU_INST_SEGREG_INDEX, madr, ldtr);
		}
		return;
	}
	VERBOSE(("SLDT: real-mode or VM86"));
	EXCEPTION(UD_EXCEPTION, 0);
}

void CPUCALL
LTR_Ew(UINT32 op)
{
	UINT32 madr;
	UINT16 src;

	if (CPU_STAT_PM && !CPU_STAT_VM86) {
		if (CPU_STAT_CPL == 0) {
			if (op >= 0xc0) {
				CPU_WORKCLOCK(5);
				src = *(reg16_b20[op]);
			} else {
				CPU_WORKCLOCK(11);
				madr = calc_ea_dst(op);
				src = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, madr);
			}
			load_tr(src);
			return;
		}
		VERBOSE(("LTR: CPL(%d) != 0", CPU_STAT_CPL));
		EXCEPTION(GP_EXCEPTION, 0);
	}
	VERBOSE(("LTR: real-mode or VM86"));
	EXCEPTION(UD_EXCEPTION, 0);
}

void CPUCALL
STR_Ew(UINT32 op)
{
	UINT32 madr;
	UINT16 tr;

	if (CPU_STAT_PM && !CPU_STAT_VM86) {
		tr = CPU_TR;
		if (op >= 0xc0) {
			CPU_WORKCLOCK(5);
			if (CPU_INST_OP32) {
				*(reg32_b20[op]) = tr;
			} else {
				*(reg16_b20[op]) = tr;
			}
		} else {
			CPU_WORKCLOCK(11);
			madr = calc_ea_dst(op);
			cpu_vmemorywrite_w(CPU_INST_SEGREG_INDEX, madr, tr);
		}
		return;
	}
	VERBOSE(("STR: real-mode or VM86"));
	EXCEPTION(UD_EXCEPTION, 0);
}

void CPUCALL
LIDT_Ms(UINT32 op)
{
	UINT32 madr;
	UINT32 base;
	UINT16 limit;

	if (op < 0xc0) {
		if (!CPU_STAT_PM || (CPU_STAT_CPL == 0 && !CPU_STAT_VM86)) {
			CPU_WORKCLOCK(11);
			madr = calc_ea_dst(op);
			limit = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, madr);
			base = cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, madr + 2);
			if (!CPU_INST_OP32) {
				base &= 0x00ffffff;
			}

#if defined(MORE_DEBUG)
			idtr_dump(base, limit);
#endif

			CPU_IDTR_BASE = base;
			CPU_IDTR_LIMIT = limit;
			return;
		}
		VERBOSE(("LIDT: VM86(%s) or CPL(%d) != 0", CPU_STAT_VM86 ? "true" : "false", CPU_STAT_CPL));
		EXCEPTION(GP_EXCEPTION, 0);
	}
	EXCEPTION(UD_EXCEPTION, 0);
}

void CPUCALL
SIDT_Ms(UINT32 op)
{
	UINT32 madr;
	UINT32 base;
	UINT16 limit;

	if (op < 0xc0) {
		CPU_WORKCLOCK(11);
		limit = CPU_IDTR_LIMIT;
		base = CPU_IDTR_BASE;
		if (!CPU_INST_OP32) {
			base &= 0x00ffffff;
		}
		madr = calc_ea_dst(op);
		cpu_vmemorywrite_w(CPU_INST_SEGREG_INDEX, madr, limit);
		cpu_vmemorywrite_d(CPU_INST_SEGREG_INDEX, madr + 2, base);
		return;
	}
	EXCEPTION(UD_EXCEPTION, 0);
}

void
MOV_CdRd(void)
{
	UINT32 op, src;
	UINT32 reg;
	int idx;

	CPU_WORKCLOCK(11);
	GET_PCBYTE(op);
	if (op >= 0xc0) {
		if (CPU_STAT_PM && (CPU_STAT_VM86 || CPU_STAT_CPL != 0)) {
			VERBOSE(("MOV_CdRd: VM86(%s) or CPL(%d) != 0", CPU_STAT_VM86 ? "true" : "false", CPU_STAT_CPL));
			EXCEPTION(GP_EXCEPTION, 0);
		}

		src = *(reg32_b20[op]);
		idx = (op >> 3) & 7;

		switch (idx) {
		case 0: /* CR0 */
			/*
			 * 0 = PE (protect enable)
			 * 1 = MP (monitor coprocesser)
			 * 2 = EM (emulation)
			 * 3 = TS (task switch)
			 * 4 = ET (extend type, FPU present = 1)
			 * 5 = NE (numeric error)
			 * 16 = WP (write protect)
			 * 18 = AM (alignment mask)
			 * 29 = NW (not write-through)
			 * 30 = CD (cache diable)
			 * 31 = PG (pageing)
			 */

			/* 下巻 p.182 割り込み 13 - 一般保護例外 */
			if ((src & (CPU_CR0_PE|CPU_CR0_PG)) == (UINT32)CPU_CR0_PG) {
				EXCEPTION(GP_EXCEPTION, 0);
			}
			if ((src & (CPU_CR0_NW|CPU_CR0_CD)) == CPU_CR0_NW) {
				EXCEPTION(GP_EXCEPTION, 0);
			}

			reg = CPU_CR0;
			src &= CPU_CR0_ALL;
#if defined(USE_FPU)
			if(i386cpuid.cpu_feature & CPU_FEATURE_FPU){
				src |= CPU_CR0_ET;	/* FPU present */
				//src &= ~CPU_CR0_EM;
			}else{
				src |= CPU_CR0_EM | CPU_CR0_NE;
				src &= ~(CPU_CR0_MP | CPU_CR0_ET);
			}
#else
			src |= CPU_CR0_EM | CPU_CR0_NE;
			src &= ~(CPU_CR0_MP | CPU_CR0_ET);
#endif
			CPU_CR0 = src;
			VERBOSE(("MOV_CdRd: %04x:%08x: cr0: 0x%08x <- 0x%08x(%s)", CPU_CS, CPU_PREV_EIP, reg, CPU_CR0, reg32_str[op & 7]));

			if ((reg ^ CPU_CR0) & (CPU_CR0_PE|CPU_CR0_PG)) {
				tlb_flush_all();
			}
			if ((reg ^ CPU_CR0) & CPU_CR0_PE) {
				if (CPU_CR0 & CPU_CR0_PE) {
					change_pm(1);
				}
			}
			if ((reg ^ CPU_CR0) & CPU_CR0_PG) {
				if (CPU_CR0 & CPU_CR0_PG) {
					change_pg(1);
				} else {
					change_pg(0);
				}
			}
			if ((reg ^ CPU_CR0) & CPU_CR0_PE) {
				if (!(CPU_CR0 & CPU_CR0_PE)) {
					change_pm(0);
				}
			}

			CPU_STAT_WP = (CPU_CR0 & CPU_CR0_WP) ? 0x10 : 0;
			break;

		case 2: /* CR2 */
			reg = CPU_CR2;
			CPU_CR2 = src;	/* page fault linear address */
			VERBOSE(("MOV_CdRd: %04x:%08x: cr2: 0x%08x <- 0x%08x(%s)", CPU_CS, CPU_PREV_EIP, reg, CPU_CR2, reg32_str[op & 7]));
			break;

		case 3: /* CR3 */
			/*
			 * 31-12 = page directory base
			 * 4 = PCD (page level cache diable)
			 * 3 = PWT (page level write throgh)
			 */
			reg = CPU_CR3;
			set_cr3(src);
			VERBOSE(("MOV_CdRd: %04x:%08x: cr3: 0x%08x <- 0x%08x(%s)", CPU_CS, CPU_PREV_EIP, reg, CPU_CR3, reg32_str[op & 7]));
			break;

		case 4: /* CR4 */
			/*
			 * 10 = OSXMMEXCPT (support non masking exception by OS)
			 * 9 = OSFXSR (support FXSAVE, FXRSTOR by OS)
			 * 8 = PCE (performance monitoring counter enable)
			 * 7 = PGE (page global enable)
			 * 6 = MCE (machine check enable)
			 * 5 = PAE (physical address extention)
			 * 4 = PSE (page size extention)
			 * 3 = DE (debug extention)
			 * 2 = TSD (time stamp diable)
			 * 1 = PVI (protected mode virtual interrupt)
			 * 0 = VME (VM8086 mode extention)
			 */
			reg = CPU_CR4_PCE;		/* allow bit */
			if (i386cpuid.cpu_feature & CPU_FEATURE_PGE) {
				reg |= CPU_CR4_PGE;
			}
			if (i386cpuid.cpu_feature & CPU_FEATURE_VME) {
				reg |= CPU_CR4_PVI | CPU_CR4_VME;
			}
			if (i386cpuid.cpu_feature & CPU_FEATURE_FXSR) {
				reg |= CPU_CR4_OSFXSR;
			}
			if (i386cpuid.cpu_feature & CPU_FEATURE_SSE) {
				reg |= CPU_CR4_OSXMMEXCPT;
			}
			if (src & ~reg) {
				//if (src & 0xfffffc00) {
				if (src & 0xfffff800) {
					EXCEPTION(GP_EXCEPTION, 0);
				}
				if ((src & ~reg) != CPU_CR4_DE) { // XXX: debug extentionは警告しない
					ia32_warning("MOV_CdRd: CR4 <- 0x%08x", src);
				}
			}

			reg = CPU_CR4;
			CPU_CR4 = src;
			VERBOSE(("MOV_CdRd: %04x:%08x: cr4: 0x%08x <- 0x%08x(%s)", CPU_CS, CPU_PREV_EIP, reg, CPU_CR4, reg32_str[op & 7]));

			if ((reg ^ CPU_CR4) & (CPU_CR4_PSE|CPU_CR4_PGE|CPU_CR4_PAE|CPU_CR4_PVI|CPU_CR4_VME|CPU_CR4_OSFXSR|CPU_CR4_OSXMMEXCPT)) {
				tlb_flush_all();
			}
			break;

		default:
			ia32_panic("MOV_CdRd: CR reg index (%d)", idx);
			/*NOTREACHED*/
			break;
		}
		return;
	}
	EXCEPTION(UD_EXCEPTION, 0);
}

void
MOV_RdCd(void)
{
	UINT32 *out;
	UINT32 op;
	int idx;

	CPU_WORKCLOCK(11);
	GET_PCBYTE(op);
	if (op >= 0xc0) {
		if (CPU_STAT_PM && (CPU_STAT_VM86 || CPU_STAT_CPL != 0)) {
			VERBOSE(("MOV_RdCd: VM86(%s) or CPL(%d) != 0", CPU_STAT_VM86 ? "true" : "false", CPU_STAT_CPL));
			EXCEPTION(GP_EXCEPTION, 0);
		}

		out = reg32_b20[op];
		idx = (op >> 3) & 7;

		switch (idx) {
		case 0:
			*out = CPU_CR0;
			break;

		case 2:
			*out = CPU_CR2;
			break;

		case 3:
			*out = CPU_CR3;
			break;

		case 4:
			*out = CPU_CR4;
			break;

		default:
			ia32_panic("MOV_RdCd: CR reg index (%d)", idx);
			/*NOTREACHED*/
			break;
		}
		VERBOSE(("MOV_RdCd: %04x:%08x: cr%d: 0x%08x -> %s", CPU_CS, CPU_PREV_EIP, idx, *out, reg32_str[op & 7]));
		return;
	}
	EXCEPTION(UD_EXCEPTION, 0);
}

void CPUCALL
LMSW_Ew(UINT32 op)
{
	UINT32 src, madr;
	UINT32 cr0;

	if (!CPU_STAT_PM || CPU_STAT_CPL == 0) {
		if (op >= 0xc0) {
			CPU_WORKCLOCK(2);
			src = *(reg16_b20[op]);
		} else {
			CPU_WORKCLOCK(3);
			madr = calc_ea_dst(op);
			src = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, madr);
		}

		cr0 = CPU_CR0;
		CPU_CR0 &= ~0xe; /* can't switch back from protected mode */
		CPU_CR0 |= (src & 0xf);	/* TS, EM, MP, PE */
		if (!(cr0 & CPU_CR0_PE) && (src & CPU_CR0_PE)) {
			change_pm(1);	/* switch to protected mode */
		}
		return;
	}
	VERBOSE(("LMSW: CPL(%d) != 0", CPU_STAT_CPL));
	EXCEPTION(GP_EXCEPTION, 0);
}

void CPUCALL
SMSW_Ew(UINT32 op)
{
	UINT32 madr;

	if (op >= 0xc0) {
		CPU_WORKCLOCK(2);
		if (CPU_INST_OP32) {
			*(reg32_b20[op]) = (UINT16)CPU_CR0;
		} else {
			*(reg16_b20[op]) = (UINT16)CPU_CR0;
		}
	} else {
		CPU_WORKCLOCK(3);
		madr = calc_ea_dst(op);
		cpu_vmemorywrite_w(CPU_INST_SEGREG_INDEX, madr, (UINT16)CPU_CR0);
	}
}

void
CLTS(void)
{

	CPU_WORKCLOCK(5);
	if (CPU_STAT_PM && (CPU_STAT_VM86 || (CPU_STAT_CPL != 0))) {
		VERBOSE(("CLTS: VM86(%s) or CPL(%d) != 0", CPU_STAT_VM86 ? "true" : "false", CPU_STAT_CPL));
		EXCEPTION(GP_EXCEPTION, 0);
	}
	CPU_CR0 &= ~CPU_CR0_TS;
}

void
ARPL_EwGw(void)
{
	UINT32 op, madr;
	UINT src, dst;

	if (CPU_STAT_PM && !CPU_STAT_VM86) {
		PREPART_EA_REG16(op, src);
		if (op >= 0xc0) {
			CPU_WORKCLOCK(2);
			dst = *(reg16_b20[op]);
			if ((dst & 3) < (src & 3)) {
				CPU_FLAGL |= Z_FLAG;
				dst &= ~3;
				dst |= (src & 3);
				*(reg16_b20[op]) = (UINT16)dst;
			} else {
				CPU_FLAGL &= ~Z_FLAG;
			}
		} else {
			CPU_WORKCLOCK(3);
			madr = calc_ea_dst(op);
			dst = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, madr);
			if ((dst & 3) < (src & 3)) {
				CPU_FLAGL |= Z_FLAG;
				dst &= ~3;
				dst |= (src & 3);
				cpu_vmemorywrite_w(CPU_INST_SEGREG_INDEX, madr, (UINT16)dst);
			} else {
				CPU_FLAGL &= ~Z_FLAG;
			}
		}
		return;
	}
	VERBOSE(("ARPL: real-mode or VM86"));
	EXCEPTION(UD_EXCEPTION, 0);
}

/*
 * DPL
 */
void
LAR_GwEw(void)
{
	selector_t sel;
	UINT16 *out;
	UINT32 op;
	UINT32 h;
	int rv;
	UINT16 selector;

	if (CPU_STAT_PM && !CPU_STAT_VM86) {
		PREPART_REG16_EA(op, selector, out, 5, 11);

		rv = parse_selector(&sel, selector);
		if (rv < 0) {
			CPU_FLAGL &= ~Z_FLAG;
			return;
		}

		if (!SEG_IS_SYSTEM(&sel.desc)) {
			/* code or data segment */
			if ((SEG_IS_DATA(&sel.desc)
			 || !SEG_IS_CONFORMING_CODE(&sel.desc))) {
				/* data or non-conforming code segment */
				if ((sel.desc.dpl < CPU_STAT_CPL)
				 || (sel.desc.dpl < sel.rpl)) {
					CPU_FLAGL &= ~Z_FLAG;
					return;
				}
			}
		} else {
			/* system segment */
			switch (sel.desc.type) {
			case CPU_SYSDESC_TYPE_TSS_16:
			case CPU_SYSDESC_TYPE_LDT:
			case CPU_SYSDESC_TYPE_TSS_BUSY_16:
			case CPU_SYSDESC_TYPE_CALL_16:
			case CPU_SYSDESC_TYPE_TASK:
			case CPU_SYSDESC_TYPE_TSS_32:
			case CPU_SYSDESC_TYPE_TSS_BUSY_32:
			case CPU_SYSDESC_TYPE_CALL_32:
				break;

			default:
				CPU_FLAGL &= ~Z_FLAG;
				return;
			}
		}

		h = cpu_kmemoryread_d(sel.addr + 4);
		*out = (UINT16)(h & 0xff00);
		CPU_FLAGL |= Z_FLAG;
		return;
	}
	VERBOSE(("LAR: real-mode or VM86"));
	EXCEPTION(UD_EXCEPTION, 0);
}

void
LAR_GdEw(void)
{
	selector_t sel;
	UINT32 *out;
	UINT32 op;
	UINT32 h;
	int rv;
	UINT32 selector;

	if (CPU_STAT_PM && !CPU_STAT_VM86) {
		PREPART_REG32_EA(op, selector, out, 5, 11);

		rv = parse_selector(&sel, (UINT16)selector);
		if (rv < 0) {
			CPU_FLAGL &= ~Z_FLAG;
			return;
		}

		if (!SEG_IS_SYSTEM(&sel.desc)) {
			/* code or data segment */
			if ((SEG_IS_DATA(&sel.desc)
			 || !SEG_IS_CONFORMING_CODE(&sel.desc))) {
				/* data or non-conforming code segment */
				if ((sel.desc.dpl < CPU_STAT_CPL)
				 || (sel.desc.dpl < sel.rpl)) {
					CPU_FLAGL &= ~Z_FLAG;
					return;
				}
			}
		} else {
			/* system segment */
			switch (sel.desc.type) {
			case CPU_SYSDESC_TYPE_TSS_16:
			case CPU_SYSDESC_TYPE_LDT:
			case CPU_SYSDESC_TYPE_TSS_BUSY_16:
			case CPU_SYSDESC_TYPE_CALL_16:
			case CPU_SYSDESC_TYPE_TASK:
			case CPU_SYSDESC_TYPE_TSS_32:
			case CPU_SYSDESC_TYPE_TSS_BUSY_32:
			case CPU_SYSDESC_TYPE_CALL_32:
				break;

			default:
				CPU_FLAGL &= ~Z_FLAG;
				return;
			}
		}

		h = cpu_kmemoryread_d(sel.addr + 4);
		*out = h & 0x00ffff00;	/* 0x00fxff00, x? */
		CPU_FLAGL |= Z_FLAG;
		return;
	}
	VERBOSE(("LAR: real-mode or VM86"));
	EXCEPTION(UD_EXCEPTION, 0);
}

void
LSL_GwEw(void)
{
	selector_t sel;
	UINT16 *out;
	UINT32 op;
	int rv;
	UINT16 selector;

	if (CPU_STAT_PM && !CPU_STAT_VM86) {
		PREPART_REG16_EA(op, selector, out, 5, 11);

		rv = parse_selector(&sel, selector);
		if (rv < 0) {
			CPU_FLAGL &= ~Z_FLAG;
			return;
		}

		if (!SEG_IS_SYSTEM(&sel.desc)) {
			/* code or data segment */
			if ((SEG_IS_DATA(&sel.desc)
			 || !SEG_IS_CONFORMING_CODE(&sel.desc))) {
				/* data or non-conforming code segment */
				if ((sel.desc.dpl < CPU_STAT_CPL)
				 || (sel.desc.dpl < sel.rpl)) {
					CPU_FLAGL &= ~Z_FLAG;
					return;
				}
			}
		} else {
			/* system segment */
			switch (sel.desc.type) {
			case CPU_SYSDESC_TYPE_TSS_16:
			case CPU_SYSDESC_TYPE_LDT:
			case CPU_SYSDESC_TYPE_TSS_BUSY_16:
			case CPU_SYSDESC_TYPE_TSS_32:
			case CPU_SYSDESC_TYPE_TSS_BUSY_32:
				break;

			default:
				CPU_FLAGL &= ~Z_FLAG;
				return;
			}
		}

		*out = (UINT16)sel.desc.u.seg.limit;
		CPU_FLAGL |= Z_FLAG;
		return;
	}
	VERBOSE(("LSL: real-mode or VM86"));
	EXCEPTION(UD_EXCEPTION, 0);
}

void
LSL_GdEw(void)
{
	selector_t sel;
	UINT32 *out;
	UINT32 op;
	int rv;
	UINT32 selector;

	if (CPU_STAT_PM && !CPU_STAT_VM86) {
		PREPART_REG32_EA(op, selector, out, 5, 11);

		rv = parse_selector(&sel, (UINT16)selector);
		if (rv < 0) {
			CPU_FLAGL &= ~Z_FLAG;
			return;
		}

		if (!SEG_IS_SYSTEM(&sel.desc)) {
			/* code or data segment */
			if ((SEG_IS_DATA(&sel.desc)
			 || !SEG_IS_CONFORMING_CODE(&sel.desc))) {
				/* data or non-conforming code segment */
				if ((sel.desc.dpl < CPU_STAT_CPL)
				 || (sel.desc.dpl < sel.rpl)) {
					CPU_FLAGL &= ~Z_FLAG;
					return;
				}
			}
		} else {
			/* system segment */
			switch (sel.desc.type) {
			case CPU_SYSDESC_TYPE_TSS_16:
			case CPU_SYSDESC_TYPE_LDT:
			case CPU_SYSDESC_TYPE_TSS_BUSY_16:
			case CPU_SYSDESC_TYPE_TSS_32:
			case CPU_SYSDESC_TYPE_TSS_BUSY_32:
				break;

			default:
				CPU_FLAGL &= ~Z_FLAG;
				return;
			}
		}

		*out = sel.desc.u.seg.limit;
		CPU_FLAGL |= Z_FLAG;
		return;
	}
	VERBOSE(("LSL: real-mode or VM86"));
	EXCEPTION(UD_EXCEPTION, 0);
}

void CPUCALL
VERR_Ew(UINT32 op)
{
	selector_t sel;
	UINT32 madr;
	int rv;
	UINT16 selector;

	if (CPU_STAT_PM && !CPU_STAT_VM86) {
		if (op >= 0xc0) {
			CPU_WORKCLOCK(5);
			selector = *(reg16_b20[op]);
		} else {
			CPU_WORKCLOCK(11);
			madr = calc_ea_dst(op);
			selector = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, madr);
		}

		rv = parse_selector(&sel, selector);
		if (rv < 0) {
			CPU_FLAGL &= ~Z_FLAG;
			return;
		}

		/* system segment */
		if (SEG_IS_SYSTEM(&sel.desc)) {
			CPU_FLAGL &= ~Z_FLAG;
			return;
		}

		/* data or non-conforming code segment */
		if ((SEG_IS_DATA(&sel.desc)
		 || !SEG_IS_CONFORMING_CODE(&sel.desc))) {
			if ((sel.desc.dpl < CPU_STAT_CPL)
			 || (sel.desc.dpl < sel.rpl)) {
				CPU_FLAGL &= ~Z_FLAG;
				return;
			}
		}
		/* code segment is not readable */
		if (SEG_IS_CODE(&sel.desc)
		 && !SEG_IS_READABLE_CODE(&sel.desc)) {
			CPU_FLAGL &= ~Z_FLAG;
			return;
		}

		CPU_FLAGL |= Z_FLAG;
		return;
	}
	VERBOSE(("VERR: real-mode or VM86"));
	EXCEPTION(UD_EXCEPTION, 0);
}

void CPUCALL
VERW_Ew(UINT32 op)
{
	selector_t sel;
	UINT32 madr;
	int rv;
	UINT16 selector;

	if (CPU_STAT_PM && !CPU_STAT_VM86) {
		if (op >= 0xc0) {
			CPU_WORKCLOCK(5);
			selector = *(reg16_b20[op]);
		} else {
			CPU_WORKCLOCK(11);
			madr = calc_ea_dst(op);
			selector = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, madr);
		}

		rv = parse_selector(&sel, selector);
		if (rv < 0) {
			CPU_FLAGL &= ~Z_FLAG;
			return;
		}

		/* system segment || code segment */
		if (SEG_IS_SYSTEM(&sel.desc) || SEG_IS_CODE(&sel.desc)) {
			CPU_FLAGL &= ~Z_FLAG;
			return;
		}
		/* data segment is not writable */
		if (!SEG_IS_WRITABLE_DATA(&sel.desc)) {
			CPU_FLAGL &= ~Z_FLAG;
			return;
		}
		/* privilege level */
		if ((CPU_STAT_CPL > sel.desc.dpl) || (sel.rpl > sel.desc.dpl)) {
			CPU_FLAGL &= ~Z_FLAG;
			return;
		}

		CPU_FLAGL |= Z_FLAG;
		return;
	}
	VERBOSE(("VERW: real-mode or VM86"));
	EXCEPTION(UD_EXCEPTION, 0);
}

void
MOV_DdRd(void)
{
	UINT32 src;
	UINT op;
	int idx;

	CPU_WORKCLOCK(11);
	GET_PCBYTE(op);
	if (op >= 0xc0) {
		if (CPU_STAT_PM && (CPU_STAT_VM86 || CPU_STAT_CPL != 0)) {
			VERBOSE(("MOV_DdRd: VM86(%s) or CPL(%d) != 0", CPU_STAT_VM86 ? "true" : "false", CPU_STAT_CPL));
			EXCEPTION(GP_EXCEPTION, 0);
		}

		if (CPU_DR7 & CPU_DR7_GD) {
			CPU_DR6 |= CPU_DR6_BD;
			CPU_DR7 &= ~CPU_DR7_GD;
			EXCEPTION(DB_EXCEPTION, 0);
		}

		src = *(reg32_b20[op]);
		idx = (op >> 3) & 7;

		CPU_DR(idx) = src;
		switch (idx) {
		case 0:
		case 1:
		case 2:
		case 3:
			CPU_DR(idx) = src;
			break;

		case 6:
			CPU_DR6 = src;
			break;

		case 7:
			CPU_DR7 = src;
			CPU_STAT_BP = 0;
			break;

		default:
			ia32_panic("MOV_DdRd: DR reg index (%d)", idx);
			/*NOTREACHED*/
			break;
		}

		return;
	}
	EXCEPTION(UD_EXCEPTION, 0);
}

void
MOV_RdDd(void)
{
	UINT32 *out;
	UINT op;
	int idx;

	CPU_WORKCLOCK(11);
	GET_PCBYTE(op);
	if (op >= 0xc0) {
		if (CPU_STAT_PM && (CPU_STAT_VM86 || CPU_STAT_CPL != 0)) {
			VERBOSE(("MOV_RdDd: VM86(%s) or CPL(%d) != 0", CPU_STAT_VM86 ? "true" : "false", CPU_STAT_CPL));
			EXCEPTION(GP_EXCEPTION, 0);
		}

		if (CPU_DR7 & CPU_DR7_GD) {
			CPU_DR6 |= CPU_DR6_BD;
			CPU_DR7 &= ~CPU_DR7_GD;
			EXCEPTION(DB_EXCEPTION, 0);
		}

		out = reg32_b20[op];
		idx = (op >> 3) & 7;

		switch (idx) {
		case 0:
		case 1:
		case 2:
		case 3:
			*out = CPU_DR(idx);
			break;

		case 4:
		case 6:
			*out = (CPU_DR6 & 0x0000f00f) | 0xffff0ff0;
			break;

		case 7:
			*out = CPU_DR7;
			break;

		default:
			ia32_panic("MOV_RdDd: DR reg index (%d)", idx);
			/*NOTREACHED*/
			break;
		}
		return;
	}
	EXCEPTION(UD_EXCEPTION, 0);
}

void
INVD(void)
{

	CPU_WORKCLOCK(11);
	if (CPU_STAT_PM && (CPU_STAT_VM86 || CPU_STAT_CPL != 0)) {
		VERBOSE(("INVD: VM86(%s) or CPL(%d) != 0", CPU_STAT_VM86 ? "true" : "false", CPU_STAT_CPL));
		EXCEPTION(GP_EXCEPTION, 0);
	}
}

void
WBINVD(void)
{

	CPU_WORKCLOCK(11);
	if (CPU_STAT_PM && (CPU_STAT_VM86 || CPU_STAT_CPL != 0)) {
		VERBOSE(("WBINVD: VM86(%s) or CPL(%d) != 0", CPU_STAT_VM86 ? "true" : "false", CPU_STAT_CPL));
		EXCEPTION(GP_EXCEPTION, 0);
	}
}

void CPUCALL
INVLPG(UINT32 op)
{
	descriptor_t *sdp;
	UINT32 madr;
	int idx;

	if (CPU_STAT_PM && (CPU_STAT_VM86 || CPU_STAT_CPL != 0)) {
		VERBOSE(("INVLPG: VM86(%s) or CPL(%d) != 0", CPU_STAT_VM86 ? "true" : "false", CPU_STAT_CPL));
		EXCEPTION(GP_EXCEPTION, 0);
	}

	if (op < 0xc0) {
		CPU_WORKCLOCK(11);
		madr = calc_ea_dst(op);

		idx = CPU_INST_SEGREG_INDEX;
		sdp = &CPU_STAT_SREG(idx);
		if (!SEG_IS_VALID(sdp)) {
			EXCEPTION(GP_EXCEPTION, 0);
		}
		switch (sdp->type) {
		case 4: case 5: case 6: case 7:
			if (madr <= sdp->u.seg.limit) {
				EXCEPTION((idx == CPU_SS_INDEX) ?
				    SS_EXCEPTION: GP_EXCEPTION, 0);
			}
			break;

		default:
			if (madr > sdp->u.seg.limit) {
				EXCEPTION((idx == CPU_SS_INDEX) ?
				    SS_EXCEPTION: GP_EXCEPTION, 0);
			}
			break;
		}
		tlb_flush_page(sdp->u.seg.segbase + madr);
		return;
	}
	EXCEPTION(UD_EXCEPTION, 0);
}

void
_LOCK(void)
{

	/* Nothing to do */
}

void
HLT(void)
{

	if (CPU_STAT_PM && CPU_STAT_CPL != 0) {
		VERBOSE(("HLT: CPL(%d) != 0", CPU_STAT_CPL));
		EXCEPTION(GP_EXCEPTION, 0);
	}

	VERBOSE(("HLT: do HLT."));
	CPU_HALT();
	CPU_EIP = CPU_PREV_EIP;
	CPU_STAT_HLT = 1;
}

void
RSM(void)
{

	ia32_panic("RSM: not implemented yet!");
}

void
RDMSR(void)
{
	int idx;

	if (CPU_STAT_PM && (CPU_STAT_VM86 || CPU_STAT_CPL != 0)) {
		VERBOSE(("RDMSR: VM86(%s) or CPL(%d) != 0", CPU_STAT_VM86 ? "true" : "false", CPU_STAT_CPL));
		EXCEPTION(GP_EXCEPTION, 0);
	}

	idx = CPU_ECX;
	switch (idx) {
	case 0x174:
		CPU_EDX = (UINT32)((i386msr.reg.ia32_sysenter_cs >> 32) & 0xffffffff);
		CPU_EAX = (UINT32)((i386msr.reg.ia32_sysenter_cs      ) & 0xffffffff);
		break;
	case 0x175:
		CPU_EDX = (UINT32)((i386msr.reg.ia32_sysenter_esp >> 32) & 0xffffffff);
		CPU_EAX = (UINT32)((i386msr.reg.ia32_sysenter_esp      ) & 0xffffffff);
		break;
	case 0x176:
		CPU_EDX = (UINT32)((i386msr.reg.ia32_sysenter_eip >> 32) & 0xffffffff);
		CPU_EAX = (UINT32)((i386msr.reg.ia32_sysenter_eip      ) & 0xffffffff);
		break;
	case 0x10:
		RDTSC();
		break;
	case 0x2c:
		CPU_EDX = 0x00000000;
		CPU_EAX = 0xfee00800;
		break;
	//case 0x1b:
	//	CPU_EDX = 0x00000000;
	//	CPU_EAX = 0x00000010;
	//	break;
	default:
		CPU_EDX = CPU_EAX = 0;
		//EXCEPTION(GP_EXCEPTION, 0); // XXX: とりあえず通す
		break;
	}
}

void
WRMSR(void)
{
	int idx;

	if (CPU_STAT_PM && (CPU_STAT_VM86 || CPU_STAT_CPL != 0)) {
		VERBOSE(("WRMSR: VM86(%s) or CPL(%d) != 0", CPU_STAT_VM86 ? "true" : "false", CPU_STAT_CPL));
		EXCEPTION(GP_EXCEPTION, 0);
	}

	idx = CPU_ECX;
	switch (idx) {
	case 0x174:
		i386msr.reg.ia32_sysenter_cs = ((UINT64)CPU_EDX << 32) | ((UINT64)CPU_EAX);
		break;
	case 0x175:
		i386msr.reg.ia32_sysenter_esp = ((UINT64)CPU_EDX << 32) | ((UINT64)CPU_EAX);
		break;
	case 0x176:
		i386msr.reg.ia32_sysenter_eip = ((UINT64)CPU_EDX << 32) | ((UINT64)CPU_EAX);
		break;
		/* MTRR への書き込み時 tlb_flush_all(); */
	default:
		//EXCEPTION(GP_EXCEPTION, 0); // XXX: とりあえず通す
		break;
	}
}

#if defined(SUPPORT_GAMEPORT)
int gameport_tsccounter = 0;
#endif
void
RDTSC(void)
{
#if defined(USE_TSC)
#if defined(NP2_X) || defined(NP2_SDL) || defined(__LIBRETRO__)
#if defined(SUPPORT_ASYNC_CPU)
	if(np2cfg.consttsc){
		// CPUクロックに依存しないカウンタ値にする
		UINT64 tsc_tmp;
		if(CPU_REMCLOCK != -1){
			tsc_tmp = CPU_MSR_TSC - CPU_REMCLOCK * pccore.maxmultiple / pccore.multiple;
		}else{
			tsc_tmp = CPU_MSR_TSC;
		}
		CPU_EDX = ((tsc_tmp >> 32) & 0xffffffff);
		CPU_EAX = (tsc_tmp & 0xffffffff);
	}else{
#endif
		// CPUクロックに依存するカウンタ値にする
		static UINT64 tsc_last = 0;
		static UINT64 tsc_cur = 0;
		UINT64 tsc_tmp;
		if(CPU_REMCLOCK != -1){
			tsc_tmp = CPU_MSR_TSC - CPU_REMCLOCK * pccore.maxmultiple / pccore.multiple;
		}else{
			tsc_tmp = CPU_MSR_TSC;
		}
		tsc_cur += (tsc_tmp - tsc_last) * pccore.multiple / pccore.maxmultiple;
		tsc_last = tsc_tmp;
		CPU_EDX = ((tsc_cur >> 32) & 0xffffffff);
		CPU_EAX = (tsc_cur & 0xffffffff);
#if defined(SUPPORT_ASYNC_CPU)
	}
#endif
#else
#if defined(SUPPORT_IA32_HAXM)
	LARGE_INTEGER li = {0};
	LARGE_INTEGER qpf;
	QueryPerformanceCounter(&li);
	if (QueryPerformanceFrequency(&qpf)) {
		li.QuadPart = li.QuadPart * pccore.realclock / qpf.QuadPart;
	}
	CPU_EDX = li.HighPart;
	CPU_EAX = li.LowPart;
#endif
#endif
#else
#if defined(SUPPORT_ASYNC_CPU)
	if(np2cfg.consttsc){
		// CPUクロックに依存しないカウンタ値にする
		UINT64 tsc_tmp;
		if(CPU_REMCLOCK != -1){
			tsc_tmp = CPU_MSR_TSC - CPU_REMCLOCK * pccore.maxmultiple / pccore.multiple;
		}else{
			tsc_tmp = CPU_MSR_TSC;
		}
		CPU_EDX = ((tsc_tmp >> 32) & 0xffffffff);
		CPU_EAX = (tsc_tmp & 0xffffffff);
	}else{
#endif
		// CPUクロックに依存するカウンタ値にする
		static UINT64 tsc_last = 0;
		static UINT64 tsc_cur = 0;
		UINT64 tsc_tmp;
		if(CPU_REMCLOCK != -1){
			tsc_tmp = CPU_MSR_TSC - CPU_REMCLOCK * pccore.maxmultiple / pccore.multiple;
		}else{
			tsc_tmp = CPU_MSR_TSC;
		}
		tsc_cur += (tsc_tmp - tsc_last) * pccore.multiple / pccore.maxmultiple;
		tsc_last = tsc_tmp;
		CPU_EDX = ((tsc_cur >> 32) & 0xffffffff);
		CPU_EAX = (tsc_cur & 0xffffffff);
#if defined(SUPPORT_ASYNC_CPU)
	}
#endif
#if defined(SUPPORT_GAMEPORT)
	if(gameport_tsccounter < INT_MAX) gameport_tsccounter++;
#endif
#endif
//	ia32_panic("RDTSC: not implemented yet!");
}

void
RDPMC(void)
{
	int idx;

	if(!(CPU_CR4 & CPU_CR4_PCE)){
		if (CPU_STAT_PM && (CPU_STAT_VM86 || CPU_STAT_CPL != 0)) {
			VERBOSE(("RDPMC: VM86(%s) or CPL(%d) != 0", CPU_STAT_VM86 ? "true" : "false", CPU_STAT_CPL));
			EXCEPTION(GP_EXCEPTION, 0);
		}
	}

	idx = CPU_ECX;
	switch (idx) {
	default:
		CPU_EDX = CPU_EAX = 0;
	}
}

void
MOV_TdRd(void)
{

	ia32_panic("MOV_TdRd: not implemented yet!");
}

void
MOV_RdTd(void)
{

	ia32_panic("MOV_RdTd: not implemented yet!");
}

// 中途半端＆ノーチェック注意
void
SYSENTER(void)
{
	// SEPなしならUD(無効オペコード例外)を発生させる
	if(!(i386cpuid.cpu_feature & CPU_FEATURE_SEP)){
		EXCEPTION(UD_EXCEPTION, 0);
	}
	// プロテクトモードチェック
	if (!CPU_STAT_PM) {
		EXCEPTION(GP_EXCEPTION, 0);
	}
	// MSRレジスタチェック
	if (i386msr.reg.ia32_sysenter_cs == 0) {
		EXCEPTION(GP_EXCEPTION, 0);
	}

	CPU_EFLAG = CPU_EFLAG & ~(VM_FLAG|I_FLAG|RF_FLAG);
	CPU_CS = (UINT32)i386msr.reg.ia32_sysenter_cs;

	CPU_SS = CPU_CS + 8;
	
	CPU_ESP = (UINT32)i386msr.reg.ia32_sysenter_esp;
	CPU_EIP = (UINT32)i386msr.reg.ia32_sysenter_eip;

	CPU_STAT_CPL = 0;
	CPU_STAT_USER_MODE = (CPU_STAT_CPL == 3) ? CPU_MODE_USER : CPU_MODE_SUPERVISER;
}

// 中途半端＆ノーチェック注意
void
SYSEXIT(void)
{
	// SEPなしならUD(無効オペコード例外)を発生させる
	if(!(i386cpuid.cpu_feature & CPU_FEATURE_SEP)){
		EXCEPTION(UD_EXCEPTION, 0);
	}
	// プロテクトモードチェック
	if (!CPU_STAT_PM) {
		EXCEPTION(GP_EXCEPTION, 0);
	}
	// MSRレジスタチェック
	if (i386msr.reg.ia32_sysenter_cs == 0) {
		EXCEPTION(GP_EXCEPTION, 0);
	}
	// 特権レベルチェック
	if (CPU_STAT_CPL != 0) {
		VERBOSE(("SYSENTER: CPL(%d) != 0", CPU_STAT_CPL));
		EXCEPTION(GP_EXCEPTION, 0);
	}

	CPU_CS = (UINT32)i386msr.reg.ia32_sysenter_cs + 16;

	CPU_SS = (UINT32)i386msr.reg.ia32_sysenter_cs + 24;
	
	CPU_ESP = CPU_ECX;
	CPU_EIP = CPU_EDX;

	CPU_STAT_CPL = 3;
	CPU_STAT_USER_MODE = (CPU_STAT_CPL == 3) ? CPU_MODE_USER : CPU_MODE_SUPERVISER;
}
