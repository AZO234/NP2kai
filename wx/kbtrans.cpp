/* === keyboard translation for wx port ===
 * Maps wxWidgets key codes to PC-98 scan codes.
 */

#include "kbtrans.h"
#include "np2.h"
#include "kbdmng.h"
#include <keystat.h>
#include <wx/wx.h>

#define NC 0xff

/* PC-98 scan code table indexed by wxKeyCode.
 * Two sub-tables: KEY_KEY106 and KEY_KEY101.
 * Entries are PC-98 make codes. */

typedef struct {
	int    wxkey;   /* wxWidgets key code */
	UINT8  kc106;   /* PC-98 scan code for 106-key */
	UINT8  kc101;   /* PC-98 scan code for 101-key */
} WXKCNV;

/* Common mappings (same for both 106 and 101 key modes) */
static const WXKCNV wxkcnv[] = {
	/* Function keys */
	{ WXK_F1,         0x62, 0x62 },
	{ WXK_F2,         0x63, 0x63 },
	{ WXK_F3,         0x64, 0x64 },
	{ WXK_F4,         0x65, 0x65 },
	{ WXK_F5,         0x66, 0x66 },
	{ WXK_F6,         0x67, 0x67 },
	{ WXK_F7,         0x68, 0x68 },
	{ WXK_F8,         0x69, 0x69 },
	{ WXK_F9,         0x6a, 0x6a },
	{ WXK_F10,        0x6b, 0x6b },
	{ WXK_F11,        0x73, 0x73 }, /* VF1 */
	{ WXK_F12,        0x74, 0x74 }, /* VF2 */
	/* Special keys */
	{ WXK_ESCAPE,     0x00, 0x00 },
	{ WXK_RETURN,     0x1c, 0x1c },
	{ WXK_BACK,       0x0e, 0x0e },
	{ WXK_TAB,        0x0f, 0x0f },
	{ WXK_SPACE,      0x35, 0x35 },
	/* Cursor keys */
	{ WXK_UP,         0x3a, 0x3a },
	{ WXK_DOWN,       0x3d, 0x3d },
	{ WXK_LEFT,       0x3b, 0x3b },
	{ WXK_RIGHT,      0x3c, 0x3c },
	/* Numpad */
	{ WXK_NUMPAD0,    0x30, 0x30 },
	{ WXK_NUMPAD1,    0x31, 0x31 },
	{ WXK_NUMPAD2,    0x32, 0x32 },
	{ WXK_NUMPAD3,    0x33, 0x33 },
	{ WXK_NUMPAD4,    0x34, 0x34 },
	{ WXK_NUMPAD5,    0x35, 0x35 },
	{ WXK_NUMPAD6,    0x36, 0x36 },
	{ WXK_NUMPAD7,    0x37, 0x37 },
	{ WXK_NUMPAD8,    0x38, 0x38 },
	{ WXK_NUMPAD9,    0x39, 0x39 },
	{ WXK_NUMPAD_ENTER, 0x3f, 0x3f },
	{ WXK_NUMPAD_ADD,   0x3e, 0x3e },
	{ WXK_NUMPAD_SUBTRACT, 0x3e, 0x3e },
	{ WXK_NUMPAD_MULTIPLY, 0x3e, 0x3e },
	{ WXK_NUMPAD_DIVIDE,   0x3e, 0x3e },
	{ WXK_NUMPAD_DECIMAL,  0x3a, 0x3a },
	/* Insert/Delete/Home/End/PageUp/PageDown */
	{ WXK_INSERT,     0x3e, 0x3e },
	{ WXK_DELETE,     0x46, 0x46 },
	{ WXK_HOME,       0x3f, 0x3f },
	{ WXK_END,        0x3d, 0x3d },
	{ WXK_PAGEUP,     0x36, 0x36 },
	{ WXK_PAGEDOWN,   0x37, 0x37 },
	/* Modifiers */
	{ WXK_SHIFT,      0x70, 0x70 },
	{ WXK_CONTROL,    0x74, 0x74 },
	{ WXK_ALT,        0x73, 0x73 }, /* GRAPH key */
	{ WXK_CAPITAL,    0x71, 0x71 },
};

/* ASCII key mappings (0x20-0x7e) for 106-key layout */
static const UINT8 ascii_to_pc98_106[0x60] = {
/*       0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F */
/* 20 */ 0x35, NC,   NC,   NC,   NC,   NC,   NC,   0x27, /* ' ( ) ... */
/* 28 */ NC,   NC,   NC,   NC,   NC,   0x0b, NC,   0x0c, /* - . / */
/* 30 */ 0x0a, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
/* 38 */ 0x08, 0x09, 0x26, NC,   NC,   NC,   NC,   NC,
/* 40 */ 0x1a, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23,
/* 48 */ 0x24, 0x17, 0x25, 0x24, 0x25, 0x23, 0x22, 0x18,
/* 50 */ 0x19, 0x10, 0x13, 0x1e, 0x14, 0x16, 0x2a, 0x11,
/* 58 */ 0x2b, 0x15, 0x29, 0x1b, 0x0d, 0x28, 0x0c, NC,
/* 60 */ NC,   0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23,
/* 68 */ 0x24, 0x17, 0x25, 0x24, 0x25, 0x23, 0x22, 0x18,
/* 70 */ 0x19, 0x10, 0x13, 0x1e, 0x14, 0x16, 0x2a, 0x11,
/* 78 */ 0x2b, 0x15, 0x29, NC,   NC,   NC,   NC,   NC,
};

static bool key_states[0x100];

void wxkbd_initialize(void)
{
	memset(key_states, 0, sizeof(key_states));
}

void wxkbd_reset(void)
{
	keystat_allrelease();
	memset(key_states, 0, sizeof(key_states));
}

static UINT8 wxkey_to_pc98(int keycode, int rawcode)
{
	unsigned int i;
	/* check special key table */
	for (i = 0; i < sizeof(wxkcnv) / sizeof(wxkcnv[0]); i++) {
		if (wxkcnv[i].wxkey == keycode) {
			return (np2oscfg.KEYBOARD == KEY_KEY106) ?
			       wxkcnv[i].kc106 : wxkcnv[i].kc101;
		}
	}
	/* ASCII range */
	if (keycode >= 0x20 && keycode < 0x7f) {
		int idx = keycode - 0x20;
		if (idx < (int)(sizeof(ascii_to_pc98_106))) {
			UINT8 kc = ascii_to_pc98_106[idx];
			if (kc != NC) return kc;
		}
	}
	(void)rawcode;
	return NC;
}

void wxkbd_keydown(int keycode, int rawcode)
{
	UINT8 kc = wxkey_to_pc98(keycode, rawcode);
	if (kc != NC && !key_states[kc]) {
		key_states[kc] = true;
		keystat_keydown(kc);
	}
}

void wxkbd_keyup(int keycode, int rawcode)
{
	UINT8 kc = wxkey_to_pc98(keycode, rawcode);
	if (kc != NC && key_states[kc]) {
		key_states[kc] = false;
		keystat_keyup(kc);
	}
}
