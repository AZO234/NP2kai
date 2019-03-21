/**
 * @file	c86ctlgimic.cpp
 * @brief	Implementation of G.I.M.I.C
 */

#include "compiler.h"
#include "c86ctlgimic.h"
#include <algorithm>

namespace c86ctl
{

/**
 * Constructor
 * @param[in] nIndex The index
 */
CGimic::CGimic(UINT nIndex)
	: m_nRef(1)
	, m_nIndex(nIndex)
	, m_nChipType(CHIP_UNKNOWN)
	, m_nQueIndex(0)
	, m_nQueCount(0)
	, m_gimic2(this)
	, m_chip3(this)
{
}

/**
 * Destructor
 */
CGimic::~CGimic()
{
}

/**
 * Increments the reference count
 * @return The new reference count
 */
size_t CGimic::AddRef()
{
	m_nRef++;
	return m_nRef;
}

/**
 * Decrements the reference count
 * @return The new reference count
 */
size_t CGimic::Release()
{
	m_nRef--;
	return m_nRef;
}

/**
 * Initialize
 * @return C86CTL_ERR
 */
C86CtlErr CGimic::initialize()
{
	if (!m_usb.Open(0x16c0, 0x05e5, m_nIndex))
	{
		return C86CTL_ERR_NODEVICE;
	}

	// Query G.I.M.I.C module info.
	Devinfo info;
	::memset(&info, 0, sizeof(info));
	if (m_gimic2.getModuleInfo(&info) != 0)
	{
		m_usb.Close();
		return C86CTL_ERR_NODEVICE;
	}

	printf("Found G.I.M.I.C!\n");
	printf("Devname: %s\n", info.Devname);
	printf("    Rev: %d\n", info.Rev);
	printf(" Serial: %s\n", info.Serial);

	if (!::memcmp(info.Devname, "GMC-OPN3L", 9))
	{
		m_nChipType = CHIP_OPN3L;
	}
	else if (!::memcmp(info.Devname, "GMC-OPM", 7))
	{
		m_nChipType = CHIP_OPM;
	}
	else if (!::memcmp(info.Devname, "GMC-OPNA", 8))
	{
		m_nChipType = CHIP_OPNA;
	}
	else if (!::memcmp(info.Devname, "GMC-OPL3", 8))
	{
		m_nChipType = CHIP_OPL3;
	}

	m_nQueIndex = 0;
	m_nQueCount = 0;
	Start();
	return C86CTL_ERR_NONE;
}

/**
 * Deinitialize
 * @return C86CTL_ERR
 */
C86CtlErr CGimic::deinitialize()
{
	Stop();
	m_usb.Close();

	m_nChipType = CHIP_UNKNOWN;

	return C86CTL_ERR_NONE;
}

/**
 * Gets the count of chips
 * @return The chips
 */
size_t CGimic::getNumberOfChip()
{
	return (m_nChipType != CHIP_UNKNOWN) ? 1 : 0;
}

/**
 * Gets interfaces
 * @param[in] id ID
 * @param[in] riid The interface ID
 * @param[out] ppi The pointer of the interface
 * @return C86CTL_ERR
 */
C86CtlErr CGimic::getChipInterface(size_t id, IID riid, void** ppi)
{
	if (ppi == NULL)
	{
		return C86CTL_ERR_INVALID_PARAM;
	}

	if (id != 0)
	{
		return C86CTL_ERR_NODEVICE;
	}

	switch (riid)
	{
		case IID_IRealChip:
		case IID_IRealChip2:
		case IID_IRealChip3:
			m_chip3.AddRef();
			*ppi = &m_chip3;
			return C86CTL_ERR_NONE;

		case IID_IGimic:
		case IID_IGimic2:
			m_gimic2.AddRef();
			*ppi = &m_gimic2;
			return C86CTL_ERR_NONE;

		default:
			break;
	}

	return C86CTL_ERR_UNSUPPORTED;
}

/**
 * Sends and receives data from USB
 * @param[in] lpOutput A pointer to the buffer that sends the data
 * @param[in] cbOutput The number of bytes to be written
 * @param[out] lpInput A pointer to the buffer that receives the data
 * @param[in] cbInput The maximum number of bytes to be read
 * @return C86CTL_ERR
 */
C86CtlErr CGimic::Transaction(const void* lpOutput, int cbOutput, void* lpInput, int cbInput)
{
	if (!m_usb.IsOpened())
	{
		return C86CTL_ERR_NODEVICE;
	}

	char sBuffer[64];
	if ((lpOutput == NULL) || (cbOutput <= 0) || (cbOutput >= sizeof(sBuffer)))
	{
		return C86CTL_ERR_INVALID_PARAM;
	}

	::memcpy(sBuffer, lpOutput, cbOutput);
	::memset(sBuffer + cbOutput, 0xff, sizeof(sBuffer) - cbOutput);

	m_usbGuard.Enter();
	int nResult = m_usb.WriteBulk(sBuffer, sizeof(sBuffer));
	if ((nResult == sizeof(sBuffer)) && (cbInput > 0))
	{
		nResult = m_usb.ReadBulk(sBuffer, sizeof(sBuffer));
	}
	m_usbGuard.Leave();

	if (nResult != sizeof(sBuffer))
	{
		return C86CTL_ERR_UNKNOWN;
	}

	if ((lpInput != NULL) && (cbInput > 0))
	{
		cbInput = (std::min)(cbInput, static_cast<int>(sizeof(sBuffer)));
		::memcpy(lpInput, sBuffer, cbInput);
	}
	return C86CTL_ERR_NONE;
}

/**
 * Gets the informations
 * @param[in] cParam The parameter
 * @param[out] pInfo A pointer to the informations
 * @return C86CTL_ERR
 */
C86CtlErr CGimic::GetInfo(UINT8 cParam, Devinfo* pInfo)
{
	UINT8 sData[3];
	sData[0] = 0xfd;
	sData[1] = 0x91;
	sData[2] = cParam;
	const C86CtlErr ret = Transaction(sData, sizeof(sData), pInfo, sizeof(*pInfo));
	if ((ret == C86CTL_ERR_NONE) && (pInfo != NULL))
	{
		TailZeroFill(pInfo->Devname, sizeof(pInfo->Devname));
		TailZeroFill(pInfo->Serial, sizeof(pInfo->Serial));
	}
	return ret;
}

/**
 * Sets ZeroFill
 * @param[in] lpBuffer A pointer to the buffer
 * @param[in] cbBuffer The number of bytes
 */
void CGimic::TailZeroFill(char* lpBuffer, size_t cbBuffer)
{
	while (cbBuffer > 0)
	{
		cbBuffer--;
		if (lpBuffer[cbBuffer] == -1)
		{
			lpBuffer[cbBuffer] = '\0';
		}
		else if (lpBuffer[cbBuffer] != '\0')
		{
			break;
		}
	}
}

/**
 * Reset
 * @return C86CTL_ERR
 */
C86CtlErr CGimic::Reset()
{
	m_queGuard.Enter();
	m_nQueIndex = 0;
	m_nQueCount = 0;
	m_queGuard.Leave();

	static const UINT8 sData[2] = {0xfd, 0x82};
	return Transaction(sData, sizeof(sData));
}

/**
 * Remaps address
 * @param[in] nAddr The address
 * @return The remapped address
 */
UINT CGimic::GetChipAddr(UINT nAddr) const
{
	switch (m_nChipType)
	{
		case CHIP_OPNA:
		case CHIP_OPN3L:
			if ((nAddr >= 0x100) && (nAddr <= 0x110))
			{
				nAddr -= 0x40;
			}
			break;

		case CHIP_OPM:
			if ((nAddr >= 0xfc) && (nAddr <= 0xff))
			{
				nAddr -= 0xe0;
			}
			break;

		default:
			break;
	}
	return nAddr;
}

/**
 * Output
 * @param[in] nAddr The address of registers
 * @param[in] cData The data
 */
void CGimic::Out(UINT nAddr, UINT8 cData)
{
	m_queGuard.Enter();
	while (m_nQueCount >= NELEMENTS(m_que))
	{
		m_queGuard.Leave();
		Delay(1000);
		m_queGuard.Enter();
	}

	FMDATA& data = m_que[(m_nQueIndex + m_nQueCount) % NELEMENTS(m_que)];
	data.wAddr = static_cast<UINT16>(nAddr);
	data.cData = cData;
	m_nQueCount++;

	m_queGuard.Leave();
}

/**
 * Thread
 * @retval true Cont.
 */
bool CGimic::Task()
{
	/* builds data */
	UINT8 sData[64];
	int nIndex = 0;

	m_queGuard.Enter();
	while (m_nQueCount)
	{
		const FMDATA& data = m_que[m_nQueIndex];
		const UINT nAddr = data.wAddr;
		const UINT8 cData = data.cData;

		if (nAddr < 0xfc)
		{
			if ((nIndex + 2 + 1) >= NELEMENTS(sData))
			{
				break;
			}
			sData[nIndex++] = static_cast<UINT8>(nAddr & 0xff);
			sData[nIndex++] = cData;
		}
		else if ((nAddr >= 0x100) && (nAddr <= 0x1fb))
		{
			if ((nIndex + 3 + 1) >= NELEMENTS(sData))
			{
				break;
			}
			sData[nIndex++] = 0xfe;
			sData[nIndex++] = static_cast<UINT8>(nAddr & 0xff);
			sData[nIndex++] = cData;
		}

		m_nQueIndex = (m_nQueIndex + 1) % NELEMENTS(m_que);
		m_nQueCount--;
	}
	m_queGuard.Leave();

	/* writes */
	if (nIndex > 0)
	{
		Transaction(sData, nIndex);
	}
	else
	{
		Delay(1000);
	}
	return true;
}

/* IGimic2 */

/**
 * Constructor
 * @param[in] pDevice The instance of the device
 */
CGimic::Gimic2::Gimic2(CGimic* pDevice)
	: m_pDevice(pDevice)
{
}

/**
 * Get the instance of the device
 * @return The instance
 */
inline CGimic* CGimic::Gimic2::GetDevice()
{
	return m_pDevice;
}

/**
 * Increments the reference count
 * @return The new reference count
 */
size_t CGimic::Gimic2::AddRef()
{
	return GetDevice()->AddRef();
}

/**
 * Decrements the reference count
 * @return The new reference count
 */
size_t CGimic::Gimic2::Release()
{
	return GetDevice()->Release();
}

/**
 * Gets the informations of firm
 * @param[out] pnMajor A pointer to the major
 * @param[out] pnMinor A pointer to the minor
 * @param[out] pnRev A pointer to the revision
 * @param[out] pnBuild A pointer to the number of build
 * @return C86CTL_ERR
 */
C86CtlErr CGimic::Gimic2::getFWVer(UINT* pnMajor, UINT* pnMinor, UINT* pnRev, UINT* pnBuild)
{
	static const UINT8 sData[2] = {0xfd, 0x92};
	UINT8 sRecv[16];
	const C86CtlErr ret = GetDevice()->Transaction(sData, sizeof(sData), sRecv, sizeof(sRecv));
	if (ret == C86CTL_ERR_NONE)
	{
		if (pnMajor != NULL)
		{
			*pnMajor = (sRecv[0] << 0) | (sRecv[1] << 8) | (sRecv[2] << 16) | (sRecv[3] << 24);
		}
		if (pnMinor != NULL)
		{
			*pnMinor = (sRecv[4] << 0) | (sRecv[5] << 8) | (sRecv[6] << 16) | (sRecv[7] << 24);
		}
		if (pnRev != NULL)
		{
			*pnRev = (sRecv[8] << 0) | (sRecv[9] << 8) | (sRecv[10] << 16) | (sRecv[11] << 24);
		}
		if (pnBuild != NULL)
		{
			*pnBuild = (sRecv[12] << 0) | (sRecv[13] << 8) | (sRecv[14] << 16) | (sRecv[15] << 24);
		}
	}
	return ret;
}

/**
 * Gets the informations of the mother
 * @param[out] pInfo A pointer to the informations
 * @return C86CTL_ERR
 */
C86CtlErr CGimic::Gimic2::getMBInfo(Devinfo* pInfo)
{
	return GetDevice()->GetInfo(0xff, pInfo);
}

/**
 * Gets the informations of modules
 * @param[out] pInfo A pointer to the informations
 * @return C86CTL_ERR
 */
C86CtlErr CGimic::Gimic2::getModuleInfo(Devinfo* pInfo)
{
	return GetDevice()->GetInfo(0, pInfo);
}

/**
 * Sets the volumes of SSG
 * @param[in] cVolume The volume
 * @return C86CTL_ERR
 */
C86CtlErr CGimic::Gimic2::setSSGVolume(UINT8 cVolume)
{
	if (GetDevice()->m_nChipType != CHIP_OPNA)
	{
		return C86CTL_ERR_UNSUPPORTED;
	}

	UINT8 sData[3];
	sData[0] = 0xfd;
	sData[1] = 0x84;
	sData[2] = cVolume;
	return GetDevice()->Transaction(sData, sizeof(sData));
}

/**
 * Gets the volume of SSG
 * @param[out] pcVolume A pointer of the volume
 * @return C86CTL_ERR
 */
C86CtlErr CGimic::Gimic2::getSSGVolume(UINT8* pcVolume)
{
	if (GetDevice()->m_nChipType != CHIP_OPNA)
	{
		return C86CTL_ERR_UNSUPPORTED;
	}

	static const UINT8 sData[2] = {0xfd, 0x86};
	return GetDevice()->Transaction(sData, sizeof(sData), pcVolume, sizeof(*pcVolume));
}

/**
 * Sets the clock
 * @param[in] nClock The clock
 * @return C86CTL_ERR
 */
C86CtlErr CGimic::Gimic2::setPLLClock(UINT nClock)
{
	const ChipType nChipType = GetDevice()->m_nChipType;
	if ((nChipType != CHIP_OPNA) && (nChipType != CHIP_OPM) && (nChipType != CHIP_OPL3))
	{
		return C86CTL_ERR_UNSUPPORTED;
	}

	UINT8 sData[6];
	sData[0] = 0xfd;
	sData[1] = 0x83;
	sData[2] = static_cast<UINT8>((nClock >> 0) & 0xff);
	sData[3] = static_cast<UINT8>((nClock >> 8) & 0xff);
	sData[4] = static_cast<UINT8>((nClock >> 16) & 0xff);
	sData[5] = static_cast<UINT8>((nClock >> 24) & 0xff);
	return GetDevice()->Transaction(sData, sizeof(sData));
}

/**
 * Gets the clock
 * @param[out] pnClock A pointer to the clock
 * @return C86CTL_ERR
 */
C86CtlErr CGimic::Gimic2::getPLLClock(UINT* pnClock)
{
	const ChipType nChipType = GetDevice()->m_nChipType;
	if ((nChipType != CHIP_OPNA) && (nChipType != CHIP_OPM) && (nChipType != CHIP_OPL3))
	{
		return C86CTL_ERR_UNSUPPORTED;
	}

	static const UINT8 sData[2] = {0xfd, 0x85};
	UINT8 sRecv[4];
	const C86CtlErr ret = GetDevice()->Transaction(sData, sizeof(sData), sRecv, sizeof(sRecv));
	if ((ret == C86CTL_ERR_NONE) && (pnClock != NULL))
	{
		*pnClock = (sRecv[0] << 0) | (sRecv[1] << 8) | (sRecv[2] << 16) | (sRecv[3] << 24);
	}
	return ret;
}

/**
 * Gets the type of the modules
 * @param[out] pnType The type
 * @return C86CTL_ERR
 */
C86CtlErr CGimic::Gimic2::getModuleType(ChipType* pnType)
{
	if (pnType != NULL)
	{
		*pnType = GetDevice()->m_nChipType;
	}
	return C86CTL_ERR_NONE;
}

/* IRealChip3 */

/**
 * Constructor
 * @param[in] pDevice The instance of the device
 */
CGimic::Chip3::Chip3(CGimic* pDevice)
	: m_pDevice(pDevice)
{
}

/**
 * Get the instance of the device
 * @return The instance
 */
inline CGimic* CGimic::Chip3::GetDevice()
{
	return m_pDevice;
}


/**
 * Increments the reference count
 * @return The new reference count
 */
size_t CGimic::Chip3::AddRef()
{
	return GetDevice()->AddRef();
}

/**
 * Decrements the reference count
 * @return The new reference count
 */
size_t CGimic::Chip3::Release()
{
	return GetDevice()->Release();
}

/**
 * Reset
 * @return C86CTL_ERR
 */
C86CtlErr CGimic::Chip3::reset()
{
	memset(m_sReg, 0, sizeof(m_sReg));
	return GetDevice()->Reset();
}

/**
 * Output
 * @param[in] nAddr The address
 * @param[in] cData The data
 */
void CGimic::Chip3::out(UINT nAddr, UINT8 cData)
{
	if (nAddr >= sizeof(m_sReg))
	{
		return;
	}
	m_sReg[nAddr] = cData;

	GetDevice()->Out(nAddr, cData);
}

/**
 * Input
 * @param[in] nAddr The address of registers
 * @return The data
 */
UINT8 CGimic::Chip3::in(UINT nAddr)
{
	if (nAddr < sizeof(m_sReg))
	{
		return m_sReg[nAddr];
	}
	return 0xff;
}

/**
 * Gets the current status
 * @param[in] nAddr The address
 * @param[out] pcStatus The status
 * @return C86CTL_ERR
 */
C86CtlErr CGimic::Chip3::getChipStatus(UINT nAddr, UINT8* pcStatus)
{
	UINT8 sData[3];
	sData[0] = 0xfd;
	sData[1] = 0x93;
	sData[2] = static_cast<UINT8>(nAddr & 1);
	UINT8 sRecv[4];
	const C86CtlErr ret = GetDevice()->Transaction(sData, sizeof(sData), sRecv, sizeof(sRecv));
	if ((ret == C86CTL_ERR_NONE) && (pcStatus != NULL))
	{
		*pcStatus = sRecv[0];
	}
	return ret;
}

/**
 * Output
 * @param[in] nAddr The address
 * @param[in] cData The data
 */
void CGimic::Chip3::directOut(UINT nAddr, UINT8 cData)
{
	if (nAddr >= sizeof(m_sReg))
	{
		return;
	}
	m_sReg[nAddr] = cData;

	nAddr = GetDevice()->GetChipAddr(nAddr);
	if (nAddr < 0xfc)
	{
		UINT8 sData[2];
		sData[0] = static_cast<UINT8>(nAddr & 0xff);
		sData[1] = cData;
		GetDevice()->Transaction(sData, sizeof(sData));
	}
	else if ((nAddr >= 0x100) && (nAddr <= 0x1fb))
	{
		UINT8 sData[3];
		sData[0] = 0xfe;
		sData[1] = static_cast<UINT8>(nAddr & 0xff);
		sData[2] = cData;
		GetDevice()->Transaction(sData, sizeof(sData));
	}
}

/**
 * Gets the type of the chip
 * @param[out] pnType A pointer of type
 * @return C86CTL_ERR
 */
C86CtlErr CGimic::Chip3::getChipType(ChipType* pnType)
{
	if (pnType != NULL)
	{
		*pnType = GetDevice()->m_nChipType;
	}
	return C86CTL_ERR_NONE;
}

}	// namespace c86ctl
