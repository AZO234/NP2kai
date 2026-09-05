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
	ERR_READFAULT			= 0x1e,
	ERR_FILE_EXISTS			= 0x50
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
typedef struct tagHostDrvFile HDRVFILE;

/**
 * @brief ファイル リスト情報
 */
struct tagHostDrvList
{
	HDRVFILE file;
	OEMCHAR szFilename[MAX_PATH];
};
typedef struct tagHostDrvList _HDRVLST;
typedef struct tagHostDrvList *HDRVLST;

/**
 * @brief パス情報
 */
struct tagHostDrvPath
{
	HDRVFILE file;
	OEMCHAR szPath[MAX_PATH];
};
typedef struct tagHostDrvPath HDRVPATH;

/**
 * @brief 短いファイル名マップ
 *
 * file.fcbname: DOS互換FCB名
 * szShortFilename: ホストから取得した短いファイル名（あれば）
 */
struct tagHostDrvShortNameEntry
{
	HDRVFILE file;
	OEMCHAR szFilename[MAX_PATH];
	OEMCHAR szShortFilename[64];
	UINT nOrder;
	BOOL bAssigned;
};
typedef struct tagHostDrvShortNameEntry HDRVSFNENTRY;

BRESULT hostdrvs_getshortnamemap(const OEMCHAR *lpPath, HDRVSFNENTRY **ppEntries, UINT *pnEntries);
void hostdrvs_freeshortnamemap(HDRVSFNENTRY *pEntries);
void hostdrvs_invalidateshortnamecache(void);
BOOL hostdrvs_lookupshortnamecached(const OEMCHAR *lpPath, const OEMCHAR *lpFilename,
								 OEMCHAR *lpShortName, UINT cchShortName);
BOOL hostdrvs_lookuplongnamecached(const OEMCHAR *lpPath, const OEMCHAR *lpShortName,
								 OEMCHAR *lpFilename, UINT cchFilename, UINT32 *lpAttr);
BOOL hostdrvs_lookupshortname(const HDRVSFNENTRY *pEntries, UINT nEntries,
							  const OEMCHAR *lpFilename, OEMCHAR *lpShortName, UINT cchShortName);
BOOL hostdrvs_lookuplongname(const HDRVSFNENTRY *pEntries, UINT nEntries,
							 const OEMCHAR *lpShortName, OEMCHAR *lpFilename, UINT cchFilename,
							 UINT32 *lpAttr);

LISTARRAY hostdrvs_getpathlist(const HDRVPATH *phdp, const char *lpMask, UINT nAttr);
UINT hostdrvs_getrealdir(HDRVPATH *phdp, char *lpFcbname, const char *lpDosPath);
UINT hostdrvs_appendname(HDRVPATH *phdp, const char *lpFcbname);
UINT hostdrvs_getrealpath(HDRVPATH *phdp, const char *lpDosPath);
BOOL hostdrvs_isroot(const HDRVPATH *phdp);
BOOL hostdrvs_issafehostpath(const OEMCHAR *lpPath);
void hostdrvs_fhdlallclose(LISTARRAY fileArray);
HDRVHANDLE hostdrvs_fhdlsea(LISTARRAY fileArray);

void hostdrvs_setshortnamemode(UINT nMode);
UINT hostdrvs_getshortnamemode(void);

#endif	/* defined(SUPPORT_HOSTDRV) */
