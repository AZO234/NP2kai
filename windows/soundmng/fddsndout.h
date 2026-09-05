/**
 * @file	fddsndout.h
 * @brief	FDD / board(relay) operation-sound separate output-device manager.
 *          Windows (DirectSound3) only. Enabled by SUPPORT_FDDSNDDEV.
 *
 * The FDD seek sounds (SOUND_PCMSEEK / SOUND_PCMSEEK1) and the window
 * accelerator board relay sound (SOUND_RELAY1) can each be routed to a
 * separate audio output device, independently of the main device.
 */

#pragma once

#if defined(SUPPORT_FDDSNDDEV)

#ifdef __cplusplus
extern "C"
{
#endif

/* sound groups */
#define	FDDSND_GROUP_FDD	0	/* SOUND_PCMSEEK, SOUND_PCMSEEK1 */
#define	FDDSND_GROUP_BOARD	1	/* SOUND_RELAY1 */
#define	FDDSND_GROUP_COUNT	2

/* store the main window handle (needed to open DirectSound devices) */
void fddsndout_setwindow(HWND hWnd);

/* (re)open the aux output devices according to np2cfg. Safe to call repeatedly.
   Slow device Open() runs outside the lock; devices are swapped atomically. */
void fddsndout_reconfig(void);

/* release the aux output devices (call on sound-manager shutdown). */
void fddsndout_close(void);

/* stop every aux-routed sound (called from CSoundMng::Disable / mute). */
void fddsndout_stopall(void);

/*
 * Dispatch hooks called from soundmng_pcmplay / soundmng_pcmstop.
 *   return TRUE  -> handled by an aux device (routed there); caller must NOT
 *                   play/stop on the main device. When muted, a routed sound
 *                   is still reported handled but not actually played.
 *   return FALSE -> not separated; caller uses the main device as usual.
 */
BOOL fddsndout_pcmplay(UINT nNum, BOOL bLoop);
BOOL fddsndout_pcmstop(UINT nNum);

#ifdef __cplusplus
}
#endif

#endif	/* SUPPORT_FDDSNDDEV */
