
#define	DEFAULT_BPP		0

#define MAKEPALETTE(r, g, b)	(((r) << 16) | ((g) << 8) | (b))


#ifdef SUPPORT_16BPP
enum {
	B16MASK		= 0x001f,
	G16MASK		= 0x07e0,
	R16MASK		= 0xf800
};

#define	MAKE16PAL(c24)		((((c24) & 0x0000f8) >> 3) |					\
							(((c24) & 0x00fc00) >> 5) |						\
							(((c24) & 0xf80000) >> 8))

#define	MAKE24B(c16)		((((c16) << 3) & 0xf8) | (((c16) >> 2) & 0x07))
#define	MAKE24G(c16)		((((c16) >> 3) & 0xfc) | (((c16) >> 9) & 0x03))
#define	MAKE24R(c16)		((((c16) >> 8) & 0xf8) | (((c16) >> 13) & 0x07))

#define MAKEALPHA16(d, s, m, a, b)	((((d) & (m)) +							\
									((((int)((s) & (m)) - (int)((d) & (m)))	\
									* (a)) >> (b))) & (m))

#define MAKEALPHA16s(d, s, m, a, b)	(((d) +									\
									((((int)((s) & (m)) - (int)(d))			\
									* (a)) >> (b))) & (m))
#endif

#ifdef SUPPORT_24BPP
#define MAKEALPHA24(d, s, a, b)		((d) +									\
									((((int)(s) - (int)(d)) * (a)) >> (b)))
#endif


typedef struct {
	int		srcpos;
	int		dstpos;
	int		width;
	int		height;
} MIX_RECT;

typedef struct {
	int		width;
	int		height;
	int		xalign;
	int		yalign;
	int		posx;
	int		posy;
	int		bpp;
	int		scrnsize;
	UINT8	*ptr;
	UINT8	*alpha;
} _VRAMHDL, *VRAMHDL;


#ifdef __cplusplus
extern "C" {
#endif

VRAMHDL vram_create(int width, int height, BOOL alpha, int bpp);
void vram_destroy(VRAMHDL hdl);
BRESULT vram_allocalpha(VRAMHDL hdl);
void vram_zerofill(VRAMHDL hdl, const RECT_T *rect);
void vram_fill(VRAMHDL hdl, const RECT_T *rect, UINT32 color, UINT8 alpha);
void vram_filldat(VRAMHDL hdl, const RECT_T *rect, UINT32 color);
void vram_fillalpha(VRAMHDL hdl, const RECT_T *rect, UINT8 alpha);
void vram_fillex(VRAMHDL hdl, const RECT_T *rect, UINT32 color, UINT8 alpha);

VRAMHDL vram_resize(VRAMHDL base, int width, int height, int bpp);

void vram_getrect(const VRAMHDL hdl, RECT_T *rect);

VRAMHDL vram_dupe(const VRAMHDL hdl);
BRESULT vram_cliprect(RECT_T *clip, const VRAMHDL vram, const RECT_T *rct);
BRESULT vram_cliprectex(RECT_T *clip, const VRAMHDL vram, const RECT_T *rct);

#ifdef __cplusplus
}
#endif


// ---- macros

#define	VRAM_RELEASE(a) { VRAMHDL v; v = (a); (a) = NULL; vram_destroy(v); }

