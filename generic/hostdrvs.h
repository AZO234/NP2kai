/**
 * @file	hostdrvs.h
 * @brief	Interface of host-drive
 */

#pragma once

#if defined(SUPPORT_HOSTDRV)

#include "hostdrv.h"
#include "dosio.h"

/**
 * dos error codes : see int2159-BX0000
 */
enum
{
	ERR_NOERROR				= 0x00,
	ERR_FILENOTFOUND		= 0x02,		/*!< File not found */
	ERR_PATHNOTFOUND		= 0x03,		/*!< Path not found */
	ERR_NOHANDLESLEFT		= 0x04,		/*!< No handles left */
	ERR_ACCESSDENIED		= 0x05,
	ERR_INVALDACCESSMODE	= 0x0c,		/*!< Invalid access mode */
	ERR_ATTEMPTEDCURRDIR	= 0x10,
	ERR_NOMOREFILES			= 0x12,
	ERR_DISKWRITEPROTECTED	= 0x13,
	ERR_WRITEFAULT			= 0x1d,
	ERR_READFAULT			= 0x1e
};

/**
 * @brief DOS ファイル情報
 */
struct tagHostDrvFile
{
	char	fcbname[11];	/*!< FCB 名 */
	UINT	caps;			/*!< 情報フラグ */
	UINT32	size;			/*!< サイズ */
	UINT32	attr;			/*!< 属性 */
	DOSDATE	date;			/*!< 日付 */
	DOSTIME	time;			/*!< 時間 */
};
typedef struct tagHostDrvFile HDRVFILE;		/*!< 定義 */

/**
 * @brief ファイル リスト情報
 */
struct tagHostDrvList
{
	HDRVFILE file;					/*!< DOS ファイル情報 */
	OEMCHAR szFilename[MAX_PATH];	/*!< ファイル名 */
};
typedef struct tagHostDrvList _HDRVLST;		/*!< 定義 */
typedef struct tagHostDrvList *HDRVLST;		/*!< 定義 */

/**
 * @brief パス情報
 */
struct tagHostDrvPath
{
	HDRVFILE file;				/*!< DOS ファイル情報 */
	OEMCHAR szPath[MAX_PATH];	/*!< パス */
};
typedef struct tagHostDrvPath HDRVPATH;		/*!< 定義 */

LISTARRAY hostdrvs_getpathlist(const HDRVPATH *phdp, const char *lpMask, UINT nAttr);
UINT hostdrvs_getrealdir(HDRVPATH *phdp, char *lpFcbname, const char *lpDosPath);
UINT hostdrvs_appendname(HDRVPATH *phdp, const char *lpFcbname);
UINT hostdrvs_getrealpath(HDRVPATH *phdp, const char *lpDosPath);
void hostdrvs_fhdlallclose(LISTARRAY fileArray);
HDRVHANDLE hostdrvs_fhdlsea(LISTARRAY fileArray);

#endif	/* defined(SUPPORT_HOSTDRV) */
