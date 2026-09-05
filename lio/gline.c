#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	<io/iocore.h>
#include	<io/gdc_sub.h>
#include	"lio.h"

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif
#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif


typedef struct {
	UINT8	x1[2];
	UINT8	y1[2];
	UINT8	x2[2];
	UINT8	y2[2];
	UINT8	pal;
	UINT8	type;
	UINT8	sw;
	UINT8	style[2];
	UINT8	patleng;
	UINT8	off[2];
	UINT8	seg[2];
} GLINE;

typedef struct {
	int		x1;
	int		y1;
	int		x2;
	int		y2;
	UINT8	pal;
} LINEPT;

static UINT gettileplanes(const _GLIO *lio) {

	if (lio->draw.flag & LIODRAW_MONO) {
		return(1);
	}
	return((lio->draw.flag & LIODRAW_4BPP)?4:3);
}

static BOOL checktileleng(const _GLIO *lio, UINT leng) {

	UINT planes;

	if (leng == 0) {
		return(FALSE);
	}
	planes = gettileplanes(lio);
	return((leng >= planes) && ((leng % planes) == 0));
}



static int divfloor_int(int num, int den) {

	int		q;
	int		r;

	q = num / den;
	r = num % den;
	if ((r != 0) && ((r < 0) != (den < 0))) {
		q--;
	}
	return(q);
}

static UINT clipcode(const _GLIO *lio, int x, int y) {

	UINT	code;

	code = 0;
	if (x < lio->draw.x1) {
		code |= 1;
	}
	else if (x > lio->draw.x2) {
		code |= 2;
	}
	if (y < lio->draw.y1) {
		code |= 4;
	}
	else if (y > lio->draw.y2) {
		code |= 8;
	}
	return(code);
}

static BOOL clipline_directional_floor(const _GLIO *lio, LINEPT *lp) {

	int		x1;
	int		y1;
	int		x2;
	int		y2;
	UINT	code1;
	UINT	code2;
	UINT	code;
	int		dx;
	int		dy;
	int		i;

	x1 = lp->x1;
	y1 = lp->y1;
	x2 = lp->x2;
	y2 = lp->y2;
	for (i=0; i<16; i++) {
		code1 = clipcode(lio, x1, y1);
		code2 = clipcode(lio, x2, y2);
		if (!(code1 | code2)) {
			lp->x1 = x1;
			lp->y1 = y1;
			lp->x2 = x2;
			lp->y2 = y2;
			return(TRUE);
		}
		if (code1 & code2) {
			return(FALSE);
		}
		if (code1) {
			code = code1;
			dx = x2 - x1;
			dy = y2 - y1;
			if (code & 1) {
				if (!dx) {
					return(FALSE);
				}
				y1 += divfloor_int(dy * (lio->draw.x1 - x1), dx);
				x1 = lio->draw.x1;
			}
			else if (code & 2) {
				if (!dx) {
					return(FALSE);
				}
				y1 += divfloor_int(dy * (lio->draw.x2 - x1), dx);
				x1 = lio->draw.x2;
			}
			else if (code & 4) {
				if (!dy) {
					return(FALSE);
				}
				x1 += divfloor_int(dx * (lio->draw.y1 - y1), dy);
				y1 = lio->draw.y1;
			}
			else {
				if (!dy) {
					return(FALSE);
				}
				x1 += divfloor_int(dx * (lio->draw.y2 - y1), dy);
				y1 = lio->draw.y2;
			}
		}
		else {
			code = code2;
			dx = x1 - x2;
			dy = y1 - y2;
			if (code & 1) {
				if (!dx) {
					return(FALSE);
				}
				y2 += divfloor_int(dy * (lio->draw.x1 - x2), dx);
				x2 = lio->draw.x1;
			}
			else if (code & 2) {
				if (!dx) {
					return(FALSE);
				}
				y2 += divfloor_int(dy * (lio->draw.x2 - x2), dx);
				x2 = lio->draw.x2;
			}
			else if (code & 4) {
				if (!dy) {
					return(FALSE);
				}
				x2 += divfloor_int(dx * (lio->draw.y1 - y2), dy);
				y2 = lio->draw.y1;
			}
			else {
				if (!dy) {
					return(FALSE);
				}
				x2 += divfloor_int(dx * (lio->draw.y2 - y2), dy);
				y2 = lio->draw.y2;
			}
		}
	}
	return(FALSE);
}

static void gline(const _GLIO *lio, const LINEPT *lp, UINT16 pat) {

	int		x1;
	int		y1;
	int		x2;
	int		y2;
	int		swap;
	int		tmp;
	int		width;
	int		height;
	int		d1;
	int		d2;

	x1 = lp->x1;
	y1 = lp->y1;
	x2 = lp->x2;
	y2 = lp->y2;

	// �т�[�ۂ����
	swap = 0;
	if (x1 > x2) {
		LINEPT clp;
		clp = *lp;
		if (!clipline_directional_floor(lio, &clp)) {
			return;
		}
		x1 = clp.x1;
		y1 = clp.y1;
		x2 = clp.x2;
		y2 = clp.y2;
		goto gline_clipped;
	}
	if ((x1 > lio->draw.x2) || (x2 < lio->draw.x1)) {
		return;
	}
	width = x2 - x1;
	height = y2 - y1;
	d1 = lio->draw.x1 - x1;
	d2 = x2 - lio->draw.x2;
	if (d1 > 0) {
		x1 = lio->draw.x1;
		y1 += (((height * d1 * 2) / width) + 1) >> 1;
	}
	if (d2 > 0) {
		x2 = lio->draw.x2;
		y2 -= (((height * d2 * 2) / width) + 1) >> 1;
	}
	if (swap) {
		tmp = x1;
		x1 = x2;
		x2 = tmp;
		tmp = y1;
		y1 = y2;
		y2 = tmp;
	}

	swap = 0;
	if (y1 > y2) {
		tmp = x1;
		x1 = x2;
		x2 = tmp;
		tmp = y1;
		y1 = y2;
		y2 = tmp;
	}
	if ((y1 > lio->draw.y2) || (y2 < lio->draw.y1)) {
		return;
	}
	width = x2 - x1;
	height = y2 - y1;
	d1 = lio->draw.y1 - y1;
	d2 = y2 - lio->draw.y2;
	if (d1 > 0) {
		y1 = lio->draw.y1;
		x1 += (((width * d1 * 2) / height) + 1) >> 1;
	}
	if (d2 > 0) {
		y2 = lio->draw.y2;
		x2 -= (((width * d2 * 2) / height) + 1) >> 1;
	}
	if (swap) {
		tmp = x1;
		x1 = x2;
		x2 = tmp;
		tmp = y1;
		y1 = y2;
		y2 = tmp;
	}

	// �i�񂾋����v�Z
gline_clipped:
	d1 = x1 - lp->x1;
	if (d1 < 0) {
		d1 = 0 - d1;
	}
	d2 = y1 - lp->y1;
	if (d2 < 0) {
		d2 = 0 - d2;
	}
	d1 = max(d1, d2) & 15;
	pat = (UINT16)((pat >> d1) | (pat << (16 - d1)));

	{
		int dx;
		int dy;
		int sx;
		int sy;
		int err;
		int e2;
		UINT16 lpat;

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
		lpat = pat;
		for (;;) {
			if (lpat & 1) {
				lio_pset(lio, (SINT16)x1, (SINT16)y1, lp->pal);
			}
			lpat = (UINT16)((lpat >> 1) | (lpat << 15));
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

}

static void glineb(const _GLIO *lio, const LINEPT *lp, UINT16 pat) {

	LINEPT	lpt;

	lpt = *lp;
	lpt.y2 = lp->y1;
	gline(lio, &lpt, pat);
	lpt.y2 = lp->y2;

	lpt.x2 = lp->x1;
	gline(lio, &lpt, pat);
	lpt.x2 = lp->x2;

	lpt.x1 = lp->x2;
	gline(lio, &lpt, pat);
	lpt.x1 = lp->x1;

	lpt.y1 = lp->y2;
	gline(lio, &lpt, pat);
	lpt.y1 = lp->y1;
}


// ----

static REG8 gbox_tilepal(const _GLIO *lio, const UINT8 *tile, UINT leng,
												int x, int y, UINT planes) {

	UINT	row;
	UINT	idx;
	UINT	pl;
	UINT	bit;
	REG8	pal;

	row = ((UINT)(y - lio->draw.y1) * planes) % leng;
	bit = 0x80 >> ((x - lio->draw.x1) & 7);
	pal = 0;
	for (pl=0; pl<planes; pl++) {
		idx = row + pl;
		if (idx >= leng) {
			idx -= leng;
		}
		if (tile[idx] & bit) {
			pal |= (REG8)(1 << pl);
		}
	}
	return(pal);
}

static void gbox(const _GLIO *lio, const LINEPT *lp, UINT8 *tile, UINT leng) {

	int		x1;
	int		y1;
	int		x2;
	int		y2;
	int		tmp;
	int		x;
	UINT	planes;
	REG8	pal;

	x1 = lp->x1;
	y1 = lp->y1;
	x2 = lp->x2;
	y2 = lp->y2;

	if (x1 > x2) {
		tmp = x1;
		x1 = x2;
		x2 = tmp;
	}
	if (y1 > y2) {
		tmp = y1;
		y1 = y2;
		y2 = tmp;
	}
	if ((x1 > lio->draw.x2) || (x2 < lio->draw.x1) ||
		(y1 > lio->draw.y2) || (y2 < lio->draw.y1)) {
		return;
	}
	x1 = max(x1, lio->draw.x1);
	y1 = max(y1, lio->draw.y1);
	x2 = min(x2, lio->draw.x2);
	y2 = min(y2, lio->draw.y2);

	if (leng == 0) {
		while(y1 <= y2) {
			lio_line(lio, (SINT16)x1, (SINT16)x2, (SINT16)y1, lp->pal);
			y1++;
		}
		return;
	}

	planes = gettileplanes(lio);
	while(y1 <= y2) {
		for (x=x1; x<=x2; x++) {
			pal = gbox_tilepal(lio, tile, leng, x, y1, planes);
			lio_pset(lio, (SINT16)x, (SINT16)y1, pal);
		}
		y1++;
	}
}



// ---- CLS

REG8 lio_gcls(GLIO lio) {

	LINEPT	lp;

	lio_updatedraw(lio);
	lp.x1 = lio->draw.x1;
	lp.y1 = lio->draw.y1;
	lp.x2 = lio->draw.x2;
	lp.y2 = lio->draw.y2;
	lp.pal = lio->work.bgcolor;
	gbox(lio, &lp, NULL, 0);
	return(LIO_SUCCESS);
}


// ----

REG8 lio_gline(GLIO lio) {

	GLINE	dat;
	LINEPT	lp;
	UINT16	pat;
	UINT	leng;
//	UINT	lengmin;
	UINT8	tile[256];

	lio_updatedraw(lio);
	MEMR_READS(CPU_DS, CPU_BX, &dat, sizeof(dat));
	lp.x1 = (SINT16)LOADINTELWORD(dat.x1);
	lp.y1 = (SINT16)LOADINTELWORD(dat.y1);
	lp.x2 = (SINT16)LOADINTELWORD(dat.x2);
	lp.y2 = (SINT16)LOADINTELWORD(dat.y2);

	TRACEOUT(("lio_gline %d,%d-%d,%d [%d]", lp.x1, lp.y1, lp.x2, lp.y2, dat.type));

	if (dat.pal == 0xff) {
		dat.pal = lio->work.fgcolor;
	}
	if ((dat.pal >= lio->draw.palmax) || (dat.sw > 2)) {
		goto gline_err;
	}
	pat = 0xffff;
	if (dat.type < 2) {
		/* sw=2�i�^�C���p�^�[���j�͓h��Ԃ��̎��̂ݗL�� */
		if (dat.sw == 2) {
			goto gline_err;
		}
		if (dat.sw == 1) {
			pat = (GDCPATREVERSE(dat.style[0]) << 8) +
											GDCPATREVERSE(dat.style[1]);
		}
		lp.pal = dat.pal;
		if (dat.type == 0) {
			gline(lio, &lp, pat);
		}
		else {
			glineb(lio, &lp, pat);
		}
	}
	else if (dat.type == 2) {
		leng = 0;
		if (dat.sw == 2) {
			leng = dat.patleng;
			if (!checktileleng(lio, leng)) {
				goto gline_err;
			}
			MEMR_READS(LOADINTELWORD(dat.seg), LOADINTELWORD(dat.off),
												tile, leng);
		}
		if (dat.sw != 1) {
			lp.pal = dat.pal;
			gbox(lio, &lp, tile, leng);
		}
		else {
			if (dat.style[0] == 0xff) {
				dat.style[0] = lio->work.fgcolor;
				if (dat.style[0] >= lio->draw.palmax) {
					dat.style[0] &= (UINT8)(lio->draw.palmax - 1);
				}
				lp.pal = dat.style[0];
				gbox(lio, &lp, tile, leng);
			}
			else {
				if (dat.style[0] >= lio->draw.palmax) {
					goto gline_err;
				}
				lp.pal = dat.style[0];
				gbox(lio, &lp, tile, leng);
				lp.pal = dat.pal;
				glineb(lio, &lp, 0xffff);
			}
		}
	}
	else {
		goto gline_err;
	}
	return(LIO_SUCCESS);

gline_err:
	return(LIO_ILLEGALFUNC);
}

