/**
 * @file	opna.h
 * @brief	Interface of OPNA
 */

#pragma once

#include "adpcm.h"
#include "opngen.h"
#include "psggen.h"
#include "rhythm.h"
#include "statsave.h"

/**
 * Chips flags
 */
enum
{
	OPNA_HAS_TIMER		= 0x01,		/*!< Has timer */
	OPNA_HAS_PSG		= 0x02,		/*!< Has PSG */
	OPNA_HAS_FM			= 0x04,		/*!< Has FM */
	OPNA_HAS_EXTENDEDFM	= 0x08,		/*!< Has Extend FM */
	OPNA_HAS_RHYTHM		= 0x10,		/*!< Has Rhythm */
	OPNA_HAS_ADPCM		= 0x20,		/*!< Has ADPCM DRAM */
	OPNA_HAS_VR			= 0x40,		/*!< Has VR */
	OPNA_S98			= 0x80,		/*!< Supports S98 */

	OPNA_MODE_2203		= OPNA_HAS_PSG | OPNA_HAS_FM,
	OPNA_MODE_2608		= OPNA_MODE_2203 | OPNA_HAS_EXTENDEDFM | OPNA_HAS_RHYTHM,
	OPNA_MODE_3438		= OPNA_HAS_FM | OPNA_HAS_EXTENDEDFM
};

/**
 * @brief opna
 */
struct tagOpnaState
{
	UINT8	addrl;
	UINT8	addrh;
	UINT8	data;
	UINT8	extend;
	UINT16	base;
	UINT8	adpcmmask;
	UINT8	cCaps;
	UINT8	status;
	UINT8	irq;
	UINT8	intr;
	UINT8	keyreg[8];
	UINT8	reg[0x200];
};

/**
 * @brief opna
 */
struct tagOpna
{
	struct tagOpnaState s;
	INTPTR userdata;
	_OPNGEN opngen;
	_PSGGEN psg;
	_RHYTHM rhythm;
	_ADPCM adpcm;
};

typedef struct tagOpna OPNA;
typedef struct tagOpna* POPNA;
typedef const struct tagOpna* PCOPNA;

#ifdef __cplusplus
extern "C"
{
#endif

void opna_construct(POPNA opna);
void opna_destruct(POPNA opna);
void opna_reset(POPNA opna, REG8 cCaps);
void opna_bind(POPNA opna);

REG8 opna_readStatus(POPNA opna);
REG8 opna_readExtendedStatus(POPNA opna);

void opna_writeRegister(POPNA opna, UINT nAddress, REG8 cData);
void opna_writeExtendedRegister(POPNA opna, UINT nAddress, REG8 cData);

REG8 opna_readRegister(POPNA opna, UINT nAddress);
REG8 opna_readExtendedRegister(POPNA opna, UINT nAddress);
REG8 opna_read3438ExtRegister(POPNA opna, UINT nAddress);

int opna_sfsave(PCOPNA opna, STFLAGH sfh, const SFENTRY *tbl);
int opna_sfload(POPNA opna, STFLAGH sfh, const SFENTRY *tbl);

#ifdef __cplusplus
}
#endif
