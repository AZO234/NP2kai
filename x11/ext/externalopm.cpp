/**
 * @file	externalopm.cpp
 * @brief	外部 OPM 演奏クラスの動作の定義を行います
 */

#include "compiler.h"
#include "externalopm.h"

/**
 * コンストラクタ
 * @param[in] pChip チップ
 */
CExternalOpm::CExternalOpm(IExternalChip* pChip)
	: m_pChip(pChip)
{
	memset(m_cAlgorithm, 0, sizeof(m_cAlgorithm));
	memset(m_cTtl, 0x7f, sizeof(m_cTtl));
}

/**
 * デストラクタ
 */
CExternalOpm::~CExternalOpm()
{
	delete m_pChip;
}

/**
 * チップ タイプを得る
 * @return チップ タイプ
 */
IExternalChip::ChipType CExternalOpm::GetChipType()
{
	return m_pChip->GetChipType();
}

/**
 * 音源リセット
 */
void CExternalOpm::Reset()
{
	memset(m_cAlgorithm, 0, sizeof(m_cAlgorithm));
	memset(m_cTtl, 0x7f, sizeof(m_cTtl));
	m_pChip->Reset();
}

/**
 * レジスタ書き込み
 * @param[in] nAddr アドレス
 * @param[in] cData データ
 */
void CExternalOpm::WriteRegister(UINT nAddr, UINT8 cData)
{
	if ((nAddr & 0xe0) == 0x60)					// ttl
	{
		m_cTtl[nAddr & 0x1f] = cData;
	}
	else if ((nAddr & 0xf8) == 0x20)			// algorithm
	{
		m_cAlgorithm[nAddr & 7] = cData;
	}
	WriteRegisterInner(nAddr, cData);
}

/**
 * メッセージ
 * @param[in] nMessage メッセージ
 * @param[in] nParameter パラメータ
 * @return 結果
 */
INTPTR CExternalOpm::Message(UINT nMessage, INTPTR nParameter)
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
void CExternalOpm::Mute(bool bMute) const
{
	const int nVolume = (bMute) ? -127 : 0;
	for (UINT ch = 0; ch < 8; ch++)
	{
		SetVolume(ch, nVolume);
	}
}

/**
 * レジスタ書き込み(内部)
 * @param[in] nAddr アドレス
 * @param[in] cData データ
 */
void CExternalOpm::WriteRegisterInner(UINT nAddr, UINT8 cData) const
{
	m_pChip->WriteRegister(nAddr, cData);
}

/**
 * ヴォリューム設定
 * @param[in] nChannel チャンネル
 * @param[in] nVolume ヴォリューム値
 */
void CExternalOpm::SetVolume(UINT nChannel, int nVolume) const
{
	/*! アルゴリズム スロット マスク */
	static const UINT8 s_opmask[] = {0x08, 0x08, 0x08, 0x08, 0x0c, 0x0e, 0x0e, 0x0f};
	UINT8 cMask = s_opmask[m_cAlgorithm[nChannel] & 7];

	int nOffset = nChannel;
	do
	{
		if (cMask & 1)
		{
			int nTtl = (m_cTtl[nOffset] & 0x7f) - nVolume;
			if (nTtl < 0)
			{
				nTtl = 0;
			}
			else if (nTtl > 0x7f)
			{
				nTtl = 0x7f;
			}
			WriteRegisterInner(0x60 + nOffset, static_cast<UINT8>(nTtl));
		}
		nOffset += 8;
		cMask >>= 1;
	} while (cMask != 0);
}
