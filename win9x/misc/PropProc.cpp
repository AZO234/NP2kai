/**
 * @file	PropProc.cpp
 * @brief	プロパティ シート クラスの動作の定義を行います
 */

#include "compiler.h"
#include "PropProc.h"

#if !defined(__GNUC__)
#pragma comment(lib, "comctl32.lib")
#endif	// !defined(__GNUC__)

// ---- プロパティ ページ

/**
 * コンストラクタ
 * @param[in] nIDTemplate このページに使用するテンプレートの ID
 * @param[in] nIDCaption このページのタブに設定される名前の ID
 */
CPropPageProc::CPropPageProc(UINT nIDTemplate, UINT nIDCaption)
{
	Construct(MAKEINTRESOURCE(nIDTemplate), nIDCaption);
}

/**
 * コンストラクタ
 * @param[in] lpszTemplateName このページのテンプレートの名前を含む文字列へのポインター
 * @param[in] nIDCaption このページのタブに設定される名前の ID
 */
CPropPageProc::CPropPageProc(LPCTSTR lpszTemplateName, UINT nIDCaption)
{
	Construct(lpszTemplateName, nIDCaption);
}

/**
 * デストラクタ
 */
CPropPageProc::~CPropPageProc()
{
	if (m_lpCaption)
	{
		free(m_lpCaption);
	}
}

/**
 * コンストラクト
 * @param[in] nIDTemplate このページに使用するテンプレートの ID
 * @param[in] nIDCaption このページのタブに設定される名前の ID
 */
void CPropPageProc::Construct(UINT nIDTemplate, UINT nIDCaption)
{
	Construct(MAKEINTRESOURCE(nIDTemplate), nIDCaption);
}

/**
 * コンストラクト
 * @param[in] lpszTemplateName このページのテンプレートの名前を含む文字列へのポインター
 * @param[in] nIDCaption このページのタブに設定される名前の ID
 */
void CPropPageProc::Construct(LPCTSTR lpszTemplateName, UINT nIDCaption)
{
	ZeroMemory(&m_psp, sizeof(m_psp));
	m_psp.dwSize = sizeof(m_psp);
	m_psp.dwFlags = PSP_USECALLBACK;
	m_psp.hInstance = FindResourceHandle(lpszTemplateName, RT_DIALOG);
	m_psp.pszTemplate = lpszTemplateName;
	m_psp.pfnDlgProc = DlgProc;
	m_psp.lParam = reinterpret_cast<LPARAM>(this);
	m_psp.pfnCallback = PropPageCallback;

	m_lpCaption = NULL;
	if (nIDCaption)
	{
		std::tstring rTitle(LoadTString(nIDCaption));
		m_lpCaption = _tcsdup(rTitle.c_str());
		m_psp.pszTitle = m_lpCaption;
		m_psp.dwFlags |= PSP_USETITLE;
	}
}

/**
 * プロパティ ページ プロシージャ
 * @param[in] hWnd ウィンドウ ハンドル
 * @param[in] message メッセージ
 * @param[in] pPropPage このプロパティ シート ページのポインタ
 * @return 0
 */
UINT CALLBACK CPropPageProc::PropPageCallback(HWND hWnd, UINT message, LPPROPSHEETPAGE pPropPage)
{
	switch (message)
	{
		case PSPCB_CREATE:
			HookWindowCreate(reinterpret_cast<CPropPageProc*>(pPropPage->lParam));
			return TRUE;

		case PSPCB_RELEASE:
			UnhookWindowCreate();
			break;
	}
	return 0;
}

/**
 * フレームワークは、イベントがコントロールに発生する場合や、コントロールが一部の種類の情報を要求するコントロールを親ウィンドウに通知するために、このメンバー関数を呼び出します
 * @param[in] wParam メッセージがコントロールからそのメッセージを送信するコントロールを識別します
 * @param[in] lParam 通知コードと追加情報を含む通知メッセージ (NMHDR) の構造体へのポインター
 * @param[out] pResult メッセージが処理されたとき結果を格納するコードする LRESULT の変数へのポインター
 * @retval TRUE メッセージを処理した
 * @retval FALSE メッセージを処理しなかった
 */
BOOL CPropPageProc::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	NMHDR* pNMHDR = reinterpret_cast<NMHDR*>(lParam);

	// allow message map to override
	if (CDlgProc::OnNotify(wParam, lParam, pResult))
	{
		return TRUE;
	}

	// don't handle messages not from the page/sheet itself
	if (pNMHDR->hwndFrom != m_hWnd && pNMHDR->hwndFrom != ::GetParent(m_hWnd))
	{
		return FALSE;
	}

	// handle default
	switch (pNMHDR->code)
	{
		case PSN_APPLY:
			*pResult = OnApply() ? PSNRET_NOERROR : PSNRET_INVALID_NOCHANGEPAGE;
			break;

		case PSN_RESET:
			OnReset();
			break;

		default:
			return FALSE;   // not handled
	}

	return TRUE;    // handled
}

/**
 * このメンバー関数は、フレームワークによって OnKillActiveフレームワークがを呼び出した直後にユーザーが[OK]を選択するか、更新時に呼び出されます
 * @retval TRUE 変更が承認された
 * @retval FALSE 変更が承認されなかった
 */
BOOL CPropPageProc::OnApply()
{
	OnOK();
	return TRUE;
}

/**
 * このメンバー関数は、フレームワークによってユーザーが[キャンセル]を選択するときに呼び出されます。
 */
void CPropPageProc::OnReset()
{
	OnCancel();
}

/**
 * このメンバー関数は、フレームワークによって OnKillActiveフレームワークがを呼び出した直後にユーザーが[OK]を選択するか、更新時に呼び出されます
 */
void CPropPageProc::OnOK()
{
}

/**
 * このメンバー関数は、フレームワークで[キャンセル]ボタンが選択されたときに呼び出されます
 */
void CPropPageProc::OnCancel()
{
}

// ---- プロパティ シート

/**
 * コンストラクタ
 */
CPropSheetProc::CPropSheetProc()
{
	CommonConstruct(NULL, 0);
}

/**
 * コンストラクタ
 * @param[in] nIDCaption キャプション
 * @param[in] hwndParent 親ウィンドウ
 * @param[in] iSelectPage スタート ページ
 */
CPropSheetProc::CPropSheetProc(UINT nIDCaption, HWND hwndParent, UINT iSelectPage)
{
	m_strCaption = LoadTString(nIDCaption);
	CommonConstruct(hwndParent, iSelectPage);
}

/**
 * コンストラクタ
 * @param[in] pszCaption キャプション
 * @param[in] hwndParent 親ウィンドウ
 * @param[in] iSelectPage スタート ページ
 */
CPropSheetProc::CPropSheetProc(LPCTSTR pszCaption, HWND hwndParent, UINT iSelectPage)
{
	m_strCaption = pszCaption;
	CommonConstruct(hwndParent, iSelectPage);
}

/**
 * コンストラクト
 * @param[in] hwndParent 親ウィンドウ
 * @param[in] iSelectPage スタート ページ
 */
void CPropSheetProc::CommonConstruct(HWND hwndParent, UINT iSelectPage)
{
	ZeroMemory(&m_psh, sizeof(m_psh));
	m_psh.dwSize = sizeof(m_psh);
	m_psh.hwndParent = hwndParent;
	m_psh.hInstance = CWndProc::GetResourceHandle();
	m_psh.nStartPage = iSelectPage;
}

/**
 * モーダル
 * @return リザルト コード
 */
INT_PTR CPropSheetProc::DoModal()
{
	m_psh.pszCaption = m_strCaption.c_str();
	m_psh.nPages = static_cast<UINT>(m_pages.size());
	m_psh.phpage = new HPROPSHEETPAGE[m_psh.nPages];
	for (UINT i = 0; i < m_pages.size(); i++)
	{
		m_psh.phpage[i] = ::CreatePropertySheetPage(&m_pages[i]->m_psp);
	}

	const INT_PTR r = ::PropertySheet(&m_psh);

	delete[] m_psh.phpage;
	m_psh.phpage = NULL;

	return r;
}

/**
 * ページの追加
 * @param[in] pPage ページ
 */
void CPropSheetProc::AddPage(CPropPageProc* pPage)
{
	m_pages.push_back(pPage);
}
