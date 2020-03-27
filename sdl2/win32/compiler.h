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

//#define TRACE
#define	msgbox(title, msg)

#define __ASSERT(s)

#define USE_TTF

#define	SUPPORT_SJIS
#define	OSLINEBREAK_CRLF

#define	GETTICK()			SDL_GetTicks()
#define	SDL_main			main

#define	SOUNDRESERVE	100

//#define	CPUSTRUC_MEMWAIT

#define	SUPPORT_HRTIMER

#define	USE_SDL_JOYSTICK

#include "milstr.h"
#include "trace.h"

#endif  // COMPILER_H

