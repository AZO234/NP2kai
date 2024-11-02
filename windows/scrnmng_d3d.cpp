/**
 * @file	scrnmng_d3d.cpp
 * @brief	Screen Manager (Direct3D9)
 *
 * @author	$Author: SimK$
 */

#include "compiler.h"

#ifdef SUPPORT_SCRN_DIRECT3D

#include <d3d9.h>
#ifndef __GNUC__
#include <winnls32.h>
#endif
#include "resource.h"
#include "np2.h"
#include "np2mt.h"
#include "winloc.h"
#include "mousemng.h"
#include "scrnmng.h"
#include "scrnmng_d3d.h"
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
#pragma comment( lib, "d3d9.lib" )
#endif	// !defined(__GNUC__)

//! 8BPP パレット数
#define PALLETES_8BPP	NP2PAL_TEXT3

#define CREATEDEVICE_RETRY_MAX	3

static int nvidia_fixflag = 0;

static int devicelostflag = 0;
static int req_enter_criticalsection = 0;

extern bool scrnmng_create_pending; // グラフィックレンダラ生成保留中
extern bool scrnmng_restore_pending;

extern WINLOCEX np2_winlocexallwin(HWND base);

typedef struct {
	LPDIRECT3D9			d3d;
	D3DPRESENT_PARAMETERS d3dparam;
	LPDIRECT3DDEVICE9	d3ddev;
	LPDIRECT3DSURFACE9	d3dbacksurf;
	LPDIRECT3DSURFACE9	backsurf;
	LPDIRECT3DSURFACE9	backsurf2;
#if defined(SUPPORT_DCLOCK)
	LPDIRECT3DSURFACE9	clocksurf;
#endif
#if defined(SUPPORT_WAB)
	LPDIRECT3DSURFACE9	wabsurf;
#endif
	UINT				scrnmode;
	int					width;
	int					height;
	int					extend;
	int					cliping;
	UINT8				menudisp;
	int					menusize;
	RECT				scrn;
	RECT				rect;
	RECT				scrnclip;
	RECT				rectclip;
	int					backsurf2width;
	int					backsurf2height;
	int					backsurf2mul;
} D3D;

static	D3D			d3d = { 0 };
static	SCRNSURF	scrnsurf = { 0 };

#ifdef SUPPORT_WAB
static	int mt_wabdrawing = 0;
static	int mt_wabpausedrawing = 0;
static	int mt_d3dwabpausedrawing = 0;
#endif

UINT8	current_d3d_imode = 0;

static int d3d_cs_initialized = 0;
static CRITICAL_SECTION d3d_cs;

static BOOL d3d_tryenter_criticalsection(void){
	if(!d3d_cs_initialized) return TRUE;
	return TryEnterCriticalSection(&d3d_cs);
}
static void d3d_enter_criticalsection(void){
	if(!d3d_cs_initialized) return;
	EnterCriticalSection(&d3d_cs);
}
static void d3d_leave_criticalsection(void){
	if(!d3d_cs_initialized) return;
	LeaveCriticalSection(&d3d_cs);
}

static void getscreensize(int *screenwidth, int *screenheight, UINT scrnmode){
	UINT		fscrnmod;
	int			width;
	int			height;
	int			scrnwidth;
	int			scrnheight;
	int			multiple;
	
	if (scrnmode & SCRNMODE_FULLSCREEN) {
		width = min(scrnstat.width, d3d.width);
		height = min(scrnstat.height, d3d.height);

		scrnwidth = width;
		scrnheight = height;
		fscrnmod = FSCRNCFG_fscrnmod & FSCRNMOD_ASPECTMASK;
		switch(fscrnmod) {
			default:
			case FSCRNMOD_NORESIZE:
				break;

			case FSCRNMOD_ASPECTFIX8:
				scrnwidth = (d3d.width << 3) / width;
				scrnheight = (d3d.height << 3) / height;
				multiple = min(scrnwidth, scrnheight);
				scrnwidth = (width * multiple) >> 3;
				scrnheight = (height * multiple) >> 3;
				break;

			case FSCRNMOD_ASPECTFIX:
				scrnwidth = d3d.width;
				scrnheight = (scrnwidth * height) / width;
				if (scrnheight >= d3d.height) {
					scrnheight = d3d.height;
					scrnwidth = (scrnheight * width) / height;
				}
				break;
				
			case FSCRNMOD_INTMULTIPLE:
				scrnwidth = (d3d.width / width) * width;
				scrnheight = (scrnwidth * height) / width;
				if (scrnheight >= d3d.height) {
					scrnheight = (d3d.height / height) * height;
					scrnwidth = (scrnheight * width) / height;
				}
				break;
				
			case FSCRNMOD_FORCE43:
				if(d3d.width*3 > d3d.height*4){
					scrnwidth = d3d.height*4/3;
					scrnheight = d3d.height;
				}else{
					scrnwidth = d3d.width;
					scrnheight = d3d.width*3/4;
				}
				break;

			case FSCRNMOD_LARGE:
				scrnwidth = d3d.width;
				scrnheight = d3d.height;
				break;
		}
	}else{
		width = min(scrnstat.width, d3d.width);
		height = min(scrnstat.height, d3d.height);

		multiple = scrnstat.multiple;
		fscrnmod = FSCRNCFG_fscrnmod & FSCRNMOD_ASPECTMASK;
		if (!(scrnmode & SCRNMODE_ROTATE)) {
			scrnwidth = (width * multiple) >> 3;
			scrnheight = (height * multiple) >> 3;
			if(fscrnmod==FSCRNMOD_FORCE43) { // Force 4:3 Screen
				if(((width * multiple) >> 3)*3 < ((height * multiple) >> 3)*4){
					scrnwidth = ((height * multiple) >> 3)*4/3;
					scrnheight = ((height * multiple) >> 3);
				}else{
					scrnwidth = ((width * multiple) >> 3);
					scrnheight = ((width * multiple) >> 3)*3/4;
				}
			}
		}
		else {
			scrnwidth = (height * multiple) >> 3;
			scrnheight = (width * multiple) >> 3;
			if(fscrnmod==FSCRNMOD_FORCE43) { // Force 4:3 Screen
				if(((width * multiple) >> 3)*4 < ((height * multiple) >> 3)*3){
					scrnwidth = ((height * multiple) >> 3)*3/4;
					scrnheight = ((height * multiple) >> 3);
				}else{
					scrnwidth = ((width * multiple) >> 3);
					scrnheight = ((width * multiple) >> 3)*4/3;
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
	int			multiple;
	int			scrnwidth;
	int			scrnheight;
	int			tmpcy;
	WINLOCEX	wlex;

	width = min(scrnstat.width, d3d.width);
	height = min(scrnstat.height, d3d.height);

	extend = 0;

	// 描画範囲～
	if (d3d.scrnmode & SCRNMODE_FULLSCREEN) {
		d3d.rect.right = width;
		d3d.rect.bottom = height;
		getscreensize(&scrnwidth, &scrnheight, d3d.scrnmode);
		if ((np2oscfg.paddingx)/* && (multiple == 8)*/) {
			extend = min(scrnstat.extend, d3d.extend);
		}
		fscrnmod = FSCRNCFG_fscrnmod & FSCRNMOD_ASPECTMASK;
		if(fscrnmod==FSCRNMOD_ASPECTFIX8) {
			multiple = min(width, height);
		}
		d3d.scrn.left = (d3d.width - scrnwidth) / 2;
		d3d.scrn.top = (d3d.height - scrnheight) / 2;
		d3d.scrn.right = d3d.scrn.left + scrnwidth;
		d3d.scrn.bottom = d3d.scrn.top + scrnheight;

		// メニュー表示時の描画領域
		d3d.rectclip = d3d.rect;
		d3d.scrnclip = d3d.scrn;
		if (d3d.scrnclip.top < d3d.menusize) {
			d3d.scrnclip.top = d3d.menusize;
			tmpcy = d3d.height - d3d.menusize;
			if (scrnheight > tmpcy) {
				switch(fscrnmod) {
					default:
					case FSCRNMOD_NORESIZE:
						tmpcy = min(tmpcy, height);
						d3d.rectclip.bottom = tmpcy;
						break;

					case FSCRNMOD_ASPECTFIX8:
					case FSCRNMOD_ASPECTFIX:
					case FSCRNMOD_INTMULTIPLE:
					case FSCRNMOD_FORCE43:
						d3d.rectclip.bottom = (tmpcy * height) / scrnheight;
						break;
						
					case FSCRNMOD_LARGE:
						break;
				}
			}
			d3d.scrnclip.bottom = d3d.menusize + tmpcy;
		}
		// Direct3Dはメニュー除外済みサイズが基準のため修正
		d3d.scrnclip.top -= d3d.menusize;
		d3d.scrnclip.bottom -= d3d.menusize;
		d3d.scrnclip.top += d3d.menusize * d3d.scrnclip.top / d3d.height;
		d3d.scrnclip.bottom += d3d.menusize * d3d.scrnclip.bottom / d3d.height;
		if (d3d.scrnclip.top < 0) {
			d3d.rectclip.top += -d3d.scrnclip.top;
			d3d.scrnclip.top = 0;
		}
	}
	else {
		fscrnmod = FSCRNCFG_fscrnmod & FSCRNMOD_ASPECTMASK;
		multiple = scrnstat.multiple;
		getscreensize(&scrnwidth, &scrnheight, d3d.scrnmode);
		if (!(d3d.scrnmode & SCRNMODE_ROTATE)) {
			if ((np2oscfg.paddingx)/* && (multiple == 8)*/) {
				extend = min(scrnstat.extend, d3d.extend);
			}
			d3d.rect.right = width + extend;
			d3d.rect.left = (multiple != 8 ? extend : 0);
			d3d.rect.bottom = height;
			d3d.scrn.left = np2oscfg.paddingx - (multiple == 8 ? extend : 0);
			d3d.scrn.top = np2oscfg.paddingy;
		}
		else {
			if ((np2oscfg.paddingy)/* && (multiple == 8)*/) {
				extend = min(scrnstat.extend, d3d.extend);
			}
			d3d.rect.right = height;
			d3d.rect.bottom = width + (multiple == 8 ? extend : 0);
			d3d.scrn.left = np2oscfg.paddingx;
			d3d.scrn.top = np2oscfg.paddingy - (multiple == 8 ? extend : 0);
		}
		d3d.scrn.right = np2oscfg.paddingx + scrnwidth;
		d3d.scrn.bottom = np2oscfg.paddingy + scrnheight;

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

	LPDIRECT3DDEVICE9	dev;
	RECT				rect;
	D3DRECT				rectd3d[4];
	int					rectd3dc = 0;

	dev = d3d.d3ddev;
	if (dev == NULL) {
		return;
	}
	
	d3d_enter_criticalsection();
	rect.left = base->left;
	rect.right = base->right;
	rect.top = base->top;
	rect.bottom = target->top;
	if (rect.top < rect.bottom) {
		rectd3d[rectd3dc].x1 = rect.left;
		rectd3d[rectd3dc].x2 = rect.right;
		rectd3d[rectd3dc].y1 = rect.top;
		rectd3d[rectd3dc].y2 = rect.bottom;
		d3d.d3ddev->ColorFill(d3d.d3dbacksurf, &rect, D3DCOLOR_XRGB(0, 0, 0));
		rectd3dc++;
	}
	rect.top = target->bottom;
	rect.bottom = base->bottom;
	if (rect.top < rect.bottom) {
		rectd3d[rectd3dc].x1 = rect.left;
		rectd3d[rectd3dc].x2 = rect.right;
		rectd3d[rectd3dc].y1 = rect.top;
		rectd3d[rectd3dc].y2 = rect.bottom;
		d3d.d3ddev->ColorFill(d3d.d3dbacksurf, &rect, D3DCOLOR_XRGB(0, 0, 0));
		rectd3dc++;
	}

	rect.top = max(base->top, target->top);
	rect.bottom = min(base->bottom, target->bottom);
	if (rect.top < rect.bottom) {
		rect.left = base->left;
		rect.right = target->left;
		if (rect.left < rect.right) {
			rectd3d[rectd3dc].x1 = rect.left;
			rectd3d[rectd3dc].x2 = rect.right;
			rectd3d[rectd3dc].y1 = rect.top;
			rectd3d[rectd3dc].y2 = rect.bottom;
			d3d.d3ddev->ColorFill(d3d.d3dbacksurf, &rect, D3DCOLOR_XRGB(0, 0, 0));
			rectd3dc++;
		}
		rect.left = target->right;
		rect.right = base->right;
		if (rect.left < rect.right) {
			rectd3d[rectd3dc].x1 = rect.left;
			rectd3d[rectd3dc].x2 = rect.right;
			rectd3d[rectd3dc].y1 = rect.top;
			rectd3d[rectd3dc].y2 = rect.bottom;
			d3d.d3ddev->ColorFill(d3d.d3dbacksurf, &rect, D3DCOLOR_XRGB(0, 0, 0));
			rectd3dc++;
		}
	}
	if(rectd3dc){
		dev->Clear(rectd3dc, rectd3d, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 0.0f, 0);
		dev->Present(NULL, NULL, NULL, NULL);
		dev->Clear(rectd3dc, rectd3d, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 0.0f, 0);
		dev->Present(NULL, NULL, NULL, NULL);
	}
	d3d_leave_criticalsection();
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
	target.left = base.left + d3d.scrn.left;
	target.top = base.top + d3d.scrn.top;
	target.right = base.left + d3d.scrn.right;
	target.bottom = base.top + d3d.scrn.bottom;
	clearoutofrect(&target, &base);
}

static void clearoutfullscreen(void) {

	RECT	base;
const RECT	*scrn;

	base.left = 0;
	base.top = 0;
	base.right = d3d.width;
	base.bottom = d3d.height;
	if (GetWindowLongPtr(g_hWndMain, NP2GWLP_HMENU)) {
		scrn = &d3d.scrn;
		base.top = 0;
	}
	else {
		scrn = &d3d.scrnclip;
		base.top = d3d.menusize;
	}
	clearoutofrect(scrn, &base);
#if defined(SUPPORT_DCLOCK)
	DispClock::GetInstance()->Redraw();
#endif
}

static void paletteinit()
{
}

static void paletteset()
{
}

static void make16mask(DWORD bmask, DWORD rmask, DWORD gmask)
{
}

static void update_backbuffer2size(){
	if(current_d3d_imode == D3D_IMODE_PIXEL || current_d3d_imode == D3D_IMODE_PIXEL2 || current_d3d_imode == D3D_IMODE_PIXEL3){
		UINT backbufwidth = d3d.d3dparam.BackBufferWidth - scrnstat.extend * 2;
		UINT backbufheight = d3d.d3dparam.BackBufferHeight;
		d3d.backsurf2width = max(scrnstat.width, 320);
		d3d.backsurf2height = max(scrnstat.height, 200);
		d3d.backsurf2mul = 1;
		switch(current_d3d_imode){
		case D3D_IMODE_PIXEL3:
			backbufwidth += backbufwidth;
			backbufheight += backbufheight;
			break;
		case D3D_IMODE_PIXEL2:
			backbufwidth += backbufwidth / 3;
			backbufheight += backbufheight / 3;
			break;
		}
		if(backbufheight < 480) backbufheight = UINT_MAX;
		while(d3d.backsurf2width * 2 < (int)backbufwidth && d3d.backsurf2height * 2 < (int)backbufheight){
			d3d.backsurf2width *= 2;
			d3d.backsurf2height *= 2;
			d3d.backsurf2mul++;
		}
		if(d3d.backsurf2height < 480){
			d3d.backsurf2height = 480;
		}
		d3d_enter_criticalsection();
		if (d3d.backsurf2) {
			d3d.backsurf2->Release();
			d3d.backsurf2 = NULL;
		}
		if(d3d.backsurf2width/2 == d3d.d3dparam.BackBufferWidth || d3d.backsurf2width/2+2 == d3d.d3dparam.BackBufferWidth || d3d.d3ddev->CreateRenderTarget(d3d.backsurf2width + 2, d3d.backsurf2height + 2, d3d.d3dparam.BackBufferFormat, D3DMULTISAMPLE_NONE, 0, FALSE, &d3d.backsurf2, NULL) != D3D_OK){
			d3d.backsurf2width /= 2;
			d3d.backsurf2height /= 2;
			d3d.backsurf2mul--;
			if(d3d.backsurf2mul <= 0 ||  d3d.d3ddev->CreateRenderTarget(d3d.backsurf2width + 2, d3d.backsurf2height + 2, d3d.d3dparam.BackBufferFormat, D3DMULTISAMPLE_NONE, 0, FALSE, &d3d.backsurf2, NULL) != D3D_OK){
				d3d.backsurf2 = NULL;
			}
		}
		d3d_leave_criticalsection();
		d3d.backsurf2width += 2;
		d3d.backsurf2height += 2;
	}
}

void scrnmngD3D_restoresurfaces() {
	HRESULT r;

	d3d_enter_criticalsection();
	r = d3d.d3ddev->TestCooperativeLevel();
	if (r == D3DERR_DRIVERINTERNALERROR)
	{
		devicelostflag = 2;
	}
	if(d3d.d3ddev==NULL || r==D3DERR_DEVICENOTRESET || r==D3DERR_DRIVERINTERNALERROR){
		static UINT  bufwidth, bufheight;
		static UINT  wabwidth, wabheight;
		D3DSURFACE_DESC d3dsdesc;

		if(devicelostflag==2 || devicelostflag==3 || d3d.d3ddev==NULL){
			// 強制再作成
			if(devicelostflag==2){
				devicelostflag = 0;
				scrnmngD3D_destroy();
			}else{
				devicelostflag = 0;
			}
			if(scrnmngD3D_create(g_scrnmode) == SUCCESS){
				devicelostflag = 0;
				mt_wabpausedrawing = 0;
				d3d_leave_criticalsection();
				scrnmngD3D_updatefsres();
				scrndraw_updateallline();
			}else{
				devicelostflag = 3;
			}
			return;
		}

#if defined(SUPPORT_DCLOCK)
		if (d3d.clocksurf) {
			d3d.clocksurf->Release();
			d3d.clocksurf = NULL;
		}
#endif
#if defined(SUPPORT_WAB)
		if (d3d.wabsurf) {
			mt_wabpausedrawing = 1; // MultiThread対策
			while(mt_wabdrawing) 
				Sleep(10);
			d3d.wabsurf->GetDesc(&d3dsdesc);
			wabwidth = d3dsdesc.Width;
			wabheight = d3dsdesc.Height;
			d3d.wabsurf->Release();
			d3d.wabsurf = NULL;
		}
#endif
		if (d3d.backsurf2) {
			d3d.backsurf2->Release();
			d3d.backsurf2 = NULL;
		}
		if (d3d.backsurf) {
			d3d.backsurf->GetDesc(&d3dsdesc);
			bufwidth = d3dsdesc.Width;
			bufheight = d3dsdesc.Height;
			d3d.backsurf->Release();
			d3d.backsurf = NULL;
		}
		if (d3d.d3dbacksurf) {
			d3d.d3dbacksurf->Release();
			d3d.d3dbacksurf = NULL;
		}
		r = d3d.d3ddev->Reset(&d3d.d3dparam);
		if(r==D3D_OK){
			//d3d.d3ddev->Clear(0, NULL, D3DCLEAR_TARGET, 0xffff00ff, 0, 0);  
			if (g_scrnmode & SCRNMODE_FULLSCREEN) {
				d3d.d3ddev->CreateOffscreenPlainSurface(bufwidth, bufheight, d3d.d3dparam.BackBufferFormat, D3DPOOL_DEFAULT, &d3d.backsurf, NULL);
				//d3d.d3ddev->CreateRenderTarget(bufwidth, bufheight, d3d.d3dparam.BackBufferFormat, D3DMULTISAMPLE_NONE, 0, FALSE, &d3d.backsurf, NULL);
#ifdef SUPPORT_WAB
				d3d.d3ddev->CreateOffscreenPlainSurface(wabwidth, wabheight, d3d.d3dparam.BackBufferFormat, D3DPOOL_DEFAULT, &d3d.wabsurf, NULL);
#endif

#if defined(SUPPORT_DCLOCK)
				d3d.d3ddev->CreateOffscreenPlainSurface(DCLOCK_WIDTH, DCLOCK_HEIGHT, d3d.d3dparam.BackBufferFormat, D3DPOOL_DEFAULT, &d3d.clocksurf, NULL);
#endif
			}else{
				d3d.d3ddev->CreateOffscreenPlainSurface(bufwidth, bufheight, d3d.d3dparam.BackBufferFormat, D3DPOOL_DEFAULT, &d3d.backsurf, NULL);
				//d3d.d3ddev->CreateRenderTarget(bufwidth, bufheight, d3d.d3dparam.BackBufferFormat, D3DMULTISAMPLE_NONE, 0, FALSE, &d3d.backsurf, NULL);
#ifdef SUPPORT_WAB
				d3d.d3ddev->CreateOffscreenPlainSurface(wabwidth, wabheight, d3d.d3dparam.BackBufferFormat, D3DPOOL_DEFAULT, &d3d.wabsurf, NULL);
#endif
			}
			update_backbuffer2size();
			d3d.d3ddev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &d3d.d3dbacksurf);
			//scrnmngD3D_destroy();
			//scrnmngD3D_create(g_scrnmode);
			devicelostflag = 0;
			mt_wabpausedrawing = 0;
			d3d_leave_criticalsection();
			scrnmngD3D_updatefsres();
			scrndraw_updateallline();
		}else{
			devicelostflag = 2;
			d3d_leave_criticalsection();
		}
	}else{
		if (r == S_OK)
		{
			devicelostflag = 0;
		}
		else
		{
			if (!devicelostflag) devicelostflag = 1;
		}
		d3d_leave_criticalsection();
	}
	if (devicelostflag)
	{
		scrnmng_restore_pending = true;
	}
}

// 解像度が使用可能かをチェック
static int checkResolution(int width, int height) {
	UINT count;
	UINT i;
	D3DDISPLAYMODE mode;

	count = d3d.d3d->GetAdapterModeCount(D3DADAPTER_DEFAULT, D3DFMT_X8R8G8B8);
	for(i=0;i<count;i++){
		d3d.d3d->EnumAdapterModes(D3DADAPTER_DEFAULT, D3DFMT_X8R8G8B8, i, &mode);
		if(mode.Width==width && mode.Height==height){
			return(SUCCESS);
		}
	}
	return(FAILURE);
}

typedef struct tagMONITORINFOENUMDATA{
	int found;
	int areasize;
    RECT np2windowrect;
    RECT monitorrect;
} MONITORINFOENUMDATA;

#define MAX(a, b)	((a>b) ? (a) :(b))
#define MIN(a, b)	((a>b) ? (b) :(a))

static BOOL CALLBACK monitorInfoEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData){
	MONITORINFOENUMDATA* mondata = (MONITORINFOENUMDATA*)dwData;
	int cx1, cx2, cy1, cy2;
	int w1, w2, h1, h2;
	int distx, disty;
	int sx1, sx2, ex1, ex2, sy1, sy2, ey1, ey2;
	sx1 = mondata->np2windowrect.left;
	ex1 = mondata->np2windowrect.right;
	sy1 = mondata->np2windowrect.top;
	ey1 = mondata->np2windowrect.bottom;
	sx2 = lprcMonitor->left;
	ex2 = lprcMonitor->right;
	sy2 = lprcMonitor->top;
	ey2 = lprcMonitor->bottom;
	cx1 = (sx1 + ex1)/2;
	cy1 = (sy1 + ey1)/2;
	w1 = ex1 - sx1;
	h1 = ey1 - sy1;
	cx2 = (sx2 + ex2)/2;
	cy2 = (sy2 + ey2)/2;
	w2 = ex2 - sx2;
	h2 = ey2 - sy2;
	distx = (cx2 > cx1 ? cx2 - cx1 : cx1 - cx2);
	disty = (cy2 > cy1 ? cy2 - cy1 : cy1 - cy2);
	if(distx < (w1 + w2)/2 && disty < (h1 + h2)/2){
		int areasize = (MIN(ex1, ex2) - MAX(sx1, sx2)) * (MIN(ey1, ey2) - MAX(sy1, sy2));
		if(areasize > mondata->areasize){
			mondata->areasize = areasize;
			mondata->monitorrect = *lprcMonitor;
			mondata->found = 1;
		}
	}
    return TRUE;
}

// ----

typedef IDirect3D9 * (WINAPI *TEST_DIRECT3DCREATE9)(UINT SDKVersion);

BRESULT scrnmngD3D_check() {
	// Direct3Dが使用できるかチェック + ベンダ確認
	HMODULE hModule;
	TEST_DIRECT3DCREATE9 fnd3dcreate9;
	LPDIRECT3D9				test_d3d;
	D3DPRESENT_PARAMETERS	test_d3dparam;
	LPDIRECT3DDEVICE9		test_d3ddev;
	D3DADAPTER_IDENTIFIER9 identifier;

	hModule = LoadLibrary(_T("d3d9.dll"));
	if(!hModule){
		goto scre_err;
	}

	fnd3dcreate9 = (TEST_DIRECT3DCREATE9)GetProcAddress(hModule, "Direct3DCreate9");
	if(!fnd3dcreate9){
		goto scre_err2;
	}
	if(!(test_d3d = Direct3DCreate9(D3D_SDK_VERSION))){
		goto scre_err2;
	}
	if(test_d3d->GetAdapterIdentifier(D3DADAPTER_DEFAULT, 0, &identifier) == D3D_OK){
		if(identifier.VendorId == 0x10DE){
			// NVIDIA Adapter
			nvidia_fixflag = 1;
		}
	}

	ZeroMemory(&test_d3dparam, sizeof(test_d3dparam));
	test_d3dparam.BackBufferWidth = 640;
	test_d3dparam.BackBufferHeight = 480;
	test_d3dparam.BackBufferFormat = D3DFMT_X8R8G8B8;
	test_d3dparam.BackBufferCount = 1;
	test_d3dparam.MultiSampleType = D3DMULTISAMPLE_NONE;
	test_d3dparam.MultiSampleQuality = 0;
	test_d3dparam.SwapEffect = D3DSWAPEFFECT_DISCARD;
	test_d3dparam.hDeviceWindow = g_hWndMain;
	test_d3dparam.Windowed = TRUE;
	test_d3dparam.EnableAutoDepthStencil = FALSE;
	test_d3dparam.AutoDepthStencilFormat = D3DFMT_D16;
	test_d3dparam.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
	test_d3dparam.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
	test_d3dparam.Flags = 0;
	if(test_d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, g_hWndMain, D3DCREATE_HARDWARE_VERTEXPROCESSING, &test_d3dparam, &test_d3ddev) != D3D_OK){
		if(test_d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, g_hWndMain, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &test_d3dparam, &test_d3ddev) != D3D_OK){
			if(test_d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, g_hWndMain, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &test_d3dparam, &test_d3ddev) != D3D_OK){
				goto scre_err3;
			}
		}
	}
	// デバイス作成まで出来そうならOKとする
	test_d3ddev->Release();
	test_d3d->Release();
	FreeLibrary(hModule);

	return(SUCCESS);
scre_err3:
	test_d3d->Release();
scre_err2:
	FreeLibrary(hModule);
scre_err:
	return(FAILURE);
}

BRESULT scrnmngD3D_create(UINT8 scrnmode) {

	DWORD			winstyle;
	DWORD			winstyleex;
	//D3DPRESENT_PARAMETERS d3ddev;
	//LPDIRECT3DDEVICE9	d3ddev;
	//DDPIXELFORMAT	ddpf;
	int				width;
	int				height;
	UINT			bitcolor;
	UINT			fscrnmod;
	DEVMODE			devmode;
	GUID			devguid = {0};
	int				bufwidth, bufheight;
	int				k = 0;

	np2oscfg.d3d_exclusive = 0;// 排他モードは廃止

	if(devicelostflag) return(FAILURE);
	
	if(!d3d_cs_initialized){
		memset(&d3d_cs, 0, sizeof(d3d_cs));
		InitializeCriticalSection(&d3d_cs);
		d3d_cs_initialized = 1;
	}

	d3d_enter_criticalsection();
	scrnmng_create_pending = false;

	current_d3d_imode = FSCRNCFG_d3d_imode;

	static UINT8 lastscrnmode = 0;
	static WINDOWPLACEMENT wp = {sizeof(WINDOWPLACEMENT)};

	ZeroMemory(&scrnmng, sizeof(scrnmng));
	winstyle = GetWindowLong(g_hWndMain, GWL_STYLE);
	winstyleex = GetWindowLong(g_hWndMain, GWL_EXSTYLE);
	if (scrnmode & SCRNMODE_FULLSCREEN) {
		//if(np2oscfg.mouse_nc){
		//	winstyle &= ~CS_DBLCLKS;
		//}else{
			winstyle |= CS_DBLCLKS;
		//}
		if(!(lastscrnmode & SCRNMODE_FULLSCREEN)){
			GetWindowPlacement(g_hWndMain, &wp);
		}
		scrnmode &= ~SCRNMODE_ROTATEMASK;
		scrnmng.flag = SCRNFLAG_FULLSCREEN;
		winstyle &= ~(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME);
		winstyle |= WS_POPUP;
		//winstyleex |= WS_EX_TOPMOST;
		d3d.menudisp = 0;
		d3d.menusize = GetSystemMetrics(SM_CYMENU);
		np2class_enablemenu(g_hWndMain, FALSE);
		SetWindowLong(g_hWndMain, GWL_STYLE, winstyle);
		SetWindowLong(g_hWndMain, GWL_EXSTYLE, winstyleex);
	}
	else {
		scrnmng.flag = SCRNFLAG_HAVEEXTEND;
		winstyle |= WS_SYSMENU;
		if(np2oscfg.mouse_nc){
			winstyle &= ~CS_DBLCLKS;
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
		}else{
			winstyle |= CS_DBLCLKS;
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
			int invalidsize = 0;
			ShowWindow(g_hWndMain, SW_HIDE);
			SetWindowLong(g_hWndMain, GWL_STYLE, winstyle);
			SetWindowLong(g_hWndMain, GWL_EXSTYLE, winstyleex);
			SetWindowPos(g_hWndMain, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_FRAMECHANGED);
			ShowWindow(g_hWndMain, SW_SHOWNORMAL);SetWindowPlacement(g_hWndMain, &wp);
		}else{
			SetWindowLong(g_hWndMain, GWL_STYLE, winstyle);
			SetWindowLong(g_hWndMain, GWL_EXSTYLE, winstyleex);
			SetWindowPos(g_hWndMain, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_FRAMECHANGED|SWP_NOACTIVATE);
			GetWindowPlacement(g_hWndMain, &wp);
		}
	}
	
	if(!(d3d.d3d = Direct3DCreate9(D3D_SDK_VERSION))){
		goto scre_err;
	}
	ZeroMemory(&d3d.d3dparam, sizeof(d3d.d3dparam));
	//d3d.d3dparam.BackBufferWidth = 0;
	//d3d.d3dparam.BackBufferHeight = 0;
	d3d.d3dparam.BackBufferFormat = D3DFMT_X8R8G8B8;
	d3d.d3dparam.BackBufferCount = 1;
	d3d.d3dparam.MultiSampleType = D3DMULTISAMPLE_NONE;
	d3d.d3dparam.MultiSampleQuality = 0;
	d3d.d3dparam.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3d.d3dparam.hDeviceWindow = g_hWndMain;
	d3d.d3dparam.Windowed = ((scrnmode & SCRNMODE_FULLSCREEN) && np2oscfg.d3d_exclusive) ? FALSE : TRUE;
	d3d.d3dparam.EnableAutoDepthStencil = FALSE;
	d3d.d3dparam.AutoDepthStencilFormat = D3DFMT_D16;
	d3d.d3dparam.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
	d3d.d3dparam.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
	d3d.d3dparam.Flags = (scrnmode & SCRNMODE_FULLSCREEN) ? D3DPRESENTFLAG_LOCKABLE_BACKBUFFER : 0;
	bitcolor = 32;

	if (scrnmode & SCRNMODE_FULLSCREEN) {

#if defined(SUPPORT_DCLOCK)
		DispClock::GetInstance()->Initialize();
#endif
		width = np2oscfg.fscrn_cx;
		height = np2oscfg.fscrn_cy;
#ifdef SUPPORT_WAB
		if(!np2wabwnd.multiwindow && (np2wab.relay&0x3)){
			if(scrnstat.width>=640 && scrnstat.height>=400){
			//if(np2wab.realWidth>=640 && np2wab.realHeight>=400){
				//width = np2wab.realWidth;
				//height = np2wab.realHeight;
				width = scrnstat.width;//np2wab.realWidth;
				height = scrnstat.height;//np2wab.realHeight;
			}else{
				width = 640;
				height = 480;
			}
		}
#endif

		fscrnmod = FSCRNCFG_fscrnmod;

		// 排他モードを使わないとき
		if(!np2oscfg.d3d_exclusive){
			MONITORINFOENUMDATA mondata = {0};
			fscrnmod |= FSCRNMOD_SAMERES | FSCRNMOD_SAMEBPP; // 排他モードを使わないとき、解像度変更しないことにする（できなくはないが面倒なことこの上ない）
			GetWindowRect(g_hWndMain, &mondata.np2windowrect); // 猫ウィンドウの位置とサイズを取得
			EnumDisplayMonitors(NULL, NULL, (MONITORENUMPROC)monitorInfoEnumProc, (LPARAM)&mondata); // 猫のいるモニターを探す
			if(!mondata.found){
				// 猫どこにもいない時にはプライマリモニタで
				mondata.monitorrect.left = mondata.monitorrect.top = 0;
				mondata.monitorrect.right = GetSystemMetrics(SM_CXSCREEN);
				mondata.monitorrect.bottom = GetSystemMetrics(SM_CYSCREEN);
			}
			MoveWindow(g_hWndMain, mondata.monitorrect.left, mondata.monitorrect.top, 
				mondata.monitorrect.right - mondata.monitorrect.left, mondata.monitorrect.bottom - mondata.monitorrect.top, TRUE); // ウィンドウサイズを全画面に変える
		}
		if(np2_multithread_Enabled()){
			fscrnmod |= FSCRNMOD_SAMERES | FSCRNMOD_SAMEBPP; // マルチスレッドモードのときも解像度変更しないことにする（手抜き）
		}

		if(!(fscrnmod & FSCRNMOD_SAMERES)){
			current_d3d_imode = D3D_IMODE_NEAREST_NEIGHBOR;
		}
		if ((fscrnmod & (FSCRNMOD_SAMERES | FSCRNMOD_SAMEBPP)) && EnumDisplaySettings(NULL, ENUM_REGISTRY_SETTINGS, &devmode)) {
			if (fscrnmod & FSCRNMOD_SAMERES) {
				width = devmode.dmPelsWidth;
				height = devmode.dmPelsHeight;
			}
			//if (fscrnmod & FSCRNMOD_SAMEBPP) {
			//	bitcolor = devmode.dmBitsPerPel;
			//}
		}
		if ((width == 0) || (height == 0)) {
			width = 640;
			height = (np2oscfg.force400)?400:480;
		}

		if(checkResolution(width, height) != SUCCESS){
			if (EnumDisplaySettings(NULL, ENUM_REGISTRY_SETTINGS, &devmode)) {
				width = devmode.dmPelsWidth;
				height = devmode.dmPelsHeight;
			}
		}
		
		d3d.d3dparam.BackBufferWidth = width;
		d3d.d3dparam.BackBufferHeight = height;
		for(k=0;k<CREATEDEVICE_RETRY_MAX;k++){
			if(d3d.d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, g_hWndMain, D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3d.d3dparam, &d3d.d3ddev) != D3D_OK){
				if(d3d.d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, g_hWndMain, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3d.d3dparam, &d3d.d3ddev) != D3D_OK){
					if(d3d.d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, g_hWndMain, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3d.d3dparam, &d3d.d3ddev) != D3D_OK){
						Sleep(1000);
						continue;
					}
				}
			}
			break;
		}
		if(k==CREATEDEVICE_RETRY_MAX){
			d3d.d3d->Release();
			d3d.d3d = NULL;
			goto scre_err;
		}

		bufwidth = width;
		bufheight = height;
		if (!(scrnmode & SCRNMODE_ROTATE)) {
			bufwidth += 1;
		}
		else {
			bufheight += 1;
		}

		d3d.d3ddev->CreateOffscreenPlainSurface(bufwidth, bufheight, d3d.d3dparam.BackBufferFormat, D3DPOOL_DEFAULT, &d3d.backsurf, NULL);
		//d3d.d3ddev->CreateRenderTarget(bufwidth, bufheight, d3d.d3dparam.BackBufferFormat, D3DMULTISAMPLE_NONE, 0, FALSE, &d3d.backsurf, NULL);
#ifdef SUPPORT_WAB
		d3d.d3ddev->CreateOffscreenPlainSurface(width, height, d3d.d3dparam.BackBufferFormat, D3DPOOL_DEFAULT, &d3d.wabsurf, NULL);
#endif

#if defined(SUPPORT_DCLOCK)
		d3d.d3ddev->CreateOffscreenPlainSurface(DCLOCK_WIDTH, DCLOCK_HEIGHT, d3d.d3dparam.BackBufferFormat, D3DPOOL_DEFAULT, &d3d.clocksurf, NULL);

		DispClock::GetInstance()->SetPalettes(bitcolor);
		DispClock::GetInstance()->Reset();
#endif

		d3d.extend = 1;
	}
	else {
		int	wabwidth;
		int	wabheight;
		RECT crect;

#ifdef SUPPORT_WAB
		//if(!np2wabwnd.multiwindow && (np2wab.relay&0x3)!=0 && np2wab.realWidth>=640 && np2wab.realHeight>=400){
		if(!np2wabwnd.multiwindow && (np2wab.relay&0x3)!=0 && scrnstat.width>=640 && scrnstat.height>=400){
			// 実サイズに
			width = wabwidth = bufwidth = scrnstat.width;//np2wab.realWidth;
			height = wabheight = bufheight = scrnstat.height;//np2wab.realHeight;
			if (scrnmode & SCRNMODE_ROTATE) {
				wabwidth = bufwidth = scrnstat.height;
				wabheight = bufheight = scrnstat.width;
			}
			bufwidth++; // +1しないと駄目らしい
			bufheight++; // +1しないと駄目らしい
		}else{
			if (!(scrnmode & SCRNMODE_ROTATE)) {
				wabwidth = 640;
				wabheight = 480;
				bufwidth = 640 + 1;
				bufheight = 480;
			}
			else {
				wabwidth = 480;
				wabheight = 640;
				bufwidth = 480;
				bufheight = 640 + 1;
			}
			width = 640;
			height = 480;
		}
#else
		if (!(scrnmode & SCRNMODE_ROTATE)) {
			wabwidth = 640;
			wabheight = 480;
			d3d.d3dparam.BackBufferWidth = 640 + 1;
			d3d.d3dparam.BackBufferHeight = 480;
		}
		else {
			wabwidth = 480;
			wabheight = 640;
			d3d.d3dparam.BackBufferWidth = 480;
			d3d.d3dparam.BackBufferHeight = 640 + 1;
		}
		width = 640;
		height = 480;
#endif

		d3d.scrnmode = scrnmode;
		d3d.width = width;
		d3d.height = height;
		renewalclientsize(TRUE);
		
		GetClientRect(g_hWndMain, &crect);
		d3d.d3dparam.BackBufferWidth = crect.right - crect.left;
		d3d.d3dparam.BackBufferHeight = crect.bottom - crect.top;
		for(k=0;k<CREATEDEVICE_RETRY_MAX;k++){
			if(d3d.d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, g_hWndMain, D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3d.d3dparam, &d3d.d3ddev) != D3D_OK){
				if(d3d.d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, g_hWndMain, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3d.d3dparam, &d3d.d3ddev) != D3D_OK){
					if(d3d.d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, g_hWndMain, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3d.d3dparam, &d3d.d3ddev) != D3D_OK){
						Sleep(1000);
						continue;
					}
				}
			}
			break;
		}
		if(k==CREATEDEVICE_RETRY_MAX){
			d3d.d3d->Release();
			d3d.d3d = NULL;
			goto scre_err;
		}

		d3d.d3ddev->CreateOffscreenPlainSurface(bufwidth, bufheight, d3d.d3dparam.BackBufferFormat, D3DPOOL_DEFAULT, &d3d.backsurf, NULL);
		//d3d.d3ddev->CreateRenderTarget(bufwidth, bufheight, d3d.d3dparam.BackBufferFormat, D3DMULTISAMPLE_NONE, 0, FALSE, &d3d.backsurf, NULL);
#ifdef SUPPORT_WAB
		d3d.d3ddev->CreateOffscreenPlainSurface(wabwidth, wabheight, d3d.d3dparam.BackBufferFormat, D3DPOOL_DEFAULT, &d3d.wabsurf, NULL);
#endif

		d3d.extend = 1;
	}

	update_backbuffer2size();

	d3d.d3ddev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &d3d.d3dbacksurf);
	
	scrnmng.bpp = (UINT8)bitcolor;
	scrnsurf.bpp = bitcolor;
	d3d.scrnmode = scrnmode;
	d3d.width = width;
	d3d.height = height;
	d3d.cliping = 0;
	//if (scrnmode & SCRNMODE_FULLSCREEN) {
		renewalclientsize(TRUE); // XXX: スナップ解除等が起こるので暫定TRUE
	//}
	lastscrnmode = scrnmode;
//	screenupdate = 3;					// update!
#if defined(SUPPORT_WAB)
	mt_wabpausedrawing = 0; // MultiThread対策
#endif

	d3d_leave_criticalsection();
	
	return(SUCCESS);

scre_err:
	d3d_leave_criticalsection();
	scrnmngD3D_destroy();
	return(FAILURE);
}

void scrnmngD3D_destroy(void) {
	BOOL oldwindowed;

	if(devicelostflag) return;
	
	d3d_enter_criticalsection();
#if defined(SUPPORT_DCLOCK)
	if (d3d.clocksurf) {
		d3d.clocksurf->Release();
		d3d.clocksurf = NULL;
	}
#endif
#if defined(SUPPORT_WAB)
	if (d3d.wabsurf) {
		mt_wabpausedrawing = 1; // MultiThread対策
		while(mt_wabdrawing) 
			Sleep(10);
		d3d.wabsurf->Release();
		d3d.wabsurf = NULL;
	}
#endif
	if (d3d.backsurf2) {
		d3d.backsurf2->Release();
		d3d.backsurf2 = NULL;
	}
	if (d3d.backsurf) {
		d3d.backsurf->Release();
		d3d.backsurf = NULL;
	}
	if (d3d.d3dbacksurf) {
		d3d.d3dbacksurf->Release();
		d3d.d3dbacksurf = NULL;
	}
	if (scrnmng.flag & SCRNFLAG_FULLSCREEN) {
		np2class_enablemenu(g_hWndMain, (!np2oscfg.wintype));
		oldwindowed = d3d.d3dparam.Windowed;
		d3d.d3dparam.Windowed = TRUE;
		d3d.d3ddev->Reset(&d3d.d3dparam);
		d3d.d3dparam.Windowed = oldwindowed;
	}
	if (d3d.d3ddev) {
		d3d.d3ddev->Release();
		d3d.d3ddev = NULL;
	}
	if (d3d.d3d) {
		d3d.d3d->Release();
		d3d.d3d = NULL;
	}
	ZeroMemory(&d3d, sizeof(d3d));
	d3d_leave_criticalsection();
}

void scrnmngD3D_shutdown(void) {
	
	if(d3d_cs_initialized){
		DeleteCriticalSection(&d3d_cs);
		d3d_cs_initialized = 0;
	}
}

void scrnmngD3D_querypalette(void) {

}

RGB16 scrnmngD3D_makepal16(RGB32 pal32) {
	return 0;
}

void scrnmngD3D_fullscrnmenu(int y) {

	UINT8	menudisp;
	
	if(d3d.d3ddev == NULL) return;
	if(devicelostflag) return;
	
	d3d_enter_criticalsection();
	if (scrnmng.flag & SCRNFLAG_FULLSCREEN) {
		menudisp = ((y >= 0) && (y < d3d.menusize)) ? 1 : 0;
		if (d3d.menudisp != menudisp) {
			d3d.menudisp = menudisp;
			if (menudisp == 1) {
				np2class_enablemenu(g_hWndMain, TRUE);
				if(np2oscfg.d3d_exclusive) d3d.d3ddev->SetDialogBoxMode(TRUE);
				//d3d.d3ddev->Present(NULL, NULL, NULL, NULL);
				//InvalidateRect(g_hWndMain, NULL, TRUE);
				//DrawMenuBar(g_hWndMain);
			}
			else {
				if(np2oscfg.d3d_exclusive) d3d.d3ddev->SetDialogBoxMode(FALSE);
				np2class_enablemenu(g_hWndMain, FALSE);
				clearoutfullscreen();
			}
		}
	}
	d3d_leave_criticalsection();
}

void scrnmngD3D_topwinui(void) {
	
	if(d3d.d3ddev == NULL) return;
	if(devicelostflag) return;

	mousemng_disable(MOUSEPROC_WINUI);
	d3d_enter_criticalsection();
	if (!d3d.cliping++) {
		if (np2oscfg.d3d_exclusive)
		{
			if (d3d.d3ddev)
			{
				d3d.d3ddev->SetDialogBoxMode(TRUE);
			}
		}
#ifndef __GNUC__
		WINNLSEnableIME(g_hWndMain, TRUE);
#endif
	}
	d3d_leave_criticalsection();
}

void scrnmngD3D_clearwinui(void) {
	
	if(d3d.d3ddev == NULL) return;
	if(devicelostflag) return;
	
	d3d_enter_criticalsection();
	if ((d3d.cliping > 0) && (!(--d3d.cliping))) {
#ifndef __GNUC__
		WINNLSEnableIME(g_hWndMain, FALSE);
#endif
		if (np2oscfg.d3d_exclusive)
		{
			if (d3d.d3ddev)
			{
				d3d.d3ddev->SetDialogBoxMode(FALSE);
			}
		}
	}
	if (scrnmng.flag & SCRNFLAG_FULLSCREEN) {
		np2class_enablemenu(g_hWndMain, FALSE);
		clearoutfullscreen();
		d3d.menudisp = 0;
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
				d3d_leave_criticalsection();
				//PostQuitMessage(0);
				scrnmng_create_pending = true;
				return;
			}
		}
		scrnmng.forcereset = 0;
	}
	
	d3d_leave_criticalsection();
}

void scrnmngD3D_setwidth(int posx, int width) {
	
	if(scrnstat.multiple < 1) scrnstat.multiple = 8;
#if defined(SUPPORT_WAB)
	if(width > WAB_MAX_WIDTH) width = WAB_MAX_WIDTH;
#else
	if(width > 640) width = 640;
#endif
	if(scrnstat.width != width){
		scrnstat.width = width;
		if(!d3d.d3dbacksurf){
			d3d_enter_criticalsection();
			scrnmngD3D_create(g_scrnmode);
			d3d_leave_criticalsection();
		}
		if(d3d.d3dbacksurf){
			if (d3d.scrnmode & SCRNMODE_FULLSCREEN) {
				renewalclientsize(TRUE);
				update_backbuffer2size();
				clearoutfullscreen();
			}else{
				DEVMODE devmode;
				if (EnumDisplaySettings(NULL, ENUM_REGISTRY_SETTINGS, &devmode)) {
					while (((width * scrnstat.multiple) >> 3) >= (int)devmode.dmPelsWidth-32){
						scrnstat.multiple--;
						if(scrnstat.multiple==1) break;
					}
				}
				d3d_enter_criticalsection();
				scrnmngD3D_destroy();
				scrnmngD3D_create(g_scrnmode);
				d3d_leave_criticalsection();
				renewalclientsize(TRUE);
			}
		}
	}
}

void scrnmngD3D_setextend(int extend) {
	
	if(scrnstat.extend != extend){
		scrnstat.extend = extend;
		scrnmng.allflash = TRUE;
		if(!d3d.d3dbacksurf){
			d3d_enter_criticalsection();
			scrnmngD3D_create(g_scrnmode);
			d3d_leave_criticalsection();
		}
		if(d3d.d3dbacksurf){
			if (d3d.scrnmode & SCRNMODE_FULLSCREEN) {
				renewalclientsize(TRUE);
				update_backbuffer2size();
				clearoutfullscreen();
			}else{
				d3d_enter_criticalsection();
				scrnmngD3D_destroy();
				scrnmngD3D_create(g_scrnmode);
				d3d_leave_criticalsection();
				renewalclientsize(TRUE);
			}
		}
	}
}

void scrnmngD3D_setheight(int posy, int height) {
	
	if(scrnstat.multiple < 1) scrnstat.multiple = 8;
#if defined(SUPPORT_WAB)
	if(height > WAB_MAX_HEIGHT) height = WAB_MAX_HEIGHT;
#else
	if(height > 1024) height = 1024;
#endif
	if(scrnstat.height != height){
		scrnstat.height = height;
		if(!d3d.d3dbacksurf){
			d3d_enter_criticalsection();
			scrnmngD3D_create(g_scrnmode);
			d3d_leave_criticalsection();
		}
		if(d3d.d3dbacksurf){
			if (d3d.scrnmode & SCRNMODE_FULLSCREEN) {
				renewalclientsize(TRUE);
				update_backbuffer2size();
				clearoutfullscreen();
			}else{
				DEVMODE devmode;
				if (EnumDisplaySettings(NULL, ENUM_REGISTRY_SETTINGS, &devmode)) {
					while (((height * scrnstat.multiple) >> 3) >= (int)devmode.dmPelsHeight-32){
						scrnstat.multiple--;
						if(scrnstat.multiple==1) break;
					}
				}
				d3d_enter_criticalsection();
				scrnmngD3D_destroy();
				scrnmngD3D_create(g_scrnmode);
				d3d_leave_criticalsection();
				renewalclientsize(TRUE);
			}
		}
	}
}

void scrnmngD3D_setsize(int posx, int posy, int width, int height) {
	
	if(scrnstat.multiple < 1) scrnstat.multiple = 8;
#if defined(SUPPORT_WAB)
	if(width > WAB_MAX_WIDTH) width = WAB_MAX_WIDTH;
	if(height > WAB_MAX_HEIGHT) height = WAB_MAX_HEIGHT;
#else
	if(width > 640) width = 640;
	if(height > 1024) height = 1024;
#endif
	if(scrnstat.width != width || scrnstat.height != height){
		scrnstat.width = width;
		scrnstat.height = height;
		if(!d3d.d3dbacksurf){
			d3d_enter_criticalsection();
			scrnmngD3D_create(g_scrnmode);
			d3d_leave_criticalsection();
		}
		if(d3d.d3dbacksurf){
			if (d3d.scrnmode & SCRNMODE_FULLSCREEN) {
				renewalclientsize(TRUE);
				update_backbuffer2size();
				clearoutfullscreen();
			}else{
				DEVMODE devmode;
				if (EnumDisplaySettings(NULL, ENUM_REGISTRY_SETTINGS, &devmode)) {
					while (((width * scrnstat.multiple) >> 3) >= (int)devmode.dmPelsWidth-32){
						scrnstat.multiple--;
						if(scrnstat.multiple==1) break;
					}
				}
				d3d_enter_criticalsection();
				scrnmngD3D_destroy();
				scrnmngD3D_create(g_scrnmode);
				d3d_leave_criticalsection();
				renewalclientsize(TRUE);
			}
		}
	}
}

const SCRNSURF *scrnmngD3D_surflock(void) {

	D3DLOCKED_RECT	destrect;
	HRESULT			r;
	
	if(devicelostflag){
		scrnmng_restore_pending = true;
		return NULL;
	}

	ZeroMemory(&destrect, sizeof(destrect));
	if (d3d.backsurf == NULL) {
		return(NULL);
	}
	d3d_enter_criticalsection();
	if (d3d.backsurf == NULL)
	{
		d3d_leave_criticalsection();
		return(NULL);
	}
	r = d3d.backsurf->LockRect(&destrect, NULL, 0);
	if (r != D3D_OK) {
		d3d_leave_criticalsection();
//		TRACEOUT(("backsurf lock error: %d (%d)", r));
		return(NULL);
	}
	if (!(d3d.scrnmode & SCRNMODE_ROTATE)) {
		scrnsurf.ptr = (UINT8 *)destrect.pBits;
		scrnsurf.xalign = scrnsurf.bpp >> 3;
		scrnsurf.yalign = destrect.Pitch;
	}
	else if (!(d3d.scrnmode & SCRNMODE_ROTATEDIR)) {
		scrnsurf.ptr = (UINT8 *)destrect.pBits;
		scrnsurf.ptr += (scrnsurf.width + scrnsurf.extend - 1) * destrect.Pitch;
		scrnsurf.xalign = 0 - destrect.Pitch;
		scrnsurf.yalign = scrnsurf.bpp >> 3;
	}
	else {
		scrnsurf.ptr = (UINT8 *)destrect.pBits;
		scrnsurf.ptr += (scrnsurf.height - 1) * (scrnsurf.bpp >> 3);
		scrnsurf.xalign = destrect.Pitch;
		scrnsurf.yalign = 0 - (scrnsurf.bpp >> 3);
	}
	return(&scrnsurf);
}

void scrnmngD3D_surfunlock(const SCRNSURF *surf) {
	
	if (d3d.backsurf == NULL)
	{
		d3d_leave_criticalsection();
		return;
	}
	d3d.backsurf->UnlockRect();
	scrnmngD3D_update();
	recvideo_update();
	d3d_leave_criticalsection();
}

void scrnmngD3D_update(void) {
	
	if(d3d.d3ddev == NULL) return;
	if(devicelostflag){
		scrnmng_restore_pending = true;
		return;
	}

	POINT	clip;
	RECT	dst;
	RECT	*rect;
	RECT	*scrn;
	HRESULT	r;
	D3DTEXTUREFILTERTYPE d3dtexf;
	int		scrnoffset;

	if(current_d3d_imode == D3D_IMODE_NEAREST_NEIGHBOR){
		d3dtexf = D3DTEXF_POINT;
	}else{
		d3dtexf = D3DTEXF_LINEAR;
	}
	
	if (scrnmng.palchanged) {
		scrnmng.palchanged = FALSE;
		paletteset();
	}
	
	if(!d3d_tryenter_criticalsection()){
		req_enter_criticalsection = 1;
		return;
	}
	//d3d_enter_criticalsection();
	if(d3d.backsurf != NULL) {
		if(d3d.backsurf2 != NULL && (current_d3d_imode == D3D_IMODE_PIXEL || current_d3d_imode == D3D_IMODE_PIXEL2 || current_d3d_imode == D3D_IMODE_PIXEL3)){
			RECT	rectbuf = {0};
			RECT	rectsrcbuf = {0};
			RECT	rectdstbuf = {0};
			if (d3d.scrnmode & SCRNMODE_FULLSCREEN) {
				if (scrnmng.allflash) {
					scrnmng.allflash = 0;
					clearoutfullscreen();
				}
				if (GetWindowLongPtr(g_hWndMain, NP2GWLP_HMENU)) {
					rect = &d3d.rect;
					scrn = &d3d.scrn;
				}
				else {
					rect = &d3d.rectclip;
					scrn = &d3d.scrnclip;
				}
				scrnoffset = (scrn->right - scrn->left)/(rect->right - rect->left);

				RECT lastrect = *rect;
				RECT lastscrn = *scrn;
				rect->right++; // なぞの調整
				scrn->left -= scrnoffset; // なぞの調整
				if(scrn->left < 0){
					// for full mode
					rect->left++; // なぞの調整
					scrn->left += scrnoffset; // なぞの調整
				}

				rectbuf.right = (rect->right - rect->left) * d3d.backsurf2mul;
				rectbuf.bottom = (rect->bottom - rect->top) * d3d.backsurf2mul;
				
				if(d3d.backsurf2mul > 1){
					// 転送時の1pxズレ対策
					rectsrcbuf = *rect;
					rectdstbuf = rectbuf;
					if(nvidia_fixflag){
						rectsrcbuf.left = rectsrcbuf.right - 1;
						rectdstbuf.left = rectdstbuf.right - ((1 << (d3d.backsurf2mul-1)) >> 1);
						rectdstbuf.bottom -= ((1 << (d3d.backsurf2mul-1)) >> 1);
						r = d3d.d3ddev->StretchRect(d3d.backsurf, &rectsrcbuf, d3d.backsurf2, &rectdstbuf, D3DTEXF_POINT); // 右端
						rectsrcbuf = *rect;
						rectdstbuf = rectbuf;
						rectsrcbuf.top = rectsrcbuf.bottom - 1;
						rectdstbuf.top = rectdstbuf.bottom - ((1 << (d3d.backsurf2mul-1)) >> 1);
						rectdstbuf.right -= ((1 << (d3d.backsurf2mul-1)) >> 1);
						r = d3d.d3ddev->StretchRect(d3d.backsurf, &rectsrcbuf, d3d.backsurf2, &rectdstbuf, D3DTEXF_POINT); // 下端
						rectsrcbuf = *rect;
						rectdstbuf = rectbuf;
						rectsrcbuf.left = rectsrcbuf.right - 1;
						rectdstbuf.left = rectdstbuf.right - ((1 << (d3d.backsurf2mul-1)) >> 1);
						rectsrcbuf.top = rectsrcbuf.bottom - 1;
						rectdstbuf.top = rectdstbuf.bottom - ((1 << (d3d.backsurf2mul-1)) >> 1);
						r = d3d.d3ddev->StretchRect(d3d.backsurf, &rectsrcbuf, d3d.backsurf2, &rectdstbuf, D3DTEXF_POINT); // 右下隅
						rectsrcbuf = *rect;
						rectdstbuf = rectbuf;
						rectdstbuf.right -= ((1 << (d3d.backsurf2mul-1)) >> 1);
						rectdstbuf.bottom -= ((1 << (d3d.backsurf2mul-1)) >> 1);
					}
					r = d3d.d3ddev->StretchRect(d3d.backsurf, &rectsrcbuf, d3d.backsurf2, &rectdstbuf, D3DTEXF_POINT);
					r = d3d.d3ddev->StretchRect(d3d.backsurf2, &rectbuf, d3d.d3dbacksurf, scrn, D3DTEXF_LINEAR);
				}else{
					r = d3d.d3ddev->StretchRect(d3d.backsurf, rect, d3d.d3dbacksurf, scrn, D3DTEXF_LINEAR);
				}
				*rect = lastrect;
				*scrn = lastscrn;
			}
			else {
				int bufwidth, bufheight;
				D3DSURFACE_DESC d3dsdesc;
				if (scrnmng.allflash) {
					scrnmng.allflash = 0;
					clearoutscreen();
				}
				d3d.d3dbacksurf->GetDesc(&d3dsdesc);
				bufwidth = d3dsdesc.Width;
				bufheight = d3dsdesc.Height;
				clip.x = 0;
				clip.y = 0;
				dst.left = clip.x + d3d.scrn.left;
				dst.top = clip.y + d3d.scrn.top;
				dst.right = clip.x + d3d.scrn.right;
				dst.bottom = clip.y + d3d.scrn.bottom;
				
				rectbuf.right = (d3d.rect.right - d3d.rect.left) * d3d.backsurf2mul;
				rectbuf.bottom = (d3d.rect.bottom - d3d.rect.top) * d3d.backsurf2mul;

				if(d3d.backsurf2mul > 1){
					// 転送時の1pxズレ対策
					rectsrcbuf = d3d.rect;
					rectdstbuf = rectbuf;
					if(nvidia_fixflag){
						rectsrcbuf.left = rectsrcbuf.right - 1;
						rectdstbuf.left = rectdstbuf.right - ((1 << (d3d.backsurf2mul-1)) >> 1);
						rectdstbuf.bottom -= ((1 << (d3d.backsurf2mul-1)) >> 1);
						r = d3d.d3ddev->StretchRect(d3d.backsurf, &rectsrcbuf, d3d.backsurf2, &rectdstbuf, D3DTEXF_POINT); // 右端
						rectsrcbuf = d3d.rect;
						rectdstbuf = rectbuf;
						rectsrcbuf.top = rectsrcbuf.bottom - 1;
						rectdstbuf.top = rectdstbuf.bottom - ((1 << (d3d.backsurf2mul-1)) >> 1);
						rectdstbuf.right -= ((1 << (d3d.backsurf2mul-1)) >> 1);
						r = d3d.d3ddev->StretchRect(d3d.backsurf, &rectsrcbuf, d3d.backsurf2, &rectdstbuf, D3DTEXF_POINT); // 下端
						rectsrcbuf = d3d.rect;
						rectdstbuf = rectbuf;
						rectsrcbuf.left = rectsrcbuf.right - 1;
						rectdstbuf.left = rectdstbuf.right - ((1 << (d3d.backsurf2mul-1)) >> 1);
						rectsrcbuf.top = rectsrcbuf.bottom - 1;
						rectdstbuf.top = rectdstbuf.bottom - ((1 << (d3d.backsurf2mul-1)) >> 1);
						r = d3d.d3ddev->StretchRect(d3d.backsurf, &rectsrcbuf, d3d.backsurf2, &rectdstbuf, D3DTEXF_POINT); // 右下隅
						rectsrcbuf = d3d.rect;
						rectdstbuf = rectbuf;
						rectdstbuf.right -= ((1 << (d3d.backsurf2mul-1)) >> 1);
						rectdstbuf.bottom -= ((1 << (d3d.backsurf2mul-1)) >> 1);
					}
					r = d3d.d3ddev->StretchRect(d3d.backsurf, &rectsrcbuf, d3d.backsurf2, &rectdstbuf, D3DTEXF_POINT);
					if(dst.bottom > bufheight){
						rectbuf.bottom -= (dst.bottom - bufheight) * (rectbuf.right - rectbuf.left) / (dst.right - dst.left);
						dst.bottom -= dst.bottom - bufheight;
					}
					r = d3d.d3ddev->StretchRect(d3d.backsurf2, &rectbuf, d3d.d3dbacksurf, &dst, D3DTEXF_LINEAR);
				}else{
					rectbuf = d3d.rect;
					if(dst.bottom > bufheight){
						rectbuf.bottom -= (dst.bottom - bufheight) * (rectbuf.right - rectbuf.left) / (dst.right - dst.left);
						dst.bottom -= dst.bottom - bufheight;
					}
					r = d3d.d3ddev->StretchRect(d3d.backsurf, &rectbuf, d3d.d3dbacksurf, &dst, D3DTEXF_LINEAR);
				}
			}
			//if((d3d.scrnmode & SCRNMODE_FULLSCREEN) && d3d.menudisp){
			//	DrawMenuBar(g_hWndMain);
			//}
			if(d3d.d3ddev->Present(NULL, NULL, NULL, NULL)==D3DERR_DEVICELOST){
				scrnmng_restore_pending = true;
				d3d_leave_criticalsection();
				return;
			}
		}else{
			if (d3d.scrnmode & SCRNMODE_FULLSCREEN) {
				if (scrnmng.allflash) {
					scrnmng.allflash = 0;
					clearoutfullscreen();
				}
				if (GetWindowLongPtr(g_hWndMain, NP2GWLP_HMENU)) {
					rect = &d3d.rect;
					scrn = &d3d.scrn;
				}
				else {
					rect = &d3d.rectclip;
					scrn = &d3d.scrnclip;
				}
				scrnoffset = (scrn->right - scrn->left)/(rect->right - rect->left);
				RECT lastrect = *rect;
				RECT lastscrn = *scrn;
				if(nvidia_fixflag){
					rect->right++; // なぞの調整
					scrn->left -= scrnoffset; // なぞの調整
					scrn->right -= ((1 << (scrnoffset-1)) >> 1);
					scrn->bottom -= ((1 << (scrnoffset-1)) >> 1);
				}
				if(scrn->left < 0){
					// for full mode
					rect->left++; // なぞの調整
					scrn->left += scrnoffset; // なぞの調整
				}
				r = d3d.d3ddev->StretchRect(d3d.backsurf, rect, d3d.d3dbacksurf, scrn, d3dtexf);
				*rect = lastrect;
				*scrn = lastscrn;
			}
			else {
				RECT	rectbuf = {0};
				int bufwidth, bufheight;
				D3DSURFACE_DESC d3dsdesc;

				if (scrnmng.allflash) {
					scrnmng.allflash = 0;
					clearoutscreen();
				}
				d3d.d3dbacksurf->GetDesc(&d3dsdesc);
				bufwidth = d3dsdesc.Width;
				bufheight = d3dsdesc.Height;
				clip.x = 0;
				clip.y = 0;
				//ClientToScreen(g_hWndMain, &clip);
				dst.left = clip.x + d3d.scrn.left;
				dst.top = clip.y + d3d.scrn.top;
				dst.right = clip.x + d3d.scrn.right;
				dst.bottom = clip.y + d3d.scrn.bottom;
				rectbuf = d3d.rect;
				if(dst.bottom > bufheight){
					rectbuf.bottom -= (dst.bottom - bufheight) * (rectbuf.right - rectbuf.left) / (dst.right - dst.left);
					dst.bottom -= dst.bottom - bufheight;
				}
				r = d3d.d3ddev->StretchRect(d3d.backsurf, &rectbuf, d3d.d3dbacksurf, &dst, d3dtexf);
			}
			//if((d3d.scrnmode & SCRNMODE_FULLSCREEN) && d3d.menudisp){
			//	DrawMenuBar(g_hWndMain);
			//}
			if(d3d.d3ddev->Present(NULL, NULL, NULL, NULL)==D3DERR_DEVICELOST){
				scrnmng_restore_pending = true;
				d3d_leave_criticalsection();
				return;
			}
		}
	}
	d3d_leave_criticalsection();
	req_enter_criticalsection = 0;
}


// ----

void scrnmngD3D_setmultiple(int multiple)
{
	if (scrnstat.multiple != multiple)
	{
		scrnstat.multiple = multiple;
		if(d3d.d3dbacksurf){
			if (d3d.scrnmode & SCRNMODE_FULLSCREEN) {
				renewalclientsize(TRUE);
				update_backbuffer2size();
				clearoutfullscreen();
			}else{
				DEVMODE devmode;
				if (EnumDisplaySettings(NULL, ENUM_REGISTRY_SETTINGS, &devmode)) {
					while (((scrnstat.width * scrnstat.multiple) >> 3) >= (int)devmode.dmPelsWidth-32){
						scrnstat.multiple--;
						if(scrnstat.multiple==1) break;
					}
				}
				d3d_enter_criticalsection();
				scrnmngD3D_destroy();
				scrnmngD3D_create(g_scrnmode);
				d3d_leave_criticalsection();
				renewalclientsize(TRUE);
			}
		}
	}
}

int scrnmngD3D_getmultiple(void)
{
	return scrnstat.multiple;
}



// ----

#if defined(SUPPORT_DCLOCK)
static const RECT rectclk = {0, 0, DCLOCK_WIDTH, DCLOCK_HEIGHT};

BOOL scrnmngD3D_isdispclockclick(const POINT *pt) {

	if (pt->y >= (d3d.height - DCLOCK_HEIGHT)) {
		return(TRUE);
	}
	else {
		return(FALSE);
	}
}

void scrnmngD3D_dispclock(void)
{
	if(devicelostflag) return;

	if (!d3d.clocksurf)
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
		scrn = &d3d.scrn;
	}
	else
	{
		scrn = &d3d.scrnclip;
	}
	if ((scrn->bottom + DCLOCK_HEIGHT) > d3d.height)
	{
		return;
	}
	DispClock::GetInstance()->Make();
	
	if(d3d.clocksurf == NULL) return;

	D3DLOCKED_RECT dest;
	ZeroMemory(&dest, sizeof(dest));
	d3d_enter_criticalsection();
	if (d3d.clocksurf->LockRect(&dest, NULL, 0) == D3D_OK)
	{
		DispClock::GetInstance()->Draw(scrnmng.bpp, dest.pBits, dest.Pitch);
		d3d.clocksurf->UnlockRect();
	}

	RECT dstrect;
	dstrect.left = d3d.width - DCLOCK_WIDTH - 4;
	dstrect.right = dstrect.left + DCLOCK_WIDTH;
	dstrect.top = d3d.height - DCLOCK_HEIGHT;
	dstrect.bottom = dstrect.top + DCLOCK_HEIGHT;
	d3d.d3ddev->StretchRect(d3d.clocksurf, &rectclk, d3d.d3dbacksurf, &dstrect, D3DTEXF_LINEAR);
	d3d_leave_criticalsection();

	DispClock::GetInstance()->CountDown(np2oscfg.DRAW_SKIP);
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

void scrnmngD3D_entersizing(void) {

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
	cx = min(scrnstat.width, d3d.width);
	cx = (cx + 7) >> 3;
	cy = min(scrnstat.height, d3d.height);
	cy = (cy + 7) >> 3;
	if (!(d3d.scrnmode & SCRNMODE_ROTATE)) {
		scrnsizing.cx = cx;
		scrnsizing.cy = cy;
	}
	else {
		scrnsizing.cx = cy;
		scrnsizing.cy = cx;
	}
	scrnsizing.mul = scrnstat.multiple;
}

void scrnmngD3D_sizing(UINT side, RECT *rect) {

	int		width;
	int		height;
	int		mul;
	const int	mul_max = 32;

	if(scrnsizing.cx==0 || scrnsizing.cy==0) return;

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

void scrnmngD3D_exitsizing(void)
{
	scrnmng_setmultiple(scrnsizing.mul);
	InvalidateRect(g_hWndMain, NULL, TRUE);		// ugh
}

// フルスクリーン解像度調整
void scrnmngD3D_updatefsres(void) {
#ifdef SUPPORT_WAB
	RECT rect;
	int width = scrnstat.width;
	int height = scrnstat.height;
	
	if(d3d.d3ddev == NULL) return;
	if(devicelostflag) return;
	
	rect.left = rect.top = 0;
	rect.right = width;
	rect.bottom = height;

	if(((FSCRNCFG_fscrnmod & FSCRNMOD_SAMERES) || !np2oscfg.d3d_exclusive || np2_multithread_Enabled()) && (g_scrnmode & SCRNMODE_FULLSCREEN)){
		d3d_enter_criticalsection();
		if (d3d.d3ddev)
		{
			d3d.d3ddev->ColorFill(d3d.wabsurf, NULL, D3DCOLOR_XRGB(0, 0, 0));
			d3d.d3ddev->ColorFill(d3d.backsurf, NULL, D3DCOLOR_XRGB(0, 0, 0));
			clearoutscreen();
		}
		d3d_leave_criticalsection();
		np2wab.lastWidth = 0;
		np2wab.lastHeight = 0;
		return;
	}
	if(scrnstat.width<100 || scrnstat.height<100){
		d3d_enter_criticalsection();
		if (d3d.d3ddev)
		{
			d3d.d3ddev->ColorFill(d3d.wabsurf, NULL, D3DCOLOR_XRGB(0, 0, 0));
			d3d.d3ddev->ColorFill(d3d.backsurf, NULL, D3DCOLOR_XRGB(0, 0, 0));
			clearoutscreen();
		}
		d3d_leave_criticalsection();
		return;
	}
	if(np2wab.lastWidth!=width || np2wab.lastHeight!=height){
		d3d_enter_criticalsection();

		np2wab.lastWidth = width;
		np2wab.lastHeight = height;
		if((g_scrnmode & SCRNMODE_FULLSCREEN)!=0){
			scrnmngD3D_destroy();
			g_scrnmode = g_scrnmode & ~SCRNMODE_FULLSCREEN;
			if (scrnmngD3D_create(g_scrnmode | SCRNMODE_FULLSCREEN) == SUCCESS) {
				g_scrnmode = g_scrnmode | SCRNMODE_FULLSCREEN;
			}
			else {
				// ウィンドウでリトライ
				if (scrnmngD3D_create(g_scrnmode) != SUCCESS) {
					//PostQuitMessage(0);
					scrnmng_create_pending = true;
					d3d_leave_criticalsection();
					return;
				}
			}
		}else if(d3d.width != width || d3d.height != height){
			scrnmngD3D_destroy();
			if (scrnmngD3D_create(g_scrnmode) != SUCCESS) {
				//if (scrnmngD3D_create(g_scrnmode | SCRNMODE_FULLSCREEN) != SUCCESS) { // フルスクリーンでリトライ
					//PostQuitMessage(0);
					scrnmng_create_pending = true;
					d3d_leave_criticalsection();
					return;
				//}
				//g_scrnmode = g_scrnmode | SCRNMODE_FULLSCREEN;
			}
		}
		clearoutscreen();
		d3d.d3ddev->ColorFill(d3d.wabsurf, NULL, D3DCOLOR_XRGB(0, 0, 0));
		d3d.d3ddev->ColorFill(d3d.backsurf, NULL, D3DCOLOR_XRGB(0, 0, 0));
		
		d3d_leave_criticalsection();
	}
#endif
}

// ウィンドウアクセラレータ画面転送 GDI Device Independent Bitmap -> Direct3D WAB surface
void scrnmngD3D_blthdc(HDC hdc) {
#if defined(SUPPORT_WAB)
	HRESULT	r;
	HDC hDCDD;
	mt_wabdrawing = 0;
	if(d3d.d3ddev == NULL) return;
	if(devicelostflag) return;
	if (np2wabwnd.multiwindow) return;
	if (mt_wabpausedrawing) return;
	if (np2wab.wndWidth < 32 || np2wab.wndHeight < 32) return;
	if (d3d.wabsurf != NULL) {
		//while(req_enter_criticalsection){
		//	np2wab_drawframe();
		//	Sleep(1);
		//}
		d3d_enter_criticalsection();
		if (d3d.wabsurf)
		{
			mt_wabdrawing = 1;
			r = d3d.wabsurf->GetDC(&hDCDD);
			if (r == D3D_OK)
			{
				POINT pt[3];
				switch (d3d.scrnmode & SCRNMODE_ROTATEMASK)
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
				d3d.wabsurf->ReleaseDC(hDCDD);
			}
			mt_wabdrawing = 0;
		}
		d3d_leave_criticalsection();
	}else{
		if(!d3d.d3dbacksurf){
			// 生成要求
			scrnmng_create_pending = true;
			Sleep(1000);
		}
	}
#endif
}

// ウィンドウアクセラレータ画面転送 Direct3D WAB surface -> Direct3D back surface
void scrnmngD3D_bltwab() {
#if defined(SUPPORT_WAB)
	RECT	*dst;
	RECT	src;
	RECT	dstmp;
	//DDBLTFX ddfx;
	int exmgn = 0;
	if(d3d.d3ddev == NULL) return;
	if(devicelostflag) return;
	if (np2wabwnd.multiwindow) return;
	if (d3d.backsurf != NULL) {
		if (d3d.scrnmode & SCRNMODE_FULLSCREEN) {
			dst = &d3d.rect;
		}else{
			dst = &d3d.rect;
			exmgn = (scrnstat.multiple == 8 ? scrnstat.extend : 0);
		}
		src.left = src.top = 0;
		
		if (!(d3d.scrnmode & SCRNMODE_ROTATE)) {
			src.right = scrnstat.width;
			src.bottom = scrnstat.height;
			dstmp = *dst;
			dstmp.left += exmgn;
			dstmp.right = dstmp.left + scrnstat.width;
		}else{
			src.right = scrnstat.height;
			src.bottom = scrnstat.width;
			dstmp = *dst;
			dstmp.left += exmgn;
			dstmp.right = dstmp.left + scrnstat.height;
		}
		d3d_enter_criticalsection();
		if (d3d.d3ddev)
		{
			d3d.d3ddev->StretchRect(d3d.wabsurf, &src, d3d.backsurf, &dstmp, D3DTEXF_POINT);
		}
		d3d_leave_criticalsection();
	}else{
		if(!d3d.d3dbacksurf){
			// 生成要求
			scrnmng_create_pending = true;
			Sleep(1000);
		}
	}
#endif
}

// 描画領域の範囲をクライアント座標で取得
void scrnmngD3D_getrect(RECT *lpRect) {
	if (d3d.scrnmode & SCRNMODE_FULLSCREEN) {
		if (GetWindowLongPtr(g_hWndMain, NP2GWLP_HMENU)) {
			*lpRect = d3d.scrn;
		}
		else {
			*lpRect = d3d.scrnclip;
		}
	}
	else {
		*lpRect = d3d.scrn;
	}
}

#else

#ifndef __GNUC__
#include <winnls32.h>
#endif
#include "resource.h"
#include "np2.h"
#include "winloc.h"
#include "mousemng.h"
#include "scrnmng.h"
#include "scrnmng_d3d.h"
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

static	SCRNSURF	scrnsurf;

BRESULT scrnmngD3D_check() {return(FAILURE);}

void scrnmngD3D_initialize(void){}
BRESULT scrnmngD3D_create(UINT8 scrnmode){return 0;}
void scrnmngD3D_destroy(void){}

void scrnmngD3D_setwidth(int posx, int width){}
void scrnmngD3D_setextend(int extend){}
void scrnmngD3D_setheight(int posy, int height){}
const SCRNSURF *scrnmngD3D_surflock(void){return NULL;}
void scrnmngD3D_surfunlock(const SCRNSURF *surf){}
void scrnmngD3D_update(void){};

RGB16 scrnmngD3D_makepal16(RGB32 pal32){return 0;}


// ---- for windows

void scrnmngD3D_setmultiple(int multiple){}
int scrnmngD3D_getmultiple(void){return 0;}
void scrnmngD3D_querypalette(void){}
void scrnmngD3D_setdefaultres(void){}
void scrnmngD3D_setfullscreen(BOOL fullscreen){}
void scrnmngD3D_setrotatemode(UINT type){}
void scrnmngD3D_fullscrnmenu(int y){}
void scrnmngD3D_topwinui(void){}
void scrnmngD3D_clearwinui(void){}

void scrnmngD3D_entersizing(void){}
void scrnmngD3D_sizing(UINT side, RECT *rect){}
void scrnmngD3D_exitsizing(void){}

void scrnmngD3D_updatefsres(void){}
void scrnmngD3D_blthdc(HDC hdc){}
void scrnmngD3D_bltwab(void){}

void scrnmngD3D_getrect(RECT &lpRect) {}

#if defined(SUPPORT_DCLOCK)
BOOL scrnmngD3D_isdispclockclick(const POINT *pt){return FALSE;}
void scrnmngD3D_dispclock(void){}
#endif

#endif
