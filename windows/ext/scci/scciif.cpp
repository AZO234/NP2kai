/**
 * @file	scciif.cpp
 * @brief	SCCI アクセス クラスの動作の定義を行います
 */

#include "compiler.h"
#include "scciif.h"
#include "scci.h"
#include "SCCIDefines.h"

/**
 * コンストラクタ
 */
CScciIf::CScciIf()
	: m_hModule(NULL)
	, m_pManager(NULL)
{
}

/**
 * デストラクタ
 */
CScciIf::~CScciIf()
{
	Deinitialize();
}

/**
 * 初期化
 * @retval true 成功
 * @retval false 失敗
 */
bool CScciIf::Initialize()
{
	if (m_hModule)
	{
		return false;
	}

	do
	{
		m_hModule = ::LoadLibrary(TEXT("SCCI.DLL"));
		if (m_hModule == NULL)
		{
			break;
		}

		/* サウンドインターフェースマネージャー取得用関数アドレス取得 */
		SCCIFUNC fnGetSoundInterfaceManager = reinterpret_cast<SCCIFUNC>(::GetProcAddress(m_hModule, "getSoundInterfaceManager"));
		if (fnGetSoundInterfaceManager == NULL)
		{
			break;
		}

		/* サウンドインターフェースマネージャー取得 */
		m_pManager = (*fnGetSoundInterfaceManager)();
		if (m_pManager == NULL)
		{
			break;
		}

		/* サウンドインターフェースマネージャーインスタンス初期化 */
		/* 必ず最初に実行してください */
		if (!m_pManager->initializeInstance())
		{
			break;
		}

		/* リセットを行う */
		Reset();
		return true;
	} while (false /*CONSTCOND*/);

	Deinitialize();
	return false;
}

/**
 * 解放
 */
void CScciIf::Deinitialize()
{
	if (m_pManager)
	{
		/* 一括開放する場合（チップ一括開放の場合） */
		m_pManager->releaseAllSoundChip();

		/* サウンドインターフェースマネージャーインスタンス開放 */
		/* FreeLibraryを行う前に必ず呼び出ししてください */
		m_pManager->releaseInstance();

		m_pManager = NULL;
	}

	if (m_hModule)
	{
		::FreeLibrary(m_hModule);
		m_hModule = NULL;
	}
}

/**
 * 音源リセット
 */
void CScciIf::Reset()
{
	if (m_pManager)
	{
		/* リセットを行う */
		m_pManager->reset();
	}
}

/**
 * インターフェイス取得
 * @param[in] nChipType タイプ
 * @param[in] nClock クロック
 * @return インスタンス
 */
IExternalChip* CScciIf::GetInterface(IExternalChip::ChipType nChipType, UINT nClock)
{
	const bool bInitialized = Initialize();

	do
	{
		if (m_pManager == NULL)
		{
			break;
		}

		SC_CHIP_TYPE iSoundChipType = SC_TYPE_NONE;
		switch (nChipType)
		{
			case IExternalChip::kAY8910:
				iSoundChipType = SC_TYPE_AY8910;
				break;

			case IExternalChip::kYM2203:
				iSoundChipType = SC_TYPE_YM2203;
				break;

			case IExternalChip::kYM2608:
				iSoundChipType = SC_TYPE_YM2608;
				break;

			case IExternalChip::kYM3438:
				iSoundChipType = SC_TYPE_YM2612;
				break;

			case IExternalChip::kYMF288:
				iSoundChipType = SC_TYPE_YMF288;
				break;

			case IExternalChip::kYM3812:
				iSoundChipType = SC_TYPE_YM3812;
				break;

			case IExternalChip::kYMF262:
				iSoundChipType = SC_TYPE_YMF262;
				break;

			case IExternalChip::kY8950:
				iSoundChipType = SC_TYPE_Y8950;
				break;

			case IExternalChip::kYM2151:
				iSoundChipType = SC_TYPE_YM2151;
				break;

			default:
				break;
		}

		SoundChip* pSoundChip = m_pManager->getSoundChip(iSoundChipType, nClock);
		if (pSoundChip != NULL)
		{
			/* サウンドチップ取得できた */
			return new Chip(this, pSoundChip);
		}
	} while (false /*CONSTCOND*/);

	if (bInitialized)
	{
//		Deinitialize();
	}
	return NULL;
}

/**
 * 解放
 * @param[in] pChip チップ
 */
void CScciIf::Detach(CScciIf::Chip* pChip)
{
	/* チップの開放（チップ単位で開放の場合） */
	if (m_pManager)
	{
		m_pManager->releaseSoundChip(*pChip);
	}
}

/* ---- チップ */

/**
 * コンストラクタ
 * @param[in] pScciIf 親インスタンス
 * @param[in] pSoundChip チップ インスタンス
 */
CScciIf::Chip::Chip(CScciIf* pScciIf, SoundChip* pSoundChip)
	: m_pScciIf(pScciIf)
	, m_pSoundChip(pSoundChip)
{
}

/**
 * デストラクタ
 */
CScciIf::Chip::~Chip()
{
	m_pScciIf->Detach(this);
}

/**
 * オペレータ
 */
CScciIf::Chip::operator SoundChip*()
{
	return m_pSoundChip;
}

/**
 * Get chip type
 * @return The type of the chip
 */
IExternalChip::ChipType CScciIf::Chip::GetChipType()
{
	int iSoundChip = m_pSoundChip->getSoundChipType();

	const SCCI_SOUND_CHIP_INFO* pInfo = m_pSoundChip->getSoundChipInfo();
	if (pInfo)
	{
		iSoundChip = pInfo->iSoundChip;
	}

	switch (iSoundChip)
	{
		case SC_TYPE_AY8910:
			return IExternalChip::kAY8910;

		case SC_TYPE_YM2203:
			return IExternalChip::kYM2203;

		case SC_TYPE_YM2608:
			return IExternalChip::kYM2608;

		case SC_TYPE_YM2612:
			return IExternalChip::kYM3438;

		case SC_TYPE_YM3812:
			return IExternalChip::kYM3812;

		case SC_TYPE_YMF262:
			return IExternalChip::kYMF262;

		case SC_TYPE_YMF288:
			return IExternalChip::kYMF288;

		case SC_TYPE_Y8950:
			return IExternalChip::kY8950;

		case SC_TYPE_YM2151:
			return IExternalChip::kYM2151;

		default:
			break;
	}
	return IExternalChip::kNone;
}

/**
 * リセット
 */
void CScciIf::Chip::Reset()
{
}

/**
 * レジスタ書き込み
 * @param[in] nAddr アドレス
 * @param[in] cData データ
 */
void CScciIf::Chip::WriteRegister(UINT nAddr, UINT8 cData)
{
	m_pSoundChip->setRegister(nAddr, cData);
}

/**
 * メッセージ
 * @param[in] nMessage メッセージ
 * @param[in] nParameter パラメータ
 * @return リザルト
 */
INTPTR CScciIf::Chip::Message(UINT nMessage, INTPTR nParameter)
{
	return 0;
}
