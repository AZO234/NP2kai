#include	"compiler.h"
#include	"resize.h"
#include	"vramhdl.h"


VRAMHDL vram_create(int width, int height, BOOL alpha, int bpp) {

	int		size;
	int		allocsize;
	int		xalign;
	int		alphasize;
	VRAMHDL	ret;

#if defined(SCREEN_BPP)
	if (bpp == DEFAULT_BPP) {
		bpp = SCREEN_BPP;
	}
#endif
	size = width * height;
	xalign = (bpp + 7) >> 3;
	if ((width <= 0) || (size <= 0) || (size > 0x1000000) ||
		(xalign <= 0) || (xalign > 4)) {
		return(NULL);
	}
	allocsize = sizeof(_VRAMHDL);
	allocsize += size * xalign;
	alphasize = 0;
	if (alpha) {
		alphasize = (size + 7) & (~7);				// boundary!!
		allocsize += alphasize;
	}
#ifdef MEMTRACE
	{
		char buf[128];
		sprintf(buf, "VRAM %dx%d (%d)", width, height, bpp);
		ret = (VRAMHDL)_MALLOC(allocsize, buf);
	}
#else
	ret = (VRAMHDL)_MALLOC(allocsize, "VRAM");
#endif
	if (ret) {
		ZeroMemory(ret, allocsize);
		ret->width = width;
		ret->height = height;
		ret->xalign = xalign;
		ret->yalign = xalign * width;
		ret->bpp = bpp;
		ret->scrnsize = size;
		if (alpha) {
			ret->alpha = (UINT8 *)(ret + 1);
			ret->ptr = ret->alpha + alphasize;
		}
		else {
			ret->ptr = (UINT8 *)(ret + 1);
		}
	}
	return(ret);
}

void vram_destroy(VRAMHDL hdl) {

	if (hdl) {
		if ((hdl->alpha) && (hdl->alpha != (UINT8 *)(hdl + 1))) {
			_MFREE(hdl->alpha);
		}
		_MFREE(hdl);
	}
}

BRESULT vram_allocalpha(VRAMHDL hdl) {

	if (hdl == NULL) {
		return(FAILURE);
	}
	if (hdl->alpha == NULL) {
		hdl->alpha = (UINT8 *)_MALLOC(hdl->scrnsize, "alpha plane");
		if (hdl->alpha == NULL) {
			return(FAILURE);
		}
		ZeroMemory(hdl->alpha, hdl->scrnsize);
	}
	return(SUCCESS);
}

void vram_zerofill(VRAMHDL hdl, const RECT_T *rect) {

	int		ptr;
	int		width;
	int		height;
	int		pos;
	int		remain;
	UINT8	*p;

	if (hdl) {
		if (rect == NULL) {
			ZeroMemory(hdl->ptr, hdl->scrnsize * hdl->xalign);
			if (hdl->alpha) {
				ZeroMemory(hdl->alpha, hdl->scrnsize);
			}
		}
		else {
			pos = max(rect->left, 0);
			ptr = pos;
			width = min(rect->right, hdl->width) - pos;
			pos = max(rect->top, 0);
			ptr += pos * hdl->width;
			height = min(rect->bottom, hdl->height) - pos;
			if ((width > 0) && (height > 0)) {
				p = hdl->ptr;
				p += ptr * hdl->xalign;
				remain = height;
				do {
					ZeroMemory(p, width * hdl->xalign);
					p += hdl->yalign;
				} while(--remain);

				if (hdl->alpha) {
					p = hdl->alpha + ptr;
					remain = height;
					do {
						ZeroMemory(p, width);
						p += hdl->width;
					} while(--remain);
				}
			}
		}
	}
}

void vram_fill(VRAMHDL hdl, const RECT_T *rect, UINT32 color, UINT8 alpha) {

	int		ptr;
	int		width;
	int		height;
	int		pos;
	int		remain;
	UINT8	*p;
#ifdef SUPPORT_16BPP
	UINT	c16;
#endif
#ifdef SUPPORT_24BPP
	UINT8	c24[3];
#endif

	if (hdl == NULL) {
		return;
	}
	if (rect == NULL) {
		p = hdl->ptr;
		remain = hdl->scrnsize;
		switch(hdl->bpp) {
			case 8:
				do {
					*p++ = (UINT8)color;
				} while(--remain);
				break;
#ifdef SUPPORT_16BPP
			case 16:
				c16 = MAKE16PAL(color);
				do {
					*(UINT16 *)p = (UINT16)c16;
					p += 2;
				} while(--remain);
				break;
#endif
#ifdef SUPPORT_24BPP
			case 24:
				c24[0] = (UINT8)color;
				c24[1] = (UINT8)(color >> 8);
				c24[2] = (UINT8)(color >> 16);
				do {
					p[0] = c24[0];
					p[1] = c24[1];
					p[2] = c24[2];
					p += 3;
				} while(--remain);
				break;
#endif
			default:
				TRACEOUT(("vram_fill: unsupport %dbpp", hdl->bpp));
				break;
		}
		if (hdl->alpha) {
			FillMemory(hdl->alpha, hdl->scrnsize, alpha);
		}
	}
	else {
		pos = max(rect->left, 0);
		ptr = pos;
		width = min(rect->right, hdl->width) - pos;
		pos = max(rect->top, 0);
		ptr += pos * hdl->width;
		height = min(rect->bottom, hdl->height) - pos;
		if ((width > 0) && (height > 0)) {
			p = hdl->ptr;
			p += ptr * hdl->xalign;
			switch(hdl->bpp) {
				case 8:
					remain = height;
					do {
						int r = width;
						do {
							*p++ = (UINT8)color;
						} while(--r);
						p += hdl->yalign - width;
					} while(--remain);
					break;
#ifdef SUPPORT_16BPP
				case 16:
					c16 = MAKE16PAL(color);
					remain = height;
					do {
						int r = width;
						do {
							*(UINT16 *)p = (UINT16)c16;
							p += 2;
						} while(--r);
						p += hdl->yalign - (width * 2);
					} while(--remain);
					break;
#endif
#ifdef SUPPORT_24BPP
				case 24:
					c24[0] = (UINT8)color;
					c24[1] = (UINT8)(color >> 8);
					c24[2] = (UINT8)(color >> 16);
					remain = height;
					do {
						int r = width;
						do {
							p[0] = c24[0];
							p[1] = c24[1];
							p[2] = c24[2];
							p += 3;
						} while(--r);
						p += hdl->yalign - (width * 3);
					} while(--remain);
					break;
#endif
				default:
					TRACEOUT(("vram_fill: unsupport %dbpp", hdl->bpp));
					break;
			}
			if (hdl->alpha) {
				p = hdl->alpha + ptr;
				remain = height;
				do {
					FillMemory(p, width, alpha);
					p += hdl->width;
				} while(--remain);
			}
		}
	}
}

void vram_filldat(VRAMHDL hdl, const RECT_T *rect, UINT32 color) {

	int		ptr;
	int		width;
	int		height;
	int		pos;
	int		remain;
	UINT8	*p;
#ifdef SUPPORT_16BPP
	UINT	c16;
#endif
#ifdef SUPPORT_24BPP
	UINT8	c24[3];
#endif

	if (hdl == NULL) {
		return;
	}
	if (rect == NULL) {
		p = hdl->ptr;
		remain = hdl->scrnsize;
		switch(hdl->bpp) {
			case 8:
				do {
					*p++ = (UINT8)color;
				} while(--remain);
				break;
#ifdef SUPPORT_16BPP
			case 16:
				c16 = MAKE16PAL(color);
				do {
					*(UINT16 *)p = (UINT16)c16;
					p += 2;
				} while(--remain);
				break;
#endif
#ifdef SUPPORT_24BPP
			case 24:
				c24[0] = (UINT8)color;
				c24[1] = (UINT8)(color >> 8);
				c24[2] = (UINT8)(color >> 16);
				do {
					p[0] = c24[0];
					p[1] = c24[1];
					p[2] = c24[2];
					p += 3;
				} while(--remain);
				break;
#endif
			default:
				TRACEOUT(("vram_filldat: unsupport %dbpp", hdl->bpp));
				break;
		}
	}
	else {
		pos = max(rect->left, 0);
		ptr = pos;
		width = min(rect->right, hdl->width) - pos;
		pos = max(rect->top, 0);
		ptr += pos * hdl->width;
		height = min(rect->bottom, hdl->height) - pos;
		if ((width > 0) && (height > 0)) {
			p = hdl->ptr;
			p += ptr * hdl->xalign;
			switch(hdl->bpp) {
				case 8:
					remain = height;
					do {
						int r = width;
						do {
							*p++ = (UINT8)color;
						} while(--r);
						p += hdl->yalign - width;
					} while(--remain);
					break;
#ifdef SUPPORT_16BPP
				case 16:
					c16 = MAKE16PAL(color);
					remain = height;
					do {
						int r = width;
						do {
							*(UINT16 *)p = (UINT16)c16;
							p += 2;
						} while(--r);
						p += hdl->yalign - (width * 2);
					} while(--remain);
					break;
#endif
#ifdef SUPPORT_24BPP
				case 24:
					c24[0] = (UINT8)color;
					c24[1] = (UINT8)(color >> 8);
					c24[2] = (UINT8)(color >> 16);
					remain = height;
					do {
						int r = width;
						do {
							p[0] = c24[0];
							p[1] = c24[1];
							p[2] = c24[2];
							p += 3;
						} while(--r);
						p += hdl->yalign - (width * 3);
					} while(--remain);
					break;
#endif
				default:
					TRACEOUT(("vram_filldat: unsupport %dbpp", hdl->bpp));
					break;
			}
		}
	}
}

void vram_fillalpha(VRAMHDL hdl, const RECT_T *rect, UINT8 alpha) {

	int		ptr;
	int		width;
	int		height;
	int		pos;
	int		remain;
	UINT8	*p;

	if ((hdl == NULL) || (hdl->alpha == NULL)) {
		return;
	}
	if (rect == NULL) {
		p = hdl->ptr;
		remain = hdl->scrnsize;
		FillMemory(hdl->alpha, hdl->scrnsize, alpha);
	}
	else {
		pos = max(rect->left, 0);
		ptr = pos;
		width = min(rect->right, hdl->width) - pos;
		pos = max(rect->top, 0);
		ptr += pos * hdl->width;
		height = min(rect->bottom, hdl->height) - pos;
		if ((width > 0) && (height > 0)) {
			p = hdl->alpha + ptr;
			remain = height;
			do {
				FillMemory(p, width, alpha);
				p += hdl->width;
			} while(--remain);
		}
	}
}

void vram_fillex(VRAMHDL hdl, const RECT_T *rect, UINT32 color, UINT8 alpha) {

	int		ptr;
	int		width;
	int		height;
	int		pos;
	int		remain;
	UINT8	*p;
#ifdef SUPPORT_16BPP
	int		tmp;
	int		c16[3];
#endif
#ifdef SUPPORT_24BPP
	int		c24[3];
#endif

	if (hdl == NULL) {
		return;
	}
	if (rect == NULL) {
		p = hdl->ptr;
		remain = hdl->scrnsize;
		switch(hdl->bpp) {
#ifdef SUPPORT_16BPP
			case 16:
				tmp = MAKE16PAL(color);
				c16[0] = tmp & B16MASK;
				c16[1] = tmp & G16MASK;
				c16[2] = tmp & R16MASK;
				tmp = 64 - alpha;
				do {
					UINT s, d;
					s = *(UINT16 *)p;
					d = MAKEALPHA16s(c16[0], s, B16MASK, tmp, 6);
					d |= MAKEALPHA16s(c16[1], s, G16MASK, tmp, 6);
					d |= MAKEALPHA16s(c16[2], s, R16MASK, tmp, 6);
					*(UINT16 *)p = (UINT16)d;
					p += 2;
				} while(--remain);
				break;
#endif
#ifdef SUPPORT_24BPP
			case 24:
				c24[0] = color & 0xff;
				c24[1] = (color >> 8) & 0xff;
				c24[2] = (color >> 16) & 0xff;
				do {
					p[0] = (UINT8)MAKEALPHA24(p[0], c24[0], alpha, 6);
					p[1] = (UINT8)MAKEALPHA24(p[1], c24[1], alpha, 6);
					p[2] = (UINT8)MAKEALPHA24(p[2], c24[2], alpha, 6);
					p += 3;
				} while(--remain);
				break;
#endif
			default:
				TRACEOUT(("vram_fillex: unsupport %dbpp", hdl->bpp));
				break;
		}
	}
	else {
		pos = max(rect->left, 0);
		ptr = pos;
		width = min(rect->right, hdl->width) - pos;
		pos = max(rect->top, 0);
		ptr += pos * hdl->width;
		height = min(rect->bottom, hdl->height) - pos;
		if ((width > 0) && (height > 0)) {
			p = hdl->ptr;
			p += ptr * hdl->xalign;
			switch(hdl->bpp) {
#ifdef SUPPORT_16BPP
				case 16:
					tmp = MAKE16PAL(color);
					c16[0] = tmp & B16MASK;
					c16[1] = tmp & G16MASK;
					c16[2] = tmp & R16MASK;
					tmp = 64 - alpha;
					remain = height;
					do {
						int r = width;
						do {
							UINT s, d;
							s = *(UINT16 *)p;
							d = MAKEALPHA16s(c16[0], s, B16MASK, tmp, 6);
							d |= MAKEALPHA16s(c16[1], s, G16MASK, tmp, 6);
							d |= MAKEALPHA16s(c16[2], s, R16MASK, tmp, 6);
							*(UINT16 *)p = (UINT16)d;
							p += 2;
						} while(--r);
						p += hdl->yalign - (width * 2);
					} while(--remain);
					break;
#endif
#ifdef SUPPORT_24BPP
				case 24:
					remain = height;
					c24[0] = color & 0xff;
					c24[1] = (color >> 8) & 0xff;
					c24[2] = (color >> 16) & 0xff;
					do {
						int r = width;
						do {
							p[0] = (UINT8)MAKEALPHA24(p[0], c24[0], alpha, 6);
							p[1] = (UINT8)MAKEALPHA24(p[1], c24[1], alpha, 6);
							p[2] = (UINT8)MAKEALPHA24(p[2], c24[2], alpha, 6);
							p += 3;
						} while(--r);
						p += hdl->yalign - (width * 3);
					} while(--remain);
					break;
#endif
				default:
					TRACEOUT(("vram_fillex: unsupport %dbpp", hdl->bpp));
					break;
			}
		}
	}
}

VRAMHDL vram_resize(VRAMHDL base, int width, int height, int bpp) {

	VRAMHDL	ret;
	RSZHDL	rsz;

	if (base == NULL) {
		goto vrs_err1;
	}
	ret = vram_create(width, height, (base->alpha != NULL), bpp);
	if (ret == NULL) {
		goto vrs_err1;
	}
	rsz = resize(width, height, base->width, base->height);
	if (rsz == NULL) {
		goto vrs_err2;
	}
	(*rsz->func)(rsz, resize_gettype(bpp, base->bpp),
							ret->ptr, ret->yalign, base->ptr, base->yalign);
	if (base->alpha) {
		(*rsz->func)(rsz, RSZFN_8BPP,
							ret->alpha, ret->width, base->alpha, base->width);
	}
	_MFREE(rsz);
	return(ret);

vrs_err2:
	vram_destroy(ret);

vrs_err1:
	return(NULL);
}

void vram_getrect(const VRAMHDL hdl, RECT_T *rct) {

	int		x, y;

	if ((hdl) && (rct)) {
		x = hdl->posx;
		y = hdl->posy;
		rct->left = x;
		rct->top = y;
		rct->right = x + hdl->width;
		rct->bottom = y + hdl->height;
	}
}

VRAMHDL vram_dupe(const VRAMHDL hdl) {

	VRAMHDL		ret = NULL;
	int			size;
	int			datsize;

	if (hdl == NULL) {
		goto vd_exit;
	}
	datsize = hdl->scrnsize * hdl->xalign;
	size = sizeof(_VRAMHDL);
	size += datsize;
	if (hdl->alpha) {
		size += hdl->scrnsize;
	}
	ret = (VRAMHDL)_MALLOC(size, "VRAM copy");
	if (ret == NULL) {
		goto vd_exit;
	}
	*ret = *hdl;
	if (hdl->alpha) {
		ret->alpha = (UINT8 *)(ret + 1);
		CopyMemory(ret->alpha, hdl->alpha, hdl->scrnsize);
		ret->ptr = ret->alpha + hdl->scrnsize;
	}
	else {
		ret->ptr = (UINT8 *)(ret + 1);
	}
	CopyMemory(ret->ptr, hdl->ptr, datsize);

vd_exit:
	return(ret);
}

BRESULT vram_cliprect(RECT_T *clip, const VRAMHDL vram, const RECT_T *rct) {

	if (vram == NULL) {
		return(FAILURE);
	}
	if (rct == NULL) {
		clip->left = 0;
		clip->top = 0;
		clip->right = vram->width;
		clip->bottom = vram->height;
		return(SUCCESS);
	}
	if ((rct->bottom <= 0) || (rct->right <= 0) ||
		(rct->left >= vram->width) || (rct->top >= vram->height)) {
		return(FAILURE);
	}
	clip->left = max(rct->left, 0);
	clip->top = max(rct->top, 0);
	clip->right = min(rct->right, vram->width);
	clip->bottom = min(rct->bottom, vram->height);
	if ((clip->top >= clip->bottom) || (clip->left >= clip->right)) {
		return(FAILURE);
	}
	return(SUCCESS);
}

BRESULT vram_cliprectex(RECT_T *clip, const VRAMHDL vram, const RECT_T *rct) {

	if ((vram == NULL) || (clip == NULL)) {
		return(FAILURE);
	}
	vram_getrect(vram, clip);
	if (rct == NULL) {
		return(SUCCESS);
	}
	clip->left = max(clip->left, rct->left);
	clip->top = max(clip->top, rct->top);
	clip->right = min(clip->right, rct->right);
	clip->bottom = min(clip->bottom, rct->bottom);
	if ((clip->left >= clip->right) || (clip->top >= clip->bottom)) {
		return(FAILURE);
	}
	return(SUCCESS);
}

