/**
 * @file	oplgen.h
 * @brief	Interface of the OPL generator
 */

#pragma once

#include "sound.h"

enum
{
	OPLCH_MAX		= 9,
	OPL_CLOCK		= 3579545,
};

typedef struct
{
	SINT32		totallevel;			/* total level */
	SINT32		totallevel2;		/* total level */
	SINT32		decaylevel;			/* decay level */
const SINT32	*attack;			/* attack ratio */
const SINT32	*decay1;			/* decay1 ratio */
const SINT32	*release;			/* release ratio */
	SINT32 		freq_cnt;			/* frequency count */
	SINT32		freq_inc;			/* frequency step */
	UINT8		multiple;			/* multiple */
	UINT8		mode;
	UINT8		keyscalelevel;		/* key scale */
	UINT8		keyscalerate;		/* key scale */
	UINT8		env_mode;			/* envelope mode */
	UINT8		envratio;			/* envelope ratio */

	SINT32		env_cnt;			/* envelope count */
	SINT32		env_end;			/* envelope end count */
	SINT32		env_inc;			/* envelope step */
	SINT32		env_inc_attack;		/* envelope attack step */
	SINT32		env_inc_decay1;		/* envelope decay1 step */
	SINT32		env_inc_release;	/* envelope release step */

	SINT32*		sintable;			/* sin */
} OPLSLOT;

typedef struct
{
	OPLSLOT	slot[2];
	UINT8	algorithm;			/* algorithm */
	UINT8	feedback;			/* self feedback */
	UINT8	playing;
	UINT8	kcode;				/* key code */
	SINT32	op1fb;				/* operator1 feedback */
	SINT32	*connect1;			/* operator1 connect */
	SINT32	*connect2;			/* operator2 connect */
	UINT	blkfnum;			/* block and fnumber */
	UINT32	keynote;			/* key note */
	UINT32	kslbase;
} OPLCH;

typedef struct
{
	UINT	playing;
	UINT8	rhythm;
	SINT32	feedback2;
	SINT32	outdc;
	SINT32	calcremain;
	UINT	noise;
	SINT32	calc1024;
	OPLCH	oplch[OPLCH_MAX];
} _OPLGEN, *OPLGEN;

#ifdef __cplusplus
extern "C"
{
#endif

void oplgen_initialize(UINT rate);
void oplgen_setvol(UINT vol);

void oplgen_reset(OPLGEN oplgen, UINT nBaseClock);
void oplgen_setreg(OPLGEN oplgen, UINT reg, REG8 value);

void SOUNDCALL oplgen_getpcm(OPLGEN oplgen, SINT32 *buf, UINT count);

#ifdef __cplusplus
}
#endif
