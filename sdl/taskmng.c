#if !defined(__APPLE__)
#define _POSIX_C_SOURCE 199309L
#endif
#include	<compiler.h>
#include	"inputmng.h"
#include	"taskmng.h"
#include	"kbtrans.h"
#include	<embed/vramhdl.h>
#include	<embed/menubase/menubase.h>
#include	"sysmenu.h"
#include	"scrnmng.h"
#include	"mousemng.h"
#include	"np2.h"
#include	<np2_thread.h>

#if defined(__LIBRETRO__)
#include <retro_miscellaneous.h>
#include <libretro.h>

extern retro_environment_t environ_cb;
#endif	/* __LIBRETRO__ */

	BOOL	task_avail;


void sighandler(int signo) {

	(void)signo;
	task_avail = FALSE;
}


void taskmng_initialize(void) {

	task_avail = TRUE;
}

void taskmng_exit(void) {

	task_avail = FALSE;
}

#if defined(__OPENDINGUX__)

int ENABLE_MOUSE=0; //0--disable

int convertKeyMap(int scancode){
  switch(scancode){
#if SDL_MAJOR_VERSION != 1
    case 82: //up
      return SDL_SCANCODE_UP;
    case 81: //down
      return SDL_SCANCODE_DOWN;
    case 80: //left
      return SDL_SCANCODE_LEFT;
    case 79: //right
      return SDL_SCANCODE_RIGHT;
    case 225: //x
      return SDL_SCANCODE_UNKNOWN;
    case 44:  //y
      return SDL_SCANCODE_UNKNOWN;
    case 224: //A
      return SDL_SCANCODE_RETURN;
    case 226: //B
      return SDLK_TAB;
    case 41: //select
      return SDL_SCANCODE_ESCAPE;
    case 40: //start
      return SDL_SCANCODE_RETURN;
    case 43: //L
      return SDL_SCANCODE_F11;
    case 42: //R
      // ENABLE_MOUSE++;
      // ENABLE_MOUSE=ENABLE_MOUSE%2;
      return 	999;
#else
    case 82: //up
      return SDLK_UP;
    case 81: //down
      return SDLK_DOWN;
    case 80: //left
      return SDLK_LEFT;
    case 79: //right
      return SDLK_RIGHT;
    case 225: //x
      return SDLK_UNKNOWN;
    case 44:  //y
      return SDLK_UNKNOWN;
    case 224: //A
      return SDLK_RETURN;
    case 226: //B
      return SDLK_TAB;
    case 41: //select
      return SDLK_ESCAPE;
    case 40: //start
      return SDLK_RETURN;
    case 43: //L
      return SDLK_F10;
    case 42: //R
      // ENABLE_MOUSE++;
      // ENABLE_MOUSE=ENABLE_MOUSE%2;
      return 	999;
#endif
  }
}

#endif  // __OPENDINGUX__

int mx = 320, my = 240;

void taskmng_rol(void) {

#if !defined(__LIBRETRO__)
	SDL_Event	e;


	if ((!task_avail) || (!SDL_PollEvent(&e))) {
		return;
	}
	switch(e.type) {
		case SDL_MOUSEMOTION:
			if (menuvram == NULL) {
				mousemng_onmove(&e.motion);
			} else {
				if((scrnmode & SCRNMODE_ROTATEMASK) == SCRNMODE_ROTATELEFT) {
					mx = (menuvram->width - 1) - e.motion.y;
					my = e.motion.x;
				} else if((scrnmode & SCRNMODE_ROTATEMASK) == SCRNMODE_ROTATERIGHT) {
					mx = e.motion.y;
					my = (menuvram->height - 1) - e.motion.x;
				} else {
					mx = e.motion.x;
					my = e.motion.y;
				}
				menubase_moving(mx, my, 0);
			}
			break;

		case SDL_MOUSEBUTTONUP:
			switch(e.button.button) {
				case SDL_BUTTON_LEFT:
					if (menuvram != NULL)
					{
//						menubase_moving(e.button.x, e.button.y, 2);
						menubase_moving(mx, my, 2);
					}
#if defined(__IPHONEOS__)
					else if (SDL_IsTextInputActive())
					{

						SDL_StopTextInput();
					}
					else if (e.button.y >= 320)
					{
						SDL_StartTextInput();
					}
#endif
					else
					{
						mousemng_buttonevent(&e.button);
					}
					break;

				case SDL_BUTTON_RIGHT:
					if (menuvram == NULL) {
						mousemng_buttonevent(&e.button);
					}
					break;
			}
			break;

		case SDL_MOUSEBUTTONDOWN:
			switch(e.button.button) {
				case SDL_BUTTON_LEFT:
					if (menuvram != NULL)
					{
//						menubase_moving(e.button.x, e.button.y, 1);
						menubase_moving(mx, my, 1);
					} else {
						mousemng_buttonevent(&e.button);
					}
					break;

				case SDL_BUTTON_RIGHT:
					if (menuvram == NULL) {
						mousemng_buttonevent(&e.button);
					}
					break;

				case SDL_BUTTON_MIDDLE:
					if (menuvram == NULL) {
//						sysmenu_menuopen(0, e.button.x, e.button.y);
						sysmenu_menuopen(0, mx, my);
					} else {
						menubase_close();
					}
					break;
			}
			break;

		case SDL_KEYDOWN:

#if defined(__OPENDINGUX__)
#if SDL_MAJOR_VERSION != 1
      e.key.keysym.scancode=convertKeyMap(e.key.keysym.scancode);
      if(e.key.keysym.scancode==SDL_SCANCODE_UNKNOWN || e.key.keysym.scancode ==999){
        return;
      }
#else
      e.key.keysym.sym=convertKeyMap(e.key.keysym.sym);
      if(e.key.keysym.sym==SDLK_UNKNOWN || e.key.keysym.sym ==999){
        return;
      }
#endif
#endif  // __OPENDINGUX__
#if SDL_MAJOR_VERSION == 1
			if (e.key.keysym.sym == SDLK_F11) {
#else
			if (e.key.keysym.scancode == SDL_SCANCODE_F11) {
#endif
#if defined(EMSCRIPTEN) && !defined(__LIBRETRO__) //in web browsers, F11 is commonly occupied. Use CTRL+F11
			if ((e.key.keysym.mod == KMOD_LCTRL) || (e.key.keysym.mod == KMOD_RCTRL)) {
#endif 
				if (menuvram == NULL) {
					sysmenu_menuopen(0, 0, 0);
				}
				else {
					menubase_close();
				}
			}
#if defined(EMSCRIPTEN) && !defined(__LIBRETRO__)
			}
#if SDL_MAJOR_VERSION == 1
			if (e.key.keysym.sym == SDLK_F12) {
#else
			else if (e.key.keysym.scancode == SDL_SCANCODE_F12) {
#endif
				if ((e.key.keysym.mod == KMOD_LCTRL) || (e.key.keysym.mod == KMOD_RCTRL)) {
					//use CTRL+F12 to lock mouse like win32 builds do
					mousemng_toggle(MOUSEPROC_SYSTEM);
				}
			}
#endif
			else {
#if SDL_MAJOR_VERSION == 1
				sdlkbd_keydown(e.key.keysym.sym);
#else
				sdlkbd_keydown(e.key.keysym.scancode);
#endif
			}
			break;

		case SDL_KEYUP:
#if defined(__OPENDINGUX__)
#if SDL_MAJOR_VERSION != 1
      e.key.keysym.scancode=convertKeyMap(e.key.keysym.scancode);
      if(e.key.keysym.scancode==SDL_SCANCODE_UNKNOWN || e.key.keysym.scancode ==999){
        return;
      }
#else
      e.key.keysym.sym=convertKeyMap(e.key.keysym.sym);
      if(e.key.keysym.sym==SDLK_UNKNOWN || e.key.keysym.sym ==999){
        return;
      }
#endif
#endif  // __OPENDINGUX__

#if SDL_MAJOR_VERSION == 1
      sdlkbd_keyup(e.key.keysym.sym);
#else
      sdlkbd_keyup(e.key.keysym.scancode);
#endif
			break;

		case SDL_QUIT:
			task_avail = FALSE;
			break;
	}
#endif	/* __LIBRETRO__ */
}

BOOL taskmng_sleep(UINT32 tick) {

	UINT32	base;

	base = GETTICK();
	while((task_avail) && ((GETTICK() - base) < tick)) {
		taskmng_rol();
#if defined(EMSCRIPTEN) && !defined(__LIBRETRO__)
//		emscripten_sleep_with_yield(1);
		emscripten_sleep(1);
#else
		NP2_Sleep_ms(1);
#endif
	}
	return(task_avail);
}
