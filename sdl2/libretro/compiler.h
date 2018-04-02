/**
 * @file	compiler.h
 * @brief	include file for standard system include files,
 *			or project specific include files that are used frequently,
 *			but are changed infrequently
 */

#pragma once

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <setjmp.h>
#include <stdarg.h>
#include <string.h>

#include "boolean.h"
#include "features/features_cpu.h"

#include "sdlremap/sdl.h"
#include "sdlremap/sdl_keycode.h"

//#define TRACE
#ifndef MSB_FIRST
#define	BYTESEX_LITTLE
#else
#define	BYTESEX_BIG
#endif
#define	OSLANG_UTF8
#define	OSLINEBREAK_CRLF
#define  RESOURCE_US
#ifdef _WIN32
#define _WINDOWS
#include <windows.h>
#endif
#define	sigjmp_buf				jmp_buf
#ifndef	sigsetjmp
#define	sigsetjmp(env, mask)	setjmp(env)
#endif
#ifndef	siglongjmp
#define	siglongjmp(env, val)	longjmp(env, val)
#endif

typedef	int32_t		SINT;
typedef	int32_t		INT;
typedef	uint32_t    UINT;

typedef	int8_t		SINT8;
typedef	int8_t		INT8;
typedef	uint8_t		UINT8;
typedef	int16_t		SINT16;
typedef	int16_t		INT16;
typedef	uint16_t		UINT16;
typedef	int32_t		SINT32;
typedef	int32_t		INT32;
typedef	uint32_t		UINT32;
typedef	int64_t		SINT64;
typedef	int64_t		INT64;
typedef	uint64_t		UINT64;

typedef  int32_t*    INTPTR;

typedef	unsigned char	BYTE;
#if !defined(_WIN32)
typedef	uint32_t	DWORD;
typedef	uint16_t	WORD;
#endif	/* _WIN32 */

#ifdef _WIN32
typedef int BOOL;
typedef	char CHAR;
#else
typedef bool BOOL;
typedef int8_t		CHAR;
typedef	signed char		TCHAR;
#endif
#define	REG8		UINT8
#define REG16		UINT16

#ifndef	TRUE
#define	TRUE	true
#endif
#ifndef	FALSE
#define	FALSE	false
#endif

#define	BRESULT				UINT
#define	OEMCHAR				char
#define	OEMTEXT(string)	string
#define	OEMSPRINTF			sprintf
#define	OEMSTRLEN			strlen

#define	SPRINTF		sprintf
#define	STRLEN		strlen
#define	GETRAND()	random()

#define NP2_SIZE_VGA

#ifdef _WIN32
#define G_DIR_SEPARATOR '\\'
#else
#define G_DIR_SEPARATOR '/'
#endif

#ifndef	MAX_PATH
#define	MAX_PATH	4096
#endif

#ifndef	np2max
#define	np2max(a,b)	(((a) > (b)) ? (a) : (b))
#endif
#ifndef	np2min
#define	np2min(a,b)	(((a) < (b)) ? (a) : (b))
#endif

#ifndef	ZeroMemory
#define	ZeroMemory(d,n)		memset((d), 0, (n))
#endif
#ifndef	CopyMemory
#define	CopyMemory(d,s,n)    memcpy((d), (s), (n))
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

//#define	msgbox(title, msg)	toolkit_messagebox(title, msg);
#define	msgbox(title, msg)

#include "milstr.h"
#include "_memory.h"
#include "rect.h"
#include "lstarray.h"
#include "trace.h"


#define	GETTICK()         (cpu_features_get_time_usec() / 1000)//millisecond timer
#define	__ASSERT(s)
#define	SPRINTF				sprintf
#define	STRLEN				strlen
#define	_tcsicmp	strcasecmp
#define	_tcsnicmp	strncasecmp

#if defined(SUPPORT_LARGE_HDD)
typedef SINT64	FILEPOS;
typedef SINT64	FILELEN;
#define	NHD_MAXSIZE		8000
#define	NHD_MAXSIZE2	32000
#else
typedef SINT32	FILEPOS;
typedef SINT32	FILELEN;
#define	NHD_MAXSIZE		2000
#define	NHD_MAXSIZE2	2000
#endif

#define _T
#define _tcscpy strcpy

#define	MEMOPTIMIZE		2

#define	VERMOUTH_LIB

#define	SUPPORT_SWSEEKSND
#if defined(SUPPORT_WAB)
#define	SUPPORT_SWWABRLYSND
#endif

#define	SUPPORT_16BPP
#define	SUPPORT_32BPP

//extras
#define	SUPPORT_EUC
#define	SUPPORT_UTF8

#define	SUPPORT_CRT31KHZ

#if defined(SUPPORT_PC9821)
#define	IA32_PAGING_EACHSIZE
#define	SUPPORT_PC9801_119
#else
#define SUPPORT_BMS
#endif

#define	SUPPORT_PC9861K
#define	SUPPORT_SOFTKBD		0
#define  SUPPORT_S98

#define	SUPPORT_KEYDISP

#define	SUPPORT_HOSTDRV
#define	SUPPORT_IDEIO
#undef	SUPPORT_SASI
#undef	SUPPORT_SCSI

#define	SUPPORT_RESUME
//#define	SUPPORT_STATSAVE	10
#define	SUPPORT_ROMEO
#define	SUPPORT_HRTIMER

#define  SUPPORT_ARC

#define  SUPPORT_FMGEN

#define	SUPPORT_NORMALDISP

#define SUPPORT_PX
#define SUPPORT_V30ORIGINAL
#define SUPPORT_V30EXT
#define VAEG_FIX
//#define SUPPORT_WAVEREC

//unused paramaters

//this is only used for threading
//#define  SOUND_CRITICAL
//#define	SOUNDRESERVE	100

//this is not a debug build
//#define	SUPPORT_MEMDBG32

//retroarch does these on its own
//#define  SUPPORT_WAVEREC
//#define  SUPPORT_RECVIDEO
//#define  SUPPORT_ZLIB

//the emulator must be self contained to work with retroarch
//#define	SUPPORT_EXTERNALCHIP

//old functions
//#define PTR_TO_UINT32(p)	((UINT32)GPOINTER_TO_UINT(p))
//#define UINT32_TO_PTR(v)	GUINT_TO_POINTER((UINT32)(v))

//outdated things to ignore
#define	FASTCALL
#define	CPUCALL
#define	MEMCALL
#define	DMACCALL
#define	IOOUTCALL
#define	IOINPCALL
#define	SOUNDCALL
#define	VRAMCALL
#define	SCRNCALL
#define	VERMOUTHCL

#include "retro_inline.h"
#include "common.h"

