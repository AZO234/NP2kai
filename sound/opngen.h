/**
 * @file	opngen.h
 * @brief	Interface of the OPN generator
 */

#pragma once

#include "sound.h"

enum
{
	OPNCH_MAX		= 6,
	OPNA_CLOCK		= 3993600,

	OPN_CHMASK		= 0x80000000,
	OPN_STEREO		= 0x80000000,
	OPN_MONORAL		= 0x00000000
};

typedef struct
{
	SINT32		*detune1;			/* detune1 */
	SINT32		totallevel;			/* total level */
	SINT32		decaylevel;			/* decay level */
const SINT32	*attack;			/* attack ratio */
const SINT32	*decay1;			/* decay1 ratio */
const SINT32	*decay2;			/* decay2 ratio */
const SINT32	*release;			/* release ratio */
	SINT32 		freq_cnt;			/* frequency count */
	SINT32		freq_inc;			/* frequency step */
	SINT32		multiple;			/* multiple */
	UINT8		keyscalerate;		/* key scale */
	UINT8		env_mode;			/* envelope mode */
	UINT8		envratio;			/* envelope ratio */
	UINT8		ssgeg1;				/* SSG-EG */

	SINT32		env_cnt;			/* envelope count */
	SINT32		env_end;			/* envelope end count */
	SINT32		env_inc;			/* envelope step */
	SINT32		env_inc_attack;		/* envelope attack step */
	SINT32		env_inc_decay1;		/* envelope decay1 step */
	SINT32		env_inc_decay2;		/* envelope decay2 step */
	SINT32		env_inc_release;	/* envelope release step */
} OPNSLOT;

typedef struct
{
	OPNSLOT	slot[4];
	UINT8	algorithm;			/* algorithm */
	UINT8	feedback;			/* self feedback */
	UINT8	playing;
	UINT8	outslot;
	SINT32	op1fb;				/* operator1 feedback */
	SINT32	*connect1;			/* operator1 connect */
	SINT32	*connect3;			/* operator3 connect */
	SINT32	*connect2;			/* operator2 connect */
	SINT32	*connect4;			/* operator4 connect */
	UINT32	keynote[4];			/* key note */

	UINT8	keyfunc[4];			/* key function */
	UINT8	kcode[4];			/* key code */
	UINT8	pan;				/* pan */
	UINT8	extop;				/* extendopelator-enable */
	UINT8	stereo;				/* stereo-enable */
	UINT8	padding;
} OPNCH;

typedef struct
{
	UINT	playchannels;
	UINT	playing;
	SINT32	feedback2;
	SINT32	feedback3;
	SINT32	feedback4;
	SINT32	outdl;
	SINT32	outdc;
	SINT32	outdr;
	SINT32	calcremain;
	OPNCH	opnch[OPNCH_MAX];
} _OPNGEN, *OPNGEN;

#ifdef __cplusplus
extern "C" {
#endif

void opngen_initialize(UINT rate);
void opngen_setvol(UINT vol);
void opngen_setVR(REG8 channel, REG8 value);

void opngen_reset(OPNGEN opngen);
void opngen_setcfg(OPNGEN opngen, REG8 maxch, UINT32 flag);
void opngen_setextch(OPNGEN opngen, UINT chnum, REG8 data);
void opngen_setreg(OPNGEN opngen, REG8 chbase, UINT reg, REG8 value);
void opngen_keyon(OPNGEN opngen, UINT chnum, REG8 value);
void opngen_csm(OPNGEN opngen);

void SOUNDCALL opngen_getpcm(OPNGEN opngen, SINT32 *buf, UINT count);
void SOUNDCALL opngen_getpcmvr(OPNGEN opngen, SINT32 *buf, UINT count);

#ifdef __cplusplus
}
#endif
