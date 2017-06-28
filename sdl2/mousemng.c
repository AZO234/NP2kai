#include	"compiler.h"
#include	"mousemng.h"

MOUSEMNG	mousemng;

#if defined(__LIBRETRO__)
#define	MOUSEMNG_RANGE		128

UINT8 mousemng_getstat(SINT16 *x, SINT16 *y, int clear) {
/*
	*x = 0;
	*y = 0;
	(void)clear;
	return(0xa0);
*/
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
}

void mousemng_sync(int pmx,int pmy) {

/*
	POINT	p;
	POINT	cp;

	if ((!mousemng.flag) && (GetCursorPos(&p))) {
		getmaincenter(&cp);
		mousemng.x += (SINT16)((p.x - cp.x) / 2);
		mousemng.y += (SINT16)((p.y - cp.y) / 2);
		SetCursorPos(cp.x, cp.y);
	}
*/
		mousemng.x += pmx;
		mousemng.y += pmy;
}

BOOL mousemng_buttonevent(UINT event) {

	if (!mousemng.flag) {
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
	else {
		return(FALSE);
	}
}

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
#else	/* __LIBRETRO__ */
void mousemng_initialize(void) {
	ZeroMemory(&mousemng, sizeof(mousemng));
	mousemng.btn = uPD8255A_LEFTBIT | uPD8255A_RIGHTBIT;
	mousemng.showcount = 1;
}

BYTE mousemng_getstat(SINT16 *x, SINT16 *y, int clear) {
	*x = mousemng.x;
	*y = mousemng.y;
	if (clear) {
		mousemng.x = 0;
		mousemng.y = 0;
	}
	return(mousemng.btn);
}

void mousemng_hidecursor() {
	if (!--mousemng.showcount) {
		SDL_ShowCursor(SDL_DISABLE);
		SDL_SetRelativeMouseMode(SDL_TRUE);
	}
}

void mousemng_showcursor() {
	if (!mousemng.showcount++) {
		SDL_ShowCursor(SDL_ENABLE);
		SDL_SetRelativeMouseMode(SDL_FALSE);
	}
}

void mousemng_onmove(SDL_MouseMotionEvent *motion) {
	mousemng.x += motion->xrel;
	mousemng.y += motion->yrel;
}

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
	if (button->state == SDL_PRESSED)
		mousemng.btn &= ~bit;
	else
		mousemng.btn |= bit;
}
#endif	/* __LIBRETRO__ */

