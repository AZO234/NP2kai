/**
 *	@file	sysmng.h
 *	@brief	Interface of the system
 */

#pragma once

// Ç«Å[Ç≈Ç‡Ç¢Ç¢í ímån

enum {
	SYS_UPDATECFG		= 0x0001,
	SYS_UPDATEOSCFG		= 0x0002,
	SYS_UPDATECLOCK		= 0x0004,
	SYS_UPDATERATE		= 0x0008,
	SYS_UPDATESBUF		= 0x0010,
	SYS_UPDATEMIDI		= 0x0020,
	SYS_UPDATESBOARD	= 0x0040,
	SYS_UPDATEFDD		= 0x0080,
	SYS_UPDATEHDD		= 0x0100,
	SYS_UPDATEMEMORY	= 0x0200,
	SYS_UPDATESERIAL1	= 0x0400
};

void sysmng_initialize(void);
void sysmng_deinitialize(void);

#ifdef __cplusplus
extern "C" {
#endif

void sysmng_update(UINT update);
void sysmng_cpureset(void);
#define	sysmng_fddaccess(a)
#define	sysmng_hddaccess(a)

#ifdef __cplusplus
}
#endif

