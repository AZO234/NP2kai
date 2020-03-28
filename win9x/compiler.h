/**
 * @file	compiler.h
 * @brief	include file for standard system include files,
 *			or project specific include files that are used frequently,
 *			but are changed infrequently
 *
 * @author	$Author: yui $
 * @date	$Date: 2011/03/09 00:22:18 $
 */

#ifndef COMPILER_H
#define COMPILER_H

#include "compiler_base.h"

#include "targetver.h"
#define _USE_MATH_DEFINES
//#include <windows.h>
/* workaround for VC6 (definition missing in the header) */
#if (_MSC_VER + 0) <= 1200
# ifdef __cplusplus
extern "C" {
# endif
WINBASEAPI BOOL WINAPI SetFilePointerEx(HANDLE, LARGE_INTEGER, PLARGE_INTEGER, DWORD);
# ifdef __cplusplus
}
# endif
#endif
#if defined(TRACE)
#include <assert.h>
#endif

#if !defined(LLONG_MIN)
#  define LLONG_MIN (SINT64)(QWORD_CONST(1)<<63)
#endif

#if !defined(__GNUC__)
#define	sigjmp_buf				jmp_buf
#define	sigsetjmp(env, mask)	setjmp(env)
#define	siglongjmp(env, val)	longjmp(env, val)
#endif	// !defined(__GNUC__)
#define	msgbox(title, msg)		MessageBoxA(NULL, msg, title, MB_OK)

#include "misc\tickcounter.h"
#include "misc\vc6macros.h"

#define	GETTICK()			GetTickCounter()
#if defined(TRACE)
#define	__ASSERT(s)			assert(s)
#else
#define	__ASSERT(s)
#endif

#define	LABEL				__declspec(naked)
#define	RELEASE(x) 			if (x) {(x)->Release(); (x) = NULL;}

#if !defined(_WIN64)
#define	OPNGENX86
#endif

#define	MT32SOUND_DLL

#define	OSLINEBREAK_CRLF
#define	SUPPORT_SJIS

#define	SUPPORT_8BPP
#define	SUPPORT_24BPP

#define	SUPPORT_SOFTKBD		0
#define SUPPORT_S98
#define SUPPORT_WAVEREC
#define SUPPORT_RECVIDEO
#define	SUPPORT_KEYDISP
#define	SUPPORT_MEMDBG32
#define	SUPPORT_HOSTDRV
#if !defined(_WIN64)
#define	SUPPORT_DCLOCK
#endif

#define SOUND_CRITICAL
#define	SOUNDRESERVE	20
//#define SUPPORT_VSTi
#define SUPPORT_ASIO
#if (_MSC_VER >= 1500)
#define SUPPORT_WASAPI
#endif	/* (_MSC_VER >= 1500) */

#define	SUPPORT_TEXTCNV

#define SUPPORT_WIN2000HOST

#if defined(SUPPORT_IA32_HAXM)
#define USE_CUSTOM_HOOKINST
#endif

#if defined(CPUCORE_IA32)
#pragma warning(disable: 4819)
#endif

#if (_MSC_VER >= 1400)
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif	/* (_MSC_VER >= 1400) */

#include "milstr.h"
#include "trace.h"

#endif  // COMPILER_H

