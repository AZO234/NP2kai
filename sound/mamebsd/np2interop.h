/**
 * @file	np2interop.h
 * @brief	Interface of np2 <-> mame opl (BSD-licensed Yamaha FM sound cores)
 */

#pragma once

#ifdef USE_MAME_BSD


#ifdef __cplusplus
extern "C" {
#endif

void* YMF262Init(INT clock, INT rate);
void YMF262ResetChip(void* chipptr, int samplerate);
void YMF262Shutdown(void* chipptr);
INT YMF262Write(void* chipptr, INT a, INT v);
UINT8 YMF262Read(void* chipptr, INT a);
void YMF262UpdateOne(void* chipptr, INT16** buffer, INT length);
int YMF262FlagSave(void* chipptr, void* dstbuf);
int YMF262FlagLoad(void* chipptr, void* srcbuf, int size);


#ifdef __cplusplus
}
#endif

#endif