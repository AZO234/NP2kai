/**
 * @file	hostdrv.h
 * @brief	Interface of host drive
 */

#pragma once

#if defined(SUPPORT_HOSTDRV)

#include <statsave.h>

#define	DIRMAX_DEPTH		8

enum {
	HDFMODE_READ		= 0x01,
	HDFMODE_WRITE		= 0x02,
	HDFMODE_DELETE		= 0x04
};

/**
 * 短いファイル名の生成方式
 */
enum {
	HOSTDRV_SHORTNAME_LEGACY	= 0,	/*!< Original truncation; colliding entries are omitted */
	HOSTDRV_SHORTNAME_TILDE	= 1	/*!< Host short name, then long-name ordered NAME‾n */
};

#ifndef HOSTDRV_SHORTNAME_DEFAULT
#if defined(USE_HOSTDRV_LEGACY_SHORTNAME)
#define HOSTDRV_SHORTNAME_DEFAULT	HOSTDRV_SHORTNAME_LEGACY
#else
#define HOSTDRV_SHORTNAME_DEFAULT	HOSTDRV_SHORTNAME_TILDE
#endif
#endif

// CDS退避用
typedef struct {
	BOOL	valid;
	BOOL	hidden;
	UINT16	cds_off;
	UINT16	cds_seg;
	UINT	cds_size;
	UINT8	cds_saved[100]; // 100もあれば十分なはず
} HOSTDRV_HANDOFF;

/**
 * @brief ファイル ハンドル
 */
struct tagHostDrvHandle
{
	INTPTR	hdl;
	UINT	mode;
	OEMCHAR	path[MAX_PATH];
};
typedef struct tagHostDrvHandle _HDRVHANDLE;
typedef struct tagHostDrvHandle *HDRVHANDLE;

typedef struct {
	struct {
		UINT8	is_mount:6;
		UINT8	newprotocol:2;
		UINT8	drive_no;
		UINT8	dosver_major;
		UINT8	dosver_minor;
		UINT16	sda_off;
		UINT16	sda_seg;
		UINT	flistpos;
		HOSTDRV_HANDOFF handoff;
	}			stat;

//	LISTARRAY	cache[DIRMAX_DEPTH];
	LISTARRAY	fhdl;
	LISTARRAY	flist;
} HOSTDRV;

typedef struct
{
	LISTARRAY	flist;
	UINT		flistpos; // 検索列挙位置
	UINT16		flistidx; // 仮想クラスタ番号
} HOSTDRV_FINDHANDLE;



#ifdef __cplusplus
extern "C" {
#endif

extern	HOSTDRV		hostdrv;

void hostdrv_initialize(void);
void hostdrv_deinitialize(void);
void hostdrv_reset(void);
// void save_hostdrv(void);
// void load_hostdrv(void);

void hostdrv_mount(const void *arg1, long arg2);
void hostdrv_unmount(const void *arg1, long arg2);
void hostdrv_intr(const void *arg1, long arg2);
void hostdrv_setn(const void* arg1, long arg2);

int hostdrv_ismounted(void);
int hostdrv_issuspended(void);
int hostdrv_iscddshidden(void);
UINT32 hostdrv_getdriveno(void);
void hostdrv_setsuspended(int suspend);

int hostdrv_sfsave(STFLAGH sfh, const SFENTRY *tbl);
int hostdrv_sfload(STFLAGH sfh, const SFENTRY *tbl);

#ifdef __cplusplus
}
#endif

#endif

