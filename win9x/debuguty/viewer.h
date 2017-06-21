/**
 * @file	viewer.h
 * @brief	DebugUty 用ビューワ クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

#include "..\misc\WndProc.h"

class CDebugUtyItem;

//! ビュー最大数
#define NP2VIEW_MAX		8

/**
 * @brief ビュー クラス
 */
class CDebugUtyView : public CWndProc
{
public:
	static void Initialize(HINSTANCE hInstance);
	static void New();
	static void AllClose();
	static void AllUpdate(bool bForce);

	CDebugUtyView();
	virtual ~CDebugUtyView();
	void UpdateCaption();
	UINT32 GetVScrollPos() const;
	void SetVScrollPos(UINT nPos);
	void SetVScroll(UINT nPos, UINT nLines);
	void UpdateVScroll();

protected:
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	int OnCreate(LPCREATESTRUCT lpCreateStruct);
	void OnSize(UINT nType, int cx, int cy);
	void OnPaint();
	void OnVScroll(UINT nSBCode, UINT nPos, HWND hwndScrollBar);
	void OnEnterMenuLoop(BOOL bIsTrackPopupMenu);
	void OnActivate(UINT nState, HWND hwndOther, BOOL bMinimized);
	virtual void PostNcDestroy();

private:
	bool m_bActive;				//!< アクティブ フラグ
	UINT m_nVPos;				//!< 位置
	UINT m_nVLines;				//!< ライン数
	UINT m_nVPage;				//!< 1ページの表示数
	UINT m_nVMultiple;			//!< 倍率
	CDebugUtyItem* m_lpItem;	//!< 表示アイテム
	static DWORD sm_dwLastTick;	//!< 最後のTick
	void SetMode(UINT nID);
	void SetSegmentItem(HMENU hMenu, int nId, LPCTSTR lpSegment, UINT nSegment);
	void UpdateView();
	static void UpdateActive();
};

/**
 * 現在の位置を返す
 * @return 位置
 */
inline UINT32 CDebugUtyView::GetVScrollPos() const
{
	return m_nVPos;
}
