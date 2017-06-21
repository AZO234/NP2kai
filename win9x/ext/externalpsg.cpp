/**
 * @file	externalpsg.cpp
 * @brief	外部 PSG 演奏クラスの動作の定義を行います
 */

#include "compiler.h"
#include "externalpsg.h"

/**
 * コンストラクタ
 * @param[in] pChip チップ
 */
CExternalPsg::CExternalPsg(IExternalChip* pChip)
	: m_pChip(pChip)
	, m_cPsgMix(0x3f)
{
}

/**
 * デストラクタ
 */
CExternalPsg::~CExternalPsg()
{
	delete m_pChip;
}

/**
 * チップ タイプを得る
 * @return チップ タイプ
 */
IExternalChip::ChipType CExternalPsg::GetChipType()
{
	return m_pChip->GetChipType();
}

/**
 * 音源リセット
 */
void CExternalPsg::Reset()
{
	m_cPsgMix = 0x3f;
	m_pChip->Reset();
}

/**
 * レジスタ書き込み
 * @param[in] nAddr アドレス
 * @param[in] cData データ
 */
void CExternalPsg::WriteRegister(UINT nAddr, UINT8 cData)
{
	if (nAddr < 0x0e)
	{
		if (nAddr == 0x07)
		{
			// psg mix
			cData &= 0x3f;
			if (m_cPsgMix == cData)
			{
				return;
			}
			m_cPsgMix = cData;
		}
		WriteRegisterInner(nAddr, cData);
	}
}

/**
 * メッセージ
 * @param[in] nMessage メッセージ
 * @param[in] nParameter パラメータ
 * @return 結果
 */
INTPTR CExternalPsg::Message(UINT nMessage, INTPTR nParameter)
{
	switch (nMessage)
	{
		case kMute:
			Mute(nParameter != 0);
			break;
	}
	return 0;
}

/**
 * ミュート
 * @param[in] bMute ミュート
 */
void CExternalPsg::Mute(bool bMute) const
{
	WriteRegisterInner(0x07, (bMute) ? 0x3f : m_cPsgMix);
}

/**
 * レジスタ書き込み(内部)
 * @param[in] nAddr アドレス
 * @param[in] cData データ
 */
void CExternalPsg::WriteRegisterInner(UINT nAddr, UINT8 cData) const
{
	m_pChip->WriteRegister(nAddr, cData);
}
