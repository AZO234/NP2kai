/**
 * @file	wab.h
 * @brief	Window Accelerator Board Interface
 *
 * @author	$Author: SimK $
 */

#pragma once

#if defined(NP2_X11)
#include <gtk/gtk.h>
#endif

// XXX: 回転させても1600x1600以上にならないので差し当たってはこれで十分
#define WAB_MAX_WIDTH	1600
#define WAB_MAX_HEIGHT	1600

#ifdef __cplusplus
extern "C" {
#endif
	
typedef struct {
	int		posx;
	int		posy;
	int		multiwindow;
	int		multithread;
	int		halftone;
	int		forcevga;
	int		readonly; // from np2oscfg
} NP2WABCFG;

typedef void NP2WAB_DrawFrame();
typedef struct {
	REG8 relay; // 画面出力リレー状態（bit0=内蔵ウィンドウアクセラレータ, bit1=RGB INスルー, それ以外のビットはReserved。bit0,1が00で98グラフィック
	REG8 paletteChanged; // パレット要更新フラグ
	int realWidth; // 画面解像度(幅)
	int realHeight; // 画面解像度(高さ)
	int wndWidth; // 描画領域サイズ(幅)
	int wndHeight; // 描画領域サイズ(高さ)
	int fps; // リフレッシュレート（大体合わせてくれるかもしれない･･･けど現時点で何もしていない）
	int lastWidth; // 前回のウィンドウアクセラレータの横解像度（デバイス再作成判定用）
	int lastHeight; // 前回のウィンドウアクセラレータの横解像度（デバイス再作成判定用）
	
	int	relaystateint;
	int	relaystateext;

	int vramoffs;
} NP2WAB;

typedef struct {
	int multiwindow; // 別窓モード
	int ready; // 0以外なら描いても良いよ
#if defined(NP2_SDL2) || defined(__LIBRETRO__)
	unsigned int* pBuffer;
#elif defined(NP2_X11)
	GtkWidget *pWABWnd;
	GdkPixbuf *pPixbuf;
#else
	HWND hWndMain; // メインウィンドウのハンドル
	HWND hWndWAB; // ウィンドウアクセラレータ別窓のハンドル
	HDC hDCWAB; // ウィンドウアクセラレータ別窓のHDC
	HBITMAP hBmpBuf; // バッファビットマップ（常に等倍）
	HDC     hDCBuf; // バッファのHDC
#endif
	NP2WAB_DrawFrame *drawframe; // 画面描画関数。hDCBufにアクセラレータ画面データを転送する。
} NP2WABWND;

#if defined(NP2_SDL2) || defined(NP2_X11) || defined(__LIBRETRO__)
void np2wab_init(void);
#else
void np2wab_init(HINSTANCE hInstance, HWND g_hWndMain);
#endif
void np2wab_reset(const NP2CFG *pConfig);
void np2wab_bind(void);
void np2wab_drawframe(void);
void np2wab_shutdown(void);

void np2wab_setRelayState(REG8 state);
void np2wab_setScreenSize(int width, int height);
void np2wab_setScreenSizeMT(int width, int height);

void wabwin_readini();
void wabwin_writeini();

extern NP2WAB		np2wab;
extern NP2WABCFG	np2wabcfg;
extern NP2WABWND	np2wabwnd;
//
//extern int		np2wab.relaystateint;
//extern int		np2wab.relaystateext;

#ifdef __cplusplus
}
#endif

