
enum {
	KEY_PC98		= 0,
	KEY_KEY101		= 1,
	KEY_KEY106		= 2,
	KEY_TYPEMAX		= 3,
	KEY_UNKNOWN		= 0xff
};

void winkbd_keydown(WPARAM wParam, LPARAM lParam);
void winkbd_keyup(WPARAM wParam, LPARAM lParam);
void winkbd_roll(BOOL pcat);
void winkbd_setf12(UINT f12key);
void winkbd_resetf12(void);

