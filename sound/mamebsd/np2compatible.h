/**
 * @file	np2compatible.h
 * @brief	Interface of compatibility with old np2 mame opl
 */

#pragma once

#ifdef USE_MAME_BSD


#ifdef __cplusplus
extern "C" {
#endif

int YMF262FlagLoad_NP2REV97(opl3bsd* chipbsd, void* srcbuf, int size);

#ifdef __cplusplus
}
#endif

#endif