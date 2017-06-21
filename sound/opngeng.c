/**
 * @file	opngeng.c
 * @brief	Implementation of the OPN generator
 */

#include "compiler.h"
#include "opngen.h"
#include "opngencfg.h"

#if defined(OPNGENX86)
extern char envshift[EVC_ENT];
extern char sinshift[SIN_ENT];
#endif	/* defined(OPNGENX86) */

#define	CALCENV(e, c, s)													\
	(c)->slot[(s)].freq_cnt += (c)->slot[(s)].freq_inc;						\
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
				(c)->slot[(s)].env_inc = (c)->slot[(s)].env_inc_decay2;		\
				break;														\
			case EM_RELEASE:												\
				(c)->slot[(s)].env_mode = EM_OFF;							\
			case EM_DECAY2:													\
				(c)->slot[(s)].env_cnt = EC_OFF;							\
				(c)->slot[(s)].env_end = EC_OFF + 1;						\
				(c)->slot[(s)].env_inc = 0;									\
				(c)->playing &= ~(1 << (s));								\
				break;														\
		}																	\
	}																		\
	(e) = (c)->slot[(s)].totallevel -										\
					opncfg.envcurve[(c)->slot[(s)].env_cnt >> ENV_BITS];

#if defined(OPNGENX86)
static SINT32 SLOTOUT(SINT32 e, SINT32 c)
{
	c = (c >> (FREQ_BITS - SIN_BITS)) & (SIN_ENT - 1);
	return (opncfg.sintable[c] * opncfg.envtable[e]) >> (sinshift[c] + envshift[e]);
}
#else	/* defined(OPNGENX86) */
#define SLOTOUT(e, c)														\
	((opncfg.sintable[((c) >> (FREQ_BITS - SIN_BITS)) & (SIN_ENT - 1)]		\
			* opncfg.envtable[(e)]) >> (ENVTBL_BIT + SINTBL_BIT - TL_BITS))
#endif	/* defined(OPNGENX86) */


static void calcratechannel(OPNGEN opngen, OPNCH *ch)
{
	SINT32 envout;
	SINT32 opout;

	opngen->feedback2 = 0;
	opngen->feedback3 = 0;
	opngen->feedback4 = 0;

	/* SLOT 1 */
	CALCENV(envout, ch, 0);
	if (envout >= 0)
	{
		if (ch->feedback)
		{
			/* with self feed back */
			opout = ch->op1fb;
			ch->op1fb = SLOTOUT(envout, ch->slot[0].freq_cnt + ((ch->op1fb >> ch->feedback) << (FREQ_BITS - (TL_BITS - 2))));
			opout = (opout + ch->op1fb) >> 1;
		}
		else
		{
			/* without self feed back */
			opout = SLOTOUT(envout, ch->slot[0].freq_cnt);
		}
		/* output slot1 */
		if (!ch->connect1)
		{
			opngen->feedback2 = opngen->feedback3 = opngen->feedback4 = opout;
		}
		else
		{
			*ch->connect1 += opout;
		}
	}
	/* SLOT 2 */
	CALCENV(envout, ch, 1);
	if (envout >= 0)
	{
		*ch->connect2 += SLOTOUT(envout, ch->slot[1].freq_cnt + (opngen->feedback2 << (FREQ_BITS - (TL_BITS - 2))));
	}
	/* SLOT 3 */
	CALCENV(envout, ch, 2);
	if (envout >= 0)
	{
		*ch->connect3 += SLOTOUT(envout, ch->slot[2].freq_cnt + (opngen->feedback3 << (FREQ_BITS - (TL_BITS - 2))));
	}
	/* SLOT 4 */
	CALCENV(envout, ch, 3);
	if (envout >= 0)
	{
		*ch->connect4 += SLOTOUT(envout, ch->slot[3].freq_cnt + (opngen->feedback4 << (FREQ_BITS - (TL_BITS - 2))));
	}
}

void SOUNDCALL opngen_getpcm(OPNGEN opngen, SINT32 *pcm, UINT count)
{
	SINT32 samp_l;
	SINT32 samp_r;
	UINT playing;
	UINT i;
	OPNCH *ch;

	if ((!opngen->playing) || (!count))
	{
		return;
	}

	do
	{
		samp_l = opngen->outdl;
		samp_r = opngen->outdr;
		if (opngen->calcremain < FMDIV_ENT)
		{
			samp_l *= opngen->calcremain;
			samp_r *= opngen->calcremain;
			opngen->calcremain = FMDIV_ENT - opngen->calcremain;
			while (TRUE /*CONSTCOND*/)
			{
				opngen->outdc = 0;
				opngen->outdl = 0;
				opngen->outdr = 0;
				playing = 0;
				for (i = 0; i < opngen->playchannels; i++)
				{
					ch = &opngen->opnch[i];
					if (ch->playing & ch->outslot)
					{
						calcratechannel(opngen, ch);
						playing++;
					}
				}
				opngen->playing = playing;

				opngen->outdl += opngen->outdc;
				opngen->outdr += opngen->outdc;
				opngen->outdl >>= FMVOL_SFTBIT;
				opngen->outdr >>= FMVOL_SFTBIT;
				if (opngen->calcremain > opncfg.calc1024)
				{
					opngen->calcremain -= opncfg.calc1024;
					samp_l += opngen->outdl * opncfg.calc1024;
					samp_r += opngen->outdr * opncfg.calc1024;
				}
				else
				{
					break;
				}
			}
			samp_l += opngen->outdl * opngen->calcremain;
			samp_r += opngen->outdr * opngen->calcremain;
			opngen->calcremain = opncfg.calc1024 - opngen->calcremain;
		}
		else
		{
			samp_l *= FMDIV_ENT;
			samp_r *= FMDIV_ENT;
			opngen->calcremain -= FMDIV_ENT;
		}

#if defined(OPNGENX86)
		pcm[0] += (SINT32)(((SINT64)samp_l * (SINT32)opncfg.fmvol) >> 32);
		pcm[1] += (SINT32)(((SINT64)samp_r * (SINT32)opncfg.fmvol) >> 32);
#else	/* defined(OPNGENX86) */
		samp_l >>= 8;
		samp_l *= opncfg.fmvol;
		samp_l >>= (OPM_OUTSB + FMDIV_BITS + 1 - FMVOL_SFTBIT - 8 + 6);
		pcm[0] += samp_l;
		samp_r >>= 8;
		samp_r *= opncfg.fmvol;
		samp_r >>= (OPM_OUTSB + FMDIV_BITS + 1 - FMVOL_SFTBIT - 8 + 6);
		pcm[1] += samp_r;
#endif	/* defined(OPNGENX86) */
		pcm += 2;
	} while (--count);
}

void SOUNDCALL opngen_getpcmvr(OPNGEN opngen, SINT32 *pcm, UINT count)
{
	SINT32 samp_l;
	SINT32 samp_r;
	UINT playing;
	UINT i;
	OPNCH *ch;

	if ((!opngen->playing) || (!count))
	{
		return;
	}

	do
	{
		samp_l = opngen->outdl;
		samp_r = opngen->outdr;
		if (opngen->calcremain < FMDIV_ENT)
		{
			samp_l *= opngen->calcremain;
			samp_r *= opngen->calcremain;
			opngen->calcremain = FMDIV_ENT - opngen->calcremain;
			while (TRUE /*CONSTCOND*/)
			{
				opngen->outdc = 0;
				opngen->outdl = 0;
				opngen->outdr = 0;
				playing = 0;
				for (i = 0; i < opngen->playchannels; i++)
				{
					ch = &opngen->opnch[i];
					if (ch->playing & ch->outslot)
					{
						calcratechannel(opngen, ch);
						playing++;
					}
				}
				opngen->playing = playing;

				if (opncfg.vr_en)
				{
					SINT32 tmpl;
					SINT32 tmpr;
					tmpl = opngen->outdl * opncfg.vr_l;
					tmpr = opngen->outdr * opncfg.vr_r;
					opngen->outdl += (tmpr >> 5) + (tmpl >> 7);
					opngen->outdr += (tmpl >> 5) + (tmpr >> 7);
				}
				opngen->outdl += opngen->outdc;
				opngen->outdr += opngen->outdc;
				opngen->outdl >>= FMVOL_SFTBIT;
				opngen->outdr >>= FMVOL_SFTBIT;
				if (opngen->calcremain > opncfg.calc1024)
				{
					samp_l += opngen->outdl * opncfg.calc1024;
					samp_r += opngen->outdr * opncfg.calc1024;
					opngen->calcremain -= opncfg.calc1024;
				}
				else
				{
					break;
				}
			}
			samp_l += opngen->outdl * opngen->calcremain;
			samp_r += opngen->outdr * opngen->calcremain;
		}
		else
		{
			samp_l *= FMDIV_ENT;
			samp_r *= FMDIV_ENT;
			opngen->calcremain -= FMDIV_ENT;
		}
#if defined(OPNGENX86)
		pcm[0] += (SINT32)(((SINT64)samp_l * (SINT32)opncfg.fmvol) >> 32);
		pcm[1] += (SINT32)(((SINT64)samp_r * (SINT32)opncfg.fmvol) >> 32);
#else	/* defined(OPNGENX86) */
		samp_l >>= 8;
		samp_l *= opncfg.fmvol;
		samp_l >>= (OPM_OUTSB + FMDIV_BITS + 1 - FMVOL_SFTBIT - 8 + 6);
		pcm[0] += samp_l;
		samp_r >>= 8;
		samp_r *= opncfg.fmvol;
		samp_r >>= (OPM_OUTSB + FMDIV_BITS + 1 - FMVOL_SFTBIT - 8 + 6);
		pcm[1] += samp_r;
#endif	/* defined(OPNGENX86) */
		opngen->calcremain = opncfg.calc1024 - opngen->calcremain;
		pcm += 2;
	} while (--count);
}
