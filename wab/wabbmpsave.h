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

#if !defined(NP2_X11) && !defined(NP2_SDL2) && !defined(__LIBRETRO__)
BRESULT np2wab_getbmp(BMPFILE *lpbf, BMPINFO *lpbi, UINT8 **lplppal, UINT8 **lplppixels);
BRESULT np2wab_writebmp(const OEMCHAR *filename);
#endif

#ifdef __cplusplus
}
#endif

