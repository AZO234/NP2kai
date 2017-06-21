/**
 * @file	extrom.h
 * @brief	EXTROM リソース クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

/**
 * @brief EXTROM リソース クラス
 */
class CExtRom
{
public:
	CExtRom();
	~CExtRom();
	bool Open(LPCTSTR lpFilename);
	void Close();
	UINT Read(LPVOID lpBuffer, UINT cbBuffer);
	LONG Seek(LONG lDistanceToMove, DWORD dwMoveMethod);

private:
	HGLOBAL m_hGlobal;	//!< ハンドル
	LPVOID m_lpRes;		//!< リソース
	UINT m_nSize;		//!< サイズ
	UINT m_nPointer;	//!< ポインタ
};
