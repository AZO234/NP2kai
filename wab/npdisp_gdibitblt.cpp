/**
 * @file	npdisp_gdioutput.c
 * @brief	Implementation of the Neko Project II Display Adapter GDI BitBlt Functions
 */

#include	"compiler.h"

#if defined(SUPPORT_WAB_NPDISP)

#include	<map>
#include	<vector>

#include	"pccore.h"
#include	"cpucore.h"

#include	"npdispdef.h"
#include	"npdisp.h"
#include	"npdisp_mem.h"
#include	"npdisp_palette.h"
#include	"npdisp_gdibitblt.h"

#pragma comment(lib, "Msimg32.lib")

//#define IMAGEDEBUG
//#define IMAGEDEBUG_SIZE	32
//#define IMAGEDEBUG_X	0

#if 0
#undef	TRACEOUT
static void trace_fmt_ex(const char* fmt, ...)
{
	char stmp[2048];
	va_list ap;
	va_start(ap, fmt);
	vsprintf(stmp, fmt, ap);
	strcat(stmp, "\n");
	va_end(ap);
	OutputDebugStringA(stmp);
}
#define	TRACEOUT(s)	trace_fmt_ex s
#if 1
#define	TRACEOUT_BITBLT(s)	TRACEOUT(s)
#else
#define	TRACEOUT_BITBLT(s)	(void)s
#endif
#else
#define	TRACEOUT_BITBLT(s)	(void)s
#endif	/* 1 */
#if 0
static void trace_fmt_ex2(const char* fmt, ...)
{
	char stmp[2048];
	va_list ap;
	va_start(ap, fmt);
	vsprintf(stmp, fmt, ap);
	strcat(stmp, "\n");
	va_end(ap);
	OutputDebugStringA(stmp);
}
#define	TRACEOUT2(s)	trace_fmt_ex2 s
#else
#define	TRACEOUT2(s)	(void)s
#endif	/* 1 */
#if 0
static void trace_fmt_ex3(const char* fmt, ...)
{
	char stmp[2048];
	va_list ap;
	va_start(ap, fmt);
	vsprintf(stmp, fmt, ap);
	strcat(stmp, "\n");
	va_end(ap);
	OutputDebugStringA(stmp);
}
#define	TRACEOUT3(s)	trace_fmt_ex3 s
#else
#define	TRACEOUT3(s)	(void)s
#endif	/* 1 */

extern NPDISP_WINDOWS	npdispwin;

// StretchBltと共用にする

UINT16 npdisp_func_StretchBlt_VRAMtoVRAM(int hasDstDev, int hasSrcDev, UINT32 lpDestDevAddr, SINT16 wDestX, SINT16 wDestY, SINT16 wDestXext, SINT16 wDestYext, UINT32 lpSrcDevAddr, SINT16 wSrcX, SINT16 wSrcY, SINT16 wSrcXext, SINT16 wSrcYext, UINT32 Rop3, UINT32 lpPBrushAddr, UINT32 lpDrawModeAddr, UINT32 lpClipAddr)
{
	// VRAM -> VRAM
	int stretchMode = COLORONCOLOR;
	bool isStretch = wDestXext != wSrcXext || wDestYext != wSrcYext;
	if (isStretch) {
		TRACEOUT_BITBLT(("Stretchlt VRAM -> VRAM DEST X:%d Y:%d W:%d H:%d, rop:%08x", wDestX, wDestY, wDestXext, wDestYext, Rop3));
		if (npdisp.version >= 4 && npdisp.isWin9x) {
			if (lpDrawModeAddr) stretchMode = npdisp_readMemory16(lpDrawModeAddr + 36); // Win9x StretchBltModeを読む
		}
		else {
			if (npdisp.bpp == 1) return 0xffff; //モノクロソースの時はCOLORONCOLOR以外だと致命的なのでGDIにやらせる
		}
	}
	else {
		TRACEOUT_BITBLT(("BitBlt VRAM -> VRAM DEST X:%d Y:%d W:%d H:%d, rop:%08x", wDestX, wDestY, wDestXext, wDestYext, Rop3));
	}
	HRGN hRgn = NULL;
	if (lpClipAddr) {
		RECT cliprect = { 0 };
		NPDISP_RECT rectTmp = { 0 };
		npdisp_readMemory(&rectTmp, lpClipAddr, sizeof(NPDISP_RECT));
		cliprect.top = rectTmp.top;
		cliprect.left = rectTmp.left;
		cliprect.bottom = rectTmp.bottom;
		cliprect.right = rectTmp.right;
		hRgn = CreateRectRgn(cliprect.left, cliprect.top, cliprect.right, cliprect.bottom);
	}
	NPDISP_DRAWMODE drawMode = { 0 };
	int hasDrawMode = npdisp_readMemory(&drawMode, lpDrawModeAddr, sizeof(NPDISP_DRAWMODE));
	if (hasDrawMode) {
		if (memcmp(&drawMode, &npdispwin.lastScreenDrawMode, sizeof(NPDISP_DRAWMODE)) != 0) {
			npdispwin.lastScreenDrawMode = drawMode;
			npdisp_AdjustDrawModeColor(&drawMode);
			SetBkColor(npdispwin.hdc, drawMode.LbkColor);
			SetTextColor(npdispwin.hdc, drawMode.LTextColor);
			SetBkMode(npdispwin.hdc, drawMode.bkMode);
			SetROP2(npdispwin.hdc, drawMode.Rop2);
		}
	}
	else {
		SetBkColor(npdispwin.hdc, 0xffffff);
		SetTextColor(npdispwin.hdc, 0x000000);
	}
	if (lpPBrushAddr) {
		// ブラシがあれば選択
		NPDISP_BRUSH brush = { 0 };
		if (npdisp_readMemory(&brush, lpPBrushAddr, sizeof(NPDISP_BRUSH))) {
			if (brush.key != 0) {
				auto it = npdispwin.brushes.find(brush.key);
				if (it != npdispwin.brushes.end()) {
					NPDISP_HOSTBRUSH value = it->second;
					if (value.brs) {
						TRACEOUT_BITBLT(("-> style=%d, hatch=%d, color=%08x", value.lbrush.lbStyle, value.lbrush.lbHatch, value.lbrush.lbColor));
						SelectObject(npdispwin.hdc, value.brs);
						if (brush.lbrush.lbStyle == NPDISP_BRUSH_STYLE_HATCHED) {
							SetBkColor(npdispwin.hdc, npdisp_AdjustColorRefForGDI(brush.lbrush.lbBkColor));
						}
					}
					else {
						SelectObject(npdispwin.hdc, (HBRUSH)GetStockObject(NULL_BRUSH));
					}
				}
			}
		}
	}
	int srcx = wSrcX;
	int srcy = wSrcY;
	int destx = wDestX;
	int desty = wDestY;
	int srcw = wSrcXext;
	int srch = wSrcYext;
	int destw = wDestXext;
	int desth = wDestYext;
	if(hRgn) SelectClipRgn(npdispwin.hdc, hRgn);
	if (!(srcx + srcw < destx || destx + srcw < srcx || srcy + srch < desty || desty + desth < srcy)) {
		// 重なっているのでバッファ経由
		BitBlt(npdispwin.hdcBltBuf, wSrcX, wSrcY, wSrcXext, wSrcYext, npdispwin.hdc, wSrcX, wSrcY, SRCCOPY);
		if (hasDrawMode && drawMode.bkMode == 4) { // TRANSPARENT1
			SetStretchBltMode(npdispwin.hdc, stretchMode);
			TransparentBlt(npdispwin.hdc, wDestX, wDestY, wDestXext, wDestYext, npdispwin.hdcBltBuf, wSrcX, wSrcY, wSrcXext, wSrcYext, drawMode.LbkColor);
		}
		else if (isStretch) {
			SetStretchBltMode(npdispwin.hdc, stretchMode);
			StretchBlt(npdispwin.hdc, wDestX, wDestY, wDestXext, wDestYext, npdispwin.hdcBltBuf, wSrcX, wSrcY, wSrcXext, wSrcYext, Rop3);
		}
		else {
			BitBlt(npdispwin.hdc, wDestX, wDestY, wDestXext, wDestYext, npdispwin.hdcBltBuf, wSrcX, wSrcY, Rop3);
		}
	}
	else {
		// 重なっていないので直接転送
		if (hasDrawMode && drawMode.bkMode == 4) { // TRANSPARENT1
			SetStretchBltMode(npdispwin.hdc, stretchMode);
			TransparentBlt(npdispwin.hdc, wDestX, wDestY, wDestXext, wDestYext, npdispwin.hdc, wSrcX, wSrcY, wSrcXext, wSrcYext, drawMode.LbkColor);
		}
		else if (isStretch) {
			SetStretchBltMode(npdispwin.hdc, stretchMode);
			StretchBlt(npdispwin.hdc, wDestX, wDestY, wDestXext, wDestYext, npdispwin.hdc, wSrcX, wSrcY, wSrcXext, wSrcYext, Rop3);
		}
		else {
			BitBlt(npdispwin.hdc, wDestX, wDestY, wDestXext, wDestYext, npdispwin.hdc, wSrcX, wSrcY, Rop3);
		}
	}
	if (hRgn) SelectClipRgn(npdispwin.hdc, NULL);
	SelectObject(npdispwin.hdc, npdispwin.hOldBrush);
	npdisp_setDirty(wDestX, wDestY, wDestX + (wDestXext < 0 ? -wDestXext : wDestXext), wDestY + (wDestYext < 0 ? -wDestYext : wDestYext));
	npdisp.updated = 1;

	if (hRgn) {
		DeleteObject(hRgn);
	}

	return 1;
}
UINT16 npdisp_func_BitBlt_VRAMtoVRAM(int hasDstDev, int hasSrcDev, UINT32 lpDestDevAddr, SINT16 wDestX, SINT16 wDestY, UINT32 lpSrcDevAddr, SINT16 wSrcX, SINT16 wSrcY, UINT16 wXext, UINT16 wYext, UINT32 Rop3, UINT32 lpPBrushAddr, UINT32 lpDrawModeAddr)
{
	return npdisp_func_StretchBlt_VRAMtoVRAM(hasDstDev, hasSrcDev, lpDestDevAddr, wDestX, wDestY, wXext, wYext, lpSrcDevAddr, wSrcX, wSrcY, wXext, wYext, Rop3, lpPBrushAddr, lpDrawModeAddr, 0);
}
UINT16 npdisp_func_StretchBlt_MEMtoVRAM(int hasDstDev, int hasSrcDev, UINT32 lpDestDevAddr, SINT16 wDestX, SINT16 wDestY, SINT16 wDestXext, SINT16 wDestYext, UINT32 lpSrcDevAddr, SINT16 wSrcX, SINT16 wSrcY, SINT16 wSrcXext, SINT16 wSrcYext, UINT32 Rop3, UINT32 lpPBrushAddr, UINT32 lpDrawModeAddr, UINT32 lpClipAddr)
{
	// MEM -> VRAM
	int stretchMode = COLORONCOLOR;
	UINT16 retValue = 1;
	
	// 実際に転送する範囲を計算
	int srcBeginLine = wSrcY;
	int srcNumLines = wSrcYext;
	int dstBeginLine = wDestY;
	int dstNumLines = wDestYext;
	if (srcBeginLine < 0) {
		srcNumLines += srcBeginLine;
		srcBeginLine = 0;
	}
	if (dstBeginLine < 0) {
		dstNumLines += dstBeginLine;
		dstBeginLine = 0;
	}
	int srcBeginX = wSrcX;
	int srcCopyWidth = wSrcXext;
	int dstBeginX = wDestX;
	int dstCopyWidth = wDestXext;
	if (srcBeginX < 0) {
		srcCopyWidth += srcBeginX;
		srcBeginX = 0;
	}
	if (dstBeginX < 0) {
		dstCopyWidth += dstBeginX;
		dstBeginX = 0;
	}

	bool isStretch = wDestXext != wSrcXext || wDestYext != wSrcYext;
	if (isStretch) {
		TRACEOUT_BITBLT(("Stretchlt MEM -> VRAM DEST X:%d Y:%d W:%d H:%d, rop:%08x", wDestX, wDestY, wDestXext, wDestYext, Rop3));
		if (npdisp.version >= 4 && npdisp.isWin9x) {
			if (lpDrawModeAddr) stretchMode = npdisp_readMemory16(lpDrawModeAddr + 36); // Win9x StretchBltModeを読む
		}
	}
	else {
		TRACEOUT_BITBLT(("BitBlt MEM -> VRAM DEST X:%d Y:%d W:%d H:%d, rop:%08x", wDestX, wDestY, wDestXext, wDestYext, Rop3));
	}
	HRGN hRgn = NULL;
	if (lpClipAddr) {
		RECT cliprect = { 0 };
		NPDISP_RECT rectTmp = { 0 };
		npdisp_readMemory(&rectTmp, lpClipAddr, sizeof(NPDISP_RECT));
		cliprect.top = rectTmp.top;
		cliprect.left = rectTmp.left;
		cliprect.bottom = rectTmp.bottom;
		cliprect.right = rectTmp.right;
		hRgn = CreateRectRgn(cliprect.left, cliprect.top, cliprect.right, cliprect.bottom);
	}
	NPDISP_PBITMAP_EXT srcPBmp;
	if (lpSrcDevAddr && npdisp_readPBitmap(&srcPBmp, lpSrcDevAddr)) {
		if (!isStretch || srcPBmp.bmBitsPixel != 1 || npdisp.isWin9x) {
			NPDISP_WINDOWS_BMPHDC bmphdc = { 0 };
			npdisp_PreloadBitmapFromPBITMAP(&srcPBmp, 0, srcBeginLine, srcNumLines, srcBeginX, srcCopyWidth);
			if (npdisp.longjmpnum == 0 && npdisp_MakeBitmapFromPBITMAP(&srcPBmp, &bmphdc, 0, srcBeginLine, srcNumLines, srcBeginX, srcCopyWidth, npdisp_palette_transTbl)) {
				npdisp_ConvertToDDBMonoBitmap(&bmphdc);
				NPDISP_DRAWMODE drawMode = { 0 };
				int hasDrawMode = npdisp_readMemory(&drawMode, lpDrawModeAddr, sizeof(NPDISP_DRAWMODE));
				if (hasDrawMode) {
					if (memcmp(&drawMode, &npdispwin.lastScreenDrawMode, sizeof(NPDISP_DRAWMODE)) != 0) {
						npdispwin.lastScreenDrawMode = drawMode;
						npdisp_AdjustDrawModeColor(&drawMode);
						SetBkColor(npdispwin.hdc, drawMode.LbkColor);
						SetTextColor(npdispwin.hdc, drawMode.LTextColor);
						SetBkMode(npdispwin.hdc, drawMode.bkMode);
						SetROP2(npdispwin.hdc, drawMode.Rop2);
					}
					// ソースにもセットが必要
					SetBkColor(bmphdc.hdc, drawMode.LbkColor);
					SetTextColor(bmphdc.hdc, drawMode.LTextColor);
					SetBkMode(bmphdc.hdc, drawMode.bkMode);
					SetROP2(bmphdc.hdc, drawMode.Rop2);
					//npdisp_AdjustSrcMonoPaletteByDrawMode(&bmphdc, NULL, &drawMode);
				}
				else {
					SetBkColor(npdispwin.hdc, 0xffffff);
					SetTextColor(npdispwin.hdc, 0x000000);
					SetBkColor(bmphdc.hdc, 0xffffff);
					SetTextColor(bmphdc.hdc, 0x000000);
				}
				if (lpPBrushAddr) {
					// ブラシがあれば選択
					NPDISP_BRUSH brush = { 0 };
					if (npdisp_readMemory(&brush, lpPBrushAddr, sizeof(NPDISP_BRUSH))) {
						if (brush.key != 0) {
							auto it = npdispwin.brushes.find(brush.key);
							if (it != npdispwin.brushes.end()) {
								NPDISP_HOSTBRUSH value = it->second;
								if (value.brs) {
									TRACEOUT_BITBLT(("-> style=%d, hatch=%d, color=%08x", value.lbrush.lbStyle, value.lbrush.lbHatch, value.lbrush.lbColor));
									SelectObject(npdispwin.hdc, value.brs);
									if (brush.lbrush.lbStyle == NPDISP_BRUSH_STYLE_HATCHED) {
										SetBkColor(npdispwin.hdc, npdisp_AdjustColorRefForGDI(brush.lbrush.lbBkColor));
									}
								}
								else {
									SelectObject(npdispwin.hdc, (HBRUSH)GetStockObject(NULL_BRUSH));
								}
							}
						}
					}
				}

				bool useActualColor = false;
				if (npdisp.bpp == 8 && npdisp.usePalette) {
					if ((srcPBmp.bmType == NPDISP_DEVTYPE_DIBENG || srcPBmp.bmType == NPDISP_DEVTYPE_DDB) && srcPBmp.bmBitsPixel != 8 && srcPBmp.bmBitsPixel != 1) {
						// パレット番号（グレースケール）から実際のデバイス色へ置き換え
						RGBQUAD pal[256];
						for (int i = 0; i < 256; i++) {
							pal[i].rgbRed = npdisp_palette_rgb256[i].r;
							pal[i].rgbGreen = npdisp_palette_rgb256[i].g;
							pal[i].rgbBlue = npdisp_palette_rgb256[i].b;
							pal[i].rgbReserved = 0;
						}
						SetDIBColorTable(npdispwin.hdc, 0, 256, pal);
						useActualColor = true;
					}
				}

				if (hRgn) SelectClipRgn(npdispwin.hdc, hRgn);
				HDC srcHDC = bmphdc.hdc;
				//if ((bmphdc.lpbi->bmiHeader.biBitCount == 16 || bmphdc.lpbi->bmiHeader.biBitCount == 15) && npdisp.bpp == 1 && npdispwin.hdc16BltBuf) {
				//	BitBlt(npdispwin.hdc16BltBuf, wSrcX, wSrcY, wDestXext, wDestYext, srcHDC, wSrcX, wSrcY, SRCCOPY);
				//	srcHDC = npdispwin.hdc16BltBuf;
				//}
				if (hasDrawMode && drawMode.bkMode == 4) { // TRANSPARENT1
					SetStretchBltMode(npdispwin.hdc, stretchMode);
					TransparentBlt(npdispwin.hdc, wDestX, wDestY, wDestXext, wDestYext, srcHDC, wSrcX, wSrcY, wSrcXext, wSrcYext, drawMode.LbkColor);
				}
				else if (isStretch) {
					SetStretchBltMode(npdispwin.hdc, stretchMode);
					StretchBlt(npdispwin.hdc, wDestX, wDestY, wDestXext, wDestYext, srcHDC, wSrcX, wSrcY, wSrcXext, wSrcYext, Rop3);
				}
				else {
					//if (wDestXext == 413 && wDestYext == 146) {
					//	int dstDevType = 0;
					//	if (lpDestDevAddr) {
					//		if (npdisp_readMemory(&dstDevType, lpDestDevAddr, 2)) {
					//			if (dstDevType == NPDISP_DEVTYPE) {
					//				NPDISP_PDEVICE pdev;
					//				npdisp_readMemory(&pdev, lpDestDevAddr, sizeof(NPDISP_PDEVICE));
					//				pdev.bmp.bmType = pdev.bmp.bmType;
					//			}
					//			else if (dstDevType == NPDISP_DEVTYPE_DDB) {
					//				NPDISP_PBITMAP_EXT pbmp;
					//				npdisp_readMemory(&pbmp, lpDestDevAddr, sizeof(NPDISP_PBITMAP_EXT));
					//				pbmp.bmType = pbmp.bmType;
					//			}
					//			else {
					//				NPDISP_PBITMAP pbmp;
					//				npdisp_readMemory(&pbmp, lpDestDevAddr, sizeof(NPDISP_PBITMAP));
					//				pbmp.bmType = pbmp.bmType;
					//			}
					//		}
					//	}
					//	BitBlt(npdispwin.hdc, wDestX, wDestY, wDestXext, wDestYext, srcHDC, wSrcX, wSrcY, SRCCOPY);
					//}
					BitBlt(npdispwin.hdc, wDestX, wDestY, wDestXext, wDestYext, srcHDC, wSrcX, wSrcY, Rop3);
					//if (srcPBmp.bmHeight <= 16 /* && srcPBmp.bmType == NPDISP_DEVTYPE_DDB */ ) {
					//	//HGDIOBJ oldbmp = GetCurrentObject(npdispwin.hdcCache[2], OBJ_BITMAP);
					//	//int ypos = 0;
					//	//for (auto it = npdispwin.bitmaps.begin(); it != npdispwin.bitmaps.end(); ++it) {
					//	//	SelectObject(npdispwin.hdcCache[2], it->second.bmphdc.hBmp);
					//	//	int h = it->second.bmphdc.lpbi->bmiHeader.biHeight >= 0 ? it->second.bmphdc.lpbi->bmiHeader.biHeight : -it->second.bmphdc.lpbi->bmiHeader.biHeight;
					//	//	if (h <= 256) {
					//	//		BitBlt(npdispwin.hdc, 0, ypos, it->second.bmphdc.lpbi->bmiHeader.biWidth, h, npdispwin.hdcCache[2], 0, 0, SRCCOPY);
					//	//		ypos += h;
					//	//	}
					//	//}
					//	//SelectObject(npdispwin.hdcCache[2], oldbmp);
					//	BitBlt(npdispwin.hdc, 0, 0, srcPBmp.bmWidth, srcPBmp.bmHeight, srcHDC, 0, 0, SRCCOPY);
					//}
					//if (srcPBmp.bmHeight == 6 && srcPBmp.bmWidth == 128 && srcPBmp.bmBitsPixel == 1) {
					//	BitBlt(npdispwin.hdc, 0, 0, srcPBmp.bmWidth, srcPBmp.bmHeight, srcHDC, 0, 0, SRCCOPY);
					//}
				}
				if (hRgn) SelectClipRgn(npdispwin.hdc, NULL);

				if (useActualColor) {
					// デバイス色をパレット番号（グレースケール）へ戻す
					RGBQUAD pal[256];
					for (int i = 0; i < 256; i++) {
						pal[i].rgbRed = npdisp_palette_gray256[i].r;
						pal[i].rgbGreen = npdisp_palette_gray256[i].g;
						pal[i].rgbBlue = npdisp_palette_gray256[i].b;
						pal[i].rgbReserved = 0;
					}
					SetDIBColorTable(npdispwin.hdc, 0, 256, pal);
				}

				//if (wDestXext == 20 && wDestYext == 18) {
				//	BitBlt(npdispwin.hdc, 0, 0, wDestXext, wDestYext, srcHDC, srcPBmp.bmWidth, srcPBmp.bmHeight, Rop3);
				//}

				SelectObject(npdispwin.hdc, npdispwin.hOldBrush);
				npdisp_setDirty(wDestX, wDestY, wDestX + (wDestXext < 0 ? -wDestXext : wDestXext), wDestY + (wDestYext < 0 ? -wDestYext : wDestYext));
				npdisp.updated = 1;

				npdisp_FreeBitmap(&bmphdc);
			}
		}
		else {
			retValue = 0xffff; //モノクロソースの時はCOLORONCOLOR以外だと致命的なのでGDIにやらせる
		}
	}
	else if (lpPBrushAddr) {
		NPDISP_BRUSH brush = { 0 };
		TRACEOUT_BITBLT(("-> BRUSH"));
		if (npdisp_readMemory(&brush, lpPBrushAddr, sizeof(NPDISP_BRUSH))) {
			if (brush.key != 0) {
				auto it = npdispwin.brushes.find(brush.key);
				if (it != npdispwin.brushes.end()) {
					NPDISP_HOSTBRUSH value = it->second;
					if (value.brs) {
						if (value.lbrush.lbColor == 0x0000ff) {
							value.lbrush.lbColor = 0x0000ff;
						}
						TRACEOUT_BITBLT(("-> style=%d, hatch=%d, color=%08x", value.lbrush.lbStyle, value.lbrush.lbHatch, value.lbrush.lbColor));
						NPDISP_DRAWMODE drawMode = { 0 };
						int hasDrawMode = npdisp_readMemory(&drawMode, lpDrawModeAddr, sizeof(NPDISP_DRAWMODE));
						SelectObject(npdispwin.hdc, value.brs);
						if (hasDrawMode) {
							if (npdispwin.hdc != npdispwin.hdc || memcmp(&drawMode, &npdispwin.lastScreenDrawMode, sizeof(NPDISP_DRAWMODE)) != 0) {
								if (npdispwin.hdc == npdispwin.hdc) npdispwin.lastScreenDrawMode = drawMode;
								npdisp_AdjustDrawModeColor(&drawMode);
								SetBkColor(npdispwin.hdc, drawMode.LbkColor);
								SetTextColor(npdispwin.hdc, drawMode.LTextColor);
								SetBkMode(npdispwin.hdc, drawMode.bkMode);
								SetROP2(npdispwin.hdc, drawMode.Rop2);
							}
						}
						else {
							SetBkColor(npdispwin.hdc, 0xffffff);
							SetTextColor(npdispwin.hdc, 0x000000);
						}
						if (brush.lbrush.lbStyle == NPDISP_BRUSH_STYLE_HATCHED) {
							SetBkColor(npdispwin.hdc, npdisp_AdjustColorRefForGDI(brush.lbrush.lbBkColor));
						}
						if (hRgn) SelectClipRgn(npdispwin.hdc, hRgn);
						if (hasDrawMode && drawMode.bkMode == 4) { // TRANSPARENT1
							SetStretchBltMode(npdispwin.hdc, COLORONCOLOR);
							TransparentBlt(npdispwin.hdc, wDestX, wDestY, wDestXext, wDestYext, npdispwin.hdc, wSrcX, wSrcY, wSrcXext, wSrcYext, drawMode.LbkColor);
						}
						else {
							PatBlt(npdispwin.hdc, wDestX, wDestY, wDestXext, wDestYext, Rop3);
						}
						//BitBlt(npdispwin.hdc, wDestX, wDestY, wDestXext, wDestYext, npdispwin.hdc, wDestX, wDestY, Rop3);
						if (hRgn) SelectClipRgn(npdispwin.hdc, NULL);
						npdisp_setDirty(wDestX, wDestY, wDestX + (wDestXext < 0 ? -wDestXext : wDestXext), wDestY + (wDestYext < 0 ? -wDestYext : wDestYext));
						npdisp.updated = 1;

						SelectObject(npdispwin.hdc, npdispwin.hOldBrush);
					}
					else {
						SelectObject(npdispwin.hdc, (HBRUSH)GetStockObject(NULL_BRUSH));
					}
				}
			}
		}
	}

	if (hRgn) {
		DeleteObject(hRgn);
	}

	return retValue;
}
UINT16 npdisp_func_BitBlt_MEMtoVRAM(int hasDstDev, int hasSrcDev, UINT32 lpDestDevAddr, SINT16 wDestX, SINT16 wDestY, UINT32 lpSrcDevAddr, SINT16 wSrcX, SINT16 wSrcY, UINT16 wXext, UINT16 wYext, UINT32 Rop3, UINT32 lpPBrushAddr, UINT32 lpDrawModeAddr)
{
	return npdisp_func_StretchBlt_MEMtoVRAM(hasDstDev, hasSrcDev, lpDestDevAddr, wDestX, wDestY, wXext, wYext, lpSrcDevAddr, wSrcX, wSrcY, wXext, wYext, Rop3, lpPBrushAddr, lpDrawModeAddr, 0);
}
UINT16 npdisp_func_StretchBlt_VRAMtoMEM(int hasDstDev, int hasSrcDev, UINT32 lpDestDevAddr, SINT16 wDestX, SINT16 wDestY, SINT16 wDestXext, SINT16 wDestYext, UINT32 lpSrcDevAddr, SINT16 wSrcX, SINT16 wSrcY, SINT16 wSrcXext, SINT16 wSrcYext, UINT32 Rop3, UINT32 lpPBrushAddr, UINT32 lpDrawModeAddr, UINT32 lpClipAddr)
{
	// VRAM -> MEM
	int stretchMode = COLORONCOLOR;

	// 実際に転送する範囲を計算
	int srcBeginLine = wSrcY;
	int srcNumLines = wSrcYext;
	int dstBeginLine = wDestY;
	int dstNumLines = wDestYext;
	if (srcBeginLine < 0) {
		srcNumLines += srcBeginLine;
		srcBeginLine = 0;
	}
	if (dstBeginLine < 0) {
		dstNumLines += dstBeginLine;
		dstBeginLine = 0;
	}
	int srcBeginX = wSrcX;
	int srcCopyWidth = wSrcXext;
	int dstBeginX = wDestX;
	int dstCopyWidth = wDestXext;
	if (srcBeginX < 0) {
		srcCopyWidth += srcBeginX;
		srcBeginX = 0;
	}
	if (dstBeginX < 0) {
		dstCopyWidth += dstBeginX;
		dstBeginX = 0;
	}

	bool isStretch = wDestXext != wSrcXext || wDestYext != wSrcYext;
	if (isStretch) {
		TRACEOUT_BITBLT(("Stretchlt VRAM -> MEM DEST X:%d Y:%d W:%d H:%d, rop:%08x", wDestX, wDestY, wDestXext, wDestYext, Rop3));
		if (npdisp.version >= 4 && npdisp.isWin9x) {
			if (lpDrawModeAddr) stretchMode = npdisp_readMemory16(lpDrawModeAddr + 36); // Win9x StretchBltModeを読む
		}
		else {
			if (npdisp.bpp == 1) return 0xffff; //モノクロソースの時はCOLORONCOLOR以外だと致命的なのでGDIにやらせる
		}
	}
	else {
		TRACEOUT_BITBLT(("BitBlt VRAM -> MEM DEST X:%d Y:%d W:%d H:%d, rop:%08x", wDestX, wDestY, wDestXext, wDestYext, Rop3));
	}
	HRGN hRgn = NULL;
	if (lpClipAddr) {
		RECT cliprect = { 0 };
		NPDISP_RECT rectTmp = { 0 };
		npdisp_readMemory(&rectTmp, lpClipAddr, sizeof(NPDISP_RECT));
		cliprect.top = rectTmp.top;
		cliprect.left = rectTmp.left;
		cliprect.bottom = rectTmp.bottom;
		cliprect.right = rectTmp.right;
		hRgn = CreateRectRgn(cliprect.left, cliprect.top, cliprect.right, cliprect.bottom);
	}
	NPDISP_PBITMAP_EXT dstPBmp;
	if (lpDestDevAddr && npdisp_readPBitmap(&dstPBmp, lpDestDevAddr)) {
		NPDISP_WINDOWS_BMPHDC bmphdc = { 0 };
		npdisp_PreloadBitmapFromPBITMAP(&dstPBmp, 0, dstBeginLine, dstNumLines, dstBeginX, dstCopyWidth);
		if (npdisp.longjmpnum == 0 && npdisp_MakeBitmapFromPBITMAP(&dstPBmp, &bmphdc, 0, dstBeginLine, dstNumLines, dstBeginX, dstCopyWidth)) {
			npdisp_ConvertToDDBMonoBitmap(&bmphdc);
			NPDISP_DRAWMODE drawMode = { 0 };
			int hasDrawMode = npdisp_readMemory(&drawMode, lpDrawModeAddr, sizeof(NPDISP_DRAWMODE));
			if (hasDrawMode) {
				if (bmphdc.hdc != npdispwin.hdc || memcmp(&drawMode, &npdispwin.lastScreenDrawMode, sizeof(NPDISP_DRAWMODE)) != 0) {
					if (bmphdc.hdc == npdispwin.hdc) npdispwin.lastScreenDrawMode = drawMode;
					npdisp_AdjustDrawModeColor(&drawMode);
					SetBkColor(bmphdc.hdc, drawMode.LbkColor);
					SetTextColor(bmphdc.hdc, drawMode.LTextColor);
					SetBkMode(bmphdc.hdc, drawMode.bkMode);
					SetROP2(bmphdc.hdc, drawMode.Rop2);
				}
				// ソースにもセットが必要
				SetBkColor(npdispwin.hdc, drawMode.LbkColor);
				SetTextColor(npdispwin.hdc, drawMode.LTextColor);
				SetBkMode(npdispwin.hdc, drawMode.bkMode);
				SetROP2(npdispwin.hdc, drawMode.Rop2);
			}
			else {
				SetBkColor(bmphdc.hdc, 0xffffff);
				SetTextColor(bmphdc.hdc, 0x000000);
				SetBkColor(npdispwin.hdc, 0xffffff);
				SetTextColor(npdispwin.hdc, 0x000000);
			}
			if (lpPBrushAddr) {
				// ブラシがあれば選択
				NPDISP_BRUSH brush = { 0 };
				if (npdisp_readMemory(&brush, lpPBrushAddr, sizeof(NPDISP_BRUSH))) {
					if (brush.key != 0) {
						auto it = npdispwin.brushes.find(brush.key);
						if (it != npdispwin.brushes.end()) {
							NPDISP_HOSTBRUSH value = it->second;
							if (value.brs) {
								TRACEOUT_BITBLT(("-> style=%d, hatch=%d, color=%08x", value.lbrush.lbStyle, value.lbrush.lbHatch, value.lbrush.lbColor));
								//if (npdisp.bpp == 1) {
								//	drawMode.LbkColor = drawMode.bkColor ? 0xffffff : 0;
								//	drawMode.LTextColor = drawMode.TextColor ? 0xffffff : 0;
								//	if (Rop3 == PATCOPY) {
								//		if (drawMode.LbkColor) {
								//			SelectObject(bmphdc.hdc, GetStockObject(WHITE_BRUSH));
								//		}
								//		else {
								//			SelectObject(bmphdc.hdc, GetStockObject(BLACK_BRUSH));
								//		}
								//	}
								//}
								SelectObject(bmphdc.hdc, value.brs);
								//if (brush.lbrush.lbStyle == NPDISP_BRUSH_STYLE_HATCHED) {
								//	SetBkColor(bmphdc.hdc, NPDISP_ADJUST_COLORREF(brush.lbrush.lbBkColor));
								//}
								//if (hasDrawMode) {
								//	SetBkColor(bmphdc.hdc, NPDISP_ADJUST_COLORREF(drawMode.LbkColor));
								//	SetTextColor(bmphdc.hdc, NPDISP_ADJUST_COLORREF(drawMode.LTextColor));
								//	if (brush.lbrush.lbStyle == NPDISP_BRUSH_STYLE_PATTERN) {
								//	}
								//	SetBkMode(bmphdc.hdc, drawMode.bkMode);
								//	SetROP2(bmphdc.hdc, drawMode.Rop2);
								//}
							}
							else {
								SelectObject(bmphdc.hdc, (HBRUSH)GetStockObject(NULL_BRUSH));
							}
						}
					}
				}
			}

			bool useActualColor = false;
			if (npdisp.bpp == 8 && npdisp.usePalette) {
				if ((dstPBmp.bmType == NPDISP_DEVTYPE_DIBENG || dstPBmp.bmType == NPDISP_DEVTYPE_DDB) && dstPBmp.bmBitsPixel != 8 && dstPBmp.bmBitsPixel != 1) {
					// パレット番号（グレースケール）から実際のデバイス色へ置き換え
					RGBQUAD pal[256];
					for (int i = 0; i < 256; i++) {
						pal[i].rgbRed = npdisp_palette_rgb256[i].r;
						pal[i].rgbGreen = npdisp_palette_rgb256[i].g;
						pal[i].rgbBlue = npdisp_palette_rgb256[i].b;
						pal[i].rgbReserved = 0;
					}
					SetDIBColorTable(npdispwin.hdc, 0, 256, pal);
					useActualColor = true;
				}
			}

			if (hRgn) SelectClipRgn(bmphdc.hdc, hRgn);
			HDC srcHDC = npdispwin.hdc;
			//if ((npdisp.bpp == 16 || npdisp.bpp == 15) && bmphdc.lpbi->bmiHeader.biBitCount == 1 && npdispwin.hdc16BltBuf) {
			//	BitBlt(npdispwin.hdc16BltBuf, wSrcX, wSrcY, wDestXext, wDestYext, srcHDC, wSrcX, wSrcY, SRCCOPY);
			//	srcHDC = npdispwin.hdc16BltBuf;
			//}
			if (hasDrawMode && drawMode.bkMode == 4) { // TRANSPARENT1
				SetStretchBltMode(npdispwin.hdc, stretchMode);
				TransparentBlt(bmphdc.hdc, wDestX, wDestY, wDestXext, wDestYext, srcHDC, wSrcX, wSrcY, wSrcXext, wSrcYext, drawMode.LbkColor);
			}
			else if (isStretch) {
				SetStretchBltMode(bmphdc.hdc, stretchMode);
				StretchBlt(bmphdc.hdc, wDestX, wDestY, wDestXext, wDestYext, srcHDC, wSrcX, wSrcY, wSrcXext, wSrcYext, Rop3);
			}
			else {
				BitBlt(bmphdc.hdc, wDestX, wDestY, wDestXext, wDestYext, srcHDC, wSrcX, wSrcY, Rop3);
			}
			if (hRgn) SelectClipRgn(bmphdc.hdc, NULL);

			if (useActualColor) {
				// デバイス色をパレット番号（グレースケール）へ戻す
				RGBQUAD pal[256];
				for (int i = 0; i < 256; i++) {
					pal[i].rgbRed = npdisp_palette_gray256[i].r;
					pal[i].rgbGreen = npdisp_palette_gray256[i].g;
					pal[i].rgbBlue = npdisp_palette_gray256[i].b;
					pal[i].rgbReserved = 0;
				}
				SetDIBColorTable(npdispwin.hdc, 0, 256, pal);
			}

			SelectObject(npdispwin.hdc, npdispwin.hOldBrush);

			npdisp_WriteBitmapToPBITMAP(&dstPBmp, &bmphdc, dstBeginLine, dstNumLines, dstBeginX, dstCopyWidth);

			npdisp_FreeBitmap(&bmphdc);
		}
	}

	if (hRgn) {
		DeleteObject(hRgn);
	}

	return 1;
}
UINT16 npdisp_func_BitBlt_VRAMtoMEM(int hasDstDev, int hasSrcDev, UINT32 lpDestDevAddr, SINT16 wDestX, SINT16 wDestY, UINT32 lpSrcDevAddr, SINT16 wSrcX, SINT16 wSrcY, UINT16 wXext, UINT16 wYext, UINT32 Rop3, UINT32 lpPBrushAddr, UINT32 lpDrawModeAddr)
{
	return npdisp_func_StretchBlt_VRAMtoMEM(hasDstDev, hasSrcDev, lpDestDevAddr, wDestX, wDestY, wXext, wYext, lpSrcDevAddr, wSrcX, wSrcY, wXext, wYext, Rop3, lpPBrushAddr, lpDrawModeAddr, 0);
}
UINT16 npdisp_func_StretchBlt_MEMtoMEM(int hasDstDev, int hasSrcDev, UINT32 lpDestDevAddr, SINT16 wDestX, SINT16 wDestY, SINT16 wDestXext, SINT16 wDestYext, UINT32 lpSrcDevAddr, SINT16 wSrcX, SINT16 wSrcY, SINT16 wSrcXext, SINT16 wSrcYext, UINT32 Rop3, UINT32 lpPBrushAddr, UINT32 lpDrawModeAddr, UINT32 lpClipAddr)
{
	// MEM -> MEM
	int stretchMode = COLORONCOLOR;
	UINT16 retValue = 0;

	// 実際に転送する範囲を計算
	int srcBeginLine = wSrcY;
	int srcNumLines = wSrcYext;
	int dstBeginLine = wDestY;
	int dstNumLines = wDestYext;
	if (srcBeginLine < 0) {
		srcNumLines += srcBeginLine;
		srcBeginLine = 0;
	}
	if (dstBeginLine < 0) {
		dstNumLines += dstBeginLine;
		dstBeginLine = 0;
	}
	int srcBeginX = wSrcX;
	int srcCopyWidth = wSrcXext;
	int dstBeginX = wDestX;
	int dstCopyWidth = wDestXext;
	if (srcBeginX < 0) {
		srcCopyWidth += srcBeginX;
		srcBeginX = 0;
	}
	if (dstBeginX < 0) {
		dstCopyWidth += dstBeginX;
		dstBeginX = 0;
	}

	bool isStretch = wDestXext != wSrcXext || wDestYext != wSrcYext;
	if (isStretch) {
		TRACEOUT_BITBLT(("Stretchlt MEM -> MEM DEST X:%d Y:%d W:%d H:%d, rop:%08x", wDestX, wDestY, wDestXext, wDestYext, Rop3));
		if (npdisp.version >= 4 && npdisp.isWin9x) {
			if (lpDrawModeAddr) stretchMode = npdisp_readMemory16(lpDrawModeAddr + 36); // Win9x StretchBltModeを読む
		}
	}
	else {
		TRACEOUT_BITBLT(("BitBlt MEM -> MEM DEST X:%d Y:%d W:%d H:%d, rop:%08x", wDestX, wDestY, wDestXext, wDestYext, Rop3));
	}
	HRGN hRgn = NULL;
	if (lpClipAddr) {
		RECT cliprect = { 0 };
		NPDISP_RECT rectTmp = { 0 };
		npdisp_readMemory(&rectTmp, lpClipAddr, sizeof(NPDISP_RECT));
		cliprect.top = rectTmp.top;
		cliprect.left = rectTmp.left;
		cliprect.bottom = rectTmp.bottom;
		cliprect.right = rectTmp.right;
		hRgn = CreateRectRgn(cliprect.left, cliprect.top, cliprect.right, cliprect.bottom);
	}
	NPDISP_PBITMAP_EXT dstPBmp;
	retValue = 1;
	if (lpDestDevAddr && npdisp_readPBitmap(&dstPBmp, lpDestDevAddr)) {
		if (lpSrcDevAddr) {
			NPDISP_PBITMAP_EXT srcPBmp;
			if (npdisp_readPBitmap(&srcPBmp, lpSrcDevAddr)) {
				if (!isStretch || srcPBmp.bmBitsPixel != 1 || npdisp.isWin9x) {
					npdisp_PreloadBitmapFromPBITMAP(&srcPBmp, 0, srcBeginLine, srcNumLines, srcBeginX, srcCopyWidth);
					npdisp_PreloadBitmapFromPBITMAP(&dstPBmp, 1, dstBeginLine, dstNumLines, dstBeginX, dstCopyWidth);
					NPDISP_WINDOWS_BMPHDC srcbmphdc = { 0 };
					if (npdisp.longjmpnum == 0 && npdisp_MakeBitmapFromPBITMAP(&srcPBmp, &srcbmphdc, 0, srcBeginLine, srcNumLines, srcBeginX, srcCopyWidth)) {
						NPDISP_WINDOWS_BMPHDC dstbmphdc = { 0 };
						if (npdisp_MakeBitmapFromPBITMAP(&dstPBmp, &dstbmphdc, 1, dstBeginLine, dstNumLines, dstBeginX, dstCopyWidth)) {
							npdisp_ConvertToDDBMonoBitmap(&srcbmphdc);
							npdisp_ConvertToDDBMonoBitmap(&dstbmphdc);
							HDC srcHDC = srcbmphdc.hdc;
							NPDISP_DRAWMODE drawMode = { 0 };
							int hasDrawMode = npdisp_readMemory(&drawMode, lpDrawModeAddr, sizeof(NPDISP_DRAWMODE));
							if (hasDrawMode) {
								if (dstbmphdc.hdc != npdispwin.hdc || memcmp(&drawMode, &npdispwin.lastScreenDrawMode, sizeof(NPDISP_DRAWMODE)) != 0) {
									if (dstbmphdc.hdc == npdispwin.hdc) npdispwin.lastScreenDrawMode = drawMode;
									npdisp_AdjustDrawModeColor(&drawMode);
									SetBkColor(dstbmphdc.hdc, drawMode.LbkColor);
									SetTextColor(dstbmphdc.hdc, drawMode.LTextColor);
									SetBkMode(dstbmphdc.hdc, drawMode.bkMode);
									SetROP2(dstbmphdc.hdc, drawMode.Rop2);
								}
								// ソースにもセットが必要
								SetBkColor(srcHDC, drawMode.LbkColor);
								SetTextColor(srcHDC, drawMode.LTextColor);
								SetBkMode(srcHDC, drawMode.bkMode);
								SetROP2(srcHDC, drawMode.Rop2);
								//npdisp_AdjustSrcMonoPaletteByDrawMode(&srcbmphdc, &dstbmphdc, &drawMode);
							}
							else {
								SetBkColor(dstbmphdc.hdc, 0xffffff);
								SetTextColor(dstbmphdc.hdc, 0x000000);
								SetBkColor(srcHDC, 0xffffff);
								SetTextColor(srcHDC, 0x000000);
							}
							if (lpPBrushAddr) {
								// ブラシがあれば選択
								NPDISP_BRUSH brush = { 0 };
								if (npdisp_readMemory(&brush, lpPBrushAddr, sizeof(NPDISP_BRUSH))) {
									if (brush.key != 0) {
										auto it = npdispwin.brushes.find(brush.key);
										if (it != npdispwin.brushes.end()) {
											NPDISP_HOSTBRUSH value = it->second;
											if (value.brs) {
												TRACEOUT_BITBLT(("-> style=%d, hatch=%d, color=%08x", value.lbrush.lbStyle, value.lbrush.lbHatch, value.lbrush.lbColor));
												//if (npdisp.bpp == 1) {
												//	drawMode.LbkColor = 0xffffff;// drawMode.bkColor ? 0xffffff : 0;
												//	drawMode.LTextColor = 0;// drawMode.TextColor ? 0xffffff : 0;
												//	if (Rop3 == PATCOPY) {
												//		if (drawMode.LbkColor) {
												//			SelectObject(dstbmphdc.hdc, GetStockObject(WHITE_BRUSH));
												//		}
												//		else {
												//			SelectObject(dstbmphdc.hdc, GetStockObject(BLACK_BRUSH));
												//		}
												//	}
												//}
												SelectObject(dstbmphdc.hdc, value.brs);
												//if (dstbmphdc.lpbi->bmiHeader.biBitCount == 1 && srcbmphdc.lpbi->bmiHeader.biBitCount > 8) {
												//	SetTextColor(dstbmphdc.hdc, 0xffffff);
												//}
												//if (brush.lbrush.lbStyle == NPDISP_BRUSH_STYLE_HATCHED) {
												//	SetBkColor(dstbmphdc.hdc, NPDISP_ADJUST_COLORREF(brush.lbrush.lbBkColor));
												//}
												//if (hasDrawMode) {
												//	SetBkColor(dstbmphdc.hdc, NPDISP_ADJUST_COLORREF(drawMode.LbkColor));
												//	SetTextColor(dstbmphdc.hdc, NPDISP_ADJUST_COLORREF(drawMode.LTextColor));
												//	if (brush.lbrush.lbStyle == NPDISP_BRUSH_STYLE_PATTERN) {
												//	}
												//	SetBkMode(dstbmphdc.hdc, drawMode.bkMode);
												//	SetROP2(dstbmphdc.hdc, drawMode.Rop2);
												//}
											}
											else {
												SelectObject(dstbmphdc.hdc, (HBRUSH)GetStockObject(NULL_BRUSH));
											}
										}
									}
								}
							}

							bool useActualColorSrc = false;
							bool useActualColorDst = false;
							if (npdisp.bpp == 8 && npdisp.usePalette) {
								if (((dstPBmp.bmType == NPDISP_DEVTYPE_DIBENG || dstPBmp.bmType == NPDISP_DEVTYPE_DDB) && dstPBmp.bmBitsPixel != 8 && dstPBmp.bmBitsPixel != 1) && srcPBmp.bmBitsPixel == 8) {
									// パレット番号（グレースケール）から実際のデバイス色へ置き換え
									RGBQUAD pal[256];
									for (int i = 0; i < 256; i++) {
										pal[i].rgbRed = npdisp_palette_rgb256[i].r;
										pal[i].rgbGreen = npdisp_palette_rgb256[i].g;
										pal[i].rgbBlue = npdisp_palette_rgb256[i].b;
										pal[i].rgbReserved = 0;
									}
									SetDIBColorTable(srcbmphdc.hdc, 0, 256, pal);
									useActualColorSrc = true;
								}
								else if (((srcPBmp.bmType == NPDISP_DEVTYPE_DIBENG || srcPBmp.bmType == NPDISP_DEVTYPE_DDB) && srcPBmp.bmBitsPixel != 8 && srcPBmp.bmBitsPixel != 1) && dstPBmp.bmBitsPixel == 8) {
									// パレット番号（グレースケール）から実際のデバイス色へ置き換え
									RGBQUAD pal[256];
									for (int i = 0; i < 256; i++) {
										pal[i].rgbRed = npdisp_palette_rgb256[i].r;
										pal[i].rgbGreen = npdisp_palette_rgb256[i].g;
										pal[i].rgbBlue = npdisp_palette_rgb256[i].b;
										pal[i].rgbReserved = 0;
									}
									SetDIBColorTable(dstbmphdc.hdc, 0, 256, pal);
									useActualColorDst = true;
								}
							}

							if (hRgn) SelectClipRgn(dstbmphdc.hdc, hRgn);
							if (hasDrawMode && drawMode.bkMode == 4) { // TRANSPARENT1
								SetStretchBltMode(npdispwin.hdc, stretchMode);
								TransparentBlt(dstbmphdc.hdc, wDestX, wDestY, wDestXext, wDestYext, srcHDC, wSrcX, wSrcY, wSrcXext, wSrcYext, drawMode.LbkColor);
							}
							else if (isStretch) {
								SetStretchBltMode(dstbmphdc.hdc, stretchMode);
								StretchBlt(dstbmphdc.hdc, wDestX, wDestY, wDestXext, wDestYext, srcHDC, wSrcX, wSrcY, wSrcXext, wSrcYext, Rop3);
							}
							else {
#ifdef IMAGEDEBUG
								if (wDestXext == IMAGEDEBUG_SIZE && wDestYext == IMAGEDEBUG_SIZE) {
									static int yyyy = 0;
									BitBlt(npdispwin.hdc, IMAGEDEBUG_X +0, yyyy, wDestXext, wDestYext, dstbmphdc.hdc, wDestX, wDestY, SRCCOPY);
									BitBlt(npdispwin.hdc, IMAGEDEBUG_X + IMAGEDEBUG_SIZE, yyyy, wDestXext, wDestYext, srcHDC, wSrcX, wSrcY, SRCCOPY);
									BitBlt(dstbmphdc.hdc, wDestX, wDestY, wDestXext, wDestYext, srcHDC, wSrcX, wSrcY, Rop3);
									SetBkColor(npdispwin.hdc, 0xffffff);
									SetTextColor(npdispwin.hdc, 0x000000);
									BitBlt(npdispwin.hdc, IMAGEDEBUG_X + IMAGEDEBUG_SIZE * 2, yyyy, wDestXext, wDestYext, dstbmphdc.hdc, wDestX, wDestY, SRCCOPY);

									yyyy += IMAGEDEBUG_SIZE;
									if (yyyy > npdisp.height) yyyy = 0;
								}
								else
#endif
								{
									BitBlt(dstbmphdc.hdc, wDestX, wDestY, wDestXext, wDestYext, srcHDC, wSrcX, wSrcY, Rop3);
								}
							}
							if (hRgn) SelectClipRgn(dstbmphdc.hdc, NULL);

							if (useActualColorSrc) {
								// デバイス色をパレット番号（グレースケール）へ戻す
								RGBQUAD pal[256];
								for (int i = 0; i < 256; i++) {
									pal[i].rgbRed = npdisp_palette_gray256[i].r;
									pal[i].rgbGreen = npdisp_palette_gray256[i].g;
									pal[i].rgbBlue = npdisp_palette_gray256[i].b;
									pal[i].rgbReserved = 0;
								}
								SetDIBColorTable(srcbmphdc.hdc, 0, 256, pal);
							}
							else if (useActualColorDst) {
								// デバイス色をパレット番号（グレースケール）へ戻す
								RGBQUAD pal[256];
								for (int i = 0; i < 256; i++) {
									pal[i].rgbRed = npdisp_palette_gray256[i].r;
									pal[i].rgbGreen = npdisp_palette_gray256[i].g;
									pal[i].rgbBlue = npdisp_palette_gray256[i].b;
									pal[i].rgbReserved = 0;
								}
								SetDIBColorTable(dstbmphdc.hdc, 0, 256, pal);
							}

							SelectObject(npdispwin.hdc, npdispwin.hOldBrush);
							retValue = 1; // 成功

							npdisp_WriteBitmapToPBITMAP(&dstPBmp, &dstbmphdc, dstBeginLine, dstNumLines, dstBeginX, dstCopyWidth);

							npdisp_FreeBitmap(&dstbmphdc);
						}
						npdisp_FreeBitmap(&srcbmphdc);
					}
				}
				else {
					retValue = 0xffff; //モノクロソースの時はCOLORONCOLOR以外だと致命的なのでGDIにやらせる
				}
			}
		}
		else if (lpPBrushAddr)
		{
			TRACEOUT_BITBLT(("-> BRUSH"));
			NPDISP_BRUSH brush = { 0 };
			if (npdisp_readMemory(&brush, lpPBrushAddr, sizeof(NPDISP_BRUSH))) {
				if (brush.key != 0) {
					auto it = npdispwin.brushes.find(brush.key);
					if (it != npdispwin.brushes.end()) {
						NPDISP_HOSTBRUSH value = it->second;
						if (value.brs) {
							NPDISP_WINDOWS_BMPHDC dstbmphdc = { 0 };
							npdisp_PreloadBitmapFromPBITMAP(&dstPBmp, 0, dstBeginLine, dstNumLines, dstBeginX, dstCopyWidth);
							if (npdisp.longjmpnum == 0 && npdisp_MakeBitmapFromPBITMAP(&dstPBmp, &dstbmphdc, 0, dstBeginLine, dstNumLines, dstBeginX, dstCopyWidth)) {
								TRACEOUT_BITBLT(("-> style=%d, hatch=%d, color=%08x", value.lbrush.lbStyle, value.lbrush.lbHatch, value.lbrush.lbColor));
								NPDISP_DRAWMODE drawMode = { 0 };
								int hasDrawMode = npdisp_readMemory(&drawMode, lpDrawModeAddr, sizeof(NPDISP_DRAWMODE));
								if (hasDrawMode) {
									if (dstbmphdc.hdc != npdispwin.hdc || memcmp(&drawMode, &npdispwin.lastScreenDrawMode, sizeof(NPDISP_DRAWMODE)) != 0) {
										if (dstbmphdc.hdc == npdispwin.hdc) npdispwin.lastScreenDrawMode = drawMode;
										npdisp_AdjustDrawModeColor(&drawMode);
										SetBkColor(dstbmphdc.hdc, drawMode.LbkColor);
										SetTextColor(dstbmphdc.hdc, drawMode.LTextColor);
										SetBkMode(dstbmphdc.hdc, drawMode.bkMode);
										SetROP2(dstbmphdc.hdc, drawMode.Rop2);
									}
								}
								else {
									SetBkColor(dstbmphdc.hdc, 0xffffff);
									SetTextColor(dstbmphdc.hdc, 0x000000);
								}
								if (brush.lbrush.lbStyle == NPDISP_BRUSH_STYLE_HATCHED) {
									SetBkColor(dstbmphdc.hdc, npdisp_AdjustColorRefForGDI(brush.lbrush.lbBkColor));
								}

								//if (npdisp.bpp == 1) {
								//	drawMode.LbkColor = drawMode.bkColor ? 0xffffff : 0;
								//	drawMode.LTextColor = drawMode.TextColor ? 0xffffff : 0;
								//	if (Rop3 == PATCOPY) {
								//		if (drawMode.LbkColor) {
								//			SelectObject(dstbmphdc.hdc, GetStockObject(WHITE_BRUSH));
								//		}
								//		else {
								//			SelectObject(dstbmphdc.hdc, GetStockObject(BLACK_BRUSH));
								//		}
								//	}
								//}

								//if (brush.lbrush.lbStyle == NPDISP_BRUSH_STYLE_HATCHED) {
								//	SetBkColor(dstbmphdc.hdc, NPDISP_ADJUST_COLORREF(brush.lbrush.lbBkColor));
								//}
								//if (hasDrawMode) {
								//	SetBkColor(dstbmphdc.hdc, NPDISP_ADJUST_COLORREF(drawMode.LbkColor));
								//	SetTextColor(dstbmphdc.hdc, NPDISP_ADJUST_COLORREF(drawMode.LTextColor));
								//	if (brush.lbrush.lbStyle == NPDISP_BRUSH_STYLE_PATTERN) {

								//	}
								//	SetBkMode(dstbmphdc.hdc, drawMode.bkMode);
								//	SetROP2(dstbmphdc.hdc, drawMode.Rop2);
								//}
								HBRUSH oldBrush = (HBRUSH)SelectObject(dstbmphdc.hdc, value.brs);
								//TRACEOUT2(("BitBlt MEM -> MEM DEST X:%d Y:%d W:%d H:%d", wDestX, wDestY, wDestXext, wDestYext));
								//TRACEOUT2(("  DEST:%d", lpDestDevAddr));
								//TRACEOUT2(("  -> style=%d, hatch=%d, color=%08x, rop=%08x", value.lbrush.lbStyle, value.lbrush.lbHatch, value.lbrush.lbColor, Rop3));
								//if (lpDestDevAddr == 701956096) {
								//	TRACEOUT2(("  CHECK:%d", lpDestDevAddr));
								//}
								if (hasDrawMode && drawMode.bkMode == 4) { // TRANSPARENT1
									SetStretchBltMode(npdispwin.hdc, COLORONCOLOR);
									TransparentBlt(dstbmphdc.hdc, wDestX, wDestY, wDestXext, wDestYext, dstbmphdc.hdc, wSrcX, wSrcY, wSrcXext, wSrcYext, drawMode.LbkColor);
								}
								else {
									PatBlt(dstbmphdc.hdc, wDestX, wDestY, wDestXext, wDestYext, Rop3);
								}
								if (hRgn) SelectClipRgn(dstbmphdc.hdc, NULL);
								SelectObject(dstbmphdc.hdc, oldBrush);

								npdisp_WriteBitmapToPBITMAP(&dstPBmp, &dstbmphdc, dstBeginLine, dstNumLines, dstBeginX, dstCopyWidth);

								npdisp_FreeBitmap(&dstbmphdc);
							}
						}
					}
				}
			}
		}
	}

	if (hRgn) {
		DeleteObject(hRgn);
	}

	return retValue;
}
UINT16 npdisp_func_BitBlt_MEMtoMEM(int hasDstDev, int hasSrcDev, UINT32 lpDestDevAddr, SINT16 wDestX, SINT16 wDestY, UINT32 lpSrcDevAddr, SINT16 wSrcX, SINT16 wSrcY, UINT16 wXext, UINT16 wYext, UINT32 Rop3, UINT32 lpPBrushAddr, UINT32 lpDrawModeAddr)
{
	return npdisp_func_StretchBlt_MEMtoMEM(hasDstDev, hasSrcDev, lpDestDevAddr, wDestX, wDestY, wXext, wYext, lpSrcDevAddr, wSrcX, wSrcY, wXext, wYext, Rop3, lpPBrushAddr, lpDrawModeAddr, 0);
}

#endif