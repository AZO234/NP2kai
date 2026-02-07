/*
 * Copyright (c) 2001-2003, 2015 NONAKA Kimihiro
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <compiler.h>

#include <soundmng.h>

UINT8
snddrv_drv2num(const char* cfgstr)
{

	if (strcasecmp(cfgstr, "SDL") == 0)
		return SNDDRV_SDL;
	return SNDDRV_NODRV;
}

const char *
snddrv_num2drv(UINT8 num)
{

	switch (num) {
	case SNDDRV_SDL:
		return "SDL";
	}
	return "nosound";
}

#if !defined(NOSOUND)

#include <np2.h>
#include <pccore.h>
#include <ini.h>
#include <dosio.h>
#include <common/parts.h>

#include <sysmng.h>
#include <sound/sound.h>

#if defined(VERMOUTH_LIB)
#include "sound/vermouth/vermouth.h"

MIDIMOD vermouth_module = NULL;
#endif

/*
 * driver
 */
static struct {
	BRESULT (*drvinit)(UINT rate, UINT samples);
	BRESULT (*drvterm)(void);
	void (*drvlock)(void);
	void (*drvunlock)(void);

	void (*sndplay)(void);
	void (*sndstop)(void);

	void *(*pcmload)(UINT num, const char *path);
	void (*pcmdestroy)(void *cookie, UINT num);
	void (*pcmplay)(void *cookie, UINT num, BOOL loop);
	void (*pcmstop)(void *cookie, UINT num);
	void (*pcmvolume)(void *cookie, UINT num, int volume);
} snddrv;
#if USE_SDL_VERSION >= 3
static SDL_AudioStream* audio_st = NULL;
#else
static int audio_fd = -1;
#endif
static BOOL opened = FALSE;
static UINT opna_frame;

static BRESULT nosound_setup(void);
static BRESULT sdlaudio_setup(void);

static void PARTSCALL (*fnmix)(SINT16* dst, const SINT32* src, UINT size);

#if defined(GCC_CPU_ARCH_IA32)
void PARTSCALL _saturation_s16(SINT16 *dst, const SINT32 *src, UINT size);
void PARTSCALL _saturation_s16x(SINT16 *dst, const SINT32 *src, UINT size);
void PARTSCALL saturation_s16mmx(SINT16 *dst, const SINT32 *src, UINT size);
#endif

/*
 * PCM
 */
typedef struct {
	void *cookie;
	char *path;
	int volume;
} pcm_channel_t;
static pcm_channel_t *pcm_channel[SOUND_MAXPCM];

static void soundmng_pcminit(void);
static void soundmng_pcmdestroy(void);

#ifndef	PCM_VOULE_DEFAULT
#define	PCM_VOULE_DEFAULT	25
#endif
int pcm_volume_default = PCM_VOULE_DEFAULT;

/*
 * buffer
 */
#ifndef	NSOUNDBUFFER
#define	NSOUNDBUFFER	4
#endif
static struct sndbuf {
	struct sndbuf *next;
	char *buf;
	UINT size;
	UINT remain;
} sound_buffer[NSOUNDBUFFER];

static struct sndbuf *sndbuf_freelist;
#define	SNDBUF_FREELIST_FIRST()		(sndbuf_freelist)
#define	SNDBUF_FREELIST_INIT()						\
do {									\
	sndbuf_freelist = NULL;						\
} while (/*CONSTCOND*/0)
#define	SNDBUF_FREELIST_INSERT_HEAD(sndbuf)				\
do {									\
	(sndbuf)->next = sndbuf_freelist;				\
	sndbuf_freelist = (sndbuf);					\
} while (/*CONSTCOND*/0)
#define	SNDBUF_FREELIST_REMOVE_HEAD()					\
do {									\
	sndbuf_freelist = sndbuf_freelist->next;			\
} while (/*CONSTCOND*/0)

static struct {
	struct sndbuf *first;
	struct sndbuf **last;
} sndbuf_filled;
#define	SNDBUF_FILLED_QUEUE_FIRST()	(sndbuf_filled.first)
#define	SNDBUF_FILLED_QUEUE_INIT()					\
do {									\
	sndbuf_filled.first = NULL;					\
	sndbuf_filled.last = &sndbuf_filled.first;			\
} while (/*CONSTCOND*/0)
#define	SNDBUF_FILLED_QUEUE_INSERT_HEAD(sndbuf)				\
do {									\
	if (((sndbuf)->next = sndbuf_filled.first) == NULL)		\
		sndbuf_filled.last = &(sndbuf)->next;			\
	sndbuf_filled.first = (sndbuf);					\
} while (/*CONSTCOND*/0)
#define	SNDBUF_FILLED_QUEUE_INSERT_TAIL(sndbuf)				\
do {									\
	(sndbuf)->next = NULL;						\
	*sndbuf_filled.last = (sndbuf);					\
	sndbuf_filled.last = &(sndbuf)->next;				\
} while (/*CONSTCOND*/0)
#define	SNDBUF_FILLED_QUEUE_REMOVE_HEAD()				\
do {									\
	sndbuf_filled.first = sndbuf_filled.first->next;		\
	if (sndbuf_filled.first == NULL)				\
		sndbuf_filled.last = &sndbuf_filled.first;		\
} while (/*CONSTCOND*/0)

#define	sndbuf_lock()
#define	sndbuf_unlock()

static BRESULT buffer_init(void);
static void buffer_destroy(void);
static void buffer_clear(void);


static UINT
calc_blocksize(UINT size)
{
	UINT s = size;

	if (size & (size - 1))
		for (s = 32; s < size; s <<= 1)
			continue;
	return s;
}

static void
snddrv_setup(void)
{

	if (np2oscfg.snddrv < SNDDRV_DRVMAX) {
		switch (np2oscfg.snddrv) {
#if defined(SUPPORT_SDL_AUDIO)
		case SNDDRV_SDL:
			sdlaudio_setup();
			return;
#endif
		}
	} else {
#if defined(SUPPORT_SDL_AUDIO)
		if (sdlaudio_setup() == SUCCESS) {
			np2oscfg.snddrv = SNDDRV_SDL;
			sysmng_update(SYS_UPDATEOSCFG);
			return;
		} else
#endif
		{
			/* Nothing to do */
			/* fall thourgh "no match" */
		}
	}

	/* no match */
	nosound_setup();
	np2oscfg.snddrv = SNDDRV_NODRV;
	sysmng_update(SYS_UPDATEOSCFG);
}

static void
sounddrv_lock(void)
{

	(*snddrv.drvlock)();
}

static void
sounddrv_unlock(void)
{

	(*snddrv.drvunlock)();
}

UINT
soundmng_create(UINT rate, UINT bufmsec)
{
	pcm_channel_t *chan;
	UINT samples;
	int i;

	if (opened || ((rate != 11025) && (rate != 22050) && (rate != 44100) && (rate != 48000) && (rate != 88200) && (rate != 96000) && (rate != 176400) && (rate != 192000))) {
		return 0;
	}

	if (bufmsec < 20)
		bufmsec = 20;
	else if (bufmsec > 1000)
		bufmsec = 1000;

	for (i = 0; i < SOUND_MAXPCM; i++) {
		chan = pcm_channel[i];
		if (chan != NULL && chan->cookie != NULL) {
			soundmng_pcmstop(i);
			(*snddrv.pcmdestroy)(chan->cookie, i);
			chan->cookie = NULL;
		}
	}

	snddrv_setup();

	samples = (rate * bufmsec) / 1000 / 2;
	samples = calc_blocksize(samples);
	opna_frame = samples * 2 * sizeof(SINT16);

	if ((*snddrv.drvinit)(rate, samples) != SUCCESS) {
#if USE_SDL_VERSION >= 3
		audio_st = NULL;
#else
		audio_fd = -1;
#endif
		nosound_setup();
		np2oscfg.snddrv = SNDDRV_NODRV;
		sysmng_update(SYS_UPDATEOSCFG);
		return 0;
	}

#if defined(VERMOUTH_LIB)
	vermouth_module = midimod_create(rate);
	midimod_loadall(vermouth_module);
#endif

	soundmng_setreverse(FALSE);
	buffer_init();

	for (i = 0; i < SOUND_MAXPCM; i++) {
		chan = pcm_channel[i];
		if (chan != NULL && chan->path != NULL) {
			chan->cookie = (*snddrv.pcmload)(i, chan->path);
			if (chan->cookie != NULL)
				(*snddrv.pcmvolume)(chan->cookie, i, chan->volume);
		}
	}

	opened = TRUE;

	return samples;
}

void
soundmng_reset(void)
{

	sounddrv_lock();
	buffer_clear();
	sounddrv_unlock();
}

void
soundmng_destroy(void)
{
	UINT i;

	if (opened) {
#if defined(VERMOUTH_LIB)
		midimod_destroy(vermouth_module);
		vermouth_module = NULL;
#endif
		for (i = 0; i < SOUND_MAXPCM; i++) {
			soundmng_pcmstop(i);
		}
		(*snddrv.sndstop)();
		(*snddrv.drvterm)();
		nosound_setup();
#if USE_SDL_VERSION >= 3
		audio_st = NULL;
#else
		audio_fd = -1;
#endif
		opened = FALSE;
	}
}

void
soundmng_play(void)
{

	(*snddrv.sndplay)();
}

void
soundmng_stop(void)
{

	(*snddrv.sndstop)();
}

BRESULT
soundmng_initialize(void)
{

	snddrv_setup();
	soundmng_pcminit();

	return SUCCESS;
}

void
soundmng_deinitialize(void)
{

	soundmng_pcmdestroy();
	soundmng_destroy();
	buffer_destroy();
}

void
soundmng_sync(void)
{
	struct sndbuf *sndbuf;
	const SINT32 *pcm;

	if (opened) {
		sounddrv_lock();
		sndbuf = SNDBUF_FREELIST_FIRST();
		if (sndbuf != NULL) {
			SNDBUF_FREELIST_REMOVE_HEAD();
			sounddrv_unlock();

			pcm = sound_pcmlock();
			if(pcm) {
				(*fnmix)((SINT16 *)sndbuf->buf, pcm, opna_frame);
			}
			sound_pcmunlock(pcm);
			sndbuf->remain = sndbuf->size;

			sounddrv_lock();
			SNDBUF_FILLED_QUEUE_INSERT_TAIL(sndbuf);
		}
		sounddrv_unlock();
	}
}

void
soundmng_setreverse(BOOL reverse)
{

#if defined(GCC_CPU_ARCH_AMD64)
	if (!reverse) {
		if (mmxflag & (MMXFLAG_NOTSUPPORT|MMXFLAG_DISABLE)) {
			fnmix = satuation_s16;
		} else {
			fnmix = saturation_s16mmx;
		}
	} else {
		fnmix = satuation_s16x;
	}
#elif defined(GCC_CPU_ARCH_IA32)
	if (!reverse) {
		if (mmxflag & (MMXFLAG_NOTSUPPORT|MMXFLAG_DISABLE)) {
			fnmix = _saturation_s16;
		} else {
			fnmix = saturation_s16mmx;
		}
	} else {
		fnmix = _saturation_s16x;
	}
#else
	if (!reverse) {
		fnmix = satuation_s16;
	} else {
		fnmix = satuation_s16x;
	}
#endif
}

/*
 * PCM function
 */
static void
soundmng_pcminit(void)
{
	int i;

	for (i = 0; i < SOUND_MAXPCM; i++) {
		pcm_channel[i] = NULL;
	}
}

static void
soundmng_pcmdestroy(void)
{
	pcm_channel_t *chan;
	int i;

	for (i = 0; i < SOUND_MAXPCM; i++) {
		chan = pcm_channel[i];
		if (chan != NULL) {
			pcm_channel[i] = NULL;
			if (chan->cookie != NULL) {
				(*snddrv.pcmdestroy)(chan->cookie, i);
				chan->cookie = NULL;
			}
			if (chan->path != NULL) {
				_MFREE(chan->path);
				chan->path = NULL;
			}
		}
	}
}

BRESULT
soundmng_pcmload(UINT num, const char *filename)
{
	pcm_channel_t *chan;
	struct stat sb;
	int rv;

	if (num < SOUND_MAXPCM) {
		rv = stat(filename, &sb);
		if (rv < 0)
			return FAILURE;

		chan = pcm_channel[num];
		if (chan != NULL) {
			if (strcmp(filename, chan->path)) {
				_MFREE(chan->path);
				chan->path = strdup(filename);
			}
		} else {
			chan = _MALLOC(sizeof(*chan), "pcm channel");
			if (chan == NULL)
				return FAILURE;
			chan->cookie = NULL;
			chan->path = strdup(filename);
			chan->volume = pcm_volume_default;
			pcm_channel[num] = chan;
		}
		return SUCCESS;
	}
	return FAILURE;
}

void
soundmng_pcmvolume(UINT num, int volume)
{
	pcm_channel_t *chan;

	if (num < SOUND_MAXPCM) {
		chan = pcm_channel[num];
		if (chan != NULL) {
			chan->volume = volume;
			if (chan->cookie != NULL)
				(*snddrv.pcmvolume)(chan->cookie, num, volume);
		}
	}
}

BRESULT
soundmng_pcmplay(UINT num, BOOL loop)
{
	pcm_channel_t *chan;

	if (num < SOUND_MAXPCM) {
		chan = pcm_channel[num];
		if (chan != NULL && chan->cookie != NULL) {
			(*snddrv.pcmplay)(chan->cookie, num, loop);
		}
		return SUCCESS;
	}
	return FAILURE;
}

void
soundmng_pcmstop(UINT num)
{
	pcm_channel_t *chan;

	if (num < SOUND_MAXPCM) {
		chan = pcm_channel[num];
		if (chan != NULL && chan->cookie != NULL) {
			(*snddrv.pcmstop)(chan->cookie, num);
		}
	}
}

/*
 * sound buffer
 */
static BRESULT
buffer_init(void)
{
	BRESULT result = SUCCESS;
	int i;

	sounddrv_lock();
	for (i = 0; i < NSOUNDBUFFER; i++) {
		if (sound_buffer[i].buf != NULL) {
			_MFREE(sound_buffer[i].buf);
			sound_buffer[i].buf = NULL;
		}
		sound_buffer[i].buf = _MALLOC(opna_frame, "sound buffer");
		if (sound_buffer[i].buf == NULL) {
			g_printerr("buffer_init: can't alloc memory\n");
			while (--i >= 0) {
				_MFREE(sound_buffer[i].buf);
				sound_buffer[i].buf = NULL;
			}
			result = FAILURE;
			goto out;
		}
	}
	buffer_clear();
 out:
	sounddrv_unlock();
	return result;
}

static void
buffer_clear(void)
{
	int i;

	SNDBUF_FREELIST_INIT();
	SNDBUF_FILLED_QUEUE_INIT();

	for (i = 0; i < NSOUNDBUFFER; i++) {
		sound_buffer[i].next = &sound_buffer[i + 1];
		if (sound_buffer[i].buf != NULL)
			memset(sound_buffer[i].buf, 0, opna_frame);
		sound_buffer[i].size = sound_buffer[i].remain = opna_frame;
	}
	sound_buffer[NSOUNDBUFFER - 1].next = NULL;

	sndbuf_freelist = sound_buffer;
}

static void
buffer_destroy(void)
{
	int i;

	sounddrv_lock();

	SNDBUF_FREELIST_INIT();
	SNDBUF_FILLED_QUEUE_INIT();

	for (i = 0; i < NSOUNDBUFFER; i++) {
		sound_buffer[i].next = NULL;
		if (sound_buffer[i].buf != NULL) {
			_MFREE(sound_buffer[i].buf);
			sound_buffer[i].buf = NULL;
		}
	}

	sounddrv_unlock();
}

/*
 * No sound support
 */
static BRESULT
nosound_drvinit(UINT rate, UINT bufmsec)
{

	return SUCCESS;
}

static BRESULT
nosound_drvterm(void)
{

	return SUCCESS;
}

static void
nosound_drvlock(void)
{

	/* Nothing to do */
}

static void
nosound_drvunlock(void)
{

	/* Nothing to do */
}

static void
nosound_sndplay(void)
{

	/* Nothing to do */
}

static void
nosound_sndstop(void)
{

	/* Nothing to do */
}

static void *
nosound_pcmload(UINT num, const char *path)
{

	return NULL;
}

static void
nosound_pcmdestroy(void *cookie, UINT num)
{

	/* Nothing to do */
}

static void
nosound_pcmplay(void *cookie, UINT num, BOOL loop)
{

	/* Nothing to do */
}

static void
nosound_pcmstop(void *cookie, UINT num)
{

	/* Nothing to do */
}

static void
nosound_pcmvolume(void *cookie, UINT num, int volume)
{

	/* Nothing to do */
}

static BRESULT
nosound_setup(void)
{

	snddrv.drvinit = nosound_drvinit;
	snddrv.drvterm = nosound_drvterm;
	snddrv.drvlock = nosound_drvlock;
	snddrv.drvunlock = nosound_drvunlock;
	snddrv.sndplay = nosound_sndplay;
	snddrv.sndstop = nosound_sndstop;
	snddrv.pcmload = nosound_pcmload;
	snddrv.pcmdestroy = nosound_pcmdestroy;
	snddrv.pcmplay = nosound_pcmplay;
	snddrv.pcmstop = nosound_pcmstop;
	snddrv.pcmvolume = nosound_pcmvolume;

	return SUCCESS;
}

#if defined(GCC_CPU_ARCH_AMD64)
void PARTSCALL
_saturation_s16(SINT16 *dst, const SINT32 *src, UINT size)
{
	asm volatile (
		"movq	%0, %%rcx;"
		"movq	%1, %%rdx;"
		"movl	%2, %%ebx;"
		"shrl	$1, %%ebx;"
		"je	.ss16_ed;"
	".ss16_lp:"
		"movl	(%%rdx), %%eax;"
		"cmpl	$0x000008000, %%eax;"
		"jl	.ss16_min;"
		"movw	$0x7fff, %%ax;"
		"jmp	.ss16_set;"
	".ss16_min:"
		"cmpl	$0x0ffff8000, %%eax;"
		"jg	.ss16_set;"
		"movw	$0x8001, %%ax;"
	".ss16_set:"
		"leal	4(%%rdx), %%edx;"
		"movw	%%ax, (%%rcx);"
		"decl	%%ebx;"
		"leal	2(%%rcx), %%ecx;"
		"jne	.ss16_lp;"
	".ss16_ed:"
		: /* output */
		: "m" (dst), "m" (src), "m" (size)
		: "ebx");
}

void PARTSCALL
_saturation_s16x(SINT16 *dst, const SINT32 *src, UINT size)
{

	asm volatile (
		"movq	%0, %%rcx;"
		"movq	%1, %%rdx;"
		"movl	%2, %%ebx;"
		"shrl	$2, %%ebx;"
		"je	.ss16x_ed;"
	".ss16x_lp:"
		"movl	(%%rdx), %%eax;"
		"cmpl	$0x000008000, %%eax;"
		"jl	.ss16xl_min;"
		"movw	$0x7fff, %%ax;"
		"jmp	.ss16xl_set;"
	".ss16xl_min:"
		"cmpl	$0x0ffff8000, %%eax;"
		"jg	.ss16xl_set;"
		"movw	$0x8001, %%ax;"
	".ss16xl_set:"
		"movw	%%ax, 2(%%rcx);"
		"movl	4(%%rdx), %%eax;"
		"cmpl	$0x000008000, %%eax;"
		"jl	.ss16xr_min;"
		"movw	$0x7fff, %%ax;"
		"jmp	.ss16xr_set;"
	".ss16xr_min:"
		"cmpl	$0x0ffff8000, %%eax;"
		"jg	.ss16xr_set;"
		"mov	$0x8001, %%ax;"
	".ss16xr_set:"
		"movw	%%ax, (%%rcx);"
		"leal	8(%%rdx), %%edx;"
		"decl	%%ebx;"
		"leal	4(%%rcx), %%ecx;"
		"jne	.ss16x_lp;"
	".ss16x_ed:"
		: /* output */
		: "m" (dst), "m" (src), "m" (size)
		: "ebx");
}

void PARTSCALL
saturation_s16mmx(SINT16 *dst, const SINT32 *src, UINT size)
{

	asm volatile (
		"movq	%0, %%rcx;"
		"movq	%1, %%rdx;"
		"movl	%2, %%eax;"
		"shrl	$3, %%eax;"
		"je	.ss16m_ed;"
		"pxor	%%mm0, %%mm0;"
	".ss16m_lp:"
		"movq	(%%rdx), %%mm1;"
		"movq	8(%%rdx), %%mm2;"
		"packssdw %%mm2, %%mm1;"
		"leaq	16(%%rdx), %%rdx;"
		"movq	%%mm1, (%%rcx);"
		"leaq	8(%%rcx), %%rcx;"
		"dec	%%eax;"
		"jne	.ss16m_lp;"
		"emms;"
	".ss16m_ed:"
		: /* output */
		: "m" (dst), "m" (src), "m" (size));
}
#elif defined(GCC_CPU_ARCH_IA32)
void PARTSCALL
_saturation_s16(SINT16 *dst, const SINT32 *src, UINT size)
{
	asm volatile (
		"movl	%0, %%ecx;"
		"movl	%1, %%edx;"
		"movl	%2, %%ebx;"
		"shrl	$1, %%ebx;"
		"je	.ss16_ed;"
	".ss16_lp:"
		"movl	(%%edx), %%eax;"
		"cmpl	$0x000008000, %%eax;"
		"jl	.ss16_min;"
		"movw	$0x7fff, %%ax;"
		"jmp	.ss16_set;"
	".ss16_min:"
		"cmpl	$0x0ffff8000, %%eax;"
		"jg	.ss16_set;"
		"movw	$0x8001, %%ax;"
	".ss16_set:"
		"leal	4(%%edx), %%edx;"
		"movw	%%ax, (%%ecx);"
		"decl	%%ebx;"
		"leal	2(%%ecx), %%ecx;"
		"jne	.ss16_lp;"
	".ss16_ed:"
		: /* output */
		: "m" (dst), "m" (src), "m" (size)
		: "ebx");
}

void PARTSCALL
_saturation_s16x(SINT16 *dst, const SINT32 *src, UINT size)
{

	asm volatile (
		"movl	%0, %%ecx;"
		"movl	%1, %%edx;"
		"movl	%2, %%ebx;"
		"shrl	$2, %%ebx;"
		"je	.ss16x_ed;"
	".ss16x_lp:"
		"movl	(%%edx), %%eax;"
		"cmpl	$0x000008000, %%eax;"
		"jl	.ss16xl_min;"
		"movw	$0x7fff, %%ax;"
		"jmp	.ss16xl_set;"
	".ss16xl_min:"
		"cmpl	$0x0ffff8000, %%eax;"
		"jg	.ss16xl_set;"
		"movw	$0x8001, %%ax;"
	".ss16xl_set:"
		"movw	%%ax, 2(%%ecx);"
		"movl	4(%%edx), %%eax;"
		"cmpl	$0x000008000, %%eax;"
		"jl	.ss16xr_min;"
		"movw	$0x7fff, %%ax;"
		"jmp	.ss16xr_set;"
	".ss16xr_min:"
		"cmpl	$0x0ffff8000, %%eax;"
		"jg	.ss16xr_set;"
		"mov	$0x8001, %%ax;"
	".ss16xr_set:"
		"movw	%%ax, (%%ecx);"
		"leal	8(%%edx), %%edx;"
		"decl	%%ebx;"
		"leal	4(%%ecx), %%ecx;"
		"jne	.ss16x_lp;"
	".ss16x_ed:"
		: /* output */
		: "m" (dst), "m" (src), "m" (size)
		: "ebx");
}

void PARTSCALL
saturation_s16mmx(SINT16 *dst, const SINT32 *src, UINT size)
{

	asm volatile (
		"movl	%0, %%ecx;"
		"movl	%1, %%edx;"
		"movl	%2, %%eax;"
		"shrl	$3, %%eax;"
		"je	.ss16m_ed;"
		"pxor	%%mm0, %%mm0;"
	".ss16m_lp:"
		"movq	(%%edx), %%mm1;"
		"movq	8(%%edx), %%mm2;"
		"packssdw %%mm2, %%mm1;"
		"leal	16(%%edx), %%edx;"
		"movq	%%mm1, (%%ecx);"
		"leal	8(%%ecx), %%ecx;"
		"dec	%%eax;"
		"jne	.ss16m_lp;"
		"emms;"
	".ss16m_ed:"
		: /* output */
		: "m" (dst), "m" (src), "m" (size));
}
#endif	/* GCC_CPU_ARCH_AMD64 */

#if defined(SUPPORT_SDL_AUDIO)

#if USE_SDL_VERSION >= 3
#include <SDL3/SDL.h>
#elif USE_SDL_VERSION == 2
#include <SDL2/SDL.h>
#elif USE_SDL_VERSION == 1
#include <SDL/SDL.h>
#endif

#if USE_SDL_VERSION >= 3
static void sdlaudio_callback(void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount);
#else
static void sdlaudio_callback(void *, unsigned char *, int);
#endif

#if USE_SDL_VERSION == 2
static UINT8 sound_silence;
#endif

static BRESULT
sdlaudio_init(UINT rate, UINT samples)
{
	int rv;

	static SDL_AudioSpec fmt;
#if USE_SDL_VERSION >= 3
	fmt.format = SDL_AUDIO_S16;
	fmt.channels = 2;
	fmt.freq = rate;
#else
	fmt.freq = rate;
	fmt.format = AUDIO_S16SYS;
	fmt.channels = 2;
	fmt.samples = samples;
	fmt.callback = sdlaudio_callback;
	fmt.userdata = UINT32_TO_PTR(samples * 2 * sizeof(SINT16));
#endif

	rv = SDL_InitSubSystem(SDL_INIT_AUDIO);
	if (rv < 0) {
		g_printerr("sdlaudio_init: SDL_InitSubSystem(): %s\n",
		    SDL_GetError());
		return FAILURE;
	}

#if USE_SDL_VERSION >= 3
	audio_st = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &fmt, sdlaudio_callback, NULL);
	if (audio_st == NULL) {
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		return FAILURE;
	}
#else
	audio_fd = SDL_OpenAudio(&fmt, NULL);
	if (audio_fd < 0) {
		g_printerr("sdlaudio_init: SDL_OpenAudio(): %s\n",
		    SDL_GetError());
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		return FAILURE;
	}
#endif
#if USE_SDL_VERSION >= 3
	SDL_ResumeAudioStreamDevice(audio_st);
#endif

#if USE_SDL_VERSION == 2
	sound_silence = fmt.silence;
#endif
	return SUCCESS;
}

static BRESULT
sdlaudio_term(void)
{

#if USE_SDL_VERSION >= 3
	SDL_PauseAudioStreamDevice(audio_st);
	SDL_DestroyAudioStream(audio_st);
#else
	SDL_PauseAudio(1);
	SDL_CloseAudio();
#endif

	return SUCCESS;
}

static void
sdlaudio_lock(void)
{

#if USE_SDL_VERSION >= 3
	SDL_LockAudioStream(audio_st);
#else
	SDL_LockAudio();
#endif
}

static void
sdlaudio_unlock(void)
{

#if USE_SDL_VERSION >= 3
	SDL_UnlockAudioStream(audio_st);
#else
	SDL_UnlockAudio();
#endif
}

static void
sdlaudio_play(void)
{

#if USE_SDL_VERSION >= 3
	SDL_ResumeAudioStreamDevice(audio_st);
#else
	SDL_PauseAudio(0);
#endif
}

static void
sdlaudio_stop(void)
{

#if USE_SDL_VERSION >= 3
	SDL_PauseAudioStreamDevice(audio_st);
#else
	SDL_PauseAudio(1);
#endif
}

static BRESULT
sdlaudio_setup(void)
{

	snddrv.drvinit = sdlaudio_init;
	snddrv.drvterm = sdlaudio_term;
	snddrv.drvlock = sdlaudio_lock;
	snddrv.drvunlock = sdlaudio_unlock;
	snddrv.sndplay = sdlaudio_play;
	snddrv.sndstop = sdlaudio_stop;
	snddrv.pcmload = nosound_pcmload;
	snddrv.pcmdestroy = nosound_pcmdestroy;
	snddrv.pcmplay = nosound_pcmplay;
	snddrv.pcmstop = nosound_pcmstop;
	snddrv.pcmvolume = nosound_pcmvolume;

	return SUCCESS;
}

static void
#if USE_SDL_VERSION >= 3
sdlaudio_callback(void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount)
#else
sdlaudio_callback(void *userdata, unsigned char *stream, int len)
#endif
{
	const UINT frame_size = PTR_TO_UINT32(userdata);
	struct sndbuf *sndbuf;

#if USE_SDL_VERSION == 2
	/* 無音で初期化 */
	memset(stream, sound_silence, len);
#endif

	sndbuf_lock();

	sndbuf = SNDBUF_FILLED_QUEUE_FIRST();
	if (sndbuf == NULL) {
#if USE_SDL_VERSION >= 3
		/* SDL3では明示的に無音データを送る */
		static SINT16 silence[4096] = {0};
		int silence_size = MIN(additional_amount, sizeof(silence));
		SDL_PutAudioStreamData(stream, silence, silence_size);
#endif
		goto out;
	}


#if USE_SDL_VERSION >= 3
	while (sndbuf->remain < additional_amount) {
#else
	while (sndbuf->remain < len) {
#endif
		SNDBUF_FILLED_QUEUE_REMOVE_HEAD();
		sndbuf_unlock();

#if USE_SDL_VERSION >= 3
		SDL_SetAudioStreamGain(stream, (float)np2cfg.vol_master / 100);
		SDL_PutAudioStreamData(stream,
			sndbuf->buf + (sndbuf->size - sndbuf->remain),
			sndbuf->remain);
#elif USE_SDL_VERSION == 2
		SDL_MixAudioFormat(stream,
		    sndbuf->buf + (sndbuf->size - sndbuf->remain),
		    AUDIO_S16LSB,
		    sndbuf->remain, SDL_MIX_MAXVOLUME);
#else
		SDL_MixAudio(stream,
		    sndbuf->buf + (sndbuf->size - sndbuf->remain),
		    sndbuf->remain, SDL_MIX_MAXVOLUME);
#endif
#if USE_SDL_VERSION >= 3
		additional_amount -= sndbuf->remain;
#else
		stream += sndbuf->remain;
		len -= sndbuf->remain;
#endif
		sndbuf->remain = 0;

		sndbuf_lock();
		SNDBUF_FREELIST_INSERT_HEAD(sndbuf);
		sndbuf = SNDBUF_FILLED_QUEUE_FIRST();
		if (sndbuf == NULL)
			goto out;
	}

#if USE_SDL_VERSION >= 3
	if (sndbuf->remain == additional_amount) {
#else
	if (sndbuf->remain == len) {
#endif
		SNDBUF_FILLED_QUEUE_REMOVE_HEAD();
		sndbuf_unlock();
	}

#if USE_SDL_VERSION >= 3
	SDL_PutAudioStreamData(stream, sndbuf->buf + (sndbuf->size - sndbuf->remain),
		additional_amount);
	sndbuf->remain -= additional_amount;
#elif USE_SDL_VERSION == 2
	SDL_MixAudioFormat(stream, sndbuf->buf + (sndbuf->size - sndbuf->remain), AUDIO_S16SYS,
	    len, (int)(((float)SDL_MIX_MAXVOLUME / 100) * np2cfg.vol_master));
	sndbuf->remain -= len;
#elif USE_SDL_VERSION == 1
	SDL_MixAudio(stream, sndbuf->buf + (sndbuf->size - sndbuf->remain),
	    len, (int)(((float)SDL_MIX_MAXVOLUME / 100) * np2cfg.vol_master));
	sndbuf->remain -= len;
#endif

	if (sndbuf->remain == 0) {
		sndbuf_lock();
		SNDBUF_FREELIST_INSERT_HEAD(sndbuf);
	}
 out:
	sndbuf_unlock();
}

#endif	/* SUPPORT_SDL_AUDIO */

#endif	/* !NOSOUND */
