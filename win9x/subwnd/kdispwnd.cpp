/**
 * @file	kdispwnd.cpp
 * @brief	キーボード クラスの動作の定義を行います
 */

#include "compiler.h"
#include "resource.h"
#include "kdispwnd.h"
#include "np2.h"
#include "ini.h"
#include "menu.h"
#include "sysmng.h"
#include "dialog/np2class.h"
#include "generic/keydisp.h"

extern WINLOCEX np2_winlocexallwin(HWND base);

#if defined(SUPPORT_KEYDISP)

//! 唯一のインスタンスです
CKeyDisplayWnd CKeyDisplayWnd::sm_instance;

/**
 * モード
 */
enum
{
	KDISPCFG_FM		= 0x00,
	KDISPCFG_MIDI	= 0x80
};

/**
 * @brief コンフィグ
 */
struct KeyDisplayConfig
{
	int		posx;		//!< X
	int		posy;		//!< Y
	UINT8	mode;		//!< モード
	UINT8	type;		//!< ウィンドウ タイプ
};

//! コンフィグ
static KeyDisplayConfig s_kdispcfg;

//! タイトル
static const TCHAR s_kdispapp[] = TEXT("Key Display");

/**
 * 設定
 */
static const PFTBL s_kdispini[] =
{
	PFVAL("WindposX", PFTYPE_SINT32,	&s_kdispcfg.posx),
	PFVAL("WindposY", PFTYPE_SINT32,	&s_kdispcfg.posy),
	PFVAL("keydmode", PFTYPE_UINT8,		&s_kdispcfg.mode),
	PFVAL("windtype", PFTYPE_BOOL,		&s_kdispcfg.type)
};

//! パレット
static const UINT32 s_kdisppal[KEYDISP_PALS] = {0x00000000, 0xffffffff, 0xf9ff0000};

/**
 * 初期化
 */
void CKeyDisplayWnd::Initialize()
{
	keydisp_initialize();
}

/**
 * 解放
 */
void CKeyDisplayWnd::Deinitialize()
{
}

/**
 * コンストラクタ
 */
CKeyDisplayWnd::CKeyDisplayWnd()
{
}

/**
 * デストラクタ
 */
CKeyDisplayWnd::~CKeyDisplayWnd()
{
}

/**
 * 8bpp色を返す
 * @param[in] self インスタンス
 * @param[in] num パレット番号
 * @return 色
 */
static UINT8 kdgetpal8(CMNPALFN* self, UINT num)
{
	if (num < KEYDISP_PALS)
	{
		return s_kdisppal[num] >> 24;
	}
	return 0;
}

/**
 * 16bpp色を返す
 * @param[in] self インスタンス
 * @param[in] pal32 パレット
 * @return 色
 */
static UINT16 kdcnvpal16(CMNPALFN* self, RGB32 pal32)
{
	return (reinterpret_cast<DD2Surface*>(self->userdata))->GetPalette16(pal32);
}

/**
 * 32bpp色を返す
 * @param[in] self インスタンス
 * @param[in] num パレット番号
 * @return 色
 */
static UINT32 kdgetpal32(CMNPALFN* self, UINT num)
{
	if (num < KEYDISP_PALS)
	{
		return s_kdisppal[num] & 0xffffff;
	}
	return 0;
}

/**
 * 作成
 */
void CKeyDisplayWnd::Create()
{
	if (m_hWnd != NULL)
	{
		return;
	}

	HINSTANCE hInstance = FindResourceHandle(MAKEINTRESOURCE(IDR_KEYDISP), RT_MENU);
	HMENU hMenu = ::LoadMenu(hInstance, MAKEINTRESOURCE(IDR_KEYDISP));
	if (!CSubWndBase::Create(IDS_CAPTION_KEYDISP, WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX, s_kdispcfg.posx, s_kdispcfg.posy, KEYDISP_WIDTH, KEYDISP_HEIGHT, NULL, hMenu))
	{
		np2oscfg.keydisp = 0;
		sysmng_update(SYS_UPDATEOSCFG);
		return;
	}

	UINT8 mode;
	switch (s_kdispcfg.mode)
	{
		case KDISPCFG_FM:
		default:
			mode = KEYDISP_MODEFM;
			break;

		case KDISPCFG_MIDI:
			mode = KEYDISP_MODEMIDI;
			break;
	}
	SetDispMode(mode);
	ShowWindow(SW_SHOWNOACTIVATE);
	UpdateWindow();

	if (!m_dd2.Create(m_hWnd, KEYDISP_WIDTH, KEYDISP_HEIGHT))
	{
		DestroyWindow();
		return;
	}

	CMNPALFN palfn;
	palfn.get8 = kdgetpal8;
	palfn.get32 = kdgetpal32;
	palfn.cnv16 = kdcnvpal16;
	palfn.userdata = reinterpret_cast<INTPTR>(&m_dd2);
	keydisp_setpal(&palfn);
	kdispwin_draw(0);
	::SetForegroundWindow(g_hWndMain);
}

/**
 * 描画する
 * @param[in] cnt 進んだフレーム
 */
void CKeyDisplayWnd::Draw(UINT8 cnt)
{
	if (m_hWnd)
	{
		if (!cnt)
		{
			cnt = 1;
		}
		UINT8 flag = keydisp_process(cnt);
		if (flag & KEYDISP_FLAGSIZING)
		{
			OnResize();
		}
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
LRESULT CKeyDisplayWnd::WindowProc(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	switch (nMsg)
	{
		case WM_CREATE:
			np2class_wmcreate(m_hWnd);
			np2class_windowtype(m_hWnd, (s_kdispcfg.type & 1) << 1);
			break;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDM_KDISPFM:
					s_kdispcfg.mode = KDISPCFG_FM;
					sysmng_update(SYS_UPDATEOSCFG);
					SetDispMode(KEYDISP_MODEFM);
					break;

				case IDM_KDISPMIDI:
					s_kdispcfg.mode = KDISPCFG_MIDI;
					sysmng_update(SYS_UPDATEOSCFG);
					SetDispMode(KEYDISP_MODEMIDI);
					break;

				case IDM_CLOSE:
					return SendMessage(WM_CLOSE, 0, 0);
			}
			break;

		case WM_PAINT:
			OnPaint();
			break;

		case WM_LBUTTONDOWN:
			if (s_kdispcfg.type & 1)
			{
				return SendMessage(WM_NCLBUTTONDOWN, HTCAPTION, 0L);
			}
			break;

		case WM_RBUTTONDOWN:
			{
				POINT pt;
				pt.x = GET_X_LPARAM(lParam);
				pt.y = GET_Y_LPARAM(lParam);
				OnRButtonDown(static_cast<UINT>(wParam), pt);
			}
			break;

		case WM_LBUTTONDBLCLK:
			s_kdispcfg.type ^= 1;
			SetWndType((s_kdispcfg.type & 1) << 1);
			sysmng_update(SYS_UPDATEOSCFG);
			break;

		case WM_MOVE:
			if (!(GetWindowLong(m_hWnd, GWL_STYLE) & (WS_MAXIMIZE | WS_MINIMIZE)))
			{
				RECT rc;
				GetWindowRect(&rc);
				s_kdispcfg.posx = rc.left;
				s_kdispcfg.posy = rc.top;
				sysmng_update(SYS_UPDATEOSCFG);
			}
			break;

		case WM_CLOSE:
			np2oscfg.keydisp = 0;
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
void CKeyDisplayWnd::OnDestroy()
{
	::np2class_wmdestroy(m_hWnd);
	m_dd2.Release();
	SetDispMode(KEYDISP_MODENONE);
}

/**
 * フレームワークは、ユーザーがマウスの右ボタンを押すと、このメンバー関数を呼び出します
 * @param[in] nFlags 仮想キーが押されているかどうかを示します
 * @param[in] point カーソルの x 座標と y 座標を指定します
 */
void CKeyDisplayWnd::OnRButtonDown(UINT nFlags, POINT point)
{
	HMENU hMenu = CreatePopupMenu();
	menu_addmenu(hMenu, 0, np2class_gethmenu(m_hWnd), FALSE);
	menu_addmenures(hMenu, -1, IDR_CLOSE, TRUE);

	ClientToScreen(&point);
	::TrackPopupMenu(hMenu, TPM_LEFTALIGN, point.x, point.y, 0, m_hWnd, NULL);
	::DestroyMenu(hMenu);
}

/**
 * 描画の時に呼ばれる
 */
void CKeyDisplayWnd::OnPaint()
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
void CKeyDisplayWnd::OnDraw(BOOL redraw)
{
	RECT rect;
	GetClientRect(&rect);

	RECT draw;
	draw.left = 0;
	draw.top = 0;
	draw.right = min(KEYDISP_WIDTH, rect.right - rect.left);
	draw.bottom = min(KEYDISP_HEIGHT, rect.bottom - rect.top);
	if ((draw.right <= 0) || (draw.bottom <= 0))
	{
		return;
	}
	CMNVRAM* vram = m_dd2.Lock();
	if (vram)
	{
		keydisp_paint(vram, redraw);
		m_dd2.Unlock();
		m_dd2.Blt(NULL, &draw);
	}
}

/**
 * リサイズ
 */
void CKeyDisplayWnd::OnResize()
{
	WINLOCEX wlex = np2_winlocexallwin(g_hWndMain);
	winlocex_setholdwnd(wlex, m_hWnd);

	int width;
	int height;
	keydisp_getsize(&width, &height);
	winloc_setclientsize(m_hWnd, width, height);
	winlocex_move(wlex);
	winlocex_destroy(wlex);
}

/**
 * モード チェンジ
 * @param[in] mode モード
 */
void CKeyDisplayWnd::SetDispMode(UINT8 mode)
{
	keydisp_setmode(mode);

	HMENU hMenu = np2class_gethmenu(m_hWnd);
	CheckMenuItem(hMenu, IDM_KDISPFM, ((mode == KEYDISP_MODEFM) ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, IDM_KDISPMIDI, ((mode == KEYDISP_MODEMIDI) ? MF_CHECKED : MF_UNCHECKED));
}

/**
 * 設定読み込み
 */
void kdispwin_readini()
{
	ZeroMemory(&s_kdispcfg, sizeof(s_kdispcfg));
	s_kdispcfg.posx = CW_USEDEFAULT;
	s_kdispcfg.posy = CW_USEDEFAULT;

	TCHAR szPath[MAX_PATH];
	initgetfile(szPath, _countof(szPath));
	ini_read(szPath, s_kdispapp, s_kdispini, _countof(s_kdispini));
}

/**
 * 設定書き込み
 */
void kdispwin_writeini()
{
	TCHAR szPath[MAX_PATH];

	initgetfile(szPath, _countof(szPath));
	ini_write(szPath, s_kdispapp, s_kdispini, _countof(s_kdispini));
}
#endif
