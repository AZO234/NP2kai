
#if defined(SUPPORT_SOFTKBD)

#include	"cmndraw.h"

enum {
	LEDFLAG_NUM		= 0x01,
	LEDFLAG_CAPS	= 0x04,
	LEDFLAG_KANA	= 0x08
};

enum {
	SOFTKEY_FLAGDRAW		= 0x01,
	SOFTKEY_FLAGREDRAW		= 0x02
};


#ifdef __cplusplus
extern "C" {
#endif

void softkbd_initialize(void);
void softkbd_deinitialize(void);
BRESULT softkbd_getsize(int *width, int *height);
REG8 softkbd_process(void);
BOOL softkbd_paint(CMNVRAM *vram, CMNPALCNV cnv, BOOL redraw);
BOOL softkbd_down(int x, int y);
void softkbd_up(void);
void softkbd_led(REG8 led);

#ifdef __cplusplus
}
#endif

#endif

