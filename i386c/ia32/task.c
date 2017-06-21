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

#define	TSS_16_SIZE	44
#define	TSS_16_LIMIT	(TSS_16_SIZE - 1)
#define	TSS_32_SIZE	104
#define	TSS_32_LIMIT	(TSS_32_SIZE - 1)

static void CPUCALL
set_task_busy(UINT16 selector)
{
	UINT32 addr;
	UINT32 h;

	addr = CPU_GDTR_BASE + (selector & CPU_SEGMENT_SELECTOR_INDEX_MASK);
	h = cpu_kmemoryread_d(addr + 4);
	if (!(h & CPU_TSS_H_BUSY)) {
		h |= CPU_TSS_H_BUSY;
		cpu_kmemorywrite_d(addr + 4, h);
	} else {
		ia32_panic("set_task_busy: already busy(%04x:%08x)",
		    selector, h);
	}
}

static void CPUCALL
set_task_free(UINT16 selector)
{
	UINT32 addr;
	UINT32 h;

	addr = CPU_GDTR_BASE + (selector & CPU_SEGMENT_SELECTOR_INDEX_MASK);
	h = cpu_kmemoryread_d(addr + 4);
	if (h & CPU_TSS_H_BUSY) {
		h &= ~CPU_TSS_H_BUSY;
		cpu_kmemorywrite_d(addr + 4, h);
	} else {
		ia32_panic("set_task_free: already free(%04x:%08x)",
		    selector, h);
	}
}

void CPUCALL
load_tr(UINT16 selector)
{
	selector_t task_sel;
	int rv;
	UINT16 iobase;

	rv = parse_selector(&task_sel, selector);
	if (rv < 0 || task_sel.ldt || !SEG_IS_SYSTEM(&task_sel.desc)) {
		EXCEPTION(GP_EXCEPTION, task_sel.idx);
	}

	/* check descriptor type & stack room size */
	switch (task_sel.desc.type) {
	case CPU_SYSDESC_TYPE_TSS_16:
		if (task_sel.desc.u.seg.limit < TSS_16_LIMIT) {
			EXCEPTION(TS_EXCEPTION, task_sel.idx);
		}
		iobase = 0;
		break;

	case CPU_SYSDESC_TYPE_TSS_32:
		if (task_sel.desc.u.seg.limit < TSS_32_LIMIT) {
			EXCEPTION(TS_EXCEPTION, task_sel.idx);
		}
		iobase = cpu_kmemoryread_w(task_sel.desc.u.seg.segbase + 102);
		break;

	default:
		EXCEPTION(GP_EXCEPTION, task_sel.idx);
		return;
	}

	/* not present */
	rv = selector_is_not_present(&task_sel);
	if (rv < 0) {
		EXCEPTION(NP_EXCEPTION, task_sel.idx);
	}

#if defined(MORE_DEBUG)
	tr_dump(task_sel.selector, task_sel.desc.u.seg.segbase, task_sel.desc.u.seg.limit);
#endif

	set_task_busy(task_sel.selector);
	CPU_TR = task_sel.selector;
	CPU_TR_DESC = task_sel.desc;
	CPU_TR_DESC.type |= CPU_SYSDESC_TYPE_TSS_BUSY_IND;

	/* I/O deny bitmap */
	CPU_STAT_IOLIMIT = 0;
	if (CPU_TR_DESC.type == CPU_SYSDESC_TYPE_TSS_BUSY_32) {
		if (iobase < CPU_TR_LIMIT) {
			CPU_STAT_IOLIMIT = (UINT16)(CPU_TR_LIMIT - iobase);
			CPU_STAT_IOADDR = CPU_TR_BASE + iobase;
			VERBOSE(("load_tr: enable ioport control: iobase=0x%04x, base=0x%08x, limit=0x%08x", iobase, CPU_STAT_IOADDR, CPU_STAT_IOLIMIT));
		}
	}
	if (CPU_STAT_IOLIMIT == 0) {
		VERBOSE(("load_tr: disable ioport control."));
	}
}

void CPUCALL
get_stack_pointer_from_tss(UINT pl, UINT16 *new_ss, UINT32 *new_esp)
{
	UINT32 tss_stack_addr;

	VERBOSE(("get_stack_pointer_from_tss: pl = %d", pl));
	VERBOSE(("get_stack_pointer_from_tss: CPU_TR type = %d, base = 0x%08x, limit = 0x%08x", CPU_TR_DESC.type, CPU_TR_BASE, CPU_TR_LIMIT));

	__ASSERT(pl < 3);

	if (CPU_TR_DESC.type == CPU_SYSDESC_TYPE_TSS_BUSY_32) {
		tss_stack_addr = pl * 8 + 4;
		if (tss_stack_addr + 7 > CPU_TR_LIMIT) {
			EXCEPTION(TS_EXCEPTION, CPU_TR & ~3);
		}
		tss_stack_addr += CPU_TR_BASE;
		*new_esp = cpu_kmemoryread_d(tss_stack_addr);
		*new_ss = cpu_kmemoryread_w(tss_stack_addr + 4);
	} else if (CPU_TR_DESC.type == CPU_SYSDESC_TYPE_TSS_BUSY_16) {
		tss_stack_addr = pl * 4 + 2;
		if (tss_stack_addr + 3 > CPU_TR_LIMIT) {
			EXCEPTION(TS_EXCEPTION, CPU_TR & ~3);
		}
		tss_stack_addr += CPU_TR_BASE;
		*new_esp = cpu_kmemoryread_w(tss_stack_addr);
		*new_ss = cpu_kmemoryread_w(tss_stack_addr + 2);
	} else {
		ia32_panic("get_stack_pointer_from_tss: task register is invalid (%d)\n", CPU_TR_DESC.type);
	}
	VERBOSE(("get_stack_pointer_from_tss: new stack pointer = %04x:%08x",
	    *new_ss, *new_esp));
}

UINT16
get_backlink_selector_from_tss(void)
{
	UINT16 backlink;

	if (CPU_TR_DESC.type == CPU_SYSDESC_TYPE_TSS_BUSY_32) {
		if (4 > CPU_TR_LIMIT) {
			EXCEPTION(TS_EXCEPTION, CPU_TR & ~3);
		}
	} else if (CPU_TR_DESC.type == CPU_SYSDESC_TYPE_TSS_BUSY_16) {
		if (2 > CPU_TR_LIMIT) {
			EXCEPTION(TS_EXCEPTION, CPU_TR & ~3);
		}
	} else {
		ia32_panic("get_backlink_selector_from_tss: task register has invalid type (%d)\n", CPU_TR_DESC.type);
	}

	backlink = cpu_kmemoryread_w(CPU_TR_BASE);
	VERBOSE(("get_backlink_selector_from_tss: backlink selector = 0x%04x",
	    backlink));
	return backlink;
}

void CPUCALL
task_switch(selector_t *task_sel, task_switch_type_t type)
{
	UINT32 regs[CPU_REG_NUM];
	UINT32 eip;
	UINT32 new_flags;
	UINT32 cr3 = 0;
	UINT16 sreg[CPU_SEGREG_NUM];
	UINT16 ldtr;
	UINT16 iobase;
	UINT16 t;

	selector_t cs_sel, ss_sel;
	int rv;

	UINT32 cur_base, cur_paddr;	/* current task state */
	UINT32 task_base, task_paddr;	/* new task state */
	UINT32 old_flags = REAL_EFLAGREG;
	BOOL task16;
	UINT i;

	VERBOSE(("task_switch: start"));

	switch (task_sel->desc.type) {
	case CPU_SYSDESC_TYPE_TSS_32:
	case CPU_SYSDESC_TYPE_TSS_BUSY_32:
		if (task_sel->desc.u.seg.limit < TSS_32_LIMIT) {
			EXCEPTION(TS_EXCEPTION, task_sel->idx);
		}
		task16 = 0;
		break;

	case CPU_SYSDESC_TYPE_TSS_16:
	case CPU_SYSDESC_TYPE_TSS_BUSY_16:
		if (task_sel->desc.u.seg.limit < TSS_16_LIMIT) {
			EXCEPTION(TS_EXCEPTION, task_sel->idx);
		}
		task16 = 1;
		break;

	default:
		ia32_panic("task_switch: descriptor type is invalid.");
		task16 = 0;		/* compiler happy */
		break;
	}

	cur_base = CPU_TR_BASE;
	cur_paddr = laddr_to_paddr(cur_base, CPU_PAGE_WRITE_DATA|CPU_MODE_SUPERVISER);
	task_base = task_sel->desc.u.seg.segbase;
	task_paddr = laddr_to_paddr(task_base, CPU_PAGE_WRITE_DATA|CPU_MODE_SUPERVISER);
	VERBOSE(("task_switch: current task (%04x) = 0x%08x:%08x (p0x%08x)",
	    CPU_TR, cur_base, CPU_TR_LIMIT, cur_paddr));
	VERBOSE(("task_switch: new task (%04x) = 0x%08x:%08x (p0x%08x)",
	    task_sel->selector, task_base, task_sel->desc.u.seg.limit,
	    task_paddr));
	VERBOSE(("task_switch: %dbit task switch", task16 ? 16 : 32));

#if defined(MORE_DEBUG)
	VERBOSE(("task_switch: new task"));
	for (i = 0; i < task_sel->desc.u.seg.limit; i += 4) {
		VERBOSE(("task_switch: 0x%08x: %08x", task_base + i,
		    cpu_memoryread_d(task_paddr + i)));
	}
#endif

	/* load task state */
	if (!task16) {
		if (CPU_STAT_PAGING) {
			cr3 = cpu_memoryread_d(task_paddr + 28);
		}
		eip = cpu_memoryread_d(task_paddr + 32);
		new_flags = cpu_memoryread_d(task_paddr + 36);
		for (i = 0; i < CPU_REG_NUM; i++) {
			regs[i] = cpu_memoryread_d(task_paddr + 40 + i * 4);
		}
		for (i = 0; i < CPU_SEGREG_NUM; i++) {
			sreg[i] = cpu_memoryread_w(task_paddr + 72 + i * 4);
		}
		ldtr = cpu_memoryread_w(task_paddr + 96);
		t = cpu_memoryread_w(task_paddr + 100);
		if (t & 1) {
			CPU_STAT_BP_EVENT |= CPU_STAT_BP_EVENT_TASK;
		}
		iobase = cpu_memoryread_w(task_paddr + 102);
	} else {
		eip = cpu_memoryread_w(task_paddr + 14);
		new_flags = cpu_memoryread_w(task_paddr + 16);
		for (i = 0; i < CPU_REG_NUM; i++) {
			regs[i] = cpu_memoryread_w(task_paddr + 18 + i * 2);
		}
		for (i = 0; i < CPU_SEGREG286_NUM; i++) {
			sreg[i] = cpu_memoryread_w(task_paddr + 34 + i * 2);
		}
		for (; i < CPU_SEGREG_NUM; i++) {
			sreg[i] = 0;
		}
		ldtr = cpu_memoryread_w(task_paddr + 42);
		iobase = 0;
		t = 0;
	}

#if defined(DEBUG)
	VERBOSE(("task_switch: current task"));
	if (!task16) {
		VERBOSE(("task_switch: CR3     = 0x%08x", CPU_CR3));
	}
	VERBOSE(("task_switch: eip     = 0x%08x", CPU_EIP));
	VERBOSE(("task_switch: eflags  = 0x%08x", old_flags));
	for (i = 0; i < CPU_REG_NUM; i++) {
		VERBOSE(("task_switch: %s = 0x%08x", reg32_str[i],
		    CPU_REGS_DWORD(i)));
	}
	for (i = 0; i < CPU_SEGREG_NUM; i++) {
		VERBOSE(("task_switch: %s = 0x%04x", sreg_str[i],
		    CPU_REGS_SREG(i)));
	}
	VERBOSE(("task_switch: ldtr    = 0x%04x", CPU_LDTR));

	VERBOSE(("task_switch: new task"));
	if (!task16) {
		VERBOSE(("task_switch: CR3     = 0x%08x", cr3));
	}
	VERBOSE(("task_switch: eip     = 0x%08x", eip));
	VERBOSE(("task_switch: eflags  = 0x%08x", new_flags));
	for (i = 0; i < CPU_REG_NUM; i++) {
		VERBOSE(("task_switch: %s = 0x%08x", reg32_str[i], regs[i]));
	}
	for (i = 0; i < CPU_SEGREG_NUM; i++) {
		VERBOSE(("task_switch: %s = 0x%04x", sreg_str[i], sreg[i]));
	}
	VERBOSE(("task_switch: ldtr    = 0x%04x", ldtr));
	if (!task16) {
		VERBOSE(("task_switch: t       = 0x%04x", t));
		VERBOSE(("task_switch: iobase  = 0x%04x", iobase));
	}
#endif

	/* if IRET or JMP, clear busy flag in this task: need */
	/* if IRET, clear NT_FLAG in current EFLAG: need */
	switch (type) {
	case TASK_SWITCH_IRET:
		/* clear NT_FLAG */
		old_flags &= ~NT_FLAG;
		/*FALLTHROUGH*/
	case TASK_SWITCH_JMP:
		/* clear busy flags in current task */
		set_task_free(CPU_TR);
		break;

	case TASK_SWITCH_CALL:
	case TASK_SWITCH_INTR:
		/* Nothing to do */
		break;
	
	default:
		ia32_panic("task_switch: task switch type is invalid");
		break;
	}

	/* store current task state in current TSS */
	if (!task16) {
		cpu_memorywrite_d(cur_paddr + 32, CPU_EIP);
		cpu_memorywrite_d(cur_paddr + 36, old_flags);
		for (i = 0; i < CPU_REG_NUM; i++) {
			cpu_memorywrite_d(cur_paddr + 40 + i * 4,
			    CPU_REGS_DWORD(i));
		}
		for (i = 0; i < CPU_SEGREG_NUM; i++) {
			cpu_memorywrite_w(cur_paddr + 72 + i * 4,
			    CPU_REGS_SREG(i));
		}
	} else {
		cpu_memorywrite_w(cur_paddr + 14, CPU_IP);
		cpu_memorywrite_w(cur_paddr + 16, (UINT16)old_flags);
		for (i = 0; i < CPU_REG_NUM; i++) {
			cpu_memorywrite_w(cur_paddr + 18 + i * 2,
			    CPU_REGS_WORD(i));
		}
		for (i = 0; i < CPU_SEGREG286_NUM; i++) {
			cpu_memorywrite_w(cur_paddr + 34 + i * 2,
			    CPU_REGS_SREG(i));
		}
	}

	/* set back link selector */
	switch (type) {
	case TASK_SWITCH_CALL:
	case TASK_SWITCH_INTR:
		/* set back link selector */
		cpu_memorywrite_w(task_paddr, CPU_TR);
		break;

	case TASK_SWITCH_IRET:
	case TASK_SWITCH_JMP:
		/* Nothing to do */
		break;

	default:
		ia32_panic("task_switch: task switch type is invalid");
		break;
	}

#if defined(MORE_DEBUG)
	VERBOSE(("task_switch: current task"));
	for (i = 0; i < CPU_TR_LIMIT; i += 4) {
		VERBOSE(("task_switch: 0x%08x: %08x", cur_base + i,
		    cpu_memoryread_d(cur_paddr + i)));
	}
#endif

	/* Now task switching! */

	/* if CALL, INTR, set EFLAGS image NT_FLAG */
	/* if CALL, INTR, JMP set busy flag */
	switch (type) {
	case TASK_SWITCH_CALL:
	case TASK_SWITCH_INTR:
		/* set back link selector */
		new_flags |= NT_FLAG;
		/*FALLTHROUGH*/
	case TASK_SWITCH_JMP:
		set_task_busy(task_sel->selector);
		break;

	case TASK_SWITCH_IRET:
		/* check busy flag is active */
		if (SEG_IS_VALID(&task_sel->desc)) {
			UINT32 h;
			h = cpu_kmemoryread_d(task_sel->addr + 4);
			if ((h & CPU_TSS_H_BUSY) == 0) {
				ia32_panic("task_switch: new task is not busy");
			}
		}
		break;

	default:
		ia32_panic("task_switch: task switch type is invalid");
		break;
	}

	/* load task selector to CPU_TR */
	CPU_TR = task_sel->selector;
	CPU_TR_DESC = task_sel->desc;
	CPU_TR_DESC.type |= CPU_SYSDESC_TYPE_TSS_BUSY_IND;

	/* set CR0 image CPU_CR0_TS */
	CPU_CR0 |= CPU_CR0_TS;

	/*
	 * load task state (CR3, EIP, GPR, segregs, LDTR, EFLAGS)
	 */

	/* set new CR3 */
	if (!task16 && CPU_STAT_PAGING) {
		set_cr3(cr3);
	}

	/* set new EIP, GPR, segregs */
	CPU_EIP = eip;
	for (i = 0; i < CPU_REG_NUM; i++) {
		CPU_REGS_DWORD(i) = regs[i];
	}
	for (i = 0; i < CPU_SEGREG_NUM; i++) {
		segdesc_init(i, sreg[i], &CPU_STAT_SREG(i));
		/* invalidate segreg descriptor */
		CPU_STAT_SREG(i).valid = 0;
	}

	CPU_CLEAR_PREV_ESP();

	/* load new LDTR */
	CPU_LDTR_DESC.valid = 0;
	load_ldtr(ldtr, TS_EXCEPTION);

	/* I/O deny bitmap */
	CPU_STAT_IOLIMIT = 0;
	if (!task16 && iobase != 0 && iobase < CPU_TR_DESC.u.seg.limit) {
		CPU_STAT_IOLIMIT = (UINT16)(CPU_TR_DESC.u.seg.limit - iobase);
		CPU_STAT_IOADDR = task_base + iobase;
	}
	VERBOSE(("task_switch: ioaddr = %08x, limit = %08x", CPU_STAT_IOADDR,
	    CPU_STAT_IOLIMIT));

	/* set new EFLAGS */
	set_eflags(new_flags, I_FLAG|IOPL_FLAG|RF_FLAG|VM_FLAG|VIF_FLAG|VIP_FLAG);

	/* set new segment register */
	if (!CPU_STAT_VM86) {
		/* load CS */
		rv = parse_selector(&cs_sel, sreg[CPU_CS_INDEX]);
		if (rv < 0) {
			VERBOSE(("task_switch: load CS failure (sel = 0x%04x, rv = %d)", sreg[CPU_CS_INDEX], rv));
			EXCEPTION(TS_EXCEPTION, cs_sel.idx);
		}

		/* CS must be code segment */
		if (SEG_IS_SYSTEM(&cs_sel.desc) || SEG_IS_DATA(&cs_sel.desc)) {
			EXCEPTION(TS_EXCEPTION, cs_sel.idx);
		}

		/* check privilege level */
		if (!SEG_IS_CONFORMING_CODE(&cs_sel.desc)) {
			/* non-confirming code segment */
			if (cs_sel.desc.dpl != cs_sel.rpl) {
				EXCEPTION(TS_EXCEPTION, cs_sel.idx);
			}
		} else {
			/* conforming code segment */
			if (cs_sel.desc.dpl > cs_sel.rpl) {
				EXCEPTION(TS_EXCEPTION, cs_sel.idx);
			}
		}

		/* code segment is not present */
		rv = selector_is_not_present(&cs_sel);
		if (rv < 0) {
			EXCEPTION(NP_EXCEPTION, cs_sel.idx);
		}

		/* load SS */
		rv = parse_selector(&ss_sel, sreg[CPU_SS_INDEX]);
		if (rv < 0) {
			VERBOSE(("task_switch: load SS failure (sel = 0x%04x, rv = %d)", sreg[CPU_SS_INDEX], rv));
			EXCEPTION(TS_EXCEPTION, ss_sel.idx);
		}

		/* SS must be writable data segment */
		if (SEG_IS_SYSTEM(&ss_sel.desc)
		 || SEG_IS_CODE(&ss_sel.desc)
		 || !SEG_IS_WRITABLE_DATA(&ss_sel.desc)) {
			EXCEPTION(TS_EXCEPTION, ss_sel.idx);
		}

		/* check privilege level */
		if ((ss_sel.desc.dpl != cs_sel.rpl)
		 || (ss_sel.desc.dpl != ss_sel.rpl)) {
			EXCEPTION(TS_EXCEPTION, ss_sel.idx);
		}

		/* stack segment is not present */
		rv = selector_is_not_present(&ss_sel);
		if (rv < 0) {
			EXCEPTION(SS_EXCEPTION, ss_sel.idx);
		}

		/* Now loading SS register */
		load_ss(ss_sel.selector, &ss_sel.desc, cs_sel.rpl);

		/* load ES, DS, FS, GS segment register */
		LOAD_SEGREG1(CPU_ES_INDEX, sreg[CPU_ES_INDEX], TS_EXCEPTION);
		LOAD_SEGREG1(CPU_DS_INDEX, sreg[CPU_DS_INDEX], TS_EXCEPTION);
		LOAD_SEGREG1(CPU_FS_INDEX, sreg[CPU_FS_INDEX], TS_EXCEPTION);
		LOAD_SEGREG1(CPU_GS_INDEX, sreg[CPU_GS_INDEX], TS_EXCEPTION);

		/* Now loading CS register */
		load_cs(cs_sel.selector, &cs_sel.desc, cs_sel.rpl);
	}

	VERBOSE(("task_switch: done."));
}
