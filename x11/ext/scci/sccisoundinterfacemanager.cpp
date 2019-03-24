/**
 * @file	sccisoundinterfacemanager.cpp
 * @brief	Implementation of manager
 */

#include "compiler.h"
#include "sccisoundinterfacemanager.h"
#include <algorithm>
#include "sccisoundinterface.h"
#include "sccispfmlight.h"
#include "dosio.h"
#include "common/profile.h"

namespace scci
{

//! Singleton instance
CSoundInterfaceManager CSoundInterfaceManager::sm_instance;

/**
 * Gets instance
 * @return The instance of sound manager
 */
SoundInterfaceManager* GetSoundInterfaceManager()
{
	return CSoundInterfaceManager::GetInstance();
}

/**
 * Constructor
 */
CSoundInterfaceManager::CSoundInterfaceManager()
	: m_nDelayTime(0)
{
}

/**
 * Destructor
 */
CSoundInterfaceManager::~CSoundInterfaceManager()
{
}

/**
 * Delete
 * @param[in] pInterface The instance of the sound interface
 */
void CSoundInterfaceManager::Delete(CSoundInterface* pInterface)
{
	std::vector<CSoundInterface*>::iterator it = std::find(m_interfaces.begin(), m_interfaces.end(), pInterface);
	if (it != m_interfaces.end())
	{
		m_interfaces.erase(it);
	}
}

/**
 * Get the count of interfaces
 * @return The count of interfaces
 */
size_t CSoundInterfaceManager::getInterfaceCount()
{
	return m_interfaces.size();
}

/**
 * Get the information of the interface
 * @param[in] iInterfaceNo The index of interfaces
 * @return The information
 */
const SCCI_INTERFACE_INFO* CSoundInterfaceManager::getInterfaceInfo(size_t iInterfaceNo)
{
	if (iInterfaceNo < m_interfaces.size())
	{
		return m_interfaces[iInterfaceNo]->GetInfo();
	}
	else
	{
		return NULL;
	}
}

/**
 * Gets interface instance
 * @param[in] iInterfaceNo The index of interfaces
 * @return The instance
 */
SoundInterface* CSoundInterfaceManager::getInterface(size_t iInterfaceNo)
{
	if (iInterfaceNo < m_interfaces.size())
	{
		CSoundInterface* pInterface = m_interfaces[iInterfaceNo];
		m_attachedInterfaces.push_back(pInterface);
		pInterface->AddRef();
		return pInterface;
	}
	else
	{
		return NULL;
	}
}

/**
 * Releases the sound interface
 * @param[in] pSoundInterface The instance of the sound interface
 * @retval true If succeeded
 * @retval false If failed
 */
bool CSoundInterfaceManager::releaseInterface(SoundInterface* pSoundInterface)
{
	std::vector<CSoundInterface*>::iterator it = std::find(m_attachedInterfaces.begin(), m_attachedInterfaces.end(), pSoundInterface);
	if (it != m_attachedInterfaces.end())
	{
		m_attachedInterfaces.erase(it);
		static_cast<CSoundInterface*>(pSoundInterface)->Release();
		return true;
	}
	else
	{
		return false;
	}
}

/**
 * Releases all interfaces
 * @retval true If succeeded
 * @retval false If failed
 */
bool CSoundInterfaceManager::releaseAllInterface()
{
	while (!m_attachedInterfaces.empty())
	{
		releaseInterface(m_attachedInterfaces.back());
	}
	return true;
}

/**
 * Gets sound chip instance
 * @param[in] iSoundChipType The type of the chip
 * @param[in] dClock The clock of the chip
 * @return The interface
 */
SoundChip* CSoundInterfaceManager::getSoundChip(SC_CHIP_TYPE iSoundChipType, UINT dClock)
{
	if (iSoundChipType == SC_TYPE_NONE)
	{
		return NULL;
	}

	for (std::vector<CSoundInterface*>::iterator it = m_interfaces.begin(); it != m_interfaces.end(); ++it)
	{
		SoundChip* pChip = (*it)->GetSoundChip(iSoundChipType, dClock);
		if (pChip)
		{
			return pChip;
		}
	}
	return NULL;
}

/**
 * Releases the instance of the sound chip
 * @param[in] pSoundChip The instance of the sound chip
 * @retval true If succeeded
 * @retval false If failed
 */
bool CSoundInterfaceManager::releaseSoundChip(SoundChip* pSoundChip)
{
	delete static_cast<CSoundChip*>(pSoundChip);
	return true;
}

/**
 * Releases all instances of the sound chip
 * @retval true If succeeded
 * @retval false If failed
 */
bool CSoundInterfaceManager::releaseAllSoundChip()
{
	return releaseAllInterface();
}

/**
 * Sets delay time
 * @param[in] dMSec delay time
 * @retval true If succeeded
 * @retval false If failed
 */
bool CSoundInterfaceManager::setDelay(UINT dMSec)
{
	if (dMSec > 10000)
	{
		return false;
	}
	m_nDelayTime = dMSec;

	for (std::vector<CSoundInterface*>::iterator it = m_interfaces.begin(); it != m_interfaces.end(); ++it)
	{
		(*it)->setDelay(dMSec);
	}
	return true;
}

/**
 * Gets delay time
 * @return delay time
 */
UINT CSoundInterfaceManager::getDelay()
{
	return m_nDelayTime;
}

/**
 * Resets all interfaces
 * @retval true If succeeded
 * @retval false If failed
 */
bool CSoundInterfaceManager::reset()
{
	bool err = false;
	for (std::vector<CSoundInterface*>::iterator it = m_interfaces.begin(); it != m_interfaces.end(); ++it)
	{
		if (!(*it)->reset())
		{
			err = true;
		}
	}
	return !err;
}

/**
 * Sound Interface instance initialize
 * @retval true If succeeded
 * @retval false If failed
 */
bool CSoundInterfaceManager::initializeInstance()
{
	OEMCHAR szPath[MAX_PATH];
	milstr_ncpy(szPath, file_getcd(OEMTEXT("SCCI.ini")), NELEMENTS(szPath));
	PFILEH pfh = profile_open(szPath, PFILEH_READONLY);

	m_nDelayTime = profile_readint(OEMTEXT("scci"), OEMTEXT("DelayTime"), 0, pfh);

	OEMCHAR szSections[4096];
	if (profile_getsectionnames(szSections, NELEMENTS(szSections), pfh))
	{
		OEMCHAR* lpSections = szSections;
		while (*lpSections != '\0')
		{
			OEMCHAR* lpKeyName = lpSections;
			const size_t cchKeyName = OEMSTRLEN(lpSections);
			lpSections += cchKeyName + 1;

			if (milstr_memcmp(lpKeyName, OEMTEXT("SPFM Light")) != 0)
			{
				continue;
			}
			if ((lpKeyName[10] != '(') || (lpKeyName[cchKeyName - 1] != ')'))
			{
				continue;
			}

			if (profile_readint(lpKeyName, OEMTEXT("ACTIVE"), 0, pfh) == 0)
			{
				continue;
			}

			std::string deviceName(lpKeyName + 11, lpKeyName + cchKeyName - 1);

			CSoundInterface* pInterface = new CSpfmLight(this, deviceName, m_nDelayTime);
			if (!pInterface->Initialize())
			{
				delete pInterface;
				continue;
			}

			for (UINT i = 0; i < 4; i++)
			{
				SCCI_SOUND_CHIP_INFO info;
				memset(&info, 0, sizeof(info));
				info.dBusID = i;

				OEMCHAR szAppName[32];
				OEMSPRINTF(szAppName, OEMTEXT("SLOT_%02d_CHIP_NAME"), i);
				profile_read(lpKeyName, szAppName, OEMTEXT(""), info.cSoundChipName, NELEMENTS(info.cSoundChipName), pfh);

				OEMSPRINTF(szAppName, OEMTEXT("SLOT_%02d_CHIP_ID"), i);
				info.iSoundChip = static_cast<SC_CHIP_TYPE>(profile_readint(lpKeyName, szAppName, 0, pfh));

				OEMSPRINTF(szAppName, OEMTEXT("SLOT_%02d_CHIP_CLOCK"), i);
				info.dClock = profile_readint(lpKeyName, szAppName, 0, pfh);

				if (info.iSoundChip == 0)
				{
					continue;
				}

				for (UINT j = 0; j < 2; j++)
				{
					OEMSPRINTF(szAppName, OEMTEXT("SLOT_%02d_CHIP_ID_CMP%d"), i, j + 1);
					info.iCompatibleSoundChip[j] = static_cast<SC_CHIP_TYPE>(profile_readint(lpKeyName, szAppName, 0, pfh));

					OEMSPRINTF(szAppName, OEMTEXT("SLOT_%02d_CHIP_CLOCK_CMP%d"), i, j + 1);
					info.dCompatibleClock[j] = profile_readint(lpKeyName, szAppName, 0, pfh);
				}

				OEMSPRINTF(szAppName, OEMTEXT("SLOT_%02d_CHIP_LOCATION"), i);
				info.dSoundLocation = static_cast<SC_CHIP_LOCATION>(profile_readint(lpKeyName, szAppName, 0, pfh));

				pInterface->Add(info);
			}
			m_interfaces.push_back(pInterface);
		}
	}
	profile_close(pfh);

	return true;
}

/**
 * Sound Interface instance release
 * @retval true If succeeded
 * @retval false If failed
 */
bool CSoundInterfaceManager::releaseInstance()
{
	while (!m_interfaces.empty())
	{
		CSoundInterface* pInterface = m_interfaces.back();
		m_interfaces.pop_back();
		delete pInterface;
	}
	return true;
}

}	// namespace scci
