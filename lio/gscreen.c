#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	<io/iocore.h>
#include	"bios/bios.h"
#include	"bios/biosmem.h"
#include	<io/gdc_sub.h>
#include	"lio.h"
#include	<vram/vram.h>


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


static void gview_vectl(const _GLIO *lio, int x1, int y1, int x2, int y2, UINT8 pal) {

	int		dx;
	int		dy;
	int		sx;
	int		sy;
	int		err;
	int		e2;

	if (y1 == y2) {
		lio_line(lio, (SINT16)x1, (SINT16)x2, (SINT16)y1, pal);
		return;
	}
	if (x1 == x2) {
		if (y1 > y2) {
			int tmp = y1;
			y1 = y2;
			y2 = tmp;
		}
		while(y1 <= y2) {
			lio_pset(lio, (SINT16)x1, (SINT16)y1, pal);
			y1++;
		}
		return;
	}

	dx = x2 - x1;
	if (dx < 0) {
		dx = -dx;
	}
	dy = y2 - y1;
	if (dy < 0) {
		dy = -dy;
	}
	sx = (x1 < x2) ? 1 : -1;
	sy = (y1 < y2) ? 1 : -1;
	err = dx - dy;
	for (;;) {
		lio_pset(lio, (SINT16)x1, (SINT16)y1, pal);
		if ((x1 == x2) && (y1 == y2)) {
			break;
		}
		e2 = err << 1;
		if (e2 > -dy) {
			err -= dy;
			x1 += sx;
		}
		if (e2 < dx) {
			err += dx;
			y1 += sy;
		}
	}
}

static void gview_box(const _GLIO *lio, UINT8 pal) {

	int		y;

	y = lio->draw.y1;
	while(y <= lio->draw.y2) {
		gview_vectl(lio, lio->draw.x1, y, lio->draw.x2, y, pal);
		y++;
	}
}

static void gview_frame(const _GLIO *lio, UINT8 pal) {

	gview_vectl(lio, lio->draw.x1, lio->draw.y1,
				lio->draw.x2, lio->draw.y1, pal);
	gview_vectl(lio, lio->draw.x1, lio->draw.y2,
				lio->draw.x2, lio->draw.y2, pal);
	gview_vectl(lio, lio->draw.x1, lio->draw.y1,
				lio->draw.x1, lio->draw.y2, pal);
	gview_vectl(lio, lio->draw.x2, lio->draw.y1,
				lio->draw.x2, lio->draw.y2, pal);
}

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


static BOOL gscreen_decode_active(UINT8 scrnmode, UINT8 colorbit, UINT8 raw,
                                                        UINT8 *pos, UINT8 *access) {

    UINT8   p;
    UINT8   a;

    a = raw;
    switch(scrnmode) {
        case 0:     /* 640x200 colour: two colour pages */
            p = a & 1;
            a >>= 1;
            break;

        case 1:     /* 640x200 mono: plane x page */
            p = (UINT8)(a % (colorbit * 2));
            a = (UINT8)(a / (colorbit * 2));
            break;

        case 2:     /* 640x400 mono: plane only, plus access bank */
            p = (UINT8)(a % colorbit);
            a = (UINT8)(a / colorbit);
            break;

        case 3:     /* 640x400 colour: one drawable colour screen */
        default:
            p = 0;
            break;
    }
    if (a >= 2) {
        return(FALSE);
    }
    *pos = p;
    *access = a;
    return(TRUE);
}

static BOOL gscreen_decode_display(UINT8 scrnmode, UINT8 colorbit, UINT8 raw,
                                                        UINT8 *plane, UINT8 *bank) {

    UINT8   p;
    UINT8   b;
    UINT8   upperbit;
    UINT8   planemax;
    UINT8   mono;

    upperbit = (UINT8)(1 << colorbit);
    p = (UINT8)(raw & ((2 << colorbit) - 1));
    b = (UINT8)(raw >> (colorbit + 1));
    if (b >= 2) {
        return(FALSE);
    }

    switch(scrnmode) {
        case 0:
            planemax = 2;       /* display page 1 or 2 */
            break;

        case 1:
            planemax = (UINT8)(upperbit * 2 - 1);
            break;

        case 2:
            planemax = (UINT8)(upperbit - 1);
            break;

        case 3:
        default:
            planemax = 1;
            break;
    }

    mono = (UINT8)((scrnmode + 1) >> 1) & 1;
    if (mono || (scrnmode == 3)) {
        if ((p == 0) || (p == upperbit)) {
            *plane = p;
            *bank = b;
            return(TRUE);
        }
    }
    else if (p == 0) {
        *plane = p;
        *bank = b;
        return(TRUE);
    }

    if (p > planemax) {
        return(FALSE);
    }
    *plane = p;
    *bank = b;
    return(TRUE);
}

static UINT8 gscreen_make_crtmode(UINT8 scrnmode, UINT8 colorbit, UINT8 plane,
                                                        UINT8 bank) {

    UINT8   upperbit;
    UINT8   lowmask;
    UINT8   mode;

    upperbit = (UINT8)(1 << colorbit);
    lowmask = (UINT8)(upperbit - 1);

    if ((plane == 0) || (((scrnmode != 0) || (colorbit != 3)) && (plane == upperbit))) {
        return(0xc0);
    }

    switch(scrnmode) {
        case 0:     /* 640x200 color */
            mode = (plane == 2) ? 0x40 : 0x80;
            break;

        case 1:     /* 640x200 mono */
            mode = (plane & upperbit) ? 0x60 : 0xa0;
            break;

        case 2:     /* 640x400 mono */
            mode = (plane & lowmask) ? 0xe0 : 0xc0;
            break;

        case 3:     /* 640x400 color */
        default:
            mode = (plane & 1) ? 0xc0 : 0xc0;
            break;
    }
    mode |= (UINT8)(bank << 4);
    return(mode);
}

// ---- SCREEN

REG8 lio_gscreen(GLIO lio) {

    GSCREEN dat;
    UINT    colorbit;
    UINT8   oldscrnmode;
    UINT8   scrnmode;
    UINT8   pos;
    UINT8   access;
    UINT8   plane;
    UINT8   dispbank;
    UINT8   crtmode;
    BOOL    mode_changed;

    colorbit = (lio->palmode == 2) ? 4 : 3;
    MEMR_READS(CPU_DS, CPU_BX, &dat, sizeof(dat));

    oldscrnmode = lio->work.scrnmode;
    scrnmode = (dat.mode == 0xff) ? oldscrnmode : dat.mode;
    if (scrnmode >= 4) {
        goto gscreen_err5;
    }
    if ((dat.mode != 0xff) && (scrnmode >= 2) && (!(mem[MEMB_PRXCRT] & 0x40))) {
        goto gscreen_err5;
    }
    if ((dat.sw != 0xff) && (dat.sw >= 4)) {
        goto gscreen_err5;
    }

    mode_changed = (scrnmode != oldscrnmode);

    if (dat.act == 0xff) {
        pos = mode_changed ? 0 : lio->work.pos;
        access = mode_changed ? 0 : lio->work.access;
    }
    else if (!gscreen_decode_active(scrnmode, (UINT8)colorbit,
                                    dat.act, &pos, &access)) {
        goto gscreen_err5;
    }

    if (dat.disp == 0xff) {
        plane = mode_changed ? 1 : lio->work.plane;
        dispbank = mode_changed ? 0 : lio->work.disp;
    }
    else if (!gscreen_decode_display(scrnmode, (UINT8)colorbit,
                                     dat.disp, &plane, &dispbank)) {
        goto gscreen_err5;
    }

    crtmode = gscreen_make_crtmode(scrnmode, (UINT8)colorbit, plane, dispbank);

    if (dat.sw != 0xff) {
        if (dat.sw & 2) {
            bios0x18_41();
        }
        else {
            bios0x18_40();
        }
    }

    lio->work.scrnmode = scrnmode;
    lio->work.pos = pos;
    lio->work.access = access;
    lio->work.plane = plane;
    lio->work.disp = dispbank;

    if (mode_changed) {
        STOREINTELWORD(lio->work.viewx1, 0);
        STOREINTELWORD(lio->work.viewy1, 0);
        STOREINTELWORD(lio->work.viewx2, 639);
        STOREINTELWORD(lio->work.viewy2, (scrnmode & 2) ? 399 : 199);
    }

    bios0x18_42(crtmode);
    iocore_out8(0x00a6, lio->work.access);
    MEMR_WRITES(CPU_DS, 0x0620, &lio->work, sizeof(lio->work));
    gdcs.palchange = 1;
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

	lio_updatedraw(lio);
	if ((dat.vdraw_bg != 0xff) && (dat.vdraw_bg >= lio->draw.palmax)) {
		return(LIO_ILLEGALFUNC);
	}
	if ((dat.vdraw_ln != 0xff) && (dat.vdraw_ln >= lio->draw.palmax)) {
		return(LIO_ILLEGALFUNC);
	}

	STOREINTELWORD(lio->work.viewx1, (UINT16)x1);
	STOREINTELWORD(lio->work.viewy1, (UINT16)y1);
	STOREINTELWORD(lio->work.viewx2, (UINT16)x2);
	STOREINTELWORD(lio->work.viewy2, (UINT16)y2);
	MEMR_WRITES(CPU_DS, 0x0620, &lio->work, sizeof(lio->work));

	// View‚Ì‹éŒ`—ÖŠsE“h‚è‚Â‚Ô‚µ•`‰æ
	if ((dat.vdraw_bg != 0xff) || (dat.vdraw_ln != 0xff)) {
		lio_updatedraw(lio);
		if (dat.vdraw_bg != 0xff) {
			gview_box(lio, dat.vdraw_bg);
		}
		if (dat.vdraw_ln != 0xff) {
			gview_frame(lio, dat.vdraw_ln);
		}
	}
	return(LIO_SUCCESS);
}


// ---- COLOR1

REG8 lio_gcolor1(GLIO lio) {

	GCOLOR1	dat;

	MEMR_READS(CPU_DS, CPU_BX, &dat, sizeof(dat));
	if (dat.bgcolor != 0xff) {
		lio->work.bgcolor = dat.bgcolor;
	}
	if (dat.fgcolor != 0xff) {
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
    gdcs.palchange = 1;
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
		if ((lio->work.scrnmode == 1) || (lio->work.scrnmode == 2)) {
			// ƒ‚ƒmƒNƒƒpƒŒƒbƒg ª‹’–³‚µ
			dat.color1 = (dat.color1 & 1) ? 7 : 0;
			lio->work.color[dat.pal] = dat.color1;
			gdc_setdegitalpal(dat.pal, dat.color1);
		}
		else {
			dat.color1 &= 7;
			lio->work.color[dat.pal] = dat.color1;
			gdc_setdegitalpal(dat.pal, dat.color1);
		}
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
    gdcs.palchange = 1;
	return(LIO_SUCCESS);

gcolor2_err5:
	return(LIO_ILLEGALFUNC);
}

