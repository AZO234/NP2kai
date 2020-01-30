#include "np2_tickcount.h"
#include <time.h>
#if defined(NP2_WIN)
#include <windows.h>
#elif defined(NP2_SDL2)
#include <SDL.h>
#endif

static int64_t initcount;

int64_t NP2_TickCount_GetCount(void) {
#if defined(NP2_WIN)
  LARGE_INTEGER count;
  QueryPerformanceCounter(&count);
  return count.QuadPart;
#elif defined(NP2_SDL2)
#if SDL_MAJOR_VERSION == 1
  return SDL_GetTicks();
#else
  return SDL_GetPerformanceCounter();
#endif
#else
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000000000 + ts.tv_nsec;
#endif
}

int64_t NP2_TickCount_GetFrequency(void) {
#if defined(NP2_WIN)
  LARGE_INTEGER freq;
  QueryPerformanceFrequency(&freq);
  return freq.QuadPart;
#elif defined(NP2_SDL2)
#if SDL_MAJOR_VERSION == 1
  return 100000000;
#else
  return SDL_GetPerformanceFrequency();
#endif
#else
  struct timespec res;
  clock_getres(CLOCK_MONOTONIC, &res);
  return 1000000000 / (res.tv_sec * 1000000000 + res.tv_nsec);
#endif
}

void NP2_TickCount_Initialize(void) {
  initcount = NP2_TickCount_GetCount();
}

int64_t NP2_TickCount_GetCountFromInit(void) {
  return NP2_TickCount_GetCount() - initcount;
}

