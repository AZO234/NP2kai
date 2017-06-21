/**
 * @file	c86ctlc86box.cpp
 * @brief	Implementation of C86BOX
 */

#include "compiler.h"
#include "c86ctlc86box.h"
#include <algorithm>

namespace c86ctl
{

//! The maximum chips
#define NMAXCHIPS	4

/**
 * @brief The information of sound baords
 */
struct BOARD_INFO
{
	UINT type;						//!< The type of boards
	UINT nchips;					//!< The numbers of chips
	ChipType chiptype[NMAXCHIPS];	//!< The type of chips
};

/**
 * @brief The board's names
 */
struct BoardName
{
	CBUS_BOARD_TYPE nId;			//!< The type of boards
	const char* lpName;				//!< The name of boards
};

/**
 * The tables of baords
 */
static const BoardName s_names[] =
{
	{CBUS_BOARD_14,							"PC-9801-14"},
	{CBUS_BOARD_26,							"PC-9801-26K"},
	{CBUS_BOARD_SOUND_ORCHESTRA,			"ORCHESTRA"},
	{CBUS_BOARD_SOUND_ORCHESTRA_L,			"ORCHESTRA-L"},
	{CBUS_BOARD_SOUND_ORCHESTRA_V,			"ORCHESTRA-V"},
	{CBUS_BOARD_SOUND_ORCHESTRA_VS,			"ORCHESTRA-VS"},
	{CBUS_BOARD_SOUND_ORCHESTRA_LS,			"ORCHESTRA-LS"},
	{CBUS_BOARD_SOUND_ORCHESTRA_MATE,		"ORCHESTRA-MATE"},
	{CBUS_BOARD_MULTIMEDIA_ORCHESTRA,		"MMO"},
	{CBUS_BOARD_LITTLE_ORCHESTRA,			"LITTLE"},
	{CBUS_BOARD_LITTLE_ORCHESTRA_L,			"LITTLE-L"},
	{CBUS_BOARD_LITTLE_ORCHESTRA_RS,		"LITTLE-RS"},
	{CBUS_BOARD_LITTLE_ORCHESTRA_LS,		"LITTLE-LS"},
	{CBUS_BOARD_LITTLE_ORCHESTRA_SS,		"LITTLE-SS"},
	{CBUS_BOARD_LITTLE_ORCHESTRA_MATE,		"LITTLE-MATE"},
	{CBUS_BOARD_LITTLE_ORCHESTRA_FELLOW,	"LITTLE-FELLOW"},
	{CBUS_BOARD_JOY2,						"JOY2"},
	{CBUS_BOARD_SOUND_GRANPRI,				"GRANPRI"},
	{CBUS_BOARD_TN_F3FM,					"TN-F3FM"},
	{CBUS_BOARD_73,							"PC-9801-73"},
	{CBUS_BOARD_86,							"PC-9801-86"},
	{CBUS_BOARD_ASB01,						"ASB-01"},
	{CBUS_BOARD_SPEAKBOARD,					"SPEAKBOARD"},
	{CBUS_BOARD_SOUNDPLAYER98,				"SPB98"},
	{CBUS_BOARD_SECONDBUS86,				"SB86"},
	{CBUS_BOARD_SOUNDEDGE,					"SOUNDEDGE"},
	{CBUS_BOARD_WINDUO,						"WINDUO"},
	{CBUS_BOARD_OTOMI,						"OTOMI"},
	{CBUS_BOARD_WAVEMASTER,					"WAVEMASTER"},
	{CBUS_BOARD_WAVESMIT,					"WAVESIMIT"},
	{CBUS_BOARD_WAVESTAR,					"WAVESTAR"},
	{CBUS_BOARD_WSN_A4F,					"WSN-A4F"},
	{CBUS_BOARD_SB16,						"SB16"},
	{CBUS_BOARD_SB16_2203,					"SB16"},
	{CBUS_BOARD_SB16VALUE,					"SB16VALUE"},
	{CBUS_BOARD_POWERWINDOW_T64S,			"PW-T64S"},
	{CBUS_BOARD_PCSB2,						"PC-SB2"},
	{CBUS_BOARD_WGS98S,						"WGS-98S"},
	{CBUS_BOARD_SXM_F,						"SXM-F"},
	{CBUS_BOARD_SRB_G,						"SRB-G"},
	{CBUS_BOARD_MIDI_ORCHESTRA_MIDI3	,	"MIDI-3"},
	{CBUS_BOARD_SB_AWE32,					"SB-AWE32"},
	{CBUS_BOARD_118,						"PC-9801-118"},
};

/**
 * Gets the name from type of boards
 * @param[in] nType The type of boards
 * @return The name of boards
 */
static const char* GetBoardName(CBUS_BOARD_TYPE nType)
{
	const CBUS_BOARD_TYPE nId = static_cast<CBUS_BOARD_TYPE>(nType & 0xffff);
	for (UINT i = 0; i < NELEMENTS(s_names); i++)
	{
		if (s_names[i].nId == nId)
		{
			return s_names[i].lpName;
		}
	}
	return "UNKNOWN";
}

/**
 * Constructor
 * @param[in] nIndex The index
 */
CC86Box::CC86Box(UINT nIndex)
	: m_nRef(1)
	, m_nIndex(nIndex)
	, m_nQueIndex(0)
	, m_nQueCount(0)
{
}

/**
 * Destructor
 */
CC86Box::~CC86Box()
{
}

/**
 * Increments the reference count
 * @return The new reference count
 */
size_t CC86Box::AddRef()
{
	m_nRef++;
	return m_nRef;
}

/**
 * Decrements the reference count
 * @return The new reference count
 */
size_t CC86Box::Release()
{
	m_nRef--;
	return m_nRef;
}

/**
 * Initialize
 * @return C86CTL_ERR
 */
C86CtlErr CC86Box::initialize()
{
	if (!m_usb.Open(0x16c0, 0x27d8, m_nIndex))
	{
		return C86CTL_ERR_NODEVICE;
	}

	for (UINT i = 0; i < 2; i++)
	{
		BOARD_INFO info;
		if (m_usb.CtrlXfer(0xc0, 0x81, 0, i, &info, sizeof(info)) != sizeof(info))
		{
			continue;
		}

		printf("Found %s in C86BOX-Slot#%c / chips: %d\n", GetBoardName(static_cast<CBUS_BOARD_TYPE>(info.type)), 'A' + i, info.nchips);

		for (UINT j = 0; j < info.nchips; j++)
		{
			const UINT nDevId = (i << 3) | j;
			m_chips.push_back(new Chip3(this, nDevId, info.chiptype[j]));
		}
	}

	if (m_chips.empty())
	{
		m_usb.Close();
		return C86CTL_ERR_NODEVICE;
	}

	m_nQueIndex = 0;
	m_nQueCount = 0;
	Start();
	return C86CTL_ERR_NONE;
}

/**
 * Deinitialize
 * @return C86CTL_ERR
 */
C86CtlErr CC86Box::deinitialize()
{
	Stop();
	m_usb.Close();

	for (std::vector<Chip3*>::iterator it = m_chips.begin(); it != m_chips.end(); ++it)
	{
		delete *it;
	}
	m_chips.clear();

	return C86CTL_ERR_NONE;
}

/**
 * Gets the count of chips
 * @return The chips
 */
size_t CC86Box::getNumberOfChip()
{
	return m_chips.size();
}

/**
 * Gets interfaces
 * @param[in] id ID
 * @param[in] riid The interface ID
 * @param[out] ppi The pointer of the interface
 * @return C86CTL_ERR
 */
C86CtlErr CC86Box::getChipInterface(size_t id, IID riid, void** ppi)
{
	if (ppi == NULL)
	{
		return C86CTL_ERR_INVALID_PARAM;
	}

	if (id >= m_chips.size())
	{
		return C86CTL_ERR_NODEVICE;
	}

	IRealUnknown* pUnknown = NULL;
	switch (riid)
	{
		case IID_IRealChip:
		case IID_IRealChip2:
		case IID_IRealChip3:
			pUnknown = m_chips[id];
			break;

		default:
			break;
	}

	if (pUnknown == NULL)
	{
		return C86CTL_ERR_UNSUPPORTED;
	}

	pUnknown->AddRef();
	*ppi = pUnknown;
	return C86CTL_ERR_NONE;
}

/**
 * Sends and receives data from USB
 * @param[in] lpOutput A pointer to the buffer that sends the data
 * @param[in] cbOutput The number of bytes to be written
 * @param[out] lpInput A pointer to the buffer that receives the data
 * @param[in] cbInput The maximum number of bytes to be read
 * @return C86CTL_ERR
 */
C86CtlErr CC86Box::Transaction(const void* lpOutput, int cbOutput, void* lpInput, int cbInput)
{
	if (!m_usb.IsOpened())
	{
		return C86CTL_ERR_NODEVICE;
	}

	char sBuffer[64];
	if ((lpOutput == NULL) || (cbOutput <= 0) || (cbOutput >= sizeof(sBuffer)))
	{
		return C86CTL_ERR_INVALID_PARAM;
	}

	::memcpy(sBuffer, lpOutput, cbOutput);
	if (cbOutput < sizeof(sBuffer))
	{
		::memset(sBuffer + cbOutput, 0xff, sizeof(sBuffer) - cbOutput);
	}

	m_usbGuard.Enter();
	int nResult = m_usb.WriteBulk(sBuffer, sizeof(sBuffer));
	if ((nResult == sizeof(sBuffer)) && (cbInput > 0))
	{
		nResult = m_usb.ReadBulk(sBuffer, sizeof(sBuffer));
	}
	m_usbGuard.Leave();

	if (nResult != sizeof(sBuffer))
	{
		return C86CTL_ERR_UNKNOWN;
	}

	if ((lpInput != NULL) && (cbInput > 0))
	{
		cbInput = (std::min)(cbInput, static_cast<int>(sizeof(sBuffer)));
		::memcpy(lpInput, sBuffer, cbInput);
	}
	return C86CTL_ERR_NONE;
}

/**
 * Reset
 * @return C86CTL_ERR
 */
C86CtlErr CC86Box::Reset()
{
	m_queGuard.Enter();
	m_nQueIndex = 0;
	m_nQueCount = 0;
	m_queGuard.Leave();

	m_usbGuard.Enter();
	const int r = m_usb.CtrlXfer(0x40, 0x11);
	m_usbGuard.Leave();

	return (r >= 0) ? C86CTL_ERR_NONE : C86CTL_ERR_UNKNOWN;
}

/**
 * Output
 * @param[in] nDevId The id of chips
 * @param[in] nAddr The address of registers
 * @param[in] cData The data
 */
void CC86Box::Out(UINT nDevId, UINT nAddr, UINT8 cData)
{
	m_queGuard.Enter();
	while (m_nQueCount >= NELEMENTS(m_que))
	{
		m_queGuard.Leave();
		Delay(1000);
		m_queGuard.Enter();
	}

	m_que[(m_nQueIndex + m_nQueCount) % NELEMENTS(m_que)] = (nDevId << 17) | ((nAddr & 0x1ff) << 8) | cData;
	m_nQueCount++;

	m_queGuard.Leave();
}

/**
 * Thread
 * @retval true Cont.
 */
bool CC86Box::Task()
{
	/* builds data */
	UINT8 sData[64];
	int nIndex = 0;

	m_queGuard.Enter();
	while ((m_nQueCount) && ((nIndex + 8) < NELEMENTS(sData)))
	{
		const UINT data = m_que[m_nQueIndex];
		m_nQueIndex = (m_nQueIndex + 1) % NELEMENTS(m_que);
		m_nQueCount--;

		sData[nIndex++] = static_cast<UINT8>(data >> 0);
		sData[nIndex++] = static_cast<UINT8>(data >> 8);
		sData[nIndex++] = static_cast<UINT8>(data >> 16);
		sData[nIndex++] = static_cast<UINT8>(data >> 24);
	}
	m_queGuard.Leave();

	/* writes */
	if (nIndex > 0)
	{
		Transaction(sData, nIndex);
	}
	else
	{
		Delay(1000);
	}
	return true;
}

/* IRealChip3 */

/**
 * Constructor
 * @param[in] pC86Box The instance of the device
 * @param[in] nDevId The id of the chip
 * @param[in] nChipType The type of the chip
 */
CC86Box::Chip3::Chip3(CC86Box* pC86Box, UINT nDevId, ChipType nChipType)
	: m_pC86Box(pC86Box)
	, m_nDevId(nDevId)
	, m_nChipType(nChipType)
{
}

/**
 * Get the instance of the device
 * @return The instance
 */
inline CC86Box* CC86Box::Chip3::GetDevice()
{
	return m_pC86Box;
}


/**
 * Increments the reference count
 * @return The new reference count
 */
size_t CC86Box::Chip3::AddRef()
{
	return GetDevice()->AddRef();
}

/**
 * Decrements the reference count
 * @return The new reference count
 */
size_t CC86Box::Chip3::Release()
{
	return GetDevice()->Release();
}

/**
 * Reset
 * @return C86CTL_ERR
 */
C86CtlErr CC86Box::Chip3::reset()
{
	memset(m_sReg, 0, sizeof(m_sReg));
	return GetDevice()->Reset();
}

/**
 * Output
 * @param[in] nAddr The address of registers
 * @param[in] cData The data
 */
void CC86Box::Chip3::out(UINT nAddr, UINT8 cData)
{
	if (nAddr >= sizeof(m_sReg))
	{
		return;
	}
	m_sReg[nAddr] = cData;

	GetDevice()->Out(m_nDevId, nAddr, cData);
}

/**
 * Input
 * @param[in] nAddr The address of registers
 * @return The data
 */
UINT8 CC86Box::Chip3::in(UINT nAddr)
{
	if (nAddr < sizeof(m_sReg))
	{
		return m_sReg[nAddr];
	}
	return 0xff;
}

/**
 * Gets the current status
 * @param[in] nAddr The address
 * @param[out] pcStatus The status
 * @return C86CTL_ERR
 */
C86CtlErr CC86Box::Chip3::getChipStatus(UINT nAddr, UINT8* pcStatus)
{
	return C86CTL_ERR_NOT_IMPLEMENTED;
}

/**
 * Output
 * @param[in] nAddr The address
 * @param[in] cData The data
 */
void CC86Box::Chip3::directOut(UINT nAddr, UINT8 cData)
{
	if (nAddr >= sizeof(m_sReg))
	{
		return;
	}
	m_sReg[nAddr] = cData;

	const UINT data = (m_nDevId << 17) | ((nAddr & 0x1ff) << 8) | cData;

	UINT8 sData[4];
	sData[0] = static_cast<UINT8>(data >> 0);
	sData[1] = static_cast<UINT8>(data >> 8);
	sData[2] = static_cast<UINT8>(data >> 16);
	sData[3] = static_cast<UINT8>(data >> 24);
	GetDevice()->Transaction(sData, sizeof(sData));
}

/**
 * Gets the type of the chip
 * @param[out] pnType A pointer of type
 * @return C86CTL_ERR
 */
C86CtlErr CC86Box::Chip3::getChipType(ChipType* pnType)
{
	if (pnType != NULL)
	{
		*pnType = m_nChipType;
	}
	return C86CTL_ERR_NONE;
}

}	// namespace c86ctl
