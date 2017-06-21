#include	"compiler.h"
#include	"vramhdl.h"
#include	"vrammix.h"
#include	"menudeco.inc"
#include	"menubase.h"


UINT32	menucolor[] = {
						0xffffff,			// MVC_BACK
						0xffffff,			// MVC_HILIGHT
						0xc0c0c0,			// MVC_LIGHT
						0x808080,			// MVC_SHADOW
						0x000000,			// MVC_DARK
						0xe0e0e0,			// MVC_SCROLLBAR

						0xc0c0c0,			// MVC_STATIC
						0x000000,			// MVC_TEXT
						0x808080,			// MVC_GRAYTEXT1
						0xffffff,			// MVC_GRAYTEXT2
						0xc0c0c0,			// MVC_BTNFACE
						0xffffff,			// MVC_CURTEXT
						0x000080,			// MVC_CURBACK
};


static const UINT8 __pat[64] = {
				0x00, 0x00, 0x00, 0x00,
				0x11, 0x00, 0x00, 0x00,
				0x11, 0x00, 0x44, 0x00,
				0x55, 0x00, 0x44, 0x00,
				0x55, 0x00, 0x55, 0x00,
				0x55, 0x22, 0x55, 0x00,
				0x55, 0x22, 0x55, 0x88,
				0x55, 0xaa, 0x55, 0x88,
				0x55, 0xaa, 0x55, 0xaa,
				0x77, 0xaa, 0x55, 0xaa,
				0x77, 0xaa, 0xdd, 0xaa,
				0xff, 0xaa, 0xdd, 0xaa,
				0xff, 0xaa, 0xff, 0xaa,
				0xff, 0xbb, 0xff, 0xaa,
				0xff, 0xbb, 0xff, 0xee,
				0xff, 0xff, 0xff, 0xee};


typedef struct {
	int		width;
	int		height;
	int		pos;
	int		step;
	int		linedel;
} RESPUT;

static BRESULT resputprepare(VRAMHDL vram, const MENURES2 *res,
											const POINT_T *pt, RESPUT *rp) {

	int		width;
	int		height;
	int		step;
	int		pos;

	if ((vram == NULL) || (res == NULL)) {
		goto rpp_err;
	}
	width = res->width;
	height = res->height;
	step = 0;
	pos = pt->x;
	if (pos < 0) {
		width += pos;
		step += pos;
		pos = 0;
	}
	rp->pos = (pos * vram->xalign);
	pos = vram->width - pos;
	if (width > pos) {
		width = pos;
	}
	if (width <= 0) {
		goto rpp_err;
	}
	rp->width = width;

	pos = pt->y;
	if (pos < 0) {
		height += pos;
		step += pos * res->width;
		pos = 0;
	}
	rp->pos += (pos * vram->yalign);
	pos = vram->height - pos;
	if (height > pos) {
		height = pos;
	}
	if (height <= 0) {
		goto rpp_err;
	}
	rp->height = height;
	rp->step = step;
	rp->linedel = width - res->width;
	return(SUCCESS);

rpp_err:
	return(FAILURE);
}


#ifdef SUPPORT_16BPP

static const UINT16 menucolor16[] = {
				MAKE16PAL(0xffffff),		// MVC_BACK
				MAKE16PAL(0xffffff),		// MVC_HILIGHT
				MAKE16PAL(0xc0c0c0),		// MVC_LIGHT
				MAKE16PAL(0x808080),		// MVC_SHADOW
				MAKE16PAL(0x000000),		// MVC_DARK
				MAKE16PAL(0xe0e0e0),		// MVC_SCROLLBAR

				MAKE16PAL(0xc0c0c0),		// MVC_STATIC
				MAKE16PAL(0x000000),		// MVC_TEXT
				MAKE16PAL(0x808080),		// MVC_GRAYTEXT1
				MAKE16PAL(0xffffff),		// MVC_GRAYTEXT2
				MAKE16PAL(0xc0c0c0),		// MVC_BTNFACE
				MAKE16PAL(0xffffff),		// MVC_CURTEXT
				MAKE16PAL(0x000080),		// MVC_CURBACK
};

static const int __rsft[] = {1, 7, 12};

static void res2put16(VRAMHDL vram, const MENURES2 *res, RESPUT *rp) {

	int		width;
const UINT8	*p;
	UINT8	*q;
	int		cnt;
	int		step;
	int		bit;
	int		pix = 0;				// for cygwin
	int		c;
	UINT16	dat;

	p = res->pat;
	q = vram->ptr + rp->pos;
	step = vram->yalign - (rp->width * 2);
	cnt = rp->step;
	width = rp->width;
	bit = 0;
	while(1) {
		while(cnt <= 0) {
			cnt++;
			pix = ((*p) >> bit) & 0x0f;
			bit ^= 4;
			if (!bit) {
				p++;
			}
			if (pix >= 7) {
				cnt += (pix - 7) + 2;
				pix = ((*p) >> bit) & 0x0f;
				bit ^= 4;
				if (!bit) {
					p++;
				}
			}
		}
		c = min(cnt, width);
		cnt -= c;
		width -= c;
		if (pix) {
			dat = menucolor16[pix - 1];
			do {
				*(UINT16 *)q = (UINT16)dat;
				q += 2;
			} while(--c);
		}
		else {
			q += c * 2;
		}
		if (!width) {
			rp->height--;
			if (!rp->height) {
				break;
			}
			width = rp->width;
			cnt += rp->linedel;
			q += step;
		}
	}
}

static void res3put16(VRAMHDL vram, const MENURES2 *res, RESPUT *rp,
																UINT mvc) {

	int		width;
const UINT8	*p;
	UINT8	*q;
	int		cnt;
	int		step;
	int		bit;
	int		pix;
	int		c;
	UINT16	dat;

	dat = menucolor16[mvc];
	p = res->pat;
	q = vram->ptr + rp->pos;
	step = vram->yalign - (rp->width * 2);
	cnt = rp->step;
	width = rp->width;
	pix = 0;
	bit = 0;
	while(1) {
		while(cnt <= 0) {
			pix ^= 1;
			c = ((*p) >> bit) & 0x0f;
			bit ^= 4;
			if (!bit) {
				p++;
			}
			if (c & 8) {
				c -= 8;
				c <<= 4;
				c |= ((*p) >> bit) & 0x0f;
				bit ^= 4;
				if (!bit) {
					p++;
				}
			}
			cnt += c;
		}
		c = min(cnt, width);
		cnt -= c;
		width -= c;
		if (!pix) {
			do {
				*(UINT16 *)q = (UINT16)dat;
				q += 2;
			} while(--c);
		}
		else {
			q += c * 2;
		}
		if (!width) {
			rp->height--;
			if (!rp->height) {
				break;
			}
			width = rp->width;
			cnt += rp->linedel;
			q += step;
		}
	}
}

static void captionbar16(VRAMHDL vram, const RECT_T *rect,
											UINT32 color1, UINT32 color2) {

	RECT_T	rct;
	int		width;
	int		height;
	int		i;
	int		x;
	int		y;
	int		col[3];
	int		step[3];
	int		dir[3];
	int		tmp;
	int		sft;
	UINT8	*p;
	UINT8	*q;
const UINT8	*r;
	UINT	pat[4];
	int		cur;
	UINT8	mask;
	int		c;

	if (vram_cliprect(&rct, vram, rect) != SUCCESS) {
		goto mvcb_end;
	}
	width = rct.right - rct.left;
	height = rct.bottom - rct.top;
	i = 0;
	do {
		sft = (i << 3) + 4;
		col[i] = ((color1 >> sft) & 0xf);
		tmp = ((color2 >> sft) & 0xf) - col[i];
		if (tmp == 0) {
			dir[i] = 0;
			step[i] = 0;
		}
		else {
			if (tmp > 0) {
				dir[i] = 1;
			}
			else {
				dir[i] = -1;
				tmp = 0 - tmp;
			}
			tmp <<= 4;			// x16
			tmp <<= 16;
			step[i] = tmp / width;
		}
	} while(++i < 3);
	p = vram->ptr;
	p += rct.left * vram->xalign;
	p += rct.top * vram->yalign;
	x = 0;
	do {
		q = p;
		p += 2;
		y = 0;
		do {
			pat[y] = 0;
		} while(++y < 4);
		mask = 0x80 >> (x & 7);
		i = 0;
		do {
			cur = (x * step[i]) >> 16;
			r = __pat + ((cur & 15) << 2);
			cur >>= 4;
			cur *= dir[i];
			cur += col[i];
			y = 0;
			do {
				c = cur;
				if (r[y] & mask) {
					c += dir[i];
				}
				pat[y] |= (c << __rsft[i]);
			} while(++y < 4);
		} while(++i < 3);
		y = 0;
		do {
			q[0] = (UINT8)pat[y & 3];
			q[1] = (UINT8)(pat[y & 3] >> 8);
			q += vram->yalign;
		} while(++y < height);
	} while(++x < width);

mvcb_end:
	return;
}

#endif

#ifdef SUPPORT_24BPP

static void res2put24(VRAMHDL vram, const MENURES2 *res, RESPUT *rp) {

	int		width;
const UINT8	*p;
	UINT8	*q;
	int		cnt;
	int		step;
	int		bit;
	int		pix;
	int		c;
	UINT32	dat;

	p = res->pat;
	q = vram->ptr + rp->pos;
	step = vram->yalign - (rp->width * 3);
	cnt = rp->step;
	width = rp->width;
	bit = 0;
	pix = 0;
	while(1) {
		while(cnt <= 0) {
			cnt++;
			pix = ((*p) >> bit) & 0x0f;
			bit ^= 4;
			if (!bit) {
				p++;
			}
			if (pix >= 7) {
				cnt += (pix - 7) + 2;
				pix = ((*p) >> bit) & 0x0f;
				bit ^= 4;
				if (!bit) {
					p++;
				}
			}
		}
		c = min(cnt, width);
		cnt -= c;
		width -= c;
		if (pix) {
			dat = menucolor[pix - 1];
			do {
				*q++ = (UINT8)dat;
				*q++ = (UINT8)(dat >> 8);
				*q++ = (UINT8)(dat >> 16);
			} while(--c);
		}
		else {
			q += c * 3;
		}
		if (!width) {
			rp->height--;
			if (!rp->height) {
				break;
			}
			width = rp->width;
			cnt += rp->linedel;
			q += step;
		}
	}
}

static void res3put24(VRAMHDL vram, const MENURES2 *res, RESPUT *rp,
																UINT mvc) {

	int		width;
const UINT8	*p;
	UINT8	*q;
	int		cnt;
	int		step;
	int		bit;
	int		pix;
	int		c;
	UINT32	dat;

	dat = menucolor[mvc];
	p = res->pat;
	q = vram->ptr + rp->pos;
	step = vram->yalign - (rp->width * 3);
	cnt = rp->step;
	width = rp->width;
	pix = 0;
	bit = 0;
	while(1) {
		while(cnt <= 0) {
			pix ^= 1;
			c = ((*p) >> bit) & 0x0f;
			bit ^= 4;
			if (!bit) {
				p++;
			}
			if (c & 8) {
				c -= 8;
				c <<= 4;
				c |= ((*p) >> bit) & 0x0f;
				bit ^= 4;
				if (!bit) {
					p++;
				}
			}
			cnt += c;
		}
		c = min(cnt, width);
		cnt -= c;
		width -= c;
		if (!pix) {
			do {
				*q++ = (UINT8)dat;
				*q++ = (UINT8)(dat >> 8);
				*q++ = (UINT8)(dat >> 16);
			} while(--c);
		}
		else {
			q += c * 3;
		}
		if (!width) {
			rp->height--;
			if (!rp->height) {
				break;
			}
			width = rp->width;
			cnt += rp->linedel;
			q += step;
		}
	}
}

static void captionbar24(VRAMHDL vram, const RECT_T *rect,
											UINT32 color1, UINT32 color2) {

	RECT_T	rct;
	int		width;
	int		height;
	int		i;
	int		x;
	int		y;
	int		col[3];
	int		step[3];
	int		dir[3];
	int		tmp;
	int		sft;
	UINT8	*p;
	UINT8	*q;
const UINT8	*r;
	UINT8	pat[3][4];
	int		cur;
	UINT8	mask;
	int		c;

	if (vram_cliprect(&rct, vram, rect) != SUCCESS) {
		goto mvcb_end;
	}
	width = rct.right - rct.left;
	height = rct.bottom - rct.top;
	i = 0;
	do {
		sft = (i << 3) + 4;
		col[i] = ((color1 >> sft) & 0xf);
		tmp = ((color2 >> sft) & 0xf) - col[i];
		if (tmp == 0) {
			dir[i] = 0;
			step[i] = 0;
		}
		else {
			if (tmp > 0) {
				dir[i] = 1;
			}
			else {
				dir[i] = -1;
				tmp = 0 - tmp;
			}
			tmp <<= 4;			// x16
			tmp <<= 16;
			step[i] = tmp / width;
		}
	} while(++i < 3);
	p = vram->ptr;
	p += rct.left * vram->xalign;
	p += rct.top * vram->yalign;
	x = 0;
	do {
		q = p;
		p += 3;
		mask = 0x80 >> (x & 7);
		i = 0;
		do {
			cur = (x * step[i]) >> 16;
			r = __pat + ((cur & 15) << 2);
			cur >>= 4;
			cur *= dir[i];
			cur += col[i];
			y = 0;
			do {
				c = cur;
				if (r[y] & mask) {
					c += dir[i];
				}
				pat[i][y] = (UINT8)((c << 4) | c);
			} while(++y < 4);
		} while(++i < 3);
		y = 0;
		do {
			q[0] = pat[0][y & 3];
			q[1] = pat[1][y & 3];
			q[2] = pat[2][y & 3];
			q += vram->yalign;
		} while(++y < height);
	} while(++x < width);

mvcb_end:
	return;
}

#endif


// ----

static void vramlzxsolve(UINT8 *ptr, int size, const UINT8 *dat) {

	int		level;
	UINT8	ctrl;
	UINT8	bit;
	UINT	mask;
	UINT	tmp;
	int		pos;
	int		leng;

	ctrl = 0;
	bit = 0;
	level = *dat++;
	mask = (1 << level) - 1;
	while(size) {
		if (!bit) {
			ctrl = *dat++;
			bit = 0x80;
		}
		if (ctrl & bit) {
			tmp = *dat++;
			tmp <<= 8;
			tmp |= *dat++;
			pos = -1 - (tmp >> level);
			leng = (tmp & mask) + 1;
			leng = min(leng, size);
			size -= leng;
			while(leng--) {
				*ptr = *(ptr + pos);
				ptr++;
			}
		}
		else {
			*ptr++ = *dat++;
			size--;
		}
		bit >>= 1;
	}
}


// ----

VRAMHDL menuvram_resload(const MENURES *res, int bpp) {

	VRAMHDL	ret;
	int		size;
	BOOL	alpha;

	alpha = (res->alpha)?TRUE:FALSE;
	ret = vram_create(res->width, res->height, alpha, bpp);
	if (ret == NULL) {
		goto mvrl_err;
	}
	size = res->width * res->height;
	vramlzxsolve(ret->ptr, size * ret->xalign, res->data);
	if (alpha) {
		vramlzxsolve(ret->alpha, size, res->alpha);
	}

mvrl_err:
	return(ret);
}


void menuvram_res2put(VRAMHDL vram, const MENURES2 *res, const POINT_T *pt) {

	RESPUT	rp;

	if (resputprepare(vram, res, pt, &rp) != SUCCESS) {
		goto mvr2_end;
	}
	switch(vram->bpp) {
#ifdef SUPPORT_16BPP
		case 16:
			res2put16(vram, res, &rp);
			break;
#endif
#ifdef SUPPORT_24BPP
		case 24:
			res2put24(vram, res, &rp);
			break;
#endif
		default:
			TRACEOUT(("menuvram_res2put: unspport %dbpp", vram->bpp));
			break;
	}

mvr2_end:
	return;
}


void menuvram_res3put(VRAMHDL vram, const MENURES2 *res, const POINT_T *pt,
																UINT mvc) {

	RESPUT	rp;

	if (resputprepare(vram, res, pt, &rp) != SUCCESS) {
		goto mvr3_end;
	}
	switch(vram->bpp) {
#ifdef SUPPORT_16BPP
		case 16:
			res3put16(vram, res, &rp, mvc);
			break;
#endif
#ifdef SUPPORT_24BPP
		case 24:
			res3put24(vram, res, &rp, mvc);
			break;
#endif
		default:
			TRACEOUT(("menuvram_res3put: unspport %dbpp", vram->bpp));
			break;
	}

mvr3_end:
	return;
}


// ----

void menuvram_linex(VRAMHDL vram, int posx, int posy, int term, UINT mvc) {

	UINT8	*p;

	if ((vram == NULL) ||
		(posy < 0) || (posy >= vram->height)) {
		goto mvlx_end;
	}
	if (posx < 0) {
		posx = 0;
	}
	if (term >= vram->width) {
		term = vram->width;
	}
	p = vram->ptr;
	p += posy * vram->yalign;
	p += posx * vram->xalign;

#ifdef SUPPORT_16BPP
	if (vram->bpp == 16) {
		UINT16 color;
		color = menucolor16[mvc];
		while(posx < term) {
			posx++;
			*(UINT16 *)p = (UINT16)color;
			p += 2;
		}
	}
#endif
#ifdef SUPPORT_24BPP
	if (vram->bpp == 24) {
		UINT32 color;
		UINT8 col[3];
		color = menucolor[mvc];
		col[0] = (UINT8)color;
		col[1] = (UINT8)(color >> 8);
		col[2] = (UINT8)(color >> 16);
		while(posx < term) {
			posx++;
			p[0] = col[0];
			p[1] = col[1];
			p[2] = col[2];
			p += 3;
		}
	}
#endif

mvlx_end:
	return;
}


void menuvram_liney(VRAMHDL vram, int posx, int posy, int term, UINT mvc) {

	UINT8	*p;

	if ((vram == NULL) ||
		(posx < 0) || (posx >= vram->width)) {
		goto mvly_end;
	}
	if (posy < 0) {
		posy = 0;
	}
	if (term >= vram->height) {
		term = vram->height;
	}
	p = vram->ptr;
	p += posy * vram->yalign;
	p += posx * vram->xalign;

#ifdef SUPPORT_16BPP
	if (vram->bpp == 16) {
		UINT16 color;
		color = menucolor16[mvc];
		while(posy < term) {
			posy++;
			*(UINT16 *)p = (UINT16)color;
			p += vram->yalign;
		}
	}
#endif
#ifdef SUPPORT_24BPP
	if (vram->bpp == 24) {
		UINT32 color;
		UINT8 col[3];
		color = menucolor[mvc];
		col[0] = (UINT8)color;
		col[1] = (UINT8)(color >> 8);
		col[2] = (UINT8)(color >> 16);
		while(posy < term) {
			posy++;
			p[0] = col[0];
			p[1] = col[1];
			p[2] = col[2];
			p += vram->yalign;
		}
	}
#endif

mvly_end:
	return;
}


void menuvram_box(VRAMHDL vram, const RECT_T *rect, UINT mvc2, int reverse) {

	UINT	c1;
	UINT	c2;

	if (rect == NULL) {
		goto mvb_exit;
	}

	if (!reverse) {
		c1 = mvc2 & 0x0f;
		c2 = (mvc2 >> 4) & 0x0f;
	}
	else {
		c1 = (mvc2 >> 4) & 0x0f;
		c2 = mvc2 & 0x0f;
	}
	menuvram_linex(vram, rect->left+0, rect->top+0, rect->right-1, c1);
	menuvram_liney(vram, rect->left+0, rect->top+1, rect->bottom-1, c1);
	menuvram_linex(vram, rect->left+0, rect->bottom-1, rect->right-1, c2);
	menuvram_liney(vram, rect->right-1, rect->top+0, rect->bottom-0, c2);

mvb_exit:
	return;
}


void menuvram_box2(VRAMHDL vram, const RECT_T *rect, UINT mvc4) {

	UINT	col;

	if (rect == NULL) {
		goto mvb2_exit;
	}

	col = mvc4 & 0x0f;
	menuvram_linex(vram, rect->left+0, rect->top+0, rect->right-1, col);
	menuvram_liney(vram, rect->left+0, rect->top+1, rect->bottom-1, col);

	col = (mvc4 >> 4) & 0x0f;
	menuvram_linex(vram, rect->left+0, rect->bottom-1, rect->right-1, col);
	menuvram_liney(vram, rect->right-1, rect->top+0, rect->bottom-0, col);

	col = (mvc4 >> 8) & 0x0f;
	menuvram_linex(vram, rect->left+1, rect->top+1, rect->right-2, col);
	menuvram_liney(vram, rect->left+1, rect->top+2, rect->bottom-2, col);

	col = (mvc4 >> 12) & 0x0f;
	menuvram_linex(vram, rect->left+1, rect->bottom-2, rect->right-2, col);
	menuvram_liney(vram, rect->right-2, rect->top+1, rect->bottom-1, col);

mvb2_exit:
	return;
}


void menuvram_base(VRAMHDL vram) {

	RECT_T	rct;

	vram_filldat(vram, NULL, menucolor[MVC_STATIC]);
	rct.left = 0;
	rct.top = 0;
	rct.right = vram->width;
	rct.bottom = vram->height;
	menuvram_box2(vram, &rct,
						MVC4(MVC_LIGHT, MVC_DARK, MVC_HILIGHT, MVC_SHADOW));
}


VRAMHDL menuvram_create(int width, int height, UINT bpp) {

	VRAMHDL	ret;

	ret = vram_create(width, height, FALSE, bpp);
	if (ret == NULL) {
		goto mvcre_err;
	}
	menuvram_base(ret);

mvcre_err:
	return(ret);
}

void menuvram_caption(VRAMHDL vram, const RECT_T *rect,
										UINT16 icon, const OEMCHAR *caption) {

	POINT_T	pt;
	VRAMHDL	work;

	if ((vram == NULL) || (rect == NULL)) {
		goto mvpt_exit;
	}

#ifdef SUPPORT_16BPP
	if (vram->bpp == 16) {
		captionbar16(vram, rect, 0x000080, 0x000000);
	}
#endif
#ifdef SUPPORT_24BPP
	if (vram->bpp == 24) {
		captionbar24(vram, rect, 0x000080, 0x000000);
	}
#endif
	pt.x = rect->left + MENU_PXCAPTION;
	if (icon) {
		pt.y = rect->top + MENU_PYCAPTION;
		work = menuicon_lock(icon, MENUSYS_SZICON, MENUSYS_SZICON, vram->bpp);
		if (work) {
			if (work->alpha) {
				vramcpy_cpyex(vram, &pt, work, NULL);
			}
			else {
				vramcpy_cpy(vram, &pt, work, NULL);
			}
			menuicon_unlock(work);
		}
		pt.x += MENUSYS_SZICON + MENU_PXCAPTION;
	}
	pt.y = rect->top + (rect->bottom - rect->top - MENU_FONTSIZE) / 2;
	vrammix_text(vram, menubase.font, caption, 0xffffff, &pt, rect);

mvpt_exit:
	return;
}


static void putbtn(VRAMHDL vram, const RECT_T *rect,
										const MENURES2 *res, BOOL focus) {
	RECT_T	rct;
	POINT_T	pt;
	UINT	mvc4;

	if (rect) {
		rct = *rect;
	}
	else {
		vram_getrect(vram, &rct);
	}

	if (!focus) {
		mvc4 = MVC4(MVC_LIGHT, MVC_DARK, MVC_HILIGHT, MVC_SHADOW);
	}
	else {
		mvc4 = MVC4(MVC_DARK, MVC_LIGHT, MVC_SHADOW, MVC_HILIGHT);
	}
	menuvram_box2(vram, &rct, mvc4);
	rct.left += MENU_LINE * 2;
	rct.top += MENU_LINE * 2;
	rct.right -= MENU_LINE * 2;
	rct.bottom -= MENU_LINE * 2;
	vram_filldat(vram, &rct, menucolor[MVC_BTNFACE]);
	pt.x = rct.left;
	pt.y = rct.top;
	if (focus) {
		pt.x += MENU_LINE;
		pt.y += MENU_LINE;
	}
	menuvram_res3put(vram, res, &pt, MVC_TEXT);
}

void menuvram_minimizebtn(VRAMHDL vram, const RECT_T *rect, BOOL focus) {

	putbtn(vram, rect, &menures_minimize, focus);
}

void menuvram_closebtn(VRAMHDL vram, const RECT_T *rect, BOOL focus) {

	putbtn(vram, rect, &menures_close, focus);
}

