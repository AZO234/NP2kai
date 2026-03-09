
enum {
	MVC_BACK,
	MVC_HILIGHT,
	MVC_LIGHT,
	MVC_SHADOW,
	MVC_DARK,
	MVC_SCROLLBAR,

	MVC_STATIC,
	MVC_TEXT,
	MVC_GRAYTEXT1,
	MVC_GRAYTEXT2,
	MVC_BTNFACE,
	MVC_CURTEXT,
	MVC_CURBACK
};

typedef struct {
	int		width;
	int		height;
const UINT8	*data;
const UINT8	*alpha;
} MENURES;

typedef struct {
	int		width;
	int		height;
const UINT8	*pat;
} MENURES2;

#define	MVC2(a, b)			(a | (b << 4))
#define	MVC4(a, b, c, d)	(a | (b << 4) | (c << 8) | (d << 12))

#ifdef __cplusplus
extern "C" {
#endif

extern UINT32 menucolor[];

VRAMHDL menuvram_resload(const MENURES *res, int bpp);
void menuvram_res2put(VRAMHDL vram, const MENURES2 *res, const POINT_T *pt);
void menuvram_res3put(VRAMHDL vram, const MENURES2 *res, const POINT_T *pt,
																UINT mvc);

void menuvram_linex(VRAMHDL vram, int posx, int posy, int term, UINT mvc);
void menuvram_liney(VRAMHDL vram, int posx, int posy, int term, UINT mvc);

void menuvram_box(VRAMHDL vram, const RECT_T *rect, UINT mvc2, int reverse);
void menuvram_box2(VRAMHDL vram, const RECT_T *rect, UINT mvc4);

void menuvram_base(VRAMHDL vram);
VRAMHDL menuvram_create(int width, int height, UINT bpp);
void menuvram_caption(VRAMHDL vram, const RECT_T *rect,
										UINT16 icon, const OEMCHAR *caption);
void menuvram_minimizebtn(VRAMHDL vram, const RECT_T *rect, BOOL focus);
void menuvram_closebtn(VRAMHDL vram, const RECT_T *rect, BOOL focus);

#ifdef __cplusplus
}
#endif

