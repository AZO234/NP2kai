/*-
 * Copyright (C) 2003, 2004 NONAKA Kimihiro <nonakap@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef	NP2_X11_COMPILER_H__
#define	NP2_X11_COMPILER_H__

#include "compiler_base.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef ENABLE_NLS
#include <libintl.h>
#define _(s)				gettext(s)
#ifdef gettext_noop
#define N_(s)				gettext_noop(s)
#else
#define N_(s)				(s)
#endif
#else /* !ENABLE_NLS */
#define _(s)				(s)
#define N_(s) (s)
#define textdomain(s)			(s)
#define gettext(s)			(s)
#define dgettext(d,s)			(s)
#define dcgettext(d,s,t)		(s)
#define bindtextdomain(d,dir)		(d)
#define bind_textdomain_codeset(d,c)	(c)
#endif /* ENABLE_NLS */

#if !defined(USE_SDLAUDIO) && !defined(USE_SDLMIXER) && !defined(USE_SDL2AUDIO) && !defined(USE_SDL2MIXER)
#ifndef	NOSOUND
#define	NOSOUND
#undef	VERMOUTH_LIB
#endif	/* !NOSOUND */
#else
#undef	NOSOUND
#endif

#include <sys/param.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#define	X11

#include <glib.h>

#define PTR_TO_UINT32(p)	((UINT32)GPOINTER_TO_UINT(p))
#define UINT32_TO_PTR(v)	GUINT_TO_POINTER((UINT32)(v))

G_BEGIN_DECLS
UINT32 gettick(void);
G_END_DECLS
#define	GETTICK()	gettick()

#define	msgbox(title, msg)	toolkit_messagebox(title, msg);

#ifndef __ASSERT
#ifdef  DEBUG
#define __ASSERT(s)	assert(s)
#else
#define __ASSERT(s)
#endif
#endif

#define USE_TTF

#define	SUPPORT_EUC

#define	SUPPORT_KEYDISP
#define	SUPPORT_SOFTKBD	0

#define	SUPPORT_SCREENSIZE

#if defined(USE_SDLAUDIO) || defined(USE_SDLMIXER) || defined(USE_SDL2AUDIO) || defined(USE_SDL2MIXER)
#define	USE_SDL_JOYSTICK
#endif

#define SUPPORT_WAVEREC

/*
 * You could specify a complete path, e.g. "/etc/timidity.cfg", and
 * then specify the library directory in the configuration file.
 */
extern char timidity_cfgfile_path[MAX_PATH];
#define	TIMIDITY_CFGFILE	timidity_cfgfile_path

#include "common/milstr.h"
#include "x11/trace.h"
#include "x11/toolkit.h"

#endif	/* NP2_X11_COMPILER_H__ */
