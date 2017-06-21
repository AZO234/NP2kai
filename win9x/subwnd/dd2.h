/**
 * @file	dd2.h
 * @brief	DirectDraw2 描画クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

#include <ddraw.h>
#include "cmndraw.h"

/**
 * @brief DirectDraw2 class
 */
class DD2Surface
{
public:
	DD2Surface();
	~DD2Surface();

	bool Create(HWND hWnd, int nWidth, int nHeight);
	void Release();
	CMNVRAM* Lock();
	void Unlock();
	void Blt(const POINT* pt, const RECT* lpRect = NULL);
	UINT16 GetPalette16(RGB32 pal) const;

protected:
	HWND					m_hWnd;				/*!< ウィンドウ ハンドル */
	LPDIRECTDRAW			m_pDDraw;			/*!< DirectDraw インスタンス */
	LPDIRECTDRAW2			m_pDDraw2;			/*!< DirectDraw2 インスタンス */
	LPDIRECTDRAWSURFACE		m_pPrimarySurface;	/*!< プライマリ サーフェス */
	LPDIRECTDRAWSURFACE		m_pBackSurface;		/*!< バック サーフェス */
	LPDIRECTDRAWCLIPPER		m_pClipper;			/*!< クリッパー */
	LPDIRECTDRAWPALETTE		m_pPalette;			/*!< パレット */
	RGB32					m_pal16;			/*!< 16BPPマスク */
	UINT8					m_r16b;				/*!< B シフト量 */
	UINT8					m_l16r;				/*!< R シフト量 */
	UINT8					m_l16g;				/*!< G シフト量 */
	CMNVRAM					m_vram;				/*!< VRAM */
	PALETTEENTRY			m_pal[256];			/*!< パレット */
};
