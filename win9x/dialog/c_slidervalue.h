/**
 * @file	c_slidervalue.h
 * @brief	値付きスライダー クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

#include "misc/DlgProc.h"

/**
 * @brief スライダー クラス
 */
class CSliderValue : public CSliderProc
{
public:
	CSliderValue();
	void SetRange(int nMin, int nMax, BOOL bRedraw = FALSE);
	void SetPos(int nPos);
	void SetStaticId(UINT nId);
	void UpdateValue();

private:
	UINT m_nStaticId;			/*!< 値コントロール */
};

/**
 * 値コントロールの指定
 * @param[in] nId コントロール ID
 */
inline void CSliderValue::SetStaticId(UINT nId)
{
	m_nStaticId = nId;
}
