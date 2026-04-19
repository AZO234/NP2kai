/**
 * @file	externalopm.h
 * @brief	外部 OPM 演奏クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

#include "externalchip.h"

/**
 * @brief 外部 OPM 演奏クラス
 */
class CExternalOpm : public IExternalChip
{
public:
	CExternalOpm(IExternalChip* pChip);
	virtual ~CExternalOpm();
	virtual ChipType GetChipType();
	virtual void Reset();
	virtual void WriteRegister(UINT nAddr, UINT8 cData);
	virtual INTPTR Message(UINT nMessage, INTPTR nParameter);

protected:
	IExternalChip* m_pChip;				/*!< チップ */
	UINT8 m_cAlgorithm[8];				/*!< アルゴリズム テーブル */
	UINT8 m_cTtl[8 * 4];				/*!< TTL テーブル */

	void Mute(bool bMute) const;
	void WriteRegisterInner(UINT nAddr, UINT8 cData) const;
	void SetVolume(UINT nChannel, int nVolume) const;
};
