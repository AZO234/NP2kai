
enum {
	MENUTYPE_NORMAL	= 0
};


#ifdef __cplusplus
extern "C" {
#endif

BRESULT sysmenu_create(void);
void sysmenu_destroy(void);

BRESULT sysmenu_menuopen(UINT menutype, int x, int y);

BOOL scrnmng_fullscreen(BOOL val);


#ifdef __cplusplus
}
#endif

