/**
 * @file	d_about.cpp
 * @brief	バージョン情報ダイアログ
 */

#include "compiler.h"
#include "resource.h"
#include "dialog.h"
#include "np2class.h"
#include "np2.h"
#include "misc/DlgProc.h"
#include "pccore.h"
#include "np2ver.h"
#include "generic/np2info.h"

//! タイトル
static const TCHAR s_np2title[] = TEXT(PROJECTNAME) TEXT(PROJECTSUBNAME) TEXT("  ");

//! 情報
static const TCHAR s_np2infostr[] = TEXT("CPU: %CPU% %CLOCK%\nMEM: %MEM1%\nGDC: %GDC%\n     %GDC2%\nTEXT: %TEXT%\nGRPH: %GRPH%\nSOUND: %EXSND%\n\nBIOS: %BIOS%\nRHYTHM: %RHYTHM%\n\nSCREEN: %DISP%");

/**
 * @brief バージョン情報ダイアログ
 * @param[in] hwndParent 親ウィンドウ
 */
class CAboutDlg : public CDlgProc
{
public:
	CAboutDlg(HWND hwndParent);

protected:
	virtual BOOL OnInitDialog();
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

private:
	SIZE m_szAbout;			//!< ウィンドウのサイズ
	void GetDlgItemRect(UINT nID, RECT& rect);
};

/**
 * コンストラクタ
 * @param[in] hwndParent 親ウィンドウ
 */
CAboutDlg::CAboutDlg(HWND hwndParent)
	: CDlgProc(IDD_ABOUT, hwndParent)
{
	ZeroMemory(&m_szAbout, sizeof(m_szAbout));
}

/**
 * このメソッドは WM_INITDIALOG のメッセージに応答して呼び出されます
 * @retval TRUE 最初のコントロールに入力フォーカスを設定
 * @retval FALSE 既に設定済
 */
BOOL CAboutDlg::OnInitDialog()
{
	TCHAR szWork[128];
	milstr_ncpy(szWork, s_np2title, _countof(szWork));
	milstr_ncat(szWork, np2version, _countof(szWork));
#if defined(NP2VER_WIN9X)
	milstr_ncat(szWork, NP2VER_WIN9X, _countof(szWork));
#endif
	SetDlgItemText(IDC_NP2VER, szWork);

	RECT rect;
	GetWindowRect(&rect);
	m_szAbout.cx = rect.right - rect.left;
	m_szAbout.cy = rect.bottom - rect.top;


	RECT rectMore;
	GetDlgItemRect(IDC_MORE, rectMore);
	RECT rectInfo;
	GetDlgItemRect(IDC_NP2INFO, rectInfo);
	const int nHeight = m_szAbout.cy - (rectInfo.bottom - rectMore.bottom);

	CWndBase wndParent = GetParent();
	wndParent.GetClientRect(&rect);

	POINT pt;
	pt.x = (rect.right - rect.left - m_szAbout.cx) / 2;
	pt.y = (rect.bottom - rect.top - m_szAbout.cy) / 2;
	wndParent.ClientToScreen(&pt);
	np2class_move(m_hWnd, pt.x, pt.y, m_szAbout.cx, nHeight);

	GetDlgItem(IDOK).SetFocus();

	return TRUE;
}

/**
 * ユーザーがメニューの項目を選択したときに、フレームワークによって呼び出されます
 * @param[in] wParam パラメタ
 * @param[in] lParam パラメタ
 * @retval TRUE アプリケーションがこのメッセージを処理した
 */
BOOL CAboutDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if (LOWORD(wParam) == IDC_MORE)
	{
		TCHAR szInfo[1024];
		np2info(szInfo, s_np2infostr, _countof(szInfo), NULL);

		SetDlgItemText(IDC_NP2INFO, szInfo);
		GetDlgItem(IDC_MORE).EnableWindow(FALSE);

		RECT rect;
		GetWindowRect(&rect);
		np2class_move(m_hWnd, rect.left, rect.top, m_szAbout.cx, m_szAbout.cy);
		GetDlgItem(IDOK).SetFocus();

		return TRUE;
	}
	return FALSE;
}

/**
 * アイテムの領域を得る
 * @param[in] nID ID
 * @param[out] rect 領域
 */
void CAboutDlg::GetDlgItemRect(UINT nID, RECT& rect)
{
	CWndBase wnd = GetDlgItem(nID);
	wnd.GetWindowRect(&rect);
	::MapWindowPoints(HWND_DESKTOP, m_hWnd, reinterpret_cast<LPPOINT>(&rect), 2);
}

/**
 * バージョン情報ダイアログ
 * @param[in] hwndParent 親ウィンドウ
 */
void dialog_about(HWND hwndParent)
{
	CAboutDlg dlg(hwndParent);
	dlg.DoModal();
}
