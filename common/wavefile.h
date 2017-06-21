/**
 * @file	wavefile.h
 * @brief	Interface of wave file
 */

#pragma once

typedef struct TagWaveFile	*WAVEFILEH;			/*!< Defines handle */

#ifdef __cplusplus
extern "C"
{
#endif

WAVEFILEH wavefile_create(const OEMCHAR *lpFilename, UINT nRate, UINT nBits, UINT nChannels);
UINT wavefile_write(WAVEFILEH hWave, const void *lpBuffer, UINT cbBuffer);
void wavefile_close(WAVEFILEH hWave);

#ifdef __cplusplus
}
#endif
