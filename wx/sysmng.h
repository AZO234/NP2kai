/* === system management header for wx port === */

#ifndef NP2_WX_SYSMNG_H
#define NP2_WX_SYSMNG_H

enum {
	SYS_UPDATECFG      = 0x0001,
	SYS_UPDATEOSCFG    = 0x0002,
	SYS_UPDATECLOCK    = 0x0004,
	SYS_UPDATERATE     = 0x0008,
	SYS_UPDATESBUF     = 0x0010,
	SYS_UPDATEMIDI     = 0x0020,
	SYS_UPDATESBOARD   = 0x0040,
	SYS_UPDATEFDD      = 0x0080,
	SYS_UPDATEHDD      = 0x0100,
	SYS_UPDATEMEMORY   = 0x0200,
	SYS_UPDATESERIAL1  = 0x0400
};

enum {
	SYS_UPDATECAPTION_FDD  = 0x01,
	SYS_UPDATECAPTION_CLK  = 0x02,
	SYS_UPDATECAPTION_MISC = 0x04,
	SYS_UPDATECAPTION_ALL  = 0xff,
};

typedef struct {
	UINT8 showvolume;
	UINT8 showmousespeed;
} SYSMNGMISCINFO;

/* Drive and network activity since the last sysmng_takeaccess(), one bit per
 * drive: floppies by drive number, SxSI drives 0-3 (SASI/IDE) and 8-15 (SCSI).
 * The core reports it from the emulation thread and the network threads. */
enum {
	SYS_ACCESS_NETSEND = 0x01,
	SYS_ACCESS_NETRECV = 0x02
};

typedef struct {
	UINT32 fddread;
	UINT32 fddwrite;
	UINT32 hddread;
	UINT32 hddwrite;
	UINT32 net;
} SYSMNGACCESS;

#ifdef __cplusplus
extern "C" {
#endif

extern UINT         sys_updates;
extern SYSMNGMISCINFO sys_miscinfo;

void sysmng_initialize(void);
void sysmng_deinitialize(void);
void sysmng_update(UINT update);
void sysmng_cpureset(void);
void sysmng_fddaccess(UINT8 drv);
void sysmng_fddwrite(UINT8 drv);
void sysmng_hddaccess(UINT8 drv);
void sysmng_hddwrite(UINT8 drv);
void sysmng_netsend(void);
void sysmng_netrecv(void);
void sysmng_workclockreset(void);
BOOL sysmng_workclockrenewal(void);
void sysmng_updatecaption(UINT8 flag);
void sysmng_takeaccess(SYSMNGACCESS *access);

#ifdef __cplusplus
}
#endif

#endif /* NP2_WX_SYSMNG_H */
