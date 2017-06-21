#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"bios/bios.h"
#include	"bios/biosmem.h"
#include	"lio.h"
#include	"vram.h"


typedef struct {
	UINT8	mode;
	UINT8	sw;
	UINT8	act;
	UINT8	disp;
} GSCREEN;

typedef struct {
	UINT8	x1[2];
	UINT8	y1[2];
	UINT8	x2[2];
	UINT8	y2[2];
	UINT8	vdraw_bg;
	UINT8	vdraw_ln;
} GVIEW;

typedef struct {
	UINT8	dummy;
	UINT8	bgcolor;
	UINT8	bdcolor;
	UINT8	fgcolor;
	UINT8	palmode;
} GCOLOR1;

typedef struct {
	UINT8	pal;
	UINT8	color1;
	UINT8	color2;
} GCOLOR2;


// ---- INIT

REG8 lio_ginit(GLIO lio) {

	UINT	i;

	vramop.operate &= ~(1 << VOPBIT_ACCESS);
	MEMM_VRAM(vramop.operate);
	bios0x18_42(0x80);
	bios0x18_40();
	iocore_out8(0x006a, 0);
	gdc_paletteinit();

	ZeroMemory(&lio->work, sizeof(lio->work));
//	lio->work.scrnmode = 0;
//	lio->work.pos = 0;
	lio->work.plane = 1;
//	lio->work.bgcolor = 0;
	lio->work.fgcolor = 7;
	for (i=0; i<8; i++) {
		lio->work.color[i] = (UINT8)i;
	}
//	STOREINTELWORD(lio->work.viewx1, 0);
//	STOREINTELWORD(lio->work.viewy1, 0);
	STOREINTELWORD(lio->work.viewx2, 639);
	STOREINTELWORD(lio->work.viewy2, 399);
	lio->palmode = 0;
	MEMR_WRITES(CPU_DS, 0x0620, &lio->work, sizeof(lio->work));
	MEMR_WRITE8(CPU_DS, 0x0a08, lio->palmode);
	return(LIO_SUCCESS);
}


// ---- SCREEN

REG8 lio_gscreen(GLIO lio) {

	GSCREEN	dat;
	UINT	colorbit;
	UINT8	scrnmode;
	UINT8	mono;
	UINT8	act;
	UINT8	pos;
	UINT8	disp;
	UINT8	plane;
	UINT8	planemax;
	UINT8	mode;

	if (lio->palmode != 2) {
		colorbit = 3;
	}
	else {
		colorbit = 4;
	}
	MEMR_READS(CPU_DS, CPU_BX, &dat, sizeof(dat));
	scrnmode = dat.mode;
	if (scrnmode == 0xff) {
		scrnmode = lio->work.scrnmode;
	}
	else {
		if ((dat.mode >= 2) && (!(mem[MEMB_PRXCRT] & 0x40))) {
			goto gscreen_err5;
		}
	}
	if (scrnmode >= 4) {
		goto gscreen_err5;
	}
	if (dat.sw != 0xff) {
		if (!(dat.sw & 2)) {
			bios0x18_40();
		}
		else {
			bios0x18_41();
		}
	}

	mono = ((scrnmode + 1) >> 1) & 1;
	act = dat.act;
	if (act == 0xff) {
		if (scrnmode != lio->work.scrnmode) {
			lio->work.pos = 0;
			lio->work.access = 0;
		}
	}
	else {
		switch(scrnmode) {
			case 0:
				pos = act & 1;
				act >>= 1;
				break;

			case 1:
				pos = act % (colorbit * 2);
				act = act / (colorbit * 2);
				break;

			case 2:
				pos = act % colorbit;
				act = act / colorbit;
				break;

			case 3:
			default:
				pos = 0;
				break;
		}
		if (act >= 2) {
			goto gscreen_err5;
		}
		lio->work.pos = pos;
		lio->work.access = act;
	}
	disp = dat.disp;
	if (disp == 0xff) {
		if (scrnmode != lio->work.scrnmode) {
			lio->work.plane = 1;
			lio->work.disp = 0;
		}
	}
	else {
		plane = disp & ((2 << colorbit) - 1);
		disp >>= (colorbit + 1);
		if (disp >= 2) {
			goto gscreen_err5;
		}
		lio->work.disp = disp;
		planemax = 1;
		if (mono) {
			planemax <<= colorbit;
		}
		if (!(scrnmode & 2)) {
			planemax <<= 1;
		}
		if ((plane > planemax) &&
			(plane != (1 << colorbit))) {
			goto gscreen_err5;
		}
		lio->work.plane = plane;
		lio->work.disp = disp;
	}

	lio->work.scrnmode = scrnmode;
	pos = lio->work.pos;
	switch(scrnmode) {
		case 0:
			mode = (pos)?0x40:0x80;
			break;

		case 1:
			mode = (pos >= colorbit)?0x60:0xa0;
			break;

		case 2:
			mode = 0xe0;
			break;

		case 3:
		default:
			mode = 0xc0;
			break;
	}
	mode |= disp << 4;
	bios0x18_42(mode);
	iocore_out8(0x00a6, lio->work.access);
	MEMR_WRITES(CPU_DS, 0x0620, &lio->work, sizeof(lio->work));
	return(LIO_SUCCESS);

gscreen_err5:
	TRACEOUT(("screen error! %d %d %d %d",
								dat.mode, dat.sw, dat.act, dat.disp));
	return(LIO_ILLEGALFUNC);
}


// ---- VIEW

REG8 lio_gview(GLIO lio) {

	GVIEW	dat;
	int		x1;
	int		y1;
	int		x2;
	int		y2;

	MEMR_READS(CPU_DS, CPU_BX, &dat, sizeof(dat));
	x1 = (SINT16)LOADINTELWORD(dat.x1);
	y1 = (SINT16)LOADINTELWORD(dat.y1);
	x2 = (SINT16)LOADINTELWORD(dat.x2);
	y2 = (SINT16)LOADINTELWORD(dat.y2);
	if ((x1 >= x2) || (y1 >= y2)) {
		return(LIO_ILLEGALFUNC);
	}
	STOREINTELWORD(lio->work.viewx1, (UINT16)x1);
	STOREINTELWORD(lio->work.viewy1, (UINT16)y1);
	STOREINTELWORD(lio->work.viewx2, (UINT16)x2);
	STOREINTELWORD(lio->work.viewy2, (UINT16)y2);
	MEMR_WRITES(CPU_DS, 0x0620, &lio->work, sizeof(lio->work));
	return(LIO_SUCCESS);
}


// ---- COLOR1

REG8 lio_gcolor1(GLIO lio) {

	GCOLOR1	dat;

	MEMR_READS(CPU_DS, CPU_BX, &dat, sizeof(dat));
	if (dat.bgcolor != 0xff) {
		lio->work.bgcolor = dat.bgcolor;
	}
	if (dat.fgcolor == 0xff) {
		lio->work.fgcolor = dat.fgcolor;
	}
	if (dat.palmode != 0xff) {
		if (!(mem[MEMB_PRXCRT] & 1)) {				// 8color lio
			dat.palmode = 0;
		}
		else {
			if (!(mem[MEMB_PRXCRT] & 4)) {			// have e-plane?
				goto gcolor1_err5;
			}
			if (!dat.palmode) {
				iocore_out8(0x006a, 0);
			}
			else {
				iocore_out8(0x006a, 1);
			}
		}
		lio->palmode = dat.palmode;
	}
	MEMR_WRITES(CPU_DS, 0x0620, &lio->work, sizeof(lio->work));
	MEMR_WRITE8(CPU_DS, 0x0a08, lio->palmode);
	return(LIO_SUCCESS);

gcolor1_err5:
	return(LIO_ILLEGALFUNC);
}


// ---- COLOR2

REG8 lio_gcolor2(GLIO lio) {

	GCOLOR2	dat;

	MEMR_READS(CPU_DS, CPU_BX, &dat, sizeof(dat));
	if (dat.pal >= ((lio->palmode == 2)?16:8)) {
		goto gcolor2_err5;
	}
	if (!lio->palmode) {
		dat.color1 &= 7;
		lio->work.color[dat.pal] = dat.color1;
		gdc_setdegitalpal(dat.pal, dat.color1);
	}
	else {
		gdc_setanalogpal(dat.pal, offsetof(RGB32, p.b),
												(UINT8)(dat.color1 & 0x0f));
		gdc_setanalogpal(dat.pal, offsetof(RGB32, p.r),
												(UINT8)(dat.color1 >> 4));
		gdc_setanalogpal(dat.pal, offsetof(RGB32, p.g),
												(UINT8)(dat.color2 & 0x0f));
	}
	MEMR_WRITES(CPU_DS, 0x0620, &lio->work, sizeof(lio->work));
	return(LIO_SUCCESS);

gcolor2_err5:
	return(LIO_ILLEGALFUNC);
}

