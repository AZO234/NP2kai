/* === NP2kai wx port - emulator thread management === */

#include <compiler.h>
#include "np2mt.h"
#include <np2_thread.h>
#include <nevent.h>

/* ---- state ---- */

static NP2_Thread_t  s_emuthread  = NULL;
static SDL_Mutex    *s_cs         = NULL;
static volatile int  s_running    = 0;
static volatile int  s_pause_req  = 0;   /* set by UI thread to request pause */
static volatile int  s_paused     = 0;   /* set by emu thread when idle */

/* ---- emulator thread ---- */

extern "C" void np2_exec(void);

static int emu_thread_func(void * /*arg*/)
{
	while (s_running) {
		if (s_pause_req) {
			s_paused = 1;
			SDL_Delay(5);
			continue;
		}
		s_paused = 0;
		np2_exec();
	}
	s_paused = 1;
	return 0;
}

/* ---- public API ---- */

void np2_multithread_initialize(void)
{
	s_cs = SDL_CreateMutex();
#if defined(SUPPORT_MULTITHREAD)
	nevent_initialize();
#endif
}

void np2_multithread_shutdown(void)
{
	np2_multithread_stop();
#if defined(SUPPORT_MULTITHREAD)
	nevent_shutdown();
#endif
	if (s_cs) {
		SDL_DestroyMutex(s_cs);
		s_cs = NULL;
	}
}

void np2_multithread_start(void)
{
	if (s_emuthread) return;
	s_running   = 1;
	s_pause_req = 0;
	s_paused    = 0;
	NP2_Thread_Create(&s_emuthread,
	                  (void *(*)(void *))emu_thread_func, NULL);
}

void np2_multithread_stop(void)
{
	if (!s_emuthread) return;
	s_pause_req = 0;   /* unblock if paused */
	s_running   = 0;
	NP2_Thread_Wait(&s_emuthread, NULL);
	s_emuthread = NULL;
}

void np2_multithread_suspend(void)
{
	s_pause_req = 1;
	/* wait until emulator thread reports it is idle (≤ 1 frame + margin) */
	int timeout_ms = 2000;
	while (!s_paused && timeout_ms > 0) {
		SDL_Delay(5);
		timeout_ms -= 5;
	}
}

void np2_multithread_resume(void)
{
	s_paused    = 0;
	s_pause_req = 0;
}

void np2_multithread_enter_cs(void)
{
	if (s_cs) SDL_LockMutex(s_cs);
}

void np2_multithread_leave_cs(void)
{
	if (s_cs) SDL_UnlockMutex(s_cs);
}

int np2_multithread_enabled(void)
{
	return s_emuthread != NULL;
}

/* Alias with capital E — called by wab.c */
int np2_multithread_Enabled(void)
{
	return np2_multithread_enabled();
}
