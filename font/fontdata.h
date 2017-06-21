
enum {
	FONT_ANK8		= 0x01,
	FONT_ANK16a		= 0x02,
	FONT_ANK16b		= 0x04,
	FONT_KNJ1		= 0x08,
	FONT_KNJ2		= 0x10,
	FONT_KNJ3		= 0x20,

	FONTLOAD_KNJ	= (FONT_KNJ1 | FONT_KNJ2 | FONT_KNJ3),
	FONTLOAD_ANK	= (FONT_ANK16a | FONT_ANK16b),
	FONTLOAD_16		= (FONTLOAD_ANK | FONTLOAD_KNJ),
	FONTLOAD_ALL	= (FONT_ANK8 | FONTLOAD_16)
};

enum {
	FONTTYPE_NONE	= 0,
	FONTTYPE_PC98,
	FONTTYPE_V98,
	FONTTYPE_PC88,
	FONTTYPE_FM7,
	FONTTYPE_X1,
	FONTTYPE_X68
};


#ifdef __cplusplus
extern "C" {
#endif

extern const OEMCHAR pc88ankname[];
extern const OEMCHAR pc88knj1name[];
extern const OEMCHAR pc88knj2name[];
extern const OEMCHAR pc98fontname[];
extern const OEMCHAR v98fontname[];
extern const OEMCHAR fm7ankname[];
extern const OEMCHAR fm7knjname[];
extern const OEMCHAR x1ank1name[];
extern const OEMCHAR x1ank2name[];
extern const OEMCHAR x1knjname[];
extern const OEMCHAR x68kfontname[];

extern const UINT8 fontdata_8[256*8];
extern const UINT8 fontdata_16[3*32*16];
extern const UINT8 fontdata_29[94*16];
extern const UINT8 fontdata_2a[94*16];
extern const UINT8 fontdata_2b[94*16];
extern const UINT8 fontdata_2c[76*16*2];


void fontdata_ank8store(const UINT8 *ptr, UINT pos, UINT cnt);
void fontdata_patch16a(void);
void fontdata_patch16b(void);
void fontdata_patchjis(void);

UINT8 fontpc88_read(const OEMCHAR *filename, UINT8 loading);
UINT8 fontpc98_read(const OEMCHAR *filename, UINT8 loading);
UINT8 fontv98_read(const OEMCHAR *filename, UINT8 loading);
UINT8 fontfm7_read(const OEMCHAR *filename, UINT8 loading);
UINT8 fontx1_read(const OEMCHAR *filename, UINT8 loading);
UINT8 fontx68k_read(const OEMCHAR *filename, UINT8 loading);

#ifdef __cplusplus
}
#endif

