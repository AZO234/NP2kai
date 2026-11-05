/**
 * @file	fdd_live.h
 * @brief	Floppy images served from a real drive
 */

#pragma once

#include <compiler.h>

#if defined(SUPPORT_NP2_THREAD) && defined(__LIBRETRO__)
#define SUPPORT_FDD_LIVE
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern BOOL fddlive_enable;

BOOL fddlive_wait(void);
void fddlive_callback(void);
void fddlive_shutdown(void);

#ifdef __cplusplus
}
#endif
