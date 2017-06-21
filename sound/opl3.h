/**
 * @file	opl3.h
 * @brief	Interface of OPL3
 */

#pragma once

#include "oplgen.h"
#include "statsave.h"

/**
 * Chips flags
 */
enum
{
	OPL3_HAS_TIMER		= 0x01,		/*!< Has timer */
	OPL3_HAS_OPL		= 0x02,		/*!< Has OPL */
	OPL3_HAS_OPL2		= 0x04,		/*!< Has OPL2 */
	OPL3_HAS_OPL3		= 0x08,		/*!< Has OPL3 */
	OPL3_HAS_OPL3L		= 0x10,		/*!< Has OPL3-L */
	OPL3_HAS_8950ADPCM	= 0x20,		/*!< Has 8950 ADPCM */

	OPL3_MODE_8950		= OPL3_HAS_OPL | OPL3_HAS_8950ADPCM,
	OPL3_MODE_3812 		= OPL3_HAS_OPL | OPL3_HAS_OPL2,
	OPL3_MODE_262		= OPL3_HAS_OPL | OPL3_HAS_OPL2 | OPL3_HAS_OPL3
};

/**
 * @brief opl3
 */
struct tagOpl3State
{
	UINT8	addrl;
	UINT8	addrh;
	UINT8	data;
	UINT8	cCaps;
	UINT8	reg[0x200];
};

/**
 * @brief opl3
 */
struct tagOpl3
{
	struct tagOpl3State s;
	INTPTR userdata;
	_OPLGEN oplgen;
};

typedef struct tagOpl3 OPL3;
typedef struct tagOpl3* POPL3;
typedef const struct tagOpl3* PCOPL3;

#ifdef __cplusplus
extern "C"
{
#endif

void opl3_construct(POPL3 opl3);
void opl3_destruct(POPL3 opl3);
void opl3_reset(POPL3 opl3, REG8 cCaps);
void opl3_bind(POPL3 opl3);

REG8 opl3_readStatus(POPL3 opl3);

void opl3_writeRegister(POPL3 opl3, UINT nAddress, REG8 cData);
void opl3_writeExtendedRegister(POPL3 opl3, UINT nAddress, REG8 cData);

REG8 opl3_readRegister(POPL3 opl3, UINT nAddress);
REG8 opl3_readExtendedRegister(POPL3 opl3, UINT nAddress);

int opl3_sfsave(PCOPL3 opl3, STFLAGH sfh, const SFENTRY *tbl);
int opl3_sfload(POPL3 opl3, STFLAGH sfh, const SFENTRY *tbl);

#ifdef __cplusplus
}
#endif
