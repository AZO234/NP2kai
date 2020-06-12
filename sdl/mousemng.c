#include	"compiler.h"
#include	"mousemng.h"

MOUSEMNG	mousemng;
#if defined(EMSCRIPTEN) && !defined(__LIBRETRO__)
int captured=0;
#endif

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
#if !defined(DEBUG)
#if defined(EMSCRIPTEN) && !defined(__LIBRETRO__)
	captured = capture;
	if(captured)
	{
#if SDL_MAJOR_VERSION == 1
		SDL_WM_GrabInput(SDL_GRAB_ON);
#else
		SDL_CaptureMouse(TRUE);
#endif
		mousemng_hidecursor();
	}	
	else
	{
		mousemng_showcursor();
#if SDL_MAJOR_VERSION == 1
		SDL_WM_GrabInput(SDL_GRAB_OFF);
#else
		SDL_CaptureMouse(FALSE);
#endif
	}
#endif
#endif
}

#if defined(EMSCRIPTEN) && !defined(__LIBRETRO__)
int ismouse_captured(void){
	return captured;
}
#endif

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
#if defined(EMSCRIPTEN) && !defined(__LIBRETRO__)
/*	if(!captured)
	{
		if (button->button == SDL_BUTTON_LEFT && button->type == SDL_MOUSEBUTTONUP)
			mousemng_enable(MOUSEPROC_SYSTEM);
	}*/
	if(captured) {
#endif
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
#if defined(EMSCRIPTEN) && !defined(__LIBRETRO__)
	}
#endif
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
#if !defined(DEBUG)
#if defined(EMSCRIPTEN) && !defined(__LIBRETRO__)
	if(captured) {
		SDL_ShowCursor(SDL_DISABLE);
#if SDL_MAJOR_VERSION > 1
		SDL_SetRelativeMouseMode(SDL_TRUE);
#else
		SDL_WM_GrabInput(SDL_GRAB_ON);
#endif
	}
#else
	if (!--mousemng.showcount) {
		SDL_ShowCursor(SDL_DISABLE);
#if SDL_MAJOR_VERSION == 1
		SDL_WM_GrabInput(SDL_GRAB_ON);
#else
		SDL_SetRelativeMouseMode(SDL_TRUE);
#endif
	}
#endif
#endif
}

void mousemng_showcursor() {
#if !defined(DEBUG)
#if defined(EMSCRIPTEN) && !defined(__LIBRETRO__)
	SDL_ShowCursor(SDL_ENABLE);
#if SDL_MAJOR_VERSION == 1
	SDL_WM_GrabInput(SDL_GRAB_OFF);
#else
	SDL_SetRelativeMouseMode(SDL_FALSE);
#endif
#else
	if (!mousemng.showcount++) {
		SDL_ShowCursor(SDL_ENABLE);
#if SDL_MAJOR_VERSION == 1
		SDL_WM_GrabInput(SDL_GRAB_OFF);
#else
		SDL_SetRelativeMouseMode(SDL_FALSE);
#endif
	}
#endif
#endif
}

void mousemng_onmove(SDL_MouseMotionEvent *motion) {
#if defined(EMSCRIPTEN) && !defined(__LIBRETRO__)
	if(captured) {
		mousemng.x += motion->xrel;
		mousemng.y += motion->yrel;
	}
#else
	mousemng.x += motion->xrel;
	mousemng.y += motion->yrel;
#endif
}
#else	/* __LIBRETRO__ */
void mousemng_onmove(int x, int y) {
	mousemng.x += x;
	mousemng.y += y;
}
#endif	/* __LIBRETRO__ */

