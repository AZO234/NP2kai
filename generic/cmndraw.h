
#ifndef __CMNDRAW
#define __CMNDRAW

typedef union {
	RGB32	pal32;
	UINT16	pal16;
	UINT8	pal8;
} CMNPAL;

typedef struct {
	UINT8	*ptr;
	int		width;
	int		height;
	int		xalign;
	int		yalign;
	int		bpp;
} CMNVRAM;

typedef void (*CMNPALCNV)(CMNPAL *dst, const RGB32 *src, UINT pals, UINT bpp);

#ifdef __cplusplus
extern "C" {
#endif

void cmndraw_makegrad(RGB32 *pal, int pals, RGB32 bg, RGB32 fg);

void cmndraw_fill(const CMNVRAM *vram, int x, int y,
										int cx, int cy, CMNPAL fg);
void cmndraw_setfg(const CMNVRAM *vram, const UINT8 *src,
										int x, int y, CMNPAL fg);
void cmndraw_setpat(const CMNVRAM *vram, const UINT8 *src,
										int x, int y, CMNPAL bg, CMNPAL fg);
void cmddraw_text8(CMNVRAM *vram, int x, int y, const char *str, CMNPAL fg);


// ----

enum {
	CMNBMP_LEFT		= 0x00,
	CMNBMP_CENTER	= 0x01,
	CMNBMP_RIGHT	= 0x02,
	CMNBMP_TOP		= 0x00,
	CMNBMP_MIDDLE	= 0x04,
	CMNBMP_BOTTOM	= 0x08
};

typedef struct {
	UINT8	*ptr;
	int		width;
	int		height;
	int		align;
	UINT	pals;
	RGB32	paltbl[16];
} CMNBMP;

BRESULT cmndraw_bmp4inf(CMNBMP *bmp, const void *ptr);
void cmndraw_bmp16(CMNVRAM *vram, const void *ptr, CMNPALCNV cnv, UINT flag);

#ifdef __cplusplus
}
#endif

#endif

