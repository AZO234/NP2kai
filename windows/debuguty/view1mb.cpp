/**
 * @file	view1mb.cpp
 * @brief	メイン メモリ表示クラスの動作の定義を行います
 */

#include "compiler.h"
#include "resource.h"
#include "np2.h"
#include "view1mb.h"
#include "viewer.h"
#include "cpucore.h"

/**
 * コンストラクタ
 * @param[in] lpView ビューワ インスタンス
 */
CDebugUty1MB::CDebugUty1MB(CDebugUtyView* lpView)
	: CDebugUtyItem(lpView, IDM_VIEWMODE1MB)
{
}

/**
 * デストラクタ
 */
CDebugUty1MB::~CDebugUty1MB()
{
}

/**
 * 初期化
 * @param[in] lpItem 基準となるアイテム
 */
void CDebugUty1MB::Initialize(const CDebugUtyItem* lpItem)
{
	m_lpView->SetVScroll(0, 0x10fff);
}

/**
 * 更新
 * @retval true 更新あり
 * @retval false 更新なし
 */
bool CDebugUty1MB::Update()
{
	if (!m_buffer.empty())
	{
		return false;
	}
	m_mem.Update();
	return true;
}

/**
 * ロック
 * @retval true 成功
 * @retval false 失敗
 */
bool CDebugUty1MB::Lock()
{
	m_buffer.resize(0x10fff0);

	m_mem.Update();
	m_mem.Read(0, &m_buffer.at(0), static_cast<UINT>(m_buffer.size()));
	return true;
}

/**
 * アンロック
 */
void CDebugUty1MB::Unlock()
{
	m_buffer.clear();
}

/**
 * ロック中?
 * @retval true ロック中である
 * @retval false ロック中でない
 */
bool CDebugUty1MB::IsLocked()
{
	return (!m_buffer.empty());
}

/**
 * ユーザーがメニューの項目を選択したときに、フレームワークによって呼び出されます
 * @param[in] wParam パラメタ
 * @param[in] lParam パラメタ
 * @retval TRUE アプリケーションがこのメッセージを処理した
 * @retval FALSE アプリケーションがこのメッセージを処理しなかった
 */
BOOL CDebugUty1MB::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
		case IDM_SEGCS:
			SetSegment(CPU_CS);
			break;

		case IDM_SEGDS:
			SetSegment(CPU_DS);
			break;

		case IDM_SEGES:
			SetSegment(CPU_ES);
			break;

		case IDM_SEGSS:
			SetSegment(CPU_SS);
			break;

		case IDM_SEGTEXT:
			SetSegment(0xa000);
			break;

		default:
			return FALSE;

	}
	return TRUE;
}

/**
 * セグメント変更
 * @param[in] nSegment セグメント
 */
void CDebugUty1MB::SetSegment(UINT nSegment)
{
	m_lpView->SetVScrollPos(nSegment);
}

/**
 * 描画
 * @param[in] hDC デバイス コンテキスト
 * @param[in] rect 領域
 */
void CDebugUty1MB::OnPaint(HDC hDC, const RECT& rect)
{
	UINT nIndex = m_lpView->GetVScrollPos();
	for (int y = 0; (y < rect.bottom) && (nIndex < 0x10fff); y += 16, nIndex++)
	{
		TCHAR szTmp[16];
		::wsprintf(szTmp, _T("%08x:"), nIndex << 4);
		::TextOut(hDC, 0, y, szTmp, 9);

		unsigned char sBuf[16];
		if (!m_buffer.empty())
		{
			CopyMemory(sBuf, &m_buffer.at(nIndex << 4), sizeof(sBuf));
		}
		else
		{
			m_mem.Read(nIndex << 4, sBuf, sizeof(sBuf));
		}
		for (int x = 0; x < 16; x++)
		{
			TCHAR szTmp[4];
			::wsprintf(szTmp, TEXT("%02X"), sBuf[x]);
			::TextOut(hDC, ((x * 3) + 10) * 8, y, szTmp, 2);
		}
	}
}
