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

#if USE_SDL_VERSION >= 3
#include	<SDL3/SDL.h>
#elif USE_SDL_VERSION == 2
#include	<SDL2/SDL.h>
#elif USE_SDL_VERSION == 1
#include	<SDL/SDL.h>
#endif

#define	msgbox(title, msg)

#define	__ASSERT(s)

#define RESOURCE_US

#define NP2_SIZE_VGA

#define	GETTICK()			SDL_GetTicks()
#define	SDL_main			main

//#define	CPUSTRUC_MEMWAIT

#define	USE_SDL_JOYSTICK

#include <common/milstr.h>
#include <trace.h>

#endif  // COMPILER_H

