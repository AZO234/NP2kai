/**
 *	@file	fontmng.c
 *	@brief	Implementation of the font manager
 */

#include "compiler.h"
#include "fontmng.h"

#if defined(SIZE_QVGA)
#include "ank10.res"
#else	/* defined(SIZE_QVGA) */
#include "ank12.res"
#endif	/* defined(SIZE_QVGA) */

#if !defined(RESOURCE_US)		/* use TTF */

#if TARGET_OS_IPHONE
#include "SDL_ttf.h"
#else
#include <SDL2/SDL_ttf.h>
#endif

#define FONTMNG_CACHE		64						/*!< Cache count */

/*! White */
static const SDL_Color s_white = {0xff, 0xff, 0xff, 0};

#if defined(FONTMNG_CACHE)
typedef struct
{
	UINT16	str;			/*!< String Id */
	UINT16	next;			/*!< Next index */
} FNTCTBL;
#endif

#endif	/* !defined(RESOURCE_US) */

/*! Font face */
static char s_sFontName[MAX_PATH] = "./default.ttf";

/**
 * @brief Handle
 */
struct TagFontManager
{
	int			fontsize;
	UINT		fonttype;

#if !defined(RESOURCE_US)
	TTF_Font	*ttf_font;
	int			ptsize;
	int			fontalign;
#endif	/* !defined(RESOURCE_US) */

#if defined(FONTMNG_CACHE)
	UINT		caches;
	UINT		cachehead;
	FNTCTBL		cache[FONTMNG_CACHE];
#endif	/* defined(FONTMNG_CACHE) */
};
typedef struct TagFontManager		*FNTMNG;	/*!< Defines handle */

#if defined(FONTMNG_CACHE)
/**
 *
 */
static BOOL fdatgetcache(FNTMNG fhdl, UINT16 c, FNTDAT *pfdat)
{
	BOOL	r;
	FNTCTBL	*fct;
	UINT	pos;
	UINT	prev;
	UINT	cnt;

	r = FALSE;
	fct = fhdl->cache;
	cnt = fhdl->caches;
	pos = fhdl->cachehead;
	prev = FONTMNG_CACHE;
	while (cnt--)
	{
		if (fct[pos].str != c)
		{
			prev = pos;
			pos = fct[pos].next;
			continue;
		}
		if (prev < FONTMNG_CACHE)
		{
			fct[prev].next = fct[pos].next;
			fct[pos].next = (UINT16)fhdl->cachehead;
			fhdl->cachehead = pos;
		}
		r = TRUE;
		break;
	}
	if (r == FALSE)
	{
		if (fhdl->caches < FONTMNG_CACHE)
		{
			pos = fhdl->caches;
			fhdl->caches++;
		}
		else
		{
			pos = prev;
		}
		fct[pos].str = c;
		fct[pos].next = (UINT16)fhdl->cachehead;
		fhdl->cachehead = pos;
	}
	if (pfdat)
	{
		*pfdat = (FNTDAT)(((UINT8 *)(fhdl + 1)) + (pos * fhdl->fontalign));
	}
	return r;
}
#endif	/* defined(FONTMNG_CACHE) */

/**
 * Initialize
 * @retval SUCCESS Succeeded
 * @retval FAILURE Failed
 */
BRESULT fontmng_init(void)
{
#if !defined(RESOURCE_US)

	if (TTF_Init() < 0)
	{
		fprintf(stderr, "Couldn't initialize TTF: %s\n", SDL_GetError());
		return FAILURE;
	}
#ifndef WIN32
	atexit(TTF_Quit);
#endif

#endif	/* !defined(RESOURCE_US) */

	return SUCCESS;
}

/**
 * Sets font face
 * @param[in] name name
 */
void fontmng_setdeffontname(const char *name)
{
	milstr_ncpy(s_sFontName, name, NELEMENTS(s_sFontName));
}

/**
 * Creates instance
 */
void *fontmng_create(int size, UINT type, const char *fontface)
{
	int		fontalign;
	int		fontwork;
	int		allocsize;
	FNTMNG	ret;

#if !defined(RESOURCE_US)
	TTF_Font	*ttf_font;
	int			ptsize;
#endif	/* !defined(RESOURCE_US) */

	if (size < 0)
	{
		size = -size;
	}
	if (size < 6)
	{
		size = 6;
	}
	else if (size > 128)
	{
		size = 128;
	}

#if !defined(RESOURCE_US)

	if (size < 10)
	{
		type |= FDAT_ALIAS;
	}
	else if (size < 16)
	{
		type &= ~FDAT_BOLD;
	}

	ptsize = size;
	if (type & FDAT_ALIAS)
	{
		ptsize *= 2;
	}
	ttf_font = TTF_OpenFont(s_sFontName, ptsize);
	if (ttf_font == NULL)
	{
		fprintf(stderr, "Couldn't load %d points font from %s: %s\n", ptsize, s_sFontName, SDL_GetError());

		if (size < ANKFONTSIZE)
		{
			return NULL;
		}

		ptsize = size;
		type &= FDAT_PROPORTIONAL;
	}

#else	/* !defined(RESOURCE_US) */

	if (size < ANKFONTSIZE)
	{
		return NULL;
	}

#endif	/* !defined(RESOURCE_US) */

	fontalign = sizeof(_FNTDAT) + (size * size);
	fontalign = (fontalign + 3) & (~3);
#if defined(FONTMNG_CACHE)
	fontwork = fontalign * FONTMNG_CACHE;
#else
	fontwork = fontalign;
#endif

	allocsize = sizeof(*ret) + fontwork;
	ret = (FNTMNG)_MALLOC(allocsize, "font mng");
	if (ret == NULL)
	{
#if !defined(RESOURCE_US)
		TTF_CloseFont(ttf_font);
#endif	/* !defined(RESOURCE_US) */
		return NULL;
	}

	memset(ret, 0, allocsize);
	ret->fontsize = size;
	ret->fonttype = type;

#if !defined(RESOURCE_US)
	ret->ttf_font = ttf_font;
	ret->ptsize = ptsize;
	ret->fontalign = fontalign;
#endif	/* !defined(RESOURCE_US) */

	return ret;
}

/**
 * Destroy
 * @param[in] hdl Handle
 */
void fontmng_destroy(void *hdl)
{
	FNTMNG _this;

	_this = (FNTMNG)hdl;
	if (_this)
	{

#if !defined(RESOURCE_US)
		TTF_CloseFont(_this->ttf_font);
#endif	/* !defined(RESOURCE_US) */

		_MFREE(_this);
	}
}

/**
 * Sets font header
 * @param[in] _this Instance
 * @param[out] fdat Data header
 * @param[in] s SDL_Surface
 */
static void AnkSetFontHeader(FNTMNG _this, FNTDAT fdat, int width)
{
	if (_this->fonttype & FDAT_PROPORTIONAL)
	{
		fdat->width = width;
		fdat->pitch = width + 1;
		fdat->height = _this->fontsize;
	}
	else
	{
		fdat->width = max(width, _this->fontsize >> 1);
		fdat->pitch = (_this->fontsize >> 1) + 1;
		fdat->height = _this->fontsize;
	}
}

/**
 * Gets font length
 * @param[in] _this Instance
 * @param[out] fdat Data
 * @param[in] c Charactor
 */
static void AnkGetLength1(FNTMNG _this, FNTDAT fdat, UINT16 c)
{
	c = c - 0x20;
	if ((c < 0) || (c >= 0x60))
	{
		c = 0x1f;							/* '?' */
	}
	AnkSetFontHeader(_this, fdat, ankfont[c * ANKFONTSIZE]);
}

/**
 * Gets font face (TTF)
 * @param[in] _this Instance
 * @param[out] fdat Data
 * @param[in] c Charactor
 */
static void AnkGetFont1(FNTMNG _this, FNTDAT fdat, UINT16 c)
{
const UINT8	*src;
	int		width;
	UINT8	*dst;
	int		x;
	int		y;

	c = c - 0x20;
	if ((c < 0) || (c >= 0x60))
	{
		c = 0x1f;							/* '?' */
	}
	src = ankfont + (c * ANKFONTSIZE);
	width = *src++;
	AnkSetFontHeader(_this, fdat, width);
	dst = (UINT8 *)(fdat + 1);
	memset(dst, 0, fdat->width * fdat->height);
	dst += ((fdat->height - ANKFONTSIZE) / 2) * fdat->width;
	dst += (fdat->width - width) / 2;
	for (y = 0; y < (ANKFONTSIZE - 1); y++)
	{
		dst += fdat->width;
		for (x = 0; x < width; x++)
		{
			dst[x] = (src[0] & (0x80 >> x)) ? 0xff : 0x00;
		}
		src++;
	}
}

#if defined(RESOURCE_US)

#define GetLength1		AnkGetLength1		/*!< length function */
#define GetFont1		AnkGetFont1			/*!< face function */

#else	// defined(RESOURCE_US)

#define GetLength1		TTFGetLength1		/*!< length function */
#define GetFont1		TTFGetFont1			/*!< face function */

/**
 * Sets font header (TTF)
 * @param[in] _this Instance
 * @param[out] fdat Data header
 * @param[in] s SDL_Surface
 */
static void TTFSetFontHeader(FNTMNG _this, FNTDAT fdat, const SDL_Surface *s)
{
	int width;
	int height;
	int pitch;

	if (s)
	{
		width = min(s->w, _this->ptsize);
		height = min(s->h, _this->ptsize);
	}
	else
	{
		width = _this->fontsize;
		height = _this->fontsize;
	}

	pitch = width;
	if (_this->fonttype & FDAT_ALIAS)
	{
		width = (width + 1) >> 1;
		pitch = width >> 1;
		height = (height + 1) >> 1;
	}
	fdat->width = width;
	fdat->pitch = pitch;
	fdat->height = height;
}

/**
 * Get pixel
 * @param[in] s SDL_Surface
 * @param[in] x x
 * @param[in] y y
 * @return pixel
 */
static UINT8 TTFGetPixelDepth(const SDL_Surface *s, int x, int y)
{
	int nXAlign;
	const UINT8 *ptr;

	if ((x >= 0) && (x < s->w) && (y >= 0) && (y < s->h))
	{
		nXAlign = s->format->BytesPerPixel;
		ptr = (UINT8 *)s->pixels + (y * s->pitch) + (x * nXAlign);
		switch (nXAlign)
		{
			case 1:
				return (ptr[0] != 0) ? FDAT_DEPTH : 0;

			case 3:
			case 4:
				return (ptr[0] * FDAT_DEPTH / 255);
		}
	}
	return 0;
}

/**
 * Gets font length (TTF)
 * @param[in] _this Instance
 * @param[out] fdat Data
 * @param[in] c Charactor
 */
static void TTFGetLength1(FNTMNG _this, FNTDAT fdat, UINT16 c)
{
	UINT16 sString[2];
	SDL_Surface *s;

	sString[0] = c;
	sString[1] = 0;
	s = NULL;
	if (_this->ttf_font)
	{
		s = TTF_RenderUNICODE_Solid(_this->ttf_font, sString, s_white);
	}
	if (s)
	{
		TTFSetFontHeader(_this, fdat, s);
		SDL_FreeSurface(s);
	}
	else
	{
		AnkGetLength1(_this, fdat, c);
	}
}

/**
 * Gets font face (TTF)
 * @param[in] _this Instance
 * @param[out] fdat Data
 * @param[in] c Charactor
 */
static void TTFGetFont1(FNTMNG _this, FNTDAT fdat, UINT16 c)
{
	UINT16		sString[2];
	SDL_Surface	*s;
	UINT8		*dst;
	int			x;
	int			y;
	int			depth;

	sString[0] = c;
	sString[1] = 0;
	s = NULL;
	if (_this->ttf_font)
	{
		s = TTF_RenderUNICODE_Solid(_this->ttf_font, sString, s_white);
	}
	if (s)
	{
		TTFSetFontHeader(_this, fdat, s);
		dst = (UINT8 *)(fdat + 1);
		if (_this->fonttype & FDAT_ALIAS)
		{
			for (y = 0; y < fdat->height; y++)
			{
				for (x = 0; x < fdat->width; x++)
				{
					depth = TTFGetPixelDepth(s, x*2+0, y*2+0);
					depth += TTFGetPixelDepth(s, x*2+1, y*2+0);
					depth += TTFGetPixelDepth(s, x*2+0, y*2+1);
					depth += TTFGetPixelDepth(s, x*2+1, y*2+1);
					*dst++ = (UINT8)((depth + 2) / 4);
				}
			}
		}
		else
		{
			for (y = 0; y < fdat->height; y++)
			{
				for (x = 0; x < fdat->width; x++)
				{
					*dst++ = TTFGetPixelDepth(s, x, y);
				}
			}
		}
		SDL_FreeSurface(s);
	}
	else
	{
		AnkGetFont1(_this, fdat, c);
	}
}

#endif	/* defined(RESOURCE_US) */

/**
 * Get charactor
 * @param[in,out] lppString Pointer
 * @return Charactor
 */
static UINT16 GetChar(const char** lppString)
{
	const char *lpString;
	UINT16 c;

	lpString = *lppString;
	if (lpString == NULL)
	{
		return 0;
	}

	c = 0;
	if ((lpString[0] & 0x80) == 0)
	{
		c = lpString[0] & 0x7f;
		lpString++;
	}
	else if (((lpString[0] & 0xe0) == 0xc0) && ((lpString[1] & 0xc0) == 0x80))
	{
		c = ((lpString[0] & 0x1f) << 6) | (lpString[1] & 0x3f);
		lpString += 2;
	}
	else if (((lpString[0] & 0xf0) == 0xe0) && ((lpString[1] & 0xc0) == 0x80) && ((lpString[2] & 0xc0) == 0x80))
	{
		c = ((lpString[0] & 0x0f) << 12) | ((lpString[1] & 0x3f) << 6) | (lpString[2] & 0x3f);
		lpString += 3;
	}

	*lppString = lpString;
	return c;
}

/**
 * Get font size
 * @param[in] hdl Handle
 * @param[in] lpString String
 * @param[out] pt Size
 * @retval SUCCESS Succeeded
 * @retval FAILURE Failed
 */
BRESULT fontmng_getsize(void *hdl, const char *lpString, POINT_T *pt)
{
	FNTMNG _this;
	int nWidth;
	UINT16 c;
	_FNTDAT fontData;

	_this = (FNTMNG)hdl;
	if ((_this == NULL) || (lpString == NULL))
	{
		return FAILURE;
	}

	nWidth = 0;
	while (1 /* EVER */)
	{
		c = GetChar(&lpString);
		if (c == 0)
		{
			break;
		}
		GetLength1(_this, &fontData, c);
		nWidth += fontData.pitch;
	}

	if (pt)
	{
		pt->x = nWidth;
		pt->y = _this->fontsize;
	}
	return SUCCESS;
}

/**
 * Get draw area
 * @param[in] hdl Handle
 * @param[in] lpString String
 * @param[out] pt An area
 * @retval SUCCESS Succeeded
 * @retval FAILURE Failed
 */
BRESULT fontmng_getdrawsize(void *hdl, const char *lpString, POINT_T *pt)
{
	FNTMNG _this;
	int nWidth;
	int nPosX;
	UINT16 c;
	_FNTDAT fontData;

	_this = (FNTMNG)hdl;
	if (_this == NULL)
	{
		return FAILURE;
	}

	nWidth = 0;
	nPosX = 0;
	while (1 /* EVER */)
	{
		c = GetChar(&lpString);
		if (c == 0)
		{
			break;
		}
		GetLength1(_this, &fontData, c);
		nWidth = nPosX + max(fontData.width, fontData.pitch);
		nPosX += fontData.pitch;
	}
	if (pt)
	{
		pt->x = nWidth;
		pt->y = _this->fontsize;
	}
	return SUCCESS;
}

/**
 * Get font data
 * @param[in] hdl Handle
 * @param[in] lpString String
 * @return Data
 */
FNTDAT fontmng_get(void *hdl, const char *lpString)
{
	FNTMNG _this;
	UINT16 c;
	FNTDAT fontData;

	_this = (FNTMNG)hdl;
	if (_this  == NULL)
	{
		return NULL;
	}

	c = GetChar(&lpString);
	if (c == 0)
	{
		return NULL;
	}

#if defined(FONTMNG_CACHE)
	if (fdatgetcache(_this, c, &fontData))
	{
		return fontData;
	}
#else	/*! defined(FONTMNG_CACHE) */
	fontData = (FNTDAT)(_this + 1);
#endif	/*! defined(FONTMNG_CACHE) */

	GetFont1(_this, fontData, c);
	return fontData;
}
