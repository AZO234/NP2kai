#include	<windows.h>
#include	<stdio.h>
#include	<stddef.h>
#include	<SDL2\SDL.h>

#define _WINDOWS

#define	BYTESEX_LITTLE
#define	OSLANG_UTF8
#define	OSLINEBREAK_CRLF

#ifndef __GNUC__
typedef	signed int			SINT;
typedef	signed char			SINT8;
typedef	unsigned char		UINT8;
typedef	signed short		SINT16;
typedef	unsigned short		UINT16;
typedef	signed int			SINT32;
typedef	signed int			INT32;
typedef	unsigned int		UINT32;
typedef	signed long long	SINT64;
typedef	unsigned long long	SINT64;
typedef	signed int			SINT;
typedef	unsigned int		UINT;
#else
#include	<stdlib.h>
typedef	signed char			SINT8;
typedef	unsigned char		UINT8;
typedef	signed short		SINT16;
typedef	unsigned short		UINT16;
typedef	signed int			SINT32;
typedef	signed int			INT32;
typedef	unsigned int		UINT32;
typedef	signed long long	SINT64;
typedef	unsigned long long	UINT64;
typedef	signed int			SINT;
typedef	unsigned int		UINT;
#endif

#define	REG8		UINT8
#define REG16		UINT16

#ifndef _SSIZE_T_DEFINED
#define _SSIZE_T_DEFINED
typedef LONG_PTR ssize_t;
#endif

#define	BRESULT				UINT
#define	OEMCHAR				char
#define	OEMTEXT(string)		string
#define	OEMSPRINTF			sprintf
#define	OEMSTRLEN			strlen
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

#define SIZE_VGA
#if !defined(SIZE_VGA)
#define	RGB16		UINT32
#define	SIZE_QVGA
#endif


#include	"common.h"
#include	"milstr.h"
#include	"_memory.h"
#include	"rect.h"
#include	"lstarray.h"
#include	"trace.h"

#ifndef	max
#define	max(a,b)	(((a) > (b)) ? (a) : (b))
#endif
#ifndef	min
#define	min(a,b)	(((a) < (b)) ? (a) : (b))
#endif

#ifndef	ZeroMemory
#define	ZeroMemory(d,n)		bzero((d),(n))
#endif
#ifndef	CopyMemory
#define	CopyMemory(d,s,n)	memcpy((d),(s),(n))
#endif
#ifndef	FillMemory
#define	FillMemory(a, b, c)	memset((a),(c),(b))
#endif

#ifndef	roundup
#define	roundup(x, y)	((((x)+((y)-1))/(y))*(y))
#endif

#define	GETTICK()			SDL_GetTicks()
#define	__ASSERT(s)
#define	SPRINTF				sprintf
#define	STRLEN				strlen

#define	VERMOUTH_LIB
// #define	SOUND_CRITICAL

#define	SUPPORT_UTF8

#define	SUPPORT_16BPP
#define	MEMOPTIMIZE		2

#define SOUND_CRITICAL
#define	SOUNDRESERVE	100

#define _T
#define _tcscpy strcpy
#define	msgbox(title, msg)
#define	sigjmp_buf	jmp_buf
#define	siglongjmp(env, val)	longjmp(env, val)
#define	sigsetjmp(env, mask)	setjmp(env)

#define	SUPPORT_PC9861K
#define	SUPPORT_CRT15KHZ
#if defined(SUPPORT_PC9821)
#define	SUPPORT_CRT31KHZ
#define	SUPPORT_PC9801_119
#endif
#define	SUPPORT_HOSTDRV
#define	SUPPORT_SWSEEKSND
#undef	SUPPORT_SASI
#undef	SUPPORT_SCSI
#define SUPPORT_IDEIO
#define	SUPPORT_HRTIMER

#undef SUPPORT_EXTERNALCHIP

#define	SUPPORT_STATSAVE

#define SUPPORT_ARC
#define SUPPORT_ZLIB

#define	SCREEN_BPP		16
