/**
 * @file	sccisoundchip.cpp
 * @brief	Implementation of sound chip
 */

#include "compiler.h"
#include "sccisoundchip.h"
#include "sccisoundinterface.h"

namespace scci
{

/**
 * Constructor
 * @param[in] pInterface The instance of the sound interface
 * @param[in] info The information
 */
CSoundChip::CSoundChip(CSoundInterface* pInterface, const SCCI_SOUND_CHIP_INFO& info)
	: m_pInterface(pInterface)
	, m_info(info)
{
}

/**
 * Destructor
 */
CSoundChip::~CSoundChip()
{
	Release();
	m_pInterface->Delete(m_info.dBusID);
}

/**
 * Release the chip
 * @retval true If succeeded
 * @retval false If failed
 */
bool CSoundChip::Release()
{
	if (m_info.bIsUsed)
	{
		m_info.bIsUsed = false;
		m_pInterface->Release();
		return true;
	}
	else
	{
		return false;
	}
}

/**
 * Is macthed?
 * @param[in] iSoundChipType The type of the chip
 * @param[in] dClock The clock of the chip
 * @retval true Yes
 * @retval false No
 */
bool CSoundChip::IsMatch(SC_CHIP_TYPE iSoundChipType, UINT dClock) const
{
	if ((m_info.iSoundChip == iSoundChipType) && (m_info.dClock == dClock))
	{
		return true;
	}
	for (UINT i = 0; i < 2; i++)
	{
		if ((m_info.iCompatibleSoundChip[i] == iSoundChipType) && (m_info.dCompatibleClock[i] == dClock))
		{
			return true;
		}
	}
	return false;
}

/**
 * Gets the informations of the sound chip
 * @return The pointer of informations
 */
const SCCI_SOUND_CHIP_INFO* CSoundChip::getSoundChipInfo()
{
	return &m_info;
}

/**
 * Gets sound chip type
 * @return The type of the chip
 */
SC_CHIP_TYPE CSoundChip::getSoundChipType()
{
	return m_info.iSoundChip;
}

}	// namespace scci
