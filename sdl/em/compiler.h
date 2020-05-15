#ifndef COMPILER_H
#define COMPILER_H

#include "compiler_base.h"

#include	<sys/param.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<setjmp.h>
#include	<stdarg.h>
#include	<stddef.h>
#include	<string.h>
#include	<unistd.h>
#include	<assert.h>

#include	<SDL.h>

#ifdef EMSCRIPTEN
#include <emscripten.h>
#endif

#define	msgbox(title, msg)

#define	GETTICK()			SDL_GetTicks()
#define	__ASSERT(s)
#define	SPRINTF				sprintf
#define	STRLEN				strlen
#define	SDL_main			main

#include "common/milstr.h"
#include	"trace.h"

#define EMSCRIPTEN_DIR "/emulator/np2kai/"

#endif  // COMPILER_H

