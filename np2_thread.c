/* === NP2 threading library === (c) 2019 AZO */

#ifdef SUPPORT_NP2_THREAD

#include "np2_thread.h"

/* --- thread --- */

/* for caller */
void NP2_Thread_Create(NP2_Thread_t* pth, void *(*thread)(void *), void* param) {
#if defined(NP2_THREAD_WIN)
  *pth = (NP2_Thread_t)_beginthread((void (__cdecl *)(void *))thread, 0, param);
#elif defined(NP2_THREAD_POSIX)
  pthread_create(pth, NULL, thread, param);
#elif defined(NP2_THREAD_SDL2)
  *(SDL_Thread**)pth = SDL_CreateThread((SDL_ThreadFunction)thread, NULL, param);
#elif defined(NP2_THREAD_LR)
  pth = sthread_create(*thread, param);
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
#elif defined(NP2_THREAD_SDL2)
  return (intptr_t)retval;
#elif defined(NP2_THREAD_LR)
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
#elif defined(NP2_THREAD_SDL2)
  if(pth)
    SDL_WaitThread((SDL_Thread*)*pth, (int*)retval);
  pth = NULL;
#elif defined(NP2_THREAD_LR)
  (void)retval;
  if(pth)
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
#elif defined(NP2_THREAD_SDL2)
  if(pth)
    SDL_DetachThread((SDL_Thread*)*pth);
  *pth = NULL;
#elif defined(NP2_THREAD_LR)
  if(pth)
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
#elif defined(NP2_THREAD_SDL2)
  *(SDL_sem**)psem = SDL_CreateSemaphore(initcount);
#elif defined(NP2_THREAD_LR)
  *psem = ssem_new(initcount);
#endif
}

/* for caller */
void NP2_Semaphore_Destroy(NP2_Semaphore_t* psem) {
#if defined(NP2_THREAD_WIN)
  CloseHandle(*psem);
  *psem = NULL;
#elif defined(NP2_THREAD_POSIX)
  sem_destroy(psem);
  psem = NULL;
#elif defined(NP2_THREAD_SDL2)
  SDL_DestroySemaphore((SDL_sem*)*psem);
  *psem = NULL;
#elif defined(NP2_THREAD_LR)
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
#elif defined(NP2_THREAD_SDL2)
  SDL_SemWait((SDL_sem*)*psem);
#elif defined(NP2_THREAD_LR)
  ssem_wait(*psem);
#endif
}

/* for caller/callee */
void NP2_Semaphore_Release(NP2_Semaphore_t* psem) {
#if defined(NP2_THREAD_WIN)
  ReleaseSemaphore(*psem, 1, NULL);
#elif defined(NP2_THREAD_POSIX)
  sem_post(psem);
#elif defined(NP2_THREAD_SDL2)
  SDL_SemPost((SDL_sem*)*psem);
#elif defined(NP2_THREAD_LR)
  ssem_signal(*psem);
#endif
}

/* --- queue --- */

/* for caller */
void NP2_Queue_Create(NP2_Queue_t* pque) {
  memset(pque, 0, sizeof(NP2_Queue_t));
}

/* for caller */
void NP2_Queue_Destroy(NP2_Queue_t* pque) {
  NP2_Queue_Item_t* item;
  while(pque->first) {
    item = pque->first;
    pque->first = pque->first->next;
    free(item);
  }
}

/* for caller */
void NP2_Queue_Append(NP2_Queue_t* pque, NP2_Semaphore_t* psem, void* param) {
  NP2_Queue_Item_t* item = (NP2_Queue_Item_t*)malloc(sizeof(NP2_Queue_Item_t));
  item->next = NULL;
  item->param = param;

  NP2_Semaphore_Wait(psem);
  if(pque->first == NULL) {
    pque->first = item;
  } else {
    pque->last->next = item;
  }
  pque->last = item;
  NP2_Semaphore_Release(psem);
}

/* for callee */
void NP2_Queue_Shift(NP2_Queue_t* pque, NP2_Semaphore_t* psem, void** param) {
  NP2_Queue_Item_t* item;

  NP2_Semaphore_Wait(psem);
  if(param)
    *param = pque->first->param;
  item = pque->first;
  pque->first = pque->first->next;
  NP2_Semaphore_Release(psem);

  free(item);
}

/* for callee */
void NP2_Queue_Shift_Wait(NP2_Queue_t* pque, NP2_Semaphore_t* psem, void** param) {
  NP2_Queue_Item_t* item;

  do {
    NP2_Semaphore_Wait(psem);
    item = pque->first;
    NP2_Semaphore_Release(psem);
    if(!item)
      NP2_Sleep_ms(10);
  } while(!item);
  NP2_Queue_Shift(pque, psem, param);
}

#endif  /* SUPPORT_NP2_THREAD */
