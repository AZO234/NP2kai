/**
 * @file	lio.c
 * @brief	Implementation of LIO
 */

#include "compiler.h"
#include "lio.h"
#include "cpucore.h"
#include "pccore.h"
#include "iocore.h"
#include "gdc_sub.h"
#include "bios/bios.h"
#include "bios/biosmem.h"
#include "vram.h"
#include "lio.res"

void lio_initialize(void) {

	CopyMemory(mem + (LIO_SEGMENT << 4), liorom, sizeof(liorom));
}

void bios_lio(REG8 cmd) {

	_GLIO	lio;
	UINT8	ret;

//	TRACEOUT(("lio command %.2x", cmd));
	MEMR_READS(CPU_DS, 0x0620, &lio.work, sizeof(lio.work));
	lio.palmode = MEMR_READ8(CPU_DS, 0x0a08);
	lio.wait = 0;
	switch(cmd) {
		case 0x00:			// a0: GINIT
			ret = lio_ginit(&lio);
			break;

		case 0x01:			// a1: GSCREEN
			ret = lio_gscreen(&lio);
			break;

		case 0x02:			// a2: GVIEW
			ret = lio_gview(&lio);
			break;

		case 0x03:			// a3: GCOLOR1
			ret = lio_gcolor1(&lio);
			break;

		case 0x04:			// a4: GCOLOR2
			ret = lio_gcolor2(&lio);
			break;

		case 0x05:			// a5: GCLS
			ret = lio_gcls(&lio);
			break;

		case 0x06:			// a6: GPSET
			ret = lio_gpset(&lio);
			break;

		case 0x07:			// a7: GLINE
			ret = lio_gline(&lio);
			break;

		case 0x08:			// a8: GCIRCLE
			ret = lio_gcircle(&lio);
			break;

//		case 0x09:			// a9: GPAINT1
//			break;

//		case 0x0a:			// aa: GPAINT2
//			break;

		case 0x0b:			// ab: GGET
			ret = lio_gget(&lio);
			break;

		case 0x0c:			// ac: GPUT1
			ret = lio_gput1(&lio);
			break;

		case 0x0d:			// ad: GPUT2
			ret = lio_gput2(&lio);
			break;

//		case 0x0e:			// ae: GROLL
//			break;

		case 0x0f:			// af: GPOINT2
			ret = lio_gpoint2(&lio);
			break;

//		case 0x10:			// ce: GCOPY
//			break;

		default:
			ret = LIO_SUCCESS;
			break;
	}
	CPU_AH = ret;
	if (lio.wait) {
		gdcsub_setslavewait(lio.wait);
	}
}


// ----

const UINT32 lioplaneadrs[4] = {VRAM_B, VRAM_R, VRAM_G, VRAM_E};

void lio_updatedraw(GLIO lio) {

	UINT8	flag;
	UINT8	colorbit;
	SINT16	maxline;
	SINT16	tmp;

	flag = 0;
	colorbit = 3;
	maxline = 399;
	if (lio->palmode == 2) {
		flag |= LIODRAW_4BPP;
		colorbit = 4;
	}
	switch(lio->work.scrnmode) {
		case 0:
			if (lio->work.pos & 1) {
				flag |= LIODRAW_UPPER;
			}
			maxline = 199;
			break;

		case 1:
			flag |= lio->work.pos % colorbit;
			flag |= LIODRAW_MONO;
			if (lio->work.pos >= colorbit) {
				flag |= LIODRAW_UPPER;
			}
			maxline = 199;
			break;

		case 2:
			flag |= lio->work.pos % colorbit;
			flag |= LIODRAW_MONO;
			break;
	}
	lio->draw.flag = flag;
	lio->draw.palmax = 1 << colorbit;

	tmp = (SINT16)LOADINTELWORD(lio->work.viewx1);
	lio->draw.x1 = max(tmp, 0);
	tmp = (SINT16)LOADINTELWORD(lio->work.viewy1);
	lio->draw.y1 = max(tmp, 0);
	tmp = (SINT16)LOADINTELWORD(lio->work.viewx2);
	lio->draw.x2 = min(tmp, 639);
	tmp = (SINT16)LOADINTELWORD(lio->work.viewy2);
	lio->draw.y2 = min(tmp, maxline);
	if (!gdcs.access) {
		lio->draw.base = 0;
		lio->draw.bank = 0;
		lio->draw.sbit = 0x01;
	}
	else {
		lio->draw.base = VRAM_STEP;
		lio->draw.bank = 1;
		lio->draw.sbit = 0x02;
	}
}


// ----

static void pixed8(const _GLIO *lio, UINT addr, REG8 bit, REG8 pal) {

	UINT8	*ptr;

	addr = LOW15(addr);
	vramupdate[addr] |= lio->draw.sbit;
	ptr = mem + lio->draw.base + addr;
	if (!(lio->draw.flag & LIODRAW_MONO)) {
		if (pal & 1) {
			ptr[VRAM_B] |= bit;
		}
		else {
			ptr[VRAM_B] &= ~bit;
		}
		if (pal & 2) {
			ptr[VRAM_R] |= bit;
		}
		else {
			ptr[VRAM_R] &= ~bit;
		}
		if (pal & 4) {
			ptr[VRAM_G] |= bit;
		}
		else {
			ptr[VRAM_G] &= ~bit;
		}
		if (lio->draw.flag & LIODRAW_4BPP) {
			if (pal & 8) {
				ptr[VRAM_E] |= bit;
			}
			else {
				ptr[VRAM_E] &= ~bit;
			}
		}
	}
	else {
		ptr += lioplaneadrs[lio->draw.flag & LIODRAW_PMASK];
		if (pal) {
			*ptr |= bit;
		}
		else {
			*ptr &= ~bit;
		}
	}
}

void lio_pset(const _GLIO *lio, SINT16 x, SINT16 y, REG8 pal) {

	UINT	addr;
	UINT8	bit;

	if ((lio->draw.x1 > x) || (lio->draw.x2 < x) ||
		(lio->draw.y1 > y) || (lio->draw.y2 < y)) {
		return;
	}
	addr = (y * 80) + (x >> 3);
	bit = 0x80 >> (x & 7);
	if (lio->draw.flag & LIODRAW_UPPER) {
		addr += 16000;
	}
	gdcs.grphdisp |= lio->draw.sbit;
	pixed8(lio, addr, bit, pal);
}

#if 0
void lio_line(const _GLIO *lio, SINT16 x1, SINT16 x2, SINT16 y, REG8 pal) {

	UINT	addr;
	UINT8	bit, dbit;
	SINT16	width;

	if ((lio->draw.y1 > y) || (lio->draw.y2 < y)) {
		return;
	}
	if (lio->draw.x1 > x1) {
		x1 = lio->draw.x1;
	}
	if (lio->draw.x2 < x2) {
		x2 = lio->draw.x2;
	}
	width = x2 - x1 + 1;
	if (width <= 0) {
		return;
	}
	addr = (y * 80) + (x1 >> 3);
	bit = 0x80 >> (x1 & 7);
	if (lio->draw.flag & LIODRAW_UPPER) {
		addr += 16000;
	}
	gdcs.grphdisp |= lio->draw.sbit;
	dbit = 0;
	while((bit) && (width)) {
		dbit |= bit;
		bit >>= 1;
		width--;
	}
	pixed8(lio, addr, dbit, pal);
	addr++;
	while(width >= 8) {
		width -= 8;
		pixed8(lio, addr, 0xff, pal);
		addr++;
	}
	dbit = 0;
	bit = 0x80;
	while((bit) && (width)) {
		dbit |= bit;
		bit >>= 1;
		width--;
	}
	if (dbit) {
		pixed8(lio, addr, dbit, pal);
	}
}
#endif

