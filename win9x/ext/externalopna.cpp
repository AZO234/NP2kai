/**
 * @file	externalopna.cpp
 * @brief	外部 OPNA 演奏クラスの動作の定義を行います
 */

#include "compiler.h"
#include "externalopna.h"

/**
 * コンストラクタ
 * @param[in] pChip チップ
 */
CExternalOpna::CExternalOpna(IExternalChip* pChip)
	: CExternalPsg(pChip)
	, m_bHasPsg(false)
	, m_bHasExtend(false)
	, m_bHasRhythm(false)
	, m_bHasADPCM(false)
	, m_cMode(0)
{
	memset(m_cAlgorithm, 0, sizeof(m_cAlgorithm));
	memset(m_cTtl, 0x7f, sizeof(m_cTtl));

	switch (GetChipType())
	{
		case IExternalChip::kYM2203:
			m_bHasPsg = true;
			break;

		case IExternalChip::kYM2608:
			m_bHasPsg = true;
			m_bHasExtend = true;
			m_bHasRhythm = true;
			m_bHasADPCM = true;
			break;

		case IExternalChip::kYM3438:
			m_bHasExtend = true;
			break;

		case IExternalChip::kYMF288:
			m_bHasPsg = true;
			m_bHasExtend = true;
			m_bHasRhythm = true;
			break;

		default:
			break;
	}
}

/**
 * デストラクタ
 */
CExternalOpna::~CExternalOpna()
{
}

/**
 * 音源リセット
 */
void CExternalOpna::Reset()
{
	m_cMode = 0;
	memset(m_cAlgorithm, 0, sizeof(m_cAlgorithm));
	memset(m_cTtl, 0x7f, sizeof(m_cTtl));
	if (m_bHasPsg)
	{
		CExternalPsg::Reset();
	}
	else
	{
		m_pChip->Reset();
	}
}

/**
 * レジスタ書き込み
 * @param[in] nAddr アドレス
 * @param[in] cData データ
 */
void CExternalOpna::WriteRegister(UINT nAddr, UINT8 cData)
{
	if (nAddr < 0x10)
	{
		if (m_bHasPsg)
		{
			CExternalPsg::WriteRegister(nAddr, cData);
		}
	}
	else
	{
		if (nAddr == 0x27)
		{
			cData &= 0xc0;
			if (m_cMode == cData)
			{
				return;
			}
			m_cMode = cData;
		}
		else if ((nAddr & 0xf0) == 0x40)
		{
			// ttl
			m_cTtl[((nAddr & 0x100) >> 4) + (nAddr & 15)] = cData;
		}
		else if ((nAddr & 0xfc) == 0xb0)
		{
			// algorithm
			m_cAlgorithm[((nAddr & 0x100) >> 6) + (nAddr & 3)] = cData;
		}
		WriteRegisterInner(nAddr, cData);
	}
}

/**
 * ミュート
 * @param[in] bMute ミュート
 */
void CExternalOpna::Mute(bool bMute) const
{
	if (m_bHasPsg)
	{
		CExternalPsg::Mute(bMute);
	}

	const int nVolume = (bMute) ? -127 : 0;
	for (UINT ch = 0; ch < 3; ch++)
	{
		SetVolume(ch + 0, nVolume);
		if (m_bHasExtend)
		{
			SetVolume(ch + 4, nVolume);
		}
	}
}

/**
 * ヴォリューム設定
 * @param[in] nChannel チャンネル
 * @param[in] nVolume ヴォリューム値
 */
void CExternalOpna::SetVolume(UINT nChannel, int nVolume) const
{
	const UINT nBaseReg = (nChannel & 4) ? 0x140 : 0x40;

	/*! アルゴリズム スロット マスク */
	static const UINT8 s_opmask[] = {0x08, 0x08, 0x08, 0x08, 0x0c, 0x0e, 0x0e, 0x0f};
	UINT8 cMask = s_opmask[m_cAlgorithm[nChannel & 7] & 7];
	const UINT8* pTtl = m_cTtl + ((nChannel & 4) << 2);

	int nOffset = nChannel & 3;
	do
	{
		if (cMask & 1)
		{
			int nTtl = (pTtl[nOffset] & 0x7f) - nVolume;
			if (nTtl < 0)
			{
				nTtl = 0;
			}
			else if (nTtl > 0x7f)
			{
				nTtl = 0x7f;
			}
			WriteRegisterInner(nBaseReg + nOffset, static_cast<UINT8>(nTtl));
		}
		nOffset += 4;
		cMask >>= 1;
	} while (cMask != 0);
}
