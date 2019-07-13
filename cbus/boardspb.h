/**
 * @file	boardspb.h
 * @brief	Interface of Speak board
 */

#pragma once

#include "pccore.h"

#ifdef __cplusplus
extern "C"
{
#endif

void boardspb_reset(const NP2CFG *pConfig, int opnaidx);
void boardspb_bind(void);
void boardspb_unbind(void);

void boardspr_reset(const NP2CFG *pConfig);
void boardspr_bind(void);
void boardspr_unbind(void);

#ifdef __cplusplus
}
#endif
