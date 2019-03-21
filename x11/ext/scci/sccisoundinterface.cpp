/**
 * @file	sccisoundinterface.cpp
 * @brief	Implementation of sound interfaces
 */

#include "compiler.h"
#include "sccisoundinterface.h"
#include "sccisoundchip.h"
#include "sccisoundinterfacemanager.h"

namespace scci
{

/**
 * Constructor
 * @param[in] pManager The instance of the manager
 * @param[in] deviceName The information
 */
CSoundInterface::CSoundInterface(CSoundInterfaceManager* pManager, const std::oemstring& deviceName)
	: m_nRef(0)
	, m_pManager(pManager)
{
	memset(&m_info, 0, sizeof(m_info));

	milstr_ncpy(m_info.cInterfaceName, deviceName.c_str(), NELEMENTS(m_info.cInterfaceName));
	m_info.iSoundChipCount = 0;
}

/**
 * Destructor
 */
CSoundInterface::~CSoundInterface()
{
	while (!m_chips.empty())
	{
		delete m_chips.begin()->second;
	}
	m_pManager->Delete(this);
}

/**
 * Increments the reference count
 * @return The new reference count
 */
size_t CSoundInterface::AddRef()
{
	m_nRef++;
	return m_nRef;
}

/**
 * Decrements the reference count
 * @return The new reference count
 */
size_t CSoundInterface::Release()
{
	if (m_nRef)
	{
		m_nRef--;
	}
	return m_nRef;
}

/**
 * Release
 */
void CSoundInterface::ReleaseAllChips()
{
	for (std::map<UINT, CSoundChip*>::iterator it = m_chips.begin(); it != m_chips.end(); ++it)
	{
		it->second->Release();
	}
}

/**
 * Get the chip
 * @param[in] iSoundChipType The type of the chip
 * @param[in] dClock The clock of the chip
 * @return The instance of the chip
 */
SoundChip* CSoundInterface::GetSoundChip(SC_CHIP_TYPE iSoundChipType, UINT dClock)
{
	for (std::map<UINT, CSoundChip*>::iterator it = m_chips.begin(); it != m_chips.end(); ++it)
	{
		CSoundChip* pChip = it->second;
		if (!pChip->IsMatch(iSoundChipType, dClock))
		{
			continue;
		}

		SCCI_SOUND_CHIP_INFO* pInfo = pChip->GetSoundChipInfo();
		if (pInfo->bIsUsed)
		{
			continue;
		}
		pInfo->bIsUsed = true;
		AddRef();
		return pChip;
	}
	return NULL;
}

/**
 * Delete
 * @param[in] dBusID Then number of the slot
 */
void CSoundInterface::Delete(UINT dBusID)
{
	std::map<UINT, CSoundChip*>::iterator it = m_chips.find(dBusID);
	if (it != m_chips.end())
	{
		m_chips.erase(it);
		m_info.iSoundChipCount--;
	}
}

}	// namespace scci
