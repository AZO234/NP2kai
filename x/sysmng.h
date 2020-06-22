#ifndef	NP2_X_SYSMNG_H__
#define	NP2_X_SYSMNG_H__

#include "toolwin.h"

// どーでもいい通知系

G_BEGIN_DECLS

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

extern	UINT	sys_updates;

extern	SYSMNGMISCINFO	sys_miscinfo;

#define	sysmng_initialize() \
do { \
	sys_updates = 0; \
} while (/*CONSTCOND*/ 0)

#define	sysmng_update(a) \
do { \
	sys_updates |= (a); \
	if ((a) & SYS_UPDATEFDD) \
		sysmng_updatecaption(1); \
} while (/*CONSTCOND*/ 0)

#define	sysmng_cpureset() \
do { \
	sys_updates &= (SYS_UPDATECFG | SYS_UPDATEOSCFG); \
	sysmng_workclockreset(); \
} while (/*CONSTCOND*/ 0)

#define	sysmng_fddaccess(a)	toolwin_fddaccess((a))
#define	sysmng_hddaccess(a)	toolwin_hddaccess((a))

void sysmng_workclockreset(void);
BOOL sysmng_workclockrenewal(void);
void sysmng_updatecaption(UINT8 flag);

G_END_DECLS

#endif	/* NP2_X_SYSMNG_H__ */
