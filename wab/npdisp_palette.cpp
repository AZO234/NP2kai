/**
 * @file	npdisp_palette.c
 * @brief	Implementation of the Neko Project II Display Adapter Palette
 */

#include	"compiler.h"

#if defined(SUPPORT_WAB_NPDISP)

#include	<windows.h>
#include	<cmath>
#include	<cstdint>

#include	<map>
#include	<vector>

#include	"pccore.h"
#include	"cpucore.h"

#include	"npdispdef.h"
#include	"npdisp.h"
#include	"npdisp_mem.h"
#include	"npdisp_palette.h"

extern NPDISP_WINDOWS	npdispwin;

 // パレットカラー指定 or RGB指定を調整
#define NPDISP_ADJUST_COLORREF(a)	(((a) & 0xff000000) == 0x01000000 ? (a) : ((a) & 0xffffff))

// モノクロの場合RGB指定を白黒へ変換
#define NPDISP_ADJUST_MONOCOLOR_R(a)	((a) & 0xff)
#define NPDISP_ADJUST_MONOCOLOR_G(a)	(((a) >> 8) & 0xff)
#define NPDISP_ADJUST_MONOCOLOR_B(a)	(((a) >> 16) & 0xff)
#define NPDISP_ADJUST_MONOCOLOR_SUMRGB(a)	( NPDISP_ADJUST_MONOCOLOR_R(a) + NPDISP_ADJUST_MONOCOLOR_G(a) + NPDISP_ADJUST_MONOCOLOR_B(a) )
#define NPDISP_ADJUST_MONOCOLOR_TOMONO(a)	( (NPDISP_ADJUST_MONOCOLOR_SUMRGB(a) > NPDISP_MONO_THRESHOLD) ? 0xffffff : 0 )
#define NPDISP_ADJUST_MONOCOLOR_PAL(a)		( (a)==1 ? 0xffffff : 0x000000 )
#define NPDISP_ADJUST_MONOCOLOR(a)			(npdisp.bpp == 1 ? ( (((a) & 0xff000000) == 0x01000000) ? NPDISP_ADJUST_MONOCOLOR_PAL(a) : NPDISP_ADJUST_MONOCOLOR_TOMONO(a) ) : (a))

NPDISP_RGB3 npdisp_palette_rgb2[2] = {
	{0x00,0x00,0x00}, /* 0 black */
	{0xFF,0xFF,0xFF}  /* 1 white */
};
NPDISP_RGB3 npdisp_palette_rgb16[16] = {
	{0x00,0x00,0x00}, /* 0 black */
	{0x00,0x00,0x80}, /* 1 dark red */
	{0x00,0x80,0x00}, /* 2 dark green */
	{0x00,0x80,0x80}, /* 3 dark yellow */
	{0x80,0x00,0x00}, /* 4 dark blue */
	{0x80,0x00,0x80}, /* 5 dark magenta */
	{0x80,0x80,0x00}, /* 6 dark cyan */
	{0x80,0x80,0x80}, /* 7 dark gray */
	{0xC0,0xC0,0xC0}, /* 8 light gray */
	{0x00,0x00,0xFF}, /* 9 red */
	{0x00,0xFF,0x00}, /* 10 green */
	{0x00,0xFF,0xFF}, /* 11 yellow */
	{0xFF,0x00,0x00}, /* 12 blue */
	{0xFF,0x00,0xFF}, /* 13 magenta */
	{0xFF,0xFF,0x00}, /* 14 cyan */
	{0xFF,0xFF,0xFF}  /* 15 white */
};
NPDISP_RGB3 npdisp_palette_rgb256[256] = { 0 };
NPDISP_RGB3 npdisp_palette_gray256[256] = { 0 };

UINT16 npdisp_palette_transTbl[256] = { 0 };

void npdisp_palette_makeTable()
{
	NPDISP_RGB3* p256 = npdisp_palette_rgb256;

	// static colors
	p256[0].r = 0x00; p256[0].g = 0x00; p256[0].b = 0x00;
	p256[1].r = 0x80; p256[1].g = 0x00; p256[1].b = 0x00;
	p256[2].r = 0x00; p256[2].g = 0x80; p256[2].b = 0x00;
	p256[3].r = 0x80; p256[3].g = 0x80; p256[3].b = 0x00;
	p256[4].r = 0x00; p256[4].g = 0x00; p256[4].b = 0x80;
	p256[5].r = 0x80; p256[5].g = 0x00; p256[5].b = 0x80;
	p256[6].r = 0x00; p256[6].g = 0x80; p256[6].b = 0x80;
	p256[7].r = 0xc0; p256[7].g = 0xc0; p256[7].b = 0xc0;
	p256[8].r = 0xc0; p256[8].g = 0xdc; p256[8].b = 0xc0;
	p256[9].r = 0xa6; p256[9].g = 0xca; p256[9].b = 0xf0;

	p256[246].r = 0xff; p256[246].g = 0xfb; p256[246].b = 0xf0;
	p256[247].r = 0xa0; p256[247].g = 0xa0; p256[247].b = 0xa4;
	p256[248].r = 0x80; p256[248].g = 0x80; p256[248].b = 0x80;
	p256[249].r = 0xff; p256[249].g = 0x00; p256[249].b = 0x00;
	p256[250].r = 0x00; p256[250].g = 0xff; p256[250].b = 0x00;
	p256[251].r = 0xff; p256[251].g = 0xff; p256[251].b = 0x00;
	p256[252].r = 0x00; p256[252].g = 0x00; p256[252].b = 0xff;
	p256[253].r = 0xff; p256[253].g = 0x00; p256[253].b = 0xff;
	p256[254].r = 0x00; p256[254].g = 0xff; p256[254].b = 0xff;
	p256[255].r = 0xff; p256[255].g = 0xff; p256[255].b = 0xff;

	// とりあえずカラーキューブで埋める
	int index = 10;
	for (int r = 0; r < 6; r++) {
		for (int g = 0; g < 6; g++) {
			for (int b = 0; b < 6; b++) {
				p256[index].r = r * 51;
				p256[index].g = g * 51;
				p256[index].b = b * 51;
				index++;
			}
		}
	}

	// グレースケール256色
	for (int i = 0; i < NELEMENTS(npdisp_palette_gray256); i++) {
		npdisp_palette_gray256[i].r = i;
		npdisp_palette_gray256[i].g = i;
		npdisp_palette_gray256[i].b = i;
	}

	//// グレースケール16色
	//for (int i = 0; i < NELEMENTS(npdisp_palette_rgb16); i++) {
	//	npdisp_palette_rgb16[i].r = i * 255 / 15;
	//	npdisp_palette_rgb16[i].g = i * 255 / 15;
	//	npdisp_palette_rgb16[i].b = i * 255 / 15;
	//}

	//// 変なパレットテスト
	//npdisp_palette_rgb16[5].r = 0x60; npdisp_palette_rgb16[5].g = 0x38; npdisp_palette_rgb16[5].b = 0x30;
	//npdisp_palette_rgb16[6].r = 0xff; npdisp_palette_rgb16[6].g = 0xe0; npdisp_palette_rgb16[6].b = 0xd0;

	// 変換無し
	for (int i = 0; i < NELEMENTS(npdisp_palette_transTbl); i++) {
		npdisp_palette_transTbl[i] = i;
	}
}

void npdisp_palette_clearCache(int indexbegin, int indexEnd)
{

}

int npdisp_FindNearest2(UINT8 r, UINT8 g, UINT8 b)
{
	return ((UINT32)r + g + b > NPDISP_MONO_THRESHOLD_N) ? 1 : 0;
	//int i;
	//int best = 0;
	//long bestDist = 0x7FFFFFFFL;
	//for (i = 0; i < NELEMENTS(npdisp_palette_rgb2); i++) {
	//	long dr = (long)r - npdisp_palette_rgb2[i].r;
	//	long dg = (long)g - npdisp_palette_rgb2[i].g;
	//	long db = (long)b - npdisp_palette_rgb2[i].b;
	//	long dist = dr * dr + dg * dg + db * db;
	//	if (dist < bestDist) {
	//		bestDist = dist;
	//		best = i;
	//	}
	//}
	//return best;
}
int npdisp_FindNearest16(UINT8 r, UINT8 g, UINT8 b)
{
	int i;
	int best = 0;
	long bestDist = 0x7FFFFFFFL;
	for (i = 0; i < NELEMENTS(npdisp_palette_rgb16); i++) {
		long dr = (long)r - npdisp_palette_rgb16[i].r;
		long dg = (long)g - npdisp_palette_rgb16[i].g;
		long db = (long)b - npdisp_palette_rgb16[i].b;
		long dist = dr * dr + dg * dg + db * db;
		if (dist < bestDist) {
			bestDist = dist;
			best = i;
		}
	}
	return best;
}
int npdisp_FindNearest256(UINT8 r, UINT8 g, UINT8 b, bool fromSystemColor)
{
	int i;
	int best = 0;
	long bestDist = 0x7FFFFFFFL;
	// システムカラー（固定色）優先で探す
	for (i = 246; i < 256; i++) {
		long dr = (long)r - npdisp_palette_rgb256[i].r;
		long dg = (long)g - npdisp_palette_rgb256[i].g;
		long db = (long)b - npdisp_palette_rgb256[i].b;
		long dist = dr * dr + dg * dg + db * db;
		if (dist < bestDist) {
			bestDist = dist;
			best = i;
		}
	}
	for (i = 0; i < 10; i++) {
		long dr = (long)r - npdisp_palette_rgb256[i].r;
		long dg = (long)g - npdisp_palette_rgb256[i].g;
		long db = (long)b - npdisp_palette_rgb256[i].b;
		long dist = dr * dr + dg * dg + db * db;
		if (dist < bestDist) {
			bestDist = dist;
			best = i;
		}
	}
	if (!fromSystemColor) {
		// 無ければ自由色で
		for (i = 10; i < 246; i++) {
			long dr = (long)r - npdisp_palette_rgb256[i].r;
			long dg = (long)g - npdisp_palette_rgb256[i].g;
			long db = (long)b - npdisp_palette_rgb256[i].b;
			long dist = dr * dr + dg * dg + db * db;
			if (dist < bestDist) {
				bestDist = dist;
				best = i;
			}
		}
	}
	return best;
}
int npdisp_FindNearest256Tbl(UINT8 r, UINT8 g, UINT8 b)
{
	int i;
	int best = 0;
	long bestDist = 0x7FFFFFFFL;
	// テーブルから引く
	for (i = 0; i < 256; i++) {
		long dr = (long)r - npdisp_palette_rgb256[npdisp_palette_transTbl[i]].r;
		long dg = (long)g - npdisp_palette_rgb256[npdisp_palette_transTbl[i]].g;
		long db = (long)b - npdisp_palette_rgb256[npdisp_palette_transTbl[i]].b;
		long dist = dr * dr + dg * dg + db * db;
		if (dist < bestDist) {
			bestDist = dist;
			best = i;
		}
	}
	return best;
}

UINT32 npdisp_FindNearestColor(UINT8 r, UINT8 g, UINT8 b)
{
	if (npdisp.bpp == 1) {
		// 2値
		int idx = npdisp_FindNearest2(r, g, b);
		return ((UINT32)npdisp_palette_rgb2[idx].r) | ((UINT32)npdisp_palette_rgb2[idx].g << 8) | ((UINT32)npdisp_palette_rgb2[idx].b << 16);
	}
	else if (npdisp.bpp == 4) {
		// 16色
		int idx = npdisp_FindNearest16(r, g, b);
		return ((UINT32)npdisp_palette_rgb16[idx].r) | ((UINT32)npdisp_palette_rgb16[idx].g << 8) | ((UINT32)npdisp_palette_rgb16[idx].b << 16);
	}
	else if (npdisp.bpp == 8) {
		// 256色
		int idx = npdisp_FindNearest256(r, g, b);
		return ((UINT32)npdisp_palette_rgb16[idx].r) | ((UINT32)npdisp_palette_rgb16[idx].g << 8) | ((UINT32)npdisp_palette_rgb16[idx].b << 16);
	}
	else {
		// そのまま
		return ((UINT32)r) | ((UINT32)g << 8) | ((b << 16));
	}
}
UINT32 npdisp_FindNearestColorUINT32(UINT32 color)
{
	UINT8 r = (UINT8)(color & 0xFF);
	UINT8 g = (UINT8)((color >> 8) & 0xFF);
	UINT8 b = (UINT8)((color >> 16) & 0xFF);
	return npdisp_FindNearestColor(r, g, b);
}
UINT32 npdisp_FindNearestColorIndex(UINT8 r, UINT8 g, UINT8 b)
{
	if (npdisp.bpp == 1) {
		// 2値
		return npdisp_FindNearest2(r, g, b);
	}
	else if (npdisp.bpp == 4) {
		// 16色
		return npdisp_FindNearest16(r, g, b);
	}
	else if (npdisp.bpp == 8) {
		// 256色
		return npdisp_FindNearest256(r, g, b);
	}
	else {
		// ない
		return 0xffffffff;
	}
}
UINT32 npdisp_FindNearestColorIndexUINT32(UINT32 color)
{
	UINT8 r = (UINT8)(color & 0xFF);
	UINT8 g = (UINT8)((color >> 8) & 0xFF);
	UINT8 b = (UINT8)((color >> 16) & 0xFF);
	return npdisp_FindNearestColor(r, g, b);
}

UINT32 npdisp_ObjIdxToColor(int idx)
{
	if (npdisp.bpp == 1) {
		// 2値カラーを返す
		if (idx < NELEMENTS(npdisp_palette_rgb2)) {
			return ((UINT32)npdisp_palette_rgb2[idx].r) | ((UINT32)npdisp_palette_rgb2[idx].g << 8) | ((UINT32)npdisp_palette_rgb2[idx].b << 16);
		}
	}
	else if (npdisp.bpp == 4) {
		// 16色パレットカラーを返す
		if (idx < NELEMENTS(npdisp_palette_rgb16)) {
			return ((UINT32)npdisp_palette_rgb16[idx].r) | ((UINT32)npdisp_palette_rgb16[idx].g << 8) | ((UINT32)npdisp_palette_rgb16[idx].b << 16);
		}
	}
	else {
		// 256色固定カラーを返す
		if (idx < 10) {
			return ((UINT32)npdisp_palette_rgb256[idx].r) | ((UINT32)npdisp_palette_rgb256[idx].g << 8) | ((UINT32)npdisp_palette_rgb256[idx].b << 16);
		}
		else if (idx < 20) {
			int lidx = NELEMENTS(npdisp_palette_rgb256) - 20 + idx;
			return ((UINT32)npdisp_palette_rgb256[lidx].r) | ((UINT32)npdisp_palette_rgb256[lidx].g << 8) | ((UINT32)npdisp_palette_rgb256[lidx].b << 16);
		}
	}
	return 0xffffffff;
}

static UINT32 npdisp_AdjustRGB555(UINT32 rgb, bool use24buf) {
	UINT8 r = rgb & 0xff;
	UINT8 g = (rgb >> 8) & 0xff;
	UINT8 b = (rgb >> 16) & 0xff;
	UINT16* pBits = (UINT16*)npdispwin.pBitsBltBuf;

	if (!npdispwin.pBitsBltBuf) return rgb;

	*pBits = (b >> 3) | ((g >> 3) << 5) | ((r >> 3) << 10);
	//if (use24buf) {
	//	BitBlt(npdispwin.hdc16BltBuf, 0, 0, 1, 1, npdispwin.hdcBltBuf, 0, 0, SRCCOPY);
	//	return GetPixel(npdispwin.hdc16BltBuf, 0, 0);
	//}
	//else {
		return GetPixel(npdispwin.hdcBltBuf, 0, 0);
	//}
}
static UINT32 npdisp_AdjustRGB565(UINT32 rgb, bool use24buf) {
	UINT8 r = rgb & 0xff;
	UINT8 g = (rgb >> 8) & 0xff;
	UINT8 b = (rgb >> 16) & 0xff;
	UINT16* pBits = (UINT16*)npdispwin.pBitsBltBuf;

	if (!npdispwin.pBitsBltBuf) return rgb;

	*pBits = (b >> 3) | ((g >> 2) << 5) | ((r >> 3) << 11);
	//if (use24buf) {
	//	BitBlt(npdispwin.hdc16BltBuf, 0, 0, 1, 1, npdispwin.hdcBltBuf, 0, 0, SRCCOPY);
	//	return GetPixel(npdispwin.hdc16BltBuf, 0, 0);
	//}
	//else {
		return GetPixel(npdispwin.hdcBltBuf, 0, 0);
	//}
}

UINT32 npdisp_AdjustColorRefForGDI(UINT32 color, bool* preferDither)
{
	if (preferDither) *preferDither = false;
	if (npdisp.bpp == 1) {
		if (color & 0xff000000) {
			// パレットで渡された場合、白黒変換
			int colorIdx = color & 0x1;
			return ((UINT32)npdisp_palette_rgb2[colorIdx].r) | ((UINT32)npdisp_palette_rgb2[colorIdx].g << 8) | ((UINT32)npdisp_palette_rgb2[colorIdx].b << 16);
		}
		else {
			// 色で渡された場合、素通し
			if (preferDither) {
				// 純色か調べ、そうでなければディザ推奨とする
				int physicalColor = npdisp_FindNearestColorUINT32(color);
				*preferDither = (physicalColor != color);
			}
			return color;
		}
	}
	else if (npdisp.bpp == 4) {
		if (color & 0xff000000) {
			// パレットで渡された場合、色変換
			int colorIdx = color & 0xf;
			return ((UINT32)npdisp_palette_rgb16[colorIdx].r) | ((UINT32)npdisp_palette_rgb16[colorIdx].g << 8) | ((UINT32)npdisp_palette_rgb16[colorIdx].b << 16);
		}
		else {
			// 色で渡された場合、素通し
			if (preferDither) {
				// 純色か調べ、そうでなければディザ推奨とする
				int physicalColor = npdisp_FindNearestColorUINT32(color);
				*preferDither = (physicalColor != color);
			}
			return color;
		}
	}
	else if (npdisp.bpp == 8) {
		UINT32 idx = 0;
		if (color & 0xff000000) {
			// パレットで渡された場合、既に物理パレットなのでそのまま
			idx = color & 0xff;
		}
		else {
			// 色で渡された場合、物理パレットインデックスへ変換
			idx = npdisp_FindNearest256(color & 0xff, (color >> 8) & 0xff, (color >> 16) & 0xff);
			if (preferDither) {
				// 純色か調べ、そうでなければディザ推奨とする
				int physicalColor = ((UINT32)npdisp_palette_rgb256[idx].r) | ((UINT32)npdisp_palette_rgb256[idx].g << 8) | ((UINT32)npdisp_palette_rgb256[idx].b << 16);
				if (physicalColor != color) {
					*preferDither = true;
					return color; // 返す色も実際の色とする
				}
			}
		}
		return idx | (idx << 8) | (idx << 16);
	}
	else {
		// 素通し
		return color;
	}
}
void npdisp_AdjustDrawModeColor(NPDISP_DRAWMODE* drawMode, bool use24buf) {
	if (npdisp.bpp <= 8) {
		drawMode->LTextColor = npdisp_AdjustColorRefForGDI(drawMode->TextColor);
		drawMode->LbkColor = npdisp_AdjustColorRefForGDI(drawMode->bkColor);
	}
	else if (npdisp.bpp == 15) {
		// RGB555が展開された実際の色を設定
		drawMode->LTextColor = npdisp_AdjustRGB555(drawMode->TextColor, use24buf);
		drawMode->LbkColor = npdisp_AdjustRGB555(drawMode->bkColor, use24buf);
	}
	else if (npdisp.bpp == 16) {
		// RGB565が展開された実際の色を設定
		drawMode->LTextColor = npdisp_AdjustRGB565(drawMode->TextColor, use24buf);
		drawMode->LbkColor = npdisp_AdjustRGB565(drawMode->bkColor, use24buf);
	}
	else if (npdisp.bpp == 24) {
		//// 仮想パレット色が渡されたら256色扱いで変換
		//if ((drawMode->LTextColor & 0xff000000) == 0x01000000) {
		//	int idx = drawMode->LTextColor & 0xff;
		//	drawMode->LTextColor = ((UINT32)npdisp_palette_rgb256[idx].r) | ((UINT32)npdisp_palette_rgb256[idx].g << 8) | ((UINT32)npdisp_palette_rgb256[idx].b << 16);
		//}
		//if ((drawMode->LbkColor & 0xff000000) == 0x01000000) {
		//	int idx = drawMode->LbkColor & 0xff;
		//	drawMode->LbkColor = ((UINT32)npdisp_palette_rgb256[idx].r) | ((UINT32)npdisp_palette_rgb256[idx].g << 8) | ((UINT32)npdisp_palette_rgb256[idx].b << 16);
		//}
		// XXX: パレット色が渡されたらTextColorやbkColorを素通し
		if ((drawMode->LTextColor & 0xff000000) == 0x01000000) {
			drawMode->LTextColor = drawMode->TextColor;
		}
		if ((drawMode->LbkColor & 0xff000000) == 0x01000000) {
			drawMode->LbkColor = drawMode->bkColor;
		}
	}
}
void npdisp_AdjustSrcMonoPaletteByDrawMode(NPDISP_WINDOWS_BMPHDC* bmpHdcSrc, NPDISP_WINDOWS_BMPHDC* bmpHdcDst, NPDISP_DRAWMODE* drawMode) {
	if (bmpHdcSrc->lpbi->bmiHeader.biBitCount == 1 && (bmpHdcDst && bmpHdcDst->lpbi->bmiHeader.biBitCount != 1 || !bmpHdcDst && npdisp.bpp != 1)) {
		RGBQUAD pal[2];
		pal[0].rgbRed = drawMode->LTextColor & 0xff;
		pal[0].rgbGreen = (drawMode->LTextColor >> 8) & 0xff;
		pal[0].rgbBlue = (drawMode->LTextColor >> 16) & 0xff;
		pal[0].rgbReserved = 0;
		pal[1].rgbRed = drawMode->LbkColor & 0xff;
		pal[1].rgbGreen = (drawMode->LbkColor >> 8) & 0xff;
		pal[1].rgbBlue = (drawMode->LbkColor >> 16) & 0xff;
		pal[1].rgbReserved = 0;
		SetDIBColorTable(bmpHdcSrc->hdc, 0, 2, pal);
	}
}

static double Dist2(const NPDISP_RGB3& color, BYTE r, BYTE g, BYTE b)
{
	double dr = double(color.r) - r;
	double dg = double(color.g) - g;
	double db = double(color.b) - b;
	return dr * dr + dg * dg + db * db;
}

static double Dist2Color(const NPDISP_RGB3& a, const NPDISP_RGB3& b)
{
	double dr = double(a.r) - b.r;
	double dg = double(a.g) - b.g;
	double db = double(a.b) - b.b;
	return dr * dr + dg * dg + db * db;
}

static double Clamp01(double x)
{
	if (x < 0.0) return 0.0;
	if (x > 1.0) return 1.0;
	return x;
}

// c0 + t*(c1-c0) が target に最も近くなる t を求める
static double MixFactor(const NPDISP_RGB3& c0, const NPDISP_RGB3& c1, BYTE tr, BYTE tg, BYTE tb)
{
	double v0r = c0.r, v0g = c0.g, v0b = c0.b;
	double v1r = c1.r, v1g = c1.g, v1b = c1.b;
	double dr = v1r - v0r, dg = v1g - v0g, db = v1b - v0b;
	double rr = tr - v0r, rg = tg - v0g, rb = tb - v0b;

	double denom = dr * dr + dg * dg + db * db;
	if (denom <= 1e-12) return 0.0;

	double t = (rr * dr + rg * dg + rb * db) / denom;
	return Clamp01(t);
}

void MakePaletteDitherBrushColor(UINT32 target, UINT32* actual1, UINT32* actual2, double* bestTValue)
{
	NPDISP_RGB3* colors = NULL;
	int n = 0;

	if (npdisp.bpp == 1) {
		colors = npdisp_palette_rgb2;
		n = NELEMENTS(npdisp_palette_rgb2);
	}
	else if (npdisp.bpp == 4) {
		colors = npdisp_palette_rgb16;
		n = NELEMENTS(npdisp_palette_rgb16);
	}
	else if (npdisp.bpp == 8) {
		colors = npdisp_palette_rgb256;
		n = NELEMENTS(npdisp_palette_rgb256);
	}
	else {
		*actual1 = target;
		*actual2 = target;
		return;
	}

	BYTE tr = GetRValue(target);
	BYTE tg = GetGValue(target);
	BYTE tb = GetBValue(target);

	// 単色最近傍も初期値として保持しておく
	int i0 = 0;
	int i1 = 0;
	double bestErr = Dist2(colors[0], tr, tg, tb);
	double bestT = 0.0;

	// 2色が離れすぎる組を避けるための重み
	const double pairPenaltyWeight = 0.005;

	if (npdisp.bpp == 8) {
		if (tr == tg && tg == tb) {
			// 特例　グレーの範囲の色だけを探す
			for (int a = 0; a < n; ++a) {
				if (a == 10) a += n - 20;
				for (int b = 0; b < n; ++b) {
					if (b == 10) b += n - 20;
					if (a == b) continue;

					if (colors[a].r != colors[a].g || colors[a].g != colors[a].b) continue;
					if (colors[b].r != colors[b].g || colors[b].g != colors[b].b) continue;

					double t = MixFactor(colors[a], colors[b], tr, tg, tb);

					double mr = colors[a].r + t * (colors[b].r - colors[a].r);
					double mg = colors[a].g + t * (colors[b].g - colors[a].g);
					double mb = colors[a].b + t * (colors[b].b - colors[a].b);

					double er = mr - tr;
					double eg = mg - tg;
					double eb = mb - tb;
					double err = er * er + eg * eg + eb * eb;

					// 離れすぎた2色の組にペナルティ
					err += pairPenaltyWeight * Dist2Color(colors[a], colors[b]);

					if (err < bestErr) {
						bestErr = err;
						i0 = a;
						i1 = b;
						bestT = t;
					}
				}
			}
		}
		else {
			// 全組み合わせを探索
			for (int a = 0; a < n; ++a) {
				if (a == 10) a += n - 20;
				for (int b = 0; b < n; ++b) {
					if (b == 10) b += n - 20;
					if (a == b) continue;

					double t = MixFactor(colors[a], colors[b], tr, tg, tb);

					double mr = colors[a].r + t * (colors[b].r - colors[a].r);
					double mg = colors[a].g + t * (colors[b].g - colors[a].g);
					double mb = colors[a].b + t * (colors[b].b - colors[a].b);

					double er = mr - tr;
					double eg = mg - tg;
					double eb = mb - tb;
					double err = er * er + eg * eg + eb * eb;

					// 離れすぎた2色の組にペナルティ
					err += pairPenaltyWeight * Dist2Color(colors[a], colors[b]);

					if (err < bestErr) {
						bestErr = err;
						i0 = a;
						i1 = b;
						bestT = t;
					}
				}
			}
		}
	}
	else if (npdisp.bpp == 1) {
		// 選択の余地無し
		i0 = 0;
		i1 = 1;
		bestT = MixFactor(colors[0], colors[1], tr, tg, tb);
	}
	else {
		// 全組み合わせを探索
		for (int a = 0; a < n; ++a) {
			for (int b = 0; b < n; ++b) {
				if (a == b) continue;

				double t = MixFactor(colors[a], colors[b], tr, tg, tb);

				double mr = colors[a].r + t * (colors[b].r - colors[a].r);
				double mg = colors[a].g + t * (colors[b].g - colors[a].g);
				double mb = colors[a].b + t * (colors[b].b - colors[a].b);

				double er = mr - tr;
				double eg = mg - tg;
				double eb = mb - tb;
				double err = er * er + eg * eg + eb * eb;

				// 離れすぎた2色の組にペナルティ
				err += pairPenaltyWeight * Dist2Color(colors[a], colors[b]);

				if (err < bestErr) {
					bestErr = err;
					i0 = a;
					i1 = b;
					bestT = t;
				}
			}
		}
	}

	*actual1 = i0;
	*actual2 = i1;
	*bestTValue = bestT;
}

HBRUSH CreatePaletteDitherBrush(UINT32 actual1, UINT32 actual2, double bestTValue)
{
	NPDISP_RGB3* colors = NULL;
	int n = 0;

	if (npdisp.bpp == 1) {
		colors = npdisp_palette_rgb2;
		n = NELEMENTS(npdisp_palette_rgb2);
	}
	else if (npdisp.bpp == 4) {
		colors = npdisp_palette_rgb16;
		n = NELEMENTS(npdisp_palette_rgb16);
	}
	else if (npdisp.bpp == 8) {
		colors = npdisp_palette_gray256;
		n = NELEMENTS(npdisp_palette_gray256);
	}
	else {
		return CreateSolidBrush(actual1);
	}

	int i0 = actual1;
	int i1 = actual2;
	double bestT = bestTValue;

	// 4x4 Bayer matrix
	static BYTE bayer4[4][4] = {
		{ 15, 7,13, 5 },
		{  0, 8, 2,10 },
		{ 12, 4,14, 6 },
		{  3,11, 1, 9 },
	};

	// 8bpp BI_RGB DIB:
	// [BITMAPINFOHEADER][RGBQUAD x paletteSize][pixel data]
	const int W = 8;
	const int H = 8;
	const int paletteSize = n;
	const int stride = ((W + 3) & ~3); // 8bppなので1pixel=1byte
	const int imageSize = stride * H;

	size_t totalSize =
		sizeof(BITMAPINFOHEADER) +
		sizeof(RGBQUAD) * paletteSize +
		imageSize;

	std::vector<BYTE> dib(totalSize, 0);

	BITMAPINFOHEADER* bih = reinterpret_cast<BITMAPINFOHEADER*>(dib.data());
	bih->biSize = sizeof(BITMAPINFOHEADER);
	bih->biWidth = W;
	bih->biHeight = H;   // bottom-up
	bih->biPlanes = 1;
	bih->biBitCount = 8;
	bih->biCompression = BI_RGB;
	bih->biSizeImage = imageSize;
	bih->biClrUsed = paletteSize;
	bih->biClrImportant = paletteSize;

	RGBQUAD* table = reinterpret_cast<RGBQUAD*>(dib.data() + sizeof(BITMAPINFOHEADER));
	for (int i = 0; i < paletteSize; ++i) {
		table[i].rgbRed = colors[i].r;
		table[i].rgbGreen = colors[i].g;
		table[i].rgbBlue = colors[i].b;
		table[i].rgbReserved = 0;
	}

	BYTE* bits = dib.data() + sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * paletteSize;

	int threshold = int(bestT * 16.0 + 0.5);

	for (int y = 0; y < H; ++y) {
		BYTE* row = bits + (H - 1 - y) * stride; // bottom-up
		for (int x = 0; x < W; ++x) {
			row[x] = (bayer4[y % 4][x % 4] < threshold) ? (BYTE)i1 : (BYTE)i0;
		}
	}

	return CreateDIBPatternBrushPt(dib.data(), DIB_RGB_COLORS);
}

#endif