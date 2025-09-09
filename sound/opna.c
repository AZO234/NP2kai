/**
 * @file	opna.c
 * @brief	Implementation of OPNA
 */
#if !defined(NP2_X) && !defined(NP2_SDL) && !defined(__LIBRETRO__)
#define bool _Bool
#define false FALSE
#endif

#include <compiler.h>
#include "opna.h"
#include <pccore.h>
#include <io/iocore.h>
#include <sound/fmboard.h>
#include <sound/sound.h>
#include <sound/s98.h>
#include <generic/keydisp.h>
#if defined(SUPPORT_FMGEN)
#include <math.h>
#include <sound/fmgen/fmgen_fmgwrap.h>
#endif	/* SUPPORT_FMGEN */

static void writeRegister(POPNA opna, UINT nAddress, REG8 cData);
static void writeExtendedRegister(POPNA opna, UINT nAddress, REG8 cData);

#if defined(SUPPORT_FMGEN)
// dB = 20 log10( (音量0〜1) * (pow(10, 最大dB値/20) - pow(10, 最小dB値/20)) + pow(10, 最小dB値/20) )
//#define LINEAR2DB(a)	(20 * log10((a) * (pow(10.0, 20/20) - pow(10.0, -192/20)) + pow(10.0, -192/20)))
#define LINEAR2DB(a)	(pow(a,0.12)*(20+192) - 192)	// XXX: fmgen音量と猫音源音量を一致させるための実験式･･･

// XXX: 音量調整を出来るようにするためにとりあえず･･･
POPNA opnalist[OPNA_MAX] = {0}; 
int opnalistconunt = 0;
void opnalist_push(POPNA opna)
{
	int i;
	for(i=0;i<opnalistconunt;i++){
		if(opnalist[i] == opna) return;
	}
	opnalist[opnalistconunt] = opna;
	opnalistconunt++;
}
void opnalist_remove(POPNA opna)
{
	int i;
	for(i=0;i<opnalistconunt;i++){
		if(opnalist[i] == opna) break;
	}
	if(i == opnalistconunt) return;
	opnalistconunt--;
	for(;i<opnalistconunt;i++){
		opnalist[i] = opnalist[i+1];
	}
}
void opnalist_clear()
{
	opnalistconunt = 0;
}
void opna_fmgen_setallvolumeFM_linear(int lvol)
{
	for(int i=0;i<opnalistconunt;i++){
		OPNA_SetVolumeFM(opnalist[i]->fmgen, (int)LINEAR2DB((double)lvol / 128));
	}
}
void opna_fmgen_setallvolumePSG_linear(int lvol)
{
	for(int i=0;i<opnalistconunt;i++){
		OPNA_SetVolumePSG(opnalist[i]->fmgen, (int)LINEAR2DB((double)lvol / 128));
	}
}
void opna_fmgen_setallvolumeADPCM_linear(int lvol)
{
	for(int i=0;i<opnalistconunt;i++){
		OPNA_SetVolumeADPCM(opnalist[i]->fmgen, (int)LINEAR2DB((double)lvol / 128));
	}
}
void opna_fmgen_setallvolumeRhythmTotal_linear(int lvol)
{
	for(int i=0;i<opnalistconunt;i++){
		OPNA_SetVolumeRhythmTotal(opnalist[i]->fmgen, (int)LINEAR2DB((double)lvol / 128));
	}
}
#endif	/* SUPPORT_FMGEN */

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
#if defined(SUPPORT_FMGEN)
	if(enable_fmgen) {
		if(opna->fmgen) OPNA_Destruct(opna->fmgen);
	}
#endif	/* SUPPORT_FMGEN */
}

/**
 * Reset
 * @param[in] opna The instance
 * @param[in] cCaps
 */
void opna_reset(POPNA opna, REG8 cCaps)
{
	UINT i;
#if defined(SUPPORT_FMGEN)
	UINT j;
	OEMCHAR	path[MAX_PATH];

	if(enable_fmgen) {
		if(!opna->fmgen) OPNA_Destruct(opna->fmgen);
		opna->fmgen = OPNA_Construct();
		OPNA_Init(opna->fmgen, OPNA_CLOCK*2, np2cfg.samplingrate, false, "");
		getbiospath(path, "", NELEMENTS(path));
		OPNA_LoadRhythmSample(opna->fmgen, path);
		OPNA_SetVolumeFM(opna->fmgen, pow((double)np2cfg.vol_fm / 128, 0.12) * (20 + 192) - 192);
		OPNA_SetVolumePSG(opna->fmgen, pow((double)np2cfg.vol_ssg / 128, 0.12) * (20 + 192) - 192);
		OPNA_SetVolumeADPCM(opna->fmgen, pow((double)np2cfg.vol_pcm / 128, 0.12) * (20 + 192) - 192);
		OPNA_SetVolumeRhythmTotal(opna->fmgen, pow((double)np2cfg.vol_rhythm / 128, 0.12) * (20 + 192) - 192);
	}
#endif	/* SUPPORT_FMGEN */

	memset(&opna->s, 0, sizeof(opna->s));
	opna->s.adpcmmask = ~(0x1c);
	opna->s.cCaps = cCaps;
	opna->s.irq = 0xff;
	opna->s.reg[0x07] = 0xbf;
	opna->s.reg[0x0e] = 0xff;
	opna->s.reg[0x0f] = 0xff;
	opna->s.reg[0xff] = 0x01;
	for (i = 0; i < 2; i++)
	{
		memset(opna->s.reg + (i * 0x100) + 0x30, 0xff, 0x60);
		memset(opna->s.reg + (i * 0x100) + 0xb4, 0xc0, 0x04);
	}
	for (i = 0; i < 7; i++)
	{
		opna->s.keyreg[i] = i & 7;
	}
#if defined(SUPPORT_FMGEN)
	if(enable_fmgen) {
		OPNA_Reset(opna->fmgen);
		OPNA_SetReg(opna->fmgen, 0x07, 0xbf);
		OPNA_SetReg(opna->fmgen, 0x0e, 0xff);
		OPNA_SetReg(opna->fmgen, 0x0f, 0xff);
		OPNA_SetReg(opna->fmgen, 0xff, 0x01);
		for (i = 0; i < 2; i++)
		{
			for (j = 0; j < 0x60; j++) {
				OPNA_SetReg(opna->fmgen, (i * 0x100) + 0x30 + j, 0xff);
			}
			for (j = 0; j < 0x04; j++) {
				OPNA_SetReg(opna->fmgen, (i * 0x100) + 0xb4 + j, 0xc0);
			}
		}
	}
#endif	/* SUPPORT_FMGEN */

	opngen_reset(&opna->opngen);
	psggen_reset(&opna->psg);
	rhythm_reset(&opna->rhythm);
	adpcm_reset(&opna->adpcm);

	
	// 音量初期化
	opngen_setvol(np2cfg.vol_fm);
	psggen_setvol(np2cfg.vol_ssg);
	rhythm_setvol(np2cfg.vol_rhythm);
#if defined(SUPPORT_FMGEN)
	if(np2cfg.usefmgen) {
		opna_fmgen_setallvolumeFM_linear(np2cfg.vol_fm);
		opna_fmgen_setallvolumePSG_linear(np2cfg.vol_ssg);
		opna_fmgen_setallvolumeRhythmTotal_linear(np2cfg.vol_rhythm);
	}
#endif	/* SUPPORT_FMGEN */
	for (UINT i = 0; i < NELEMENTS(g_opna); i++)
	{
		rhythm_update(&g_opna[i].rhythm);
	}
}

/**
 * Restore
 * @param[in] opna The instance
 */
static void restore(POPNA opna)
{
	UINT i;

	// FM
	writeRegister(opna, 0x22, opna->s.reg[0x22]);
	for (i = 0x30; i < 0xa0; i++)
	{
		if ((i & 3) == 3)
		{
			continue;
		}
		writeRegister(opna, i, opna->s.reg[i]);
		writeExtendedRegister(opna, i, opna->s.reg[i + 0x100]);
	}
	for (i = 0xb0; i < 0xb8; i++)
	{
		if ((i & 3) == 3)
		{
			continue;
		}
		writeRegister(opna, i, opna->s.reg[i]);
		writeExtendedRegister(opna, i, opna->s.reg[i + 0x100]);
	}
	for (i = 0; i < 8; i++)
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
	for (i = 0; i < 8; i++)
	{
		if ((i & 3) == 3)
		{
			continue;
		}
		writeRegister(opna, 0x28, opna->s.keyreg[i]);
	}
#if defined(SUPPORT_FMGEN)
	if(enable_fmgen) {
		OPNA_SetReg(opna->fmgen, 0x22, opna->s.reg[0x22]);
		for (i = 0x30; i < 0xa0; i++)
		{
			if ((i & 3) == 3)
			{
				continue;
			}
			OPNA_SetReg(opna->fmgen, i, opna->s.reg[i]);
			OPNA_SetReg(opna->fmgen, i + 0x100, opna->s.reg[i + 0x100]);
		}
		for (i = 0xb0; i < 0xb8; i++)
		{
			if ((i & 3) == 3)
			{
				continue;
			}
			OPNA_SetReg(opna->fmgen, i, opna->s.reg[i]);
			OPNA_SetReg(opna->fmgen, i + 0x100, opna->s.reg[i + 0x100]);
		}
		for (i = 0; i < 8; i++)
		{
			if ((i & 3) == 3)
			{
				continue;
			}
			OPNA_SetReg(opna->fmgen, i + 0xa4, opna->s.reg[i + 0xa4]);
			OPNA_SetReg(opna->fmgen, i + 0xa0, opna->s.reg[i + 0xa0]);
			OPNA_SetReg(opna->fmgen, i + 0x1a4, opna->s.reg[i + 0x1a4]);
			OPNA_SetReg(opna->fmgen, i + 0x1a0, opna->s.reg[i + 0x1a0]);
		}
		for (i = 0; i < 8; i++)
		{
			if ((i & 3) == 3)
			{
				continue;
			}
			OPNA_SetReg(opna->fmgen, 0x28, opna->s.keyreg[i]);
		}
	}
#endif	/* SUPPORT_FMGEN */

	// PSG
	for (i = 0; i < 0x10; i++)
	{
		writeRegister(opna, i, opna->s.reg[i]);
	}
#if defined(SUPPORT_FMGEN)
	if(enable_fmgen) {
		for (i = 0; i < 0x10; i++)
		{
			OPNA_SetReg(opna->fmgen, i, opna->s.reg[i]);
		}
	}
#endif	/* SUPPORT_FMGEN */

	// Rhythm
	writeRegister(opna, 0x11, opna->s.reg[0x11]);
	for (i = 0x18; i < 0x1e; i++)
	{
		writeRegister(opna, i, opna->s.reg[i]);
	}
#if defined(SUPPORT_FMGEN)
	if(enable_fmgen) {
		OPNA_SetReg(opna->fmgen, 0x11, opna->s.reg[0x11]);
		for (i = 0x18; i < 0x1e; i++)
		{
			OPNA_SetReg(opna->fmgen, i, opna->s.reg[i]);
		}
	}
#endif	/* SUPPORT_FMGEN */
}

/**
 * Bind
 * @param[in] opna The instance
 */
void opna_bind(POPNA opna)
{
	const UINT8 cCaps = opna->s.cCaps;

	keydisp_bindopna(opna->s.reg, (cCaps & OPNA_HAS_EXTENDEDFM) ? 6 : 3, 3993600);
	if (cCaps & OPNA_HAS_PSG)
	{
		keydisp_bindpsg(opna->s.reg, 3993600);
	}

	opna->opngen.opnch[2].extop = opna->s.reg[0x27] & 0xc0;
	restore(opna);

#if defined(SUPPORT_FMGEN)
	if(enable_fmgen) {
		sound_streamregist(opna->fmgen, (SOUNDCB)OPNA_Mix);
	} else {
#endif	/* SUPPORT_FMGEN */
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
#if defined(SUPPORT_FMGEN)
	}
#endif	/* SUPPORT_FMGEN */
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
#if defined(SUPPORT_FMGEN)
	if(enable_fmgen) {
		OPNA_SetReg(opna->fmgen, nAddress, cData);
	}
#endif	/* SUPPORT_FMGEN */
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
	REG8 cChannel;

	if (nAddress < 0x10)
	{
		if (cCaps & OPNA_HAS_PSG)
		{
			keydisp_psg(opna->s.reg, nAddress);
			psggen_setreg(&opna->psg, nAddress, cData);
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
			rhythm_setreg(&opna->rhythm, nAddress, cData);
		}
	}
	else if (nAddress < 0x30)
	{
		if (nAddress == 0x28)
		{
			cChannel = cData & 0x0f;
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

			opngen_keyon(&opna->opngen, cChannel, cData);
			keydisp_opnakeyon(opna->s.reg, cData);
		}
		else if (nAddress == 0x27)
		{
			if (cCaps & OPNA_HAS_TIMER)
			{
				opna_settimer(opna, cData);
			}
			opna->opngen.opnch[2].extop = cData & 0xc0;
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
		opngen_setreg(&opna->opngen, 0, nAddress, cData);
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
#if defined(SUPPORT_FMGEN)
	if(enable_fmgen) {
		OPNA_SetReg(opna->fmgen, nAddress + 0x100, cData);
	}
#endif	/* SUPPORT_FMGEN */
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
			opngen_setreg(&opna->opngen, 3, nAddress, cData);
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
#if defined(SUPPORT_FMGEN)
	ret |= statflag_write(sfh, &enable_fmgen, sizeof(enable_fmgen));
	if(enable_fmgen) {
		void* buf;

		buf = malloc(fmgen_opnadata_size);
		OPNA_DataSave(opna->fmgen, buf);
		ret |= statflag_write(sfh, buf, fmgen_opnadata_size);
		free(buf);
	}
#endif	/* SUPPORT_FMGEN */
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
#if defined(SUPPORT_FMGEN)
	if(statflag_read(sfh, &enable_fmgen, sizeof(enable_fmgen))==STATFLAG_SUCCESS){
		if(enable_fmgen) {
			OEMCHAR	path[MAX_PATH];
			void* buf;

			buf = malloc(fmgen_opnadata_size);
			ret |= statflag_read(sfh, buf, fmgen_opnadata_size);
			OPNA_DataLoad(opna->fmgen, buf);
			free(buf);
			getbiospath(path, "", NELEMENTS(path));
			OPNA_LoadRhythmSample(opna->fmgen, path);
		}
	}
#endif	/* SUPPORT_FMGEN */
	if (opna->s.cCaps & OPNA_HAS_ADPCM)
	{
		ret |= statflag_read(sfh, &opna->adpcm, sizeof(opna->adpcm));
		adpcm_update(&opna->adpcm);
	}

	return ret;
}
