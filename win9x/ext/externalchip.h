/**
 * @file	externalchip.h
 * @brief	Interface of thg external modules
 */

#pragma once

/**
 * @brief The interface of thg external modules
 */
class IExternalChip
{
public:
	/**
	 * ChipType
	 */
	enum ChipType
	{
		kNone		= 0,	/*!< None */
		kAY8910,			/*!< AY-3-8910 */

		kYM2203,			/*!< OPN */
		kYM2608,			/*!< OPNA */
		kYM3438,			/*!< OPN2 */
		kYMF288,			/*!< OPN3 */

		kYM3812,			/*!< OPL2 */
		kYMF262,			/*!< OPL3 */
		kY8950,				/*!< Y8950 */

		kYM2151				/*!< OPM */
	};

	/**
	 * MessageType
	 */
	enum
	{
		kMute				= 0,
		kBusy
	};

	/**
	 * Destructor
	 */
	virtual ~IExternalChip()
	{
	}

	/**
	 * Get chip type
	 * @return The type of the chip
	 */
	virtual ChipType GetChipType() = 0;

	/**
	 * Reset
	 */
	virtual void Reset() = 0;

	/**
	 * Writes register
	 * @param[in] nAddr The address
	 * @param[in] cData The data
	 */
	virtual void WriteRegister(UINT nAddr, UINT8 cData) = 0;

	/**
	 * Message
	 * @param[in] nMessage The message
	 * @param[in] nParameter The parameter
	 * @return Result
	 */
	virtual INTPTR Message(UINT nMessage, INTPTR nParameter = 0) = 0;
};
