/**
 * @file  sndmtcs.h
 * @brief Cross-platform critical-section helpers for sound drivers
 *
 * Provides SNDMTCS_DECL(name) / SNDMTCS_ENTER(name) / SNDMTCS_LEAVE(name) /
 * SNDMTCS_INIT(name) / SNDMTCS_TERM(name) macros.
 *
 * On Windows  → CRITICAL_SECTION
 * On SDL3     → SDL_Mutex
 * On pthread  → pthread_mutex_t
 * Otherwise   → no-ops
 */

#pragma once

#include <compiler.h>

#if defined(SUPPORT_MULTITHREAD)

#if defined(NP2_WIN)

#define SNDMTCS_DECL(n) \
    static int n##_cs_initialized = 0; \
    static CRITICAL_SECTION n##_cs

#define SNDMTCS_ENTER(n) \
    do { if ((n##_cs_initialized)) EnterCriticalSection(&(n##_cs)); } while(0)

#define SNDMTCS_LEAVE(n) \
    do { if ((n##_cs_initialized)) LeaveCriticalSection(&(n##_cs)); } while(0)

#define SNDMTCS_INIT(n) \
    do { if (!(n##_cs_initialized)) { \
        memset(&(n##_cs), 0, sizeof((n##_cs))); \
        InitializeCriticalSection(&(n##_cs)); \
        (n##_cs_initialized) = 1; } } while(0)

#define SNDMTCS_TERM(n) \
    do { if ((n##_cs_initialized)) { \
        memset(&(n##_cs), 0, sizeof((n##_cs))); \
        DeleteCriticalSection(&(n##_cs)); \
        (n##_cs_initialized) = 0; } } while(0)

#elif defined(USE_SDL) && USE_SDL >= 3

#define SNDMTCS_DECL(n) \
    static int n##_cs_initialized = 0; \
    static SDL_Mutex *n##_cs = NULL

#define SNDMTCS_ENTER(n) \
    do { if ((n##_cs_initialized) && (n##_cs)) SDL_LockMutex((n##_cs)); } while(0)

#define SNDMTCS_LEAVE(n) \
    do { if ((n##_cs_initialized) && (n##_cs)) SDL_UnlockMutex((n##_cs)); } while(0)

#define SNDMTCS_INIT(n) \
    do { if (!(n##_cs_initialized)) { \
        (n##_cs) = SDL_CreateMutex(); \
        (n##_cs_initialized) = 1; } } while(0)

#define SNDMTCS_TERM(n) \
    do { if ((n##_cs_initialized) && (n##_cs)) { \
        SDL_DestroyMutex((n##_cs)); \
        (n##_cs) = NULL; \
        (n##_cs_initialized) = 0; } } while(0)

#elif defined(SUPPORT_PTHREAD)

#include <pthread.h>

#define SNDMTCS_DECL(n) \
    static pthread_mutex_t n##_cs = PTHREAD_MUTEX_INITIALIZER; \
    static int n##_cs_initialized = 1

#define SNDMTCS_ENTER(n)  pthread_mutex_lock(&(n##_cs))
#define SNDMTCS_LEAVE(n)  pthread_mutex_unlock(&(n##_cs))
#define SNDMTCS_INIT(n)   do {} while(0)
#define SNDMTCS_TERM(n)   do {} while(0)

#else

#define SNDMTCS_DECL(n)
#define SNDMTCS_ENTER(n)  do {} while(0)
#define SNDMTCS_LEAVE(n)  do {} while(0)
#define SNDMTCS_INIT(n)   do {} while(0)
#define SNDMTCS_TERM(n)   do {} while(0)

#endif /* platform */

#else /* !SUPPORT_MULTITHREAD */

#define SNDMTCS_DECL(n)
#define SNDMTCS_ENTER(n)  do {} while(0)
#define SNDMTCS_LEAVE(n)  do {} while(0)
#define SNDMTCS_INIT(n)   do {} while(0)
#define SNDMTCS_TERM(n)   do {} while(0)

#endif /* SUPPORT_MULTITHREAD */
