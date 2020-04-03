/**
 * @file	compiler.h
 * @brief	include file for standard system include files,
 *			or project specific include files that are used frequently,
 *			but are changed infrequently
 */

#ifndef COMPILER_H
#define COMPILER_H

#include "compiler_base.h"

#include	<pthread.h>
#include	<SDL.h>

#define	msgbox(title, msg)

#define	__ASSERT(s)

#define USE_TTF

#define	OSLINEBREAK_LF
#define RESOURCE_US

#define NP2_SIZE_VGA

#define	GETTICK()			SDL_GetTicks()
#define	SDL_main			main

//#define	CPUSTRUC_MEMWAIT

#define	USE_SDL_JOYSTICK

#include "common/milstr.h"
#include "sdl2/trace.h"

#endif  // COMPILER_H

