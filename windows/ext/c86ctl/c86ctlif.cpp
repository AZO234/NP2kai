/**
 * @file	c86ctlif.cpp
 * @brief	G.I.M.I.C アクセス クラスの動作の定義を行います
 */

#include "compiler.h"
#include "c86ctlif.h"
#include "c86ctl.h"

using namespace c86ctl;

/*! インタフェイス */
typedef HRESULT (WINAPI * FnCreateInstance)(REFIID riid, LPVOID* ppi);

/**
 * コンストラクタ
 */
C86CtlIf::C86CtlIf()
	: m_hModule(NULL)
	, m_pChipBase(NULL)
{
}

/**
 * デストラクタ
 */
C86CtlIf::~C86CtlIf()
{
	Deinitialize();
}

/**
 * 初期化
 * @retval true 成功
 * @retval false 失敗
 */
bool C86CtlIf::Initialize()
{
	if (m_hModule)
	{
		return false;
	}

	do
	{
		/* DLL 読み込み */
		m_hModule = ::LoadLibrary(TEXT("c86ctl.dll"));
		if (m_hModule == NULL)
		{
			break;
		}
		FnCreateInstance CreateInstance = reinterpret_cast<FnCreateInstance>(::GetProcAddress(m_hModule, "CreateInstance"));
		if (CreateInstance == NULL)
		{
			break;
		}

		/* インスタンス作成 */
		(*CreateInstance)(IID_IRealChipBase, reinterpret_cast<LPVOID*>(&m_pChipBase));
		if (m_pChipBase == NULL)
		{
			break;
		}

		/* 初期化 */
		if (m_pChipBase->initialize() != C86CTL_ERR_NONE)
		{
			break;
		}
		return true;

	} while (0 /*CONSTCOND*/);

	Deinitialize();
	return false;
}

/**
 * 解放
 */
void C86CtlIf::Deinitialize()
{
	if (m_pChipBase)
	{
		while (!m_chips.empty())
		{
			std::map<int, Chip*>::iterator it = m_chips.begin();
			delete it->second;
		}

		m_pChipBase->deinitialize();
		m_pChipBase = NULL;
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
void C86CtlIf::Reset()
{
}

/**
 * インターフェイス取得
 * @param[in] nChipType タイプ
 * @param[in] nClock クロック
 * @return インスタンス
 */
IExternalChip* C86CtlIf::GetInterface(IExternalChip::ChipType nChipType, UINT nClock)
{
	const bool bInitialized = Initialize();

	do
	{
		if (m_pChipBase == NULL)
		{
			break;
		}

		/* 音源を探す */
		const int nDeviceCount = m_pChipBase->getNumberOfChip();
		for (int i = 0; i < nDeviceCount; i++)
		{
			/* 使用中? */
			if (m_chips.find(i) != m_chips.end())
			{
				continue;
			}

			/* チップを探す */
			IRealChip* pRealChip = NULL;
			m_pChipBase->getChipInterface(i, IID_IRealChip, reinterpret_cast<LPVOID*>(&pRealChip));
			if (pRealChip == NULL)
			{
				continue;
			}

			/* G.I.M.I.C 判定 */
			IGimic* pGimic = NULL;
			m_pChipBase->getChipInterface(i, IID_IGimic, reinterpret_cast<LPVOID*>(&pGimic));
			if (pGimic)
			{
				Devinfo info;
				if (pGimic->getModuleInfo(&info) == C86CTL_ERR_NONE)
				{
					IExternalChip::ChipType nRealChipType = IExternalChip::kNone;
					if (!memcmp(info.Devname, "GMC-OPN3L", 9))
					{
						nRealChipType = IExternalChip::kYMF288;
					}
					else if (!memcmp(info.Devname, "GMC-OPNA", 8))
					{
						nRealChipType = IExternalChip::kYM2608;
					}
					else if (!memcmp(info.Devname, "GMC-OPL3", 8))
					{
						nRealChipType = IExternalChip::kYMF262;
					}
					else if (!memcmp(info.Devname, "GMC-OPM", 7))
					{
						nRealChipType = IExternalChip::kYM2151;
					}

					if (nChipType == nRealChipType)
					{
						/* サウンドチップ取得できた */
						Chip* pChip = new Chip(this, pRealChip, pGimic, nRealChipType, nClock);
						m_chips[i] = pChip;
						return pChip;
					}
				}
			}

			/* その他の判定 */
			IRealChip3* pChip3 = NULL;
			m_pChipBase->getChipInterface(i, IID_IRealChip3, reinterpret_cast<LPVOID*>(&pChip3));
			if (pChip3 != NULL)
			{
				c86ctl::ChipType nType = CHIP_UNKNOWN;
				pChip3->getChipType(&nType);

				IExternalChip::ChipType nRealChipType = IExternalChip::kNone;
				if (nType == CHIP_YM2203)
				{
					nRealChipType = IExternalChip::kYM2203;
				}
				else if (nType == CHIP_OPNA)
				{
					nRealChipType = IExternalChip::kYM2608;
				}
				else if ((nType == CHIP_YM2608NOADPCM) || (nType == CHIP_OPN3L))
				{
					nRealChipType = IExternalChip::kYMF288;
				}
				else if (nType == CHIP_Y8950ADPCM)
				{
					nRealChipType = IExternalChip::kY8950;
				}
				if (nChipType == nRealChipType)
				{
					/* サウンドチップ取得できた */
					Chip* pChip = new Chip(this, pChip3, NULL, nRealChipType, nClock);
					m_chips[i] = pChip;
					return pChip;
				}
			}
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
void C86CtlIf::Detach(C86CtlIf::Chip* pChip)
{
	std::map<int, Chip*>::iterator it = m_chips.begin();
	while (it != m_chips.end())
	{
		if (it->second == pChip)
		{
			it = m_chips.erase(it);
		}
		else
		{
			++it;
		}
	}
}

/* ---- チップ */

/**
 * コンストラクタ
 * @param[in] pC86CtlIf C86CtlIf インスタンス
 * @param[in] pRealChip チップ インスタンス
 * @param[in] pGimic G.I.M.I.C インスタンス
 * @param[in] nChipType チップ タイプ
 * @param[in] nClock クロック
 */
C86CtlIf::Chip::Chip(C86CtlIf* pC86CtlIf, c86ctl::IRealChip* pRealChip, c86ctl::IGimic* pGimic, ChipType nChipType, UINT nClock)
	: m_pC86CtlIf(pC86CtlIf)
	, m_pRealChip(pRealChip)
	, m_pGimic(pGimic)
	, m_nChipType(nChipType)
	, m_nClock(nClock)
{
}

/**
 * デストラクタ
 */
C86CtlIf::Chip::~Chip()
{
	m_pC86CtlIf->Detach(this);
}

/**
 * Get chip type
 * @return The type of the chip
 */
IExternalChip::ChipType C86CtlIf::Chip::GetChipType()
{
	return m_nChipType;
}

/**
 * リセット
 */
void C86CtlIf::Chip::Reset()
{
	m_pRealChip->reset();
	if (m_pGimic)
	{
		m_pGimic->setPLLClock(m_nClock);
		m_pGimic->setSSGVolume(31);
	}
}

/**
 * レジスタ書き込み
 * @param[in] nAddr アドレス
 * @param[in] cData データ
 */
void C86CtlIf::Chip::WriteRegister(UINT nAddr, UINT8 cData)
{
	m_pRealChip->out(nAddr, cData);
}

/**
 * メッセージ
 * @param[in] nMessage メッセージ
 * @param[in] nParameter パラメータ
 * @return リザルト
 */
INTPTR C86CtlIf::Chip::Message(UINT nMessage, INTPTR nParameter)
{
	return 0;
}
