/**
 * @file	boardmo.h
 * @brief	Interface of SNE Multimedia Orchestra
 */

#pragma once

#include <pccore.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct {
	int irqflag;
	int playing;
	int stoppending;
	int current_reg;
	int reg[0x10];
} MMOSTAT;

void boardmo_reset(const NP2CFG *pConfig);
void boardmo_bind(void);
void boardmo_unbind(void);
void boardmo_finalize(void);

#ifdef __cplusplus
}
#endif
