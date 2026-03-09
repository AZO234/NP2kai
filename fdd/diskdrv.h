/**
 * @file	diskdrv.h
 * @brief	Interface of the disk-drive
 */

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

void diskdrv_setsxsi(REG8 drv, const OEMCHAR *fname);
const OEMCHAR *diskdrv_getsxsi(REG8 drv);
void diskdrv_hddbind(void);

void diskdrv_readyfddex(REG8 drv, const OEMCHAR *fname, UINT ftype, int readonly);
void diskdrv_setfddex(REG8 drv, const OEMCHAR *fname, UINT ftype, int readonly);
void diskdrv_callback(void);

#ifdef __cplusplus
}
#endif


/* macro */
#define diskdrv_readyfdd(d, f, r)	diskdrv_readyfddex(d, f, FTYPE_NONE, r)		/*!< set disk (force) */
#define diskdrv_setfdd(d, f, r)		diskdrv_setfddex(d, f, FTYPE_NONE, r)		/*!< set disk */
