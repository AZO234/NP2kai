/**
 * @file	steptrap.h
 * @brief	Interface of the step trap
 */

#pragma once

#if defined(ENABLE_TRAP)

#ifdef __cplusplus
extern "C" {
#endif

void steptrap_hisfileout(void);

void CPUCALL steptrap(UINT cs, UINT32 eip);

#ifdef __cplusplus
}
#endif

#endif
