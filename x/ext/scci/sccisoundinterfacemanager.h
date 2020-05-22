/**
 * @file	sccisoundinterfacemanager.h
 * @brief	Interface of the SCCI manager
 */

#pragma once

#include <vector>
#include "scci.h"

namespace scci
{

class CSoundInterface;

/**
 * @brief The class of the sound interface manager
 */
class CSoundInterfaceManager : public SoundInterfaceManager
{
public:
	static CSoundInterfaceManager* GetInstance();

	virtual size_t getInterfaceCount();
	virtual const SCCI_INTERFACE_INFO* getInterfaceInfo(size_t iInterfaceNo);
	virtual SoundInterface* getInterface(size_t iInterfaceNo);
	virtual bool releaseInterface(SoundInterface* pSoundInterface);
	virtual bool releaseAllInterface();
	virtual SoundChip* getSoundChip(SC_CHIP_TYPE iSoundChipType, UINT dClock);
	virtual bool releaseSoundChip(SoundChip* pSoundChip);
	virtual bool releaseAllSoundChip();
	virtual bool setDelay(UINT dMSec);
	virtual UINT getDelay();
	virtual bool reset();
	virtual bool initializeInstance();
	virtual bool releaseInstance();

private:
	static CSoundInterfaceManager sm_instance;				/*!< Singleton */
	UINT m_nDelayTime;										/*!< Delay time */
	std::vector<CSoundInterface*> m_interfaces;				/*!< The list of interfaces */
	std::vector<CSoundInterface*> m_attachedInterfaces;		/*!< The list of attached interfaces */

	CSoundInterfaceManager();
	~CSoundInterfaceManager();
	void Delete(CSoundInterface* pInterface);

	friend class CSoundInterface;
};

/**
 * Gets instance
 * @return The instance of sound manager
 */
inline CSoundInterfaceManager* CSoundInterfaceManager::GetInstance()
{
	return &sm_instance;
}

}	// namespace scci
