
#if defined(SIZE_QVGA)
enum {
	DLGSCR_WIDTH	= 280,
	DLGSCR_HEIGHT	= 166
};
#else
enum {
	DLGSCR_WIDTH	= 393,
	DLGSCR_HEIGHT	= 235
};
#endif

#ifdef __cplusplus
extern "C" {
#endif

int dlgscr_cmd(int msg, MENUID id, long param);

#ifdef __cplusplus
}
#endif

