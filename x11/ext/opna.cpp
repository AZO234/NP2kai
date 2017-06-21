/**
 * @file	opna.cpp
 * @brief	Implementation of OPNA
 */

#include "compiler.h"
#include "sound/opna.h"
#include "pccore.h"
#include "iocore.h"
#include "sound/fmboard.h"
#include "sound/sound.h"
#include "sound/s98.h"
#include "generic/keydisp.h"
#include "externalchipmanager.h"
#include "externalopna.h"

static void writeRegister(POPNA opna, UINT nAddress, REG8 cData);
static void writeExtendedRegister(POPNA opna, UINT nAddress, REG8 cData);

/**
 * Initialize instance
 * @param[in] opna The instance
 */
void opna_construct(POPNA opna)
{
	memset(opna, 0, sizeof(*opna));
}

/**
 * Deinitialize instance
 * @param[in] opna The instance
 */
void opna_destruct(POPNA opna)
{
	CExternalOpna* pExt = reinterpret_cast<CExternalOpna*>(opna->userdata);
	CExternalChipManager::GetInstance()->Release(pExt);
	opna->userdata = 0;
}

/**
 * Reset
 * @param[in] opna The instance
 * @param[in] cCaps
 */
void opna_reset(POPNA opna, REG8 cCaps)
{
	memset(&opna->s, 0, sizeof(opna->s));
	opna->s.adpcmmask = ~(0x1c);
	opna->s.cCaps = cCaps;
	opna->s.irq = 0xff;
	opna->s.reg[0x07] = 0xbf;
	opna->s.reg[0x0e] = 0xff;
	opna->s.reg[0x0f] = 0xff;
	opna->s.reg[0xff] = (cCaps & OPNA_HAS_RHYTHM) ? 0x01 : 0x00;
	for (UINT i = 0; i < 2; i++)
	{
		memset(opna->s.reg + (i * 0x100) + 0x30, 0xff, 0x60);
		memset(opna->s.reg + (i * 0x100) + 0xb4, 0xc0, 0x04);
	}
	for (UINT i = 0; i < 7; i++)
	{
		opna->s.keyreg[i] = i & 7;
	}

	opngen_reset(&opna->opngen);
	psggen_reset(&opna->psg);
	rhythm_reset(&opna->rhythm);
	adpcm_reset(&opna->adpcm);

	if (cCaps == 0)
	{
		CExternalOpna* pExt = reinterpret_cast<CExternalOpna*>(opna->userdata);
		if (pExt)
		{
			CExternalChipManager::GetInstance()->Release(pExt);
			opna->userdata = 0;
		}
	}
}

/**
 * Restore
 * @param[in] opna The instance
 */
static void restore(POPNA opna)
{
	// FM
	writeRegister(opna, 0x22, opna->s.reg[0x22]);
	for (UINT i = 0x30; i < 0xa0; i++)
	{
		if ((i & 3) == 3)
		{
			continue;
		}
		writeRegister(opna, i, opna->s.reg[i]);
		writeExtendedRegister(opna, i, opna->s.reg[i + 0x100]);
	}
	for (UINT i = 0xb0; i < 0xb8; i++)
	{
		if ((i & 3) == 3)
		{
			continue;
		}
		writeRegister(opna, i, opna->s.reg[i]);
		writeExtendedRegister(opna, i, opna->s.reg[i + 0x100]);
	}
	for (UINT i = 0; i < 8; i++)
	{
		if ((i & 3) == 3)
		{
			continue;
		}
		writeRegister(opna, i + 0xa4, opna->s.reg[i + 0xa4]);
		writeRegister(opna, i + 0xa0, opna->s.reg[i + 0xa0]);
		writeExtendedRegister(opna, i + 0xa4, opna->s.reg[i + 0x1a4]);
		writeExtendedRegister(opna, i + 0xa0, opna->s.reg[i + 0x1a0]);
	}
	for (UINT i = 0; i < 8; i++)
	{
		if ((i & 3) == 3)
		{
			continue;
		}
		writeRegister(opna, 0x28, opna->s.keyreg[i]);
	}

	// PSG
	for (UINT i = 0; i < 0x10; i++)
	{
		writeRegister(opna, i, opna->s.reg[i]);
	}

	// Rhythm
	writeRegister(opna, 0x11, opna->s.reg[0x11]);
	for (UINT i = 0x18; i < 0x1e; i++)
	{
		writeRegister(opna, i, opna->s.reg[i]);
	}
}

/**
 * Bind
 * @param[in] opna The instance
 */
void opna_bind(POPNA opna)
{
	UINT8 cCaps = opna->s.cCaps;
	UINT nClock = 3993600;

	keydisp_bindopna(opna->s.reg, (cCaps & OPNA_HAS_EXTENDEDFM) ? 6 : 3, nClock);
	if (cCaps & OPNA_HAS_PSG)
	{
		keydisp_bindpsg(opna->s.reg, nClock);
	}

	CExternalOpna* pExt = reinterpret_cast<CExternalOpna*>(opna->userdata);
	if (pExt == NULL)
	{
		IExternalChip::ChipType nChipType = IExternalChip::kYM2203;
		if (cCaps & OPNA_HAS_EXTENDEDFM)
		{
			nChipType = IExternalChip::kYMF288;
			nClock *= 2;
			if (cCaps & OPNA_HAS_ADPCM)
			{
				nChipType = IExternalChip::kYM2608;
			}
			else if (cCaps == OPNA_MODE_3438)
			{
				nChipType = IExternalChip::kYM3438;
			}
		}
		pExt = static_cast<CExternalOpna*>(CExternalChipManager::GetInstance()->GetInterface(nChipType, nClock));
		opna->userdata = reinterpret_cast<INTPTR>(pExt);
	}
	if (pExt)
	{
		pExt->Reset();
		pExt->WriteRegister(0x22, 0x00);
		pExt->WriteRegister(0x29, 0x80);
		pExt->WriteRegister(0x10, 0xbf);
		pExt->WriteRegister(0x11, 0x30);
		pExt->WriteRegister(0x27, opna->s.reg[0x27]);
	}
	else
	{
		opna->opngen.opnch[2].extop = opna->s.reg[0x27] & 0xc0;
	}
	restore(opna);

	if (pExt)
	{
		if ((cCaps & OPNA_HAS_PSG) && (pExt->HasPsg()))
		{
			cCaps &= ~OPNA_HAS_PSG;
		}
		if ((cCaps & OPNA_HAS_RHYTHM) && (pExt->HasRhythm()))
		{
			cCaps &= ~OPNA_HAS_RHYTHM;
		}
		if ((cCaps & OPNA_HAS_ADPCM) && (pExt->HasADPCM()))
		{
			sound_streamregist(&opna->adpcm, (SOUNDCB)adpcm_getpcm_dummy);
			cCaps &= ~OPNA_HAS_ADPCM;
		}
	}

	if (cCaps & OPNA_HAS_PSG)
	{
		sound_streamregist(&opna->psg, (SOUNDCB)psggen_getpcm);
	}
	if (cCaps & OPNA_HAS_VR)
	{
		sound_streamregist(&opna->opngen, (SOUNDCB)opngen_getpcmvr);
	}
	else
	{
		sound_streamregist(&opna->opngen, (SOUNDCB)opngen_getpcm);
	}
	if (cCaps & OPNA_HAS_RHYTHM)
	{
		rhythm_bind(&opna->rhythm);
	}
	if (cCaps & OPNA_HAS_ADPCM)
	{
		sound_streamregist(&opna->adpcm, (SOUNDCB)adpcm_getpcm);
	}
}

/**
 * Status
 * @param[in] opna The instance
 * @return Status
 */
REG8 opna_readStatus(POPNA opna)
{
	if (opna->s.cCaps & OPNA_HAS_TIMER)
	{
		return opna->s.status;
	}
	return 0;
}

/**
 * Status
 * @param[in] opna The instance
 * @return Status
 */
REG8 opna_readExtendedStatus(POPNA opna)
{
	const UINT8 cCaps = opna->s.cCaps;
	REG8 ret = 0;

	if (cCaps & OPNA_HAS_ADPCM)
	{
		ret = adpcm_status(&opna->adpcm);
	}
	else
	{
		ret = opna->s.adpcmmask & 8;
	}

	if (cCaps & OPNA_HAS_TIMER)
	{
		ret |= opna->s.status;
	}

	return ret;
}

/**
 * Writes register
 * @param[in] opna The instance
 * @param[in] nAddress The address
 * @param[in] cData The data
 */
void opna_writeRegister(POPNA opna, UINT nAddress, REG8 cData)
{
	opna->s.reg[nAddress] = cData;

	if (opna->s.cCaps & OPNA_S98)
	{
		S98_put(NORMAL2608, nAddress, cData);
	}

	writeRegister(opna, nAddress, cData);
}

/**
 * Writes register (Inner)
 * @param[in] opna The instance
 * @param[in] nAddress The address
 * @param[in] cData The data
 */
static void writeRegister(POPNA opna, UINT nAddress, REG8 cData)
{
	const UINT8 cCaps = opna->s.cCaps;
	CExternalOpna* pExt = reinterpret_cast<CExternalOpna*>(opna->userdata);

	if (nAddress < 0x10)
	{
		if (cCaps & OPNA_HAS_PSG)
		{
			keydisp_psg(opna->s.reg, nAddress);
			if ((!pExt) || (!pExt->HasPsg()))
			{
				psggen_setreg(&opna->psg, nAddress, cData);
			}
			else
			{
				pExt->WriteRegister(nAddress, cData);
			}
		}
	}
	else if (nAddress < 0x20)
	{
		if (cCaps & OPNA_HAS_RHYTHM)
		{
			if ((cCaps & OPNA_HAS_VR) && (nAddress >= 0x18) && (nAddress <= 0x1d))
			{
				switch (cData & 0xc0)
				{
					case 0x40:
					case 0x80:
						cData ^= 0xc0;
						break;
				}
			}
			if ((!pExt) || (!pExt->HasRhythm()))
			{
				rhythm_setreg(&opna->rhythm, nAddress, cData);
			}
			else
			{
				pExt->WriteRegister(nAddress, cData);
			}
		}
	}
	else if (nAddress < 0x30)
	{
		if (nAddress == 0x28)
		{
			REG8 cChannel = cData & 0x0f;
			if (cChannel < 8)
			{
				opna->s.keyreg[cChannel] = cData;
			}
			if (cChannel < 3)
			{
			}
			else if ((cCaps & OPNA_HAS_EXTENDEDFM) && (cChannel >= 4) && (cChannel < 7))
			{
				cChannel--;
			}
			else
			{
				return;
			}

			if (!pExt)
			{
				opngen_keyon(&opna->opngen, cChannel, cData);
			}
			else
			{
				pExt->WriteRegister(nAddress, cData);
			}
			keydisp_opnakeyon(opna->s.reg, cData);
		}
		else if (nAddress == 0x27)
		{
			if (cCaps & OPNA_HAS_TIMER)
			{
				opna_settimer(opna, cData);
			}

			if (pExt)
			{
				pExt->WriteRegister(nAddress, cData);
			}
			else
			{
				opna->opngen.opnch[2].extop = cData & 0xc0;
			}
		}
		else if (nAddress == 0x22)
		{
			if (pExt)
			{
				pExt->WriteRegister(nAddress, cData);
			}
		}
	}
	else if (nAddress < 0xc0)
	{
		if ((cCaps & OPNA_HAS_VR) && ((nAddress & 0xfc) == 0xb4))
		{
			switch (cData & 0xc0)
			{
				case 0x40:
				case 0x80:
					cData ^= 0xc0;
					break;
			}
		}
		if (!pExt)
		{
			opngen_setreg(&opna->opngen, 0, nAddress, cData);
		}
		else
		{
			pExt->WriteRegister(nAddress, cData);
		}
	}
}

/**
 * Writes extended register
 * @param[in] opna The instance
 * @param[in] nAddress The address
 * @param[in] cData The data
 */
void opna_writeExtendedRegister(POPNA opna, UINT nAddress, REG8 cData)
{
	opna->s.reg[nAddress + 0x100] = cData;

	if (opna->s.cCaps & OPNA_S98)
	{
		S98_put(EXTEND2608, nAddress, cData);
	}

	writeExtendedRegister(opna, nAddress, cData);
}

/**
 * Writes extended register (Inner)
 * @param[in] opna The instance
 * @param[in] nAddress The address
 * @param[in] cData The data
 */
static void writeExtendedRegister(POPNA opna, UINT nAddress, REG8 cData)
{
	const UINT8 cCaps = opna->s.cCaps;
	CExternalOpna* pExt = reinterpret_cast<CExternalOpna*>(opna->userdata);

	if (nAddress < 0x12)
	{
		if (cCaps & OPNA_HAS_ADPCM)
		{
			if ((cCaps & OPNA_HAS_VR) && (nAddress == 0x01))
			{
				switch (cData & 0xc0)
				{
					case 0x40:
					case 0x80:
						cData ^= 0xc0;
						break;
				}
			}
			adpcm_setreg(&opna->adpcm, nAddress, cData);
			if ((pExt) && (pExt->HasADPCM()))
			{
				pExt->WriteRegister(nAddress + 0x100, cData);
			}
		}
		else
		{
			if (nAddress == 0x10)
			{
				if (!(cData & 0x80))
				{
					opna->s.adpcmmask = ~(cData & 0x1c);
				}
			}
		}
	}
	else if (nAddress >= 0x30)
	{
		if (cCaps & OPNA_HAS_EXTENDEDFM)
		{
			if ((cCaps & OPNA_HAS_VR) && ((nAddress & 0xfc) == 0xb4))
			{
				switch (cData & 0xc0)
				{
					case 0x40:
					case 0x80:
						cData ^= 0xc0;
						break;
				}
			}
			if (!pExt)
			{
				opngen_setreg(&opna->opngen, 3, nAddress, cData);
			}
			else
			{
				pExt->WriteRegister(nAddress + 0x100, cData);
			}
		}
	}
}

/**
 * Reads register
 * @param[in] opna The instance
 * @param[in] nAddress The address
 * @return data
 */
REG8 opna_readRegister(POPNA opna, UINT nAddress)
{
	if (nAddress < 0x10)
	{
		if (!(opna->s.cCaps & OPNA_HAS_PSG))
		{
			return 0xff;
		}
	}
	else if (nAddress < 0x20)
	{
		if (!(opna->s.cCaps & OPNA_HAS_RHYTHM))
		{
			return 0xff;
		}
	}
	else if (nAddress == 0xff)
	{
		return (opna->s.cCaps & OPNA_HAS_RHYTHM) ? 1 : 0;
	}
	return opna->s.reg[nAddress];
}

/**
 * Reads extended register
 * @param[in] opna The instance
 * @param[in] nAddress The address
 * @return data
 */
REG8 opna_readExtendedRegister(POPNA opna, UINT nAddress)
{
	if ((opna->s.cCaps & OPNA_HAS_ADPCM) && (nAddress == 0x08))
	{
		return adpcm_readsample(&opna->adpcm);
	}
	return opna->s.reg[nAddress + 0x100];
}

/**
 * Reads 3438 extended register
 * @param[in] opna The instance
 * @param[in] nAddress The address
 * @return data
 */
REG8 opna_read3438ExtRegister(POPNA opna, UINT nAddress)
{
	return opna->s.reg[nAddress];
}



// ---- statsave

/**
 * Save
 * @param[in] opna The instance
 * @param[in] sfh The handle of statsave
 * @param[in] tbl The item of statsave
 * @return Error
 */
int opna_sfsave(PCOPNA opna, STFLAGH sfh, const SFENTRY *tbl)
{
	int ret = statflag_write(sfh, &opna->s, sizeof(opna->s));
	if (opna->s.cCaps & OPNA_HAS_ADPCM)
	{
		ret |= statflag_write(sfh, &opna->adpcm, sizeof(opna->adpcm));
	}

	return ret;
}

/**
 * Load
 * @param[in] opna The instance
 * @param[in] sfh The handle of statsave
 * @param[in] tbl The item of statsave
 * @return Error
 */
int opna_sfload(POPNA opna, STFLAGH sfh, const SFENTRY *tbl)
{
	int ret = statflag_read(sfh, &opna->s, sizeof(opna->s));
	if (opna->s.cCaps & OPNA_HAS_ADPCM)
	{
		ret |= statflag_read(sfh, &opna->adpcm, sizeof(opna->adpcm));
		adpcm_update(&opna->adpcm);
	}

	return ret;
}
