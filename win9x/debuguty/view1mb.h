/**
 * @file	view1mb.h
 * @brief	メイン メモリ レジスタ表示クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

#include <vector>
#include "viewitem.h"
#include "viewmem.h"

/**
 * @brief メモリ レジスタ表示クラス
 */
class CDebugUty1MB : public CDebugUtyItem
{
public:
	CDebugUty1MB(CDebugUtyView* lpView);
	virtual ~CDebugUty1MB();

	virtual void Initialize(const CDebugUtyItem* lpItem = NULL);
	virtual bool Update();
	virtual bool Lock();
	virtual void Unlock();
	virtual bool IsLocked();
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual void OnPaint(HDC hDC, const RECT& rect);

private:
	DebugUtyViewMemory m_mem;					//!< メモリ
	std::vector<unsigned char> m_buffer;		//!< バッファ
	void SetSegment(UINT nSegment);
};
