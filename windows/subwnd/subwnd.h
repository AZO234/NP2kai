/**
 * @file	subwnd.h
 * @brief	サブ ウィンドウの基底クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

#include "winloc.h"
#include "misc\WndProc.h"

/**
 * @brief サブ ウィンドウの基底クラス
 */
class CSubWndBase : public CWndProc
{
public:
	static void Initialize(HINSTANCE hInstance);

	CSubWndBase();
	virtual ~CSubWndBase();
	BOOL Create(UINT nCaptionID, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hwndParent, HMENU nIDorHMenu);
	BOOL Create(LPCTSTR lpCaption, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hwndParent, HMENU nIDorHMenu);
	void SetWndType(UINT8 nType);

protected:
	virtual LRESULT WindowProc(UINT nMsg, WPARAM wParam, LPARAM lParam);

private:
	WINLOCEX m_wlex;		/*!< ウィンドウ ロケータ */
};
