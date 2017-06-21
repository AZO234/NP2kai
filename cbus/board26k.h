/**
 * @file	board26k.h
 * @brief	Interface of PC-9801-26K
 */

#pragma once

#include "pccore.h"

#ifdef __cplusplus
extern "C"
{
#endif

void board26k_reset(const NP2CFG *pConfig);
void board26k_bind(void);

#ifdef __cplusplus
}
#endif
