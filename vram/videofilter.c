#include "videofilter.h"

#if defined(SUPPORT_VIDEOFILTER)

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <common.h>
#include <scrnmng.h>
#include <vram/palettes.h>
#include <vram/scrndraw.h>

h_VideoFilterMng hVFMng1;

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

static uint32_t RGBtoHSV(const uint32_t u32RGB) {
	uint8_t u8R = GETR(u32RGB);
	uint8_t u8G = GETG(u32RGB);
	uint8_t u8B = GETB(u32RGB);
	int16_t i16H;
	uint8_t u8S;
	uint8_t u8V;
	uint8_t u8D;

	if(u8R == u8G && u8R == u8B) {
		i16H = 0;
		u8S = 0;
		u8V = u8R;
	} else if(u8R >= u8G && u8R >= u8B) {
		if(u8B >= u8G) {
			u8D = u8R - u8G;
			i16H = 60 * ((int16_t)u8G - u8B) / u8D;
		} else {
			u8D = u8R - u8B;
			i16H = 60 * ((int16_t)u8G - u8B) / u8D;
		}
		u8S = (uint8_t)((uint16_t)u8D * 255 / u8R);
		u8V = u8R;
	} else if(u8G >= u8R && u8G >= u8B) {
		if(u8R >= u8B) {
			u8D = u8G - u8B;
			i16H = 60 * ((int16_t)u8B - u8R) / u8D + 120;
		} else {
			u8D = u8G - u8R;
			i16H = 60 * ((int16_t)u8B - u8R) / u8D + 120;
		}
		u8S = (uint8_t)((uint16_t)u8D * 255 / u8G);
		u8V = u8G;
	} else {
		if(u8G >= u8R) {
			u8D = u8B - u8R;
			i16H = 60 * ((int16_t)u8R - u8G) / u8D + 240;
		} else {
			u8D = u8B - u8G;
			i16H = 60 * ((int16_t)u8R - u8G) / u8D + 240;
		}
		u8S = (uint8_t)((uint16_t)u8D * 255 / u8B);
		u8V = u8B;
	}
	if(i16H < 0) {
		i16H += 360;
	}

	return SETHSV(i16H, u8S, u8V);
}

static uint32_t HSVtoRGB(const uint32_t u32HSV) {
	uint16_t u16H = GETH(u32HSV) % 360;
	uint8_t u8S = GETS(u32HSV);
	uint8_t u8V = GETV(u32HSV);
	uint8_t u8Min = u8V - (u8S * u8V) / 255;
	uint8_t u8R, u8G, u8B;

	if(u16H < 60) {
		u8R = u8V;
		u8G =        u16H  * (u8V - u8Min) / 60 + u8Min;
		u8B = u8Min;
	} else if(u16H >= 60 && u16H < 120) {
		u8R = (120 - u16H) * (u8V - u8Min) / 60 + u8Min;
		u8G = u8V;
		u8B = u8Min;
	} else if(u16H >= 120 && u16H < 180) {
		u8R = u8Min;
		u8G = u8V;
		u8B = (u16H - 120) * (u8V - u8Min) / 60 + u8Min;
	} else if(u16H >= 180 && u16H < 240) {
		u8R = u8Min;
		u8G = (240 - u16H) * (u8V - u8Min) / 60 + u8Min;
		u8B = u8V;
	} else if(u16H >= 240 && u16H < 300) {
		u8R = (u16H - 240) * (u8V - u8Min) / 60 + u8Min;
		u8G = u8Min;
		u8B = u8V;
	} else if(u16H >= 300 && u16H < 360) {
		u8R = u8V;
		u8G = u8Min;
		u8B = (360 - u16H) * (u8V - u8Min) / 60 + u8Min;
	}

	return SETRGB(u8R, u8G, u8B);
}

static uint32_t RGBtoXYZ(const uint32_t u32RGB) {
	uint8_t u8R = GETR(u32RGB);
	uint8_t u8G = GETG(u32RGB);
	uint8_t u8B = GETB(u32RGB);

	//  0.4887180  0.3106803  0.2006017
	//  0.1762044  0.8129847  0.0108109
	//  0.0000000  0.0102048  0.9897952
	return SETXYZ(
		(488718 * u8R + 310680 * u8G + 200601 * u8B) / 1000000,
		(176204 * u8R + 812984 * u8G +  10810 * u8B) / 1000000,
		(     0 * u8R +  10204 * u8G + 989795 * u8B) / 1000000
	);
}

static uint32_t XYZtoRGB(const uint32_t u32XYZ) {
	uint8_t u8X = GETX(u32XYZ);
	uint8_t u8Y = GETY(u32XYZ);
	uint8_t u8Z = GETZ(u32XYZ);

	//  2.3706743 -0.9000405 -0.4706338
	// -0.5138850  1.4253036  0.0885814
	//  0.0052982 -0.0146949  1.0093968
	return SETRGB(
		(2370674 * u8X -  900040 * u8Y -  470633 * u8Z) / 1000000,
		(1425303 * u8Y -  513885 * u8X +   88581 * u8Z) / 1000000,
		(   5298 * u8X -   14684 * u8Y + 1009396 * u8Z) / 1000000
	);
}

enum {
	VF_COLOR_TRANSPARENT = 1 << 25,
	VF_COLOR_OUTOFRANGE,
};

enum {
	VFE98_TYPE_THRU = 0,
	VFE98_TYPE_TOPALLET,  // pallet x to y
	VFE98_TYPE_SWAP,      // pallet swap x and y
	VFE98_TYPE_TORGB,     // pallet x to RGB
	VFE98_TYPE_TOHSV,     // pallet x to HSV
};

typedef struct VF_CalcSample_t_ {
	uint32_t u32X;
	uint32_t u32Y;
	uint8_t u8Weight;
} VF_CalcSample_t;

typedef enum {
	VFE_TYPE_THRU = 0,
	VFE_TYPE_NP,
	VFE_TYPE_DDOWN,
	VFE_TYPE_GREY,
	VFE_TYPE_GAMMA,
	VFE_TYPE_ROTATEH,
	VFE_TYPE_HSVSMOOTH,
	VFE_TYPE_RGBSMOOTH,

	VFE_TYPE_END
} VFE_Type_t;

typedef struct VFE_Base_t_ {
	BOOL       bEnable;
	VFE_Type_t tType;
} VFE_Base_t;

typedef struct VFE_NP_t_ {
	VFE_Base_t tBase;
} VFE_NP_t;

typedef struct VFE_DDown_t_ {
	VFE_Base_t tBase;
	uint32_t u32DDown;  // down depth Dbit (default 0) : 0 <= D <= 7
} VFE_DDown_t;

typedef struct VFE_Grey_t_ {
	VFE_Base_t tBase;
	uint32_t u32Bit;  // depth bit (default 8) : 0 <= D <= 8
	uint32_t u32H;    // H of white (default 360) : 0 <= H <= 360
	uint32_t u32S;    // S of white (default 100) : 0 <= S <= 255
	uint32_t u32V;    // V of white (default 100) : 0 <= V <= 255
} VFE_Grey_t;

typedef struct VFE_Gamma_t_ {
	VFE_Base_t tBase;
	uint32_t u32Gamma;  // Gamma G/10 (default 10) : 1 <= G <= 255
} VFE_Gamma_t;

typedef struct VFE_RotateH_t_ {
	VFE_Base_t tBase;
	uint32_t u32RotateH;  // 0 <= H <= 360 (default 0)
} VFE_RotateH_t;

typedef struct VFE_HSVSmooth_t_ {
	VFE_Base_t tBase;
	uint32_t u32Radius;  // radius of range R/10dot units (default 5) : 5 <= R <= 25
	uint32_t u32Sample;  // samples in range N*N (default 1) : 1 <= N <= 5
	uint32_t u32HDiff;   // 0 <= dH (default 0) <= 180
	uint32_t u32SDiff;   // 0 <= dS (default 0) <= 128
	uint32_t u32VDiff;   // 0 <= dV (default 0) <= 128
	uint32_t u32WType;   // weight type (default 0) 0:none 1:linear 2:sign
} VFE_HSVSmooth_t;

typedef struct VFE_RGBSmooth_t_ {
	VFE_Base_t tBase;
	uint32_t u32Radius;  // radius of range R/10dot units (default 5) : 5 <= R <= 25
	uint32_t u32Sample;  // samples in range N*N (default 1) : 1 <= N <= 5
	uint32_t u32RDiff;   // 0 <= dR (default 0) <= 128
	uint32_t u32GDiff;   // 0 <= dG (default 0) <= 128
	uint32_t u32BDiff;   // 0 <= dB (default 0) <= 128
	uint32_t u32WType;   // weight type (default 0) 0:none 1:linear 2:sign
} VFE_RGBSmooth_t;

typedef struct VFE_MaxParam_t_ {
	VFE_Base_t tBase;
	uint32_t au32Param[VF_PARAM_COUNT];
} VFE_MaxParam_t;

typedef union VFE_t_ {
	VFE_Base_t      tBase;
	VFE_NP_t        tNP;
	VFE_DDown_t     tDDown;
	VFE_Grey_t      tGrey;
	VFE_Gamma_t     tGamma;
	VFE_RotateH_t   tRotateH;
	VFE_HSVSmooth_t tHSVSmooth;
	VFE_RGBSmooth_t tRGBSmooth;

	VFE_MaxParam_t  tMaxParam;
} VFE_t;

typedef struct {
	uint8_t u8FilterCount;
	VFE_t   atFilters[VF_FILTER_COUNT];
	uint8_t u8OutputNo;
} VF_Profile_t;

typedef struct VF_Mng_t_ {
	BOOL             bEnable;
	uint8_t          u8ProfileCount;
	VF_Profile_t     atProfile[VF_PROFILE_COUNT];
	uint8_t          u8ProfileNo;

	uint16_t         u16MaxWidth;
	uint16_t         u16MaxHeight;
	uint16_t         u16Width;
	uint16_t         u16Height;
	uint8_t          u8MaxRadius;
	BOOL             bBufferMain;
	uint32_t*        pu32Buffer;
	uint8_t          u8WorkSize;
	uint32_t*        pu32Work;
	uint32_t         au32WorkFF[256];
	uint16_t         u16WorkX;
	uint16_t         u16WorkY;
	uint16_t         u16WorkSrcX;
	uint16_t         u16WorkSrcY;
	BOOL             bWorkHSV;
	uint8_t          u8MaxSample;
	VF_CalcSample_t* ptCalcSample;
} VF_Mng_t;

h_VideoFilterMng VideoFilter_Init(const uint16_t u16MaxWidth, const uint16_t u16MaxHeight, const uint8_t u8MaxRadius, const uint8_t u8MaxSample) {
	VF_Mng_t* ptMng = NULL;

	if(!u16MaxWidth || u16MaxWidth < 640 || !u16MaxHeight || u16MaxWidth < 480 || !u8MaxRadius || u8MaxRadius < 5 || !u8MaxSample) {
		return NULL;
	}

	ptMng = (VF_Mng_t*)malloc(sizeof(VF_Mng_t));
	if(!ptMng) {
		return NULL;
	}

	memset(ptMng, 0, sizeof(VF_Mng_t));

	ptMng->u16MaxWidth  = u16MaxWidth;
	ptMng->u16MaxHeight = u16MaxHeight;
	ptMng->u16Width     = 640;
	ptMng->u16Height    = 480;
	ptMng->u8MaxRadius = u8MaxRadius;
	ptMng->u8MaxSample = u8MaxSample;

	ptMng->pu32Buffer = (uint32_t*)malloc(ptMng->u16Width * ptMng->u16Height * 2 * sizeof(uint32_t));
	if(!ptMng->pu32Buffer) {
		VideoFilter_Deinit(ptMng);
		return NULL;
	}
	ptMng->pu32Work = (uint32_t*)malloc(ptMng->u8WorkSize * ptMng->u8WorkSize * sizeof(uint32_t));
	if(!ptMng->pu32Work) {
		VideoFilter_Deinit(ptMng);
		return NULL;
	}
	ptMng->ptCalcSample = (VF_CalcSample_t*)malloc(u8MaxSample * u8MaxSample * sizeof(VF_CalcSample_t));
	if(!ptMng->ptCalcSample) {
		VideoFilter_Deinit(ptMng);
		return NULL;
	}

	return ptMng;
}

void VideoFilter_Deinit(h_VideoFilterMng hMng) {
	VF_Mng_t* ptMng = (VF_Mng_t*)hMng;

	if(!hMng) {
		return;
	}

	if(ptMng->pu32Buffer) {
		free(ptMng->pu32Buffer);
	}
	if(ptMng->pu32Work) {
		free(ptMng->pu32Work);
	}
	if(ptMng->ptCalcSample) {
		free(ptMng->ptCalcSample);
	}
	free(ptMng);
}

void VideoFilterMng_LoadSetting(
	h_VideoFilterMng hMng,
	const BOOL bEnable,
	const uint8_t u8ProfileCount,
	const uint8_t u8ProfileNo
) {
	VF_Mng_t* ptMng = (VF_Mng_t*)hMng;
	uint8_t u8UseProfileNo = u8ProfileNo;

	if(!hMng || !u8ProfileCount) {
		return;
	}
	if(u8ProfileNo >= u8ProfileCount) {
		u8UseProfileNo = 0;
	}

	ptMng->bEnable        = bEnable;
	ptMng->u8ProfileCount = u8ProfileCount;
	ptMng->u8ProfileNo    = u8UseProfileNo;
	ptMng->u16Height      = 640;
	ptMng->u16Width       = 480;
	ptMng->u8WorkSize     = 0;
	ptMng->bWorkHSV       = FALSE;
}

void VideoFilter_LoadProfile(h_VideoFilterMng hMng, const uint8_t u8ProfileNo, const uint8_t u8FilterCount, const uint8_t u8OutputNo) {
	VF_Mng_t* ptMng = (VF_Mng_t*)hMng;

	if(!hMng) {
		return;
	}
	if(u8ProfileNo >= ptMng->u8ProfileCount) {
		return;
	}

	ptMng->atProfile[u8ProfileNo].u8FilterCount = u8FilterCount;
	if(u8OutputNo < ptMng->atProfile[u8ProfileNo].u8FilterCount) {
		ptMng->atProfile[u8ProfileNo].u8OutputNo = u8OutputNo;
	} else {
		ptMng->atProfile[u8ProfileNo].u8OutputNo = 0;
	}
}

void VideoFilter_LoadFilter(h_VideoFilterMng hMng, const uint8_t u8ProfileNo, const uint8_t u8FilterNo, const uint32_t au32Param[2 + VF_PARAM_COUNT]) {
	VF_Mng_t* ptMng = (VF_Mng_t*)hMng;

	if(!hMng) {
		return;
	}
	if(u8ProfileNo >= ptMng->u8ProfileCount) {
		return;
	}

	if(u8FilterNo < ptMng->atProfile[u8ProfileNo].u8FilterCount) {
		ptMng->atProfile[u8ProfileNo].atFilters[u8FilterNo].tBase.bEnable = au32Param[0];
		ptMng->atProfile[u8ProfileNo].atFilters[u8FilterNo].tBase.tType = au32Param[1];
		memcpy(ptMng->atProfile[u8ProfileNo].atFilters[u8FilterNo].tMaxParam.au32Param, &au32Param[2], VF_PARAM_COUNT * sizeof(uint32_t));
	}
}

void VideoFilterMng_SaveSetting(
	h_VideoFilterMng hMng,
	BOOL* pbEnable,
	uint8_t* pu8ProfileCount,
	uint8_t* pu8ProfileNo
) {
	VF_Mng_t* ptMng = (VF_Mng_t*)hMng;

	if(!hMng || !pbEnable || !pu8ProfileCount || !pu8ProfileNo) {
		return;
	}

	*pbEnable = ptMng->bEnable;
	*pu8ProfileCount = ptMng->u8ProfileCount;
	*pu8ProfileNo = ptMng->u8ProfileNo;
}

void VideoFilter_SaveProfile(h_VideoFilterMng hMng, uint8_t* pu8FilterCount, uint8_t* pu8OutputNo, const uint8_t u8ProfileNo) {
	VF_Mng_t* ptMng = (VF_Mng_t*)hMng;

	if(!hMng || !pu8FilterCount || !pu8OutputNo) {
		return;
	}
	if(u8ProfileNo >= ptMng->u8ProfileCount) {
		return;
	}

	*pu8FilterCount = ptMng->atProfile[u8ProfileNo].u8FilterCount;
	*pu8OutputNo = ptMng->atProfile[u8ProfileNo].u8OutputNo;
}

void VideoFilter_SaveFilter(h_VideoFilterMng hMng, uint32_t au32Param[2 + VF_PARAM_COUNT], const uint8_t u8ProfileNo, const uint8_t u8FilterNo) {
	VF_Mng_t* ptMng = (VF_Mng_t*)hMng;

	if(!hMng || au32Param) {
		return;
	}
	if(u8ProfileNo >= ptMng->u8ProfileCount) {
		return;
	}
	if(ptMng->atProfile[u8ProfileNo].u8FilterCount) {
		return;
	}

	au32Param[0] = ptMng->atProfile[u8ProfileNo].atFilters[u8FilterNo].tBase.bEnable;
	au32Param[1] = ptMng->atProfile[u8ProfileNo].atFilters[u8FilterNo].tBase.tType;
	memcpy(&au32Param[2], ptMng->atProfile[u8ProfileNo].atFilters[u8FilterNo].tMaxParam.au32Param, VF_PARAM_COUNT * sizeof(uint32_t));
}

void VideoFilter_Enable(h_VideoFilterMng hMng, const BOOL bEnable) {
	VF_Mng_t* ptMng = (VF_Mng_t*)hMng;

	if(!hMng) {
		return;
	}

	ptMng->bEnable = bEnable;
}

void VideoFilter_SetSize(h_VideoFilterMng hMng, const uint16_t u16Width, const uint16_t u16Height) {
	VF_Mng_t* ptMng = (VF_Mng_t*)hMng;

	if(!hMng) {
		return;
	}

	ptMng->u16Width  = u16Width;
	ptMng->u16Height = u16Height;
}

void VideoFilter_SetSrcPos(h_VideoFilterMng hMng, const uint16_t u16X, const uint16_t u16Y, const uint32_t u32RGB) {
	VF_Mng_t* ptMng = (VF_Mng_t*)hMng;
	uint32_t* pu32Src;

	if(!hMng) {
		return;
	}

	pu32Src  = &ptMng->pu32Buffer[ ptMng->bBufferMain * ptMng->u16Width * ptMng->u16Height];

	if(u16X < ptMng->u16Width && u16Y < ptMng->u16Height) {
		pu32Src[u16Y * ptMng->u16Width + u16X] = u32RGB;
	}
}

void VideoFilter_Import98(h_VideoFilterMng hMng, uint8_t* pu8VRAM) {
	VF_Mng_t* ptMng = (VF_Mng_t*)hMng;
	uint16_t u16X, u16Y;
	uint32_t* pu32Src;
	uint32_t u32Loc;

	if(!hMng || !pu8VRAM) {
		return;
	}

	VideoFilter_SetSize(hMng, SURFACE_WIDTH, SURFACE_HEIGHT);
	pu32Src  = &ptMng->pu32Buffer[ ptMng->bBufferMain * ptMng->u16Width * ptMng->u16Height];

	for(u16Y = 0; u16Y < SURFACE_HEIGHT; u16Y++) {
		for(u16X = 0; u16X < SURFACE_WIDTH; u16X++) {
		u32Loc = u16Y * SURFACE_WIDTH + u16X;
			pu32Src[u32Loc] = SETRGB(
				np2_pal32[pu8VRAM[u32Loc] + NP2PAL_GRPH].p.r,
				np2_pal32[pu8VRAM[u32Loc] + NP2PAL_GRPH].p.g,
				np2_pal32[pu8VRAM[u32Loc] + NP2PAL_GRPH].p.b
			);
		}
	}
}

uint32_t* VideoFilter_GetDest(h_VideoFilterMng hMng) {
	VF_Mng_t *ptMng = (VF_Mng_t *) hMng;

	if (hMng) {
		return &ptMng->pu32Buffer[!ptMng->bBufferMain * ptMng->u16Width * ptMng->u16Height];
	} else {
		return NULL;
	}
}

void VideoFilter_NP(h_VideoFilterMng hMng) {
	VF_Mng_t* ptMng = (VF_Mng_t*)hMng;
	uint16_t u16X, u16Y;
	uint32_t* pu32Src;
	uint32_t* pu32Dest;
	uint32_t* pu32YSrc;
	uint32_t* pu32YDest;

	if(!hMng) {
		return;
	}

	pu32Src  = &ptMng->pu32Buffer[ ptMng->bBufferMain * ptMng->u16Width * ptMng->u16Height];
	pu32Dest = &ptMng->pu32Buffer[!ptMng->bBufferMain * ptMng->u16Width * ptMng->u16Height];

	for(u16Y = 0; u16Y < ptMng->u16Height; u16Y++) {
		pu32YSrc  = &pu32Src[ptMng->u16Width * u16Y];
		pu32YDest = &pu32Dest[ptMng->u16Width * u16Y];
		for(u16X = 0; u16X < ptMng->u16Width; u16X++) {
			*pu32YDest = ~(*pu32YSrc) & 0x00FFFFFF;
			pu32YSrc++;
			pu32YDest++;
		}
	}
}

void VideoFilter_DDown(h_VideoFilterMng hMng, const uint8_t u8DDown) {
	VF_Mng_t* ptMng = (VF_Mng_t*)hMng;
	uint16_t u16X, u16Y;
	uint32_t* pu32Src;
	uint32_t* pu32Dest;
	uint32_t* pu32YSrc;
	uint32_t* pu32YDest;
	uint32_t u32RGB;
	uint8_t u8UseDDown = u8DDown;
	uint8_t u8Div;

	if(!hMng) {
		return;
	}

	if(u8DDown > 8) {
		u8UseDDown = 7;
	}

	u8Div = (1 << (8 - u8DDown)) - 1;

	pu32Src  = &ptMng->pu32Buffer[ ptMng->bBufferMain * ptMng->u16Width * ptMng->u16Height];
	pu32Dest = &ptMng->pu32Buffer[!ptMng->bBufferMain * ptMng->u16Width * ptMng->u16Height];

	for(u16Y = 0; u16Y < ptMng->u16Height; u16Y++) {
		pu32YSrc  = &pu32Src[ptMng->u16Width * u16Y];
		pu32YDest = &pu32Dest[ptMng->u16Width * u16Y];
		for(u16X = 0; u16X < ptMng->u16Width; u16X++) {
			u32RGB = *pu32YSrc;
			*pu32YDest = SETRGB(
				(GETR(u32RGB) >> u8DDown) * 255 / u8Div,
				(GETG(u32RGB) >> u8DDown) * 255 / u8Div,
				(GETB(u32RGB) >> u8DDown) * 255 / u8Div
			);
			pu32YSrc++;
			pu32YDest++;
		}
	}
}

void VideoFilter_Grey(h_VideoFilterMng hMng, const uint8_t u8Bit, const uint16_t u16H, const uint8_t u8S, const uint8_t u8V) {
	VF_Mng_t* ptMng = (VF_Mng_t*)hMng;
	uint16_t u16X, u16Y;
	uint32_t* pu32Src;
	uint32_t* pu32Dest;
	uint32_t* pu32YSrc;
	uint32_t* pu32YDest;
	uint32_t u32RGB;
	uint8_t u8R;
	uint8_t u8G;
	uint8_t u8B;
	uint16_t u16UseH = u16H % 360;
	uint8_t u8UseBit = u8Bit;

	if(!hMng) {
		return;
	}

	if(u8Bit > 8) {
		u8UseBit = 8;
	}

	pu32Src  = &ptMng->pu32Buffer[ ptMng->bBufferMain * ptMng->u16Width * ptMng->u16Height];
	pu32Dest = &ptMng->pu32Buffer[!ptMng->bBufferMain * ptMng->u16Width * ptMng->u16Height];

	for(u16Y = 0; u16Y < ptMng->u16Height; u16Y++) {
		pu32YSrc  = &pu32Src[ptMng->u16Width * u16Y];
		pu32YDest = &pu32Dest[ptMng->u16Width * u16Y];
		for(u16X = 0; u16X < ptMng->u16Width; u16X++) {
			u32RGB = *pu32YSrc;
			*pu32YDest = ((uint16_t)GETR(u32RGB) + GETG(u32RGB) + GETB(u32RGB)) / 3;
			pu32YSrc++;
			pu32YDest++;
		}
	}

	ptMng->bBufferMain ^= 1;
	pu32Src  = &ptMng->pu32Buffer[ ptMng->bBufferMain * ptMng->u16Width * ptMng->u16Height];
	pu32Dest = &ptMng->pu32Buffer[!ptMng->bBufferMain * ptMng->u16Width * ptMng->u16Height];

	u32RGB = HSVtoRGB(SETHSV(u16UseH, u8S, u8V));
	u8R = GETR(u32RGB);
	u8G = GETG(u32RGB);
	u8B = GETB(u32RGB);
	for(u16Y = 0; u16Y < ptMng->u16Height; u16Y++) {
		pu32YSrc  = &pu32Src[ptMng->u16Width * u16Y];
		pu32YDest = &pu32Dest[ptMng->u16Width * u16Y];
		for(u16X = 0; u16X < ptMng->u16Width; u16X++) {
			*pu32YDest = SETRGB(
				(*pu32YSrc * u8R) / 255,
				(*pu32YSrc * u8G) / 255,
				(*pu32YSrc * u8B) / 255
			);
			pu32YSrc++;
			pu32YDest++;
		}
	}
}

void VideoFilter_Gamma(h_VideoFilterMng hMng, const uint8_t u8Gamma) {
	VF_Mng_t* ptMng = (VF_Mng_t*)hMng;
	uint16_t u16X, u16Y;
	uint32_t* pu32Src;
	uint32_t* pu32Dest;
	uint32_t* pu32YSrc;
	uint32_t* pu32YDest;
	uint8_t u8UseGamma = u8Gamma;
	uint32_t u32HSV;

	if(!hMng) {
		return;
	}

	if(u8Gamma == 0) {
		u8UseGamma = 1;
	}

	for(u16X = 0; u16X < 256; u16X++) {
		ptMng->au32WorkFF[u16X] = (uint8_t)(255 * pow(1.0 * u16X / 255, 1.0 / (u8UseGamma / 10.0)));
	}

	pu32Src  = &ptMng->pu32Buffer[ ptMng->bBufferMain * ptMng->u16Width * ptMng->u16Height];
	pu32Dest = &ptMng->pu32Buffer[!ptMng->bBufferMain * ptMng->u16Width * ptMng->u16Height];

	for(u16Y = 0; u16Y < ptMng->u16Height; u16Y++) {
		pu32YSrc  = &pu32Src[ptMng->u16Width * u16Y];
		pu32YDest = &pu32Dest[ptMng->u16Width * u16Y];
		for(u16X = 0; u16X < ptMng->u16Width; u16X++) {
			u32HSV = RGBtoHSV(*pu32YSrc);
			*pu32YDest = HSVtoRGB(SETHSV(GETH(u32HSV), GETS(u32HSV), ptMng->au32WorkFF[GETV(u32HSV)]));
			pu32YSrc++;
			pu32YDest++;
		}
	}
}

void VideoFilter_RotateH(h_VideoFilterMng hMng, const uint16_t u16RotateH) {
	VF_Mng_t* ptMng = (VF_Mng_t*)hMng;
	uint16_t u16X, u16Y;
	uint32_t* pu32Src;
	uint32_t* pu32Dest;
	uint32_t* pu32YSrc;
	uint32_t* pu32YDest;
	uint16_t u16UseRotateH = u16RotateH % 360;
	uint32_t u32HSV;

	if(!hMng) {
		return;
	}

	pu32Src  = &ptMng->pu32Buffer[ ptMng->bBufferMain * ptMng->u16Width * ptMng->u16Height];
	pu32Dest = &ptMng->pu32Buffer[!ptMng->bBufferMain * ptMng->u16Width * ptMng->u16Height];

	for(u16Y = 0; u16Y < ptMng->u16Height; u16Y++) {
		pu32YSrc  = &pu32Src[ptMng->u16Width * u16Y];
		pu32YDest = &pu32Dest[ptMng->u16Width * u16Y];
		for(u16X = 0; u16X < ptMng->u16Width; u16X++) {
			u32HSV = RGBtoHSV(*pu32YSrc);
			*pu32YDest = HSVtoRGB(SETHSV((GETH(u32HSV) + u16UseRotateH) % 360, GETS(u32HSV), GETV(u32HSV)));
			pu32YSrc++;
			pu32YDest++;
		}
	}
}

static void FetchWork(h_VideoFilterMng hMng) {
	VF_Mng_t* ptMng = (VF_Mng_t*)hMng;
	int16_t i16SelectX, i16SelectY;
	uint32_t *pu32Src;
	uint32_t *pu32SrcPos;
	uint32_t *pu32WorkPos;

	if(!hMng) {
		return;
	}

	pu32Src = &(ptMng->pu32Buffer[ptMng->bBufferMain * ptMng->u16Width * ptMng->u16Height]);

	i16SelectY = (int16_t)ptMng->u16WorkSrcY - ptMng->u8WorkSize / 2;
	for(ptMng->u16WorkY = 0; ptMng->u16WorkY < ptMng->u8WorkSize; ptMng->u16WorkY++) {
		pu32WorkPos = &ptMng->pu32Work[ptMng->u16WorkY * ptMng->u8WorkSize];
		if(i16SelectY >= 0 && i16SelectY < ptMng->u16Height) {
			i16SelectX = (int16_t)ptMng->u16WorkSrcX - ptMng->u8WorkSize / 2;
			for(ptMng->u16WorkX = 0; ptMng->u16WorkX < ptMng->u8WorkSize; ptMng->u16WorkX++) {
				if(i16SelectX >= 0 && i16SelectX < ptMng->u16Width) {
					pu32SrcPos = &pu32Src[i16SelectY * ptMng->u16Width + i16SelectX];
					if(ptMng->bWorkHSV) {
						*pu32WorkPos = RGBtoHSV(*pu32SrcPos);
					} else {
						*pu32WorkPos = *pu32SrcPos;
					}
				} else {
					*pu32WorkPos = VF_COLOR_OUTOFRANGE;
				}
				pu32WorkPos++;
				i16SelectX++;
			}
		} else {
			for(ptMng->u16WorkX = 0; ptMng->u16WorkX < ptMng->u8WorkSize; ptMng->u16WorkX++) {
				*pu32WorkPos = VF_COLOR_OUTOFRANGE;
				pu32WorkPos++;
			}
		}
		i16SelectY++;
	}
}

static void WorkRight(h_VideoFilterMng hMng) {
	VF_Mng_t* ptMng = (VF_Mng_t*)hMng;
	int16_t i16SelectX, i16SelectY;
	uint32_t *pu32Src;
	uint32_t *pu32SrcPos;
	uint32_t *pu32WorkPos;

	if(!hMng) {
		return;
	}

	pu32Src  = &ptMng->pu32Buffer[ ptMng->bBufferMain * ptMng->u16Width * ptMng->u16Height];

	for(ptMng->u16WorkY = 0; ptMng->u16WorkY < ptMng->u8WorkSize; ptMng->u16WorkY++) {
		pu32WorkPos = &ptMng->pu32Work[ptMng->u16WorkY * ptMng->u8WorkSize];
		for(ptMng->u16WorkX = 0; ptMng->u16WorkX < ptMng->u8WorkSize - 1; ptMng->u16WorkX++) {
			*pu32WorkPos = *(pu32WorkPos + 1);
			pu32WorkPos++;
		}
	}

	i16SelectX = (int16_t)ptMng->u16WorkSrcX + ptMng->u8WorkSize / 2 + 1;
	i16SelectY = (int16_t)ptMng->u16WorkSrcY - ptMng->u8WorkSize / 2;
	for(ptMng->u16WorkY = 0; ptMng->u16WorkY < ptMng->u8WorkSize; ptMng->u16WorkY++) {
        pu32WorkPos = &ptMng->pu32Work[ptMng->u16WorkY * ptMng->u8WorkSize + ptMng->u8WorkSize - 1];
		if(i16SelectX < ptMng->u16Width) {
			if(i16SelectY >= 0 && i16SelectY < ptMng->u16Height) {
				pu32SrcPos = &pu32Src[i16SelectY * ptMng->u16Width + i16SelectX];
				if(ptMng->bWorkHSV) {
					*pu32WorkPos = RGBtoHSV(*pu32SrcPos);
				} else {
					*pu32WorkPos = *pu32SrcPos;
				}
			} else {
				*pu32WorkPos = VF_COLOR_OUTOFRANGE;
			}
		} else {
			*pu32WorkPos = VF_COLOR_OUTOFRANGE;
		}
		i16SelectY++;
	}
}

void VideoFilter_HSVSmooth(h_VideoFilterMng hMng, const uint8_t u8Radius, const uint8_t u8Sample, const uint8_t u8HDiff, const uint8_t u8SDiff, const uint8_t u8VDiff, const uint8_t u8WType) {
	VF_Mng_t* ptMng = (VF_Mng_t*)hMng;
	uint16_t u16X, u16Y;
	uint32_t* pu32Src;
	uint32_t* pu32Dest;
	uint32_t* pu32YSrc;
	uint32_t* pu32YDest;
	uint8_t u8UseRadius = u8Radius;
	uint8_t u8UseSample = u8Sample;
	uint8_t u8UseHDiff = u8HDiff;
	uint8_t u8UseSDiff = u8SDiff;
	uint8_t u8UseVDiff = u8VDiff;
	uint32_t u32HSV, u32SrcHSV;
	int16_t i16H;
	uint16_t u16CH;
	uint8_t u8CS, u8CV;
	int16_t i16DH, i16DS, i16DV;
	uint32_t u32AllH, u32AllS, u32AllV;
	BOOL bCalc;
	VF_CalcSample_t* ptCalcSample;
	uint32_t u32Half;
	uint32_t u32SampleCount;
	uint8_t u8SampleX, u8SampleY;

	if(!hMng) {
		return;
	}

	if(u8Radius < 5) {
		u8UseRadius = 5;
	} else if(u8Radius > ptMng->u8MaxRadius) {
		u8UseRadius = ptMng->u8MaxRadius;
	}
	ptMng->u8WorkSize = u8UseRadius * 2 / 10;
	if(u8Sample == 0) {
		u8UseSample = 1;
	} else if(u8Sample > ptMng->u8MaxSample) {
		u8UseSample = ptMng->u8MaxSample;
	}
	if(u8HDiff > 180) {
		u8UseHDiff = 180;
	}
	if(u8SDiff > 128) {
		u8UseSDiff = 128;
	}
	if(u8VDiff > 128) {
		u8UseVDiff = 128;
	}

	switch(u8WType) {
	case 1:
		for(u16X = 0; u16X < 90; u16X++) {
			ptMng->au32WorkFF[u16X] = (uint8_t)(((90 - u16X) * 255) / 90);
		}
		break;
	case 2:
		for(u16X = 0; u16X < 90; u16X++) {
			ptMng->au32WorkFF[u16X] = (uint8_t)(cos(u16X * 3.141592 / 180.0) * 255);
		}
		break;
	default:
		for(u16X = 0; u16X < 90; u16X++) {
			ptMng->au32WorkFF[u16X] = 255;
		}
		break;
	}

	u32Half = (uint32_t)((u8Radius * 10) / 1.414);  // x100
	for(u8SampleY = 0; u8SampleY < u8UseSample; u8SampleY++) {
		ptCalcSample = &ptMng->ptCalcSample[u8SampleY * u8UseSample];
		for(u8SampleX = 0; u8SampleX < u8UseSample; u8SampleX++) {
			ptCalcSample->u32X = (((u32Half * 2) * u8SampleX / (u8UseSample - 1)) + u8UseRadius * 10 - u32Half) / 100;
			ptCalcSample->u32Y = (((u32Half * 2) * u8SampleY / (u8UseSample - 1)) + u8UseRadius * 10 - u32Half) / 100;
			ptCalcSample->u8Weight = ptMng->au32WorkFF[(uint32_t)sqrt(((int32_t)((u32Half * 2) * u8SampleX / (u8UseSample - 1)) - u32Half) * ((int32_t)((u32Half * 2) * u8SampleX / (u8UseSample - 1)) - u32Half) + ((int32_t)((u32Half * 2) * u8SampleY / (u8UseSample - 1)) - u32Half) * ((int32_t)((u32Half * 2) * u8SampleY / (u8UseSample - 1)) - u32Half)) * 90 / (u8Radius * 10)];
			ptCalcSample++;
		}
	}

	pu32Src  = &ptMng->pu32Buffer[ ptMng->bBufferMain * ptMng->u16Width * ptMng->u16Height];
	pu32Dest = &ptMng->pu32Buffer[!ptMng->bBufferMain * ptMng->u16Width * ptMng->u16Height];

	ptMng->bWorkHSV = TRUE;
	for(ptMng->u16WorkSrcY = 0; ptMng->u16WorkSrcY < ptMng->u16Height; ptMng->u16WorkSrcY++) {
		pu32YDest = &pu32Dest[ptMng->u16WorkSrcY * ptMng->u16Width];
		ptMng->u16WorkSrcX = 0;
		FetchWork(hMng);
		for(; ptMng->u16WorkSrcX < ptMng->u16Width; ptMng->u16WorkSrcX++) {
			u32SrcHSV = RGBtoHSV(pu32Src[ptMng->u16WorkSrcY * ptMng->u16Width + ptMng->u16WorkSrcX]);
			u16CH = GETH(u32SrcHSV);
			u8CS  = GETS(u32SrcHSV);
			u8CV  = GETV(u32SrcHSV);
			u32AllH = u32AllS = u32AllV = 0;
			u32SampleCount = 0;
			for(u8SampleY = 0; u8SampleY < u8UseSample; u8SampleY++) {
				ptCalcSample = &ptMng->ptCalcSample[u8SampleY * u8UseSample];
				for(u8SampleX = 0; u8SampleX < u8UseSample; u8SampleX++) {
					u32HSV = ptMng->pu32Work[ptCalcSample->u32Y * ptMng->u8WorkSize + ptCalcSample->u32X];
					if(u32SrcHSV == u32HSV) {
						u32AllH += u16CH;
						u32AllS += u8CS;
						u32AllV += u8CV;
						u32SampleCount++;
					} else if(u32HSV != VF_COLOR_OUTOFRANGE) {
						i16H = (int16_t)GETH(u32HSV) - u16CH;
						i16DH = i16H - u16CH;
						i16DS = (int16_t)GETS(u32HSV) -  u8CS;
						i16DV = (int16_t)GETV(u32HSV) -  u8CV;
						bCalc = TRUE;
						if(bCalc) {
							if(i16DS < -1 * u8UseSDiff || u8UseSDiff < i16DS) {
								bCalc = FALSE;
							}
						}
						if(bCalc) {
							if(u8CS && GETS(u32HSV)) {
								if((int16_t)u16CH - u8UseHDiff < 0) {
									if(u16CH + u8UseHDiff < i16H && i16H < u16CH + 360 - u8UseHDiff) {
										bCalc = FALSE;
									}
								} else if(u16CH + u8UseHDiff > 360) {
									if(u16CH + u8UseHDiff - 360 < i16H && i16H < u16CH - u8UseHDiff) {
										bCalc = FALSE;
									}
								} else {
									if(i16H < u16CH - u8UseHDiff || u16CH + u8UseHDiff < i16H) {
										bCalc = FALSE;
									}
								}
							}
						}
						if(bCalc) {
							if(i16DV < -1 * u8UseVDiff || u8UseVDiff < i16DV) {
								bCalc = FALSE;
							}
						}
						if(bCalc) {
							u32AllH += u16CH + (i16DH * ptCalcSample->u8Weight) / 255;
							u32AllS += u8CS  + (i16DS * ptCalcSample->u8Weight) / 255;
							u32AllV += u8CV  + (i16DV * ptCalcSample->u8Weight) / 255;
							u32SampleCount++;
						}
					}
					ptCalcSample++;
				}
			}
			*pu32YDest = HSVtoRGB(SETHSV(u32AllH / u32SampleCount, u32AllS / u32SampleCount, u32AllV / u32SampleCount));
			pu32YDest++;
			WorkRight(hMng);
		}
	}
}

void VideoFilter_RGBSmooth(h_VideoFilterMng hMng, const uint8_t u8Radius, const uint8_t u8Sample, const uint8_t u8RDiff, const uint8_t u8GDiff, const uint8_t u8BDiff, const uint8_t u8WType) {
	VF_Mng_t* ptMng = (VF_Mng_t*)hMng;
	uint16_t u16X, u16Y;
	uint32_t* pu32Src;
	uint32_t* pu32Dest;
	uint32_t* pu32YSrc;
	uint32_t* pu32YDest;
	uint8_t u8UseRadius = u8Radius;
	uint8_t u8UseSample = u8Sample;
	uint8_t u8UseRDiff = u8RDiff;
	uint8_t u8UseGDiff = u8GDiff;
	uint8_t u8UseBDiff = u8BDiff;
	uint32_t u32RGB, u32SrcRGB;
	uint8_t u8CR, u8CG, u8CB;
	int16_t i16DR, i16DG, i16DB;
	uint32_t u32AllR, u32AllG, u32AllB;
	BOOL bCalc;
	VF_CalcSample_t* ptCalcSample;
	uint32_t u32Half;
	uint32_t u32SampleCount;
	uint8_t u8SampleX, u8SampleY;

	if(!hMng) {
		return;
	}

	if(u8Radius < 5) {
		u8UseRadius = 5;
	} else if(u8Radius > ptMng->u8MaxRadius) {
		u8UseRadius = ptMng->u8MaxRadius;
	}
	ptMng->u8WorkSize = u8UseRadius * 2 / 10;
	if(u8Sample == 0) {
		u8UseSample = 1;
	} else if(u8Sample > ptMng->u8MaxSample) {
		u8UseSample = ptMng->u8MaxSample;
	}
	if(u8RDiff > 128) {
		u8UseRDiff = 128;
	}
	if(u8GDiff > 128) {
		u8UseGDiff = 128;
	}
	if(u8BDiff > 128) {
		u8UseBDiff = 128;
	}

	switch(u8WType) {
	case 1:
		for(u16X = 0; u16X < 90; u16X++) {
			ptMng->au32WorkFF[u16X] = (uint8_t)(((90 - u16X) * 255) / 90);
		}
		break;
	case 2:
		for(u16X = 0; u16X < 90; u16X++) {
			ptMng->au32WorkFF[u16X] = (uint8_t)(cos(u16X * 3.141592 / 180.0) * 255);
		}
		break;
	default:
		for(u16X = 0; u16X < 90; u16X++) {
			ptMng->au32WorkFF[u16X] = 255;
		}
		break;
	}

	u32Half = (uint32_t)((u8Radius * 10) / 1.414);  // x100
	for(u8SampleY = 0; u8SampleY < u8UseSample; u8SampleY++) {
		ptCalcSample = &ptMng->ptCalcSample[u8SampleY * u8UseSample];
		for(u8SampleX = 0; u8SampleX < u8UseSample; u8SampleX++) {
			ptCalcSample->u32X = (((u32Half * 2) * u8SampleX / (u8UseSample - 1)) + u8UseRadius * 10 - u32Half) / 100;
			ptCalcSample->u32Y = (((u32Half * 2) * u8SampleY / (u8UseSample - 1)) + u8UseRadius * 10 - u32Half) / 100;
			ptCalcSample->u8Weight = ptMng->au32WorkFF[(uint32_t)sqrt(((int32_t)((u32Half * 2) * u8SampleX / (u8UseSample - 1)) - u32Half) * ((int32_t)((u32Half * 2) * u8SampleX / (u8UseSample - 1)) - u32Half) + ((int32_t)((u32Half * 2) * u8SampleY / (u8UseSample - 1)) - u32Half) * ((int32_t)((u32Half * 2) * u8SampleY / (u8UseSample - 1)) - u32Half)) * 90 / (u8Radius * 10)];
			ptCalcSample++;
		}
	}

	pu32Src  = &ptMng->pu32Buffer[ ptMng->bBufferMain * ptMng->u16Width * ptMng->u16Height];
	pu32Dest = &ptMng->pu32Buffer[!ptMng->bBufferMain * ptMng->u16Width * ptMng->u16Height];

	ptMng->bWorkHSV = FALSE;
	for(ptMng->u16WorkSrcY = 0; ptMng->u16WorkSrcY < ptMng->u16Height; ptMng->u16WorkSrcY++) {
		pu32YDest = &pu32Dest[ptMng->u16WorkSrcY * ptMng->u16Width];
		ptMng->u16WorkSrcX = 0;
		FetchWork(hMng);
		for(; ptMng->u16WorkSrcX < ptMng->u16Width; ptMng->u16WorkSrcX++) {
			u32SrcRGB = pu32Src[ptMng->u16WorkSrcY * ptMng->u16Width + ptMng->u16WorkSrcX];
			u8CR = GETR(u32SrcRGB);
			u8CG = GETG(u32SrcRGB);
			u8CB = GETB(u32SrcRGB);
			u32AllR = u32AllG = u32AllB = 0;
			u32SampleCount = 0;
			for(u8SampleY = 0; u8SampleY < u8UseSample; u8SampleY++) {
				ptCalcSample = &ptMng->ptCalcSample[u8SampleY * u8UseSample];
				for(u8SampleX = 0; u8SampleX < u8UseSample; u8SampleX++) {
					u32RGB = ptMng->pu32Work[ptCalcSample->u32Y * ptMng->u8WorkSize + ptCalcSample->u32X];
					if(u32SrcRGB == u32RGB) {
						u32AllR += u8CR;
						u32AllG += u8CG;
						u32AllB += u8CB;
						u32SampleCount++;
					} else if(u32RGB != VF_COLOR_OUTOFRANGE) {
						i16DR = (int16_t)GETR(u32RGB) - u8CR;
						i16DG = (int16_t)GETG(u32RGB) - u8CG;
						i16DB = (int16_t)GETB(u32RGB) - u8CB;
						bCalc = TRUE;
						if(bCalc) {
							if(i16DR < -1 * u8UseRDiff || u8UseRDiff < i16DR) {
								bCalc = FALSE;
							}
						}
						if(bCalc) {
							if(i16DG < -1 * u8UseGDiff || u8UseGDiff < i16DG) {
								bCalc = FALSE;
							}
						}
						if(bCalc) {
							if(i16DB < -1 * u8UseBDiff || u8UseBDiff < i16DB) {
								bCalc = FALSE;
							}
						}
						if(bCalc) {
							u32AllR += u8CR + (i16DR * ptCalcSample->u8Weight) / 255;
							u32AllG += u8CG + (i16DG * ptCalcSample->u8Weight) / 255;
							u32AllB += u8CB + (i16DB * ptCalcSample->u8Weight) / 255;
							u32SampleCount++;
						}
					}
					ptCalcSample++;
				}
			}
			*pu32YDest = SETRGB(u32AllR / u32SampleCount, u32AllG / u32SampleCount, u32AllB / u32SampleCount);
			pu32YDest++;
			WorkRight(hMng);
		}
	}
}

void VideoFilter_Calc(h_VideoFilterMng hMng) {
	VF_Mng_t* ptMng = (VF_Mng_t*)hMng;
	uint16_t u16X, u16Y;
	uint32_t* pu32Src;
	uint32_t* pu32Dest;
	VF_Profile_t* ptProfile;
	VFE_t* ptFilter;
	uint8_t i;

	if(!hMng) {
		return;
	}

	pu32Src  = &ptMng->pu32Buffer[ ptMng->bBufferMain * ptMng->u16Width * ptMng->u16Height];
	pu32Dest = &ptMng->pu32Buffer[!ptMng->bBufferMain * ptMng->u16Width * ptMng->u16Height];

	if(ptMng->bEnable) {
		ptProfile = &ptMng->atProfile[ptMng->u8ProfileNo];
		for(i = 0; i <= ptProfile->u8OutputNo && i < ptProfile->u8FilterCount; i++) {
			ptFilter = &ptProfile->atFilters[i];
			if(ptFilter->tBase.bEnable) {
				switch (ptFilter->tBase.tType) {
				case VFE_TYPE_NP:
					VideoFilter_NP(hMng);
					break;
				case VFE_TYPE_DDOWN:
					VideoFilter_DDown(
						hMng,
						(uint8_t) ptFilter->tDDown.u32DDown
					);
					break;
				case VFE_TYPE_GREY:
					VideoFilter_Grey(
						hMng,
						(uint8_t) ptFilter->tGrey.u32Bit,
						(uint8_t) ptFilter->tGrey.u32H,
						(uint8_t) ptFilter->tGrey.u32S,
						(uint8_t) ptFilter->tGrey.u32V
					);
					break;
				case VFE_TYPE_GAMMA:
					VideoFilter_Gamma(
						hMng,
						(uint8_t) ptFilter->tGamma.u32Gamma
					);
					break;
				case VFE_TYPE_ROTATEH:
					VideoFilter_RotateH(
						hMng,
						(uint16_t) ptFilter->tRotateH.u32RotateH
					);
					break;
				case VFE_TYPE_HSVSMOOTH:
					VideoFilter_HSVSmooth(
						hMng,
						(uint8_t) ptFilter->tHSVSmooth.u32Radius,
						(uint8_t) ptFilter->tHSVSmooth.u32Sample,
						(uint8_t) ptFilter->tHSVSmooth.u32HDiff,
						(uint8_t) ptFilter->tHSVSmooth.u32SDiff,
						(uint8_t) ptFilter->tHSVSmooth.u32VDiff,
						(uint8_t) ptFilter->tHSVSmooth.u32WType
					);
					break;
				case VFE_TYPE_RGBSMOOTH:
					VideoFilter_RGBSmooth(
						hMng,
						(uint8_t) ptFilter->tRGBSmooth.u32Radius,
						(uint8_t) ptFilter->tRGBSmooth.u32Sample,
						(uint8_t) ptFilter->tRGBSmooth.u32RDiff,
						(uint8_t) ptFilter->tRGBSmooth.u32GDiff,
						(uint8_t) ptFilter->tRGBSmooth.u32BDiff,
						(uint8_t) ptFilter->tRGBSmooth.u32WType
					);
					break;
				default:
					break;
				}
			}
		}
	} else {
		ptMng->bBufferMain ^= 1;
	}
}

void VideoFilter_GetDestPos(h_VideoFilterMng hMng, void* pOutput, const uint16_t u16X, const uint16_t u16Y, const uint8_t u8OutputBPP) {
	VF_Mng_t* ptMng = (VF_Mng_t*)hMng;
	uint32_t* pu32Dest;
	uint32_t u32RGB;

	if(!hMng  || !u8OutputBPP) {
		return;
	}
	if(u16X >= ptMng->u16Width || u16Y >= ptMng->u16Height) {
		return;
	}

	pu32Dest = &ptMng->pu32Buffer[!ptMng->bBufferMain * ptMng->u16Width * ptMng->u16Height];

	u32RGB = pu32Dest[u16Y * ptMng->u16Width + u16X];
	switch(u8OutputBPP) {
	case 2:
//		*(uint16_t*)pOutput = (GETB(u32RGB) >> 3) | ((GETG(u32RGB) >> 3) << 5) | ((GETR(u32RGB) >> 3) << 11);  // RGB555
		*(uint16_t*)pOutput = (GETB(u32RGB) >> 3) | ((GETG(u32RGB) >> 2) << 5) | ((GETR(u32RGB) >> 3) << 11);  // RGB565
		break;
	case 3:
		((uint8_t*)pOutput)[RGB24_R] = GETR(u32RGB);
		((uint8_t*)pOutput)[RGB24_G] = GETG(u32RGB);
		((uint8_t*)pOutput)[RGB24_B] = GETB(u32RGB);
		break;
	case 4:
		*(uint32_t*)pOutput = u32RGB;
		break;
	}
}

void VideoFilter_Export(h_VideoFilterMng hMng, void* pOutputBuf, const uint8_t u8OutputBPP, const uint16_t u16YAlign) {
	VF_Mng_t* ptMng = (VF_Mng_t*)hMng;
	uint16_t u16X, u16Y;
	uint8_t* pu8YDest;
	uint32_t u32RGB;

	if(!hMng || !pOutputBuf || !u8OutputBPP || !u16YAlign) {
		return;
	}

	for(u16Y = 0; u16Y < ptMng->u16Height; u16Y++) {
		pu8YDest = &((uint8_t*)pOutputBuf)[u16Y * u16YAlign];
		for(u16X = 0; u16X < ptMng->u16Width; u16X++) {
			VideoFilter_GetDestPos(hMng, pu8YDest, u16X, u16Y, u8OutputBPP);
			pu8YDest += u8OutputBPP;
		}
	}
}

#endif  // SUPPORT_VIDEOFILTER

