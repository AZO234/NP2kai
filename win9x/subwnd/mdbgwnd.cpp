/**
 * @file	mdbgwnd.cpp
 * @brief	メモリ デバガ クラスの動作の定義を行います
 */

#include "compiler.h"
#include "resource.h"
#include "mdbgwnd.h"
#include "np2.h"
#include "ini.h"
#include "sysmng.h"
#include "dialog/np2class.h"
#include "generic/memdbg32.h"

#if defined(CPUCORE_IA32) && defined(SUPPORT_MEMDBG32)

//! 唯一のインスタンスです
CMemDebugWnd CMemDebugWnd::sm_instance;

/**
 * @brief コンフィグ
 */
struct MemDebugConfig
{
	int		posx;
	int		posy;
	UINT8	type;
};

//! コンフィグ
static MemDebugConfig s_mdbgcfg;

//! タイトル
static const TCHAR s_mdbgapp[] = _T("Memory Map");

/**
 * 設定
 */
static const PFTBL s_mdbgini[] =
{
	PFVAL("WindposX", PFTYPE_SINT32,	&s_mdbgcfg.posx),
	PFVAL("WindposY", PFTYPE_SINT32,	&s_mdbgcfg.posy),
	PFVAL("windtype", PFTYPE_BOOL,		&s_mdbgcfg.type)
};

/**
 * 初期化
 */
void CMemDebugWnd::Initialize()
{
	memdbg32_initialize();
}

/**
 * 解放
 */
void CMemDebugWnd::Deinitialize()
{
}

/**
 * コンストラクタ
 */
CMemDebugWnd::CMemDebugWnd()
	: m_nWidth(0)
	, m_nHeight(0)
{
}

/**
 * デストラクタ
 */
CMemDebugWnd::~CMemDebugWnd()
{
}

/**
 * 作成
 */
void CMemDebugWnd::Create()
{
	if (m_hWnd != NULL)
	{
		return;
	}

	memdbg32_getsize(&m_nWidth, &m_nHeight);
	if (!CSubWndBase::Create(s_mdbgapp, WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX, s_mdbgcfg.posx, s_mdbgcfg.posy, m_nWidth, m_nHeight, NULL, NULL))
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
void CMemDebugWnd::OnIdle()
{
	if ((m_hWnd) && (memdbg32_process()))
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
LRESULT CMemDebugWnd::WindowProc(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	switch (nMsg)
	{
		case WM_CREATE:
			np2class_wmcreate(m_hWnd);
			winloc_setclientsize(m_hWnd, m_nWidth, m_nHeight);
			np2class_windowtype(m_hWnd, (s_mdbgcfg.type & 1) + 1);
			break;

		case WM_PAINT:
			OnPaint();
			break;

		case WM_LBUTTONDOWN:
			if (s_mdbgcfg.type & 1)
			{
				return SendMessage(WM_NCLBUTTONDOWN, HTCAPTION, 0L);
			}
			break;

		case WM_LBUTTONDBLCLK:
			s_mdbgcfg.type ^= 1;
			SetWndType((s_mdbgcfg.type & 1) + 1);
			sysmng_update(SYS_UPDATEOSCFG);
			break;

		case WM_MOVE:
			if (!(GetWindowLong(m_hWnd, GWL_STYLE) & (WS_MAXIMIZE | WS_MINIMIZE)))
			{
				RECT rc;
				GetWindowRect(&rc);
				s_mdbgcfg.posx = rc.left;
				s_mdbgcfg.posy = rc.top;
				sysmng_update(SYS_UPDATEOSCFG);
			}
			break;

		case WM_DESTROY:
			OnDestroy();
			break;

		default:
			return CSubWndBase::WindowProc(nMsg, wParam, lParam);
	}
	return(0);
}

/**
 * ウィンドウ破棄の時に呼ばれる
 */
void CMemDebugWnd::OnDestroy()
{
	::np2class_wmdestroy(m_hWnd);
	m_dd2.Release();
}

/**
 * 描画の時に呼ばれる
 */
void CMemDebugWnd::OnPaint()
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
void CMemDebugWnd::OnDraw(BOOL redraw)
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
		memdbg32_paint(vram, mdpalcnv, redraw);
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
void CMemDebugWnd::mdpalcnv(CMNPAL *dst, const RGB32 *src, UINT pals, UINT bpp)
{
	switch (bpp)
	{
#if defined(SUPPORT_16BPP)
		case 16:
			for (UINT i = 0; i < pals; i++)
			{
				dst[i].pal16 = CMemDebugWnd::GetInstance()->m_dd2.GetPalette16(src[i]);
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
void mdbgwin_readini()
{
	s_mdbgcfg.posx = CW_USEDEFAULT;
	s_mdbgcfg.posy = CW_USEDEFAULT;

	OEMCHAR szPath[MAX_PATH];
	initgetfile(szPath, _countof(szPath));
	ini_read(szPath, s_mdbgapp, s_mdbgini, _countof(s_mdbgini));
}

/**
 * 設定書き込み
 */
void mdbgwin_writeini()
{
	TCHAR szPath[MAX_PATH];
	initgetfile(szPath, _countof(szPath));
	ini_write(szPath, s_mdbgapp, s_mdbgini, _countof(s_mdbgini));
}
#endif
