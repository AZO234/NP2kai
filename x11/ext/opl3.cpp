/**
 * @file	opl3.cpp
 * @brief	Implementation of OPL3
 */

#include "compiler.h"
#include "sound/opl3.h"
#include "pccore.h"
#include "iocore.h"
#include "sound/fmboard.h"
#include "generic/keydisp.h"
#include "externalchipmanager.h"
#include "externalopl3.h"

static void writeRegister(POPL3 opl3, UINT nAddress, REG8 cData);
static void writeExtendedRegister(POPL3 opl3, UINT nAddress, REG8 cData);

/**
 * Initialize instance
 * @param[in] opl3 The instance
 */
void opl3_construct(POPL3 opl3)
{
	memset(opl3, 0, sizeof(*opl3));
}

/**
 * Deinitialize instance
 * @param[in] opl3 The instance
 */
void opl3_destruct(POPL3 opl3)
{
	CExternalOpl3* pExt = reinterpret_cast<CExternalOpl3*>(opl3->userdata);
	CExternalChipManager::GetInstance()->Release(pExt);
	opl3->userdata = 0;
}

/**
 * Reset
 * @param[in] opl3 The instance
 * @param[in] cCaps
 */
void opl3_reset(POPL3 opl3, REG8 cCaps)
{
	memset(&opl3->s, 0, sizeof(opl3->s));
	opl3->s.cCaps = cCaps;
	for (UINT i = 0; i < 2; i++)
	{
		memset(opl3->s.reg + (i * 0x100) + 0x20, 0xff, 0x80);
	}

	if (cCaps == 0)
	{
		CExternalOpl3* pExt = reinterpret_cast<CExternalOpl3*>(opl3->userdata);
		if (pExt)
		{
			CExternalChipManager::GetInstance()->Release(pExt);
			opl3->userdata = 0;
		}
	}
}

/**
 * Restore
 * @param[in] opl3 The instance
 */
static void restore(POPL3 opl3)
{
	writeExtendedRegister(opl3, 0x05, opl3->s.reg[0x105]);
	writeExtendedRegister(opl3, 0x04, opl3->s.reg[0x104]);
	writeExtendedRegister(opl3, 0x08, opl3->s.reg[0x108]);

	for (UINT i = 0x20; i < 0x100; i++)
	{
		if (((i & 0xe0) == 0xa0) || ((i & 0xe0) == 0xc0))
		{
			continue;
		}
		if (((i & 0x1f) >= 0x18) || ((i & 0x07) >= 0x06))
		{
			continue;
		}
		writeRegister(opl3, i, opl3->s.reg[i]);
		writeExtendedRegister(opl3, i, opl3->s.reg[i + 0x100]);
	}
	for (UINT i = 0xa0; i < 0xa9; i++)
	{
		writeRegister(opl3, i, opl3->s.reg[i]);
		writeRegister(opl3, i + 0x10, opl3->s.reg[i + 0x10] & 0xdf);
		writeRegister(opl3, i + 0x20, opl3->s.reg[i + 0x20]);
		writeExtendedRegister(opl3, i, opl3->s.reg[i + 0x100]);
		writeExtendedRegister(opl3, i + 0x10, opl3->s.reg[i + 0x110] & 0xdf);
		writeExtendedRegister(opl3, i + 0x20, opl3->s.reg[i + 0x120]);
	}
	writeExtendedRegister(opl3, 0xbd, opl3->s.reg[0xbd]);
}

/**
 * Bind
 * @param[in] opl3 The instance
 */
void opl3_bind(POPL3 opl3)
{
	UINT nBaseClock = 3579545;
	UINT8 cCaps = opl3->s.cCaps;

	nBaseClock = (cCaps & OPL3_HAS_OPL3) ? 3579545 : 3993600;

	CExternalOpl3* pExt = reinterpret_cast<CExternalOpl3*>(opl3->userdata);
	if (pExt == NULL)
	{
		IExternalChip::ChipType nChipType = IExternalChip::kNone;
		UINT nClock = nBaseClock;
		if (cCaps & OPL3_HAS_OPL3)
		{
			nChipType = IExternalChip::kYMF262;
			nClock *= 4;
		}
		else if (cCaps & OPL3_HAS_OPL2)
		{
			nChipType = IExternalChip::kYM3812;
		}
		else
		{
			nChipType = IExternalChip::kY8950;
		}
		pExt = static_cast<CExternalOpl3*>(CExternalChipManager::GetInstance()->GetInterface(nChipType, nClock));
		opl3->userdata = reinterpret_cast<INTPTR>(pExt);
	}
	if (pExt)
	{
		pExt->Reset();
	}
	else
	{
		oplgen_reset(&opl3->oplgen, nBaseClock);
	}
	restore(opl3);

	if (pExt == NULL)
	{
		sound_streamregist(&opl3->oplgen, (SOUNDCB)oplgen_getpcm);
	}

	keydisp_bindopl3(opl3->s.reg, (cCaps & OPL3_HAS_OPL3) ? 18 : 9, nBaseClock);
}

/**
 * Status
 * @param[in] opl3 The instance
 * @return Status
 */
REG8 opl3_readStatus(POPL3 opl3)
{
	return 0;
}

/**
 * Writes register
 * @param[in] opl3 The instance
 * @param[in] nAddress The address
 * @param[in] cData The data
 */
void opl3_writeRegister(POPL3 opl3, UINT nAddress, REG8 cData)
{
	opl3->s.reg[nAddress] = cData;
	writeRegister(opl3, nAddress, cData);
}

/**
 * Writes register (Inner)
 * @param[in] opl3 The instance
 * @param[in] nAddress The address
 * @param[in] cData The data
 */
static void writeRegister(POPL3 opl3, UINT nAddress, REG8 cData)
{
	const UINT8 cCaps = opl3->s.cCaps;

	switch (nAddress & 0xe0)
	{
		case 0x20:
		case 0x40:
		case 0x60:
		case 0x80:
			if (((nAddress & 0x1f) >= 0x18) || ((nAddress & 7) >= 6))
			{
				return;
			}
			break;

		case 0xa0:
			if (nAddress == 0xbd)
			{
				break;
			}
			if ((nAddress & 0x0f) >= 9)
			{
				return;
			}
			if (nAddress & 0x10)
			{
				keydisp_opl3keyon(opl3->s.reg, nAddress & 0x0f, cData);
			}
			break;

		case 0xc0:
			if ((nAddress & 0x1f) >= 9)
			{
				return;
			}
			if (!(cCaps & OPL3_HAS_OPL3))
			{
				cData |= 0x30;
			}
			break;

		case 0xe0:
			if (!(cCaps & OPL3_HAS_OPL2))
			{
				return;
			}
			if (((nAddress & 0x1f) >= 0x18) || ((nAddress & 7) >= 6))
			{
				return;
			}
			break;

		default:
			return;
	}

	CExternalOpl3* pExt = reinterpret_cast<CExternalOpl3*>(opl3->userdata);
	if (pExt)
	{
		pExt->WriteRegister(nAddress, cData);
	}
	else
	{
		sound_sync();
		oplgen_setreg(&opl3->oplgen, nAddress, cData);
	}
}

/**
 * Writes extended register
 * @param[in] opl3 The instance
 * @param[in] nAddress The address
 * @param[in] cData The data
 */
void opl3_writeExtendedRegister(POPL3 opl3, UINT nAddress, REG8 cData)
{
	opl3->s.reg[nAddress + 0x100] = cData;
	writeExtendedRegister(opl3, nAddress, cData);
}

/**
 * Writes extended register (Inner)
 * @param[in] opl3 The instance
 * @param[in] nAddress The address
 * @param[in] cData The data
 */
static void writeExtendedRegister(POPL3 opl3, UINT nAddress, REG8 cData)
{
	const UINT8 cCaps = opl3->s.cCaps;

	if (!(cCaps & OPL3_HAS_OPL3))
	{
		return;
	}

	switch (nAddress & 0xe0)
	{
		case 0x20:
		case 0x40:
		case 0x60:
		case 0x80:
		case 0xe0:
			if (((nAddress & 0x1f) >= 0x18) || ((nAddress & 7) >= 6))
			{
				return;
			}
			break;

		case 0xa0:
			if ((nAddress & 0x0f) >= 9)
			{
				return;
			}
			if (nAddress & 0x10)
			{
				keydisp_opl3keyon(opl3->s.reg, (nAddress & 0x0f) + 9, cData);
			}
			break;

		case 0xc0:
			if ((nAddress & 0x1f) >= 9)
			{
				return;
			}
			break;

		default:
			if ((nAddress == 0x04) || (nAddress == 0x05) || (nAddress == 0x08))
			{
				break;
			}
			return;
	}

	CExternalOpl3* pExt = reinterpret_cast<CExternalOpl3*>(opl3->userdata);
	if (pExt)
	{
		pExt->WriteRegister(nAddress + 0x100, cData);
	}
#if 0
	else
	{
		sound_sync();
		oplgen_setreg(&opl3->oplgen, nAddress + 0x100, cData);
	}
#endif
}

/**
 * Reads register
 * @param[in] opl3 The instance
 * @param[in] nAddress The address
 * @return data
 */
REG8 opl3_readRegister(POPL3 opl3, UINT nAddress)
{
	return 0xff;
}

/**
 * Reads extended register
 * @param[in] opl3 The instance
 * @param[in] nAddress The address
 * @return data
 */
REG8 opl3_readExtendedRegister(POPL3 opl3, UINT nAddress)
{
	return 0xff;
}



// ---- statsave

/**
 * Save
 * @param[in] opl3 The instance
 * @param[in] sfh The handle of statsave
 * @param[in] tbl The item of statsave
 * @return Error
 */
int opl3_sfsave(PCOPL3 opl3, STFLAGH sfh, const SFENTRY *tbl)
{
	int ret = statflag_write(sfh, &opl3->s, sizeof(opl3->s));
	if (opl3->s.cCaps & OPL3_HAS_8950ADPCM)
	{
	}
	return ret;
}

/**
 * Load
 * @param[in] opl3 The instance
 * @param[in] sfh The handle of statsave
 * @param[in] tbl The item of statsave
 * @return Error
 */
int opl3_sfload(POPL3 opl3, STFLAGH sfh, const SFENTRY *tbl)
{
	int ret = statflag_read(sfh, &opl3->s, sizeof(opl3->s));
	if (opl3->s.cCaps & OPL3_HAS_8950ADPCM)
	{
	}
	return ret;
}
