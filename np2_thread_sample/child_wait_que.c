/* === NP2 threading library === (c) 2019 AZO */
/* child wait queue sample */

/* --- Windows --- */
/* > cl /Fechild_wait_que.exe -DSUPPORT_NP2_THREAD -DNP2_THREAD_WIN \ */
/*   child_wait_que.c ../np2_thread.c */
/* --- POSIX --- */
/* $ gcc -o child_wait_que -DSUPPORT_NP2_THREAD -DNP2_THREAD_POSIX \ */
/*   child_wait_que.c ../np2_thread.c -lpthread */
/* --- SDL2 --- */
/* $ gcc -o child_wait_que -DSUPPORT_NP2_THREAD -DNP2_THREAD_SDL2 \ */
/*   child_wait_que.c ../np2_thread.c  `sdl2-config --cflags --libs` */

#include <stdio.h>
#include <string.h>

#include "../np2_thread.h"

NP2_Semaphore_t sem;
NP2_Queue_t que;

void* child_wait_que(void* param) {
  while(1) {
    NP2_Queue_Shift_Wait(&que, &sem, NULL);
    printf("called parent!\n");
  }

/* not run */
  NP2_Thread_Exit(NULL);

  return NULL;
}

void main(void) {
	int i;

  NP2_Thread_t thread;

  memset(&que, 0, sizeof(NP2_Queue_t));
  NP2_Semaphore_Create(&sem, 1);

  NP2_Thread_Create(&thread, &child_wait_que, NULL);

  while(1) {
    i = rand() % 1000;
    printf("sleep %dms\n", i);
    NP2_Sleep_ms(i);
    printf("call child...\n");
    NP2_Queue_Append(&que, &sem, NULL);
  }

/* not run */
  NP2_Thread_Wait(&thread, NULL);

  NP2_Thread_Destroy(&thread);

  NP2_Semaphore_Destroy(&sem);
}
