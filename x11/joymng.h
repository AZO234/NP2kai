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

#ifndef	NP2_X11_JOYMNG_H__
#define	NP2_X11_JOYMNG_H__

/*
 * joystick manager
 */

G_BEGIN_DECLS

#define	JOY_NAXIS		2
#define	JOY_NBUTTON		4
#define	JOY_AXIS_INVALID	0xff
#define	JOY_BUTTON_INVALID	0xff
#define	JOY_NAXIS_MAX		(JOY_AXIS_INVALID-1)
#define	JOY_NBUTTON_MAX		(JOY_BUTTON_INVALID-1)

enum {
	JOY_UP_BIT		= (1 << 0),
	JOY_DOWN_BIT		= (1 << 1),
	JOY_LEFT_BIT		= (1 << 2),
	JOY_RIGHT_BIT		= (1 << 3),
	JOY_RAPIDBTN1_BIT	= (1 << 4),
	JOY_RAPIDBTN2_BIT	= (1 << 5),
	JOY_BTN1_BIT		= (1 << 6),
	JOY_BTN2_BIT		= (1 << 7)
};

typedef struct {
	int devindex;
	char *devname;

	int naxis;
	int axis[JOY_NAXIS];

	int nbutton;
	int button[JOY_NBUTTON];
} joymng_devinfo_t;

#if defined(SUPPORT_JOYSTICK)

REG8 joymng_getstat(void);

// -- X11
void joymng_initialize(void);
void joymng_deinitialize(void);
joymng_devinfo_t **joymng_get_devinfo_list(void);
void joymng_sync(void);

#else	/* !SUPPORT_JOYSTICK */

#define	joymng_getstat()		(REG8)0xff

// -- X11
#define	joymng_initialize()		(np2oscfg.JOYPAD1 |= 2)
#define	joymng_deinitialize()		(np2oscfg.JOYPAD1 &= 1)
#define	joymng_get_devinfo_list()	NULL
#define	joymng_sync()

#endif	/* SUPPORT_JOYSTICK */

G_END_DECLS

#endif	/* NP2_X11_JOYMNG_H__ */
