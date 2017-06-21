/**
 * @file	toolwnd.h
 * @brief	ツール ウィンドウ クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

#include "subwnd.h"

enum {
	SKINMRU_MAX			= 4,
	FDDLIST_DRV			= 2,
	FDDLIST_MAX			= 8
};

typedef struct {
	int		insert;
	UINT	cnt;
	UINT	pos[FDDLIST_MAX];
	OEMCHAR	name[FDDLIST_MAX][MAX_PATH];
} TOOLFDD;

typedef struct {
	int		posx;
	int		posy;
	BOOL	type;
	TOOLFDD	fdd[FDDLIST_DRV];
	OEMCHAR	skin[MAX_PATH];
	OEMCHAR	skinmru[SKINMRU_MAX][MAX_PATH];
} NP2TOOL;

enum
{
	IDC_TOOLHDDACC			= 0,
	IDC_TOOLFDD1ACC,
	IDC_TOOLFDD1LIST,
	IDC_TOOLFDD1BROWSE,
	IDC_TOOLFDD1EJECT,
	IDC_TOOLFDD2ACC,
	IDC_TOOLFDD2LIST,
	IDC_TOOLFDD2BROWSE,
	IDC_TOOLFDD2EJECT,
	IDC_TOOLRESET,
	IDC_TOOLPOWER,
	IDC_MAXITEMS
};

/**
 * @brief ツール ウィンドウ クラス
 */
class CToolWnd : public CSubWndBase
{
public:
	static CToolWnd* GetInstance();
	CToolWnd();
	virtual ~CToolWnd();
	void Create();

protected:
	virtual LRESULT WindowProc(UINT nMsg, WPARAM wParam, LPARAM lParam);
	int OnCreate(LPCREATESTRUCT lpCreateStruct);
	void OnDestroy();
	void OnPaint();
	void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);

private:
	void OnDraw(BOOL redraw);
	void OpenPopUp(LPARAM lParam);
	void InitializeSubItems();
	void CreateSubItems();
	void DestroySubItems();
	void ChangeSkin();

public:
	HBITMAP			m_hbmp;
	UINT8			m_fddaccess[2];
	UINT8			m_hddaccess;
	UINT8			m_padding;
	HFONT			m_hfont;
	HDC				m_hdcfont;
	HBRUSH			m_access[2];
	HWND			m_sub[IDC_MAXITEMS];
	WNDPROC			m_subproc[IDC_MAXITEMS];
};

#define toolwin_create		CToolWnd::GetInstance()->Create
#define toolwin_destroy		CToolWnd::GetInstance()->DestroyWindow
#define toolwin_gethwnd		CToolWnd::GetInstance()->GetSafeHwnd

void toolwin_setfdd(UINT8 drv, const OEMCHAR *name);

#ifdef __cplusplus
extern "C"
{
#endif
void toolwin_fddaccess(UINT8 drv);
void toolwin_hddaccess(UINT8 drv);
#ifdef __cplusplus
}
#endif
void toolwin_draw(UINT8 frame);

void toolwin_readini();
void toolwin_writeini();
