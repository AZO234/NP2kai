/**
 * @file	externalopl3.h
 * @brief	外部 OPL3 演奏クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

#include "externalchip.h"

/**
 * @brief 外部 OPL3 演奏クラス
 */
class CExternalOpl3 : public IExternalChip
{
public:
	CExternalOpl3(IExternalChip* pChip);
	virtual ~CExternalOpl3();
	virtual ChipType GetChipType();
	virtual void Reset();
	virtual void WriteRegister(UINT nAddr, UINT8 cData);
	virtual INTPTR Message(UINT nMessage, INTPTR nParameter);

protected:
	IExternalChip* m_pChip;				/*!< チップ*/
	UINT8 m_cKon[2][16];				/*!< KON テーブル */

	void Mute(bool bMute);
	void WriteRegisterInner(UINT nAddr, UINT8 cData) const;
};
