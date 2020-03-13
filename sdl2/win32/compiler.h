#ifndef COMPILER_H
#define COMPILER_H

#include	<windows.h>
#include	<stdio.h>
#include	<stdint.h>
#include	<stdbool.h>
#include	<setjmp.h>
#include	<stddef.h>
#include	<limits.h>
#include	<SDL.h>

//#define TRACE

#define _WINDOWS

#define	BYTESEX_LITTLE
#define	OSLANG_UTF8
#define	OSLINEBREAK_CRLF
#define	RESOURCE_US
#define	USE_TTF

#define	sigjmp_buf				jmp_buf
#ifndef	sigsetjmp
#define	sigsetjmp(env, mask)	setjmp(env)
#endif
#ifndef	siglongjmp
#define	siglongjmp(env, val)	longjmp(env, val)
#endif

//typedef	bool				BOOL;
#ifndef __GNUC__
typedef	signed int			SINT;
typedef	signed char			SINT8;
typedef	signed char			INT8;
typedef	unsigned char		UINT8;
typedef	signed short		SINT16;
typedef	signed short		INT16;
typedef	unsigned short		UINT16;
typedef	signed int			SINT32;
typedef	signed int			INT32;
typedef	unsigned int		UINT32;
typedef	signed long long	SINT64;
typedef	signed long long	INT64;
typedef	unsigned long long	UINT64;
typedef	signed int			SINT;
typedef	signed int			INT;
typedef	unsigned int		UINT;
#else
#include	<stdlib.h>
typedef	signed char			SINT8;
typedef	signed char			INT8;
typedef	unsigned char		UINT8;
typedef	signed short		SINT16;
typedef	signed short		INT16;
typedef	unsigned short		UINT16;
typedef	signed int			SINT32;
typedef	signed int			INT32;
typedef	unsigned int		UINT32;
typedef	signed long long	SINT64;
typedef	signed long long	INT64;
typedef	unsigned long long	UINT64;
typedef	signed int			SINT;
typedef	signed int			INT;
typedef	unsigned int		UINT;
#endif

#define	REG8		UINT8
#define REG16		UINT16

#define NP2_64_COPY(pd, ps) *pd = *ps;

#define	BRESULT				UINT
#define	OEMCHAR				char
#define	OEMTEXT(string)		string
#define	OEMSPRINTF			sprintf
#define	OEMSTRLEN			strlen
#define	_tcsicmp	strcasecmp
#define	_tcsnicmp	strncasecmp

#ifndef	TRUE
#define	TRUE	true
#endif

#ifndef	FALSE
#define	FALSE	false
#endif

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

#define NP2_SIZE_VGA
#if !defined(NP2_SIZE_VGA)
#define	RGB16		UINT32
#define	NP2_SIZE_QVGA
#endif


#include	"milstr.h"
#include	"_memory.h"
#include	"rect.h"
#include	"lstarray.h"
#include	"trace.h"

#ifndef	np2max
#define	np2max(a,b)	(((a) > (b)) ? (a) : (b))
#endif
#ifndef	np2min
#define	np2min(a,b)	(((a) < (b)) ? (a) : (b))
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

#define	SOUNDCALL	__fastcall

#define	VERMOUTH_LIB

#define	SUPPORT_UTF8

#define	SUPPORT_16BPP
#define	SUPPORT_32BPP
#define	MEMOPTIMIZE		2

#define SOUND_CRITICAL
#define	SOUNDRESERVE	100

#define _T
#define _tcscpy strcpy
#define	msgbox(title, msg)
#define	sigjmp_buf	jmp_buf
//#define	siglongjmp(env, val)	longjmp(env, val)
//#define	sigsetjmp(env, mask)	setjmp(env)

#define	SUPPORT_PC9861K
#define	SUPPORT_CRT15KHZ
#if defined(SUPPORT_PC9821)
#define	IA32_PAGING_EACHSIZE
#define	IA32_REBOOT_ON_PANIC
#define	SUPPORT_CRT31KHZ
#define	SUPPORT_PC9801_119
#else
#define SUPPORT_BMS
#endif
#define	SUPPORT_HOSTDRV
#define	SUPPORT_SWSEEKSND
#if defined(SUPPORT_WAB)
#define	SUPPORT_SWWABRLYSND
#endif
#undef	SUPPORT_SASI
#undef	SUPPORT_SCSI
#define SUPPORT_IDEIO
#define	SUPPORT_HRTIMER

#undef SUPPORT_EXTERNALCHIP

#define	SUPPORT_STATSAVE

#define SUPPORT_PX
#define SUPPORT_V30ORIGINAL
#define SUPPORT_V30EXT
#define VAEG_FIX
//#define SUPPORT_WAVEREC

#define	FASTCALL
#define	CPUCALL
#define	MEMCALL
#define	DMACCALL
#define	IOOUTCALL
#define	IOINPCALL
//#define	SOUNDCALL
#define	VRAMCALL
#define	SCRNCALL
#define	VERMOUTHCL
#define	INLINE inline

#if defined(SUPPORT_LARGE_MEMORY)
#define	MEMORY_MAXSIZE		4000
#else
#define	MEMORY_MAXSIZE		230
#endif

#include	"common.h"

#endif  // COMPILER_H

