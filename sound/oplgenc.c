/**
 * @file	oplgenc.c
 * @brief	Implementation of the OPL generator
 */

#include "compiler.h"
#include "oplgen.h"
#include <math.h>
#include "oplgencfg.h"

	OPLCFG oplcfg;

#define	OPM_ARRATE		 399128L
#define	OPM_DRRATE		5514396L

#define	EG_STEP			(96.0 / EVC_ENT)					/* dB step */
#define	SC(db)			(SINT32)((db) * ((3.0 / EG_STEP) * (1 << ENV_BITS))) + EC_DECAY

static	SINT32	attacktable[94];
static	SINT32	decaytable[94];

static const SINT32	decayleveltable[16] = {
		 			SC( 0),SC( 1),SC( 2),SC( 3),SC( 4),SC( 5),SC( 6),SC( 7),
		 			SC( 8),SC( 9),SC(10),SC(11),SC(12),SC(13),SC(14),SC(31)};
static const UINT8 multipletable[] = {
					1, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 20, 24, 24, 30, 30};
static const SINT32 nulltable[] = {
					0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
					0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

#define DV(db) (UINT32)((db) / (0.1875 / 2.0))
static const UINT32 ksl_tab[8 * 16]=
{
	/* OCT 0 */
	DV( 0.000),DV( 0.000),DV( 0.000),DV( 0.000),
	DV( 0.000),DV( 0.000),DV( 0.000),DV( 0.000),
	DV( 0.000),DV( 0.000),DV( 0.000),DV( 0.000),
	DV( 0.000),DV( 0.000),DV( 0.000),DV( 0.000),
	/* OCT 1 */
	DV( 0.000),DV( 0.000),DV( 0.000),DV( 0.000),
	DV( 0.000),DV( 0.000),DV( 0.000),DV( 0.000),
	DV( 0.000),DV( 0.750),DV( 1.125),DV( 1.500),
	DV( 1.875),DV( 2.250),DV( 2.625),DV( 3.000),
	/* OCT 2 */
	DV( 0.000),DV( 0.000),DV( 0.000),DV( 0.000),
	DV( 0.000),DV( 1.125),DV( 1.875),DV( 2.625),
	DV( 3.000),DV( 3.750),DV( 4.125),DV( 4.500),
	DV( 4.875),DV( 5.250),DV( 5.625),DV( 6.000),
	/* OCT 3 */
	DV( 0.000),DV( 0.000),DV( 0.000),DV( 1.875),
	DV( 3.000),DV( 4.125),DV( 4.875),DV( 5.625),
	DV( 6.000),DV( 6.750),DV( 7.125),DV( 7.500),
	DV( 7.875),DV( 8.250),DV( 8.625),DV( 9.000),
	/* OCT 4 */
	DV( 0.000),DV( 0.000),DV( 3.000),DV( 4.875),
	DV( 6.000),DV( 7.125),DV( 7.875),DV( 8.625),
	DV( 9.000),DV( 9.750),DV(10.125),DV(10.500),
	DV(10.875),DV(11.250),DV(11.625),DV(12.000),
	/* OCT 5 */
	DV( 0.000),DV( 3.000),DV( 6.000),DV( 7.875),
	DV( 9.000),DV(10.125),DV(10.875),DV(11.625),
	DV(12.000),DV(12.750),DV(13.125),DV(13.500),
	DV(13.875),DV(14.250),DV(14.625),DV(15.000),
	/* OCT 6 */
	DV( 0.000),DV( 6.000),DV( 9.000),DV(10.875),
	DV(12.000),DV(13.125),DV(13.875),DV(14.625),
	DV(15.000),DV(15.750),DV(16.125),DV(16.500),
	DV(16.875),DV(17.250),DV(17.625),DV(18.000),
	/* OCT 7 */
	DV( 0.000),DV( 9.000),DV(12.000),DV(13.875),
	DV(15.000),DV(16.125),DV(16.875),DV(17.625),
	DV(18.000),DV(18.750),DV(19.125),DV(19.500),
	DV(19.875),DV(20.250),DV(20.625),DV(21.000)
};

void oplgen_initialize(UINT rate)
{
	UINT	ratebit;
	int		i;
#if defined(OPLGEN_ENVSHIFT)
	char	sft;
#endif	/* defined(OPLGEN_ENVSHIFT) */
	double	pom;
	double	freq;

	if (rate > (OPL_CLOCK / 144.0))
	{
		ratebit = 0;
	}
	else if (rate > (OPL_CLOCK / 288.0))
	{
		ratebit = 1;
	}
	else
	{
		ratebit = 2;
	}

	for (i = 0; i < EVC_ENT; i++)
	{
#if defined(OPLGEN_ENVSHIFT)
		sft = ENVTBL_BIT;
		while (sft < (ENVTBL_BIT + 8))
		{
			pom = (double)(1 << sft) / pow(10.0, EG_STEP * (EVC_ENT - i) / 20.0);
			oplcfg.envtable[i] = (SINT32)pom;
			oplcfg.envshift[i] = sft - TL_BITS;
			if (oplcfg.envtable[i] >= (1 << (ENVTBL_BIT - 1)))
			{
				break;
			}
			sft++;
		}
#else	/* defined(OPLGEN_ENVSHIFT) */
		pom = (double)(1 << ENVTBL_BIT) / pow(10.0, EG_STEP * (EVC_ENT - i) / 20.0);
		oplcfg.envtable[i] = (SINT32)pom;
#endif	/* defined(OPLGEN_ENVSHIFT) */
	}
	for (i = 0; i < SIN_ENT; i++)
	{
		pom = (double)((1 << SINTBL_BIT) - 1) * sin(2 * M_PI * i / SIN_ENT);
		oplcfg.sintable[0][i] = (SINT32)pom;
	}
	for (i = 0; i < SIN_ENT; i++)
	{
		oplcfg.sintable[1][i] = (i & (SIN_ENT >> 1)) ? 0 : oplcfg.sintable[0][i];
		oplcfg.sintable[2][i] = oplcfg.sintable[0][i & ((SIN_ENT >> 1) - 1)];
		oplcfg.sintable[3][i] = (i & (SIN_ENT >> 2)) ? 0 : oplcfg.sintable[0][i & ((SIN_ENT >> 2) - 1)];
	}
	for (i = 0; i < EVC_ENT; i++)
	{
		pom = pow(((double)(EVC_ENT - 1 - i) / EVC_ENT), 8) * EVC_ENT;
		oplcfg.envcurve[i] = (SINT32)pom;
		oplcfg.envcurve[EVC_ENT + i] = i;
	}
	oplcfg.envcurve[EVC_ENT * 2] = EVC_ENT;

	oplcfg.rate = rate;
	oplcfg.ratebit = ratebit;

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
		attacktable[i] = (SINT32)(freq * 3.0 / OPM_ARRATE);
		decaytable[i] = (SINT32)(freq * 2.0 / OPM_DRRATE);
#if 1
		if (attacktable[i] >= EC_DECAY)
		{
			printf("attacktable %d %d %ld\n", i, attacktable[i], EC_DECAY);
		}
		if (decaytable[i] >= EC_DECAY)
		{
			printf("decaytable %d %d %ld\n", i, decaytable[i], EC_DECAY);
		}
#endif
	}
	attacktable[62] = EC_DECAY - 1;
	attacktable[63] = EC_DECAY - 1;
	for (i = 64; i < 94; i++)
	{
		attacktable[i] = attacktable[63];
		decaytable[i] = decaytable[63];
	}
}

void oplgen_setvol(UINT vol)
{
	oplcfg.fmvol = vol * 5 / 4;
#if defined(OPLGENX86)
	oplcfg.fmvol <<= (32 - (OPM_OUTSB + 1 - FMVOL_SFTBIT + FMDIV_BITS + 6));
#endif
}


/* ---- */

static void set_am_vib_egt_ksr_mult(OPLSLOT *slot, REG8 value)
{
	slot->mode = value;
	slot->keyscalerate = (value & 0x10) ? 0 : 2;
	slot->multiple = (SINT32)multipletable[value & 0x0f];
}

static void set_ksl_tl(OPLSLOT *slot, REG8 value)
{
	REG8 ksl;

	ksl = value >> 6;
	slot->keyscalelevel = (ksl) ? (3 - ksl) : 31;

#if (EVC_BITS >= 7)
	slot->totallevel = (0x7f - (value & 0x3f)) << (EVC_BITS - 7);
#else
	slot->totallevel = (0x7f - (value & 0x3f)) >> (7 - EVC_BITS);
#endif
}

static void set_ar_dr(OPLSLOT *slot, REG8 value)
{
	UINT attack;
	UINT decay1;

	attack = value >> 4;
	slot->attack = (value) ? (attacktable + (attack << 2)) : nulltable;
	slot->env_inc_attack = slot->attack[slot->envratio];
	if (slot->env_mode == EM_ATTACK)
	{
		slot->env_inc = slot->env_inc_attack;
	}

	decay1 = value & 15;
	slot->decay1 = (decay1) ? (decaytable + (decay1 << 2)) : nulltable;
	slot->env_inc_decay1 = slot->decay1[slot->envratio];
	if (slot->env_mode == EM_DECAY1)
	{
		slot->env_inc = slot->env_inc_decay1;
	}
}

static void set_sl_rr(OPLSLOT *slot, REG8 value)
{
	slot->decaylevel = decayleveltable[(value >> 4)];
	slot->release = decaytable + ((value & 15) << 2) + 2;
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

static void set_wavesel(OPLSLOT *slot, REG8 value)
{
	slot->sintable = oplcfg.sintable[value & 3];
}

/* ----- */

static void set_fnumberl(OPLCH *ch, REG8 value)
{
	UINT blk;
	UINT fn;

	ch->blkfnum = (ch->blkfnum & 0x1f00) | value;

	blk = ch->blkfnum >> 10;
	fn = ch->blkfnum & 0x3ff;
	ch->kcode = ch->blkfnum >> 9;
	ch->keynote = (fn << blk) << (FREQ_BITS - 20);
	ch->kslbase = ksl_tab[ch->blkfnum >> 6] << 1;
}

static void set_kon_block_fnumh(OPLCH *ch, REG8 value)
{
	UINT blk;
	UINT fn;

	ch->blkfnum = (ch->blkfnum & 0xff) | ((value & 0x1f) << 8);

	blk = ch->blkfnum >> 10;
	fn = ch->blkfnum & 0x3ff;
	ch->kcode = ch->blkfnum >> 9;
	ch->keynote = (fn << blk) << (FREQ_BITS - 20);
	ch->kslbase = ksl_tab[ch->blkfnum >> 6] << 1;
}

static void set_fb_cnt(OPLCH *ch, REG8 value)
{
	REG8 feedback;

	feedback = (value >> 1) & 7;
	if (feedback)
	{
		ch->feedback = 8 - feedback;
	}
	else
	{
		ch->feedback = 0;
	}
	ch->algorithm = value & 1;
}

static void set_algorithm(OPLGEN oplgen, OPLCH *ch)
{
	SINT32 *outd;

	outd = &oplgen->outdc;

	if (!ch->algorithm)
	{
		ch->connect1 = &oplgen->feedback2;
	}
	else
	{
		ch->connect1 = outd;
	}
	ch->connect2 = outd;
}

static void channleupdate(OPLCH *ch)
{
	OPLSLOT *slot;
	UINT i;
	UINT evr;

	slot = ch->slot;
	for (i = 0; i < 2; i++, slot++)
	{
		slot->totallevel2 = slot->totallevel - (ch->kslbase >> slot->keyscalelevel);
		slot->freq_inc = (ch->keynote * slot->multiple) >> 1;
		evr = ch->kcode >> slot->keyscalerate;
		if (slot->envratio != evr)
		{
			slot->envratio = evr;
			slot->env_inc_attack = slot->attack[evr];
			slot->env_inc_decay1 = slot->decay1[evr];
			slot->env_inc_release = slot->release[evr];
		}
	}
}

static void keyon(OPLGEN oplgen, OPLCH *ch, UINT keyon)
{
	UINT i;
	OPLSLOT *slot;

	oplgen->playing = 1;
	ch->playing |= keyon;

	slot = ch->slot;
	for (i = 0; i < 2; i++)
	{
		if (keyon & (1 << i))				/* keyon */
		{
			if (slot->env_mode <= EM_RELEASE)
			{
				slot->freq_cnt = 0;
				if (i == OPLSLOT1)
				{
					ch->op1fb = 0;
				}
				slot->env_mode = EM_ATTACK;
				slot->env_inc = slot->env_inc_attack;
				slot->env_cnt = EC_ATTACK;
				slot->env_end = EC_DECAY;
			}
		}
		else								/* keyoff */
		{
			if (slot->env_mode > EM_RELEASE)
			{
				slot->env_mode = EM_RELEASE;
				if (!(slot->env_cnt & EC_DECAY))
				{
					slot->env_cnt = (oplcfg.envcurve[slot->env_cnt >> ENV_BITS] << ENV_BITS) + EC_DECAY;
				}
				slot->env_end = EC_OFF;
				slot->env_inc = slot->env_inc_release;
			}
		}
		slot++;
	}
}

static void rhythmon(OPLGEN oplgen, REG8 value)
{
	oplgen->rhythm = value;

	if (!(value & 0x20))
	{
		value = 0;
	}

	keyon(oplgen, &oplgen->oplch[6], (value & 0x10) ? 3 : 0);
	keyon(oplgen, &oplgen->oplch[7], ((value & 0x01) ? 1 : 0) | ((value & 0x08) ? 2 : 0));
	keyon(oplgen, &oplgen->oplch[8], ((value & 0x04) ? 1 : 0) | ((value & 0x02) ? 2 : 0));
}

/* ---- */

void oplgen_reset(OPLGEN oplgen, UINT nBaseClock)
{
	OPLCH	*ch;
	UINT	i;
	OPLSLOT	*slot;
	UINT	j;

	memset(oplgen, 0, sizeof(*oplgen));
	oplgen->noise = 1;
	oplgen->calc1024 = (SINT32)((FMDIV_ENT * (oplcfg.rate << oplcfg.ratebit) / (nBaseClock / 72.0)) + 0.5);

	ch = oplgen->oplch;
	for (i = 0; i < OPLCH_MAX; i++)
	{
		ch->keynote = 0;
		slot = ch->slot;
		for (j = 0; j < 2; j++)
		{
			slot->env_mode = EM_OFF;
			slot->env_cnt = EC_OFF;
			slot->env_end = EC_OFF + 1;
			slot->env_inc = 0;
			slot->attack = nulltable;
			slot->decay1 = nulltable;
			slot->release = decaytable;
			slot++;
		}
		ch++;
	}

	for (i = 0x20; i < 0xa0; i++)
	{
		oplgen_setreg(oplgen, i, 0xff);
	}
	for (i = 0xa0; i < 0x100; i++)
	{
		oplgen_setreg(oplgen, i, 0x00);
	}
}

static void setslot(OPLGEN oplgen, UINT reg, REG8 value)
{
	UINT nBase;
	UINT nSlot;
	OPLCH *ch;
	OPLSLOT *slot;

	nBase = (reg >> 3) & 3;
	if (nBase >= 3)
	{
		return;
	}

	nSlot = reg & 7;
	if (nSlot >= 6)
	{
		return;
	}

	ch = oplgen->oplch + (nBase * 3) + (nSlot % 3);
	slot = &ch->slot[nSlot / 3];

	switch (reg & 0xe0)
	{
		case 0x20:	/* AM:VIB:EGT:KSR:MULT */
			set_am_vib_egt_ksr_mult(slot, value);
			channleupdate(ch);
			break;

		case 0x40:	/* KSL:TL */
			set_ksl_tl(slot, value);
			channleupdate(ch);
			break;

		case 0x60:	/* AR:DR */
			set_ar_dr(slot, value);
			break;

		case 0x80:	/* SL:RR */
			set_sl_rr(slot, value);
			break;

		case 0xe0:	/* WSEL */
			set_wavesel(slot, value);
			break;
	}
}

static void setch(OPLGEN oplgen, UINT reg, REG8 value)
{
	UINT nChannel;
	OPLCH *ch;

	if (reg == 0xbd)
	{
		rhythmon(oplgen, value);
		return;
	}

	nChannel = reg & 15;
	if (nChannel >= 9)
	{
		return;
	}
	ch = oplgen->oplch + nChannel;

	switch (reg & 0xf0)
	{
		case 0xa0:	/* F-NUMBER(L) */
			set_fnumberl(ch, value);
			channleupdate(ch);
			break;

		case 0xb0:	/* KON:BLOCK:F-NUM(H) */
			set_kon_block_fnumh(ch, value);
			channleupdate(ch);
			keyon(oplgen, ch, (value & 0x20) ? 3 : 0);
			break;

		case 0xc0:	/* FB:CNT */
			set_fb_cnt(ch, value);
			set_algorithm(oplgen, ch);
			break;
	}
}

void oplgen_setreg(OPLGEN oplgen, UINT reg, REG8 value)
{
	switch (reg & 0xe0)
	{
		case 0x00:
			break;

		case 0x20:
		case 0x40:
		case 0x60:
		case 0x80:
		case 0xe0:
			setslot(oplgen, reg, value);
			break;

		case 0xa0:
		case 0xc0:
			setch(oplgen, reg, value);
			break;
	}
}
