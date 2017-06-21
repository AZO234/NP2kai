/**
 * @file	sccisoundchip.h
 * @brief	Interface of sound chip
 */

#pragma once

#include "scci.h"

namespace scci
{

class CSoundInterface;

/**
 * @brief The class of chip
 */
class CSoundChip : public SoundChip
{
public:
	CSoundChip(CSoundInterface* pInterface, const SCCI_SOUND_CHIP_INFO& info);
	virtual ~CSoundChip();
	bool Release();
	SCCI_SOUND_CHIP_INFO* GetSoundChipInfo();
	bool IsMatch(SC_CHIP_TYPE iSoundChipType, UINT dClock) const;

	// SoundChip
	virtual const SCCI_SOUND_CHIP_INFO* getSoundChipInfo();
	virtual SC_CHIP_TYPE getSoundChipType();

protected:
	CSoundInterface* m_pInterface;	/*!< Interface */
	SCCI_SOUND_CHIP_INFO m_info;	/*!< The information */
};

/**
 * Gets the informations of the sound chip
 * @return The pointer of informations
 */
inline SCCI_SOUND_CHIP_INFO* CSoundChip::GetSoundChipInfo()
{
	return &m_info;
}

}	// namespace scci
