
#if defined(SIZE_QVGA)
enum {
	DLGCFG_WIDTH	= 254,
	DLGCFG_HEIGHT	= 180
};
#else
enum {
	DLGCFG_WIDTH	= 303,
	DLGCFG_HEIGHT	= 214
};
#endif


#ifdef __cplusplus
extern "C" {
#endif

int dlgcfg_cmd(int msg, MENUID id, long param);

#ifdef __cplusplus
}
#endif

