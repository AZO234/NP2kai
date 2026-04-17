/* === mouse management for wx port === */

#ifndef NP2_WX_MOUSEMNG_H
#define NP2_WX_MOUSEMNG_H

#include <compiler.h>

enum {
	uPD8255A_LEFTBIT  = 0x80,
	uPD8255A_RIGHTBIT = 0x20
};

enum {
	MOUSEPROC_SYSTEM = 0,
	MOUSEPROC_WINUI,
	MOUSEPROC_BG
};

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	SINT16  x;
	SINT16  y;
	UINT8   btn;
	UINT    flag;
	UINT8   showcount;
} MOUSEMNG;

extern MOUSEMNG mousemng;

void mousemng_initialize(void);
BYTE mousemng_getstat(SINT16 *x, SINT16 *y, int clear);
void mousemng_sync(int mpx, int mpy);
void mousemng_enable(UINT proc);
void mousemng_disable(UINT proc);
void mousemng_toggle(UINT proc);
void mousemng_hidecursor(void);
void mousemng_showcursor(void);

/* called from wxPanel event handlers */
void mousemng_onmove(int dx, int dy);
void mousemng_onleft(BOOL pressed);
void mousemng_onright(BOOL pressed);

#ifdef __cplusplus
}
#endif

#endif /* NP2_WX_MOUSEMNG_H */
