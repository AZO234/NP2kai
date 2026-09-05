#include	"compiler.h"
#include	"lio.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	<io/iocore.h>
#include	<io/gdc_sub.h>
#include	"bios/bios.h"
#include	"bios/biosmem.h"
#include	<vram/vram.h>

typedef struct {
	UINT8	dy[2];
	UINT8	dx[2];
	UINT8	clr;
} GROLL;

static UINT8 grollbuf[32000];

static UINT groll_fullheight(const _GLIO* lio) {

	if ((lio->work.scrnmode == 0) || (lio->work.scrnmode == 1)) {
		return(200);
	}
	return(400);
}

static UINT groll_pageoff(const _GLIO* lio) {

	return((lio->draw.flag & LIODRAW_UPPER) ? 16000 : 0);
}

static UINT groll_planemask(const _GLIO *lio) {

	if (lio->draw.flag & LIODRAW_MONO) {
		return(1U << (lio->draw.flag & LIODRAW_PMASK));
	}
	return((lio->draw.flag & LIODRAW_4BPP)?0x0f:0x07);
}

REG8 lio_groll(GLIO lio) {

	GROLL	dat;
	SINT16	dy;
	SINT16	dx;
	SINT16	dxbyte;
	UINT	height;
	UINT	pageoff;
	UINT	mask;
	UINT	pl;
	UINT	y;
	UINT	b;
	int		srcy;
	int		srcb;
	UINT	base;
	UINT8	fill;
	UINT8	clrpal;
	UINT8	*ptr;

	lio_updatedraw(lio);
	MEMR_READS(CPU_DS, CPU_BX, &dat, sizeof(dat));
	dy = (SINT16)LOADINTELWORD(dat.dy);
	dx = (SINT16)LOADINTELWORD(dat.dx);
	if (dat.clr > 1) {
		return(LIO_ILLEGALFUNC);
	}
	height = groll_fullheight(lio);
	if ((dy <= -((SINT16)height)) || (dy >= (SINT16)height) ||
		(dx < -639) || (dx > 639)) {
		return(LIO_ILLEGALFUNC);
	}
	dxbyte = (SINT16)(dx / 8);
	clrpal = dat.clr?lio->work.bgcolor:0;
	pageoff = groll_pageoff(lio);
	mask = groll_planemask(lio);
	for (pl=0; pl<4; pl++) {
		if (!(mask & (1U << pl))) {
			continue;
		}
		base = lio->draw.base + pageoff + lioplaneadrs[pl];
		CopyMemory(grollbuf, mem + base, height * 80);
		fill = (clrpal & (1U << pl))?0xff:0x00;
		ptr = mem + base;
		for (y=0; y<height; y++) {
			for (b=0; b<80; b++) {
				srcy = (int)y + (int)dy;
				srcb = (int)b + (int)dxbyte;
				if ((srcy >= 0) && (srcy < (int)height) &&
					(srcb >= 0) && (srcb < 80)) {
					ptr[(y * 80) + b] = grollbuf[(srcy * 80) + srcb];
				}
				else {
					ptr[(y * 80) + b] = fill;
				}
			}
		}
	}
	gdcs.grphdisp |= lio->draw.sbit;
	for (y=0; y<(height * 80); y++) {
		vramupdate[LOW15(pageoff + y)] |= lio->draw.sbit;
	}
	lio->wait += (UINT32)height * 80 * 4;
	return(LIO_SUCCESS);
}

