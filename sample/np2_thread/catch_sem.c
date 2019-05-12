/* === NP2 threading library === (c) 2019 AZO */
/* catch semaphore sample (to stop, Ctrl+C) */

/* --- Windows --- */
/* > cl /Fecatch_sem.exe -DSUPPORT_NP2_THREAD -DNP2_THREAD_WIN \ */
/*   catch_sem.c ../../np2_thread.c */
/* --- POSIX --- */
/* $ gcc -o catch_sem -DSUPPORT_NP2_THREAD -DNP2_THREAD_POSIX \ */
/*   catch_sem.c ../../np2_thread.c -lpthread */
/* --- SDL2 --- */
/* $ gcc -o catch_sem -DSUPPORT_NP2_THREAD -DNP2_THREAD_SDL2 \ */
/*   catch_sem.c ../../np2_thread.c `sdl2-config --cflags --libs` */

#include <stdio.h>

#include "../../np2_thread.h"

#define THREAD_NUM 4

NP2_Semaphore_t sem;

static void* catch_sem(void* param) {
  int i = *(int*)param;

  while(1) {
    NP2_Semaphore_Wait(&sem);
    printf("%d ", i);
    NP2_Semaphore_Release(&sem);
  }

/* not run */
  NP2_Thread_Exit(NULL);

  return NULL;
}

void main(void) {
  int i;

  int num[THREAD_NUM];
  NP2_Thread_t threads[THREAD_NUM];
  NP2_Semaphore_Create(&sem, 1);

  for(i = 0; i < THREAD_NUM; i++) {
    num[i] = i;
    NP2_Thread_Create(&threads[i], &catch_sem, &num[i]);
  }

  for(i = 0; i < THREAD_NUM; i++)
    NP2_Thread_Wait(&threads[i], NULL);
/* not run */
  for(i = 0; i < THREAD_NUM; i++)
    NP2_Thread_Destroy(&threads[i]);

  NP2_Semaphore_Destroy(&sem);
}
