/**
 * @file	externalopl3.cpp
 * @brief	外部 OPL3 演奏クラスの動作の定義を行います
 */

#include "compiler.h"
#include "externalopl3.h"

/**
 * コンストラクタ
 * @param[in] pChip チップ
 */
CExternalOpl3::CExternalOpl3(IExternalChip* pChip)
	: m_pChip(pChip)
{
	memset(m_cKon, 0x00, sizeof(m_cKon));
}

/**
 * デストラクタ
 */
CExternalOpl3::~CExternalOpl3()
{
	delete m_pChip;
}

/**
 * チップ タイプを得る
 * @return チップ タイプ
 */
IExternalChip::ChipType CExternalOpl3::GetChipType()
{
	return m_pChip->GetChipType();
}

/**
 * 音源リセット
 */
void CExternalOpl3::Reset()
{
	m_pChip->Reset();
}

/**
 * レジスタ書き込み
 * @param[in] nAddr アドレス
 * @param[in] cData データ
 */
void CExternalOpl3::WriteRegister(UINT nAddr, UINT8 cData)
{
	if ((nAddr & 0xf0) == 0xb0)
	{
		m_cKon[(nAddr & 0x100) >> 8][nAddr & 0x0f] = cData;
	}
	WriteRegisterInner(nAddr, cData);
}

/**
 * メッセージ
 * @param[in] nMessage メッセージ
 * @param[in] nParameter パラメータ
 * @return 結果
 */
INTPTR CExternalOpl3::Message(UINT nMessage, INTPTR nParameter)
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
void CExternalOpl3::Mute(bool bMute)
{
	if (bMute)
	{
		for (UINT i = 0; i < 2; i++)
		{
			for (UINT j = 0; j < 9; j++)
			{
				if (m_cKon[i][j] & 0x20)
				{
					m_cKon[i][j] &= 0xdf;
					WriteRegisterInner((i * 0x100) + j + 0xb0, m_cKon[i][j]);
				}
			}
		}
	}
}

/**
 * レジスタ書き込み(内部)
 * @param[in] nAddr アドレス
 * @param[in] cData データ
 */
void CExternalOpl3::WriteRegisterInner(UINT nAddr, UINT8 cData) const
{
	m_pChip->WriteRegister(nAddr, cData);
}
