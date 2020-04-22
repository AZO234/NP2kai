/* === compiler base header === (c) 2020 AZO */

// for VC2013, ICC12, GCC4, Clang3

#ifndef _COMPILER_BASE_H_
#define _COMPILER_BASE_H_

#if defined(__LIBRETRO__)
#include <libretro.h>
#endif

// secure
#if defined(__MINGW32__) || defined(__MINGW64__)
#define MINGW_HAS_SECURE_API 1
#endif

// Windows
#if defined(_WIN32) || defined(_WIN64) || defined(__MINGW32__) || defined(__MINGW64__) || defined(__CYGWIN__)
#if !defined(_WINDOWS)
#define _WINDOWS
#endif
#endif

/* archtecture */
#if defined(amd64) || defined(__AMD64__) || defined(__amd64__) || \
    defined(x86_64) || defined(__x86_64__) || defined(__X86_64__) || \
    defined(__aarch64__) || defined(_WIN64) || defined(_M_X64) || \
    defined(__LP64__) || defined(__LLP64__) || defined(__LLP64__)
#define	NP2_CPU_64BIT
#endif
#if defined(i386) || defined(__i386__) || defined(__arm__) || \
    defined(_WIN32) || defined(_M_IX86) || \
    defined(NP2_CPU_ARCH_AMD64)
#define	NP2_CPU_32BIT
#endif

#if defined(amd64) || defined(__AMD64__) || defined(__amd64__) || \
    defined(x86_64) || defined(__x86_64__) || defined(__X86_64__)
#define	NP2_CPU_ARCH_AMD64
#endif
#if defined(i386) || defined(__i386__) || defined(NP2_CPU_ARCH_AMD64)
#define	NP2_CPU_ARCH_IA32
#endif
#if defined(__GNUC__)
#if defined(NP2_CPU_ARCH_IA32)
#define	GCC_CPU_ARCH_IA32
#endif
#if defined(NP2_CPU_ARCH_AMD64)
#define	GCC_CPU_ARCH_AMD64
#endif
#endif

// standard include
#if defined(_WINDOWS)
#include <windows.h>
#include <tchar.h>
// not define _UNICODE, UNICODE now
#endif
#if defined(__cplusplus)
#include <cstdio>
#include <cstdlib>  // include cwchar
#include <cstddef>
#include <cstring>
#include <cmath>
#include <climits>
#include <csetjmp>
#include <cstdarg>
#else
#include <stdio.h>
#include <stdlib.h>  // include wchar.h
#include <stddef.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <setjmp.h>
#include <stdarg.h>
#endif

// C/C++ standard
#if defined(__cplusplus)
#if defined(__APPLE__)
#include <cstdint>
#include <inttypes.h>
#if !defined(CPP11)
#define CPP11
#endif
#elif defined(_MSC_VER)
#include <inttypes.h>
#include <stdint.h>
#if !defined(CPP11)
#define CPP11
#endif
#else
#if __cplusplus >= 201103L
#include <cinttypes>
#include <cstdint>
#if !defined(CPP11)
#define CPP11
#endif
#endif
#if defined(__GXX_EXPERIMENTAL_CXX0X__)
#if !defined(CPP0X)
#define CPP0X
#endif
#endif
#endif
#if !defined(_MSC_VER)
#if !defined(SUPPORT_SNPRINTF)
#define SUPPORT_SNPRINTF
#endif
#endif
#else
#if defined(_MSC_VER)
#if _MSC_VER >= 1800
#include <inttypes.h>
#include <stdint.h>
#if !defined(C99)
#define C99
#endif
// _MSC_VER<1900(older VC2015) not support snprintf()
#elif _MSC_VER >= 1900
#if !defined(SUPPORT_SNPRINTF)
#define SUPPORT_SNPRINTF
#endif
#endif
#elif defined(__STDC_VERSION__)
#if __STDC_VERSION__ >= 199901L
#include <inttypes.h>
#if !defined(C99)
#define C99
#endif
#if !defined(SUPPORT_SNPRINTF)
#define SUPPORT_SNPRINTF
#endif
#endif
#if __STDC_VERSION__ >= 201112L
#if !defined(C11)
#define C11
#endif
#if !defined(SUPPORT_STRNLENS)
//#define SUPPORT_STRNLENS
#endif
#endif
#endif
#endif

#if __STDC_VERSION__ >= 199901L
#define _XOPEN_SOURCE 600
#else
#define _XOPEN_SOURCE 500
#endif /* __STDC_VERSION__ */

// size fixed integer
#if !defined(__cplusplus) && !defined(C99)
typedef char               int8_t;
typedef unsigned char      uint8_t;
typedef short              int16_t;
typedef unsigned short     uint16_t;
typedef long               int32_t;
typedef unsigned long      uint32_t;
#if defined(NP2_CPU_64BIT)
typedef long long          int64_t;   // literal: nnnLL  format: %PRId64
typedef unsigned long long uint64_t;  // literal: nnnULL format: %PRIu64
#else
typedef int32_t            int64_t;
typedef uint32_t           uint64_t;
#endif
#if defined(NP2_CPU_64BIT)
typedef int64_t  intptr_t;
typedef uint64_t uintptr_t;
typedef int64_t  intmax_t;
typedef uint64_t uintmax_t;
#else
typedef int32_t  intptr_t;
typedef uint32_t uintptr_t;
typedef int32_t  intmax_t;
typedef uint32_t uintmax_t;
#endif
#endif

#if !defined(_MSC_VER)
typedef	int32_t  INT;
typedef	uint32_t UINT;
typedef	int8_t   INT8;
typedef	uint8_t  UINT8;
typedef	int32_t  INT32;
typedef	uint32_t UINT32;
#endif
typedef	INT      SINT;
typedef	INT8     SINT8;
typedef	int16_t  INT16;
typedef	INT16    SINT16;
typedef	uint16_t UINT16;
typedef	INT32    SINT32;
#if defined(NP2_CPU_64BIT)
typedef	int64_t  INT64;
typedef	uint64_t UINT64;
typedef	INT64    SINT64;
#else
#if !defined(__MINGW32__)  // for libretro
//#if !defined(_WINDOWS)  // for me
typedef	int32_t  INT64;
typedef	uint32_t UINT64;
#endif
typedef	INT32    SINT64;
#endif

// variable size
typedef size_t    SIZET;    // format: %zu
typedef intptr_t  INTPTR;   // format: %PRIdPTR
typedef uintptr_t UINTPTR;  // format: %PRIuPTR
typedef intmax_t  INTMAX;   // format: %PRIdMAX
typedef uintmax_t UINTMAX;  // format: %PRIuMAX

// bool
#if defined(__cplusplus)
#if !defined(_WINDOWS)  // BOOL typedefed as int in winnt.h
typedef bool BOOL;
#endif
#if !defined(TRUE)
#define TRUE  true
#endif
#if !defined(FALSE)
#define FALSE false
#endif
#else
#if defined(C99)
#include <stdbool.h>
#if !defined(_WINDOWS)  // BOOL typedefed as int in winnt.h
typedef bool BOOL;
#endif
#if !defined(TRUE)
#define TRUE  true
#endif
#if !defined(FALSE)
#define FALSE false
#endif
#else
typedef int  BOOL;
#if !defined(TRUE)
#define TRUE  (1==1)
#endif
#if !defined(FALSE)
#define FALSE (1==0)
#endif
#endif
#endif

// inline
#if !defined(DEBUG) || !defined(_DEBUG)
#if !defined(INLINE)
#if defined(_MSC_VER)
#pragma warning(disable: 4244)
#pragma warning(disable: 4245)
#define INLINE __inline
#elif defined(__BORLANDC__)
#define INLINE __inline
#elif defined(__GNUC__)
#define INLINE __inline__ __attribute__((always_inline))
#else
#define INLINE
#endif
#endif
#else
#undef  INLINE
#define INLINE
#endif

// pi
#ifndef M_PI
#define M_PI  3.14159265358979323846
#endif
#ifndef M_PI
#define M_PIl 3.1415926535897932384626433832795029L
#endif

// --> milstr OEMCHAR
#if defined(_WINDOWS)
#define	OEMNEWLINE  "\r\n"
#define	OEMPATHDIV  "\\"
#define	OEMPATHDIVC '\\'
#else
#define	OEMNEWLINE  "\n"
#define	OEMPATHDIV  "/"
#define	OEMPATHDIVC '/'
#endif
#define	OEMSLASH   "/"
#define	OEMSLASHC  '/'

#if defined(SUPPORT_STRNLENS)
#define	OEMSTRNLENS       strnlen_s
#define	OEMSTRNLEN        strnlen_s
#elif defined(C99) || defined(CPP11)
#define	OEMSTRNLENS       strnlen
#define	OEMSTRNLEN        strnlen
#else
#define	OEMSTRNLENS(s, z) strlen(s)
#define	OEMSTRNLEN(s, z)  strlen(s)
#endif
#define	OEMSTRLEN         strlen
#define	STRNLENS          OEMSTRNLENS
#define	STRNLEN           OEMSTRNLEN
#define	STRLEN            OEMSTRLEN

#if defined(SUPPORT_SNPRINTF)
#define	OEMSNPRINTF               snprintf
#else
#if defined(C99) || defined(CPP11)
#define	OEMSNPRINTF(s, z, f, ...) sprintf(s, f, __VA_ARGS__)
#else
#define	OEMSNPRINTF(s, z, f, d)   sprintf(s, f, d)
#endif
#endif
#define	OEMSPRINTF                sprintf
#define	SNPRINTF                  OEMSNPRINTF
#define	SPRINTF                   OEMSPRINTF

#define	OEMSTRCPY(s1, s2) OEMSPRINTF(s1, OEMTEXT("%s"), s2)
#define	OEMPRINTFSTR(s)   printf(OEMTEXT("%s"), s)

// future depracted maybe
#define	OEMCHAR         char
#define	OEMTEXT(string) string

#define STRCALL
// <-- milstr OEMCHAR

// --> Windowsnize
// calling convention
#undef  CDECL
#undef  STDCALL
#undef  CLRCALL
#undef  FASTCALL
#undef  VECTORCALL
#if defined(__cpluscplus)
#undef  THISCALL
#endif

#if defined(_MSC_VER) && defined(_M_IX86) && !defined(LR_VS2017)
#define CDECL      __cdecl
#define STDCALL    __stdcall
#define FASTCALL   __fastcall
#define SAFECALL   __safecall
#define CLRCALL    __clrcall
#define VECTORCALL __vectorcall
#if defined(__cpluscplus)
#define THISCALL   __thiscall
#endif
#elif defined(__GNUC__) && defined(__i386__) && !defined(__ANDROID__)
#define CDECL      __attribute__ ((cdecl))
#define STDCALL    __attribute__ ((stdcall))
#define FASTCALL   __attribute__ ((fastcall))
#define CLRCALL
#define VECTORCALL __attribute__ ((interrupt))
#if defined(__cpluscplus)
#define THISCALL   __attribute__ ((thiscall))
#endif
#else
#define CDECL
#define STDCALL
#define FASTCALL
#define SAFECALL
#define CLRCALL
#define VECTORCALL
#if defined(__cpluscplus)
#define THISCALL
#endif
#endif

#if !defined(_WINDOWS)
#define WINAPI

typedef uint8_t  BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef bool     BRESULT;
typedef wchar_t  TCHAR;

typedef union {
  struct {
    UINT32 LowPart;
    SINT32 HighPart;
  } u;
  SINT64 QuadPart;
} LARGE_INTEGER;

#define _T(string) string
#define _tcscpy    OEMSTRCPY
#define	_tcsicmp   milstr_cmp
#define	_tcsnicmp  strncasecmp

#ifndef ZeroMemory
#define ZeroMemory(d, z)    memset((d), 0, (z))
#endif
#ifndef CopyMemory
#define CopyMemory(d, s, z) memcpy((d), (s), (z))
#endif
#ifndef	FillMemory
#define	FillMemory(d, z, c)	memset((d), (c), (z))
#endif

#if defined(__WINRT__) && defined(_M_IX86)
#define CreateFileW(f, a, s, sec, p, flg, t) CreateFile2(f, a, s, p, NULL)
#endif
#endif
// <-- Windowsnize

typedef uint8_t  REG8;
typedef uint16_t REG16;

#define	UNUSED(v) (void)(v)

#define	CPUCALL    FASTCALL
#define	MEMCALL    FASTCALL
#define	DMACCALL   FASTCALL
#define	IOOUTCALL  FASTCALL
#define	IOINPCALL  FASTCALL
#define	SOUNDCALL  FASTCALL
#define	VRAMCALL   FASTCALL
#define	SCRNCALL   FASTCALL
#define	VERMOUTHCL FASTCALL
#define	PARTSCALL  FASTCALL

#define GETRAND() rand()

#ifndef MSB_FIRST
#define BYTESEX_LITTLE
#else
#define BYTESEX_BIG
#endif

#define sigjmp_buf           jmp_buf
#ifndef sigsetjmp
#define sigsetjmp(env, mask) setjmp(env)
#endif
#ifndef siglongjmp
#define siglongjmp(env, val) longjmp(env, val)
#endif

#define COPY64(pd, ps) *(UINT64*)(pd) = *(UINT64*)(ps);

#ifndef MAX_PATH
#define MAX_PATH 4096
#endif

#ifndef	MAX
#define	MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef	MIN
#define	MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef	NELEMENTS
#define	NELEMENTS(a) (sizeof(a) / sizeof(a[0]))
#endif

#if defined(SUPPORT_LARGE_MEMORY)
#define MEMORY_MAXSIZE 4000
#else
#define MEMORY_MAXSIZE 230
#endif

#if defined(NP2_CPU_64BIT)
#if defined(SUPPORT_LARGE_HDD)
typedef int64_t FILEPOS;
typedef int64_t FILELEN;
#define	NHD_MAXSIZE  8000
#define	NHD_MAXSIZE2 32000
#else
typedef int32_t FILEPOS;
typedef int32_t FILELEN;
#define	NHD_MAXSIZE  2000
#define	NHD_MAXSIZE2 2000
#endif
#else
typedef int32_t FILEPOS;
typedef int32_t FILELEN;
#define	NHD_MAXSIZE  2000
#define	NHD_MAXSIZE2 2000
#endif

#undef  MEMOPTIMIZE
#if defined(arm) || defined (__arm__)
#define	MEMOPTIMIZE 2
#define	LOW12(a)  ((((UINT32)(a)) << 20) >> 20)
#define	LOW14(a)  ((((UINT32)(a)) << 18) >> 18)
#define	LOW15(a)  ((((UINT32)(a)) << 17) >> 17)
#define	LOW16(a)  ((UINT16)(a))
#define	HIGH16(a) (((UINT32)(a)) >> 16)
#endif

#include "common.h"
#include "common/_memory.h"
#include "common/rect.h"
#include "common/lstarray.h"

#endif  // _COMPILER_BASE_H_
