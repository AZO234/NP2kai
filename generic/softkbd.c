#include	"compiler.h"

#if defined(SUPPORT_SOFTKBD)

#include	"bmpdata.h"
#include	"softkbd.h"
#include	"keystat.h"


#define	SOFTKEY_MENU	0xfe
#define	SOFTKEY_NC		0xff


typedef struct {
	UINT8	key;
	UINT8	key2;
	UINT8	led;
	UINT8	flag;
	void	*ptr;
	CMNBMP	bmp;
	CMNPAL	pal[16];
} SOFTKBD;

static	SOFTKBD	softkbd;

#if (SUPPORT_SOFTKBD == 1)
#if defined(SIZE_QVGA)
#include	"softkbd1.res"
#else
#include	"softkbd2.res"
#endif
#elif (SUPPORT_SOFTKBD == 2)
#error not support (SUPPORT_SOFTKBD == 2)
#else
#if !defined(SUPPORT_PC9801_119)
#include	"softkbd.res"
#else
#include	"softkbd3.res"
#endif
#endif


static void loadbmp(const char *filename) {

	void	*ptr;

	softkbd.ptr = NULL;
	ptr = (void *)bmpdata_solvedata(np2kbd_bmp);
	if (ptr != NULL) {
		if (cmndraw_bmp4inf(&softkbd.bmp, ptr) == SUCCESS) {
			softkbd.ptr = ptr;
		}
		else {
			_MFREE(ptr);
		}
	}
	softkbd.flag |= SOFTKEY_FLAGREDRAW;
	(void)filename;
}

void softkbd_initialize(void) {

	softkbd.key = SOFTKEY_NC;
	softkbd.led = 0;
	loadbmp(NULL);
}

void softkbd_deinitialize(void) {

	void	*ptr;

	ptr = softkbd.ptr;
	softkbd.ptr = NULL;
	if (ptr) {
		_MFREE(ptr);
	}
}

BRESULT softkbd_getsize(int *width, int *height) {

	if (softkbd.ptr == NULL) {
		return(FAILURE);
	}
	if (width) {
		*width = softkbd.bmp.width;
	}
	if (height) {
		*height = softkbd.bmp.height;
	}
	return(SUCCESS);
}

REG8 softkbd_process(void) {

	return(softkbd.flag);
}

BOOL softkbd_paint(CMNVRAM *vram, CMNPALCNV cnv, BOOL redraw) {

	UINT8	flag;
	BOOL	ret;

	flag = softkbd.flag;
	softkbd.flag = 0;
	if (redraw) {
		flag = SOFTKEY_FLAGREDRAW | SOFTKEY_FLAGDRAW;
	}
	ret = FALSE;
	if ((flag & SOFTKEY_FLAGREDRAW) && (vram) && (cnv)) {
		(*cnv)(softkbd.pal, softkbd.bmp.paltbl, softkbd.bmp.pals, vram->bpp);
		cmndraw_bmp16(vram, softkbd.ptr, cnv, CMNBMP_LEFT | CMNBMP_TOP);
		ret = TRUE;
	}
	if (flag & SOFTKEY_FLAGDRAW) {
		TRACEOUT(("softkbd_paint"));
		ledpaint(vram);
		ret = TRUE;
	}
	return(ret);
}

BOOL softkbd_down(int x, int y) {

	UINT8	key;

	softkbd_up();
	key = getsoftkbd(x, y);
	if (key == SOFTKEY_MENU) {
		return(TRUE);
	}
	else if (key != SOFTKEY_NC) {
		keystat_down(&key, 1, NKEYREF_SOFTKBD);
		softkbd.key = key;
	}
	return(FALSE);
}

void softkbd_up(void) {

	if (softkbd.key != SOFTKEY_NC) {
		keystat_up(&softkbd.key, 1, NKEYREF_SOFTKBD);
		softkbd.key = SOFTKEY_NC;
	}
}

void softkbd_led(REG8 led) {

	TRACEOUT(("softkbd_led(%x)", led));
	if (softkbd.led != led) {
		softkbd.led = led;
		softkbd.flag |= SOFTKEY_FLAGDRAW;
	}
}
#endif

