#include	"compiler.h"
#include	"mousemng.h"

MOUSEMNG	mousemng;

UINT8 mousemng_getstat(SINT16 *x, SINT16 *y, int clear) {
	*x = mousemng.x;
	*y = mousemng.y;
	if (clear) {
		mousemng.x = 0;
		mousemng.y = 0;
	}
	return(mousemng.btn);
}

static void mousecapture(BOOL capture) {

}

void mousemng_initialize(void) {

	ZeroMemory(&mousemng, sizeof(mousemng));
	mousemng.btn = uPD8255A_LEFTBIT | uPD8255A_RIGHTBIT;
	mousemng.flag = (1 << MOUSEPROC_SYSTEM);
#if !defined(__LIBRETRO__)
	mousemng.showcount = 1;
#endif	/* __LIBRETRO__ */
}

void mousemng_sync(int pmx,int pmy) {

		mousemng.x += pmx;
		mousemng.y += pmy;
}

#if defined(__LIBRETRO__)
BOOL mousemng_buttonevent(UINT event) {
	switch(event) {
		case MOUSEMNG_LEFTDOWN:
			mousemng.btn &= ~(uPD8255A_LEFTBIT);
			break;

		case MOUSEMNG_LEFTUP:
			mousemng.btn |= uPD8255A_LEFTBIT;
			break;

		case MOUSEMNG_RIGHTDOWN:
			mousemng.btn &= ~(uPD8255A_RIGHTBIT);
			break;

		case MOUSEMNG_RIGHTUP:
			mousemng.btn |= uPD8255A_RIGHTBIT;
			break;
	}
	return(TRUE);
}
#else	/* __LIBRETRO__ */
void mousemng_buttonevent(SDL_MouseButtonEvent *button) {
	UINT8 bit;
	switch (button->button) {
	case SDL_BUTTON_LEFT:
		bit = uPD8255A_LEFTBIT;
		break;
	case SDL_BUTTON_RIGHT:
		bit = uPD8255A_RIGHTBIT;
		break;
	default:
		return;
	}
	if (button->type == SDL_MOUSEBUTTONDOWN)
		mousemng.btn &= ~bit;
	else
		mousemng.btn |= bit;
}
#endif	/* __LIBRETRO__ */

void mousemng_enable(UINT proc) {

	UINT	bit;

	bit = 1 << proc;
	if (mousemng.flag & bit) {
		mousemng.flag &= ~bit;
		if (!mousemng.flag) {
			mousecapture(TRUE);
		}
	}
}

void mousemng_disable(UINT proc) {

	if (!mousemng.flag) {
		mousecapture(FALSE);
	}
	mousemng.flag |= (1 << proc);
}

void mousemng_toggle(UINT proc) {

	if (!mousemng.flag) {
		mousecapture(FALSE);
	}
	mousemng.flag ^= (1 << proc);
	if (!mousemng.flag) {
		mousecapture(TRUE);
	}
}

#if !defined(__LIBRETRO__)
void mousemng_hidecursor() {
	if (!--mousemng.showcount) {
		SDL_ShowCursor(SDL_DISABLE);
#if SDL_MAJOR_VERSION == 1
		SDL_WM_GrabInput(SDL_GRAB_ON);
#else
		SDL_SetRelativeMouseMode(SDL_TRUE);
#endif
	}
}

void mousemng_showcursor() {
	if (!mousemng.showcount++) {
		SDL_ShowCursor(SDL_ENABLE);
#if SDL_MAJOR_VERSION == 1
		SDL_WM_GrabInput(SDL_GRAB_OFF);
#else
		SDL_SetRelativeMouseMode(SDL_FALSE);
#endif
	}
}

void mousemng_onmove(SDL_MouseMotionEvent *motion) {
	mousemng.x += motion->xrel;
	mousemng.y += motion->yrel;
}
#else	/* __LIBRETRO__ */
void mousemng_onmove(int x, int y) {
	mousemng.x += x;
	mousemng.y += y;
}
#endif	/* __LIBRETRO__ */

