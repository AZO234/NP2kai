/**
 * @file	externalpsg.h
 * @brief	外部 PSG 演奏クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

#include "externalchip.h"

/**
 * @brief 外部 PSG 演奏クラス
 */
class CExternalPsg : public IExternalChip
{
public:
	CExternalPsg(IExternalChip* pChip);
	virtual ~CExternalPsg();
	virtual ChipType GetChipType();
	virtual void Reset();
	virtual void WriteRegister(UINT nAddr, UINT8 cData);
	virtual INTPTR Message(UINT nMessage, INTPTR nParameter);

protected:
	IExternalChip* m_pChip;				//!< チップ
	UINT8 m_cPsgMix;					//!< PSG ミキサー

	virtual void Mute(bool bMute) const;
	void WriteRegisterInner(UINT nAddr, UINT8 cData) const;
};
