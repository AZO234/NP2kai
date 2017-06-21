#include	"compiler.h"
#include	"mousemng.h"

typedef struct {
	SINT16	x;
	SINT16	y;
	UINT8	btn;
	UINT8	showcount;
} MOUSEMNG;

static	MOUSEMNG	mousemng;

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
