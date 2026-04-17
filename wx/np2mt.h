/* === NP2kai wx port - emulator thread management === */

#ifndef NP2_WX_NP2MT_H
#define NP2_WX_NP2MT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Call once at startup (before np2_multithread_start) */
void np2_multithread_initialize(void);
/* Call once at shutdown (after np2_multithread_stop) */
void np2_multithread_shutdown(void);

/* Start / stop the emulator thread */
void np2_multithread_start(void);
void np2_multithread_stop(void);

/* Pause the emulator thread and wait until it is actually idle.
 * Used for modal dialogs, preference changes, etc. */
void np2_multithread_suspend(void);
/* Resume after a suspend */
void np2_multithread_resume(void);

/* Short-term critical section — protects shared state (keyboard,
 * disk changes, autokey) from the UI thread side. */
void np2_multithread_enter_cs(void);
void np2_multithread_leave_cs(void);

/* Returns non-zero while the emulator thread is alive */
int  np2_multithread_enabled(void);
int  np2_multithread_Enabled(void);   /* alias used by wab.c */

#ifdef __cplusplus
}
#endif

#endif /* NP2_WX_NP2MT_H */
