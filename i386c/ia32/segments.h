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

#ifndef	IA32_CPU_SEGMENTS_H__
#define	IA32_CPU_SEGMENTS_H__

#ifdef __cplusplus
extern "C" {
#endif

/*
 * segment descriptor
 */
typedef struct {
	union {
		struct {
			UINT32	segbase;
			UINT32	d_pad;
			UINT32	limit;

			UINT8	c;	/* 0 = data, 1 = code */
			UINT8	g;	/* 4k base */
			UINT8	wr;	/* readable/writable */
			UINT8	ec;	/* expand down/conforming */
		} seg;

		struct {
			UINT16	selector;
			UINT16	w_pad;
			UINT32	offset;

			UINT8	count;		/* parameter count:call gate */

			UINT8	b_pad[7];
		} gate;
	} u;

	UINT8	valid;	/* descriptor valid flag */
	UINT8	p;	/* avail flag */

	UINT8	type;	/* descriptor type */
	UINT8	dpl;	/* DPL */
	UINT8	rpl;	/* RPL */
	UINT8	s;	/* 0 = system, 1 = code/data */
	UINT8	d;	/* 0 = 16bit, 1 = 32bit */

	UINT8	flag;
#define	CPU_DESC_FLAG_READABLE	(1 << 0)
#define	CPU_DESC_FLAG_WRITABLE	(1 << 1)
#define	CPU_DESC_FLAG_WHOLEADR	(1 << 2)
} descriptor_t;

#define	SEG_IS_VALID(sdp)		((sdp)->valid)
#define	SEG_IS_PRESENT(sdp)		((sdp)->p)
#define	SEG_IS_32BIT(sdp)		((sdp)->d)
#define	SEG_IS_SYSTEM(sdp)		(!(sdp)->s)
#define	SEG_IS_CODE(sdp)		((sdp)->s && (sdp)->u.seg.c)
#define	SEG_IS_DATA(sdp)		((sdp)->s && !(sdp)->u.seg.c)
#define	SEG_IS_READABLE_CODE(sdp)	((sdp)->u.seg.wr)
#define	SEG_IS_WRITABLE_DATA(sdp)	((sdp)->u.seg.wr)
#define	SEG_IS_CONFORMING_CODE(sdp)	((sdp)->u.seg.ec)
#define	SEG_IS_EXPANDDOWN_DATA(sdp)	((sdp)->u.seg.ec)

/*
 * segment descriptor
 *
 *  31            24 23 22 21 20 19   16 15 14 13 12 11    8 7             0
 * +----------------+--+--+--+--+-------+--+-----+--+-------+---------------+
 * |  Base  31..16  | G|DB| 0| A|limit_h| P| DPL | S|  type |  Base  23:16  | 4
 * +----------------+--+--+--+--+-------+--+-----+--+-------+---------------+
 *  31                                16 15                                0
 * +------------------------------------+-----------------------------------+
 * |           Base  15..00             |            limit  15..0           | 0
 * +------------------------------------+-----------------------------------+
 */

/* descriptor common */
#define	CPU_DESC_H_TYPE_SHIFT	8
#define	CPU_DESC_H_TYPE		(0xf << CPU_DESC_H_TYPE_SHIFT)
#define	CPU_DESC_H_S		(  1 << 12)	/* 0 = system, 1 = code/data */
#define	CPU_DESC_H_DPL_SHIFT	13
#define	CPU_DESC_H_DPL		(  3 << CPU_DESC_H_DPL_SHIFT)
#define	CPU_DESC_H_P		(  1 << 15)	/* exist */

/* for segment descriptor */
#define	CPU_SEGDESC_H_A		(  1 <<  8)
#define	CPU_SEGDESC_H_D_C	(  1 << 11)	/* 0 = data, 1 = code */
#define	CPU_SEGDESC_H_D		(  1 << 22)
#define	CPU_SEGDESC_H_G		(  1 << 23)

/* for gate descriptor */
#define	CPU_GATEDESC_H_D	(  1 << 11)

/* for tss descriptor */
#define	CPU_TSS_H_BUSY		(  1 <<  9)

/*
 * descriptor type
 */
#define	CPU_SEGDESC_TYPE_A		0x01
#define	CPU_SEGDESC_TYPE_WR		0x02
#define	CPU_SEGDESC_TYPE_EC		0x04

#define	CPU_SYSDESC_TYPE_TSS_16		0x01
#define	CPU_SYSDESC_TYPE_LDT		0x02
#define	CPU_SYSDESC_TYPE_TSS_BUSY_16	0x03
#define	CPU_SYSDESC_TYPE_CALL_16	0x04	/* call gate */
#define	CPU_SYSDESC_TYPE_TASK		0x05	/* task gate */
#define	CPU_SYSDESC_TYPE_INTR_16	0x06	/* hardware interrupt */
#define	CPU_SYSDESC_TYPE_TRAP_16	0x07	/* software interrupt */
#define	CPU_SYSDESC_TYPE_TSS_32		0x09
#define	CPU_SYSDESC_TYPE_TSS_BUSY_32	0x0b
#define	CPU_SYSDESC_TYPE_CALL_32	0x0c	/* call gate */
#define	CPU_SYSDESC_TYPE_INTR_32	0x0e	/* hardware interrupt */
#define	CPU_SYSDESC_TYPE_TRAP_32	0x0f	/* software interrupt */

#define	CPU_SYSDESC_TYPE_TSS		0x01
/*	CPU_SYSDESC_TYPE_LDT		0x02	*/
#define	CPU_SYSDESC_TYPE_TSS_BUSY	0x03
#define	CPU_SYSDESC_TYPE_CALL		0x04
/*	CPU_SYSDESC_TYPE_TASK		0x05	*/
#define	CPU_SYSDESC_TYPE_INTR		0x06
#define	CPU_SYSDESC_TYPE_TRAP		0x07
#define	CPU_SYSDESC_TYPE_MASKBIT	0x07
#define	CPU_SYSDESC_TYPE_32BIT		0x08

#define	CPU_SYSDESC_TYPE_TSS_BUSY_IND	0x02


void CPUCALL segdesc_init(int idx, UINT16 sreg, descriptor_t *sdp);
void CPUCALL load_descriptor(descriptor_t *sdp, UINT32 addr);

void CPUCALL load_segreg(int idx, UINT16 selector, UINT16 *sregp, descriptor_t *sdp, int exc);
#define	LOAD_SEGREG1(idx, selector, e) \
	load_segreg(idx, selector, &CPU_REGS_SREG(idx), &CPU_STAT_SREG(idx), e)
#define	LOAD_SEGREG(idx, selector) \
	LOAD_SEGREG1((idx), (selector), GP_EXCEPTION)
void CPUCALL load_ss(UINT16 selector, const descriptor_t *sdp, int cpl);
void CPUCALL load_cs(UINT16 selector, const descriptor_t *sdp, int cpl);
void CPUCALL load_ldtr(UINT16 selector, int exc);


/*
 * segment selector
 */
#define	CPU_SEGMENT_SELECTOR_INDEX_MASK	(~7)
#define	CPU_SEGMENT_SELECTOR_RPL_MASK	(3)
#define	CPU_SEGMENT_TABLE_IND		(1 << 2)	/* 0 = GDT, 1 = LDT */

typedef struct {
	UINT16		selector;
	UINT16		idx;
	UINT16		rpl;
	UINT8		ldt;
	UINT8		pad;

	UINT32		addr;		/* descriptor linear address */

	descriptor_t	desc;
} selector_t;

int CPUCALL parse_selector(selector_t *ssp, UINT16 selector);
int CPUCALL selector_is_not_present(const selector_t *ssp);

#ifdef __cplusplus
}
#endif

#endif	/* !IA32_CPU_SEGMENTS_H__ */
