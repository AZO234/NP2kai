/**
 * @file	viewsnd.cpp
 * @brief	サウンド レジスタ表示クラスの動作の定義を行います
 */

#include "compiler.h"
#include "strres.h"
#include "resource.h"
#include "np2.h"
#include "viewsnd.h"
#include "viewer.h"
#include "pccore.h"
#include "iocore.h"
#include "sound.h"
#include "fmboard.h"

/**
 * @brief 表示アイテム
 */
struct SoundRegisterTable
{
	LPCTSTR lpString;		//!< 文字列
	UINT16 wAddress;		//!< アドレス
	UINT16 wMask;			//!< 表示マスク
};

//! テーブル
static const SoundRegisterTable s_table[] =
{
	{TEXT("Sound-Board I"), 0, 0},
	{NULL, 0x0000, 0xffff},
	{NULL, 0x0010, 0x3f07},
	{NULL, 0x0020, 0x07f6},
	{NULL, 0x0030, 0x7777},
	{NULL, 0x0040, 0x7777},
	{NULL, 0x0050, 0x7777},
	{NULL, 0x0060, 0x7777},
	{NULL, 0x0070, 0x7777},
	{NULL, 0x0080, 0x7777},
	{NULL, 0x0090, 0x7777},
	{NULL, 0x00a0, 0x7777},
	{NULL, 0x00b0, 0x0077},
	{str_null, 0, 0},
	{NULL, 0x0100, 0xffff},
	{NULL, 0x0110, 0x0001},
	{NULL, 0x0130, 0x7777},
	{NULL, 0x0140, 0x7777},
	{NULL, 0x0150, 0x7777},
	{NULL, 0x0160, 0x7777},
	{NULL, 0x0170, 0x7777},
	{NULL, 0x0180, 0x7777},
	{NULL, 0x0190, 0x7777},
	{NULL, 0x01a0, 0x7777},
	{NULL, 0x01b0, 0x0077},
#if 0
	{str_null, 0, 0},
	{TEXT("Sound-Board II"), 0, 0},
	{NULL, 0x0200, 0xffff},
	{NULL, 0x0220, 0x07e6},
	{NULL, 0x0230, 0x7777},
	{NULL, 0x0240, 0x7777},
	{NULL, 0x0250, 0x7777},
	{NULL, 0x0260, 0x7777},
	{NULL, 0x0270, 0x7777},
	{NULL, 0x0280, 0x7777},
	{NULL, 0x0290, 0x7777},
	{NULL, 0x02a0, 0x7777},
	{NULL, 0x02b0, 0x0077},
	{str_null, 0, 0},
	{NULL, 0x0230, 0x7777},
	{NULL, 0x0240, 0x7777},
	{NULL, 0x0250, 0x7777},
	{NULL, 0x0260, 0x7777},
	{NULL, 0x0270, 0x7777},
	{NULL, 0x0280, 0x7777},
	{NULL, 0x0290, 0x7777},
	{NULL, 0x02a0, 0x7777},
	{NULL, 0x02b0, 0x0077}
#endif
};

/**
 * コンストラクタ
 * @param[in] lpView ビューワ インスタンス
 */
CDebugUtySnd::CDebugUtySnd(CDebugUtyView* lpView)
	: CDebugUtyItem(lpView, IDM_VIEWMODESND)
{
}

/**
 * デストラクタ
 */
CDebugUtySnd::~CDebugUtySnd()
{
}

/**
 * 初期化
 * @param[in] lpItem 基準となるアイテム
 */
void CDebugUtySnd::Initialize(const CDebugUtyItem* lpItem)
{
	m_lpView->SetVScroll(0, _countof(s_table));
}

/**
 * 更新
 * @retval true 更新あり
 * @retval false 更新なし
 */
bool CDebugUtySnd::Update()
{
	return m_buffer.empty();
}

/**
 * ロック
 * @retval true 成功
 * @retval false 失敗
 */
bool CDebugUtySnd::Lock()
{
	m_buffer.resize(0x200);
	CopyMemory(&m_buffer.at(0), g_opna[0].s.reg, 0x200);
	return true;
}

/**
 * アンロック
 */
void CDebugUtySnd::Unlock()
{
	m_buffer.clear();
}

/**
 * ロック中?
 * @retval true ロック中である
 * @retval false ロック中でない
 */
bool CDebugUtySnd::IsLocked()
{
	return (!m_buffer.empty());
}

/**
 * 描画
 * @param[in] hDC デバイス コンテキスト
 * @param[in] rect 領域
 */
void CDebugUtySnd::OnPaint(HDC hDC, const RECT& rect)
{
	UINT nIndex = m_lpView->GetVScrollPos();
	for (int y = 0; (y < rect.bottom) && (nIndex < _countof(s_table)); y += 16, nIndex++)
	{
		const SoundRegisterTable& item = s_table[nIndex];
		if (item.lpString)
		{
			::TextOut(hDC, 0, y, item.lpString, ::lstrlen(item.lpString));
		}
		else
		{
			TCHAR szTmp[8];
			::wsprintf(szTmp, TEXT("%04x"), item.wAddress);
			TextOut(hDC, 8, y, szTmp, 4);

			const unsigned char* p = NULL;
			if (!m_buffer.empty())
			{
				p = &m_buffer.at(item.wAddress);
			}
			else
			{
				p = g_opna[0].s.reg + item.wAddress;
			}
			for (int x = 0; x < 16; x++)
			{
				if (item.wMask & (1 << x))
				{
					TCHAR szTmp[4];
					::wsprintf(szTmp, TEXT("%02X"), p[x]);
					::TextOut(hDC, ((x * 3) + 6) * 8, y, szTmp, 2);
				}
			}
		}
	}
}
