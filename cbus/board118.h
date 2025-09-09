/**
 * @file	board118.h
 * @brief	Interface of PC-9801-118
 */

#pragma once

#include <pccore.h>

#ifdef __cplusplus
extern "C"
{
#endif

void board118_reset(const NP2CFG *pConfig);
void board118_bind(void);
void board118_unbind(void);
void board118_finalize(void);

#ifdef __cplusplus
}
#endif
