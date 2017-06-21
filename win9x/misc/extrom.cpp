/**
 * @file	extrom.cpp
 * @brief	EXTROM リソース クラスの動作の定義を行います
 */

#include "compiler.h"
#include "ExtRom.h"
#include "np2.h"
#include "WndProc.h"

//! リソース名
static const TCHAR s_szExtRom[] = TEXT("EXTROM");

/**
 * コンストラクタ
 */
CExtRom::CExtRom()
	: m_hGlobal(NULL)
	, m_lpRes(NULL)
	, m_nSize(0)
	, m_nPointer(0)
{
}

/**
 * デストラクタ
 */
CExtRom::~CExtRom()
{
	Close();
}

/**
 * オープン
 * @param[in] lpFilename ファイル名
 * @retval true 成功
 * @retval false 失敗
 */
bool CExtRom::Open(LPCTSTR lpFilename)
{
	Close();

	HINSTANCE hInstance = CWndProc::FindResourceHandle(lpFilename, s_szExtRom);
	HRSRC hRsrc = ::FindResource(hInstance, lpFilename, s_szExtRom);
	if (hRsrc == NULL)
	{
		return false;
	}

	m_hGlobal = ::LoadResource(hInstance, hRsrc);
	m_lpRes = ::LockResource(m_hGlobal);
	m_nSize = ::SizeofResource(hInstance, hRsrc);
	m_nPointer = 0;
	return true;
}

/**
 * クローズ
 */
void CExtRom::Close()
{
	if (m_hGlobal)
	{
		::FreeResource(m_hGlobal);
		m_hGlobal = NULL;
	}
	m_lpRes = NULL;
	m_nSize = 0;
	m_nPointer = 0;
}

/**
 * 読み込み
 * @param[out] lpBuffer バッファ
 * @param[out] cbBuffer バッファ長
 * @return サイズ
 */
UINT CExtRom::Read(LPVOID lpBuffer, UINT cbBuffer)
{
	UINT nLength = m_nSize - m_nPointer;
	nLength = min(nLength, cbBuffer);
	if (nLength)
	{
		if (lpBuffer)
		{
			CopyMemory(lpBuffer, static_cast<char*>(m_lpRes) + m_nPointer, nLength);
		}
		m_nPointer += nLength;
	}
	return nLength;
}

/**
 * シーク
 * @param[in] lDistanceToMove ファイルポインタの移動バイト数
 * @param[in] dwMoveMethod ファイルポインタを移動するための開始点（基準点）を指定します
 * @return 現在の位置
 */
LONG CExtRom::Seek(LONG lDistanceToMove, DWORD dwMoveMethod)
{
	switch (dwMoveMethod)
	{
		case FILE_BEGIN:
		default:
			break;

		case FILE_CURRENT:
			lDistanceToMove += m_nPointer;
			break;

		case FILE_END:
			lDistanceToMove += m_nSize;
			break;
	}

	if (lDistanceToMove < 0)
	{
		lDistanceToMove = 0;
	}
	else if (static_cast<UINT>(lDistanceToMove) > m_nSize)
	{
		lDistanceToMove = m_nSize;
	}
	m_nPointer = lDistanceToMove;
	return lDistanceToMove;
}
