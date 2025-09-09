
enum {
	uPD8255A_LEFTBIT	= 0x80,
	uPD8255A_RIGHTBIT	= 0x20
};

enum {
	MOUSEPROC_SYSTEM		= 0,
	MOUSEPROC_WINUI,
	MOUSEPROC_BG
};

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	SINT16	x;
	SINT16	y;
	UINT8	btn;
	UINT	flag;
#if !defined(__LIBRETRO__)
	UINT8	showcount;
#endif	/* __LIBRETRO__ */
} MOUSEMNG;

extern MOUSEMNG	mousemng;

void mousemng_initialize(void);
BYTE mousemng_getstat(SINT16 *x, SINT16 *y, int clear);
void mousemng_sync(int mpx,int mpy);
void mousemng_enable(UINT proc);
void mousemng_disable(UINT proc);
void mousemng_toggle(UINT proc);
#if !defined(__LIBRETRO__)
void mousemng_hidecursor();
void mousemng_showcursor();
void mousemng_onmove(SDL_MouseMotionEvent *motion);
void mousemng_buttonevent(SDL_MouseButtonEvent *button);
#if defined(EMSCRIPTEN) && !defined(__LIBRETRO__)
int ismouse_captured(void);
#endif
#else	/* __LIBRETRO__ */
void mousemng_onmove(int x, int y);
BOOL mousemng_buttonevent(UINT event);
#endif	/* __LIBRETRO__ */

#ifdef __cplusplus
}
#endif

#if defined(__LIBRETRO__)
// ---- for libretro

enum {
	MOUSEMNG_LEFTDOWN		= 0,
	MOUSEMNG_LEFTUP,
	MOUSEMNG_RIGHTDOWN,
	MOUSEMNG_RIGHTUP
};

BOOL mousemng_buttonevent(UINT event);
#endif	/* __LIBRETRO__ */
