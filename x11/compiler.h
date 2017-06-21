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

#ifdef	WORDS_BIGENDIAN
#define	BYTESEX_BIG
#else	/* !WORDS_BIGENDIAN */
#define	BYTESEX_LITTLE
#endif	/* WORDS_BIGENDIAN */

#if !defined(USE_SDLAUDIO) && !defined(USE_SDLMIXER)
#ifndef	NOSOUND
#define	NOSOUND
#undef	VERMOUTH_LIB
#endif	/* !NOSOUND */
#else	/* USE_SDLAUDIO || USE_SDLMIXER */
#undef	NOSOUND
#define	VERMOUTH_LIB
#endif	/* !USE_SDLAUDIO && !USE_SDLMIXER */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>

#define	X11
#define	OSLANG_UTF8
#define	OSLINEBREAK_LF

#include <glib.h>

typedef	gint32		SINT;
typedef	guint32		UINT;

typedef	gint8		SINT8;
typedef	gint16		SINT16;
typedef	gint32		SINT32;
typedef	gint64		SINT64;

typedef	guint8		UINT8;
typedef	guint16		UINT16;
typedef	guint32		UINT32;
typedef	guint64		UINT64;

typedef	gboolean	BOOL;

#define	INTPTR		gintptr

#define PTR_TO_UINT32(p)	((UINT32)GPOINTER_TO_UINT(p))
#define UINT32_TO_PTR(v)	GUINT_TO_POINTER((UINT32)(v))

#ifndef	FALSE
#define	FALSE	0
#endif

#ifndef	TRUE
#define	TRUE	(!FALSE)
#endif

#ifndef	MAX_PATH
#define	MAX_PATH	MAXPATHLEN
#endif

#ifndef	max
#define	max(a,b)	(((a) > (b)) ? (a) : (b))
#endif
#ifndef	min
#define	min(a,b)	(((a) < (b)) ? (a) : (b))
#endif

#ifndef	ZeroMemory
#define	ZeroMemory(d,n)		memset((d), 0, (n))
#endif
#ifndef	CopyMemory
#define	CopyMemory(d,s,n)	memcpy((d), (s), (n))
#endif
#ifndef	FillMemory
#define	FillMemory(a, b, c)	memset((a), (c), (b))
#endif

#ifndef	roundup
#define	roundup(x, y)	((((x)+((y)-1))/(y))*(y))
#endif

#ifndef	NELEMENTS
#define	NELEMENTS(a)	((int)(sizeof(a) / sizeof(a[0])))
#endif

/* archtecture */
/* amd64 */
#if defined(amd64) || defined(__AMD64__) || defined(__amd64__) || \
    defined(x86_64) || defined(__x86_64__) || defined(__X86_64__)
#define	NP2_CPU_ARCH_AMD64
#endif /* amd64 */
/* i386 */
#if defined(i386) || defined(__i386__) || defined(NP2_CPU_ARCH_AMD64)
#define	NP2_CPU_ARCH_IA32
#endif /* i386 */

#if defined(__GNUC__)
#if defined(NP2_CPU_ARCH_IA32)
#define	GCC_CPU_ARCH_IA32
#endif
#if defined(NP2_CPU_ARCH_AMD64)
#define	GCC_CPU_ARCH_AMD64
#endif
#endif /* __GNUC__ */

G_BEGIN_DECLS
UINT32 gettick(void);
G_END_DECLS
#define	GETTICK()	gettick()
#define	GETRAND()	random()
#define	SPRINTF		sprintf
#define	STRLEN		strlen

#define	OEMCHAR		gchar
#define OEMTEXT(s)	s
#define	OEMSPRINTF	sprintf
#define	OEMSTRLEN	strlen

#if defined(CPUCORE_IA32)
#define	msgbox(title, msg)	toolkit_messagebox(title, msg);

#if !defined(DISABLE_PC9821)
#define	SUPPORT_PC9821
#define	SUPPORT_CRT31KHZ
#endif
#define	SUPPORT_IDEIO
#else
#define	SUPPORT_CRT15KHZ
#endif

#if defined(NP2_CPU_ARCH_IA32)
#undef	MEMOPTIMIZE
#define LOADINTELDWORD(a)	(*((const UINT32 *)(a)))
#define LOADINTELWORD(a)	(*((const UINT16 *)(a)))
#define STOREINTELDWORD(a, b)	*(UINT32 *)(a) = (b)
#define STOREINTELWORD(a, b)	*(UINT16 *)(a) = (b)
#if !defined(DEBUG) && !defined(NP2_CPU_ARCH_AMD64)
#define	FASTCALL	__attribute__((regparm(2)))
#endif	/* !DEBUG && !NP2_CPU_ARCH_AMD64 */
#elif defined(arm) || defined (__arm__)
#define	MEMOPTIMIZE	2
#define	REG8		UINT
#define	REG16		UINT
#define	OPNGENARM
#else
#define	MEMOPTIMIZE	1
#endif

#ifndef	FASTCALL
#define	FASTCALL
#endif
#define	CPUCALL		FASTCALL
#define	MEMCALL		FASTCALL
#define	DMACCALL	FASTCALL
#define	IOOUTCALL	FASTCALL
#define	IOINPCALL	FASTCALL
#define	SOUNDCALL	FASTCALL
#define	VRAMCALL	FASTCALL
#define	SCRNCALL	FASTCALL
#define	VERMOUTHCL	FASTCALL

#ifdef	DEBUG
#define	INLINE
#define	__ASSERT(s)	assert(s)
#else
#ifndef	__ASSERT
#define	__ASSERT(s)
#endif
#ifndef	INLINE
#define	INLINE		inline
#endif
#endif

#define	SUPPORT_EUC
#define	SUPPORT_UTF8

#undef	SUPPORT_8BPP
#define	SUPPORT_16BPP
#define	SUPPORT_24BPP
#define	SUPPORT_32BPP
#define	SUPPORT_NORMALDISP

#undef	SOUND_CRITICAL
#undef	SOUNDRESERVE
#define	SUPPORT_EXTERNALCHIP

#define	SUPPORT_PC9861K
#define	SUPPORT_HOSTDRV

#define	SUPPORT_RESUME
#define	SUPPORT_STATSAVE

#undef	SUPPORT_SASI
#undef	SUPPORT_SCSI

#define	SUPPORT_S98
#define	SUPPORT_KEYDISP
#define	SUPPORT_SOFTKBD	0

#define	SUPPORT_SCREENSIZE

#if defined(USE_SDLAUDIO) || defined(USE_SDLMIXER)
#define	SUPPORT_JOYSTICK
#define	USE_SDL_JOYSTICK
#endif	/* USE_SDLAUDIO || USE_SDLMIXER */

/*
 * You could specify a complete path, e.g. "/etc/timidity.cfg", and
 * then specify the library directory in the configuration file.
 */
extern char timidity_cfgfile_path[MAX_PATH];
#define	TIMIDITY_CFGFILE	timidity_cfgfile_path

#include "common.h"
#include "milstr.h"
#include "_memory.h"
#include "rect.h"
#include "lstarray.h"
#include "trace.h"
#include "toolkit.h"

#endif	/* NP2_X11_COMPILER_H__ */
