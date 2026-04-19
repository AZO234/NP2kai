#include	"compiler.h"
#include	"fontmng.h"


typedef struct {
	int			fontsize;
	UINT		fonttype;
	int			fontwidth;
	int			fontheight;

// あとは拡張～
	HDC			hdcimage;
	HBITMAP		hBitmap;
	UINT8		*image;
	HFONT		hfont;
	RECT		rect;
	int			bmpwidth;
	int			bmpheight;
	int			bmpalign;
	int			tmAscent;
} _FNTMNG, *FNTMNG;


static const OEMCHAR deffontface[] = OEMTEXT("ＭＳ ゴシック");
static const OEMCHAR deffontface2[] = OEMTEXT("ＭＳ Ｐゴシック");
static const OEMCHAR edeffontface[] = OEMTEXT("MS Gothic");
static const OEMCHAR edeffontface2[] = OEMTEXT("MS PGothic");

static const OEMCHAR *deffont[4] = {
	deffontface,	deffontface2,
	edeffontface,	edeffontface2};

typedef struct {
	UINT32 indexSubtableListOffset;
	UINT32 indexSubtableListSize;
	UINT32 numberOfIndexSubtables;
	UINT32 colorRef;
	UINT8 hori[12];
	UINT8 vert[12];
	UINT16 startGlyphIndex;
	UINT16 endGlyphIndex;
	UINT8 ppemX;
	UINT8 ppemY;
	UINT8 bitDepth;
	UINT8 flags;
} EBLC_BITMAPSIZETABLE;

static bool hasBitmapFont(HDC hdc, int checksize) {
	DWORD size;
	BYTE* buf;
	UINT32 numSizes;
	EBLC_BITMAPSIZETABLE* tbl;
	int i;

	// EBLCテーブルサイズを問い合わせ
	size = GetFontData(hdc, 'CLBE', 0, NULL, 0);
	if (size == GDI_ERROR) {
		return false;
	}

	// EBLCテーブル取得
	buf = (BYTE*)malloc(size);
	if (!buf) return false;
	if (GetFontData(hdc, 'CLBE', 0, buf, size) == GDI_ERROR) {
		free(buf);
		return false;
	}

	// ビットマップがあるか？
	numSizes = ((UINT32)buf[4] << 24) | ((UINT32)buf[5] << 16) | ((UINT32)buf[6] << 8) | (UINT32)buf[7];
	if (numSizes == 0) {
		free(buf);
		return false;
	}

	// 指定したサイズのビットマップフォントがあるか確認
	tbl = (EBLC_BITMAPSIZETABLE*)(buf + 8);
	for (i = 0; i < numSizes; i++) {
		if (tbl[i].ppemY == checksize) {
			free(buf);
			return true;
		}
	}
	free(buf);
	return false;
}

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
	TEXTMETRIC	metric;
	GLYPHMETRICS gm;
	MAT2 mat = { {0,1},{0,0},{0,0},{0,1} };

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
	if (fontface == NULL || fontface[0] == '\0') {
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
						OUT_RASTER_PRECIS, CLIP_DEFAULT_PRECIS,
						NONANTIALIASED_QUALITY, pitch, fontface);
	ret->hfont = (HFONT)SelectObject(ret->hdcimage, ret->hfont);
	if (hasBitmapFont(ret->hdcimage, size)) {
		// 指定サイズのビットマップフォントがありそうな場合、それを選択
		ret->hfont = (HFONT)SelectObject(ret->hdcimage, ret->hfont);
		DeleteObject(ret->hfont);
		ret->hfont = CreateFont(-size, 0,
			FW_DONTCARE, FW_DONTCARE, weight,
			FALSE, FALSE, FALSE, charset,
			OUT_RASTER_PRECIS, CLIP_DEFAULT_PRECIS,
			NONANTIALIASED_QUALITY, pitch, fontface);
		ret->hfont = (HFONT)SelectObject(ret->hdcimage, ret->hfont);
	}
	GetTextMetrics(ret->hdcimage, &metric);
	ret->tmAscent = metric.tmAscent;
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
										const OEMCHAR *string, int length) {

	SIZE	fntsize;

	if (GetTextExtentPoint32(fhdl->hdcimage, string, length, &fntsize)) {
#ifdef UNICODE
		// Unicode文字なら文字コードで判定
		if ((0x00 <= string[0] && string[0] <= 0x7F) || (0xFF61 <= string[0] && string[0] <= 0xFF9F)) {
			// 半角なら8
			fdat->width = 8;
			fdat->pitch = 8;
		}
		else {
			// 全角なら16
			fdat->width = 16;
			fdat->pitch = 16;
		}
#else
		// マルチバイト文字の場合
		if (leng == 1) {
			// 半角なら8
			fdat->width = 8;
			fdat->pitch = 8;
		}
		else {
			// 全角なら16
			fdat->width = 16;
			fdat->pitch = 16;
		}
#endif
	}
	else {
		fdat->width = fhdl->fontwidth;
		fdat->pitch = (fhdl->fontsize + 1) >> 1;
	}
	fdat->height = fhdl->fontheight;
}

static void fontmng_getchar(FNTMNG fhdl, FNTDAT fdat, const OEMCHAR *string) {

	int		leng;
	GLYPHMETRICS gm;
	MAT2 mat = { {0,1},{0,0},{0,0},{0,1} };
	DWORD bufSize;

	FillRect(fhdl->hdcimage, &fhdl->rect, (HBRUSH)GetStockObject(BLACK_BRUSH));
	leng = milstr_charsize(string);

	getlength1(fhdl, fdat, string, leng);

#ifdef UNICODE
	// Unicode文字なら最初の値をとるだけでOK
	bufSize = GetGlyphOutline(fhdl->hdcimage, string[0], GGO_BITMAP, &gm, 0, NULL, &mat);
#else
	// マルチバイト文字の場合
	if (leng == 1) {
		// 半角なら最初の値をとるだけでOK
		bufSize = GetGlyphOutline(fhdl->hdcimage, string[0], GGO_BITMAP, &gm, 0, NULL, &mat);
	}
	else {
		// 全角なら2文字をとる
		bufSize = GetGlyphOutline(fhdl->hdcimage, (UINT)(string[0] << 8) | string[1], GGO_BITMAP, &gm, 0, NULL, &mat);
	}
#endif
	if (bufSize != GDI_ERROR) {
		// 右をはみ出す場合、出来るだけ収まるように調整
		int ofsx = 0;
		int ofsy = 0;
		int realY = 0;
		if (gm.gmptGlyphOrigin.x + gm.gmBlackBoxX > fdat->width) {
			ofsx = fdat->width - 1 - (gm.gmptGlyphOrigin.x + gm.gmBlackBoxX);
			if (ofsx < -gm.gmptGlyphOrigin.x) ofsx = -gm.gmptGlyphOrigin.x;
		}
		realY = (fhdl->tmAscent - gm.gmptGlyphOrigin.y);
		if (realY + gm.gmBlackBoxY > fdat->height - 1) {
			ofsy = fdat->height - 1 - (realY + gm.gmBlackBoxY);
			if (ofsy < -realY) ofsy = -realY;
		}
		if (gm.gmBlackBoxX > fdat->width) {
			// 幅が大きいなら強制スケール
			XFORM xForm;
			xForm.eM11 = 1.0f;
			xForm.eM12 = 0.0f;
			xForm.eM21 = 0.0f;
			xForm.eM22 = 1.0f;
			xForm.eDx = 0.0f;
			xForm.eDy = 0.0f;

			xForm.eM11 = (float)(fdat->width - 1) / gm.gmBlackBoxX;
			ofsx = (int)(ofsx * xForm.eM11);

			SetGraphicsMode(fhdl->hdcimage, GM_ADVANCED);
			SetWorldTransform(fhdl->hdcimage, &xForm);
			TextOut(fhdl->hdcimage, ofsx, ofsy, string, leng);
			ModifyWorldTransform(fhdl->hdcimage, NULL, MWT_IDENTITY);
			SetGraphicsMode(fhdl->hdcimage, GM_COMPATIBLE);
		}
		else {
			// そのまま描画
			TextOut(fhdl->hdcimage, ofsx, ofsy, string, leng);
		}
	}
	else {
		// 描画詳細をとれないので普通に出力
		TextOut(fhdl->hdcimage, 0, 0, string, leng);
	}

	// 左側1pxは切れる可能性があるので意図的に避ける
	if (fdat->width == 8) {
		int i;
		UINT8 hasbit = 0;
		int align = fhdl->bmpalign;
		for (i = 0; i < fdat->height * align; i += align) {
			hasbit |= fhdl->image[i];
		}
		if (!(hasbit & 0x01)) {
			// 右側が空いているのでずらして左側を空ける
			for (i = 0; i < fdat->height * align; i += align) {
				fhdl->image[i] >>= 1;
			}
		}
	}
	else if (fdat->width == 16) {
		int i;
		UINT8 hasbit = 0;
		int align = fhdl->bmpalign;
		for (i = 1; i < fdat->height * align; i += align) {
			hasbit |= fhdl->image[i];
		}
		if (!(hasbit & 0x01)) {
			// 右側が空いているのでずらして左側を空ける
			for (i = 1; i < fdat->height * align; i += align) {
				fhdl->image[i] = (fhdl->image[i] >> 1) | (fhdl->image[i - 1] << 7);
				fhdl->image[i - 1] >>= 1;
			}
		}
	}
}

BRESULT fontmng_getsize(void *hdl, const OEMCHAR *string, POINT_T *pt) {

	int		width;
	OEMCHAR	buf[4];
	_FNTDAT	fdat;
	int		leng;

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
		getlength1((FNTMNG)hdl, &fdat, buf, leng);
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
		getlength1((FNTMNG)hdl, &fdat, buf, leng);
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

