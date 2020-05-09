#ifndef NP2_SDL2_KBTRANS_H
#define NP2_SDL2_KBTRANS_H

#include <compiler.h>

#if defined(__LIBRETRO__)
typedef struct {
  UINT  lrkey;
  UINT8 keycode;
} LRKCNV;

extern uint16_t keys_needed;
extern LRKCNV*  keys_poll;

void init_lrkey_to_pc98();
void reset_lrkey();
void send_libretro_key_down(UINT key);
void send_libretro_key_up(UINT key);
#else	/* __LIBRETRO__ */
void sdlkbd_initialize(void);
void sdlkbd_reset();
void sdlkbd_keydown(UINT key);
void sdlkbd_keyup(UINT key);
#endif	/* __LIBRETRO__ */
#endif

