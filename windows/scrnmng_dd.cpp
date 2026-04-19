/**
 * @file	scrnmng_dd.cpp
 * @brief	Screen Manager (DirectDraw2)
 *
 * @author	$Author: yui (modified by SimK)$
 * @date	$Date: 2011/03/07 09:54:11 $
 */

#include "compiler.h"
#include <ddraw.h>
#ifndef __GNUC__
#include <winnls32.h>
#endif
#include "resource.h"
#include "np2.h"
#include "np2mt.h"
#include "winloc.h"
#include "mousemng.h"
#include "scrnmng.h"
#include "scrnmng_dd.h"
// #include "sysmng.h"
#include "dialog\np2class.h"
#include "pccore.h"
#include "scrndraw.h"
#include "palettes.h"

#if defined(SUPPORT_DCLOCK)
#include "subwnd\dclock.h"
#endif
#include "recvideo.h"

#ifdef SUPPORT_WAB
#include "wab/wab.h"
#endif

#include <shlwapi.h>

#if !defined(__GNUC__)
#pragma comment(lib, "ddraw.lib")
#pragma comment(lib, "dxguid.lib")
#endif	// !defined(__GNUC__)

//! 8BPP パレット数
#define PALLETES_8BPP	NP2PAL_TEXT3

#define FILLSURF_SIZE	32

static int req_enter_criticalsection = 0;

extern bool scrnmng_create_pending; // グラフィックレンダラ生成保留中
extern bool scrnmng_restore_pending;

extern WINLOCEX np2_winlocexallwin(HWND base);

typedef struct {
	LPDIRECTDRAW		ddraw1;
	LPDIRECTDRAW2		ddraw2;
	LPDIRECTDRAWSURFACE	primsurf;
	LPDIRECTDRAWSURFACE	backsurf;
	LPDIRECTDRAWSURFACE	fillsurf;
#if defined(SUPPORT_DCLOCK)
	LPDIRECTDRAWSURFACE	clocksurf;
#endif
#if defined(SUPPORT_WAB)
	LPDIRECTDRAWSURFACE	wabsurf;
#endif
	LPDIRECTDRAWCLIPPER	clipper;
	LPDIRECTDRAWPALETTE	palette;
	UINT				scrnmode;
	int					width;
	int					height;
	int					extend;
	int					cliping;
	RGB32				pal16mask;
	UINT8				r16b;
	UINT8				l16r;
	UINT8				l16g;
	UINT8				menudisp;
	int					menusize;
	RECT				scrn;
	RECT				rect;
	RECT				scrnclip;
	RECT				rectclip;
	PALETTEENTRY		pal[256];
} DDRAW;

static	DDRAW		ddraw = { 0 };
static	SCRNSURF	scrnsurf = { 0 };

#ifdef SUPPORT_WAB
static	int mt_wabdrawing = 0;
static	int mt_wabpausedrawing = 0;
#endif

static int dd_cs_initialized = 0;
static CRITICAL_SECTION dd_cs;

static BOOL dd_tryenter_criticalsection(void){
	if(!dd_cs_initialized) return TRUE;
	return TryEnterCriticalSection(&dd_cs);
}
static void dd_enter_criticalsection(void){
	if(!dd_cs_initialized) return;
	EnterCriticalSection(&dd_cs);
}
static void dd_leave_criticalsection(void){
	if(!dd_cs_initialized) return;
	LeaveCriticalSection(&dd_cs);
}

// プライマリサーフェイスのDirectDraw DDBLT_COLORFILLで例外が出る環境があるようなので代替
#define DDBLT_COLORFILL_FIX

#ifdef DDBLT_COLORFILL_FIX
static void DDBlt_ColorFill(LPDIRECTDRAWSURFACE lpDst, LPRECT lpDstRect, LPDDBLTFX lpDDBltFx, LPDIRECTDRAWSURFACE lpOffScrBuf)
{
	HDC hDC = NULL;
	RECT	src;
	src.left = 1;
	src.top = 1;
	src.right = FILLSURF_SIZE - 1;
	src.bottom = FILLSURF_SIZE - 1;
	lpDst->Blt(lpDstRect, lpOffScrBuf, &src, DDBLT_WAIT, NULL);
}
#else
static void DDBlt_ColorFill(LPDIRECTDRAWSURFACE lpDst, LPRECT lpRect, LPDDBLTFX lpDDBltFx, LPDIRECTDRAWSURFACE lpOffScrBuf)
{
	lpDst->Blt(lpRect,NULL,NULL, DDBLT_COLORFILL | DDBLT_WAIT, lpDDBltFx);
}
#endif

static void getscreensize(int* screenwidth, int* screenheight, UINT scrnmode) {
	UINT		fscrnmod;
	int			width;
	int			height;
	int			scrnwidth;
	int			scrnheight;
	int			multiple;

	if (scrnmode & SCRNMODE_FULLSCREEN) {
		width = min(scrnstat.width, ddraw.width);
		height = min(scrnstat.height, ddraw.height);

		scrnwidth = width;
		scrnheight = height;
		fscrnmod = FSCRNCFG_fscrnmod & FSCRNMOD_ASPECTMASK;
		switch (fscrnmod) {
		default:
		case FSCRNMOD_NORESIZE:
			break;

		case FSCRNMOD_ASPECTFIX8:
			scrnwidth = (ddraw.width << 3) / width;
			scrnheight = (ddraw.height << 3) / height;
			multiple = min(scrnwidth, scrnheight);
			scrnwidth = (width * multiple) >> 3;
			scrnheight = (height * multiple) >> 3;
			break;

		case FSCRNMOD_ASPECTFIX:
			scrnwidth = ddraw.width;
			scrnheight = (scrnwidth * height) / width;
			if (scrnheight >= ddraw.height) {
				scrnheight = ddraw.height;
				scrnwidth = (scrnheight * width) / height;
			}
			break;

		case FSCRNMOD_INTMULTIPLE:
			scrnwidth = (ddraw.width / width) * width;
			scrnheight = (scrnwidth * height) / width;
			if (scrnheight >= ddraw.height) {
				scrnheight = (ddraw.height / height) * height;
				scrnwidth = (scrnheight * width) / height;
			}
			break;

		case FSCRNMOD_FORCE43:
			if (ddraw.width * 3 > ddraw.height * 4) {
				scrnwidth = ddraw.height * 4 / 3;
				scrnheight = ddraw.height;
			}
			else {
				scrnwidth = ddraw.width;
				scrnheight = ddraw.width * 3 / 4;
			}
			break;

		case FSCRNMOD_LARGE:
			scrnwidth = ddraw.width;
			scrnheight = ddraw.height;
			break;
		}
	}
	else {
		width = min(scrnstat.width, ddraw.width);
		height = min(scrnstat.height, ddraw.height);

		multiple = scrnstat.multiple;
		fscrnmod = FSCRNCFG_fscrnmod & FSCRNMOD_ASPECTMASK;
		if (!(scrnmode & SCRNMODE_ROTATE)) {
			scrnwidth = (width * multiple) >> 3;
			scrnheight = (height * multiple) >> 3;
			if (fscrnmod == FSCRNMOD_FORCE43) { // Force 4:3 Screen
				if (((width * multiple) >> 3) * 3 < ((height * multiple) >> 3) * 4) {
					scrnwidth = ((height * multiple) >> 3) * 4 / 3;
					scrnheight = ((height * multiple) >> 3);
				}
				else {
					scrnwidth = ((width * multiple) >> 3);
					scrnheight = ((width * multiple) >> 3) * 3 / 4;
				}
			}
		}
		else {
			scrnwidth = (height * multiple) >> 3;
			scrnheight = (width * multiple) >> 3;
			if (fscrnmod == FSCRNMOD_FORCE43) { // Force 4:3 Screen
				if (((width * multiple) >> 3) * 4 < ((height * multiple) >> 3) * 3) {
					scrnwidth = ((height * multiple) >> 3) * 3 / 4;
					scrnheight = ((height * multiple) >> 3);
				}
				else {
					scrnwidth = ((width * multiple) >> 3);
					scrnheight = ((width * multiple) >> 3) * 4 / 3;
				}
			}
		}
	}
	*screenwidth = scrnwidth;
	*screenheight = scrnheight;
}

static void renewalclientsize(BOOL winloc) {

	int			width;
	int			height;
	int			extend;
	UINT		fscrnmod;
	int			scrnwidth;
	int			scrnheight;
	int			tmpcy;
	WINLOCEX	wlex;

	width = min(scrnstat.width, ddraw.width);
	height = min(scrnstat.height, ddraw.height);

	extend = 0;

	// 描画範囲～
	if (ddraw.scrnmode & SCRNMODE_FULLSCREEN) {
		ddraw.rect.right = width;
		ddraw.rect.bottom = height;
		getscreensize(&scrnwidth, &scrnheight, ddraw.scrnmode);
		if ((np2oscfg.paddingx)/* && (multiple == 8)*/)
		{
			extend = min(scrnstat.extend, ddraw.extend);
		}
		fscrnmod = FSCRNCFG_fscrnmod & FSCRNMOD_ASPECTMASK;
		ddraw.scrn.left = (ddraw.width - scrnwidth) / 2;
		ddraw.scrn.top = (ddraw.height - scrnheight) / 2;
		ddraw.scrn.right = ddraw.scrn.left + scrnwidth;
		ddraw.scrn.bottom = ddraw.scrn.top + scrnheight;

		// メニュー表示時の描画領域
		ddraw.rectclip = ddraw.rect;
		ddraw.scrnclip = ddraw.scrn;
		if (ddraw.scrnclip.top < ddraw.menusize) {
			ddraw.scrnclip.top = ddraw.menusize;
			tmpcy = ddraw.height - ddraw.menusize;
			if (scrnheight > tmpcy) {
				switch(fscrnmod) {
					default:
					case FSCRNMOD_NORESIZE:
						tmpcy = min(tmpcy, height);
						ddraw.rectclip.bottom = tmpcy;
						break;

					case FSCRNMOD_ASPECTFIX8:
					case FSCRNMOD_ASPECTFIX:
					case FSCRNMOD_INTMULTIPLE:
					case FSCRNMOD_FORCE43:
						ddraw.rectclip.bottom = (tmpcy * height) / scrnheight;
						break;
						
					case FSCRNMOD_LARGE:
						break;
				}
			}
			ddraw.scrnclip.bottom = ddraw.menusize + tmpcy;
		}
	}
	else {
		int multiple;
		fscrnmod = FSCRNCFG_fscrnmod & FSCRNMOD_ASPECTMASK;
		multiple = scrnstat.multiple;
		getscreensize(&scrnwidth, &scrnheight, ddraw.scrnmode);
		if (!(ddraw.scrnmode & SCRNMODE_ROTATE)) {
			if ((np2oscfg.paddingx)/* && (multiple == 8)*/) {
				extend = min(scrnstat.extend, ddraw.extend);
			}
			ddraw.rect.left = 0;
			ddraw.rect.right = width + extend;
			ddraw.rect.top = 0;
			ddraw.rect.bottom = height;
			ddraw.scrn.left = (np2oscfg.paddingx - extend) * scrnstat.multiple / 8;
			ddraw.scrn.top = np2oscfg.paddingy * scrnstat.multiple / 8;
		}
		else {
			if ((np2oscfg.paddingy)/* && (multiple == 8)*/) {
				extend = min(scrnstat.extend, ddraw.extend);
			}
			ddraw.rect.left = 0;
			ddraw.rect.right = height;
			ddraw.rect.top = 0;
			ddraw.rect.bottom = width + extend;
			ddraw.scrn.left = np2oscfg.paddingx * scrnstat.multiple / 8;
			ddraw.scrn.top = (np2oscfg.paddingy - extend) * scrnstat.multiple / 8;
		}
		ddraw.scrn.right = np2oscfg.paddingx * scrnstat.multiple / 8 + scrnwidth;
		ddraw.scrn.bottom = np2oscfg.paddingy * scrnstat.multiple / 8 + scrnheight;

		wlex = NULL;
		if (winloc) {
			wlex = np2_winlocexallwin(g_hWndMain);
		}
		winlocex_setholdwnd(wlex, g_hWndMain);
		scrnmng_setwindowsize(g_hWndMain, scrnwidth, scrnheight);
		winlocex_move(wlex);
		winlocex_destroy(wlex);
	}
	scrnsurf.width = width;
	scrnsurf.height = height;
	scrnsurf.extend = extend;
}

static void clearoutofrect(const RECT *target, const RECT *base) {

	LPDIRECTDRAWSURFACE	primsurf;
	DDBLTFX				ddbf;
	RECT				rect;
	
	dd_enter_criticalsection();

	primsurf = ddraw.primsurf;
	if (primsurf == NULL) {
		dd_leave_criticalsection();
		return;
	}
	ZeroMemory(&ddbf, sizeof(ddbf));
	ddbf.dwSize = sizeof(ddbf);
	ddbf.dwFillColor = 0;

	rect.left = base->left;
	rect.right = base->right;
	rect.top = base->top;
	rect.bottom = target->top;
	if (rect.top < rect.bottom) {
		//primsurf->Blt(&rect, NULL, NULL, DDBLT_WAIT | DDBLT_COLORFILL, &ddbf);
		DDBlt_ColorFill(primsurf, &rect, &ddbf, ddraw.fillsurf);
	}
	rect.top = target->bottom;
	rect.bottom = base->bottom;
	if (rect.top < rect.bottom) {
		//primsurf->Blt(&rect, NULL, NULL, DDBLT_WAIT | DDBLT_COLORFILL, &ddbf);
		DDBlt_ColorFill(primsurf, &rect, &ddbf, ddraw.fillsurf);
	}

	rect.top = max(base->top, target->top);
	rect.bottom = min(base->bottom, target->bottom);
	if (rect.top < rect.bottom) {
		rect.left = base->left;
		rect.right = target->left;
		if (rect.left < rect.right) {
			//primsurf->Blt(&rect, NULL, NULL, DDBLT_WAIT | DDBLT_COLORFILL, &ddbf);
			DDBlt_ColorFill(primsurf, &rect, &ddbf, ddraw.fillsurf);
		}
		rect.left = target->right;
		rect.right = base->right;
		if (rect.left < rect.right) {
			//primsurf->Blt(&rect, NULL, NULL, DDBLT_WAIT | DDBLT_COLORFILL, &ddbf);
			DDBlt_ColorFill(primsurf, &rect, &ddbf, ddraw.fillsurf);
		}
	}

	dd_leave_criticalsection();
}

static void clearoutscreen(void) {

	RECT	base;
	POINT	clipt;
	RECT	target;

	GetClientRect(g_hWndMain, &base);
	clipt.x = 0;
	clipt.y = 0;
	ClientToScreen(g_hWndMain, &clipt);
	base.left += clipt.x;
	base.top += clipt.y;
	base.right += clipt.x;
	base.bottom += clipt.y;
	target.left = base.left + ddraw.scrn.left;
	target.top = base.top + ddraw.scrn.top;
	target.right = base.left + ddraw.scrn.right;
	target.bottom = base.top + ddraw.scrn.bottom;
	clearoutofrect(&target, &base);
}

static void clearoutfullscreen(void) {

	RECT	base;
const RECT	*scrn;

	base.left = 0;
	base.top = 0;
	base.right = ddraw.width;
	base.bottom = ddraw.height;
	if (GetWindowLongPtr(g_hWndMain, NP2GWLP_HMENU)) {
		scrn = &ddraw.scrn;
		base.top = 0;
	}
	else {
		scrn = &ddraw.scrnclip;
		base.top = ddraw.menusize;
	}
	clearoutofrect(scrn, &base);
#if defined(SUPPORT_DCLOCK)
	DispClock::GetInstance()->Redraw();
#endif
}

static void paletteinit()
{
	HDC hdc = GetDC(g_hWndMain);
	dd_enter_criticalsection();
	GetSystemPaletteEntries(hdc, 0, 256, ddraw.pal);
	ReleaseDC(g_hWndMain, hdc);
#if defined(SUPPORT_DCLOCK)
	const RGB32* pal32 = DispClock::GetInstance()->GetPalettes();
	for (UINT i = 0; i < 4; i++)
	 {
		ddraw.pal[i + START_PALORG].peBlue = pal32[i].p.b;
		ddraw.pal[i + START_PALORG].peRed = pal32[i].p.r;
		ddraw.pal[i + START_PALORG].peGreen = pal32[i].p.g;
		ddraw.pal[i + START_PALORG].peFlags = PC_RESERVED | PC_NOCOLLAPSE;
	}
#endif
	for (UINT i = 0; i < PALLETES_8BPP; i++)
	{
		ddraw.pal[i + START_PAL].peFlags = PC_RESERVED | PC_NOCOLLAPSE;
	}
	ddraw.ddraw2->CreatePalette(DDPCAPS_8BIT, ddraw.pal, &ddraw.palette, 0);
	ddraw.primsurf->SetPalette(ddraw.palette);
	dd_leave_criticalsection();
}

static void paletteset()
{
	dd_enter_criticalsection();
	if (ddraw.palette != NULL)
	{
		for (UINT i = 0; i < PALLETES_8BPP; i++)
		{
			ddraw.pal[i + START_PAL].peRed = np2_pal32[i].p.r;
			ddraw.pal[i + START_PAL].peBlue = np2_pal32[i].p.b;
			ddraw.pal[i + START_PAL].peGreen = np2_pal32[i].p.g;
		}
		ddraw.palette->SetEntries(0, START_PAL, PALLETES_8BPP, &ddraw.pal[START_PAL]);
	}
	dd_leave_criticalsection();
}

static void make16mask(DWORD bmask, DWORD rmask, DWORD gmask) {

	UINT8	sft;
	
	dd_enter_criticalsection();
	sft = 0;
	while((!(bmask & 0x80)) && (sft < 32)) {
		bmask <<= 1;
		sft++;
	}
	ddraw.pal16mask.p.b = (UINT8)bmask;
	ddraw.r16b = sft;

	sft = 0;
	while((rmask & 0xffffff00) && (sft < 32)) {
		rmask >>= 1;
		sft++;
	}
	ddraw.pal16mask.p.r = (UINT8)rmask;
	ddraw.l16r = sft;

	sft = 0;
	while((gmask & 0xffffff00) && (sft < 32)) {
		gmask >>= 1;
		sft++;
	}
	ddraw.pal16mask.p.g = (UINT8)gmask;
	ddraw.l16g = sft;
	dd_leave_criticalsection();
}

void scrnmngDD_restoresurfaces() {
	dd_enter_criticalsection();
	ddraw.backsurf->Restore();
	ddraw.primsurf->Restore();
	ddraw.fillsurf->Restore();
#if defined(SUPPORT_WAB)
	ddraw.wabsurf->Restore();
#endif
#if defined(SUPPORT_DCLOCK)
	if (ddraw.clocksurf)
	{
		ddraw.clocksurf->Restore();
	}
#endif
	scrndraw_updateallline();
	dd_leave_criticalsection();
}


// ----

static TCHAR dd_displayName[32] = {0};
BOOL WINAPI DDEnumDisplayCallbackA(GUID FAR *lpGUID, LPSTR lpDriverDescription, LPSTR lpDriverName, LPVOID lpContext, HMONITOR hm) {
	MONITORINFOEX  monitorInfoEx;
	RECT rc;
	RECT rcwnd;
	if(hm){
		GetWindowRect(g_hWndMain, &rcwnd);
		monitorInfoEx.cbSize = sizeof(MONITORINFOEX);
		GetMonitorInfo(hm, &monitorInfoEx);
		_tcscpy(dd_displayName, monitorInfoEx.szDevice);
		if(IntersectRect(&rc, &monitorInfoEx.rcWork, &rcwnd)){
			*((LPGUID)lpContext) = *lpGUID;
			return FALSE;
		}
	}
	return TRUE;
}

BRESULT scrnmngDD_create(UINT8 scrnmode) {

	DWORD			winstyle;
	DWORD			winstyleex;
	LPDIRECTDRAW2	ddraw2;
	DDSURFACEDESC	ddsd;
	DDPIXELFORMAT	ddpf;
	int				width;
	int				height;
	UINT			bitcolor;
	UINT			fscrnmod;
	DEVMODE			devmode;
	GUID			devguid = {0};
	LPGUID			devlpguid;
	HRESULT			r;

	static UINT8 lastscrnmode = 0;
	static WINDOWPLACEMENT wp = {sizeof(WINDOWPLACEMENT)};
	
	if(!dd_cs_initialized){
		memset(&dd_cs, 0, sizeof(dd_cs));
		InitializeCriticalSection(&dd_cs);
		dd_cs_initialized = 1;
	}
	
	dd_enter_criticalsection();
	scrnmng_create_pending = false;
	ZeroMemory(&scrnmng, sizeof(scrnmng));
	winstyle = GetWindowLong(g_hWndMain, GWL_STYLE);
	winstyleex = GetWindowLong(g_hWndMain, GWL_EXSTYLE);
	if (scrnmode & SCRNMODE_FULLSCREEN) {
		if(!(lastscrnmode & SCRNMODE_FULLSCREEN)){
			GetWindowPlacement(g_hWndMain, &wp);
		}
		scrnmode &= ~SCRNMODE_ROTATEMASK;
		scrnmng.flag = SCRNFLAG_FULLSCREEN;
		winstyle &= ~(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME);
		winstyle |= WS_POPUP;
		winstyleex |= WS_EX_TOPMOST;
		ddraw.menudisp = 0;
		ddraw.menusize = GetSystemMetrics(SM_CYMENU);
		np2class_enablemenu(g_hWndMain, FALSE);
		SetWindowLong(g_hWndMain, GWL_STYLE, winstyle);
		SetWindowLong(g_hWndMain, GWL_EXSTYLE, winstyleex);
	}
	else {
		scrnmng.flag = SCRNFLAG_HAVEEXTEND;
		winstyle |= WS_SYSMENU;
		if(np2oscfg.mouse_nc){
			if (np2oscfg.wintype != 0) {
				WINLOCEX	wlex;
				// XXX: メニューが出せなくなって詰むのを回避（暫定）
				np2oscfg.wintype = 0;
				np2oscfg.wintype = 0;
				wlex = np2_winlocexallwin(g_hWndMain);
				winlocex_setholdwnd(wlex, g_hWndMain);
				np2class_windowtype(g_hWndMain, np2oscfg.wintype);
				winlocex_move(wlex);
				winlocex_destroy(wlex);
			}
		}
		if (np2oscfg.thickframe) {
			winstyle |= WS_THICKFRAME;
		}
		if (np2oscfg.wintype < 2) {
			winstyle |= WS_CAPTION;
		}
		winstyle &= ~WS_POPUP;
		winstyleex &= ~WS_EX_TOPMOST;
		if(lastscrnmode & SCRNMODE_FULLSCREEN){
			char *strtmp;
			char szModulePath[MAX_PATH];
			GetModuleFileNameA(NULL, szModulePath, sizeof(szModulePath)/sizeof(szModulePath[0]));
			strtmp = strrchr(szModulePath, '\\');
			if(strtmp){
				*strtmp = 0;
			}else{
				szModulePath[0] = 0;
			}
			strcat(szModulePath, "\\ddraw.dll");
			if(PathFileExistsA(szModulePath) && !PathIsDirectoryA(szModulePath)){
				// DXGLが使われていそうなので素直なコードに変更
				SetWindowLong(g_hWndMain, GWL_STYLE, winstyle);
				SetWindowLong(g_hWndMain, GWL_EXSTYLE, winstyleex);
			}else{
				ShowWindow(g_hWndMain, SW_HIDE); // Aeroな環境でフルスクリーン→ウィンドウの時にシステムアイコンが消える対策のためのおまじない（その1）
				SetWindowLong(g_hWndMain, GWL_STYLE, winstyle);
				SetWindowLong(g_hWndMain, GWL_EXSTYLE, winstyleex);
				ShowWindow(g_hWndMain, SW_SHOWNORMAL); // Aeroな環境で(ry（その2）
				ShowWindow(g_hWndMain, SW_HIDE); // Aeroな環境で(ry（その3）
				ShowWindow(g_hWndMain, SW_SHOWNORMAL); // Aeroな環境で(ry（その4）
			}
			SetWindowPlacement(g_hWndMain, &wp);
		}else{
			SetWindowLong(g_hWndMain, GWL_STYLE, winstyle);
			SetWindowLong(g_hWndMain, GWL_EXSTYLE, winstyleex);
			GetWindowPlacement(g_hWndMain, &wp);
		}
	}
	
	if(np2oscfg.emuddraw){
		devlpguid = (LPGUID)DDCREATE_EMULATIONONLY;
	}else{
		devlpguid = NULL;
		if(scrnmode & SCRNMODE_FULLSCREEN){
			r = DirectDrawEnumerateExA(DDEnumDisplayCallbackA, &devguid, DDENUM_ATTACHEDSECONDARYDEVICES);
			if(devguid != GUID_NULL){
				devlpguid = &devguid;
			}
		}
	}
	if ((r = DirectDrawCreate(devlpguid, &ddraw.ddraw1, NULL)) != DD_OK) {
		// プライマリで再挑戦
		if (DirectDrawCreate(np2oscfg.emuddraw ? (LPGUID)DDCREATE_EMULATIONONLY : NULL, &ddraw.ddraw1, NULL) != DD_OK) {
			goto scre_err;
		}
	}
	ddraw.ddraw1->QueryInterface(IID_IDirectDraw2, (void **)&ddraw2);
	ddraw.ddraw2 = ddraw2;

	if (scrnmode & SCRNMODE_FULLSCREEN) {
#if defined(SUPPORT_DCLOCK)
		DispClock::GetInstance()->Initialize();
#endif
		ddraw2->SetCooperativeLevel(g_hWndMain,
										DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN | DDSCL_MULTITHREADED);
		width = np2oscfg.fscrn_cx;
		height = np2oscfg.fscrn_cy;
#ifdef SUPPORT_WAB
		if(!np2wabwnd.multiwindow && (np2wab.relay&0x3)){
			if(np2wab.realWidth>=640 && np2wab.realHeight>=400){
				width = np2wab.realWidth;
				height = np2wab.realHeight;
			}else{
				width = 640;
				height = 480;
			}
		}
#endif
		bitcolor = np2oscfg.fscrnbpp;
		fscrnmod = FSCRNCFG_fscrnmod;
		if (((fscrnmod & (FSCRNMOD_SAMERES | FSCRNMOD_SAMEBPP)) || np2_multithread_Enabled()) &&
			(dd_displayName[0] ? EnumDisplaySettings(dd_displayName, ENUM_REGISTRY_SETTINGS, &devmode) : EnumDisplaySettings(NULL, ENUM_REGISTRY_SETTINGS, &devmode))) {
			if ((fscrnmod & FSCRNMOD_SAMERES) || np2_multithread_Enabled()) {
				width = devmode.dmPelsWidth;
				height = devmode.dmPelsHeight;
			}
			if ((fscrnmod & FSCRNMOD_SAMEBPP) || np2_multithread_Enabled()) {
				bitcolor = devmode.dmBitsPerPel;
			}
		}
		if ((width == 0) || (height == 0)) {
			width = 640;
			height = (np2oscfg.force400)?400:480;
		}
		if (bitcolor == 0) {
#if !defined(SUPPORT_PC9821)
			bitcolor = (scrnmode & SCRNMODE_HIGHCOLOR)?16:8;
#else
			bitcolor = 16;
#endif
		}
		if (!((fscrnmod & FSCRNMOD_SAMERES) || np2_multithread_Enabled()))
		{
			// 解像度変えるモードなら帰る
			if (ddraw2->SetDisplayMode(width, height, bitcolor, 0, 0) != DD_OK)
			{
				width = 640;
				height = 480;
				if (ddraw2->SetDisplayMode(width, height, bitcolor, 0, 0) != DD_OK)
				{
					goto scre_err;
				}
			}
		}
		ddraw2->CreateClipper(0, &ddraw.clipper, NULL);
		ddraw.clipper->SetHWnd(0, g_hWndMain);

		ZeroMemory(&ddsd, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		ddsd.dwFlags = DDSD_CAPS;
		ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
		if (ddraw2->CreateSurface(&ddsd, &ddraw.primsurf, NULL) != DD_OK) {
			goto scre_err;
		}
//		fullscrn_clearblank();

		ZeroMemory(&ddpf, sizeof(ddpf));
		ddpf.dwSize = sizeof(DDPIXELFORMAT);
		if (ddraw.primsurf->GetPixelFormat(&ddpf) != DD_OK) {
			goto scre_err;
		}

		ZeroMemory(&ddsd, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
		ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
#ifdef SUPPORT_WAB
		if(!np2wabwnd.multiwindow){
			if((FSCRNCFG_fscrnmod & FSCRNMOD_SAMERES) || np2_multithread_Enabled()){
				int maxx = GetSystemMetrics(SM_CXSCREEN);
				int maxy = GetSystemMetrics(SM_CYSCREEN);
				ddsd.dwWidth = (WAB_MAX_WIDTH > maxx ? maxx : WAB_MAX_WIDTH);
				ddsd.dwHeight = (WAB_MAX_HEIGHT > maxy ? maxy : WAB_MAX_HEIGHT);
			}else{
				if((np2wab.relay&0x3)!=0 && np2wab.realWidth>=640 && np2wab.realHeight>=400){
					// 実サイズに
					ddsd.dwWidth = np2wab.realWidth;
					ddsd.dwHeight = np2wab.realHeight;
				}else{
					ddsd.dwWidth = 640;
					ddsd.dwHeight = 480;
				}
			}
		}else{
			ddsd.dwWidth = 640;
			ddsd.dwHeight = 480;
		}
#else
		ddsd.dwWidth = 640;
		ddsd.dwHeight = 480;
#endif
		if (ddraw2->CreateSurface(&ddsd, &ddraw.backsurf, NULL) != DD_OK) {
			goto scre_err;
		}
#ifdef SUPPORT_WAB
		if (ddraw2->CreateSurface(&ddsd, &ddraw.wabsurf, NULL) != DD_OK) {
			goto scre_err;
		}else{
			DDBLTFX	ddbf = {0};
			ddbf.dwSize = sizeof(ddbf);
			ddbf.dwFillColor = 0;
			ddraw.wabsurf->Blt(NULL, NULL, NULL, DDBLT_WAIT | DDBLT_COLORFILL, &ddbf);
		}
#endif
		ZeroMemory(&ddsd, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
		ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
		ddsd.dwWidth = FILLSURF_SIZE;
		ddsd.dwHeight = FILLSURF_SIZE;
		if (ddraw2->CreateSurface(&ddsd, &ddraw.fillsurf, NULL) != DD_OK) {
			goto scre_err;
		}
		{
			DDBLTFX	ddbf = {0};
			ddbf.dwSize = sizeof(ddbf);
			ddbf.dwFillColor = 0;
			ddraw.fillsurf->Blt(NULL, NULL, NULL, DDBLT_WAIT | DDBLT_COLORFILL, &ddbf);
		}

		if (bitcolor == 8) {
			paletteinit();
		}
		else if (bitcolor == 16) {
			make16mask(ddpf.dwBBitMask, ddpf.dwRBitMask, ddpf.dwGBitMask);
		}
		else if (bitcolor == 24) {
		}
		else if (bitcolor == 32) {
		}
		else {
			goto scre_err;
		}
#if defined(SUPPORT_DCLOCK)
		DispClock::GetInstance()->SetPalettes(bitcolor);
		ZeroMemory(&ddsd, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
		ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
		ddsd.dwWidth = DCLOCK_WIDTH;
		ddsd.dwHeight = DCLOCK_HEIGHT;
		ddraw2->CreateSurface(&ddsd, &ddraw.clocksurf, NULL);
		DispClock::GetInstance()->Reset();
#endif
		ddraw.extend = 1;
	}
	else {
		ddraw2->SetCooperativeLevel(g_hWndMain, DDSCL_NORMAL | DDSCL_MULTITHREADED);

		ZeroMemory(&ddsd, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		ddsd.dwFlags = DDSD_CAPS;
		ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
		if (ddraw2->CreateSurface(&ddsd, &ddraw.primsurf, NULL) != DD_OK) {
			goto scre_err;
		}

		ddraw2->CreateClipper(0, &ddraw.clipper, NULL);
		ddraw.clipper->SetHWnd(0, g_hWndMain);
		ddraw.primsurf->SetClipper(ddraw.clipper);

		ZeroMemory(&ddpf, sizeof(ddpf));
		ddpf.dwSize = sizeof(DDPIXELFORMAT);
		if (ddraw.primsurf->GetPixelFormat(&ddpf) != DD_OK) {
			goto scre_err;
		}

		ZeroMemory(&ddsd, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
		ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
#ifdef SUPPORT_WAB
		if(!np2wabwnd.multiwindow && (np2wab.relay&0x3)!=0 && np2wab.realWidth>=640 && np2wab.realHeight>=400){
			// 実サイズに
			width = ddsd.dwWidth = scrnstat.width;//np2wab.realWidth;
			height = ddsd.dwHeight = scrnstat.height;//np2wab.realHeight;
			if (scrnmode & SCRNMODE_ROTATE) {
				ddsd.dwWidth = scrnstat.height;
				ddsd.dwHeight = scrnstat.width;
				ddsd.dwHeight++;
			}
			else {
				ddsd.dwWidth++;
			}
		}else{
			if (!(scrnmode & SCRNMODE_ROTATE)) {
				ddsd.dwWidth = 640 + 1;
				ddsd.dwHeight = 480;
			}
			else {
				ddsd.dwWidth = 480;
				ddsd.dwHeight = 640 + 1;
			}
			width = 640;
			height = 480;
		}
#else
		if (!(scrnmode & SCRNMODE_ROTATE)) {
			ddsd.dwWidth = 640 + 1;
			ddsd.dwHeight = 480;
		}
		else {
			ddsd.dwWidth = 480;
			ddsd.dwHeight = 640 + 1;
		}
		width = 640;
		height = 480;
#endif

		if (ddraw2->CreateSurface(&ddsd, &ddraw.backsurf, NULL) != DD_OK) {
			goto scre_err;
		}
#ifdef SUPPORT_WAB
		if (ddraw2->CreateSurface(&ddsd, &ddraw.wabsurf, NULL) != DD_OK) {
			goto scre_err;
		}
#endif
		ZeroMemory(&ddsd, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
		ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
		ddsd.dwWidth = FILLSURF_SIZE;
		ddsd.dwHeight = FILLSURF_SIZE;
		if (ddraw2->CreateSurface(&ddsd, &ddraw.fillsurf, NULL) != DD_OK) {
			goto scre_err;
		}
		{
			DDBLTFX	ddbf = {0};
			ddbf.dwSize = sizeof(ddbf);
			ddbf.dwFillColor = 0;
			ddraw.fillsurf->Blt(NULL, NULL, NULL, DDBLT_WAIT | DDBLT_COLORFILL, &ddbf);
		}
		bitcolor = ddpf.dwRGBBitCount;
		if (bitcolor == 8) {
			paletteinit();
		}
		else if (bitcolor == 16) {
			make16mask(ddpf.dwBBitMask, ddpf.dwRBitMask, ddpf.dwGBitMask);
		}
		else if (bitcolor == 24) {
		}
		else if (bitcolor == 32) {
		}
		else {
			goto scre_err;
		}
		ddraw.extend = 1;
	}
	scrnmng.bpp = (UINT8)bitcolor;
	scrnsurf.bpp = bitcolor;
	ddraw.scrnmode = scrnmode;
	ddraw.width = width;
	ddraw.height = height;
	ddraw.cliping = 0;
	renewalclientsize(TRUE); // XXX: スナップ解除等が起こるので暫定TRUE
	lastscrnmode = scrnmode;
//	screenupdate = 3;					// update!
#if defined(SUPPORT_WAB)
	mt_wabpausedrawing = 0; // MultiThread対策
#endif
	dd_leave_criticalsection();
	return(SUCCESS);

scre_err:
	dd_leave_criticalsection();
	scrnmngDD_destroy();
	return(FAILURE);
}

void scrnmngDD_destroy(void) {
	
	dd_enter_criticalsection();
	if (scrnmng.flag & SCRNFLAG_FULLSCREEN) {
		np2class_enablemenu(g_hWndMain, (!np2oscfg.wintype));
	}
#if defined(SUPPORT_DCLOCK)
	if (ddraw.clocksurf) {
		ddraw.clocksurf->Release();
		ddraw.clocksurf = NULL;
	}
#endif
	if (ddraw.fillsurf) {
		ddraw.fillsurf->Release();
		ddraw.fillsurf = NULL;
	}
#if defined(SUPPORT_WAB)
	if (ddraw.wabsurf) {
		mt_wabpausedrawing = 1; // MultiThread対策
		while(mt_wabdrawing) 
			Sleep(10);
		ddraw.wabsurf->Release();
		ddraw.wabsurf = NULL;
	}
#endif
	if (ddraw.backsurf) {
		ddraw.backsurf->Release();
		ddraw.backsurf = NULL;
	}
	if (ddraw.palette) {
		ddraw.palette->Release();
		ddraw.palette = NULL;
	}
	if (ddraw.clipper) {
		ddraw.clipper->Release();
		ddraw.clipper = NULL;
	}
	if (ddraw.primsurf) {
		ddraw.primsurf->Release();
		ddraw.primsurf = NULL;
	}
	if (ddraw.ddraw2) {
		if (ddraw.scrnmode & SCRNMODE_FULLSCREEN) {
			ddraw.ddraw2->SetCooperativeLevel(g_hWndMain, DDSCL_NORMAL);
		}
		ddraw.ddraw2->Release();
		ddraw.ddraw2 = NULL;
	}
	if (ddraw.ddraw1) {
		ddraw.ddraw1->Release();
		ddraw.ddraw1 = NULL;
	}
	ZeroMemory(&ddraw, sizeof(ddraw));
	dd_leave_criticalsection();
}

void scrnmngDD_shutdown(void) {
	
	if(dd_cs_initialized){
		DeleteCriticalSection(&dd_cs);
		dd_cs_initialized = 0;
	}
}

void scrnmngDD_querypalette(void) {
	
	dd_enter_criticalsection();
	if (ddraw.palette) {
		ddraw.primsurf->SetPalette(ddraw.palette);
	}
	dd_leave_criticalsection();
}

RGB16 scrnmngDD_makepal16(RGB32 pal32) {

	RGB32	pal;

	pal.d = pal32.d & ddraw.pal16mask.d;
	return((RGB16)((pal.p.g << ddraw.l16g) +
						(pal.p.r << ddraw.l16r) + (pal.p.b >> ddraw.r16b)));
}

void scrnmngDD_fullscrnmenu(int y) {

	UINT8	menudisp;
	
	dd_enter_criticalsection();
	if (scrnmng.flag & SCRNFLAG_FULLSCREEN) {
		menudisp = ((y >= 0) && (y < ddraw.menusize))?1:0;
		if (ddraw.menudisp != menudisp) {
			ddraw.menudisp = menudisp;
			if (menudisp == 1) {
				np2class_enablemenu(g_hWndMain, TRUE);
			}
			else {
				np2class_enablemenu(g_hWndMain, FALSE);
				clearoutfullscreen();
			}
		}
	}
	dd_leave_criticalsection();
}

void scrnmngDD_topwinui(void) {

	mousemng_disable(MOUSEPROC_WINUI);
	dd_enter_criticalsection();
	if (!ddraw.cliping++) {											// ver0.28
		if (scrnmng.flag & SCRNFLAG_FULLSCREEN) {
			if (ddraw.primsurf)
			{
				ddraw.primsurf->SetClipper(ddraw.clipper);
			}
		}
#ifndef __GNUC__
		WINNLSEnableIME(g_hWndMain, TRUE);
#endif
	}
	dd_leave_criticalsection();
}

void scrnmngDD_clearwinui(void) {
	
	dd_enter_criticalsection();
	if ((ddraw.cliping > 0) && (!(--ddraw.cliping))) {
#ifndef __GNUC__
		WINNLSEnableIME(g_hWndMain, FALSE);
#endif
		if (scrnmng.flag & SCRNFLAG_FULLSCREEN) {
			if (ddraw.primsurf)
			{
				ddraw.primsurf->SetClipper(0);
			}
		}
	}
	if (scrnmng.flag & SCRNFLAG_FULLSCREEN) {
		np2class_enablemenu(g_hWndMain, FALSE);
		clearoutfullscreen();
		ddraw.menudisp = 0;
	}
	else {
		if (np2oscfg.wintype) {
			np2class_enablemenu(g_hWndMain, FALSE);
			InvalidateRect(g_hWndMain, NULL, TRUE);
		}
	}
	mousemng_enable(MOUSEPROC_WINUI);

	if(scrnmng.forcereset){
		scrnmng_destroy();
		if (scrnmng_create(g_scrnmode) != SUCCESS) {
			g_scrnmode &= ~SCRNMODE_FULLSCREEN;
			if (scrnmng_create(g_scrnmode) != SUCCESS) {
				scrnmng_create_pending = true;
				dd_leave_criticalsection();
				//PostQuitMessage(0);
				return;
			}
		}
		scrnmng.forcereset = 0;
	}
	dd_leave_criticalsection();
}

void scrnmngDD_setwidth(int posx, int width) {
	
	if(scrnstat.multiple < 1) scrnstat.multiple = 8;
	scrnstat.width = width;
	renewalclientsize(TRUE);
}

void scrnmngDD_setextend(int extend) {

	scrnstat.extend = extend;
	scrnmng.allflash = TRUE;
	renewalclientsize(TRUE);
}

void scrnmngDD_setheight(int posy, int height) {
	
	if(scrnstat.multiple < 1) scrnstat.multiple = 8;
	scrnstat.height = height;
	renewalclientsize(TRUE);
}

void scrnmngDD_setsize(int posx, int posy, int width, int height) {
	
	if(scrnstat.multiple < 1) scrnstat.multiple = 8;
	scrnstat.width = width;
	scrnstat.height = height;
	renewalclientsize(TRUE);
}

const SCRNSURF *scrnmngDD_surflock(void) {

	DDSURFACEDESC	destscrn;
	HRESULT			r;

	ZeroMemory(&destscrn, sizeof(destscrn));
	destscrn.dwSize = sizeof(destscrn);
	if (ddraw.backsurf == NULL) {
		return(NULL);
	}
	dd_enter_criticalsection();
	if (!ddraw.backsurf){
		dd_leave_criticalsection();
		return(NULL);
	}
	r = ddraw.backsurf->Lock(NULL, &destscrn, DDLOCK_WAIT, NULL);
	if (r == DDERR_SURFACELOST) {
		scrnmng_restore_pending = true;
		dd_leave_criticalsection();
		return(NULL);
		//r = ddraw.backsurf->Lock(NULL, &destscrn, DDLOCK_WAIT, NULL);
	}
	if (r != DD_OK) {
//		TRACEOUT(("backsurf lock error: %d (%d)", r));
		dd_leave_criticalsection();
		return(NULL);
	}
	if (!(ddraw.scrnmode & SCRNMODE_ROTATE)) {
		scrnsurf.ptr = (UINT8 *)destscrn.lpSurface;
		scrnsurf.xalign = scrnsurf.bpp >> 3;
		scrnsurf.yalign = destscrn.lPitch;
	}
	else if (!(ddraw.scrnmode & SCRNMODE_ROTATEDIR)) {
		scrnsurf.ptr = (UINT8 *)destscrn.lpSurface;
		scrnsurf.ptr += (scrnsurf.width + scrnsurf.extend - 1) * destscrn.lPitch;
		scrnsurf.xalign = 0 - destscrn.lPitch;
		scrnsurf.yalign = scrnsurf.bpp >> 3;
	}
	else {
		scrnsurf.ptr = (UINT8 *)destscrn.lpSurface;
		scrnsurf.ptr += (scrnsurf.height - 1) * (scrnsurf.bpp >> 3);
		scrnsurf.xalign = destscrn.lPitch;
		scrnsurf.yalign = 0 - (scrnsurf.bpp >> 3);
	}
	return(&scrnsurf);
}

void scrnmngDD_surfunlock(const SCRNSURF *surf) {

	if (ddraw.backsurf == NULL)
	{
		dd_leave_criticalsection();
		return;
	}
	ddraw.backsurf->Unlock(NULL);
	scrnmngDD_update();
	recvideo_update();
	dd_leave_criticalsection();
}

void scrnmngDD_update(void) {

	POINT	clip;
	RECT	dst;
	RECT	*rect;
	RECT	*scrn;
	HRESULT	r;
	
	if(!dd_tryenter_criticalsection()){
		req_enter_criticalsection = 1;
		return;
	}
	//dd_enter_criticalsection();
	if (scrnmng.palchanged) {
		scrnmng.palchanged = FALSE;
		paletteset();
	}
	if (ddraw.backsurf != NULL) {
		if (ddraw.scrnmode & SCRNMODE_FULLSCREEN) {
			if (scrnmng.allflash) {
				scrnmng.allflash = 0;
				clearoutfullscreen();
			}
			if (GetWindowLongPtr(g_hWndMain, NP2GWLP_HMENU)) {
				rect = &ddraw.rect;
				scrn = &ddraw.scrn;
			}
			else {
				rect = &ddraw.rectclip;
				scrn = &ddraw.scrnclip;
			}
			r = ddraw.primsurf->Blt(scrn, ddraw.backsurf, rect,
															DDBLT_WAIT, NULL);
			if (r == DDERR_SURFACELOST) {
				scrnmng_restore_pending = true;
				dd_leave_criticalsection();
				return;
				//ddraw.primsurf->Blt(scrn, ddraw.backsurf, rect,
				//											DDBLT_WAIT, NULL);
			}
		}
		else {
			if (scrnmng.allflash) {
				scrnmng.allflash = 0;
				clearoutscreen();
			}
			clip.x = 0;
			clip.y = 0;
			ClientToScreen(g_hWndMain, &clip);
			dst.left = clip.x + ddraw.scrn.left;
			dst.top = clip.y + ddraw.scrn.top;
			dst.right = clip.x + ddraw.scrn.right;
			dst.bottom = clip.y + ddraw.scrn.bottom;
			r = ddraw.primsurf->Blt(&dst, ddraw.backsurf, &ddraw.rect,
									DDBLT_WAIT, NULL);
			if (r == DDERR_SURFACELOST) {
				scrnmng_restore_pending = true;
				dd_leave_criticalsection();
				return;
				//ddraw.primsurf->Blt(&dst, ddraw.backsurf, &ddraw.rect,
				//										DDBLT_WAIT, NULL);
			}
		}
	}
	dd_leave_criticalsection();
	req_enter_criticalsection = 0;
}


// ----

void scrnmngDD_setmultiple(int multiple)
{
	if (scrnstat.multiple != multiple)
	{
		scrnstat.multiple = multiple;
		renewalclientsize(TRUE);
	}
}

int scrnmngDD_getmultiple(void)
{
	return scrnstat.multiple;
}



// ----

#if defined(SUPPORT_DCLOCK)
static const RECT rectclk = {0, 0, DCLOCK_WIDTH, DCLOCK_HEIGHT};

BOOL scrnmngDD_isdispclockclick(const POINT *pt) {

	if (pt->y >= (ddraw.height - DCLOCK_HEIGHT)) {
		return(TRUE);
	}
	else {
		return(FALSE);
	}
}

void scrnmngDD_dispclock(void)
{
	if (!ddraw.clocksurf)
	{
		return;
	}
	if (!DispClock::GetInstance()->IsDisplayed())
	{
		return;
	}

	const RECT* scrn;
	if (GetWindowLongPtr(g_hWndMain, NP2GWLP_HMENU))
	{
		scrn = &ddraw.scrn;
	}
	else
	{
		scrn = &ddraw.scrnclip;
	}
	if ((scrn->bottom + DCLOCK_HEIGHT) > ddraw.height)
	{
		return;
	}
	DispClock::GetInstance()->Make();
	
	dd_enter_criticalsection();
	DDSURFACEDESC dest;
	ZeroMemory(&dest, sizeof(dest));
	dest.dwSize = sizeof(dest);
	if (ddraw.clocksurf->Lock(NULL, &dest, DDLOCK_WAIT, NULL) == DD_OK)
	{
		DispClock::GetInstance()->Draw(scrnmng.bpp, dest.lpSurface, dest.lPitch);
		ddraw.clocksurf->Unlock(NULL);
	}
	if (ddraw.primsurf->BltFast(ddraw.width - DCLOCK_WIDTH - 4,
									ddraw.height - DCLOCK_HEIGHT,
									ddraw.clocksurf, (RECT *)&rectclk,
									DDBLTFAST_WAIT) == DDERR_SURFACELOST)
	{
		scrnmng_restore_pending = true;
	}
	DispClock::GetInstance()->CountDown(np2oscfg.DRAW_SKIP);
	dd_leave_criticalsection();
}
#endif


// ----

typedef struct {
	int		bx;
	int		by;
	int		cx;
	int		cy;
	int		mul;
} SCRNSIZING;

static	SCRNSIZING	scrnsizing;

enum {
	SIZING_ADJUST	= 12
};

void scrnmngDD_entersizing(void) {

	RECT	rectwindow;
	RECT	rectclient;
	int		cx;
	int		cy;

	GetWindowRect(g_hWndMain, &rectwindow);
	GetClientRect(g_hWndMain, &rectclient);
	scrnsizing.bx = (np2oscfg.paddingx * 2) +
					(rectwindow.right - rectwindow.left) -
					(rectclient.right - rectclient.left);
	scrnsizing.by = (np2oscfg.paddingy * 2) +
					(rectwindow.bottom - rectwindow.top) -
					(rectclient.bottom - rectclient.top);
	cx = min(scrnstat.width, ddraw.width);
	cx = (cx + 7) >> 3;
	cy = min(scrnstat.height, ddraw.height);
	cy = (cy + 7) >> 3;
	if (!(ddraw.scrnmode & SCRNMODE_ROTATE)) {
		scrnsizing.cx = cx;
		scrnsizing.cy = cy;
	}
	else {
		scrnsizing.cx = cy;
		scrnsizing.cy = cx;
	}
	scrnsizing.mul = scrnstat.multiple;
}

void scrnmngDD_sizing(UINT side, RECT *rect) {

	int		width;
	int		height;
	int		mul;
	const int	mul_max = 255;

	if ((side != WMSZ_TOP) && (side != WMSZ_BOTTOM)) {
		width = rect->right - rect->left - scrnsizing.bx + SIZING_ADJUST;
		width /= scrnsizing.cx;
	}
	else {
		width = mul_max;
	}
	if ((side != WMSZ_LEFT) && (side != WMSZ_RIGHT)) {
		height = rect->bottom - rect->top - scrnsizing.by + SIZING_ADJUST;
		height /= scrnsizing.cy;
	}
	else {
		height = mul_max;
	}
	mul = min(width, height);
	if (mul <= 0) {
		mul = 1;
	}
	else if (mul > mul_max) {
		mul = mul_max;
	}
	width = scrnsizing.bx + (scrnsizing.cx * mul);
	height = scrnsizing.by + (scrnsizing.cy * mul);
	switch(side) {
		case WMSZ_LEFT:
		case WMSZ_TOPLEFT:
		case WMSZ_BOTTOMLEFT:
			rect->left = rect->right - width;
			break;

		case WMSZ_RIGHT:
		case WMSZ_TOP:
		case WMSZ_TOPRIGHT:
		case WMSZ_BOTTOM:
		case WMSZ_BOTTOMRIGHT:
		default:
			rect->right = rect->left + width;
			break;
	}

	switch(side) {
		case WMSZ_TOP:
		case WMSZ_TOPLEFT:
		case WMSZ_TOPRIGHT:
			rect->top = rect->bottom - height;
			break;

		case WMSZ_LEFT:
		case WMSZ_RIGHT:
		case WMSZ_BOTTOM:
		case WMSZ_BOTTOMLEFT:
		case WMSZ_BOTTOMRIGHT:
		default:
			rect->bottom = rect->top + height;
			break;
	}
	scrnsizing.mul = mul;
}

void scrnmngDD_exitsizing(void)
{
	scrnmng_setmultiple(scrnsizing.mul);
	InvalidateRect(g_hWndMain, NULL, TRUE);		// ugh
}

// フルスクリーン解像度調整
void scrnmngDD_updatefsres(void) {
#ifdef SUPPORT_WAB
	RECT rect;
	int width = scrnstat.width;
	int height = scrnstat.height;
	DDBLTFX	ddbf = {0};

	rect.left = rect.top = 0;
	rect.right = width;
	rect.bottom = height;

	ddbf.dwSize = sizeof(ddbf);
	ddbf.dwFillColor = 0;

	if(((FSCRNCFG_fscrnmod & FSCRNMOD_SAMERES) || np2_multithread_Enabled()) && (g_scrnmode & SCRNMODE_FULLSCREEN)){
		dd_enter_criticalsection();
		ddraw.wabsurf->Blt(NULL, NULL, NULL, DDBLT_WAIT | DDBLT_COLORFILL, &ddbf);
		ddraw.backsurf->Blt(NULL, NULL, NULL, DDBLT_WAIT | DDBLT_COLORFILL, &ddbf);
		clearoutscreen();
		np2wab.lastWidth = 0;
		np2wab.lastHeight = 0;
		dd_leave_criticalsection();
		return;
	}
	if(scrnstat.width<100 || scrnstat.height<100){
		dd_enter_criticalsection();
		ddraw.wabsurf->Blt(NULL, NULL, NULL, DDBLT_WAIT | DDBLT_COLORFILL, &ddbf);
		ddraw.backsurf->Blt(NULL, NULL, NULL, DDBLT_WAIT | DDBLT_COLORFILL, &ddbf);
		clearoutscreen();
		dd_leave_criticalsection();
		return;
	}
	
	if(np2wab.lastWidth!=width || np2wab.lastHeight!=height){
		dd_enter_criticalsection();
		np2wab.lastWidth = width;
		np2wab.lastHeight = height;
		if((g_scrnmode & SCRNMODE_FULLSCREEN)!=0){
			g_scrnmode = g_scrnmode & ~SCRNMODE_FULLSCREEN;
			scrnmngDD_destroy();
			if (scrnmngDD_create(g_scrnmode | SCRNMODE_FULLSCREEN) == SUCCESS) {
				g_scrnmode = g_scrnmode | SCRNMODE_FULLSCREEN;
			}
			else {
				if (scrnmngDD_create(g_scrnmode) != SUCCESS) {
					scrnmng_create_pending = true;
					//PostQuitMessage(0);
					dd_leave_criticalsection();
					return;
				}
			}
		}else if(ddraw.width < width || ddraw.height < height){
			scrnmngDD_destroy();
			if (scrnmngDD_create(g_scrnmode) != SUCCESS) {
				if (scrnmngDD_create(g_scrnmode | SCRNMODE_FULLSCREEN) != SUCCESS) { // フルスクリーンでリトライ
					scrnmng_create_pending = true;
					//PostQuitMessage(0);
					dd_leave_criticalsection();
					return;
				}
				g_scrnmode = g_scrnmode | SCRNMODE_FULLSCREEN;
			}
		}
		clearoutscreen();
		ddraw.wabsurf->Blt(NULL, NULL, NULL, DDBLT_WAIT | DDBLT_COLORFILL, &ddbf);
		ddraw.backsurf->Blt(NULL, NULL, NULL, DDBLT_WAIT | DDBLT_COLORFILL, &ddbf);
		dd_leave_criticalsection();
	}
#endif
}

// ウィンドウアクセラレータ画面転送
void scrnmngDD_blthdc(HDC hdc) {
#if defined(SUPPORT_WAB)
	HRESULT	r;
	HDC hDCDD;
	mt_wabdrawing = 0;
	if (np2wabwnd.multiwindow) return;
	if (mt_wabpausedrawing) return;
	if (np2wab.wndWidth < 32 || np2wab.wndHeight < 32) return;
	if (ddraw.wabsurf != NULL) {
		dd_enter_criticalsection();
		if (ddraw.wabsurf != NULL)
		{
			mt_wabdrawing = 1;
			r = ddraw.wabsurf->GetDC(&hDCDD);
			if (r == DD_OK)
			{
				POINT pt[3];
				switch (ddraw.scrnmode & SCRNMODE_ROTATEMASK)
				{
				case SCRNMODE_ROTATELEFT:
					pt[0].x = 0;
					pt[0].y = scrnstat.width;
					pt[1].x = 0;
					pt[1].y = 0;
					pt[2].x = scrnstat.height;
					pt[2].y = scrnstat.width;
					r = PlgBlt(hDCDD, pt, hdc, 0, 0, np2wab.realWidth, np2wab.realHeight, NULL, 0, 0);
					break;
				case SCRNMODE_ROTATERIGHT:
					pt[0].x = scrnstat.height;
					pt[0].y = 0;
					pt[1].x = scrnstat.height;
					pt[1].y = scrnstat.width;
					pt[2].x = 0;
					pt[2].y = 0;
					r = PlgBlt(hDCDD, pt, hdc, 0, 0, np2wab.realWidth, np2wab.realHeight, NULL, 0, 0);
					break;
				default:
					r = BitBlt(hDCDD, 0, 0, scrnstat.width, scrnstat.height, hdc, 0, 0, SRCCOPY);
				}
				ddraw.wabsurf->ReleaseDC(hDCDD);
			}
			mt_wabdrawing = 0;
		}
		dd_leave_criticalsection();
	}
#endif
}
void scrnmngDD_bltwab() {
#if defined(SUPPORT_WAB)
	RECT	*dst;
	RECT	src;
	RECT	dstmp;
	//DDBLTFX ddfx;
	HRESULT	r;
	int exmgn = 0;
	if (np2wabwnd.multiwindow) return;
	if (ddraw.backsurf != NULL) {
		if (ddraw.scrnmode & SCRNMODE_FULLSCREEN) {
			if (GetWindowLongPtr(g_hWndMain, NP2GWLP_HMENU)) {
				dst = &ddraw.rect;
			}
			else {
				dst = &ddraw.rectclip;
			}
		}else{
			dst = &ddraw.rect;
			exmgn = scrnstat.extend;
		}
		src.left = src.top = 0;

		if (!(ddraw.scrnmode & SCRNMODE_ROTATE)) {
			src.right = scrnstat.width;
			src.bottom = scrnstat.height;
			dstmp = *dst;
			dstmp.left += exmgn;
			dstmp.right = dstmp.left + scrnstat.width;
		}else{
			src.right = scrnstat.height;
			src.bottom = scrnstat.width;
			dstmp = *dst;
			dstmp.top += exmgn;
			dstmp.bottom = dstmp.top + scrnstat.width;
		}
		dd_enter_criticalsection();
		if (ddraw.backsurf != NULL)
		{
			r = ddraw.backsurf->Blt(&dstmp, ddraw.wabsurf, &src, DDBLT_WAIT, NULL);
			if (r == DDERR_SURFACELOST)
			{
				scrnmng_restore_pending = true;
			}
		}
		dd_leave_criticalsection();
	}
#endif
}

// 描画領域の範囲をクライアント座標で取得
void scrnmngDD_getrect(RECT *lpRect) {
	if (ddraw.scrnmode & SCRNMODE_FULLSCREEN) {
		if (GetWindowLongPtr(g_hWndMain, NP2GWLP_HMENU)) {
			*lpRect = ddraw.scrn;
		}
		else {
			*lpRect = ddraw.scrnclip;
		}
	}
	else {
		*lpRect = ddraw.scrn;
	}
}