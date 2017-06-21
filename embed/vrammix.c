#include	"compiler.h"
#include	"fontmng.h"
#include	"vramhdl.h"
#include	"vrammix.h"


enum {
	VRAMALPHABASE	= (1 << VRAMALPHABIT) - VRAMALPHA,
	FDATDEPTHBASE	= (1 << FDAT_DEPTHBIT) - FDAT_DEPTH
};


static BRESULT cpyrect(MIX_RECT *r, const VRAMHDL dst, const POINT_T *pt, 
									const VRAMHDL src, const RECT_T *rct) {

	POINT_T	p;
	int		width;
	int		height;

	if ((dst == NULL) || (src == NULL)) {
		return(FAILURE);
	}
	if (pt) {
		p = *pt;
	}
	else {
		p.x = 0;
		p.y = 0;
	}
	r->srcpos = 0;
	if (rct) {
		width = min(rct->right, src->width);
		if (rct->left >= 0) {
			r->srcpos += rct->left;
			width -= rct->left;
		}
		else {
			p.x -= rct->left;
		}
		height = min(rct->bottom, src->height);
		if (rct->top >= 0) {
			r->srcpos += rct->top * src->width;
			height -= rct->top;
		}
		else {
			p.y -= rct->top;
		}
	}
	else {
		width = src->width;
		height = src->height;
	}

	r->dstpos = 0;
	r->width = min(width + p.x, dst->width);
	if (p.x > 0) {
		r->dstpos += p.x;
		r->width = min(r->width, dst->width) - p.x;
	}
	else {
		r->srcpos -= p.x;
	}
	if (r->width <= 0) {
		return(FAILURE);
	}

	r->height = min(height + p.y, dst->height);
	if (p.y > 0) {
		r->dstpos += p.y * dst->width;
		r->height = min(r->height, dst->height) - p.y;
	}
	else {
		r->srcpos -= p.y * src->width;
	}
	if (r->height <= 0) {
		return(FAILURE);
	}
	return(SUCCESS);
}

static BRESULT mixrect(MIX_RECT *r, const VRAMHDL dst, const RECT_T *rct,
									const VRAMHDL src, const POINT_T *pt) {

	int		pos;
	RECT_T	s;

	if ((dst == NULL) || (src == NULL)) {
		return(FAILURE);
	}
	r->srcpos = 0;
	if (rct == NULL) {
		s.left = 0;
		s.top = 0;
		s.right = dst->width;
		s.bottom = dst->height;
		r->dstpos = 0;
	}
	else {
		if ((rct->bottom <= 0) || (rct->right <= 0) ||
			(rct->left >= dst->width) || (rct->top >= dst->height)) {
			return(FAILURE);
		}
		s.left = max(rct->left, 0);
		s.top = max(rct->top, 0);
		s.right = min(rct->right, dst->width);
		s.bottom = min(rct->bottom, dst->height);
		if ((s.top >= s.bottom) || (s.left >= s.right)) {
			return(FAILURE);
		}
		r->dstpos = s.top * dst->width;
		r->dstpos += s.left;
	}

	pos = src->posy - s.top;
	if (pt) {
		pos += pt->y;
	}
	if (pos < 0) {
		r->srcpos -= pos * src->width;
		r->height = min(src->height + pos, s.bottom - s.top);
	}
	else {
		r->dstpos += pos * dst->width;
		r->height = min(s.bottom - s.top - pos, src->height);
	}
	if (r->height <= 0) {
		return(FAILURE);
	}

	pos = src->posx - s.left;
	if (pt) {
		pos += pt->x;
	}
	if (pos < 0) {
		r->srcpos -= pos;
		r->width = min(src->width + pos, s.right - s.left);
	}
	else {
		r->dstpos += pos;
		r->width = min(s.right - s.left - pos, src->width);
	}
	if (r->width <= 0) {
		return(FAILURE);
	}
	return(SUCCESS);
}


// ----

typedef struct {
	int		orgpos;
	int		srcpos;
	int		dstpos;
	int		width;
	int		height;
} MIXRECTEX;

static BRESULT cpyrectex(MIXRECTEX *r, const VRAMHDL dst, const POINT_T *pt,
					const VRAMHDL org, const VRAMHDL src, const RECT_T *rct) {

	POINT_T	p;
	int		width;
	int		height;
	int		dstwidth;
	int		dstheight;

	if ((dst == NULL) || (org == NULL) || (src == NULL) ||
		(dst->bpp != org->bpp) || (dst->bpp != src->bpp)) {
		return(FAILURE);
	}
	if (pt) {
		p = *pt;
	}
	else {
		p.x = 0;
		p.y = 0;
	}

	r->srcpos = 0;
	if (rct) {
		width = min(rct->right, src->width);
		if (rct->left >= 0) {
			r->srcpos += rct->left;
			width -= rct->left;
		}
		else {
			p.x -= rct->left;
		}
		height = min(rct->bottom, src->height);
		if (rct->top >= 0) {
			r->srcpos += rct->top * src->width;
			height -= rct->top;
		}
		else {
			p.y -= rct->top;
		}
	}
	else {
		width = src->width;
		height = src->height;
	}

	r->orgpos = 0;
	r->dstpos = 0;
	dstwidth = min(dst->width, org->width);
	r->width = min(width + p.x, dstwidth);
	if (p.x > 0) {
		r->orgpos += p.x;
		r->dstpos += p.x;
		r->width = min(r->width, dstwidth) - p.x;
	}
	else {
		r->srcpos -= p.x;
	}
	if (r->width <= 0) {
		return(FAILURE);
	}

	dstheight = min(dst->height, org->height);
	r->height = min(height + p.y, dstheight);
	if (p.y > 0) {
		r->orgpos += p.y * org->width;
		r->dstpos += p.y * dst->width;
		r->height = min(r->height, dstheight) - p.y;
	}
	else {
		r->srcpos -= p.y * src->width;
	}
	if (r->height <= 0) {
		return(FAILURE);
	}
	return(SUCCESS);
}

static BRESULT mixrectex(MIXRECTEX *r, const VRAMHDL dst, const VRAMHDL org,
					const RECT_T *rct, const VRAMHDL src, const POINT_T *pt) {

	int		pos;
	RECT_T	s;
	int		dstwidth;
	int		dstheight;

	if ((dst == NULL) || (org == NULL) || (src == NULL) ||
		(dst->bpp != org->bpp) || (dst->bpp != src->bpp)) {
		return(FAILURE);
	}
	dstwidth = min(dst->width, org->width);
	dstheight = min(dst->height, org->height);
	r->srcpos = 0;
	if (rct == NULL) {
		s.left = 0;
		s.top = 0;
		s.right = dstwidth;
		s.bottom = dstheight;
		r->orgpos = 0;
		r->dstpos = 0;
	}
	else {
		if ((rct->bottom <= 0) || (rct->right <= 0) ||
			(rct->left >= dstwidth) || (rct->top >= dstheight)) {
			return(FAILURE);
		}
		s.left = max(rct->left, 0);
		s.top = max(rct->top, 0);
		s.right = min(rct->right, dstwidth);
		s.bottom = min(rct->bottom, dstheight);
		if ((s.top >= s.bottom) || (s.left >= s.right)) {
			return(FAILURE);
		}
		r->orgpos = s.top * org->width;
		r->orgpos += s.left;
		r->dstpos = s.top * dst->width;
		r->dstpos += s.left;
	}

	pos = src->posy - s.top;
	if (pt) {
		pos += pt->y;
	}
	if (pos < 0) {
		r->srcpos -= pos * src->width;
		r->height = min(src->height + pos, s.bottom - s.top);
	}
	else {
		r->orgpos += pos * org->width;
		r->dstpos += pos * dst->width;
		r->height = min(s.bottom - s.top - pos, src->height);
	}
	if (r->height <= 0) {
		return(FAILURE);
	}

	pos = src->posx - s.left;
	if (pt) {
		pos += pt->x;
	}
	if (pos < 0) {
		r->srcpos -= pos;
		r->width = min(src->width + pos, s.right - s.left);
	}
	else {
		r->orgpos += pos;
		r->dstpos += pos;
		r->width = min(s.right - s.left - pos, src->width);
	}
	if (r->width <= 0) {
		return(FAILURE);
	}
	return(SUCCESS);
}


// ----

static void vramsub_cpy(VRAMHDL dst, const VRAMHDL src, MIX_RECT *mr) {

const UINT8	*p;
	UINT8	*q;

	p = src->ptr + (mr->srcpos * src->xalign);
	q = dst->ptr + (mr->dstpos * src->xalign);
	do {
		CopyMemory(q, p, mr->width * src->xalign);
		p += src->yalign;
		q += dst->yalign;
	} while(--mr->height);
}

static void vramsub_move(VRAMHDL dst, const VRAMHDL src, MIX_RECT *mr) {

const UINT8	*p;
	UINT8	*q;
	int		align;
	int		r;

	align = mr->width * src->xalign;
	p = src->ptr + (mr->srcpos * src->xalign);
	q = dst->ptr + (mr->dstpos * src->xalign);
	if ((src->ptr != dst->ptr) || (p >= q)) {
		do {
			CopyMemory(q, p, align);
			p += src->yalign;
			q += dst->yalign;
		} while(--mr->height);
	}
	else {
		p += (mr->height * src->yalign);
		q += (mr->height * dst->yalign);
		do {
			p -= src->yalign - align;
			q -= dst->yalign - align;
			r = align;
			do {
				p--;
				q--;
				*q = *p;
			} while(--r);
		} while(--mr->height);
	}
}
static void vramsub_cpyall(VRAMHDL dst, const VRAMHDL src, MIX_RECT *mr) {

const UINT8	*p;
	UINT8	*q;
	int		height;

	p = src->ptr + (mr->srcpos * src->xalign);
	q = dst->ptr + (mr->dstpos * src->xalign);
	height = mr->height;
	do {
		CopyMemory(q, p, mr->width * src->xalign);
		p += src->yalign;
		q += dst->yalign;
	} while(--height);
	if ((src->alpha) && (dst->alpha)) {
		p = src->alpha + mr->srcpos;
		q = dst->alpha + mr->dstpos;
		do {
			CopyMemory(q, p, mr->width);
			p += src->width;
			q += dst->width;
		} while(--mr->height);
	}
}

static void vramsub_cpy2(VRAMHDL dst, const VRAMHDL src, UINT alpha,
															MIX_RECT *mr) {

const UINT8	*p;
	UINT8	*q;
	UINT8	*r;

	p = src->ptr + (mr->srcpos * src->xalign);
	q = dst->ptr + (mr->dstpos * src->xalign);
	r = dst->alpha + mr->dstpos;
	do {
		CopyMemory(q, p, mr->width * src->xalign);
		FillMemory(r, mr->width, alpha);
		p += src->yalign;
		q += dst->yalign;
		r += dst->width;
	} while(--mr->height);
}


// ---- bpp=16

#ifdef SUPPORT_16BPP

static void vramsub_cpyp16(VRAMHDL dst, const VRAMHDL src, const UINT8 *pat8,
													MIX_RECT *mr) {

const UINT8	*p;
	UINT8	*q;
	int		x;
	int		step;
	int		posx;
	int		posy;

	p = src->ptr + (mr->srcpos * 2);
	q = dst->ptr + (mr->dstpos * 2);
	posx = mr->dstpos % dst->width;
	posy = mr->dstpos / dst->width;
	step = mr->width * 2;

	do {
		UINT pat;
		x = mr->width;
		pat = pat8[posy & 7];
		posy++;
		pat <<= (posx & 7);
		pat |= (pat >> 8);
		do {
			pat <<= 1;
			if (pat & 0x100) {
				*(UINT16 *)q = *(UINT16 *)p;
				pat++;
			}
			p += 2;
			q += 2;
		} while(--x);
		p += src->yalign - step;
		q += dst->yalign - step;
	} while(--mr->height);
}

static void vramsub_cpyp16w16(VRAMHDL dst, const VRAMHDL src,
												UINT pat16, MIX_RECT *mr) {

const UINT8	*p;
	UINT8	*q;
	int		x;
	int		step;
	int		posx;

	p = src->ptr + (mr->srcpos * 2);
	q = dst->ptr + (mr->dstpos * 2);
	posx = mr->dstpos % dst->width;
	step = mr->width * 2;

	do {
		UINT32 pat;
		x = mr->width;
		pat = pat16;
		pat |= (pat << 16);
		pat >>= (posx & 15);
		do {
			if (pat & 1) {
				*(UINT16 *)q = *(UINT16 *)p;
				pat |= 0x10000;
			}
			pat >>= 1;
			p += 2;
			q += 2;
		} while(--x);
		p += src->yalign - step;
		q += dst->yalign - step;
	} while(--mr->height);
}

static void vramsub_cpyp16h16(VRAMHDL dst, const VRAMHDL src,
												UINT pat16, MIX_RECT *mr) {

const UINT8	*p;
	UINT8	*q;
	int		step;
	int		posy;

	p = src->ptr + (mr->srcpos * 2);
	q = dst->ptr + (mr->dstpos * 2);
	posy = mr->dstpos / dst->width;
	step = mr->width * 2;

	do {
		if (pat16 & (1 << (posy & 15))) {
			CopyMemory(q, p, step);
		}
		posy++;
		p += src->yalign;
		q += dst->yalign;
	} while(--mr->height);
}

static void vramsub_cpyex16(VRAMHDL dst, const VRAMHDL src, MIX_RECT *mr) {

const UINT8	*p;
	UINT8	*q;
	int		x;
	int		step;

	p = src->ptr + (mr->srcpos * 2);
	q = dst->ptr + (mr->dstpos * 2);
	step = mr->width * 2;

	do {
		x = mr->width;
		do {
			UINT16 dat;
			dat = *(UINT16 *)p;
			p += 2;
			if (dat) {
				*(UINT16 *)q = dat;
			}
			q += 2;
		} while(--x);
		p += src->yalign - step;
		q += dst->yalign - step;
	} while(--mr->height);
}

static void vramsub_cpyex16a(VRAMHDL dst, const VRAMHDL src, MIX_RECT *mr) {

const UINT8	*p;
const UINT8	*a;
	UINT8	*q;
	int		x;
	int		step;

	a = src->alpha + mr->srcpos;
	p = src->ptr + (mr->srcpos * 2);
	q = dst->ptr + (mr->dstpos * 2);
	step = mr->width * 2;

	do {
		x = mr->width;
		do {
			UINT alpha;
			alpha = *a++;
			if (alpha) {
				UINT s1, s2, d;
				s1 = *(UINT16 *)p;
				s2 = *(UINT16 *)q;
				alpha += VRAMALPHABASE;
				d = MAKEALPHA16(s2, s1, B16MASK, alpha, VRAMALPHABIT);
				d |= MAKEALPHA16(s2, s1, G16MASK, alpha, VRAMALPHABIT);
				d |= MAKEALPHA16(s2, s1, R16MASK, alpha, VRAMALPHABIT);
				*(UINT16 *)q = (UINT16)d;
			}
			p += 2;
			q += 2;
		} while(--x);
		a += src->width - mr->width;
		p += src->yalign - step;
		q += dst->yalign - step;
	} while(--mr->height);
}

static void vramsub_cpyex16a2(VRAMHDL dst, const VRAMHDL src,
												UINT alpha64, MIX_RECT *mr) {

const UINT8	*p;
const UINT8	*a;
	UINT8	*q;
	int		x;
	int		step;

	a = src->alpha + mr->srcpos;
	p = src->ptr + (mr->srcpos * 2);
	q = dst->ptr + (mr->dstpos * 2);
	step = mr->width * 2;

	do {
		x = mr->width;
		do {
			UINT alpha;
			alpha = *a++;
			if (alpha) {
				UINT s1, s2, d;
				alpha = (alpha + VRAMALPHABASE) * alpha64;
				s1 = *(UINT16 *)p;
				s2 = *(UINT16 *)q;
				d = MAKEALPHA16(s2, s1, B16MASK, alpha, VRAMALPHABIT+6);
				d |= MAKEALPHA16(s2, s1, G16MASK, alpha, VRAMALPHABIT+6);
				d |= MAKEALPHA16(s2, s1, R16MASK, alpha, VRAMALPHABIT+6);
				*(UINT16 *)q = (UINT16)d;
			}
			p += 2;
			q += 2;
		} while(--x);
		a += src->width - mr->width;
		p += src->yalign - step;
		q += dst->yalign - step;
	} while(--mr->height);
}

static void vramsub_cpyexa16a(VRAMHDL dst, const VRAMHDL src, MIX_RECT *mr) {

const UINT8	*p;
const UINT8	*a;
	UINT8	*q;
	UINT8	*b;
	int		x;
	int		step;

	p = src->ptr + (mr->srcpos * 2);
	a = src->alpha + mr->srcpos;
	q = dst->ptr + (mr->dstpos * 2);
	b = dst->alpha + mr->dstpos;
	step = mr->width * 2;

	do {
		x = mr->width;
		do {
			UINT alpha;
			alpha = *a++;
			if (alpha) {
				UINT s1, s2, d;
				s1 = *(UINT16 *)p;
				s2 = *(UINT16 *)q;
				alpha += VRAMALPHABASE;
				d = MAKEALPHA16(s2, s1, B16MASK, alpha, VRAMALPHABIT);
				d |= MAKEALPHA16(s2, s1, G16MASK, alpha, VRAMALPHABIT);
				d |= MAKEALPHA16(s2, s1, R16MASK, alpha, VRAMALPHABIT);
				*(UINT16 *)q = (UINT16)d;
				b[0] = VRAMALPHA;
			}
			p += 2;
			q += 2;
			b += 1;
		} while(--x);
		p += src->yalign - step;
		a += src->width - mr->width;
		q += dst->yalign - step;
		b += dst->width - mr->width;
	} while(--mr->height);
}

static void vramsub_cpya16(VRAMHDL dst, const VRAMHDL src,
												UINT alpha256, MIX_RECT *mr) {

const UINT8	*p;
	UINT8	*q;
	int		x;
	int		step;

	p = src->ptr + (mr->srcpos * 2);
	q = dst->ptr + (mr->dstpos * 2);
	step = mr->width * 2;

	do {
		x = mr->width;
		do {
			UINT s1, s2, d;
			s1 = *(UINT16 *)p;
			s2 = *(UINT16 *)q;
			d = MAKEALPHA16(s2, s1, B16MASK, alpha256, 8);
			d |= MAKEALPHA16(s2, s1, G16MASK, alpha256, 8);
			d |= MAKEALPHA16(s2, s1, R16MASK, alpha256, 8);
			*(UINT16 *)q = (UINT16)d;
			p += 2;
			q += 2;
		} while(--x);
		p += src->yalign - step;
		q += dst->yalign - step;
	} while(--mr->height);
}

static void vramsub_cpyexp16w16(VRAMHDL dst, const VRAMHDL src,
												UINT pat16, MIX_RECT *mr) {

const UINT8	*p;
const UINT8	*a;
	UINT8	*q;
	int		x;
	int		step;
	int		posx;

	p = src->ptr + (mr->srcpos * 2);
	q = dst->ptr + (mr->dstpos * 2);
	a = src->alpha + mr->srcpos;
	posx = mr->dstpos % dst->width;
	step = mr->width * 2;

	do {
		UINT32 pat;
		x = mr->width;
		pat = pat16;
		pat |= (pat << 16);
		pat >>= (posx & 15);
		do {
			if (pat & 1) {
				UINT alpha;
				alpha = *a;
				if (alpha) {
					UINT s1, s2, d;
					s1 = *(UINT16 *)p;
					s2 = *(UINT16 *)q;
					alpha += VRAMALPHABASE;
					d = MAKEALPHA16(s2, s1, B16MASK, alpha, VRAMALPHABIT);
					d |= MAKEALPHA16(s2, s1, G16MASK, alpha, VRAMALPHABIT);
					d |= MAKEALPHA16(s2, s1, R16MASK, alpha, VRAMALPHABIT);
					*(UINT16 *)q = (UINT16)d;
				}
				pat |= 0x10000;
			}
			pat >>= 1;
			p += 2;
			q += 2;
			a += 1;
		} while(--x);
		p += src->yalign - step;
		q += dst->yalign - step;
		a += src->width - mr->width;
	} while(--mr->height);
}

static void vramsub_cpyexp16h16(VRAMHDL dst, const VRAMHDL src,
												UINT pat16, MIX_RECT *mr) {

const UINT8	*p;
const UINT8	*a;
	UINT8	*q;
	int		x;
	int		step;
	int		posy;

	p = src->ptr + (mr->srcpos * 2);
	q = dst->ptr + (mr->dstpos * 2);
	a = src->alpha + mr->srcpos;
	posy = mr->dstpos / dst->width;
	step = mr->width * 2;

	do {
		if (pat16 & (1 << (posy & 15))) {
			x = mr->width;
			do {
				UINT alpha;
				alpha = *a;
				if (alpha) {
					UINT s1, s2, d;
					s1 = *(UINT16 *)p;
					s2 = *(UINT16 *)q;
					alpha += VRAMALPHABASE;
					d = MAKEALPHA16(s2, s1, B16MASK, alpha, VRAMALPHABIT);
					d |= MAKEALPHA16(s2, s1, G16MASK, alpha, VRAMALPHABIT);
					d |= MAKEALPHA16(s2, s1, R16MASK, alpha, VRAMALPHABIT);
					*(UINT16 *)q = (UINT16)d;
				}
				p += 2;
				q += 2;
				a += 1;
			} while(--x);
			p += src->yalign - step;
			q += dst->yalign - step;
			a += src->width - mr->width;
		}
		else {
			p += src->yalign;
			q += dst->yalign;
			a += src->width;
		}
		posy++;
	} while(--mr->height);
}

static void vramsub_mix16(VRAMHDL dst, const VRAMHDL org, const VRAMHDL src,
											UINT alpha64, MIXRECTEX *mr) {

const UINT8	*p;
const UINT8	*q;
	UINT8	*r;
	int		x;
	int		ostep;
	int		sstep;
	int		dstep;

	p = org->ptr + (mr->orgpos * 2);
	q = src->ptr + (mr->srcpos * 2);
	r = dst->ptr + (mr->dstpos * 2);
	ostep = org->yalign - (mr->width * 2);
	sstep = src->yalign - (mr->width * 2);
	dstep = dst->yalign - (mr->width * 2);

	do {
		x = mr->width;
		do {
			UINT s1, s2, d;
			s1 = *(UINT16 *)p;
			s2 = *(UINT16 *)q;
			d = MAKEALPHA16(s1, s2, B16MASK, alpha64, 6);
			d |= MAKEALPHA16(s1, s2, G16MASK, alpha64, 6);
			d |= MAKEALPHA16(s1, s2, R16MASK, alpha64, 6);
			*(UINT16 *)r = (UINT16)d;
			p += 2;
			q += 2;
			r += 2;
		} while(--x);
		p += ostep;
		q += sstep;
		r += dstep;
	} while(--mr->height);
}

static void vramsub_mixcol16(VRAMHDL dst, const VRAMHDL src, UINT32 color,
												UINT alpha64, MIX_RECT *mr) {

const UINT8	*p;
	UINT8	*q;
	int		x;
	int		step;
	int		tmp;
	int		c16[3];

	p = src->ptr + (mr->srcpos * 2);
	q = dst->ptr + (mr->dstpos * 2);
	step = mr->width * 2;

	tmp = MAKE16PAL(color);
	c16[0] = tmp & B16MASK;
	c16[1] = tmp & G16MASK;
	c16[2] = tmp & R16MASK;
	do {
		x = mr->width;
		do {
			UINT s, d;
			s = *(UINT16 *)p;
			d = MAKEALPHA16s(c16[0], s, B16MASK, alpha64, 6);
			d |= MAKEALPHA16s(c16[1], s, G16MASK, alpha64, 6);
			d |= MAKEALPHA16s(c16[2], s, R16MASK, alpha64, 6);
			*(UINT16 *)q = (UINT16)d;
			p += 2;
			q += 2;
		} while(--x);
		p += src->yalign - step;
		q += dst->yalign - step;
	} while(--mr->height);
}

static void vramsub_mixalpha16(VRAMHDL dst, const VRAMHDL src, UINT32 color,
															MIX_RECT *mr) {

const UINT8	*p;
	UINT8	*q;
	int		x;
	int		step;
	int		tmp;
	int		c16[3];

	p = src->ptr + (mr->srcpos * 2);
	q = dst->ptr + (mr->dstpos * 2);
	step = mr->width * 2;

	tmp = MAKE16PAL(color);
	c16[0] = tmp & B16MASK;
	c16[1] = tmp & G16MASK;
	c16[2] = tmp & R16MASK;
	do {
		x = mr->width;
		do {
			UINT s, d, e;
			int a;
			s = *(UINT16 *)q;
			e = *(UINT16 *)p;
			e ^= 0xffff;
			a = e & 0x1f;
			if (a) {
				a++;
			}
			d = MAKEALPHA16s(c16[0], s, B16MASK, a, 5);
			a = (e >> 5) & 0x3f;
			if (a) {
				a++;
			}
			d |= MAKEALPHA16s(c16[1], s, G16MASK, a, 6);
			a = (e >> 11) & 0x1f;
			if (a) {
				a++;
			}
			d |= MAKEALPHA16s(c16[2], s, R16MASK, a, 5);
			*(UINT16 *)q = (UINT16)d;
			p += 2;
			q += 2;
		} while(--x);
		p += src->yalign - step;
		q += dst->yalign - step;
	} while(--mr->height);
}

static void vramsub_gray16(VRAMHDL dst, const VRAMHDL org, const VRAMHDL src,
								const VRAMHDL bmp, int delta, MIXRECTEX *mr) {

const UINT8	*p;
const UINT8	*q;
const UINT8	*a;
	UINT8	*r;
	int		rm;
	int		x, y;
	int		ostep;
	int		sstep;
	int		dstep;
	int		xstep;
	int		ystep;

	if ((bmp == NULL) || (bmp->bpp != 8)) {
		return;
	}

	p = org->ptr + (mr->orgpos * 2);
	q = src->ptr + (mr->srcpos * 2);
	r = dst->ptr + (mr->dstpos * 2);
	ostep = org->yalign - (mr->width * 2);
	sstep = src->yalign - (mr->width * 2);
	dstep = dst->yalign - (mr->width * 2);

	xstep = (bmp->width << 10) / mr->width;
	ystep = (bmp->height << 10) / mr->height;

	y = 0;
	do {
		a = bmp->ptr + ((y >> 10) * bmp->yalign);
		rm = mr->width;
		x = 0;
		do {
			int alpha;
			alpha = a[x >> 10] + delta + 1;
			if (alpha >= 256) {
				*(UINT16 *)r = *(UINT16 *)q;
			}
			else if (alpha > 0) {
				UINT s1, s2, d;
				s1 = *(UINT16 *)p;
				s2 = *(UINT16 *)q;
				d = MAKEALPHA16(s1, s2, B16MASK, alpha, 8);
				d |= MAKEALPHA16(s1, s2, G16MASK, alpha, 8);
				d |= MAKEALPHA16(s1, s2, R16MASK, alpha, 8);
				*(UINT16 *)r = (UINT16)d;
			}
			else {
				*(UINT16 *)r = *(UINT16 *)p;
			}
			p += 2;
			q += 2;
			r += 2;
			x += xstep;
		} while(--rm);
		p += ostep;
		q += sstep;
		r += dstep;
		y += ystep;
	} while(--mr->height);
}

static void vramsub_zoom16(VRAMHDL dst, const VRAMHDL src, int dot,
															MIX_RECT *mr) {

const UINT8	*pbase;
const UINT8	*p;
	UINT8	*qbase;
	UINT8	*q;
	int		x;
	int		dstep;
	int		xstep;
	int		ystep;
	int		xx;
	int		yy;
	int		xstep2;
	UINT16	col;

	pbase = src->ptr + (mr->srcpos * 2);
	qbase = dst->ptr + (mr->dstpos * 2);
	dstep = (dst->yalign * dot) - (mr->width * 2);

	do {
		p = pbase;
		ystep = min(mr->height, dot);
		x = mr->width;
		do {
			xstep = min(x, dot);
			xstep2 = xstep * 2;
			q = qbase;
			yy = ystep;
			col = *(UINT16 *)p;
			do {
				xx = xstep;
				do {
					*(UINT16 *)q = col;
					q += 2;
				} while(--xx);
				q += dst->yalign;
				q -= xstep2;
			} while(--yy);
			p += 2;
			qbase += xstep2;
			x -= xstep;
		} while(x);
		pbase += src->yalign;
		qbase += dstep;
		mr->height -= ystep;
	} while(mr->height);
}

static void vramsub_mosaic16(VRAMHDL dst, const VRAMHDL src, int dot,
															MIX_RECT *mr) {

const UINT8	*p;
	UINT8	*q;
	UINT8	*r;
	int		x;
	int		sstep;
	int		dstep;
	int		xstep;
	int		ystep;
	int		xx;
	int		yy;
	int		xstep2;
	UINT16	col;

	p = src->ptr + (mr->srcpos * 2);
	q = dst->ptr + (mr->dstpos * 2);
	sstep = (src->yalign * dot) - (mr->width * 2);
	dstep = (dst->yalign * dot) - (mr->width * 2);

	do {
		ystep = min(mr->height, dot);
		x = mr->width;
		do {
			xstep = min(x, dot);
			xstep2 = xstep * 2;
			r = q;
			yy = ystep;
			col = *(UINT16 *)p;
			do {
				xx = xstep;
				do {
					*(UINT16 *)r = col;
					r += 2;
				} while(--xx);
				r += dst->yalign;
				r -= xstep2;
			} while(--yy);
			p += xstep2;
			q += xstep2;
			x -= xstep;
		} while(x);
		p += sstep;
		q += dstep;
		mr->height -= ystep;
	} while(mr->height);
}

static void vramsub_colex16(VRAMHDL dst, const VRAMHDL src, UINT32 color,
															MIX_RECT *mr) {

	UINT8	*p, *q;
	int		x;
	int		step;
	UINT	tmp;
	int		c16[3];
	int		a;

	tmp = MAKE16PAL(color);
	c16[0] = tmp & B16MASK;
	c16[1] = tmp & G16MASK;
	c16[2] = tmp & R16MASK;

	p = src->ptr + mr->srcpos;
	q = dst->ptr + (mr->dstpos * 2);
	step = mr->width * 2;

	do {
		x = mr->width;
		do {
			a = p[0];
			if (a) {
				UINT s, d;
				a = VRAMALPHA - a;
				s = *(UINT16 *)q;
				d = MAKEALPHA16s(c16[0], s, B16MASK, a, VRAMALPHABIT);
				d |= MAKEALPHA16s(c16[1], s, G16MASK, a, VRAMALPHABIT);
				d |= MAKEALPHA16s(c16[2], s, R16MASK, a, VRAMALPHABIT);
				*(UINT16 *)q = (UINT16)d;
			}
			p += 1;
			q += 2;
		} while(--x);
		p += src->width - mr->width;
		q += dst->yalign - step;
	} while(--mr->height);
}
#endif


// ---- bpp=24

#ifdef SUPPORT_24BPP

static void vramsub_cpyp24(VRAMHDL dst, const VRAMHDL src, const UINT8 *pat8,
														MIX_RECT *mr) {

const UINT8	*p;
	UINT8	*q;
	int		x;
	int		step;
	int		posx;
	int		posy;

	p = src->ptr + (mr->srcpos * 3);
	q = dst->ptr + (mr->dstpos * 3);
	posx = mr->dstpos % dst->width;
	posy = mr->dstpos / dst->width;
	step = mr->width * 3;

	do {
		UINT pat;
		x = mr->width;
		pat = pat8[posy & 7];
		posy++;
		pat <<= (posx & 7);
		pat |= (pat >> 8);
		do {
			pat <<= 1;
			if (pat & 0x100) {
				q[0] = p[0];
				q[1] = p[1];
				q[2] = p[2];
				pat++;
			}
			p += 3;
			q += 3;
		} while(--x);
		p += src->yalign - step;
		q += dst->yalign - step;
	} while(--mr->height);
}

static void vramsub_cpyp16w24(VRAMHDL dst, const VRAMHDL src,
												UINT pat16, MIX_RECT *mr) {

const UINT8	*p;
	UINT8	*q;
	int		x;
	int		step;
	int		posx;

	p = src->ptr + (mr->srcpos * 3);
	q = dst->ptr + (mr->dstpos * 3);
	posx = mr->dstpos % dst->width;
	step = mr->width * 3;

	do {
		UINT32 pat;
		x = mr->width;
		pat = pat16;
		pat |= (pat << 16);
		pat >>= (posx & 15);
		do {
			if (pat & 1) {
				q[0] = p[0];
				q[1] = p[1];
				q[2] = p[2];
				pat |= 0x10000;
			}
			pat >>= 1;
			p += 3;
			q += 3;
		} while(--x);
		p += src->yalign - step;
		q += dst->yalign - step;
	} while(--mr->height);
}

static void vramsub_cpyp16h24(VRAMHDL dst, const VRAMHDL src,
												UINT pat16, MIX_RECT *mr) {

const UINT8	*p;
	UINT8	*q;
	int		step;
	int		posy;

	p = src->ptr + (mr->srcpos * 3);
	q = dst->ptr + (mr->dstpos * 3);
	posy = mr->dstpos / dst->width;
	step = mr->width * 3;

	do {
		if (pat16 & (1 << (posy & 15))) {
			CopyMemory(q, p, step);
		}
		posy++;
		p += src->yalign;
		q += dst->yalign;
	} while(--mr->height);
}

static void vramsub_cpyex24(VRAMHDL dst, const VRAMHDL src, MIX_RECT *mr) {

const UINT8	*p;
	UINT8	*q;
	int		x;
	int		step;

	p = src->ptr + (mr->srcpos * src->xalign);
	q = dst->ptr + (mr->dstpos * src->xalign);
	step = mr->width * 3;

	do {
		x = mr->width;
		do {
			UINT8 r, g, b;
			b = p[0];
			g = p[1];
			r = p[2];
			p += 3;
			if ((b) || (g) || (r)) {
				q[0] = b;
				q[1] = g;
				q[2] = r;
			}
			q += 3;
		} while(--x);
		p += src->yalign - step;
		q += dst->yalign - step;
	} while(--mr->height);
}

static void vramsub_cpyex24a(VRAMHDL dst, const VRAMHDL src, MIX_RECT *mr) {

const UINT8	*p;
const UINT8	*a;
	UINT8	*q;
	int		x;
	int		step;

	a = src->alpha + mr->srcpos;
	p = src->ptr + (mr->srcpos * 3);
	q = dst->ptr + (mr->dstpos * 3);
	step = mr->width * 3;

	do {
		x = mr->width;
		do {
			UINT alpha;
			alpha = *a++;
			if (alpha) {
				alpha += VRAMALPHABASE;
				q[0] = (UINT8)MAKEALPHA24(q[0], p[0], alpha, VRAMALPHABIT);
				q[1] = (UINT8)MAKEALPHA24(q[1], p[1], alpha, VRAMALPHABIT);
				q[2] = (UINT8)MAKEALPHA24(q[2], p[2], alpha, VRAMALPHABIT);
			}
			p += 3;
			q += 3;
		} while(--x);
		a += src->width - mr->width;
		p += src->yalign - step;
		q += dst->yalign - step;
	} while(--mr->height);
}

static void vramsub_cpyex24a2(VRAMHDL dst, const VRAMHDL src,
											UINT alpha64, MIX_RECT *mr) {

const UINT8	*p;
const UINT8	*a;
	UINT8	*q;
	int		x;
	int		step;

	a = src->alpha + mr->srcpos;
	p = src->ptr + (mr->srcpos * 3);
	q = dst->ptr + (mr->dstpos * 3);
	step = mr->width * 3;

	do {
		x = mr->width;
		do {
			UINT alpha;
			alpha = *a++;
			if (alpha) {
				alpha = (alpha + VRAMALPHABASE) * alpha64;
				q[0] = (UINT8)MAKEALPHA24(q[0], p[0], alpha, VRAMALPHABIT+6);
				q[1] = (UINT8)MAKEALPHA24(q[1], p[1], alpha, VRAMALPHABIT+6);
				q[2] = (UINT8)MAKEALPHA24(q[2], p[2], alpha, VRAMALPHABIT+6);
			}
			p += 3;
			q += 3;
		} while(--x);
		a += src->width - mr->width;
		p += src->yalign - step;
		q += dst->yalign - step;
	} while(--mr->height);
}

static void vramsub_cpyexa24a(VRAMHDL dst, const VRAMHDL src, MIX_RECT *mr) {

const UINT8	*p;
const UINT8	*a;
	UINT8	*q;
	UINT8	*b;
	int		x;
	int		step;

	p = src->ptr + (mr->srcpos * 3);
	a = src->alpha + mr->srcpos;
	q = dst->ptr + (mr->dstpos * 3);
	b = dst->alpha + mr->dstpos;
	step = mr->width * 3;

	do {
		x = mr->width;
		do {
			UINT alpha;
			alpha = *a++;
			if (alpha) {
				alpha += VRAMALPHABASE;
				q[0] = (UINT8)MAKEALPHA24(q[0], p[0], alpha, VRAMALPHABIT);
				q[1] = (UINT8)MAKEALPHA24(q[1], p[1], alpha, VRAMALPHABIT);
				q[2] = (UINT8)MAKEALPHA24(q[2], p[2], alpha, VRAMALPHABIT);
				b[0] = VRAMALPHA;
			}
			p += 3;
			q += 3;
			b += 1;
		} while(--x);
		p += src->yalign - step;
		a += src->width - mr->width;
		q += dst->yalign - step;
		b += dst->width - mr->width;
	} while(--mr->height);
}

static void vramsub_cpya24(VRAMHDL dst, const VRAMHDL src,
												UINT alpha256, MIX_RECT *mr) {

const UINT8	*p;
	UINT8	*q;
	int		x;
	int		step;

	p = src->ptr + (mr->srcpos * 3);
	q = dst->ptr + (mr->dstpos * 3);
	step = mr->width * 3;

	do {
		x = mr->width;
		do {
			q[0] = (UINT8)MAKEALPHA24(q[0], p[0], alpha256, 8);
			q[1] = (UINT8)MAKEALPHA24(q[1], p[1], alpha256, 8);
			q[2] = (UINT8)MAKEALPHA24(q[2], p[2], alpha256, 8);
			p += 3;
			q += 3;
		} while(--x);
		p += src->yalign - step;
		q += dst->yalign - step;
	} while(--mr->height);
}

static void vramsub_cpyexp16w24(VRAMHDL dst, const VRAMHDL src,
												UINT pat16, MIX_RECT *mr) {

const UINT8	*p;
const UINT8	*a;
	UINT8	*q;
	int		x;
	int		step;
	int		posx;

	p = src->ptr + (mr->srcpos * 3);
	q = dst->ptr + (mr->dstpos * 3);
	a = src->alpha + mr->srcpos;
	posx = mr->dstpos % dst->width;
	step = mr->width * 3;

	do {
		UINT32 pat;
		x = mr->width;
		pat = pat16;
		pat |= (pat << 16);
		pat >>= (posx & 15);
		do {
			if (pat & 1) {
				UINT alpha;
				alpha = *a;
				if (alpha) {
					alpha += VRAMALPHABASE;
					q[0] = (UINT8)MAKEALPHA24(q[0], p[0], alpha, VRAMALPHABIT);
					q[1] = (UINT8)MAKEALPHA24(q[1], p[1], alpha, VRAMALPHABIT);
					q[2] = (UINT8)MAKEALPHA24(q[2], p[2], alpha, VRAMALPHABIT);
				}
				pat |= 0x10000;
			}
			pat >>= 1;
			p += 3;
			q += 3;
			a += 1;
		} while(--x);
		p += src->yalign - step;
		q += dst->yalign - step;
		a += src->width - mr->width;
	} while(--mr->height);
}

static void vramsub_cpyexp16h24(VRAMHDL dst, const VRAMHDL src,
												UINT pat16, MIX_RECT *mr) {

const UINT8	*p;
const UINT8	*a;
	UINT8	*q;
	int		x;
	int		step;
	int		posy;

	p = src->ptr + (mr->srcpos * 3);
	q = dst->ptr + (mr->dstpos * 3);
	a = src->alpha + mr->srcpos;
	posy = mr->dstpos / dst->width;
	step = mr->width * 3;

	do {
		if (pat16 & (1 << (posy & 15))) {
			x = mr->width;
			do {
				UINT alpha;
				alpha = *a;
				if (alpha) {
					alpha += VRAMALPHABASE;
					q[0] = (UINT8)MAKEALPHA24(q[0], p[0], alpha, VRAMALPHABIT);
					q[1] = (UINT8)MAKEALPHA24(q[1], p[1], alpha, VRAMALPHABIT);
					q[2] = (UINT8)MAKEALPHA24(q[2], p[2], alpha, VRAMALPHABIT);
				}
				p += 3;
				q += 3;
				a += 1;
			} while(--x);
			p += src->yalign - step;
			q += dst->yalign - step;
			a += src->width - mr->width;
		}
		else {
			p += src->yalign;
			q += dst->yalign;
			a += src->width;
		}
		posy++;
	} while(--mr->height);
}

static void vramsub_mix24(VRAMHDL dst, const VRAMHDL org, const VRAMHDL src,
												UINT alpha64, MIXRECTEX *mr) {

const UINT8	*p;
const UINT8	*q;
	UINT8	*r;
	int		x;
	int		ostep;
	int		sstep;
	int		dstep;

	p = org->ptr + (mr->orgpos * 3);
	q = src->ptr + (mr->srcpos * 3);
	r = dst->ptr + (mr->dstpos * 3);
	ostep = org->yalign - (mr->width * 3);
	sstep = src->yalign - (mr->width * 3);
	dstep = dst->yalign - (mr->width * 3);

	do {
		x = mr->width;
		do {
			r[0] = (UINT8)MAKEALPHA24(p[0], q[0], alpha64, 6);
			r[1] = (UINT8)MAKEALPHA24(p[1], q[1], alpha64, 6);
			r[2] = (UINT8)MAKEALPHA24(p[2], q[2], alpha64, 6);
			p += 3;
			q += 3;
			r += 3;
		} while(--x);
		p += ostep;
		q += sstep;
		r += dstep;
	} while(--mr->height);
}

static void vramsub_mixcol24(VRAMHDL dst, const VRAMHDL src, UINT32 color,
												UINT alpha64, MIX_RECT *mr) {

const UINT8	*p;
	UINT8	*q;
	int		x;
	int		step;
	int		c24[3];

	p = src->ptr + (mr->srcpos * 3);
	q = dst->ptr + (mr->dstpos * 3);
	step = mr->width * 3;

	c24[0] = color & 0xff;
	c24[1] = (color >> 8) & 0xff;
	c24[2] = (color >> 16) & 0xff;
	do {
		x = mr->width;
		do {
			q[0] = (UINT8)MAKEALPHA24(c24[0], p[0], alpha64, 6);
			q[1] = (UINT8)MAKEALPHA24(c24[1], p[1], alpha64, 6);
			q[2] = (UINT8)MAKEALPHA24(c24[2], p[2], alpha64, 6);
			p += 3;
			q += 3;
		} while(--x);
		p += src->yalign - step;
		q += dst->yalign - step;
	} while(--mr->height);
}

static void vramsub_mixalpha24(VRAMHDL dst, const VRAMHDL src, UINT32 color,
															MIX_RECT *mr) {

const UINT8	*p;
	UINT8	*q;
	int		x;
	int		step;
	int		c24[3];

	p = src->ptr + (mr->srcpos * src->xalign);
	q = dst->ptr + (mr->dstpos * src->xalign);
	step = mr->width * 3;

	c24[0] = color & 0xff;
	c24[1] = (color >> 8) & 0xff;
	c24[2] = (color >> 16) & 0xff;
	do {
		x = mr->width;
		do {
			int a;
			a = p[0];
			if (a) {
				a++;
			}
			q[0] = (UINT8)MAKEALPHA24(q[0], c24[0], a, 8);
			a = p[1];
			if (a) {
				a++;
			}
			q[1] = (UINT8)MAKEALPHA24(q[1], c24[1], a, 8);
			a = p[2];
			if (a) {
				a++;
			}
			q[2] = (UINT8)MAKEALPHA24(q[2], c24[2], a, 8);
			p += 3;
			q += 3;
		} while(--x);
		p += src->yalign - step;
		q += dst->yalign - step;
	} while(--mr->height);
}

static void vramsub_gray24(VRAMHDL dst, const VRAMHDL org, const VRAMHDL src,
								const VRAMHDL bmp, int delta, MIXRECTEX *mr) {

const UINT8	*p;
const UINT8	*q;
const UINT8	*a;
	UINT8	*r;
	int		rm;
	int		x, y;
	int		ostep;
	int		sstep;
	int		dstep;
	int		xstep;
	int		ystep;

	if ((bmp == NULL) || (bmp->bpp != 8)) {
		return;
	}

	p = org->ptr + (mr->orgpos * 3);
	q = src->ptr + (mr->srcpos * 3);
	r = dst->ptr + (mr->dstpos * 3);
	ostep = org->yalign - (mr->width * 3);
	sstep = src->yalign - (mr->width * 3);
	dstep = dst->yalign - (mr->width * 3);

	xstep = (bmp->width << 10) / mr->width;
	ystep = (bmp->height << 10) / mr->height;

	y = 0;
	do {
		a = bmp->ptr + ((y >> 10) * bmp->yalign);
		rm = mr->width;
		x = 0;
		do {
			int alpha;
			alpha = a[x >> 10] + delta + 1;
			if (alpha >= 256) {
				r[0] = q[0];
				r[1] = q[1];
				r[2] = q[2];
			}
			else if (alpha > 0) {
				r[0] = (UINT8)MAKEALPHA24(p[0], q[0], alpha, 8);
				r[1] = (UINT8)MAKEALPHA24(p[1], q[1], alpha, 8);
				r[2] = (UINT8)MAKEALPHA24(p[2], q[2], alpha, 8);
			}
			else {
				r[0] = p[0];
				r[1] = p[1];
				r[2] = p[2];
			}
			p += 3;
			q += 3;
			r += 3;
			x += xstep;
		} while(--rm);
		p += ostep;
		q += sstep;
		r += dstep;
		y += ystep;
	} while(--mr->height);
}

static void vramsub_zoom24(VRAMHDL dst, const VRAMHDL src, int dot,
															MIX_RECT *mr) {

const UINT8	*pbase;
const UINT8	*p;
	UINT8	*qbase;
	UINT8	*q;
	int		x;
	int		dstep;
	int		xstep;
	int		ystep;
	int		xx;
	int		yy;
	int		xstep3;

	pbase = src->ptr + (mr->srcpos * 3);
	qbase = dst->ptr + (mr->dstpos * 3);
	dstep = (dst->yalign * dot) - (mr->width * 3);

	do {
		p = pbase;
		ystep = min(mr->height, dot);
		x = mr->width;
		do {
			xstep = min(x, dot);
			xstep3 = xstep * 3;
			q = qbase;
			yy = ystep;
			do {
				xx = xstep;
				do {
					q[0] = p[0];
					q[1] = p[1];
					q[2] = p[2];
					q += 3;
				} while(--xx);
				q += dst->yalign;
				q -= xstep3;
			} while(--yy);
			p += 3;
			qbase += xstep3;
			x -= xstep;
		} while(x);
		pbase += src->yalign;
		qbase += dstep;
		mr->height -= ystep;
	} while(mr->height);
}

static void vramsub_mosaic24(VRAMHDL dst, const VRAMHDL src, int dot,
															MIX_RECT *mr) {

const UINT8	*p;
	UINT8	*q;
	UINT8	*r;
	int		x;
	int		sstep;
	int		dstep;
	int		xstep;
	int		ystep;
	int		xx;
	int		yy;
	int		xstep3;

	p = src->ptr + (mr->srcpos * 3);
	q = dst->ptr + (mr->dstpos * 3);
	sstep = (src->yalign * dot) - (mr->width * 3);
	dstep = (dst->yalign * dot) - (mr->width * 3);

	do {
		ystep = min(mr->height, dot);
		x = mr->width;
		do {
			xstep = min(x, dot);
			xstep3 = xstep * 3;
			r = q;
			yy = ystep;
			do {
				xx = xstep;
				do {
					r[0] = p[0];
					r[1] = p[1];
					r[2] = p[2];
					r += 3;
				} while(--xx);
				r += dst->yalign;
				r -= xstep3;
			} while(--yy);
			p += xstep3;
			q += xstep3;
			x -= xstep;
		} while(x);
		p += sstep;
		q += dstep;
		mr->height -= ystep;
	} while(mr->height);
}

static void vramsub_colex24(VRAMHDL dst, const VRAMHDL src, UINT32 color,
															MIX_RECT *mr) {

	UINT8	*p, *q;
	int		x;
	int		step;
	int		c24[3];
	int		a;

	c24[0] = color & 0xff;
	c24[1] = (color >> 8) & 0xff;
	c24[2] = (color >> 16) & 0xff;

	p = src->ptr + mr->srcpos;
	q = dst->ptr + (mr->dstpos * dst->xalign);
	step = mr->width * 3;

	do {
		x = mr->width;
		do {
			a = p[0];
			if (a) {
				a += VRAMALPHABASE;
				q[0] = (UINT8)MAKEALPHA24(q[0], c24[0], a, VRAMALPHABIT);
				q[1] = (UINT8)MAKEALPHA24(q[1], c24[1], a, VRAMALPHABIT);
				q[2] = (UINT8)MAKEALPHA24(q[2], c24[2], a, VRAMALPHABIT);
			}
			p += 1;
			q += 3;
		} while(--x);
		p += src->width - mr->width;
		q += dst->yalign - step;
	} while(--mr->height);
}
#endif


// ----

// サーフェスをバッファとして使う場合…
// dst(posx, posy) <-src:rct

void vramcpy_cpy(VRAMHDL dst, const POINT_T *pt,
							const VRAMHDL src, const RECT_T *rct) {

	MIX_RECT	mr;

	if ((cpyrect(&mr, dst, pt, src, rct) != SUCCESS) ||
		(dst->bpp != src->bpp)) {
		return;
	}
	vramsub_cpy(dst, src, &mr);
}

void vramcpy_move(VRAMHDL dst, const POINT_T *pt,
							const VRAMHDL src, const RECT_T *rct) {

	MIX_RECT	mr;

	if ((cpyrect(&mr, dst, pt, src, rct) != SUCCESS) ||
		(dst->bpp != src->bpp)) {
		return;
	}
	vramsub_move(dst, src, &mr);
}

void vramcpy_cpyall(VRAMHDL dst, const POINT_T *pt,
							const VRAMHDL src, const RECT_T *rct) {

	MIX_RECT	mr;

	if ((cpyrect(&mr, dst, pt, src, rct) != SUCCESS) ||
		(dst->bpp != src->bpp)) {
		return;
	}
	vramsub_cpyall(dst, src, &mr);
}

void vramcpy_cpypat(VRAMHDL dst, const POINT_T *pt,
							const VRAMHDL src, const RECT_T *rct,
							const UINT8 *pat8) {

	MIX_RECT	mr;

	if ((cpyrect(&mr, dst, pt, src, rct) != SUCCESS) ||
		(dst->bpp != src->bpp)) {
		return;
	}
#ifdef SUPPORT_16BPP
	if (src->bpp == 16) {
		vramsub_cpyp16(dst, src, pat8, &mr);
	}
#endif
#ifdef SUPPORT_24BPP
	if (src->bpp == 24) {
		vramsub_cpyp24(dst, src, pat8, &mr);
	}
#endif
}

void vramcpy_cpyex(VRAMHDL dst, const POINT_T *pt,
							const VRAMHDL src, const RECT_T *rct) {

	MIX_RECT	mr;

	if ((cpyrect(&mr, dst, pt, src, rct) != SUCCESS) ||
		(dst->bpp != src->bpp)) {
		return;
	}
#ifdef SUPPORT_16BPP
	if (src->bpp == 16) {
		if (!src->alpha) {
			vramsub_cpyex16(dst, src, &mr);
		}
		else {
			vramsub_cpyex16a(dst, src, &mr);
		}
	}
#endif
#ifdef SUPPORT_24BPP
	if (src->bpp == 24) {
		if (!src->alpha) {
			vramsub_cpyex24(dst, src, &mr);
		}
		else {
			vramsub_cpyex24a(dst, src, &mr);
		}
	}
#endif
}

void vramcpy_cpyexa(VRAMHDL dst, const POINT_T *pt,
							const VRAMHDL src, const RECT_T *rct) {

	MIX_RECT	mr;

	if ((cpyrect(&mr, dst, pt, src, rct) != SUCCESS) ||
		(dst->bpp != src->bpp) ||
		(dst->alpha == NULL) || (src->alpha == NULL)) {
		return;
	}
#ifdef SUPPORT_16BPP
	if (src->bpp == 16) {
		vramsub_cpyexa16a(dst, src, &mr);
	}
#endif
#ifdef SUPPORT_24BPP
	if (src->bpp == 24) {
		vramsub_cpyexa24a(dst, src, &mr);
	}
#endif
}

void vramcpy_cpyalpha(VRAMHDL dst, const POINT_T *pt,
							const VRAMHDL src, const RECT_T *rct,
							UINT alpha256) {

	MIX_RECT	mr;

	if ((cpyrect(&mr, dst, pt, src, rct) != SUCCESS) ||
		(dst->bpp != src->bpp)) {
		return;
	}
	if (alpha256 < 256) {
		alpha256 = 256 - alpha256;
	}
	else {
		alpha256 = 0;
	}
#ifdef SUPPORT_16BPP
	if (src->bpp == 16) {
		vramsub_cpya16(dst, src, alpha256, &mr);
	}
#endif
#ifdef SUPPORT_24BPP
	if (src->bpp == 24) {
		vramsub_cpya24(dst, src, alpha256, &mr);
	}
#endif
}

void vramcpy_mix(VRAMHDL dst, const VRAMHDL org, const POINT_T *pt,
							const VRAMHDL src, const RECT_T *rct,
							UINT alpha64) {

	MIXRECTEX	mr;

	if (cpyrectex(&mr, dst, pt, org, src, rct) != SUCCESS) {
		return;
	}
#ifdef SUPPORT_16BPP
	if (src->bpp == 16) {
		vramsub_mix16(dst, org, src, alpha64, &mr);
	}
#endif
#ifdef SUPPORT_24BPP
	if (src->bpp == 24) {
		vramsub_mix24(dst, org, src, alpha64, &mr);
	}
#endif
}

void vramcpy_mixcol(VRAMHDL dst, const POINT_T *pt,
							const VRAMHDL src, const RECT_T *rct,
							UINT32 color, UINT alpha64) {

	MIX_RECT	mr;

	if ((cpyrect(&mr, dst, pt, src, rct) != SUCCESS) ||
		(dst->bpp != src->bpp)) {
		return;
	}
#ifdef SUPPORT_16BPP
	if (src->bpp == 16) {
		vramsub_mixcol16(dst, src, color, alpha64, &mr);
	}
#endif
#ifdef SUPPORT_24BPP
	if (src->bpp == 24) {
		vramsub_mixcol24(dst, src, color, alpha64, &mr);
	}
#endif
}

void vramcpy_zoom(VRAMHDL dst, const POINT_T *pt,
							const VRAMHDL src, const RECT_T *rct,
							int dot) {

	MIX_RECT	mr;

	if ((cpyrect(&mr, dst, pt, src, rct) != SUCCESS) ||
		(dst->bpp != src->bpp)) {
		return;
	}

	if (dot <= 0) {
		vramsub_cpy(dst, src, &mr);
		return;
	}
#ifdef SUPPORT_16BPP
	if (src->bpp == 16) {
		vramsub_zoom16(dst, src, dot, &mr);
	}
#endif
#ifdef SUPPORT_24BPP
	if (src->bpp == 24) {
		vramsub_zoom24(dst, src, dot, &mr);
	}
#endif
}

void vramcpy_mosaic(VRAMHDL dst, const POINT_T *pt, 
							const VRAMHDL src, const RECT_T *rct,
							int dot) {

	MIX_RECT	mr;

	if ((cpyrect(&mr, dst, pt, src, rct) != SUCCESS) ||
		(dst->bpp != src->bpp)) {
		return;
	}

	if (dot <= 0) {
		vramsub_cpy(dst, src, &mr);
		return;
	}
#ifdef SUPPORT_16BPP
	if (src->bpp == 16) {
		vramsub_mosaic16(dst, src, dot, &mr);
	}
#endif
#ifdef SUPPORT_24BPP
	if (src->bpp == 24) {
		vramsub_mosaic24(dst, src, dot, &mr);
	}
#endif
}


// ----

// サーフェスをウィンドウとして使う場合…
// dst:rct <- src(posx, posy)

void vrammix_cpy(VRAMHDL dst, const RECT_T *rct,
							const VRAMHDL src, const POINT_T *pt) {

	MIX_RECT	mr;

	if ((mixrect(&mr, dst, rct, src, pt) != SUCCESS) ||
		(dst->bpp != src->bpp)) {
		return;
	}
	vramsub_cpy(dst, src, &mr);
}

void vrammix_cpyall(VRAMHDL dst, const RECT_T *rct,
							const VRAMHDL src, const POINT_T *pt) {

	MIX_RECT	mr;

	if ((mixrect(&mr, dst, rct, src, pt) != SUCCESS) ||
		(dst->bpp != src->bpp)) {
		return;
	}
	vramsub_cpyall(dst, src, &mr);
}

void vrammix_cpy2(VRAMHDL dst, const RECT_T *rct,
							const VRAMHDL src, const POINT_T *pt,
							UINT alpha) {

	MIX_RECT	mr;

	if ((mixrect(&mr, dst, rct, src, pt) != SUCCESS) ||
		(dst->bpp != src->bpp) || (dst->alpha == NULL)) {
		return;
	}
	vramsub_cpy2(dst, src, alpha, &mr);
}

void vrammix_cpypat(VRAMHDL dst, const RECT_T *rct,
							const VRAMHDL src, const POINT_T *pt,
							const UINT8 *pat8) {

	MIX_RECT	mr;

	if ((mixrect(&mr, dst, rct, src, pt) != SUCCESS) ||
		(dst->bpp != src->bpp)) {
		return;
	}
#ifdef SUPPORT_16BPP
	if (src->bpp == 16) {
		vramsub_cpyp16(dst, src, pat8, &mr);
	}
#endif
#ifdef SUPPORT_24BPP
	if (src->bpp == 24) {
		vramsub_cpyp24(dst, src, pat8, &mr);
	}
#endif
}

void vrammix_cpypat16w(VRAMHDL dst, const RECT_T *rct,
							const VRAMHDL src, const POINT_T *pt,
							const UINT pat16) {

	MIX_RECT	mr;

	if ((mixrect(&mr, dst, rct, src, pt) != SUCCESS) ||
		(dst->bpp != src->bpp)) {
		return;
	}
#ifdef SUPPORT_16BPP
	if (src->bpp == 16) {
		vramsub_cpyp16w16(dst, src, pat16, &mr);
	}
#endif
#ifdef SUPPORT_24BPP
	if (src->bpp == 24) {
		vramsub_cpyp16w24(dst, src, pat16, &mr);
	}
#endif
}

void vrammix_cpypat16h(VRAMHDL dst, const RECT_T *rct,
							const VRAMHDL src, const POINT_T *pt,
							const UINT pat16) {

	MIX_RECT	mr;

	if ((mixrect(&mr, dst, rct, src, pt) != SUCCESS) ||
		(dst->bpp != src->bpp)) {
		return;
	}
#ifdef SUPPORT_16BPP
	if (src->bpp == 16) {
		vramsub_cpyp16h16(dst, src, pat16, &mr);
	}
#endif
#ifdef SUPPORT_24BPP
	if (src->bpp == 24) {
		vramsub_cpyp16h24(dst, src, pat16, &mr);
	}
#endif
}

void vrammix_cpyex(VRAMHDL dst, const RECT_T *rct,
							const VRAMHDL src, const POINT_T *pt) {

	MIX_RECT	mr;

	if ((mixrect(&mr, dst, rct, src, pt) != SUCCESS) ||
		(dst->bpp != src->bpp)) {
		return;
	}
#ifdef SUPPORT_16BPP
	if (src->bpp == 16) {
		if (!src->alpha) {
			vramsub_cpyex16(dst, src, &mr);
		}
		else {
			vramsub_cpyex16a(dst, src, &mr);
		}
	}
#endif
#ifdef SUPPORT_24BPP
	if (src->bpp == 24) {
		if (!src->alpha) {
			vramsub_cpyex24(dst, src, &mr);
		}
		else {
			vramsub_cpyex24a(dst, src, &mr);
		}
	}
#endif
}

void vrammix_cpyex2(VRAMHDL dst, const RECT_T *rct,
							const VRAMHDL src, const POINT_T *pt,
							UINT alpha64) {

	MIX_RECT	mr;

	if ((mixrect(&mr, dst, rct, src, pt) != SUCCESS) ||
		(src->alpha == NULL) || (dst->bpp != src->bpp)) {
		return;
	}
#ifdef SUPPORT_16BPP
	if (src->bpp == 16) {
		vramsub_cpyex16a2(dst, src, alpha64, &mr);
	}
#endif
#ifdef SUPPORT_24BPP
	if (src->bpp == 24) {
		vramsub_cpyex24a2(dst, src, alpha64, &mr);
	}
#endif
}

void vrammix_cpyexpat16w(VRAMHDL dst, const RECT_T *rct,
							const VRAMHDL src, const POINT_T *pt,
							const UINT pat16) {

	MIX_RECT	mr;

	if ((mixrect(&mr, dst, rct, src, pt) != SUCCESS) ||
		(src->alpha == NULL) || (dst->bpp != src->bpp)) {
		return;
	}
#ifdef SUPPORT_16BPP
	if (src->bpp == 16) {
		vramsub_cpyexp16w16(dst, src, pat16, &mr);
	}
#endif
#ifdef SUPPORT_24BPP
	if (src->bpp == 24) {
		vramsub_cpyexp16w24(dst, src, pat16, &mr);
	}
#endif
}

void vrammix_cpyexpat16h(VRAMHDL dst, const RECT_T *rct,
							const VRAMHDL src, const POINT_T *pt,
							const UINT pat16) {

	MIX_RECT	mr;

	if ((mixrect(&mr, dst, rct, src, pt) != SUCCESS) ||
		(src->alpha == NULL) || (dst->bpp != src->bpp)) {
		return;
	}
#ifdef SUPPORT_16BPP
	if (src->bpp == 16) {
		vramsub_cpyexp16h16(dst, src, pat16, &mr);
	}
#endif
#ifdef SUPPORT_24BPP
	if (src->bpp == 24) {
		vramsub_cpyexp16h24(dst, src, pat16, &mr);
	}
#endif
}

void vrammix_mix(VRAMHDL dst, const VRAMHDL org, const RECT_T *rct,
							const VRAMHDL src, const POINT_T *pt,
							UINT alpha64) {

	MIXRECTEX	mr;

	if (mixrectex(&mr, dst, org, rct, src, pt) != SUCCESS) {
		return;
	}
#ifdef SUPPORT_16BPP
	if (src->bpp == 16) {
		vramsub_mix16(dst, org, src, alpha64, &mr);
	}
#endif
#ifdef SUPPORT_24BPP
	if (src->bpp == 24) {
		vramsub_mix24(dst, org, src, alpha64, &mr);
	}
#endif
}

void vrammix_mixcol(VRAMHDL dst, const RECT_T *rct,
							const VRAMHDL src, const POINT_T *pt,
							UINT32 color, UINT alpha64) {

	MIX_RECT	mr;

	if ((mixrect(&mr, dst, rct, src, pt) != SUCCESS) ||
		(dst->bpp != src->bpp)) {
		return;
	}
#ifdef SUPPORT_16BPP
	if (src->bpp == 16) {
		vramsub_mixcol16(dst, src, color, alpha64, &mr);
	}
#endif
#ifdef SUPPORT_24BPP
	if (src->bpp == 24) {
		vramsub_mixcol24(dst, src, color, alpha64, &mr);
	}
#endif
}

void vrammix_mixalpha(VRAMHDL dst, const RECT_T *rct,
							const VRAMHDL src, const POINT_T *pt,
							UINT32 color) {

	MIX_RECT	mr;

	if ((mixrect(&mr, dst, rct, src, pt) != SUCCESS) ||
		(dst->bpp != src->bpp)) {
		return;
	}
#ifdef SUPPORT_16BPP
	if (src->bpp == 16) {
		vramsub_mixalpha16(dst, src, color, &mr);
	}
#endif
#ifdef SUPPORT_24BPP
	if (src->bpp == 24) {
		vramsub_mixalpha24(dst, src, color, &mr);
	}
#endif
}

void vrammix_graybmp(VRAMHDL dst, const VRAMHDL org, const RECT_T *rct,
							const VRAMHDL src, const POINT_T *pt,
							const VRAMHDL bmp, int delta) {

	MIXRECTEX	mr;

	if (mixrectex(&mr, dst, org, rct, src, pt) != SUCCESS) {
		return;
	}
#ifdef SUPPORT_16BPP
	if (src->bpp == 16) {
		vramsub_gray16(dst, org, src, bmp, delta, &mr);
	}
#endif
#ifdef SUPPORT_24BPP
	if (src->bpp == 24) {
		vramsub_gray24(dst, org, src, bmp, delta, &mr);
	}
#endif
}

void vrammix_colex(VRAMHDL dst, const RECT_T *rct,
							const VRAMHDL src, const POINT_T *pt,
							UINT32 color) {

	MIX_RECT	mr;

	if ((mixrect(&mr, dst, rct, src, pt) != SUCCESS) || (src->bpp != 8)) {
		return;
	}

#ifdef SUPPORT_16BPP
	if (dst->bpp == 16) {
		vramsub_colex16(dst, src, color, &mr);
	}
#endif
#ifdef SUPPORT_24BPP
	if (dst->bpp == 24) {
		vramsub_colex24(dst, src, color, &mr);
	}
#endif
}


// ---- resize

#ifdef SUPPORT_16BPP
static void vramsub_resize16(VRAMHDL dst, MIX_RECT *drct,
								const VRAMHDL src, const MIX_RECT *srct) {

const UINT8	*p;
	UINT8	*q;
const UINT8	*r;
const UINT8	*s;
	int		dstep;
	int		xstep;
	int		ystep;
	int		xx;
	int		yy;
	int		x;

	p = src->ptr + (srct->srcpos * 2);
	q = dst->ptr + (drct->dstpos * 2);
	dstep = dst->yalign - (drct->width * 2);

	xstep = (srct->width << 10) / drct->width;
	ystep = (srct->height << 10) / drct->height;
	yy = 0;
	do {
		xx = 0;
		r = p;
		r += (yy >> 10) * src->yalign;
		x = drct->width;
		do {
			s = r + ((xx >> 10) * 2);
			*(UINT16 *)q = *(UINT16 *)s;
			xx += xstep;
			q += 2;
		} while(--x);
		yy += ystep;
		q += dstep;
	} while(--drct->height);
}
#endif

#ifdef SUPPORT_24BPP
static void vramsub_resize24(VRAMHDL dst, MIX_RECT *drct,
								const VRAMHDL src, const MIX_RECT *srct) {

const UINT8	*p;
	UINT8	*q;
const UINT8	*r;
const UINT8	*s;
	int		dstep;
	int		xstep;
	int		ystep;
	int		xx;
	int		yy;
	int		x;

	p = src->ptr + (srct->srcpos * 3);
	q = dst->ptr + (drct->dstpos * 3);
	dstep = dst->yalign - (drct->width * 3);

	xstep = (srct->width << 10) / drct->width;
	ystep = (srct->height << 10) / drct->height;
	yy = 0;
	do {
		xx = 0;
		r = p;
		r += (yy >> 10) * src->yalign;
		x = drct->width;
		do {
			s = r + ((xx >> 10) * 3);
			q[0] = s[0];
			q[1] = s[1];
			q[2] = s[2];
			xx += xstep;
			q += 3;
		} while(--x);
		yy += ystep;
		q += dstep;
	} while(--drct->height);
}
#endif

static BRESULT cliprect(const VRAMHDL hdl, const RECT_T *rct, MIX_RECT *r) {

	RECT_T	rect;

	if (vram_cliprect(&rect, hdl, rct) != SUCCESS) {
		return(FAILURE);
	}
	r->srcpos = (rect.top * hdl->width) + rect.left;
	r->dstpos = r->srcpos;
	r->width = rect.right - rect.left;
	r->height = rect.bottom - rect.top;
	return(SUCCESS);
}

void vrammix_resize(VRAMHDL dst, const RECT_T *drct,
									const VRAMHDL src, const RECT_T *srct) {

	MIX_RECT	drect;
	MIX_RECT	srect;

	if ((cliprect(src, srct, &srect) != SUCCESS) ||
		(cliprect(dst, drct, &drect) != SUCCESS)) {
		return;
	}
	if (dst->bpp != src->bpp) {
		return;
	}
#ifdef SUPPORT_16BPP
	if (src->bpp == 16) {
		vramsub_resize16(dst, &drect, src, &srect);
	}
#endif
#ifdef SUPPORT_24BPP
	if (src->bpp == 24) {
		vramsub_resize24(dst, &drect, src, &srect);
	}
#endif
}


// ---- font

static BRESULT txtrect(VRAMHDL dst, const FNTDAT fnt, const POINT_T *pt,
										const RECT_T *rct, MIX_RECT *r) {

	int		pos;

	r->srcpos = 0;
	r->dstpos = pt->y * dst->width;
	r->dstpos += pt->x;

	pos = pt->y - rct->top;
	if (pos < 0) {
		r->srcpos -= pos * fnt->width;
		r->height = min(fnt->height + pos, rct->bottom - rct->top);
	}
	else {
		r->height = min(rct->bottom - rct->top - pos, fnt->height);
	}
	if (r->height <= 0) {
		return(FAILURE);
	}

	pos = pt->x - rct->left;
	if (pos < 0) {
		r->srcpos -= pos;
		r->width = min(fnt->width + pos, rct->right - rct->left);
	}
	else {
		r->width = min(rct->right - rct->left - pos, fnt->width);
	}
	if (r->width <= 0) {
		return(FAILURE);
	}
	return(SUCCESS);
}

static void vramsub_txt8p(VRAMHDL dst, const FNTDAT fnt,
												UINT32 color, MIX_RECT *mr) {

const UINT8	*p;
	UINT8	*q;
	UINT	alpha;
	int		cnt;

	p = (UINT8 *)(fnt + 1);
	p += mr->srcpos;
	q = dst->ptr + mr->dstpos;
	do {
		cnt = mr->width;
		do {
			alpha = *p++;
			if (alpha) {
				alpha = alpha * color / FDAT_DEPTH;
				q[0] = (UINT8)alpha;
			}
			q += 1;
		} while(--cnt);
		p += fnt->width - mr->width;
		q += dst->width - mr->width;
	} while(--mr->height);
}

#ifdef SUPPORT_16BPP
static void vramsub_txt16p(VRAMHDL dst, const FNTDAT fnt,
												UINT32 color, MIX_RECT *mr) {

const UINT8	*p;
	UINT8	*q;
	UINT	alpha;
	int		cnt;
	UINT	col16;
	int		c16[3];

	col16 = MAKE16PAL(color);
	c16[0] = col16 & B16MASK;
	c16[1] = col16 & G16MASK;
	c16[2] = col16 & R16MASK;

	p = (UINT8 *)(fnt + 1);
	p += mr->srcpos;
	q = dst->ptr + (mr->dstpos * 2);
	do {
		cnt = mr->width;
		do {
			alpha = *p++;
			if (alpha) {
				alpha = FDAT_DEPTH - alpha;
				if (!alpha) {
					*(UINT16 *)q = (UINT16)col16;
				}
				else {
					UINT d, s;
					s = *(UINT16 *)q;
					d = MAKEALPHA16s(c16[0], s, B16MASK,
														alpha, FDAT_DEPTHBIT);
					d |= MAKEALPHA16s(c16[1], s, G16MASK,
														alpha, FDAT_DEPTHBIT);
					d |= MAKEALPHA16s(c16[2], s, R16MASK,
														alpha, FDAT_DEPTHBIT);
					*(UINT16 *)q = (UINT16)d;
				}
			}
			q += 2;
		} while(--cnt);
		p += fnt->width - mr->width;
		q += (dst->width - mr->width) * 2;
	} while(--mr->height);
}

static void vramsub_txt16a(VRAMHDL dst, const FNTDAT fnt,
												UINT32 color, MIX_RECT *mr) {

const UINT8	*p;
	UINT8	*q;
	UINT8	*a;
	UINT	alpha;
	int		cnt;
	UINT	col16;
	int		c16[3];

	col16 = MAKE16PAL(color);
	c16[0] = col16 & B16MASK;
	c16[1] = col16 & G16MASK;
	c16[2] = col16 & R16MASK;

	p = (UINT8 *)(fnt + 1);
	p += mr->srcpos;
	q = dst->ptr + (mr->dstpos * 2);
	a = dst->alpha + mr->dstpos;
	do {
		cnt = mr->width;
		do {
			alpha = *p++;
			if (alpha) {
				alpha = FDAT_DEPTH - alpha;
				if (!alpha) {
					*(UINT16 *)q = (UINT16)col16;
				}
				else {
					UINT d, s;
					s = *(UINT16 *)q;
					d = MAKEALPHA16s(c16[0], s, B16MASK,
														alpha, FDAT_DEPTHBIT);
					d |= MAKEALPHA16s(c16[1], s, G16MASK,
														alpha, FDAT_DEPTHBIT);
					d |= MAKEALPHA16s(c16[2], s, R16MASK,
														alpha, FDAT_DEPTHBIT);
					*(UINT16 *)q = (UINT16)d;
				}
				a[0] = VRAMALPHA;
			}
			q += 2;
			a += 1;
		} while(--cnt);
		p += fnt->width - mr->width;
		q += (dst->width - mr->width) * 2;
		a += dst->width - mr->width;
	} while(--mr->height);
}

static void vramsub_txt16e(VRAMHDL dst, const FNTDAT fnt,
												UINT32 color, MIX_RECT *mr) {

const UINT8	*p;
	UINT8	*q;
	UINT8	*a;
	UINT	alpha;
	int		cnt;
	UINT	col16;
	int		c16[3];

	col16 = MAKE16PAL(color);
	c16[0] = col16 & B16MASK;
	c16[1] = col16 & G16MASK;
	c16[2] = col16 & R16MASK;

	p = (UINT8 *)(fnt + 1);
	p += mr->srcpos;
	q = dst->ptr + (mr->dstpos * 2);

	a = dst->alpha + mr->dstpos;
	do {
		cnt = mr->width;
		do {
			alpha = (*p++) * VRAMALPHA / FDAT_DEPTH;
			if (alpha) {
				*(UINT16 *)q = (UINT16)col16;
				a[0] = (UINT8)alpha;
			}
			q += 2;
			a += 1;
		} while(--cnt);
		p += fnt->width - mr->width;
		q += (dst->width - mr->width) * 2;
		a += dst->width - mr->width;
	} while(--mr->height);
}
#endif

#ifdef SUPPORT_24BPP
static void vramsub_txt24p(VRAMHDL dst, const FNTDAT fnt,
												UINT32 color, MIX_RECT *mr) {

const UINT8	*p;
	UINT8	*q;
	UINT	alpha;
	int		cnt;
	int		c24[3];

	p = (UINT8 *)(fnt + 1);
	p += mr->srcpos;
	q = dst->ptr + (mr->dstpos * 3);
	c24[0] = color & 0xff;
	c24[1] = (color >> 8) & 0xff;
	c24[2] = (color >> 16) & 0xff;
	do {
		cnt = mr->width;
		do {
			alpha = *p++;
			if (alpha) {
				if (alpha == FDAT_DEPTH) {
					q[0] = (UINT8)c24[0];
					q[1] = (UINT8)c24[1];
					q[2] = (UINT8)c24[2];
				}
				else {
					alpha += FDATDEPTHBASE;
					q[0] = (UINT8)MAKEALPHA24(q[0], c24[0],
														alpha, FDAT_DEPTHBIT);
					q[1] = (UINT8)MAKEALPHA24(q[1], c24[1],
														alpha, FDAT_DEPTHBIT);
					q[2] = (UINT8)MAKEALPHA24(q[2], c24[2],
														alpha, FDAT_DEPTHBIT);
				}
			}
			q += 3;
		} while(--cnt);
		p += (fnt->width - mr->width);
		q += (dst->width - mr->width) * 3;
	} while(--mr->height);
}

static void vramsub_txt24a(VRAMHDL dst, const FNTDAT fnt,
												UINT32 color, MIX_RECT *mr) {

const UINT8	*p;
	UINT8	*q;
	UINT8	*a;
	UINT	alpha;
	int		cnt;
	int		c24[3];

	p = (UINT8 *)(fnt + 1);
	p += mr->srcpos;
	q = dst->ptr + (mr->dstpos * 3);
	a = dst->alpha + mr->dstpos;
	c24[0] = color & 0xff;
	c24[1] = (color >> 8) & 0xff;
	c24[2] = (color >> 16) & 0xff;
	do {
		cnt = mr->width;
		do {
			alpha = *p++;
			if (alpha) {
				if (alpha == FDAT_DEPTH) {
					q[0] = (UINT8)c24[0];
					q[1] = (UINT8)c24[1];
					q[2] = (UINT8)c24[2];
				}
				else {
					alpha += FDATDEPTHBASE;
					q[0] = (UINT8)MAKEALPHA24(q[0], c24[0],
														alpha, FDAT_DEPTHBIT);
					q[1] = (UINT8)MAKEALPHA24(q[1], c24[1],
														alpha, FDAT_DEPTHBIT);
					q[2] = (UINT8)MAKEALPHA24(q[2], c24[2],
														alpha, FDAT_DEPTHBIT);
				}
				a[0] = VRAMALPHA;
			}
			q += 3;
			a += 1;
		} while(--cnt);
		p += (fnt->width - mr->width);
		q += (dst->width - mr->width) * 3;
		a += dst->width - mr->width;
	} while(--mr->height);
}

static void vramsub_txt24e(VRAMHDL dst, const FNTDAT fnt,
												UINT32 color, MIX_RECT *mr) {

const UINT8	*p;
	UINT8	*q;
	UINT8	*a;
	UINT	alpha;
	int		cnt;
	int		c24[3];

	p = (UINT8 *)(fnt + 1);
	p += mr->srcpos;
	q = dst->ptr + (mr->dstpos * 3);
	c24[0] = color & 0xff;
	c24[1] = (color >> 8) & 0xff;
	c24[2] = (color >> 16) & 0xff;
	a = dst->alpha + mr->dstpos;
	do {
		cnt = mr->width;
		do {
			alpha = (*p++) * VRAMALPHA / FDAT_DEPTH;
			if (alpha) {
				q[0] = (UINT8)c24[0];
				q[1] = (UINT8)c24[1];
				q[2] = (UINT8)c24[2];
				a[0] = (UINT8)alpha;
			}
			q += 3;
			a += 1;
		} while(--cnt);
		p += (fnt->width - mr->width);
		q += (dst->width - mr->width) * 3;
		a += dst->width - mr->width;
	} while(--mr->height);
}
#endif

static void vramsub_text(VRAMHDL dst, void *fhdl, const OEMCHAR *str,
							UINT32 color, POINT_T *pt, const RECT_T *rct,
							void (*func)(VRAMHDL dst, const FNTDAT fnt,
											UINT32 color, MIX_RECT *mr)) {

	int			leng;
	OEMCHAR		buf[4];
	RECT_T		rect;
	FNTDAT		fnt;
	MIX_RECT	mr;

	if ((str == NULL) || (pt == NULL) || (func == NULL) ||
		(vram_cliprect(&rect, dst, rct) != SUCCESS)) {
		goto vstxt_end;
	}

	while(1) {
		leng = milstr_charsize(str);
		if (!leng) {
			break;
		}
		CopyMemory(buf, str, leng * sizeof(OEMCHAR));
		buf[leng] = '\0';
		str += leng;
		fnt = fontmng_get(fhdl, buf);
		if (fnt) {
			if (txtrect(dst, fnt, pt, &rect, &mr) == SUCCESS) {
				func(dst, fnt, color, &mr);
			}
			pt->x += fnt->pitch;
		}
	}

vstxt_end:
	return;
}

void vrammix_text(VRAMHDL dst, void *fhdl, const OEMCHAR *str,
							UINT32 color, POINT_T *pt, const RECT_T *rct) {

	void	(*func)(VRAMHDL dst, const FNTDAT fnt,
											UINT32 color, MIX_RECT *mr);

	if (dst == NULL) {
		return;
	}
	func = NULL;
	if (dst->bpp == 8) {
		func = vramsub_txt8p;
	}
#ifdef SUPPORT_16BPP
	if (dst->bpp == 16) {
		if (dst->alpha) {
			func = vramsub_txt16a;
		}
		else {
			func = vramsub_txt16p;
		}
	}
#endif
#ifdef SUPPORT_24BPP
	if (dst->bpp == 24) {
		if (dst->alpha) {
			func = vramsub_txt24a;
		}
		else {
			func = vramsub_txt24p;
		}
	}
#endif
	vramsub_text(dst, fhdl, str, color, pt, rct, func);
}

void vrammix_textex(VRAMHDL dst, void *fhdl, const OEMCHAR *str,
							UINT32 color, POINT_T *pt, const RECT_T *rct) {

	void	(*func)(VRAMHDL dst, const FNTDAT fnt,
											UINT32 color, MIX_RECT *mr);

	if (dst == NULL) {
		return;
	}
	func = NULL;
	if (dst->bpp == 8) {
		func = vramsub_txt8p;
	}
#ifdef SUPPORT_16BPP
	if (dst->bpp == 16) {
		if (dst->alpha) {
			func = vramsub_txt16e;
		}
		else {
			func = vramsub_txt16p;
		}
	}
#endif
#ifdef SUPPORT_24BPP
	if (dst->bpp == 24) {
		if (dst->alpha) {
			func = vramsub_txt24e;
		}
		else {
			func = vramsub_txt24p;
		}
	}
#endif
	vramsub_text(dst, fhdl, str, color, pt, rct, func);
}

