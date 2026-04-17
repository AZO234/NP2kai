/* === mouse management for wx port === */

#include <compiler.h>
#include "mousemng.h"
#include "inputmng.h"
MOUSEMNG mousemng;

static UINT enabled_procs = 0;

void mousemng_initialize(void)
{
	memset(&mousemng, 0, sizeof(mousemng));
	mousemng.btn = uPD8255A_LEFTBIT | uPD8255A_RIGHTBIT;  /* active-low: both released */
	mousemng.showcount = 1;
}

BYTE mousemng_getstat(SINT16 *x, SINT16 *y, int clear)
{
	BYTE btn = mousemng.btn;
	if (x) *x = mousemng.x;
	if (y) *y = mousemng.y;
	if (clear) {
		mousemng.x = 0;
		mousemng.y = 0;
		/* Apply pending releases: button was released before this sync,
		 * but we kept the pressed state latched so the emulator could see it.
		 * Now that the sync has sampled btn, apply the releases. */
		if (mousemng.flag & LBUTTON_UPBIT) {
			mousemng.btn |= uPD8255A_LEFTBIT;
			mousemng.flag &= ~LBUTTON_UPBIT;
		}
		if (mousemng.flag & RBUTTON_UPBIT) {
			mousemng.btn |= uPD8255A_RIGHTBIT;
			mousemng.flag &= ~RBUTTON_UPBIT;
		}
	}
	return btn;
}

void mousemng_sync(int mpx, int mpy)
{
	(void)mpx; (void)mpy;
	/* Mouse state is read via mousemng_getstat() from the emulator core */
}

void mousemng_enable(UINT proc)
{
	enabled_procs |= (1u << proc);
}

void mousemng_disable(UINT proc)
{
	enabled_procs &= ~(1u << proc);
}

void mousemng_toggle(UINT proc)
{
	enabled_procs ^= (1u << proc);
}

void mousemng_hidecursor(void)
{
	if (mousemng.showcount > 0) {
		mousemng.showcount--;
	}
}

void mousemng_showcursor(void)
{
	mousemng.showcount++;
}

void mousemng_onmove(int dx, int dy)
{
	mousemng.x += (SINT16)dx;
	mousemng.y += (SINT16)dy;
	mousemng.flag |= MOUSE_MOVEBIT;
}

void mousemng_onleft(BOOL pressed)
{
	if (pressed) {
		/* Cancel any pending release, latch button as pressed */
		mousemng.flag &= ~LBUTTON_UPBIT;
		mousemng.btn  &= ~uPD8255A_LEFTBIT;  /* active-low: clear = pressed */
	} else {
		/* Schedule release — keep btn latched until mouseif_sync samples it */
		mousemng.flag |= LBUTTON_UPBIT;
	}
}

void mousemng_onright(BOOL pressed)
{
	if (pressed) {
		/* Cancel any pending release, latch button as pressed */
		mousemng.flag &= ~RBUTTON_UPBIT;
		mousemng.btn  &= ~uPD8255A_RIGHTBIT;  /* active-low: clear = pressed */
	} else {
		/* Schedule release — keep btn latched until mouseif_sync samples it */
		mousemng.flag |= RBUTTON_UPBIT;
	}
}
