#include	"compiler.h"
#include	"cpucore.h"
#include	"lio.h"
#include	<math.h>

#ifndef M_PI
#define M_PI	3.14159265358979323846
#endif

#define GCIRCLE_F_START       0x01
#define GCIRCLE_F_STARTLINE   0x02
#define GCIRCLE_F_END         0x04
#define GCIRCLE_F_ENDLINE     0x08
#define GCIRCLE_F_SAMEPOINT   0x10
#define GCIRCLE_F_FILL        0x20
#define GCIRCLE_F_TILE        0x40
#define GCIRCLE_F_RESERVED    0x80

typedef struct {
	UINT8	cx[2];
	UINT8	cy[2];
	UINT8	rx[2];
	UINT8	ry[2];
	UINT8	pal;
	UINT8	flag;
	UINT8	sx[2];
	UINT8	sy[2];
	UINT8	ex[2];
	UINT8	ey[2];
	UINT8	pat;
	UINT8	off[2];
	UINT8	seg[2];
} GCIRCLE;

typedef struct {
	long	x;
	long	y;
} LIOVEC;

typedef struct {
	SINT16	x1;
	SINT16	x2;
	UINT8	used;
} GCSPAN;


// ---- ヘルパー

static int gc_posmod(int v, int m) {

	int		r;

	r = v % m;
	if (r < 0) {
		r += m;
	}
	return(r);
}

// 座標系は数学と同じ右上が正
static LIOVEC gc_vec_from_point(SINT16 cx, SINT16 cy, SINT16 x, SINT16 y) {

	LIOVEC	v;

	v.x = (long)x - (long)cx;
	v.y = (long)cy - (long)y;
	if ((v.x == 0) && (v.y == 0)) {
		v.x = 1;
	}
	return(v);
}

static int gc_half(const LIOVEC *v) {

	return((v->y < 0) || ((v->y == 0) && (v->x < 0)));
}

static int gc_anglecmp(const LIOVEC *a, const LIOVEC *b) {

	int		ha;
	int		hb;
	double	cr;

	ha = gc_half(a);
	hb = gc_half(b);
	if (ha != hb) {
		return(ha - hb);
	}
	cr = ((double)a->x * (double)b->y) - ((double)a->y * (double)b->x);
	if (cr > 0) {
		return(-1);
	}
	if (cr < 0) {
		return(1);
	}
	return(0);
}

static int gc_angle_le(const LIOVEC *a, const LIOVEC *b) {

	return(gc_anglecmp(a, b) <= 0);
}

static int gc_same_dir(const LIOVEC *a, const LIOVEC *b) {

	double	cr;
	double	dot;

	cr = ((double)a->x * (double)b->y) - ((double)a->y * (double)b->x);
	dot = ((double)a->x * (double)b->x) + ((double)a->y * (double)b->y);
	return((cr == 0) && (dot > 0));
}

static int gc_arc_contains(SINT16 cx, SINT16 cy, SINT16 x, SINT16 y,
										const LIOVEC *sv, const LIOVEC *ev,
										int usearc, int fullarc, int onepoint) {

	LIOVEC	pv;
	int		se;

	if (!usearc || fullarc) {
		return(1);
	}
	if ((x == cx) && (y == cy)) {
		return(!onepoint);
	}
	pv = gc_vec_from_point(cx, cy, x, y);
	if (onepoint) {
		return(gc_same_dir(sv, &pv));
	}
	se = gc_anglecmp(sv, ev);
	if (se <= 0) {
		return(gc_angle_le(sv, &pv) && gc_angle_le(&pv, ev));
	}
	return(gc_angle_le(sv, &pv) || gc_angle_le(&pv, ev));
}

static void gc_drawline(const _GLIO *lio, SINT16 x1, SINT16 y1,
										SINT16 x2, SINT16 y2, REG8 pal) {

	int		dx;
	int		dy;
	int		sx;
	int		sy;
	int		err;
	int		e2;

	dx = x2 - x1;
	if (dx < 0) {
		dx = 0 - dx;
	}
	dy = y2 - y1;
	if (dy < 0) {
		dy = 0 - dy;
	}
	sx = (x1 < x2) ? 1 : -1;
	sy = (y1 < y2) ? 1 : -1;
	err = dx - dy;
	for (;;) {
		lio_pset(lio, x1, y1, pal);
		if ((x1 == x2) && (y1 == y2)) {
			break;
		}
		e2 = err << 1;
		if (e2 > -dy) {
			err -= dy;
			x1 = (SINT16)(x1 + sx);
		}
		if (e2 < dx) {
			err += dx;
			y1 = (SINT16)(y1 + sy);
		}
	}
}


#define GCIRC_RND(v)   (((v) >= 0.0) ? (SINT16)((v) + 0.5) : (SINT16)((v) - 0.5))

static void gc_draw4(const _GLIO *lio, SINT16 x, SINT16 y,
                                            SINT16 d1, SINT16 d2, REG8 pal,
                                            UINT32 *waitcnt) {

    SINT16  x1;
    SINT16  x2;
    SINT16  y1;
    SINT16  y2;

    x1 = (SINT16)(x - d1);
    x2 = (SINT16)(x + d1);
    y1 = (SINT16)(y - d2);
    y2 = (SINT16)(y + d2);
    lio_pset(lio, x1, y1, pal);
    lio_pset(lio, x1, y2, pal);
    lio_pset(lio, x2, y1, pal);
    lio_pset(lio, x2, y2, pal);
    *waitcnt += 4;
}

static void gc_draw_circle_outline(const _GLIO *lio, SINT16 cx, SINT16 cy,
                                            SINT16 r, REG8 pal,
                                            UINT32 *waitcnt) {

    SINT16  d1;
    SINT16  d2;
    SINT16  d3;

    d1 = 0;
    d2 = r;
    d3 = (SINT16)(0 - r);
    while (d1 <= d2) {
        gc_draw4(lio, cx, cy, d1, d2, pal, waitcnt);
        gc_draw4(lio, cx, cy, d2, d1, pal, waitcnt);
        d1++;
        d3 = (SINT16)(d3 + (d1 * 2) - 1);
        if (d3 >= 0) {
            d2--;
            d3 = (SINT16)(d3 - (d2 * 2));
        }
    }
}

static double gc_angle_from_point(SINT16 cx, SINT16 cy, SINT16 rx, SINT16 ry,
                                            SINT16 x, SINT16 y) {

    double  ax;
    double  ay;

    if ((rx == 0) || (ry == 0)) {
        return(0.0);
    }
    ax = (double)(x - cx) * (double)ry;
    ay = (double)(0 - (y - cy)) * (double)rx;
    return(atan2(ay, ax));
}

static void gc_draw_parametric_outline(const _GLIO *lio,
                                            SINT16 cx, SINT16 cy,
                                            SINT16 rx, SINT16 ry,
                                            SINT16 sx, SINT16 sy,
                                            SINT16 ex, SINT16 ey,
                                            int usearc, int fullarc,
                                            REG8 pal, UINT32 *waitcnt) {

    int     maxr;
    int     n;
    int     i;
    double  a0;
    double  a1;
    double  a;
    SINT16  px;
    SINT16  py;

    if ((rx == 0) && (ry == 0)) {
        lio_pset(lio, cx, cy, pal);
        (*waitcnt)++;
        return;
    }
    if (rx == 0) {
        gc_drawline(lio, cx, (SINT16)(cy - ry), cx, (SINT16)(cy + ry), pal);
        *waitcnt += (UINT32)(ry * 2 + 1);
        return;
    }
    if (ry == 0) {
        gc_drawline(lio, (SINT16)(cx - rx), cy, (SINT16)(cx + rx), cy, pal);
        *waitcnt += (UINT32)(rx * 2 + 1);
        return;
    }

    a0 = 0.0;
    a1 = 2.0 * M_PI;
    if (usearc && !fullarc) {
        a0 = gc_angle_from_point(cx, cy, rx, ry, sx, sy);
        a1 = gc_angle_from_point(cx, cy, rx, ry, ex, ey);
        while (a1 < (a0 + 1e-9)) {
            a1 += 2.0 * M_PI;
        }
    }

    maxr = (rx > ry) ? rx : ry;
    n = (int)((a1 - a0) * (double)maxr) + 1;
    if (n < 1) {
        n = 1;
    }
    for (i=0; i<=n; i++) {
        a = a0 + (a1 - a0) * (double)i / (double)n;
        px = (SINT16)(cx + GCIRC_RND((double)rx * cos(a)));
        py = (SINT16)(cy - GCIRC_RND((double)ry * sin(a)));
        lio_pset(lio, px, py, pal);
        (*waitcnt)++;
    }
}

static void gc_span_clear(GCSPAN *spans, int height) {

	int		i;

	for (i=0; i<height; i++) {
		spans[i].x1 = 0;
		spans[i].x2 = 0;
		spans[i].used = 0;
	}
}

static void gc_span_add(GCSPAN *spans, int ytop, int ybottom,
											SINT16 x, SINT16 y) {

	int		idx;

	if ((y < ytop) || (y > ybottom)) {
		return;
	}
	idx = (int)y - ytop;
	if (!spans[idx].used) {
		spans[idx].x1 = x;
		spans[idx].x2 = x;
		spans[idx].used = 1;
	}
	else {
		if (x < spans[idx].x1) {
			spans[idx].x1 = x;
		}
		if (x > spans[idx].x2) {
			spans[idx].x2 = x;
		}
	}
}

static void gc_span_add4(GCSPAN *spans, int ytop, int ybottom,
					SINT16 x, SINT16 y, SINT16 d1, SINT16 d2) {

	gc_span_add(spans, ytop, ybottom, (SINT16)(x - d1), (SINT16)(y - d2));
	gc_span_add(spans, ytop, ybottom, (SINT16)(x - d1), (SINT16)(y + d2));
	gc_span_add(spans, ytop, ybottom, (SINT16)(x + d1), (SINT16)(y - d2));
	gc_span_add(spans, ytop, ybottom, (SINT16)(x + d1), (SINT16)(y + d2));
}

static void gc_make_circle_spans(GCSPAN *spans, int ytop, int ybottom,
									SINT16 cx, SINT16 cy, SINT16 r) {

	SINT16	d1;
	SINT16	d2;
	SINT16	d3;

	d1 = 0;
	d2 = r;
	d3 = (SINT16)(0 - r);
	while (d1 <= d2) {
		gc_span_add4(spans, ytop, ybottom, cx, cy, d1, d2);
		gc_span_add4(spans, ytop, ybottom, cx, cy, d2, d1);
		d1++;
		d3 = (SINT16)(d3 + (d1 * 2) - 1);
		if (d3 >= 0) {
			d2--;
			d3 = (SINT16)(d3 - (d2 * 2));
		}
	}
}

static void gc_span_line(GCSPAN *spans, int ytop, int ybottom,
								SINT16 x1, SINT16 y1, SINT16 x2, SINT16 y2) {

	int		dx;
	int		dy;
	int		sx;
	int		sy;
	int		err;
	int		e2;

	dx = x2 - x1;
	if (dx < 0) {
		dx = 0 - dx;
	}
	dy = y2 - y1;
	if (dy < 0) {
		dy = 0 - dy;
	}
	sx = (x1 < x2) ? 1 : -1;
	sy = (y1 < y2) ? 1 : -1;
	err = dx - dy;
	for (;;) {
		gc_span_add(spans, ytop, ybottom, x1, y1);
		if ((x1 == x2) && (y1 == y2)) {
			break;
		}
		e2 = err << 1;
		if (e2 > -dy) {
			err -= dy;
			x1 = (SINT16)(x1 + sx);
		}
		if (e2 < dx) {
			err += dx;
			y1 = (SINT16)(y1 + sy);
		}
	}
}

static void gc_make_parametric_spans(GCSPAN *spans, int ytop, int ybottom,
                                            SINT16 cx, SINT16 cy,
                                            SINT16 rx, SINT16 ry,
                                            SINT16 sx, SINT16 sy,
                                            SINT16 ex, SINT16 ey,
                                            int usearc, int fullarc) {

    int     maxr;
    int     n;
    int     i;
    double  a0;
    double  a1;
    double  a;
    SINT16  px;
    SINT16  py;

    if ((rx == 0) && (ry == 0)) {
        gc_span_add(spans, ytop, ybottom, cx, cy);
        return;
    }
    if (rx == 0) {
        for (py=(SINT16)(cy - ry); py<=(SINT16)(cy + ry); py++) {
            gc_span_add(spans, ytop, ybottom, cx, py);
        }
        return;
    }
    if (ry == 0) {
        gc_span_add(spans, ytop, ybottom, (SINT16)(cx - rx), cy);
        gc_span_add(spans, ytop, ybottom, (SINT16)(cx + rx), cy);
        return;
    }

    a0 = 0.0;
    a1 = 2.0 * M_PI;
    if (usearc && !fullarc) {
        a0 = gc_angle_from_point(cx, cy, rx, ry, sx, sy);
        a1 = gc_angle_from_point(cx, cy, rx, ry, ex, ey);
        while (a1 < (a0 + 1e-9)) {
            a1 += 2.0 * M_PI;
        }
    }

    maxr = (rx > ry) ? rx : ry;
    n = (int)((a1 - a0) * (double)maxr) + 1;
    if (n < 1) {
        n = 1;
    }
    for (i=0; i<=n; i++) {
        a = a0 + (a1 - a0) * (double)i / (double)n;
        px = (SINT16)(cx + GCIRC_RND((double)rx * cos(a)));
        py = (SINT16)(cy - GCIRC_RND((double)ry * sin(a)));
        gc_span_add(spans, ytop, ybottom, px, py);
    }
}

static REG8 gc_tilepal(const _GLIO *lio, const GCIRCLE *dat,
								SINT16 x, SINT16 y, UINT planes) {

	UINT	pl;
	UINT	idx;
	UINT	seg;
	UINT	off;
	UINT8	bit;
	REG8	pal;

	seg = LOADINTELWORD(dat->seg);
	off = LOADINTELWORD(dat->off);
	bit = (UINT8)(0x80 >> gc_posmod((int)x - (int)lio->draw.x1, 8));
	pal = 0;
	for (pl=0; pl<planes; pl++) {
		idx = (UINT)gc_posmod(((int)y - (int)lio->draw.y1) * (int)planes +
														(int)pl, dat->pat);
		if (MEMR_READ8(seg, off + idx) & bit) {
			pal |= (REG8)(1 << pl);
		}
	}
	return(pal);
}

static REG8 gc_fillpal(const _GLIO *lio, const GCIRCLE *dat,
								SINT16 x, SINT16 y, REG8 arcpal) {

	UINT	planes;

	if (!(dat->flag & GCIRCLE_F_TILE)) {
		if (dat->pat == 0xff) {
			return( arcpal );
		}
		return(dat->pat);
	}
	planes = 1;
	if (!(lio->draw.flag & LIODRAW_MONO)) {
		planes = (lio->draw.flag & LIODRAW_4BPP) ? 4 : 3;
	}
	return(gc_tilepal(lio, dat, x, y, planes));
}


// ---- GCIRCLE

REG8 lio_gcircle(GLIO lio) {

	GCIRCLE	dat;
	SINT16	cx;
	SINT16	cy;
	SINT16	rx;
	SINT16	ry;
	SINT16	sx;
	SINT16	sy;
	SINT16	ex;
	SINT16	ey;
	SINT16	y;
	REG8	pal;
	REG8	fpal;
	LIOVEC	sv;
	LIOVEC	ev;
	int		usearc;
	int		fullarc;
	int		onepoint;
	int		fill;
	int		planes;
	UINT32	waitcnt;

	lio_updatedraw(lio);
	MEMR_READS(CPU_DS, CPU_BX, &dat, sizeof(dat));

	if (dat.flag & GCIRCLE_F_RESERVED) {
		goto gcircle_err;
	}
	cx = (SINT16)LOADINTELWORD(dat.cx);
	cy = (SINT16)LOADINTELWORD(dat.cy);
	rx = (SINT16)LOADINTELWORD(dat.rx);
	ry = (SINT16)LOADINTELWORD(dat.ry);
	if ((rx < 0) || (ry < 0)) {
		goto gcircle_err;
	}
	pal = dat.pal;
	if (pal == 0xff) {
		pal = lio->work.fgcolor;
	}
	if (pal >= lio->draw.palmax) {
		goto gcircle_err;
	}
	if ((dat.flag & GCIRCLE_F_FILL) && !(dat.flag & GCIRCLE_F_TILE) &&
		(dat.pat != 0xff) && (dat.pat >= lio->draw.palmax)) {
		goto gcircle_err;
	}
	if ((dat.flag & GCIRCLE_F_FILL) && (dat.flag & GCIRCLE_F_TILE)) {
		planes = 1;
		if (!(lio->draw.flag & LIODRAW_MONO)) {
			planes = (lio->draw.flag & LIODRAW_4BPP) ? 4 : 3;
		}
		if (dat.pat < planes) {
			goto gcircle_err;
		}
	}

	sx = (dat.flag & GCIRCLE_F_START) ?
			(SINT16)LOADINTELWORD(dat.sx) : (SINT16)(cx + rx);
	sy = (dat.flag & GCIRCLE_F_START) ?
			(SINT16)LOADINTELWORD(dat.sy) : cy;
	ex = (dat.flag & GCIRCLE_F_END) ?
			(SINT16)LOADINTELWORD(dat.ex) : (SINT16)(cx + rx);
	ey = (dat.flag & GCIRCLE_F_END) ?
			(SINT16)LOADINTELWORD(dat.ey) : cy;
	sv = gc_vec_from_point(cx, cy, sx, sy);
	ev = gc_vec_from_point(cx, cy, ex, ey);
	usearc = (dat.flag & (GCIRCLE_F_START | GCIRCLE_F_END)) != 0;
	onepoint = 0;
	fullarc = !usearc;
	if (usearc && gc_same_dir(&sv, &ev)) {
		if (dat.flag & GCIRCLE_F_SAMEPOINT) {
			onepoint = 1;
			fullarc = 0;
		}
		else {
			fullarc = 1;
		}
	}

	waitcnt = 0;
	fill = (dat.flag & GCIRCLE_F_FILL) && !onepoint;
	if (fill) {
		int		ytop;
		int		ybottom;
		int		height;
		int		idx;
		SINT16	x1;
		SINT16	x2;
		SINT16	fx;
		GCSPAN	*spans;

		ytop = cy - ry;
		ybottom = cy + ry;
		if (ytop < lio->draw.y1) {
			ytop = lio->draw.y1;
		}
		if (ybottom > lio->draw.y2) {
			ybottom = lio->draw.y2;
		}
		if (ytop <= ybottom) {
			height = ybottom - ytop + 1;
			spans = (GCSPAN *)malloc(sizeof(GCSPAN) * height);
			if (spans == NULL) {
				return(LIO_OUTOFMEMORY);
			}
			gc_span_clear(spans, height);
			if (!usearc && (rx == ry)) {
				gc_make_circle_spans(spans, ytop, ybottom, cx, cy, rx);
			}
			else {
				gc_make_parametric_spans(spans, ytop, ybottom, cx, cy,
											rx, ry, sx, sy, ex, ey, usearc, fullarc);
				if (usearc && !fullarc) {
					gc_span_line(spans, ytop, ybottom, cx, cy, sx, sy);
					gc_span_line(spans, ytop, ybottom, cx, cy, ex, ey);
				}
			}
			for (idx=0; idx<height; idx++) {
				if (!spans[idx].used) {
					continue;
				}
				y = (SINT16)(ytop + idx);
				x1 = spans[idx].x1;
				x2 = spans[idx].x2;
				if (x1 > x2) {
					continue;
				}
				if (!usearc || fullarc) {
					if (!(dat.flag & GCIRCLE_F_TILE)) {
						fpal = gc_fillpal(lio, &dat, x1, y, pal);
						lio_line(lio, x1, x2, y, fpal);
						if (x1 < lio->draw.x1) {
							x1 = lio->draw.x1;
						}
						if (x2 > lio->draw.x2) {
							x2 = lio->draw.x2;
						}
						if (x1 <= x2) {
							waitcnt += (UINT32)(x2 - x1 + 1);
						}
					}
					else {
						if (x1 < lio->draw.x1) {
							x1 = lio->draw.x1;
						}
						if (x2 > lio->draw.x2) {
							x2 = lio->draw.x2;
						}
						for (fx=x1; fx<=x2; fx++) {
							fpal = gc_fillpal(lio, &dat, fx, y, pal);
							lio_pset(lio, fx, y, fpal);
							waitcnt++;
						}
					}
				}
				else {
					if (x1 < lio->draw.x1) {
						x1 = lio->draw.x1;
					}
					if (x2 > lio->draw.x2) {
						x2 = lio->draw.x2;
					}
					for (fx=x1; fx<=x2; fx++) {
						if (gc_arc_contains(cx, cy, fx, y, &sv, &ev, usearc,
														fullarc, onepoint)) {
							fpal = gc_fillpal(lio, &dat, fx, y, pal);
							lio_pset(lio, fx, y, fpal);
							waitcnt++;
						}
					}
				}
			}
			free(spans);
		}
	}

	if (onepoint) {
		lio_pset(lio, sx, sy, pal);
		waitcnt++;
	}
	else if (!usearc && (rx == ry)) {
		gc_draw_circle_outline(lio, cx, cy, rx, pal, &waitcnt);
	}
	else {
		gc_draw_parametric_outline(lio, cx, cy, rx, ry, sx, sy, ex, ey,
										usearc, fullarc, pal, &waitcnt);
	}


	if ((dat.flag & GCIRCLE_F_STARTLINE) && !fullarc) {
		gc_drawline(lio, cx, cy, sx, sy, pal);
	}
	if ((dat.flag & GCIRCLE_F_ENDLINE) && !fullarc) {
		gc_drawline(lio, cx, cy, ex, ey, pal);
	}
	lio->wait += waitcnt * (10 + 10 + 10);
	return(LIO_SUCCESS);

 gcircle_err:
	TRACEOUT(("LIO GCIRCLE illegal %.2x", dat.flag));
	return(LIO_ILLEGALFUNC);
}
