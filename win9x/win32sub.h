#include "compiler.h"

#ifdef STRICT
#define	SUBCLASSPROC	WNDPROC
#else
#define	SUBCLASSPROC	FARPROC
#endif

// for VC6SDK
#if (_MSC_VER < 1300)
#ifndef LONG_PTR
#define LONG_PTR			LONG
#endif
#ifndef DWORD_PTR
#define DWORD_PTR			DWORD
#endif
#ifndef GetWindowLongPtr
#define GetWindowLongPtr	GetWindowLong
#endif
#ifndef SetWindowLongPtr
#define SetWindowLongPtr	SetWindowLong
#endif
#ifndef GWLP_WNDPROC
#define GWLP_WNDPROC		GWL_WNDPROC
#endif
#ifndef GWLP_HINSTANCE
#define GWLP_HINSTANCE		GWL_HINSTANCE
#endif
#ifndef GWLP_HWNDPARENT
#define GWLP_HWNDPARENT		GWL_HWNDPARENT
#endif
#ifndef GWLP_USERDATA
#define GWLP_USERDATA		GWL_USERDATA
#endif
#ifndef GWLP_ID
#define GWLP_ID				GWL_ID
#endif
#endif

#define	LOADSTRING			LoadString

#ifdef __cplusplus
extern "C" {
#endif

void __msgbox(const char *title, const char *msg);
int loadstringresource(UINT uID, LPTSTR lpszBuffer, int nBufferMax);
LPTSTR lockstringresource(LPCTSTR lpcszString);
void unlockstringresource(LPTSTR lpszString);

#ifdef __cplusplus
}
#endif

