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

	if (!CSubWndBase::Create(IDS_CAPTION_SOFTKEY, WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX, s_skbdcfg.posx, s_skbdcfg.posy, m_nWidth, m_nHeight, NULL, NULL))
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
			winloc_setclientsize(m_hWnd, m_nWidth, m_nHeight);
			np2class_windowtype(m_hWnd, (s_skbdcfg.type & 1) + 1);
			break;

		case WM_PAINT:
			OnPaint();
			break;

		case WM_LBUTTONDOWN:
			if ((softkbd_down(LOWORD(lParam), HIWORD(lParam))) && (s_skbdcfg.type & 1))
			{
				return SendMessage(WM_NCLBUTTONDOWN, HTCAPTION, 0L);
			}
			break;

		case WM_LBUTTONDBLCLK:
			if (softkbd_down(LOWORD(lParam), HIWORD(lParam)))
			{
				s_skbdcfg.type ^= 1;
				SetWndType((s_skbdcfg.type & 1) + 1);
				sysmng_update(SYS_UPDATEOSCFG);
			}
			break;

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
			
		case WM_CLOSE:
			np2oscfg.skbdwin = 0;
			sysmng_update(SYS_UPDATEOSCFG);
			DestroyWindow();
			break;

		case WM_DESTROY:
			OnDestroy();
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
	draw.left = 0;
	draw.top = 0;
	draw.right = min(m_nWidth, rect.right - rect.left);
	draw.bottom = min(m_nHeight, rect.bottom - rect.top);
	CMNVRAM* vram = m_dd2.Lock();
	if (vram)
	{
		softkbd_paint(vram, skpalcnv, redraw);
		m_dd2.Unlock();
		m_dd2.Blt(NULL, &draw);
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
