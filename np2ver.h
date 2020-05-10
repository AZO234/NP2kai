/**
 * @file	np2ver.h
 * @brief	The version
 */

#if defined(SUPPORT_IA32_HAXM)
#define	NP2VER_CORE			"0.86 HAXM"
#else
#define	NP2VER_CORE			"0.86"
#endif
#if defined(_MSC_VER) && !defined(__LIBRETRO__)
#include "np2_git_version.h"
#define	NP2VER_GIT			VER_SHASH
#else
#define	NP2VER_GIT			GIT_VERSION
#endif
