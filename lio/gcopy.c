#include	"compiler.h"
#include	"lio.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	<io/iocore.h>
#include	<io/gdc_sub.h>
#include	"bios/bios.h"
#include	"bios/biosmem.h"
#include	<vram/vram.h>

static REG8 gcopy_bit_at_addr(UINT base, UINT addr, UINT pl, UINT sft) {

const UINT8	*ptr;

	ptr = mem + base + addr + lioplaneadrs[pl];
	return((REG8)(((*ptr) >> sft) & 1));
}

static REG8 gcopy_display_pixel(const _GLIO *lio, SINT16 x, SINT16 y) {

	UINT	colorbit;
	UINT	addr;
	UINT	sft;
	UINT	base;
	UINT	pageoff;
	UINT	plane;
	UINT	bit;
	UINT	pl;
	REG8	pal;

	colorbit = (lio->palmode == 2)?4:3;
	addr = (UINT)((y * 80) + (x >> 3));
	sft = (~x) & 7;
	if ((lio->work.scrnmode == 0) || (lio->work.scrnmode == 3)) {
		if ((!lio->work.plane) || (lio->work.plane == (1U << colorbit))) {
			return(0);
		}
		base = lio->work.disp?VRAM_STEP:0;
		pageoff = 0;
		if ((lio->work.scrnmode == 0) && (lio->work.plane & 2)) {
			pageoff = 16000;
		}
		pal = 0;
		for (pl=0; pl<3; pl++) {
			pal |= (REG8)(gcopy_bit_at_addr(base + pageoff, addr, pl, sft) << pl);
		}
		if (colorbit == 4) {
			pal |= (REG8)(gcopy_bit_at_addr(base + pageoff, addr, 3, sft) << 3);
		}
		return(pal?1:0);
	}
	else {
		base = lio->work.disp?VRAM_STEP:0;
		plane = lio->work.plane;
		if (lio->work.scrnmode == 1) {
			for (bit=0; bit<(colorbit * 2); bit++) {
				if (!(plane & (1U << bit))) {
					continue;
				}
				pl = bit % colorbit;
				pageoff = (bit >= colorbit)?16000:0;
				if (gcopy_bit_at_addr(base + pageoff, addr, pl, sft)) {
					return(1);
				}
			}
		}
		else {
			for (bit=0; bit<colorbit; bit++) {
				if ((plane & (1U << bit)) &&
					gcopy_bit_at_addr(base, addr, bit, sft)) {
					return(1);
				}
			}
		}
	}
	return(0);
}

REG8 lio_gcopy(GLIO lio) {

	SINT16	x;
	SINT16	y;
	UINT	width;
	UINT	code;
	UINT	group;
	UINT	height;
	UINT	outbytes;
	UINT	maxy;
	UINT	xp;
	UINT	r;
	REG8	b0;
	REG8	b1;
	UINT	off;
	UINT	seg;

	x = (SINT16)CPU_AX;
	y = (SINT16)CPU_BX;
	width = CPU_CX & 0xff;
	if (!width) {
		width = 256;
	}
	code = (CPU_CX >> 8) & 0xff;
	if ((code == 2) || (code == 4) || (code == 8)) {
		group = code;
		height = code;
		outbytes = 1;
	}
	else if ((code == 0x82) || (code == 0x84)) {
		group = code & 0x7f;
		height = group * 2;
		outbytes = 2;
	}
	else {
		return(LIO_ILLEGALFUNC);
	}
	maxy = ((lio->work.scrnmode == 0) || (lio->work.scrnmode == 1))?200:400;
	if ((x < 0) || (y < 0) || (x & 7) || (width & 7) ||
		(((UINT)x + width) > 640) || (((UINT)y + height) > maxy)) {
		return(LIO_ILLEGALFUNC);
	}
	off = CPU_DI;
	seg = CPU_ES;
	for (xp=0; xp<width; xp++) {
		b0 = 0;
		b1 = 0;
		if (outbytes == 1) {
			for (r=0; r<height; r++) {
				if (gcopy_display_pixel(lio, (SINT16)(x + xp), (SINT16)(y + r))) {
					b0 |= (REG8)(1 << r);
				}
			}
			MEMR_WRITE8(seg, off++, b0);
		}
		else {
			for (r=0; r<group; r++) {
				if (gcopy_display_pixel(lio, (SINT16)(x + xp), (SINT16)(y + r))) {
					b0 |= (REG8)(1 << (r * 2));
				}
				if (gcopy_display_pixel(lio, (SINT16)(x + xp),
										(SINT16)(y + group + r))) {
					b1 |= (REG8)(1 << (r * 2));
				}
			}
			MEMR_WRITE8(seg, off++, b0);
			MEMR_WRITE8(seg, off++, b1);
		}
	}
	return(LIO_SUCCESS);
}
