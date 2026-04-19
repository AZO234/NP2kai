#include	"compiler.h"
#include	"taskmng.h"
#if defined(SUPPORT_MULTITHREAD)
#include	"np2.h"
#endif

void taskmng_exit(void) {
	
#if defined(SUPPORT_MULTITHREAD)
	PostMessage(g_hWndMain, WM_CLOSE, 0, 0);
#else
	PostQuitMessage(0);
#endif
}

