/**
 * @file	bios.h
 * @brief	Interface of BIOS
 */

#pragma once

#include "cpumem.h"

enum {
	BIOS_SEG		= 0xfd80,
	BIOS_BASE		= (BIOS_SEG << 4),

	BIOS_TABLE		= 0x0040,

	BIOSOFST_ITF	= 0x0080,
	BIOSOFST_INIT	= 0x0084,

	BIOSOFST_09		= 0x0088,					// Keyboard
	BIOSOFST_0c		= 0x008c,					// Serial

	BIOSOFST_12		= 0x0090,					// FDC
	BIOSOFST_13		= 0x0094,					// FDC

	BIOSOFST_18		= 0x0098,					// Common
	BIOSOFST_19		= 0x009c,					// RS-232C
	BIOSOFST_CMT	= 0x00a0,					// CMT
	BIOSOFST_PRT	= 0x00a4,					// Printer
	BIOSOFST_1b		= 0x00a8,					// Disk
	BIOSOFST_1c		= 0x00ac,					// Timer
	BIOSOFST_1f		= 0x00b0,					// Ext

	BIOSOFST_WAIT	= 0x00b4					// FDD waiting
};


#ifdef __cplusplus
extern "C" {
#endif

// extern	BOOL	biosrom;


void bios_initialize(void);
UINT MEMCALL biosfunc(UINT32 adrs);

void bios0x09(void);
void bios0x09_init(void);

void bios0x0c(void);

void bios0x12(void);
void bios0x13(void);

void bios0x18(void);
void bios0x18_0a(REG8 mode);
void bios0x18_0c(void);
void bios0x18_10(REG8 curdel);
REG16 bios0x18_14(REG16 seg, REG16 off, REG16 code);
void bios0x18_16(REG8 chr, REG8 atr);
void bios0x18_40(void);
void bios0x18_41(void);
void bios0x18_42(REG8 mode);

void bios0x19(void);

void bios0x1a_cmt(void);
void bios0x1a_prt(void);

void bios0x1b(void);
UINT bios0x1b_wait(void);
void fddbios_equip(REG8 type, BOOL clear);

REG16 bootstrapload(void);

void bios0x1c(void);

void bios0x1f(void);

#ifdef __cplusplus
}
#endif

