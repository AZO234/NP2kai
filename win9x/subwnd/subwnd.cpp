/**
 * @file	subwnd.cpp
 * @brief	サブ ウィンドウの基底クラスの動作の定義を行います
 */

#include "compiler.h"
#include "resource.h"
#include "subwnd.h"
#include "np2.h"
#include "soundmng.h"
#include "winloc.h"
#include "dialog/np2class.h"
#include "misc\tstring.h"

extern WINLOCEX np2_winlocexallwin(HWND base);

//! クラス名
static const TCHAR s_szClassName[] = TEXT("NP2-SubWnd");

/**
 * 初期化
 * @param[in] hInstance インスタンス
 */
void CSubWndBase::Initialize(HINSTANCE hInstance)
{
	WNDCLASS wc;
	ZeroMemory(&wc, sizeof(wc));
	wc.style = CS_BYTEALIGNCLIENT | CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wc.lpfnWndProc = ::DefWindowProc;
	wc.cbWndExtra = NP2GWLP_SIZE;
	wc.hInstance = hInstance;
	wc.hIcon = ::LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON2));
	wc.hCursor = ::LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = static_cast<HBRUSH>(::GetStockObject(NULL_BRUSH));
	wc.lpszClassName = s_szClassName;
	RegisterClass(&wc);
}

/**
 * コンストラクタ
 */
CSubWndBase::CSubWndBase()
	: m_wlex(NULL)
{
}

/**
 * デストラクタ
 */
CSubWndBase::~CSubWndBase()
{
}

/**
 * ウィンドウ作成
 * @param[in] nCaptionID キャプション ID
 * @param[in] dwStyle スタイル
 * @param[in] x X座標
 * @param[in] y Y座標
 * @param[in] nWidth 幅
 * @param[in] nHeight 高さ
 * @param[in] hwndParent 親ウィンドウ
 * @param[in] nIDorHMenu ID もしくは メニュー
 * @retval TRUE 成功
 * @retval FALSE 失敗
 */
BOOL CSubWndBase::Create(UINT nCaptionID, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hwndParent, HMENU nIDorHMenu)
{
	std::tstring rCaption(LoadTString(nCaptionID));
	return CreateEx(0, s_szClassName, rCaption.c_str(), dwStyle, x, y, nWidth, nHeight, hwndParent, nIDorHMenu);
}

/**
 * ウィンドウ作成
 * @param[in] lpCaption キャプション
 * @param[in] dwStyle スタイル
 * @param[in] x X座標
 * @param[in] y Y座標
 * @param[in] nWidth 幅
 * @param[in] nHeight 高さ
 * @param[in] hwndParent 親ウィンドウ
 * @param[in] nIDorHMenu ID もしくは メニュー
 * @retval TRUE 成功
 * @retval FALSE 失敗
 */
BOOL CSubWndBase::Create(LPCTSTR lpCaption, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hwndParent, HMENU nIDorHMenu)
{
	return CreateEx(0, s_szClassName, lpCaption, dwStyle, x, y, nWidth, nHeight, hwndParent, nIDorHMenu);
}

/**
 * ウィンドウ タイプの設定
 * @param[in] nType タイプ
 */
void CSubWndBase::SetWndType(UINT8 nType)
{
	WINLOCEX wlex = ::np2_winlocexallwin(g_hWndMain);
	winlocex_setholdwnd(wlex, m_hWnd);
	np2class_windowtype(m_hWnd, nType);
	winlocex_move(wlex);
	winlocex_destroy(wlex);
}

/**
 * CWndProc オブジェクトの Windows プロシージャ (WindowProc) が用意されています
 * @param[in] nMsg 処理される Windows メッセージを指定します
 * @param[in] wParam メッセージの処理で使う付加情報を提供します。このパラメータの値はメッセージに依存します
 * @param[in] lParam メッセージの処理で使う付加情報を提供します。このパラメータの値はメッセージに依存します
 * @return メッセージに依存する値を返します
 */
LRESULT CSubWndBase::WindowProc(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	switch (nMsg)
	{
		case WM_KEYDOWN:
		case WM_KEYUP:
			::SendMessage(g_hWndMain, nMsg, wParam, lParam);
			break;

		case WM_ENTERMENULOOP:
			CSoundMng::GetInstance()->Disable(SNDPROC_SUBWIND);
			break;

		case WM_EXITMENULOOP:
			CSoundMng::GetInstance()->Enable(SNDPROC_SUBWIND);
			break;

		case WM_ENTERSIZEMOVE:
			CSoundMng::GetInstance()->Disable(SNDPROC_SUBWIND);
			winlocex_destroy(m_wlex);
			m_wlex = np2_winlocexallwin(m_hWnd);
			break;

		case WM_MOVING:
			if (np2oscfg.WINSNAP) { // スナップ設定を共通に np21w ver0.86 rev22
				winlocex_moving(m_wlex, reinterpret_cast<RECT*>(lParam));
			}
			break;

		case WM_EXITSIZEMOVE:
			::winlocex_destroy(m_wlex);
			m_wlex = NULL;
			CSoundMng::GetInstance()->Enable(SNDPROC_SUBWIND);
			break;

		case WM_CLOSE:
			DestroyWindow();
			break;

		default:
			return CWndProc::WindowProc(nMsg, wParam, lParam);
	}
	return 0L;
}
