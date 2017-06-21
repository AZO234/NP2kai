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

static void CPUCALL segdesc_set_default(int, UINT16, descriptor_t *);

void CPUCALL
load_segreg(int idx, UINT16 selector, UINT16 *sregp, descriptor_t *sdp, int exc)
{
	selector_t sel;
	int rv;

	__ASSERT((unsigned int)idx < CPU_SEGREG_NUM);
	__ASSERT((sregp != NULL));
	__ASSERT((sdp != NULL));

	if (!CPU_STAT_PM || CPU_STAT_VM86) {
		/* real-mode or vm86 mode */
		*sregp = selector;
		segdesc_set_default(idx, selector, &sel.desc);
		*sdp = sel.desc;
		return;
	}

	VERBOSE(("load_segreg: EIP = %04x:%08x, idx = %d, selector = %04x, sregp = %p, dp = %p, exc = %d", CPU_CS, CPU_PREV_EIP, idx, selector, sregp, sdp, exc));

	/*
	 * protected mode
	 */
	if (idx == CPU_CS_INDEX) {
		ia32_panic("load_segreg: CS");
	}

	rv = parse_selector(&sel, selector);
	if (rv < 0) {
		if ((rv != -2) || (idx == CPU_SS_INDEX)) {
			EXCEPTION(exc, sel.idx);
		}
		/* null selector */
		*sregp = sel.selector;
		memset(sdp, 0, sizeof(*sdp));
		return;
	}

	switch (idx) {
	case CPU_SS_INDEX:
		if ((CPU_STAT_CPL != sel.rpl)
		 || (CPU_STAT_CPL != sel.desc.dpl)
		 || SEG_IS_SYSTEM(&sel.desc)
		 || SEG_IS_CODE(&sel.desc)
		 || !SEG_IS_WRITABLE_DATA(&sel.desc)) {
			EXCEPTION(exc, sel.idx);
		}

		/* not present */
		rv = selector_is_not_present(&sel);
		if (rv < 0) {
			EXCEPTION(SS_EXCEPTION, sel.idx);
		}

		load_ss(sel.selector, &sel.desc, CPU_STAT_CPL);
		break;

	case CPU_ES_INDEX:
	case CPU_DS_INDEX:
	case CPU_FS_INDEX:
	case CPU_GS_INDEX:
		if (SEG_IS_SYSTEM(&sel.desc)
		 || (SEG_IS_CODE(&sel.desc) && !SEG_IS_READABLE_CODE(&sel.desc))) {
			EXCEPTION(exc, sel.idx);
		}
		if (SEG_IS_DATA(&sel.desc)
		 || !SEG_IS_CONFORMING_CODE(&sel.desc)) {
			/* check privilege level */
			if ((sel.rpl > sel.desc.dpl)
			 || (CPU_STAT_CPL > sel.desc.dpl)) {
				EXCEPTION(exc, sel.idx);
			}
		}

		/* not present */
		rv = selector_is_not_present(&sel);
		if (rv < 0) {
			EXCEPTION(NP_EXCEPTION, sel.idx);
		}

		*sregp = sel.selector;
		*sdp = sel.desc;
		break;
	
	default:
		ia32_panic("load_segreg(): segment register index is invalid");
		break;
	}
}

/*
 * load SS register
 */
void CPUCALL
load_ss(UINT16 selector, const descriptor_t *sdp, int cpl)
{

	CPU_STAT_SS32 = sdp->d;
	CPU_SS = (UINT16)((selector & ~3) | (cpl & 3));
	CPU_SS_DESC = *sdp;
}

/*
 * load CS register
 */
void CPUCALL
load_cs(UINT16 selector, const descriptor_t *sdp, int new_cpl)
{
	int cpl = new_cpl & 3;

	CPU_INST_OP32 = CPU_INST_AS32 =
	    CPU_STATSAVE.cpu_inst_default.op_32 =
	    CPU_STATSAVE.cpu_inst_default.as_32 = sdp->d;
	CPU_CS = (UINT16)((selector & ~3) | cpl);
	CPU_CS_DESC = *sdp;
	set_cpl(cpl);
}

/*
 * load LDT register
 */
void CPUCALL
load_ldtr(UINT16 selector, int exc)
{
	selector_t sel;
	int rv;

	memset(&sel, 0, sizeof(sel));

	rv = parse_selector(&sel, selector);
	if (rv < 0 || sel.ldt) {
		if (rv == -2) {
			/* null segment */
			VERBOSE(("load_ldtr: null segment"));
			CPU_LDTR = 0;
			memset(&CPU_LDTR_DESC, 0, sizeof(CPU_LDTR_DESC));
			return;
		}
		EXCEPTION(exc, sel.selector);
	}

	/* check descriptor type */
	if (!SEG_IS_SYSTEM(&sel.desc)
	 || (sel.desc.type != CPU_SYSDESC_TYPE_LDT)) {
		EXCEPTION(exc, sel.selector);
	}

	/* not present */
	rv = selector_is_not_present(&sel);
	if (rv < 0) {
		EXCEPTION((exc == TS_EXCEPTION) ? TS_EXCEPTION : NP_EXCEPTION, sel.selector);
	}

#if defined(MORE_DEBUG)
	ldtr_dump(sel.desc.u.seg.segbase, sel.desc.u.seg.limit);
#endif

	CPU_LDTR = sel.selector;
	CPU_LDTR_DESC = sel.desc;
}

void CPUCALL
load_descriptor(descriptor_t *sdp, UINT32 addr)
{
	UINT32 l, h;

	__ASSERT(sdp != NULL);

	VERBOSE(("load_descriptor: address = 0x%08x", addr));

	l = cpu_kmemoryread_d(addr);
	h = cpu_kmemoryread_d(addr + 4);
	VERBOSE(("descriptor value = 0x%08x%08x", h, l));

	memset(sdp, 0, sizeof(*sdp));
	sdp->flag = 0;

	sdp->p = (h & CPU_DESC_H_P) ? 1 : 0;
	sdp->type = (UINT8)((h & CPU_DESC_H_TYPE) >> CPU_DESC_H_TYPE_SHIFT);
	sdp->dpl = (UINT8)((h & CPU_DESC_H_DPL) >> CPU_DESC_H_DPL_SHIFT);
	sdp->s = (h & CPU_DESC_H_S) ? 1 : 0;

	if (!SEG_IS_SYSTEM(sdp)) {
		/* code/data */
		sdp->valid = 1;
		sdp->d = (h & CPU_SEGDESC_H_D) ? 1 : 0;

		sdp->u.seg.c = (h & CPU_SEGDESC_H_D_C) ? 1 : 0;
		sdp->u.seg.g = (h & CPU_SEGDESC_H_G) ? 1 : 0;
		sdp->u.seg.wr = (sdp->type & CPU_SEGDESC_TYPE_WR) ? 1 : 0;
		sdp->u.seg.ec = (sdp->type & CPU_SEGDESC_TYPE_EC) ? 1 : 0;

		sdp->u.seg.segbase  = (l >> 16) & 0xffff;
		sdp->u.seg.segbase |= (h & 0xff) << 16;
		sdp->u.seg.segbase |= h & 0xff000000;
		sdp->u.seg.limit = (h & 0xf0000) | (l & 0xffff);
		if (sdp->u.seg.g) {
			sdp->u.seg.limit <<= 12;
			if (SEG_IS_CODE(sdp) || !SEG_IS_EXPANDDOWN_DATA(sdp)) {
				/* expand-up segment */
				sdp->u.seg.limit |= 0xfff;
			}
		}
	} else {
		/* system */
		switch (sdp->type) {
		case CPU_SYSDESC_TYPE_LDT:		/* LDT */
			sdp->valid = 1;
			sdp->u.seg.g = (h & CPU_SEGDESC_H_G) ? 1 : 0;

			sdp->u.seg.segbase  = h & 0xff000000;
			sdp->u.seg.segbase |= (h & 0xff) << 16;
			sdp->u.seg.segbase |= l >> 16;
			sdp->u.seg.limit  = h & 0xf0000;
			sdp->u.seg.limit |= l & 0xffff;
			if (sdp->u.seg.g) {
				sdp->u.seg.limit <<= 12;
				sdp->u.seg.limit |= 0xfff;
			}
			break;

		case CPU_SYSDESC_TYPE_TASK:		/* task gate */
			sdp->valid = 1;
			sdp->u.gate.selector = (UINT16)(l >> 16);
			break;

		case CPU_SYSDESC_TYPE_TSS_16:		/* 286 TSS */
		case CPU_SYSDESC_TYPE_TSS_BUSY_16:	/* 286 TSS Busy */
		case CPU_SYSDESC_TYPE_TSS_32:		/* 386 TSS */
		case CPU_SYSDESC_TYPE_TSS_BUSY_32:	/* 386 TSS Busy */
			sdp->valid = 1;
			sdp->d = (h & CPU_GATEDESC_H_D) ? 1 : 0;
			sdp->u.seg.g = (h & CPU_SEGDESC_H_G) ? 1 : 0;

			sdp->u.seg.segbase  = h & 0xff000000;
			sdp->u.seg.segbase |= (h & 0xff) << 16;
			sdp->u.seg.segbase |= l >> 16;
			sdp->u.seg.limit  = h & 0xf0000;
			sdp->u.seg.limit |= l & 0xffff;
			if (sdp->u.seg.g) {
				sdp->u.seg.limit <<= 12;
				sdp->u.seg.limit |= 0xfff;
			}
			break;

		case CPU_SYSDESC_TYPE_CALL_16:		/* 286 call gate */
		case CPU_SYSDESC_TYPE_INTR_16:		/* 286 interrupt gate */
		case CPU_SYSDESC_TYPE_TRAP_16:		/* 286 trap gate */
		case CPU_SYSDESC_TYPE_CALL_32:		/* 386 call gate */
		case CPU_SYSDESC_TYPE_INTR_32:		/* 386 interrupt gate */
		case CPU_SYSDESC_TYPE_TRAP_32:		/* 386 trap gate */
			if ((h & 0x0000000e0) == 0) {
				sdp->valid = 1;
				sdp->d = (h & CPU_GATEDESC_H_D) ? 1 : 0;
				sdp->u.gate.selector = (UINT16)(l >> 16);
				sdp->u.gate.offset  = h & 0xffff0000;
				sdp->u.gate.offset |= l & 0xffff;
				sdp->u.gate.count = (UINT8)(h & 0x1f);
			} else {
				sdp->valid = 0;
				VERBOSE(("load_descriptor: gate is invalid"));
			}
			break;

		case 0: case 8: case 10: case 13: /* reserved */
		default:
			sdp->valid = 0;
			break;
		}
	}
#if defined(DEBUG)
	segdesc_dump(sdp);
#endif
}

int CPUCALL
parse_selector(selector_t *ssp, UINT16 selector)
{
	UINT32 base;
	UINT limit;
	UINT idx;

	ssp->selector = selector;
	ssp->idx = selector & ~3;
	ssp->rpl = selector & 3;
	ssp->ldt = (UINT8)(selector & CPU_SEGMENT_TABLE_IND);

	VERBOSE(("parse_selector: selector = %04x, index = %d, RPL = %d, %cDT", ssp->selector, ssp->idx >> 3, ssp->rpl, ssp->ldt ? 'L' : 'G'));

	/* descriptor table */
	idx = selector & CPU_SEGMENT_SELECTOR_INDEX_MASK;
	if (ssp->ldt) {
		/* LDT */
		if (!SEG_IS_VALID(&CPU_LDTR_DESC)) {
			VERBOSE(("parse_selector: LDT is invalid"));
			return -1;
		}
		base = CPU_LDTR_BASE;
		limit = CPU_LDTR_LIMIT;
	} else {
		/* check null segment */
		if (idx == 0) {
			VERBOSE(("parse_selector: null segment"));
			return -2;
		}
		base = CPU_GDTR_BASE;
		limit = CPU_GDTR_LIMIT;
	}
	if (idx + 7 > limit) {
		VERBOSE(("parse_selector: segment limit check failed: 0x%08x > 0x%08x", idx + 7, limit));
		return -3;
	}

	/* load descriptor */
	ssp->addr = base + idx;
	load_descriptor(&ssp->desc, ssp->addr);
	if (!SEG_IS_VALID(&ssp->desc)) {
		VERBOSE(("parse_selector: segment descriptor is invalid"));
		return -4;
	}

	return 0;
}

int CPUCALL
selector_is_not_present(const selector_t *ssp)
{
	UINT32 h;

	/* not present */
	if (!SEG_IS_PRESENT(&ssp->desc)) {
		VERBOSE(("selector_is_not_present: not present"));
		return -1;
	}

	/* set access bit if code/data segment descriptor */
	if (!SEG_IS_SYSTEM(&ssp->desc)) {
		h = cpu_kmemoryread_d(ssp->addr + 4);
		if (!(h & CPU_SEGDESC_H_A)) {
			h |= CPU_SEGDESC_H_A;
			cpu_kmemorywrite_d(ssp->addr + 4, h);
		}
	}

	return 0;
}

void CPUCALL
segdesc_init(int idx, UINT16 sreg, descriptor_t *sdp)
{

	__ASSERT(((unsigned int)idx < CPU_SEGREG_NUM));
	__ASSERT((sdp != NULL));

	CPU_REGS_SREG(idx) = sreg;
	segdesc_set_default(idx, sreg, sdp);
}

static void CPUCALL
segdesc_set_default(int idx, UINT16 selector, descriptor_t *sdp)
{

	__ASSERT(((unsigned int)idx < CPU_SEGREG_NUM));
	__ASSERT((sdp != NULL));

	sdp->u.seg.segbase = (UINT32)selector << 4;
	sdp->u.seg.limit = 0xffff;
	sdp->u.seg.c = (idx == CPU_CS_INDEX) ? 1 : 0;	/* code or data */
	sdp->u.seg.g = 0;	/* non 4k factor scale */
	sdp->u.seg.wr = 1;	/* execute/read(CS) or read/write(others) */
	sdp->u.seg.ec = 0;	/* nonconforming(CS) or expand-up(others) */
	sdp->valid = 1;		/* valid */
	sdp->p = 1;		/* present */
	sdp->type = (CPU_SEGDESC_TYPE_WR << CPU_DESC_H_TYPE_SHIFT)
	            | ((idx == CPU_CS_INDEX) ? CPU_SEGDESC_H_D_C : 0);
				/* readable code/writable data segment */
	sdp->dpl = CPU_STAT_VM86 ? 3 : 0; /* descriptor privilege level */
	sdp->rpl = CPU_STAT_VM86 ? 3 : 0; /* request privilege level */
	sdp->s = 1;		/* code/data */
	sdp->d = 0;		/* 16bit */
	sdp->flag = CPU_DESC_FLAG_READABLE|CPU_DESC_FLAG_WRITABLE;
}
