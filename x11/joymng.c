/*-
 * Copyright (C) 2004 NONAKA Kimihiro <nonakap@gmail.com>
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

#include "compiler.h"

#if defined(SUPPORT_JOYSTICK)

#include "np2.h"

#include "joymng.h"

static struct {
	void *hdl;
	BOOL inited;

	joymng_devinfo_t **devlist;

	UINT8 pad1btn[NELEMENTS(np2oscfg.JOY1BTN)];
	REG8 flag;
} joyinfo = {
	NULL,
	FALSE,

	NULL,

	{ 0, },
	0xff,
};

typedef struct {
	SINT16	axis[JOY_NAXIS];
	UINT8	button[JOY_NBUTTON];
} JOYINFO_T;

static joymng_devinfo_t **joydrv_initialize(void);
static void joydrv_terminate(void);
static void *joydrv_open(const char *dev);
static void joydrv_close(void *hdl);
static BRESULT joydrv_getstat(void *hdl, JOYINFO_T *ji);

void
joymng_initialize(void)
{
	int i;

	if (!joyinfo.inited) {
		joyinfo.devlist = joydrv_initialize();
		if (joyinfo.devlist == NULL) {
			np2oscfg.JOYPAD1 |= 2;
		}
		joyinfo.inited = TRUE;
	}

	if (joyinfo.hdl) {
		joydrv_close(joyinfo.hdl);
		joyinfo.hdl = NULL;
	}
	if (np2oscfg.JOYPAD1 == 1) {
		joyinfo.hdl = joydrv_open(np2oscfg.JOYDEV[0]);
		if (joyinfo.hdl == NULL) {
			np2oscfg.JOYPAD1 |= 2;
		}
	}

	for (i = 0; i < JOY_NBUTTON; i++) {
		joyinfo.pad1btn[i] = 0xff ^ ((np2oscfg.JOY1BTN[i] & 3) << ((np2oscfg.JOY1BTN[i] & 4) ? 4 : 6));
	}
}

void
joymng_deinitialize(void)
{

	if (joyinfo.hdl) {
		joydrv_close(joyinfo.hdl);
		joyinfo.hdl = NULL;
	}
	if (joyinfo.devlist) {
		_MFREE(joyinfo.devlist);
		joyinfo.devlist = NULL;
	}
	joydrv_terminate();
	joyinfo.inited = FALSE;
	np2oscfg.JOYPAD1 &= 1;
}

joymng_devinfo_t **
joymng_get_devinfo_list(void)
{

	return joyinfo.devlist;
}

void
joymng_sync(void)
{

	np2oscfg.JOYPAD1 &= 0x7f;
	joyinfo.flag = 0xff;
}

REG8
joymng_getstat(void)
{
	JOYINFO_T ji;
	int i;

	if ((np2oscfg.JOYPAD1 == 1) && joyinfo.hdl) {
		if (joydrv_getstat(joyinfo.hdl, &ji) == SUCCESS) {
			np2oscfg.JOYPAD1 |= 0x80;
			joyinfo.flag = 0xff;

			/* X */
			if (ji.axis[0] > 0x4000) {
				joyinfo.flag &= ~JOY_RIGHT_BIT;
			} else if (ji.axis[0] < -0x4000) {
				joyinfo.flag &= ~JOY_LEFT_BIT;
			}

			/* Y */
			if (ji.axis[1] > 0x4000) {
				joyinfo.flag &= ~JOY_DOWN_BIT;
			} else if (ji.axis[1] < -0x4000) {
				joyinfo.flag &= ~JOY_UP_BIT;
			}

			/* button */
			for (i = 0; i < JOY_NBUTTON; ++i) {
				if (ji.button[i]) {
					joyinfo.flag &= joyinfo.pad1btn[i];
				}
			}
		}
	}

	return joyinfo.flag;
}

#if defined(USE_SDL_JOYSTICK)

#include <SDL.h>
#include <SDL_joystick.h>

typedef struct {
	joymng_devinfo_t	dev;
	SDL_Joystick		*joyhdl;
} joydrv_sdl_hdl_t;

static joymng_devinfo_t **
joydrv_initialize(void)
{
	char str[32];
	joydrv_sdl_hdl_t *shdl;
	joymng_devinfo_t **devlist = NULL;
	size_t allocsize;
	int ndrv = 0;
	int rv;
	int i, n;

	rv = SDL_InitSubSystem(SDL_INIT_JOYSTICK);
	if (rv < 0) {
		return NULL;
	}

	ndrv = SDL_NumJoysticks();
	if (ndrv <= 0) {
		goto sdl_err;
	}

	allocsize = sizeof(joymng_devinfo_t *) * (ndrv + 1);
	devlist = _MALLOC(allocsize, "joy device list");
	if (devlist == NULL) {
		goto sdl_err;
	}
	memset(devlist, 0, allocsize);

	for (n = 0, i = 0; i < ndrv; ++i) {
		g_snprintf(str, sizeof(str), "%d", i);
		devlist[n] = joydrv_open(str);
		if (devlist[n] == NULL) {
			continue;
		}
		shdl = (joydrv_sdl_hdl_t *)devlist[n];
		SDL_JoystickClose(shdl->joyhdl);
		shdl->joyhdl = NULL;
		n++;
	}
	devlist[n] = NULL;

	return devlist;

sdl_err:
	if (devlist) {
		for (i = 0; i < ndrv; ++i) {
			if (devlist[i]) {
				joydrv_close(devlist[i]);
			}
		}
		_MFREE(devlist);
	}

	SDL_QuitSubSystem(SDL_INIT_JOYSTICK);

	return NULL;
}

static void
joydrv_terminate(void)
{

	SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
}

static void *
joydrv_open(const char *dvname)
{
	joydrv_sdl_hdl_t *shdl = NULL;
	joymng_devinfo_t *dev;
	SDL_Joystick *joy = NULL;
	char *endptr;
	size_t allocsize;
	long lval;
	int drv;
	int ndrv;
	int naxis;
	int nbutton;
	int i;

	if (dvname == NULL) {
		goto sdl_err;
	}

	errno = 0;
	lval = strtol(dvname, &endptr, 10);
	if (dvname[0] == '\0' || *endptr != '\0') {
		goto sdl_err;
	}
	if (errno == ERANGE && (lval == LONG_MAX || lval == LONG_MIN)) {
		goto sdl_err;
	}
	if (lval < 0 || lval > INT_MAX) {
		goto sdl_err;
	}
	drv = (int)lval;

	ndrv = SDL_NumJoysticks();
	if (ndrv <= 0 || drv >= ndrv) {
		goto sdl_err;
	}

	joy = SDL_JoystickOpen(drv);
	if (joy == NULL) {
		goto sdl_err;
	}

	naxis = SDL_JoystickNumAxes(joy);
	if (naxis < 2 || naxis >= 255) {
		goto sdl_err;
	}
	nbutton = SDL_JoystickNumButtons(joy);
	if (nbutton < 2 || nbutton >= 255) {
		goto sdl_err;
	}

	allocsize = sizeof(joydrv_sdl_hdl_t);
	shdl = _MALLOC(allocsize, "SDL joystick handle");
	if (shdl == NULL) {
		goto sdl_err;
	}
	memset(shdl, 0, allocsize);

	shdl->joyhdl = joy;

	dev = &shdl->dev;
	dev->devindex = drv;
#if SDL_VERSION_ATLEAST(2, 0, 0)
	dev->devname = strdup(SDL_JoystickName(joy));
#else
	dev->devname = strdup(SDL_JoystickName(drv));
#endif
	dev->naxis = naxis;
	for (i = 0; i < JOY_NAXIS; ++i) {
		if (np2oscfg.JOYAXISMAP[0][i] < naxis) {
			dev->axis[i] = np2oscfg.JOYAXISMAP[0][i];
		} else {
			dev->axis[i] = JOY_AXIS_INVALID;
		}
	}
	dev->nbutton = nbutton;
	for (i = 0; i < JOY_NBUTTON; ++i) {
		if (np2oscfg.JOYBTNMAP[0][i] < nbutton) {
			dev->button[i] = np2oscfg.JOYBTNMAP[0][i];
		} else {
			dev->button[i] = JOY_BUTTON_INVALID;
		}
	}

	return shdl;

sdl_err:
	if (shdl) {
		if (shdl->dev.devname) {
			free(shdl->dev.devname);
			shdl->dev.devname = NULL;
		}
		_MFREE(shdl);
	}
	if (joy) {
		SDL_JoystickClose(joy);
	}
	return NULL;
}

static void
joydrv_close(void *hdl)
{
	joydrv_sdl_hdl_t *shdl = (joydrv_sdl_hdl_t *)hdl;
	joymng_devinfo_t *dev = &shdl->dev;
	SDL_Joystick *joy = shdl->joyhdl;

	if (joy) {
		SDL_JoystickClose(joy);
	}
	if (dev->devname) {
		free(dev->devname);
		dev->devname = NULL;
	}
	_MFREE(shdl);
}

static BRESULT
joydrv_getstat(void *hdl, JOYINFO_T *ji)
{
	joydrv_sdl_hdl_t *shdl = (joydrv_sdl_hdl_t *)hdl;
	joymng_devinfo_t *dev = &shdl->dev;
	SDL_Joystick *joy = shdl->joyhdl;
	int i;

	SDL_JoystickUpdate();

	for (i = 0; i < JOY_NAXIS; ++i) {
		ji->axis[i] = (dev->axis[i] == JOY_AXIS_INVALID) ? 0 :
		    SDL_JoystickGetAxis(joy, dev->axis[i]);
	}
	for (i = 0; i < JOY_NBUTTON; ++i) {
		ji->button[i] = (dev->button[i] == JOY_BUTTON_INVALID) ? 0 :
		    SDL_JoystickGetButton(joy, dev->button[i]);
	}

	return SUCCESS;
}
#endif	/* USE_SDL_JOYSTICK */

#endif	/* SUPPORT_JOYSTICK */
