/**
 * @file	opntimer.h
 * @brief	Interface of OPN timer
 */

#pragma once

#include "nevent.h"
#include "opna.h"

#ifdef __cplusplus
extern "C" {
#endif

void fmport_a(NEVENTITEM item);
void fmport_b(NEVENTITEM item);

void opna_timer(POPNA opna, UINT nIrq, NEVENTID nTimerA, NEVENTID nTimerB);
void opna_settimer(POPNA opna, REG8 cData);

#ifdef __cplusplus
}
#endif
