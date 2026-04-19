/**
 * @file	viewasm.h
 * @brief	アセンブラ リスト表示クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

#include <vector>
#include "viewitem.h"
#include "viewmem.h"

/**
 * @brief アセンブラ リスト表示クラス
 */
class CDebugUtyAsm : public CDebugUtyItem
{
public:
	CDebugUtyAsm(CDebugUtyView* lpView);
	virtual ~CDebugUtyAsm();

	virtual void Initialize(const CDebugUtyItem* lpItem = NULL);
	virtual bool Update();
	virtual bool Lock();
	virtual void Unlock();
	virtual bool IsLocked();
	virtual void OnPaint(HDC hDC, const RECT& rect);

private:
	UINT m_nSegment;							//!< セグメント
	UINT m_nOffset;								//!< セグメント
	DebugUtyViewMemory m_mem;					//!< メモリ
	std::vector<unsigned char> m_buffer;		//!< バッファ
	std::vector<UINT> m_address;				//!< アドレス バッファ
	void ReadMemory(UINT nOffset, unsigned char* lpBuffer, UINT cbBuffer) const;
};
