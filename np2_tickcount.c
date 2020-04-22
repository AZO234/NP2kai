#if defined(SUPPORT_NP2_TICKCOUNT)

#include "np2_tickcount.h"
#include <time.h>
#if defined(NP2_WIN)
#include <windows.h>
#elif defined(NP2_SDL2)
#include <SDL.h>
#elif defined(__LIBRETRO__)
#include <features/features_cpu.h>
#endif

static int64_t initcount;
#if defined(__LIBRETRO__)
static int64_t inittime;
static int64_t lastcount;
static int64_t lasttime;
#endif

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
#elif defined(__LIBRETRO__)
  lastcount = cpu_features_get_perf_counter();
  lasttime = cpu_features_get_time_usec();
  return lastcount;
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
#elif defined(__LIBRETRO__)
  int64_t nowcount = cpu_features_get_perf_counter();
  int64_t nowtime = cpu_features_get_time_usec();
  int64_t ret;
  if(nowtime > lasttime) {
    ret = ((nowcount - lastcount) / (nowtime - lasttime)) * 1000000;
  } else {
    ret = 0;
  }
  lastcount = nowtime;
  lasttime = nowtime;
  return ret;
#else
  struct timespec res;
  clock_getres(CLOCK_MONOTONIC, &res);
  return 1000000000 / (res.tv_sec * 1000000000 + res.tv_nsec);
#endif
}

void NP2_TickCount_Initialize(void) {
  initcount = NP2_TickCount_GetCount();
#if defined(__LIBRETRO__)
  inittime = cpu_features_get_time_usec();
  lastcount = initcount;
  lasttime = inittime;
#endif
}

int64_t NP2_TickCount_GetCountFromInit(void) {
  return NP2_TickCount_GetCount() - initcount;
}

#if !defined(_WINDOWS) && defined(SUPPORT_NP2_TICKCOUNT)
BOOL QueryPerformanceCounter(LARGE_INTEGER* count) {
  int64_t icount = NP2_TickCount_GetCount();
  COPY64(count, &icount);
  return TRUE;
}
BOOL QueryPerformanceFrequency(LARGE_INTEGER* freq) {
  int64_t ifreq = NP2_TickCount_GetFrequency();
  COPY64(freq, &ifreq);
  return TRUE;
}
#endif

#endif  // SUPPORT_NP2_TICKCOUNT

