#ifndef NP2_TICKCOUNT_H
#define NP2_TICKCOUNT_H

#if defined(SUPPORT_NP2_TICKCOUNT)

#include "compiler.h"

#ifdef __cplusplus
extern "C" {
#endif

void NP2_TickCount_Initialize(void);

extern int64_t NP2_TickCount_GetCount(void);
extern int64_t NP2_TickCount_GetFrequency(void);
extern int64_t NP2_TickCount_GetCountFromInit(void);

#ifdef __cplusplus
}
#endif

#endif  // SUPPORT_NP2_TICKCOUNT

#endif  // NP2_TICKCOUNT_H

