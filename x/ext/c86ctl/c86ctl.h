/**
 * @file	c86ctl.h
 * @brief	Defines of C86CTL
 */

#pragma once

namespace c86ctl
{

/**
 * Status code
 */
enum C86CtlErr
{
	C86CTL_ERR_NONE				= 0,		/*!< Succeeded */
	C86CTL_ERR_UNKNOWN			= -1,		/*!< Unknown */
	C86CTL_ERR_INVALID_PARAM	= -2,		/*!< Invalid parameter */
	C86CTL_ERR_UNSUPPORTED		= -3,		/*!< Unspported */
	C86CTL_ERR_NODEVICE			= -1000,	/*!< No devices */
	C86CTL_ERR_NOT_IMPLEMENTED	= -9999,	/*!< Not implemented */
};

/**
 * Chip type
 */
enum ChipType
{
	CHIP_UNKNOWN		= 0,		/*!< Unknown */
	CHIP_OPNA			= 0x0001,	/*!< OPNA */
	CHIP_OPM			= 0x0002,	/*!< OPM */
	CHIP_OPN3L			= 0x0003,	/*!< OPN3L */
	CHIP_OPL3			= 0x0004,	/*!< OPL3 */
	CHIP_OPLL			= 0x0005,	/*!< OPLL */
	CHIP_SN76489		= 0x0006,
	CHIP_AY38910		= 0x0007,
	CHIP_YM2203			= 0x0008,
	CHIP_YM2612			= 0x0009,
	CHIP_YM3526			= 0x000a,
	CHIP_YM3812			= 0x000b,
	CHIP_YMF271			= 0x000c,
	CHIP_YMF278B		= 0x000d,
	CHIP_YMZ280B		= 0x000e,
	CHIP_YMF297			= 0x000f,
	CHIP_YM2610B		= 0x0010,
	CHIP_Y8950			= 0x0020,
	CHIP_YM3438			= 0x0021,

	CHIP_YM2608NOADPCM	= 0x10000 | CHIP_OPNA,
	CHIP_SN76496		= 0x10000 | CHIP_SN76489,
	CHIP_YM2149			= 0x10000 | CHIP_AY38910,
	CHIP_Y8950ADPCM 	= 0x10020 | CHIP_Y8950
};

/**
 * The type of CBUS boards
 */
enum CBUS_BOARD_TYPE
{
	CBUS_BOARD_UNKNOWN					= 0,			/*!< Unknown */
	CBUS_BOARD_14						= 0x0001,		/*!< NEC PC-9801-14 */
	CBUS_BOARD_AMD98					= 0x0011,		/*!< SYSTEM SACOM AMD-98 */
	CBUS_BOARD_26						= 0x0002,		/*!< NEC PC-9801-26, 26K */
	CBUS_BOARD_SOUND_ORCHESTRA			= 0x0012,		/*!< SNE Sound Orchestra */
	CBUS_BOARD_SOUND_ORCHESTRA_L		= 0x0022,		/*!< SNE Sound Orchestra L */
	CBUS_BOARD_SOUND_ORCHESTRA_V		= 0x0032,		/*!< SNE Sound Orchestra V */
	CBUS_BOARD_SOUND_ORCHESTRA_VS		= 0x0042,		/*!< SNE Sound Orchestra VS */
	CBUS_BOARD_SOUND_ORCHESTRA_LS		= 0x0052,		/*!< SNE Sound Orchestra LS */
	CBUS_BOARD_SOUND_ORCHESTRA_MATE		= 0x0062,		/*!< SNE Sound Orchestra MATE */
	CBUS_BOARD_MULTIMEDIA_ORCHESTRA		= 0x0072,		/*!< SNE Multimedia Orchestra */
	CBUS_BOARD_LITTLE_ORCHESTRA			= 0x0082,		/*!< SNE Litte Orchestra */
	CBUS_BOARD_LITTLE_ORCHESTRA_L		= 0x0092,		/*!< SNE Litte Orchestra L */
	CBUS_BOARD_LITTLE_ORCHESTRA_RS		= 0x00a2,		/*!< SNE Litte Orchestra RS */
	CBUS_BOARD_LITTLE_ORCHESTRA_LS		= 0x00b2,		/*!< SNE Litte Orchestra LS */
	CBUS_BOARD_LITTLE_ORCHESTRA_SS		= 0x00c2,		/*!< SNE Litte Orchestra SS */
	CBUS_BOARD_LITTLE_ORCHESTRA_MATE	= 0x00d2,		/*!< SNE Litte Orchestra MATE */
	CBUS_BOARD_LITTLE_ORCHESTRA_FELLOW	= 0x00e2,		/*!< SNE Litte Orchestra FELLOW */
	CBUS_BOARD_JOY2						= 0x00f2,		/*!< SNE JOY-2 */
	CBUS_BOARD_SOUND_GRANPRI			= 0x0102,		/*!< SNE SOUND GRANPRI */
	CBUS_BOARD_TN_F3FM					= 0x0112,		/*!< TOKYO NEEDS TN-F3FM */
	CBUS_BOARD_73						= 0x0003,		/*!< NEC PC-9801-73 */
	CBUS_BOARD_86						= 0x0023,		/*!< NEC PC-9801-86 */
	CBUS_BOARD_ASB01					= 0x0043,		/*!< SIS Amusement Sound Board ASB-01 */
	CBUS_BOARD_SPEAKBOARD				= 0x0053,		/*!< IDOL JAPAN SpeakBoard */
	CBUS_BOARD_SOUNDPLAYER98			= 0x0063,		/*!< SPB-98 */
	CBUS_BOARD_SECONDBUS86				= 0x0073,		/*!< second-bus86 */
	CBUS_BOARD_SOUNDEDGE				= 0x0083,		/*!< sound-edge */
	CBUS_BOARD_WINDUO					= 0x0093,		/*!< win-duo */
	CBUS_BOARD_OTOMI					= 0x00a3,		/*!< MAD FACTORY OTOMI */
	CBUS_BOARD_WAVEMASTER				= 0x00b3,		/*!< Q-Vision WaveMaster */
	CBUS_BOARD_WAVESMIT					= 0x00c3,		/*!< Q-Vision WaveSMIT */
	CBUS_BOARD_WAVESTAR					= 0x00d3,		/*!< Q-Vision WaveStar */
	CBUS_BOARD_WSN_A4F					= 0x00e3,		/*!< Buffalo WSN-A4F/A2F */
	CBUS_BOARD_SXM_F					= 0x00f3,		/*!< SXM-F */
	CBUS_BOARD_SRN_F					= 0x0103,		/*!< SRN-F */
	CBUS_BOARD_SB16						= 0x0004,		/*!< sound-blaster 16 (CT2720) */
	CBUS_BOARD_SB16_2203				= 0x0014,		/*!< sound-blaster 16 with YM2203 (CT2720) */
	CBUS_BOARD_SB16VALUE				= 0x0024,		/*!< sound-blaster 16Value (CT3720) */
	CBUS_BOARD_POWERWINDOW_T64S			= 0x0034,		/*!< canopus PowerWindow T64S */
	CBUS_BOARD_PCSB2					= 0x0044,		/*!< EPSON PCSB2 */
	CBUS_BOARD_WGS98S					= 0x0054,		/*!< COM.TECH WGS-98S */
	CBUS_BOARD_SRB_G					= 0x0064,		/*!< buffalo SRB-G */
	CBUS_BOARD_MIDI_ORCHESTRA_MIDI3		= 0x0074,		/*!< SNE MIDI ORCHESTRA MIDI-3 */
	CBUS_BOARD_SB_AWE32					= 0x0005,		/*!< SoundBlaster AWE32 (CT3610) */
	CBUS_BOARD_118						= 0x0006		/*!< NEC PC-9801-118 */
};

/**
 * @brief Device information structure
 */
struct Devinfo
{
	char Devname[16];		/*!< Device name */
	char Rev;				/*!< Revision */
	char Serial[15];		/*!< Serial */
};

/**
 * The interface ID
 */
enum IID
{
	IID_IRealChipBase	= 0,
	IID_IRealChip,
	IID_IRealChip2,
	IID_IRealChip3,
	IID_IGimic,
	IID_IGimic2,
	IID_IC86Usb
};

/**
 * @brief The class of IUnknown
 */
class IRealUnknown
{
public:
	/**
	 * Increments the reference count
	 * @return The new reference count
	 */
	virtual size_t AddRef() = 0;

	/**
	 * Decrements the reference count
	 * @return The new reference count
	 */
	virtual size_t Release() = 0;
};

/**
 * @brief The class of IRealChipBase
 */
class IRealChipBase : public IRealUnknown
{
public:
	/**
	 * Initialize
	 * @return C86CTL_ERR
	 */
	virtual C86CtlErr initialize() = 0;

	/**
	 * Deinitialize
	 * @return C86CTL_ERR
	 */
	virtual C86CtlErr deinitialize() = 0;

	/**
	 * Gets the count of chips
	 * @return The chips
	 */
	virtual size_t getNumberOfChip() = 0;

	/**
	 * Gets interfaces
	 * @param[in] id ID
	 * @param[in] riid The interface ID
	 * @param[out] ppi The pointer of the interface
	 * @return C86CTL_ERR
	 */
	virtual C86CtlErr getChipInterface(size_t id, IID riid, void** ppi) = 0;
};

/**
 * @brief The class of IRealChip
 */
class IRealChip : public IRealUnknown
{
public:
	/**
	 * Reset
	 * @return C86CTL_ERR
	 */
	virtual C86CtlErr reset() = 0;

	/**
	 * Output
	 * @param[in] nAddr The address
	 * @param[in] cData The data
	 */
	virtual void out(UINT nAddr, UINT8 cData) = 0;

	/**
	 * Input
	 * @param[in] nAddr The address of registers
	 * @return The data
	 */
	virtual UINT8 in(UINT nAddr) = 0;
};

/**
 * @brief The class of IRealChip2
 */
class IRealChip2 : public IRealChip
{
public:
	/**
	 * Gets the current status
	 * @param[in] nAddr The address
	 * @param[out] pcStatus The status
	 * @return C86CTL_ERR
	 */
	virtual C86CtlErr getChipStatus(UINT nAddr, UINT8* pcStatus) = 0;

	/**
	 * Output
	 * @param[in] nAddr The address
	 * @param[in] cData The data
	 */
	virtual void directOut(UINT nAddr, UINT8 cData) = 0;
};

/**
 * @brief The class of IRealChip3
 */
class IRealChip3 : public IRealChip2
{
public:
	/**
	 * Gets the type of the chip
	 * @param[out] pnType A pointer of type
	 * @return C86CTL_ERR
	 */
	virtual C86CtlErr getChipType(ChipType* pnType) = 0;
};

/**
 * @brief The class of IGimic
 */
class IGimic : public IRealUnknown
{
public:
	/**
	 * Gets the informations of firm
	 * @param[out] pnMajor A pointer to the major
	 * @param[out] pnMinor A pointer to the minor
	 * @param[out] pnRev A pointer to the revision
	 * @param[out] pnBuild A pointer to the number of build
	 * @return C86CTL_ERR
	 */
	virtual C86CtlErr getFWVer(UINT* pnMajor, UINT* pnMinor, UINT* pnRev, UINT* pnBuild) = 0;

	/**
	 * Gets the informations of the mother
	 * @param[out] pInfo A pointer to the informations
	 * @return C86CTL_ERR
	 */
	virtual C86CtlErr getMBInfo(Devinfo* pInfo) = 0;

	/**
	 * Gets the informations of modules
	 * @param[out] pInfo A pointer to the informations
	 * @return C86CTL_ERR
	 */
	virtual C86CtlErr getModuleInfo(Devinfo* pInfo) = 0;

	/**
	 * Sets the volumes of SSG
	 * @param[in] cVolume The volume
	 * @return C86CTL_ERR
	 */
	virtual C86CtlErr setSSGVolume(UINT8 cVolume) = 0;

	/**
	 * Gets the volume of SSG
	 * @param[out] pcVolume A pointer of the volume
	 * @return C86CTL_ERR
	 */
	virtual C86CtlErr getSSGVolume(UINT8* pcVolume) = 0;

	/**
	 * Sets the clock
	 * @param[in] nClock The clock
	 * @return C86CTL_ERR
	 */
	virtual C86CtlErr setPLLClock(UINT nClock) = 0;

	/**
	 * Gets the clock
	 * @param[out] pnClock A pointer to the clock
	 * @return C86CTL_ERR
	 */
	virtual C86CtlErr getPLLClock(UINT* pnClock) = 0;
};

/**
 * @brief The class of IGimic2
 */
class IGimic2 : public IGimic
{
public:
	/**
	 * Gets the type of the modules
	 * @param[out] pnType The type
	 * @return C86CTL_ERR
	 */
	virtual C86CtlErr getModuleType(ChipType* pnType) = 0;
};

C86CtlErr CreateInstance(IID riid, void** ppi);

}
