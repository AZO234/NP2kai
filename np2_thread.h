/* === NP2 threading library === (c) 2019 AZO */

#ifndef _NP2_THREAD_H_
#define _NP2_THREAD_H_

#ifdef SUPPORT_NP2_THREAD

#include "compiler.h"
#include <stdlib.h>

#if defined(NP2_THREAD_WIN)
#include <windows.h>
#include <process.h>
#elif defined(NP2_THREAD_POSIX)
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#elif defined(NP2_THREAD_SDL2)
#if defined(USE_SDL_CONFIG)
#include "SDL.h"
#else
#include <SDL2\SDL.h>
#endif
#elif defined(NP2_THREAD_LR)
#include <rthreads/rthreads.h>
#include <rthreads/rsemaphore.h>
#include <retro_timers.h>
#endif

#if defined(NP2_THREAD_WIN)
typedef HANDLE NP2_Thread_t;
#elif defined(NP2_THREAD_POSIX)
typedef pthread_t NP2_Thread_t;
#elif defined(NP2_THREAD_SDL2)
typedef SDL_Thread* NP2_Thread_t;
#elif defined(NP2_THREAD_LR)
typedef sthread_t* NP2_Thread_t;
#endif

/* --- thread --- */

/* for caller */
void NP2_Thread_Create(NP2_Thread_t* pth, void *(*thread) (void *), void* param);
/* for caller */
void NP2_Thread_Destroy(NP2_Thread_t* pth);
/* for callee */
int NP2_Thread_Exit(void* retval);
/* for caller */
void NP2_Thread_Wait(NP2_Thread_t* pth, void **retval);
/* for caller */
void NP2_Thread_Detach(NP2_Thread_t* pth);

/* --- semaphore --- */

#if defined(NP2_THREAD_WIN)
typedef HANDLE NP2_Semaphore_t;
#elif defined(NP2_THREAD_POSIX)
typedef sem_t NP2_Semaphore_t;
#elif defined(NP2_THREAD_SDL2)
typedef SDL_sem* NP2_Semaphore_t;
#elif defined(NP2_THREAD_LR)
typedef ssem_t* NP2_Semaphore_t;
#endif

/* for caller */
void NP2_Semaphore_Create(NP2_Semaphore_t* psem, const unsigned int initcount);
/* for caller */
void NP2_Semaphore_Destroy(NP2_Semaphore_t* psem);
/* for caller/callee */
void NP2_Semaphore_Wait(NP2_Semaphore_t* psem);
/* for caller/callee */
void NP2_Semaphore_Release(NP2_Semaphore_t* psem);

/* --- queue --- */

typedef struct NP2_Queue_Item_t_ {
  struct NP2_Queue_Item_t_* next;
  void* param;
} NP2_Queue_Item_t;

typedef struct NP2_Queue_t_ {
  NP2_Queue_Item_t* first;
  NP2_Queue_Item_t* last;
} NP2_Queue_t;

/* for caller */
void NP2_Queue_Create(NP2_Queue_t* pque);
/* for caller */
void NP2_Queue_Destroy(NP2_Queue_t* pque);
/* for caller */
void NP2_Queue_Append(NP2_Queue_t* pque, NP2_Semaphore_t* psem, void* param);
/* for callee */
void NP2_Queue_Shift(NP2_Queue_t* pque, NP2_Semaphore_t* psem, void** param);
/* for callee */
void NP2_Queue_Shift_Wait(NP2_Queue_t* pque, NP2_Semaphore_t* psem, void** param);

/* --- sleep ms --- */

#if defined(NP2_THREAD_WIN)
#define NP2_Sleep_ms(ms) Sleep(ms);
#elif defined(NP2_THREAD_POSIX)
#define NP2_Sleep_ms(ms) usleep(ms * 1000);
#elif defined(NP2_THREAD_SDL2)
#define NP2_Sleep_ms(ms) SDL_Delay(ms);
#elif defined(NP2_THREAD_LR)
#define NP2_Sleep_ms(ms) retro_sleep(ms);
#endif

#endif  /* SUPPORT_NP2_THREAD */
#endif  /* _NP2_THREAD_H_ */

