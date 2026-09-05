/**
 * @file hostdrv9x.h
 * @brief Interface of native Windows 9x host drive backend.
 */
#pragma once

#if defined(SUPPORT_HOSTDRV9X)

#define NP2HOSTDRV9X_FILES_MAX 4096

#ifdef __cplusplus
extern "C" {
#endif

void hostdrv9x_initialize(void);
void hostdrv9x_deinitialize(void);
void hostdrv9x_reset(void);
void hostdrv9x_bind(void);
void hostdrv9x_updateHDrvRoot(void);
int hostdrv9x_sfsave(STFLAGH sfh, const SFENTRY * tbl);
int hostdrv9x_sfload(STFLAGH sfh, const SFENTRY * tbl);

#ifdef __cplusplus
}
#endif

#endif
