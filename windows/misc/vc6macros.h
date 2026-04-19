/**
 * @file	vc6macros.h
 * @brief	VC6 ópÉ}ÉNÉç
 */

#pragma once

#ifdef __cplusplus

#ifndef _countof
#define _countof(x)		(sizeof((x)) / sizeof((x)[0]))		/*!< countof */
#endif	/* _countof */

#if (_MSC_VER < 1300)
#define for					if (0 /*NEVER*/) { /* no process */ } else for			/*!< for scope */
#endif	/* (_MSC_VER < 1300) */

#endif	/* __cplusplus */

#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lp)	((int)(short)LOWORD(lp))		/*!< x-coordinate from LPARAM */
#endif
#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(lp)	((int)(short)HIWORD(lp))		/*!< y-coordinate from LPARAM */
#endif

// for VC6SDK
#if (_MSC_VER < 1300)
#ifndef LONG_PTR
#define LONG_PTR			LONG							/*!< LONG_PTR */
#endif
#ifndef DWORD_PTR
#define DWORD_PTR			DWORD							/*!< DWORD_PTR */
#endif
#ifndef GetWindowLongPtr
#define GetWindowLongPtr	GetWindowLong					/*!< Retrieves information about the specified window */
#endif
#ifndef SetWindowLongPtr
#define SetWindowLongPtr	SetWindowLong					/*!< Changes an attribute of the specified window */
#endif
#ifndef GWLP_WNDPROC
#define GWLP_WNDPROC		GWL_WNDPROC						/*!< Retrieves the pointer to the window procedure */
#endif
#ifndef GWLP_HINSTANCE
#define GWLP_HINSTANCE		GWL_HINSTANCE					/*!< Retrieves a handle to the application instance */
#endif
#ifndef GWLP_HWNDPARENT
#define GWLP_HWNDPARENT		GWL_HWNDPARENT					/*!< Retrieves a handle to the parent window */
#endif
#ifndef GWLP_USERDATA
#define GWLP_USERDATA		GWL_USERDATA					/*!< Retrieves the user data associated with the window */
#endif
#ifndef GWLP_ID
#define GWLP_ID				GWL_ID							/*!< Retrieves the identifier of the window */
#endif
#endif

#if (_MSC_VER < 1300)
#if defined(_USE_MATH_DEFINES) && !defined(_MATH_DEFINES_DEFINED)
#define _MATH_DEFINES_DEFINED
#define M_PI		3.14159265358979323846
#endif
#endif
