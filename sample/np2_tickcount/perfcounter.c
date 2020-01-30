/* === NP2 threading library === (c) 2019 AZO */
/* performance counter sample (to stop, Ctrl+C) */

/* --- Windows --- */
/* > cl /Feperfcounter.exe -DNP2_WIN perfcounter.c ../../np2_tickcount.c */
/* --- POSIX --- */
/* $ gcc -o perfcounter -DNP2_X11 perfcounter.c ../../np2_tickcount.c */
/* --- SDL2 --- */
/* $ gcc -o perfcounter -DNP2_SDL2 perfcounter.c ../../np2_tickcount.c \ */
/*   `sdl2-config --cflags --libs` */

#include <stdio.h>
#if defined(NP2_WIN)
#include <windows.h>
#endif
#if defined(NP2_SDL2)
#include <SDL.h>
#endif

#include "../../np2_tickcount.h"

void main(void) {
#if defined(NP2_SDL2)
  SDL_Init(SDL_INIT_EVERYTHING);
#endif
  NP2_TickCount_Initialize();

#if defined(NP2_WIN)
  printf("Count      : %lld\n",    NP2_TickCount_GetCount());
  printf("Frequency  : %lld Hz\n", NP2_TickCount_GetFrequency());
  printf("Count(run) : %lld\n",    NP2_TickCount_GetCountFromInit());
#else
  printf("Count      : %ld\n",    NP2_TickCount_GetCount());
  printf("Frequency  : %ld Hz\n", NP2_TickCount_GetFrequency());
  printf("Count(run) : %ld\n",    NP2_TickCount_GetCountFromInit());
#endif
}
