#include	"compiler.h"
#include	"scrnmng.h"
#include	"scrndraw.h"
#include	"sdraw.h"
#include	"palettes.h"


#if defined(SIZE_QVGA) && !defined(SIZE_VGATEST) && defined(SUPPORT_16BPP)

// vram off
static void SCRNCALL qvga16p_0(SDRAW sdraw, int maxy) {

	int		xbytes;
	UINT32	palwork;
	UINT16	pal;
	UINT8	*p;
	int		y;
	int		x;

	xbytes = sdraw->xalign * sdraw->width / 2;
	palwork = np2_pal16[NP2PAL_TEXT2];
	pal = (UINT16)(palwork + (palwork >> 16));
	p = sdraw->dst;
	y = sdraw->y;
	do {
		if (*(UINT16 *)(sdraw->dirty + y)) {
			for (x=0; x<sdraw->width; x+=2) {
				*(UINT16 *)p = pal;
				p += sdraw->xalign;
			}
			p -= xbytes;
		}
		p += sdraw->yalign;
		y += 2;
	} while(y < maxy);

	sdraw->dst = p;
	sdraw->y = y;
}

// text or grph 1プレーン
static void SCRNCALL qvga16p_1(SDRAW sdraw, int maxy) {

	int		xbytes;
const UINT8	*p;
	UINT8	*q;
	int		y;
	int		x;
	UINT32	work;

	xbytes = sdraw->xalign * sdraw->width / 2;
	p = sdraw->src;
	q = sdraw->dst;
	y = sdraw->y;
	do {
		if (*(UINT16 *)(sdraw->dirty + y)) {
			for (x=0; x<sdraw->width; x+=2) {
				work = np2_pal16[p[x+0] + NP2PAL_GRPH];
				work += np2_pal16[p[x+1] + NP2PAL_GRPH];
				work += np2_pal16[p[x+0+SURFACE_WIDTH] + NP2PAL_GRPH];
				work += np2_pal16[p[x+1+SURFACE_WIDTH] + NP2PAL_GRPH];
				work &= 0x07e0f81f << 2;
				*(UINT16 *)q = (UINT16)((work >> 2) + (work >> 18));
				q += sdraw->xalign;
			}
			q -= xbytes;
		}
		p += SURFACE_WIDTH * 2;
		q += sdraw->yalign;
		y += 2;
	} while(y < maxy);

	sdraw->src = p;
	sdraw->dst = q;
	sdraw->y = y;
}

// text + grph
static void SCRNCALL qvga16p_2(SDRAW sdraw, int maxy) {

	int		xbytes;
const UINT8	*p;
const UINT8	*q;
	UINT8	*r;
	int		y;
	int		x;
	UINT32	work;

	xbytes = sdraw->xalign * sdraw->width / 2;
	p = sdraw->src;
	q = sdraw->src2;
	r = sdraw->dst;
	y = sdraw->y;
	do {
		if (*(UINT16 *)(sdraw->dirty + y)) {
			for (x=0; x<sdraw->width; x+=2) {
				work = np2_pal16[p[x+0] + q[x+0] + NP2PAL_GRPH];
				work += np2_pal16[p[x+1] + q[x+1] + NP2PAL_GRPH];
				work += np2_pal16[p[x+0+SURFACE_WIDTH] +
										q[x+0+SURFACE_WIDTH] + NP2PAL_GRPH];
				work += np2_pal16[p[x+1+SURFACE_WIDTH] + 
										q[x+1+SURFACE_WIDTH] + NP2PAL_GRPH];
				work &= 0x07e0f81f << 2;
				*(UINT16 *)r = (UINT16)((work >> 2) + (work >> 18));
				r += sdraw->xalign;
			}
			r -= xbytes;
		}
		p += SURFACE_WIDTH * 2;
		q += SURFACE_WIDTH * 2;
		r += sdraw->yalign;
		y += 2;
	} while(y < maxy);

	sdraw->src = p;
	sdraw->src2 = q;
	sdraw->dst = r;
	sdraw->y = y;
}

// text + (grph:interleave) - > qvga16p_1

// grph:interleave
static void SCRNCALL qvga16p_gi(SDRAW sdraw, int maxy) {

	int		xbytes;
const UINT8	*p;
	UINT8	*q;
	int		y;
	int		x;
	UINT32	work;

	xbytes = sdraw->xalign * sdraw->width / 2;
	p = sdraw->src;
	q = sdraw->dst;
	y = sdraw->y;
	do {
		if (*(UINT16 *)(sdraw->dirty + y)) {
			for (x=0; x<sdraw->width; x+=2) {
				work = np2_pal16[p[x+0] + NP2PAL_GRPH];
				work += np2_pal16[p[x+1] + NP2PAL_GRPH];
				work &= 0x07e0f81f << 1;
				*(UINT16 *)q = (UINT16)((work >> 1) + (work >> 17));
				q += sdraw->xalign;
			}
			q -= xbytes;
		}
		p += SURFACE_WIDTH * 2;
		q += sdraw->yalign;
		y += 2;
	} while(y < maxy);

	sdraw->src = p;
	sdraw->dst = q;
	sdraw->y = y;
}

// text + grph:interleave
static void SCRNCALL qvga16p_2i(SDRAW sdraw, int maxy) {

	int		xbytes;
const UINT8	*p;
const UINT8	*q;
	UINT8	*r;
	int		y;
	int		x;
	UINT32	work;

	xbytes = sdraw->xalign * sdraw->width / 2;
	p = sdraw->src;
	q = sdraw->src2;
	r = sdraw->dst;
	y = sdraw->y;
	do {
		if (*(UINT16 *)(sdraw->dirty + y)) {
			for (x=0; x<sdraw->width; x+=2) {
				work = np2_pal16[p[x+0] + q[x+0] + NP2PAL_GRPH];
				work += np2_pal16[p[x+1] + q[x+1] + NP2PAL_GRPH];
				if (q[x+0+SURFACE_WIDTH] & 0xf0) {
					work += np2_pal16[(q[x+0+SURFACE_WIDTH] >> 4)
															+ NP2PAL_TEXT];
				}
				else {
					work += np2_pal16[p[x+0] + NP2PAL_GRPH];
				}
				if (q[x+1+SURFACE_WIDTH] & 0xf0) {
					work += np2_pal16[(q[x+1+SURFACE_WIDTH] >> 4)
															+ NP2PAL_TEXT];
				}
				else {
					work += np2_pal16[p[x+1] + NP2PAL_GRPH];
				}
				work &= 0x07e0f81f << 2;
				*(UINT16 *)r = (UINT16)((work >> 2) + (work >> 18));
				r += sdraw->xalign;
			}
			r -= xbytes;
		}
		p += SURFACE_WIDTH * 2;
		q += SURFACE_WIDTH * 2;
		r += sdraw->yalign;
		y += 2;
	} while(y < maxy);

	sdraw->src = p;
	sdraw->src2 = q;
	sdraw->dst = r;
	sdraw->y = y;
}

#if defined(SUPPORT_CRT15KHZ)
// text or grph 1プレーン (15kHz)
static void SCRNCALL qvga16p_1d(SDRAW sdraw, int maxy) {

	int		xbytes;
const UINT8	*p;
	UINT8	*q;
	int		y;
	int		x;
	UINT32	work;

	xbytes = sdraw->xalign * sdraw->width / 2;
	p = sdraw->src;
	q = sdraw->dst;
	y = sdraw->y;
	do {
		if (sdraw->dirty[y]) {
			for (x=0; x<sdraw->width; x+=2) {
				work = np2_pal16[p[x+0] + NP2PAL_GRPH];
				work += np2_pal16[p[x+1] + NP2PAL_GRPH];
				work &= 0x07e0f81f << 1;
				*(UINT16 *)q = (UINT16)((work >> 1) + (work >> 17));
				q += sdraw->xalign;
			}
			q -= xbytes;
		}
		p += SURFACE_WIDTH;
		q += sdraw->yalign;
	} while(++y < maxy);

	sdraw->src = p;
	sdraw->dst = q;
	sdraw->y = y;
}

// text + grph (15kHz)
static void SCRNCALL qvga16p_2d(SDRAW sdraw, int maxy) {

	int		xbytes;
const UINT8	*p;
const UINT8	*q;
	UINT8	*r;
	int		y;
	int		x;
	UINT32	work;

	xbytes = sdraw->xalign * sdraw->width / 2;
	p = sdraw->src;
	q = sdraw->src2;
	r = sdraw->dst;
	y = sdraw->y;
	do {
		if (sdraw->dirty[y]) {
			for (x=0; x<sdraw->width; x+=2) {
				work = np2_pal16[p[x+0] + q[x+0] + NP2PAL_GRPH];
				work += np2_pal16[p[x+1] + q[x+1] + NP2PAL_GRPH];
				work &= 0x07e0f81f << 1;
				*(UINT16 *)r = (UINT16)((work >> 1) + (work >> 17));
				r += sdraw->xalign;
			}
			r -= xbytes;
		}
		p += SURFACE_WIDTH;
		q += SURFACE_WIDTH;
		r += sdraw->yalign;
	} while(++y < maxy);

	sdraw->src = p;
	sdraw->src2 = q;
	sdraw->dst = r;
	sdraw->y = y;
}
#endif


static const SDRAWFN qvga16p[] = {
		qvga16p_0,		qvga16p_1,		qvga16p_1,		qvga16p_2,
		qvga16p_0,		qvga16p_1,		qvga16p_gi,		qvga16p_2i,
		qvga16p_0,		qvga16p_1,		qvga16p_gi,		qvga16p_2i,
#if defined(SUPPORT_CRT15KHZ)
		qvga16p_0,		qvga16p_1d,		qvga16p_1d,		qvga16p_2d,
#endif
	};

const SDRAWFN *sdraw_getproctbl(const SCRNSURF *surf) {

	if (surf->bpp == 16) {
		return(qvga16p);
	}
	else {
		return(NULL);
	}
}

#endif

