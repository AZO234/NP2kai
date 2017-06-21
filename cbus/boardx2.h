/**
 * @file	boardx2.h
 * @brief	Interface of PC-9801-86 + 26K
 */

#pragma once

#include "pccore.h"

#ifdef __cplusplus
extern "C"
{
#endif

void boardx2_reset(const NP2CFG *pConfig);
void boardx2_bind(void);

#ifdef __cplusplus
}
#endif
