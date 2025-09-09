/**
 * @file	opngenc.c
 * @brief	Implementation of the OPN generator
 */

#include <compiler.h>
#include <sound/opngen.h>
#include <math.h>
#include "opngencfg.h"
#include <pccore.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846264338327
#endif

#define	OPM_ARRATE		 399128L
#define	OPM_DRRATE		5514396L

#define	EG_STEP			(96.0 / EVC_ENT)					/* dB step */
#define	SC(db)			(SINT32)((db) * ((3.0 / EG_STEP) * (1 << ENV_BITS))) + EC_DECAY


	OPNCFG	opncfg;
#ifdef OPNGENX86
	char	envshift[EVC_ENT];
	char	sinshift[SIN_ENT];
#endif


static	SINT32	detunetable[8][32];
static	SINT32	attacktable[94];
static	SINT32	decaytable[94];

static const SINT32	decayleveltable[16] = {
		 			SC( 0),SC( 1),SC( 2),SC( 3),SC( 4),SC( 5),SC( 6),SC( 7),
		 			SC( 8),SC( 9),SC(10),SC(11),SC(12),SC(13),SC(14),SC(31)};
static const UINT8 multipletable[] = {
			    	1, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30};
static const SINT32 nulltable[] = {
					0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
					0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static const UINT8 kftable[16] = {0,0,0,0,0,0,0,1,2,3,3,3,3,3,3,3};
static const UINT8 dttable[] = {
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2,
					2, 3, 3, 3, 4, 4, 4, 5, 5, 6, 6, 7, 8, 8, 8, 8,
					1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5,
					5, 6, 6, 7, 8, 8, 9,10,11,12,13,14,16,16,16,16,
					2, 2, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 6, 6, 7,
					8, 8, 9,10,11,12,13,14,16,17,19,20,22,22,22,22};
static const int extendslot[4] = {2, 3, 1, 0};
static const int fmslot[4] = {0, 2, 1, 3};


void opngen_initialize(UINT rate)
{
	UINT	ratebit;
	int		i;
	char	sft;
	int		j;
	double	pom;
	SINT32	detune;
	double	freq;

	if (rate > (OPNA_CLOCK / 144.0))
	{
		ratebit = 0;
	}
	else if (rate > (OPNA_CLOCK / 288.0))
	{
		ratebit = 1;
	}
	else
	{
		ratebit = 2;
	}
	opncfg.calc1024 = (SINT32)((FMDIV_ENT * (rate << ratebit) / (OPNA_CLOCK / 72.0)) + 0.5);

	for (i = 0; i < EVC_ENT; i++)
	{
#ifdef OPNGENX86
		sft = ENVTBL_BIT;
		while (sft < (ENVTBL_BIT + 8))
		{
			pom = (double)(1 << sft) / pow(10.0, EG_STEP * (EVC_ENT - i) / 20.0);
			opncfg.envtable[i] = (SINT32)pom;
			envshift[i] = sft - TL_BITS;
			if (opncfg.envtable[i] >= (1 << (ENVTBL_BIT - 1)))
			{
				break;
			}
			sft++;
		}
#else
		pom = (double)(1 << ENVTBL_BIT) / pow(10.0, EG_STEP * (EVC_ENT - i) / 20.0);
		opncfg.envtable[i] = (SINT32)pom;
#endif
	}
	for (i = 0; i < SIN_ENT; i++)
	{
#ifdef OPNGENX86
		sft = SINTBL_BIT;
		while (sft < (SINTBL_BIT + 8))
		{
			pom = (double)((1 << sft) - 1) * sin(2 * M_PI * i / SIN_ENT);
			opncfg.sintable[i] = (SINT32)pom;
			sinshift[i] = sft;
			if (opncfg.sintable[i] >= (1 << (SINTBL_BIT - 1)))
			{
				break;
			}
			if (opncfg.sintable[i] <= -1 * (1 << (SINTBL_BIT - 1)))
			{
				break;
			}
			sft++;
		}
#else
		pom = (double)((1 << SINTBL_BIT) - 1) * sin(2 * M_PI * i / SIN_ENT);
		opncfg.sintable[i] = (SINT32)pom;
#endif
	}
	for (i = 0; i < EVC_ENT; i++)
	{
		pom = pow(((double)(EVC_ENT - 1 - i) / EVC_ENT), 8) * EVC_ENT;
		opncfg.envcurve[i] = (SINT32)pom;
		opncfg.envcurve[EVC_ENT + i] = i;
	}
	opncfg.envcurve[EVC_ENT * 2] = EVC_ENT;

	opncfg.ratebit = ratebit;

	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 32; j++)
		{
			detune = dttable[i*32 + j];
			sft = ratebit + (FREQ_BITS - 20);
			if (sft >= 0)
			{
				detune <<= sft;
			}
			else
			{
				detune >>= (0 - sft);
			}

			detunetable[i + 0][j] = detune;
			detunetable[i + 4][j] = -detune;
		}
	}
	for (i = 0; i < 4; i++)
	{
		attacktable[i] = decaytable[i] = 0;
	}
	for (i = 4; i < 64; i++)
	{
		freq = (EVC_ENT << (ENV_BITS + ratebit)) * 72.0 / 64.0;
		if (i < 60)
		{
			freq *= 1.0 + (i & 3) * 0.25;
		}
		freq *= (double)(1 << ((i >> 2) - 1));
		attacktable[i] = (SINT32)(freq / OPM_ARRATE);
		decaytable[i] = (SINT32)(freq / OPM_DRRATE);
		if (attacktable[i] >= EC_DECAY)
		{
			TRACEOUT(("attacktable %d %d %ld", i, attacktable[i], EC_DECAY));
		}
		if (decaytable[i] >= EC_DECAY)
		{
			TRACEOUT(("decaytable %d %d %ld", i, decaytable[i], EC_DECAY));
		}
	}
	attacktable[62] = EC_DECAY - 1;
	attacktable[63] = EC_DECAY - 1;
	for (i = 64; i < 94; i++)
	{
		attacktable[i] = attacktable[63];
		decaytable[i] = decaytable[63];
	}
}

void opngen_setvol(UINT vol)
{
	opncfg.fmvol = vol * 5 / 4;
#if defined(OPNGENX86)
	opncfg.fmvol <<= (32 - (OPM_OUTSB + 1 - FMVOL_SFTBIT + FMDIV_BITS + 6));
#endif
}

void opngen_setVR(REG8 channel, REG8 value)
{
	if ((channel & 3) && (value))
	{
		opncfg.vr_en = TRUE;
		opncfg.vr_l = (channel & 1) ? value : 0;
		opncfg.vr_r = (channel & 2) ? value : 0;
	}
	else
	{
		opncfg.vr_en = FALSE;
	}
}


/* ---- */

static void set_algorithm(OPNGEN opngen, OPNCH *ch)
{
	SINT32	*outd;
	UINT8	outslot;

	outd = &opngen->outdc;
	if (ch->stereo)
	{
		switch (ch->pan & 0xc0)
		{
			case 0x80:
				outd = &opngen->outdl;
				break;

			case 0x40:
				outd = &opngen->outdr;
				break;
		}
	}
	switch (ch->algorithm)
	{
		case 0:
			ch->connect1 = &opngen->feedback2;
			ch->connect2 = &opngen->feedback3;
			ch->connect3 = &opngen->feedback4;
			outslot = 0x08;
			break;

		case 1:
			ch->connect1 = &opngen->feedback3;
			ch->connect2 = &opngen->feedback3;
			ch->connect3 = &opngen->feedback4;
			outslot = 0x08;
			break;

		case 2:
			ch->connect1 = &opngen->feedback4;
			ch->connect2 = &opngen->feedback3;
			ch->connect3 = &opngen->feedback4;
			outslot = 0x08;
			break;

		case 3:
			ch->connect1 = &opngen->feedback2;
			ch->connect2 = &opngen->feedback4;
			ch->connect3 = &opngen->feedback4;
			outslot = 0x08;
			break;

		case 4:
			ch->connect1 = &opngen->feedback2;
			ch->connect2 = outd;
			ch->connect3 = &opngen->feedback4;
			outslot = 0x0a;
			break;

		case 5:
			ch->connect1 = 0;
			ch->connect2 = outd;
			ch->connect3 = outd;
			outslot = 0x0e;
			break;

		case 6:
			ch->connect1 = &opngen->feedback2;
			ch->connect2 = outd;
			ch->connect3 = outd;
			outslot = 0x0e;
			break;

		case 7:
		default:
			ch->connect1 = outd;
			ch->connect2 = outd;
			ch->connect3 = outd;
			outslot = 0x0f;
	}
	ch->connect4 = outd;
	ch->outslot = outslot;
}

static void set_dt1_mul(OPNSLOT *slot, REG8 value)
{
	slot->multiple = (SINT32)multipletable[value & 0x0f];
	slot->detune1 = detunetable[(value >> 4) & 7];
}

static void set_tl(OPNSLOT *slot, REG8 value)
{
#if (EVC_BITS >= 7)
	slot->totallevel = ((~value) & 0x007f) << (EVC_BITS - 7);
#else
	slot->totallevel = ((~value) & 0x007f) >> (7 - EVC_BITS);
#endif
}

static void set_ks_ar(OPNSLOT *slot, REG8 value)
{
	slot->keyscalerate = ((~value) >> 6) & 3;
	value &= 0x1f;
	slot->attack = (value) ? (attacktable + (value << 1)) : nulltable;
	slot->env_inc_attack = slot->attack[slot->envratio];
	if (slot->env_mode == EM_ATTACK)
	{
		slot->env_inc = slot->env_inc_attack;
	}
}

static void set_d1r(OPNSLOT *slot, REG8 value)
{
	value &= 0x1f;
	slot->decay1 = (value) ? (decaytable + (value << 1)) : nulltable;
	slot->env_inc_decay1 = slot->decay1[slot->envratio];
	if (slot->env_mode == EM_DECAY1)
	{
		slot->env_inc = slot->env_inc_decay1;
	}
}

static void set_dt2_d2r(OPNSLOT *slot, REG8 value)
{
	value &= 0x1f;
	slot->decay2 = (value) ? (decaytable + (value << 1)) : nulltable;
	if (slot->ssgeg1)
	{
		slot->env_inc_decay2 = 0;
	}
	else
	{
		slot->env_inc_decay2 = slot->decay2[slot->envratio];
	}
	if (slot->env_mode == EM_DECAY2)
	{
		slot->env_inc = slot->env_inc_decay2;
	}
}

static void set_d1l_rr(OPNSLOT *slot, REG8 value)
{
	slot->decaylevel = decayleveltable[(value >> 4)];
	slot->release = decaytable + ((value & 0x0f) << 2) + 2;
	slot->env_inc_release = slot->release[slot->envratio];
	if (slot->env_mode == EM_RELEASE)
	{
		slot->env_inc = slot->env_inc_release;
		if (value == 0xff)
		{
			slot->env_mode = EM_OFF;
			slot->env_cnt = EC_OFF;
			slot->env_end = EC_OFF + 1;
			slot->env_inc = 0;
		}
	}
}

static void set_ssgeg(OPNSLOT *slot, REG8 value)
{
	value &= 0xf;
	if ((value == 0xb) || (value == 0xd))
	{
		slot->ssgeg1 = 1;
		slot->env_inc_decay2 = 0;
	}
	else
	{
		slot->ssgeg1 = 0;
		slot->env_inc_decay2 = slot->decay2[slot->envratio];
	}
	if (slot->env_mode == EM_DECAY2)
	{
		slot->env_inc = slot->env_inc_decay2;
	}
}

static void channleupdate(OPNCH *ch)
{
	UINT	i;
	UINT32	fc = ch->keynote[0];
	UINT8	kc = ch->kcode[0];
	UINT	evr;
	OPNSLOT	*slot;
	int		s;

	slot = ch->slot;
	if (!(ch->extop))
	{
		for (i = 0; i < 4; i++, slot++)
		{
			slot->freq_inc = ((fc + slot->detune1[kc]) * slot->multiple) >> 1;
			evr = kc >> slot->keyscalerate;
			if (slot->envratio != evr)
			{
				slot->envratio = evr;
				slot->env_inc_attack = slot->attack[evr];
				slot->env_inc_decay1 = slot->decay1[evr];
				slot->env_inc_decay2 = slot->decay2[evr];
				slot->env_inc_release = slot->release[evr];
			}
		}
	}
	else
	{
		for (i = 0; i < 4; i++, slot++)
		{
			s = extendslot[i];
			slot->freq_inc = ((ch->keynote[s] + slot->detune1[ch->kcode[s]]) * slot->multiple) >> 1;
			evr = ch->kcode[s] >> slot->keyscalerate;
			if (slot->envratio != evr)
			{
				slot->envratio = evr;
				slot->env_inc_attack = slot->attack[evr];
				slot->env_inc_decay1 = slot->decay1[evr];
				slot->env_inc_decay2 = slot->decay2[evr];
				slot->env_inc_release = slot->release[evr];
			}
		}
	}
}


/* ---- */

void opngen_reset(OPNGEN opngen)
{
	OPNCH	*ch;
	UINT	i;
	OPNSLOT	*slot;
	UINT	j;

	memset(opngen, 0, sizeof(*opngen));
	opngen->playchannels = 3;

	ch = opngen->opnch;
	for (i = 0; i < OPNCH_MAX; i++)
	{
		ch->keynote[0] = 0;
		slot = ch->slot;
		for (j = 0; j < 4; j++)
		{
			slot->env_mode = EM_OFF;
			slot->env_cnt = EC_OFF;
			slot->env_end = EC_OFF + 1;
			slot->env_inc = 0;
			slot->detune1 = detunetable[0];
			slot->attack = nulltable;
			slot->decay1 = nulltable;
			slot->decay2 = nulltable;
			slot->release = decaytable;
			slot++;
		}
		ch++;
	}
	for (i = 0x30; i < 0xc0; i++)
	{
		opngen_setreg(opngen, 0, i, 0xff);
		opngen_setreg(opngen, 3, i, 0xff);
	}
}

void opngen_setcfg(OPNGEN opngen, REG8 maxch, UINT32 flag)
{
	OPNCH	*ch;
	UINT	i;

	opngen->playchannels = maxch;
	ch = opngen->opnch;
	if ((flag & OPN_CHMASK) == OPN_STEREO)
	{
		for (i = 0; i < OPNCH_MAX; i++)
		{
			if (flag & (1 << i))
			{
				ch->stereo = TRUE;
				set_algorithm(opngen, ch);
			}
			ch++;
		}
	}
	else
	{
		for (i = 0; i < OPNCH_MAX; i++)
		{
			if (flag & (1 << i))
			{
				ch->stereo = FALSE;
				set_algorithm(opngen, ch);
			}
			ch++;
		}
	}
}

void opngen_setextch(OPNGEN opngen, UINT chnum, REG8 data)
{
	OPNCH	*ch;

	ch = opngen->opnch;
	ch[chnum].extop = data;
}

void opngen_setreg(OPNGEN opngen, REG8 chbase, UINT reg, REG8 value)
{
	UINT	chpos;
	OPNCH	*ch;
	OPNSLOT	*slot;
	UINT	fn;
	UINT8	blk;

	chpos = reg & 3;
	if (chpos == 3)
	{
		return;
	}
	sound_sync();
	ch = opngen->opnch + chbase + chpos;
	if (reg < 0xa0)
	{
		slot = ch->slot + fmslot[(reg >> 2) & 3];
		switch (reg & 0xf0)
		{
			case 0x30:					/* DT1 MUL */
				set_dt1_mul(slot, value);
				channleupdate(ch);
				break;

			case 0x40:					/* TL */
				set_tl(slot, value);
				break;

			case 0x50:					/* KS AR */
				set_ks_ar(slot, value);
				channleupdate(ch);
				break;

			case 0x60:					/* D1R */
				set_d1r(slot, value);
				break;

			case 0x70:					/* DT2 D2R */
				set_dt2_d2r(slot, value);
				channleupdate(ch);
				break;

			case 0x80:					/* D1L RR */
				set_d1l_rr(slot, value);
				break;

			case 0x90:
				set_ssgeg(slot, value);
				channleupdate(ch);
				break;
		}
	}
	else
	{
		switch (reg & 0xfc)
		{
			case 0xa0:
				blk = ch->keyfunc[0] >> 3;
				fn = ((ch->keyfunc[0] & 7) << 8) + value;
				ch->kcode[0] = (blk << 2) | kftable[fn >> 7];
				ch->keynote[0] = fn << (opncfg.ratebit + blk + FREQ_BITS - 21);
				channleupdate(ch);
				break;

			case 0xa4:
				ch->keyfunc[0] = value & 0x3f;
				break;

			case 0xa8:
				ch = opngen->opnch + chbase + 2;
				blk = ch->keyfunc[chpos+1] >> 3;
				fn = ((ch->keyfunc[chpos+1] & 7) << 8) + value;
				ch->kcode[chpos + 1] = (blk << 2) | kftable[fn >> 7];
				ch->keynote[chpos + 1] = fn << (opncfg.ratebit + blk + FREQ_BITS - 21);
				channleupdate(ch);
				break;

			case 0xac:
				ch = opngen->opnch + chbase + 2;
				ch->keyfunc[chpos + 1] = value & 0x3f;
				break;

			case 0xb0:
				ch->algorithm = (UINT8)(value & 7);
				value = (value >> 3) & 7;
				if (value)
				{
					ch->feedback = 8 - value;
				}
				else
				{
					ch->feedback = 0;
				}
				set_algorithm(opngen, ch);
				break;

			case 0xb4:
				ch->pan = (UINT8)(value & 0xc0);
				set_algorithm(opngen, ch);
				break;
		}
	}
}

void opngen_keyon(OPNGEN opngen, UINT chnum, REG8 value)
{
	OPNCH	*ch;
	OPNSLOT	*slot;
	REG8	bit;
	UINT	i;

	sound_sync();
	opngen->playing++;
	ch = opngen->opnch + chnum;
	ch->playing |= value >> 4;
	slot = ch->slot;
	bit = 0x10;
	for (i = 0; i < 4; i++)
	{
		if (value & bit)							/* keyon */
		{
			if (slot->env_mode <= EM_RELEASE)
			{
				slot->freq_cnt = 0;
				if (i == OPNSLOT1)
				{
					ch->op1fb = 0;
				}
				slot->env_mode = EM_ATTACK;
				slot->env_inc = slot->env_inc_attack;
				slot->env_cnt = EC_ATTACK;
				slot->env_end = EC_DECAY;
			}
		}
		else										/* keyoff */
		{
			if (slot->env_mode > EM_RELEASE)
			{
				slot->env_mode = EM_RELEASE;
				if (!(slot->env_cnt & EC_DECAY))
				{
					slot->env_cnt = (opncfg.envcurve[slot->env_cnt >> ENV_BITS] << ENV_BITS) + EC_DECAY;
				}
				slot->env_end = EC_OFF;
				slot->env_inc = slot->env_inc_release;
			}
		}
		slot++;
		bit <<= 1;
	}
}

void opngen_csm(OPNGEN opngen)
{
	opngen_keyon(opngen, 2, 0x02);
	opngen_keyon(opngen, 2, 0xf2);
}
