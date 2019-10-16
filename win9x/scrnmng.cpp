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

#include <shlwapi.h>

SCRNMNG		scrnmng;
SCRNSTAT	scrnstat;
SCRNRESCFG	scrnrescfg = {0};

int d3davailable = 0;

// ----

UINT8 scrnmng_current_drawtype = DRAWTYPE_INVALID;

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
		GetWindowRect(hWnd, &rectwindow);
		RECT rectclient;
		GetClientRect(hWnd, &rectclient);
		int winx = (np2oscfg.winx != CW_USEDEFAULT) ? np2oscfg.winx : rectwindow.left;
		int winy = (np2oscfg.winy != CW_USEDEFAULT) ? np2oscfg.winy : rectwindow.top;
		int cx = width;
		cx += np2oscfg.paddingx * 2;
		cx += rectwindow.right - rectwindow.left;
		cx -= rectclient.right - rectclient.left;
		int cy = height;
		cy += np2oscfg.paddingy * 2;
		cy += rectwindow.bottom - rectwindow.top;
		cy -= rectclient.bottom - rectclient.top;

		if (scx < cx)
		{
			winx = (scx - cx) / 2;
		}
		else
		{
			if ((winx + cx) > workrc.right)
			{
				winx = workrc.right - cx;
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
	
	scrnstat.width = 640;
	scrnstat.height = 400;
	scrnstat.extend = 1;
	scrnstat.multiple = 8;
	scrnmng_setwindowsize(g_hWndMain, 640, 400);
	
	if(np2oscfg.fsrescfg) scrnres_readini_res(640, 400);
}

BRESULT scrnmng_create(UINT8 scrnmode) {
	
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
		return scrnmngD3D_create(scrnmode);
	}else
#else
	scrnmng_current_drawtype = DRAWTYPE_DIRECTDRAW_HW;
#endif
	{
		return scrnmngDD_create(scrnmode);
	}
}

void scrnmng_destroy(void) {
	
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
}

void scrnmng_querypalette(void) {
	
#ifdef SUPPORT_SCRN_DIRECT3D
	if(scrnmng_current_drawtype==DRAWTYPE_INVALID) return;
	if(scrnmng_current_drawtype==DRAWTYPE_DIRECT3D){
		scrnmngD3D_querypalette();
	}else
#endif
	{
		scrnmngDD_querypalette();
	}
}

RGB16 scrnmng_makepal16(RGB32 pal32) {
	
#ifdef SUPPORT_SCRN_DIRECT3D
	if(scrnmng_current_drawtype==DRAWTYPE_INVALID) return 0;
	if(scrnmng_current_drawtype==DRAWTYPE_DIRECT3D){
		return scrnmngD3D_makepal16(pal32);
	}else
#endif
	{
		return scrnmngDD_makepal16(pal32);
	}
}

void scrnmng_fullscrnmenu(int y) {
	
#ifdef SUPPORT_SCRN_DIRECT3D
	if(scrnmng_current_drawtype==DRAWTYPE_INVALID) return;
	if(scrnmng_current_drawtype==DRAWTYPE_DIRECT3D){
		scrnmngD3D_fullscrnmenu(y);
	}else
#endif
	{
		scrnmngDD_fullscrnmenu(y);
	}
}

void scrnmng_topwinui(void) {
	
#ifdef SUPPORT_SCRN_DIRECT3D
	if(scrnmng_current_drawtype==DRAWTYPE_INVALID) return;
	if(scrnmng_current_drawtype==DRAWTYPE_DIRECT3D){
		scrnmngD3D_topwinui();
	}else
#endif
	{
		scrnmngDD_topwinui();
	}
}

void scrnmng_clearwinui(void) {
	
#ifdef SUPPORT_SCRN_DIRECT3D
	if(scrnmng_current_drawtype==DRAWTYPE_INVALID) return;
	if(scrnmng_current_drawtype==DRAWTYPE_DIRECT3D){
		scrnmngD3D_clearwinui();
	}else
#endif
	{
		scrnmngDD_clearwinui();
	}
}

void scrnmng_setwidth(int posx, int width) {
	
	if(np2oscfg.fsrescfg){
		scrnres_readini_res(width, scrnstat.height);
		scrnstat.multiple = scrnrescfg.scrn_mul;
	}

#ifdef SUPPORT_SCRN_DIRECT3D
	if(scrnmng_current_drawtype==DRAWTYPE_INVALID) return;
	if(scrnmng_current_drawtype==DRAWTYPE_DIRECT3D){
		scrnmngD3D_setwidth(posx, width);
	}else
#endif
	{
		scrnmngDD_setwidth(posx, width);
	}
}

void scrnmng_setextend(int extend) {
	
#ifdef SUPPORT_SCRN_DIRECT3D
	if(scrnmng_current_drawtype==DRAWTYPE_INVALID) return;
	if(scrnmng_current_drawtype==DRAWTYPE_DIRECT3D){
		scrnmngD3D_setextend(extend);
	}else
#endif
	{
		scrnmngDD_setextend(extend);
	}
}

void scrnmng_setheight(int posy, int height) {
	
	if(np2oscfg.fsrescfg){
		scrnres_readini_res(scrnstat.width, height);
		scrnstat.multiple = scrnrescfg.scrn_mul;
	}

#ifdef SUPPORT_SCRN_DIRECT3D
	if(scrnmng_current_drawtype==DRAWTYPE_INVALID) return;
	if(scrnmng_current_drawtype==DRAWTYPE_DIRECT3D){
		scrnmngD3D_setheight(posy, height);
	}else
#endif
	{
		scrnmngDD_setheight(posy, height);
	}
}

void scrnmng_setsize(int posx, int posy, int width, int height) {
	
	if(np2oscfg.fsrescfg){
		scrnres_readini_res(width, height);
		scrnstat.multiple = scrnrescfg.scrn_mul;
	}

#ifdef SUPPORT_SCRN_DIRECT3D
	if(scrnmng_current_drawtype==DRAWTYPE_INVALID) return;
	if(scrnmng_current_drawtype==DRAWTYPE_DIRECT3D){
		scrnmngD3D_setsize(posx, posy, width, height);
	}else
#endif
	{
		scrnmngDD_setsize(posx, posy, width, height);
	}
}

const SCRNSURF *scrnmng_surflock(void) {
	
#ifdef SUPPORT_SCRN_DIRECT3D
	if(scrnmng_current_drawtype==DRAWTYPE_INVALID) return NULL;
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
	if(scrnmng_current_drawtype==DRAWTYPE_INVALID) return;
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
	if(scrnmng_current_drawtype==DRAWTYPE_INVALID) return;
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
	if(scrnmng_current_drawtype==DRAWTYPE_INVALID) return;
	if(scrnmng_current_drawtype==DRAWTYPE_DIRECT3D){
		scrnmngD3D_setmultiple(multiple);
	}else
#endif
	{
		scrnmngDD_setmultiple(multiple);
	}
}

int scrnmng_getmultiple(void)
{
#ifdef SUPPORT_SCRN_DIRECT3D
	if(scrnmng_current_drawtype==DRAWTYPE_INVALID) return 0;
	if(scrnmng_current_drawtype==DRAWTYPE_DIRECT3D){
		return scrnmngD3D_getmultiple();
	}else
#endif
	{
		return scrnmngDD_getmultiple();
	}
}



// ----

#if defined(SUPPORT_DCLOCK)
BOOL scrnmng_isdispclockclick(const POINT *pt) {
	
#ifdef SUPPORT_SCRN_DIRECT3D
	if(scrnmng_current_drawtype==DRAWTYPE_INVALID) return FALSE;
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
#ifdef SUPPORT_SCRN_DIRECT3D
	if(scrnmng_current_drawtype==DRAWTYPE_INVALID) return;
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
	
#ifdef SUPPORT_SCRN_DIRECT3D
	if(scrnmng_current_drawtype==DRAWTYPE_DIRECT3D){
		scrnmngD3D_entersizing();
	}else
#endif
	{
		scrnmngDD_entersizing();
	}
}

void scrnmng_sizing(UINT side, RECT *rect) {
	
#ifdef SUPPORT_SCRN_DIRECT3D
	if(scrnmng_current_drawtype==DRAWTYPE_DIRECT3D){
		scrnmngD3D_sizing(side, rect);
	}else
#endif
	{
		scrnmngDD_sizing(side, rect);
	}
}

void scrnmng_exitsizing(void)
{
#ifdef SUPPORT_SCRN_DIRECT3D
	if(scrnmng_current_drawtype==DRAWTYPE_DIRECT3D){
		scrnmngD3D_exitsizing();		// ugh
	}else
#endif
	{
		scrnmngDD_exitsizing();		// ugh
	}
}

// フルスクリーン解像度調整
void scrnmng_updatefsres(void) {
#ifdef SUPPORT_SCRN_DIRECT3D
	if(scrnmng_current_drawtype==DRAWTYPE_DIRECT3D){
		scrnmngD3D_updatefsres();
	}else
#endif
	{
		scrnmngDD_updatefsres();
	}
}

// ウィンドウアクセラレータ画面転送
void scrnmng_blthdc(HDC hdc) {
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
#ifdef SUPPORT_SCRN_DIRECT3D
	if(scrnmng_current_drawtype==DRAWTYPE_DIRECT3D){
		scrnmngD3D_getrect(lpRect);
	}else
#endif
	{
		scrnmngDD_getrect(lpRect);
	}
}
