/**
 * @file	viewasm.cpp
 * @brief	アセンブラ リスト表示クラスの動作の定義を行います
 */

#include "compiler.h"
#include "resource.h"
#include "np2.h"
#include "viewasm.h"
#include "viewer.h"
#include "unasm.h"
#include "cpucore.h"

/**
 * コンストラクタ
 * @param[in] lpView ビューワ インスタンス
 */
CDebugUtyAsm::CDebugUtyAsm(CDebugUtyView* lpView)
	: CDebugUtyItem(lpView, IDM_VIEWMODEASM)
	, m_nSegment(0)
	, m_nOffset(0)
{
}

/**
 * デストラクタ
 */
CDebugUtyAsm::~CDebugUtyAsm()
{
}

/**
 * 初期化
 * @param[in] lpItem 基準となるアイテム
 */
void CDebugUtyAsm::Initialize(const CDebugUtyItem* lpItem)
{
	m_nSegment = CPU_CS;
	m_nOffset = CPU_IP;
	m_lpView->SetVScroll(0, 0x1000);
}

/**
 * 更新
 * @retval true 更新あり
 * @retval false 更新なし
 */
bool CDebugUtyAsm::Update()
{
	if (!m_buffer.empty())
	{
		return false;
	}

	m_nSegment = CPU_CS;
	m_nOffset = CPU_IP;
	m_lpView->SetVScrollPos(0);
	m_mem.Update();
	m_address.clear();
	return true;
}

/**
 * ロック
 * @retval true 成功
 * @retval false 失敗
 */
bool CDebugUtyAsm::Lock()
{
	m_buffer.resize(0x10000);
	m_address.clear();

	m_mem.Update();
	m_mem.Read(m_nSegment << 4, &m_buffer.at(0), static_cast<UINT>(m_buffer.size()));
	return true;
}

/**
 * アンロック
 */
void CDebugUtyAsm::Unlock()
{
	m_buffer.clear();
	m_address.clear();
}

/**
 * ロック中?
 * @retval true ロック中である
 * @retval false ロック中でない
 */
bool CDebugUtyAsm::IsLocked()
{
	return (!m_buffer.empty());
}

/**
 * 描画
 * @param[in] hDC デバイス コンテキスト
 * @param[in] rect 領域
 */
void CDebugUtyAsm::OnPaint(HDC hDC, const RECT& rect)
{
	UINT nIndex = m_lpView->GetVScrollPos();

	if (m_address.size() < nIndex)
	{
		UINT nOffset = (!m_address.empty()) ? m_address.back() : m_nOffset;
		do
		{
			unsigned char sBuf[16];
			ReadMemory(nOffset, sBuf, sizeof(sBuf));

			UINT nStep = ::unasm(NULL, sBuf, sizeof(sBuf), FALSE, nOffset);
			if (nStep == 0)
			{
				return;
			}

			nOffset = (nOffset + nStep) & 0xffff;
			m_address.push_back(nOffset);
		} while (m_address.size() < nIndex);
	}

	UINT nOffset = (nIndex) ? m_address[nIndex - 1] : m_nOffset;
	for (int y = 0; (y < rect.bottom) && (nIndex < 0x1000); y += 16, nIndex++)
	{
		TCHAR szTmp[16];
		::wsprintf(szTmp, _T("%04x:%04x"), m_nSegment, nOffset);
		::TextOut(hDC, 0, y, szTmp, 9);

		unsigned char sBuf[16];
		ReadMemory(nOffset, sBuf, sizeof(sBuf));

		_UNASM una;
		UINT nStep = ::unasm(&una, sBuf, sizeof(sBuf), FALSE, nOffset);
		if (nStep == 0)
		{
			return;
		}

		::TextOutA(hDC, 11 * 8, y, una.mnemonic, ::lstrlenA(una.mnemonic));
		if (una.operand[0])
		{
			::TextOutA(hDC, (11 + 7) * 8, y, una.operand, ::lstrlenA(una.operand));
		}

		nOffset = (nOffset + nStep) & 0xffff;
		if (m_address.size() == nIndex)
		{
			m_address.push_back(nOffset);
		}
	}
}

/**
 * メモリ取得
 * @param[in] nOffset オフセット
 * @param[out] lpBuffer バッファ
 * @param[in] cbBuffer バッファ長
 */
void CDebugUtyAsm::ReadMemory(UINT nOffset, unsigned char* lpBuffer, UINT cbBuffer) const
{
	while (cbBuffer)
	{
		const UINT nLimit = min(nOffset + cbBuffer, 0x10000);
		const UINT nSize = nLimit - nOffset;

		if (!m_buffer.empty())
		{
			CopyMemory(lpBuffer, &m_buffer.at(nOffset), nSize);
		}
		else
		{
			m_mem.Read((m_nSegment << 4) + nOffset, lpBuffer, nSize);
		}

		nOffset = (nOffset + nSize) & 0xffff;
		lpBuffer += nSize;
		cbBuffer -= nSize;
	}
}
