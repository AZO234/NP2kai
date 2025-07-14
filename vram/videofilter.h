#ifndef _VIDEOFILTER_H_
#define _VIDEOFILTER_H_

#include <compiler.h>

#define VF_PROFILE_COUNT 3
#define VF_FILTER_COUNT  3
#define VF_PARAM_COUNT   6

#define GETR(c) (uint8_t)(((c) & 0x00FF0000) >> 16)
#define GETG(c) (uint8_t)(((c) & 0x0000FF00) >>  8)
#define GETB(c) (uint8_t)( (c) & 0x000000FF       )
#define SETRGB(r, g, b) ((uint32_t)(b) | ((uint32_t)(g) << 8) | ((uint32_t)(r) << 16))
#define GETH(c) (uint16_t)(((c) & 0x01FF0000) >> 16)
#define GETS(c) (uint8_t) (((c) & 0x0000FF00) >>  8)
#define GETV(c) (uint8_t) ( (c) & 0x000000FF)
#define SETHSV(h, s, v) ((uint32_t)(v) | ((uint32_t)(s) << 8) | ((uint32_t)(h) << 16))
#define GETZ(c) (uint8_t)(((c) & 0x00FF0000) >> 16)
#define GETY(c) (uint8_t)(((c) & 0x0000FF00) >>  8)
#define GETX(c) (uint8_t)( (c) & 0x000000FF       )
#define SETXYZ(x, y, z) ((uint32_t)(z) | ((uint32_t)(y) << 8) | ((uint32_t)(x) << 16))

#if defined(__cplusplus)
extern "C" {
#endif

typedef void* h_VideoFilterMng;

extern h_VideoFilterMng hVFMng1;

h_VideoFilterMng VideoFilter_Init(const uint16_t u16MaxWidth, const uint16_t u16MaxHeight, const uint8_t u8MaxRadius, const uint8_t u8MaxSample);
void VideoFilter_Deinit(h_VideoFilterMng hMng);

void VideoFilterMng_LoadSetting(h_VideoFilterMng hMng, const BOOL bEnable, const uint8_t u8ProfileCount, const uint8_t u8ProfileNo);
void VideoFilter_LoadProfile(h_VideoFilterMng hMng, const uint8_t u8ProfileNo, const uint8_t u8FilterCount, const uint8_t u8OutputNo);
void VideoFilter_LoadFilter(h_VideoFilterMng hMng, const uint8_t u8ProfileNo, const uint8_t u8FilterNo, const uint32_t au32Param[2 + VF_PARAM_COUNT]);
void VideoFilterMng_SaveSetting(h_VideoFilterMng hMng, BOOL* pbEnable, uint8_t* pu8ProfileCount, uint8_t* pu8ProfileNo);
void VideoFilter_SaveProfile(h_VideoFilterMng hMng, uint8_t* pu8FilterCount, uint8_t* pu8OutputNo, const uint8_t u8ProfileNo);
void VideoFilter_SaveFilter(h_VideoFilterMng hMng, uint32_t au32Param[2 + VF_PARAM_COUNT], const uint8_t u8ProfileNo, const uint8_t u8FilterNo);

BOOL VideoFilter_GetEnable(h_VideoFilterMng hMng);
void VideoFilter_SetEnable(h_VideoFilterMng hMng, const BOOL bEnable);
void VideoFilter_SetSize(h_VideoFilterMng hMng, const uint16_t u16Width, const uint16_t u16Height);
BOOL VideoFilter_GetProfileNo(h_VideoFilterMng hMng);
void VideoFilter_SetProfileNo(h_VideoFilterMng hMng, const uint8_t u8ProfileNo);
void VideoFilter_SetSrcRGB_d(h_VideoFilterMng hMng, const uint16_t u16X, const uint16_t u16Y, const uint8_t u8R, const uint8_t u8G, const uint8_t u8B);
void VideoFilter_SetSrcRGB(h_VideoFilterMng hMng, const uint16_t u16X, const uint16_t u16Y, const uint32_t u32RGB);
void VideoFilter_SetSrcHSV_d(h_VideoFilterMng hMng, const uint16_t u16X, const uint16_t u16Y, const uint16_t u16H, const uint8_t u8S, const uint8_t u8V);
void VideoFilter_SetSrcHSV(h_VideoFilterMng hMng, const uint16_t u16X, const uint16_t u16Y, const uint32_t u32HSV);
void VideoFilter_Import98(h_VideoFilterMng hMng, uint8_t* pu8VRAM, uint8_t* pu8Dirty, BOOL bPalletEx);
void VideoFilter_Import(h_VideoFilterMng hMng, void* pInputBuf, const uint8_t u8InputBPP, const uint16_t u16YAlign);
void VideoFilter_Calc(h_VideoFilterMng hMng);
uint32_t* VideoFilter_GetDest(h_VideoFilterMng hMng);
void VideoFilter_PutSrc(h_VideoFilterMng hMng, void* pOutput, const uint16_t u16X, const uint16_t u16Y, const uint8_t u8OutputBPP);
void VideoFilter_PutDest(h_VideoFilterMng hMng, void* pOutput, const uint16_t u16X, const uint16_t u16Y, const uint8_t u8OutputBPP);
void VideoFilter_ExportSrc(h_VideoFilterMng hMng, void* pOutputBuf, const uint8_t u8OutputBPP, const uint16_t u16YAlign);
void VideoFilter_ExportDest(h_VideoFilterMng hMng, void* pOutputBuf, const uint8_t u8OutputBPP, const uint16_t u16YAlign);

#if defined(__cplusplus)
}
#endif

#endif  // _VIDEOFILTER_H_

