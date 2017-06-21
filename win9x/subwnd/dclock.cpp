/**
 * @file	dclock.cpp
 * @brief	時刻表示クラスの動作の定義を行います
 */

#include "compiler.h"
#include "dclock.h"
#include "parts.h"
#include "np2.h"
#include "scrnmng.h"
#include "timemng.h"
#include "scrndraw.h"
#include "palettes.h"

//! 唯一のインスタンスです
DispClock DispClock::sm_instance;

/**
 * @brief パターン構造体
 */
struct DispClockPattern
{
	UINT8 cWidth;											/*!< フォント サイズ */
	UINT8 cMask;											/*!< マスク */
	UINT8 cPosition[6];										/*!< 位置 */
	void (*fnInitialize)(UINT8* lpBuffer, UINT8 nDegits);	/*!< 初期化関数 */
	UINT8 font[11][16];										/*!< フォント */
};

/**
 * 初期化-1
 * @param[out] lpBuffer バッファ
 * @param[in] nDegits 桁数
 */
static void InitializeFont1(UINT8* lpBuffer, UINT8 nDegits)
{
	if (nDegits)
	{
		const UINT32 pat = (nDegits <= 4) ? 0x00008001 : 0x30008001;
		*(UINT32 *)(lpBuffer + 1 + ( 4 * DCLOCK_YALIGN)) = pat;
		*(UINT32 *)(lpBuffer + 1 + ( 5 * DCLOCK_YALIGN)) = pat;
		*(UINT32 *)(lpBuffer + 1 + ( 9 * DCLOCK_YALIGN)) = pat;
		*(UINT32 *)(lpBuffer + 1 + (10 * DCLOCK_YALIGN)) = pat;
	}
}

/**
 * 初期化-2
 * @param[out] lpBuffer バッファ
 * @param[in] nDegits 桁数
 */
static void InitializeFont2(UINT8* lpBuffer, UINT8 nDegits)
{
	if (nDegits)
	{
		UINT32 pat = (nDegits <= 4) ? 0x00000002 : 0x00020002;
		*(UINT32 *)(lpBuffer + 1 + ( 4 * DCLOCK_YALIGN)) = pat;
		*(UINT32 *)(lpBuffer + 1 + ( 5 * DCLOCK_YALIGN)) = pat;
		pat <<= 1;
		*(UINT32 *)(lpBuffer + 1 + ( 9 * DCLOCK_YALIGN)) = pat;
		*(UINT32 *)(lpBuffer + 1 + (10 * DCLOCK_YALIGN)) = pat;
	}
}

/**
 * 初期化-3
 * @param[out] lpBuffer バッファ
 * @param[in] nDegits 桁数
 */
static void InitializeFont3(UINT8* lpBuffer, UINT8 nDegits)
{
	if (nDegits)
	{
		const UINT32 pat = (nDegits <= 4) ? 0x00000010 : 0x00400010;
		*(UINT32 *)(lpBuffer + 1 + ( 4 * DCLOCK_YALIGN)) = pat;
		*(UINT32 *)(lpBuffer + 1 + ( 5 * DCLOCK_YALIGN)) = pat;
		*(UINT32 *)(lpBuffer + 1 + ( 9 * DCLOCK_YALIGN)) = pat;
		*(UINT32 *)(lpBuffer + 1 + (10 * DCLOCK_YALIGN)) = pat;
	}
}

/**
 * 初期化-4
 * @param[out] lpBuffer バッファ
 * @param[in] nDegits 桁数
 */
static void InitializeFont4(UINT8* lpBuffer, UINT8 nDegits)
{
	if (nDegits)
	{
		const UINT32 pat = (nDegits <= 4) ? 0x00000004 : 0x00040004;
		*(UINT32 *)(lpBuffer + 1 + ( 5 * DCLOCK_YALIGN)) = pat;
		*(UINT32 *)(lpBuffer + 1 + ( 6 * DCLOCK_YALIGN)) = pat;
		*(UINT32 *)(lpBuffer + 1 + ( 9 * DCLOCK_YALIGN)) = pat;
		*(UINT32 *)(lpBuffer + 1 + (10 * DCLOCK_YALIGN)) = pat;
	}
}

/**
 * 初期化-5
 * @param[out] lpBuffer バッファ
 * @param[in] nDegits 桁数
 */
static void InitializeFont5(UINT8* lpBuffer, UINT8 nDegits)
{
	if (nDegits)
	{
		const UINT32 pat = (nDegits <= 4) ? 0x00000006 : 0x00030006;
		*(UINT32 *)(lpBuffer + 1 + ( 6 * DCLOCK_YALIGN)) = pat;
		*(UINT32 *)(lpBuffer + 1 + ( 7 * DCLOCK_YALIGN)) = pat;
		*(UINT32 *)(lpBuffer + 1 + ( 9 * DCLOCK_YALIGN)) = pat;
		*(UINT32 *)(lpBuffer + 1 + (10 * DCLOCK_YALIGN)) = pat;
	}
}

/**
 * 初期化-6
 * @param[out] lpBuffer バッファ
 * @param[in] nDegits 桁数
 */
static void InitializeFont6(UINT8* lpBuffer, UINT8 nDegits)
{
	if (nDegits)
	{
		const UINT32 pat = (nDegits <= 4) ? 0x00000020 : 0x00000220;
		*(UINT32 *)(lpBuffer + 1 + ( 8 * DCLOCK_YALIGN)) = pat;
		*(UINT32 *)(lpBuffer + 1 + (10 * DCLOCK_YALIGN)) = pat;
	}
}

/**
 * パターン
 */
static const DispClockPattern s_pattern[6] =
{
	// FONT-1
	{
		6, 0xfc,
		{0, 7, 19, 26, 38, 45},
		InitializeFont1,
		{
			{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,},
			{0x78, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0x78,},
			{0x30, 0x70, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30,},
			{0x78, 0xcc, 0xcc, 0x0c, 0x18, 0x30, 0x60, 0xc0, 0xfc,},
			{0xfc, 0x18, 0x30, 0x70, 0x18, 0x0c, 0x0c, 0xcc, 0x78,},
			{0x18, 0x38, 0x78, 0xd8, 0xd8, 0xfc, 0x18, 0x18, 0x18,},
			{0xfc, 0xc0, 0xc0, 0xf8, 0x0c, 0x0c, 0x0c, 0x8c, 0x78,},
			{0x38, 0x60, 0xc0, 0xf8, 0xcc, 0xcc, 0xcc, 0xcc, 0x78,},
			{0xfc, 0x0c, 0x0c, 0x18, 0x18, 0x18, 0x30, 0x30, 0x30,},
			{0x78, 0xcc, 0xcc, 0xcc, 0x78, 0xcc, 0xcc, 0xcc, 0x78,},
			{0x78, 0xcc, 0xcc, 0xcc, 0xcc, 0x7c, 0x0c, 0x18, 0x70,}
		}
	},

	// FONT-2
	{
		6, 0xfc,
		{0, 6, 16, 22, 32, 38},
		InitializeFont2,
		{
			{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,},
			{0x00, 0x00, 0x30, 0x48, 0x88, 0x88, 0x88, 0x88, 0x70,},
			{0x10, 0x30, 0x10, 0x10, 0x10, 0x20, 0x20, 0x20, 0x20,},
			{0x38, 0x44, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0xf8,},
			{0x7c, 0x08, 0x10, 0x30, 0x10, 0x08, 0x08, 0x90, 0x60,},
			{0x20, 0x40, 0x40, 0x88, 0x88, 0x90, 0x78, 0x10, 0x20,},
			{0x3c, 0x20, 0x20, 0x70, 0x08, 0x08, 0x08, 0x90, 0x60,},
			{0x10, 0x10, 0x20, 0x70, 0x48, 0x88, 0x88, 0x90, 0x60,},
			{0x7c, 0x04, 0x08, 0x08, 0x10, 0x10, 0x20, 0x20, 0x40,},
			{0x38, 0x44, 0x44, 0x48, 0x30, 0x48, 0x88, 0x88, 0x70,},
			{0x18, 0x24, 0x40, 0x44, 0x48, 0x38, 0x10, 0x20, 0x20,}
		}
	},

	// FONT-3
	{
		4, 0xf0,
		{0, 5, 14, 19, 28, 33},
		InitializeFont3,
		{
			{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,},
			{0x60, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x60,},
			{0x20, 0x60, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,},
			{0x60, 0x90, 0x90, 0x10, 0x20, 0x40, 0x40, 0x80, 0xf0,},
			{0x60, 0x90, 0x90, 0x10, 0x60, 0x10, 0x90, 0x90, 0x60,},
			{0x20, 0x60, 0x60, 0xa0, 0xa0, 0xa0, 0xf0, 0x20, 0x20,},
			{0xf0, 0x80, 0x80, 0xe0, 0x90, 0x10, 0x90, 0x90, 0x60,},
			{0x60, 0x90, 0x90, 0x80, 0xe0, 0x90, 0x90, 0x90, 0x60,},
			{0xf0, 0x10, 0x10, 0x20, 0x20, 0x20, 0x40, 0x40, 0x40,},
			{0x60, 0x90, 0x90, 0x90, 0x60, 0x90, 0x90, 0x90, 0x60,},
			{0x60, 0x90, 0x90, 0x90, 0x70, 0x10, 0x90, 0x90, 0x60,}
		}
	},

	// FONT-4
	{
		5, 0xf8,
		{0, 6, 16, 22, 32, 38},
		InitializeFont4,
		{
			{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,},
			{0x00, 0x70, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x70,},
			{0x00, 0x60, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x70,},
			{0x00, 0x70, 0x88, 0x08, 0x08, 0x30, 0x40, 0x88, 0xf8,},
			{0x00, 0x70, 0x88, 0x08, 0x30, 0x08, 0x08, 0x08, 0xf0,},
			{0x00, 0x10, 0x30, 0x50, 0x50, 0x90, 0xf8, 0x10, 0x10,},
			{0x00, 0x38, 0x40, 0x60, 0x10, 0x08, 0x08, 0x08, 0xf0,},
			{0x00, 0x18, 0x20, 0x40, 0xb0, 0xc8, 0x88, 0x88, 0x70,},
			{0x00, 0x70, 0x88, 0x88, 0x10, 0x10, 0x10, 0x20, 0x20,},
			{0x00, 0x70, 0x88, 0x88, 0x70, 0x50, 0x88, 0x88, 0x70,},
			{0x00, 0x70, 0x88, 0x88, 0x88, 0x78, 0x10, 0x20, 0xc0,}
		}
	},

	// FONT-5
	{
		5, 0xf8,
		{0, 6, 17, 23, 34, 40},
		InitializeFont5,
		{
			{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,},
			{0x00, 0x00, 0x70, 0x88, 0x88, 0x88, 0x88, 0x88, 0x70,},
			{0x00, 0x00, 0x20, 0x60, 0x20, 0x20, 0x20, 0x20, 0x20,},
			{0x00, 0x00, 0x70, 0x88, 0x08, 0x10, 0x20, 0x40, 0xf8,},
			{0x00, 0x00, 0xf8, 0x10, 0x20, 0x10, 0x08, 0x88, 0x70,},
			{0x00, 0x00, 0x30, 0x50, 0x50, 0x90, 0xf8, 0x10, 0x10,},
			{0x00, 0x00, 0xf8, 0x80, 0xf0, 0x08, 0x08, 0x88, 0x70,},
			{0x00, 0x00, 0x30, 0x40, 0xf0, 0x88, 0x88, 0x88, 0x70,},
			{0x00, 0x00, 0xf8, 0x08, 0x10, 0x20, 0x20, 0x40, 0x40,},
			{0x00, 0x00, 0x70, 0x88, 0x88, 0x70, 0x88, 0x88, 0x70,},
			{0x00, 0x00, 0x70, 0x88, 0x88, 0x88, 0x78, 0x10, 0x60,}
		}
	},

	// FONT-6
	{
		4, 0xf0,
		{0, 5, 12, 17, 24, 29},
		InitializeFont6,
		{
			{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,},
			{0x00, 0x00, 0x00, 0x60, 0x90, 0x90, 0x90, 0x90, 0x60,},
			{0x00, 0x00, 0x00, 0x20, 0x60, 0x20, 0x20, 0x20, 0x20,},
			{0x00, 0x00, 0x00, 0x60, 0x90, 0x10, 0x20, 0x40, 0xf0,},
			{0x00, 0x00, 0x00, 0xf0, 0x20, 0x60, 0x10, 0x90, 0x60,},
			{0x00, 0x00, 0x00, 0x40, 0x80, 0xa0, 0xa0, 0xf0, 0x20,},
			{0x00, 0x00, 0x00, 0xf0, 0x80, 0x60, 0x10, 0x90, 0x60,},
			{0x00, 0x00, 0x00, 0x40, 0x80, 0xe0, 0x90, 0x90, 0x60,},
			{0x00, 0x00, 0x00, 0xe0, 0x10, 0x10, 0x20, 0x20, 0x40,},
			{0x00, 0x00, 0x00, 0x60, 0x90, 0x60, 0x90, 0x90, 0x60,},
			{0x00, 0x00, 0x00, 0x60, 0x90, 0x90, 0x70, 0x20, 0x40,}
		}
	}
};

/**
 * コンストラクタ
 */
DispClock::DispClock()
{
	ZeroMemory(this, sizeof(*this));
}

/**
 * 初期化
 */
void DispClock::Initialize()
{
	::pal_makegrad(m_pal32, 4, np2oscfg.clk_color1, np2oscfg.clk_color2);
}

/**
 * パレット設定
 * @param[in] bpp 色
 */
void DispClock::SetPalettes(UINT bpp)
{
	switch (bpp)
	{
		case 8:
			SetPalette8();
			break;

		case 16:
			SetPalette16();
			break;
	}
}

/**
 * 8bpp パレット設定
 */
void DispClock::SetPalette8()
{
	for (UINT i = 0; i < 16; i++)
	{
		UINT nBits = 0;
		for (UINT j = 1; j < 0x10; j <<= 1)
		{
			nBits <<= 8;
			if (i & j)
			{
				nBits |= 1;
			}
		}
		for (UINT j = 0; j < 4; j++)
		{
			m_pal8[j][i] = nBits * (START_PALORG + j);
		}
	}
}

/**
 * 16bpp パレット設定
 */
void DispClock::SetPalette16()
{
	for (UINT i = 0; i < 4; i++)
	{
		m_pal16[i] = scrnmng_makepal16(m_pal32[i]);
	}
}

/**
 * リセット
 */
void DispClock::Reset()
{
	if (np2oscfg.clk_x)
	{
		if (np2oscfg.clk_x <= 4)
		{
			np2oscfg.clk_x = 4;
		}
		else if (np2oscfg.clk_x <= 6)
		{
			np2oscfg.clk_x = 6;
		}
		else
		{
			np2oscfg.clk_x = 0;
		}
	}
	if (np2oscfg.clk_fnt >= _countof(s_pattern))
	{
		np2oscfg.clk_fnt = 0;
	}

	ZeroMemory(m_cTime, sizeof(m_cTime));
	ZeroMemory(m_cLastTime, sizeof(m_cLastTime));
	ZeroMemory(m_buffer, sizeof(m_buffer));
	m_pPattern = &s_pattern[np2oscfg.clk_fnt];
	m_cCharaters = np2oscfg.clk_x;
	(*m_pPattern->fnInitialize)(m_buffer, m_cCharaters);
	Update();
	Redraw();
}

/**
 * 更新
 */
void DispClock::Update()
{
	if ((scrnmng_isfullscreen()) && (m_cCharaters))
	{
		_SYSTIME st;
		timemng_gettime(&st);

		UINT8 buf[6];
		buf[0] = (st.hour / 10) + 1;
		buf[1] = (st.hour % 10) + 1;
		buf[2] = (st.minute / 10) + 1;
		buf[3] = (st.minute % 10) + 1;
		if (m_cCharaters > 4)
		{
			buf[4] = (st.second / 10) + 1;
			buf[5] = (st.second % 10) + 1;
		}

		UINT8 count = 13;
		for (int i = m_cCharaters; i--; )
		{
			if (m_cTime[i] != buf[i])
			{
				m_cTime[i] = buf[i];
				m_nCounter.b[i] = count;
				m_cDirty |= (1 << i);
				count += 4;
			}
		}
	}
}

/**
 * 再描画
 */
void DispClock::Redraw()
{
	m_cDirty = 0x3f;
}

/**
 * 描画が必要?
 * @retval true 描画中
 * @retval false 非描画
 */
bool DispClock::IsDisplayed() const
{
	return ((m_cDirty != 0) || (m_nCounter.q != 0));
}

//! オフセット テーブル
static const UINT8 s_dclocky[13] = {0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3};

/**
 * 位置計算
 * @param[in] nCount カウント値
 * @return オフセット
 */
UINT8 DispClock::CountPos(UINT nCount)
{
	if (nCount < _countof(s_dclocky))
	{
		return s_dclocky[nCount];
	}
	else
	{
		return 255;
	}
}

/**
 * カウントダウン処理
 * @param[in] nFrames フレーム数
 */
void DispClock::CountDown(UINT nFrames)
{
	if ((m_cCharaters == 0) || (m_nCounter.q == 0))
	{
		return;
	}

	if (nFrames == 0)
	{
		nFrames = 1;
	}

	for (UINT i = 0; i < m_cCharaters; i++)
	{
		UINT nRemain = m_nCounter.b[i];
		if (nRemain == 0)
		{
			continue;
		}
		const UINT8 y = CountPos(nRemain);

		if (nFrames < nRemain)
		{
			nRemain -= nFrames;
		}
		else
		{
			nRemain = 0;
		}
		if (y != CountPos(nRemain))
		{
			m_cDirty |= (1 << i);
		}
		m_nCounter.b[i] = nRemain;
	}
}

/**
 * バッファに描画
 * @retval true 更新あり
 * @retval false 更新なし
 */
bool DispClock::Make()
{
	if ((m_cCharaters == 0) || (m_cDirty == 0))
	{
		return false;
	}

	for (UINT i = 0; i < m_cCharaters; i++)
	{
		if ((m_cDirty & (1 << i)) == 0)
		{
			continue;
		}

		UINT nNumber = m_cTime[i];
		UINT nPadding = 3;

		const UINT nRemain = m_nCounter.b[i];
		if (nRemain == 0)
		{
			m_cLastTime[i] = nNumber;
		}
		else if (nRemain < _countof(s_dclocky))
		{
			nPadding -= s_dclocky[nRemain];
		}
		else
		{
			nNumber = this->m_cLastTime[i];
		}

		UINT8* q = m_buffer + (m_pPattern->cPosition[i] >> 3);
		const UINT8 cShifter = m_pPattern->cPosition[i] & 7;
		const UINT8 cMask0 = ~(m_pPattern->cMask >> cShifter);
		const UINT8 cMask1 = ~(m_pPattern->cMask << (8 - cShifter));
		for (UINT y = 0; y < nPadding; y++)
		{
			q[0] = (q[0] & cMask0);
			q[1] = (q[1] & cMask1);
			q += DCLOCK_YALIGN;
		}

		const UINT8* p = m_pPattern->font[nNumber];
		for (UINT y = 0; y < 9; y++)
		{
			q[0] = (q[0] & cMask0) | (p[y] >> cShifter);
			q[1] = (q[1] & cMask1) | (p[y] << (8 - cShifter));
			q += DCLOCK_YALIGN;
		}
	}
	m_cDirty = 0;
	return true;
}

/**
 * 描画
 * @param[in] nBpp 色数
 * @param[out] lpBuffer 描画ポインタ
 * @param[in] nYAlign アライメント
 */
void DispClock::Draw(UINT nBpp, void* lpBuffer, int nYAlign) const
{
	switch (nBpp)
	{
		case 8:
			Draw8(lpBuffer, nYAlign);
			break;

		case 16:
			Draw16(lpBuffer, nYAlign);
			break;

		case 24:
			Draw24(lpBuffer, nYAlign);
			break;

		case 32:
			Draw32(lpBuffer, nYAlign);
			break;
	}
}

/**
 * 描画(8bpp)
 * @param[out] lpBuffer 描画ポインタ
 * @param[in] nYAlign アライメント
 */
void DispClock::Draw8(void* lpBuffer, int nYAlign) const
{
	const UINT8* p = m_buffer;

	for (UINT i = 0; i < 4; i++)
	{
		const UINT32* pPattern = m_pal8[i];
		for (UINT j = 0; j < 3; j++)
		{
			for (UINT x = 0; x < DCLOCK_YALIGN; x++)
			{
				(static_cast<UINT32*>(lpBuffer))[x * 2 + 0] = pPattern[p[x] >> 4];
				(static_cast<UINT32*>(lpBuffer))[x * 2 + 1] = pPattern[p[x] & 15];
			}
			p += DCLOCK_YALIGN;
			lpBuffer = reinterpret_cast<void*>(reinterpret_cast<INTPTR>(lpBuffer) + nYAlign);
		}
	}
}

/**
 * 描画(16bpp)
 * @param[out] lpBuffer 描画ポインタ
 * @param[in] nYAlign アライメント
 */
void DispClock::Draw16(void* lpBuffer, int nYAlign) const
{
	const UINT8* p = m_buffer;

	for (UINT i = 0; i < 4; i++)
	{
		const RGB16 pal = m_pal16[i];
		for (UINT j = 0; j < 3; j++)
		{
			for (UINT x = 0; x < (8 * DCLOCK_YALIGN); x++)
			{
				(static_cast<UINT16*>(lpBuffer))[x] = (p[x >> 3] & (0x80 >> (x & 7))) ? pal : 0;
			}
			p += DCLOCK_YALIGN;
			lpBuffer = reinterpret_cast<void*>(reinterpret_cast<INTPTR>(lpBuffer) + nYAlign);
		}
	}
}

/**
 * 描画(24bpp)
 * @param[out] lpBuffer 描画ポインタ
 * @param[in] nYAlign アライメント
 */
void DispClock::Draw24(void* lpBuffer, int nYAlign) const
{
	const UINT8* p = m_buffer;
	UINT8* q = static_cast<UINT8*>(lpBuffer);

	for (UINT i = 0; i < 4; i++)
	{
		const RGB32 pal = m_pal32[i];
		for (UINT j = 0; j < 3; j++)
		{
			for (UINT x = 0; x < (8 * DCLOCK_YALIGN); x++)
			{
				if (p[x >> 3] & (0x80 >> (x & 7)))
				{
					q[0] = pal.p.b;
					q[1] = pal.p.g;
					q[2] = pal.p.g;
				}
				else
				{
					q[0] = 0;
					q[1] = 1;
					q[2] = 2;
				}
				q += 3;
			}
			p += DCLOCK_YALIGN;
			q += nYAlign - (8 * DCLOCK_YALIGN) * 3;
		}
	}
}

/**
 * 描画(32bpp)
 * @param[out] lpBuffer 描画ポインタ
 * @param[in] nYAlign アライメント
 */
void DispClock::Draw32(void* lpBuffer, int nYAlign) const
{
	const UINT8* p = m_buffer;

	for (UINT i = 0; i < 4; i++)
	{
		const UINT32 pal = m_pal32[i].d;
		for (UINT j = 0; j < 3; j++)
		{
			for (UINT x = 0; x < (8 * DCLOCK_YALIGN); x++)
			{
				(static_cast<UINT32*>(lpBuffer))[x] = (p[x >> 3] & (0x80 >> (x & 7))) ? pal : 0;
			}
			p += DCLOCK_YALIGN;
			lpBuffer = reinterpret_cast<void*>(reinterpret_cast<INTPTR>(lpBuffer) + nYAlign);
		}
	}
}
