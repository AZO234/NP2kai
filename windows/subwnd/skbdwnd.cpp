/**
 * @file	skbdwnd.cpp
 * @brief	ソフトウェア キーボード クラスの動作の定義を行います
 */

#include "compiler.h"
#include "resource.h"
#include "skbdwnd.h"
#include "np2.h"
#include "ini.h"
#include "sysmng.h"
#include "dialog/np2class.h"
#include "generic/softkbd.h"
#include "menu.h"

#if defined(SUPPORT_SOFTKBD)

//! 唯一のインスタンスです
CSoftKeyboardWnd CSoftKeyboardWnd::sm_instance;

/**
 * @brief コンフィグ
 */
struct SoftKeyboardConfig
{
	int		posx;		//!< X
	int		posy;		//!< Y
	int		width;		//!< Width  0=Invalid
	int		height;		//!< Height 0=Invalid
	UINT8	type;		//!< ウィンドウ タイプ
};

//! コンフィグ
static SoftKeyboardConfig s_skbdcfg;

//! タイトル
static const TCHAR s_skbdapp[] = TEXT("Soft Keyboard");

/**
 * 設定
 */
static const PFTBL s_skbdini[] =
{
	PFVAL("WindposX", PFTYPE_SINT32,	&s_skbdcfg.posx),
	PFVAL("WindposY", PFTYPE_SINT32,	&s_skbdcfg.posy),
	PFVAL("WindsizW", PFTYPE_SINT32,	&s_skbdcfg.width),
	PFVAL("WindsizH", PFTYPE_SINT32,	&s_skbdcfg.height),
	PFVAL("windtype", PFTYPE_BOOL,		&s_skbdcfg.type)
};

/**
 * 初期化
 */
void CSoftKeyboardWnd::Initialize()
{
	softkbd_initialize();
}

/**
 * 解放
 */
void CSoftKeyboardWnd::Deinitialize()
{
	softkbd_deinitialize();
}

/**
 * コンストラクタ
 */
CSoftKeyboardWnd::CSoftKeyboardWnd()
	: m_nWidth(0)
	, m_nHeight(0)
{
}

/**
 * デストラクタ
 */
CSoftKeyboardWnd::~CSoftKeyboardWnd()
{
}

/**
 * 作成
 */
void CSoftKeyboardWnd::Create()
{
	if (m_hWnd != NULL)
	{
		return;
	}

	if (softkbd_getsize(&m_nWidth, &m_nHeight) != SUCCESS)
	{
		return;
	}

	if (s_skbdcfg.width == 0) s_skbdcfg.width = m_nWidth;
	if (s_skbdcfg.height == 0) s_skbdcfg.height = m_nHeight;

	if (!CSubWndBase::Create(IDS_CAPTION_SOFTKEY, WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX | WS_SIZEBOX, s_skbdcfg.posx, s_skbdcfg.posy, s_skbdcfg.width, s_skbdcfg.height, NULL, NULL))
	{
		return;
	}
	ShowWindow(SW_SHOWNOACTIVATE);
	UpdateWindow();

	if (!m_dd2.Create(m_hWnd, m_nWidth, m_nHeight))
	{
		DestroyWindow();
		return;
	}
	Invalidate();
	SetForegroundWindow(g_hWndMain);
}

/**
 * アイドル処理
 */
void CSoftKeyboardWnd::OnIdle()
{
	if ((m_hWnd) && (softkbd_process()))
	{
		OnDraw(FALSE);
	}
}

void CSoftKeyboardWnd::ConvertClientPointToSoftkbdPoint(int &x, int &y)
{
	RECT rect;
	GetClientRect(&rect);

	if (rect.right - rect.left == 0) return;
	if (rect.bottom - rect.top == 0) return;

	x = x * m_nWidth / (rect.right - rect.left);
	y = y * m_nHeight / (rect.bottom - rect.top);
}

/**
 * CWndProc オブジェクトの Windows プロシージャ (WindowProc) が用意されています
 * @param[in] nMsg 処理される Windows メッセージを指定します
 * @param[in] wParam メッセージの処理で使う付加情報を提供します。このパラメータの値はメッセージに依存します
 * @param[in] lParam メッセージの処理で使う付加情報を提供します。このパラメータの値はメッセージに依存します
 * @return メッセージに依存する値を返します
 */
LRESULT CSoftKeyboardWnd::WindowProc(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	switch (nMsg)
	{
		case WM_CREATE:
			np2class_wmcreate(m_hWnd);
			if (s_skbdcfg.width == 0) s_skbdcfg.width = m_nWidth;
			if (s_skbdcfg.height == 0) s_skbdcfg.height = m_nHeight;
			winloc_setclientsize(m_hWnd, s_skbdcfg.width, s_skbdcfg.height);
			np2class_windowtype(m_hWnd, (s_skbdcfg.type & 1) + 1);

			// システムメニュー追加
			{
				HMENU hMenu = GetSystemMenu(FALSE);
				int pos = menu_addmenures(hMenu, 0, IDR_SOFTKBD_SYS, FALSE);
				InsertMenu(hMenu, pos, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
			}

			break;

		case WM_PAINT:
			OnPaint();
			break;

		case WM_LBUTTONDOWN:
		{
			int x = LOWORD(lParam);
			int y = HIWORD(lParam);
			ConvertClientPointToSoftkbdPoint(x, y);
			if ((softkbd_down(x, y)) && (s_skbdcfg.type & 1))
			{
				return SendMessage(WM_NCLBUTTONDOWN, HTCAPTION, 0L);
			}
			break;
		}

		case WM_LBUTTONDBLCLK:
		{
			int x = LOWORD(lParam);
			int y = HIWORD(lParam);
			ConvertClientPointToSoftkbdPoint(x, y);
			if (softkbd_down(x, y))
			{
				s_skbdcfg.type ^= 1;
				SetWndType((s_skbdcfg.type & 1) + 1);
				sysmng_update(SYS_UPDATEOSCFG);
			}
			break;
		}

		case WM_LBUTTONUP:
			softkbd_up();
			break;

		case WM_MOVE:
			if (!(GetWindowLong(m_hWnd, GWL_STYLE) & (WS_MAXIMIZE | WS_MINIMIZE)))
			{
				RECT rc;
				GetWindowRect(&rc);
				s_skbdcfg.posx = rc.left;
				s_skbdcfg.posy = rc.top;
				sysmng_update(SYS_UPDATEOSCFG);
			}
			break;

		case WM_SIZE:
			if (!(GetWindowLong(m_hWnd, GWL_STYLE) & (WS_MAXIMIZE | WS_MINIMIZE)))
			{
				RECT rc;
				GetClientRect(&rc);
				s_skbdcfg.width = rc.right - rc.left;
				s_skbdcfg.height = rc.bottom - rc.top;
				sysmng_update(SYS_UPDATEOSCFG);
			}
			break;

		case WM_GETMINMAXINFO:
		{
			MINMAXINFO* pInfo = (MINMAXINFO*)lParam;
			RECT rc = { 0, 0, m_nWidth, m_nHeight }; // 最小サイズ
			AdjustWindowRectEx(
				&rc,
				GetWindowLong(m_hWnd, GWL_STYLE),
				FALSE,
				GetWindowLong(m_hWnd, GWL_EXSTYLE)
			);

			pInfo->ptMinTrackSize.x = rc.right - rc.left;
			pInfo->ptMinTrackSize.y = rc.bottom - rc.top;
			return 0;
		}
			
		case WM_CLOSE:
			np2oscfg.skbdwin = 0;
			sysmng_update(SYS_UPDATEOSCFG);
			DestroyWindow();
			break;

		case WM_DESTROY:
			OnDestroy();
			break;

		case WM_SYSCOMMAND:
			if (IDM_SOFTKBD_X1 <= wParam && wParam <= IDM_SOFTKBD_XEND)
			{
				s_skbdcfg.width = m_nWidth * (wParam - IDM_SOFTKBD_X1 + 1);
				s_skbdcfg.height = m_nHeight * (wParam - IDM_SOFTKBD_X1 + 1);
				winloc_setclientsize(m_hWnd, s_skbdcfg.width, s_skbdcfg.height);

				RECT rc;
				GetClientRect(&rc);
				s_skbdcfg.width = rc.right - rc.left;
				s_skbdcfg.height = rc.bottom - rc.top;
				sysmng_update(SYS_UPDATEOSCFG);
			}
			else 
			{
				return CSubWndBase::WindowProc(nMsg, wParam, lParam);
			}
			break;

		default:
			return CSubWndBase::WindowProc(nMsg, wParam, lParam);
	}
	return 0L;
}

/**
 * ウィンドウ破棄の時に呼ばれる
 */
void CSoftKeyboardWnd::OnDestroy()
{
	::np2class_wmdestroy(m_hWnd);
	m_dd2.Release();
}

/**
 * 描画の時に呼ばれる
 */
void CSoftKeyboardWnd::OnPaint()
{
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(&ps);
	OnDraw(TRUE);
	EndPaint(&ps);
}

/**
 * 描画
 * @param[in] redraw 再描画
 */
void CSoftKeyboardWnd::OnDraw(BOOL redraw)
{
	RECT rect;
	GetClientRect(&rect);

	RECT draw;
	RECT dstrect;
	draw.left = 0;
	draw.top = 0;
	draw.right = m_nWidth;
	draw.bottom = m_nHeight;
	dstrect.left = 0;
	dstrect.top = 0;
	dstrect.right = rect.right - rect.left;
	dstrect.bottom = rect.bottom - rect.top;
	CMNVRAM* vram = m_dd2.Lock();
	if (vram)
	{
		softkbd_paint(vram, skpalcnv, redraw);
		m_dd2.Unlock();
		m_dd2.Blt(NULL, &dstrect, &draw);
	}
}

/**
 * パレット変換コールバック
 * @param[out] dst 出力先
 * @param[in] src パレット
 * @param[in] pals パレット数
 * @param[in] bpp 色数
 */
void CSoftKeyboardWnd::skpalcnv(CMNPAL *dst, const RGB32 *src, UINT pals, UINT bpp)
{
	switch (bpp)
	{
#if defined(SUPPORT_16BPP)
		case 16:
			for (UINT i = 0; i < pals; i++)
			{
				dst[i].pal16 = CSoftKeyboardWnd::GetInstance()->m_dd2.GetPalette16(src[i]);
			}
			break;
#endif
#if defined(SUPPORT_24BPP)
		case 24:
#endif
#if defined(SUPPORT_32BPP)
		case 32:
#endif
#if defined(SUPPORT_24BPP) || defined(SUPPORT_32BPP)
			for (UINT i = 0; i < pals; i++)
			{
				dst[i].pal32.d = src[i].d;
			}
			break;
#endif
	}
}

/**
 * 設定読み込み
 */
void skbdwin_readini()
{
	s_skbdcfg.posx = CW_USEDEFAULT;
	s_skbdcfg.posy = CW_USEDEFAULT;
	s_skbdcfg.width = 0;
	s_skbdcfg.height = 0;

	TCHAR szPath[MAX_PATH];
	initgetfile(szPath, _countof(szPath));
	ini_read(szPath, s_skbdapp, s_skbdini, _countof(s_skbdini));
}

/**
 * 設定書き込み
 */
void skbdwin_writeini()
{
	if(!np2oscfg.readonly){
		TCHAR szPath[MAX_PATH];
		initgetfile(szPath, _countof(szPath));
		ini_write(szPath, s_skbdapp, s_skbdini, _countof(s_skbdini));
	}
}
#endif
