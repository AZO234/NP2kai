#ifndef _VIDEOFILTER_H_
#define _VIDEOFILTER_H_

#include <compiler.h>

#define VF_PROFILE_COUNT 3
#define VF_FILTER_COUNT  3
#define VF_PARAM_COUNT   6

#if defined(__cplusplus)
extern "C" {
#endif

typedef void* h_VideoFilterMng;

extern h_VideoFilterMng hVFMng1;

h_VideoFilterMng VideoFilter_Init(const uint16_t u16MaxWidth, const uint16_t u16MaxHeight, const uint8_t u8MaxRadius, const uint8_t u8MaxSample);
void VideoFilter_Deinit(h_VideoFilterMng hMng);

void VideoFilterMng_LoadSetting(
	h_VideoFilterMng hMng,
	const BOOL bEnable,
	const uint8_t u8ProfileCount,
	const uint8_t u8ProfileNo
);
void VideoFilter_LoadProfile(h_VideoFilterMng hMng, const uint8_t u8ProfileNo, const uint8_t u8FilterCount, const uint8_t u8OutputNo);
void VideoFilter_LoadFilter(h_VideoFilterMng hMng, const uint8_t u8ProfileNo, const uint8_t u8FilterNo, const uint32_t au32Param[2 + VF_PARAM_COUNT]);
void VideoFilterMng_SaveSetting(
	h_VideoFilterMng hMng,
	BOOL* pbEnable,
	uint8_t* pu8ProfileCount,
	uint8_t* pu8ProfileNo
);
void VideoFilter_SaveProfile(h_VideoFilterMng hMng, uint8_t* pu8FilterCount, uint8_t* pu8OutputNo, const uint8_t u8ProfileNo);
void VideoFilter_SaveFilter(h_VideoFilterMng hMng, uint32_t au32Param[2 + VF_PARAM_COUNT], const uint8_t u8ProfileNo, const uint8_t u8FilterNo);

void VideoFilter_Enable(h_VideoFilterMng hMng, const BOOL bEnable);
void VideoFilter_SetSize(h_VideoFilterMng hMng, const uint16_t u16Width, const uint16_t u16Height);
void VideoFilter_SetSrcPos(h_VideoFilterMng hMng, const uint16_t u16X, const uint16_t u16Y, const uint32_t u32RGB);
void VideoFilter_Import98(h_VideoFilterMng hMng, uint8_t* pu8VRAM);
void VideoFilter_Calc(h_VideoFilterMng hMng);
uint32_t* VideoFilter_GetDest(h_VideoFilterMng hMng);
void VideoFilter_GetDestPos(h_VideoFilterMng hMng, void* pOutput, const uint16_t u16X, const uint16_t u16Y, const uint8_t u8OutputBPP);
void VideoFilter_Export(h_VideoFilterMng hMng, void* pOutputBuf, const uint8_t u8OutputBPP, const uint16_t u16YAlign);

#if defined(__cplusplus)
}
#endif

#endif  // _VIDEOFILTER_H_

