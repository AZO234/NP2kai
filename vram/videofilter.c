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

static void RGBtoHSV_d(uint16_t* pu16H, uint8_t* pu8S, uint8_t* pu8V, const uint8_t u8R, const uint8_t u8G, const uint8_t u8B) {
	int16_t i16H;
	uint8_t u8D;

//	if(!pu16H || !pu8S || !pu8V) {
//		return;
//	}

	if(u8R == u8G && u8R == u8B) {
		i16H = 0;
		*pu8S = 0;
		*pu8V = u8R;
	} else if(u8R >= u8G && u8R >= u8B) {
		if(u8B >= u8G) {
			u8D = u8R - u8G;
			i16H = 60 * ((int16_t)u8G - u8B) / u8D;
		} else {
			u8D = u8R - u8B;
			i16H = 60 * ((int16_t)u8G - u8B) / u8D;
		}
		*pu8S = (uint8_t)((uint16_t)u8D * 255 / u8R);
		*pu8V = u8R;
	} else if(u8G >= u8R && u8G >= u8B) {
		if(u8R >= u8B) {
			u8D = u8G - u8B;
			i16H = 60 * ((int16_t)u8B - u8R) / u8D + 120;
		} else {
			u8D = u8G - u8R;
			i16H = 60 * ((int16_t)u8B - u8R) / u8D + 120;
		}
		*pu8S = (uint8_t)((uint16_t)u8D * 255 / u8G);
		*pu8V = u8G;
	} else {
		if(u8G >= u8R) {
			u8D = u8B - u8R;
			i16H = 60 * ((int16_t)u8R - u8G) / u8D + 240;
		} else {
			u8D = u8B - u8G;
			i16H = 60 * ((int16_t)u8R - u8G) / u8D + 240;
		}
		*pu8S = (uint8_t)((uint16_t)u8D * 255 / u8B);
		*pu8V = u8B;
	}
	if(i16H < 0) {
		i16H += 360;
	}
	*pu16H = (uint16_t)i16H;
}

static uint32_t RGBtoHSV(const uint32_t u32RGB) {
	uint8_t u8R = GETR(u32RGB);
	uint8_t u8G = GETG(u32RGB);
	uint8_t u8B = GETB(u32RGB);
	int16_t u16H;
	uint8_t u8S;
	uint8_t u8V;

	RGBtoHSV_d(&u16H, &u8S, &u8V, u8R, u8G, u8B);

	return SETHSV(u16H, u8S, u8V);
}

static void HSVtoRGB_d(uint8_t* pu8R, uint8_t* pu8G, uint8_t* pu8B, const uint16_t u16H, const uint8_t u8S, const uint8_t u8V) {
	uint16_t u16UseH = u16H % 360;
	uint8_t u8Min = u8V - (u8S * u8V) / 255;
	uint8_t u8R, u8G, u8B;

//	if(!pu8R || !pu8G || !pu8B) {
//		return;
//	}

	if(u16UseH < 60) {
		*pu8R = u8V;
		*pu8G =        u16UseH  * (u8V - u8Min) / 60 + u8Min;
		*pu8B = u8Min;
	} else if(u16UseH >= 60 && u16UseH < 120) {
		*pu8R = (120 - u16UseH) * (u8V - u8Min) / 60 + u8Min;
		*pu8G = u8V;
		*pu8B = u8Min;
	} else if(u16UseH >= 120 && u16UseH < 180) {
		*pu8R = u8Min;
		*pu8G = u8V;
		*pu8B = (u16UseH - 120) * (u8V - u8Min) / 60 + u8Min;
	} else if(u16UseH >= 180 && u16UseH < 240) {
		*pu8R = u8Min;
		*pu8G = (240 - u16UseH) * (u8V - u8Min) / 60 + u8Min;
		*pu8B = u8V;
	} else if(u16UseH >= 240 && u16UseH < 300) {
		*pu8R = (u16UseH - 240) * (u8V - u8Min) / 60 + u8Min;
		*pu8G = u8Min;
		*pu8B = u8V;
	} else if(u16UseH >= 300 && u16UseH < 360) {
		*pu8R = u8V;
		*pu8G = u8Min;
		*pu8B = (360 - u16UseH) * (u8V - u8Min) / 60 + u8Min;
	}
}

static uint32_t HSVtoRGB(const uint32_t u32HSV) {
	uint16_t u16H = GETH(u32HSV);
	uint8_t u8S = GETS(u32HSV);
	uint8_t u8V = GETV(u32HSV);
	uint8_t u8R;
	uint8_t u8G;
	uint8_t u8B;

	HSVtoRGB_d(&u8R, &u8G, &u8B, u16H, u8S, u8V);

	return SETRGB(u8R, u8G, u8B);
}

static void RGBtoXYZ_d(uint8_t* pu8X, uint8_t* pu8Y, uint8_t* pu8Z, const uint8_t u8R, const uint8_t u8G, const uint8_t u8B) {
//	if(!pu8X || !pu8Y || !pu8Z) {
//		return;
//	}

	//  0.4887180  0.3106803  0.2006017
	//  0.1762044  0.8129847  0.0108109
	//  0.0000000  0.0102048  0.9897952
	*pu8X = (488718 * u8R + 310680 * u8G + 200601 * u8B) / 1000000;
	*pu8Y = (176204 * u8R + 812984 * u8G +  10810 * u8B) / 1000000;
	*pu8Z = (     0 * u8R +  10204 * u8G + 989795 * u8B) / 1000000;
}

static uint32_t RGBtoXYZ(const uint32_t u32RGB) {
	uint8_t u8R = GETR(u32RGB);
	uint8_t u8G = GETG(u32RGB);
	uint8_t u8B = GETB(u32RGB);
	int16_t u8X;
	uint8_t u8Y;
	uint8_t u8Z;

	RGBtoHSV_d(&u8X, &u8Y, &u8Z, u8R, u8G, u8B);

	return SETXYZ(u8X, u8Y, u8Z);
}

static uint32_t XYZtoRGB_d(uint8_t* pu8R, uint8_t* pu8G, uint8_t* pu8B, const uint8_t u8X, const uint8_t u8Y, const uint8_t u8Z) {
//	if(!pu8R || !pu8G || !pu8B) {
//		return;
//	}

	//  2.3706743 -0.9000405 -0.4706338
	// -0.5138850  1.4253036  0.0885814
	//  0.0052982 -0.0146949  1.0093968
	*pu8R = (2370674 * u8X -  900040 * u8Y -  470633 * u8Z) / 1000000;
	*pu8G = (1425303 * u8Y -  513885 * u8X +   88581 * u8Z) / 1000000;
	*pu8B = (   5298 * u8X -   14684 * u8Y + 1009396 * u8Z) / 1000000;
}

static uint32_t XYZtoRGB(const uint32_t u32XYZ) {
	uint8_t u8X = GETX(u32XYZ);
	uint8_t u8Y = GETY(u32XYZ);
	uint8_t u8Z = GETZ(u32XYZ);
	uint8_t u8R;
	uint8_t u8G;
	uint8_t u8B;

	XYZtoRGB_d(&u8R, &u8G, &u8B, u8X, u8Y, u8Z);

	return SETRGB(u8R, u8G, u8B);
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

typedef struct VF_Palette_t_ {
	union {
		struct {
			uint8_t u8B;
			uint8_t u8G;
			uint8_t u8R;
			uint8_t u8Z;
		} tRGB;
		uint32_t u32RGB;
	};
	union {
		struct {
			uint8_t  u8V;
			uint8_t  u8S;
			uint16_t u16H;
		} tHSV;
		uint32_t u32HSV;
	};
} VF_Palette_t;

VF_Palette_t m_atPalette[256];

typedef union VF_Dot_t_ {
	struct {
		uint8_t u8B;
		uint8_t u8G;
		uint8_t u8R;
		uint8_t u8Z;
	} tRGB;
	uint32_t u32RGB;
} VF_Dot_t;

typedef union VF_WorkDot_t_ {
	struct {
		uint8_t u8B;
		uint8_t u8G;
		uint8_t u8R;
		uint8_t u8Z;
	} tRGB;
	uint32_t u32RGB;
	struct {
		uint8_t  u8V;
		uint8_t  u8S;
		uint16_t u16H;
	} tHSV;
	uint32_t u32HSV;
} VF_WorkDot_t;

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
	uint8_t          u8SetProfileNo;

	uint16_t         u16MaxWidth;
	uint16_t         u16MaxHeight;
	uint16_t         u16Width;
	uint16_t         u16Height;
	uint8_t          u8MaxRadius;
	BOOL             bBufferMain;
	VF_Dot_t*        ptBuffer;
	uint8_t*         pu8VRAM;
	uint8_t*         pu8Dirty;
	uint8_t*         pu8DirtyOrg;
	uint32_t         u32Pallet;
	uint8_t          u8WorkSize;
	VF_WorkDot_t*    ptWork;
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
	ptMng->u8MaxRadius  = u8MaxRadius;
	ptMng->u8WorkSize   = u8MaxRadius * 2;
	ptMng->u8MaxSample  = u8MaxSample;

	ptMng->ptBuffer = (VF_Dot_t*)malloc(ptMng->u16MaxWidth * ptMng->u16MaxHeight * 2 * sizeof(VF_Dot_t));
	if(!ptMng->ptBuffer) {
		VideoFilter_Deinit(ptMng);
		return NULL;
	}
	ptMng->pu8Dirty = (uint8_t*)malloc(u16MaxHeight);
	if(!ptMng->pu8Dirty) {
		VideoFilter_Deinit(ptMng);
		return NULL;
	}
	ptMng->ptWork = (VF_WorkDot_t*)malloc(ptMng->u8WorkSize * ptMng->u8WorkSize * sizeof(VF_WorkDot_t));
	if(!ptMng->ptWork) {
		VideoFilter_Deinit(ptMng);
		return NULL;
	}
	ptMng->ptCalcSample = (VF_CalcSample_t*)malloc(u8MaxSample * u8MaxSample * sizeof(VF_CalcSample_t));
	if(!ptMng->ptCalcSample) {
		VideoFilter_Deinit(ptMng);
		return NULL;
	}

	VideoFilter_SetSize(ptMng, 640, 480);

	return ptMng;
}

void VideoFilter_Deinit(h_VideoFilterMng hMng) {
	VF_Mng_t* ptMng = (VF_Mng_t*)hMng;

	if(!hMng) {
		return;
	}

	if(ptMng->ptBuffer) {
		free(ptMng->ptBuffer);
	}
	if(ptMng->pu8Dirty) {
		free(ptMng->pu8Dirty);
	}
	if(ptMng->ptWork) {
		free(ptMng->ptWork);
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

	ptMng->bEnable = bEnable;
	ptMng->u8ProfileCount = u8ProfileCount;
	ptMng->u8SetProfileNo = u8UseProfileNo;
	ptMng->u8WorkSize = 0;
	ptMng->bWorkHSV = FALSE;

	VideoFilter_SetSize(hMng, 640, 480);
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

BOOL VideoFilter_GetEnable(h_VideoFilterMng hMng) {
	VF_Mng_t* ptMng = (VF_Mng_t*)hMng;

	if(!hMng) {
		return FALSE;
	}

	return ptMng->bEnable;
}

void VideoFilter_SetEnable(h_VideoFilterMng hMng, const BOOL bEnable) {
	VF_Mng_t* ptMng = (VF_Mng_t*)hMng;

	if(!hMng) {
		return;
	}

	ptMng->bEnable = bEnable ? TRUE : FALSE;
}

BOOL VideoFilter_GetProfileNo(h_VideoFilterMng hMng) {
	VF_Mng_t* ptMng = (VF_Mng_t*)hMng;

	if(!hMng) {
		return FALSE;
	}

	return ptMng->u8SetProfileNo;
}

void VideoFilter_SetProfileNo(h_VideoFilterMng hMng, const uint8_t u8ProfileNo) {
	VF_Mng_t* ptMng = (VF_Mng_t*)hMng;

	if(!hMng) {
		return;
	}
	if(u8ProfileNo >= ptMng->u8ProfileCount) {
		return;
	}

	ptMng->u8SetProfileNo = u8ProfileNo;
}

void VideoFilter_SetSize(h_VideoFilterMng hMng, const uint16_t u16Width, const uint16_t u16Height) {
	VF_Mng_t* ptMng = (VF_Mng_t*)hMng;

	if(!hMng) {
		return;
	}
	if(u16Width > ptMng->u16MaxWidth || u16Height > ptMng->u16MaxHeight) {
		return;
	}

	ptMng->u16Width  = u16Width;
	ptMng->u16Height = u16Height;

	memset(ptMng->ptBuffer, 0, ptMng->u16MaxWidth * ptMng->u16MaxHeight * sizeof(VF_Dot_t));
	memset(ptMng->pu8Dirty, 1, ptMng->u16MaxHeight);
}

void VideoFilter_SetSrcRGB_d(h_VideoFilterMng hMng, const uint16_t u16X, const uint16_t u16Y, const uint8_t u8R, const uint8_t u8G, const uint8_t u8B) {
	VF_Mng_t* ptMng = (VF_Mng_t*)hMng;
	VF_Dot_t* ptSrc;

	if(!hMng) {
		return;
	}

	ptSrc = &ptMng->ptBuffer[ptMng->bBufferMain * ptMng->u16Width * ptMng->u16Height];

	if(u16X < ptMng->u16Width && u16Y < ptMng->u16Height) {
		ptSrc[u16Y * ptMng->u16Width + u16X].tRGB.u8R = u8R;
		ptSrc[u16Y * ptMng->u16Width + u16X].tRGB.u8G = u8G;
		ptSrc[u16Y * ptMng->u16Width + u16X].tRGB.u8B = u8B;
	}
}

void VideoFilter_SetSrcRGB(h_VideoFilterMng hMng, const uint16_t u16X, const uint16_t u16Y, const uint32_t u32RGB) {
	VF_Mng_t* ptMng = (VF_Mng_t*)hMng;
	VF_Dot_t* ptSrc;

	if(!hMng) {
		return;
	}

	ptSrc = &ptMng->ptBuffer[ptMng->bBufferMain * ptMng->u16Width * ptMng->u16Height];

	if(u16X < ptMng->u16Width && u16Y < ptMng->u16Height) {
		ptSrc[u16Y * ptMng->u16Width + u16X].u32RGB = u32RGB;
	}
}

void VideoFilter_SetSrcHSV_d(h_VideoFilterMng hMng, const uint16_t u16X, const uint16_t u16Y, const uint16_t u16H, const uint8_t u8S, const uint8_t u8V) {
	VF_Mng_t* ptMng = (VF_Mng_t*)hMng;
	VF_Dot_t* ptSrc;
	VF_Dot_t* ptSrcPos;
	uint16_t u16UseH = u16H % 360;

	if(!hMng) {
		return;
	}

	ptSrc = &ptMng->ptBuffer[ptMng->bBufferMain * ptMng->u16Width * ptMng->u16Height];

	if(u16X < ptMng->u16Width && u16Y < ptMng->u16Height) {
		ptSrcPos = &ptSrc[u16Y * ptMng->u16Width + u16X];
		HSVtoRGB_d(&ptSrcPos->tRGB.u8R, &ptSrcPos->tRGB.u8G, &ptSrcPos->tRGB.u8B, u16UseH, u8S, u8V);
	}
}
void VideoFilter_SetSrcHSV(h_VideoFilterMng hMng, const uint16_t u16X, const uint16_t u16Y, const uint32_t u32HSV) {
	VF_Mng_t* ptMng = (VF_Mng_t*)hMng;
	VF_Dot_t* ptSrc;
	VF_Dot_t* ptSrcPos;

	if(!hMng) {
		return;
	}

	ptSrc = &ptMng->ptBuffer[ptMng->bBufferMain * ptMng->u16Width * ptMng->u16Height];

	if(u16X < ptMng->u16Width && u16Y < ptMng->u16Height) {
		ptSrcPos = &ptSrc[u16Y * ptMng->u16Width + u16X];
		HSVtoRGB_d(&ptSrcPos->tRGB.u8R, &ptSrcPos->tRGB.u8G, &ptSrcPos->tRGB.u8B, GETH(u32HSV), GETS(u32HSV), GETV(u32HSV));
	}
}

static void VideoFilter_SetPalette(BOOL bPalletEx) {
	uint16_t u16Color;
	uint32_t u32Pallet;

	memset(m_atPalette, 0, sizeof(VF_Palette_t) * 256);

#if defined(SUPPORT_PC9821)
	if(bPalletEx) {
		u32Pallet = NP2PAL_GRPHEX;
	} else {
		u32Pallet = NP2PAL_GRPH;
	}
#else
	u32Pallet = NP2PAL_GRPH;
#endif

	for(u16Color = 0; u16Color < 256; u16Color++) {
		m_atPalette[u16Color].u32RGB = np2_pal32[u16Color + u32Pallet].d;
		RGBtoHSV_d(
			&m_atPalette[u16Color].tHSV.u16H,
			&m_atPalette[u16Color].tHSV.u8S,
			&m_atPalette[u16Color].tHSV.u8V,
			np2_pal32[u16Color + u32Pallet].p.r,
			np2_pal32[u16Color + u32Pallet].p.g,
			np2_pal32[u16Color + u32Pallet].p.b
		);
	}
}

void VideoFilter_Import98(h_VideoFilterMng hMng, uint8_t* pu8VRAM, uint8_t* pu8Dirty, BOOL bPalletEx) {
	VF_Mng_t* ptMng = (VF_Mng_t*)hMng;

	if(!hMng || !pu8VRAM || !pu8Dirty) {
		return;
	}

//	VideoFilter_SetSize(hMng, SURFACE_WIDTH, SURFACE_HEIGHT);
	ptMng->pu8VRAM = pu8VRAM;

#if defined(SUPPORT_PC9821)
	if(bPalletEx) {
		ptMng->u32Pallet = NP2PAL_GRPHEX;
	} else {
		ptMng->u32Pallet = NP2PAL_GRPH;
	}
#else
	ptMng->u32Pallet = NP2PAL_GRPH;
#endif
	VideoFilter_SetPalette(bPalletEx);

	memcpy(ptMng->pu8Dirty, pu8Dirty, ptMng->u16Height);
	ptMng->pu8DirtyOrg = pu8Dirty;
}

static void VideoFilter_Thru98(h_VideoFilterMng hMng) {
	VF_Mng_t* ptMng = (VF_Mng_t*)hMng;
	uint16_t u16X, u16Y;
	VF_Dot_t* ptSrc;
	VF_Dot_t* ptYSrc;
	uint8_t*  pu8YVRAMSrc;

	if(!hMng) {
		return;
	}

	ptSrc = &ptMng->ptBuffer[ptMng->bBufferMain * ptMng->u16Width * ptMng->u16Height];

	for(u16Y = 0; u16Y < ptMng->u16Height; u16Y++) {
		if(ptMng->pu8Dirty[u16Y]) {
			ptYSrc = &ptSrc[u16Y * ptMng->u16Width];
			pu8YVRAMSrc = &ptMng->pu8VRAM[u16Y * ptMng->u16Width];
			for(u16X = 0; u16X < ptMng->u16Width; u16X++) {
				ptYSrc->tRGB.u8R = m_atPalette[*pu8YVRAMSrc].tRGB.u8R;
				ptYSrc->tRGB.u8G = m_atPalette[*pu8YVRAMSrc].tRGB.u8G;
				ptYSrc->tRGB.u8B = m_atPalette[*pu8YVRAMSrc].tRGB.u8B;
				ptYSrc++;
				pu8YVRAMSrc++;
			}
		}
	}
}

void VideoFilter_Import(h_VideoFilterMng hMng, void* pInputBuf, const uint8_t u8InputBPP, const uint16_t u16YAlign) {
	VF_Mng_t* ptMng = (VF_Mng_t*)hMng;
	uint16_t u16X, u16Y;
	uint8_t* pu8YInput;
	VF_Dot_t* ptSrc;
	VF_Dot_t* ptSrcPos;

	if(!hMng || !pInputBuf || !u8InputBPP || !u16YAlign) {
		return;
	}

	ptSrc = &ptMng->ptBuffer[!ptMng->bBufferMain * ptMng->u16Width * ptMng->u16Height];

	for(u16Y = 0; u16Y < ptMng->u16Height; u16Y++) {
		pu8YInput = &((uint8_t*)pInputBuf)[u16Y * u16YAlign];
		ptSrcPos = &ptSrc[u16Y * ptMng->u16Width];
		for(u16X = 0; u16X < ptMng->u16Width; u16X++) {
			switch(u8InputBPP) {
			case 16:
				ptSrcPos->tRGB.u8R = (*(uint16_t*)pu8YInput >> 11) & 0x1F;
				ptSrcPos->tRGB.u8G = (*(uint16_t*)pu8YInput >>  5) & 0x3F;
				ptSrcPos->tRGB.u8B =  *(uint16_t*)pu8YInput        & 0x1F;
				break;
			case 24:
				ptSrcPos->tRGB.u8R = pu8YInput[RGB24_R];
				ptSrcPos->tRGB.u8G = pu8YInput[RGB24_G];
				ptSrcPos->tRGB.u8B = pu8YInput[RGB24_B];
				break;
			case 32:
				ptSrcPos->u32RGB = *(uint32_t*)pu8YInput;
				break;
			}
			pu8YInput += u8InputBPP;
		}
	}
}

uint32_t* VideoFilter_GetDest(h_VideoFilterMng hMng) {
	VF_Mng_t *ptMng = (VF_Mng_t *) hMng;

	if (hMng) {
		return &ptMng->ptBuffer[!ptMng->bBufferMain * ptMng->u16Width * ptMng->u16Height].u32RGB;
	} else {
		return NULL;
	}
}

void VideoFilter_NP(h_VideoFilterMng hMng) {
	VF_Mng_t* ptMng = (VF_Mng_t*)hMng;
	uint16_t u16X, u16Y;
	VF_Dot_t* ptSrc;
	VF_Dot_t* ptDest;
	uint8_t*  pu8YVRAMSrc;
	VF_Dot_t* ptYSrc;
	VF_Dot_t* ptYDest;

	if(!hMng) {
		return;
	}

	ptDest = &ptMng->ptBuffer[!ptMng->bBufferMain * ptMng->u16Width * ptMng->u16Height];

	if(ptMng->pu8VRAM) {
		for(u16Y = 0; u16Y < ptMng->u16Height; u16Y++) {
			if(ptMng->pu8Dirty[u16Y]) {
				pu8YVRAMSrc = &ptMng->pu8VRAM[ptMng->u16Width * u16Y];
				ptYDest     = &ptDest[ptMng->u16Width * u16Y];
				for(u16X = 0; u16X < ptMng->u16Width; u16X++) {
					ptYDest->u32RGB = ~(m_atPalette[*pu8YVRAMSrc].u32RGB) & 0x00FFFFFF;
					pu8YVRAMSrc++;
					ptYDest++;
				}
			}
		}
	} else {
		ptSrc = &ptMng->ptBuffer[ ptMng->bBufferMain * ptMng->u16Width * ptMng->u16Height];

		for(u16Y = 0; u16Y < ptMng->u16Height; u16Y++) {
			ptYSrc  = &ptSrc[ptMng->u16Width * u16Y];
			ptYDest = &ptDest[ptMng->u16Width * u16Y];
			for(u16X = 0; u16X < ptMng->u16Width; u16X++) {
				ptYDest->u32RGB = ~(ptYSrc->u32RGB) & 0x00FFFFFF;
				ptYSrc++;
				ptYDest++;
			}
		}
	}
}

void VideoFilter_DDown(h_VideoFilterMng hMng, const uint8_t u8DDown) {
	VF_Mng_t* ptMng = (VF_Mng_t*)hMng;
	uint16_t u16X, u16Y;
	VF_Dot_t* ptSrc;
	VF_Dot_t* ptDest;
	uint8_t*  pu8YVRAMSrc;
	VF_Dot_t* ptYSrc;
	VF_Dot_t* ptYDest;
	uint8_t u8UseDDown = u8DDown;
	uint8_t u8Div;

	if(!hMng) {
		return;
	}

	if(u8DDown > 8) {
		u8UseDDown = 7;
	}

	u8Div = (1 << (8 - u8DDown)) - 1;

	ptDest = &ptMng->ptBuffer[!ptMng->bBufferMain * ptMng->u16Width * ptMng->u16Height];

	if(ptMng->pu8VRAM) {
		for(u16Y = 0; u16Y < ptMng->u16Height; u16Y++) {
			if(ptMng->pu8Dirty[u16Y]) {
				pu8YVRAMSrc = &ptMng->pu8VRAM[ptMng->u16Width * u16Y];
				ptYDest     = &ptDest[ptMng->u16Width * u16Y];
				for(u16X = 0; u16X < ptMng->u16Width; u16X++) {
					ptYDest->tRGB.u8R = (m_atPalette[*pu8YVRAMSrc].tRGB.u8R >> u8DDown) * 255 / u8Div;
					ptYDest->tRGB.u8G = (m_atPalette[*pu8YVRAMSrc].tRGB.u8G >> u8DDown) * 255 / u8Div;
					ptYDest->tRGB.u8B = (m_atPalette[*pu8YVRAMSrc].tRGB.u8B >> u8DDown) * 255 / u8Div;
					pu8YVRAMSrc++;
					ptYDest++;
				}
			}
		}
	} else {
		ptSrc  = &ptMng->ptBuffer[ ptMng->bBufferMain * ptMng->u16Width * ptMng->u16Height];

		for(u16Y = 0; u16Y < ptMng->u16Height; u16Y++) {
			ptYSrc  = &ptSrc[ptMng->u16Width * u16Y];
			ptYDest = &ptDest[ptMng->u16Width * u16Y];
			for(u16X = 0; u16X < ptMng->u16Width; u16X++) {
				ptYDest->tRGB.u8R = (ptYSrc->tRGB.u8R >> u8DDown) * 255 / u8Div;
				ptYDest->tRGB.u8G = (ptYSrc->tRGB.u8G >> u8DDown) * 255 / u8Div;
				ptYDest->tRGB.u8B = (ptYSrc->tRGB.u8B >> u8DDown) * 255 / u8Div;
				ptYSrc++;
				ptYDest++;
			}
		}
	}
}

void VideoFilter_Grey(h_VideoFilterMng hMng, const uint8_t u8Bit, const uint16_t u16H, const uint8_t u8S, const uint8_t u8V) {
	VF_Mng_t* ptMng = (VF_Mng_t*)hMng;
	uint16_t u16X, u16Y;
	VF_Dot_t* ptSrc;
	VF_Dot_t* ptDest;
	uint8_t*  pu8YVRAMSrc;
	VF_Dot_t* ptYSrc;
	VF_Dot_t* ptYDest;
	uint8_t u8R, u8G, u8B;
	uint16_t u16UseH = u16H % 360;
	uint8_t u8UseBit = u8Bit;

	if(!hMng) {
		return;
	}

	if(u8Bit > 8) {
		u8UseBit = 8;
	}

	ptDest = &ptMng->ptBuffer[!ptMng->bBufferMain * ptMng->u16Width * ptMng->u16Height];

	if(ptMng->pu8VRAM) {
		for(u16Y = 0; u16Y < ptMng->u16Height; u16Y++) {
			if(ptMng->pu8Dirty[u16Y]) {
				pu8YVRAMSrc = &ptMng->pu8VRAM[ptMng->u16Width * u16Y];
				ptYDest     = &ptDest[ptMng->u16Width * u16Y];
				for(u16X = 0; u16X < ptMng->u16Width; u16X++) {
					ptYDest->u32RGB =
						((uint16_t)m_atPalette[*pu8YVRAMSrc].tRGB.u8R +
						m_atPalette[*pu8YVRAMSrc].tRGB.u8G +
						m_atPalette[*pu8YVRAMSrc].tRGB.u8B) / 3;
					pu8YVRAMSrc++;
					ptYDest++;
				}
			}
		}
	} else {
		ptSrc  = &ptMng->ptBuffer[ ptMng->bBufferMain * ptMng->u16Width * ptMng->u16Height];

		for(u16Y = 0; u16Y < ptMng->u16Height; u16Y++) {
			ptYSrc  = &ptSrc[ptMng->u16Width * u16Y];
			ptYDest = &ptDest[ptMng->u16Width * u16Y];
			for(u16X = 0; u16X < ptMng->u16Width; u16X++) {
				ptYDest->u32RGB = ((uint16_t)ptYDest->tRGB.u8R + ptYDest->tRGB.u8G + ptYDest->tRGB.u8B) / 3;
				ptYSrc++;
				ptYDest++;
			}
		}
	}

	ptMng->bBufferMain ^= 1;
	ptSrc  = &ptMng->ptBuffer[ ptMng->bBufferMain * ptMng->u16Width * ptMng->u16Height];
	ptDest = &ptMng->ptBuffer[!ptMng->bBufferMain * ptMng->u16Width * ptMng->u16Height];

	HSVtoRGB_d(&u8R, &u8G, &u8B, u16UseH, u8S, u8V);
	for(u16Y = 0; u16Y < ptMng->u16Height; u16Y++) {
		ptYSrc  = &ptSrc[ptMng->u16Width * u16Y];
		ptYDest = &ptDest[ptMng->u16Width * u16Y];
		for(u16X = 0; u16X < ptMng->u16Width; u16X++) {
			ptYDest->tRGB.u8R = (ptYSrc->u32RGB * u8R) / 255;
			ptYDest->tRGB.u8G = (ptYSrc->u32RGB * u8G) / 255;
			ptYDest->tRGB.u8B = (ptYSrc->u32RGB * u8B) / 255;
			ptYSrc++;
			ptYDest++;
		}
	}
}

void VideoFilter_Gamma(h_VideoFilterMng hMng, const uint8_t u8Gamma) {
	VF_Mng_t* ptMng = (VF_Mng_t*)hMng;
	uint16_t u16X, u16Y;
	VF_Dot_t* ptSrc;
	VF_Dot_t* ptDest;
	uint8_t*  pu8YVRAMSrc;
	VF_Dot_t* ptYSrc;
	VF_Dot_t* ptYDest;
	uint8_t u8UseGamma = u8Gamma;
	uint16_t u16H;
	uint8_t u8S, u8V;

	if(!hMng) {
		return;
	}

	if(u8Gamma == 0) {
		u8UseGamma = 1;
	}

	for(u16X = 0; u16X < 256; u16X++) {
		ptMng->au32WorkFF[u16X] = (uint8_t)(255 * pow(1.0 * u16X / 255, 1.0 / (u8UseGamma / 10.0)));
	}

	ptDest = &ptMng->ptBuffer[!ptMng->bBufferMain * ptMng->u16Width * ptMng->u16Height];

	if(ptMng->pu8VRAM) {
		for(u16Y = 0; u16Y < ptMng->u16Height; u16Y++) {
			if(ptMng->pu8Dirty[u16Y]) {
				pu8YVRAMSrc = &ptMng->pu8VRAM[ptMng->u16Width * u16Y];
				ptYDest     = &ptDest[ptMng->u16Width * u16Y];
				for(u16X = 0; u16X < ptMng->u16Width; u16X++) {
					ptYDest->u32RGB = HSVtoRGB(SETHSV(
						m_atPalette[*pu8YVRAMSrc].tHSV.u16H,
						m_atPalette[*pu8YVRAMSrc].tHSV.u8S,
						ptMng->au32WorkFF[m_atPalette[*pu8YVRAMSrc].tHSV.u8V]
					));
					pu8YVRAMSrc++;
					ptYDest++;
				}
			}
		}
	} else {
		ptSrc = &ptMng->ptBuffer[ptMng->bBufferMain * ptMng->u16Width * ptMng->u16Height];

		for(u16Y = 0; u16Y < ptMng->u16Height; u16Y++) {
			ptYSrc  = &ptSrc[ptMng->u16Width * u16Y];
			ptYDest = &ptDest[ptMng->u16Width * u16Y];
			for(u16X = 0; u16X < ptMng->u16Width; u16X++) {
				RGBtoHSV_d(&u16H, &u8S, &u8V, ptYSrc->tRGB.u8R, ptYSrc->tRGB.u8G, ptYSrc->tRGB.u8B);
				ptYDest->u32RGB = HSVtoRGB(SETHSV(u16H, u8S, ptMng->au32WorkFF[u8V]));
				ptYSrc++;
				ptYDest++;
			}
		}
	}
}

void VideoFilter_RotateH(h_VideoFilterMng hMng, const uint16_t u16RotateH) {
	VF_Mng_t* ptMng = (VF_Mng_t*)hMng;
	uint16_t u16X, u16Y;
	VF_Dot_t* ptSrc;
	VF_Dot_t* ptDest;
	uint8_t*  pu8YVRAMSrc;
	VF_Dot_t* ptYSrc;
	VF_Dot_t* ptYDest;
	uint16_t u16UseRotateH = u16RotateH % 360;
	uint16_t u16H;
	uint8_t u8S, u8V;

	if(!hMng) {
		return;
	}

	ptDest = &ptMng->ptBuffer[!ptMng->bBufferMain * ptMng->u16Width * ptMng->u16Height];

	if(ptMng->pu8VRAM) {
		for(u16Y = 0; u16Y < ptMng->u16Height; u16Y++) {
			if(ptMng->pu8Dirty[u16Y]) {
				pu8YVRAMSrc = &ptMng->pu8VRAM[ptMng->u16Width * u16Y];
				ptYDest     = &ptDest[ptMng->u16Width * u16Y];
				for(u16X = 0; u16X < ptMng->u16Width; u16X++) {
					ptYDest->u32RGB = HSVtoRGB(SETHSV(
						(m_atPalette[*pu8YVRAMSrc].tHSV.u16H + u16UseRotateH) % 360,
						m_atPalette[*pu8YVRAMSrc].tHSV.u8S,
						m_atPalette[*pu8YVRAMSrc].tHSV.u8V
					));
					pu8YVRAMSrc++;
					ptYDest++;
				}
			}
		}
	} else {
		ptSrc = &ptMng->ptBuffer[ptMng->bBufferMain * ptMng->u16Width * ptMng->u16Height];

		for(u16Y = 0; u16Y < ptMng->u16Height; u16Y++) {
			ptYSrc  = &ptSrc[ptMng->u16Width * u16Y];
			ptYDest = &ptDest[ptMng->u16Width * u16Y];
			for(u16X = 0; u16X < ptMng->u16Width; u16X++) {
				RGBtoHSV_d(&u16H, &u8S, &u8V, ptYSrc->tRGB.u8R, ptYSrc->tRGB.u8G, ptYSrc->tRGB.u8B);
				ptYDest->u32RGB = HSVtoRGB(SETHSV((u16H + u16UseRotateH) % 360, u8S, u8V));
				ptYSrc++;
				ptYDest++;
			}
		}
	}
}

static void FetchWork(h_VideoFilterMng hMng) {
	VF_Mng_t* ptMng = (VF_Mng_t*)hMng;
	int16_t i16SelectX, i16SelectY;
	VF_Dot_t* ptSrc;
	uint8_t*  pu8YVRAMSrc;
	VF_Dot_t* ptSrcPos;
	VF_WorkDot_t *ptWorkPos;

	if(!hMng) {
		return;
	}

	ptSrc = &ptMng->ptBuffer[ptMng->bBufferMain * ptMng->u16Width * ptMng->u16Height];

	i16SelectY = (int16_t)ptMng->u16WorkSrcY - ptMng->u8WorkSize / 2;
	for(ptMng->u16WorkY = 0; ptMng->u16WorkY < ptMng->u8WorkSize; ptMng->u16WorkY++) {
		ptWorkPos = &ptMng->ptWork[ptMng->u16WorkY * ptMng->u8WorkSize];
		if(i16SelectY >= 0 && i16SelectY < ptMng->u16Height) {
			i16SelectX = (int16_t)ptMng->u16WorkSrcX - ptMng->u8WorkSize / 2;
			for(ptMng->u16WorkX = 0; ptMng->u16WorkX < ptMng->u8WorkSize; ptMng->u16WorkX++) {
				if(i16SelectX >= 0 && i16SelectX < ptMng->u16Width) {
					if(ptMng->pu8VRAM) {
						pu8YVRAMSrc = &ptMng->pu8VRAM[i16SelectY * ptMng->u16Width + i16SelectX];
						if(ptMng->bWorkHSV) {
							RGBtoHSV_d(
								&ptWorkPos->tHSV.u16H,
								&ptWorkPos->tHSV.u8S,
								&ptWorkPos->tHSV.u8V,
								m_atPalette[*pu8YVRAMSrc].tRGB.u8R,
								m_atPalette[*pu8YVRAMSrc].tRGB.u8G,
								m_atPalette[*pu8YVRAMSrc].tRGB.u8B
							);
						} else {
							ptWorkPos->u32RGB = m_atPalette[*pu8YVRAMSrc].u32RGB;
						}
					} else {
						ptSrcPos = &ptSrc[i16SelectY * ptMng->u16Width + i16SelectX];
						if(ptMng->bWorkHSV) {
							RGBtoHSV_d(
								&ptWorkPos->tHSV.u16H,
								&ptWorkPos->tHSV.u8S,
								&ptWorkPos->tHSV.u8V,
								ptSrcPos->tRGB.u8R,
								ptSrcPos->tRGB.u8G,
								ptSrcPos->tRGB.u8B
							);
						} else {
							ptWorkPos->u32RGB = ptSrcPos->u32RGB;
						}
					}
				} else {
					ptWorkPos->u32RGB = VF_COLOR_OUTOFRANGE;
				}
				ptWorkPos++;
				i16SelectX++;
			}
		} else {
			for(ptMng->u16WorkX = 0; ptMng->u16WorkX < ptMng->u8WorkSize; ptMng->u16WorkX++) {
				ptWorkPos->u32RGB = VF_COLOR_OUTOFRANGE;
				ptWorkPos++;
			}
		}
		i16SelectY++;
	}
}

static void WorkRight(h_VideoFilterMng hMng) {
	VF_Mng_t* ptMng = (VF_Mng_t*)hMng;
	int16_t i16SelectX, i16SelectY;
	VF_Dot_t *ptSrc;
	uint8_t*  pu8YVRAMSrc;
	VF_Dot_t *ptSrcPos;
	VF_WorkDot_t *ptWorkPos;

	if(!hMng) {
		return;
	}

	ptSrc  = &ptMng->ptBuffer[ ptMng->bBufferMain * ptMng->u16Width * ptMng->u16Height];

	for(ptMng->u16WorkY = 0; ptMng->u16WorkY < ptMng->u8WorkSize; ptMng->u16WorkY++) {
		ptWorkPos = &ptMng->ptWork[ptMng->u16WorkY * ptMng->u8WorkSize];
		for(ptMng->u16WorkX = 0; ptMng->u16WorkX < ptMng->u8WorkSize - 1; ptMng->u16WorkX++) {
			ptWorkPos->u32RGB = (ptWorkPos + 1)->u32RGB;
			ptWorkPos++;
		}
	}

	i16SelectX = (int16_t)ptMng->u16WorkSrcX + ptMng->u8WorkSize / 2 + 1;
	i16SelectY = (int16_t)ptMng->u16WorkSrcY - ptMng->u8WorkSize / 2;
	for(ptMng->u16WorkY = 0; ptMng->u16WorkY < ptMng->u8WorkSize; ptMng->u16WorkY++) {
		ptWorkPos = &ptMng->ptWork[ptMng->u16WorkY * ptMng->u8WorkSize + ptMng->u8WorkSize - 1];
		if(i16SelectX < ptMng->u16Width) {
			if(i16SelectY >= 0 && i16SelectY < ptMng->u16Height) {
				if(ptMng->pu8VRAM) {
					pu8YVRAMSrc = &ptMng->pu8VRAM[i16SelectY * ptMng->u16Width + i16SelectX];
					if(ptMng->bWorkHSV) {
						RGBtoHSV_d(
							&ptWorkPos->tHSV.u16H,
							&ptWorkPos->tHSV.u8S,
							&ptWorkPos->tHSV.u8V,
							m_atPalette[*pu8YVRAMSrc].tRGB.u8R,
							m_atPalette[*pu8YVRAMSrc].tRGB.u8G,
							m_atPalette[*pu8YVRAMSrc].tRGB.u8B
						);
					} else {
						ptWorkPos->u32RGB = m_atPalette[*pu8YVRAMSrc].u32RGB;
					}
				} else {
					ptSrcPos = &ptSrc[i16SelectY * ptMng->u16Width + i16SelectX];
					if(ptMng->bWorkHSV) {
						RGBtoHSV_d(
							&ptWorkPos->tHSV.u16H,
							&ptWorkPos->tHSV.u8S,
							&ptWorkPos->tHSV.u8V,
							ptSrcPos->tRGB.u8R,
							ptSrcPos->tRGB.u8G,
							ptSrcPos->tRGB.u8B
						);
					} else {
						ptWorkPos->u32RGB = ptSrcPos->u32RGB;
					}
				}
			} else {
				ptWorkPos->u32RGB = VF_COLOR_OUTOFRANGE;
			}
		} else {
			ptWorkPos->u32RGB = VF_COLOR_OUTOFRANGE;
		}
		i16SelectY++;
	}
}

void VideoFilter_HSVSmooth(h_VideoFilterMng hMng, const uint8_t u8Radius, const uint8_t u8Sample, const uint8_t u8HDiff, const uint8_t u8SDiff, const uint8_t u8VDiff, const uint8_t u8WType) {
	VF_Mng_t* ptMng = (VF_Mng_t*)hMng;
	VF_Dot_t* ptDest;
	VF_Dot_t* ptYDest;
	VF_WorkDot_t* ptC;
	VF_WorkDot_t* ptD;
	uint8_t u8UseRadius = u8Radius;
	uint8_t u8UseSample = u8Sample;
	uint8_t u8UseHDiff = u8HDiff;
	uint8_t u8UseSDiff = u8SDiff;
	uint8_t u8UseVDiff = u8VDiff;
	uint16_t u16CH;
	int16_t i16DH, i16DS, i16DV;
	int32_t i32AllH;
	uint32_t u32AllS, u32AllV;
	int16_t i16DY;
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
		for(u32SampleCount = 0; u32SampleCount < 90; u32SampleCount++) {
			ptMng->au32WorkFF[u32SampleCount] = (uint8_t)(((90 - u32SampleCount) * 255) / 90);
		}
		break;
	case 2:
		for(u32SampleCount = 0; u32SampleCount < 90; u32SampleCount++) {
			ptMng->au32WorkFF[u32SampleCount] = (uint8_t)(cos(u32SampleCount * 3.141592 / 180.0) * 255);
		}
		break;
	default:
		for(u32SampleCount = 0; u32SampleCount < 90; u32SampleCount++) {
			ptMng->au32WorkFF[u32SampleCount] = 255;
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

	ptDest = &ptMng->ptBuffer[!ptMng->bBufferMain * ptMng->u16Width * ptMng->u16Height];
	ptC = &ptMng->ptWork[(ptMng->u8WorkSize / 2) * ptMng->u8WorkSize + ptMng->u8WorkSize / 2];

	ptMng->bWorkHSV = TRUE;
	for(ptMng->u16WorkSrcY = 0; ptMng->u16WorkSrcY < ptMng->u16Height; ptMng->u16WorkSrcY++) {
		bCalc = TRUE;
		if(ptMng->pu8VRAM) {
			bCalc = FALSE;
			for(i16DY = ptMng->u16WorkSrcY - ptMng->u8WorkSize / 2; i16DY < ptMng->u16WorkSrcY + ptMng->u8WorkSize / 2 + 1; i16DY++) {
				if(0 <= i16DY && i16DY <= ptMng->u16Height) {
					if(ptMng->pu8Dirty[i16DY]) {
						bCalc = TRUE;
						break;
					}
				}
			}
			if(bCalc) {
				for(i16DY = ptMng->u16WorkSrcY - ptMng->u8WorkSize / 2; i16DY < ptMng->u16WorkSrcY + ptMng->u8WorkSize / 2 + 1; i16DY++) {
					if(0 <= i16DY && i16DY <= SURFACE_HEIGHT) {
						ptMng->pu8DirtyOrg[i16DY] = 1;
					}
				}
			}
		}
		if(bCalc) {
			ptYDest = &ptDest[ptMng->u16WorkSrcY * ptMng->u16Width];
			ptMng->u16WorkSrcX = 0;
			FetchWork(hMng);
			for(; ptMng->u16WorkSrcX < ptMng->u16Width; ptMng->u16WorkSrcX++) {
				i32AllH = u32AllS = u32AllV = 0;
				u32SampleCount = 0;
				for(u8SampleY = 0; u8SampleY < u8UseSample; u8SampleY++) {
					ptCalcSample = &ptMng->ptCalcSample[u8SampleY * u8UseSample];
					for(u8SampleX = 0; u8SampleX < u8UseSample; u8SampleX++) {
						ptD = &ptMng->ptWork[ptCalcSample->u32Y * ptMng->u8WorkSize + ptCalcSample->u32X];
						if(ptC->u32HSV == ptD->u32HSV) {
							i32AllH += ptC->tHSV.u16H;
							u32AllS += ptC->tHSV.u8S;
							u32AllV += ptC->tHSV.u8V;
							u32SampleCount++;
						} else if(ptD->u32HSV != VF_COLOR_OUTOFRANGE) {
							i16DH = ptD->tHSV.u16H - ptC->tHSV.u16H;
							if(i16DH > 180) {
								i16DH -= 360;
							} else if(i16DH < -180) {
								i16DH += 360;
							}
							if(!ptD->tHSV.u8S) {
								i16DH = 0;
							}
							i16DS = ptD->tHSV.u8S - ptC->tHSV.u8S;
							i16DV = ptD->tHSV.u8V - ptC->tHSV.u8V;
							bCalc = TRUE;
							if(bCalc) {
								if(ptD->tHSV.u8V && (i16DH < -1 * u8UseHDiff || u8UseHDiff < i16DH)) {
									bCalc = FALSE;
								}
							}
							if(bCalc) {
								if(ptD->tHSV.u8V && (i16DS < -1 * u8UseSDiff || u8UseSDiff < i16DS)) {
									bCalc = FALSE;
								}
							}
							if(bCalc) {
								if(i16DV < -1 * u8UseVDiff || u8UseVDiff < i16DV) {
									bCalc = FALSE;
								}
							}
							if(bCalc) {
								i32AllH += ptC->tHSV.u16H + (i16DH * ptCalcSample->u8Weight) / 255;
								u32AllS += ptC->tHSV.u8S  + (i16DS * ptCalcSample->u8Weight) / 255;
								u32AllV += ptC->tHSV.u8V  + (i16DV * ptCalcSample->u8Weight) / 255;
								u32SampleCount++;
							}
						}
						ptCalcSample++;
					}
				}
				while(i32AllH < 0) {
					i32AllH += 360;
				}
				i32AllH /= u32SampleCount;
				if(i32AllH > 360) {
					i32AllH %= 360;
				}
				HSVtoRGB_d(
					&ptYDest->tRGB.u8R,
					&ptYDest->tRGB.u8G,
					&ptYDest->tRGB.u8B,
					i32AllH,
					u32AllS / u32SampleCount,
					u32AllV / u32SampleCount
				);
				ptYDest++;
				WorkRight(hMng);
			}
		}
	}
}

void VideoFilter_RGBSmooth(h_VideoFilterMng hMng, const uint8_t u8Radius, const uint8_t u8Sample, const uint8_t u8RDiff, const uint8_t u8GDiff, const uint8_t u8BDiff, const uint8_t u8WType) {
	VF_Mng_t* ptMng = (VF_Mng_t*)hMng;
	VF_Dot_t* ptDest;
	VF_Dot_t* ptYDest;
	VF_WorkDot_t* ptC;
	VF_WorkDot_t* ptD;
	uint8_t u8UseRadius = u8Radius;
	uint8_t u8UseSample = u8Sample;
	uint8_t u8UseRDiff = u8RDiff;
	uint8_t u8UseGDiff = u8GDiff;
	uint8_t u8UseBDiff = u8BDiff;
	int16_t i16DR, i16DG, i16DB;
	uint32_t u32AllR, u32AllG, u32AllB;
	int16_t i16DY;
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
		for(u32SampleCount = 0; u32SampleCount < 90; u32SampleCount++) {
			ptMng->au32WorkFF[u32SampleCount] = (uint8_t)(((90 - u32SampleCount) * 255) / 90);
		}
		break;
	case 2:
		for(u32SampleCount = 0; u32SampleCount < 90; u32SampleCount++) {
			ptMng->au32WorkFF[u32SampleCount] = (uint8_t)(cos(u32SampleCount * 3.141592 / 180.0) * 255);
		}
		break;
	default:
		for(u32SampleCount = 0; u32SampleCount < 90; u32SampleCount++) {
			ptMng->au32WorkFF[u32SampleCount] = 255;
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

	ptDest = &ptMng->ptBuffer[!ptMng->bBufferMain * ptMng->u16Width * ptMng->u16Height];
	ptC = &ptMng->ptWork[(ptMng->u8WorkSize / 2) * ptMng->u8WorkSize + ptMng->u8WorkSize / 2];

	ptMng->bWorkHSV = FALSE;
	for(ptMng->u16WorkSrcY = 0; ptMng->u16WorkSrcY < ptMng->u16Height; ptMng->u16WorkSrcY++) {
		bCalc = TRUE;
		if(ptMng->pu8VRAM) {
			bCalc = FALSE;
			for(i16DY = ptMng->u16WorkSrcY - ptMng->u8WorkSize / 2; i16DY < ptMng->u16WorkSrcY + ptMng->u8WorkSize / 2 + 1; i16DY++) {
				if(0 <= i16DY && i16DY <= ptMng->u16Height) {
					if(ptMng->pu8Dirty[i16DY]) {
						bCalc = TRUE;
						break;
					}
				}
			}
			if(bCalc) {
				for(i16DY = ptMng->u16WorkSrcY - ptMng->u8WorkSize / 2; i16DY < ptMng->u16WorkSrcY + ptMng->u8WorkSize / 2 + 1; i16DY++) {
					if(0 <= i16DY && i16DY <= SURFACE_HEIGHT) {
						ptMng->pu8DirtyOrg[i16DY] = 1;
					}
				}
			}
		}
		if(bCalc) {
			ptYDest = &ptDest[ptMng->u16WorkSrcY * ptMng->u16Width];
			ptMng->u16WorkSrcX = 0;
			FetchWork(hMng);
			for(; ptMng->u16WorkSrcX < ptMng->u16Width; ptMng->u16WorkSrcX++) {
				u32AllR = u32AllG = u32AllB = 0;
				u32SampleCount = 0;
				for(u8SampleY = 0; u8SampleY < u8UseSample; u8SampleY++) {
					ptCalcSample = &ptMng->ptCalcSample[u8SampleY * u8UseSample];
					for(u8SampleX = 0; u8SampleX < u8UseSample; u8SampleX++) {
						ptD = &ptMng->ptWork[ptCalcSample->u32Y * ptMng->u8WorkSize + ptCalcSample->u32X];
						if(ptC->u32RGB == ptD->u32RGB) {
							u32AllR += ptC->tRGB.u8R;
							u32AllG += ptC->tRGB.u8G;
							u32AllB += ptC->tRGB.u8B;
							u32SampleCount++;
						} else if(ptD->u32RGB != VF_COLOR_OUTOFRANGE) {
							i16DR = ptD->tRGB.u8R - ptC->tRGB.u8R;
							i16DG = ptD->tRGB.u8G - ptC->tRGB.u8G;
							i16DB = ptD->tRGB.u8B - ptC->tRGB.u8B;
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
								u32AllR += ptC->tRGB.u8R + (i16DR * ptCalcSample->u8Weight) / 255;
								u32AllG += ptC->tRGB.u8G + (i16DG * ptCalcSample->u8Weight) / 255;
								u32AllB += ptC->tRGB.u8B + (i16DB * ptCalcSample->u8Weight) / 255;
								u32SampleCount++;
							}
						}
						ptCalcSample++;
					}
				}
				ptYDest->tRGB.u8R = u32AllR / u32SampleCount;
				ptYDest->tRGB.u8G = u32AllG / u32SampleCount;
				ptYDest->tRGB.u8B = u32AllB / u32SampleCount;
				ptYDest++;
				WorkRight(hMng);
			}
		}
	}
}

void VideoFilter_Calc(h_VideoFilterMng hMng) {
	VF_Mng_t* ptMng = (VF_Mng_t*)hMng;
	VF_Profile_t* ptProfile;
	VFE_t* ptFilter;
	uint8_t i;

	if(!hMng) {
		return;
	}

	if(ptMng->u8ProfileNo != ptMng->u8SetProfileNo) {
		memset(ptMng->pu8Dirty, 1, ptMng->u16Height);
		if(ptMng->pu8VRAM) {
			memset(ptMng->pu8DirtyOrg, 1, SURFACE_HEIGHT);
		}
		ptMng->u8ProfileNo = ptMng->u8SetProfileNo;
	}

	if(ptMng->bEnable && ptMng->atProfile[ptMng->u8ProfileNo].atFilters[0].tBase.bEnable) {
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
				ptMng->pu8VRAM = NULL;
			}
		}
	} else {
		if(ptMng->pu8VRAM) {
			VideoFilter_Thru98(hMng);
		}
		ptMng->bBufferMain ^= 1;
		ptMng->pu8VRAM = NULL;
	}
}

void VideoFilter_PutSrc(h_VideoFilterMng hMng, void* pOutput, const uint16_t u16X, const uint16_t u16Y, const uint8_t u8OutputBPP) {
	VF_Mng_t* ptMng = (VF_Mng_t*)hMng;
	VF_Dot_t* ptSrc;
	VF_Dot_t* ptSrcPos;

	if(!hMng  || !u8OutputBPP) {
		return;
	}
	if(u16X >= ptMng->u16Width || u16Y >= ptMng->u16Height) {
		return;
	}

	ptSrc = &ptMng->ptBuffer[ptMng->bBufferMain * ptMng->u16Width * ptMng->u16Height];

	ptSrcPos = &ptSrc[u16Y * ptMng->u16Width + u16X];
	switch(u8OutputBPP) {
	case 2:
//		*(uint16_t*)pOutput = (ptSrcPos->tRGB.u8B >> 3) | ((ptSrcPos->tRGB.u8G >> 3) << 5) | ((ptSrcPos->tRGB.u8R >> 3) << 11);  // RGB555
		*(uint16_t*)pOutput = (ptSrcPos->tRGB.u8B >> 3) | ((ptSrcPos->tRGB.u8G >> 2) << 5) | ((ptSrcPos->tRGB.u8R >> 3) << 11);  // RGB565
		break;
	case 3:
		((uint8_t*)pOutput)[RGB24_R] = ptSrcPos->tRGB.u8R;
		((uint8_t*)pOutput)[RGB24_G] = ptSrcPos->tRGB.u8G;
		((uint8_t*)pOutput)[RGB24_B] = ptSrcPos->tRGB.u8B;
		break;
	case 4:
		*(uint32_t*)pOutput = ptSrcPos->u32RGB;
		break;
	}
}

void VideoFilter_PutDest(h_VideoFilterMng hMng, void* pOutput, const uint16_t u16X, const uint16_t u16Y, const uint8_t u8OutputBPP) {
	VF_Mng_t* ptMng = (VF_Mng_t*)hMng;
	VF_Dot_t* ptDest;
	VF_Dot_t* ptDestPos;

	if(!hMng  || !u8OutputBPP) {
		return;
	}
	if(u16X >= ptMng->u16Width || u16Y >= ptMng->u16Height) {
		return;
	}

	ptDest = &ptMng->ptBuffer[!ptMng->bBufferMain * ptMng->u16Width * ptMng->u16Height];

	ptDestPos = &ptDest[u16Y * ptMng->u16Width + u16X];
	switch(u8OutputBPP) {
	case 2:
//		*(uint16_t*)pOutput = (ptDestPos->tRGB.u8B >> 3) | ((ptDestPos->tRGB.u8G >> 3) << 5) | ((ptDestPos->tRGB.u8R >> 3) << 11);  // RGB555
		*(uint16_t*)pOutput = (ptDestPos->tRGB.u8B >> 3) | ((ptDestPos->tRGB.u8G >> 2) << 5) | ((ptDestPos->tRGB.u8R >> 3) << 11);  // RGB565
		break;
	case 3:
		((uint8_t*)pOutput)[RGB24_R] = ptDestPos->tRGB.u8R;
		((uint8_t*)pOutput)[RGB24_G] = ptDestPos->tRGB.u8G;
		((uint8_t*)pOutput)[RGB24_B] = ptDestPos->tRGB.u8B;
		break;
	case 4:
		*(uint32_t*)pOutput = ptDestPos->u32RGB;
		break;
	}
}

void VideoFilter_ExportDest(h_VideoFilterMng hMng, void* pOutputBuf, const uint8_t u8OutputBPP, const uint16_t u16YAlign) {
	VF_Mng_t* ptMng = (VF_Mng_t*)hMng;
	uint16_t u16X, u16Y;
	uint8_t* pu8YOutput;

	if(!hMng || !pOutputBuf || !u8OutputBPP || !u16YAlign) {
		return;
	}

	for(u16Y = 0; u16Y < ptMng->u16Height; u16Y++) {
		pu8YOutput = &((uint8_t*)pOutputBuf)[u16Y * u16YAlign];
		for(u16X = 0; u16X < ptMng->u16Width; u16X++) {
			VideoFilter_PutDest(hMng, pu8YOutput, u16X, u16Y, u8OutputBPP);
			pu8YOutput += u8OutputBPP;
		}
	}
}

#endif  // SUPPORT_VIDEOFILTER

