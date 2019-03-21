/*
 * Copyright (c) 2003 NONAKA Kimihiro
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

#include "np2.h"
#include "pccore.h"
#include "iocore.h"

#include "kbdmng.h"

static const UINT8 kbdmng_f12keys[] = {
	0x61,	/* Copy */
	0x60,	/* Stop */
	0x4f,	/* tenkey [,] */
	0x4d,	/* tenkey [=] */
	0x76,	/* User1 */
	0x77,	/* User2 */
	0x3f,	/* Help */
};

UINT8
kbdmng_getf12key(void)
{
	int key;

	key = np2oscfg.F12KEY - 1; /* 0 is Mouse mode */
	if (key >= 0 && key < NELEMENTS(kbdmng_f12keys))
		return kbdmng_f12keys[key];
	return KEYBOARD_KC_NC;
}

void
kbdmng_resetf12(void)
{
	int i;

	for (i = 0; i < NELEMENTS(kbdmng_f12keys); i++) {
		keystat_forcerelease(kbdmng_f12keys[i]);
	}
}
