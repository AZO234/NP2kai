/**
 * @file	juliet.cpp
 * @brief	ROMEO アクセス クラスの動作の定義を行います
 */

#include "compiler.h"
#include "juliet.h"
#include <stddef.h>
#include "romeo.h"

//! DLL 名
#define	PCIDEBUG_DLL			TEXT("pcidebug.dll")

//! PCI デバイス アドレスを作成
#define PCIBUSDEVFUNC(b, d, f)	(((b) << 8) | ((d) << 3) | (f))

//! ベンダ/デバイスを作成
#define	DEVVEND(v, d)			((ULONG)((v) | ((d) << 16)))

/**
 * コンストラクタ
 */
CJuliet::CJuliet()
	: m_hModule(NULL)
	, m_fnRead32(NULL)
	, m_fnOut8(NULL)
	, m_fnOut32(NULL)
	, m_fnIn8(NULL)
	, m_ulAddress(0)
	, m_ucIrq(0)
	, m_nQueIndex(0)
	, m_nQueCount(0)
	, m_pChip288(NULL)
{
}

/**
 * デストラクタ
 */
CJuliet::~CJuliet()
{
}

/**
 * 初期化
 * @retval true 成功
 * @retval false 失敗
 */
bool CJuliet::Initialize()
{
	if (m_hModule)
	{
		return false;
	}

	m_hModule = ::LoadLibrary(PCIDEBUG_DLL);
	if (m_hModule == NULL)
	{
		return false;
	}

	//! ロード関数リスト
	static const ProcItem s_dllProc[] =
	{
		{"_pciConfigReadLong",	offsetof(CJuliet, m_fnRead32)},
		{"_MemWriteChar",		offsetof(CJuliet, m_fnOut8)},
		{"_MemWriteLong",		offsetof(CJuliet, m_fnOut32)},
		{"_MemReadChar",		offsetof(CJuliet, m_fnIn8)},
	};

	for (size_t i = 0; i < _countof(s_dllProc); i++)
	{
		FARPROC proc = ::GetProcAddress(m_hModule, s_dllProc[i].lpSymbol);
		if (proc == NULL)
		{
			Deinitialize();
			return false;
		}
		*(reinterpret_cast<FARPROC*>(reinterpret_cast<INT_PTR>(this) + s_dllProc[i].nOffset)) = proc;
	}

	const ULONG ulPciAddress = SearchRomeo();
	if (ulPciAddress == static_cast<ULONG>(-1))
	{
		Deinitialize();
		return false;
	}
	m_ulAddress = (*m_fnRead32)(ulPciAddress, ROMEO_BASEADDRESS1);
	m_ucIrq = static_cast<UCHAR>((*m_fnRead32)(ulPciAddress, ROMEO_PCIINTERRUPT));
	if (!m_ulAddress)
	{
		Deinitialize();
		return false;
	}

	Reset();
	Start();

	return true;
}

/**
 * 解放
 */
void CJuliet::Deinitialize()
{
	Stop();
	m_nQueIndex = 0;
	m_nQueCount = 0;

	if (m_pChip288)
	{
		delete m_pChip288;
	}

	if (m_hModule)
	{
		::FreeLibrary(m_hModule);
		m_hModule = NULL;
		m_fnRead32 = NULL;
		m_fnOut8 = NULL;
		m_fnOut32 = NULL;
		m_fnIn8 = NULL;
		m_ulAddress = 0;
		m_ucIrq = 0;
	}
}

/**
 * ROMEO を探す
 * @note pciFindPciDevice使うと、OS起動後一発目に見つけられないことが多いので、自前で検索する（矢野さん方式）
 * @return バス アドレス
 */
ULONG CJuliet::SearchRomeo() const
{
	for (UINT bus = 0; bus < 0x100; bus++)
	{
		for (UINT dev = 0; dev < 0x20; dev++)
		{
			for (UINT func = 0; func < 0x08; func++)
			{
				const ULONG ulPciAddress = PCIBUSDEVFUNC(bus, dev, func);
				const ULONG ulDeviceVendor = (*m_fnRead32)(ulPciAddress, 0x0000);
				if ((ulDeviceVendor == DEVVEND(ROMEO_VENDORID, ROMEO_DEVICEID)) || (ulDeviceVendor == DEVVEND(ROMEO_VENDORID, ROMEO_DEVICEID2)))
				{
					return ulPciAddress;
				}
			}
		}
	}
	return static_cast<ULONG>(-1);
}

/**
 * 音源リセット
 */
void CJuliet::Reset()
{
	m_queGuard.Enter();
	m_nQueIndex = 0;
	m_nQueCount = 0;
	m_queGuard.Leave();

	m_pciGuard.Enter();
	if (m_fnOut32 != NULL)
	{
		(*m_fnOut32)(m_ulAddress + ROMEO_YMF288CTRL, 0x00);
		::Sleep(150);

		(*m_fnOut32)(m_ulAddress + ROMEO_YMF288CTRL, 0x80);
		::Sleep(150);
	}
	m_pciGuard.Leave();
}

/**
 * インターフェイス取得
 * @param[in] nChipType タイプ
 * @param[in] nClock クロック
 * @return インスタンス
 */
IExternalChip* CJuliet::GetInterface(IExternalChip::ChipType nChipType, UINT nClock)
{
	const bool bInitialized = Initialize();

	do
	{
		if (m_hModule == NULL)
		{
			break;
		}

		if ((nChipType == IExternalChip::kYMF288) && (m_pChip288 == NULL))
		{
			m_pChip288 = new Chip288(this);
			return m_pChip288;
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
void CJuliet::Detach(IExternalChip* pChip)
{
	if (m_pChip288 == pChip)
	{
		m_pChip288 = NULL;
	}
}

/**
 * Write
 * @param[in] nAddr The address of registers
 * @param[in] cData The data
 */
void CJuliet::Write288(UINT nAddr, UINT8 cData)
{
	m_queGuard.Enter();
	while (m_nQueCount >= _countof(m_que))
	{
		m_queGuard.Leave();
		Delay(1000);
		m_queGuard.Enter();
	}

	m_que[(m_nQueIndex + m_nQueCount) % _countof(m_que)] = ((nAddr & 0x1ff) << 8) | cData;
	m_nQueCount++;

	m_queGuard.Leave();
}

/**
 * Thread
 * @retval true Cont.
 */
bool CJuliet::Task()
{
	m_queGuard.Enter();
	if (m_nQueCount == 0)
	{
		m_queGuard.Leave();
		Delay(1000);
	}
	else
	{
		while (m_nQueCount)
		{
			const UINT nData = m_que[m_nQueIndex];
			m_nQueIndex = (m_nQueIndex + 1) % _countof(m_que);
			m_nQueCount--;
			m_queGuard.Leave();

			m_pciGuard.Enter();
			while (((*m_fnIn8)(m_ulAddress + ROMEO_YMF288ADDR1) & 0x80) != 0)
			{
				::Sleep(0);
			}
			(*m_fnOut8)(m_ulAddress + ((nData & 0x10000) ? ROMEO_YMF288ADDR2 : ROMEO_YMF288ADDR1), static_cast<UINT8>(nData >> 8));

			while (((*m_fnIn8)(m_ulAddress + ROMEO_YMF288ADDR1) & 0x80) != 0)
			{
				::Sleep(0);
			}
			(*m_fnOut8)(m_ulAddress + ((nData & 0x10000) ? ROMEO_YMF288DATA2 : ROMEO_YMF288DATA1), static_cast<UINT8>(nData));
			m_pciGuard.Leave();

			m_queGuard.Enter();
		}
		m_queGuard.Leave();
	}
	return true;
}



// ---- チップ

/**
 * コンストラクタ
 * @param[in] pJuliet 親インスタンス
 */
CJuliet::Chip288::Chip288(CJuliet* pJuliet)
	: m_pJuliet(pJuliet)
{
}

/**
 * デストラクタ
 */
CJuliet::Chip288::~Chip288()
{
	m_pJuliet->Detach(this);
}

/**
 * Get chip type
 * @return The type of the chip
 */
IExternalChip::ChipType CJuliet::Chip288::GetChipType()
{
	return IExternalChip::kYMF288;
}

/**
 * リセット
 */
void CJuliet::Chip288::Reset()
{
}

/**
 * レジスタ書き込み
 * @param[in] nAddr アドレス
 * @param[in] cData データ
 */
void CJuliet::Chip288::WriteRegister(UINT nAddr, UINT8 cData)
{
	m_pJuliet->Write288(nAddr, cData);
}

/**
 * メッセージ
 * @param[in] nMessage メッセージ
 * @param[in] nParameter パラメータ
 * @return リザルト
 */
INTPTR CJuliet::Chip288::Message(UINT nMessage, INTPTR nParameter)
{
	return 0;
}
