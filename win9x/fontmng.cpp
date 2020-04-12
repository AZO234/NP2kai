#include	"compiler.h"
#include	"fontmng.h"
#include	"codecnv/codecnv.h"

typedef struct {
	int			fontsize;
	UINT		fonttype;
	int			fontwidth;
	int			fontheight;

// あとは拡張〜
	HDC			hdcimage;
	HBITMAP		hBitmap;
	UINT8		*image;
	HFONT		hfont;
	RECT		rect;
	int			bmpwidth;
	int			bmpheight;
	int			bmpalign;
} _FNTMNG, *FNTMNG;


static const OEMCHAR deffontface[] = OEMTEXT("ＭＳ ゴシック");
static const OEMCHAR deffontface2[] = OEMTEXT("ＭＳ Ｐゴシック");
static const OEMCHAR edeffontface[] = OEMTEXT("MS Gothic");
static const OEMCHAR edeffontface2[] = OEMTEXT("MS PGothic");

static const OEMCHAR *deffont[4] = {
	deffontface,	deffontface2,
	edeffontface,	edeffontface2};

void *fontmng_create(int size, UINT type, const OEMCHAR *fontface) {

	int			i;
	int			fontalign;
	int			allocsize;
	FNTMNG		ret;
	BITMAPINFO	*bi;
	HDC			hdc;
	int			fontwidth;
	int			fontheight;
	int			weight;
	int			deffontnum;
	DWORD		pitch;
	DWORD		charset;

	if (size < 0) {
		size *= -1;
	}
	if (size < 6) {
		size = 6;
	}
	else if (size > 128) {
		size = 128;
	}
	fontwidth = size;
	fontheight = size;
	if (type & FDAT_BOLD) {
		fontwidth++;
	}

	fontalign = sizeof(_FNTDAT) + (fontwidth * fontheight);
	fontalign = (fontalign + 3) & (~3);

	allocsize = sizeof(_FNTMNG);
	allocsize += fontalign;
	allocsize += sizeof(BITMAPINFOHEADER) + (4 * 2);

	ret = (FNTMNG)_MALLOC(allocsize, "font mng");
	if (ret == NULL) {
		return(NULL);
	}
	ZeroMemory(ret, allocsize);
	ret->fontsize = size;
	ret->fonttype = type;
	ret->fontwidth = fontwidth;
	ret->fontheight = fontheight;
	ret->bmpwidth = fontwidth;
	ret->bmpheight = fontheight;
	ret->bmpalign = (((ret->bmpwidth + 31) / 8) & ~3);


	bi = (BITMAPINFO *)(((UINT8 *)(ret + 1)) + fontalign);
	bi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bi->bmiHeader.biWidth = ret->bmpwidth;
	bi->bmiHeader.biHeight = ret->bmpheight;
	bi->bmiHeader.biPlanes = 1;
	bi->bmiHeader.biBitCount = 1;
	bi->bmiHeader.biCompression = BI_RGB;
	bi->bmiHeader.biSizeImage = ret->bmpalign * ret->bmpheight;
	bi->bmiHeader.biXPelsPerMeter = 0;
	bi->bmiHeader.biYPelsPerMeter = 0;
	bi->bmiHeader.biClrUsed = 2;
	bi->bmiHeader.biClrImportant = 2;
	for (i=0; i<2; i++) {
		bi->bmiColors[i].rgbRed = (i ^ 1) - 1;
		bi->bmiColors[i].rgbGreen = (i ^ 1) - 1;
		bi->bmiColors[i].rgbBlue = (i ^ 1) - 1;
		bi->bmiColors[i].rgbReserved = PC_RESERVED;
	}

	hdc = GetDC(NULL);
	ret->hBitmap = CreateDIBSection(hdc, bi, DIB_RGB_COLORS,
											(void **)&ret->image, NULL, 0);
	ret->hdcimage = CreateCompatibleDC(hdc);
	ReleaseDC(NULL, hdc);
	ret->hBitmap = (HBITMAP)SelectObject(ret->hdcimage, ret->hBitmap);
	SetDIBColorTable(ret->hdcimage, 0, 2, bi->bmiColors);

	weight = (type & FDAT_BOLD)?FW_BOLD:FW_REGULAR;
	pitch = (type & FDAT_PROPORTIONAL)?VARIABLE_PITCH:FIXED_PITCH;
	if (fontface == NULL) {
		deffontnum = (type & FDAT_PROPORTIONAL)?1:0;
		if (GetOEMCP() != 932) {			// !Japanese
			deffontnum += 2;
		}
		fontface = deffont[deffontnum];
	}
	charset = (type & FDAT_SHIFTJIS)?SHIFTJIS_CHARSET:DEFAULT_CHARSET;
	ret->hfont = CreateFont(size, 0,
						FW_DONTCARE, FW_DONTCARE, weight,
						FALSE, FALSE, FALSE, charset,
						OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
						NONANTIALIASED_QUALITY, pitch, fontface);
	ret->hfont = (HFONT)SelectObject(ret->hdcimage, ret->hfont);
	SetTextColor(ret->hdcimage, RGB(255, 255, 255));
	SetBkColor(ret->hdcimage, RGB(0, 0, 0));
	SetRect(&ret->rect, 0, 0, ret->bmpwidth, ret->bmpheight);
	return(ret);
}

void fontmng_destroy(void *hdl) {

	FNTMNG	fhdl;

	fhdl = (FNTMNG)hdl;
	if (fhdl) {
		DeleteObject(SelectObject(fhdl->hdcimage, fhdl->hBitmap));
		DeleteObject(SelectObject(fhdl->hdcimage, fhdl->hfont));
		DeleteDC(fhdl->hdcimage);
		_MFREE(hdl);
	}
}


// ----

static void getlength1(FNTMNG fhdl, FNTDAT fdat,
										const wchar_t *string, int length) {

	SIZE	fntsize;

	if (GetTextExtentPoint32W(fhdl->hdcimage, string, length, &fntsize)) {
		fntsize.cx = min(fntsize.cx, fhdl->bmpwidth);
		fdat->width = fntsize.cx;
		fdat->pitch = fntsize.cx;
	}
	else {
		fdat->width = fhdl->fontwidth;
		fdat->pitch = (fhdl->fontsize + 1) >> 1;
	}
	fdat->height = fhdl->fontheight;
}

static void fontmng_getchar(FNTMNG fhdl, FNTDAT fdat, const OEMCHAR *string) {

	int		leng;
	UINT16	c[8];
	OEMCHAR* startstr = (OEMCHAR*)string;

	FillRect(fhdl->hdcimage, &fhdl->rect,
										(HBRUSH)GetStockObject(BLACK_BRUSH));
	codecnv_utf8toucs2(c, 8, string, -1);
	leng = codecnv_ucs2len(c);
	TextOutW(fhdl->hdcimage, 0, 0, (wchar_t*)c, leng);
	getlength1(fhdl, fdat, (wchar_t*)c, leng);
}

BRESULT fontmng_getsize(void *hdl, const OEMCHAR *string, POINT_T *pt) {

	int		width;
	OEMCHAR	buf[4];
	_FNTDAT	fdat;
	int		leng;
	UINT16	c[8];

	if ((hdl == NULL) || (string == NULL)) {
		goto fmgs_exit;
	}

	width = 0;
	while(1) {
		leng = milstr_charsize(string);
		if (!leng) {
			break;
		}
		CopyMemory(buf, string, leng * sizeof(OEMCHAR));
		buf[leng] = '\0';
		string += leng;
		codecnv_utf8toucs2(c, 8, buf, -1);
		leng = codecnv_ucs2len(c);
		getlength1((FNTMNG)hdl, &fdat, (wchar_t*)c, leng);
		width += fdat.pitch;
	}

	if (pt) {
		pt->x = width;
		pt->y = ((FNTMNG)hdl)->fontsize;
	}
	return(SUCCESS);

fmgs_exit:
	return(FAILURE);
}

BRESULT fontmng_getdrawsize(void *hdl, const OEMCHAR *string, POINT_T *pt) {

	OEMCHAR	buf[4];
	_FNTDAT	fdat;
	int		width;
	int		posx;
	int		leng;
	UINT16	c[8];

	if ((hdl == NULL) || (string == NULL)) {
		goto fmgds_exit;
	}

	width = 0;
	posx = 0;
	while(1) {
		leng = milstr_charsize(string);
		if (!leng) {
			break;
		}
		CopyMemory(buf, string, leng * sizeof(OEMCHAR));
		buf[leng] = '\0';
		string += leng;
		codecnv_utf8toucs2(c, 8, buf, -1);
		getlength1((FNTMNG)hdl, &fdat, (wchar_t*)c, leng);
		width = posx + max(fdat.width, fdat.pitch);
		posx += fdat.pitch;
	}

	if (pt) {
		pt->x = width;
		pt->y = ((FNTMNG)hdl)->fontsize;
	}
	return(SUCCESS);

fmgds_exit:
	return(FAILURE);
}

static void fontmng_setpat(FNTMNG fhdl, FNTDAT fdat) {

	UINT	remx;
	UINT	remy;
	UINT8	*src;
	UINT8	*dst;
	UINT8	*s;
	UINT8	bit;
	UINT8	b1 = 0;				// for cygwin
	int		align;

	align = fhdl->bmpalign;
	src = fhdl->image + (fhdl->rect.bottom * align);
	if (fdat->width <= 0) {
		goto fmsp_end;
	}

	dst = (UINT8 *)(fdat + 1);
	align *= -1;

	remy = fdat->height;
	do {
		src += align;
		s = src;
		remx = fdat->width;
		bit = 0;
		do {
			if (bit == 0) {
				bit = 0x80;
				b1 = *s++;
			}
			*dst++ = (b1 & bit)?FDAT_DEPTH:0x00;
			bit >>= 1;
		} while(--remx);
	} while(--remy);

fmsp_end:
	return;
}


// ----

FNTDAT fontmng_get(void *hdl, const OEMCHAR *string) {

	FNTMNG	fhdl;
	FNTDAT	fdat;
	if ((hdl == NULL) || (string == NULL)) {
		goto ftmggt_err;
	}
	fhdl = (FNTMNG)hdl;
	fdat = (FNTDAT)(fhdl + 1);
	fontmng_getchar(fhdl, fdat, string);
	fontmng_setpat(fhdl, fdat);
	return(fdat);

ftmggt_err:
	return(NULL);
}

