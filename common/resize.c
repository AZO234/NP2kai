#include	"compiler.h"
#include	"resize.h"


enum {
	RESIZE_HDIF		= 0x01,
	RESIZE_VDIF		= 0x02
};

enum {
	MXBITS		= 8,
	MYBITS		= 8,
	MULTIX		= (1 << MXBITS),
	MULTIY		= (1 << MYBITS),
	MXMASK		= (MULTIX - 1),
	MYMASK		= (MULTIY - 1)
};

enum {
	B16BIT		= 5,
	G16BIT		= 6,
	R16BIT		= 5,
	B16SFT		= 0,
	G16SFT		= 5,
	R16SFT		= 11
};

#define	GET16B(c)	(((c) >> B16SFT) & ((1 << B16BIT) - 1))
#define	GET16G(c)	(((c) >> G16SFT) & ((1 << G16BIT) - 1))
#define	GET16R(c)	(((c) >> R16SFT) & ((1 << R16BIT) - 1))

typedef struct {
	RSZFN	func;
	int		width;
	int		height;

	int		orgx;
	int		orgy;
	UINT	*h;
	UINT	*buf;
	UINT	bufsize;
} _RSZEX, *RSZEX;



// ---- convert sub

typedef void (*FNCNV)(RSZHDL hdl, UINT8 *dst, const UINT8 *src);

#if defined(RESIZE_FASTCOPY) || defined(RESIZE_BILINEAR)
static void cc16by24(RSZHDL hdl, UINT8 *dst, const UINT8 *src) {

	UINT	width;
	UINT	col;

	width = hdl->width;
	do {
		col = (src[0] >> (8 - B16BIT)) << B16SFT;
		col += (src[1] >> (8 - G16BIT)) << G16SFT;
		col += (src[2] >> (8 - R16BIT)) << R16SFT;
		*(UINT16 *)dst = (UINT16)col;
		src += 3;
		dst += 2;
	} while(--width);
}

static void cc24by16(RSZHDL hdl, UINT8 *dst, const UINT8 *src) {

	UINT	width;
	UINT	col;
	UINT	tmp;

	width = hdl->width;
	do {
		col = *(UINT16 *)src;
		tmp = (col >> B16SFT) & ((1 << B16BIT) - 1);
		dst[0] = (UINT8)((tmp << (8 - B16BIT)) + (tmp >> (B16BIT * 2 - 8)));
		tmp = (col >> G16SFT) & ((1 << G16BIT) - 1);
		dst[1] = (UINT8)((tmp << (8 - G16BIT)) + (tmp >> (G16BIT * 2 - 8)));
		tmp = (col >> R16SFT) & ((1 << R16BIT) - 1);
		dst[2] = (UINT8)((tmp << (8 - R16BIT)) + (tmp >> (R16BIT * 2 - 8)));
		src += 2;
		dst += 3;
	} while(--width);
}
#endif

#if defined(RESIZE_FASTCOPY)
static void cc8(RSZHDL hdl, UINT8 *dst, const UINT8 *src) {

	CopyMemory(dst, src, hdl->width);
}

static void cc16(RSZHDL hdl, UINT8 *dst, const UINT8 *src) {

	CopyMemory(dst, src, hdl->width * 2);
}

static void cc24(RSZHDL hdl, UINT8 *dst, const UINT8 *src) {

	CopyMemory(dst, src, hdl->width * 3);
}

static const FNCNV cnvcpy[RSZFNMAX] = {cc8, cc16, cc24, cc16by24, cc24by16};
#endif



// ----

#if defined(RESIZE_FASTCOPY)
static void fastcopyfunc(RSZHDL hdl, UINT type, UINT8 *dst, int dalign,
											const UINT8 *src, int salign) {

	UINT	height;
	FNCNV	cnv;

	if (type >= RSZFNMAX) {
		return;
	}
	height = hdl->height;
	cnv = cnvcpy[type];
	do {
		cnv(hdl, dst, src);
		src += salign;
		dst += dalign;
	} while(--height);
}

static RSZHDL fastcopy(int width, int height) {

	RSZHDL	ret;

	ret = (RSZHDL)_MALLOC(sizeof(_RSZHDL), "fastcopy");
	if (ret) {
		ret->func = fastcopyfunc;
		ret->width = width;
		ret->height = height;
	}
	return(ret);
}
#endif


// ---- area average

#if defined(RESIZE_AREAAVG)
static void aamix8(RSZEX hdl, const UINT8 *src, int volume) {

	UINT	*buf;
	UINT	posx;
	int		i;
	UINT	curx;
	int		vol;

	buf = hdl->buf;
	posx = 0;
	for (i=0; i<hdl->orgx; i++) {
		curx = hdl->h[i];
		while((curx ^ posx) >> MXBITS) {
			vol = (int)(MULTIX - (posx & MXMASK)) * volume;
			buf[0] += src[0] * vol;
			buf += 1;
			posx &= ~MXMASK;
			posx += MULTIX;
		}
		curx -= posx;
		if (curx) {
			posx += curx;
			vol = (int)curx * volume;
			buf[0] += src[0] * vol;
		}
		src++;
	}
}

static void aamix16(RSZEX hdl, const UINT8 *src, int volume) {

	UINT	*buf;
	UINT	posx;
	int		i;
	UINT	col;
	int		r, g, b;
	UINT	curx;
	int		vol;

	buf = hdl->buf;
	posx = 0;
	for (i=0; i<hdl->orgx; i++) {
		col = *(UINT16 *)src;
		src += 2;
		b = (col >> B16SFT) & ((1 << B16BIT) - 1);
		g = (col >> G16SFT) & ((1 << G16BIT) - 1);
		r = (col >> R16SFT) & ((1 << R16BIT) - 1);
		curx = hdl->h[i];
		while((curx ^ posx) >> MXBITS) {
			vol = (int)(MULTIX - (posx & MXMASK)) * volume;
			buf[0] += b * vol;
			buf[1] += g * vol;
			buf[2] += r * vol;
			buf += 3;
			posx &= ~MXMASK;
			posx += MULTIX;
		}
		curx -= posx;
		if (curx) {
			posx += curx;
			vol = (int)curx * volume;
			buf[0] += b * vol;
			buf[1] += g * vol;
			buf[2] += r * vol;
		}
	}
}

static void aamix24(RSZEX hdl, const UINT8 *src, int volume) {

	UINT	*buf;
	UINT	posx;
	int		i;
	UINT	curx;
	int		vol;

	buf = hdl->buf;
	posx = 0;
	for (i=0; i<hdl->orgx; i++) {
		curx = hdl->h[i];
		while((curx ^ posx) >> MXBITS) {
			vol = (int)(MULTIX - (posx & MXMASK)) * volume;
			buf[0] += (int)src[0] * vol;
			buf[1] += (int)src[1] * vol;
			buf[2] += (int)src[2] * vol;
			buf += 3;
			posx &= ~MXMASK;
			posx += MULTIX;
		}
		curx -= posx;
		if (curx) {
			posx += curx;
			vol = (int)curx * volume;
			buf[0] += (int)src[0] * vol;
			buf[1] += (int)src[1] * vol;
			buf[2] += (int)src[2] * vol;
		}
		src += 3;
	}
}

static void aaout8(RSZEX hdl, UINT8 *dst) {

const UINT	*buf;
	int		rem;

	buf = hdl->buf;
	rem = hdl->width;
	do {
		*dst++ = (UINT8)((*buf++) >> (MXBITS + MYBITS));
	} while(--rem);
}

static void aaout16(RSZEX hdl, UINT8 *dst) {

const UINT	*buf;
	int		rem;
	UINT	tmp;
	UINT	col;

	buf = hdl->buf;
	rem = hdl->width;
	do {
		tmp = buf[0] + (buf[0] >> (8 - B16BIT));
		col = (tmp >> (MXBITS + MYBITS - B16SFT)) &
											(((1 << B16BIT) - 1) << B16SFT);
		tmp = buf[1] + (buf[1] >> (8 - G16BIT));
		col += (tmp >> (MXBITS + MYBITS - G16SFT)) &
											(((1 << G16BIT) - 1) << G16SFT);
		tmp = buf[2] + (buf[2] >> (8 - R16BIT));
		col += (tmp >> (MXBITS + MYBITS - R16SFT)) &
											(((1 << R16BIT) - 1) << R16SFT);
		*(UINT16 *)dst = (UINT16)col;
		dst += 2;
		buf += 3;
	} while(--rem);
}

static void aaout24(RSZEX hdl, UINT8 *dst) {

const UINT	*buf;
	int		rem;

	buf = hdl->buf;
	rem = hdl->width * 3;
	do {
		*dst++ = (UINT8)((*buf++) >> (MXBITS + MYBITS));
	} while(--rem);
}

static void aaout16by24(RSZEX hdl, UINT8 *dst) {

const UINT	*buf;
	int		rem;
	UINT	col;

	buf = hdl->buf;
	rem = hdl->width;
	do {
		col = (buf[0] >> (MXBITS + MYBITS + 8 - B16BIT - B16SFT)) &
											(((1 << B16BIT) - 1) << B16SFT);
		col += (buf[1] >> (MXBITS + MYBITS + 8 - G16BIT - G16SFT)) &
											(((1 << G16BIT) - 1) << G16SFT);
		col += (buf[2] >> (MXBITS + MYBITS + 8 - R16BIT - R16SFT)) &
											(((1 << R16BIT) - 1) << R16SFT);
		*(UINT16 *)dst = (UINT16)col;
		dst += 2;
		buf += 3;
	} while(--rem);
}

static void aaout24by16(RSZEX hdl, UINT8 *dst) {

const UINT	*buf;
	int		rem;

	buf = hdl->buf;
	rem = hdl->width;
	do {
		dst[0] = (UINT8)(buf[0] >> (MXBITS + MYBITS - 8 + B16BIT));
		dst[1] = (UINT8)(buf[1] >> (MXBITS + MYBITS - 8 + G16BIT));
		dst[2] = (UINT8)(buf[2] >> (MXBITS + MYBITS - 8 + R16BIT));
		dst += 3;
		buf += 3;
	} while(--rem);
}

typedef void (*AAMIX)(RSZEX hdl, const UINT8 *src, int volume);
typedef void (*AAOUT)(RSZEX hdl, UINT8 *dst);

static const AAMIX fnaamix[RSZFNMAX] = {aamix8, aamix16, aamix24,
										aamix24, aamix16};
static const AAOUT fnaaout[RSZFNMAX] = {aaout8, aaout16, aaout24,
										aaout16by24, aaout24by16};

static void areaavefunc(RSZEX hdl, UINT type, UINT8 *dst, int dalign,
											const UINT8 *src, int salign) {

	AAMIX	aamix;
	AAOUT	aaout;
	UINT	posy;
	UINT	i;
	UINT	cury;

	if (type >= RSZFNMAX) {
		return;
	}
	aamix = fnaamix[type];
	aaout = fnaaout[type];
	ZeroMemory(hdl->buf, hdl->bufsize);
	posy = 0;
	for (i=0; i<(UINT)hdl->orgy; i++) {
		cury = (((i + 1) << MYBITS) * (UINT)hdl->height) / (UINT)hdl->orgy;
		while((cury ^ posy) >> MYBITS) {
			aamix(hdl, src, MULTIY - (posy & MYMASK));
			aaout(hdl, dst);
			dst += dalign;
			ZeroMemory(hdl->buf, hdl->bufsize);
			posy &= ~MYMASK;
			posy += MULTIY;
		}
		cury -= posy;
		if (cury) {
			posy += cury;
			aamix(hdl, src, cury);
		}
		src += salign;
	}
}

static RSZHDL areaave(int width, int height, int orgx, int orgy) {

	UINT	dstcnt;
	UINT	tbls;
	RSZEX	rszex;
	UINT	*ptr;
	UINT	i;

	dstcnt = width * 3;
	tbls = orgx + orgy + dstcnt;
	rszex = (RSZEX)_MALLOC(sizeof(_RSZEX) + (tbls * sizeof(UINT)), "RSZEX");
	if (rszex) {
		rszex->func = (RSZFN)areaavefunc;
		rszex->width = width;
		rszex->height = height;
		rszex->orgx = orgx;
		rszex->orgy = orgy;
		ptr = (UINT *)(rszex + 1);
		rszex->h = ptr;
		i = 0;
		do {
			i++;
			*ptr++ = ((i << MXBITS) * (UINT)width) / (UINT)orgx;
		} while(i < (UINT)orgx);
		rszex->buf = ptr;
		rszex->bufsize = dstcnt * sizeof(UINT);
	}
	return((RSZHDL)rszex);
}
#endif


// ----

UINT resize_gettype(int dbpp, int sbpp) {

	UINT	ret;

	if (dbpp == 8) {
		ret = RSZFN_8BPP;
	}
	else if (dbpp == 16) {
		ret = (sbpp == 24)?RSZFN_16BY24:RSZFN_16BPP;
	}
	else if (dbpp == 24) {
		ret = (sbpp == 16)?RSZFN_24BY16:RSZFN_24BPP;
	}
	else {
		ret = RSZFNMAX;
	}
	if ((dbpp != sbpp) && (ret < RSZFN_16BY24)) {
		ret = RSZFNMAX;
	}
	return(ret);
}

RSZHDL resize(int xdst, int ydst, int xsrc, int ysrc) {

	UINT	flag;

	flag = 0;
	if ((xsrc <= 0) || (xdst <= 0)) {
		goto rsz_err;
	}
	if (xsrc != xdst) {
		flag += RESIZE_HDIF;
	}
	if ((ysrc <= 0) || (ydst <= 0)) {
		goto rsz_err;
	}
	if (ysrc != ydst) {
		flag += RESIZE_VDIF;
	}
#if defined(RESIZE_FASTCOPY)
	if (flag == 0) {
		return(fastcopy(xdst, ydst));
	}
#endif
#if defined(RESIZE_AREAAVG)
	return(areaave(xdst, ydst, xsrc, ysrc));
#endif

rsz_err:
	return(NULL);
}

