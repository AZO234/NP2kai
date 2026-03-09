/**
 *	@file	fongmng.h
 *	@brief	Interface of the font manager
 */

#pragma once

enum
{
	FDAT_BOLD			= 0x01,
	FDAT_PROPORTIONAL	= 0x02,
	FDAT_ALIAS			= 0x04,
	FDAT_ANSI			= 0x08
};

enum
{
	FDAT_DEPTH			= 255,
	FDAT_DEPTHBIT		= 8
};

typedef struct
{
	int width;
	int height;
	int pitch;
} _FNTDAT, *FNTDAT;


#ifdef __cplusplus
extern "C"
{
#endif

void *fontmng_create(int size, UINT type, const char *fontface);
void fontmng_destroy(void *hdl);

BRESULT fontmng_getsize(void *hdl, const char *string, POINT_T *pt);
BRESULT fontmng_getdrawsize(void *hdl, const char *string, POINT_T *pt);
FNTDAT fontmng_get(void *hdl, const char *string);

#ifdef __cplusplus
}
#endif



// ---- for SDL

BRESULT fontmng_init(void);
void fontmng_setdeffontname(const char *name);
