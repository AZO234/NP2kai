/**
 * @file	viewseg.h
 * @brief	メモリ レジスタ表示クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

#include <vector>
#include "viewitem.h"
#include "viewmem.h"

/**
 * @brief メモリ レジスタ表示クラス
 */
class CDebugUtySeg : public CDebugUtyItem
{
public:
	CDebugUtySeg(CDebugUtyView* lpView);
	virtual ~CDebugUtySeg();

	virtual void Initialize(const CDebugUtyItem* lpItem = NULL);
	virtual bool Update();
	virtual bool Lock();
	virtual void Unlock();
	virtual bool IsLocked();
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual void OnPaint(HDC hDC, const RECT& rect);

private:
	UINT m_nSegment;							//!< セグメント
	DebugUtyViewMemory m_mem;					//!< メモリ
	std::vector<unsigned char> m_buffer;		//!< バッファ
	void SetSegment(UINT nSegment);
};
