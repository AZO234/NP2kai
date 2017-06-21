/**
 * @file	dd2.cpp
 * @brief	DirectDraw2 描画クラスの動作の定義を行います
 */

#include "compiler.h"
#include "dd2.h"

#if !defined(__GNUC__)
#pragma comment(lib, "ddraw.lib")
#pragma comment(lib, "dxguid.lib")
#endif	// !defined(__GNUC__)

/**
 * コンストラクタ
 */
DD2Surface::DD2Surface()
	: m_hWnd(NULL)
	, m_pDDraw(NULL)
	, m_pDDraw2(NULL)
	, m_pPrimarySurface(NULL)
	, m_pBackSurface(NULL)
	, m_pClipper(NULL)
	, m_pPalette(NULL)
	, m_r16b(0)
	, m_l16r(0)
	, m_l16g(0)
{
	m_pal16.d = 0;
	ZeroMemory(&m_vram, sizeof(m_vram));
	ZeroMemory(&m_pal, sizeof(m_pal));
}

/**
 * デストラクタ
 */
DD2Surface::~DD2Surface()
{
	Release();
}

/**
 * 作成
 * @param[in] hWnd ウィンドウ ハンドル
 * @param[in] nWidth 幅
 * @param[in] nHeight 高さ
 * @retval true 成功
 * @retval false 失敗
 */
bool DD2Surface::Create(HWND hWnd, int nWidth, int nHeight)
{
	m_hWnd = hWnd;

	do
	{
		if (DirectDrawCreate(NULL, &m_pDDraw, NULL) != DD_OK)
		{
			break;
		}
		m_pDDraw->QueryInterface(IID_IDirectDraw2, reinterpret_cast<LPVOID*>(&m_pDDraw2));
		m_pDDraw2->SetCooperativeLevel(hWnd, DDSCL_NORMAL);

		DDSURFACEDESC ddsd;
		ZeroMemory(&ddsd, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		ddsd.dwFlags = DDSD_CAPS;
		ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
		if (m_pDDraw2->CreateSurface(&ddsd, &m_pPrimarySurface, NULL) != DD_OK)
		{
			break;
		}
		m_pDDraw2->CreateClipper(0, &m_pClipper, NULL);
		m_pClipper->SetHWnd(0, hWnd);
		m_pPrimarySurface->SetClipper(m_pClipper);

		DDPIXELFORMAT ddpf;
		ZeroMemory(&ddpf, sizeof(ddpf));
		ddpf.dwSize = sizeof(ddpf);
		if (m_pPrimarySurface->GetPixelFormat(&ddpf) != DD_OK)
		{
			break;
		}

		ZeroMemory(&ddsd, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
		ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
		ddsd.dwWidth = nWidth;
		ddsd.dwHeight = nHeight;
		if (m_pDDraw2->CreateSurface(&ddsd, &m_pBackSurface, NULL) != DD_OK)
		{
			break;
		}
		if (ddpf.dwRGBBitCount == 8)
		{
			HDC hdc = ::GetDC(hWnd);
			::GetSystemPaletteEntries(hdc, 0, 256, m_pal);
			::ReleaseDC(hWnd, hdc);
			m_pDDraw2->CreatePalette(DDPCAPS_8BIT, m_pal, &m_pPalette, 0);
			m_pPrimarySurface->SetPalette(m_pPalette);
		}
		else if (ddpf.dwRGBBitCount == 16)
		{
			WORD bit;
			UINT8 cnt;

			m_pal16.d = 0;
			for (bit = 1; (bit) && (!(ddpf.dwBBitMask & bit)); bit <<= 1)
			{
			}
			for (m_r16b = 8; (m_r16b) && (ddpf.dwBBitMask & bit); m_r16b--, bit <<= 1)
			{
				m_pal16.p.b >>= 1;
				m_pal16.p.b |= 0x80;
			}
			for (m_l16r = 0, bit = 1; (bit) && (!(ddpf.dwRBitMask & bit)); m_l16r++, bit <<= 1)
			{
			}
			for (cnt = 0x80; (cnt) && (ddpf.dwRBitMask & bit); cnt >>= 1, bit <<= 1)
			{
				m_pal16.p.r |= cnt;
			}
			for (; cnt; cnt>>=1)
			{
				m_l16r--;
			}
			for (m_l16g = 0, bit = 1; (bit) && (!(ddpf.dwGBitMask & bit)); m_l16g++, bit <<= 1)
			{
			}
			for (cnt = 0x80; (cnt) && (ddpf.dwGBitMask & bit); cnt >>= 1, bit <<= 1)
			{
				m_pal16.p.g |= cnt;
			}
			for (; cnt; cnt >>= 1)
			{
				m_l16g--;
			}
		}
		else if (ddpf.dwRGBBitCount == 24)
		{
		}
		else if (ddpf.dwRGBBitCount == 32)
		{
		}
		else
		{
			break;
		}
		m_vram.width = nWidth;
		m_vram.height = nHeight;
		m_vram.xalign = ddpf.dwRGBBitCount / 8;
		m_vram.bpp = ddpf.dwRGBBitCount;
		return true;
	} while (false /*CONSTCOND*/);

	Release();
	return false;
}

/**
 * 解放
 */
void DD2Surface::Release()
{
	if (m_pPalette)
	{
		m_pPalette->Release();
		m_pPalette = NULL;
	}
	if (m_pClipper)
	{
		m_pClipper->Release();
		m_pClipper = NULL;
	}
	if (m_pBackSurface)
	{
		m_pBackSurface->Release();
		m_pBackSurface = NULL;
	}
	if (m_pPrimarySurface)
	{
		m_pPrimarySurface->Release();
		m_pPrimarySurface = NULL;
	}
	if (m_pDDraw2)
	{
		m_pDDraw2->Release();
		m_pDDraw2 = NULL;
	}
	if (m_pDDraw)
	{
		m_pDDraw->Release();
		m_pDDraw = NULL;
	}
}

/**
 * バッファ ロック
 * @return バッファ
 */
CMNVRAM* DD2Surface::Lock()
{
	if (m_pBackSurface == NULL)
	{
		return NULL;
	}
	DDSURFACEDESC surface;
	ZeroMemory(&surface, sizeof(DDSURFACEDESC));
	surface.dwSize = sizeof(surface);
	HRESULT r = m_pBackSurface->Lock(NULL, &surface, DDLOCK_WAIT, NULL);
	if (r == DDERR_SURFACELOST)
	{
		m_pBackSurface->Restore();
		r = m_pBackSurface->Lock(NULL, &surface, DDLOCK_WAIT, NULL);
	}
	if (r != DD_OK)
	{
		return(NULL);
	}
	m_vram.ptr = static_cast<UINT8*>(surface.lpSurface);
	m_vram.yalign = surface.lPitch;
	return &m_vram;
}

/**
 * バッファ アンロック
 */
void DD2Surface::Unlock()
{
	if (m_pBackSurface)
	{
		m_pBackSurface->Unlock(NULL);
	}
}

/**
 * blt
 * @param[in] pt 位置
 * @param[in] lpRect 領域
 */
void DD2Surface::Blt(const POINT* pt, const RECT* lpRect)
{
	if (m_pBackSurface)
	{
		POINT clipt;
		if (pt)
		{
			clipt = *pt;
		}
		else
		{
			clipt.x = 0;
			clipt.y = 0;
		}
		::ClientToScreen(m_hWnd, &clipt);
		RECT scrn;
		scrn.left = clipt.x;
		scrn.top = clipt.y;
		scrn.right = clipt.x + lpRect->right - lpRect->left;
		scrn.bottom = clipt.y + lpRect->bottom - lpRect->top;
		if (m_pPrimarySurface->Blt(&scrn, m_pBackSurface, const_cast<LPRECT>(lpRect), DDBLT_WAIT, NULL) == DDERR_SURFACELOST)
		{
			m_pBackSurface->Restore();
			m_pPrimarySurface->Restore();
		}
	}
}

/**
 * 16BPP 色を得る
 * @param[in] pal 色
 * @return 16BPP色
 */
UINT16 DD2Surface::GetPalette16(RGB32 pal) const
{
	pal.d &= m_pal16.d;
	return (static_cast<UINT>(pal.p.g) << m_l16g) | (static_cast<UINT>(pal.p.r) << m_l16r) | (pal.p.b >> m_r16b);
}
