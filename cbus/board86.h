/**
 * @file	board86.h
 * @brief	Interface of PC-9801-86
 */

#pragma once

#include "pccore.h"

#ifdef __cplusplus
extern "C"
{
#endif

void board86_reset(const NP2CFG *pConfig, BOOL adpcm);
void board86_bind(void);

#ifdef __cplusplus
}
#endif
