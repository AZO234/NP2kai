/**
 * @file	joymng.cpp
 * @brief	Joystick manager
 *
 * @author	$Author: yui $
 * @date	$Date: 2011/03/07 09:54:11 $
 */

#include "compiler.h"
#include "np2.h"
#include "joymng.h"
#include "menu.h"

#if !defined(__GNUC__)
#pragma comment(lib, "winmm.lib")
#endif	// !defined(__GNUC__)

enum {
	JOY_LEFT_BIT	= 0x04,
	JOY_RIGHT_BIT	= 0x08,
	JOY_UP_BIT		= 0x01,
	JOY_DOWN_BIT	= 0x02,
	JOY_BTN1_BIT	= 0x10,
	JOY_BTN2_BIT	= 0x20
};

static	REG8	joyflag = 0xff;
static	UINT8	joypad1btn[4];


void joymng_initialize(void) {

	JOYINFO		ji;
	int			i;

	if ((!joyGetNumDevs()) ||
		(joyGetPos(JOYSTICKID1, &ji) == JOYERR_UNPLUGGED)) {
		np2oscfg.JOYPAD1 |= 2;
	}
	for (i=0; i<4; i++) {
		joypad1btn[i] = 0xff ^
			((np2oscfg.JOY1BTN[i] & 3) << ((np2oscfg.JOY1BTN[i] & 4)?4:6));
	}
}

void joymng_sync(void) {

	np2oscfg.JOYPAD1 &= 0x7f;
	joyflag = 0xff;
}

REG8 joymng_getstat(void) {

	JOYINFOEX		ji;
	static DWORD nojoy_time = 0;

	if (np2oscfg.JOYPAD1 == 1){
		if(nojoy_time == 0 || GetTickCount() - nojoy_time > 5000){
			ji.dwSize = sizeof(JOYINFOEX);
			ji.dwFlags = JOY_RETURNALL;
			if(joyGetPosEx(JOYSTICKID1, &ji) == JOYERR_NOERROR) {
				np2oscfg.JOYPAD1 |= 0x80;
				joyflag = 0xff;
				if (ji.dwXpos < 0x4000U) {
					joyflag &= ~JOY_LEFT_BIT;
				}
				else if (ji.dwXpos > 0xc000U) {
					joyflag &= ~JOY_RIGHT_BIT;
				}
				if (ji.dwYpos < 0x4000U) {
					joyflag &= ~JOY_UP_BIT;
				}
				else if (ji.dwYpos > 0xc000U) {
					joyflag &= ~JOY_DOWN_BIT;
				}
				if (ji.dwButtons & JOY_BUTTON1) {
					joyflag &= joypad1btn[0];							// ver0.28
				}
				if (ji.dwButtons & JOY_BUTTON2) {
					joyflag &= joypad1btn[1];							// ver0.28
				}
				if (ji.dwButtons & JOY_BUTTON3) {
					joyflag &= joypad1btn[2];							// ver0.28
				}
				if (ji.dwButtons & JOY_BUTTON4) {
					joyflag &= joypad1btn[3];							// ver0.28
				}
				nojoy_time = 0;
			}else{
				nojoy_time = GetTickCount();
			}
		}
	}
	return(joyflag);
}

// joyflag	bit:0		up
// 			bit:1		down
// 			bit:2		left
// 			bit:3		right
// 			bit:4		trigger1 (rapid)
// 			bit:5		trigger2 (rapid)
// 			bit:6		trigger1
// 			bit:7		trigger2

