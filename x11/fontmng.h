#ifndef	NP2_X11_FONTMNG_H__
#define	NP2_X11_FONTMNG_H__

G_BEGIN_DECLS

enum {
	FDAT_BOLD		= 0x01,
	FDAT_PROPORTIONAL	= 0x02,
	FDAT_ALIAS		= 0x04,
	FDAT_ANSI		= 0x08
};

enum {
	FDAT_DEPTH		= 255,
	FDAT_DEPTHBIT		= 8
};

typedef struct {
	int	width;
	int	height;
	int	pitch;
} _FNTDAT, *FNTDAT;

BRESULT fontmng_init(void);
void fontmng_terminate(void);
void fontmng_setdeffontname(const OEMCHAR *fontface);
void* fontmng_create(int size, UINT type, const OEMCHAR *fontface);
void fontmng_destroy(void *hdl);

BRESULT fontmng_getsize(void *hdl, const char *string, POINT_T *pt);
BRESULT fontmng_getdrawsize(void *hdl, const char *string, POINT_T *pt);
FNTDAT fontmng_get(void *hdl, const char *string);

G_END_DECLS

#endif	/* NP2_X11_FONTMNG_H__ */
