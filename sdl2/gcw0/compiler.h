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

#define	__ASSERT(s)

#define USE_TTF

#define	OSLINEBREAK_LF
#define RESOURCE_US

#define NP2_SIZE_VGA
#if !defined(NP2_SIZE_VGA)
#define	RGB16		UINT32
#define	NP2_SIZE_QVGA
#endif

#define	GETTICK()			SDL_GetTicks()
#define	SDL_main			main

#define	SOUND_CRITICAL

#define	SOUNDRESERVE	100

//#define	CPUSTRUC_MEMWAIT

#define	SUPPORT_HRTIMER

#define	SUPPORT_JOYSTICK
#define	USE_SDL_JOYSTICK

#include "milstr.h"
#include "trace.h"

#endif  // COMPILER_H

