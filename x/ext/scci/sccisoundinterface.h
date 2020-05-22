/**
 * @file	sccisoundinterface.h
 * @brief	Interface of sound interfaces
 */

#pragma once

#include <map>
#include "scci.h"
#include "oemtext.h"

namespace scci
{

class CSoundChip;
class CSoundInterfaceManager;

/**
 * @brief The class of interface
 */
class CSoundInterface : public SoundInterface
{
public:
	CSoundInterface(CSoundInterfaceManager* pManager, const std::oemstring& deviceName);
	virtual ~CSoundInterface();
	virtual size_t AddRef();
	virtual size_t Release();
	void ReleaseAllChips();
	const SCCI_INTERFACE_INFO* GetInfo() const;
	SoundChip* GetSoundChip(SC_CHIP_TYPE iSoundChipType, UINT dClock);

	/**
	 * Initialize
	 * @retval true If succeeded
	 * @retval false If failed
	 */
	virtual bool Initialize() = 0;

	/**
	 * Deinitialize
	 */
	virtual void Deinitialize() = 0;

	/**
	 * Add
	 * @param[in] info The information
	 */
	virtual void Add(const SCCI_SOUND_CHIP_INFO& info) = 0;

protected:
	size_t m_nRef;							/*!< The reference counter */
	CSoundInterfaceManager* m_pManager;		/*!< Manager */
	SCCI_INTERFACE_INFO m_info;				/*!< The information */
	std::map<UINT, CSoundChip*> m_chips;	/*!< The interfaces */
	void Delete(UINT dBusID);

	friend class CSoundChip;
};

/**
 * Gets the informations of the sound interface
 * @return The poitner of the information
 */
inline const SCCI_INTERFACE_INFO* CSoundInterface::GetInfo() const
{
	return &m_info;
}

}	// namespace scci
