/* === NP2 threading library === (c) 2019 AZO */

#ifdef SUPPORT_NP2_THREAD

#include "np2_thread.h"
#include <string.h>

/* --- thread --- */

/* for caller */
void NP2_Thread_Create(NP2_Thread_t* pth, void *(*thread)(void *), void* param) {
#if defined(NP2_THREAD_WIN)
  *pth = (NP2_Thread_t)_beginthread((void (__cdecl *)(void *))thread, 0, param);
#elif defined(NP2_THREAD_POSIX)
  pthread_create(pth, NULL, thread, param);
#elif defined(NP2_SDL2)
#if SDL_MAJOR_VERSION == 1
  *(SDL_Thread**)pth = SDL_CreateThread(thread, param);
#else
  *(SDL_Thread**)pth = SDL_CreateThread((SDL_ThreadFunction)thread, NULL, param);
#endif
#elif defined(__LIBRETRO__)
  *pth = sthread_create((void (*)(void*))thread, param);
#endif
}

/* for caller */
void NP2_Thread_Destroy(NP2_Thread_t* pth) {
  NP2_Thread_Detach(pth);
#if defined(NP2_THREAD_POSIX)
  pth = NULL;
#else
  *pth = NULL;
#endif
}

/* for callee */
int NP2_Thread_Exit(void* retval) {
#if defined(NP2_THREAD_WIN)
  (void)retval;
  _endthread();
  return 0;
#elif defined(NP2_THREAD_POSIX)
  pthread_exit(retval);
  return 0;
#elif defined(NP2_SDL2)
  return (intptr_t)retval;
#elif defined(__LIBRETRO__)
  (void)retval;
  return 0;
#endif
}

/* for caller */
void NP2_Thread_Wait(NP2_Thread_t* pth, void **retval) {
#if defined(NP2_THREAD_WIN)
  (void)retval;
  if(*pth)
    WaitForSingleObject(*pth, INFINITE);
#elif defined(NP2_THREAD_POSIX)
  if(pth)
    pthread_join(*pth, retval);
  pth = NULL;
#elif defined(NP2_SDL2)
  if(pth)
    SDL_WaitThread((SDL_Thread*)*pth, (int*)retval);
  pth = NULL;
#elif defined(__LIBRETRO__)
  (void)retval;
  if(*pth)
    sthread_join(*pth);
  *pth = NULL;
#endif
}

/* for caller */
void NP2_Thread_Detach(NP2_Thread_t* pth) {
#if defined(NP2_THREAD_WIN)
  if(*pth)
    CloseHandle(*pth);
  *pth = NULL;
#elif defined(NP2_THREAD_POSIX)
  if(pth)
    pthread_detach(*pth);
  pth = NULL;
#elif defined(NP2_SDL2)
  if(*pth)
#if SDL_MAJOR_VERSION == 1
    SDL_KillThread((SDL_Thread*)*pth);
#else
    SDL_DetachThread((SDL_Thread*)*pth);
#endif
  *pth = NULL;
#elif defined(__LIBRETRO__)
  if(*pth)
    sthread_detach(*pth);
  *pth = NULL;
#endif
}

/* --- semaphore --- */

/* for caller */
void NP2_Semaphore_Create(NP2_Semaphore_t* psem, const unsigned int initcount) {
#if defined(NP2_THREAD_WIN)
  *psem = CreateSemaphore(NULL, initcount, initcount, NULL);
#elif defined(NP2_THREAD_POSIX)
  sem_init(psem, 0, initcount);
#elif defined(NP2_SDL2)
  *(SDL_sem**)psem = SDL_CreateSemaphore(initcount);
#elif defined(__LIBRETRO__)
  *psem = ssem_new(initcount);
#endif
}

/* for caller */
void NP2_Semaphore_Destroy(NP2_Semaphore_t* psem) {
#if defined(NP2_THREAD_WIN)
  if(*psem)
    CloseHandle(*psem);
  *psem = NULL;
#elif defined(NP2_THREAD_POSIX)
  if(psem)
    sem_destroy(psem);
  psem = NULL;
#elif defined(NP2_SDL2)
  if(*psem)
    SDL_DestroySemaphore((SDL_sem*)*psem);
  *psem = NULL;
#elif defined(__LIBRETRO__)
  if(*psem)
    ssem_free(*psem);
  *psem = NULL;
#endif
}

/* for caller/callee */
void NP2_Semaphore_Wait(NP2_Semaphore_t* psem) {
#if defined(NP2_THREAD_WIN)
  WaitForSingleObject(*psem, INFINITE);
#elif defined(NP2_THREAD_POSIX)
  sem_wait(psem);
#elif defined(NP2_SDL2)
  SDL_SemWait((SDL_sem*)*psem);
#elif defined(__LIBRETRO__)
  ssem_wait(*psem);
#endif
}

/* for caller/callee */
void NP2_Semaphore_Release(NP2_Semaphore_t* psem) {
#if defined(NP2_THREAD_WIN)
  if(psem)
    ReleaseSemaphore(*psem, 1, NULL);
#elif defined(NP2_THREAD_POSIX)
  if(psem)
    sem_post(psem);
#elif defined(NP2_SDL2)
  if(*psem)
    SDL_SemPost((SDL_sem*)*psem);
#elif defined(__LIBRETRO__)
  if(*psem)
    ssem_signal(*psem);
#endif
}

/* --- wait queue --- */

/* for caller (ring) */
void NP2_WaitQueue_Ring_Create(NP2_WaitQueue_t* pque, unsigned int maxcount) {
  if(!pque || maxcount == 0) {
    return;
  }

  memset(pque, 0, sizeof(NP2_WaitQueue_t));
  pque->ring.type = NP2_WAITQUEUE_TYPE_RING;
  pque->ring.params = malloc(sizeof(void*) * maxcount);
  pque->ring.maxcount = maxcount;
}

/* for caller (ringint) */
void NP2_WaitQueue_RingInt_Create(NP2_WaitQueue_t* pque, unsigned int maxcount) {
  if(!pque || maxcount == 0) {
    return;
  }

  memset(pque, 0, sizeof(NP2_WaitQueue_t));
  pque->ring.type = NP2_WAITQUEUE_TYPE_RINGINT;
  pque->ring.params = malloc(sizeof(int) * maxcount);
  pque->ring.maxcount = maxcount;
}

/* for caller (list) */
void NP2_WaitQueue_List_Create(NP2_WaitQueue_t* pque) {
  if(pque != NULL) {
    memset(pque, 0, sizeof(NP2_WaitQueue_t));
    pque->list.type = NP2_WAITQUEUE_TYPE_LIST;
  }
}

/* for caller (ring,ringint,list) */
void NP2_WaitQueue_Destroy(NP2_WaitQueue_t* pque) {
  NP2_WaitQueue_List_Item_t* item;
  unsigned int i;

  if(pque) {
    switch(pque->ring.type) {
    case NP2_WAITQUEUE_TYPE_RING:
      for(i = 0; i < pque->ring.maxcount; i++) {
        if(((void**)(pque->ring.params))[i])
          free(((void**)(pque->ring.params))[i]);
      }
    case NP2_WAITQUEUE_TYPE_RINGINT:
      free(pque->ring.params);
      break;
    case NP2_WAITQUEUE_TYPE_LIST:
      while(pque->list.first) {
        item = pque->list.first;
        if(item->param)
          free(item->param);
        pque->list.first = pque->list.first->next;
        free(item);
      }
      break;
    }
    memset(pque, 0, sizeof(NP2_WaitQueue_t));
  }
}

/* for caller (ringint) */
void NP2_WaitQueue_RingInt_Append(NP2_WaitQueue_t* pque, NP2_Semaphore_t* psem, const int param) {
  NP2_WaitQueue_List_Item_t* item;

  if(pque && psem) {
    if(pque->ring.type == NP2_WAITQUEUE_TYPE_RINGINT) {
      NP2_Semaphore_Wait(psem);
      ((int*)(pque->ring.params))[pque->ring.queued] = param;
      if(pque->ring.queued + 1 >= pque->ring.maxcount) {
        pque->ring.queued = 0;
      } else {
        pque->ring.queued++;
      }
      NP2_Semaphore_Release(psem);
      if(pque->ring.queued == pque->ring.current) {
//        TRACEOUT("NP2_WaitQueue_Append: Error Queue is overlow.\n");
      }
    }
  }
}

/* for caller (ring,list) */
void NP2_WaitQueue_Append(NP2_WaitQueue_t* pque, NP2_Semaphore_t* psem, void* param) {
  NP2_WaitQueue_List_Item_t* item;

  if(pque && psem) {
    if(pque->ring.type == NP2_WAITQUEUE_TYPE_RING) {
      NP2_Semaphore_Wait(psem);
      ((void**)(pque->ring.params))[pque->ring.queued] = param;
      if(pque->ring.queued + 1 >= pque->ring.maxcount) {
        pque->ring.queued = 0;
      } else {
        pque->ring.queued++;
      }
      NP2_Semaphore_Release(psem);
      if(pque->ring.queued == pque->ring.current) {
//        TRACEOUT("NP2_WaitQueue_Append: Error Queue is overlow.\n");
      }
    } else {
      item = (NP2_WaitQueue_List_Item_t*)malloc(sizeof(NP2_WaitQueue_List_t));
      item->next = NULL;
      item->param = param;
      NP2_Semaphore_Wait(psem);
      if(pque->list.first == NULL) {
        pque->list.first = item;
      } else {
        pque->list.last->next = item;
      }
      pque->list.last = item;
      NP2_Semaphore_Release(psem);
    }
  }
}

/* for callee (ringint) */
void NP2_WaitQueue_RingInt_Shift(NP2_WaitQueue_t* pque, NP2_Semaphore_t* psem, int* param) {
  NP2_WaitQueue_List_Item_t* item;

  if(pque && psem && param) {
    if(pque->ring.type == NP2_WAITQUEUE_TYPE_RINGINT) {
      NP2_Semaphore_Wait(psem);
      if(pque->ring.queued == pque->ring.current) {
        *param = 0;
      } else {
        *param = ((int*)(pque->ring.params))[pque->ring.current];
        if(pque->ring.current + 1 >= pque->ring.maxcount) {
          pque->ring.current = 0;
        } else {
          pque->ring.current++;
        }
      }
      NP2_Semaphore_Release(psem);
    }
  }
}

/* for callee (ring,list) */
void NP2_WaitQueue_Shift(NP2_WaitQueue_t* pque, NP2_Semaphore_t* psem, void** param) {
  NP2_WaitQueue_List_Item_t* item;

  if(pque && psem && param) {
    if(pque->ring.type == NP2_WAITQUEUE_TYPE_RING) {
      NP2_Semaphore_Wait(psem);
      if(pque->ring.queued == pque->ring.current) {
        *param = NULL;
      } else {
        *param = ((void**)(pque->ring.params))[pque->ring.current];
        ((void**)(pque->ring.params))[pque->ring.current] = NULL;
        if(pque->ring.current + 1 >= pque->ring.maxcount) {
          pque->ring.current = 0;
        } else {
          pque->ring.current++;
        }
      }
      NP2_Semaphore_Release(psem);
    } else {
      NP2_Semaphore_Wait(psem);
      *param = pque->list.first->param;
      item = pque->list.first;
      pque->list.first = pque->list.first->next;
      NP2_Semaphore_Release(psem);
      free(item);
    }
  }
}

/* for callee (ringint) */
void NP2_WaitQueue_RingInt_Shift_Wait(NP2_WaitQueue_t* pque, NP2_Semaphore_t* psem, int* param) {
  void* item;

  if(pque && psem && param) {
    do {
      NP2_Semaphore_Wait(psem);
      if(pque->ring.type == NP2_WAITQUEUE_TYPE_RINGINT) {
        if(pque->ring.queued == pque->ring.current) {
          item = NULL;
        } else {
          item = (void*)1;
        }
      }
      NP2_Semaphore_Release(psem);
      if(!item)
        NP2_Sleep_ms(1);
    } while(!item);
    NP2_WaitQueue_RingInt_Shift(pque, psem, param);
  }
}

/* for callee (ring,list) */
void NP2_WaitQueue_Shift_Wait(NP2_WaitQueue_t* pque, NP2_Semaphore_t* psem, void** param) {
  void* item;

  if(pque && psem && param) {
    do {
      NP2_Semaphore_Wait(psem);
      if(pque->ring.type == NP2_WAITQUEUE_TYPE_RING) {
        if(pque->ring.queued == pque->ring.current) {
          item = NULL;
        } else {
          item = (void*)1;
        }
      } else {
        item = pque->list.first;
      }
      NP2_Semaphore_Release(psem);
      if(!item)
        NP2_Sleep_ms(1);
    } while(!item);
    NP2_WaitQueue_Shift(pque, psem, param);
  }
}

#endif  /* SUPPORT_NP2_THREAD */
