/**
 *	@file	sysmng.h
 *	@brief	Interface of the system
 */

#ifndef NP2_SYSMNG_H
#define NP2_SYSMNG_H

// どーでもいい通知系

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

enum {
	SYS_UPDATECAPTION_FDD	= 0x01,
	SYS_UPDATECAPTION_CLK	= 0x02,
	SYS_UPDATECAPTION_MISC	= 0x04,
	
	SYS_UPDATECAPTION_ALL	= 0xff,
};

typedef struct {
	UINT8	showvolume;
	UINT8	showmousespeed;
} SYSMNGMISCINFO;

#ifdef __cplusplus
extern "C" {
#endif

extern	UINT	sys_updates;

extern	SYSMNGMISCINFO	sys_miscinfo;

void sysmng_initialize(void);
void sysmng_deinitialize(void);

void sysmng_update(UINT update);
void sysmng_cpureset(void);
#define	sysmng_fddaccess(a)
#define	sysmng_hddaccess(a)
void sysmng_updatecaption(UINT8 flag);

#ifdef __cplusplus
}
#endif

#endif	/* NP2_SYSMNG_H */

