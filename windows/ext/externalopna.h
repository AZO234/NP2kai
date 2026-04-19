/**
 * @file	externalopna.h
 * @brief	外部 OPNA 演奏クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

#include "externalpsg.h"

/**
 * @brief 外部 OPNA 演奏クラス
 */
class CExternalOpna : public CExternalPsg
{
public:
	CExternalOpna(IExternalChip* pChip);
	virtual ~CExternalOpna();
	bool HasPsg() const;
	bool HasRhythm() const;
	bool HasADPCM() const;
	virtual void Reset();
	virtual void WriteRegister(UINT nAddr, UINT8 cData);

protected:
	bool m_bHasPsg;						/*!< PSG */
	bool m_bHasExtend;					/*!< Extend */
	bool m_bHasRhythm;					/*!< Rhythm */
	bool m_bHasADPCM;					/*!< ADPCM */
	UINT8 m_cMode;						/*!< モード */
	UINT8 m_cAlgorithm[8];				/*!< アルゴリズム テーブル */
	UINT8 m_cTtl[8 * 4];				/*!< TTL テーブル */

	virtual void Mute(bool bMute) const;
	void SetVolume(UINT nChannel, int nVolume) const;
};

/**
 * PSG を持っている?
 * @retval true 有効
 * @retval false 無効
 */
inline bool CExternalOpna::HasPsg() const
{
	return m_bHasPsg;
}

/**
 * Rhythm を持っている?
 * @retval true 有効
 * @retval false 無効
 */
inline bool CExternalOpna::HasRhythm() const
{
	return m_bHasRhythm;
}

/**
 * ADPCM のバッファを持っている?
 * @retval true 有効
 * @retval false 無効
 */
inline bool CExternalOpna::HasADPCM() const
{
	return m_bHasADPCM;
}
