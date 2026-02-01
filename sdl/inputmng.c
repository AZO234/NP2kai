#include	<compiler.h>
#include	<inputmng.h>


typedef struct {
#if defined(SDL_h_)
#if USE_SDL_VERSION >= 2
	SDL_Scancode	key;
#else
	SDLKey	key;
#endif
#else
	short	key;
#endif
	UINT		bit;
} KEYBIND;

typedef struct {
	UINT	kbs;
	KEYBIND	kb[32];
} INPMNG;

static	INPMNG	inpmng;

static const KEYBIND keybind[] = {
#if defined(SDL_h_)
#if USE_SDL_VERSION >= 2
					{SDL_SCANCODE_UP,		NP2_KEY_UP},
					{SDL_SCANCODE_DOWN,		NP2_KEY_DOWN},
					{SDL_SCANCODE_LEFT,		NP2_KEY_LEFT},
					{SDL_SCANCODE_RIGHT,	NP2_KEY_RIGHT},
					{SDL_SCANCODE_RETURN,	KEY_ENTER},
					{SDL_SCANCODE_ESCAPE,	KEY_MENU},
					{SDL_SCANCODE_TAB,		KEY_SKIP}		/* とりあえずね… */
#else
					{SDLK_UP,		NP2_KEY_UP},
					{SDLK_DOWN,		NP2_KEY_DOWN},
					{SDLK_LEFT,		NP2_KEY_LEFT},
					{SDLK_RIGHT,	NP2_KEY_RIGHT},
					{SDLK_RETURN,	KEY_ENTER},
					{SDLK_ESCAPE,	KEY_MENU},
					{SDLK_TAB,		KEY_SKIP}		/* とりあえずね… */
#endif
#else
					{RETROK_UP,		NP2_KEY_UP},
					{RETROK_DOWN,		NP2_KEY_DOWN},
					{RETROK_LEFT,		NP2_KEY_LEFT},
					{RETROK_RIGHT,	NP2_KEY_RIGHT},
					{RETROK_RETURN,	KEY_ENTER},
					{RETROK_ESCAPE,	KEY_MENU},
					{RETROK_TAB,		KEY_SKIP}		/* とりあえずね… */
#endif
};


// ----

void inputmng_init(void) {

	INPMNG	*im;

	im = &inpmng;
	ZeroMemory(im, sizeof(INPMNG));
	im->kbs = sizeof(keybind) / sizeof(KEYBIND);
	CopyMemory(im->kb, keybind, sizeof(keybind));
}

void inputmng_keybind(short key, UINT bit) {

	INPMNG	*im;
	UINT	i;

	im = &inpmng;
	for (i=0; i<im->kbs; i++) {
		if (im->kb[i].key == key) {
			im->kb[i].bit = bit;
			return;
		}
	}
	if (im->kbs < (sizeof(im->kb) / sizeof(KEYBIND))) {
		im->kb[im->kbs].key = key;
		im->kb[im->kbs].bit = bit;
		im->kbs++;
	}
}

UINT inputmng_getkey(short key) {

	INPMNG	*im;
	KEYBIND	*kb;
	UINT	kbs;

	im = &inpmng;
	kb = im->kb;
	kbs = im->kbs;
	while(kbs--) {
		if (kb->key == key) {
			return(kb->bit);
		}
		kb++;
	}
	return(0);
}

