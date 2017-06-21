/**
 * @file	DlgProc.cpp
 * @brief	ダイアログ クラスの動作の定義を行います
 */

#include "compiler.h"
#include "DlgProc.h"

/**
 * コンストラクタ
 */
CDlgProc::CDlgProc()
	: m_lpszTemplateName(NULL)
	, m_hwndParent(NULL)
{
}

/**
 * コンストラクタ
 * @param[in] nIDTemplate ダイアログ ボックス テンプレートのリソース id 番号を指定します
 * @param[in] hwndParent 親ウィンドウ
 */
CDlgProc::CDlgProc(UINT nIDTemplate, HWND hwndParent)
	: m_lpszTemplateName(MAKEINTRESOURCE(nIDTemplate))
	, m_hwndParent(hwndParent)
{
}

/**
 * デストラクタ
 */
CDlgProc::~CDlgProc()
{
}

/**
 * モーダル
 * @return ダイアログ ボックスを閉じるために使用される、CDialog::EndDialog のメンバー関数に渡された nResult のパラメーター値を指定する int の値
 */
INT_PTR CDlgProc::DoModal()
{
	HookWindowCreate(this);

	HINSTANCE hInstance = FindResourceHandle(m_lpszTemplateName, RT_DIALOG);
	const INT_PTR nRet = ::DialogBox(hInstance, m_lpszTemplateName, m_hwndParent, DlgProc);

	if (!UnhookWindowCreate())
	{
		PostNcDestroy();
	}
	return nRet;
}

/**
 * ダイアログ プロシージャ
 * @param[in] hWnd ウィンドウ ハンドル
 * @param[in] message 処理される Windows メッセージを指定します
 * @param[in] wParam メッセージの処理で使う付加情報を提供します。このパラメータの値はメッセージに依存します
 * @param[in] lParam メッセージの処理で使う付加情報を提供します。このパラメータの値はメッセージに依存します
 * @return メッセージに依存する値を返します
 */
#if defined(_WIN64)
INT_PTR CALLBACK CDlgProc::DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
#else	// defined(_WIN64)
BOOL CALLBACK CDlgProc::DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
#endif	// defined(_WIN64)
{
	if (message == WM_INITDIALOG)
	{
		CDlgProc* pDlg = static_cast<CDlgProc*>(FromHandlePermanent(hWnd));
		if (pDlg != NULL)
		{
			return pDlg->OnInitDialog();
		}
		else
		{
			return TRUE;
		}
	}
	return FALSE;
}

/**
 * CDlgProc オブジェクトの Windows プロシージャ
 * @param[in] nMsg 処理される Windows メッセージを指定します
 * @param[in] wParam メッセージの処理で使う付加情報を提供します。このパラメータの値はメッセージに依存します
 * @param[in] lParam メッセージの処理で使う付加情報を提供します。このパラメータの値はメッセージに依存します
 * @return メッセージに依存する値を返します
 */
LRESULT CDlgProc::WindowProc(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	if (nMsg == WM_COMMAND)
	{
		switch (LOWORD(wParam))
		{
			case IDOK:
				OnOK();
				return TRUE;

			case IDCANCEL:
				OnCancel();
				return TRUE;
		}
	}
	return CWndProc::WindowProc(nMsg, wParam, lParam);
}

/**
 * このメソッドは WM_INITDIALOG のメッセージに応答して呼び出されます
 * @retval TRUE 最初のコントロールに入力フォーカスを設定
 * @retval FALSE 既に設定済
 */
BOOL CDlgProc::OnInitDialog()
{
	return TRUE;
}

/**
 * ユーザーが OK のボタン (IDOK ID がのボタン) をクリックすると呼び出されます
 */
void CDlgProc::OnOK()
{
	EndDialog(IDOK);
}

/**
 * フレームワークは、ユーザーが [キャンセル] をクリックするか、モーダルまたはモードレス ダイアログ ボックスの Esc キーを押したときにこのメソッドを呼び出します
 */
void CDlgProc::OnCancel()
{
	EndDialog(IDCANCEL);
}



/**
 * コンストラクタ
 * @param[in] bOpenFileDialog 作成するダイアログ ボックスを指定するパラメーター
 * @param[in] lpszDefExt 既定のファイル名の拡張子です
 * @param[in] lpszFileName ボックスに表示される初期ファイル名
 * @param[in] dwFlags フラグ
 * @param[in] lpszFilter フィルター
 * @param[in] hParentWnd 親ウィンドウ
 */
CFileDlg::CFileDlg(BOOL bOpenFileDialog, LPCTSTR lpszDefExt, LPCTSTR lpszFileName, DWORD dwFlags, LPCTSTR lpszFilter, HWND hParentWnd)
	: m_bOpenFileDialog(bOpenFileDialog)
{
	ZeroMemory(&m_ofn, sizeof(m_ofn));
	m_szFileName[0] = '\0';
	m_szFileTitle[0] = '\0';

	m_ofn.lStructSize = sizeof(m_ofn);
	m_ofn.lpstrFile = m_szFileName;
	m_ofn.nMaxFile = _countof(m_szFileName);
	m_ofn.lpstrDefExt = lpszDefExt;
	m_ofn.lpstrFileTitle = m_szFileTitle;
	m_ofn.nMaxFileTitle = _countof(m_szFileTitle);
	m_ofn.Flags = dwFlags;
	m_ofn.hwndOwner = hParentWnd;

	// setup initial file name
	if (lpszFileName != NULL)
	{
		lstrcpyn(m_szFileName, lpszFileName, _countof(m_szFileName));
	}

	// Translate filter into commdlg format (lots of \0)
	if (lpszFilter != NULL)
	{
		m_strFilter = lpszFilter;
		for (std::tstring::iterator it = m_strFilter.begin(); it != m_strFilter.end(); ++it)
		{
#if !defined(_UNICODE)
			if (IsDBCSLeadByte(static_cast<BYTE>(*it)))
			{
				++it;
				if (it == m_strFilter.end())
				{
					break;
				}
				continue;
			}
#endif	// !defined(_UNICODE)
			if (*it == '|')
			{
				*it = '\0';
			}
		}
		m_ofn.lpstrFilter = m_strFilter.c_str();
	}
}

/**
 * モーダル
 * @return リザルト コード
 */
int CFileDlg::DoModal()
{
	int nResult;
	if (m_bOpenFileDialog)
	{
		nResult = ::GetOpenFileName(&m_ofn);
	}
	else
	{
		nResult = ::GetSaveFileName(&m_ofn);
	}
	return nResult;
}
