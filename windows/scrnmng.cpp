/**
 * @file	scrnmng.cpp
 * @brief	Screen Manager (Main)
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
#include "winloc.h"
#include "mousemng.h"
#include "scrnmng.h"
#include "scrnmng_dd.h"
#include "scrnmng_d3d.h"
// #include "sysmng.h"
#include "dialog\np2class.h"
#include "pccore.h"
#include "scrndraw.h"
#include "palettes.h"
#include "ini.h"

#if defined(SUPPORT_DCLOCK)
#include "subwnd\dclock.h"
#endif
#include "recvideo.h"

#ifdef SUPPORT_WAB
#include "wab/wab.h"
#endif
#include	"np2mt.h"

#include <shlwapi.h>

SCRNMNG		scrnmng;
SCRNSTAT	scrnstat;
SCRNRESCFG	scrnrescfg = {0};

int d3davailable = 0;

static BOOL scrnmng_cs_initialized = 0; // Screen Manager クリティカルセクション 初期化済みフラグ
static CRITICAL_SECTION	scrnmng_cs = {0}; // Screen Manager surflock in/out クリティカルセクション
static void scrnmng_cs_Initialize(){
	if(!scrnmng_cs_initialized){
		InitializeCriticalSection(&scrnmng_cs);
		scrnmng_cs_initialized = TRUE;
	}
}
static void scrnmng_cs_Finalize(){
	if(scrnmng_cs_initialized){
		DeleteCriticalSection(&scrnmng_cs);
		scrnmng_cs_initialized = FALSE;
	}
}
void scrnmng_cs_EnterModeChangeCriticalSection(){
	if(scrnmng_cs_initialized){
		EnterCriticalSection(&scrnmng_cs);
	}
}
void scrnmng_cs_LeaveModeChangeCriticalSection(){
	if(scrnmng_cs_initialized){
		LeaveCriticalSection(&scrnmng_cs);
	}
}

// ----

UINT8 scrnmng_current_drawtype = DRAWTYPE_INVALID;
bool scrnmng_create_pending = false; // グラフィックレンダラ生成保留中
bool scrnmng_restore_pending = false;
static bool scrnmng_changemode_pending = false; // 画面モード変更保留中
static int scrnmng_changemode_posx = INT_MAX;
static int scrnmng_changemode_posy = INT_MAX;
static int scrnmng_changemode_width = INT_MAX;
static int scrnmng_changemode_height = INT_MAX;
static int scrnmng_changemode_extend = INT_MAX;
static int scrnmng_changemode_multiple = INT_MAX;
static bool scrnmng_changemode_updatefsres = false;
static DWORD scrnmng_UIthreadID = 0;

/**
 * 設定
 */
static const PFTBL s_scrnresini[] =
{
	PFVAL("hasfscfg", PFTYPE_BOOL,	&scrnrescfg.hasfscfg),
	PFVAL("fscrnmod", PFTYPE_HEX8,	&scrnrescfg.fscrnmod),
	PFVAL("SCRN_MUL", PFTYPE_UINT8,	&scrnrescfg.scrn_mul),
	PFVAL("D3D_IMODE", PFTYPE_UINT8,&scrnrescfg.d3d_imode),
};

/**
 * 設定読み込み
 */
void scrnres_readini()
{
	scrnres_readini_res(scrnstat.width, scrnstat.height);
}
void scrnres_readini_res(int width, int height)
{
	static int lastWidth = 0;
	static int lastHeight = 0;

	if(lastWidth!=width || lastHeight!=height){
		OEMCHAR szSectionName[200] = {0};
		OEMCHAR szPath[MAX_PATH];

		OEMSPRINTF(szSectionName, OEMTEXT("Resolution%dx%d"), width, height);

		initgetfile(szPath, _countof(szPath));
		ini_read(szPath, szSectionName, s_scrnresini, _countof(s_scrnresini));

		lastWidth = width;
		lastHeight = height;
	}
}

/**
 * 設定書き込み
 */
void scrnres_writeini()
{
	if(!np2oscfg.readonly){
		OEMCHAR szSectionName[200] = {0};
		TCHAR szPath[MAX_PATH];
		
		OEMSPRINTF(szSectionName, OEMTEXT("Resolution%dx%d"), scrnstat.width, scrnstat.height);

		initgetfile(szPath, _countof(szPath));
		ini_write(szPath, szSectionName, s_scrnresini, _countof(s_scrnresini));
	}
}

void scrnmng_setwindowsize(HWND hWnd, int width, int height)
{
	RECT workrc;
	SystemParametersInfo(SPI_GETWORKAREA, 0, &workrc, 0);
	const int scx = GetSystemMetrics(SM_CXSCREEN);
	const int scy = GetSystemMetrics(SM_CYSCREEN);
	
	// マルチモニタ暫定対応 ver0.86 rev30
	workrc.right = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	workrc.bottom = GetSystemMetrics(SM_CYVIRTUALSCREEN);

	UINT cnt = 2;
	do
	{
		RECT rectwindow;
		int marginX, marginY;
		winloc_getDWMmargin(hWnd, &marginX, &marginY); // Win10環境用の対策
		GetWindowRect(hWnd, &rectwindow);
		RECT rectclient;
		GetClientRect(hWnd, &rectclient);
		int winx = (np2oscfg.winx != CW_USEDEFAULT) ? np2oscfg.winx : rectwindow.left;
		int winy = (np2oscfg.winy != CW_USEDEFAULT) ? np2oscfg.winy : rectwindow.top;
		int cx = width;
		cx += np2oscfg.paddingx * 2 * scrnstat.multiple / 8;
		cx += rectwindow.right - rectwindow.left;
		cx -= rectclient.right - rectclient.left;
		int cy = height;
		cy += np2oscfg.paddingy * 2 * scrnstat.multiple / 8;
		cy += rectwindow.bottom - rectwindow.top;
		cy -= rectclient.bottom - rectclient.top;

		if (scx < cx)
		{
			winx = (scx - cx) / 2;
		}
		else
		{
			if ((winx + cx) > workrc.right + marginX)
			{
				winx = workrc.right + marginX - cx;
			}
			if (winx < workrc.left)
			{
				winx = workrc.left;
			}
		}
		if (scy < cy)
		{
			winy = (scy - cy) / 2;
		}
		else
		{
			if ((winy + cy) > workrc.bottom)
			{
				winy = workrc.bottom - cy;
			}
			if (winy < workrc.top)
			{
				winy = workrc.top;
			}
		}
		MoveWindow(hWnd, winx, winy, cx, cy, TRUE);
	} while (--cnt);
}


void scrnmng_initialize(void) {
	
	scrnmng_cs_Initialize();

	scrnstat.width = 640;
	scrnstat.height = 400;
	scrnstat.extend = 1;
	scrnstat.multiple = 8;
	scrnmng_setwindowsize(g_hWndMain, 640, 400);

	scrnmng_UIthreadID = GetCurrentThreadId();
	
	if(np2oscfg.fsrescfg) scrnres_readini_res(640, 400);
}

BRESULT scrnmng_create(UINT8 scrnmode) {
	
	if(scrnmng_UIthreadID != GetCurrentThreadId()) return 1; // 別のスレッドからのアクセスは不可

#ifdef SUPPORT_SCRN_DIRECT3D
	scrnmng_current_drawtype = np2oscfg.drawtype;
	if(scrnmng_current_drawtype==DRAWTYPE_DIRECT3D && !d3davailable){
		if(scrnmngD3D_check() != SUCCESS){
			np2oscfg.drawtype = scrnmng_current_drawtype = DRAWTYPE_DIRECTDRAW_HW;
		}else{
			d3davailable = 1;
		}
	}
	if(scrnmng_current_drawtype==DRAWTYPE_DIRECT3D){
		BRESULT r = scrnmngD3D_create(scrnmode);
		np2wab_forceupdate();
		return  r;
	}else
#else
	scrnmng_current_drawtype = DRAWTYPE_DIRECTDRAW_HW;
#endif
	{
		BRESULT r = scrnmngDD_create(scrnmode);
		np2wab_forceupdate();
		return r;
	}
}

void scrnmng_destroy(void) {
	
	if(scrnmng_UIthreadID != GetCurrentThreadId()) return; // 別のスレッドからのアクセスは不可

#ifdef SUPPORT_SCRN_DIRECT3D
	if(scrnmng_current_drawtype==DRAWTYPE_DIRECT3D){
		scrnmngD3D_destroy();
	}else
#endif
	{
		scrnmngDD_destroy();
	}
	scrnmng_current_drawtype = DRAWTYPE_INVALID;
}

void scrnmng_shutdown(void) {
	
#ifdef SUPPORT_SCRN_DIRECT3D
	scrnmngD3D_shutdown();
#endif
	scrnmngDD_shutdown();
	
	scrnmng_cs_Finalize();
}

void scrnmng_querypalette(void) {
	
	if(scrnmng_UIthreadID != GetCurrentThreadId()) return; // 別のスレッドからのアクセスは不可

#ifdef SUPPORT_SCRN_DIRECT3D
	if(scrnmng_current_drawtype==DRAWTYPE_INVALID) {return;}
	if(scrnmng_current_drawtype==DRAWTYPE_DIRECT3D){
		scrnmngD3D_querypalette();
	}else
#endif
	{
		scrnmngDD_querypalette();
	}
}

RGB16 scrnmng_makepal16(RGB32 pal32) {
	
	if(scrnmng_UIthreadID != GetCurrentThreadId()) return 0; // 別のスレッドからのアクセスは不可

#ifdef SUPPORT_SCRN_DIRECT3D
	if(scrnmng_current_drawtype==DRAWTYPE_INVALID) {return 0;}
	if(scrnmng_current_drawtype==DRAWTYPE_DIRECT3D){
		return scrnmngD3D_makepal16(pal32);
	}else
#endif
	{
		return scrnmngDD_makepal16(pal32);
	}
}

void scrnmng_fullscrnmenu(int y) {
	
	if(scrnmng_UIthreadID != GetCurrentThreadId()) return; // 別のスレッドからのアクセスは不可

#ifdef SUPPORT_SCRN_DIRECT3D
	if(scrnmng_current_drawtype==DRAWTYPE_INVALID) {return;}
	if(scrnmng_current_drawtype==DRAWTYPE_DIRECT3D){
		scrnmngD3D_fullscrnmenu(y);
		np2wab_forceupdate();
	}else
#endif
	{
		scrnmngDD_fullscrnmenu(y);
		np2wab_forceupdate();
	}
}

void scrnmng_topwinui(void) {
	
	if(scrnmng_UIthreadID != GetCurrentThreadId()) return; // 別のスレッドからのアクセスは不可

#ifdef SUPPORT_SCRN_DIRECT3D
	if(scrnmng_current_drawtype==DRAWTYPE_INVALID) {return;}
	if(scrnmng_current_drawtype==DRAWTYPE_DIRECT3D){
		scrnmngD3D_topwinui();
		np2wab_forceupdate();
	}else
#endif
	{
		scrnmngDD_topwinui();
		np2wab_forceupdate();
	}
}

void scrnmng_clearwinui(void) {
	
	if(scrnmng_UIthreadID != GetCurrentThreadId()) return; // 別のスレッドからのアクセスは不可

#ifdef SUPPORT_SCRN_DIRECT3D
	if(scrnmng_current_drawtype==DRAWTYPE_INVALID) {return;}
	if(scrnmng_current_drawtype==DRAWTYPE_DIRECT3D){
		scrnmngD3D_clearwinui();
		np2wab_forceupdate();
	}else
#endif
	{
		scrnmngDD_clearwinui();
		np2wab_forceupdate();
	}
}

void scrnmng_setwidth(int posx, int width) {
	
	if(scrnmng_UIthreadID != GetCurrentThreadId()){
		// 別のスレッドからのアクセスの場合、遅延変更
		scrnmng_cs_EnterModeChangeCriticalSection();
		scrnmng_changemode_posx = posx;
		scrnmng_changemode_width = width;
		scrnmng_changemode_pending = true;
		scrnmng_cs_LeaveModeChangeCriticalSection();
		return;
	}
	if(np2oscfg.fsrescfg){
		scrnres_readini_res(width, scrnstat.height);
		scrnstat.multiple = scrnrescfg.scrn_mul;
	}

#ifdef SUPPORT_SCRN_DIRECT3D
	if(scrnmng_current_drawtype==DRAWTYPE_INVALID) {return;}
	if(scrnmng_current_drawtype==DRAWTYPE_DIRECT3D){
		scrnmngD3D_setwidth(posx, width);
		np2wab_forceupdate();
	}else
#endif
	{
		scrnmngDD_setwidth(posx, width);
		np2wab_forceupdate();
	}
}

void scrnmng_setextend(int extend) {
	
	if(scrnmng_UIthreadID != GetCurrentThreadId()){
		// 別のスレッドからのアクセスの場合、遅延変更
		scrnmng_cs_EnterModeChangeCriticalSection();
		scrnmng_changemode_extend = extend;
		scrnmng_changemode_pending = true;
		scrnmng_cs_LeaveModeChangeCriticalSection();
		return;
	}
#ifdef SUPPORT_SCRN_DIRECT3D
	if(scrnmng_current_drawtype==DRAWTYPE_INVALID) { return;}
	if(scrnmng_current_drawtype==DRAWTYPE_DIRECT3D){
		scrnmngD3D_setextend(extend);
		np2wab_forceupdate();
	}else
#endif
	{
		scrnmngDD_setextend(extend);
		np2wab_forceupdate();
	}
}

void scrnmng_setheight(int posy, int height) {
	
	if(scrnmng_UIthreadID != GetCurrentThreadId()){
		// 別のスレッドからのアクセスの場合、遅延変更
		scrnmng_cs_EnterModeChangeCriticalSection();
		scrnmng_changemode_posy = posy;
		scrnmng_changemode_height = height;
		scrnmng_changemode_pending = true;
		scrnmng_cs_LeaveModeChangeCriticalSection();
		return;
	}
	if(np2oscfg.fsrescfg){
		scrnres_readini_res(scrnstat.width, height);
		scrnstat.multiple = scrnrescfg.scrn_mul;
	}

#ifdef SUPPORT_SCRN_DIRECT3D
	if(scrnmng_current_drawtype==DRAWTYPE_INVALID) {return;}
	if(scrnmng_current_drawtype==DRAWTYPE_DIRECT3D){
		scrnmngD3D_setheight(posy, height);
		np2wab_forceupdate();
	}else
#endif
	{
		scrnmngDD_setheight(posy, height);
		np2wab_forceupdate();
	}
}

void scrnmng_setsize(int posx, int posy, int width, int height) {
	
	if(scrnmng_UIthreadID != GetCurrentThreadId()){
		// 別のスレッドからのアクセスの場合、遅延変更
		scrnmng_cs_EnterModeChangeCriticalSection();
		scrnmng_changemode_posx = posx;
		scrnmng_changemode_posy = posy;
		scrnmng_changemode_width = width;
		scrnmng_changemode_height = height;
		scrnmng_changemode_pending = true;
		scrnmng_cs_LeaveModeChangeCriticalSection();
		return;
	}
	if(np2oscfg.fsrescfg){
		scrnres_readini_res(width, height);
		scrnstat.multiple = scrnrescfg.scrn_mul;
	}

#ifdef SUPPORT_SCRN_DIRECT3D
	if(scrnmng_current_drawtype==DRAWTYPE_INVALID) {return;}
	if(scrnmng_current_drawtype==DRAWTYPE_DIRECT3D){
		scrnmngD3D_setsize(posx, posy, width, height);
		np2wab_forceupdate();
	}else
#endif
	{
		scrnmngDD_setsize(posx, posy, width, height);
		np2wab_forceupdate();
	}
}

const SCRNSURF *scrnmng_surflock(void) {
	if (scrnmng_changemode_pending) return NULL; // 作成待ちならロック失敗とする
	
#ifdef SUPPORT_SCRN_DIRECT3D
	if(scrnmng_current_drawtype==DRAWTYPE_INVALID) {return NULL;}
	if(scrnmng_current_drawtype==DRAWTYPE_DIRECT3D){
		return scrnmngD3D_surflock();
	}else
#endif
	{
		return scrnmngDD_surflock();
	}
}

void scrnmng_surfunlock(const SCRNSURF *surf) {
	
#ifdef SUPPORT_SCRN_DIRECT3D
	if(scrnmng_current_drawtype==DRAWTYPE_INVALID) {return;}
	if(scrnmng_current_drawtype==DRAWTYPE_DIRECT3D){
		scrnmngD3D_surfunlock(surf);
	}else
#endif
	{
		scrnmngDD_surfunlock(surf);
	}
}

void scrnmng_update(void) {
	
#ifdef SUPPORT_SCRN_DIRECT3D
	if(scrnmng_current_drawtype==DRAWTYPE_INVALID) {return;}
	if(scrnmng_current_drawtype==DRAWTYPE_DIRECT3D){
		scrnmngD3D_update();
	}else
#endif
	{
		scrnmngDD_update();
	}
}

// ----

void scrnmng_setmultiple(int multiple)
{
	if(scrnmng_UIthreadID != GetCurrentThreadId()){
		// 別のスレッドからのアクセスの場合、遅延変更
		scrnmng_cs_EnterModeChangeCriticalSection();
		scrnmng_changemode_multiple = multiple;
		scrnmng_changemode_pending = true;
		scrnmng_cs_LeaveModeChangeCriticalSection();
		return;
	}
	if(multiple < 1) multiple = 8;
	if(np2oscfg.fsrescfg && scrnrescfg.scrn_mul!=multiple){
		scrnrescfg.hasfscfg = 1;
		scrnrescfg.fscrnmod = np2oscfg.fscrnmod;
		scrnrescfg.scrn_mul = multiple;
#ifdef SUPPORT_SCRN_DIRECT3D
		scrnrescfg.d3d_imode = np2oscfg.d3d_imode;
#endif
		scrnres_writeini();
	}

#ifdef SUPPORT_SCRN_DIRECT3D
	if(scrnmng_current_drawtype==DRAWTYPE_INVALID) {return;}
	if(scrnmng_current_drawtype==DRAWTYPE_DIRECT3D){
		scrnmngD3D_setmultiple(multiple);
		np2wab_forceupdate();
	}else
#endif
	{
		scrnmngDD_setmultiple(multiple);
		np2wab_forceupdate();
	}
}

int scrnmng_getmultiple(void)
{
#ifdef SUPPORT_SCRN_DIRECT3D
	if(scrnmng_current_drawtype==DRAWTYPE_INVALID) {return 0;}
	if(scrnmng_current_drawtype==DRAWTYPE_DIRECT3D){
		int result = scrnmngD3D_getmultiple();
		return result;
	}else
#endif
	{
		int result = scrnmngDD_getmultiple();
		return result;
	}
}


// ----

#if defined(SUPPORT_DCLOCK)
BOOL scrnmng_isdispclockclick(const POINT *pt) {
	
	if(scrnmng_UIthreadID != GetCurrentThreadId()) return FALSE; // 別のスレッドからのアクセスは不可

#ifdef SUPPORT_SCRN_DIRECT3D
	if(scrnmng_current_drawtype==DRAWTYPE_INVALID) {return FALSE;}
	if(scrnmng_current_drawtype==DRAWTYPE_DIRECT3D){
		return scrnmngD3D_isdispclockclick(pt);
	}else
#endif
	{
		return scrnmngDD_isdispclockclick(pt);
	}
}

void scrnmng_dispclock(void)
{
	if(scrnmng_UIthreadID != GetCurrentThreadId()) return; // 別のスレッドからのアクセスは不可

#ifdef SUPPORT_SCRN_DIRECT3D
	if(scrnmng_current_drawtype==DRAWTYPE_INVALID) {return;}
	if(scrnmng_current_drawtype==DRAWTYPE_DIRECT3D){
		scrnmngD3D_dispclock();
	}else
#endif
	{
		scrnmngDD_dispclock();
	}
}
#endif


// ----

void scrnmng_entersizing(void) {
	
	if(scrnmng_UIthreadID != GetCurrentThreadId()) return; // 別のスレッドからのアクセスは不可

#ifdef SUPPORT_SCRN_DIRECT3D
	if(scrnmng_current_drawtype==DRAWTYPE_DIRECT3D){
		scrnmngD3D_entersizing();
		np2wab_forceupdate();
	}else
#endif
	{
		scrnmngDD_entersizing();
		np2wab_forceupdate();
	}
}

void scrnmng_sizing(UINT side, RECT *rect) {
	
	if(scrnmng_UIthreadID != GetCurrentThreadId()) return; // 別のスレッドからのアクセスは不可

#ifdef SUPPORT_SCRN_DIRECT3D
	if(scrnmng_current_drawtype==DRAWTYPE_DIRECT3D){
		scrnmngD3D_sizing(side, rect);
		np2wab_forceupdate();
	}else
#endif
	{
		scrnmngDD_sizing(side, rect);
		np2wab_forceupdate();
	}
}

void scrnmng_exitsizing(void)
{
	if(scrnmng_UIthreadID != GetCurrentThreadId()) return; // 別のスレッドからのアクセスは不可

#ifdef SUPPORT_SCRN_DIRECT3D
	if(scrnmng_current_drawtype==DRAWTYPE_DIRECT3D){
		scrnmngD3D_exitsizing();		// ugh
		np2wab_forceupdate();
	}else
#endif
	{
		scrnmngDD_exitsizing();		// ugh
		np2wab_forceupdate();
	}
}

// フルスクリーン解像度調整
void scrnmng_updatefsres(void) {
	if(scrnmng_UIthreadID != GetCurrentThreadId()){
		// 別のスレッドからのアクセスの場合、遅延変更
		scrnmng_cs_EnterModeChangeCriticalSection();
		scrnmng_changemode_updatefsres = true;
		scrnmng_changemode_pending = true;
		scrnmng_cs_LeaveModeChangeCriticalSection();
		return;
	}
#ifdef SUPPORT_SCRN_DIRECT3D
	if(scrnmng_current_drawtype==DRAWTYPE_DIRECT3D){
		scrnmngD3D_updatefsres();
		np2wab_forceupdate();
	}else
#endif
	{
		scrnmngDD_updatefsres();
		np2wab_forceupdate();
	}
}

// ウィンドウアクセラレータ画面転送
void scrnmng_blthdc(HDC hdc) {
	if (scrnmng_changemode_pending) return; // 作成待ちなら転送しない

#ifdef SUPPORT_SCRN_DIRECT3D
	if(scrnmng_current_drawtype==DRAWTYPE_DIRECT3D){
		scrnmngD3D_blthdc(hdc);
	}else
#endif
	{
		scrnmngDD_blthdc(hdc);
	}
}
void scrnmng_bltwab() {
#ifdef SUPPORT_SCRN_DIRECT3D
	if(scrnmng_current_drawtype==DRAWTYPE_DIRECT3D){
		scrnmngD3D_bltwab();
	}else
#endif
	{
		scrnmngDD_bltwab();
	}
}

void scrnmng_getrect(RECT *lpRect){
	if(scrnmng_UIthreadID != GetCurrentThreadId()) return; // 別のスレッドからのアクセスは不可

#ifdef SUPPORT_SCRN_DIRECT3D
	if(scrnmng_current_drawtype==DRAWTYPE_DIRECT3D){
		scrnmngD3D_getrect(lpRect);
	}else
#endif
	{
		scrnmngDD_getrect(lpRect);
	}
}

void scrnmng_delaychangemode(void){
	if(scrnmng_UIthreadID != GetCurrentThreadId()) return; // 別のスレッドからのアクセスは不可
	
	if(scrnmng_changemode_pending){
		DWORD oldThreadID;
		if(IsIconic(g_hWndMain)) return;
		scrnmng_cs_EnterModeChangeCriticalSection();
		oldThreadID = scrnmng_UIthreadID;
		scrnmng_UIthreadID = GetCurrentThreadId();
		if(scrnmng_changemode_posx != INT_MAX && scrnmng_changemode_width != INT_MAX && scrnmng_changemode_posy != INT_MAX && scrnmng_changemode_height != INT_MAX){
			scrnmng_setsize(scrnmng_changemode_posx, scrnmng_changemode_posy, scrnmng_changemode_width, scrnmng_changemode_height);
		}
		else if(scrnmng_changemode_posx != INT_MAX && scrnmng_changemode_width != INT_MAX){
			scrnmng_setwidth(scrnmng_changemode_posx, scrnmng_changemode_width);
		}
		else if(scrnmng_changemode_posy != INT_MAX && scrnmng_changemode_height != INT_MAX){
			scrnmng_setheight(scrnmng_changemode_posy, scrnmng_changemode_height);
		}
		if(scrnmng_changemode_extend != INT_MAX){
			scrnmng_setextend(scrnmng_changemode_extend);
		}
		if(scrnmng_changemode_multiple != INT_MAX){
			scrnmng_setmultiple(scrnmng_changemode_multiple);
		}
		if(scrnmng_changemode_updatefsres){
			scrnmng_updatefsres();
		}
		scrnmng_UIthreadID = oldThreadID;
		scrnmng_changemode_posx = INT_MAX;
		scrnmng_changemode_posy = INT_MAX;
		scrnmng_changemode_width = INT_MAX;
		scrnmng_changemode_height = INT_MAX;
		scrnmng_changemode_extend = INT_MAX;
		scrnmng_changemode_multiple = INT_MAX;
		scrnmng_changemode_updatefsres = false;
		scrnmng_changemode_pending = false;
		scrnmng_cs_LeaveModeChangeCriticalSection();

		scrndraw_redraw();
		np2wab_forceupdate();
		InvalidateRect(g_hWndMain, NULL, TRUE);		// ugh
	}
}

void scrnmng_UIThreadProc(void)
{
	if (scrnmng_restore_pending && !IsIconic(g_hWndMain))
	{
		scrnmng_cs_EnterModeChangeCriticalSection();
		scrnmng_restore_pending = false;
#ifdef SUPPORT_SCRN_DIRECT3D
		if (scrnmng_current_drawtype != DRAWTYPE_INVALID)
		{
			if (scrnmng_current_drawtype == DRAWTYPE_DIRECT3D)
			{
				scrnmngD3D_restoresurfaces();
			}
			else
			{
				scrnmngDD_restoresurfaces();
			}
		}
#else
		scrnmngDD_restoresurfaces();
#endif
		scrnmng_cs_LeaveModeChangeCriticalSection();

	}
	if (scrnmng_create_pending && !IsIconic(g_hWndMain))
	{
		if (IsIconic(g_hWndMain)) return;
		scrnmng_cs_EnterModeChangeCriticalSection();
		scrnmng_destroy();
		scrnmng_create(g_scrnmode);
		scrnmng_create_pending = false;
		scrnmng_cs_LeaveModeChangeCriticalSection();

		scrndraw_redraw();
		np2wab_forceupdate();
		InvalidateRect(g_hWndMain, NULL, TRUE);		// ugh
	}
}