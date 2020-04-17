/**
 * @file	np2ver.h
 * @brief	The version
 */

#if defined(SUPPORT_IA32_HAXM)
#define	NP2VER_CORE			"ver.0.86 kai rev.21 HAXM"
#else
#define	NP2VER_CORE			"ver.0.86 kai rev.21"
#endif
#if defined(_MSC_VER) && !defined(__LIBRETRO__)
#include "np2_git_version.h"
#define	NP2VER_GIT			VER_SHASH " " VER_DSTR
#else
#define	NP2VER_GIT			GIT_VERSION
#endif

// #define	NP2VER_WIN9X
// #define	NP2VER_MACOSX
// #define	NP2VER_X11
// #define	NP2VER_SDL2

