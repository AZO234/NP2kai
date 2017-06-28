#ifndef NP2_SDL2_KBTRANS_H
#define NP2_SDL2_KBTRANS_H

#if defined(__LIBRETRO__)
#include <stdint.h>

extern uint16_t keys_to_poll[];
extern uint16_t keys_needed;

void init_lr_key_to_pc98();
void send_libretro_key_down(uint16_t key);
void send_libretro_key_up(uint16_t key);
#else	/* __LIBRETRO__ */
void sdlkbd_initialize(void);
void sdlkbd_keydown(UINT key);
void sdlkbd_keyup(UINT key);
#endif	/* __LIBRETRO__ */
#endif

