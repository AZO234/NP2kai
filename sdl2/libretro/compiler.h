/**
 * @file	compiler.h
 * @brief	include file for standard system include files,
 *			or project specific include files that are used frequently,
 *			but are changed infrequently
 */

#ifndef COMPILER_H
#define COMPILER_H

#include "features/features_cpu.h"
#include "compiler_base.h"

#include "sdlremap/sdl.h"
#include "sdlremap/sdl_keycode.h"

#define	msgbox(title, msg)

#define __ASSERT(s)

#define	OSLINEBREAK_LF
#define RESOURCE_US

#define NP2_SIZE_VGA

#define GETTICK() (cpu_features_get_time_usec() / 1000)  // millisecond timer

//#include "retro_inline.h"
#include "common/milstr.h"
#include "sdl2/trace.h"

#endif  // COMPILER_H

