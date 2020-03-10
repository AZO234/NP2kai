#include	<sys/param.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<stdint.h>
#include	<stdbool.h>
#include	<setjmp.h>
#include	<stdarg.h>
#include	<stddef.h>
#include	<string.h>
#include	<unistd.h>
#include	<assert.h>
#include	<pthread.h>
#if defined(USE_SDL_CONFIG)
#include	"SDL.h"
#else
#include	<SDL2/SDL.h>
#endif

//#define TRACE

#define BYTESEX_LITTLE
#define	OSLANG_UTF8
#define	OSLINEBREAK_LF
#define RESOURCE_US
#define USE_TTF
#define SUPPORT_JOYSTICK

#define	sigjmp_buf				jmp_buf
#ifndef	sigsetjmp
#define	sigsetjmp(env, mask)	setjmp(env)
#endif
#ifndef	siglongjmp
#define	siglongjmp(env, val)	longjmp(env, val)
#endif

typedef	signed int		INT;

typedef	signed int		SINT;

typedef	unsigned char	UCHAR;
typedef	unsigned short	USHORT;
typedef	unsigned int	UINT;
typedef	unsigned long	ULONG;

typedef	signed char		SINT8;
typedef	signed char		INT8;
typedef	unsigned char	UINT8;
typedef	signed short	SINT16;
typedef	signed short	INT16;
typedef	unsigned short	UINT16;
typedef	signed int		SINT32;
typedef	signed int		INT32;
typedef	unsigned int	UINT32;
#if __WORDSIZE == 64
typedef signed long int   SINT64;
typedef signed long int   INT64;
typedef unsigned long int   UINT64;
#else
__extension__
typedef signed long int   SINT64;
__extension__
typedef signed long int   INT64;
__extension__
typedef unsigned long long int  UINT64;
#endif

typedef	bool				BOOL;
typedef	signed char		CHAR;
typedef	signed char		TCHAR;
typedef	unsigned char	BYTE;
typedef	unsigned int	DWORD;
typedef	unsigned short	WORD;


#ifndef	TRUE
#define	TRUE	true
#endif

#ifndef	FALSE
#define	FALSE	false
#endif

#ifndef	MAX_PATH
#define	MAX_PATH	MAXPATHLEN
#endif

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

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
#define	BYTESEX_BIG
#else /* SDL_BYTEORDER == SDL_LIL_ENDIAN */
#define	BYTESEX_LITTLE
#endif	/* SDL_BYTEORDER == SDL_BIG_ENDIAN */

#define	UNUSED(v)	((void)(v))

#ifndef	NELEMENTS
#define	NELEMENTS(a)	((int)(sizeof(a) / sizeof(a[0])))
#endif


// for ARM optimize
#define	REG8		UINT8
#define REG16		UINT16
#define	LOW12(a)	((((UINT32)(a)) << 20) >> 20)
#define	LOW14(a)	((((UINT32)(a)) << 18) >> 18)
#define	LOW15(a)	((((UINT32)(a)) << 17) >> 17)
#define	LOW16(a)	((UINT16)(a))
#define	HIGH16(a)	(((UINT32)(a)) >> 16)


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

#define _T
#define _tcscpy strcpy
#define	msgbox(title, msg)

#ifndef NP2_SIZE_VGA
#define NP2_SIZE_VGA
#endif

#if !defined(NP2_SIZE_VGA)
#define	RGB16		UINT32
#define	NP2_SIZE_QVGA
#endif


#include	"milstr.h"
#include	"_memory.h"
#include	"rect.h"
#include	"lstarray.h"
#include	"trace.h"


#define	GETTICK()			SDL_GetTicks()
#define	__ASSERT(s)
#define	SPRINTF				sprintf
#define	STRLEN				strlen
#define	SDL_main			main

#define	SOUNDCALL

#define	VERMOUTH_LIB
#define	SOUND_CRITICAL

#define	SUPPORT_UTF8

#define	SUPPORT_16BPP
#define	SUPPORT_32BPP

#define	SOUNDRESERVE	100
#define	OPNGENARM

//#define	CPUSTRUC_MEMWAIT

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
#define SUPPORT_IDEIO
#undef	SUPPORT_SCSI
#define	SUPPORT_HRTIMER

#define	SUPPORT_JOYSTICK
#define	USE_SDL_JOYSTICK

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
#define	SOUNDCALL
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
