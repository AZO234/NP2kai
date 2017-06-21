/**
 * @file	WndProc.h
 * @brief	プロシージャ クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

#include <map>
#include "WndBase.h"

/**
 * @brief プロシージャ クラス
 */
class CWndProc : public CWndBase
{
public:
	static void Initialize(HINSTANCE hInstance);
	static void Deinitialize();
	static HINSTANCE GetInstanceHandle();
	static void SetResourceHandle(HINSTANCE hInstance);
	static HINSTANCE GetResourceHandle();
	static HINSTANCE FindResourceHandle(LPCTSTR lpszName, LPCTSTR lpszType);

	CWndProc();
	virtual ~CWndProc();

	operator HWND() const;
	HWND GetSafeHwnd() const;
	static CWndProc* FromHandlePermanent(HWND hWnd);
	BOOL Attach(HWND hWndNew);
	HWND Detach();

	virtual void PreSubclassWindow();
	BOOL SubclassWindow(HWND hWnd);
	BOOL SubclassDlgItem(UINT nID, CWndProc* pParent);
	HWND UnsubclassWindow();

	BOOL CreateEx(DWORD dwExStyle, LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hwndParent, HMENU nIDorHMenu, LPVOID lpParam = NULL);
	virtual BOOL DestroyWindow();
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

protected:
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	virtual void OnNcDestroy(WPARAM wParam, LPARAM lParam);
	virtual LRESULT WindowProc(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT DefWindowProc(UINT nMsg, WPARAM wParam, LPARAM lParam);
	virtual void PostNcDestroy();

protected:
	static HINSTANCE sm_hInstance;		//!< インスタンス ハンドル
	static HINSTANCE sm_hResource;		//!< リソース ハンドル
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	static void HookWindowCreate(CWndProc* pWnd);
	static bool UnhookWindowCreate();

private:
	static DWORD sm_dwThreadId;						//!< 自分のスレッド ID
	static HHOOK sm_hHookOldCbtFilter;				//!< フック フィルター
	static CWndProc* sm_pWndInit;					//!< 初期化中のインスタンス
	static std::map<HWND, CWndProc*>* sm_pWndMap;	//!< ウィンドウ マップ
	WNDPROC m_pfnSuper;								//!< 下位プロシージャ
	static LRESULT CALLBACK CbtFilterHook(int nCode, WPARAM wParam, LPARAM lParam);
};

/**
 * インスタンス ハンドルを取得
 * @return インスタンス ハンドル
 */
inline HINSTANCE CWndProc::GetInstanceHandle()
{
	return sm_hInstance;
}

/**
 * リソース ハンドルを設定
 * @param[in] hInstance リソース ハンドル
 */
inline void CWndProc::SetResourceHandle(HINSTANCE hInstance)
{
	sm_hResource = hInstance;
}

/**
 * リソース ハンドルを取得
 * @return リソース ハンドル
 */
inline HINSTANCE CWndProc::GetResourceHandle()
{
	return sm_hResource;
}

/**
 * HWND オペレータ
 * @return HWND
 */
inline CWndProc::operator HWND() const
{
	return m_hWnd;
}

/**
 * ウィンドウのウィンドウ ハンドルを返します
 * @return ウィンドウ ハンドル
 */
inline HWND CWndProc::GetSafeHwnd() const
{
	return (this != NULL) ? m_hWnd : NULL;
}

