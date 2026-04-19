/**
 * @file	viewsnd.h
 * @brief	サウンド レジスタ表示クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

#include <vector>
#include "viewitem.h"

/**
 * @brief サウンド レジスタ表示クラス
 */
class CDebugUtySnd : public CDebugUtyItem
{
public:
	CDebugUtySnd(CDebugUtyView* lpView);
	virtual ~CDebugUtySnd();

	virtual void Initialize(const CDebugUtyItem* lpItem = NULL);
	virtual bool Update();
	virtual bool Lock();
	virtual void Unlock();
	virtual bool IsLocked();
	virtual void OnPaint(HDC hDC, const RECT& rect);

private:
	std::vector<unsigned char> m_buffer;		//!< バッファ
};
