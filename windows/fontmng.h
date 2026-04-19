
enum {
	FDAT_BOLD			= 0x01,
	FDAT_PROPORTIONAL	= 0x02,
	FDAT_ALIAS			= 0x04,
	FDAT_ANSI			= 0x08,
	FDAT_SHIFTJIS		= 0x10
};

enum {
	FDAT_DEPTH			= 64,
	FDAT_DEPTHBIT		= 6
};

typedef struct {
	int		width;
	int		height;
	int		pitch;
} _FNTDAT, *FNTDAT;

#ifdef __cplusplus
extern "C" {
#endif

void *fontmng_create(int size, UINT type, const OEMCHAR *fontface);
void fontmng_destroy(void *hdl);

BRESULT fontmng_getsize(void *hdl, const OEMCHAR *string, POINT_T *pt);
BRESULT fontmng_getdrawsize(void *hdl, const OEMCHAR *string, POINT_T *pt);
FNTDAT fontmng_get(void *hdl, const OEMCHAR *string);

#ifdef __cplusplus
}
#endif

