/**
 * @file	scci.h
 * @brief	Sound Chip common Interface
 */

#pragma once

#include "SCCIDefines.h"

namespace scci
{

class SoundChip;

/**
 * @brief Sound Interface Infomation
 */
struct SCCI_INTERFACE_INFO
{
	OEMCHAR cInterfaceName[64];				/*!< Interface Name */
	size_t iSoundChipCount;					/*!< Sound Chip Count */
};

/**
 * @brief Sound Chip Infomation
 */
struct SCCI_SOUND_CHIP_INFO
{
	OEMCHAR cSoundChipName[64];				/*!< Sound Chip Name */
	SC_CHIP_TYPE iSoundChip;				/*!< Sound Chip ID */
	SC_CHIP_TYPE iCompatibleSoundChip[2];	/*!< Compatible Sound Chip ID */
	UINT dClock;							/*!< Sound Chip clock */
	UINT dCompatibleClock[2];				/*!< Sound Chip clock */
	bool bIsUsed;							/*!< Sound Chip Used Check */
	UINT dBusID;							/*!< 接続バスID */
	SC_CHIP_LOCATION dSoundLocation;		/*!< サウンドロケーション */
};

class SoundInterface;

/**
 * @brief Sound Interface Manager
 */
class SoundInterfaceManager
{
public:
	/**
	 * Gets the count of interfaces
	 * @return The count of interfaces
	 */
	virtual size_t getInterfaceCount() = 0;

	/**
	 * Gets the information of the interface
	 * @param[in] iInterfaceNo The index of interfaces
	 * @return The information
	 */
	virtual const SCCI_INTERFACE_INFO* getInterfaceInfo(size_t iInterfaceNo) = 0;

	/**
	 * Gets interface instance
	 * @param[in] iInterfaceNo The index of interfaces
	 * @return The instance
	 */
	virtual SoundInterface* getInterface(size_t iInterfaceNo) = 0;

	/**
	 * Releases interface instance
	 * @param[in] pSoundInterface The instance of the interface
	 * @retval true If succeeded
	 * @retval false If failed
	 */
	virtual bool releaseInterface(SoundInterface* pSoundInterface) = 0;

	/**
	 * Release all interface instance
	 * @retval true If succeeded
	 * @retval false If failed
	 */
	virtual bool releaseAllInterface() = 0;

	/**
	 * Gets instance of the sound chip
	 * @param[in] iSoundChipType The type of the chip
	 * @param[in] dClock The clock of the chip
	 * @return The interface
	 */
	virtual SoundChip* getSoundChip(SC_CHIP_TYPE iSoundChipType, UINT dClock) = 0;

	/**
	 * Releases the instance of the sound chip
	 * @param[in] pSoundChip The instance of the sound chip
	 * @retval true If succeeded
	 * @retval false If failed
	 */
	virtual bool releaseSoundChip(SoundChip* pSoundChip) = 0;

	/**
	 * Releases all instances of the sound chip
	 * @retval true If succeeded
	 * @retval false If failed
	 */
	virtual bool releaseAllSoundChip() = 0;

	/**
	 * Sets delay time
	 * @param[in] dMSec delay time
	 * @retval true If succeeded
	 * @retval false If failed
	 */
	virtual bool setDelay(UINT dMSec) = 0;

	/**
	 * Gets delay time
	 * @return delay time
	 */
	virtual UINT getDelay() = 0;

	/**
	 * Resets all interfaces
	 * @retval true If succeeded
	 * @retval false If failed
	 */
	virtual bool reset() = 0;

	/**
	 * Sound Interface instance initialize
	 * @retval true If succeeded
	 * @retval false If failed
	 */
	virtual bool initializeInstance() = 0;

	/**
	 * Sound Interface instance release
	 * @retval true If succeeded
	 * @retval false If failed
	 */
	virtual bool releaseInstance() = 0;
};

/**
 * @brief Sound Interface(LOW level APIs)
 */
class SoundInterface
{
public:
	/**
	 * Is supported low level API
	 * @retval true yes
	 * @retval false no
	 */
	virtual bool isSupportLowLevelApi() = 0;

	/**
	 * Sends data to the interface
	 * @param[in] pData The buffer of data
	 * @param[in] dSendDataLen The length of data
	 * @retval true If succeeded
	 * @retval false If failed
	 */
	virtual bool setData(const unsigned char* pData, size_t dSendDataLen) = 0;

	/**
	 * Gets data from the interface
	 * @param[out] pData The buffer of data
	 * @param[in] dGetDataLen The length of data
	 * @return The size of read
	 */
	virtual size_t getData(unsigned char* pData, size_t dGetDataLen) = 0;

	/**
	 * Sets delay time
	 * @param[in] dDelay delay time
	 * @retval true If succeeded
	 * @retval false If failed
	 */
	virtual bool setDelay(UINT dDelay) = 0;

	/**
	 * Gets delay time
	 * @return delay time
	 */
	virtual UINT getDelay() = 0;

	/**
	 * Resets the interface
	 * @retval true If succeeded
	 * @retval false If failed
	 */
	virtual bool reset() = 0;
};

/**
 * @brief Sound Chip
 */
class SoundChip
{
public:
	/**
	 * Gets the informations of the sound chip
	 * @return The pointer of informations
	 */
	virtual const SCCI_SOUND_CHIP_INFO* getSoundChipInfo() = 0;

	/**
	 * Gets sound chip type
	 * @return The type of the chip
	 */
	virtual SC_CHIP_TYPE getSoundChipType() = 0;

	/**
	 * Sets Register data
	 * Writes the register
	 * @param[in] dAddr The address of register
	 * @param[in] dData The data
	 * @retval true If succeeded
	 * @retval false If failed
	 */
	virtual bool setRegister(UINT dAddr, UINT dData) = 0;

	/**
	 * Initializes sound chip(clear registers)
	 * @retval true If succeeded
	 * @retval false If failed
	 */
	virtual bool init() = 0;
};

SoundInterfaceManager* GetSoundInterfaceManager();

}	// namespace scci
