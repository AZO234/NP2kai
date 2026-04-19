/**
 * @file	viewreg.h
 * @brief	レジスタ表示クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

#include <vector>
#include "viewitem.h"

/**
 * @brief レジスタ表示クラス
 */
class CDebugUtyReg : public CDebugUtyItem
{
public:
	CDebugUtyReg(CDebugUtyView* lpView);
	virtual ~CDebugUtyReg();

	virtual void Initialize(const CDebugUtyItem* lpItem = NULL);
	virtual bool Update();
	virtual bool Lock();
	virtual void Unlock();
	virtual bool IsLocked();
	virtual void OnPaint(HDC hDC, const RECT& rect);

private:
	std::vector<unsigned char> m_buffer;		//!< バッファ
};
