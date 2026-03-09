
enum {
	VRAMALPHA		= 255,
	VRAMALPHABIT	= 8
};


#ifdef __cplusplus
extern "C" {
#endif

void vramcpy_cpy(VRAMHDL dst, const POINT_T *pt,
							const VRAMHDL src, const RECT_T *rct);
void vramcpy_move(VRAMHDL dst, const POINT_T *pt,
							const VRAMHDL src, const RECT_T *rct);
void vramcpy_cpyall(VRAMHDL dst, const POINT_T *pt,
							const VRAMHDL src, const RECT_T *rct);
void vramcpy_cpypat(VRAMHDL dst, const POINT_T *pt,
							const VRAMHDL src, const RECT_T *rct,
							const UINT8 *pat8);
void vramcpy_cpyex(VRAMHDL dst, const POINT_T *pt,
							const VRAMHDL src, const RECT_T *rct);
void vramcpy_cpyexa(VRAMHDL dst, const POINT_T *pt,
							const VRAMHDL src, const RECT_T *rct);
void vramcpy_cpyalpha(VRAMHDL dst, const POINT_T *pt,
							const VRAMHDL src, const RECT_T *rct,
							UINT alpha256);
void vramcpy_mix(VRAMHDL dst, const VRAMHDL org, const POINT_T *pt,
							const VRAMHDL src, const RECT_T *rct,
							UINT alpha64);
void vramcpy_mixcol(VRAMHDL dst, const POINT_T *pt,
							const VRAMHDL src, const RECT_T *rct,
							UINT32 color, UINT alpha64);
void vramcpy_zoom(VRAMHDL dst, const POINT_T *pt,
							const VRAMHDL src, const RECT_T *rct,
							int dot);
void vramcpy_mosaic(VRAMHDL dst, const POINT_T *pt, 
							const VRAMHDL src, const RECT_T *rct,
							int dot);

void vrammix_cpy(VRAMHDL dst, const RECT_T *rct,
							const VRAMHDL src, const POINT_T *pt);
void vrammix_cpyall(VRAMHDL dst, const RECT_T *rct,
							const VRAMHDL src, const POINT_T *pt);
void vrammix_cpy2(VRAMHDL dst, const RECT_T *rct,
							const VRAMHDL src, const POINT_T *pt,
							UINT alpha);
void vrammix_cpypat(VRAMHDL dst, const RECT_T *rct,
							const VRAMHDL src, const POINT_T *pt,
							const UINT8 *pat8);
void vrammix_cpypat16w(VRAMHDL dst, const RECT_T *rct,
							const VRAMHDL src, const POINT_T *pt,
							const UINT pat16);
void vrammix_cpypat16h(VRAMHDL dst, const RECT_T *rct,
							const VRAMHDL src, const POINT_T *pt,
							const UINT pat16);
void vrammix_cpyex(VRAMHDL dst, const RECT_T *rct,
							const VRAMHDL src, const POINT_T *pt);
void vrammix_cpyex2(VRAMHDL dst, const RECT_T *rct,
							const VRAMHDL src, const POINT_T *pt,
							UINT alpha64);
void vrammix_cpyexpat16w(VRAMHDL dst, const RECT_T *rct,
							const VRAMHDL src, const POINT_T *pt,
							const UINT pat16);
void vrammix_cpyexpat16h(VRAMHDL dst, const RECT_T *rct,
							const VRAMHDL src, const POINT_T *pt,
							const UINT pat16);
void vrammix_mix(VRAMHDL dst, const VRAMHDL org, const RECT_T *rct,
							const VRAMHDL src, const POINT_T *pt,
							UINT alpha64);
void vrammix_mixcol(VRAMHDL dst, const RECT_T *rct,
							const VRAMHDL src, const POINT_T *pt,
							UINT32 color, UINT alpha64);
void vrammix_mixalpha(VRAMHDL dst, const RECT_T *rct,
							const VRAMHDL src, const POINT_T *pt,
							UINT32 color);
void vrammix_graybmp(VRAMHDL dst, const VRAMHDL org, const RECT_T *rct,
							const VRAMHDL src, const POINT_T *pt,
							const VRAMHDL bmp, int delta);
void vrammix_colex(VRAMHDL dst, const RECT_T *rct,
							const VRAMHDL src, const POINT_T *pt,
							UINT32 color);

void vrammix_resize(VRAMHDL dst, const RECT_T *drct,
									const VRAMHDL src, const RECT_T *srct);

void vrammix_text(VRAMHDL dst, void *fhdl, const OEMCHAR *str,
							UINT32 color, POINT_T *pt, const RECT_T *rct);
void vrammix_textex(VRAMHDL dst, void *fhdl, const OEMCHAR *str,
							UINT32 color, POINT_T *pt, const RECT_T *rct);

#ifdef __cplusplus
}
#endif

