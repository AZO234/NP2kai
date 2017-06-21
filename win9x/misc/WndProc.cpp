/**
 * @file	WndProc.cpp
 * @brief	プロシージャ クラスの動作の定義を行います
 */

#include "compiler.h"
#include "WndProc.h"
#include <assert.h>
#define ASSERT assert	/*!< assert */

//! 基底クラス名
// static const TCHAR s_szClassName[] = TEXT("WndProcBase");

//! インスタンス
HINSTANCE CWndProc::sm_hInstance;
//! リソース
HINSTANCE CWndProc::sm_hResource;

DWORD CWndProc::sm_dwThreadId;						//!< 自分のスレッド ID
HHOOK CWndProc::sm_hHookOldCbtFilter;				//!< フック フィルター
CWndProc* CWndProc::sm_pWndInit;					//!< 初期化中のインスタンス
std::map<HWND, CWndProc*>* CWndProc::sm_pWndMap;	//!< ウィンドウ マップ

/**
 * 初期化
 * @param[in] hInstance インスタンス
 */
void CWndProc::Initialize(HINSTANCE hInstance)
{
	sm_hInstance = hInstance;
	sm_hResource = hInstance;

	sm_dwThreadId = ::GetCurrentThreadId();
	sm_hHookOldCbtFilter = ::SetWindowsHookEx(WH_CBT, CbtFilterHook, NULL, sm_dwThreadId);

	sm_pWndMap = new std::map<HWND, CWndProc*>;

#if 0
	WNDCLASS wc;
	ZeroMemory(&wc, sizeof(wc));
	wc.style = CS_BYTEALIGNCLIENT | CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.hIcon = ::LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON2));
	wc.hCursor = ::LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = static_cast<HBRUSH>(::GetStockObject(NULL_BRUSH));
	wc.lpszClassName = s_szClassName;
	::RegisterClass(&wc);
#endif	// 0
}

/**
 * 解放
 */
void CWndProc::Deinitialize()
{
	if (sm_hHookOldCbtFilter != NULL)
	{
		::UnhookWindowsHookEx(sm_hHookOldCbtFilter);
		sm_hHookOldCbtFilter = NULL;
	}
}

/**
 * リソースの検索
 * @param[in] lpszName リソース ID
 * @param[in] lpszType リソースの型へのポインタ
 * @return インスタンス
 */
HINSTANCE CWndProc::FindResourceHandle(LPCTSTR lpszName, LPCTSTR lpszType)
{
	HINSTANCE hInst = GetResourceHandle();
	if ((hInst != GetInstanceHandle()) && (::FindResource(hInst, lpszName, lpszType) != NULL))
	{
		return hInst;
	}
	return GetInstanceHandle();
}

/**
 * コンストラクタ
 */
CWndProc::CWndProc()
	: CWndBase(NULL)
	, m_pfnSuper(NULL)
{
}

/**
 * デストラクタ
 */
CWndProc::~CWndProc()
{
	DestroyWindow();
}

/**
 * ウィンドウのハンドルが指定されている場合、CWndProc オブジェクトへのポインターを返します
 * @param[in] hWnd ウィンドウ ハンドル
 * @return ポインタ
 */
CWndProc* CWndProc::FromHandlePermanent(HWND hWnd)
{
	std::map<HWND, CWndProc*>* pMap = sm_pWndMap;
	if (pMap)
	{
		std::map<HWND, CWndProc*>::iterator it = pMap->find(hWnd);
		if (it != pMap->end())
		{
			return it->second;
		}
	}
	return NULL;
}

/**
 * Windows のウィンドウをアタッチします
 * @param[in] hWndNew ウィンドウ ハンドル
 * @retval TRUE 成功
 * @retval FALSE 失敗
 */
BOOL CWndProc::Attach(HWND hWndNew)
{
	std::map<HWND, CWndProc*>* pMap = sm_pWndMap;
	if ((hWndNew != NULL) && (pMap))
	{
		(*pMap)[hWndNew] = this;
		m_hWnd = hWndNew;
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

/**
 * Windows のハンドルを切り離し、そのハンドルを返します
 * @return ウィンドウ ハンドル
 */
HWND CWndProc::Detach()
{
	HWND hWnd = m_hWnd;
	if (hWnd != NULL)
	{
		std::map<HWND, CWndProc*>* pMap = sm_pWndMap;
		if (pMap)
		{
			std::map<HWND, CWndProc*>::iterator it = pMap->find(hWnd);
			if (it != pMap->end())
			{
				pMap->erase(it);
			}
		}
		m_hWnd = NULL;
	}
	return hWnd;
}

/**
 * このメンバー関数はウィンドウがサブクラス化する前に、ほかのサブクラス化に必要な操作を許可するためにフレームワークから呼ばれます
 */
void CWndProc::PreSubclassWindow()
{
	// no default processing
}

/**
 * ウィンドウを動的サブクラス化し、CWnd オブジェクトに結び付けるためにこのメンバー関数を呼び出します
 * @param[in] hWnd ウィンドウ ハンドル
 * @retval TRUE 成功
 * @retval FALSE 失敗
 */
BOOL CWndProc::SubclassWindow(HWND hWnd)
{
	if (!Attach(hWnd))
	{
		return FALSE;
	}

	PreSubclassWindow();

	WNDPROC newWndProc = &CWndProc::WndProc;
	WNDPROC oldWndProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(newWndProc)));
	if ((m_pfnSuper == NULL) && (oldWndProc != newWndProc))
	{
		m_pfnSuper = oldWndProc;
	}
	return TRUE;
}

/**
 * コントロールを動的サブクラス化し、CWnd オブジェクトに結び付けるためにこのメンバー関数を呼び出します
 * @param[in] nID コントロール ID
 * @param[in] pParent コントロールの親
 * @retval TRUE 成功
 * @retval FALSE 失敗
 */
BOOL CWndProc::SubclassDlgItem(UINT nID, CWndProc* pParent)
{
	HWND hWndControl = ::GetDlgItem(pParent->m_hWnd, nID);
	if (hWndControl != NULL)
	{
		return SubclassWindow(hWndControl);
	}
	return FALSE;
}

/**
 * WndProc に元の値を設定して CWnd オブジェクトから HWND で識別されるウィンドウを切り離すために、このメンバー関数を呼び出します
 * @return 非サブクラス化されたウィンドウへのハンドル
 */
HWND CWndProc::UnsubclassWindow()
{
	if (m_pfnSuper != NULL)
	{
		::SetWindowLongPtr(m_hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(m_pfnSuper));
		m_pfnSuper = NULL;
	}
	return Detach();
}

/**
 * 指定されたウィンドウを作成し、それを CWndProc オブジェクトにアタッチします
 * @param[in] dwExStyle 拡張ウィンドウ スタイル
 * @param[in] lpszClassName 登録されているシステム ウィンドウ クラスの名前
 * @param[in] lpszWindowName ウィンドウの表示名
 * @param[in] dwStyle ウィンドウ スタイル
 * @param[in] x 画面または親ウィンドウの左端からウィンドウの初期位置までの水平方向の距離
 * @param[in] y 画面または親ウィンドウの上端からウィンドウの初期位置までの垂直方向の距離
 * @param[in] nWidth ウィンドウの幅 (ピクセル単位)
 * @param[in] nHeight ウィンドウの高さ (ピクセル単位)
 * @param[in] hwndParent 親ウィンドウへのハンドル
 * @param[in] nIDorHMenu ウィンドウ ID
 * @param[in] lpParam ユーザー データ
 * @retval TRUE 成功
 * @retval FALSE 失敗
 */
BOOL CWndProc::CreateEx(DWORD dwExStyle, LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hwndParent, HMENU nIDorHMenu, LPVOID lpParam)
{
	// 同じスレッドのみ許す
	if (sm_dwThreadId != ::GetCurrentThreadId())
	{
		PostNcDestroy();
		return FALSE;
	}

	CREATESTRUCT cs;
	cs.dwExStyle = dwExStyle;
	cs.lpszClass = lpszClassName;
	cs.lpszName = lpszWindowName;
	cs.style = dwStyle;
	cs.x = x;
	cs.y = y;
	cs.cx = nWidth;
	cs.cy = nHeight;
	cs.hwndParent = hwndParent;
	cs.hMenu = nIDorHMenu;
	cs.hInstance = sm_hInstance;
	cs.lpCreateParams = lpParam;

	if (!PreCreateWindow(cs))
	{
		PostNcDestroy();
		return FALSE;
	}

	HookWindowCreate(this);
	HWND hWnd = ::CreateWindowEx(cs.dwExStyle, cs.lpszClass, cs.lpszName, cs.style, cs.x, cs.y, cs.cx, cs.cy, cs.hwndParent, cs.hMenu, cs.hInstance, cs.lpCreateParams);
	if (!UnhookWindowCreate())
	{
		PostNcDestroy();
	}

	return (hWnd != NULL) ? TRUE : FALSE;
}

/**
 * CWnd オブジェクトに結び付けられた Windows のウィンドウが作成される前に、フレームワークから呼び出されます
 * @param[in,out] cs CREATESTRUCT の構造
 * @retval TRUE 継続
 */
BOOL CWndProc::PreCreateWindow(CREATESTRUCT& cs)
{
	return TRUE;
}

/**
 * ウィンドウ作成をフック
 * @param[in] pWnd ウィンドウ
 */
void CWndProc::HookWindowCreate(CWndProc* pWnd)
{
	// 同じスレッドのみ許す
	ASSERT(sm_dwThreadId == ::GetCurrentThreadId());

	if (sm_pWndInit == pWnd)
	{
		return;
	}

	ASSERT(sm_hHookOldCbtFilter != NULL);
	ASSERT(pWnd != NULL);
	ASSERT(pWnd->m_hWnd == NULL);
	ASSERT(sm_pWndInit == NULL);
	sm_pWndInit = pWnd;
}

/**
 * ウィンドウ作成をアンフック
 * @retval true フックした
 * @retval false フックしなかった
 */
bool CWndProc::UnhookWindowCreate()
{
	if (sm_pWndInit != NULL)
	{
		sm_pWndInit = NULL;
		return false;
	}
	return true;
}

/**
 * フック フィルタ
 * @param[in] nCode コード
 * @param[in] wParam パラメタ
 * @param[in] lParam パラメタ
 * @return リザルト コード
 */
LRESULT CALLBACK CWndProc::CbtFilterHook(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HCBT_CREATEWND)
	{
		HWND hWnd = reinterpret_cast<HWND>(wParam);
		LPCREATESTRUCT lpcs = reinterpret_cast<LPCBT_CREATEWND>(lParam)->lpcs;

		CWndProc* pWndInit = sm_pWndInit;
		if (pWndInit != NULL)
		{
			pWndInit->Attach(hWnd);
			pWndInit->PreSubclassWindow();

			WNDPROC newWndProc = &CWndProc::WndProc;
			WNDPROC oldWndProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(newWndProc)));
			if (oldWndProc != newWndProc)
			{
				pWndInit->m_pfnSuper = oldWndProc;
			}
			sm_pWndInit = NULL;
		}
	}
	return ::CallNextHookEx(sm_hHookOldCbtFilter, nCode, wParam, lParam);
}

/**
 * CWndProc オブジェクトに関連付けられた Windows のウィンドウを破棄します
 * @return ウィンドウが破棄された場合は 0 以外を返します。それ以外の場合は 0 を返します
 */
BOOL CWndProc::DestroyWindow()
{
	if (m_hWnd == NULL)
	{
		return FALSE;
	}

	CWndProc* pWnd = FromHandlePermanent(m_hWnd);

	const BOOL bResult = ::DestroyWindow(m_hWnd);

	if (pWnd == NULL)
	{
		Detach();
	}

	return bResult;
}

/**
 * ウィンドウ プロシージャ
 * @param[in] hWnd ウィンドウ ハンドル
 * @param[in] message 処理される Windows メッセージを指定します
 * @param[in] wParam メッセージの処理で使う付加情報を提供します。このパラメータの値はメッセージに依存します
 * @param[in] lParam メッセージの処理で使う付加情報を提供します。このパラメータの値はメッセージに依存します
 * @return メッセージに依存する値を返します
 */
LRESULT CALLBACK CWndProc::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	CWndProc* pWnd = FromHandlePermanent(hWnd);
	if (pWnd == NULL)
	{
		return ::DefWindowProc(hWnd, message, wParam, lParam);
	}
	else
	{
		return pWnd->WindowProc(message, wParam, lParam);
	}
}

/**
 * CWndProc オブジェクトの Windows プロシージャ (WindowProc) が用意されています
 * @param[in] nMsg 処理される Windows メッセージを指定します
 * @param[in] wParam メッセージの処理で使う付加情報を提供します。このパラメータの値はメッセージに依存します
 * @param[in] lParam メッセージの処理で使う付加情報を提供します。このパラメータの値はメッセージに依存します
 * @return メッセージに依存する値を返します
 */
LRESULT CWndProc::WindowProc(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	if (nMsg == WM_COMMAND)
	{
		if (OnCommand(wParam, lParam))
		{
			return 0;
		}
	}
	else if (nMsg == WM_NOTIFY)
	{
		NMHDR* pNMHDR = reinterpret_cast<NMHDR*>(lParam);
		if (pNMHDR->hwndFrom != NULL)
		{
			LRESULT lResult = 0;
			if (OnNotify(wParam, lParam, &lResult))
			{
				return lResult;
			}
		}
	}
	else if (nMsg == WM_NCDESTROY)
	{
		OnNcDestroy(wParam, lParam);
		return 0;
	}
	return DefWindowProc(nMsg, wParam, lParam);
}

/**
 * ユーザーがメニューの項目を選択したときに、フレームワークによって呼び出されます
 * @param[in] wParam パラメタ
 * @param[in] lParam パラメタ
 * @retval TRUE アプリケーションがこのメッセージを処理した
 */
BOOL CWndProc::OnCommand(WPARAM wParam, LPARAM lParam)
{
	return FALSE;
}

/**
 * フレームワークは、イベントがコントロールに発生する場合や、コントロールが一部の種類の情報を要求するコントロールを親ウィンドウに通知するために、このメンバー関数を呼び出します
 * @param[in] wParam メッセージがコントロールからそのメッセージを送信するコントロールを識別します
 * @param[in] lParam 通知コードと追加情報を含む通知メッセージ (NMHDR) の構造体へのポインター
 * @param[out] pResult メッセージが処理されたとき結果を格納するコードする LRESULT の変数へのポインター
 * @retval TRUE メッセージを処理した
 * @retval FALSE メッセージを処理しなかった
 */
BOOL CWndProc::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	return FALSE;
}

/**
 * Windows のウィンドウが破棄されるときに非クライアント領域が破棄されると、最後に呼び出されたメンバー関数は、フレームワークによって呼び出されます
 * @param[in] wParam パラメタ
 * @param[in] lParam パラメタ
 */
void CWndProc::OnNcDestroy(WPARAM wParam, LPARAM lParam)
{
	LONG_PTR pfnWndProc = ::GetWindowLongPtr(m_hWnd, GWLP_WNDPROC);
	DefWindowProc(WM_NCDESTROY, wParam, lParam);
	if (::GetWindowLong(m_hWnd, GWLP_WNDPROC) == pfnWndProc)
	{
		if (m_pfnSuper != NULL)
		{
			::SetWindowLongPtr(m_hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(m_pfnSuper));
			m_pfnSuper = NULL;
		}
	}
	Detach();

	// call special post-cleanup routine
	PostNcDestroy();
}

/**
 * ウィンドウが破棄された後に既定の OnNcDestroy のメンバー関数によって呼び出されます
 */
void CWndProc::PostNcDestroy()
{
}

/**
 * 既定のウィンドウ プロシージャを呼び出します
 * @param[in] nMsg 処理される Windows メッセージを指定します
 * @param[in] wParam メッセージ依存の追加情報を指定します
 * @param[in] lParam メッセージ依存の追加情報を指定します
 * @return 送られたメッセージに依存します
 */
LRESULT CWndProc::DefWindowProc(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	if (m_pfnSuper != NULL)
	{
		return ::CallWindowProc(m_pfnSuper, m_hWnd, nMsg, wParam, lParam);
	}
	else
	{
		return ::DefWindowProc(m_hWnd, nMsg, wParam, lParam);
	}
}
