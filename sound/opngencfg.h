/**
 * @file	opngencfg.h
 * @brief	Interface of the OPN generator
 */

#pragma once

enum
{
	FMDIV_BITS		= 10,
	FMDIV_ENT		= (1 << FMDIV_BITS),
	FMVOL_SFTBIT	= 4
};

#if defined(OPNGENX86)

#define SIN_BITS		11
#define EVC_BITS		10
#define ENV_BITS		16
#define FREQ_BITS		21
#define ENVTBL_BIT		14
#define SINTBL_BIT		14

#else

#define	SIN_BITS		10
#define	EVC_BITS		10
#define	ENV_BITS		16
#define	FREQ_BITS		21
#define	ENVTBL_BIT		14
#define	SINTBL_BIT		15

#endif

#define	TL_BITS			(FREQ_BITS + 2)
#define	OPM_OUTSB		(TL_BITS + 2 - 16)				/* OPM output 16bit */

#define	SIN_ENT			(1L << SIN_BITS)
#define	EVC_ENT			(1L << EVC_BITS)

#define	EC_ATTACK		0								/* ATTACK start */
#define	EC_DECAY		(EVC_ENT << ENV_BITS)			/* DECAY start */
#define	EC_OFF			((2 * EVC_ENT) << ENV_BITS)		/* OFF */

enum
{
	/* slot number */
	OPNSLOT1		= 0,
	OPNSLOT2		= 1,
	OPNSLOT3		= 2,
	OPNSLOT4		= 3,

	EM_ATTACK		= 4,
	EM_DECAY1		= 3,
	EM_DECAY2		= 2,
	EM_RELEASE		= 1,
	EM_OFF			= 0
};

typedef struct
{
	SINT32	calc1024;
	SINT32	fmvol;
	UINT	ratebit;
	UINT	vr_en;
	SINT32	vr_l;
	SINT32	vr_r;

	SINT32	sintable[SIN_ENT];
	SINT32	envtable[EVC_ENT];
	SINT32	envcurve[EVC_ENT*2 + 1];
} OPNCFG;

extern OPNCFG opncfg;
