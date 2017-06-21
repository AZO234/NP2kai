
typedef struct {
	int		x;
	int		y;
} POINT_T;

typedef struct {
	int		left;
	int		top;
	int		right;
	int		bottom;
} RECT_T;

typedef struct {
	int		left;
	int		top;
	int		width;
	int		height;
} SCRN_T;

typedef union {
	POINT_T	p;
	RECT_T	r;
	SCRN_T	s;
} RECT_U;

typedef struct {
	int		type;
	RECT_T	r;
} UNIRECT;


#ifdef __cplusplus
extern "C" {
#endif

BOOL rect_in(const RECT_T *rect, int x, int y);
int rect_num(const RECT_T *rect, int cnt, int x, int y);
BOOL rect_isoverlap(const RECT_T *r1, const RECT_T *r2);
void rect_enumout(const RECT_T *tag, const RECT_T *base,
				void *arg, void (*outcb)(void *arg, const RECT_T *rect));
void rect_add(RECT_T *dst, const RECT_T *src);

void unionrect_rst(UNIRECT *unirct);
void unionrect_add(UNIRECT *unirct, const RECT_T *rct);
const RECT_T *unionrect_get(const UNIRECT *unirct);

#ifdef __cplusplus
}
#endif

