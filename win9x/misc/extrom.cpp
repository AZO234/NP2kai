/**
 * @file	extrom.cpp
 * @brief	EXTROM リソース クラスの動作の定義を行います
 */

#include "compiler.h"
#include "ExtRom.h"
#include "np2.h"
#include "WndProc.h"
#include "dosio.h"
#include "pccore.h"


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
 * オープン
 * @param[in] lpFilename ファイル名
 * @param[in] extlen ファイル名の後ろ何文字を拡張子扱いするか
 * @retval true 成功
 * @retval false 失敗
 */
bool CExtRom::Open(LPCTSTR lpFilename, DWORD extlen)
{
	TCHAR tmpfilesname[MAX_PATH];
	TCHAR tmpfilesname2[MAX_PATH];
	FILEH	fh;

	Close();
	
	_tcscpy(tmpfilesname, lpFilename);
	int fnamelen = (int)_tcslen(tmpfilesname);
	for (int i=0; i<(int)extlen+1; i++)
	{
		tmpfilesname[fnamelen+1-i] = tmpfilesname[fnamelen+1-i-1];
	}
	tmpfilesname[fnamelen-extlen] = '.';
	getbiospath(tmpfilesname2, tmpfilesname, NELEMENTS(tmpfilesname2));
	fh = file_open_rb(tmpfilesname2);
	if (fh != FILEH_INVALID)
	{
		m_nSize = (UINT)file_getsize(fh);
		m_lpRes = malloc(m_nSize);
		file_read(fh, m_lpRes, m_nSize);
		m_hGlobal = NULL;
		m_nPointer = 0;
		m_isfile = 1;
		file_close(fh);
	}
	else
	{
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
	}

	return true;
}

/**
 * オープン
 * @param[in] lpFilename ファイル名
 * @param[in] lpExt 外部ファイルの場合の拡張子
 * @retval true 成功
 * @retval false 失敗
 */
bool CExtRom::Open(LPCTSTR lpFilename, LPCTSTR lpExt)
{
	TCHAR tmpfilesname[MAX_PATH];
	TCHAR tmpfilesname2[MAX_PATH];
	FILEH	fh;

	Close();
	
	_tcscpy(tmpfilesname, lpFilename);
	_tcscat(tmpfilesname, lpExt);
	getbiospath(tmpfilesname2, tmpfilesname, NELEMENTS(tmpfilesname2));
	fh = file_open_rb(tmpfilesname2);
	if (fh != FILEH_INVALID)
	{
		m_nSize = (UINT)file_getsize(fh);
		m_lpRes = malloc(m_nSize);
		file_read(fh, m_lpRes, m_nSize);
		m_hGlobal = NULL;
		m_nPointer = 0;
		m_isfile = 1;
		file_close(fh);
	}
	else
	{
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
	}

	return true;
}

/**
 * クローズ
 */
void CExtRom::Close()
{
	if (m_isfile)
	{
		if (m_lpRes)
		{
			free(m_lpRes);
		}
		m_isfile = 0;
	}
	else
	{
		if (m_hGlobal)
		{
			::FreeResource(m_hGlobal);
			m_hGlobal = NULL;
		}
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
