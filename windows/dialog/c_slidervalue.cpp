/**
 * @file	c_slidervalue.cpp
 * @brief	値付きスライダー クラスの動作の定義を行います
 */

#include "compiler.h"
#include "c_slidervalue.h"

/**
 * コンストラクタ
 */
CSliderValue::CSliderValue()
	: m_nStaticId(0)
{
}

/**
 * 範囲の設定
 * @param[in] nMin 最小値
 * @param[in] nMax 最大値
 * @param[in] bRedraw 再描画フラグ
 */
void CSliderValue::SetRange(int nMin, int nMax, BOOL bRedraw)
{
	SetRangeMin(nMin, FALSE);
	SetRangeMax(nMax, bRedraw);
}

/**
 * 値の設定
 * @param[in] nPos 値
 */
void CSliderValue::SetPos(int nPos)
{
	CSliderProc::SetPos(nPos);
	UpdateValue();
}

/**
 * 値の更新
 */
void CSliderValue::UpdateValue()
{
	if (m_nStaticId)
	{
		GetParent().SetDlgItemInt(m_nStaticId, GetPos(), TRUE);
	}
}
