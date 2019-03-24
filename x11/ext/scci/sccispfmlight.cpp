/**
 * @file	sccispfmlight.cpp
 * @brief	Implementation of accessing SPFM Light
 */

#include "compiler.h"
#include "sccispfmlight.h"
#include "sccisoundchip.h"
#include "misc/threadbase.h"

namespace scci
{

/**
 * Constructor
 * @param[in] pManager The instance of the manager
 * @param[in] deviceName The name of the device
 * @param[in] nDelay delay time
 */
CSpfmLight::CSpfmLight(CSoundInterfaceManager* pManager, const std::oemstring& deviceName, UINT nDelay)
	: CSoundInterface(pManager, deviceName)
	, m_nDelay(nDelay)
	, m_bReseted(false)
	, m_nQueIndex(0)
	, m_nQueCount(0)
{
}

/**
 * Destructor
 */
CSpfmLight::~CSpfmLight()
{
}

/**
 * Initialize
 * @retval true If succeeded
 * @retval false If failed
 */
bool CSpfmLight::Initialize()
{
	if (!m_serial.Open(m_info.cInterfaceName, 1500000, OEMTEXT("8N1")))
	{
		return false;
	}

	bool bResult = false;

	m_ttyGuard.Enter();
	const UINT nUntil = GETTICK() + 3000;
	const unsigned char query[1] = {0xff};
	if (Write(query, sizeof(query), nUntil) == sizeof(query))
	{
		unsigned char buffer[2];
		bResult = (Read(buffer, sizeof(buffer), nUntil) == sizeof(buffer)) && (buffer[0] == 'L') && (buffer[1] == 'T');
	}
	m_ttyGuard.Leave();

	if (bResult)
	{
		reset();
	}

	m_serial.Close();

	m_bReseted = false;
	return bResult;
}

/**
 * Deinitialize
 */
void CSpfmLight::Deinitialize()
{
	m_serial.Close();
}

/**
 * Increments the reference count
 * @return The new reference count
 */
size_t CSpfmLight::AddRef()
{
	const size_t nRef = CSoundInterface::AddRef();
	if (nRef == 1)
	{
		if (m_serial.Open(m_info.cInterfaceName, 1500000, OEMTEXT("8N1")))
		{
			Start();
		}
	}
	return nRef;
}

/**
 * Decrements the reference count
 * @return The new reference count
 */
size_t CSpfmLight::Release()
{
	const size_t nRef = CSoundInterface::Release();
	if (nRef == 0)
	{
		Stop();
		m_serial.Close();
	}
	return nRef;
}

/**
 * Add
 * @param[in] info The information
 */
void CSpfmLight::Add(const SCCI_SOUND_CHIP_INFO& info)
{
	std::map<UINT, CSoundChip*>::iterator it = m_chips.find(info.dBusID);
	if (it == m_chips.end())
	{
		m_chips[info.dBusID] = new Chip(this, info);
		m_info.iSoundChipCount++;
	}
}

/**
 * Is supported low level API
 * @retval true yes
 */
bool CSpfmLight::isSupportLowLevelApi()
{
	return true;
}

/**
 * Sends data to the interface
 * @param[in] pData The buffer of data
 * @param[in] dSendDataLen The length of data
 * @retval true If succeeded
 * @retval false If failed
 */
bool CSpfmLight::setData(const unsigned char* pData, size_t dSendDataLen)
{
	m_ttyGuard.Enter();
	m_bReseted = false;
	const size_t r = m_serial.Write(pData, dSendDataLen);
	m_ttyGuard.Leave();
	return (r == dSendDataLen);
}

/**
 * Gets data from the interface
 * @param[out] pData The buffer of data
 * @param[in] dGetDataLen The length of data
 * @return The size of read
 */
size_t CSpfmLight::getData(unsigned char* pData, size_t dGetDataLen)
{
	m_ttyGuard.Enter();
	const ssize_t r = m_serial.Read(pData, dGetDataLen);
	m_ttyGuard.Leave();
	return r;
}

/**
 * Sets delay time
 * @param[in] dDelay delay time
 * @retval true If succeeded
 * @retval false If failed
 */
bool CSpfmLight::setDelay(UINT dDelay)
{
	if (dDelay > 10000)
	{
		return false;
	}
	m_nDelay = dDelay;
	return true;
}

/**
 * Gets delay time
 * @return delay time
 */
UINT CSpfmLight::getDelay()
{
	return m_nDelay;
}

/**
 * Resets the interface
 * @retval true If succeeded
 * @retval false If failed
 */
bool CSpfmLight::reset()
{
	if (m_bReseted)
	{
		return true;
	}

	if (!m_serial.IsOpened())
	{
		return false;
	}

	m_queGuard.Enter();
	m_nQueIndex = 0;
	m_nQueCount = 0;
	m_queGuard.Leave();

	m_ttyGuard.Enter();
	const UINT nUntil = GETTICK() + 3000;
	const unsigned char reset[1] = {0xfe};
	if (Write(reset, sizeof(reset), nUntil) == sizeof(reset))
	{
		unsigned char buffer[2];
		m_bReseted = (Read(buffer, sizeof(buffer), nUntil) == sizeof(buffer)) && (buffer[0] == 'O') && (buffer[1] == 'K');
	}
	m_ttyGuard.Leave();

	return m_bReseted;
}

/**
 * Read
 * @param[out] lpBuffer The pointer of the buffer
 * @param[in] cbBuffer The size of the buffer
 * @param[in] nUntil until
 * @return The read size
 */
ssize_t CSpfmLight::Read(unsigned char* lpBuffer, ssize_t cbBuffer, UINT nUntil)
{
	ssize_t nRead = m_serial.Read(lpBuffer, cbBuffer);
	if (nRead == -1)
	{
		return -1;
	}
	while ((nRead < cbBuffer) && ((static_cast<SINT>(nUntil) - static_cast<SINT>(GETTICK())) > 0))
	{
		Delay(1000);
		const ssize_t r = m_serial.Read(lpBuffer + nRead, cbBuffer - nRead);
		if (r == -1)
		{
			return -1;
		}
		nRead += r;
	}
	return nRead;
}

/**
 * Write
 * @param[out] lpBuffer The pointer of the buffer
 * @param[in] cbBuffer The size of the buffer
 * @param[in] nUntil until
 * @return The written size
 */
ssize_t CSpfmLight::Write(const unsigned char* lpBuffer, ssize_t cbBuffer, UINT nUntil)
{
	ssize_t nWritten = m_serial.Write(lpBuffer, cbBuffer);
	if (nWritten == -1)
	{
		return -1;
	}
	while ((nWritten < cbBuffer) && ((static_cast<SINT>(nUntil) - static_cast<SINT>(GETTICK())) > 0))
	{
		Delay(1000);
		const ssize_t w = m_serial.Write(lpBuffer + nWritten, cbBuffer - nWritten);
		if (w == -1)
		{
			return -1;
		}
		nWritten += w;
	}
	return nWritten;
}

/**
 * Adds the event
 * @param[in] nData data
 * @retval true If succeeded
 * @retval false If failed
 */
bool CSpfmLight::AddEvent(UINT nData)
{
	const UINT nNow = GETTICK();

	m_queGuard.Enter();

	m_bReseted = false;

	while (m_nQueCount >= NELEMENTS(m_que))
	{
		m_queGuard.Leave();
		Delay(1000);
		m_queGuard.Enter();
	}

	QueData& que = m_que[(m_nQueIndex + m_nQueCount) % NELEMENTS(m_que)];
	m_nQueCount++;

	que.nTimestamp = nNow + m_nDelay;
	que.nData = nData;

	m_queGuard.Leave();

	return true;
}

/**
 * Thread
 * @retval true Cont.
 */
bool CSpfmLight::Task()
{
	/* builds data */
	UINT8 sData[64];
	UINT nIndex = 0;

	const UINT nNow = GETTICK();

	m_queGuard.Enter();
	while (m_nQueCount)
	{
		const QueData& que = m_que[m_nQueIndex];
		if ((static_cast<SINT>(que.nTimestamp) - static_cast<SINT>(nNow)) > 0)
		{
			break;
		}
		if ((nIndex + 4) > NELEMENTS(sData))
		{
			break;
		}

		sData[nIndex++] = static_cast<UINT8>(que.nData >> 24);
		sData[nIndex++] = static_cast<UINT8>(que.nData >> 16);
		sData[nIndex++] = static_cast<UINT8>(que.nData >> 8);
		sData[nIndex++] = static_cast<UINT8>(que.nData >> 0);

		m_nQueIndex = (m_nQueIndex + 1) % NELEMENTS(m_que);
		m_nQueCount--;
	}
	m_queGuard.Leave();

	/* writes */
	if (nIndex > 0)
	{
		m_ttyGuard.Enter();
		Write(sData, nIndex, nNow + 3000);
		m_ttyGuard.Leave();
	}
	else
	{
		Delay(1000);
	}
	return true;
}

/**
 * Constructor
 * @param[in] pInterface The instance of the sound interface
 * @param[in] info The information
 */
CSpfmLight::Chip::Chip(CSoundInterface* pInterface, const SCCI_SOUND_CHIP_INFO& info)
	: CSoundChip(pInterface, info)
{
}

/**
 * Sets Register data
 * Writes the register
 * @param[in] dAddr The address of register
 * @param[in] dData The data
 * @retval true If succeeded
 * @retval false If failed
 */
bool CSpfmLight::Chip::setRegister(UINT dAddr, UINT dData)
{
	UINT nData = (m_info.dBusID & 0x0f) << 24;
	nData |= (dAddr & 0x100) << 9;
	nData |= (dAddr & 0xff) << 8;
	nData |= (dData & 0xff) << 0;
	return (static_cast<CSpfmLight*>(m_pInterface))->AddEvent(nData);
}

/**
 * Initializes sound chip(clear registers)
 * @retval true If succeeded
 * @retval false If failed
 */
bool CSpfmLight::Chip::init()
{
	return m_pInterface->reset();
}

}	// namespace scci
