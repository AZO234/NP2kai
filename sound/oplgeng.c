/**
 * @file	oplgeng.c
 * @brief	Implementation of the OPL generator
 */

#include "compiler.h"
#include "oplgen.h"
#include "oplgencfg.h"

#define	CALCENV(e, c, s)													\
	(c)->slot[(s)].env_cnt += (c)->slot[(s)].env_inc;						\
	if ((c)->slot[(s)].env_cnt >= (c)->slot[(s)].env_end)					\
	{																		\
		switch ((c)->slot[(s)].env_mode)									\
		{																	\
			case EM_ATTACK:													\
				(c)->slot[(s)].env_mode = EM_DECAY1;						\
				(c)->slot[(s)].env_cnt = EC_DECAY;							\
				(c)->slot[(s)].env_end = (c)->slot[(s)].decaylevel;			\
				(c)->slot[(s)].env_inc = (c)->slot[(s)].env_inc_decay1;		\
				break;														\
			case EM_DECAY1:													\
				(c)->slot[(s)].env_mode = EM_DECAY2;						\
				(c)->slot[(s)].env_cnt = (c)->slot[(s)].decaylevel;			\
				(c)->slot[(s)].env_end = EC_OFF;							\
				if (!((c)->slot[(s)].mode & 0x20))							\
				{															\
					(c)->slot[(s)].env_inc = c->slot[(s)].env_inc_release;	\
					break;													\
				}															\
			case EM_DECAY2:													\
				(c)->slot[(s)].env_inc = 0;									\
				break;														\
			case EM_RELEASE:												\
				(c)->slot[(s)].env_mode = EM_OFF;							\
				(c)->slot[(s)].env_cnt = EC_OFF;							\
				(c)->slot[(s)].env_end = EC_OFF + 1;						\
				(c)->slot[(s)].env_inc = 0;									\
				(c)->playing &= ~(1 << (s));								\
				break;														\
		}																	\
	}																		\
	(e) = (c)->slot[(s)].totallevel2 -										\
					oplcfg.envcurve[(c)->slot[(s)].env_cnt >> ENV_BITS];

#if defined(OPLGEN_ENVSHIFT)
static SINT32 SLOTOUT(const SINT32* s, SINT32 e, SINT32 c)
{
	c = (c >> (FREQ_BITS - SIN_BITS)) & (SIN_ENT - 1);
	return ((s)[c] * oplcfg.envtable[e]) >> (SINTBL_BIT + oplcfg.envshift[e]);
}
#else	/* defined(OPLGEN_ENVSHIFT) */
#define SLOTOUT(s, e, c)													\
	(((s)[((c) >> (FREQ_BITS - SIN_BITS)) & (SIN_ENT - 1)]					\
			* oplcfg.envtable[(e)]) >> (ENVTBL_BIT + SINTBL_BIT - TL_BITS))
#endif	/* defined(OPLGEN_ENVSHIFT) */

#define DEG2FREQ(s)		(UINT32)((s) * (1 << FREQ_BITS) / 360.0)

static void calcratechannel(OPLGEN oplgen, OPLCH *ch)
{
	SINT32 envout;
	SINT32 opout;

	ch->slot[0].freq_cnt += ch->slot[0].freq_inc;
	ch->slot[1].freq_cnt += ch->slot[1].freq_inc;

	oplgen->feedback2 = 0;

	/* SLOT 1 */
	CALCENV(envout, ch, 0);
	if (envout >= 0)
	{
		if (ch->feedback)
		{
			/* with self feed back */
			opout = ch->op1fb;
			ch->op1fb = SLOTOUT(ch->slot[0].sintable, envout, ch->slot[0].freq_cnt + ((ch->op1fb >> ch->feedback) << (FREQ_BITS - (TL_BITS - 2))));
			opout = (opout + ch->op1fb) >> 1;
		}
		else
		{
			/* without self feed back */
			opout = SLOTOUT(ch->slot[0].sintable, envout, ch->slot[0].freq_cnt);
		}
		/* output slot1 */
		*ch->connect1 += opout;
	}
	/* SLOT 2 */
	CALCENV(envout, ch, 1);
	if (envout >= 0)
	{
		*ch->connect2 += SLOTOUT(ch->slot[1].sintable, envout, ch->slot[1].freq_cnt + (oplgen->feedback2 << (FREQ_BITS - (TL_BITS - 2))));
	}
}

static UINT calcraterhythm(OPLGEN oplgen)
{
	UINT playing;
	OPLCH *ch7;
	SINT32 envout;
	SINT32 opout;
	OPLCH *ch8;
	OPLCH *ch9;
	UINT on;
	UINT freq;

	if (oplgen->noise & 1)
	{
		oplgen->noise ^= 0x800302;
	}
	oplgen->noise >>= 1;

	playing = 0;

	/* Channel 7 */
	ch7 = &oplgen->oplch[6];
	if (ch7->playing & 2)
	{
		oplgen->feedback2 = 0;

		/* SLOT 1 */
		opout = 0;
		if (ch7->algorithm == 0)
		{
			ch7->slot[0].freq_cnt += ch7->slot[0].freq_inc;
			CALCENV(envout, ch7, 0);
			if (envout >= 0)
			{
				if (ch7->feedback)
				{
					/* with self feed back */
					opout = ch7->op1fb;
					ch7->op1fb = SLOTOUT(ch7->slot[0].sintable, envout, ch7->slot[0].freq_cnt + ((ch7->op1fb >> ch7->feedback) << (FREQ_BITS - (TL_BITS - 2))));
					opout = (opout + ch7->op1fb) >> 1;
				}
				else
				{
					/* without self feed back */
					opout = SLOTOUT(ch7->slot[0].sintable, envout, ch7->slot[0].freq_cnt);
				}
			}
		}

		/* SLOT 2 */
		ch7->slot[1].freq_cnt += ch7->slot[1].freq_inc;
		CALCENV(envout, ch7, 1);
		if (envout >= 0)
		{
			*ch7->connect2 += SLOTOUT(ch7->slot[1].sintable, envout, ch7->slot[1].freq_cnt + (opout << (FREQ_BITS - (TL_BITS - 2))));
		}
		playing++;
	}

	/* Channel 8 & 9 */
	ch8 = &oplgen->oplch[7];
	ch9 = &oplgen->oplch[8];

	ch8->slot[0].freq_cnt += ch8->slot[0].freq_inc;
	ch9->slot[1].freq_cnt += ch9->slot[1].freq_inc;

	on = ch8->slot[0].freq_cnt & (3 << (FREQ_BITS - 8));
	on ^= (ch8->slot[0].freq_cnt >> 5) & (1 << (FREQ_BITS - 8));
	on |= (ch9->slot[1].freq_cnt ^ (ch9->slot[1].freq_cnt << 2)) & (1 << (FREQ_BITS - 5));

	/* High Hat */
	if (ch8->playing & 1)
	{
		CALCENV(envout, ch8, 0);
		if (envout >= 0)
		{
			freq = (on) ? DEG2FREQ(270 - 18) : DEG2FREQ(18);
			if (oplgen->noise & 1)
			{
				freq += DEG2FREQ(90);
			}
			*ch8->connect2 += SLOTOUT(ch8->slot[0].sintable, envout, freq);
		}
		playing++;
	}

	/* Snare Drum */
	if (ch8->playing & 2)
	{
		ch8->slot[1].freq_cnt += ch8->slot[1].freq_inc;
		CALCENV(envout, ch8, 1);
		if (envout >= 0)
		{
			freq = (ch8->slot[0].freq_cnt & (1 << (FREQ_BITS - 2))) ? DEG2FREQ(180) : DEG2FREQ(90);
			if (oplgen->noise & 1)
			{
				freq += DEG2FREQ(90);
			}
			*ch8->connect2 += SLOTOUT(ch8->slot[1].sintable, envout, freq);
		}
		playing++;
	}

	/* Tom Tom */
	if (ch9->playing & 1)
	{
		ch9->slot[0].freq_cnt += ch9->slot[0].freq_inc;
		CALCENV(envout, ch9, 0);
		if (envout >= 0)
		{
			*ch9->connect2 += SLOTOUT(ch9->slot[0].sintable, envout, ch9->slot[0].freq_cnt);
		}
		playing++;
	}

	/* Top Cymbal */
	if (ch9->playing & 2)
	{
		CALCENV(envout, ch9, 1);
		if (envout >= 0)
		{
			freq = (on) ? DEG2FREQ(270) : DEG2FREQ(90);
			*ch9->connect2 += SLOTOUT(ch9->slot[1].sintable, envout, freq);
		}
		playing++;
	}
	return playing;
}

void SOUNDCALL oplgen_getpcm(OPLGEN oplgen, SINT32 *pcm, UINT count)
{
	SINT32 samp_c;
	UINT playing;
	UINT i;
	OPLCH *ch;

	if ((!oplgen->playing) || (!count))
	{
		return;
	}

	do
	{
		samp_c = oplgen->outdc;
		if (oplgen->calcremain < FMDIV_ENT)
		{
			samp_c *= oplgen->calcremain;
			oplgen->calcremain = FMDIV_ENT - oplgen->calcremain;
			while (TRUE /*CONSTCOND*/)
			{
				oplgen->outdc = 0;
				playing = 0;
				for (i = 0; i < 6; i++)
				{
					ch = &oplgen->oplch[i];
					if (ch->playing & 3)
					{
						calcratechannel(oplgen, ch);
						playing++;
					}
				}
				if (!(oplgen->rhythm & 0x20))
				{
					for (i = 6; i < 9; i++)
					{
						ch = &oplgen->oplch[i];
						if (ch->playing & 3)
						{
							calcratechannel(oplgen, ch);
							playing++;
						}
					}
				}
				else
				{
					playing += calcraterhythm(oplgen);
				}
				oplgen->playing = playing;

				oplgen->outdc >>= (FMVOL_SFTBIT + 1);
				if (oplgen->calcremain > oplgen->calc1024)
				{
					oplgen->calcremain -= oplgen->calc1024;
					samp_c += oplgen->outdc * oplgen->calc1024;
				}
				else
				{
					break;
				}
			}
			samp_c += oplgen->outdc * oplgen->calcremain;
		}
		else
		{
			samp_c *= FMDIV_ENT;
			oplgen->calcremain -= FMDIV_ENT;
		}
#if defined(OPLGENX86)
		pcm[0] += (SINT32)(((SINT64)samp_c * (SINT32)oplcfg.fmvol) >> 32);
		pcm[1] += (SINT32)(((SINT64)samp_c * (SINT32)oplcfg.fmvol) >> 32);
#else	/* defined(OPLGENX86) */
		samp_c >>= 8;
		samp_c *= oplcfg.fmvol;
		samp_c >>= (OPM_OUTSB + FMDIV_BITS + 1 - FMVOL_SFTBIT - 8 + 6);
		pcm[0] += samp_c;
		pcm[1] += samp_c;
#endif	/* defined(OPLGENX86) */
		oplgen->calcremain = oplgen->calc1024 - oplgen->calcremain;
		pcm += 2;
	} while (--count);
}
