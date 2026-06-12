#ifndef NP2_TICKCOUNT_H
#define NP2_TICKCOUNT_H

#if defined(SUPPORT_NP2_TICKCOUNT)

#include <compiler.h>

#ifdef __cplusplus
extern "C" {
#endif

void NP2_TickCount_Initialize(void);

extern int64_t NP2_TickCount_GetCount(void);
extern int64_t NP2_TickCount_GetFrequency(void);
extern int64_t NP2_TickCount_GetCountFromInit(void);

#if !defined(_WINDOWS)
enum {
  TCMODE_DEFAULT = 0,
  TCMODE_GETTICKCOUNT = 1,
  TCMODE_TIMEGETTIME = 2,
  TCMODE_PERFORMANCECOUNTER = 3,
};
int GetTickCounterMode(void);
LARGE_INTEGER GetTickCounter_Clock(void);
LARGE_INTEGER GetTickCounter_ClockPerSec(void);
#endif

#ifdef __cplusplus
}
#endif

#endif  // SUPPORT_NP2_TICKCOUNT

#endif  // NP2_TICKCOUNT_H

