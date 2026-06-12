/**
 * @file	npdisp.c
 * @brief	Implementation of the Neko Project II Display Adapter
 */

#include	"compiler.h"

#if defined(SUPPORT_WAB_NPDISP)

#include	<map>
#include	<vector>
#include	<unordered_set>

#include	"pccore.h"
#include	"wab.h"
#include	"statsave.h"
#include	"dosio.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"soundmng.h"

#if defined(SUPPORT_IA32_HAXM)
#include "i386hax/haxfunc.h"
#include "i386hax/haxcore.h"
#endif

#include	"npdispdef.h"
#include	"npdisp.h"
#include	"npdisp_statsave.h"
#include	"npdisp_rle.h"
#include	"npdisp_mem.h"
#include	"npdisp_palette.h"
#include	"npdisp_gdioutput.h"
#include	"npdisp_gdibitblt.h"

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
#endif
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
static void trace_fmt_exF(const char* fmt, ...)
{
	char stmp[2048];
	va_list ap;
	va_start(ap, fmt);
	vsprintf(stmp, fmt, ap);
	strcat(stmp, "\n");
	va_end(ap);
	OutputDebugStringA(stmp);
}
#define	TRACEOUTF(s)	trace_fmt_exF s
#else
#define	TRACEOUTF(s)	(void)s
#endif	/* 1 */
#if 0
static void trace_fmt_exF(const char* fmt, ...)
{
	char stmp[2048];
	va_list ap;
	va_start(ap, fmt);
	vsprintf(stmp, fmt, ap);
	strcat(stmp, "\n");
	va_end(ap);
	OutputDebugStringA(stmp);
}
#define	TRACEOUTP(s)	trace_fmt_exF s
#else
#define	TRACEOUTP(s)	(void)s
#endif	/* 1 */
#if 0
static void trace_fmt_exF(const char* fmt, ...)
{
	char stmp[2048];
	va_list ap;
	va_start(ap, fmt);
	vsprintf(stmp, fmt, ap);
	strcat(stmp, "\n");
	va_end(ap);
	OutputDebugStringA(stmp);
}
#define	TRACEOUT9(s)	trace_fmt_exF s
#else
#define	TRACEOUT9(s)	(void)s
#endif	/* 1 */
#if 0
static void trace_fmt_ex10(const char* fmt, ...)
{
	char stmp[2048];
	va_list ap;
	va_start(ap, fmt);
	vsprintf(stmp, fmt, ap);
	strcat(stmp, "\n");
	va_end(ap);
	OutputDebugStringA(stmp);
}
#define	TRACEOUT10(s)	trace_fmt_ex10 s
#else
#define	TRACEOUT10(s)	(void)s
#endif	/* 1 */
#if 0
static void trace_fmt_exF(const char* fmt, ...)
{
	char stmp[2048];
	va_list ap;
	va_start(ap, fmt);
	vsprintf(stmp, fmt, ap);
	strcat(stmp, "\n");
	va_end(ap);
	OutputDebugStringA(stmp);
}
#define	TRACEOUTSDIB(s)	trace_fmt_exF s
#else
#define	TRACEOUTSDIB(s)	(void)s
#endif	/* 1 */
#if 0
static void trace_fmt_exF(const char* fmt, ...)
{
	char stmp[2048];
	va_list ap;
	va_start(ap, fmt);
	vsprintf(stmp, fmt, ap);
	strcat(stmp, "\n");
	va_end(ap);
	OutputDebugStringA(stmp);
}
#define	TRACEOUT11(s)	trace_fmt_exF s
#else
#define	TRACEOUT11(s)	(void)s
#endif	/* 1 */

static void npdisp_releaseScreen(bool resize = false);
static void npdisp_createScreen(bool resize = false);

NPDISP npdisp = { 0 };
NPDISP_WINDOWS npdispwin = { 0 };

void npdisp_setDirty(int x1, int y1, int x2, int y2)
{
	if (x1 == x2 || y1 == y2) {
		return;
	}
	if (npdispwin.dirtyRect.left == npdispwin.dirtyRect.right || npdispwin.dirtyRect.top == npdispwin.dirtyRect.bottom) {
		npdispwin.dirtyRect.left = x1;
		npdispwin.dirtyRect.top = y1;
		npdispwin.dirtyRect.right = x2;
		npdispwin.dirtyRect.bottom = y2;
	}
	else {
		if (npdispwin.dirtyRect.left > x1) npdispwin.dirtyRect.left = x1;
		if (npdispwin.dirtyRect.top > y1) npdispwin.dirtyRect.top = y1;
		if (npdispwin.dirtyRect.right < x2) npdispwin.dirtyRect.right = x2;
		if (npdispwin.dirtyRect.bottom < y2) npdispwin.dirtyRect.bottom = y2;
	}
}
void npdisp_setDirtyAll()
{
	npdispwin.dirtyRect.left = 0;
	npdispwin.dirtyRect.top = 0;
	npdispwin.dirtyRect.right = npdisp.width;
	npdispwin.dirtyRect.bottom = npdisp.height;
}
void npdisp_resetDirty()
{
	npdispwin.dirtyRect.left = npdispwin.dirtyRect.top = npdispwin.dirtyRect.right = npdispwin.dirtyRect.bottom = 0;
}

// *** 排他制御用 *****************

static int npdisp_cs_initialized = 0;
static CRITICAL_SECTION npdisp_cs;
//static int npdisp_cs_execflag = 0;

void npdispcs_enter_criticalsection(void)
{
	if (!npdisp_cs_initialized) return;
	EnterCriticalSection(&npdisp_cs);
}
BOOL npdispcs_tryenter_criticalsection(void)
{
	if (!npdisp_cs_initialized) return FALSE;
	return TryEnterCriticalSection(&npdisp_cs);
}
void npdispcs_leave_criticalsection(void)
{
	if (!npdisp_cs_initialized) return;
	LeaveCriticalSection(&npdisp_cs);
}

void npdispcs_initialize(void)
{
	/* クリティカルセクション準備 */
	if (!npdisp_cs_initialized)
	{
		memset(&npdisp_cs, 0, sizeof(npdisp_cs));
		InitializeCriticalSection(&npdisp_cs);
		npdisp_cs_initialized = 1;
	}
}
void npdispcs_shutdown(void)
{
	/* クリティカルセクション破棄 */
	if (npdisp_cs_initialized)
	{
		DeleteCriticalSection(&npdisp_cs);
		memset(&npdisp_cs, 0, sizeof(npdisp_cs));
		npdisp_cs_initialized = 0;
	}
}

// *** エクスポート関数処理 *****************

static void npdisp_func_NP2Initialize(UINT16 dpiX, UINT16 dpiY, UINT16 width, UINT16 height, UINT16 bpp, UINT8 isWin9x, UINT32 bmpinfoAddr, UINT32 beginAccessAddr, UINT32 endAccessAddr, UINT32 dcibufAddr, UINT32 dciBeginAccessAddr, UINT32 dciEndAccessAddr, UINT32 dciDestroySurfaceAddr, UINT32 vramLinearAddr, UINT32 vramPhysicalAddr)
{
	bool resize = npdisp.enabled && npdisp.active;

	if (!resize) {
		// 初期化
		npdisp.enabled = 0;
		npdisp.active = 0;
		np2wab.relaystateext = 0;
	}
	np2wab_setRelayState(np2wab.relaystateint | np2wab.relaystateext);
	npdisp_releaseScreen(resize);

	if (width) npdisp.width = width;
	if (height) npdisp.height = height;
	if (npdisp.width > WAB_MAX_WIDTH) npdisp.width = WAB_MAX_WIDTH;
	if (npdisp.height > WAB_MAX_HEIGHT) npdisp.height = WAB_MAX_HEIGHT;
	if (npdisp.width < 160) npdisp.width = 160;
	if (npdisp.height < 100) npdisp.height = 100;
	if (bpp) npdisp.bpp = bpp;
	if (npdisp.bpp <= 1) npdisp.bpp = 1;
	else if (npdisp.bpp <= 4) npdisp.bpp = 4;
	else if (npdisp.bpp <= 8) npdisp.bpp = 8;
	else if (npdisp.bpp <= 15) npdisp.bpp = 15;
	else if (npdisp.bpp <= 16) npdisp.bpp = 16;
	else if (npdisp.bpp <= 24) npdisp.bpp = 24;
	else if (npdisp.bpp <= 32) npdisp.bpp = 32;
	if (dpiX) npdisp.dpiX = dpiX;
	if (dpiY) npdisp.dpiY = dpiY;

	npdisp.usePalette = (npdisp.bpp == 8);

	if (npdisp.version >= 4) {
		npdisp.isWin9x = isWin9x;
	}
	else {
		npdisp.isWin9x = 0;
	}

	if (npdisp.version >= 5) {
		npdisp.mm_vramPhysicalAddr = vramPhysicalAddr;
		npdisp.mm_bmpinfoAddr = bmpinfoAddr;
		npdisp.mm_beginAccessAddr = beginAccessAddr;
		npdisp.mm_endAccessAddr = endAccessAddr;
		npdisp.mm_dcibufAddr = dcibufAddr;
		npdisp.mm_dciBeginAccessAddr = dciBeginAccessAddr;
		npdisp.mm_dciEndAccessAddr = dciEndAccessAddr;
		npdisp.mm_dciDestroySurfaceAddr = dciDestroySurfaceAddr;
		npdisp.mm_vramLinearAddr = vramLinearAddr;
		if (!resize) {
			npdisp.mm_dciEnable = 0;
		}
	}
	else {
		npdisp.mm_vramPhysicalAddr = 0;
		npdisp.mm_bmpinfoAddr = 0;
		npdisp.mm_beginAccessAddr = 0;
		npdisp.mm_endAccessAddr = 0;
		npdisp.mm_dcibufAddr = 0;
		npdisp.mm_dciBeginAccessAddr = 0;
		npdisp.mm_dciEndAccessAddr = 0;
		npdisp.mm_dciDestroySurfaceAddr = 0;
		npdisp.mm_vramLinearAddr = 0;
		npdisp.mm_dciEnable = 0;
	}
	
	// バージョンを返す
	npdisp_writeMemory16(npdisp.version, npdisp.dataAddr);
}

static UINT16 npdisp_func_Enable_PDEVICE(NPDISP_PDEVICE *lpDevInfo, UINT16 wStyle, const char* lpDestDevType, const char* lpOutputFile, const NPDISP_DEVMODE* lpData) 
{
	memset(lpDevInfo, 0, sizeof(NPDISP_PDEVICE));

	//lpDevInfo->bmp.bmType = NPDISP_DEVTYPE;
	//lpDevInfo->bmp.bmWidth = npdisp.width;
	//lpDevInfo->bmp.bmHeight = npdisp.height;
	//lpDevInfo->bmp.bmBitsPixel = 1;
	//lpDevInfo->bmp.bmPlanes = 4;

	// DIB Engine互換  DirectDrawはDIB Engine互換を要求する。識別子は0x5250でないとNG。
	lpDevInfo->dibe.deType = NPDISP_DEVTYPE;
	lpDevInfo->dibe.deWidth = npdisp.width;
	lpDevInfo->dibe.deHeight = npdisp.height;
	lpDevInfo->dibe.deWidthBytes = ((npdisp.width * npdisp.bpp + 31) / 32) * 4;
	lpDevInfo->dibe.dePlanes = 1;
	lpDevInfo->dibe.deBitsPixel = npdisp.bpp;
	lpDevInfo->dibe.deReserved1 = 0;
	lpDevInfo->dibe.deDeltaScan = lpDevInfo->dibe.deWidthBytes;
	lpDevInfo->dibe.delpPDeviceAddr = 0;
	lpDevInfo->dibe.deBitsOffset = 0;
	lpDevInfo->dibe.deBitsSelector = 0;
	lpDevInfo->dibe.deFlags = 0x8000 | 0x0020 | 0x0010 | 0x0001;
	lpDevInfo->dibe.deVersion = 0x0400;
	lpDevInfo->dibe.deBitmapInfoAddr = 0;
	lpDevInfo->dibe.deBeginAccessFuncAddr = 0;
	lpDevInfo->dibe.deEndAccessFuncAddr = 0;
	lpDevInfo->dibe.deDriverReserved = 0;

	if (npdisp.mm_vramLinearAddr) {
		//lpDevInfo->dibe.delpPDeviceAddr = npdisp.mm_linearAddr;
		lpDevInfo->dibe.deBitsSelector = 0;
		lpDevInfo->dibe.deBitsOffset = npdisp.mm_vramLinearAddr;
		lpDevInfo->dibe.deFlags &= ~0x0020;
	}

	if (npdisp.mm_bmpinfoAddr) {
		BITMAPINFO_8BPP bi;
		if (npdisp_readMemory(&bi, npdisp.mm_bmpinfoAddr, sizeof(BITMAPINFO_8BPP))) {
			memcpy(&bi, &npdispwin.bi, sizeof(BITMAPINFO_8BPP));
			if (bi.bmiHeader.biHeight < 0) {
				bi.bmiHeader.biHeight = -bi.bmiHeader.biHeight;
			}
			npdisp_writeMemory(&bi, npdisp.mm_bmpinfoAddr, sizeof(BITMAPINFO_8BPP));
		}
		if (npdisp.isWin9x) {
			// WORKAROUND: Win3.1環境下でWinGがVRAM直接アクセスするようになるが、リアルタイムのVRAM更新に対応しておらずバグるので、暫定でWin9x限定で設定
			lpDevInfo->dibe.deBitmapInfoAddr = npdisp.mm_bmpinfoAddr;
		}
	}

	if (npdisp.bpp == 16) {
		lpDevInfo->dibe.deFlags |= 0x0040; // FIVE6FIVE
	}
	if (npdisp.usePalette) {
		lpDevInfo->dibe.deFlags |= 0x0002; // PALETTIZED
	}

	npdisp.devType = lpDevInfo->bmp.bmType;
	return 1;
}
static UINT16 npdisp_func_Enable_GDIINFO(NPDISP_GDIINFO *lpDevInfo, UINT16 wStyle, const char* lpDestDevType, const char* lpOutputFile, const NPDISP_DEVMODE* lpData) 
{
	//lpDevInfo->dpVersion = 0x030A;
	lpDevInfo->dpVersion = 0x0400;
	lpDevInfo->dpTechnology = NPDISP_DT_RASDISPLAY;
	// 値が大きいとオーバーフローしておかしくなるので、解像度640x400の画面サイズ値を基準にしてスケール
	int virtualWidth = 640;
	int virtualHeight = 400;
	if (npdisp.width * 400 > npdisp.height * 640) {
		// 640x400よりも横長 → 横を640相当にする
		virtualHeight = npdisp.height * 640 / npdisp.width;
	}
	else {
		// 640x400よりも縦長 → 縦を400相当にする
		virtualWidth = npdisp.width * 400 / npdisp.height;
	}
	lpDevInfo->dpHorzSize = 240 * virtualWidth / 640;
	lpDevInfo->dpVertSize = 150 * virtualHeight / 400;
	lpDevInfo->dpHorzRes = npdisp.width;
	lpDevInfo->dpVertRes = npdisp.height;
	lpDevInfo->dpNumBrushes = -1;
	lpDevInfo->dpNumPens = -1;// 16 * 5;
	lpDevInfo->futureuse = 0;
	lpDevInfo->dpNumFonts = 0;
	lpDevInfo->dpDEVICEsize = sizeof(NPDISP_PDEVICE);
	lpDevInfo->dpCurves = NPDISP_CC_CIRCLES | NPDISP_CC_ELLIPSES | NPDISP_CC_WIDE | NPDISP_CC_STYLED | NPDISP_CC_WIDESTYLED | NPDISP_CC_INTERIORS | NPDISP_CC_PIE | NPDISP_CC_CHORD | NPDISP_CC_ROUNDRECT;
	lpDevInfo->dpLines = NPDISP_LC_POLYLINE | NPDISP_LC_STYLED | NPDISP_LC_WIDE | NPDISP_LC_WIDESTYLED | NPDISP_LC_INTERIORS;
	lpDevInfo->dpPolygonals = NPDISP_PC_SCANLINE | NPDISP_PC_RECTANGLE | NPDISP_PC_POLYGON | NPDISP_PC_WINDPOLYGON | NPDISP_PC_WIDE | NPDISP_PC_STYLED | NPDISP_PC_WIDESTYLED | NPDISP_PC_INTERIORS | NPDISP_PC_POLYPOLYGON;
	lpDevInfo->dpText = NPDISP_TC_RA_ABLE;// 0x0004 | 0x2000;
	lpDevInfo->dpClip = NPDISP_CP_RECTANGLE;
	lpDevInfo->dpRaster = NPDISP_RC_BITBLT | NPDISP_RC_BITMAP64 | NPDISP_RC_DI_BITMAP | NPDISP_RC_BIGFONT | NPDISP_RC_SAVEBITMAP | NPDISP_RC_DIBTODEV | NPDISP_RC_GDI20_OUTPUT | NPDISP_RC_OP_DX_OUTPUT | NPDISP_RC_STRETCHBLT | NPDISP_RC_GDI20_STATE | NPDISP_RC_FLOODFILL; // 0x4699; // RC_BITBLT | RC_BITMAP64 | RC_SAVEBITMAP | RC_GDI20_OUTPUT | RC_DI_BITMAP;
	if (npdisp.version >= 3) {
		lpDevInfo->dpRaster |= NPDISP_RC_STRETCHDIB | NPDISP_RC_DEVBITS;
		lpDevInfo->dpCurves |= NPDISP_CC_POLYBEZIER;
		lpDevInfo->dpPolygonals |= NPDISP_PC_POLYPOLYGON;
	}
	lpDevInfo->dpAspectX = 71;
	lpDevInfo->dpAspectY = 71;
	lpDevInfo->dpAspectXY = 100;
	lpDevInfo->dpStyleLen = lpDevInfo->dpAspectXY * 2;
	lpDevInfo->dpLogPixelsX = 96; // ここのDPIはアイコンの文字サイズ等が変わる　変えない方がよさそう？
	lpDevInfo->dpLogPixelsY = 96; // ここのDPIはアイコンの文字サイズ等が変わる　変えない方がよさそう？
	lpDevInfo->dpDCManage = 0x0004;
	lpDevInfo->dpCaps1 = NPDISP_C1_TRANSPARENT | NPDISP_C1_REINIT_ABLE | NPDISP_C1_COLORCURSOR;
	if (npdisp.version >= 3) {
		// DIB Engine準拠
		lpDevInfo->dpCaps1 |= NPDISP_C1_DIBENGINE;
	}
	lpDevInfo->dpSpotSizeX = 0;
	lpDevInfo->dpSpotSizeY = 0;
	lpDevInfo->dpMLoWin.x = lpDevInfo->dpHorzSize * 10;
	lpDevInfo->dpMLoWin.y = lpDevInfo->dpVertSize * 10;
	lpDevInfo->dpMLoVpt.x = (int)virtualWidth;
	lpDevInfo->dpMLoVpt.y = -(int)virtualHeight;
	lpDevInfo->dpMHiWin.x = lpDevInfo->dpHorzSize * 100;
	lpDevInfo->dpMHiWin.y = lpDevInfo->dpVertSize * 100;
	lpDevInfo->dpMHiVpt.x = (int)virtualWidth;
	lpDevInfo->dpMHiVpt.y = -(int)virtualHeight;
	lpDevInfo->dpELoWin.x = 375 * virtualWidth / 640;
	lpDevInfo->dpELoWin.y = 188 * virtualHeight / 400;
	lpDevInfo->dpELoVpt.x = 254 * virtualWidth / 640;
	lpDevInfo->dpELoVpt.y = -127 * virtualHeight / 400;
	lpDevInfo->dpEHiWin.x = 3750 * virtualWidth / 640;
	lpDevInfo->dpEHiWin.y = 1875 * virtualHeight / 400;
	lpDevInfo->dpEHiVpt.x = 254 * virtualWidth / 640;
	lpDevInfo->dpEHiVpt.y = -127 * virtualHeight / 400;
	lpDevInfo->dpTwpWin.x = 5400 * virtualWidth / 640;
	lpDevInfo->dpTwpWin.y = 2700 * virtualHeight / 400;
	lpDevInfo->dpTwpVpt.x = 254 * virtualWidth / 640;
	lpDevInfo->dpTwpVpt.y = -127 * virtualHeight / 400;

	switch (npdisp.bpp) {
	case 1:
		// 2色
		lpDevInfo->dpBitsPixel = 1;
		lpDevInfo->dpPlanes = 1;
		lpDevInfo->dpNumColors = 2;
		break;
	case 4:
		// 16色
		lpDevInfo->dpBitsPixel = 4;
		lpDevInfo->dpPlanes = 1;
		lpDevInfo->dpNumColors = 16;
		break;
	case 8:
		// 256色
		lpDevInfo->dpBitsPixel = 8;
		lpDevInfo->dpPlanes = 1;
		lpDevInfo->dpNumColors = 20; // 20;
		break;
	case 15:
	case 16:
		// 64k色
		lpDevInfo->dpBitsPixel = 16;
		lpDevInfo->dpPlanes = 1;
		lpDevInfo->dpNumColors = 4096;
		break;
	case 24:
		// 16M色(24bit)
		lpDevInfo->dpBitsPixel = 24;
		lpDevInfo->dpPlanes = 1;
		lpDevInfo->dpNumColors = 4096;
		break;
	case 32:
		// 16M色(32bit)
		lpDevInfo->dpBitsPixel = 32;
		lpDevInfo->dpPlanes = 1;
		lpDevInfo->dpNumColors = 4096;
		break;
	}
	if (npdisp.usePalette) {
		lpDevInfo->dpRaster |= RC_PALETTE;
		lpDevInfo->dpPalColors = 256;
		lpDevInfo->dpPalReserved = 20;
		lpDevInfo->dpPalResolution = 24;
	}
	else {
		lpDevInfo->dpPalColors = 0;
		lpDevInfo->dpPalReserved = 0;
		lpDevInfo->dpPalResolution = 0;
	}

	return sizeof(NPDISP_GDIINFO); // ドキュメントに書かれていないがサイズを返さないと駄目
}
static UINT16 npdisp_func_Enable(UINT32 lpDevInfoAddr, UINT16 wStyle, UINT32 lpDestDevTypeAddr, UINT32 lpOutputFileAddr, UINT32 lpDataAddr)
{
	UINT16 retValue = 0;
	if (lpDevInfoAddr) {
		char* lpDestDevType;
		char* lpOutputFile;
		NPDISP_DEVMODE data;
		lpDestDevType = npdisp_readMemoryString(lpDestDevTypeAddr);
		lpOutputFile = npdisp_readMemoryString(lpOutputFileAddr);
		if (lpDataAddr) {
			npdisp_readMemory(&data, lpDataAddr, sizeof(data));
		}
		switch (wStyle & 0x7fff) {
		case 0:
		{
			NPDISP_PDEVICE devInfo;
			npdisp_readMemory(&devInfo, lpDevInfoAddr, sizeof(devInfo));
			retValue = npdisp_func_Enable_PDEVICE(&devInfo, wStyle, lpDestDevType, lpOutputFile, lpDataAddr ? &data : NULL);
			npdisp_writeMemory(&devInfo, lpDevInfoAddr, sizeof(devInfo));
			npdisp_createScreen();
			if (npdisp.mm_bmpinfoAddr) {
				BITMAPINFO_8BPP bi;
				memcpy(&bi, &npdispwin.bi, sizeof(BITMAPINFO_8BPP));
				if (bi.bmiHeader.biHeight < 0) {
					bi.bmiHeader.biHeight = -bi.bmiHeader.biHeight;
				}
				npdisp_writeMemory(&bi, npdisp.mm_bmpinfoAddr, sizeof(BITMAPINFO_8BPP));
			}
			npdisp.enabled = 1;
			npdisp.active = 1;
			np2wab.realWidth = 0; // Force Reset
			np2wab.realHeight = 0; // Force Reset
			np2wab.relaystateext = 3;
			np2wab_setRelayState(np2wab.relaystateint | np2wab.relaystateext);
			npdisp_setDirtyAll();
			npdisp.updated = 1;
			TRACEOUT(("Enable PDEVICE"));
			break;
		}
		case 1:
		{
			NPDISP_GDIINFO gdiInfo;
			npdisp_readMemory(&gdiInfo, lpDevInfoAddr, sizeof(gdiInfo));
			retValue = npdisp_func_Enable_GDIINFO(&gdiInfo, wStyle, lpDestDevType, lpOutputFile, lpDataAddr ? &data : NULL);
			npdisp_writeMemory(&gdiInfo, lpDevInfoAddr, sizeof(gdiInfo));
			TRACEOUT(("Enable GDIINFO"));
			break;
		}
		}
		if (lpDestDevType) free(lpDestDevType);
		if (lpOutputFile) free(lpOutputFile);
	}
	return retValue;
}
static UINT16 npdisp_func_ReEnable(UINT32 lpPDeviceAddr, UINT32 lpGDIInfoAddr)
{
	if (lpGDIInfoAddr) {
		NPDISP_GDIINFO gdiInfo;
		npdisp_readMemory(&gdiInfo, lpGDIInfoAddr, sizeof(gdiInfo));
		npdisp_func_Enable_GDIINFO(&gdiInfo, 1, NULL, NULL, NULL);
		npdisp_writeMemory(&gdiInfo, lpGDIInfoAddr, sizeof(gdiInfo));
	}
	if (lpPDeviceAddr) {
		NPDISP_PDEVICE devInfo;
		npdisp_readMemory(&devInfo, lpPDeviceAddr, 2);
		if (*((UINT16*)&devInfo) == NPDISP_DEVTYPE) npdisp_readMemory(&devInfo, lpPDeviceAddr, sizeof(devInfo));
		npdisp_func_Enable_PDEVICE(&devInfo, 0, NULL, NULL, NULL);
		npdisp_writeMemory(&devInfo, lpPDeviceAddr, sizeof(devInfo));
		npdisp_createScreen(npdisp.enabled && npdisp.active);
		if (npdisp.mm_bmpinfoAddr) {
			BITMAPINFO_8BPP bi;
			memcpy(&bi, &npdispwin.bi, sizeof(BITMAPINFO_8BPP));
			if (bi.bmiHeader.biHeight < 0) {
				bi.bmiHeader.biHeight = -bi.bmiHeader.biHeight;
			}
			npdisp_writeMemory(&bi, npdisp.mm_bmpinfoAddr, sizeof(BITMAPINFO_8BPP));
		}
		npdisp.enabled = 1;
		npdisp.active = 1;
		np2wab.realWidth = 0; // Force Reset
		np2wab.realHeight = 0; // Force Reset
		np2wab.relaystateext = 3;
		np2wab_setRelayState(np2wab.relaystateint | np2wab.relaystateext);
		npdisp_setDirtyAll();
		npdisp.updated = 1;
	}
	return 1;
}
static UINT16 npdisp_func_ValidateMode(UINT32 lpValModeAddr)
{
	if (lpValModeAddr) {
		NPDISP_DISPVALMODE dispValMode;
		npdisp_readMemory(&dispValMode, lpValModeAddr, sizeof(NPDISP_DISPVALMODE));
		dispValMode.dvmBpp = dispValMode.dvmBpp;
		return NPDISP_VALMODE_YES;
	}
	return NPDISP_VALMODE_NO_UNKNOWN;
}

static UINT32 npdisp_func_SelectBitmap(UINT32 lpDeviceAddr, UINT32 lpPrevBitmapAddr, UINT32 lpBitmapAddr, UINT32 fFlags)
{
	// 意味ありげな関数だが何もしなくてよい
	return 1;
}
static UINT32 npdisp_func_BitmapBits(UINT32 lpDeviceAddr, UINT32 fFlags, UINT32 dwCount, UINT32 lpBitsAddr)
{
	UINT32 copyCount = dwCount;
	//return copyCount;
	if (lpDeviceAddr) {
		UINT16 type;
		npdisp_readMemory(&type, lpDeviceAddr, 2);
		if (npdisp_isDisplayDevice(lpDeviceAddr)) {
			// Display
			if (fFlags == NPDISP_DBB_COPY) {
				NPDISP_PBITMAP_EXT ddbmp;
				copyCount = 0;
				npdisp_readPBitmap(&ddbmp, lpBitsAddr);
				auto it = npdispwin.bitmaps.find(ddbmp.ddbmpKey);
				if (it != npdispwin.bitmaps.end()) {
					NPDISP_HOSTBITMAP* hostbmpSrc = &(it->second);
					if (hostbmpSrc->bmphdc.lpbi->bmiHeader.biWidth == npdisp.width &&
						hostbmpSrc->bmphdc.lpbi->bmiHeader.biHeight == npdisp.height &&
						hostbmpSrc->bmphdc.lpbi->bmiHeader.biBitCount == npdisp.bpp) {
						int stride = ((npdisp.width * npdisp.bpp + 31) / 32) * 4;
						copyCount = stride * npdisp.height;
						memcpy(npdispwin.pBits, hostbmpSrc->bmphdc.pBits, copyCount);
					}
				}
			}
			else if (fFlags == NPDISP_DBB_SET) {
				int stride = ((npdisp.width * npdisp.bpp + 31) / 32) * 4;
				int memstride = ((npdisp.width * npdisp.bpp + 15) / 16) * 16 / 8; // lpBitsはWORDアライメント前提
				int remain = min(copyCount, stride * npdisp.height);
				UINT32 selector = (lpBitsAddr >> 16) & 0xffff;
				UINT32 offset = lpBitsAddr & 0xffff;
				UINT8 *pBitsDst = (UINT8*)npdispwin.pBits;
				for (int i = 0; remain > 0; i++) {
					int size = (remain < stride) ? remain : stride;
					npdisp_readMemoryWith32Offset(pBitsDst, selector, offset, stride);
					remain -= stride;
					pBitsDst += stride;
					offset += memstride;
				}
			}
			else if (fFlags == NPDISP_DBB_GET) {
				int stride = ((npdisp.width * npdisp.bpp + 31) / 32) * 4;
				int memstride = ((npdisp.width * npdisp.bpp + 15) / 16) * 16 / 8; // lpBitsはWORDアライメント前提
				int remain = min(copyCount, stride * npdisp.height);
				UINT32 selector = (lpBitsAddr >> 16) & 0xffff;
				UINT32 offset = lpBitsAddr & 0xffff;
				UINT8* pBitsDst = (UINT8*)npdispwin.pBits;
				for (int i = 0; remain > 0; i++) {
					int size = (remain < stride) ? remain : stride;
					npdisp_writeMemoryWith32Offset(pBitsDst, selector, offset, stride);
					remain -= stride;
					pBitsDst += stride;
					offset += memstride;
				}
			}
		}
		else {
			// DDB
			NPDISP_PBITMAP_EXT ddbmpdev;
			NPDISP_HOSTBITMAP *hostbmp = NULL;
			npdisp_readPBitmap(&ddbmpdev, lpDeviceAddr);
			auto it = npdispwin.bitmaps.find(ddbmpdev.ddbmpKey);
			if (it != npdispwin.bitmaps.end()) {
				hostbmp = &(it->second);
			}
			if (hostbmp) {
				if (fFlags == NPDISP_DBB_COPY) {
					NPDISP_PBITMAP_EXT ddbmp;
					copyCount = 0;
					npdisp_readPBitmap(&ddbmp, lpBitsAddr);
					auto it = npdispwin.bitmaps.find(ddbmp.ddbmpKey);
					if (it != npdispwin.bitmaps.end()) {
						NPDISP_HOSTBITMAP* hostbmpSrc = &(it->second);
						if (hostbmpSrc->bmphdc.lpbi->bmiHeader.biWidth == hostbmp->bmphdc.lpbi->bmiHeader.biWidth &&
							hostbmpSrc->bmphdc.lpbi->bmiHeader.biHeight == hostbmp->bmphdc.lpbi->bmiHeader.biHeight &&
							hostbmpSrc->bmphdc.lpbi->bmiHeader.biBitCount == hostbmp->bmphdc.lpbi->bmiHeader.biBitCount) {
							int stride = ((hostbmp->bmphdc.lpbi->bmiHeader.biWidth * hostbmp->bmphdc.lpbi->bmiHeader.biBitCount + 31) / 32) * 4;
							int height = hostbmp->bmphdc.lpbi->bmiHeader.biHeight;
							if (height < 0) height = -height;
							copyCount = stride * height;
							memcpy(hostbmp->bmphdc.pBits, hostbmpSrc->bmphdc.pBits, copyCount);
						}
					}
				}
				else if (fFlags == NPDISP_DBB_SET) {
					int stride = ((hostbmp->bmphdc.lpbi->bmiHeader.biWidth * hostbmp->bmphdc.lpbi->bmiHeader.biBitCount + 31) / 32) * 4;
					int memstride = ((hostbmp->bmphdc.lpbi->bmiHeader.biWidth * hostbmp->bmphdc.lpbi->bmiHeader.biBitCount + 15) / 16) * 16 / 8; // lpBitsはWORDアライメント前提
					int height = hostbmp->bmphdc.lpbi->bmiHeader.biHeight;
					if (height < 0) height = -height;
					int remain = min(copyCount, stride * height);
					UINT32 selector = (lpBitsAddr >> 16) & 0xffff;
					UINT32 offset = lpBitsAddr & 0xffff;
					UINT8* pBitsDst = (UINT8*)hostbmp->bmphdc.pBits;
					for (int i = 0; remain > 0; i++) {
						int size = (remain < stride) ? remain : stride;
						npdisp_readMemoryWith32Offset(pBitsDst, selector, offset, stride);
						remain -= stride;
						pBitsDst += stride;
						offset += memstride;
					}
				}
				else if (fFlags == NPDISP_DBB_GET) {
					int stride = ((hostbmp->bmphdc.lpbi->bmiHeader.biWidth * hostbmp->bmphdc.lpbi->bmiHeader.biBitCount + 31) / 32) * 4;
					int memstride = ((hostbmp->bmphdc.lpbi->bmiHeader.biWidth * hostbmp->bmphdc.lpbi->bmiHeader.biBitCount + 15) / 16) * 16 / 8; // lpBitsはWORDアライメント前提
					int height = hostbmp->bmphdc.lpbi->bmiHeader.biHeight;
					if (height < 0) height = -height;
					int remain = min(copyCount, stride * height);
					UINT32 selector = (lpBitsAddr >> 16) & 0xffff;
					UINT32 offset = lpBitsAddr & 0xffff;
					UINT8* pBitsSrc = (UINT8*)hostbmp->bmphdc.pBits;
					for (int i = 0; remain > 0; i++) {
						int size = (remain < stride) ? remain : stride;
						npdisp_writeMemoryWith32Offset(pBitsSrc, selector, offset, stride);
						remain -= stride;
						pBitsSrc += stride;
						offset += memstride;
					}
				}
				else if (fFlags == NPDISP_DBB_SETWITHFILLER) {
					int stride = ((hostbmp->bmphdc.lpbi->bmiHeader.biWidth * hostbmp->bmphdc.lpbi->bmiHeader.biBitCount + 31) / 32) * 4;
					int memstride = ((hostbmp->bmphdc.lpbi->bmiHeader.biWidth * hostbmp->bmphdc.lpbi->bmiHeader.biBitCount + 15) / 16) * 16 / 8; // lpBitsはWORDアライメント前提
					int height = hostbmp->bmphdc.lpbi->bmiHeader.biHeight;
					if (height < 0) height = -height;
					int remain = min(copyCount, stride * height);
					UINT32 selector = (lpBitsAddr >> 16) & 0xffff;
					UINT32 offset = lpBitsAddr & 0xffff;
					UINT8* pBitsDst = (UINT8*)hostbmp->bmphdc.pBits;
					for (int i = 0; remain > 0; i++) {
						UINT32 remainSegment = 0xffff - (offset & 0xffff);
						if (remainSegment < memstride) {
							offset += remainSegment;
						}
						int size = (remain < stride) ? remain : stride;
						npdisp_readMemoryWith32Offset(pBitsDst, selector, offset, stride);
						remain -= stride;
						pBitsDst += stride;
						offset += memstride;
					}
				}
			}
		}
	}
	return copyCount;
}

static void npdisp_func_Disable(UINT32 lpDestDevAddr)
{
	if (lpDestDevAddr) {
		npdisp.enabled = 0;
		npdisp.active = 0;
		np2wab.relaystateext = 0;
		np2wab_setRelayState(np2wab.relaystateint | np2wab.relaystateext);
	}
}

static SINT16 npdisp_func_GetDriverResourceID(SINT16 iResId, UINT32 lpResTypeAddr)
{
	// DPI毎のリソース変換？
	if (lpResTypeAddr) {
		if (lpResTypeAddr & 0xffff0000) {
			char* lpResType;
			lpResType = npdisp_readMemoryString(lpResTypeAddr);
			if (lpResType) free(lpResType);
		}
		else {
			// 上位が0の時はただの値
			SINT16 iResType = lpResTypeAddr;
		}
	}
	if (npdisp.dpiX >= 96 && (iResId > 32647 || iResId == 1 || iResId == 3)) {
		iResId += 2000;
	}
	return iResId;
}

static UINT32 npdisp_func_ColorInfo(NPDISP_PDEVICE* lpDestDev, UINT32 dwColorin, UINT32* lpPColor)
{
	if (npdisp.bpp != 8) {
		// 256色以外　色を素通しする
		if (lpPColor) {
			if (dwColorin & 0xff000000) {
				if (npdisp.bpp == 1) {
					int idx = dwColorin & 0x1;
					*lpPColor = ((UINT32)npdisp_palette_rgb2[idx].r) | ((UINT32)npdisp_palette_rgb2[idx].g << 8) | ((UINT32)npdisp_palette_rgb2[idx].b << 16);
				}
				else if (npdisp.bpp == 4) {
					int idx = dwColorin & 0xf;
					*lpPColor = ((UINT32)npdisp_palette_rgb16[idx].r) | ((UINT32)npdisp_palette_rgb16[idx].g << 8) | ((UINT32)npdisp_palette_rgb16[idx].b << 16);
				}
				else {
					*lpPColor = dwColorin & 0xffffff;
				}
			}
			else {
				*lpPColor = dwColorin;
			}
		}
		return dwColorin;
	}
	else {
		// 256色
		UINT32 rgb;
		int idx = 0;
		if (lpPColor) {
			// 論理カラー値を最も近い物理デバイスカラー値へ変換　
			if (dwColorin & 0xff000000) {
				// dwColorinは論理カラーインデックス？
				*lpPColor = dwColorin;
				idx = dwColorin & 0xffffff;
				if (idx < 0 || (1 << npdisp.bpp) <= idx) {
					return 0;
				}
			}
			else {
				// dwColorinは論理カラー値（RGB値）
				UINT8 r, g, b;
				r = (UINT8)(dwColorin & 0xFF);
				g = (UINT8)((dwColorin >> 8) & 0xFF);
				b = (UINT8)((dwColorin >> 16) & 0xFF);
				idx = npdisp_FindNearest256(r, g, b);
				if (idx < 20 || 256 - 20 <= idx) {
					// スタティックカラーはRGBで
					*lpPColor = ((UINT32)npdisp_palette_rgb256[idx].r) | ((UINT32)npdisp_palette_rgb256[idx].g << 8) | ((UINT32)npdisp_palette_rgb256[idx].b << 16);
				}
				else {
					// その他の色は物理パレット番号で
					*lpPColor = (UINT32)idx | 0xff000000;
				}
				if (idx != 0 && idx != 0xff) {
					TRACEOUTP(("IN:%08x IDX: %d", dwColorin, idx));
					if (dwColorin & 0xff000000) {
						TRACEOUTP(("P IN:%08x IDX: %d", dwColorin, idx));
					}
				}
			}
		}
		else {
			// 物理デバイスカラー値を論理カラー値へ変換　dwColorinは物理デバイスカラー値（パレット番号など）
			idx = dwColorin & 0xffffff;
			if (idx < 0 || (1 << npdisp.bpp) <= idx) {
				return 0;
			}
		}

		// 求めたカラーパレットの色をRGBで返す
		return ((UINT32)npdisp_palette_rgb256[idx].r) | ((UINT32)npdisp_palette_rgb256[idx].g << 8) | ((UINT32)npdisp_palette_rgb256[idx].b << 16);
	}
}

static UINT32 npdisp_func_RealizeObject_DeletePen(UINT32 lpInObjAddr)
{
	if (lpInObjAddr) {
		// 指定されたキーのペンを削除
		NPDISP_PEN pen = { {NPDISP_PEN_STYLE_SOLID, {1, 0}, 0} };
		npdisp_readMemory(&pen, lpInObjAddr, sizeof(NPDISP_PEN));
		if (pen.key != 0) {
			auto it = npdispwin.pens.find(pen.key);
			if (it != npdispwin.pens.end()) {
				NPDISP_HOSTPEN value = it->second;
				if (value.refCount > 0) {
					value.refCount--;
				}
				if (value.refCount == 0) {
					npdispwin.pens.erase(it);
					if (value.pen) {
						DeleteObject(value.pen);
					}
				}
			}
		}
		//memset(&pen, 0, sizeof(NPDISP_PEN));
		//npdisp_writeMemory(&pen, lpOutObjAddr, sizeof(NPDISP_PEN));
	}
	TRACEOUT(("RealizeObject Release OBJ_PEN"));

	// サイズを返す
	return sizeof(NPDISP_PEN);
}
static UINT32 npdisp_func_RealizeObject_DeleteBrush(UINT32 lpInObjAddr)
{
	if (lpInObjAddr) {
		// 指定されたキーのブラシを削除
		NPDISP_BRUSH brush = { {NPDISP_BRUSH_STYLE_SOLID, 15, NPDISP_BRUSH_HATCH_HORIZONTAL, 15} };
		npdisp_readMemory(&brush, lpInObjAddr, sizeof(NPDISP_BRUSH));
		if (brush.key != 0) {
			auto it = npdispwin.brushes.find(brush.key);
			if (it != npdispwin.brushes.end()) {
				NPDISP_HOSTBRUSH value = it->second;
				if (value.refCount > 0) {
					value.refCount--;
				}
				if (value.refCount == 0) {
					npdispwin.brushes.erase(it);
					if (value.brs) {
						DeleteObject(value.brs);
					}
				}
			}
		}
		//memset(&brush, 0, sizeof(NPDISP_BRUSH));
		//npdisp_writeMemory(&brush, lpOutObjAddr, sizeof(NPDISP_BRUSH));
	}
	TRACEOUT(("RealizeObject Release OBJ_BRUSH"));

	// サイズを返す
	return sizeof(NPDISP_BRUSH);
}
static UINT32 npdisp_func_RealizeObject_DeleteBitmap(UINT32 lpInObjAddr)
{
	if (lpInObjAddr) {
		// 指定されたキーのDDBitmapを削除
		NPDISP_PBITMAP_EXT ddbmp = { 0 };
		npdisp_readMemory(&ddbmp, lpInObjAddr, sizeof(NPDISP_PBITMAP_EXT));
		if (ddbmp.bmType == NPDISP_DEVTYPE_DDB) {
			// キーが有効か確認
			if (ddbmp.ddbmpKey) {
				auto it = npdispwin.bitmaps.find(ddbmp.ddbmpKey);
				if (it != npdispwin.bitmaps.end()) {
					NPDISP_HOSTBITMAP value = it->second;
					npdispwin.bitmaps.erase(it->first);
					if (value.bmphdc.hBmp) {
						//if (ddbmp.bmBitsAddr) {
						//	npdisp_WriteBitmapToPBITMAP(&ddbmp, &value.bmphdc);
						//}
						value.bmphdc.hdc = NULL; // hdcは捨てない
						npdisp_FreeBitmap(&value.bmphdc, true);
					}
					if (it->first + 1 == npdispwin.bitmapsIdx) {
						npdispwin.bitmapsIdx--;
						if (npdispwin.bitmaps.size() > 0) {
							while (npdispwin.bitmaps.find(npdispwin.bitmapsIdx - 1) == npdispwin.bitmaps.end()) {
								npdispwin.bitmapsIdx--;
							}
						}
					}
				}
				TRACEOUT10(("Release Bitmap %d %08x", npdispwin.bitmapsIdx, lpInObjAddr));
				//ddbmp.ddbmpKey = 0;
				//ddbmp.ddbmpKeyInv = 0;
				ddbmp.bmType = 0;
				npdisp_writePBitmap(&ddbmp, lpInObjAddr);
			}
		}
	}
	TRACEOUT(("RealizeObject Release OBJ_PBITMAP"));

	// サイズを返す
	return sizeof(NPDISP_PBITMAP_EXT);
}
static void npdisp_createPen(NPDISP_HOSTPEN *lpHostPen) 
{
	if (lpHostPen->pen) return; // 既にあるなら作り直さない

	if (lpHostPen->lpen.opnStyle != PS_NULL) {
		if (lpHostPen->actualColorNum == 0) {
			lpHostPen->actualColorNum = 1;
			lpHostPen->actualColor = npdisp_AdjustColorRefForGDI(lpHostPen->lpen.lopnColor);
		}
		// 実線固定
		SINT16 style = lpHostPen->lpen.opnStyle;
		if (style == PS_INSIDEFRAME) {
			style = PS_SOLID;
		}
		lpHostPen->pen = CreatePen(style, lpHostPen->lpen.lopnWidth.x, lpHostPen->actualColor); // PS_INSIDEFRAMEは二重補正になるので消す
	}
}
static UINT32 npdisp_func_RealizeObject_CreatePen(UINT32 lpInObjAddr, UINT32 lpOutObjAddr)
{
	TRACEOUT(("RealizeObject Create OBJ_PEN"));
	if (lpOutObjAddr) {
		// 作成
		NPDISP_PEN pen = { {NPDISP_PEN_STYLE_SOLID, {1, 0}, 0} };
		NPDISP_HOSTPEN hostpen = { 0 };
		if (lpInObjAddr) {
			// 指定した設定で作る
			npdisp_readMemory(&(pen.lpen), lpInObjAddr, sizeof(NPDISP_LPEN));
		}
		TRACEOUT((" -> Color %08x", pen.lpen.lopnColor));
		int key = 0;
		for (auto it = npdispwin.pens.begin(); it != npdispwin.pens.end(); ++it) {
			if (it->second.lpen.lopnColor == pen.lpen.lopnColor &&
				it->second.lpen.lopnWidth.x == pen.lpen.lopnWidth.x &&
				it->second.lpen.opnStyle == pen.lpen.opnStyle) {
				key = it->first;
				break;
			}
		}
		if (key) {
			pen.key = key;
			if (npdispwin.pens[pen.key].refCount < UINT_MAX) {
				npdispwin.pens[pen.key].refCount++;
			}
		}
		else {
			hostpen.lpen = pen.lpen;
			npdisp_createPen(&hostpen); // ペン生成
			hostpen.refCount = 1;
			pen.key = npdispwin.pensIdx;
			npdispwin.pensIdx++;
			if (npdispwin.pensIdx == 0) npdispwin.pensIdx++; // 0は使わないことにする
			npdispwin.pens[pen.key] = hostpen;
		}

		// 書き込み
		npdisp_writeMemory(&pen, lpOutObjAddr, sizeof(NPDISP_PEN));
	}
	// サイズを返す
	return sizeof(NPDISP_PEN);
}
static void npdisp_createBrush(NPDISP_HOSTBRUSH* lpHostBrush) 
{
	if (lpHostBrush->brs) return; // 既にあるなら作り直さない

	if (lpHostBrush->lbrush.lbStyle == NPDISP_BRUSH_STYLE_SOLID) {
		// 単色ブラシ生成
		if (lpHostBrush->actualColorNum == 0) {
			bool preferDither;
			UINT32 color = npdisp_AdjustColorRefForGDI(lpHostBrush->lbrush.lbColor, &preferDither);
			if (!preferDither) {
				// 純色
				lpHostBrush->actualColorNum = 1;
				lpHostBrush->actualColor = color;
			}
			else {
				// ディザ
				MakePaletteDitherBrushColor(color, &lpHostBrush->actualColor, &lpHostBrush->actualColor2, &lpHostBrush->actualColor2Ratio);
				lpHostBrush->actualColorNum = 2;
			}
		}
		if (lpHostBrush->actualColorNum == 1) {
			// 純色
			lpHostBrush->brs = CreateSolidBrush(lpHostBrush->actualColor);
		}
		else {
			// ディザ
			lpHostBrush->brs = CreatePaletteDitherBrush(lpHostBrush->actualColor, lpHostBrush->actualColor2, lpHostBrush->actualColor2Ratio);
		}
		if (!lpHostBrush->brs) {
			TRACEOUT2(("RealizeObject Create OBJ_BRUSH SOLID ERROR!!!!!!!!!!!!!!"));
		}
		TRACEOUT((" -> Style:%d, Color:%08x", lpHostBrush->lbrush.lbStyle, lpHostBrush->lbrush.lbColor));
	}
	else if (lpHostBrush->lbrush.lbStyle == NPDISP_BRUSH_STYLE_HATCHED) {
		// ハッチブラシ生成
		if (lpHostBrush->actualColorNum == 0) {
			lpHostBrush->actualColorNum = 1;
			lpHostBrush->actualColor = npdisp_AdjustColorRefForGDI(lpHostBrush->lbrush.lbColor);
		}
		lpHostBrush->brs = CreateHatchBrush(lpHostBrush->lbrush.lbHatch, lpHostBrush->actualColor);
		if (!lpHostBrush->brs) {
			TRACEOUT2(("RealizeObject Create OBJ_BRUSH HATCHED ERROR!!!!!!!!!!!!!!"));
		}
		TRACEOUT((" -> Style:%d, Color:%08x", lpHostBrush->lbrush.lbStyle, lpHostBrush->lbrush.lbColor));
	}
	else if (lpHostBrush->lbrush.lbStyle == NPDISP_BRUSH_STYLE_PATTERN) {
		// パターンブラシ生成
		HDC hdcSrc = npdispwin.hdcCache[0];
		void* pBits = NULL;
		HBITMAP hPatBmpSrc = CreateDIBSection(hdcSrc, (BITMAPINFO*)(&(lpHostBrush->pattern)), DIB_RGB_COLORS, &pBits, NULL, 0);
		if (hPatBmpSrc) {
			const int height = lpHostBrush->pattern.biHeader.biHeight >= 0 ? lpHostBrush->pattern.biHeader.biHeight : -lpHostBrush->pattern.biHeader.biHeight;
			const int stride = ((lpHostBrush->pattern.biHeader.biWidth * lpHostBrush->pattern.biHeader.biBitCount + 31) / 32) * 4;
			memcpy(pBits, lpHostBrush->pattern.bmBits, stride * height);
			if (hPatBmpSrc) {
				if (lpHostBrush->pattern.biHeader.biBitCount == 1) {
					HGDIOBJ hOldBmpSrc = SelectObject(hdcSrc, hPatBmpSrc);
					HBITMAP hPatBmp = CreateBitmap(8, 8, 1, 1, NULL); // DDBでないとパターンにできない？
					if (hPatBmp) {
						HDC hdcPat = npdispwin.hdcCache[1];
						HGDIOBJ hOldBmp = SelectObject(hdcPat, hPatBmp);
						BitBlt(hdcPat, 0, 0, 8, 8, hdcSrc, 0, 0, SRCCOPY);
						lpHostBrush->brs = CreatePatternBrush(hPatBmp);
						SelectObject(hdcPat, hOldBmp);
						DeleteObject(hPatBmp);
					}
					SelectObject(hdcSrc, hOldBmpSrc);
				}
				else {
					HDC hdcPat = npdispwin.hdcCache[1];
					HGDIOBJ hOldBmp = SelectObject(hdcPat, hPatBmpSrc);
					lpHostBrush->brs = CreatePatternBrush(hPatBmpSrc);
					SelectObject(hdcPat, hOldBmp);
				}
				DeleteObject(hPatBmpSrc);
			}
		}
		else {
			// Error!
			lpHostBrush->brs = NULL;
		}
	}
	else if (lpHostBrush->lbrush.lbStyle == NPDISP_BRUSH_STYLE_HOLLOW) {
		// 何もしないブラシ生成
		lpHostBrush->brs = NULL;
	}
}
static UINT32 npdisp_func_RealizeObject_CreateBrush(UINT32 lpInObjAddr, UINT32 lpOutObjAddr, UINT32 lpTextXFormAddr)
{
	NPDISP_POINT ptOrigin = { 0 };
	TRACEOUT(("RealizeObject Create OBJ_BRUSH"));
	if (lpTextXFormAddr) {
		ptOrigin.x = ((SINT16)(lpTextXFormAddr & 0xffff)) % 8;
		ptOrigin.y = ((SINT16)((lpTextXFormAddr >> 16) & 0xffff)) % 8;
		if (ptOrigin.x < 0) ptOrigin.x += 8;
		if (ptOrigin.y < 0) ptOrigin.y += 8;
	}
	if (lpOutObjAddr) {
		// 作成
		NPDISP_BRUSH brush = { {NPDISP_BRUSH_STYLE_SOLID, 15, NPDISP_BRUSH_HATCH_HORIZONTAL, 15} };
		NPDISP_HOSTBRUSH hostbrush = { 0 };
		if (lpInObjAddr) {
			// 指定した設定で作る
			npdisp_readMemory(&(brush.lbrush), lpInObjAddr, sizeof(NPDISP_LBRUSH));
		}
		int key = 0;
		if (brush.lbrush.lbStyle != NPDISP_BRUSH_STYLE_PATTERN) {
			for (auto it = npdispwin.brushes.begin(); it != npdispwin.brushes.end(); ++it) {
				if (it->second.lbrush.lbStyle == brush.lbrush.lbStyle &&
					it->second.lbrush.lbColor == brush.lbrush.lbColor &&
					it->second.lbrush.lbBkColor == brush.lbrush.lbBkColor &&
					it->second.lbrush.lbHatch == brush.lbrush.lbHatch) {
					key = it->first;
					break;
				}
			}
		}
		if (key) {
			brush.key = key;
			if (npdispwin.brushes[brush.key].refCount < UINT_MAX) {
				npdispwin.brushes[brush.key].refCount++;
			}
			TRACEOUT((" -> Style:%d, Color:%08x", brush.lbrush.lbStyle, brush.lbrush.lbColor));
		}
		else {
			hostbrush.lbrush = brush.lbrush;
			if (brush.lbrush.lbStyle == NPDISP_BRUSH_STYLE_PATTERN) {
				// パターンブラシデータ取得
				NPDISP_PBITMAP_EXT patternBmp = { 0 };
				if (npdisp_readPBitmap(&patternBmp, brush.lbrush.lbColor)) {
					NPDISP_WINDOWS_BMPHDC patternBmphdc = { 0 };
					if (npdisp_MakeBitmapFromPBITMAP(&patternBmp, &patternBmphdc, 0)) {
						if (patternBmp.bmHeight < 0) patternBmp.bmHeight = -patternBmp.bmHeight;
						if (patternBmp.bmBitsPixel == 1) {
							HBITMAP hPatBmp = CreateBitmap(8, 8, 1, 1, NULL); // DDBでないとパターンにできない？
							if (hPatBmp) {
								HDC hdcPat = npdispwin.hdcCache[1];
								HGDIOBJ hOldBmp = SelectObject(hdcPat, hPatBmp);
								if (ptOrigin.x == 0 && ptOrigin.y == 0) {
									BitBlt(hdcPat, 0, 0, 8, 8, patternBmphdc.hdc, 0, 0, SRCCOPY);
								}
								else {
									BitBlt(hdcPat, ptOrigin.x - 8, ptOrigin.y - 8, 8, 8, patternBmphdc.hdc, 0, 0, SRCCOPY);
									BitBlt(hdcPat, ptOrigin.x, ptOrigin.y - 8, 8, 8, patternBmphdc.hdc, 0, 0, SRCCOPY);
									BitBlt(hdcPat, ptOrigin.x - 8, ptOrigin.y, 8, 8, patternBmphdc.hdc, 0, 0, SRCCOPY);
									BitBlt(hdcPat, ptOrigin.x, ptOrigin.y, 8, 8, patternBmphdc.hdc, 0, 0, SRCCOPY);
								}
								hostbrush.brs = CreatePatternBrush(hPatBmp); // 取得のついでに生成までやる　npdisp_createBrushはnop
								hostbrush.pattern.biHeader.biSize = sizeof(BITMAPINFOHEADER);
								hostbrush.pattern.biHeader.biWidth = 8;
								hostbrush.pattern.biHeader.biHeight = -8;
								hostbrush.pattern.biHeader.biPlanes = 1;
								hostbrush.pattern.biHeader.biBitCount = 1;
								hostbrush.pattern.biHeader.biCompression = BI_RGB;
								GetDIBits(hdcPat, hPatBmp, 0, 8, hostbrush.pattern.bmBits, (BITMAPINFO*)(&hostbrush.pattern.biHeader), DIB_RGB_COLORS);
								SelectObject(hdcPat, hOldBmp);
								DeleteObject(hPatBmp);
							}
						}
						else {
							hostbrush.pattern.biHeader.biSize = sizeof(BITMAPINFOHEADER);
							hostbrush.pattern.biHeader.biWidth = 8;
							hostbrush.pattern.biHeader.biHeight = -8;
							hostbrush.pattern.biHeader.biPlanes = 1;
							hostbrush.pattern.biHeader.biBitCount = patternBmp.bmBitsPixel;
							hostbrush.pattern.biHeader.biCompression = BI_RGB;
							hostbrush.pattern.biHeader.biSizeImage = 0;
							hostbrush.pattern.biHeader.biXPelsPerMeter = 0;
							hostbrush.pattern.biHeader.biYPelsPerMeter = 0;
							hostbrush.pattern.biHeader.biClrUsed = 1 << patternBmp.bmBitsPixel;
							hostbrush.pattern.biHeader.biClrImportant = 0;
							if (patternBmp.bmBitsPixel <= 8) {
								memcpy(hostbrush.pattern.pal, patternBmphdc.lpbi->bmiColors, sizeof(RGBQUAD) * hostbrush.pattern.biHeader.biClrUsed);
							}
							HDC hdcPat = npdispwin.hdcCache[1];
							void* pBits = NULL;
							HBITMAP hPatBmp = CreateDIBSection(hdcPat, (BITMAPINFO*)(&hostbrush.pattern.biHeader), DIB_RGB_COLORS, &pBits, NULL, 0);
							if (hPatBmp) {
								HGDIOBJ hOldBmp = SelectObject(hdcPat, hPatBmp);
								if (ptOrigin.x == 0 && ptOrigin.y == 0) {
									BitBlt(hdcPat, 0, 0, 8, 8, patternBmphdc.hdc, 0, 0, SRCCOPY);
								}
								else {
									BitBlt(hdcPat, ptOrigin.x - 8, ptOrigin.y - 8, 8, 8, patternBmphdc.hdc, 0, 0, SRCCOPY);
									BitBlt(hdcPat, ptOrigin.x, ptOrigin.y - 8, 8, 8, patternBmphdc.hdc, 0, 0, SRCCOPY);
									BitBlt(hdcPat, ptOrigin.x - 8, ptOrigin.y, 8, 8, patternBmphdc.hdc, 0, 0, SRCCOPY);
									BitBlt(hdcPat, ptOrigin.x, ptOrigin.y, 8, 8, patternBmphdc.hdc, 0, 0, SRCCOPY);
								}
								hostbrush.brs = CreatePatternBrush(hPatBmp); // 取得のついでに生成までやる　npdisp_createBrushはnop
								const int height = hostbrush.pattern.biHeader.biHeight >= 0 ? hostbrush.pattern.biHeader.biHeight : -hostbrush.pattern.biHeader.biHeight;
								const int stride = ((hostbrush.pattern.biHeader.biWidth * hostbrush.pattern.biHeader.biBitCount + 31) / 32) * 4;
								memcpy(hostbrush.pattern.bmBits, pBits, stride * height);
								SelectObject(hdcPat, hOldBmp);
								DeleteObject(hPatBmp);
							}

						}
						npdisp_FreeBitmap(&patternBmphdc);
					}
					else {
						hostbrush.brs = CreateSolidBrush(npdisp_AdjustColorRefForGDI(brush.lbrush.lbColor));
					}
				}
			}
			npdisp_createBrush(&hostbrush); // ブラシ生成
			hostbrush.refCount = 1;
			brush.key = npdispwin.brushesIdx;
			npdispwin.brushesIdx++;
			if (npdispwin.brushesIdx == 0) npdispwin.brushesIdx++; // 0は使わないことにする
			npdispwin.brushes[brush.key] = hostbrush;
		}
		// 書き込み
		npdisp_writeMemory(&brush, lpOutObjAddr, sizeof(NPDISP_BRUSH));
		// 色に応じて返す？
		return brush.lbrush.lbStyle == NPDISP_BRUSH_STYLE_SOLID ? 0x8002 : 0x8000;
	}
	// サイズを返す
	return sizeof(NPDISP_BRUSH);
}
static UINT32 npdisp_func_RealizeObject_CreateBitmap(UINT32 lpInObjAddr, UINT32 lpOutObjAddr)
{
	TRACEOUT(("RealizeObject Create OBJ_PBITMAP"));
	if (lpOutObjAddr) {
		// 作成
		NPDISP_PBITMAP_EXT ddbmp = { 0 };
		NPDISP_HOSTBITMAP hostbmp = { 0 };
		int bitmapsIdx = npdispwin.bitmapsIdx;
		if (lpInObjAddr) {
			// 指定した設定で作る
			npdisp_readMemory(&ddbmp, lpInObjAddr, sizeof(NPDISP_PBITMAP));

			// WORKAROUND: Win3.1ファイル選択ダイアログ特例　bmBitsAddrが0かつ異常なbppやplanesが指定されたとき、デバイスと同じbppで作成する
			// もしかすると、bmBitsAddrが0という条件だけでよい？
			if (((ddbmp.bmBitsPixel != 1 &&
				  ddbmp.bmBitsPixel != 4 &&
				  ddbmp.bmBitsPixel != 8 &&
				  ddbmp.bmBitsPixel != 15 &&
				  ddbmp.bmBitsPixel != 16 &&
				  ddbmp.bmBitsPixel != 24 &&
				  ddbmp.bmBitsPixel != 32) || ddbmp.bmPlanes != 1) && ddbmp.bmBitsAddr == 0) {
				ddbmp.bmBitsPixel = npdisp.bpp;
				ddbmp.bmPlanes = 1;
			}
		}
		else {
			ddbmp.bmPlanes = 1;
			ddbmp.bmBitsPixel = 1;
			ddbmp.bmWidth = 1;
			ddbmp.bmHeight = 1;
		}
		if (ddbmp.bmType != NPDISP_DEVTYPE_DDB) {
			if (npdisp_MakeBitmapFromPBITMAP(&ddbmp, &hostbmp.bmphdc, 2)) {
				// HDC切り離し
				if (hostbmp.bmphdc.hdc) {
					SelectObject(hostbmp.bmphdc.hdc, hostbmp.bmphdc.hOldBmp);
					hostbmp.bmphdc.hdc = NULL;
				}
				//TRACEOUT11(("KEY %08x %08x", ddbmp.ddbmpKey, lpOutObjAddr));
				ddbmp.ddbmpKey = bitmapsIdx;
				if (bitmapsIdx == npdispwin.bitmapsIdx) {
					npdispwin.bitmapsIdx++;
					if (npdispwin.bitmapsIdx > UINT_MAX) npdispwin.bitmapsIdx = 1; // 32bitの範囲で制限、0は使わないことにする
					if (npdispwin.bitmapsIdx == 0) npdispwin.bitmapsIdx++; // 0は使わないことにする
				}
				npdispwin.bitmaps[ddbmp.ddbmpKey] = hostbmp;
				if (npdispwin.bitmaps.size() < 2000000) { // 流石にこの数あるのは異常かと・・・
					// 空きの位置にしておく
					auto it = npdispwin.bitmaps.find(bitmapsIdx);
					while (it != npdispwin.bitmaps.end()) {
						bitmapsIdx++;
						if (npdispwin.bitmapsIdx > UINT_MAX) npdispwin.bitmapsIdx = 1; // 32bitの範囲で制限、0は使わないことにする
						it = npdispwin.bitmaps.find(bitmapsIdx);
					}
				}

				TRACEOUT10(("Realize Bitmap %d %08x", npdispwin.bitmapsIdx, lpOutObjAddr));

				// 書き込み
				ddbmp.bmType = NPDISP_DEVTYPE_DDB;
				npdisp_writePBitmap(&ddbmp, lpOutObjAddr);
			}
		}
	}
	// サイズを返す
	return sizeof(NPDISP_PBITMAP_EXT);
}
static UINT32 npdisp_func_RealizeObject(UINT32 lpDestDevAddr, UINT16 wStyle, UINT32 lpInObjAddr, UINT32 lpOutObjAddr, UINT32 lpTextXFormAddr)
{
	UINT32 retValue = 0;
	if ((SINT16)wStyle < 0) {
		// 削除
		retValue = 1; // 常に成功したことにする
		switch (-((SINT16)wStyle)) {
		case 1: // OBJ_PEN
		{
			retValue = npdisp_func_RealizeObject_DeletePen(lpInObjAddr);
			break;
		}
		case 2: // OBJ_BRUSH
		{
			retValue = npdisp_func_RealizeObject_DeleteBrush(lpInObjAddr);
			break;
		}
		case 3: // OBJ_FONT
		{
			TRACEOUT(("RealizeObject Release OBJ_FONT"));
			// 失敗ということにして0を返す
			retValue = 0;
			break;
		}
		case 5: // OBJ_PBITMAP
		{
			retValue = npdisp_func_RealizeObject_DeleteBitmap(lpInObjAddr != 0 ? lpInObjAddr : lpOutObjAddr);
			break;
		}
		default:
		{
			TRACEOUT(("RealizeObject Release UNKNOWN"));
			break;
		}
		}
	}
	else {
		switch (wStyle) {
		case 1: // OBJ_PEN
		{
			retValue = npdisp_func_RealizeObject_CreatePen(lpInObjAddr, lpOutObjAddr);
			break;
		}
		case 2: // OBJ_BRUSH
		{
			retValue = npdisp_func_RealizeObject_CreateBrush(lpInObjAddr, lpOutObjAddr, lpTextXFormAddr);
			break;
		}
		case 3: // OBJ_FONT
		{
			TRACEOUT(("RealizeObject Create OBJ_FONT"));
			// 失敗ということにして0を返す
			retValue = 0;
			break;
		}
		case 5: // OBJ_PBITMAP
		{
			retValue = npdisp_func_RealizeObject_CreateBitmap(lpInObjAddr, lpOutObjAddr);
			break;
		}
		default:
		{
			retValue = 0; // デバイス作成不可
			TRACEOUT(("RealizeObject Create UNKNOWN"));
			break;
		}
		}
	}
	return retValue;
}

static UINT16 npdisp_func_Control(UINT32 lpDestDevAddr, UINT16 wFunction, UINT32 lpInDataAddr, UINT32 lpOutDataAddr)
{
	UINT16 retValue = 0;
	if (lpDestDevAddr) {
		NPDISP_PDEVICE destDev;
		npdisp_readMemory(&destDev, lpDestDevAddr, sizeof(destDev));
		switch (wFunction) {
		case NPDISP_CONTROL_QUERYESCSUPPORT:
		{
			UINT16 escNum = npdisp_readMemory16(lpInDataAddr);
			switch (escNum) {
			case NPDISP_CONTROL_QUERYESCSUPPORT: // QUERYESCSUPPORTは必ずサポート
			case NPDISP_CONTROL_OPENGL_CMD:
			case NPDISP_CONTROL_OPENGL_GETINFO:
			case NPDISP_CONTROL_WNDOBJ_SETUP:
			{
				retValue = 1;
				break;
			}
			case NPDISP_CONTROL_QUERYDIBSUPPORT: // Undocumented: DIB Support? これを返すだけでパフォーマンスが大幅に上がる
			{
				retValue = NPDISP_QDI_SETDIBITS | NPDISP_QDI_GETDIBITS | NPDISP_QDI_DIBTOSCREEN | NPDISP_QDI_STRETCHDIB;
				break;
			}
			case NPDISP_DEVTYPE_DIBENG: // Undocumented: 
			{
				retValue = NPDISP_DEVTYPE_DIBENG;
				break;
			}
			case SETCOLORTABLE:
			case GETCOLORTABLE:
			{
				retValue = 1;
				break;
			}
			case NPDISP_CONTROL_DCICOMMAND: // DirectDraw
			{
				retValue = 0x0100; // DD_HAL_VERSION 
				break;
			}
			default:
			{
				retValue = 0;
				break;
			}
			}
			break;
		}
		case NPDISP_CONTROL_OPENGL_CMD:
		{
			retValue = -1;
			break;
		}
		case NPDISP_CONTROL_OPENGL_GETINFO:
		{
			if (lpOutDataAddr) {
				NPDISP_OPENGL_INFO info = { 1, 1, "" };
				npdisp_writeMemory(&info, lpOutDataAddr, sizeof(NPDISP_OPENGL_INFO));
				retValue = 1;
			}
			else {
				retValue = -1;
			}
			break;
		}
		case NPDISP_CONTROL_WNDOBJ_SETUP:
		{
			retValue = -1;
			break;
		}
		case NPDISP_CONTROL_NP2DCIENABLE:
		{
			npdisp.mm_dciEnable = 1;
			break;
		}
		case NPDISP_CONTROL_NP2DCIDISABLE:
		{
			npdisp.mm_dciEnable = 0;
			break;
		}
		case NPDISP_CONTROL_SETCOLORTABLE:
		{
			retValue = 0;
			break;
		}
		case NPDISP_CONTROL_GETCOLORTABLE:
		{
			retValue = 0;
			break;
		}
		case NPDISP_CONTROL_DCICOMMAND:
		{
			retValue = -1;
			if (lpInDataAddr) {
				NPDISP_DCICMD dciCmd;
				if (npdisp_readMemory(&dciCmd, lpInDataAddr, sizeof(dciCmd))) {
					// NOTE: DirectDraw関係のコマンドが呼ばれるためにはDIB Engine互換でないとだめ
					switch (dciCmd.dwCommand) {
					case NPDISP_CONTROL_DCI_DCICREATEPRIMARYSURFACE:
					{
						if (npdisp.version >= 5 && npdisp.mm_vramLinearAddr && lpOutDataAddr && npdisp.mm_dciEnable) {
							//static UINT64 lastClock = 0;
							//static UINT32 lastAddr = 0;
							//UINT64 curClock = CPU_CLOCK + CPU_BASECLOCK - CPU_REMCLOCK;
							//UINT32 curAddr = lpOutDataAddr;

							TRACEOUT11(("MEM %08x %08x", lpInDataAddr, lpOutDataAddr));

							NPDISP_DCICREATEINPUT createInput = { 0 };
							npdisp_readMemory(&createInput, lpInDataAddr, sizeof(createInput));

							NPDISP_DCISURFACEINFO surfaceInfo = { 0 };
							surfaceInfo.dwSize = sizeof(surfaceInfo);
							surfaceInfo.dwDCICaps = 0x00000010;// DCI_PRIMARY | DCI_VISIBLE;
							surfaceInfo.dwCompression = BI_RGB;
							if (npdisp.bpp == 15 || npdisp.bpp == 16 || npdisp.bpp == 32) {
								if (npdisp.bpp == 16) {
									// ビットフィールド 565
									surfaceInfo.dwMask[0] = 0x0000F800;
									surfaceInfo.dwMask[1] = 0x000007E0;
									surfaceInfo.dwMask[2] = 0x0000001F;
								}
								else if (npdisp.bpp == 15) {
									// ビットフィールド 555
									surfaceInfo.dwMask[0] = 0x00007C00;
									surfaceInfo.dwMask[1] = 0x000003E0;
									surfaceInfo.dwMask[2] = 0x0000001F;
								}
								surfaceInfo.dwCompression |= BI_BITFIELDS;
							}
							surfaceInfo.dwWidth = npdisp.width;
							surfaceInfo.dwHeight = npdisp.height;
							surfaceInfo.lStride = ((npdisp.width * npdisp.bpp + 31) / 32) * 4;
							surfaceInfo.dwBitCount = npdisp.bpp;

							surfaceInfo.dwOffSurface = npdisp.mm_vramLinearAddr;
							surfaceInfo.wSelSurface = 0;
							surfaceInfo.wReserved = 0;

							surfaceInfo.dwReserved1 = 0;
							surfaceInfo.dwReserved2 = 0;
							surfaceInfo.dwReserved3 = 0;

							surfaceInfo.BeginAccessAddr = npdisp.mm_dciBeginAccessAddr;
							surfaceInfo.EndAccessAddr = npdisp.mm_dciEndAccessAddr;
							surfaceInfo.DestroySurfaceAddr = npdisp.mm_dciDestroySurfaceAddr;

							npdisp_writeMemory(&surfaceInfo, npdisp.mm_dcibufAddr, sizeof(surfaceInfo));

							npdisp_writeMemory32(npdisp.mm_dcibufAddr, lpOutDataAddr);
							retValue = 0;
							//retValue = (curAddr != lastAddr && curClock - lastClock < 5000000) ? 0 : -1;
							//lastClock = CPU_CLOCK + CPU_BASECLOCK - CPU_REMCLOCK;
							//lastAddr = lpOutDataAddr;
						}
						else {
							retValue = -1;
						}
						break;
					}
					case NPDISP_CONTROL_DCI_DDCREATEDRIVEROBJECT:
					{
						if (lpOutDataAddr) {
							NPDISP_DDHALINFO halInfo = { 0 };
							npdisp_readMemory(&halInfo, lpOutDataAddr, sizeof(halInfo));
							halInfo.dwSize = sizeof(halInfo);
							halInfo.vmiData.fpPrimary = npdisp.mm_vramLinearAddr;
							halInfo.vmiData.dwDisplayWidth = npdisp.width;
							halInfo.vmiData.dwDisplayHeight = npdisp.height;
							//halInfo.ddCaps.dwCaps | 0x02000000l;
							halInfo.vmiData.lDisplayPitch = ((npdisp.width * npdisp.bpp + 31) / 32) * 4;
							//halInfo.vmiData.ddpfDisplay.dwSize = sizeof(NPDISP_DDPIXELFORMAT);
							//halInfo.dwFlags |= 0x00000001; // DDHALINFO_ISPRIMARYDISPLAY
							//halInfo.lpPDevice = lpDestDevAddr;
							npdisp_writeMemory(&halInfo, lpOutDataAddr, sizeof(halInfo));
						}
						retValue = 0;
						break;
					}
					case NPDISP_CONTROL_DCI_DDGET32BITDRIVERNAME:
					{
						retValue = 0;
						break;
					}
					case NPDISP_CONTROL_DCI_DDNEWCALLBACKFNS:
					{
						retValue = 0;
						break;
					}
					}
				}
			}
			break;
		}
		}
		// 必要ならサポート
	}
	return retValue;
}

static UINT16 npdisp_func_BitBlt(UINT32 lpDestDevAddr, SINT16 wDestX, SINT16 wDestY, UINT32 lpSrcDevAddr, SINT16 wSrcX, SINT16 wSrcY, UINT16 wXext, UINT16 wYext, UINT32 Rop3, UINT32 lpPBrushAddr, UINT32 lpDrawModeAddr)
{
	bool dstIsDisplay = false;
	bool srcIsDisplay = false;
	int hasDstDev = 0;
	int hasSrcDev = 0;
	UINT16 retValue = 0;
	if (lpDestDevAddr) {
		dstIsDisplay = npdisp_isDisplayDevice(lpDestDevAddr);
	}
	if (lpSrcDevAddr) {
		srcIsDisplay = npdisp_isDisplayDevice(lpSrcDevAddr);
	}
	if (npdisp.longjmpnum == 0) {
		if (dstIsDisplay && srcIsDisplay) {
			retValue = npdisp_func_BitBlt_VRAMtoVRAM(hasDstDev, hasSrcDev, lpDestDevAddr, wDestX, wDestY, lpSrcDevAddr, wSrcX, wSrcY, wXext, wYext, Rop3, lpPBrushAddr, lpDrawModeAddr);
		}
		else if (dstIsDisplay && !srcIsDisplay) {
			retValue = npdisp_func_BitBlt_MEMtoVRAM(hasDstDev, hasSrcDev, lpDestDevAddr, wDestX, wDestY, lpSrcDevAddr, wSrcX, wSrcY, wXext, wYext, Rop3, lpPBrushAddr, lpDrawModeAddr);
		}
		else if (!dstIsDisplay && srcIsDisplay) {
			retValue = npdisp_func_BitBlt_VRAMtoMEM(hasDstDev, hasSrcDev, lpDestDevAddr, wDestX, wDestY, lpSrcDevAddr, wSrcX, wSrcY, wXext, wYext, Rop3, lpPBrushAddr, lpDrawModeAddr);
		}
		else {
			retValue = npdisp_func_BitBlt_MEMtoMEM(hasDstDev, hasSrcDev, lpDestDevAddr, wDestX, wDestY, lpSrcDevAddr, wSrcX, wSrcY, wXext, wYext, Rop3, lpPBrushAddr, lpDrawModeAddr);
		}
	}
	else {
		retValue = 1; // 成功したことにする
	}
	return retValue;
}


static UINT16 npdisp_func_StretchBlt(UINT32 lpDestDevAddr, SINT16 wDestX, SINT16 wDestY, SINT16 wDestXext, SINT16 wDestYext, UINT32 lpSrcDevAddr, SINT16 wSrcX, SINT16 wSrcY, SINT16 wSrcXext, SINT16 wSrcYext, UINT32 Rop3, UINT32 lpPBrushAddr, UINT32 lpDrawModeAddr, UINT32 lpClipAddr)
{
	bool dstIsDisplay = false;
	bool srcIsDisplay = false;
	int hasDstDev = 0;
	int hasSrcDev = 0;
	UINT16 retValue = 0;

	if (wDestXext < 0 || wDestYext < 0 || wSrcXext < 0 || wSrcYext < 0) {
		// 暫定　反転転送はややこしいのでGDIにやらせる
		return 0xffff;
	}

	if (lpDestDevAddr) {
		dstIsDisplay = npdisp_isDisplayDevice(lpDestDevAddr);
	}
	if (lpSrcDevAddr) {
		srcIsDisplay = npdisp_isDisplayDevice(lpSrcDevAddr);
	}
	if (npdisp.longjmpnum == 0) {
		if (dstIsDisplay && srcIsDisplay) {
			retValue = npdisp_func_StretchBlt_VRAMtoVRAM(hasDstDev, hasSrcDev, lpDestDevAddr, wDestX, wDestY, wDestXext, wDestYext, lpSrcDevAddr, wSrcX, wSrcY, wSrcXext, wSrcYext, Rop3, lpPBrushAddr, lpDrawModeAddr, lpClipAddr);
		}
		else if (dstIsDisplay && !srcIsDisplay) {
			retValue = npdisp_func_StretchBlt_MEMtoVRAM(hasDstDev, hasSrcDev, lpDestDevAddr, wDestX, wDestY, wDestXext, wDestYext, lpSrcDevAddr, wSrcX, wSrcY, wSrcXext, wSrcYext, Rop3, lpPBrushAddr, lpDrawModeAddr, lpClipAddr);
		}
		else if (!dstIsDisplay && srcIsDisplay) {
			retValue = npdisp_func_StretchBlt_VRAMtoMEM(hasDstDev, hasSrcDev, lpDestDevAddr, wDestX, wDestY, wDestXext, wDestYext, lpSrcDevAddr, wSrcX, wSrcY, wSrcXext, wSrcYext, Rop3, lpPBrushAddr, lpDrawModeAddr, lpClipAddr);
		}
		else {
			retValue = npdisp_func_StretchBlt_MEMtoMEM(hasDstDev, hasSrcDev, lpDestDevAddr, wDestX, wDestY, wDestXext, wDestYext, lpSrcDevAddr, wSrcX, wSrcY, wSrcXext, wSrcYext, Rop3, lpPBrushAddr, lpDrawModeAddr, lpClipAddr);
		}
	}
	else {
		retValue = 1; // 成功したことにする
	}
	return retValue;
}

static UINT16 npdisp_func_DeviceBitmapBits(UINT32 lpBitmapAddr, UINT16 fGet, UINT16 iStart, UINT16 cScans, UINT32 lpDIBitsAddr, UINT32 lpBitmapInfoAddr, UINT32 lpDrawModeAddr, UINT32 lpTranslateAddr)
{
	UINT16 retValue = 0;
	if (lpBitmapAddr) {
		NPDISP_PBITMAP_EXT tgtPBmp;
		if (npdisp_readPBitmap(&tgtPBmp, lpBitmapAddr)) {
			BITMAPINFOHEADER biHeader = { 0 };
			npdisp_readMemory(&biHeader, lpBitmapInfoAddr, sizeof(BITMAPINFOHEADER));
			if (biHeader.biPlanes == 1 && (biHeader.biBitCount == 1 || biHeader.biBitCount == 4 || biHeader.biBitCount == 8 || biHeader.biBitCount == 15 || biHeader.biBitCount == 16 || biHeader.biBitCount == 24 || biHeader.biBitCount == 32) && biHeader.biHeight > iStart) { // XXX: biHeader.biHeightがマイナスはあり得るか？？　要確認
				NPDISP_WINDOWS_BMPHDC tgtbmphdc = { 0 };
				int stride = ((biHeader.biWidth * biHeader.biBitCount + 31) / 32) * 4;
				int height = cScans;
				int beginLine = biHeader.biHeight - height - iStart;
				int lpbiLen = 0;
				int lpbiReadLen = 0;
				int lpbiWriteLen = 0;
				if (biHeader.biBitCount <= 8) {
					lpbiLen = sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * (1 << biHeader.biBitCount);
					if (npdisp.usePalette) {
						if (!lpTranslateAddr) {
							lpbiReadLen = sizeof(BITMAPINFOHEADER) + sizeof(UINT16) * (1 << biHeader.biBitCount);
							lpbiWriteLen = lpbiReadLen;
						}
						else {
							lpbiReadLen = sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * (1 << biHeader.biBitCount);
							lpbiWriteLen = lpbiLen;
						}
					}
					else {
						lpbiReadLen = lpbiLen;
						lpbiWriteLen = lpbiLen;
					}
					if (lpbiLen < lpbiReadLen) {
						lpbiLen = lpbiReadLen;
					}
					if (lpbiLen < lpbiWriteLen) {
						lpbiLen = lpbiWriteLen;
					}
				}
				else if ((biHeader.biBitCount == 15 || biHeader.biBitCount == 16 || biHeader.biBitCount == 32) && biHeader.biCompression == BI_BITFIELDS) {
					lpbiReadLen = lpbiLen = sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * 3;
				}
				else {
					lpbiReadLen = lpbiLen = sizeof(BITMAPINFOHEADER);
				}
				BITMAPINFO* lpbi;
				lpbi = (BITMAPINFO*)malloc(lpbiLen);
				if (lpbi) {
					if (lpbiLen > lpbiReadLen) {
						memset((UINT8*)lpbi + lpbiReadLen, 0, lpbiLen - lpbiReadLen);
					}
					npdisp_readMemory(lpbi, lpBitmapInfoAddr, lpbiReadLen);
					if (lpDIBitsAddr) {
						bool useRGBBlt = false;
						UINT16* transTbl = NULL; // transTblはbiBitCountによらずUINT16配列に変換する
						int colors = (1 << lpbi->bmiHeader.biBitCount);
						if (lpTranslateAddr && npdisp.bpp <= 8) {
							if (lpbi->bmiHeader.biBitCount == 8) {
								if (fGet) {
									// 1byte x 256色 で渡される
									UINT8* transTbl8 = (UINT8*)malloc(colors);
									if (transTbl8) {
										if (npdisp_readMemory(transTbl8, lpTranslateAddr, colors)) {
											transTbl = (UINT16*)malloc(colors * sizeof(UINT16));
											if (transTbl) {
												for (int i = 0; i < colors; i++) {
													((UINT16*)transTbl)[i] = transTbl8[i];
												}
											}
										}
										free(transTbl8);
									}
								}
								else {
									// 2byte x 256色 で渡される?
									transTbl = (UINT16*)malloc(colors * sizeof(UINT16));
									if (transTbl) {
										npdisp_readMemory(transTbl, lpTranslateAddr, colors * sizeof(UINT16));
									}
								}
							}
							else {
								if (fGet) {
									// XXX: よく分からない
									useRGBBlt = true;
								}
								else {
									if (lpbi->bmiHeader.biBitCount == 4) {
										// 2byte x 16色 で渡される?
										transTbl = (UINT16*)malloc(colors * sizeof(UINT16));
										if (transTbl) {
											npdisp_readMemory(transTbl, lpTranslateAddr, colors * sizeof(UINT16));
										}
									}
									else if (lpbi->bmiHeader.biBitCount == 1) {
										// 2byte x 2色 で渡される？
										transTbl = (UINT16*)malloc(colors * sizeof(UINT16));
										if (transTbl) {
											npdisp_readMemory(transTbl, lpTranslateAddr, colors * sizeof(UINT16));
										}
									}
								}
							}
						}
						npdisp_PreloadBitmapFromPBITMAP(&tgtPBmp, 0, beginLine, height);
						if (lpbi->bmiHeader.biCompression == BI_RGB || lpbi->bmiHeader.biCompression == BI_BITFIELDS) {
							npdisp_preloadMemory(lpDIBitsAddr, stride * height); // 無圧縮なら画像サイズで先読み
						}
						else if (lpbi->bmiHeader.biSizeImage) {
							npdisp_preloadMemory(lpDIBitsAddr, lpbi->bmiHeader.biSizeImage); // RLE圧縮でサイズ既知なら先読み
						}
						if (npdisp.longjmpnum == 0 && npdisp_MakeBitmapFromPBITMAP(&tgtPBmp, &tgtbmphdc, 0, beginLine, height)) {
							npdisp_ConvertToDDBMonoBitmap(&tgtbmphdc);
							int i;
							if (iStart + height > biHeader.biHeight) {
								height = biHeader.biHeight - iStart;
							}
							lpbi->bmiHeader.biHeight = height;
							//if (lpbi->bmiHeader.biSizeImage == 0 && lpbi->bmiHeader.biBitCount <= 8) {
							//	// XXX: 画像データがなければパレットセット、あればそのまま　根拠無し
							//	int colors = (1 << lpbi->bmiHeader.biBitCount);
							//	for (i = 0; i < colors; i++) {
							//		if (lpbi->bmiColors[i].rgbReserved || fGet) {
							//			lpbi->bmiColors[i].rgbRed = tgtbmphdc.lpbi->bmiColors[i].rgbRed;
							//			lpbi->bmiColors[i].rgbGreen = tgtbmphdc.lpbi->bmiColors[i].rgbGreen;
							//			lpbi->bmiColors[i].rgbBlue = tgtbmphdc.lpbi->bmiColors[i].rgbBlue;
							//			lpbi->bmiColors[i].rgbReserved = tgtbmphdc.lpbi->bmiColors[i].rgbReserved;
							//		}
							//	}
							//}
							if (npdisp.usePalette) {
								if (lpbi->bmiHeader.biBitCount <= 8) {
									if (lpTranslateAddr) {
										// transTblにインデックス変換表が入る
										if (transTbl) {
											for (i = 0; i < colors; i++) {
												lpbi->bmiColors[i].rgbRed = transTbl[i] & 0xff;
												lpbi->bmiColors[i].rgbGreen = transTbl[i] & 0xff;
												lpbi->bmiColors[i].rgbBlue = transTbl[i] & 0xff;
												lpbi->bmiColors[i].rgbReserved = 0;
											}
										}
									}
									else {
										UINT16 palTrans[256];
										memcpy(palTrans, lpbi->bmiColors, colors * sizeof(UINT16));
										for (i = 0; i < colors; i++) {
											lpbi->bmiColors[i].rgbRed = palTrans[i] & 0xff;
											lpbi->bmiColors[i].rgbGreen = palTrans[i] & 0xff;
											lpbi->bmiColors[i].rgbBlue = palTrans[i] & 0xff;
											lpbi->bmiColors[i].rgbReserved = 0;
										}

									}
								}
							}
							else if (fGet) {
								if (lpbi->bmiHeader.biBitCount == 1) {
									// 有効なパレットでなければ2色パレットセット
									if (lpbi->bmiColors[0].rgbRed != 0 || lpbi->bmiColors[0].rgbGreen != 0 || lpbi->bmiColors[0].rgbBlue != 0 || lpbi->bmiColors[0].rgbReserved != 0 ||
										lpbi->bmiColors[1].rgbRed != 0xff || lpbi->bmiColors[1].rgbGreen != 0xff || lpbi->bmiColors[1].rgbBlue != 0xff || lpbi->bmiColors[1].rgbReserved != 0) {
										for (i = biHeader.biClrUsed; i < NELEMENTS(npdisp_palette_rgb2); i++) {
											lpbi->bmiColors[i].rgbRed = npdisp_palette_rgb2[i].r;
											lpbi->bmiColors[i].rgbGreen = npdisp_palette_rgb2[i].g;
											lpbi->bmiColors[i].rgbBlue = npdisp_palette_rgb2[i].b;
											lpbi->bmiColors[i].rgbReserved = 0;
										}
										if (fGet) {
											npdisp_writeMemory(lpbi, lpBitmapInfoAddr, lpbiReadLen); // 変更したパレットを書き戻し
										}
									}
								}
								else if (lpbi->bmiHeader.biBitCount == 4) {
									// 有効なパレットでなければ16色パレットセット
									if (lpbi->bmiColors[0].rgbRed != 0 || lpbi->bmiColors[0].rgbGreen != 0 || lpbi->bmiColors[0].rgbBlue != 0 ||
										lpbi->bmiColors[15].rgbRed != 0xff || lpbi->bmiColors[15].rgbGreen != 0xff || lpbi->bmiColors[15].rgbBlue != 0xff) {
										for (i = biHeader.biClrUsed; i < NELEMENTS(npdisp_palette_rgb16); i++) {
											lpbi->bmiColors[i].rgbRed = npdisp_palette_rgb16[i].r;
											lpbi->bmiColors[i].rgbGreen = npdisp_palette_rgb16[i].g;
											lpbi->bmiColors[i].rgbBlue = npdisp_palette_rgb16[i].b;
											lpbi->bmiColors[i].rgbReserved = 0;
										}
										if (fGet) {
											npdisp_writeMemory(lpbi, lpBitmapInfoAddr, lpbiReadLen); // 変更したパレットを書き戻し
										}
									}
								}
								else if (lpbi->bmiHeader.biBitCount == 8) {
									// 有効なパレットでなければ256色パレットセット
									if (lpbi->bmiColors[0].rgbRed != 0 || lpbi->bmiColors[0].rgbGreen != 0 || lpbi->bmiColors[0].rgbBlue != 0 ||
										lpbi->bmiColors[255].rgbRed != 0xff || lpbi->bmiColors[255].rgbGreen != 0xff || lpbi->bmiColors[255].rgbBlue != 0xff) {
										for (i = biHeader.biClrUsed; i < NELEMENTS(npdisp_palette_rgb256); i++) {
											lpbi->bmiColors[i].rgbRed = npdisp_palette_rgb256[i].r;
											lpbi->bmiColors[i].rgbGreen = npdisp_palette_rgb256[i].g;
											lpbi->bmiColors[i].rgbBlue = npdisp_palette_rgb256[i].b;
											lpbi->bmiColors[i].rgbReserved = 0;
										}
										if (fGet) {
											npdisp_writeMemory(lpbi, lpBitmapInfoAddr, lpbiReadLen); // 変更したパレットを書き戻し
										}
									}
								}
							}
							UINT32 biCompression = lpbi->bmiHeader.biCompression;
							if (lpbi->bmiHeader.biCompression != BI_BITFIELDS) {
								lpbi->bmiHeader.biCompression = BI_RGB;
							}
							bool isCompress = !(biCompression == BI_RGB || biCompression == BI_BITFIELDS);
							if (isCompress) {
								// 圧縮なら常に全部を読む
								if (biHeader.biHeight < 0) {
									lpbi->bmiHeader.biHeight = -biHeader.biHeight;
								}
								else {
									lpbi->bmiHeader.biHeight = biHeader.biHeight;
								}
							}
							void* pBits = NULL;
							HBITMAP hBmp = CreateDIBSection(npdispwin.hdc, lpbi, DIB_RGB_COLORS, &pBits, NULL, 0);
							void* pBitsValid = NULL;
							HBITMAP hBmpValid = NULL;
							if (isCompress) {
								int colors = (1 << lpbi->bmiHeader.biBitCount);
								int oldColorUsed = lpbi->bmiHeader.biClrUsed;
								RGBQUAD oldpal0 = lpbi->bmiColors[0];
								RGBQUAD oldpalf = lpbi->bmiColors[colors - 1];

								// マスク用パレットに変更
								lpbi->bmiHeader.biClrUsed = colors;
								lpbi->bmiColors[0].rgbRed = lpbi->bmiColors[0].rgbGreen = lpbi->bmiColors[0].rgbBlue = lpbi->bmiColors[0].rgbReserved = 0x00;
								lpbi->bmiColors[colors - 1].rgbRed = lpbi->bmiColors[colors - 1].rgbGreen = lpbi->bmiColors[colors - 1].rgbBlue = lpbi->bmiColors[colors - 1].rgbReserved = 0xff;

								hBmpValid = CreateDIBSection(npdispwin.hdc, lpbi, DIB_RGB_COLORS, &pBitsValid, NULL, 0);

								// 元のパレットへ戻す
								lpbi->bmiHeader.biClrUsed = oldColorUsed;
								lpbi->bmiColors[0] = oldpal0;
								lpbi->bmiColors[colors - 1] = oldpalf;
							}
							if (hBmp) {
								HDC hdc = npdispwin.hdcCache[1];
								HGDIOBJ hOldBmp = SelectObject(hdc, hBmp);
								bool hasError = false;
								if (!isCompress) {
									npdisp_readMemory(pBits, lpDIBitsAddr, stride * height);
								}
								else {
									UINT32 rleSize = lpbi->bmiHeader.biSizeImage;
									if (rleSize == 0) {
										rleSize = stride * height; // とりあえず無圧縮のサイズを確保
									}
									UINT8* cdata = (UINT8*)malloc(rleSize);
									if (cdata) {
										BITMAPINFOHEADER bmiHeaderRLE = lpbi->bmiHeader;
										bmiHeaderRLE.biCompression = biCompression;
										if (bmiHeaderRLE.biHeight > 0) {
											bmiHeaderRLE.biHeight = -bmiHeaderRLE.biHeight; // 逆さで出力
										}
										if (lpbi->bmiHeader.biSizeImage != 0) {
											npdisp_readMemory(cdata, lpDIBitsAddr, rleSize);
										}
										else {
											// RLE終端が来るまで読む 最大読み取りサイズは無圧縮サイズとする
											rleSize = npdisp_RLEBMPReadAndCalcSize(lpDIBitsAddr, bmiHeaderRLE.biCompression, cdata, rleSize);
										}
										npdisp_DecompressRLEBMP(&bmiHeaderRLE, cdata, rleSize, (UINT8*)pBits, (UINT8*)pBitsValid);
										free(cdata);
									}
								}
								if (!hasError) {
									bool palChanged = false;
									if (npdisp.usePalette) {
										if (lpbi->bmiHeader.biBitCount > 8 || useRGBBlt) {
											// グレースケールから実際のデバイス色へ置き換え
											if (npdisp.bpp == 8) {
												RGBQUAD pal[256];
												for (int i = 0; i < 256; i++) {
													pal[i].rgbRed = npdisp_palette_rgb256[i].r;
													pal[i].rgbGreen = npdisp_palette_rgb256[i].g;
													pal[i].rgbBlue = npdisp_palette_rgb256[i].b;
													pal[i].rgbReserved = 0;
												}
												SetDIBColorTable(tgtbmphdc.hdc, 0, 256, pal);
												palChanged = true;
											}
										}
									}
									NPDISP_DRAWMODE drawMode = { 0 };
									if (npdisp_readMemory(&drawMode, lpDrawModeAddr, sizeof(NPDISP_DRAWMODE))) {
										if (hdc != npdispwin.hdc || memcmp(&drawMode, &npdispwin.lastScreenDrawMode, sizeof(NPDISP_DRAWMODE)) != 0) {
											if (hdc == npdispwin.hdc) npdispwin.lastScreenDrawMode = drawMode;
											npdisp_AdjustDrawModeColor(&drawMode);
											SetBkColor(hdc, drawMode.LbkColor);
											SetTextColor(hdc, drawMode.LTextColor);
											SetBkMode(hdc, drawMode.bkMode);
											SetROP2(hdc, drawMode.Rop2);
										}
									}
									if (fGet) {
										// Get Bits
										//BitBlt(hdc, 0, iStart, biHeader.biWidth, cScans, NULL, 0, 0, WHITENESS);
										BitBlt(hdc, 0, 0, biHeader.biWidth, height, tgtbmphdc.hdc, 0, biHeader.biHeight - height - iStart, SRCCOPY);
										retValue = height;
										if (!isCompress) {
											if (lpbi->bmiHeader.biBitCount == 1) {
												//memset(pBits, 0x0f, stride* height);
												npdisp_writeMemory(pBits, lpDIBitsAddr, stride * height);
											}
											else if (lpbi->bmiHeader.biBitCount == 8) {
												npdisp_writeMemory(pBits, lpDIBitsAddr, stride * height);
											}
											else {
												npdisp_writeMemory(pBits, lpDIBitsAddr, stride * height);
											}
										}
										else {
											// サポートしない
											//UINT8* cdata = (UINT8*)malloc(lpbi->bmiHeader.biSizeImage);
											//if (cdata) {

											//	npdisp_writeMemory(cdata, lpDIBitsAddr, lpbi->bmiHeader.biSizeImage);
											//	free(cdata);
											//}
										}
										//if (npdisp.usePalette) {
										//	if (lpbi->bmiHeader.biBitCount <= 8) {
										//		if (lpTranslateAddr) {
										//			int colors = (1 << lpbi->bmiHeader.biBitCount);
										//			if (lpbi->bmiHeader.biBitCount == 8) {
										//				for (i = 0; i < colors; i++) {
										//					int idx = lpbi->bmiColors[i].rgbRed;
										//					lpbi->bmiColors[i].rgbRed = npdisp_palette_rgb256[idx].r;
										//					lpbi->bmiColors[i].rgbGreen = npdisp_palette_rgb256[idx].g;
										//					lpbi->bmiColors[i].rgbBlue = npdisp_palette_rgb256[idx].b;
										//					lpbi->bmiColors[i].rgbReserved = 0;
										//				}
										//				npdisp_writeMemory(lpbi, lpBitmapInfoAddr, lpbiWriteLen); // 論理カラーを書き戻し
										//			}
										//			//else if (lpbi->bmiHeader.biBitCount == 4) {
										//			//	for (i = 0; i < colors; i++) {
										//			//		int idx = lpbi->bmiColors[i].rgbRed;
										//			//		lpbi->bmiColors[i].rgbRed = npdisp_palette_rgb16[idx].r;
										//			//		lpbi->bmiColors[i].rgbGreen = npdisp_palette_rgb16[idx].g;
										//			//		lpbi->bmiColors[i].rgbBlue = npdisp_palette_rgb16[idx].b;
										//			//		lpbi->bmiColors[i].rgbReserved = 0;
										//			//	}
										//			//	npdisp_writeMemory(lpbi, lpBitmapInfoAddr, lpbiWriteLen); // 論理カラーを書き戻し
										//			//}
										//		}
										//	}
										//}
									}
									else {
										// Set Bits
										if (isCompress && pBitsValid) {
											int y = biHeader.biHeight - height - iStart;
											// 有効範囲をマスク（RLEの未定義ピクセルは描画しない）
											HGDIOBJ hOldBmp2 = SelectObject(hdc, hBmpValid);
											BitBlt(tgtbmphdc.hdc, 0, biHeader.biHeight - height - iStart, biHeader.biWidth, height, hdc, 0, y, SRCAND);
											// 有効範囲を転送
											SelectObject(hdc, hOldBmp2);
											BitBlt(tgtbmphdc.hdc, 0, biHeader.biHeight - height - iStart, biHeader.biWidth, height, hdc, 0, y, SRCPAINT);
										}
										else {
											BitBlt(tgtbmphdc.hdc, 0, biHeader.biHeight - height - iStart, biHeader.biWidth, height, hdc, 0, 0, SRCCOPY);
										}

										retValue = height;
										npdisp_WriteBitmapToPBITMAP(&tgtPBmp, &tgtbmphdc, beginLine, height);
									}

									if (palChanged) {
										// 色を戻す
										SetDIBColorTable(tgtbmphdc.hdc, 0, 256, (RGBQUAD*)npdisp_palette_gray256);
									}
								}

								SelectObject(hdc, hOldBmp);
								DeleteObject(hBmp);
							}
							if (hBmpValid) {
								DeleteObject(hBmpValid);
							}
							npdisp_FreeBitmap(&tgtbmphdc);
						}
						if (transTbl) {
							free(transTbl);
						}
					}
					else {
						int i;
						// lpDIBitsがNULLの時、biSizeImageを設定する
						lpbi->bmiHeader.biSizeImage = stride * (biHeader.biHeight >= 0 ? biHeader.biHeight : -biHeader.biHeight);
						if (lpbi->bmiHeader.biBitCount == 16 || lpbi->bmiHeader.biBitCount == 15) {
							// ドキュメントに明言されていないが、16bitカラーの時はビットフィールドを返さないと駄目
							if (lpbi->bmiHeader.biCompression) {
								if (npdisp.bpp == 16 && lpbi->bmiHeader.biBitCount != 15) {
									// ビットフィールド 565
									lpbi->bmiHeader.biCompression = BI_BITFIELDS;
									*((DWORD*)(lpbi->bmiColors + 0)) = 0x0000F800;
									*((DWORD*)(lpbi->bmiColors + 1)) = 0x000007E0;
									*((DWORD*)(lpbi->bmiColors + 2)) = 0x0000001F;
								}
								else {
									// ビットフィールド 555
									lpbi->bmiHeader.biCompression = BI_BITFIELDS;
									*((DWORD*)(lpbi->bmiColors + 0)) = 0x00007C00;
									*((DWORD*)(lpbi->bmiColors + 1)) = 0x000003E0;
									*((DWORD*)(lpbi->bmiColors + 2)) = 0x0000001F;
									lpbi->bmiHeader.biBitCount = 16;
								}
							}
						}
						else if (lpbi->bmiHeader.biBitCount == 1) {
							// 2色パレットセット
							memcpy(lpbi->bmiColors, npdisp_palette_rgb2, sizeof(npdisp_palette_rgb2));
						}
						else if (lpbi->bmiHeader.biBitCount == 4) {
							// 16色パレットセット
							memcpy(lpbi->bmiColors, npdisp_palette_rgb16, sizeof(npdisp_palette_rgb16));
						}
						//else if (lpbi->bmiHeader.biBitCount == 8) {
						//	// 256色パレットセット
						//	if (npdisp.usePalette) {
						//		// パレット番号変換の上転送
						//		for (i = 0; i < NELEMENTS(npdisp_palette_rgb256); i++) {
						//			lpbi->bmiColors[i].rgbRed = npdisp_palette_rgb256[npdisp_palette_transTbl[i] & 0xff].r;
						//			lpbi->bmiColors[i].rgbGreen = npdisp_palette_rgb256[npdisp_palette_transTbl[i] & 0xff].g;
						//			lpbi->bmiColors[i].rgbBlue = npdisp_palette_rgb256[npdisp_palette_transTbl[i] & 0xff].b;
						//			lpbi->bmiColors[i].rgbReserved = 0;
						//		}
						//	}
						//	else {
						//		for (i = 0; i < NELEMENTS(npdisp_palette_rgb256); i++) {
						//			lpbi->bmiColors[i].rgbRed = npdisp_palette_rgb256[i].r;
						//			lpbi->bmiColors[i].rgbGreen = npdisp_palette_rgb256[i].g;
						//			lpbi->bmiColors[i].rgbBlue = npdisp_palette_rgb256[i].b;
						//			lpbi->bmiColors[i].rgbReserved = 0;
						//		}
						//	}
						//}
						else if (lpbi->bmiHeader.biBitCount == 32) {
							if (lpbi->bmiHeader.biCompression == BI_BITFIELDS) {
								// ビットフィールド 32bit
								*((DWORD*)(lpbi->bmiColors + 0)) = 0x00FF0000;
								*((DWORD*)(lpbi->bmiColors + 1)) = 0x0000FF00;
								*((DWORD*)(lpbi->bmiColors + 2)) = 0x000000FF;
							}
						}
						npdisp_writeMemory(lpbi, lpBitmapInfoAddr, lpbiReadLen);
						retValue = height; // ここで値を返すとアイコンキャッシュを使うようになる？
					}
					free(lpbi);
				}
				else {
					retValue = 0;
				}
			}
		}
	}
	return retValue;
}

static UINT32 npdisp_func_ExtTextOut(UINT32 lpDestDevAddr, SINT16 wDestXOrg, SINT16 wDestYOrg, UINT32 lpClipRectAddr, UINT32 lpStringAddr, SINT16 wCount, UINT32 lpFontInfoAddr, UINT32 lpDrawModeAddr, UINT32 lpTextXFormAddr, UINT32 lpCharWidthsAddr, UINT32 lpOpaqueRectAddr, UINT16 wOptions)
{
	UINT32 retValue = 0;
	UINT8* lpText;
	if (wCount != 0) {
		if (!lpStringAddr) return 0;
		lpText = (UINT8*)npdisp_readMemoryStringWithCount(lpStringAddr, wCount < 0 ? -wCount : wCount);
	}
	else {
		// ダミーをいれておく
		lpText = (UINT8*)malloc(1);
		lpText[0] = '\0';
	}
	if (lpText) {
		if (npdisp.longjmpnum == 0) {
			int i;
			RECT cliprect = { 0 };
			NPDISP_RECT rectTmp = { 0 };
			NPDISP_RECT opaquerect = { 0 };
			if (lpClipRectAddr) npdisp_readMemory(&rectTmp, lpClipRectAddr, sizeof(NPDISP_RECT));
			if (lpOpaqueRectAddr) npdisp_readMemory(&opaquerect, lpOpaqueRectAddr, sizeof(NPDISP_RECT));
			cliprect.top = rectTmp.top;
			cliprect.left = rectTmp.left;
			cliprect.bottom = rectTmp.bottom;
			cliprect.right = rectTmp.right;
			NPDISP_DRAWMODE drawMode = { 0 };
			int hasDrawMode = 0;
			if (lpDrawModeAddr) hasDrawMode = npdisp_readMemory(&drawMode, lpDrawModeAddr, sizeof(NPDISP_DRAWMODE));
			if (hasDrawMode) {
				npdisp_AdjustDrawModeColor(&drawMode);
			}
			else {
				drawMode.LbkColor = 0xffffff;
				drawMode.LTextColor = 0x000000;
			}
			//HGDIOBJ oldFont = SelectObject(tgtDC, npdispwin.hFont);
			NPDISP_FONTINFO fontInfo;
			if (npdisp_readMemory(&fontInfo, lpFontInfoAddr, sizeof(NPDISP_FONTINFO))) {
				SIZE sz = { 0, fontInfo.dfPixHeight };
				int maxCharWidth = 0;
				int loopLen = wCount >= 0 ? wCount : -wCount;
				SINT16* charWidths = NULL;
				SINT16* charRealWidths = NULL;
				UINT32* charOffsets = NULL;
				if (loopLen > 0) {
					charWidths = (SINT16*)malloc(loopLen * sizeof(SINT16));
					if (lpCharWidthsAddr) {
						npdisp_readMemory(charWidths, lpCharWidthsAddr, sizeof(SINT16) * loopLen);
					}
					charRealWidths = (SINT16*)malloc(loopLen * sizeof(SINT16));
					charOffsets = (UINT32*)malloc(loopLen * sizeof(UINT32));
				}
				if (loopLen == 0 || (charOffsets && charRealWidths && charOffsets)) {
					if (npdisp.longjmpnum == 0) {
						for (i = 0; i < loopLen; i++) {
							NPDISP_FONTCHARINFO3 charInfo;
							int charIdx = (int)lpText[i] - fontInfo.dfFirstChar;
							if (charIdx < 0 || fontInfo.dfLastChar < charIdx) {
								charIdx = fontInfo.dfDefaultChar;
							}
							int charWidth = 0;
							if (lpCharWidthsAddr) {
								charWidth = charWidths[i];
								if (npdisp_readMemory(&charInfo, lpFontInfoAddr + sizeof(NPDISP_FONTINFO) + sizeof(NPDISP_FONTCHARINFO3) * charIdx, sizeof(NPDISP_FONTCHARINFO3))) {
									charOffsets[i] = charInfo.offset;
									charRealWidths[i] = charInfo.width;
									if (i == loopLen - 1) {
										// 最後の文字は実際の描画幅分を確保
										if (charInfo.width > charWidth) {
											charWidth = charInfo.width;
										}
									}
								}
							}
							else {
								if (npdisp_readMemory(&charInfo, lpFontInfoAddr + sizeof(NPDISP_FONTINFO) + sizeof(NPDISP_FONTCHARINFO3) * charIdx, sizeof(NPDISP_FONTCHARINFO3))) {
									charWidth = charInfo.width;
									charOffsets[i] = charInfo.offset;
									charRealWidths[i] = charInfo.width;
								}
							}
							charWidths[i] = charWidth;
							if (charWidth > maxCharWidth) maxCharWidth = charWidth;
							if (charRealWidths[i] > maxCharWidth) maxCharWidth = charRealWidths[i];
							sz.cx += charWidth;
							if (npdisp.longjmpnum != 0) break;
						}
						retValue = ((UINT32)sz.cx) | ((UINT32)sz.cy << 16);
					}
					if (npdisp.longjmpnum != 0 && sz.cy > npdisp.height) {
						// ページフォールト or 高さ異常
					}
					else if (wCount < 0) {
						// nothing to do
					}
					else if (wCount == 0) {
						// 塗りつぶし
						TRACEOUT(("-> FILL"));
						if (wOptions & 2) {
							NPDISP_PBITMAP_EXT dstPBmp;
							NPDISP_WINDOWS_BMPHDC bmphdc = { 0 };
							int dstDevType = 0;
							HDC tgtDC = npdispwin.hdc;
							npdisp_readMemory(&dstDevType, lpDestDevAddr, 2);
							if (!npdisp_isDisplayDevice(lpDestDevAddr)) {
								// memory 
								if (lpDestDevAddr && npdisp_readPBitmap(&dstPBmp, lpDestDevAddr)) {
									if (npdisp_MakeBitmapFromPBITMAP(&dstPBmp, &bmphdc, 0)) {
										tgtDC = bmphdc.hdc;
									}
								}
							}
							if (npdisp.longjmpnum == 0) {
								if (lpOpaqueRectAddr && (wOptions & 2)) {
									int dstLeft = opaquerect.left;
									int dstTop = opaquerect.top;
									int dstRight = opaquerect.right;
									int dstBottom = opaquerect.bottom;

									if (lpClipRectAddr) {
										dstLeft = max(dstLeft, cliprect.left);
										dstTop = max(dstTop, cliprect.top);
										dstRight = min(dstRight, cliprect.right);
										dstBottom = min(dstBottom, cliprect.bottom);
									}

									int dstX = dstLeft;
									int dstY = dstTop;

									int srcX = dstLeft - wDestXOrg;
									int srcY = dstTop - wDestYOrg;
									int srcW = dstRight - dstLeft;
									int srcH = dstBottom - dstTop;

									if (srcW > 0 && srcH > 0) {
										HBRUSH hBrush;
										bool isBlack = false;
										bool isWhite = false;
										bool preferDither;
										if ((drawMode.bkColor & 0xffffff) == 0) {
											// 黒で確定
											isBlack = true;
										}
										else if ((drawMode.bkColor & 0xffffff) == 0xffffff) {
											// 白で確定
											isWhite = true;
										}
										else {
											UINT32 color = npdisp_AdjustColorRefForGDI(drawMode.bkColor, &preferDither);
											if (!preferDither) {
												// 純色
												if (color == 0) {
													isBlack = true;
												}
												else if ((color & 0xffffff) == 0xffffff) {
													isWhite = true;
												}
												else {
													hBrush = CreateSolidBrush(color);
												}
											}
											else {
												// ディザ
												UINT32 actualColor1;
												UINT32 actualColor2;
												double ratio;
												MakePaletteDitherBrushColor(color, &actualColor1, &actualColor2, &ratio);
												hBrush = CreatePaletteDitherBrush(actualColor1, actualColor2, ratio);
											}
										}
										RECT gdiopaquerect = { dstX, dstY, dstX + srcW, dstY + srcH };
										if (isBlack) {
											PatBlt(tgtDC, dstX, dstY, srcW, srcH, BLACKNESS);
										}
										else if (isWhite) {
											PatBlt(tgtDC, dstX, dstY, srcW, srcH, WHITENESS);
										}
										else {
											HGDIOBJ oldBrush = SelectObject(tgtDC, hBrush);
											SetBkMode(tgtDC, OPAQUE);
											SetROP2(tgtDC, drawMode.Rop2);
											FillRect(tgtDC, &gdiopaquerect, hBrush);
											//PatBlt(tgtDC, opaquerect.left, opaquerect.top, opaquerect.right - opaquerect.left, opaquerect.bottom - opaquerect.top, drawMode.Rop2);
											//Rectangle(tgtDC, opaquerect.left, opaquerect.top, opaquerect.right, opaquerect.bottom);
											SelectObject(tgtDC, oldBrush);
											DeleteObject(hBrush);
										}
										if (bmphdc.hdc) {
											// 書き戻し
											npdisp_WriteBitmapToPBITMAP(&dstPBmp, &bmphdc);
										}
										else {
											npdisp_setDirty(gdiopaquerect.left, gdiopaquerect.top, gdiopaquerect.right, gdiopaquerect.bottom);
											npdisp.updated = 1;
										}
									}
								}
							}
							if (bmphdc.hdc) {
								npdisp_FreeBitmap(&bmphdc);
							}
						}
					}
					else {
						// 描画
						TRACEOUT(("-> TEXT"));

						BITMAPINFO* lpbi = (BITMAPINFO*)malloc(sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * 2);
						if (lpbi) {
							HDC hdcText = npdispwin.hdcCache[1];
							int ddbWidth = sz.cx;
							int stride = ((ddbWidth + 7) / 8 + 1) / 2 * 2;
							int stride4 = ((ddbWidth + 31) / 32) * 4;
							if (stride != stride4) {
								ddbWidth += 16; // 16dot = 2byte増やすことで4バイトアライメント強制する
								stride = stride4;
							}
							void* pBits = (char*)malloc(stride * sz.cy);
							if (pBits) {
								HGDIOBJ hbmpOld;
								memset(pBits, 0x00, stride * sz.cy);
								int maxCharXLen = (maxCharWidth + 7) / 8;
								UINT8* srcBuf = (UINT8*)malloc(maxCharXLen * sz.cy);
								if (srcBuf) {
									// てんそう
									int baseXbyte = 0;
									int baseXbit = 0;
									for (i = 0; i < loopLen; i++) {
										int charIdx = (int)lpText[i] - fontInfo.dfFirstChar;
										if (charIdx < 0 || fontInfo.dfLastChar < charIdx) {
											charIdx = fontInfo.dfDefaultChar;
										}
										int y, x;
										int charXLen = (charRealWidths[i] + 7) / 8;
										npdisp_readMemoryWith32Offset(srcBuf, fontInfo.dfBitsPointer >> 16, charOffsets[i], charXLen * sz.cy);
										for (x = 0; x < charXLen; x++) {
											int curWidth = charRealWidths[i] - x * 8;
											if (curWidth > 8) curWidth = 8;
											int bitMask = ((1 << curWidth) - 1) << (8 - curWidth);
											int dstBitMask1 = (bitMask >> baseXbit) & 0xff;
											int dstBitMask2 = (bitMask << (8 - baseXbit)) & 0xff;
											char* buf = (char*)pBits + baseXbyte + x;
											for (y = 0; y < sz.cy; y++) {
												UINT8 data = srcBuf[x * sz.cy + y] & bitMask;
												UINT8 data1 = (data >> baseXbit) & 0xff;
												UINT8 data2 = (data << (8 - baseXbit)) & 0xff;
												//*buf = (*buf & ~dstBitMask1) | (data1 & dstBitMask1);
												*buf |= data1 & dstBitMask1;
												if (dstBitMask2) {
													//*(buf + 1) = (*(buf + 1) & ~dstBitMask2) | (data2 & dstBitMask2);
													*(buf + 1) |= data2 & dstBitMask2;
												}
												buf += stride;
											}
										}
										baseXbyte += (baseXbit + charWidths[i]) / 8;
										baseXbit = (baseXbit + charWidths[i]) % 8;
									}
									free(srcBuf);
								}
								if (npdisp.longjmpnum == 0) {
									NPDISP_PBITMAP_EXT dstPBmp;
									NPDISP_WINDOWS_BMPHDC bmphdc = { 0 };
									HDC tgtDC = npdispwin.hdc;
									if (!npdisp_isDisplayDevice(lpDestDevAddr)) {
										// memory 
										if (lpDestDevAddr && npdisp_readPBitmap(&dstPBmp, lpDestDevAddr)) {
											if (npdisp_MakeBitmapFromPBITMAP(&dstPBmp, &bmphdc, 0)) {
												tgtDC = bmphdc.hdc;
											}
										}
									}
									if (npdisp.longjmpnum == 0) {
										bool hasClipRect = lpClipRectAddr != 0;
										if (lpOpaqueRectAddr && (wOptions & 2)) {
											int dstLeft = opaquerect.left;
											int dstTop = opaquerect.top;
											int dstRight = opaquerect.right;
											int dstBottom = opaquerect.bottom;

											if (lpClipRectAddr) {
												dstLeft = max(dstLeft, cliprect.left);
												dstTop = max(dstTop, cliprect.top);
												dstRight = min(dstRight, cliprect.right);
												dstBottom = min(dstBottom, cliprect.bottom);
											}

											int dstX = dstLeft;
											int dstY = dstTop;

											int srcX = dstLeft - wDestXOrg;
											int srcY = dstTop - wDestYOrg;
											int srcW = dstRight - dstLeft;
											int srcH = dstBottom - dstTop;

											if (srcW > 0 && srcH > 0) {
												HBRUSH hBrush;
												bool isBlack = false;
												bool isWhite = false;
												bool preferDither;
												if ((drawMode.bkColor & 0xffffff) == 0) {
													// 黒で確定
													isBlack = true;
												}
												else if ((drawMode.bkColor & 0xffffff) == 0xffffff) {
													// 白で確定
													isWhite = true;
												}
												else {
													UINT32 color = npdisp_AdjustColorRefForGDI(drawMode.bkColor, &preferDither);
													if (!preferDither) {
														// 純色
														if (color == 0) {
															isBlack = true;
														}
														else if ((color & 0xffffff) == 0xffffff) {
															isWhite = true;
														}
														else {
															hBrush = CreateSolidBrush(color);
														}
													}
													else {
														// ディザ
														UINT32 actualColor1;
														UINT32 actualColor2;
														double ratio;
														MakePaletteDitherBrushColor(color, &actualColor1, &actualColor2, &ratio);
														hBrush = CreatePaletteDitherBrush(actualColor1, actualColor2, ratio);
													}
												}
												TRACEOUT(("-> HAS BACKGROUND"));

												RECT gdiopaquerect = { dstX, dstY, dstX + srcW, dstY + srcH };
												if (isBlack) {
													PatBlt(tgtDC, dstX, dstY, srcW, srcH, BLACKNESS);
												}
												else if (isWhite) {
													PatBlt(tgtDC, dstX, dstY, srcW, srcH, WHITENESS);
												}
												else {
													FillRect(tgtDC, &gdiopaquerect, hBrush);
													DeleteObject(hBrush);
												}

												npdisp_setDirty(dstX, dstY, dstX + srcW, dstY + srcH);

											}
										}
										{
											int dstLeft = wDestXOrg;
											int dstTop = wDestYOrg;
											int dstRight = wDestXOrg + sz.cx;
											int dstBottom = wDestYOrg + sz.cy;

											if (hasClipRect) {
												dstLeft = max(dstLeft, cliprect.left);
												dstTop = max(dstTop, cliprect.top);
												dstRight = min(dstRight, cliprect.right);
												dstBottom = min(dstBottom, cliprect.bottom);
											}

											if (lpOpaqueRectAddr && (wOptions & 2)) {
												dstLeft = max(dstLeft, opaquerect.left);
												dstTop = max(dstTop, opaquerect.top);
												dstRight = min(dstRight, opaquerect.right);
												dstBottom = min(dstBottom, opaquerect.bottom);
											}

											int dstX = dstLeft;
											int dstY = dstTop;

											int srcX = dstLeft - wDestXOrg;
											int srcY = dstTop - wDestYOrg;
											int srcW = dstRight - dstLeft;
											int srcH = dstBottom - dstTop;

											if (srcW > 0 && srcH > 0) {

												//SetROP2(tgtDC, R2_COPYPEN);
												bool isTransparentBk = (drawMode.bkMode == 1 || drawMode.bkMode == 4);
												if (!isTransparentBk) {
													bool preferDither;
													UINT32 color = npdisp_AdjustColorRefForGDI(drawMode.bkColor, &preferDither);
													if (preferDither) {
														// ディザ背景
														UINT32 actualColor1;
														UINT32 actualColor2;
														double ratio;
														MakePaletteDitherBrushColor(color, &actualColor1, &actualColor2, &ratio);
														HBRUSH hBrush = CreatePaletteDitherBrush(actualColor1, actualColor2, ratio);
														HGDIOBJ oldBrush = SelectObject(tgtDC, hBrush);
														RECT gdiopaquerect = { dstX, dstY, dstX + srcW, dstY + srcH };
														FillRect(tgtDC, &gdiopaquerect, hBrush);
														SelectObject(tgtDC, oldBrush);
														DeleteObject(hBrush);
														// 背景はもう描いたので背景透過扱いにする
														isTransparentBk = true;
													}
												}
												if (isTransparentBk) {
													// 背景透過 ROPが要るのでDDB経由
													TRACEOUT(("FG:%08x BG:TRANS", drawMode.LTextColor));
													HBITMAP hBmp = CreateBitmap(ddbWidth, sz.cy, 1, 1, pBits);
													if (hBmp) {
														HGDIOBJ hbmpOld;
														hbmpOld = SelectObject(hdcText, hBmp);

														SetBkMode(tgtDC, OPAQUE);
														SetBkColor(tgtDC, 0x000000);
														SetTextColor(tgtDC, 0xffffff);
														BitBlt(tgtDC, dstX, dstY, srcW, srcH, hdcText, srcX, srcY, SRCAND);
														SetBkColor(tgtDC, drawMode.LTextColor);
														SetTextColor(tgtDC, 0x000000);
														BitBlt(tgtDC, dstX, dstY, srcW, srcH, hdcText, srcX, srcY, SRCPAINT);

														SelectObject(hdcText, hbmpOld);
														DeleteObject(hBmp);
													}
												}
												else if (drawMode.bkMode == 2) {
													// 背景不透明 SetDIBitsToDeviceを使う
													TRACEOUT(("FG:%08x BG:%08x", drawMode.LTextColor, drawMode.LbkColor));

													BITMAPINFO_1BPP biText;
													biText.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
													biText.bmiHeader.biWidth = ddbWidth;
													biText.bmiHeader.biHeight = -sz.cy;
													biText.bmiHeader.biPlanes = 1;
													biText.bmiHeader.biBitCount = 1;
													biText.bmiHeader.biCompression = BI_RGB;
													biText.bmiHeader.biSizeImage = 0;
													biText.bmiHeader.biXPelsPerMeter = 0;
													biText.bmiHeader.biYPelsPerMeter = 0;
													biText.bmiHeader.biClrUsed = 2;
													biText.bmiHeader.biClrImportant = 2;
													biText.bmiColors[0].rgbRed = drawMode.LbkColor & 0xff;
													biText.bmiColors[0].rgbGreen = (drawMode.LbkColor >> 8) & 0xff;
													biText.bmiColors[0].rgbBlue = (drawMode.LbkColor >> 16) & 0xff;
													biText.bmiColors[0].rgbReserved = 0;
													biText.bmiColors[1].rgbRed = drawMode.LTextColor & 0xff;
													biText.bmiColors[1].rgbGreen = (drawMode.LTextColor >> 8) & 0xff;
													biText.bmiColors[1].rgbBlue = (drawMode.LTextColor >> 16) & 0xff;
													biText.bmiColors[1].rgbReserved = 0;

													SetDIBitsToDevice(tgtDC, dstX, dstY, srcW, srcH, srcX, 0, 0, srcH, (UINT8*)pBits + srcY * stride, (BITMAPINFO*)&biText, DIB_RGB_COLORS);
												}
												if (bmphdc.hdc) {
													// 書き戻し
													npdisp_WriteBitmapToPBITMAP(&dstPBmp, &bmphdc);
												}
												else {
													memset(&npdispwin.lastScreenDrawMode, 0, sizeof(NPDISP_DRAWMODE));
													npdisp_setDirty(dstX, dstY, dstX + srcW, dstY + srcH);
													npdisp.updated = 1;
												}
											}
										}
									}
									if (bmphdc.hdc) {
										npdisp_FreeBitmap(&bmphdc);
									}
								}
								free(pBits);
							}
							free(lpbi);
						}
					}
				}
				if (charWidths) {
					free(charWidths);
				}
				if (charRealWidths) {
					free(charRealWidths);
				}
				if (charOffsets) {
					free(charOffsets);
				}
			}
		}
		//SelectObject(npdispwin.hdc, oldFont);
		free(lpText);
	}
	return retValue;
}

static UINT16 npdisp_func_SetDIBitsToDevice(UINT32 lpDestDevAddr, SINT16 X, SINT16 Y, UINT16 iScan ,UINT16 cScans, UINT32 lpClipRectAddr, UINT32 lpDrawModeAddr, UINT32 lpDIBitsAddr, UINT32 lpBitmapInfoAddr, UINT32 lpTranslateAddr)
{
	int dstDevType = 0;
	UINT16* transTbl = NULL;
	if (lpTranslateAddr && npdisp.bpp <= 8) {
		int colors = (1 << npdisp.bpp);
		UINT8* transTbl8 = (UINT8*)malloc(colors);
		if (transTbl8) {
			if (npdisp_readMemory(transTbl8, lpTranslateAddr, colors)) {
				transTbl = (UINT16*)malloc(colors * sizeof(UINT16));
				if (transTbl) {
					for (int i = 0; i < colors; i++) {
						transTbl[i] = transTbl8[i];
					}
				}
			}
			free(transTbl8);
		}
	}
	UINT16 retValue = 0;
	if (lpDestDevAddr) {
		if (npdisp_readMemory(&dstDevType, lpDestDevAddr, 2)) {
			BITMAPINFOHEADER biHeader = { 0 };
			if (npdisp_readMemory(&biHeader, lpBitmapInfoAddr, sizeof(BITMAPINFOHEADER))) {
				int stride = ((biHeader.biWidth * biHeader.biBitCount + 31) / 32) * 4;
				int height = biHeader.biHeight >= 0 ? biHeader.biHeight : -biHeader.biHeight;
				int lpbiLen = 0;
				int lpbiReadLen = 0;
				if (biHeader.biBitCount <= 8) {
					lpbiLen = sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * (1 << biHeader.biBitCount);
					if (npdisp.usePalette) {
						lpbiReadLen = sizeof(BITMAPINFOHEADER) + sizeof(UINT16) * (1 << biHeader.biBitCount);
					}
					else {
						lpbiReadLen = lpbiLen;
					}
				}
				else if ((biHeader.biBitCount == 15 || biHeader.biBitCount == 16 || biHeader.biBitCount == 32) && biHeader.biCompression == BI_BITFIELDS) {
					lpbiReadLen = lpbiLen = sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * 3;
				}
				else {
					lpbiReadLen = lpbiLen = sizeof(BITMAPINFOHEADER);
				}
				int iScanHost = (int)iScan;
				int cScansHost = (int)cScans;
				RECT cliprect = { 0, 0, npdisp.width, npdisp.height };
				if (lpClipRectAddr) {
					NPDISP_RECT rectTmp = { 0 };
					npdisp_readMemory(&rectTmp, lpClipRectAddr, sizeof(NPDISP_RECT));
					cliprect.top = rectTmp.top;
					cliprect.left = rectTmp.left;
					cliprect.bottom = rectTmp.bottom;
					cliprect.right = rectTmp.right;
					if (cliprect.top > cliprect.bottom) {
						int tmp = cliprect.top;
						cliprect.top = cliprect.bottom;
						cliprect.bottom = tmp;
					}
				}

				// クリップ領域だけ転送
				const int clipYBegin = cliprect.top - Y;
				const int clipYEnd = cliprect.bottom - Y;
				int iScanTmp = (int)iScanHost;
				int cScansTmp = (int)cScansHost;
				if (biHeader.biHeight >= 0) {
					// bottom-up -> top-down
					iScanTmp = biHeader.biHeight - (int)iScanHost - (int)cScansHost;
					cScansTmp = (int)cScansHost;
				}
				if (iScanTmp < clipYBegin) {
					cScansTmp -= clipYBegin - (int)iScanTmp;
					iScanTmp = clipYBegin;
				}
				if (clipYEnd <= iScanTmp) {
					cScansTmp = 0;
				}
				else {
					if (clipYEnd < iScanTmp + cScansTmp) {
						cScansTmp = clipYEnd - iScanTmp;
					}
				}
				if (biHeader.biHeight >= 0) {
					// top-down -> bottom-up
					iScanHost = (UINT16)(biHeader.biHeight - iScanTmp - cScansTmp);
					cScansHost = (UINT16)cScansTmp;
				}

				if (cScansHost <= 0) {
					retValue = cScans;
				}
				else if (iScanHost < height && cScansHost > 0) {
					BITMAPINFO* lpbi;
					lpbi = (BITMAPINFO*)malloc(lpbiLen);
					if (lpbi) {
						npdisp_readMemory(lpbi, lpBitmapInfoAddr, lpbiReadLen);
						UINT32 biCompression = lpbi->bmiHeader.biCompression;
						if (lpbi->bmiHeader.biCompression != BI_BITFIELDS) {
							lpbi->bmiHeader.biCompression = BI_RGB;
						}
						bool isCompress = !(biCompression == BI_RGB || biCompression == BI_BITFIELDS);
						UINT8* pBitsBase;
						void* pBitsValid = NULL;
						if (!isCompress) {
							pBitsBase = (UINT8*)malloc(cScansHost * stride);
						}
						else {
							int fullHeight = biHeader.biHeight;
							if (fullHeight < 0) fullHeight = -fullHeight;
							pBitsBase = (UINT8*)malloc(fullHeight * stride);
							pBitsValid = (UINT8*)malloc(fullHeight * stride);
						}
						if (pBitsBase) {
							UINT8* pBits;
							if (!isCompress) {
								npdisp_readMemoryWith32Offset(pBitsBase, lpDIBitsAddr >> 16, (lpDIBitsAddr & 0xffff) + (iScanHost - (int)iScan) * stride, cScansHost * stride);
								pBits = pBitsBase;
							}
							else {
								UINT32 rleSize = lpbi->bmiHeader.biSizeImage;
								if (rleSize == 0) {
									rleSize = stride * height; // とりあえず無圧縮のサイズを確保
								}
								UINT8* cdata = (UINT8*)malloc(rleSize);
								if (cdata) {
									BITMAPINFOHEADER bmiHeaderRLE = lpbi->bmiHeader;
									bmiHeaderRLE.biCompression = biCompression;
									if (bmiHeaderRLE.biHeight > 0) {
										bmiHeaderRLE.biHeight = -bmiHeaderRLE.biHeight; // 逆さで出力
									}
									if (lpbi->bmiHeader.biSizeImage != 0) {
										npdisp_readMemory(cdata, lpDIBitsAddr, rleSize);
									}
									else {
										// RLE終端が来るまで読む 最大読み取りサイズは無圧縮サイズとする
										rleSize = npdisp_RLEBMPReadAndCalcSize(lpDIBitsAddr, bmiHeaderRLE.biCompression, cdata, rleSize);
									}
									npdisp_DecompressRLEBMP(&bmiHeaderRLE, cdata, rleSize, (UINT8*)pBitsBase, (UINT8*)pBitsValid);
									free(cdata);
								}
								pBits = pBitsBase + (iScanHost - (int)iScan) * stride; // 開始アドレス分をずらす
							}
							if (npdisp.longjmpnum == 0) {
								HDC tgtDC = npdispwin.hdc;
								if (height > iScanHost) {
									int i;
									NPDISP_PBITMAP_EXT dstPBmp;
									NPDISP_WINDOWS_BMPHDC bmphdc = { 0 };
									if (!npdisp_isDisplayDevice(lpDestDevAddr)) {
										// memory 
										if (lpDestDevAddr && npdisp_readPBitmap(&dstPBmp, lpDestDevAddr)) {
											npdisp_PreloadBitmapFromPBITMAP(&dstPBmp, 0, Y + iScanHost, cScansHost, X, biHeader.biWidth);
											if (npdisp_MakeBitmapFromPBITMAP(&dstPBmp, &bmphdc, 0, Y + iScanHost, cScansHost, X, biHeader.biWidth)) {
												npdisp_ConvertToDDBMonoBitmap(&bmphdc);
												tgtDC = bmphdc.hdc;
											}
										}
									}
									if (npdisp.longjmpnum == 0) {
										if (npdisp.usePalette) {
											if (lpbi->bmiHeader.biBitCount <= 8) {
												int colors = (1 << lpbi->bmiHeader.biBitCount);
												UINT16 palTrans[256];
												memcpy(palTrans, lpbi->bmiColors, colors * sizeof(UINT16));
												for (i = 0; i < colors; i++) {
													lpbi->bmiColors[i].rgbRed = palTrans[i] & 0xff;
													lpbi->bmiColors[i].rgbGreen = palTrans[i] & 0xff;
													lpbi->bmiColors[i].rgbBlue = palTrans[i] & 0xff;
													lpbi->bmiColors[i].rgbReserved = 0;
													lpbi->bmiColors[i].rgbReserved = 0;
												}
											}
										}
										else {
											if (lpbi->bmiHeader.biBitCount == 1) {
												// 2色パレットセット
												for (i = 0; i < NELEMENTS(npdisp_palette_rgb2); i++) {
													lpbi->bmiColors[i].rgbRed = npdisp_palette_rgb2[i].r;
													lpbi->bmiColors[i].rgbGreen = npdisp_palette_rgb2[i].g;
													lpbi->bmiColors[i].rgbBlue = npdisp_palette_rgb2[i].b;
													lpbi->bmiColors[i].rgbReserved = 0;
												}
											}
											else if (lpbi->bmiHeader.biBitCount == 4) {
												if (lpbi->bmiHeader.biClrUsed == 0 || lpbi->bmiHeader.biClrImportant == 0) {
													// 有効なパレットでなければ16色パレットセット
													if (lpbi->bmiColors[0].rgbReserved != 0 || lpbi->bmiColors[15].rgbReserved != 0) {
														for (i = 0; i < NELEMENTS(npdisp_palette_rgb16); i++) {
															lpbi->bmiColors[i].rgbRed = npdisp_palette_rgb16[i].r;
															lpbi->bmiColors[i].rgbGreen = npdisp_palette_rgb16[i].g;
															lpbi->bmiColors[i].rgbBlue = npdisp_palette_rgb16[i].b;
															lpbi->bmiColors[i].rgbReserved = 0;
														}
													}
												}
											}
										}
										NPDISP_DRAWMODE drawMode = { 0 };
										if (npdisp_readMemory(&drawMode, lpDrawModeAddr, sizeof(NPDISP_DRAWMODE))) {
											if (tgtDC != npdispwin.hdc || memcmp(&drawMode, &npdispwin.lastScreenDrawMode, sizeof(NPDISP_DRAWMODE)) != 0) {
												if (tgtDC == npdispwin.hdc) npdispwin.lastScreenDrawMode = drawMode;
												npdisp_AdjustDrawModeColor(&drawMode);
												SetBkColor(tgtDC, drawMode.LbkColor);
												SetTextColor(tgtDC, drawMode.LTextColor);
												SetBkMode(tgtDC, drawMode.bkMode);
												SetROP2(tgtDC, drawMode.Rop2);
											}
											if (lpbi->bmiHeader.biBitCount == 1) {
												// モノクロ特例
												lpbi->bmiColors[0].rgbRed = drawMode.LTextColor & 0xff;
												lpbi->bmiColors[0].rgbGreen = (drawMode.LTextColor >> 8) & 0xff;
												lpbi->bmiColors[0].rgbBlue = (drawMode.LTextColor >> 16) & 0xff;
												lpbi->bmiColors[1].rgbRed = drawMode.LbkColor & 0xff;
												lpbi->bmiColors[1].rgbGreen = (drawMode.LbkColor >> 8) & 0xff;
												lpbi->bmiColors[1].rgbBlue = (drawMode.LbkColor >> 16) & 0xff;
											}
										}
										HRGN hRgn = NULL;
										if (lpClipRectAddr) {
											hRgn = CreateRectRgn(cliprect.left, cliprect.top, cliprect.right, cliprect.bottom);
											SelectClipRgn(tgtDC, hRgn);
										}

										//if (iScanHost + cScansHost > height) {
										//	cScansHost = height - iScanHost;
										//}

										bool palChanged = false;
										if (npdisp.usePalette) {
											if (lpbi->bmiHeader.biBitCount > 8) {
												// グレースケールから実際のデバイス色へ置き換え
												if (npdisp.bpp == 8) {
													RGBQUAD pal[256];
													for (int i = 0; i < 256; i++) {
														pal[i].rgbRed = npdisp_palette_rgb256[i].r;
														pal[i].rgbGreen = npdisp_palette_rgb256[i].g;
														pal[i].rgbBlue = npdisp_palette_rgb256[i].b;
														pal[i].rgbReserved = 0;
													}
													SetDIBColorTable(tgtDC, 0, 256, pal);
													palChanged = true;
												}
											}
										}

										if (isCompress && pBitsValid) {
											int colors = (1 << lpbi->bmiHeader.biBitCount);
											int oldColorUsed = lpbi->bmiHeader.biClrUsed;
											RGBQUAD oldpal0 = lpbi->bmiColors[0];
											RGBQUAD oldpalf = lpbi->bmiColors[colors - 1];

											// 有効範囲をマスク（RLEの未定義ピクセルは描画しない）
											lpbi->bmiHeader.biClrUsed = colors;
											lpbi->bmiColors[0].rgbRed = lpbi->bmiColors[0].rgbGreen = lpbi->bmiColors[0].rgbBlue = lpbi->bmiColors[0].rgbReserved = 0x00;
											lpbi->bmiColors[colors - 1].rgbRed = lpbi->bmiColors[colors - 1].rgbGreen = lpbi->bmiColors[colors - 1].rgbBlue = lpbi->bmiColors[colors - 1].rgbReserved = 0xff;
											if (SetDIBitsToDevice(npdispwin.hdcBltBuf, X, Y, biHeader.biWidth, height, 0, 0,
												iScanHost, cScansHost, pBitsValid, lpbi, DIB_RGB_COLORS) == 0) {
												TRACEOUTF(("ERROR"));
											}
											BitBlt(tgtDC, X, Y, biHeader.biWidth, height, npdispwin.hdcBltBuf, X, Y, SRCAND);

											// 有効範囲を転送
											lpbi->bmiHeader.biClrUsed = oldColorUsed;
											lpbi->bmiColors[0] = oldpal0;
											lpbi->bmiColors[colors - 1] = oldpalf;
											if (SetDIBitsToDevice(npdispwin.hdcBltBuf, X, Y, biHeader.biWidth, height, 0, 0,
												iScanHost, cScansHost, pBits, lpbi, DIB_RGB_COLORS) == 0) {
												TRACEOUTF(("ERROR"));
											}
											BitBlt(tgtDC, X, Y, biHeader.biWidth, height, npdispwin.hdcBltBuf, X, Y, SRCPAINT);
										}
										else {
											if (SetDIBitsToDevice(tgtDC, X, Y, biHeader.biWidth, height, 0, 0,
												iScanHost, cScansHost, pBits, lpbi, DIB_RGB_COLORS) == 0) {
												TRACEOUTF(("ERROR"));
											}
										}

										if (hRgn) {
											SelectClipRgn(tgtDC, NULL);
											DeleteObject(hRgn);
										}

										if (palChanged) {
											// 色を戻す
											SetDIBColorTable(tgtDC, 0, 256, (RGBQUAD*)npdisp_palette_gray256);
										}

										if (bmphdc.hBmp) {
											npdisp_WriteBitmapToPBITMAP(&dstPBmp, &bmphdc, Y + iScanHost, cScansHost, X, biHeader.biWidth);
											npdisp_FreeBitmap(&bmphdc);
										}
										else {
											if (biHeader.biHeight >= 0) {
												// bottom-up
												npdisp_setDirty(X, Y + biHeader.biHeight - iScanHost - cScansHost, X + biHeader.biWidth, Y + biHeader.biHeight - iScanHost);
												//HGDIOBJ oldPen = SelectObject(npdispwin.hdc, GetStockObject(BLACK_PEN));
												//HGDIOBJ oldBrush = SelectObject(npdispwin.hdc, GetStockObject(NULL_BRUSH));
												//Rectangle(npdispwin.hdc, X, Y + biHeader.biHeight - iScanHost - cScansHost, X + biHeader.biWidth, Y + biHeader.biHeight - iScanHost);
												//SelectObject(npdispwin.hdc, oldPen);
												//SelectObject(npdispwin.hdc, oldBrush);
											}
											else {
												npdisp_setDirty(X, Y + iScanHost, X + biHeader.biWidth, Y + iScanHost + cScansHost);
												//HGDIOBJ oldPen = SelectObject(npdispwin.hdc, GetStockObject(BLACK_PEN));
												//HGDIOBJ oldBrush = SelectObject(npdispwin.hdc, GetStockObject(NULL_BRUSH));
												//Rectangle(npdispwin.hdc, X, Y + iScanHost, X + biHeader.biWidth, Y + iScanHost + cScansHost);
												//SelectObject(npdispwin.hdc, oldPen);
												//SelectObject(npdispwin.hdc, oldBrush);
											}

											npdisp.updated = 1;
										}

										retValue = cScans;
									}
									else {
										if (bmphdc.hBmp) {
											npdisp_FreeBitmap(&bmphdc);
										}
									}
								}
							}
							free(pBitsBase);
						}
						if (pBitsValid) {
							free(pBitsValid);
						}
						free(lpbi);
					}
				}
			}
		}
	}
	if (transTbl) {
		free(transTbl);
	}
	return retValue;
}

static UINT16 npdisp_func_SaveScreenBitmap(UINT32 lpRect, UINT16 wCommand)
{
	UINT16 retValue = 0;
	NPDISP_RECT rect = { 0 };
	if (npdisp_readMemory(&rect, lpRect, sizeof(rect)) && npdisp.longjmpnum == 0) {
		if (wCommand == 0) {
			BitBlt(npdispwin.hdcShadow, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, npdispwin.hdc, rect.left, rect.top, SRCCOPY);
			npdispwin.rectShadow.left = rect.left;
			npdispwin.rectShadow.right = rect.right;
			npdispwin.rectShadow.top = rect.top;
			npdispwin.rectShadow.bottom = rect.bottom;
			retValue = 1;
		}
		else if (wCommand == 1) {
			if (npdispwin.rectShadow.left == rect.left && npdispwin.rectShadow.right == rect.right && npdispwin.rectShadow.top == rect.top && npdispwin.rectShadow.bottom == rect.bottom) {
				BitBlt(npdispwin.hdc, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, npdispwin.hdcShadow, rect.left, rect.top, SRCCOPY);
				npdisp_setDirty(rect.left, rect.top, rect.right, rect.bottom);
				npdisp.updated = 1;
				retValue = 1;
			}
			else {
				retValue = 0;
			}
		}
		else if (wCommand == 2) {
			BitBlt(npdispwin.hdcShadow, 0, 0, npdisp.width, npdisp.height, NULL, 0, 0, BLACKNESS);
			npdispwin.rectShadow.left = 0;
			npdispwin.rectShadow.right = 0;
			npdispwin.rectShadow.top = 0;
			npdispwin.rectShadow.bottom = 0;
			retValue = 1;
		}
	}
	return retValue;
}

static void npdisp_func_SetCursor_Make(void** ppBitsCursor, HBITMAP* phBmpCursor, HBITMAP* phOldBmpCursor, HDC hdcCursor, UINT32 pixelBufAddr, int width, int height, int srcStride, int dstStride, int srcBpp, int dstBpp) {
	HBITMAP hBmp = NULL;
	void* pBitsCursor = NULL;
	if (npdisp.cursorStride == dstStride && *ppBitsCursor && *phBmpCursor) {
		// 既存の流用
		pBitsCursor = *ppBitsCursor;
		hBmp = *phBmpCursor;
	}
	else {
		// 新規作成
		if (dstBpp > 1) {
			// モノクロ以外はDIBSectionで
			BITMAPINFO_8BPP bi;
			bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			bi.bmiHeader.biWidth = width;
			bi.bmiHeader.biHeight = -height;
			bi.bmiHeader.biPlanes = 1;
			bi.bmiHeader.biBitCount = dstBpp;
			bi.bmiHeader.biCompression = BI_RGB;
			bi.bmiHeader.biSizeImage = 0;
			bi.bmiHeader.biXPelsPerMeter = 0;
			bi.bmiHeader.biYPelsPerMeter = 0;
			bi.bmiHeader.biClrUsed = 0;
			bi.bmiHeader.biClrImportant = 0;
			if (dstBpp == 8) {
				bi.bmiHeader.biClrUsed = 256;
				memcpy(bi.bmiColors, npdisp_palette_rgb256, sizeof(npdisp_palette_rgb256));
			}
			else if (dstBpp == 4) {
				bi.bmiHeader.biClrUsed = 256;
				memcpy(bi.bmiColors, npdisp_palette_rgb16, sizeof(npdisp_palette_rgb16));
			}
			else if (dstBpp == 15 || dstBpp == 16) {
				if (dstBpp == 16) {
					// ビットフィールド 565
					bi.bmiHeader.biCompression = BI_BITFIELDS;
					*((DWORD*)(bi.bmiColors + 0)) = 0x0000F800;
					*((DWORD*)(bi.bmiColors + 1)) = 0x000007E0;
					*((DWORD*)(bi.bmiColors + 2)) = 0x0000001F;
				}
				else if (dstBpp == 15) {
					// ビットフィールド 555
					bi.bmiHeader.biCompression = BI_BITFIELDS;
					*((DWORD*)(bi.bmiColors + 0)) = 0x00007C00;
					*((DWORD*)(bi.bmiColors + 1)) = 0x000003E0;
					*((DWORD*)(bi.bmiColors + 2)) = 0x0000001F;
					bi.bmiHeader.biBitCount = 16;
				}
				bi.bmiHeader.biBitCount = 16; // 16扱いにする
			}
			hBmp = CreateDIBSection(hdcCursor, (BITMAPINFO*)(&bi), DIB_RGB_COLORS, &pBitsCursor, NULL, 0);
			if (!hBmp) return;
		}
		else {
			// モノクロはDDBで（後で作成）
			pBitsCursor = (char*)malloc(dstStride * height);
			if (!pBitsCursor) return;
		}
	}
	int x, y;
	if (srcBpp == dstBpp) {
		// bpp同じ
		UINT8* pBits8 = (UINT8*)pBitsCursor;
		for (y = 0; y < height; y++) {
			for (x = 0; x < srcStride; x++) {
				UINT8 value = npdisp_readMemory8(pixelBufAddr);
				*pBits8 = value;
				pixelBufAddr++;
				pBits8++;
			}
		}
		if (dstBpp == 1) {
			// 1bppはここで作成 それ以外はDIBSectionで反映済み
			hBmp = CreateBitmap(width, height, 1, 1, pBitsCursor);
		}
	}
	else if (srcBpp == 1 && dstBpp == 4) {
		// bpp変更 1bpp -> 4bpp
		UINT8* pBits8 = (UINT8*)pBitsCursor;
		for (y = 0; y < height; y++) {
			for (x = 0; x < srcStride; x++) {
				UINT8 value = npdisp_readMemory8(pixelBufAddr);
				for (int xx = 0; xx < 4; xx++) {
					*pBits8 = value & 0x80 ? 0xf0 : 0x00;
					*pBits8 |= value & 0x40 ? 0x0f : 0x00;
					pBits8++;
					value <<= 2;
				}
				pixelBufAddr++;
			}
		}
	}
	else if (srcBpp == 1 && dstBpp == 8) {
		// bpp変更 1bpp -> 8bpp
		UINT8* pBits8 = (UINT8*)pBitsCursor;
		for (y = 0; y < height; y++) {
			for (x = 0; x < srcStride; x++) {
				UINT8 value = npdisp_readMemory8(pixelBufAddr);
				for (int xx = 0; xx < 8; xx++) {
					*pBits8 = value & 0x80 ? 0xff : 0x00;
					pBits8++;
					value <<= 1;
				}
				pixelBufAddr++;
			}
		}
	}
	else if (srcBpp == 1 && dstBpp == 16) {
		// bpp変更 1bpp -> 16bpp
		UINT16* pBits16 = (UINT16*)pBitsCursor;
		for (y = 0; y < height; y++) {
			for (x = 0; x < srcStride; x++) {
				UINT8 value = npdisp_readMemory8(pixelBufAddr);
				for (int xx = 0; xx < 8; xx++) {
					*pBits16 = value & 0x80 ? 0xffff : 0x0000;
					pBits16++;
					value <<= 1;
				}
				pixelBufAddr++;
			}
		}
	}
	else if (srcBpp == 1 && dstBpp == 24) {
		// bpp変更 1bpp -> 24bpp
		UINT8* pBits8 = (UINT8*)pBitsCursor;
		for (y = 0; y < height; y++) {
			for (x = 0; x < srcStride; x++) {
				UINT8 value = npdisp_readMemory8(pixelBufAddr);
				for (int xx = 0; xx < 8; xx++) {
					if (value & 0x80) {
						pBits8[0] = pBits8[1] = pBits8[2] = 0xff;
					}
					else {
						pBits8[0] = pBits8[1] = pBits8[2] = 0x00;
					}
					pBits8 += 3;
					value <<= 1;
				}
				pixelBufAddr++;
			}
		}
	}
	else if (srcBpp == 1 && dstBpp == 32) {
		// bpp変更 1bpp -> 32bpp
		UINT8* pBits8 = (UINT8*)pBitsCursor;
		for (y = 0; y < height; y++) {
			for (x = 0; x < srcStride; x++) {
				UINT8 value = npdisp_readMemory8(pixelBufAddr);
				for (int xx = 0; xx < 8; xx++) {
					if (value & 0x80) {
						pBits8[0] = pBits8[1] = pBits8[2] = 0xff;
					}
					else {
						pBits8[0] = pBits8[1] = pBits8[2] = 0x00;
					}
					pBits8 += 4;
					value <<= 1;
				}
				pixelBufAddr++;
			}
		}
	}
	if (hBmp) {
		if (*phBmpCursor && *phBmpCursor != hBmp) {
			// 新規作成されていたら既存の物は捨てる
			SelectObject(hdcCursor, *phOldBmpCursor);
			DeleteObject(*phBmpCursor);
		}
		*phOldBmpCursor = (HBITMAP)SelectObject(hdcCursor, hBmp);
		*phBmpCursor = hBmp;
	}

	void* oldpBitsCursor = *ppBitsCursor;
	*ppBitsCursor = pBitsCursor;
	if (npdisp.cursorBpp <= 1 && oldpBitsCursor && oldpBitsCursor != pBitsCursor) {
		// モノクロの場合ビットデータはfreeで捨てる
		free(oldpBitsCursor);
	}
}

static void npdisp_func_SetCursor(UINT32 lpCursorShapeAddr)
{
	NPDISP_CURSORSHAPE cursorShape = { 0 };
	if (lpCursorShapeAddr) {
		if (npdisp_readMemory(&cursorShape, lpCursorShapeAddr, sizeof(cursorShape)) && npdisp.longjmpnum == 0) {
			//int cursorDataStride = cursorShape.csWidthBytes;
			int cursorMaskBpp = cursorShape.csColor & 0xff;
			int cursorXORBpp = (cursorShape.csColor) >> 8 & 0xff;
			int cursorBpp = max(cursorMaskBpp, cursorXORBpp);
			int strideMask = cursorMaskBpp == 1 ? ((cursorShape.csWidth + 15) / 16 * 16 / 8) : ((cursorShape.csWidth * cursorMaskBpp + 31) / 32 * 32 / 8);
			int strideXOR = cursorXORBpp == 1 ? ((cursorShape.csWidth + 15) / 16 * 16 / 8) : ((cursorShape.csWidth * cursorXORBpp + 31) / 32 * 32 / 8);
			int stride = max(strideMask, strideXOR);
			if (cursorShape.csWidth > 0 && cursorShape.csHeight > 0) {
				// AND画像
				UINT32 pixelBufAddr = lpCursorShapeAddr + sizeof(cursorShape);
				npdisp_func_SetCursor_Make(&npdispwin.pBitsCursorMask, &npdispwin.hBmpCursorMask, &npdispwin.hOldBmpCursorMask, npdispwin.hdcCursorMask,
					pixelBufAddr, cursorShape.csWidth, cursorShape.csHeight, strideMask, stride, cursorMaskBpp, cursorBpp);

				// XOR画像
				pixelBufAddr += strideMask * cursorShape.csHeight;
				npdisp_func_SetCursor_Make(&npdispwin.pBitsCursor, &npdispwin.hBmpCursor, &npdispwin.hOldBmpCursor, npdispwin.hdcCursor,
					pixelBufAddr, cursorShape.csWidth, cursorShape.csHeight, strideXOR, stride, cursorXORBpp, cursorBpp);

				npdisp.cursorHotSpotX = cursorShape.csHotX;
				npdisp.cursorHotSpotY = cursorShape.csHotY;
				npdisp.cursorWidth = cursorShape.csWidth;
				npdisp.cursorHeight = cursorShape.csHeight;
				npdisp.cursorBpp = cursorBpp;
				npdisp.cursorStride = stride;
				npdispwin.cursorUpdated = true;
				npdisp.updated = 1;
			}
			else {
				// 破棄
				if (npdispwin.hBmpCursorMask) {
					SelectObject(npdispwin.hdcCursorMask, npdispwin.hOldBmpCursorMask);
					DeleteObject(npdispwin.hBmpCursorMask);
					npdispwin.hBmpCursorMask = NULL;
					npdispwin.pBitsCursorMask = NULL;
				}
				if (npdispwin.hBmpCursor) {
					SelectObject(npdispwin.hdcCursor, npdispwin.hOldBmpCursor);
					DeleteObject(npdispwin.hBmpCursor);
					npdispwin.hBmpCursor = NULL;
					npdispwin.pBitsCursor = NULL;
				}
				npdispwin.cursorUpdated = true;
				npdisp.updated = 1;
			}
		}
	}
	else {
		// 破棄
		if (npdispwin.hBmpCursorMask) {
			SelectObject(npdispwin.hdcCursorMask, npdispwin.hOldBmpCursorMask);
			DeleteObject(npdispwin.hBmpCursorMask);
			npdispwin.hBmpCursorMask = NULL;
			npdispwin.pBitsCursorMask = NULL;
		}
		if (npdispwin.hBmpCursor) {
			SelectObject(npdispwin.hdcCursor, npdispwin.hOldBmpCursor);
			DeleteObject(npdispwin.hBmpCursor);
			npdispwin.hBmpCursor = NULL;
			npdispwin.pBitsCursor = NULL;
		}
		npdispwin.cursorUpdated = true;
		npdisp.updated = 1;
	}
}
static void npdisp_func_MoveCursor(UINT16 wAbsX, UINT16 wAbsY)
{
	if (npdisp.cursorX != wAbsX || npdisp.cursorY != wAbsY) {
		npdisp.cursorX = wAbsX;
		npdisp.cursorY = wAbsY;
		npdispwin.cursorUpdated = true;
		npdisp.updated = 1;
	}
}
static void npdisp_func_CheckCursor()
{
	// nothing to do
}

static UINT16 npdisp_func_FastBorder(UINT32 lpRectAddr, UINT16 wHorizBorderThick, UINT16 wVertBorderThick, UINT32 dwRasterOp, UINT32 lpDestDevAddr, UINT32 lpPBrushAddr, UINT32 lpDrawModeAddr, UINT32 lpClipRectAddr)
{
	UINT16 retValue = 0;
	int dstDevType = 0;
	HDC tgtDC = npdispwin.hdc;
	npdisp_readMemory(&dstDevType, lpDestDevAddr, 2);
	if (dstDevType != 0) {
		// PDEVICE
		if (lpPBrushAddr) {
			// ブラシがあれば選択
			NPDISP_BRUSH brush = { 0 };
			if (npdisp_readMemory(&brush, lpPBrushAddr, sizeof(NPDISP_BRUSH))) {
				if (brush.key != 0) {
					auto it = npdispwin.brushes.find(brush.key);
					if (it != npdispwin.brushes.end()) {
						NPDISP_HOSTBRUSH value = it->second;
						if (value.brs) {
							SelectObject(tgtDC, value.brs);
						}
						else {
							SelectObject(tgtDC, (HBRUSH)GetStockObject(NULL_BRUSH));
						}
					}
				}
			}
		}
		NPDISP_DRAWMODE drawMode = { 0 };
		if (npdisp_readMemory(&drawMode, lpDrawModeAddr, sizeof(NPDISP_DRAWMODE))) {
			if (tgtDC != npdispwin.hdc || memcmp(&drawMode, &npdispwin.lastScreenDrawMode, sizeof(NPDISP_DRAWMODE)) != 0) {
				if (tgtDC == npdispwin.hdc) npdispwin.lastScreenDrawMode = drawMode;
				npdisp_AdjustDrawModeColor(&drawMode);
				SetBkColor(tgtDC, drawMode.LbkColor);
				SetTextColor(tgtDC, drawMode.LTextColor);
				SetBkMode(tgtDC, drawMode.bkMode);
				SetROP2(tgtDC, drawMode.Rop2);
			}
		}
		HRGN hRgn = NULL;
		if (lpClipRectAddr) {
			RECT cliprect = { 0 };
			NPDISP_RECT rectTmp = { 0 };
			npdisp_readMemory(&rectTmp, lpClipRectAddr, sizeof(NPDISP_RECT));
			cliprect.top = rectTmp.top;
			cliprect.left = rectTmp.left;
			cliprect.bottom = rectTmp.bottom;
			cliprect.right = rectTmp.right;
			hRgn = CreateRectRgn(cliprect.left, cliprect.top, cliprect.right, cliprect.bottom);
			SelectClipRgn(tgtDC, hRgn);
		}

		NPDISP_RECT rectBdr = { 0 };
		npdisp_readMemory(&rectBdr, lpRectAddr, sizeof(NPDISP_RECT));

		int tx = wVertBorderThick;
		int ty = wHorizBorderThick;
		PatBlt(tgtDC, rectBdr.left, rectBdr.top, rectBdr.right - rectBdr.left, ty, dwRasterOp);
		PatBlt(tgtDC, rectBdr.left, rectBdr.top + ty, tx, (rectBdr.bottom - rectBdr.top) - ty * 2, dwRasterOp);
		PatBlt(tgtDC, rectBdr.right - tx, rectBdr.top + ty, tx, (rectBdr.bottom - rectBdr.top) - ty * 2, dwRasterOp);
		PatBlt(tgtDC, rectBdr.left, rectBdr.bottom - ty, rectBdr.right - rectBdr.left, ty, dwRasterOp);

		retValue = 1;
		npdisp_setDirty(rectBdr.left, rectBdr.top, rectBdr.right, rectBdr.bottom);
		npdisp.updated = 1;

		if (hRgn) {
			SelectClipRgn(tgtDC, NULL);
			DeleteObject(hRgn);
		}
	}

	return retValue;
}

static UINT16 npdisp_func_Output(UINT32 lpDestDevAddr, UINT16 wStyle, UINT16 wCount, UINT32 lpPointsAddr, UINT32 lpPPenAddr, UINT32 lpPBrushAddr, UINT32 lpDrawModeAddr, UINT32 lpClipRectAddr)
{
	UINT16 retValue = 0xffff;
	NPDISP_PBITMAP_EXT dstPBmp;
	NPDISP_WINDOWS_BMPHDC bmphdc = { 0 };
	int dstDevType = 0;
	HDC tgtDC = npdispwin.hdc;
	npdisp_readMemory(&dstDevType, lpDestDevAddr, 2);
	bool isDisplayDevice = npdisp_isDisplayDevice(lpDestDevAddr);

	HPEN curPen = NULL;
	int curPenWidth = 0;
	if (lpPPenAddr) {
		// ペンがあれば取得
		NPDISP_PEN pen = { 0 };
		if (npdisp_readMemory(&pen, lpPPenAddr, sizeof(NPDISP_PEN))) {
			if (pen.key != 0) {
				auto it = npdispwin.pens.find(pen.key);
				if (it != npdispwin.pens.end()) {
					NPDISP_HOSTPEN value = it->second;
					if (value.pen) {
						curPen = value.pen;
						curPenWidth = value.lpen.lopnWidth.x;
					}
					else {
						curPen = (HPEN)GetStockObject(NULL_PEN);
					}
				}
			}
		}
	}
	HBRUSH curBrush = NULL;
	if (lpPBrushAddr) {
		// ブラシがあれば取得
		NPDISP_BRUSH brush = { 0 };
		if (npdisp_readMemory(&brush, lpPBrushAddr, sizeof(NPDISP_BRUSH))) {
			if (brush.key != 0) {
				auto it = npdispwin.brushes.find(brush.key);
				if (it != npdispwin.brushes.end()) {
					NPDISP_HOSTBRUSH value = it->second;
					if (value.brs) {
						curBrush = value.brs;
					}
					else {
						curBrush = (HBRUSH)GetStockObject(NULL_BRUSH);
					}
				}
			}
		}
	}

	if (npdisp.longjmpnum != 0) return retValue;

	NPDISP_POINT ptBuf[4];
	int dstBeginLine = 0;
	int dstNumLines = -1;
	int dstBeginX = 0;
	int dstWidth = -1;
	if (dstDevType != NPDISP_DEVTYPE_DDB) {
		// 転送先がDDBでないときはレンジチェック
		switch (wStyle) {
		case 18: // OS_POLYLINE
		{
			npdisp_func_Output_GetXYRange_POLYLINE(&dstBeginX, &dstWidth, &dstBeginLine, &dstNumLines, curPenWidth, curBrush, wCount, lpPointsAddr);
			break;
		}
		case 80: // OS_BEGINNSCAN
		case 81: // OS_ENDNSCAN
		{
			break;
		}
		case 4: // OS_SCANLINES
		{
			npdisp_func_Output_GetXYRange_SCANLINES(&dstBeginX, &dstWidth, &dstBeginLine, &dstNumLines, curPenWidth, curBrush, wCount, lpPointsAddr);
			break;
		}
		case 6: // OS_RECTANGLE
		{
			npdisp_func_Output_GetXYRange_RECTANGLE(&dstBeginX, &dstWidth, &dstBeginLine, &dstNumLines, curPenWidth, curBrush, wCount, lpPointsAddr, ptBuf);
			break;
		}
		case 20: // OS_WINDPOLYGON
		case 22: // OS_ALTPOLYGON
		{
			npdisp_func_Output_GetXYRange_POLYGON(&dstBeginX, &dstWidth, &dstBeginLine, &dstNumLines, curPenWidth, curBrush, wCount, lpPointsAddr);
			break;
		}
		case 55: // OS_CIRCLE
		case 7: // OS_ELLIPSE 
		{
			npdisp_func_Output_GetXYRange_ELLIPSE(&dstBeginX, &dstWidth, &dstBeginLine, &dstNumLines, curPenWidth, curBrush, wCount, lpPointsAddr, ptBuf);
			break;
		}
		case 3: // OS_ARC
		{
			npdisp_func_Output_GetXYRange_ARC(&dstBeginX, &dstWidth, &dstBeginLine, &dstNumLines, curPenWidth, curBrush, wCount, lpPointsAddr);
			break;
		}
		case 23: // OS_PIE
		{
			npdisp_func_Output_GetXYRange_PIE(&dstBeginX, &dstWidth, &dstBeginLine, &dstNumLines, curPenWidth, curBrush, wCount, lpPointsAddr);
			break;
		}
		case 39: // OS_CHORD 
		{
			npdisp_func_Output_GetXYRange_CHORD(&dstBeginX, &dstWidth, &dstBeginLine, &dstNumLines, curPenWidth, curBrush, wCount, lpPointsAddr);
			break;
		}
		case 72: // OS_ROUNDRECT 
		{
			npdisp_func_Output_GetXYRange_ROUNDRECT(&dstBeginX, &dstWidth, &dstBeginLine, &dstNumLines, curPenWidth, curBrush, wCount, lpPointsAddr, ptBuf);
			break;
		}
		case 1: // OS_POLYBEZIER 
		{
			npdisp_func_Output_GetXYRange_POLYBEZIER(&dstBeginX, &dstWidth, &dstBeginLine, &dstNumLines, curPenWidth, curBrush, wCount, lpPointsAddr);
			break;
		}
		case (0x4000 | 20): // OS_POLYPOLYGON | OS_WINDPOLYGON
		case (0x4000 | 22): // OS_POLYPOLYGON | OS_ALTPOLYGON
		{
			npdisp_func_Output_GetXYRange_POLYPOLYGON(&dstBeginX, &dstWidth, &dstBeginLine, &dstNumLines, curPenWidth, curBrush, wCount, lpPointsAddr);
			break;
		}
		default:
		{
			TRACEOUTF(("Unsupported Output: %d", wStyle));
			break;
		}
		}
	}
	else {
		// 頂点先読みの場合は読み込み
		switch (wStyle) {
			case 6: // OS_RECTANGLE
			case 55: // OS_CIRCLE
			case 7: // OS_ELLIPSE 
				npdisp_readMemory(ptBuf, lpPointsAddr, sizeof(NPDISP_POINT) * 2);
				break;
			case 72: // OS_ROUNDRECT 
				npdisp_readMemory(ptBuf, lpPointsAddr, sizeof(NPDISP_POINT) * 3);
				break;
		}
	}

	// 負になっていたら補正
	if (dstBeginLine < 0) {
		dstNumLines += dstBeginLine;
		dstBeginLine = 0;
	}
	if (dstBeginX < 0) {
		dstWidth += dstBeginX;
		dstBeginX = 0;
	}

	if ((dstNumLines > 0 || dstNumLines == -1) && (dstWidth > 0 || dstWidth == -1)) {
		if (!isDisplayDevice) {
			// memory 
			if (dstNumLines != 0) {
				if (lpDestDevAddr && npdisp_readPBitmap(&dstPBmp, lpDestDevAddr)) {
					npdisp_PreloadBitmapFromPBITMAP(&dstPBmp, 0, dstBeginLine, dstNumLines, dstBeginX, dstWidth);
					if (npdisp_MakeBitmapFromPBITMAP(&dstPBmp, &bmphdc, 0, dstBeginLine, dstNumLines, dstBeginX, dstWidth)) {
						npdisp_ConvertToDDBMonoBitmap(&bmphdc);
						tgtDC = bmphdc.hdc;
					}
				}
			}
			else {
				tgtDC = NULL;
			}
		}
		if (npdisp.longjmpnum == 0) {
			// ペンがあれば選択
			if (curPen) {
				SelectObject(tgtDC, curPen);
			}
			else {
				SelectObject(tgtDC, GetStockObject(NULL_PEN));
			}
			// ブラシがあれば選択
			if (curBrush) {
				SelectObject(tgtDC, curBrush);
			}
			else {
				SelectObject(tgtDC, GetStockObject(NULL_BRUSH));
			}
			NPDISP_DRAWMODE drawMode = { 0 };
			if (npdisp_readMemory(&drawMode, lpDrawModeAddr, sizeof(NPDISP_DRAWMODE))) {
				if (tgtDC != npdispwin.hdc || memcmp(&drawMode, &npdispwin.lastScreenDrawMode, sizeof(NPDISP_DRAWMODE)) != 0) {
					if (tgtDC == npdispwin.hdc) npdispwin.lastScreenDrawMode = drawMode;
					npdisp_AdjustDrawModeColor(&drawMode);
					SetBkColor(tgtDC, drawMode.LbkColor);
					SetTextColor(tgtDC, drawMode.LTextColor);
					SetBkMode(tgtDC, drawMode.bkMode);
					SetROP2(tgtDC, drawMode.Rop2);
				}
			}
			HRGN hRgn = NULL;
			if (lpClipRectAddr) {
				RECT cliprect = { 0 };
				NPDISP_RECT rectTmp = { 0 };
				npdisp_readMemory(&rectTmp, lpClipRectAddr, sizeof(NPDISP_RECT));
				cliprect.top = rectTmp.top;
				cliprect.left = rectTmp.left;
				cliprect.bottom = rectTmp.bottom;
				cliprect.right = rectTmp.right;
				hRgn = CreateRectRgn(cliprect.left, cliprect.top, cliprect.right, cliprect.bottom);
				SelectClipRgn(tgtDC, hRgn);
			}
			bool success = false;
			switch (wStyle) {
			case 18: // OS_POLYLINE
			{
				success = npdisp_func_Output_POLYLINE(tgtDC, &bmphdc, &dstPBmp, curPenWidth, curBrush, wCount, lpPointsAddr);
				retValue = 1;
				break;
			}
			case 80: // OS_BEGINNSCAN
			case 81: // OS_ENDNSCAN
			{
				retValue = 1;
				break;
			}
			case 4: // OS_SCANLINES
			{
				success = npdisp_func_Output_SCANLINES(tgtDC, &bmphdc, &dstPBmp, curPenWidth, curBrush, wCount, lpPointsAddr);
				retValue = 1;
				break;
			}
			case 6: // OS_RECTANGLE
			{
				success = npdisp_func_Output_RECTANGLE(tgtDC, &bmphdc, &dstPBmp, curPenWidth, curBrush, wCount, lpPointsAddr, ptBuf);
				retValue = 1;
				break;
			}
			case 20: // OS_WINDPOLYGON
			{
				success = npdisp_func_Output_WINDPOLYGON(tgtDC, &bmphdc, &dstPBmp, curPenWidth, curBrush, wCount, lpPointsAddr);
				retValue = 1;
				break;
			}
			case 22: // OS_ALTPOLYGON
			{
				success = npdisp_func_Output_ALTPOLYGON(tgtDC, &bmphdc, &dstPBmp, curPenWidth, curBrush, wCount, lpPointsAddr);
				retValue = 1;
				break;
			}
			case 55: // OS_CIRCLE
			case 7: // OS_ELLIPSE 
			{
				success = npdisp_func_Output_ELLIPSE(tgtDC, &bmphdc, &dstPBmp, curPenWidth, curBrush, wCount, lpPointsAddr, ptBuf);
				retValue = 1;
				break;
			}
			case 3: // OS_ARC
			{
				success = npdisp_func_Output_ARC(tgtDC, &bmphdc, &dstPBmp, curPenWidth, curBrush, wCount, lpPointsAddr);
				retValue = 1;
				break;
			}
			case 23: // OS_PIE
			{
				success = npdisp_func_Output_PIE(tgtDC, &bmphdc, &dstPBmp, curPenWidth, curBrush, wCount, lpPointsAddr);
				retValue = 1;
				break;
			}
			case 39: // OS_CHORD 
			{
				success = npdisp_func_Output_CHORD(tgtDC, &bmphdc, &dstPBmp, curPenWidth, curBrush, wCount, lpPointsAddr);
				retValue = 1;
				break;
			}
			case 72: // OS_ROUNDRECT 
			{
				success = npdisp_func_Output_ROUNDRECT(tgtDC, &bmphdc, &dstPBmp, curPenWidth, curBrush, wCount, lpPointsAddr, ptBuf);
				retValue = 1;
				break;
			}
			case 1: // OS_POLYBEZIER 
			{
				npdisp_func_Output_POLYBEZIER(tgtDC, &bmphdc, &dstPBmp, curPenWidth, curBrush, wCount, lpPointsAddr);
				break;
			}
			case (0x4000 | 20): // OS_POLYPOLYGON | OS_WINDPOLYGON
			{
				success = npdisp_func_Output_WINDPOLYPOLYGON(tgtDC, &bmphdc, &dstPBmp, curPenWidth, curBrush, wCount, lpPointsAddr);
				retValue = 1;
				break;
			}
			case (0x4000 | 22): // OS_POLYPOLYGON | OS_ALTPOLYGON
			{
				success = npdisp_func_Output_ALTPOLYPOLYGON(tgtDC, &bmphdc, &dstPBmp, curPenWidth, curBrush, wCount, lpPointsAddr);
				retValue = 1;
				break;
			}
			default:
			{
				TRACEOUTF(("Unsupported Output: %d", wStyle));
				break;
			}
			}
			if (success && bmphdc.hdc) {
				// 書き戻し
				npdisp_WriteBitmapToPBITMAP(&dstPBmp, &bmphdc, dstBeginLine, dstNumLines, dstBeginX, dstWidth);
			}
			if (hRgn) {
				SelectClipRgn(tgtDC, NULL);
				DeleteObject(hRgn);
			}
			if (bmphdc.hdc) {
				npdisp_FreeBitmap(&bmphdc);
			}
			else {
				if (dstNumLines == -1 && dstWidth == -1) {
					npdisp_setDirtyAll();
				}
				else if (dstNumLines == -1) {
					npdisp_setDirty(dstBeginX, 0, dstBeginX + dstWidth, npdisp.height);
				}
				else if (dstWidth == -1) {
					npdisp_setDirty(0, dstBeginLine, npdisp.width, dstBeginLine + dstNumLines);
				}
				else {
					npdisp_setDirty(dstBeginX, dstBeginLine, dstBeginX + dstWidth, dstBeginLine + dstNumLines);
				}
				npdisp.updated = 1;
			}
		}
	}
	else {
		// 成功したことにする 
		retValue = 1;
	}
	return retValue;
}

static UINT32 npdisp_func_Pixel(UINT32 lpDestDevAddr, UINT16 X, UINT16 Y, UINT32 dwPhysColor, UINT32 lpDrawModeAddr)
{
	UINT32 retValue = 0x80000000L;
	int dstDevType = 0;
	HDC tgtDC = npdispwin.hdc;
	npdisp_readMemory(&dstDevType, lpDestDevAddr, 2);
	NPDISP_PBITMAP_EXT dstPBmp;
	NPDISP_WINDOWS_BMPHDC bmphdc = { 0 };
	if (!npdisp_isDisplayDevice(lpDestDevAddr)) {
		// memory 
		if (lpDestDevAddr && npdisp_readPBitmap(&dstPBmp, lpDestDevAddr)) {
			if (npdisp_MakeBitmapFromPBITMAP(&dstPBmp, &bmphdc, 0, Y, 1, X, 1)) {
				tgtDC = bmphdc.hdc;
			}
		}
	}
	if (npdisp.longjmpnum == 0) {
		NPDISP_DRAWMODE drawMode = { 0 };
		int hasDrawMode = npdisp_readMemory(&drawMode, lpDrawModeAddr, sizeof(NPDISP_DRAWMODE));
		if (hasDrawMode) {
			if (npdisp.usePalette && npdisp.bpp == 8) {
				if (dwPhysColor & 0xff000000) {
					dwPhysColor = dwPhysColor & 0xff;
					dwPhysColor = dwPhysColor | (dwPhysColor << 8) | (dwPhysColor << 16);
				}
				else {
					const UINT8 r = (UINT8)(dwPhysColor & 0xFF);
					const UINT8 g = (UINT8)((dwPhysColor >> 8) & 0xFF);
					const UINT8 b = (UINT8)((dwPhysColor >> 16) & 0xFF);
					dwPhysColor = npdisp_FindNearest256(r, g, b);
					dwPhysColor = dwPhysColor | (dwPhysColor << 8) | (dwPhysColor << 16);
				}
			}
			if (SetPixel(tgtDC, X, Y, dwPhysColor) != -1) {
				retValue = 1;
			}
			if (bmphdc.hdc) {
				npdisp_WriteBitmapToPBITMAP(&dstPBmp, &bmphdc, Y, 1, X, 1);
			}
			else {
				npdisp_setDirty(X, Y, X + 1, Y + 1);
				npdisp.updated = 1;
			}
		}
		else {
			retValue = GetPixel(tgtDC, X, Y);
			if (npdisp.usePalette && npdisp.bpp == 8) {
				retValue = (retValue & 0xff) | 0xff000000; // to palette index
			}
		}
	}
	if (bmphdc.hdc) {
		npdisp_FreeBitmap(&bmphdc);
	}

	return retValue;
}

static UINT16 npdisp_func_ScanLR(UINT32 lpDestDevAddr, UINT16 X, UINT16 Y, UINT32 dwPhysColor, UINT16 Style)
{
	UINT16 retValue = 0xffff;
	int dstDevType = 0;
	HDC tgtDC = npdispwin.hdc;
	npdisp_readMemory(&dstDevType, lpDestDevAddr, 2);
	NPDISP_PBITMAP_EXT dstPBmp;
	NPDISP_WINDOWS_BMPHDC bmphdc = { 0 };
	BITMAPINFOHEADER* lpBiHeader = &(npdispwin.bi.bmiHeader);
	UINT8* lpBits = (UINT8*)npdispwin.pBits;
	if (!npdisp_isDisplayDevice(lpDestDevAddr)) {
		// memory 
		if (lpDestDevAddr && npdisp_readPBitmap(&dstPBmp, lpDestDevAddr)) {
			if (npdisp_MakeBitmapFromPBITMAP(&dstPBmp, &bmphdc, 0)) {
				tgtDC = bmphdc.hdc;
				lpBiHeader = &(bmphdc.lpbi->bmiHeader);
				lpBits = (UINT8*)bmphdc.pBits;
			}
		}
	}
	if (npdisp.longjmpnum == 0) {
		UINT32 devColor = dwPhysColor;
		if (npdisp.usePalette && npdisp.bpp == 8 && (devColor & 0xff000000)) {
			devColor &= 0xff; // to palette index
		}
		else {
			const UINT8 r = (UINT8)(devColor & 0xFF);
			const UINT8 g = (UINT8)((devColor >> 8) & 0xFF);
			const UINT8 b = (UINT8)((devColor >> 16) & 0xFF);
			if (npdisp.bpp == 24 || npdisp.bpp == 32) {
				// RGB逆順注意
				devColor = (r << 16) | (g << 8) | b;
			}
			else if (npdisp.bpp == 16) {
				const UINT8 r5 = r >> 3;
				const UINT8 g6 = g >> 2;
				const UINT8 b5 = b >> 3;
				devColor = (r5 << 11) | (g6 << 5) | b5;
				{
					DIBSECTION ds;
					GetObject(bmphdc.hBmp, sizeof(ds), &ds);
					ds.dsBitfields[0] = ds.dsBitfields[0];
				}
			}
			else if (npdisp.bpp == 15) {
				const UINT8 r5 = r >> 3;
				const UINT8 g5 = g >> 3;
				const UINT8 b5 = b >> 3;
				devColor = (r5 << 10) | (g5 << 5) | b5;
			}
			else if (npdisp.bpp == 8) {
				devColor = npdisp_FindNearest256(r, g, b);
			}
			else if (npdisp.bpp == 4) {
				devColor = npdisp_FindNearest16(r, g, b);
			}
			else if (npdisp.bpp == 1) {
				devColor = npdisp_FindNearest2(r, g, b);
			}
		}
		int w = lpBiHeader->biWidth;
		int h = lpBiHeader->biHeight;
		UINT32 compMask = (1 << lpBiHeader->biBitCount) - 1;
		if (lpBiHeader->biBitCount > 24) {
			compMask = 0xffffff;
		}
		UINT32 stepBit = lpBiHeader->biBitCount;
		if (h < 0) h = -h;
		if (Y < h && X < w) {
			int stride = ((lpBiHeader->biWidth * lpBiHeader->biBitCount + 31) / 32) * 4;
			int x;
			if (Style & 2) {
				// 左へスキャン
				lpBits += stride * Y;
				for (x = X; x >= 0; x--) {
					UINT8* p = lpBits + x * stepBit / 8;
					if (lpBiHeader->biBitCount > 16) {
						if (((*((UINT32*)p) & compMask) == devColor) == !!(Style & 0x1)) {
							break;
						}
					}
					else if (lpBiHeader->biBitCount > 8) {
						if (((*((UINT16*)p) & compMask) == devColor) == !!(Style & 0x1)) {
							break;
						}
					}
					else {
						int bitPos = (x * stepBit) % 8;
						bitPos = 7 - bitPos - (stepBit - 1); // 並びを反転
						if ((((*p >> bitPos) & compMask) == devColor) == !!(Style & 0x1)) {
							break;
						}
					}
				}
				if (x == -1) {
					retValue = -1; // 端まで到達
				}
				else {
					retValue = x;
					//if (!(Style & 0x1))retValue++;
				}
			}
			else {
				// 右へスキャン
				lpBits += stride * Y;
				for (x = X; x < w; x++) {
					UINT8* p = lpBits + x * stepBit / 8;
					if (lpBiHeader->biBitCount > 16) {
						if (((*((UINT32*)p) & compMask) == devColor) == !!(Style & 0x1)) {
							break;
						}
					}
					else if (lpBiHeader->biBitCount > 8) {
						if (((*((UINT16*)p) & compMask) == devColor) == !!(Style & 0x1)) {
							break;
						}
					}
					else {
						int bitPos = (x * stepBit) % 8;
						bitPos = 7 - bitPos - (stepBit - 1); // 並びを反転
						if ((((*p >> bitPos) & compMask) == devColor) == !!(Style & 0x1)) {
							break;
						}
					}
				}
				if (x == w) {
					retValue = -1; // 端まで到達
				}
				else {
					retValue = x;
					//if (!(Style & 0x1))retValue--;
				}
			}
		}
	}
	if (bmphdc.hdc) {
		npdisp_FreeBitmap(&bmphdc);
	}

	return retValue;
}

static UINT16 npdisp_func_EnumObj(UINT32 lpDestDevAddr, UINT16 wStyle, UINT16 enumIdx, UINT32 lpLogObjAddr)
{
	UINT16 retValue = 0;
	int dstDevType = 0;
	npdisp_readMemory(&dstDevType, lpDestDevAddr, 2);
	if (dstDevType != 0) {
		UINT16 idx = enumIdx;
		if (wStyle == 1) {
			// pen
			NPDISP_LPEN pen;
			pen.lopnWidth.x = 1;
			pen.lopnWidth.y = 0;
			pen.opnStyle = 0;
			pen.lopnColor = npdisp_ObjIdxToColor(idx);
			if (pen.lopnColor != -1) {
				retValue = 1;
			}
			npdisp_writeMemory(&pen, lpLogObjAddr, sizeof(pen));
		}
		else if (wStyle == 2) {
			// brush
			NPDISP_LBRUSH brush;
			brush.lbBkColor = 1;
			brush.lbHatch = 0;
			brush.lbStyle = 0;
			brush.lbColor = npdisp_ObjIdxToColor(idx);
			if (brush.lbColor != -1) {
				retValue = 1;
			}
			npdisp_writeMemory(&brush, lpLogObjAddr, sizeof(brush));
		}
	}
	return retValue;
}

static void npdisp_func_GetPalette(UINT16 nStartIndex, UINT16 nNumEntries, UINT32 lpPaletteAddr)
{
	if (!npdisp.usePalette) return;
	if (!lpPaletteAddr) return;

	if (nStartIndex < NELEMENTS(npdisp_palette_rgb256)) {
		UINT32 endIdx = (UINT32)nStartIndex + nNumEntries;
		if (endIdx > NELEMENTS(npdisp_palette_rgb256)) endIdx = NELEMENTS(npdisp_palette_rgb256);
		for (int i = nStartIndex; i < endIdx; i++) {
			UINT32 col = ((UINT32)npdisp_palette_rgb256[i].b << 16) | ((UINT32)npdisp_palette_rgb256[i].g << 8) | ((UINT32)npdisp_palette_rgb256[i].r);
			npdisp_writeMemory32(col, lpPaletteAddr);
			lpPaletteAddr += 4;
		}
	}
}
static void npdisp_func_SetPalette(UINT16 nStartIndex, UINT16 nNumEntries, UINT32 lpPaletteAddr)
{
	if (!npdisp.usePalette) return;
	if (!lpPaletteAddr) return;

	if (nStartIndex < NELEMENTS(npdisp_palette_rgb256)) {
		UINT32 endIdx = (UINT32)nStartIndex + nNumEntries;
		if (endIdx > NELEMENTS(npdisp_palette_rgb256)) endIdx = NELEMENTS(npdisp_palette_rgb256);
		for (int i = nStartIndex; i < endIdx; i++) {
			UINT32 col = npdisp_readMemory32(lpPaletteAddr);
			//npdisp_palette_rgb256[i].reserved = (col >> 24) & 0xff;
			npdisp_palette_rgb256[i].b = (col >> 16) & 0xff;
			npdisp_palette_rgb256[i].g = (col >> 8) & 0xff;
			npdisp_palette_rgb256[i].r = col & 0xff;
			lpPaletteAddr += 4;
		}
		if (npdisp.mm_bmpinfoAddr) {
			UINT32 palAddr = npdisp.mm_bmpinfoAddr + sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * nStartIndex;
			npdisp_writeMemory(npdisp_palette_rgb256 + nStartIndex, palAddr, sizeof(RGBQUAD) * (endIdx - nStartIndex));
		}
		npdisp_palette_clearCache(nStartIndex, endIdx);
		npdisp_setDirtyAll();
		npdisp.paletteUpdated = 1;
	}
}
static void npdisp_func_GetPalTrans(UINT32 lpIndexesAddr)
{
	if (!npdisp.usePalette) return;
	if (!lpIndexesAddr) return;

	npdisp_writeMemory(npdisp_palette_transTbl, lpIndexesAddr, sizeof(npdisp_palette_transTbl));
}
static void npdisp_func_SetPalTrans(UINT32 lpIndexesAddr)
{
	if (!npdisp.usePalette) return;

	if (lpIndexesAddr) {
		npdisp_readMemory(npdisp_palette_transTbl, lpIndexesAddr, sizeof(npdisp_palette_transTbl));
	}
	else {
		for (int i = 0; i < NELEMENTS(npdisp_palette_transTbl); i++) {
			npdisp_palette_transTbl[i] = i;
		}
	}
}
static void npdisp_func_UpdateColors(SINT16 wStartX, SINT16 wStartY, UINT16 wExtX, UINT16 wExtY, UINT32 lpTranslateAddr)
{
	if (!npdisp.usePalette) return;
	if (!lpTranslateAddr) return;
	if (npdisp.bpp != 8) return;

	UINT16 transTbl[256];
	npdisp_readMemory(transTbl, lpTranslateAddr, sizeof(transTbl));

	int colors = (npdisp.bpp <= 8) ? (1 << npdisp.bpp) : 0;

	if (wStartX + (int)wExtX <= 0) return;
	if (wStartY + (int)wExtY <= 0) return;
	if (wStartX < 0) {
		wExtX += wStartX;
		wStartX = 0;
	}
	if (wStartY < 0) {
		wExtY += wStartY;
		wStartY = 0;
	}
	if (wStartX >= npdisp.width) return;
	if (wStartY >= npdisp.height) return;
	if ((UINT32)wStartX + wExtX > npdisp.width) wExtX = npdisp.width - wStartX;
	if ((UINT32)wStartY + wExtY > npdisp.height) wExtY = npdisp.height - wStartY;

	// 与えられた変換テーブルを使用してパレット色を置き換え
	int stride = npdispwin.stride;
	UINT8 *lpBuf = (UINT8*)npdispwin.pBits + wStartY * stride + wStartX;
	for (int y = 0; y < wExtY; y++) {
		for (int x = 0; x < wExtX; x++) {
			*lpBuf = transTbl[*lpBuf] & 0xff;
			lpBuf++;
		}
		lpBuf += stride - wExtX;
	}
}

static UINT16 npdisp_func_GetCharWidth(UINT32 lpDestDevAddr, UINT32 lpBufferAddr, UINT16 wFirstChar, UINT16 wLastChar, UINT32 lpFontInfoAddr, UINT32 lpDrawModeAddr, UINT32 lpFontTransAddr) {

	NPDISP_FONTINFO fontInfo;
	if (npdisp_readMemory(&fontInfo, lpFontInfoAddr, sizeof(NPDISP_FONTINFO))) {
		for (int i = wFirstChar; i <= wLastChar; i++) {
			NPDISP_FONTCHARINFO3 charInfo;
			int charIdx = i - (int)fontInfo.dfFirstChar;
			if (charIdx < 0 || fontInfo.dfLastChar - fontInfo.dfFirstChar < charIdx) {
				charIdx = fontInfo.dfDefaultChar;
			}
			if (npdisp_readMemory(&charInfo, lpFontInfoAddr + sizeof(NPDISP_FONTINFO) + sizeof(NPDISP_FONTCHARINFO3) * charIdx, sizeof(NPDISP_FONTCHARINFO3))) {
				npdisp_writeMemory16(charInfo.width, lpBufferAddr);
				lpBufferAddr += 2;
			}
			else {
				return 0;
			}
		}
		return 1;
	}
	return 0;
}

static UINT16 npdisp_func_StretchDIBits(UINT32 lpPDevice, UINT16 fGet, SINT16 DestX, SINT16 DestY, SINT16 DestXE, SINT16 DestYE, UINT16 SrcX, UINT16 SrcY, UINT16 SrcXE, UINT16 SrcYE, UINT32 lpBitsAddr, UINT32 lpBitmapInfoAddr, UINT32 lpTranslateAddr, UINT32 dwROP, UINT32 lpPBrushAddr, UINT32 lpDrawModeAddr, UINT32 lpClipRectAddr)
{
	UINT16 retValue = -1;

	int stretchMode = COLORONCOLOR;

	int absDestXE = DestXE >= 0 ? DestXE : -DestXE;
	int absDestYE = DestYE >= 0 ? DestYE : -DestYE;

	RECT cliprect = { 0, 0, npdisp.width, npdisp.height };
	if (lpClipRectAddr) {
		NPDISP_RECT rectTmp = { 0 };
		npdisp_readMemory(&rectTmp, lpClipRectAddr, sizeof(NPDISP_RECT));
		cliprect.top = rectTmp.top;
		cliprect.left = rectTmp.left;
		cliprect.bottom = rectTmp.bottom;
		cliprect.right = rectTmp.right;
		if (cliprect.top > cliprect.bottom) {
			int tmp = cliprect.top;
			cliprect.top = cliprect.bottom;
			cliprect.bottom = tmp;
		}
	}
	BITMAPINFOHEADER biHeader = { 0 };
	npdisp_readMemory(&biHeader, lpBitmapInfoAddr, sizeof(BITMAPINFOHEADER));
	if (npdisp.longjmpnum == 0) {
		TRACEOUTSDIB(("(w:%d, h:%d) Y:%d %d, X:%d %d", biHeader.biWidth, biHeader.biHeight, DestY, DestYE, DestX, DestXE));
		int dstDevType = 0;
		HDC tgtDC = npdispwin.hdc;
		npdisp_readMemory(&dstDevType, lpPDevice, 2);
		NPDISP_PBITMAP_EXT dstPBmp;
		NPDISP_WINDOWS_BMPHDC bmphdc = { 0 };
		bool toBitmap = false;
		if (!npdisp_isDisplayDevice(lpPDevice)) {
			// memory 
			if (lpPDevice && npdisp_readPBitmap(&dstPBmp, lpPDevice)) {
				npdisp_PreloadBitmapFromPBITMAP(&dstPBmp, 0, DestY, absDestYE, DestX, absDestXE);
				if (npdisp.longjmpnum == 0 && npdisp_MakeBitmapFromPBITMAP(&dstPBmp, &bmphdc, 0, DestY, absDestYE, DestX, absDestXE)) {
					tgtDC = bmphdc.hdc;
					toBitmap = true;
				}
			}
		}
		if (npdisp.version >= 4 && npdisp.isWin9x) {
			if (lpDrawModeAddr) stretchMode = npdisp_readMemory16(lpDrawModeAddr + 36); // Win9x StretchBltModeを読む
		}
		else {
			if (toBitmap) {
				if (dstPBmp.bmBitsPixel == 1) return 0xffff; //モノクロソースの時はCOLORONCOLOR以外だと致命的なのでGDIにやらせる
			}
			else {
				if (npdisp.bpp == 1) return 0xffff; //モノクロソースの時はCOLORONCOLOR以外だと致命的なのでGDIにやらせる
			}
		}
		if (npdisp.longjmpnum == 0 && biHeader.biPlanes == 1 && (biHeader.biBitCount == 1 || biHeader.biBitCount == 4 || biHeader.biBitCount == 8 || biHeader.biBitCount == 15 || biHeader.biBitCount == 16 || biHeader.biBitCount == 24 || biHeader.biBitCount == 32) && (biHeader.biHeight > SrcY || -biHeader.biHeight > SrcY)) {
			int stride = ((biHeader.biWidth * biHeader.biBitCount + 31) / 32) * 4;
			int bmpHeight = biHeader.biHeight >= 0 ? biHeader.biHeight : -biHeader.biHeight;
			int height = SrcYE;
			int beginLine = SrcY;
			int lpbiLen = 0;
			int lpbiReadLen = 0;
			int lpbiWriteLen = 0;
			if (biHeader.biBitCount <= 8) {
				lpbiLen = sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * (1 << biHeader.biBitCount);
				if (npdisp.usePalette) {
					if (!lpTranslateAddr) {
						lpbiReadLen = sizeof(BITMAPINFOHEADER) + sizeof(UINT16) * (1 << biHeader.biBitCount);
						lpbiWriteLen = lpbiReadLen;
					}
					else {
						lpbiReadLen = sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * (1 << biHeader.biBitCount);
						lpbiWriteLen = lpbiLen;
					}
				}
				else {
					lpbiReadLen = lpbiLen;
					lpbiWriteLen = lpbiLen;
				}
				if (lpbiLen < lpbiReadLen) {
					lpbiLen = lpbiReadLen;
				}
				if (lpbiLen < lpbiWriteLen) {
					lpbiLen = lpbiWriteLen;
				}
			}
			else if ((biHeader.biBitCount == 15 || biHeader.biBitCount == 16 || biHeader.biBitCount == 32) && biHeader.biCompression == BI_BITFIELDS) {
				lpbiReadLen = lpbiLen = sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * 3;
			}
			else {
				lpbiReadLen = lpbiLen = sizeof(BITMAPINFOHEADER);
			}
			BITMAPINFO* lpbi;
			lpbi = (BITMAPINFO*)malloc(lpbiLen);
			if (lpbi) {
				if (lpbiLen > lpbiReadLen) {
					memset((UINT8*)lpbi + lpbiReadLen, 0, lpbiLen - lpbiReadLen);
				}
				npdisp_readMemory(lpbi, lpBitmapInfoAddr, lpbiReadLen);
				if (lpBitsAddr) {
					bool useRGBBlt = false;
					UINT16* transTbl = NULL; // transTblはbiBitCountによらずUINT16配列に変換する
					int colors = (1 << lpbi->bmiHeader.biBitCount);
					if (lpTranslateAddr && npdisp.bpp <= 8) {
						if (lpbi->bmiHeader.biBitCount == 8) {
							if (fGet) {
								// 1byte x 256色 で渡される
								UINT8* transTbl8 = (UINT8*)malloc(colors);
								if (transTbl8) {
									if (npdisp_readMemory(transTbl8, lpTranslateAddr, colors)) {
										transTbl = (UINT16*)malloc(colors * sizeof(UINT16));
										if (transTbl) {
											for (int i = 0; i < colors; i++) {
												((UINT16*)transTbl)[i] = transTbl8[i];
											}
										}
									}
									free(transTbl8);
								}
							}
							else {
								// 2byte x 256色 で渡される?
								transTbl = (UINT16*)malloc(colors * sizeof(UINT16));
								if (transTbl) {
									npdisp_readMemory(transTbl, lpTranslateAddr, colors * sizeof(UINT16));
								}
							}
						}
						else {
							if (fGet) {
								// XXX: よく分からない
								useRGBBlt = true;
							}
							else {
								if (lpbi->bmiHeader.biBitCount == 4) {
									// 2byte x 16色 で渡される?
									transTbl = (UINT16*)malloc(colors * sizeof(UINT16));
									if (transTbl) {
										npdisp_readMemory(transTbl, lpTranslateAddr, colors * sizeof(UINT16));
									}
								}
								else if (lpbi->bmiHeader.biBitCount == 1) {
									// 2byte x 2色 で渡される？
									transTbl = (UINT16*)malloc(colors * sizeof(UINT16));
									if (transTbl) {
										npdisp_readMemory(transTbl, lpTranslateAddr, colors * sizeof(UINT16));
									}
								}
							}
						}
					}
					//if (lpbi->bmiHeader.biCompression == BI_RGB || lpbi->bmiHeader.biCompression == BI_BITFIELDS) {
					//	npdisp_preloadMemoryWith32Offset(lpBitsAddr >> 16, (lpBitsAddr & 0xffff) + stride * SrcY, stride * height); // 無圧縮なら画像サイズで先読み
					//}
					//else if (lpbi->bmiHeader.biSizeImage) {
					//	npdisp_preloadMemory(lpBitsAddr, lpbi->bmiHeader.biSizeImage); // RLE圧縮でサイズ既知なら先読み
					//}
					void* pBits = NULL;
					int i;
					if (SrcY + height > bmpHeight) {
						height = bmpHeight - SrcY;
					}
					if (npdisp.usePalette) {
						if (lpbi->bmiHeader.biBitCount <= 8) {
							if (lpTranslateAddr) {
								// transTblにインデックス変換表が入る
								if (transTbl) {
									for (i = 0; i < colors; i++) {
										lpbi->bmiColors[i].rgbRed = transTbl[i] & 0xff;
										lpbi->bmiColors[i].rgbGreen = transTbl[i] & 0xff;
										lpbi->bmiColors[i].rgbBlue = transTbl[i] & 0xff;
										lpbi->bmiColors[i].rgbReserved = 0;
									}
								}
							}
							else {
								UINT16 palTrans[256];
								memcpy(palTrans, lpbi->bmiColors, colors * sizeof(UINT16));
								for (i = 0; i < colors; i++) {
									lpbi->bmiColors[i].rgbRed = palTrans[i] & 0xff;
									lpbi->bmiColors[i].rgbGreen = palTrans[i] & 0xff;
									lpbi->bmiColors[i].rgbBlue = palTrans[i] & 0xff;
									lpbi->bmiColors[i].rgbReserved = 0;
								}
							}
						}
					}
					else if (fGet) {
						if (lpbi->bmiHeader.biBitCount == 1) {
							// 有効なパレットでなければ2色パレットセット
							if (lpbi->bmiColors[0].rgbRed != 0 || lpbi->bmiColors[0].rgbGreen != 0 || lpbi->bmiColors[0].rgbBlue != 0 || lpbi->bmiColors[0].rgbReserved != 0 ||
								lpbi->bmiColors[1].rgbRed != 0xff || lpbi->bmiColors[1].rgbGreen != 0xff || lpbi->bmiColors[1].rgbBlue != 0xff || lpbi->bmiColors[1].rgbReserved != 0) {
								for (i = biHeader.biClrUsed; i < NELEMENTS(npdisp_palette_rgb2); i++) {
									lpbi->bmiColors[i].rgbRed = npdisp_palette_rgb2[i].r;
									lpbi->bmiColors[i].rgbGreen = npdisp_palette_rgb2[i].g;
									lpbi->bmiColors[i].rgbBlue = npdisp_palette_rgb2[i].b;
									lpbi->bmiColors[i].rgbReserved = 0;
								}
								if (fGet) {
									npdisp_writeMemory(lpbi, lpBitmapInfoAddr, lpbiReadLen); // 変更したパレットを書き戻し
								}
							}
						}
						else if (lpbi->bmiHeader.biBitCount == 4) {
							// 有効なパレットでなければ16色パレットセット
							if (lpbi->bmiColors[0].rgbRed != 0 || lpbi->bmiColors[0].rgbGreen != 0 || lpbi->bmiColors[0].rgbBlue != 0 ||
								lpbi->bmiColors[15].rgbRed != 0xff || lpbi->bmiColors[15].rgbGreen != 0xff || lpbi->bmiColors[15].rgbBlue != 0xff) {
								for (i = biHeader.biClrUsed; i < NELEMENTS(npdisp_palette_rgb16); i++) {
									lpbi->bmiColors[i].rgbRed = npdisp_palette_rgb16[i].r;
									lpbi->bmiColors[i].rgbGreen = npdisp_palette_rgb16[i].g;
									lpbi->bmiColors[i].rgbBlue = npdisp_palette_rgb16[i].b;
									lpbi->bmiColors[i].rgbReserved = 0;
								}
								if (fGet) {
									npdisp_writeMemory(lpbi, lpBitmapInfoAddr, lpbiReadLen); // 変更したパレットを書き戻し
								}
							}
						}
						else if (lpbi->bmiHeader.biBitCount == 8) {
							// 有効なパレットでなければ256色パレットセット
							if (lpbi->bmiColors[0].rgbRed != 0 || lpbi->bmiColors[0].rgbGreen != 0 || lpbi->bmiColors[0].rgbBlue != 0 ||
								lpbi->bmiColors[255].rgbRed != 0xff || lpbi->bmiColors[255].rgbGreen != 0xff || lpbi->bmiColors[255].rgbBlue != 0xff) {
								for (i = biHeader.biClrUsed; i < NELEMENTS(npdisp_palette_rgb256); i++) {
									lpbi->bmiColors[i].rgbRed = npdisp_palette_rgb256[i].r;
									lpbi->bmiColors[i].rgbGreen = npdisp_palette_rgb256[i].g;
									lpbi->bmiColors[i].rgbBlue = npdisp_palette_rgb256[i].b;
									lpbi->bmiColors[i].rgbReserved = 0;
								}
								if (fGet) {
									npdisp_writeMemory(lpbi, lpBitmapInfoAddr, lpbiReadLen); // 変更したパレットを書き戻し
								}
							}
						}
					}
					UINT32 biCompression = lpbi->bmiHeader.biCompression;
					bool isCompress = !(biCompression == BI_RGB || biCompression == BI_BITFIELDS); 
					if (isCompress) {
						// StretchDIBitsの圧縮フォーマットはややこしそうなのでGDIエミュレーションにやらせる
						// 未定義ピクセルを除いてラスタオペレーションが必要？
						retValue = -1;
					}
					else {
						if (lpbi->bmiHeader.biHeight >= 0) {
							SrcY = bmpHeight - SrcY - height; // Bottom-up換算???
						}
						HBITMAP hBmp = CreateDIBSection(npdispwin.hdc, lpbi, DIB_RGB_COLORS, &pBits, NULL, 0);
						if (hBmp) {
							HDC hdc = npdispwin.hdcCache[1];
							HGDIOBJ hOldBmp = SelectObject(hdc, hBmp);
							bool hasError = false;
							if (lpbi->bmiHeader.biHeight < 0) {
								int bottomUpY = bmpHeight - SrcY - height;
								npdisp_readMemoryWith32Offset((UINT8*)pBits + stride * SrcY, lpBitsAddr >> 16, (lpBitsAddr & 0xffff) + stride * bottomUpY, stride * height);
							}
							else {
								// Bottom-up換算
								int bottomUpY = bmpHeight - SrcY - height;
								npdisp_readMemoryWith32Offset((UINT8*)pBits + stride * bottomUpY, lpBitsAddr >> 16, (lpBitsAddr & 0xffff) + stride * bottomUpY, stride * height);
							}
							if (!hasError) {
								bool palChanged = false;
								if (npdisp.usePalette) {
									if (lpbi->bmiHeader.biBitCount > 8 || useRGBBlt) {
										// グレースケールから実際のデバイス色へ置き換え
										if (npdisp.bpp == 8) {
											RGBQUAD pal[256];
											for (int i = 0; i < 256; i++) {
												pal[i].rgbRed = npdisp_palette_rgb256[i].r;
												pal[i].rgbGreen = npdisp_palette_rgb256[i].g;
												pal[i].rgbBlue = npdisp_palette_rgb256[i].b;
												pal[i].rgbReserved = 0;
											}
											SetDIBColorTable(tgtDC, 0, 256, pal);
											palChanged = true;
										}
									}
								}
								NPDISP_DRAWMODE drawMode = { 0 };
								if (npdisp_readMemory(&drawMode, lpDrawModeAddr, sizeof(NPDISP_DRAWMODE))) {
									if (hdc != npdispwin.hdc || memcmp(&drawMode, &npdispwin.lastScreenDrawMode, sizeof(NPDISP_DRAWMODE)) != 0) {
										if (hdc == npdispwin.hdc) npdispwin.lastScreenDrawMode = drawMode;
										npdisp_AdjustDrawModeColor(&drawMode);
										SetBkColor(hdc, drawMode.LbkColor);
										SetTextColor(hdc, drawMode.LTextColor);
										SetBkMode(hdc, drawMode.bkMode);
										SetROP2(hdc, drawMode.Rop2);
									}
								}

								HBRUSH brs = NULL;
								if (lpPBrushAddr) {
									// ブラシがあれば選択
									NPDISP_BRUSH brush = { 0 };
									if (npdisp_readMemory(&brush, lpPBrushAddr, sizeof(NPDISP_BRUSH))) {
										if (brush.key != 0) {
											auto it = npdispwin.brushes.find(brush.key);
											if (it != npdispwin.brushes.end()) {
												NPDISP_HOSTBRUSH value = it->second;
												brs = value.brs;
											}
										}
									}
								}

								HRGN hRgn = lpClipRectAddr ? CreateRectRgn(cliprect.left, cliprect.top, cliprect.right, cliprect.bottom) : NULL;

								if (fGet) {
									// Get Bits
									int bottomUpY = bmpHeight - SrcY - height;
									int adjustedSrcY = (lpbi->bmiHeader.biHeight >= 0) ? SrcY : bottomUpY;
									int adjustedSrcYE = (lpbi->bmiHeader.biHeight >= 0) ? SrcYE : height;
									if (hRgn) {
										SelectClipRgn(hdc, hRgn);
									}
									HGDIOBJ oldBrush = NULL;
									if (brs) {
										oldBrush = SelectObject(hdc, brs);
									}
									if (DestXE == SrcXE && DestYE == SrcYE) {
										BitBlt(hdc, DestX, DestY, DestXE, DestYE, tgtDC, SrcX, adjustedSrcY, dwROP);
									}
									else {
										SetStretchBltMode(hdc, stretchMode);
										StretchBlt(hdc, DestX, DestY, DestXE, DestYE, tgtDC, SrcX, adjustedSrcY, SrcXE, adjustedSrcYE, dwROP);
									}
									if (lpbi->bmiHeader.biHeight < 0) {
										npdisp_writeMemoryWith32Offset((UINT8*)pBits + stride * SrcY, lpBitsAddr >> 16, (lpBitsAddr & 0xffff), stride * height);
									}
									else {
										// Bottom-up換算
										npdisp_writeMemoryWith32Offset((UINT8*)pBits + stride * bottomUpY, lpBitsAddr >> 16, (lpBitsAddr & 0xffff) + stride * bottomUpY, stride * height);
									}
									if (brs) {
										SelectObject(hdc, oldBrush);
									}
									if (hRgn) {
										SelectClipRgn(hdc, NULL);
									}
								}
								else {
									// Set Bits
									int bottomUpY = bmpHeight - SrcY - height;
									int adjustedSrcY = (lpbi->bmiHeader.biHeight >= 0) ? SrcY : bottomUpY;
									int adjustedSrcYE = (lpbi->bmiHeader.biHeight >= 0) ? SrcYE : height;
									if (hRgn) {
										SelectClipRgn(tgtDC, hRgn);
									}
									HGDIOBJ oldBrush = NULL;
									if (brs) {
										oldBrush = SelectObject(tgtDC, brs);
									}
									if (DestXE == SrcXE && DestYE == SrcYE) {
										BitBlt(tgtDC, DestX, DestY, DestXE, DestYE, hdc, SrcX, SrcY, dwROP);
									}
									else {
										SetStretchBltMode(tgtDC, stretchMode);
										StretchBlt(tgtDC, DestX, DestY, DestXE, DestYE, hdc, SrcX, SrcY, SrcXE, SrcYE, dwROP);
									}
									if (toBitmap) {
										npdisp_WriteBitmapToPBITMAP(&dstPBmp, &bmphdc, DestY, absDestYE, DestX, absDestXE);
									}
									else {
										npdisp_setDirty(DestX, DestY, DestX + absDestXE, DestY + absDestYE);
										npdisp.updated = 1;
									}
									if (brs) {
										SelectObject(tgtDC, oldBrush);
									}
									if (hRgn) {
										SelectClipRgn(tgtDC, NULL);
									}
								}
								retValue = height;

								if (hRgn) {
									DeleteObject(hRgn);
								}

								if (palChanged) {
									// 色を戻す
									SetDIBColorTable(tgtDC, 0, 256, (RGBQUAD*)npdisp_palette_gray256);
								}
							}

							SelectObject(hdc, hOldBmp);
							DeleteObject(hBmp);
						}
					}
					if (transTbl) {
						free(transTbl);
					}
				}
				free(lpbi);
			}
		}
		if (toBitmap) {
			npdisp_FreeBitmap(&bmphdc);
		}
	}
	return retValue;
}

static void npdisp_func_INT2Fh(UINT16 ax)
{
	if (ax == 0x4001) {
		// DOS窓全画面モード設定
		np2wab.relaystateext = 0;
		np2wab_setRelayState(np2wab.relaystateint | np2wab.relaystateext);
		npdisp.active = 0;
	}
	else if (ax == 0x4002) {
		// DOS窓全画面モード解除
		npdisp.active = 1;
		npdisp_setDirtyAll();
		npdisp.updated = 1;
		np2wab.relaystateext = 3;
		np2wab_setRelayState(np2wab.relaystateint | np2wab.relaystateext);
	}
}

//static void npdisp_func_MEMORYMAP(UINT32 physicalAddr, UINT32 linearAddr, UINT16 farSelector, UINT32 farOffset)
//{
//	npdisp.mm_physicalAddr = physicalAddr;
//	npdisp.mm_linearAddr = linearAddr;
//	npdisp.mm_farSelector = farSelector;
//	npdisp.mm_farOffset = farOffset;
//}

static UINT16 npdisp_func_DCI_BeginAccess(UINT32 lpDeviceAddr, UINT32 lpRectAddr) {
	if (lpDeviceAddr) {
		NPDISP_RECT r;
		if (npdisp_readMemory(&r, lpRectAddr, sizeof(NPDISP_RECT))) {
			npdispwin.dciDirtyRect.left = r.left;
			npdispwin.dciDirtyRect.top = r.top;
			npdispwin.dciDirtyRect.right = r.right;
			npdispwin.dciDirtyRect.bottom = r.bottom;
		}
		return 0; // DCI_OK
	}
	return -3; // DCI_FAIL_INVALIDSURFACE
}
static void npdisp_func_DCI_EndAccess(UINT32 lpDeviceAddr) {
	npdisp_setDirty(npdispwin.dciDirtyRect.left, npdispwin.dciDirtyRect.top, npdispwin.dciDirtyRect.right, npdispwin.dciDirtyRect.bottom);
	npdispwin.dciDirtyRect.left = 0;
	npdispwin.dciDirtyRect.top = 0;
	npdispwin.dciDirtyRect.right = 0;
	npdispwin.dciDirtyRect.bottom = 0;
	npdisp.updated = 1;
}
static void npdisp_func_DCI_DestroySurface(UINT32 lpDeviceAddr) {
	npdisp_setDirty(npdispwin.dciDirtyRect.left, npdispwin.dciDirtyRect.top, npdispwin.dciDirtyRect.right, npdispwin.dciDirtyRect.bottom);
	npdispwin.dciDirtyRect.left = 0;
	npdispwin.dciDirtyRect.top = 0;
	npdispwin.dciDirtyRect.right = 0;
	npdispwin.dciDirtyRect.bottom = 0;
	npdisp.updated = 1;
}

static void npdisp_func_WEP()
{
	// Windows終了
	npdisp.enabled = 0;
	npdisp.active = 0;
	np2wab.relaystateext = 0;
	np2wab_setRelayState(np2wab.relaystateint | np2wab.relaystateext);
	npdisp_releaseScreen();
}


// *** エクスポート関数処理 エントリ *****************

/// <summary>
/// ゲストから渡された序数に対応する機能を実行する　データはnpdisp.dataAddrから受け取る
/// </summary>
/// <param name=""></param>
void npdisp_exec(void) {

	// 共用先読みを指定
	npdisp_memory_setFunctionId(0);

	// 読み書き開始位置を先頭へ戻す
	npdisp_memory_resetposition();

	UINT16 version = npdisp_readMemory16(npdisp.dataAddr); // バージョンだけ取得

	// 排他開始
	npdispcs_enter_criticalsection();

	if (version >= 1) {
		UINT16 retCode = NPDISP_RETCODE_SUCCESS;
		NPDISP_REQUEST req;
		npdisp_readMemory(&req, npdisp.dataAddr, sizeof(req)); // 全体読み込み
		npdisp.version = req.version; // プロトコルバージョン
		switch (req.funcOrder) {
		case NPDISP_FUNCORDER_NP2INITIALIZE:
		{
			TRACEOUT(("Initialize"));
			npdisp_func_NP2Initialize(req.parameters.init.dpiX, req.parameters.init.dpiY, req.parameters.init.width, req.parameters.init.height, req.parameters.init.bpp, req.parameters.init.isWin9x, req.parameters.init.bmpinfoAddr, req.parameters.init.beginAccessAddr, req.parameters.init.endAccessAddr, req.parameters.init.dcibufAddr, req.parameters.init.dciBeginAccessAddr, req.parameters.init.dciEndAccessAddr, req.parameters.init.dciDestroySurfaceAddr, req.parameters.init.vramLinearAddr, req.parameters.init.vramPhysicalAddr);
			break;
		}
		case NPDISP_FUNCORDER_Enable:
		{
			TRACEOUT(("Enable"));
			const UINT16 retValue = npdisp_func_Enable(req.parameters.enable.lpDevInfoAddr, req.parameters.enable.wStyle, req.parameters.enable.lpDestDevTypeAddr, req.parameters.enable.lpOutputFileAddr, req.parameters.enable.lpDataAddr);
			npdisp_writeMemory16(retValue, req.parameters.enable.lpRetValueAddr);
			break;
		}
		case NPDISP_FUNCORDER_Disable:
		{
			TRACEOUT(("Disable"));
			npdisp_func_Disable(req.parameters.disable.lpDestDevAddr);
			break;
		}
		case NPDISP_FUNCORDER_GetDriverResourceID:
		{
			TRACEOUT(("GetDriverResourceID"));
			const SINT16 retValue = npdisp_func_GetDriverResourceID(req.parameters.GetDriverResourceID.iResId, req.parameters.GetDriverResourceID.lpResTypeAddr);
			npdisp_writeMemory16(retValue, req.parameters.GetDriverResourceID.lpRetValueAddr);
			break;
		}
		case NPDISP_FUNCORDER_ColorInfo:
		{
			//TRACEOUT(("ColorInfo"));
			// 色の変換
			UINT32 retValue = 0;
			NPDISP_PDEVICE devInfo;
			UINT32 pcolor;
			npdisp_readMemory(&retValue, req.parameters.ColorInfo.lpRetValueAddr, 4);
			npdisp_readMemory(&devInfo, req.parameters.ColorInfo.lpDestDevAddr, 2);
			//if (*((UINT16*)&devInfo) == NPDISP_DEVTYPE || *((UINT16*)&devInfo) == 0) {
			//	npdisp_readMemory(&devInfo, req.parameters.ColorInfo.lpDestDevAddr, sizeof(NPDISP_PBITMAP));
			//}
			if (*((UINT16*)&devInfo) == NPDISP_DEVTYPE) npdisp_readMemory(&devInfo, req.parameters.ColorInfo.lpDestDevAddr, sizeof(devInfo));
			if (req.parameters.ColorInfo.lpPColorAddr) {
				npdisp_readMemory(&pcolor, req.parameters.ColorInfo.lpPColorAddr, sizeof(pcolor));
				retValue = npdisp_func_ColorInfo(&devInfo, req.parameters.ColorInfo.dwColorin, &pcolor);
				npdisp_writeMemory(&pcolor, req.parameters.ColorInfo.lpPColorAddr, sizeof(pcolor));
			}
			else {
				retValue = npdisp_func_ColorInfo(&devInfo, req.parameters.ColorInfo.dwColorin, NULL);
			}
			npdisp_writeMemory32(retValue, req.parameters.ColorInfo.lpRetValueAddr);
			break;
		}
		case NPDISP_FUNCORDER_RealizeObject:
		{
			TRACEOUT(("RealizeObject"));
			// オブジェクト生成と破棄
			const UINT32 retValue = npdisp_func_RealizeObject(req.parameters.RealizeObject.lpDestDevAddr, req.parameters.RealizeObject.wStyle, req.parameters.RealizeObject.lpInObjAddr, req.parameters.RealizeObject.lpOutObjAddr, req.parameters.RealizeObject.lpTextXFormAddr);
			npdisp_writeMemory32(retValue, req.parameters.RealizeObject.lpRetValueAddr);
			break;
		}
		case NPDISP_FUNCORDER_Control:
		{
			TRACEOUT(("Control %d", req.parameters.Control.wFunction));
			const UINT16 retValue = npdisp_func_Control(req.parameters.Control.lpDestDevAddr, req.parameters.Control.wFunction, req.parameters.Control.lpInDataAddr, req.parameters.Control.lpOutDataAddr);
			npdisp_writeMemory16(retValue, req.parameters.Control.lpRetValueAddr);
			break;
		}
		case NPDISP_FUNCORDER_BitBlt:
		{
			//TRACEOUT(("BitBlt"));
			const UINT16 retValue = npdisp_func_BitBlt(req.parameters.BitBlt.lpDestDevAddr, req.parameters.BitBlt.wDestX, req.parameters.BitBlt.wDestY, req.parameters.BitBlt.lpSrcDevAddr, req.parameters.BitBlt.wSrcX, req.parameters.BitBlt.wSrcY, req.parameters.BitBlt.wXext, req.parameters.BitBlt.wYext, req.parameters.BitBlt.Rop3, req.parameters.BitBlt.lpPBrushAddr, req.parameters.BitBlt.lpDrawModeAddr);
			npdisp_writeMemory16(retValue, req.parameters.BitBlt.lpRetValueAddr);
			break;
		}
		case NPDISP_FUNCORDER_StretchBlt:
		{
			//TRACEOUT(("StretchBlt"));
			const UINT16 retValue = npdisp_func_StretchBlt(req.parameters.stretchBlt.lpDestDevAddr, req.parameters.stretchBlt.wDestX, req.parameters.stretchBlt.wDestY, req.parameters.stretchBlt.wDestXext, req.parameters.stretchBlt.wDestYext, req.parameters.stretchBlt.lpSrcDevAddr, req.parameters.stretchBlt.wSrcX, req.parameters.stretchBlt.wSrcY, req.parameters.stretchBlt.wSrcXext, req.parameters.stretchBlt.wSrcYext, req.parameters.stretchBlt.Rop3, req.parameters.stretchBlt.lpPBrushAddr, req.parameters.stretchBlt.lpDrawModeAddr, req.parameters.stretchBlt.lpClipAddr);
			npdisp_writeMemory16(retValue, req.parameters.stretchBlt.lpRetValueAddr);
			break;
		}
		case NPDISP_FUNCORDER_DeviceBitmapBits:
		{
			TRACEOUT(("DeviceBitmapBits"));
			const UINT16 retValue = npdisp_func_DeviceBitmapBits(req.parameters.DeviceBitmapBits.lpBitmapAddr, req.parameters.DeviceBitmapBits.fGet, req.parameters.DeviceBitmapBits.iStart, req.parameters.DeviceBitmapBits.cScans, req.parameters.DeviceBitmapBits.lpDIBitsAddr, req.parameters.DeviceBitmapBits.lpBitmapInfoAddr, req.parameters.DeviceBitmapBits.lpDrawModeAddr, req.parameters.DeviceBitmapBits.lpTranslateAddr);
			npdisp_writeMemory16(retValue, req.parameters.DeviceBitmapBits.lpRetValueAddr);
			break;
		}
		case NPDISP_FUNCORDER_StrBlt:
		case NPDISP_FUNCORDER_ExtTextOut:
		{
			if (req.funcOrder == NPDISP_FUNCORDER_StrBlt) {
				TRACEOUT(("StrBlt"));
				req.parameters.extTextOut.lpCharWidthsAddr = 0;
				req.parameters.extTextOut.lpOpaqueRectAddr = 0;
				req.parameters.extTextOut.wOptions = 0;
			}
			else {
				TRACEOUT(("ExtTextOut"));
			}
			const UINT32 retValue = npdisp_func_ExtTextOut(req.parameters.extTextOut.lpDestDevAddr, req.parameters.extTextOut.wDestXOrg, req.parameters.extTextOut.wDestYOrg, req.parameters.extTextOut.lpClipRectAddr, req.parameters.extTextOut.lpStringAddr, req.parameters.extTextOut.wCount, req.parameters.extTextOut.lpFontInfoAddr, req.parameters.extTextOut.lpDrawModeAddr, req.parameters.extTextOut.lpTextXFormAddr, req.parameters.extTextOut.lpCharWidthsAddr, req.parameters.extTextOut.lpOpaqueRectAddr, req.parameters.extTextOut.wOptions);
			npdisp_writeMemory32(retValue, req.parameters.extTextOut.lpRetValueAddr);
			break;
		}
		case NPDISP_FUNCORDER_SetDIBitsToDevice:
		{
			TRACEOUT(("SetDIBitsToDevice"));
			const UINT16 retValue = npdisp_func_SetDIBitsToDevice(req.parameters.SetDIBitsToDevice.lpDestDevAddr, req.parameters.SetDIBitsToDevice.X, req.parameters.SetDIBitsToDevice.Y, req.parameters.SetDIBitsToDevice.iScan, req.parameters.SetDIBitsToDevice.cScans, req.parameters.SetDIBitsToDevice.lpClipRectAddr, req.parameters.SetDIBitsToDevice.lpDrawModeAddr, req.parameters.SetDIBitsToDevice.lpDIBitsAddr, req.parameters.SetDIBitsToDevice.lpBitmapInfoAddr, req.parameters.SetDIBitsToDevice.lpTranslateAddr);
			npdisp_writeMemory16(retValue, req.parameters.SetDIBitsToDevice.lpRetValueAddr);
			break;
		}
		case NPDISP_FUNCORDER_SaveScreenBitmap:
		{
			TRACEOUT(("SaveScreenBitmap"));
			const UINT16 retValue = npdisp_func_SaveScreenBitmap(req.parameters.SaveScreenBitmap.lpRect, req.parameters.SaveScreenBitmap.wCommand);
			npdisp_writeMemory16(retValue, req.parameters.SaveScreenBitmap.lpRetValueAddr);
			break;
		}
		case NPDISP_FUNCORDER_SetCursor:
		{
			TRACEOUT(("SetCursor"));
			npdisp_func_SetCursor(req.parameters.SetCursor.lpCursorShapeAddr);
			break;
		}
		case NPDISP_FUNCORDER_MoveCursor:
		{
			//TRACEOUT(("MoveCursor"));
			npdisp_func_MoveCursor(req.parameters.MoveCursor.wAbsX, req.parameters.MoveCursor.wAbsY);
			break;
		}
		case NPDISP_FUNCORDER_CheckCursor:
		{
			//TRACEOUT(("CheckCursor"));
			npdisp_func_CheckCursor();
			break;
		}
		case NPDISP_FUNCORDER_FastBorder:
		{
			TRACEOUT(("FastBorder"));
			const UINT16 retValue = npdisp_func_FastBorder(req.parameters.fastBorder.lpRectAddr, req.parameters.fastBorder.wHorizBorderThick, req.parameters.fastBorder.wVertBorderThick, req.parameters.fastBorder.dwRasterOp, req.parameters.fastBorder.lpDestDevAddr, req.parameters.fastBorder.lpPBrushAddr, req.parameters.fastBorder.lpDrawModeAddr, req.parameters.fastBorder.lpClipRectAddr);
			npdisp_writeMemory16(retValue, req.parameters.fastBorder.lpRetValueAddr);
			break;
		}
		case NPDISP_FUNCORDER_Output:
		{
			//TRACEOUT(("Output"));
			const UINT16 retValue = npdisp_func_Output(req.parameters.output.lpDestDevAddr, req.parameters.output.wStyle, req.parameters.output.wCount, req.parameters.output.lpPointsAddr, req.parameters.output.lpPPenAddr, req.parameters.output.lpPBrushAddr, req.parameters.output.lpDrawModeAddr, req.parameters.output.lpClipRectAddr);
			npdisp_writeMemory16(retValue, req.parameters.output.lpRetValueAddr);
			break;
		}
		case NPDISP_FUNCORDER_Pixel:
		{
			TRACEOUT(("Pixel"));
			const UINT32 retValue = npdisp_func_Pixel(req.parameters.pixel.lpDestDevAddr, req.parameters.pixel.X, req.parameters.pixel.Y, req.parameters.pixel.dwPhysColor, req.parameters.pixel.lpDrawModeAddr);
			npdisp_writeMemory32(retValue, req.parameters.scanLR.lpRetValueAddr);
			break;
		}
		case NPDISP_FUNCORDER_ScanLR:
		{
			TRACEOUT(("ScanLR"));
			const UINT16 retValue = npdisp_func_ScanLR(req.parameters.scanLR.lpDestDevAddr, req.parameters.scanLR.X, req.parameters.scanLR.Y, req.parameters.scanLR.dwPhysColor, req.parameters.scanLR.Style);
			npdisp_writeMemory16(retValue, req.parameters.scanLR.lpRetValueAddr);
			break;
		}
		case NPDISP_FUNCORDER_EnumObj:
		{
			TRACEOUT(("EnumObj"));
			const UINT16 retValue = npdisp_func_EnumObj(req.parameters.enumObj.lpDestDevAddr, req.parameters.enumObj.wStyle, req.parameters.enumObj.enumIdx, req.parameters.enumObj.lpLogObjAddr);
			npdisp_writeMemory16(retValue, req.parameters.enumObj.lpRetValueAddr);
			break;
		}
		case NPDISP_FUNCORDER_GetPalette:
		{
			TRACEOUTP(("GetPalette"));
			npdisp_func_GetPalette(req.parameters.getPalette.nStartIndex, req.parameters.getPalette.nNumEntries, req.parameters.getPalette.lpPaletteAddr);
			break;
		}
		case NPDISP_FUNCORDER_SetPalette:
		{
			TRACEOUTP(("SetPalette"));
			npdisp_func_SetPalette(req.parameters.setPalette.nStartIndex, req.parameters.setPalette.nNumEntries, req.parameters.setPalette.lpPaletteAddr);
			break;
		}
		case NPDISP_FUNCORDER_GetPalTrans:
		{
			TRACEOUTP(("GetPalTrans"));
			npdisp_func_GetPalTrans(req.parameters.getPalTrans.lpIndexesAddr);
			break;
		}
		case NPDISP_FUNCORDER_SetPalTrans:
		{
			TRACEOUTP(("SetPalTrans"));
			npdisp_func_SetPalTrans(req.parameters.setPalTrans.lpIndexesAddr);
			break;
		}
		case NPDISP_FUNCORDER_UpdateColors:
		{
			TRACEOUTP(("UpdateColors"));
			npdisp_func_UpdateColors(req.parameters.updateColors.wStartX, req.parameters.updateColors.wStartY, req.parameters.updateColors.wExtX, req.parameters.updateColors.wExtY, req.parameters.updateColors.lpTranslateAddr);
			break;
		}
		case NPDISP_FUNCORDER_GetCharWidth:
		{
			TRACEOUTP(("GetCharWidth"));
			const UINT16 retValue = npdisp_func_GetCharWidth(req.parameters.getCharWidth.lpDestDevAddr, req.parameters.getCharWidth.lpBufferAddr, req.parameters.getCharWidth.wFirstChar, req.parameters.getCharWidth.wLastChar, req.parameters.getCharWidth.lpFontInfoAddr, req.parameters.getCharWidth.lpDrawModeAddr, req.parameters.getCharWidth.lpFontTransAddr);
			npdisp_writeMemory16(retValue, req.parameters.getCharWidth.lpRetValueAddr);
			break;
		}
		case NPDISP_FUNCORDER_StretchDIBits:
		{
			TRACEOUT(("StretchDIBits"));
			const UINT16 retValue = npdisp_func_StretchDIBits(req.parameters.stretchDIBits.lpPDevice, req.parameters.stretchDIBits.fGet, req.parameters.stretchDIBits.DestX, req.parameters.stretchDIBits.DestY, req.parameters.stretchDIBits.DestXE, req.parameters.stretchDIBits.DestYE, req.parameters.stretchDIBits.SrcX, req.parameters.stretchDIBits.SrcY, req.parameters.stretchDIBits.SrcXE, req.parameters.stretchDIBits.SrcYE, req.parameters.stretchDIBits.lpBitsAddr, req.parameters.stretchDIBits.lpBitmapInfoAddr, req.parameters.stretchDIBits.lpTranslateAddr, req.parameters.stretchDIBits.dwROP, req.parameters.stretchDIBits.lpPBrushAddr, req.parameters.stretchDIBits.lpDrawModeAddr, req.parameters.stretchDIBits.lpClipRecAddr);
			npdisp_writeMemory16(retValue, req.parameters.stretchDIBits.lpRetValueAddr);
			break;
		}
		case NPDISP_FUNCORDER_INT2Fh:
		{
			TRACEOUT(("INT2Fh"));
			npdisp_func_INT2Fh(req.parameters.INT2Fh.ax);
			break;
		}
		case NPDISP_FUNCORDER_WEP:
		{
			TRACEOUT(("WEP"));
			npdisp_func_WEP();
			break;
		}
		case NPDISP_FUNCORDER_ReEnable:
		{
			TRACEOUT9(("ReEnable"));
			const UINT16 retValue = npdisp_func_ReEnable(req.parameters.reEnable.lpPDeviceAddr, req.parameters.reEnable.lpGDIInfoAddr);
			npdisp_writeMemory16(retValue, req.parameters.reEnable.lpRetValueAddr);
			break;
		}
		case NPDISP_FUNCORDER_ValidateMode:
		{
			TRACEOUT9(("ValidateMode"));
			const UINT16 retValue = npdisp_func_ValidateMode(req.parameters.validateMode.lpValModeAddr);
			npdisp_writeMemory16(retValue, req.parameters.validateMode.lpRetValueAddr);
			break;
		}
		case NPDISP_FUNCORDER_SelectBitmap:
		{
			TRACEOUT9(("SelectBitmap"));
			const UINT32 retValue = npdisp_func_SelectBitmap(req.parameters.selectBitmap.lpDeviceAddr, req.parameters.selectBitmap.lpPrevBitmapAddr, req.parameters.selectBitmap.lpBitmapAddr, req.parameters.selectBitmap.fFlags);
			npdisp_writeMemory32(retValue, req.parameters.selectBitmap.lpRetValueAddr);
			break;
		}
		case NPDISP_FUNCORDER_BitmapBits:
		{
			TRACEOUT9(("BitmapBits"));
			const UINT32 retValue = npdisp_func_BitmapBits(req.parameters.bitmapBits.lpDeviceAddr, req.parameters.bitmapBits.fFlags, req.parameters.bitmapBits.dwCount, req.parameters.bitmapBits.lpBitsAddr);
			npdisp_writeMemory32(retValue, req.parameters.bitmapBits.lpRetValueAddr);
			break;
		}
		case NPDISP_FUNCORDER_MEMORYMAP:
		{
			TRACEOUT9(("MEMORYMAP"));
			// VxD用だったが廃止
			//npdisp_func_MEMORYMAP(req.parameters.MEMORYMAP.physicalAddr, req.parameters.MEMORYMAP.linearAddr, req.parameters.MEMORYMAP.farSelector, req.parameters.MEMORYMAP.farOffset);
			break;
		}
		case NPDISP_FUNCORDER_DCI_BEGINACCESS:
		{
			TRACEOUT9(("DCI_BeginAccess"));
			npdisp_func_DCI_BeginAccess(req.parameters.DCI_BeginAccess.lpDeviceAddr, req.parameters.DCI_BeginAccess.lpRectAddr);
			break;
		}
		case NPDISP_FUNCORDER_DCI_ENDACCESS:
		{
			TRACEOUT9(("DCI_EndAccess"));
			npdisp_func_DCI_EndAccess(req.parameters.DCI_EndAccess.lpDeviceAddr);
			break;
		}
		case NPDISP_FUNCORDER_DCI_DESTROYSURFACE:
		{
			TRACEOUT9(("DCI_DestroySurface"));
			npdisp_func_DCI_DestroySurface(req.parameters.DCI_DestroySurface.lpDeviceAddr);
			break;
		}
		default:
			TRACEOUT(("Function %d", req.funcOrder));
			retCode = NPDISP_RETCODE_FAILED;
			break;
		}
		npdisp_writeReturnCode(&req, npdisp.dataAddr, retCode); // ReturnCode書き込み
	}
	
	// 排他終了
	npdispcs_leave_criticalsection();

	// 例外が発生していたらlongjmpで戻る
	if (npdisp.longjmpnum) {
		if (npdisp_memory_hasNewCacheData()) {
			CPU_STAT_EXCEPTION_COUNTER_CLEAR(); // 読み書きが進んでいたら例外繰り返しではない
		}
		int longjmpnum = npdisp.longjmpnum;
		npdisp.longjmpnum_nonfast = longjmpnum; // 最後の例外発生を記憶
		siglongjmp(exec_1step_jmpbuf, longjmpnum); // 転送
	}

	// 例外発生せずに全部送れたらCPUクロックを進め、読み書きバッファはクリアする
	CPU_REMCLOCK -= (npdisp_memory_getTotalReadSize() + npdisp_memory_getTotalWriteSize()) / 4; // 4byteメモリアクセスあたり1clock
	npdisp_memory_clearpreload();
}


#define NPDISP_REQUEST_READFROMSTACK(a, b, c)  npdisp_readMemoryWith32Offset(&(a.b.c), CPU_SS, CPU_BP + 6 + sizeof(a.b) - ((UINT32)((char*)&a.b.c - (char*)&a) - offsetof(NPDISP_REQUEST, parameters.others.arguments)) - sizeof(a.b.c), sizeof(a.b.c))

/// <summary>
/// npdisp_execの高速実行版。スタックから直接パラメータを読み取る。一部のfuncOrderには非対応のため注意。使用するには一度ver.2を指定してnpdisp_execを実行する必要あり。
/// </summary>
/// <param name=""></param>
void npdisp_exec_fast(void) {
	UINT16 lastAX = CPU_AX;
	UINT16 lastDX = CPU_DX;

	UINT16 bx = CPU_BX;

	// 関数番号指定
	npdisp_memory_setFunctionId(bx);

	// 読み書き開始位置を先頭へ戻す
	npdisp_memory_resetposition();

	UINT16 version = npdisp_readMemory16(npdisp.dataAddr); // バージョンだけ取得

	// 排他開始
	npdispcs_enter_criticalsection();

	// スタックの状態は次のようになっている前提
	// [bp + 6以降]　引数（PASCALコール）
	// [bp + 4]  return CS
	// [bp + 2]  return IP
	// [bp + 0]  old BP

	switch (bx) {
	//case NPDISP_FUNCORDER_NP2INITIALIZE: // 非対応
	//{
	//	break;
	//}
	case NPDISP_FUNCORDER_Enable:
	{
		NPDISP_REQUEST req;
		NPDISP_REQUEST_READFROMSTACK(req, parameters.enable, lpDevInfoAddr);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.enable, wStyle);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.enable, lpDestDevTypeAddr);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.enable, lpOutputFileAddr);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.enable, lpDataAddr);

		TRACEOUT(("Enable"));
		const UINT16 retValue = npdisp_func_Enable(req.parameters.enable.lpDevInfoAddr, req.parameters.enable.wStyle, req.parameters.enable.lpDestDevTypeAddr, req.parameters.enable.lpOutputFileAddr, req.parameters.enable.lpDataAddr);

		if (!npdisp.longjmpnum) {
			// 戻り値
			CPU_AX = retValue;

			CPU_CX = 0; // 成功の時CXを0に
		}
		break;
	}
	case NPDISP_FUNCORDER_Disable:
	{
		NPDISP_REQUEST req;
		NPDISP_REQUEST_READFROMSTACK(req, parameters.disable, lpDestDevAddr);

		TRACEOUT(("Disable"));
		npdisp_func_Disable(req.parameters.disable.lpDestDevAddr);
		break;
	}
	case NPDISP_FUNCORDER_GetDriverResourceID:
	{
		NPDISP_REQUEST req;
		NPDISP_REQUEST_READFROMSTACK(req, parameters.GetDriverResourceID, iResId);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.GetDriverResourceID, lpResTypeAddr);

		TRACEOUT(("GetDriverResourceID"));
		const SINT16 retValue = npdisp_func_GetDriverResourceID(req.parameters.GetDriverResourceID.iResId, req.parameters.GetDriverResourceID.lpResTypeAddr);

		if (!npdisp.longjmpnum) {
			// 戻り値
			CPU_AX = retValue;

			CPU_CX = 0; // 成功の時CXを0に
		}
		break;
	}
	case NPDISP_FUNCORDER_ColorInfo:
	{
		NPDISP_REQUEST req;
		NPDISP_REQUEST_READFROMSTACK(req, parameters.ColorInfo, lpDestDevAddr);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.ColorInfo, dwColorin);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.ColorInfo, lpPColorAddr);

		// 色の変換
		UINT32 retValue = 0;
		NPDISP_PDEVICE devInfo;
		UINT32 pcolor;
		npdisp_readMemory(&devInfo, req.parameters.ColorInfo.lpDestDevAddr, 2);
		//if (*((UINT16*)&devInfo) == NPDISP_DEVTYPE || *((UINT16*)&devInfo) == 0) {
		//	npdisp_readMemory(&devInfo, req.parameters.ColorInfo.lpDestDevAddr, sizeof(NPDISP_PBITMAP));
		//}
		if (req.parameters.ColorInfo.lpPColorAddr) {
			npdisp_readMemory(&pcolor, req.parameters.ColorInfo.lpPColorAddr, sizeof(pcolor));
			retValue = npdisp_func_ColorInfo(&devInfo, req.parameters.ColorInfo.dwColorin, &pcolor);
			npdisp_writeMemory(&pcolor, req.parameters.ColorInfo.lpPColorAddr, sizeof(pcolor));
		}
		else {
			retValue = npdisp_func_ColorInfo(&devInfo, req.parameters.ColorInfo.dwColorin, NULL);
		}

		if (!npdisp.longjmpnum) {
			// 戻り値
			CPU_AX = retValue & 0xffff;
			CPU_DX = (retValue >> 16) & 0xffff;

			CPU_CX = 0; // 成功の時CXを0に
		}
		break;
	}
	case NPDISP_FUNCORDER_RealizeObject:
	{
		NPDISP_REQUEST req;
		NPDISP_REQUEST_READFROMSTACK(req, parameters.RealizeObject, lpDestDevAddr);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.RealizeObject, wStyle);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.RealizeObject, lpInObjAddr);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.RealizeObject, lpOutObjAddr);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.RealizeObject, lpTextXFormAddr);

		TRACEOUT(("RealizeObject"));
		// オブジェクト生成と破棄
		const UINT32 retValue = npdisp_func_RealizeObject(req.parameters.RealizeObject.lpDestDevAddr, req.parameters.RealizeObject.wStyle, req.parameters.RealizeObject.lpInObjAddr, req.parameters.RealizeObject.lpOutObjAddr, req.parameters.RealizeObject.lpTextXFormAddr);

		if (!npdisp.longjmpnum) {
			// 戻り値
			CPU_AX = retValue & 0xffff;
			CPU_DX = (retValue >> 16) & 0xffff;

			CPU_CX = 0; // 成功の時CXを0に
		}

		break;
	}
	case NPDISP_FUNCORDER_Control:
	{
		NPDISP_REQUEST req;
		NPDISP_REQUEST_READFROMSTACK(req, parameters.Control, lpDestDevAddr);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.Control, wFunction);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.Control, lpInDataAddr);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.Control, lpOutDataAddr);

		TRACEOUT(("Control"));
		const UINT16 retValue = npdisp_func_Control(req.parameters.Control.lpDestDevAddr, req.parameters.Control.wFunction, req.parameters.Control.lpInDataAddr, req.parameters.Control.lpOutDataAddr);

		if (!npdisp.longjmpnum) {
			// 戻り値
			CPU_AX = retValue;

			CPU_CX = 0; // 成功の時CXを0に
		}
		break;
	}
	case NPDISP_FUNCORDER_BitBlt:
	{
		NPDISP_REQUEST req;
		NPDISP_REQUEST_READFROMSTACK(req, parameters.BitBlt, lpDestDevAddr);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.BitBlt, wDestX);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.BitBlt, wDestY);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.BitBlt, lpSrcDevAddr);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.BitBlt, wSrcX);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.BitBlt, wSrcY);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.BitBlt, wXext);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.BitBlt, wYext);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.BitBlt, Rop3);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.BitBlt, lpPBrushAddr);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.BitBlt, lpDrawModeAddr);

		const UINT16 retValue = npdisp_func_BitBlt(req.parameters.BitBlt.lpDestDevAddr, req.parameters.BitBlt.wDestX, req.parameters.BitBlt.wDestY, req.parameters.BitBlt.lpSrcDevAddr, req.parameters.BitBlt.wSrcX, req.parameters.BitBlt.wSrcY, req.parameters.BitBlt.wXext, req.parameters.BitBlt.wYext, req.parameters.BitBlt.Rop3, req.parameters.BitBlt.lpPBrushAddr, req.parameters.BitBlt.lpDrawModeAddr);

		if (!npdisp.longjmpnum) {
			// 戻り値
			CPU_AX = retValue;

			CPU_CX = 0; // 成功の時CXを0に

			// 処理負荷バランス調整
			int w = req.parameters.BitBlt.wXext < 0 ? -req.parameters.BitBlt.wXext : req.parameters.BitBlt.wXext;
			int h = req.parameters.BitBlt.wYext < 0 ? -req.parameters.BitBlt.wYext : req.parameters.BitBlt.wYext;
			CPU_REMCLOCK -= w * h * pccore.multiple / 8000;
		}
		break;
	}
	case NPDISP_FUNCORDER_StretchBlt:
	{
		NPDISP_REQUEST req;
		NPDISP_REQUEST_READFROMSTACK(req, parameters.stretchBlt, lpDestDevAddr);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.stretchBlt, wDestX);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.stretchBlt, wDestY);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.stretchBlt, wDestXext);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.stretchBlt, wDestYext);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.stretchBlt, lpSrcDevAddr);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.stretchBlt, wSrcX);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.stretchBlt, wSrcY);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.stretchBlt, wSrcXext);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.stretchBlt, wSrcYext);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.stretchBlt, Rop3);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.stretchBlt, lpPBrushAddr);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.stretchBlt, lpDrawModeAddr);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.stretchBlt, lpClipAddr);

		//TRACEOUT(("StretchBlt"));
		const UINT16 retValue = npdisp_func_StretchBlt(req.parameters.stretchBlt.lpDestDevAddr, req.parameters.stretchBlt.wDestX, req.parameters.stretchBlt.wDestY, req.parameters.stretchBlt.wDestXext, req.parameters.stretchBlt.wDestYext, req.parameters.stretchBlt.lpSrcDevAddr, req.parameters.stretchBlt.wSrcX, req.parameters.stretchBlt.wSrcY, req.parameters.stretchBlt.wSrcXext, req.parameters.stretchBlt.wSrcYext, req.parameters.stretchBlt.Rop3, req.parameters.stretchBlt.lpPBrushAddr, req.parameters.stretchBlt.lpDrawModeAddr, req.parameters.stretchBlt.lpClipAddr);

		if (!npdisp.longjmpnum) {
			// 戻り値
			CPU_AX = retValue;

			CPU_CX = 0; // 成功の時CXを0に

			// 処理負荷バランス調整
			int w = req.parameters.stretchBlt.wDestXext < 0 ? -req.parameters.stretchBlt.wDestXext : req.parameters.stretchBlt.wDestXext;
			int h = req.parameters.stretchBlt.wDestYext < 0 ? -req.parameters.stretchBlt.wDestYext : req.parameters.stretchBlt.wDestYext;
			CPU_REMCLOCK -= w * h * pccore.multiple / 8000;
		}
		break;
	}
	case NPDISP_FUNCORDER_DeviceBitmapBits:
	{
		NPDISP_REQUEST req;
		NPDISP_REQUEST_READFROMSTACK(req, parameters.DeviceBitmapBits, lpBitmapAddr);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.DeviceBitmapBits, fGet);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.DeviceBitmapBits, iStart);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.DeviceBitmapBits, cScans);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.DeviceBitmapBits, lpDIBitsAddr);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.DeviceBitmapBits, lpBitmapInfoAddr);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.DeviceBitmapBits, lpDrawModeAddr);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.DeviceBitmapBits, lpTranslateAddr);

		TRACEOUT(("DeviceBitmapBits"));
		const UINT16 retValue = npdisp_func_DeviceBitmapBits(req.parameters.DeviceBitmapBits.lpBitmapAddr, req.parameters.DeviceBitmapBits.fGet, req.parameters.DeviceBitmapBits.iStart, req.parameters.DeviceBitmapBits.cScans, req.parameters.DeviceBitmapBits.lpDIBitsAddr, req.parameters.DeviceBitmapBits.lpBitmapInfoAddr, req.parameters.DeviceBitmapBits.lpDrawModeAddr, req.parameters.DeviceBitmapBits.lpTranslateAddr);

		if (!npdisp.longjmpnum) {
			// 戻り値
			CPU_AX = retValue;

			CPU_CX = 0; // 成功の時CXを0に
		}
		break;
	}
	case NPDISP_FUNCORDER_StrBlt:
	case NPDISP_FUNCORDER_ExtTextOut:
	{
		NPDISP_REQUEST req;
		if (bx == NPDISP_FUNCORDER_StrBlt) {
			TRACEOUT(("StrBlt"));
			NPDISP_REQUEST_READFROMSTACK(req, parameters.strBlt, lpDestDevAddr);
			NPDISP_REQUEST_READFROMSTACK(req, parameters.strBlt, wDestXOrg);
			NPDISP_REQUEST_READFROMSTACK(req, parameters.strBlt, wDestYOrg);
			NPDISP_REQUEST_READFROMSTACK(req, parameters.strBlt, lpClipRectAddr);
			NPDISP_REQUEST_READFROMSTACK(req, parameters.strBlt, lpStringAddr);
			NPDISP_REQUEST_READFROMSTACK(req, parameters.strBlt, wCount);
			NPDISP_REQUEST_READFROMSTACK(req, parameters.strBlt, lpFontInfoAddr);
			NPDISP_REQUEST_READFROMSTACK(req, parameters.strBlt, lpDrawModeAddr);
			NPDISP_REQUEST_READFROMSTACK(req, parameters.strBlt, lpTextXFormAddr);
			req.parameters.extTextOut.lpCharWidthsAddr = 0;
			req.parameters.extTextOut.lpOpaqueRectAddr = 0;
			req.parameters.extTextOut.wOptions = 0;
		}
		else {
			TRACEOUT(("ExtTextOut"));
			NPDISP_REQUEST_READFROMSTACK(req, parameters.extTextOut, lpDestDevAddr);
			NPDISP_REQUEST_READFROMSTACK(req, parameters.extTextOut, wDestXOrg);
			NPDISP_REQUEST_READFROMSTACK(req, parameters.extTextOut, wDestYOrg);
			NPDISP_REQUEST_READFROMSTACK(req, parameters.extTextOut, lpClipRectAddr);
			NPDISP_REQUEST_READFROMSTACK(req, parameters.extTextOut, lpStringAddr);
			NPDISP_REQUEST_READFROMSTACK(req, parameters.extTextOut, wCount);
			NPDISP_REQUEST_READFROMSTACK(req, parameters.extTextOut, lpFontInfoAddr);
			NPDISP_REQUEST_READFROMSTACK(req, parameters.extTextOut, lpDrawModeAddr);
			NPDISP_REQUEST_READFROMSTACK(req, parameters.extTextOut, lpTextXFormAddr);
			NPDISP_REQUEST_READFROMSTACK(req, parameters.extTextOut, lpCharWidthsAddr);
			NPDISP_REQUEST_READFROMSTACK(req, parameters.extTextOut, lpOpaqueRectAddr);
			NPDISP_REQUEST_READFROMSTACK(req, parameters.extTextOut, wOptions);
		}
		const UINT32 retValue = npdisp_func_ExtTextOut(req.parameters.extTextOut.lpDestDevAddr, req.parameters.extTextOut.wDestXOrg, req.parameters.extTextOut.wDestYOrg, req.parameters.extTextOut.lpClipRectAddr, req.parameters.extTextOut.lpStringAddr, req.parameters.extTextOut.wCount, req.parameters.extTextOut.lpFontInfoAddr, req.parameters.extTextOut.lpDrawModeAddr, req.parameters.extTextOut.lpTextXFormAddr, req.parameters.extTextOut.lpCharWidthsAddr, req.parameters.extTextOut.lpOpaqueRectAddr, req.parameters.extTextOut.wOptions);
		
		if (!npdisp.longjmpnum) {
			// 戻り値
			CPU_AX = retValue & 0xffff;
			CPU_DX = (retValue >> 16) & 0xffff;

			CPU_CX = 0; // 成功の時CXを0に

			// 処理負荷バランス調整
			if (req.parameters.extTextOut.wCount > 0) {
				CPU_REMCLOCK -= (int)req.parameters.extTextOut.wCount * pccore.multiple * 10;
			}
		}
		break;
	}
	case NPDISP_FUNCORDER_SetDIBitsToDevice:
	{
		NPDISP_REQUEST req;
		NPDISP_REQUEST_READFROMSTACK(req, parameters.SetDIBitsToDevice, lpDestDevAddr);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.SetDIBitsToDevice, X);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.SetDIBitsToDevice, Y);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.SetDIBitsToDevice, iScan);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.SetDIBitsToDevice, cScans);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.SetDIBitsToDevice, lpClipRectAddr);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.SetDIBitsToDevice, lpDrawModeAddr);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.SetDIBitsToDevice, lpDIBitsAddr);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.SetDIBitsToDevice, lpBitmapInfoAddr);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.SetDIBitsToDevice, lpTranslateAddr);

		TRACEOUT(("SetDIBitsToDevice"));
		const UINT16 retValue = npdisp_func_SetDIBitsToDevice(req.parameters.SetDIBitsToDevice.lpDestDevAddr, req.parameters.SetDIBitsToDevice.X, req.parameters.SetDIBitsToDevice.Y, req.parameters.SetDIBitsToDevice.iScan, req.parameters.SetDIBitsToDevice.cScans, req.parameters.SetDIBitsToDevice.lpClipRectAddr, req.parameters.SetDIBitsToDevice.lpDrawModeAddr, req.parameters.SetDIBitsToDevice.lpDIBitsAddr, req.parameters.SetDIBitsToDevice.lpBitmapInfoAddr, req.parameters.SetDIBitsToDevice.lpTranslateAddr);

		if (!npdisp.longjmpnum) {
			// 戻り値
			CPU_AX = retValue;

			CPU_CX = 0; // 成功の時CXを0に
		}
		break;
	}
	case NPDISP_FUNCORDER_SaveScreenBitmap:
	{
		NPDISP_REQUEST req;
		NPDISP_REQUEST_READFROMSTACK(req, parameters.SaveScreenBitmap, lpRect);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.SaveScreenBitmap, wCommand);

		TRACEOUT(("SaveScreenBitmap"));
		const UINT16 retValue = npdisp_func_SaveScreenBitmap(req.parameters.SaveScreenBitmap.lpRect, req.parameters.SaveScreenBitmap.wCommand);

		if (!npdisp.longjmpnum) {
			// 戻り値
			CPU_AX = retValue;

			CPU_CX = 0; // 成功の時CXを0に
		}
		break;
	}
	case NPDISP_FUNCORDER_SetCursor:
	{
		NPDISP_REQUEST req;
		NPDISP_REQUEST_READFROMSTACK(req, parameters.SetCursor, lpCursorShapeAddr);

		npdisp_func_SetCursor(req.parameters.SetCursor.lpCursorShapeAddr);

		if (!npdisp.longjmpnum) {
			CPU_CX = 0; // 成功の時CXを0に
		}
		break;
	}
	case NPDISP_FUNCORDER_MoveCursor:
	{
		NPDISP_REQUEST req;
		NPDISP_REQUEST_READFROMSTACK(req, parameters.MoveCursor, wAbsX);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.MoveCursor, wAbsY);

		npdisp_func_MoveCursor(req.parameters.MoveCursor.wAbsX, req.parameters.MoveCursor.wAbsY);

		if (!npdisp.longjmpnum) {
			CPU_CX = 0; // 成功の時CXを0に
		}
		break;
	}
	case NPDISP_FUNCORDER_CheckCursor:
	{
		npdisp_func_CheckCursor();

		if (!npdisp.longjmpnum) {
			CPU_CX = 0; // 成功の時CXを0に
		}
		break;
	}
	case NPDISP_FUNCORDER_FastBorder:
	{
		NPDISP_REQUEST req;
		NPDISP_REQUEST_READFROMSTACK(req, parameters.fastBorder, lpRectAddr);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.fastBorder, wHorizBorderThick);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.fastBorder, wVertBorderThick);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.fastBorder, dwRasterOp);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.fastBorder, lpDestDevAddr);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.fastBorder, lpPBrushAddr);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.fastBorder, lpDrawModeAddr);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.fastBorder, lpClipRectAddr);

		TRACEOUT(("FastBorder"));
		const UINT16 retValue = npdisp_func_FastBorder(req.parameters.fastBorder.lpRectAddr, req.parameters.fastBorder.wHorizBorderThick, req.parameters.fastBorder.wVertBorderThick, req.parameters.fastBorder.dwRasterOp, req.parameters.fastBorder.lpDestDevAddr, req.parameters.fastBorder.lpPBrushAddr, req.parameters.fastBorder.lpDrawModeAddr, req.parameters.fastBorder.lpClipRectAddr);

		if (!npdisp.longjmpnum) {
			// 戻り値
			CPU_AX = retValue;

			CPU_CX = 0; // 成功の時CXを0に
		}
		break;
	}
	case NPDISP_FUNCORDER_Output:
	{
		NPDISP_REQUEST req;
		NPDISP_REQUEST_READFROMSTACK(req, parameters.output, lpDestDevAddr);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.output, wStyle);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.output, wCount);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.output, lpPointsAddr);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.output, lpPPenAddr);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.output, lpPBrushAddr);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.output, lpDrawModeAddr);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.output, lpClipRectAddr);

		const UINT16 retValue = npdisp_func_Output(req.parameters.output.lpDestDevAddr, req.parameters.output.wStyle, req.parameters.output.wCount, req.parameters.output.lpPointsAddr, req.parameters.output.lpPPenAddr, req.parameters.output.lpPBrushAddr, req.parameters.output.lpDrawModeAddr, req.parameters.output.lpClipRectAddr);

		if (!npdisp.longjmpnum) {
			// 戻り値
			CPU_AX = retValue;

			CPU_CX = 0; // 成功の時CXを0に
		}
		break;
	}
	case NPDISP_FUNCORDER_Pixel:
	{
		NPDISP_REQUEST req;
		NPDISP_REQUEST_READFROMSTACK(req, parameters.pixel, lpDestDevAddr);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.pixel, X);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.pixel, Y);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.pixel, dwPhysColor);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.pixel, lpDrawModeAddr);

		TRACEOUT(("Pixel"));
		const UINT32 retValue = npdisp_func_Pixel(req.parameters.pixel.lpDestDevAddr, req.parameters.pixel.X, req.parameters.pixel.Y, req.parameters.pixel.dwPhysColor, req.parameters.pixel.lpDrawModeAddr);

		if (!npdisp.longjmpnum) {
			// 戻り値
			CPU_AX = retValue & 0xffff;
			CPU_DX = (retValue >> 16) & 0xffff;

			CPU_CX = 0; // 成功の時CXを0に
		}
		break;
	}
	case NPDISP_FUNCORDER_ScanLR:
	{
		NPDISP_REQUEST req;
		NPDISP_REQUEST_READFROMSTACK(req, parameters.scanLR, lpDestDevAddr);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.scanLR, X);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.scanLR, Y);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.scanLR, dwPhysColor);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.scanLR, Style);

		TRACEOUT(("ScanLR"));
		const UINT16 retValue = npdisp_func_ScanLR(req.parameters.scanLR.lpDestDevAddr, req.parameters.scanLR.X, req.parameters.scanLR.Y, req.parameters.scanLR.dwPhysColor, req.parameters.scanLR.Style);

		if (!npdisp.longjmpnum) {
			// 戻り値
			CPU_AX = retValue;

			CPU_CX = 0; // 成功の時CXを0に
		}
		break;
	}
	//case NPDISP_FUNCORDER_EnumObj: // 非対応
	//{
	//	break;
	//}
	case NPDISP_FUNCORDER_GetPalette:
	{
		NPDISP_REQUEST req;
		NPDISP_REQUEST_READFROMSTACK(req, parameters.getPalette, nStartIndex);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.getPalette, nNumEntries);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.getPalette, lpPaletteAddr);

		TRACEOUTP(("GetPalette"));
		npdisp_func_GetPalette(req.parameters.getPalette.nStartIndex, req.parameters.getPalette.nNumEntries, req.parameters.getPalette.lpPaletteAddr);

		if (!npdisp.longjmpnum) {
			CPU_CX = 0; // 成功の時CXを0に
		}
		break;
	}
	case NPDISP_FUNCORDER_SetPalette:
	{
		NPDISP_REQUEST req;
		NPDISP_REQUEST_READFROMSTACK(req, parameters.setPalette, nStartIndex);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.setPalette, nNumEntries);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.setPalette, lpPaletteAddr);

		TRACEOUTP(("SetPalette"));
		npdisp_func_SetPalette(req.parameters.setPalette.nStartIndex, req.parameters.setPalette.nNumEntries, req.parameters.setPalette.lpPaletteAddr);

		if (!npdisp.longjmpnum) {
			CPU_CX = 0; // 成功の時CXを0に
		}
		break;
	}
	case NPDISP_FUNCORDER_GetPalTrans:
	{
		NPDISP_REQUEST req;
		NPDISP_REQUEST_READFROMSTACK(req, parameters.getPalTrans, lpIndexesAddr);

		TRACEOUTP(("GetPalTrans"));
		npdisp_func_GetPalTrans(req.parameters.getPalTrans.lpIndexesAddr);

		if (!npdisp.longjmpnum) {
			CPU_CX = 0; // 成功の時CXを0に
		}
		break;
	}
	case NPDISP_FUNCORDER_SetPalTrans:
	{
		NPDISP_REQUEST req;
		NPDISP_REQUEST_READFROMSTACK(req, parameters.setPalTrans, lpIndexesAddr);

		TRACEOUTP(("SetPalTrans"));
		npdisp_func_SetPalTrans(req.parameters.setPalTrans.lpIndexesAddr);

		if (!npdisp.longjmpnum) {
			CPU_CX = 0; // 成功の時CXを0に
		}
		break;
	}
	case NPDISP_FUNCORDER_UpdateColors:
	{
		NPDISP_REQUEST req;
		NPDISP_REQUEST_READFROMSTACK(req, parameters.updateColors, wStartX);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.updateColors, wStartY);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.updateColors, wExtX);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.updateColors, wExtY);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.updateColors, lpTranslateAddr);

		TRACEOUTP(("UpdateColors"));
		npdisp_func_UpdateColors(req.parameters.updateColors.wStartX, req.parameters.updateColors.wStartY, req.parameters.updateColors.wExtX, req.parameters.updateColors.wExtY, req.parameters.updateColors.lpTranslateAddr);

		if (!npdisp.longjmpnum) {
			CPU_CX = 0; // 成功の時CXを0に
		}
		break;
	}
	case NPDISP_FUNCORDER_GetCharWidth:
	{
		NPDISP_REQUEST req;
		NPDISP_REQUEST_READFROMSTACK(req, parameters.getCharWidth, lpDestDevAddr);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.getCharWidth, lpBufferAddr);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.getCharWidth, wFirstChar);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.getCharWidth, wLastChar);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.getCharWidth, lpFontInfoAddr);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.getCharWidth, lpDrawModeAddr);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.getCharWidth, lpFontTransAddr);

		TRACEOUTP(("GetCharWidth"));
		const UINT16 retValue = npdisp_func_GetCharWidth(req.parameters.getCharWidth.lpDestDevAddr, req.parameters.getCharWidth.lpBufferAddr, req.parameters.getCharWidth.wFirstChar, req.parameters.getCharWidth.wLastChar, req.parameters.getCharWidth.lpFontInfoAddr, req.parameters.getCharWidth.lpDrawModeAddr, req.parameters.getCharWidth.lpFontTransAddr);

		if (!npdisp.longjmpnum) {
			// 戻り値
			CPU_AX = retValue;

			CPU_CX = 0; // 成功の時CXを0に
		}
		break;
	}
	case NPDISP_FUNCORDER_StretchDIBits:
	{
		NPDISP_REQUEST req;
		NPDISP_REQUEST_READFROMSTACK(req, parameters.stretchDIBits, lpPDevice);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.stretchDIBits, fGet);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.stretchDIBits, DestX);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.stretchDIBits, DestY);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.stretchDIBits, DestXE);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.stretchDIBits, DestYE);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.stretchDIBits, SrcX);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.stretchDIBits, SrcY);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.stretchDIBits, SrcXE);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.stretchDIBits, SrcYE);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.stretchDIBits, lpBitsAddr);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.stretchDIBits, lpBitmapInfoAddr);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.stretchDIBits, lpTranslateAddr);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.stretchDIBits, dwROP);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.stretchDIBits, lpPBrushAddr);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.stretchDIBits, lpDrawModeAddr);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.stretchDIBits, lpClipRecAddr);

		TRACEOUT(("StretchDIBits"));
		const UINT16 retValue = npdisp_func_StretchDIBits(req.parameters.stretchDIBits.lpPDevice, req.parameters.stretchDIBits.fGet, req.parameters.stretchDIBits.DestX, req.parameters.stretchDIBits.DestY, req.parameters.stretchDIBits.DestXE, req.parameters.stretchDIBits.DestYE, req.parameters.stretchDIBits.SrcX, req.parameters.stretchDIBits.SrcY, req.parameters.stretchDIBits.SrcXE, req.parameters.stretchDIBits.SrcYE, req.parameters.stretchDIBits.lpBitsAddr, req.parameters.stretchDIBits.lpBitmapInfoAddr, req.parameters.stretchDIBits.lpTranslateAddr, req.parameters.stretchDIBits.dwROP, req.parameters.stretchDIBits.lpPBrushAddr, req.parameters.stretchDIBits.lpDrawModeAddr, req.parameters.stretchDIBits.lpClipRecAddr);

		if (!npdisp.longjmpnum) {
			// 戻り値
			CPU_AX = retValue;

			CPU_CX = 0; // 成功の時CXを0に
		}
		break;
	}
	case NPDISP_FUNCORDER_ValidateMode:
	{
		NPDISP_REQUEST req;
		NPDISP_REQUEST_READFROMSTACK(req, parameters.validateMode, lpValModeAddr);

		TRACEOUT9(("ValidateMode"));
		const UINT16 retValue = npdisp_func_ValidateMode(req.parameters.validateMode.lpValModeAddr);

		if (!npdisp.longjmpnum) {
			// 戻り値
			CPU_AX = retValue;

			CPU_CX = 0; // 成功の時CXを0に
		}
		break;
	}
	case NPDISP_FUNCORDER_SelectBitmap:
	{
		NPDISP_REQUEST req;
		NPDISP_REQUEST_READFROMSTACK(req, parameters.selectBitmap, lpDeviceAddr);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.selectBitmap, lpPrevBitmapAddr);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.selectBitmap, lpBitmapAddr);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.selectBitmap, fFlags);

		TRACEOUT9(("SelectBitmap"));
		const UINT32 retValue = npdisp_func_SelectBitmap(req.parameters.selectBitmap.lpDeviceAddr, req.parameters.selectBitmap.lpPrevBitmapAddr, req.parameters.selectBitmap.lpBitmapAddr, req.parameters.selectBitmap.fFlags);

		if (!npdisp.longjmpnum) {
			// 戻り値
			CPU_AX = retValue & 0xffff;
			CPU_DX = (retValue >> 16) & 0xffff;

			CPU_CX = 0; // 成功の時CXを0に
		}
		break;
	}
	case NPDISP_FUNCORDER_BitmapBits:
	{
		NPDISP_REQUEST req;
		NPDISP_REQUEST_READFROMSTACK(req, parameters.bitmapBits, lpDeviceAddr);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.bitmapBits, fFlags);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.bitmapBits, dwCount);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.bitmapBits, lpBitsAddr);

		TRACEOUT9(("BitmapBits"));
		const UINT32 retValue = npdisp_func_BitmapBits(req.parameters.bitmapBits.lpDeviceAddr, req.parameters.bitmapBits.fFlags, req.parameters.bitmapBits.dwCount, req.parameters.bitmapBits.lpBitsAddr);

		if (!npdisp.longjmpnum) {
			// 戻り値
			CPU_AX = retValue & 0xffff;
			CPU_DX = (retValue >> 16) & 0xffff;

			CPU_CX = 0; // 成功の時CXを0に
		}
		break;
	}
	case NPDISP_FUNCORDER_DCI_BEGINACCESS:
	{
		NPDISP_REQUEST req;
		NPDISP_REQUEST_READFROMSTACK(req, parameters.DCI_BeginAccess, lpDeviceAddr);
		NPDISP_REQUEST_READFROMSTACK(req, parameters.DCI_BeginAccess, lpRectAddr);

		TRACEOUT9(("DCI_BeginAccess"));
		const UINT16 retValue = npdisp_func_DCI_BeginAccess(req.parameters.DCI_BeginAccess.lpDeviceAddr, req.parameters.DCI_BeginAccess.lpRectAddr);

		if (!npdisp.longjmpnum) {
			// 戻り値
			CPU_AX = retValue & 0xffff;

			CPU_CX = 0; // 成功の時CXを0に
		}
		break;
	}
	case NPDISP_FUNCORDER_DCI_ENDACCESS:
	{
		NPDISP_REQUEST req;
		NPDISP_REQUEST_READFROMSTACK(req, parameters.DCI_EndAccess, lpDeviceAddr);

		TRACEOUT9(("DCI_EndAccess"));
		npdisp_func_DCI_EndAccess(req.parameters.DCI_EndAccess.lpDeviceAddr);

		if (!npdisp.longjmpnum) {
			CPU_CX = 0; // 成功の時CXを0に
		}
		break;
	}
	case NPDISP_FUNCORDER_DCI_DESTROYSURFACE:
	{
		NPDISP_REQUEST req;
		NPDISP_REQUEST_READFROMSTACK(req, parameters.DCI_DestroySurface, lpDeviceAddr);

		TRACEOUT9(("DCI_DestroySurface"));
		npdisp_func_DCI_DestroySurface(req.parameters.DCI_DestroySurface.lpDeviceAddr);

		if (!npdisp.longjmpnum) {
			CPU_CX = 0; // 成功の時CXを0に
		}
		break;
	}
	case NPDISP_FUNCORDER_INT2Fh:
	{
		TRACEOUT(("INT2Fh"));
		npdisp_func_INT2Fh(CPU_SI); // 特例 SIに元のAXの値を格納すること

		if (!npdisp.longjmpnum) {
			CPU_CX = 0; // 成功の時CXを0に
		}
		break;
	}
	case NPDISP_FUNCORDER_WEP:
	{
		TRACEOUT(("WEP"));
		npdisp_func_WEP();

		if (!npdisp.longjmpnum) {
			CPU_CX = 0; // 成功の時CXを0に
		}
		break;
	}
	default:
	{
		TRACEOUT(("npdisp_exec_fast not supported (Function %d).", bx));
		break;
	}
	}

	// 排他終了
	npdispcs_leave_criticalsection();

	// 例外が発生していたらlongjmpで戻る
	if (npdisp.longjmpnum) {
		if (npdisp_memory_hasNewCacheData()) {
			CPU_STAT_EXCEPTION_COUNTER_CLEAR(); // 読み書きが進んでいたら例外繰り返しではない
		}
		TRACEOUTF(("EXCEPTION!!!!!!"));

		// 戻れるようにレジスタセット
		CPU_AX = lastAX;
		CPU_DX = lastDX;
		CPU_CX = (NPDISP_EXEC_MAGIC & 0xffff);

		int longjmpnum = npdisp.longjmpnum;
		siglongjmp(exec_1step_jmpbuf, longjmpnum); // 転送
	}

	// 例外発生せずに全部送れたらCPUクロックを進め、読み書きバッファはクリアする
	CPU_REMCLOCK -= (npdisp_memory_getTotalReadSize() + npdisp_memory_getTotalWriteSize()) / 4; // 4byteメモリアクセスあたり1clock
	npdisp_memory_clearpreload();
}

 // ---------- IO Ports

static int npdisp_debug_seqCounter = 0;

static char dbgBuf[32] = { 0 };
static int dbgBufIdx = 0;
static void IOOUTCALL npdisp_o7e7(UINT port, REG8 dat)
{
	dbgBuf[dbgBufIdx] = dat;
	dbgBufIdx++;
	if (dbgBufIdx >= sizeof(dbgBuf) - 1) {
		dbgBufIdx = 0;
	}
}

static void IOOUTCALL npdisp_o7e8(UINT port, REG8 dat)
{
	npdisp.dataAddr = (dat << 24) | (npdisp.dataAddr >> 8);
	//if (npdisp_debug_seqCounter >= 4) {
	//	TRACEOUT(("ADDRESS ERROR! %d %08x %08x", npdisp_debug_seqCounter, CPU_SS, lastID));
	//}
	//else {
	//	//TRACEOUT(("ADDRESS %d %08x", npdisp_debug_seqCounter, CPU_SS));
	//}
	npdisp_debug_seqCounter++;
	(void)port;
}

static void IOOUTCALL npdisp_o7e9(UINT port, REG8 dat)
{
	if (npdisp.version >= 2 && npdisp.enabled && dat == 'F' && CPU_CX == (NPDISP_EXEC_MAGIC & 0xffff)) {
		// 高速実行パス ver.2以降で対応
		npdisp_exec_fast();
		return;
	}

	const int retFromException = (dat == '1' && npdisp.longjmpnum_nonfast != 0);
	if (npdisp.cmdBuf != NPDISP_EXEC_MAGIC || !retFromException) {
		npdisp.cmdBuf = (dat << 24) | (npdisp.cmdBuf >> 8);
		if (npdisp.longjmpnum_nonfast && npdisp_memory_getLastEIP() != CPU_EIP) {
			// 例外処理中に他が来た場合は放棄
			npdisp_memory_setFunctionId(0); // 共用先読みバッファ
			npdisp_memory_clearpreload();
			TRACEOUTF(("DISCARD! %c %08x", (char)dat, CPU_EIP));
		}
	}
	else {
		// 例外復帰の再実行を認める
		TRACEOUTF(("EXCEPTION!!!!!!!!!!!!: %c", (char)dat));
	}

	// エクスポート関数処理実行
	if (npdisp.cmdBuf == NPDISP_EXEC_MAGIC) {
		npdisp_debug_seqCounter = 0;
		npdisp_exec();
	}

	(void)port;
}

static REG8 IOINPCALL npdisp_i7e8(UINT port)
{
	return(98);
}

static REG8 IOINPCALL npdisp_i7e9(UINT port)
{
	return(21);
}

int npdisp_drawGraphic(void) 
{
	UINT32 updated;
	UINT32 paletteUpdated;
	HDC hdc = np2wabwnd.hDCBuf;

	if (!npdispwin.hdc) return 0;

	updated = npdisp.updated;
	paletteUpdated = npdisp.paletteUpdated;
	npdisp.updated = 0;
	npdisp.paletteUpdated = 0;

	if (!updated && !paletteUpdated) return 0;

	np2wab.realWidth = npdisp.width;
	np2wab.realHeight = npdisp.height;

	npdispcs_enter_criticalsection();
	//if (paletteUpdated) {
	//	npdisp_updatePalette();
	//}

	// カーソル再描画判定
	if (npdispwin.hBmpCursorMask && npdispwin.hBmpCursor) {
		if (npdispwin.cursorUpdated) {
			// カーソル自体に更新があれば旧カーソル領域と新カーソル領域を足す
			if (npdispwin.lastCursorRect.left != npdispwin.lastCursorRect.right && npdispwin.lastCursorRect.top != npdispwin.lastCursorRect.bottom) {
				npdisp_setDirty(npdispwin.lastCursorRect.left, npdispwin.lastCursorRect.top, npdispwin.lastCursorRect.right, npdispwin.lastCursorRect.bottom);
			}
			npdisp_setDirty(
				npdisp.cursorX - npdisp.cursorHotSpotX,
				npdisp.cursorY - npdisp.cursorHotSpotY,
				npdisp.cursorX - npdisp.cursorHotSpotX + npdisp.cursorWidth,
				npdisp.cursorY - npdisp.cursorHotSpotY + npdisp.cursorHeight);
			npdispwin.cursorUpdated = 0;
		}
		else if (npdispwin.dirtyRect.left != npdispwin.dirtyRect.right && npdispwin.dirtyRect.top != npdispwin.dirtyRect.bottom) {
			// カーソル自体に更新がない場合、カーソル領域とDirtyRectが交差していれば領域に追加する
			bool intersects = npdisp.cursorX - npdisp.cursorHotSpotX < npdispwin.dirtyRect.right &&
				npdispwin.dirtyRect.left < npdisp.cursorX - npdisp.cursorHotSpotX + npdisp.cursorWidth &&
				npdisp.cursorY - npdisp.cursorHotSpotY < npdispwin.dirtyRect.bottom &&
				npdispwin.dirtyRect.top < npdisp.cursorHotSpotY + npdisp.cursorHeight;
			if (intersects) {
				npdisp_setDirty(
					npdisp.cursorX - npdisp.cursorHotSpotX,
					npdisp.cursorY - npdisp.cursorHotSpotY,
					npdisp.cursorX - npdisp.cursorHotSpotX + npdisp.cursorWidth,
					npdisp.cursorY - npdisp.cursorHotSpotY + npdisp.cursorHeight);
			}
		}
	}
	else if(npdispwin.lastCursorRect.left != npdispwin.lastCursorRect.right && npdispwin.lastCursorRect.top != npdispwin.lastCursorRect.bottom) {
		// カーソル非表示で前回カーソル表示状態なら前回カーソル領域を足す
		npdisp_setDirty(npdispwin.lastCursorRect.left, npdispwin.lastCursorRect.top, npdispwin.lastCursorRect.right, npdispwin.lastCursorRect.bottom);
	}

	// DirtyRectの範囲で更新
	if (npdispwin.dirtyRect.left != npdispwin.dirtyRect.right && npdispwin.dirtyRect.top != npdispwin.dirtyRect.bottom) {
		bool palChanged = false;
		if (npdisp.usePalette) {
			// グレースケールから実際のデバイス色へ置き換え
			SetDIBColorTable(npdispwin.hdc, 0, 256, (RGBQUAD*)npdisp_palette_rgb256);
			palChanged = true;
		}
		BitBlt(hdc, npdispwin.dirtyRect.left, npdispwin.dirtyRect.top, npdispwin.dirtyRect.right - npdispwin.dirtyRect.left, npdispwin.dirtyRect.bottom - npdispwin.dirtyRect.top, npdispwin.hdc, npdispwin.dirtyRect.left, npdispwin.dirtyRect.top, SRCCOPY);
		//BitBlt(hdc, npdisp.width - 256, 0, npdisp.width, npdisp.height, npdispwin.hdcBltBuf, 0, 0, SRCCOPY);
		if (npdispwin.hBmpCursorMask && npdispwin.hBmpCursor) {
			SetTextColor(npdispwin.hdcCursorMask, 0);
			SetBkColor(npdispwin.hdcCursorMask, 0xffffff);
			SetTextColor(npdispwin.hdcCursor, 0);
			SetBkColor(npdispwin.hdcCursor, 0xffffff);
			BitBlt(hdc, npdisp.cursorX - npdisp.cursorHotSpotX, npdisp.cursorY - npdisp.cursorHotSpotY, npdisp.cursorWidth, npdisp.cursorHeight, npdispwin.hdcCursorMask, 0, 0, SRCAND);
			BitBlt(hdc, npdisp.cursorX - npdisp.cursorHotSpotX, npdisp.cursorY - npdisp.cursorHotSpotY, npdisp.cursorWidth, npdisp.cursorHeight, npdispwin.hdcCursor, 0, 0, SRCINVERT);
		}
		else {
			//// Test用
			//BitBlt(hdc, npdisp.cursorX, npdisp.cursorY, 4, 4, NULL, 0, 0, BLACKNESS);
			//BitBlt(hdc, npdisp.cursorX + 1, npdisp.cursorY + 1, 2, 2, NULL, 0, 0, WHITENESS);
		}
		if (palChanged) {
			// 描画後に元のグレースケールへ戻す
			SetDIBColorTable(npdispwin.hdc, 0, 256, (RGBQUAD*)npdisp_palette_gray256);
		}

		//// DEBUG: DirtyRect確認用
		//HGDIOBJ oldPen = SelectObject(npdispwin.hdc, GetStockObject(BLACK_PEN));
		//HGDIOBJ oldBrush = SelectObject(npdispwin.hdc, GetStockObject(NULL_BRUSH));
		//Rectangle(npdispwin.hdc, npdispwin.dirtyRect.left, npdispwin.dirtyRect.top, npdispwin.dirtyRect.right, npdispwin.dirtyRect.bottom);
		//SelectObject(npdispwin.hdc, oldPen);
		//SelectObject(npdispwin.hdc, oldBrush);

		// WAB転送時のDirtyRect設定
		np2wab_setDirtyRect(npdispwin.dirtyRect);

		// DirtyRectリセット
		npdisp_resetDirty();

		// 旧カーソル位置を記憶
		if (npdispwin.hBmpCursorMask && npdispwin.hBmpCursor) {
			npdispwin.lastCursorRect.left = npdisp.cursorX - npdisp.cursorHotSpotX;
			npdispwin.lastCursorRect.top = npdisp.cursorY - npdisp.cursorHotSpotY;
			npdispwin.lastCursorRect.right = npdisp.cursorX - npdisp.cursorHotSpotX + npdisp.cursorWidth;
			npdispwin.lastCursorRect.bottom = npdisp.cursorY - npdisp.cursorHotSpotY + npdisp.cursorHeight;
		}
		else {
			// カーソル非表示ならクリア
			npdispwin.lastCursorRect.left = npdispwin.lastCursorRect.top = npdispwin.lastCursorRect.right = npdispwin.lastCursorRect.bottom = 0;
		}
	}
	npdispcs_leave_criticalsection();

	return 1;
}

static void npdisp_releaseScreen(bool resize) {
	if (npdispwin.hdc) {
		SelectObject(npdispwin.hdc, npdispwin.hOldPen);
		SelectObject(npdispwin.hdc, npdispwin.hOldBrush);
		if (!resize) {
			for (auto it = npdispwin.pens.begin(); it != npdispwin.pens.end(); ++it) {
				if (it->second.pen) DeleteObject(it->second.pen);
			}
			npdispwin.pens.clear();
			npdispwin.pensIdx = 1;
			for (auto it = npdispwin.brushes.begin(); it != npdispwin.brushes.end(); ++it) {
				if (it->second.brs) DeleteObject(it->second.brs);
			}
			npdispwin.brushes.clear();
			npdispwin.brushesIdx = 1;
			for (auto it = npdispwin.bitmaps.begin(); it != npdispwin.bitmaps.end(); ++it) {
				if (it->second.bmphdc.hBmp) npdisp_FreeBitmap(&it->second.bmphdc);
			}
			npdispwin.bitmaps.clear();
			npdispwin.bitmapsIdx = 1;
		}
		SelectObject(npdispwin.hdc, npdispwin.hOldBmp);
		DeleteObject(npdispwin.hBmp);
		SelectObject(npdispwin.hdcShadow, npdispwin.hOldBmpShadow);
		SelectObject(npdispwin.hdcBltBuf, npdispwin.hOldBmpBltBuf);
		//SelectObject(npdispwin.hdc16BltBuf, npdispwin.hOldBmp16BltBuf);
		DeleteObject(npdispwin.hBmpShadow);
		DeleteObject(npdispwin.hBmpBltBuf);
		if (npdispwin.hOldhFont) SelectObject(npdispwin.hdc, npdispwin.hOldhFont);
		DeleteObject(npdispwin.hFont);
		DeleteDC(npdispwin.hdc);
		DeleteDC(npdispwin.hdcShadow);
		DeleteDC(npdispwin.hdcBltBuf);
		//DeleteDC(npdispwin.hdc16BltBuf);
		npdispwin.hdc = NULL;
		npdispwin.hBmp = NULL;
		npdispwin.hOldBmp = NULL;
		npdispwin.pBits = NULL;
		npdispwin.hdcShadow = NULL;
		npdispwin.hBmpShadow = NULL;
		npdispwin.hOldBmpShadow = NULL;
		npdispwin.pBitsShadow = NULL;
		npdispwin.hdcBltBuf = NULL;
		npdispwin.hBmpBltBuf = NULL;
		npdispwin.hOldBmpBltBuf = NULL;
		npdispwin.pBitsBltBuf = NULL;
		//npdispwin.hBmp16BltBuf = NULL;
		//npdispwin.hOldBmp16BltBuf = NULL;
		npdispwin.hFont = NULL;
		npdispwin.hOldhFont = NULL;

		if (npdisp.cursorBpp <= 1) {
			// 1bppは自前mallocなのでfreeする
			if (npdispwin.pBitsCursor) {
				free(npdispwin.pBitsCursor);
			}
			if (npdispwin.pBitsCursorMask) {
				free(npdispwin.pBitsCursorMask);
			}
		}
		npdispwin.pBitsCursor = NULL;
		npdispwin.pBitsCursorMask = NULL;

		for (int i = 0; i < NELEMENTS(npdispwin.hdcCache); i++) {
			if (npdispwin.hdcCache[i]) {
				DeleteDC(npdispwin.hdcCache[i]);
				npdispwin.hdcCache[i] = NULL;
			}
		}

		if (npdispwin.hdcCursor) {
			if (npdispwin.hBmpCursor) {
				SelectObject(npdispwin.hdcCursor, npdispwin.hOldBmpCursor);
				DeleteObject(npdispwin.hBmpCursor);
				npdispwin.hBmpCursor = NULL;
			}
			DeleteDC(npdispwin.hdcCursor);
			npdispwin.hdcCursor = NULL;
		}
		if (npdispwin.hdcCursorMask) {
			if (npdispwin.hBmpCursorMask) {
				SelectObject(npdispwin.hdcCursorMask, npdispwin.hOldBmpCursorMask);
				DeleteObject(npdispwin.hBmpCursorMask);
				npdispwin.hBmpCursorMask = NULL;
			}
			DeleteDC(npdispwin.hdcCursorMask);
			npdispwin.hdcCursorMask = NULL;
		}
		npdisp.cursorHotSpotX = 0;
		npdisp.cursorHotSpotY = 0;
		npdisp.cursorWidth = 0;
		npdisp.cursorHeight = 0;

		npdisp.mm_screenPtr = NULL;
		npdisp.mm_screenSize = 0;
	}
}
static void npdisp_createScreen(bool resize) {
	const int width = npdisp.width;
	const int height = npdisp.height;
	int i;

	npdisp_releaseScreen(resize);

	HDC hdcScreen = GetDC(NULL);
	npdispwin.hdc = CreateCompatibleDC(hdcScreen);
	npdispwin.hdcShadow = CreateCompatibleDC(hdcScreen);
	npdispwin.hdcBltBuf = CreateCompatibleDC(hdcScreen);
	//npdispwin.hdc16BltBuf = CreateCompatibleDC(hdcScreen);

	int colors = (npdisp.bpp <= 8) ? (1 << npdisp.bpp) : 0;

	npdispwin.bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	npdispwin.bi.bmiHeader.biWidth = width;
	npdispwin.bi.bmiHeader.biHeight = -height; 
	npdispwin.bi.bmiHeader.biPlanes = 1;
	npdispwin.bi.bmiHeader.biBitCount = npdisp.bpp;
	npdispwin.bi.bmiHeader.biCompression = BI_RGB;
	npdispwin.bi.bmiHeader.biSizeImage = 0;
	npdispwin.bi.bmiHeader.biXPelsPerMeter = 0;
	npdispwin.bi.bmiHeader.biYPelsPerMeter = 0;
	npdispwin.bi.bmiHeader.biClrUsed = colors;
	npdispwin.bi.bmiHeader.biClrImportant = colors;

	if (colors == 2) {
		// 2色パレットセット
		memcpy(npdispwin.bi.bmiColors, npdisp_palette_rgb2, sizeof(RGBQUAD) * NELEMENTS(npdisp_palette_rgb2));
	}
	else if (colors == 16) {
		// 16色パレットセット
		memcpy(npdispwin.bi.bmiColors, npdisp_palette_rgb16, sizeof(RGBQUAD) * NELEMENTS(npdisp_palette_rgb16));
	}
	else if (colors == 256) {
		// 256色パレットセット
		memcpy(npdispwin.bi.bmiColors, npdisp_palette_gray256, sizeof(RGBQUAD) * NELEMENTS(npdisp_palette_gray256));
	}

	if (npdisp.bpp == 16) {
		// ビットフィールド 565
		npdispwin.bi.bmiHeader.biCompression = BI_BITFIELDS;
		*((DWORD*)(npdispwin.bi.bmiColors + 0)) = 0x0000F800;
		*((DWORD*)(npdispwin.bi.bmiColors + 1)) = 0x000007E0;
		*((DWORD*)(npdispwin.bi.bmiColors + 2)) = 0x0000001F;
	}
	else if (npdisp.bpp == 15) {
		// ビットフィールド 555
		npdispwin.bi.bmiHeader.biCompression = BI_BITFIELDS;
		*((DWORD*)(npdispwin.bi.bmiColors + 0)) = 0x00007C00;
		*((DWORD*)(npdispwin.bi.bmiColors + 1)) = 0x000003E0;
		*((DWORD*)(npdispwin.bi.bmiColors + 2)) = 0x0000001F;
		npdispwin.bi.bmiHeader.biBitCount = 16;
	}

	npdispwin.hBmp = CreateDIBSection(hdcScreen, (BITMAPINFO*)&npdispwin.bi, DIB_RGB_COLORS, &npdispwin.pBits, NULL, 0);
	if (!npdispwin.hBmp || !npdispwin.pBits) {
		DeleteDC(npdispwin.hdc);
		npdispwin.hdc = NULL;
		DeleteDC(npdispwin.hdcShadow);
		npdispwin.hdcShadow = NULL;
		DeleteDC(npdispwin.hdcBltBuf);
		npdispwin.hdcBltBuf = NULL;
		ReleaseDC(NULL, hdcScreen);
		return;
	}
	npdispwin.hBmpShadow = CreateDIBSection(hdcScreen, (BITMAPINFO*)&npdispwin.bi, DIB_RGB_COLORS, &npdispwin.pBitsShadow, NULL, 0);
	if (!npdispwin.hBmpShadow || !npdispwin.pBitsShadow) {
		DeleteObject(npdispwin.hBmp);
		npdispwin.hBmp = NULL;
		DeleteDC(npdispwin.hdc);
		npdispwin.hdc = NULL;
		DeleteDC(npdispwin.hdcShadow);
		npdispwin.hdcShadow = NULL;
		DeleteDC(npdispwin.hdcBltBuf);
		npdispwin.hdcBltBuf = NULL;
		ReleaseDC(NULL, hdcScreen);
		return;
	}
	npdispwin.hBmpBltBuf = CreateDIBSection(hdcScreen, (BITMAPINFO*)&npdispwin.bi, DIB_RGB_COLORS, &npdispwin.pBitsBltBuf, NULL, 0);
	if (!npdispwin.hBmpBltBuf) {
		DeleteObject(npdispwin.hBmpShadow);
		npdispwin.hBmpShadow = NULL;
		DeleteObject(npdispwin.hBmp);
		npdispwin.hBmp = NULL;
		DeleteDC(npdispwin.hdc);
		npdispwin.hdc = NULL;
		DeleteDC(npdispwin.hdcShadow);
		npdispwin.hdcShadow = NULL;
		DeleteDC(npdispwin.hdcBltBuf);
		npdispwin.hdcBltBuf = NULL;
		ReleaseDC(NULL, hdcScreen);
		return;
	}

	//npdispwin.hBmp16BltBuf = CreateCompatibleBitmap(hdcScreen, width, height);

	ReleaseDC(NULL, hdcScreen); // もういらない

	npdispwin.hOldPen = SelectObject(npdispwin.hdc, GetStockObject(WHITE_PEN));
	npdispwin.hOldBrush = SelectObject(npdispwin.hdc, GetStockObject(BLACK_BRUSH));

	npdispwin.stride = ((width * npdispwin.bi.bmiHeader.biBitCount + 31) / 32) * 4;
	memset(npdispwin.pBits, 0x00, npdispwin.stride * height);

	npdisp.mm_screenPtr = (UINT8*)npdispwin.pBits;
	npdisp.mm_screenSize = width * npdispwin.stride;

	npdispwin.hOldBmp = SelectObject(npdispwin.hdc, npdispwin.hBmp);
	npdispwin.hOldBmpShadow = SelectObject(npdispwin.hdcShadow, npdispwin.hBmpShadow);
	npdispwin.hOldBmpBltBuf = SelectObject(npdispwin.hdcBltBuf, npdispwin.hBmpBltBuf);
	//npdispwin.hOldBmp16BltBuf = SelectObject(npdispwin.hdc16BltBuf, npdispwin.hBmp16BltBuf);

	for (int i = 0; i < NELEMENTS(npdispwin.hdcCache); i++) {
		if (!npdispwin.hdcCache[i]) {
			npdispwin.hdcCache[i] = CreateCompatibleDC(NULL);
		}
	}

	npdispwin.rectShadow.left = 0;
	npdispwin.rectShadow.right = 0;
	npdispwin.rectShadow.top = 0;
	npdispwin.rectShadow.bottom = 0;

	npdispwin.hdcCursor = CreateCompatibleDC(NULL);
	npdispwin.hdcCursorMask = CreateCompatibleDC(NULL);

	BitBlt(npdispwin.hdcShadow, 0, 0, npdisp.width, npdisp.height, npdispwin.hdc, 0, 0, BLACKNESS);
	BitBlt(npdispwin.hdcBltBuf, 0, 0, npdisp.width, npdisp.height, npdispwin.hdc, 0, 0, BLACKNESS);

	memset(&npdispwin.lastScreenDrawMode, 0, sizeof(NPDISP_DRAWMODE));

	// デバッグ用フォント
	LOGFONT lf = { 0 };
	lf.lfHeight = -9;
	lf.lfWeight = FW_NORMAL;
	lf.lfCharSet = DEFAULT_CHARSET;
	lf.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
	lstrcpy(lf.lfFaceName, _T("MS Gothic"));
	npdispwin.hFont = CreateFontIndirect(&lf);
	npdispwin.hOldhFont = SelectObject(npdispwin.hdc, npdispwin.hFont);

	//// DDBの色数を変更
	//for (auto it = npdispwin.bitmaps.begin(); it != npdispwin.bitmaps.end(); ++it) {
	//	if (it->second.bmphdc.hBmp) {
	//		HDC hdcTmp = npdispwin.hdcCache[0];
	//		BITMAPINFO_8BPP* lpbi = (BITMAPINFO_8BPP*)malloc(sizeof(BITMAPINFO_8BPP));
	//		if (lpbi) {
	//			HBITMAP hBmpNew = NULL;
	//			NPDISP_WINDOWS_BMPHDC* bmpHDC = &(it->second.bmphdc);
	//			void* pBits;
	//			*lpbi = npdispwin.bi;
	//			lpbi->bmiHeader.biWidth = bmpHDC->lpbi->bmiHeader.biWidth;
	//			lpbi->bmiHeader.biHeight = bmpHDC->lpbi->bmiHeader.biHeight;
	//			hBmpNew = CreateDIBSection(npdispwin.hdcCache[0], (BITMAPINFO*)lpbi, DIB_RGB_COLORS, &pBits, NULL, 0);
	//			if (hBmpNew) {
	//				// DDB書き戻し
	//				if (bmpHDC->hBmpDDB) {
	//					const int ddbWidth = bmpHDC->lpbi->bmiHeader.biWidth;
	//					const int ddbHeight = (bmpHDC->lpbi->bmiHeader.biHeight >= 0) ? bmpHDC->lpbi->bmiHeader.biHeight : -bmpHDC->lpbi->bmiHeader.biHeight;
	//					HDC hdcTemp = npdispwin.hdcCache[2];
	//					HGDIOBJ hOldBmp = SelectObject(hdcTemp, bmpHDC->hBmp);
	//					SetTextColor(hdcTemp, 0);
	//					SetBkColor(hdcTemp, 0xffffff);
	//					BitBlt(hdcTemp, 0, 0, ddbWidth, ddbHeight, bmpHDC->hdc, 0, 0, SRCCOPY);
	//					SelectObject(hdcTemp, hOldBmp);
	//				}

	//				// 旧→新 転送
	//				const int w = lpbi->bmiHeader.biWidth;
	//				const int h = lpbi->bmiHeader.biHeight >= 0 ? lpbi->bmiHeader.biHeight : -lpbi->bmiHeader.biHeight;
	//				HGDIOBJ hBmpOld0 = SelectObject(npdispwin.hdcCache[0], bmpHDC->hBmp);
	//				HGDIOBJ hBmpOld1 = SelectObject(npdispwin.hdcCache[1], hBmpNew);
	//				if (lpbi->bmiHeader.biBitCount == 8) {
	//					RGBQUAD pal[256];
	//					for (int i = 0; i < 256; i++) {
	//						pal[i].rgbRed = npdisp_palette_rgb256[i].r;
	//						pal[i].rgbGreen = npdisp_palette_rgb256[i].g;
	//						pal[i].rgbBlue = npdisp_palette_rgb256[i].b;
	//						pal[i].rgbReserved = 0;
	//					}
	//					SetDIBColorTable(npdispwin.hdcCache[1], 0, 256, pal);
	//				}
	//				BitBlt(npdispwin.hdcCache[1], 0, 0, w, h, npdispwin.hdcCache[0], 0, 0, SRCCOPY);
	//				if (lpbi->bmiHeader.biBitCount == 8) {
	//					RGBQUAD pal[256];
	//					for (int i = 0; i < 256; i++) {
	//						pal[i].rgbRed = npdisp_palette_gray256[i].r;
	//						pal[i].rgbGreen = npdisp_palette_gray256[i].g;
	//						pal[i].rgbBlue = npdisp_palette_gray256[i].b;
	//						pal[i].rgbReserved = 0;
	//					}
	//					SetDIBColorTable(npdispwin.hdcCache[1], 0, 256, pal);
	//				}
	//				SelectObject(npdispwin.hdcCache[0], hBmpOld0);
	//				SelectObject(npdispwin.hdcCache[1], hBmpOld1);

	//				// 旧BMP削除
	//				if (bmpHDC->hBmpDDB) {
	//					// DDB削除
	//					if (bmpHDC->hdc && bmpHDC->hOldBmp) {
	//						SelectObject(bmpHDC->hdc, bmpHDC->hOldBmp);
	//					}
	//					DeleteObject(bmpHDC->hBmpDDB);
	//					bmpHDC->hBmpDDB = NULL;
	//				}
	//				if (bmpHDC->lpbi) {
	//					free(bmpHDC->lpbi);
	//					bmpHDC->lpbi = NULL;
	//				}
	//				if (bmpHDC->hBmp) {
	//					if (bmpHDC->hdc && bmpHDC->hOldBmp) {
	//						SelectObject(bmpHDC->hdc, bmpHDC->hOldBmp);
	//					}
	//					DeleteObject(bmpHDC->hBmp);
	//					bmpHDC->hBmp = NULL;
	//					bmpHDC->pBits = NULL;
	//				}

	//				// 新BMP設定
	//				bmpHDC->hBmp = hBmpNew;
	//				bmpHDC->pBits = pBits;
	//				bmpHDC->lpbi = (BITMAPINFO*)lpbi;
	//			}
	//			else {
	//				free(lpbi);
	//			}
	//		}
	//	}
	//}
}

void npdisp_reset(const NP2CFG* pConfig)
{
	int i;
	npdispcs_initialize();

	npdisp_palette_makeTable();

	npdisp_releaseScreen();

	npdisp.ioenabled = pConfig->usenpdisp;
	npdisp.enabled = 0;
	npdisp.active = 0;
	npdisp.width = 1024;
	npdisp.height = 720;
	npdisp.bpp = 24;
	npdisp.dpiX = 96;
	npdisp.dpiY = 96;
	npdisp.usePalette = 0;
	npdisp.cursorX = 0;
	npdisp.cursorY = 0;
	npdisp.isWin9x = 0;

	npdisp.mm_vramPhysicalAddr = 0;
	npdisp.mm_screenPtr = NULL;
	npdisp.mm_screenSize = 0;
	npdisp.mm_bmpinfoAddr = 0;
	npdisp.mm_beginAccessAddr = 0;
	npdisp.mm_endAccessAddr = 0;
	npdisp.mm_dcibufAddr = 0;
	npdisp.mm_dciBeginAccessAddr = 0;
	npdisp.mm_dciEndAccessAddr = 0;
	npdisp.mm_dciDestroySurfaceAddr = 0;
	npdisp.mm_vramLinearAddr = 0;
	npdisp.mm_dciEnable = 0;

	npdispwin.pensIdx = 1;
	npdispwin.brushesIdx = 1;
	npdispwin.bitmapsIdx = 1;

	npdispwin.hdcCursor = NULL;
	npdispwin.hBmpCursor = NULL;
	npdispwin.hOldBmpCursor = NULL;
	npdispwin.hdcCursorMask = NULL;
	npdispwin.hBmpCursorMask = NULL;
	npdispwin.hOldBmpCursorMask = NULL;

	npdispwin.dirtyRect.left = npdispwin.dirtyRect.top = npdispwin.dirtyRect.right = npdispwin.dirtyRect.bottom = 0;
	npdispwin.lastCursorRect.left = npdispwin.lastCursorRect.top = npdispwin.lastCursorRect.right = npdispwin.lastCursorRect.bottom = 0;

	npdisp.cursorHotSpotX = 0;
	npdisp.cursorHotSpotY = 0;
	npdisp.cursorWidth = 0;
	npdisp.cursorHeight = 0;
	npdisp.cursorBpp = 0;
	npdisp.cursorStride = 0;

	npdisp.updated = 0;
	npdisp.paletteUpdated = 0;

	npdisp_memory_clearallpreload();
}
void npdisp_bind(void)
{
	if (npdisp.ioenabled) {
		iocore_attachout(0x07e7, npdisp_o7e7);
		iocore_attachout(0x07e8, npdisp_o7e8);
		iocore_attachout(0x07e9, npdisp_o7e9);
		iocore_attachinp(0x07e8, npdisp_i7e8);
		iocore_attachinp(0x07e9, npdisp_i7e9);
	}
}
void npdisp_unbind(void)
{
	iocore_detachout(0x07e7);
	iocore_detachout(0x07e8);
	iocore_detachout(0x07e9);
	iocore_detachinp(0x07e8);
	iocore_detachinp(0x07e9);
}

void npdisp_shutdown()
{
	npdisp_releaseScreen();
	npdispcs_shutdown();
}

// ---------- state save

int npdisp_sfsave(STFLAGH sfh, const SFENTRY* tbl)
{
	int	sfVersion = 2;
	int	ret = STATFLAG_SUCCESS;

	ret = statflag_write(sfh, &sfVersion, sizeof(int));
	if (ret != STATFLAG_SUCCESS) return ret;

	std::vector<UINT8> buffer;

	// 必要な範囲で記録
	// 共通
	UINT32 npdisplen = sizeof(npdisp);
	buffer.insert(buffer.end(), (UINT8*)(&npdisplen), (UINT8*)(&npdisplen + 1));
	buffer.insert(buffer.end(), (UINT8*)(&npdisp), (UINT8*)(&npdisp + 1));

	// WAB有効なら保存
	if (npdisp.enabled) {
		// OS依存部
		// スクリーン
		buffer.insert(buffer.end(), (UINT8*)(&npdispwin.bi), (UINT8*)(&npdispwin.bi + 1));
		UINT32 screenBufSize = npdispwin.stride * npdisp.height;
		if (npdispwin.pBits) {
			buffer.insert(buffer.end(), (UINT8*)(&screenBufSize), (UINT8*)(&screenBufSize + 1));
			buffer.insert(buffer.end(), (UINT8*)npdispwin.pBits, (UINT8*)npdispwin.pBits + screenBufSize);
		}
		else {
			screenBufSize = 0;
			buffer.insert(buffer.end(), (UINT8*)(&screenBufSize), (UINT8*)(&screenBufSize + 1));
		}
		// カーソル
		UINT32 cursorBufSize = 0;
		cursorBufSize = npdisp.cursorStride * npdisp.cursorHeight;
		if (npdispwin.pBitsCursorMask && npdispwin.pBitsCursor) {
			buffer.insert(buffer.end(), (UINT8*)(&cursorBufSize), (UINT8*)(&cursorBufSize + 1));
			buffer.insert(buffer.end(), (UINT8*)npdispwin.pBitsCursorMask, (UINT8*)npdispwin.pBitsCursorMask + cursorBufSize);
			buffer.insert(buffer.end(), (UINT8*)npdispwin.pBitsCursor, (UINT8*)npdispwin.pBitsCursor + cursorBufSize);
		}
		else {
			cursorBufSize = 0;
			buffer.insert(buffer.end(), (UINT8*)(&cursorBufSize), (UINT8*)(&cursorBufSize + 1));
		}
		// パレット
		if (npdisp.bpp == 1) {
			buffer.insert(buffer.end(), (UINT8*)npdisp_palette_rgb2, (UINT8*)npdisp_palette_rgb2 + sizeof(npdisp_palette_rgb2));
		}
		else if (npdisp.bpp == 4) {
			buffer.insert(buffer.end(), (UINT8*)npdisp_palette_rgb16, (UINT8*)npdisp_palette_rgb16 + sizeof(npdisp_palette_rgb16));
		}
		else if (npdisp.bpp == 8) {
			buffer.insert(buffer.end(), (UINT8*)npdisp_palette_rgb256, (UINT8*)npdisp_palette_rgb256 + sizeof(npdisp_palette_rgb256));
			buffer.insert(buffer.end(), (UINT8*)npdisp_palette_transTbl, (UINT8*)npdisp_palette_transTbl + sizeof(npdisp_palette_transTbl));
		}
		// ペン
		buffer.insert(buffer.end(), (UINT8*)(&npdispwin.pensIdx), (UINT8*)(&npdispwin.pensIdx + 1));
		UINT32 penCount = npdispwin.pens.size();
		buffer.insert(buffer.end(), (UINT8*)(&penCount), (UINT8*)(&penCount + 1));
		for (auto it = npdispwin.pens.begin(); it != npdispwin.pens.end(); ++it) {
			buffer.insert(buffer.end(), (UINT8*)(&(it->first)), (UINT8*)(&(it->first) + 1));
			buffer.insert(buffer.end(), (UINT8*)(&(it->second)), (UINT8*)(&(it->second) + 1));
		}
		// ブラシ
		buffer.insert(buffer.end(), (UINT8*)(&npdispwin.brushesIdx), (UINT8*)(&npdispwin.brushesIdx + 1));
		UINT32 brushCount = npdispwin.brushes.size();
		buffer.insert(buffer.end(), (UINT8*)(&brushCount), (UINT8*)(&brushCount + 1));
		for (auto it = npdispwin.brushes.begin(); it != npdispwin.brushes.end(); ++it) {
			buffer.insert(buffer.end(), (UINT8*)(&(it->first)), (UINT8*)(&(it->first) + 1));
			buffer.insert(buffer.end(), (UINT8*)(&(it->second)), (UINT8*)(&(it->second) + 1));
		}
		// ビットマップ
		buffer.insert(buffer.end(), (UINT8*)(&npdispwin.bitmapsIdx), (UINT8*)(&npdispwin.bitmapsIdx + 1));
		UINT32 bitmapCount = npdispwin.bitmaps.size();
		buffer.insert(buffer.end(), (UINT8*)(&bitmapCount), (UINT8*)(&bitmapCount + 1));
		for (auto it = npdispwin.bitmaps.begin(); it != npdispwin.bitmaps.end(); ++it) {
			buffer.insert(buffer.end(), (UINT8*)(&(it->first)), (UINT8*)(&(it->first) + 1));
			buffer.insert(buffer.end(), (UINT8*)(&(it->second)), (UINT8*)(&(it->second) + 1));
			int width = it->second.bmphdc.lpbi->bmiHeader.biWidth;
			int height = it->second.bmphdc.lpbi->bmiHeader.biHeight;
			int bpp = it->second.bmphdc.lpbi->bmiHeader.biBitCount;
			if (height < 0) height = -height;
			int stride = ((width * bpp + 31) / 32) * 4;
			int biSize = sizeof(BITMAPINFOHEADER);
			if (bpp <= 8) {
				biSize += sizeof(RGBQUAD) * (1 << bpp);
			}
			else if ((bpp == 15 || bpp == 16 || bpp == 32) && it->second.bmphdc.lpbi->bmiHeader.biCompression == BI_BITFIELDS) {
				biSize += sizeof(RGBQUAD) * 3;
			}
			if (it->second.bmphdc.lpbi->bmiHeader.biBitCount <= 8) {
				HGDIOBJ oldBmp = SelectObject(npdispwin.hdcCache[0], it->second.bmphdc.hBmp);
				if (oldBmp) {
					if (!GetDIBColorTable(npdispwin.hdcCache[0], 0, (1 << bpp), it->second.bmphdc.lpbi->bmiColors)) {
						npdispwin.hdcCache[0] = npdispwin.hdcCache[0];
					}
					SelectObject(npdispwin.hdcCache[0], oldBmp);
				}
			}
			int pBitsSize = stride * height;
			UINT8* lpbiUINT8 = (UINT8*)it->second.bmphdc.lpbi;
			UINT8* pBitsUINT8 = (UINT8*)it->second.bmphdc.pBits;
			buffer.insert(buffer.end(), (UINT8*)&biSize, (UINT8*)(&biSize + 1));
			buffer.insert(buffer.end(), lpbiUINT8, lpbiUINT8 + biSize);
			buffer.insert(buffer.end(), (UINT8*)&pBitsSize, (UINT8*)(&pBitsSize + 1));
			buffer.insert(buffer.end(), pBitsUINT8, pBitsUINT8 + pBitsSize);
		}
	}

	// 書き込み
	int statLen = buffer.size();
	ret = statflag_write(sfh, &statLen, sizeof(int));
	if (ret != STATFLAG_SUCCESS) return ret;
	if (statLen) {
		ret = statflag_write(sfh, &(buffer[0]), statLen);
		if (ret != STATFLAG_SUCCESS) return ret;
	}

	return(ret);
}

int npdisp_sfload(STFLAGH sfh, const SFENTRY* tbl)
{
	int	sfVersion = 0;
	int statLen = 0;
	int	ret = STATFLAG_SUCCESS;

	// 画面など解放
	npdisp_releaseScreen();

	ret = statflag_read(sfh, &sfVersion, sizeof(sfVersion));
	if (ret != STATFLAG_SUCCESS) return ret;
	ret = statflag_read(sfh, &statLen, sizeof(statLen));
	if (ret != STATFLAG_SUCCESS) return ret;
	if (statLen == 0) return STATFLAG_SUCCESS; // データ長さ0はバージョンに関係なくOK

	int readBufLen = 0;

	int oldCursorBpp = npdisp.cursorBpp;
	int oldCursorStride = npdisp.cursorStride;

	// 共通
	if (sfVersion == 1) {
		// ステートセーブ ver.1
		ret = statflag_read(sfh, &npdisp, sizeof(npdisp) - 1);
		if (ret != STATFLAG_SUCCESS) return ret;
		readBufLen += sizeof(npdisp) - 1;
		npdisp.active = npdisp.enabled;
	}
	else {
		// ステートセーブ ver.2以降
		UINT32 npdisplen = 0;
		ret = statflag_read(sfh, &npdisplen, sizeof(npdisplen));
		readBufLen += sizeof(npdisplen);
		if (ret != STATFLAG_SUCCESS) return ret;
		if (npdisplen < 0 || npdisplen > 32768) return STATFLAG_FAILURE; // 異常
		std::vector<UINT8> temp(npdisplen);
		ret = statflag_read(sfh, &(temp[0]), npdisplen);
		if (ret != STATFLAG_SUCCESS) return ret;
		readBufLen += npdisplen;
		memcpy(&npdisp, &(temp[0]), min(sizeof(npdisp), npdisplen));
	}

	// WAB有効なら読み込み
	if (npdisp.enabled) {
		if (sfVersion == 1 || sfVersion == 2)
		{
			// 画面など生成
			npdisp_createScreen();

			// OS依存部
			// スクリーン
			ret = statflag_read(sfh, &npdispwin.bi, sizeof(npdispwin.bi));
			if (ret != STATFLAG_SUCCESS) goto error;
			readBufLen += sizeof(npdispwin.bi);
			UINT32 screenBufSize;
			ret = statflag_read(sfh, &screenBufSize, sizeof(screenBufSize));
			if (ret != STATFLAG_SUCCESS) goto error;
			readBufLen += sizeof(screenBufSize);
			if (screenBufSize) {
				ret = statflag_read(sfh, npdispwin.pBits, screenBufSize);
				if (ret != STATFLAG_SUCCESS) goto error;
				readBufLen += screenBufSize;
			}
			// カーソル
			UINT32 cursorBufSize;
			ret = statflag_read(sfh, &cursorBufSize, sizeof(cursorBufSize));
			if (ret != STATFLAG_SUCCESS) goto error;
			readBufLen += sizeof(cursorBufSize);
			if (cursorBufSize) {
				// 読み取りと再生成
				HBITMAP hBmpCursorMask = NULL;
				HBITMAP hBmpCursor = NULL;

				void* pBitsCursorMask = NULL;
				void* pBitsCursor = NULL;
				if (npdisp.cursorBpp > 1) {
					// モノクロ以外はDIBSectionで
					BITMAPINFO_8BPP bi;
					bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
					bi.bmiHeader.biWidth = npdisp.cursorWidth;
					bi.bmiHeader.biHeight = -npdisp.cursorHeight;
					bi.bmiHeader.biPlanes = 1;
					bi.bmiHeader.biBitCount = npdisp.cursorBpp;
					bi.bmiHeader.biCompression = BI_RGB;
					bi.bmiHeader.biSizeImage = 0;
					bi.bmiHeader.biXPelsPerMeter = 0;
					bi.bmiHeader.biYPelsPerMeter = 0;
					bi.bmiHeader.biClrUsed = 0;
					bi.bmiHeader.biClrImportant = 0;
					if (npdisp.cursorBpp == 8) {
						bi.bmiHeader.biClrUsed = 256;
						memcpy(bi.bmiColors, npdisp_palette_rgb256, sizeof(npdisp_palette_rgb256));
					}
					else if (npdisp.cursorBpp == 4) {
						bi.bmiHeader.biClrUsed = 256;
						memcpy(bi.bmiColors, npdisp_palette_rgb16, sizeof(npdisp_palette_rgb16));
					}
					else if (npdisp.cursorBpp == 15 || npdisp.cursorBpp == 16) {
						if (npdisp.cursorBpp == 16) {
							// ビットフィールド 565
							bi.bmiHeader.biCompression = BI_BITFIELDS;
							*((DWORD*)(bi.bmiColors + 0)) = 0x0000F800;
							*((DWORD*)(bi.bmiColors + 1)) = 0x000007E0;
							*((DWORD*)(bi.bmiColors + 2)) = 0x0000001F;
						}
						else if (npdisp.cursorBpp == 15) {
							// ビットフィールド 555
							bi.bmiHeader.biCompression = BI_BITFIELDS;
							*((DWORD*)(bi.bmiColors + 0)) = 0x00007C00;
							*((DWORD*)(bi.bmiColors + 1)) = 0x000003E0;
							*((DWORD*)(bi.bmiColors + 2)) = 0x0000001F;
							bi.bmiHeader.biBitCount = 16;
						}
						bi.bmiHeader.biBitCount = 16; // 16扱いにする
					}
					hBmpCursorMask = CreateDIBSection(npdispwin.hdcCursorMask, (BITMAPINFO*)(&bi), DIB_RGB_COLORS, &pBitsCursorMask, NULL, 0);
					if (!hBmpCursorMask) goto error;
					hBmpCursor = CreateDIBSection(npdispwin.hdcCursor, (BITMAPINFO*)(&bi), DIB_RGB_COLORS, &pBitsCursor, NULL, 0);
					if (!hBmpCursor) {
						DeleteObject(hBmpCursorMask);
						goto error;
					}
				}
				else {
					// モノクロはDDBで（後で作成）
					pBitsCursorMask = (char*)malloc(cursorBufSize);
					if (!pBitsCursorMask) goto error;
					pBitsCursor = (char*)malloc(cursorBufSize);
					if (!pBitsCursor) {
						free(pBitsCursorMask);
						goto error;
					}
				}

				ret = statflag_read(sfh, pBitsCursorMask, cursorBufSize);
				if (ret != STATFLAG_SUCCESS) goto error;
				readBufLen += cursorBufSize;
				ret = statflag_read(sfh, pBitsCursor, cursorBufSize);
				if (ret != STATFLAG_SUCCESS) goto error;
				readBufLen += cursorBufSize;

				if (npdisp.cursorBpp <= 1) {
					// モノクロはDDBで（後作成）
					hBmpCursorMask = CreateBitmap(npdisp.cursorWidth, npdisp.cursorHeight, 1, 1, pBitsCursorMask);
					hBmpCursor = CreateBitmap(npdisp.cursorWidth, npdisp.cursorHeight, 1, 1, pBitsCursor);
				}

				// ビットマップ置き換え
				if (hBmpCursorMask) {
					if (npdispwin.hBmpCursorMask) {
						SelectObject(npdispwin.hdcCursorMask, npdispwin.hOldBmpCursorMask);
						DeleteObject(npdispwin.hBmpCursorMask);
						npdispwin.hBmpCursorMask = NULL;
					}
					npdispwin.hOldBmpCursorMask = (HBITMAP)SelectObject(npdispwin.hdcCursorMask, hBmpCursorMask);
					npdispwin.hBmpCursorMask = hBmpCursorMask;
				}
				if (hBmpCursor) {
					if (npdispwin.hBmpCursor) {
						SelectObject(npdispwin.hdcCursor, npdispwin.hOldBmpCursor);
						DeleteObject(npdispwin.hBmpCursor);
						npdispwin.hBmpCursor = NULL;
					}
					npdispwin.hOldBmpCursor = (HBITMAP)SelectObject(npdispwin.hdcCursor, hBmpCursor);
					npdispwin.hBmpCursor = hBmpCursor;
				}

				// ピクセルバッファを置き換え
				void* oldpBitsCursor = npdispwin.pBitsCursor;
				void* oldpBitsCursorMask = npdispwin.pBitsCursorMask;
				npdispwin.pBitsCursor = pBitsCursor;
				npdispwin.pBitsCursorMask = pBitsCursorMask;
				if (oldCursorBpp <= 1) {
					// 1bppは自前mallocなのでfreeする
					if (oldpBitsCursor) free(oldpBitsCursor);
					if (oldpBitsCursorMask) free(oldpBitsCursorMask);
				}
			}
			// パレット
			if (npdisp.bpp == 1) {
				ret = statflag_read(sfh, npdisp_palette_rgb2, sizeof(npdisp_palette_rgb2));
				if (ret != STATFLAG_SUCCESS) goto error;
				readBufLen += sizeof(npdisp_palette_rgb2);
			}
			else if (npdisp.bpp == 4) {
				ret = statflag_read(sfh, npdisp_palette_rgb16, sizeof(npdisp_palette_rgb16));
				if (ret != STATFLAG_SUCCESS) goto error;
				readBufLen += sizeof(npdisp_palette_rgb16);
			}
			else if (npdisp.bpp == 8) {
				ret = statflag_read(sfh, npdisp_palette_rgb256, sizeof(npdisp_palette_rgb256));
				if (ret != STATFLAG_SUCCESS) goto error;
				readBufLen += sizeof(npdisp_palette_rgb256);
				ret = statflag_read(sfh, npdisp_palette_transTbl, sizeof(npdisp_palette_transTbl));
				if (ret != STATFLAG_SUCCESS) goto error;
				readBufLen += sizeof(npdisp_palette_transTbl);
			}
			// ペン
			ret = statflag_read(sfh, &npdispwin.pensIdx, sizeof(npdispwin.pensIdx));
			if (ret != STATFLAG_SUCCESS) goto error;
			readBufLen += sizeof(npdispwin.pensIdx);
			UINT32 penCount;
			ret = statflag_read(sfh, &penCount, sizeof(penCount));
			if (ret != STATFLAG_SUCCESS) goto error;
			readBufLen += sizeof(penCount);
			for (int i = 0; i < penCount; i++) {
				UINT32 key;
				NPDISP_HOSTPEN pen;
				ret = statflag_read(sfh, &key, sizeof(key));
				if (ret != STATFLAG_SUCCESS) goto error;
				readBufLen += sizeof(key);
				ret = statflag_read(sfh, &pen, sizeof(pen));
				if (ret != STATFLAG_SUCCESS) goto error;
				readBufLen += sizeof(pen);
				pen.pen = NULL; // statロードなので無効
				npdisp_createPen(&pen); // ブラシ生成
				npdispwin.pens[key] = pen;

			}
			// ブラシ
			ret = statflag_read(sfh, &npdispwin.brushesIdx, sizeof(npdispwin.brushesIdx));
			if (ret != STATFLAG_SUCCESS) goto error;
			readBufLen += sizeof(npdispwin.brushesIdx);
			UINT32 brushCount;
			ret = statflag_read(sfh, &brushCount, sizeof(brushCount));
			if (ret != STATFLAG_SUCCESS) goto error;
			readBufLen += sizeof(brushCount);
			for (int i = 0; i < brushCount; i++) {
				UINT32 key;
				NPDISP_HOSTBRUSH brush;
				ret = statflag_read(sfh, &key, sizeof(key));
				if (ret != STATFLAG_SUCCESS) goto error;
				readBufLen += sizeof(key);
				ret = statflag_read(sfh, &brush, sizeof(brush));
				if (ret != STATFLAG_SUCCESS) goto error;
				readBufLen += sizeof(brush);
				brush.brs = NULL; // statロードなので無効
				npdisp_createBrush(&brush); // ブラシ生成
				npdispwin.brushes[key] = brush;
			}

			if (readBufLen < statLen) {
				// ビットマップ
				ret = statflag_read(sfh, &npdispwin.bitmapsIdx, sizeof(npdispwin.bitmapsIdx));
				if (ret != STATFLAG_SUCCESS) goto error;
				readBufLen += sizeof(npdispwin.bitmapsIdx);
				UINT32 bitmapCount;
				ret = statflag_read(sfh, &bitmapCount, sizeof(bitmapCount));
				if (ret != STATFLAG_SUCCESS) goto error;
				readBufLen += sizeof(bitmapCount);
				std::vector<UINT32> keys;
				for (int i = 0; i < bitmapCount; i++) {
					UINT32 key;
					NPDISP_HOSTBITMAP hostbmp = { 0 };
					ret = statflag_read(sfh, &key, sizeof(key));
					if (ret != STATFLAG_SUCCESS) goto error;
					readBufLen += sizeof(key);
					ret = statflag_read(sfh, &hostbmp, sizeof(hostbmp));
					if (ret != STATFLAG_SUCCESS) goto error;
					readBufLen += sizeof(hostbmp);

					int biSize;
					ret = statflag_read(sfh, &biSize, sizeof(biSize));
					readBufLen += sizeof(biSize);
					hostbmp.bmphdc.lpbi = (BITMAPINFO*)malloc(biSize);
					if(!hostbmp.bmphdc.lpbi) goto error;
					ret = statflag_read(sfh, hostbmp.bmphdc.lpbi, biSize);
					readBufLen += biSize;
					hostbmp.bmphdc.hBmp = CreateDIBSection(npdispwin.hdcCache[0], hostbmp.bmphdc.lpbi, DIB_RGB_COLORS, &hostbmp.bmphdc.pBits, NULL, 0);

					int pBitsSize;
					ret = statflag_read(sfh, &pBitsSize, sizeof(pBitsSize));
					readBufLen += sizeof(pBitsSize);
					ret = statflag_read(sfh, hostbmp.bmphdc.pBits, pBitsSize);
					readBufLen += pBitsSize;

					npdispwin.bitmaps[key] = hostbmp;
					keys.push_back(key);
				}
			}

			if (readBufLen != statLen) goto error;

			// 読み込みバッファリセット
			int longjmpnum = npdisp.longjmpnum;
			npdisp_memory_clearallpreload();
			npdisp.longjmpnum = longjmpnum; // 読み込み中例外のフラグは残す
			
			// 画面更新
			npdisp_setDirtyAll();
			npdisp.paletteUpdated = 1;
			npdisp.updated = 1;
			npdispwin.dciDirtyRect.left = 0;
			npdispwin.dciDirtyRect.top = 0;
			npdispwin.dciDirtyRect.right = npdisp.width;
			npdispwin.dciDirtyRect.bottom = npdisp.height;
		}
		else
		{
			return(STATFLAG_FAILURE);
		}
	}
	return(ret);

error:

	npdisp_releaseScreen();
	return(STATFLAG_FAILURE);
}

#endif