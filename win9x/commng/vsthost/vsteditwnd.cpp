/**
 * @file	vsteditwnd.cpp
 * @brief	VST edit ウィンドウ クラスの動作の定義を行います
 */

#include "compiler.h"
#include "vsteditwnd.h"
#include "vsteffect.h"

//! ウィンドウ クラス名
static const TCHAR s_szClassName[] = TEXT("VstEffectWnd");

//! インスタンス
HINSTANCE CVstEditWnd::sm_hInstance;

//! ウィンドウ マップ
std::map<HWND, CVstEditWnd*> CVstEditWnd::sm_wndMap;

/**
 * 初期化
 * @param[in] hInstance アプリケーションの現在のインスタンスのハンドル
 * @retval true 成功
 * @retval false 失敗
 */
bool CVstEditWnd::Initialize(HINSTANCE hInstance)
{
	sm_hInstance = hInstance;

	WNDCLASS ws;
	ZeroMemory(&ws, sizeof(ws));
	ws.style = CS_BYTEALIGNCLIENT | CS_HREDRAW | CS_VREDRAW;
	ws.lpfnWndProc = WndProc;
	ws.hInstance = hInstance;
//	ws.hIcon = ::LoadIcon(hInstance, MAKEINTRESOURCE(IDI_EDMUS));
	ws.hCursor = ::LoadCursor(NULL, IDC_ARROW);
	ws.hbrBackground = static_cast<HBRUSH>(::GetStockObject(NULL_BRUSH));
	ws.lpszClassName = s_szClassName;
	if (!::RegisterClass(&ws))
	{
		return false;
	}
	return true;
}

/**
 * アイドリング
 */
void CVstEditWnd::OnIdle()
{
	for (std::map<HWND, CVstEditWnd*>::iterator it = sm_wndMap.begin(); it != sm_wndMap.end(); ++it)
	{
		it->second->m_pEffect->idle();
	}
}

/**
 * コンストラクタ
 */
CVstEditWnd::CVstEditWnd()
	: m_hWnd(NULL)
	, m_pEffect(NULL)
{
}

/**
 * デストラクタ
 */
CVstEditWnd::~CVstEditWnd()
{
}

/**
 * 作成
 * @param[in] pEffect エフェクト
 * @param[in] lpszWindowName タイトル
 * @param[in] dwStyle スタイル
 * @param[in] x 位置
 * @param[in] y 位置
 * @retval true 成功
 * @retval false 失敗
 */
bool CVstEditWnd::Create(CVstEffect* pEffect, LPCTSTR lpszWindowName, DWORD dwStyle, LONG x, LONG y)
{
	if (pEffect == NULL)
	{
		return false;
	}
	ERect* lpRect = NULL;
	if ((!pEffect->editGetRect(&lpRect)) || (lpRect == NULL))
	{
		return false;
	}

	LONG nWidth = CW_USEDEFAULT;
	LONG nHeight = CW_USEDEFAULT;

	HWND hWnd = ::CreateWindow(s_szClassName, lpszWindowName, dwStyle, x, y, nWidth, nHeight, NULL, NULL, sm_hInstance, this);
	if (hWnd == NULL)
	{
		return false;
	}
	::ShowWindow(hWnd, SW_SHOWNORMAL);
	::UpdateWindow(hWnd);

	m_pEffect = pEffect;
	pEffect->Attach(this);
	if (!pEffect->editOpen(hWnd))
	{
		Destroy();
		return false;
	}
	return true;
}

/**
 * 破棄
 */
void CVstEditWnd::Destroy()
{
	if (m_pEffect)
	{
		m_pEffect->editClose();
		m_pEffect->Attach();
		m_pEffect = NULL;
	}
	if (m_hWnd)
	{
		::DestroyWindow(m_hWnd);
	}
}

/**
 * ウィンドウ プロシージャ
 * @param[in] hWnd ウィンドウ ハンドル
 * @param[in] message 処理される Windows メッセージを指定します
 * @param[in] wParam メッセージの処理で使う付加情報を提供します。このパラメータの値はメッセージに依存します
 * @param[in] lParam メッセージの処理で使う付加情報を提供します。このパラメータの値はメッセージに依存します
 * @return メッセージに依存する値を返します
 */
LRESULT CALLBACK CVstEditWnd::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	CVstEditWnd* pWnd = NULL;
	if (message == WM_CREATE)
	{
		LPCREATESTRUCT pCreate = reinterpret_cast<LPCREATESTRUCT>(lParam);
		pWnd = static_cast<CVstEditWnd*>(pCreate->lpCreateParams);
		pWnd->m_hWnd = hWnd;
		sm_wndMap[hWnd] = pWnd;
		::SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pWnd));
	}
	else
	{
		pWnd = reinterpret_cast<CVstEditWnd*>(::GetWindowLongPtr(hWnd, GWLP_USERDATA));
	}

	if (pWnd == NULL)
	{
		return ::DefWindowProc(hWnd, message, wParam, lParam);
	}

	LRESULT lResult = pWnd->WindowProc(message, wParam, lParam);

	if (message == WM_NCDESTROY)
	{
		sm_wndMap.erase(pWnd->m_hWnd);
		pWnd->m_hWnd = NULL;
		::SetWindowLongPtr(hWnd, GWLP_USERDATA, 0);
	}

	return lResult;
}

/**
 * CMainWnd オブジェクトの Windows プロシージャ (WindowProc) が用意されています
 * @param[in] message 処理される Windows メッセージを指定します
 * @param[in] wParam メッセージの処理で使う付加情報を提供します。このパラメータの値はメッセージに依存します
 * @param[in] lParam メッセージの処理で使う付加情報を提供します。このパラメータの値はメッセージに依存します
 * @return メッセージに依存する値を返します
 */
LRESULT CVstEditWnd::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_CREATE:
			return OnCreate(reinterpret_cast<LPCREATESTRUCT>(lParam));
			break;

		default:
			break;
	}
	return ::DefWindowProc(m_hWnd, message, wParam, lParam);
}

/**
 * フレームワークは、Windows のウィンドウは [作成] または CreateEx のメンバー関数を呼び出すことによって作成されたアプリケーションが必要とすると、このメンバー関数を呼び出します
 * @param[in] lpCreateStruct CREATESTRUCT
 * @retval 0 継続
 * @retval -1 破棄
 */
int CVstEditWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	HMENU hMenu = ::GetSystemMenu(m_hWnd, FALSE);
	if (hMenu)
	{
		::RemoveMenu(hMenu, SC_CLOSE, MF_BYCOMMAND);
	}
	return 0;
}

/**
 * リサイズ
 * @param[in] nWidth 幅
 * @param[in] nHeight 高さ
 * @retval true 成功
 * @retval false 失敗
 */
bool CVstEditWnd::OnResize(int nWidth, int nHeight)
{
	if (m_hWnd == NULL)
	{
		return false;
	}
	for (UINT i = 0; i < 2; i++)
	{
		RECT rectWindow;
		::GetWindowRect(m_hWnd, &rectWindow);

		RECT rectClient;
		::GetClientRect(m_hWnd, &rectClient);

		const LONG x = rectWindow.left;
		const LONG y = rectWindow.top;
		const LONG w = nWidth + (rectWindow.right - rectWindow.left) - (rectClient.right - rectClient.left);
		const LONG h = nHeight + (rectWindow.bottom - rectWindow.top) - (rectClient.bottom - rectClient.top);
		::MoveWindow(m_hWnd, x, y, w, h, TRUE);
	}
	return true;
}

/**
 * 再描画要求
 * @retval true 成功
 * @retval false 失敗
 */
bool CVstEditWnd::OnUpdateDisplay()
{
	return (m_hWnd != NULL);
}
