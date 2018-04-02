
#if defined(SUPPORT_WAB) && defined(SUPPORT_CL_GD5430)

enum {
	DLGWAB_WIDTH	= 393,
	DLGWAB_HEIGHT	= 235
};

#ifdef __cplusplus
extern "C" {
#endif

int dlgwab_cmd(int msg, MENUID id, long param);

#ifdef __cplusplus
}
#endif

#endif

