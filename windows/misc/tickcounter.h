/**
 * @file	tickcounter.h
 * @brief	TICK カウンタの宣言およびインターフェイスの定義をします
 */

#pragma once

enum {
	TCMODE_DEFAULT = 0,
	TCMODE_GETTICKCOUNT = 1,
	TCMODE_TIMEGETTIME = 2,
	TCMODE_PERFORMANCECOUNTER = 3,
};

#ifdef __cplusplus
extern "C"
{
#endif	// __cplusplus

DWORD GetTickCounter();
void SetTickCounterMode(int mode);
int GetTickCounterMode();
LARGE_INTEGER GetTickCounter_Clock();
LARGE_INTEGER GetTickCounter_ClockPerSec();

#ifdef __cplusplus
}
#endif	// __cplusplus
