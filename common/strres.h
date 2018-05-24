
#ifdef __cplusplus
extern "C" {
#endif

extern const UINT8 str_utf8[3];
extern const UINT16 str_ucs2[1];

extern const OEMCHAR str_null[];
extern const OEMCHAR str_space[];
extern const OEMCHAR str_dot[];

extern const OEMCHAR str_cr[];
extern const OEMCHAR str_crlf[];
#define	str_lf	(str_crlf + 1)

#if defined(OSLINEBREAK_CR)
#define	str_oscr	str_cr
#elif defined(OSLINEBREAK_CRLF)
#define	str_oscr	str_crlf
#else
#define	str_oscr	str_lf
#endif

extern const OEMCHAR str_ini[];
extern const OEMCHAR str_cfg[];
extern const OEMCHAR str_sav[];
extern const OEMCHAR str_bmp[];
extern const OEMCHAR str_bmp_b[];
extern const OEMCHAR str_d88[];
extern const OEMCHAR str_d98[];
extern const OEMCHAR str_88d[];
extern const OEMCHAR str_98d[];
extern const OEMCHAR str_thd[];
extern const OEMCHAR str_hdi[];
extern const OEMCHAR str_fdi[];
extern const OEMCHAR str_hdd[];
extern const OEMCHAR str_nhd[];
extern const OEMCHAR str_vhd[];
extern const OEMCHAR str_slh[];
extern const OEMCHAR str_hdn[];
extern const OEMCHAR str_hdm[];
extern const OEMCHAR str_hd4[];

extern const OEMCHAR str_d[];
extern const OEMCHAR str_u[];
extern const OEMCHAR str_x[];
extern const OEMCHAR str_2d[];
extern const OEMCHAR str_2x[];
extern const OEMCHAR str_4x[];
extern const OEMCHAR str_4X[];

extern const OEMCHAR str_false[];
extern const OEMCHAR str_true[];

extern const OEMCHAR str_posx[];
extern const OEMCHAR str_posy[];
extern const OEMCHAR str_width[];
extern const OEMCHAR str_height[];

extern const OEMCHAR str_np2[];
extern const OEMCHAR str_resume[];

extern const OEMCHAR str_VM[];
extern const OEMCHAR str_VX[];
extern const OEMCHAR str_EPSON[];

extern const OEMCHAR str_biosrom[];
extern const OEMCHAR str_sasirom[];
extern const OEMCHAR str_scsirom[];

#ifdef __cplusplus
}
#endif

