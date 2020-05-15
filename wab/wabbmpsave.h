/**
 * @file	wabbmpsave.h
 * @brief	Window Accelerator Board BMP Save
 *
 * @author	$Author: SimK $
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <common/bmpdata.h>

BRESULT np2wab_getbmp(BMPFILE *lpbf, BMPINFO *lpbi, UINT8 **lplppal, UINT8 **lplppixels);
BRESULT np2wab_writebmp(const OEMCHAR *filename);

#ifdef __cplusplus
}
#endif

