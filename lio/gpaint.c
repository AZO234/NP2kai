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
	UINT8	x[2];
	UINT8	y[2];
	UINT8	pal;
	UINT8	bdpal;
	UINT8	workend[2];
	UINT8	workstart[2];
} GPAINT1;

typedef struct {
	UINT8	x[2];
	UINT8	y[2];
	UINT8	dummy04;
	UINT8	patleng;
	UINT8	off[2];
	UINT8	seg[2];
	UINT8	bdpal;
	UINT8	dummy0b[5];
	UINT8	workend[2];
	UINT8	workstart[2];
} GPAINT2;

typedef struct {
	SINT16	x;
	SINT16	y;
} PAINTPT;

typedef struct {
	UINT8	type;
	UINT8	pal;
	UINT8	bdpal;
	UINT8	pat[256];
	UINT	patleng;
	UINT	planes;
	UINT	rows;
} PAINTCTX;

enum {
	PAINT_SOLID = 0,
	PAINT_TILE = 1
};

static UINT8 paintmark[(640 * 400) >> 3];

static UINT paint_colorbits(const _GLIO *lio) {

	return((lio->draw.flag & LIODRAW_4BPP)?4:3);
}

static UINT paint_planes(const _GLIO *lio) {

	if (lio->draw.flag & LIODRAW_MONO) {
		return(1);
	}
	return(paint_colorbits(lio));
}

static UINT paint_fullheight(const _GLIO *lio) {

	if ((lio->work.scrnmode == 0) || (lio->work.scrnmode == 1)) {
		return(200);
	}
	return(400);
}

static UINT paint_pageoff(const _GLIO *lio) {

	return((lio->draw.flag & LIODRAW_UPPER)?16000:0);
}

static REG8 paint_getpixel(const _GLIO *lio, SINT16 x, SINT16 y) {

	UINT	addr;
	UINT	sft;
	UINT	pl;
	REG8	ret;
const UINT8	*ptr;

	if ((x < lio->draw.x1) || (x > lio->draw.x2) ||
		(y < lio->draw.y1) || (y > lio->draw.y2)) {
		return(0xff);
	}
	addr = (UINT)((y * 80) + (x >> 3));
	if (lio->draw.flag & LIODRAW_UPPER) {
		addr += 16000;
	}
	addr += lio->draw.base;
	sft = (~x) & 7;
	ret = 0;
	if (!(lio->draw.flag & LIODRAW_MONO)) {
		for (pl=0; pl<3; pl++) {
			ptr = mem + addr + lioplaneadrs[pl];
			ret |= (REG8)((((*ptr) >> sft) & 1) << pl);
		}
		if (lio->draw.flag & LIODRAW_4BPP) {
			ptr = mem + addr + lioplaneadrs[3];
			ret |= (REG8)((((*ptr) >> sft) & 1) << 3);
		}
	}
	else {
		ptr = mem + addr + lioplaneadrs[lio->draw.flag & LIODRAW_PMASK];
		ret = (REG8)(((*ptr) >> sft) & 1);
	}
	return(ret);
}

static int paint_is_marked(SINT16 x, SINT16 y) {

	UINT	p;

	p = ((UINT)y * 640) + (UINT)x;
	return((paintmark[p >> 3] >> (p & 7)) & 1);
}

static void paint_set_mark(SINT16 x, SINT16 y) {

	UINT	p;

	p = ((UINT)y * 640) + (UINT)x;
	paintmark[p >> 3] |= (UINT8)(1 << (p & 7));
}

static int paint_canfill(const _GLIO *lio, const PAINTCTX *ctx,
										SINT16 x, SINT16 y) {

	if ((x < lio->draw.x1) || (x > lio->draw.x2) ||
		(y < lio->draw.y1) || (y > lio->draw.y2)) {
		return(0);
	}
	if (paint_is_marked(x, y)) {
		return(0);
	}
	return(paint_getpixel(lio, x, y) != ctx->bdpal);
}

static REG8 paint_tilepal(const _GLIO *lio, const PAINTCTX *ctx,
										SINT16 x, SINT16 y) {

	UINT	row;
	UINT	bit;
	UINT	pl;
	REG8	pal;

	if (!ctx->rows) {
		return(0);
	}
	row = ((UINT)(y - lio->draw.y1)) % ctx->rows;
	bit = 0x80 >> ((x - lio->draw.x1) & 7);
	pal = 0;
	if (lio->draw.flag & LIODRAW_MONO) {
		pal = (ctx->pat[row] & bit)?1:0;
	}
	else {
		for (pl=0; pl<ctx->planes; pl++) {
			if (ctx->pat[(row * ctx->planes) + pl] & bit) {
				pal |= (REG8)(1 << pl);
			}
		}
	}
	return(pal);
}

static void paint_putpixel(const _GLIO *lio, const PAINTCTX *ctx,
										SINT16 x, SINT16 y) {

	REG8	pal;

	if (ctx->type == PAINT_TILE) {
		pal = paint_tilepal(lio, ctx, x, y);
	}
	else {
		pal = ctx->pal;
	}
	lio_pset(lio, x, y, pal);
}

static int paint_stack_push(UINT workstart, UINT workend, UINT *sp,
										SINT16 x, SINT16 y) {

	UINT	off;

	off = workstart + ((*sp) << 2);
	if ((off + 4) > workend) {
		return(0);
	}
	MEMR_WRITE16(CPU_DS, off, (REG16)x);
	MEMR_WRITE16(CPU_DS, off + 2, (REG16)y);
	(*sp)++;
	return(1);
}

static int paint_stack_pop(UINT workstart, UINT *sp, PAINTPT *pt) {

	UINT	off;

	if (!(*sp)) {
		return(0);
	}
	(*sp)--;
	off = workstart + ((*sp) << 2);
	pt->x = (SINT16)MEMR_READ16(CPU_DS, off);
	pt->y = (SINT16)MEMR_READ16(CPU_DS, off + 2);
	return(1);
}

static REG8 paint_fill(GLIO lio, const PAINTCTX *ctx,
								SINT16 sx, SINT16 sy,
								UINT workstart, UINT workend) {

	UINT	sp;
	PAINTPT	pt;
	SINT16	x;
	SINT16	y;
	SINT16	xl;
	SINT16	xr;
	SINT16	xs;
	SINT16	xx;
	int		inrun;

	if ((sx < lio->draw.x1) || (sx > lio->draw.x2) ||
		(sy < lio->draw.y1) || (sy > lio->draw.y2)) {
		return(LIO_ILLEGALFUNC);
	}
	if ((workend <= workstart) || ((workend - workstart) < 16)) {
		return(LIO_ILLEGALFUNC);
	}
	ZeroMemory(paintmark, sizeof(paintmark));
	if (!paint_canfill(lio, ctx, sx, sy)) {
		return(LIO_SUCCESS);
	}
	sp = 0;
	if (!paint_stack_push(workstart, workend, &sp, sx, sy)) {
		return(LIO_OUTOFMEMORY);
	}
	while(paint_stack_pop(workstart, &sp, &pt)) {
		x = pt.x;
		y = pt.y;
		if (!paint_canfill(lio, ctx, x, y)) {
			continue;
		}
		xl = x;
		while((xl > lio->draw.x1) && paint_canfill(lio, ctx, (SINT16)(xl - 1), y)) {
			xl--;
		}
		xr = x;
		while((xr < lio->draw.x2) && paint_canfill(lio, ctx, (SINT16)(xr + 1), y)) {
			xr++;
		}
		for (xx=xl; xx<=xr; xx++) {
			paint_set_mark(xx, y);
			paint_putpixel(lio, ctx, xx, y);
		}
		if (y > lio->draw.y1) {
			xs = xl;
			inrun = 0;
			for (xx=xl; xx<=xr; xx++) {
				if (paint_canfill(lio, ctx, xx, (SINT16)(y - 1))) {
					if (!inrun) {
						xs = xx;
						inrun = 1;
					}
				}
				else if (inrun) {
					if (!paint_stack_push(workstart, workend, &sp, xs, (SINT16)(y - 1))) {
						return(LIO_OUTOFMEMORY);
					}
					inrun = 0;
				}
			}
			if (inrun) {
				if (!paint_stack_push(workstart, workend, &sp, xs, (SINT16)(y - 1))) {
					return(LIO_OUTOFMEMORY);
				}
			}
		}
		if (y < lio->draw.y2) {
			xs = xl;
			inrun = 0;
			for (xx=xl; xx<=xr; xx++) {
				if (paint_canfill(lio, ctx, xx, (SINT16)(y + 1))) {
					if (!inrun) {
						xs = xx;
						inrun = 1;
					}
				}
				else if (inrun) {
					if (!paint_stack_push(workstart, workend, &sp, xs, (SINT16)(y + 1))) {
						return(LIO_OUTOFMEMORY);
					}
					inrun = 0;
				}
			}
			if (inrun) {
				if (!paint_stack_push(workstart, workend, &sp, xs, (SINT16)(y + 1))) {
					return(LIO_OUTOFMEMORY);
				}
			}
		}
	}
	lio->wait += (UINT32)(lio->draw.x2 - lio->draw.x1 + 1) *
				(UINT32)(lio->draw.y2 - lio->draw.y1 + 1);
	return(LIO_SUCCESS);
}


REG8 lio_gpaint1(GLIO lio) {

	GPAINT1	dat;
	PAINTCTX	ctx;
	SINT16	x;
	SINT16	y;
	UINT	workstart;
	UINT	workend;

	lio_updatedraw(lio);
	MEMR_READS(CPU_DS, CPU_BX, &dat, sizeof(dat));
	x = (SINT16)LOADINTELWORD(dat.x);
	y = (SINT16)LOADINTELWORD(dat.y);
	ctx.type = PAINT_SOLID;
	ctx.pal = dat.pal;
	if (ctx.pal == 0xff) {
		ctx.pal = lio->work.fgcolor;
	}
	if (ctx.pal >= lio->draw.palmax) {
		return(LIO_ILLEGALFUNC);
	}
	ctx.bdpal = dat.bdpal;
	if (ctx.bdpal == 0xff) {
		ctx.bdpal = ctx.pal;
	}
	if (ctx.bdpal >= lio->draw.palmax) {
		return(LIO_ILLEGALFUNC);
	}
	ctx.patleng = 0;
	ctx.planes = 0;
	ctx.rows = 0;
	workend = LOADINTELWORD(dat.workend);
	workstart = LOADINTELWORD(dat.workstart);
	return(paint_fill(lio, &ctx, x, y, workstart, workend));
}

REG8 lio_gpaint2(GLIO lio) {

	GPAINT2	dat;
	PAINTCTX	ctx;
	SINT16	x;
	SINT16	y;
	UINT	workstart;
	UINT	workend;
	UINT	tileoff;
	UINT	tileseg;

	lio_updatedraw(lio);
	MEMR_READS(CPU_DS, CPU_BX, &dat, sizeof(dat));
	x = (SINT16)LOADINTELWORD(dat.x);
	y = (SINT16)LOADINTELWORD(dat.y);
	ctx.type = PAINT_TILE;
	ctx.pal = 0;
	ctx.bdpal = dat.bdpal;
	if (ctx.bdpal == 0xff) {
		ctx.bdpal = lio->work.fgcolor;
	}
	if (ctx.bdpal >= lio->draw.palmax) {
		return(LIO_ILLEGALFUNC);
	}
	ctx.planes = paint_planes(lio);
	ctx.patleng = dat.patleng;
	if ((ctx.patleng < ctx.planes) || (ctx.planes == 0) ||
		(ctx.patleng % ctx.planes)) {
		return(LIO_ILLEGALFUNC);
	}
	ctx.rows = ctx.patleng / ctx.planes;
	tileoff = LOADINTELWORD(dat.off);
	tileseg = LOADINTELWORD(dat.seg);
	MEMR_READS(tileseg, tileoff, ctx.pat, ctx.patleng);
	workend = LOADINTELWORD(dat.workend);
	workstart = LOADINTELWORD(dat.workstart);
	return(paint_fill(lio, &ctx, x, y, workstart, workend));
}



