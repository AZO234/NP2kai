/*
 * Copyright (c) 2002-2003 NONAKA Kimihiro
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

#include "gtk2/xnp2.h"
#include "gtk2/gtk_keyboard.h"


#define	NC	KEYBOARD_KC_NC

static const UINT8 xkeyconv_jis[256] = {
	/*	    ,    ,    ,    ,    ,    ,    ,    		; 0x00 */
		  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
	/*	    ,    ,    ,    ,    ,    ,    ,    		; 0x08 */
		  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
	/*	    ,    ,    ,    ,    ,    ,    ,    		; 0x10 */
		  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
	/*	    ,    ,    ,    ,    ,    ,    ,    		; 0x18 */
		  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
	/*	 SPC,  ! ,  " ,  # ,  $ ,  % ,  & ,  '		; 0x20 */
		0x34,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
	/*	  ( ,  ) ,  * ,  + ,  , ,  - ,  . ,  /		; 0x28 */
		0x08,0x09,0x27,0x26,0x30,0x0b,0x31,0x32,
	/*	  0 ,  1 ,  2 ,  3 ,  4 ,  5 ,  6 ,  7		; 0x30 */
		0x0a,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
	/*	  8 ,  9 ,  ; ,  : ,  < ,  = ,  > ,  ? 		; 0x38 */
		0x08,0x09,0x27,0x26,0x30,0x0b,0x31,0x32,
	/*	  @ ,  A ,  B ,  C ,  D ,  E ,  F ,  G		; 0x40 */
		0x1a,0x1d,0x2d,0x2b,0x1f,0x12,0x20,0x21,
	/*	  H ,  I ,  J ,  K ,  L ,  M ,  N ,  O		; 0x48 */
		0x22,0x17,0x23,0x24,0x25,0x2f,0x2e,0x18,
	/*	  P ,  Q ,  R ,  S ,  T ,  U ,  V ,  W		; 0x50 */
		0x19,0x10,0x13,0x1e,0x14,0x16,0x2c,0x11,
	/*	  X ,  Y ,  Z ,  [ ,  \ ,  ] ,  ^ ,  _		; 0x58 */
		0x2a,0x15,0x29,0x1b,0x0d,0x28,0x0c,0x33,
	/*	  ` ,  a ,  b ,  c ,  d ,  e ,  f ,  g		; 0x60 */
		0x1a,0x1d,0x2d,0x2b,0x1f,0x12,0x20,0x21,
	/*	  h ,  i ,  j ,  k ,  l ,  m ,  n ,  o		; 0x68 */
		0x22,0x17,0x23,0x24,0x25,0x2f,0x2e,0x18,
	/*	  p ,  q ,  r ,  s ,  t ,  u ,  v ,  w		; 0x70 */
		0x19,0x10,0x13,0x1e,0x14,0x16,0x2c,0x11,
	/*	  x ,  y ,  z ,  { ,  | ,  } ,  ~ ,   		; 0x78 */
		0x2a,0x15,0x29,0x1b,0x0d,0x28,0x0c,  NC,
	/*	    ,    ,    ,    ,    ,    ,    ,  		; 0x80 */
		  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
	/*	    ,    ,    ,    ,    ,    ,    ,   		; 0x88 */
		  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
	/*	    ,    ,    ,    ,    ,    ,    ,  		; 0x90 */
		  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
	/*	    ,    ,    ,    ,    ,    ,    ,   		; 0x98 */
		  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
	/*	    ,    ,    ,    ,    ,    ,    ,    		; 0xa0 */
		  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
	/*	    ,    ,    ,    ,    ,    ,    ,   		; 0xa8 */
		  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
	/*	    ,    ,    ,    ,    ,    ,    ,    		; 0xb0 */
		  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
	/*	    ,    ,    ,    ,    ,    ,    ,   		; 0xb8 */
		  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
	/*	    ,    ,    ,    ,    ,    ,    ,    		; 0xc0 */
		  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
	/*	    ,    ,    ,    ,    ,    ,    ,   		; 0xc8 */
		  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
	/*	    ,    ,    ,    ,    ,    ,    ,    		; 0xd0 */
		  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
	/*	    ,    ,    ,    ,    ,    ,    ,   		; 0xd8 */
		  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
	/*	    ,    ,    ,    ,    ,    ,    ,    		; 0xe0 */
		  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
	/*	    ,    ,    ,    ,    ,    ,    ,   		; 0xe8 */
		  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
	/*	    ,    ,    ,    ,    ,    ,    ,    		; 0xf0 */
		  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
	/*	    ,    ,    ,    ,    ,    ,    ,    		; 0xf8 */
		  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC
};

static const UINT8 xkeyconv_ascii[256] = {
	/*	    ,    ,    ,    ,    ,    ,    ,    		; 0x00 */
		  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
	/*	    ,    ,    ,    ,    ,    ,    ,    		; 0x08 */
		  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
	/*	    ,    ,    ,    ,    ,    ,    ,    		; 0x10 */
		  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
	/*	    ,    ,    ,    ,    ,    ,    ,    		; 0x18 */
		  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
	/*	 SPC,  ! ,  " ,  # ,  $ ,  % ,  & ,  '		; 0x20 */
		0x34,0x01,0x02,0x03,0x04,0x05,0x06,0x87,
	/*	  ( ,  ) ,  * ,  + ,  , ,  - ,  . ,  /		; 0x28 */
		0x08,0x09,0x27,0x26,0x30,0x0b,0x31,0x32,
	/*	  0 ,  1 ,  2 ,  3 ,  4 ,  5 ,  6 ,  7		; 0x30 */
		0x0a,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
	/*	  8 ,  9 ,  ; ,  : ,  < ,  = ,  > ,  ? 		; 0x38 */
		0x08,0x09,0xa7,0x26,0x30,0x8b,0x31,0x32,
	/*	  @ ,  A ,  B ,  C ,  D ,  E ,  F ,  G		; 0x40 */
		0x9a,0x1d,0x2d,0x2b,0x1f,0x12,0x20,0x21,
	/*	  H ,  I ,  J ,  K ,  L ,  M ,  N ,  O		; 0x48 */
		0x22,0x17,0x23,0x24,0x25,0x2f,0x2e,0x18,
	/*	  P ,  Q ,  R ,  S ,  T ,  U ,  V ,  W		; 0x50 */
		0x19,0x10,0x13,0x1e,0x14,0x16,0x2c,0x11,
	/*	  X ,  Y ,  Z ,  [ ,  \ ,  ] ,  ^ ,  _		; 0x58 */
		0x2a,0x15,0x29,0x1b,0x0d,0x28,0x8c,0x33,
	/*	  ` ,  a ,  b ,  c ,  d ,  e ,  f ,  g		; 0x60 */
		0x8c,0x1d,0x2d,0x2b,0x1f,0x12,0x20,0x21,
	/*	  h ,  i ,  j ,  k ,  l ,  m ,  n ,  o		; 0x68 */
		0x22,0x17,0x23,0x24,0x25,0x2f,0x2e,0x18,
	/*	  p ,  q ,  r ,  s ,  t ,  u ,  v ,  w		; 0x70 */
		0x19,0x10,0x13,0x1e,0x14,0x16,0x2c,0x11,
	/*	  x ,  y ,  z ,  { ,  | ,  } ,  ~ ,   		; 0x78 */
		0x2a,0x15,0x29,0x1b,0x0d,0x28,0x1a,  NC,
	/*	    ,    ,    ,    ,    ,    ,    ,  		; 0x80 */
		  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
	/*	    ,    ,    ,    ,    ,    ,    ,   		; 0x88 */
		  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
	/*	    ,    ,    ,    ,    ,    ,    ,  		; 0x90 */
		  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
	/*	    ,    ,    ,    ,    ,    ,    ,   		; 0x98 */
		  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
	/*	    ,    ,    ,    ,    ,    ,    ,    		; 0xa0 */
		  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
	/*	    ,    ,    ,    ,    ,    ,    ,   		; 0xa8 */
		  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
	/*	    ,    ,    ,    ,    ,    ,    ,    		; 0xb0 */
		  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
	/*	    ,    ,    ,    ,    ,    ,    ,   		; 0xb8 */
		  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
	/*	    ,    ,    ,    ,    ,    ,    ,    		; 0xc0 */
		  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
	/*	    ,    ,    ,    ,    ,    ,    ,   		; 0xc8 */
		  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
	/*	    ,    ,    ,    ,    ,    ,    ,    		; 0xd0 */
		  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
	/*	    ,    ,    ,    ,    ,    ,    ,   		; 0xd8 */
		  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
	/*	    ,    ,    ,    ,    ,    ,    ,    		; 0xe0 */
		  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
	/*	    ,    ,    ,    ,    ,    ,    ,   		; 0xe8 */
		  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
	/*	    ,    ,    ,    ,    ,    ,    ,    		; 0xf0 */
		  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
	/*	    ,    ,    ,    ,    ,    ,    ,    		; 0xf8 */
		  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
};

static const UINT8 xkeyconv_misc[256] = {
	/*	    ,    ,    ,    ,    ,    ,    ,  		; 0x00 */
		  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
	/*	  BS, TAB,  LF, CLR,    , RET,    ,   		; 0x08 */
		0x0e,0x0f,0x1c,0x47,  NC,0x1c,  NC,  NC,
	/*	    ,    ,    ,PAUS,SCRL,SYSQ,    ,  		; 0x10 */
		  NC,  NC,  NC,0x60,0x71,0x62,  NC,  NC,
	/*	    ,    ,    , ESC,    ,    ,    ,   		; 0x18 */
		  NC,  NC,  NC,0x00,  NC,  NC,  NC,  NC,
	/*	    ,KANJ,MUHE,HENM,HENK,RONM,HIRA,KATA		; 0x20 */
		  NC,  NC,0x51,0x35,0x35,0x72,0x72,0x72,
	/*	HIKA,ZENK,HANK,ZNHN,    ,KANA,    ,   		; 0x28 */
		0x72,  NC,  NC,  NC,  NC,0x72,  NC,  NC,
	/*	ALNU,    ,    ,    ,    ,    ,    ,    		; 0x30 */
		0x71,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
	/*	    ,    ,    ,    ,    ,ZKOU,MKOU,   		; 0x38 */
		  NC,  NC,  NC,  NC,  NC,0x35,0x35,  NC,
	/*	    ,    ,    ,    ,    ,    ,    ,    		; 0x40 */
		  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
	/*	    ,    ,    ,    ,    ,    ,    ,   		; 0x48 */
		  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
	/*	HOME,  ←,  ↑,  →,  ↓,RLDN,RLUP, END		; 0x50 */
		0x3e,0x3b,0x3a,0x3c,0x3d,0x37,0x36,0x3f,
	/*	    ,    ,    ,    ,    ,    ,    ,   		; 0x58 */
		  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
	/*	    ,PRNT, INS,    ,    ,    ,    ,    		; 0x60 */
		  NC,0x62,0x38,  NC,  NC,  NC,  NC,  NC,
	/*	    ,    ,    ,BREA,    ,    ,    ,   		; 0x68 */
		  NC,  NC,  NC,0x60,  NC,  NC,  NC,  NC,
	/*	    ,    ,    ,    ,    ,    ,    ,    		; 0x70 */
		  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
	/*	    ,    ,    ,    ,    ,    ,    ,    		; 0x78 */
		  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
	/*	<SPC,    ,    ,    ,    ,    ,    ,    		; 0x80 */
		0x34,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
	/*	    ,<TAB,    ,    ,    ,<ENT,    ,  		; 0x88 */
		  NC,0x0f,  NC,  NC,  NC,0x1c,  NC,  NC,
	/*	    ,    ,    ,    ,    ,<HOM,<←>,<↑>		; 0x90 */
		  NC,  NC,  NC,  NC,  NC,0x3e,0x3b,0x3a,
	/*	<→>,<↓>,<RDN,<RUP,<END,    ,<INS,<DEL		; 0x98 */
		0x3c,0x3d,0x37,0x36,0x3f,  NC,0x38,0x39,
	/*	    ,    ,    ,    ,    ,    ,    ,    		; 0xa0 */
		  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
	/*	    ,    , <*>, <+>, <,>, <->, <.>, </>		; 0xa8 */
		  NC,  NC,0x45,0x49,0x4f,0x40,0x50,0x41,
	/*	 <0>, <1>, <2>, <3>, <4>, <5>, <6>, <7>		; 0xb0 */
		0x4e,0x4a,0x4b,0x4c,0x46,0x47,0x48,0x42,
	/*	 <8>, <9>,    ,    ,    ,    , f.1, f.2		; 0xb8 */
		0x43,0x44,  NC,  NC,  NC,  NC,0x62,0x63,
	/*	 f.3, f.4, f.5, f.6, f.7, f.8, f.9,f.10		; 0xc0 */
		0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,
	/*	f.11,f.12,f.13,f.14,f.15,    ,    ,   		; 0xc8 */
		0x73,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
	/*	    ,    ,    ,    ,    ,    ,    ,    		; 0xd0 */
		  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
	/*	    ,    ,    ,    ,    ,    ,    ,   		; 0xd8 */
		  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
	/*	    ,SFTL,SFTR,CTLL,CTLR,CAPS,    ,METL		; 0xe0 */
		  NC,0x70,0x70,0x74,0x74,0x71,  NC,0x51,
	/*	METR,ALTL,ALTR,    ,    ,    ,    ,    		; 0xe8 */
		0x35,0x51,0x35,  NC,  NC,  NC,  NC,  NC,
	/*	    ,    ,    ,    ,    ,    ,    ,    		; 0xf0 */
		  NC,  NC,  NC,  NC,  NC,  NC,  NC,  NC,
	/*	    ,    ,    ,    ,    ,    ,    , DEL		; 0xf8 */
		  NC,  NC,  NC,  NC,  NC,  NC,  NC,0x39
};

static const UINT8 *xkeyconv = xkeyconv_jis;
static UINT8 shift_stat = 0x00;

BRESULT
kbdmng_init(void)
{

	shift_stat = 0x00;
	if (np2oscfg.KEYBOARD == KEY_KEY101)
		xkeyconv = xkeyconv_ascii;
	else
		xkeyconv = xkeyconv_jis;

	return SUCCESS;
}

static UINT8
get_data(guint keysym, UINT8 down)
{
	UINT8 data;

	if (keysym & ~0xff) {
		if (keysym == GDK_VoidSymbol) {
			data = NC;
		} else if (keysym == GDK_KEY_F12) {
			data = kbdmng_getf12key();
		} else if ((keysym & 0xff00) == 0xff00) {
			data = xkeyconv_misc[keysym & 0xff];
			if (data == 0x70) {
				shift_stat = down;
			}
		} else {
			data = NC;
		}
	} else {
		if ((keysym == GDK_asciitilde)
		 && shift_stat
		 && (np2oscfg.KEYBOARD == KEY_KEY106)) {
			data = 0x0a;	/* Shift + '0' -> '0', not '~' */
		} else {
			data = xkeyconv[keysym];
		}
	}

	return data;
}

void
gtkkbd_keydown(guint keysym)
{
	UINT8 data;

	data = get_data(keysym, 0x80);
	if (data != NC) {
		if ((data & 0x80) == 0) {
			keystat_senddata(data);
		} else {
			UINT8 s = (shift_stat & 0x80) | 0x70;
			keystat_senddata(s);
			keystat_senddata(data & 0x7f);
			keystat_senddata(s ^ 0x80);
		}
	}
}

void
gtkkbd_keyup(guint keysym)
{
	UINT8 data;

	data = get_data(keysym, 0x00);
	if (data != NC) {
		keystat_senddata(data | 0x80);
	}
}
