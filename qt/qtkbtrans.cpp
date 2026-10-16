/* === keyboard translation for the Qt port ===
 * Implements the wxkbd_* functions declared in wx/kbtrans.h, which the shared
 * np2.cpp calls. Keys are taken by position, not by symbol: under both xcb and
 * Wayland, QKeyEvent::nativeScanCode() is the XKB keycode, i.e. the Linux evdev
 * code plus 8, so the host keyboard layout doesn't matter. The tables follow
 * the SDL port's (sdl/kbtrans.c), which has been tested on real keyboards.
 *
 * A 101-key keyboard has US legends, which the PC-98's JIS keys only partly
 * share: ' is Shift+7 there, : has a key of its own, _ is Shift on the key
 * a US keyboard lacks. Those keys type what their legend says, pressing or
 * releasing the PC-98's Shift around them as needed (bios/keytable.res).
 */

#include <compiler.h>
#include "kbtrans.h"
#include "kbdmng.h"
#include "np2.h"
#include <keystat.h>

#include <cstring>

#if defined(__linux__)
#include <linux/input-event-codes.h>
#endif

#define NC 0xff

namespace {

struct KeyMap {
	UINT16 evdev;
	UINT8  pc98;
};

#if defined(__linux__)

/* CAPS and KANA latch on the PC-98 keyboard. The core's key table sends its
 * entries 0x81 and 0x82 as these locking keys (keystat.c, 0xf1 and 0xf2), so
 * each press toggles them as Caps Lock does on the host; the Windows port gets
 * there through keystat_senddata(). */

/* Keys that are the same on both keyboard types */
const KeyMap kCommon[] = {
	{ KEY_PAUSE,      0x60 },  /* STOP */
	{ KEY_SYSRQ,      0x61 },  /* COPY */
	{ KEY_F1,         0x62 },
	{ KEY_F2,         0x63 },
	{ KEY_F3,         0x64 },
	{ KEY_F4,         0x65 },
	{ KEY_F5,         0x66 },
	{ KEY_F6,         0x67 },
	{ KEY_F7,         0x68 },
	{ KEY_F8,         0x69 },
	{ KEY_F9,         0x6a },
	{ KEY_F10,        0x6b },
	{ KEY_ESC,        0x00 },
	{ KEY_1,          0x01 },
	{ KEY_2,          0x02 },
	{ KEY_3,          0x03 },
	{ KEY_4,          0x04 },
	{ KEY_5,          0x05 },
	{ KEY_6,          0x06 },
	{ KEY_7,          0x07 },
	{ KEY_8,          0x08 },
	{ KEY_9,          0x09 },
	{ KEY_0,          0x0a },
	{ KEY_MINUS,      0x0b },  /* - = */
	{ KEY_EQUAL,      0x0c },  /* ^ ` */
	{ KEY_BACKSPACE,  0x0e },
	{ KEY_TAB,        0x0f },
	{ KEY_Q,          0x10 },
	{ KEY_W,          0x11 },
	{ KEY_E,          0x12 },
	{ KEY_R,          0x13 },
	{ KEY_T,          0x14 },
	{ KEY_Y,          0x15 },
	{ KEY_U,          0x16 },
	{ KEY_I,          0x17 },
	{ KEY_O,          0x18 },
	{ KEY_P,          0x19 },
	{ KEY_LEFTBRACE,  0x1a },  /* @ ~ */
	{ KEY_RIGHTBRACE, 0x1b },  /* [ { */
	{ KEY_ENTER,      0x1c },
	{ KEY_LEFTCTRL,   0x74 },  /* CTRL */
	{ KEY_CAPSLOCK,   0x81 },  /* CAPS, locking (see below) */
	{ KEY_A,          0x1d },
	{ KEY_S,          0x1e },
	{ KEY_D,          0x1f },
	{ KEY_F,          0x20 },
	{ KEY_G,          0x21 },
	{ KEY_H,          0x22 },
	{ KEY_J,          0x23 },
	{ KEY_K,          0x24 },
	{ KEY_L,          0x25 },
	{ KEY_SEMICOLON,  0x26 },  /* ; + */
	{ KEY_APOSTROPHE, 0x27 },  /* : * */
	{ KEY_BACKSLASH,  0x28 },  /* ] } */
	{ KEY_LEFTSHIFT,  0x70 },
	{ KEY_Z,          0x29 },
	{ KEY_X,          0x2a },
	{ KEY_C,          0x2b },
	{ KEY_V,          0x2c },
	{ KEY_B,          0x2d },
	{ KEY_N,          0x2e },
	{ KEY_M,          0x2f },
	{ KEY_COMMA,      0x30 },
	{ KEY_DOT,        0x31 },
	{ KEY_SLASH,      0x32 },
	{ KEY_RIGHTSHIFT, 0x7d },
	{ KEY_LEFTMETA,   0x77 },
	{ KEY_RIGHTCTRL,  0x73 },  /* GRPH */
	{ KEY_LEFTALT,    0x51 },  /* NFER */
	{ KEY_SPACE,      0x34 },
	{ KEY_RIGHTALT,   0x35 },  /* XFER */
	{ KEY_RIGHTMETA,  0x78 },
	{ KEY_COMPOSE,    0x79 },  /* Menu */
	{ KEY_INSERT,     0x38 },
	{ KEY_DELETE,     0x39 },
	{ KEY_PAGEUP,     0x36 },  /* ROLL UP */
	{ KEY_PAGEDOWN,   0x37 },  /* ROLL DOWN */
	{ KEY_UP,         0x3a },
	{ KEY_LEFT,       0x3b },
	{ KEY_RIGHT,      0x3c },
	{ KEY_DOWN,       0x3d },
	{ KEY_HOME,       0x3e },  /* HOME/CLR */
	{ KEY_END,        0x3f },  /* HELP */
	{ KEY_KPMINUS,    0x40 },
	{ KEY_KPSLASH,    0x41 },
	{ KEY_KP7,        0x42 },
	{ KEY_KP8,        0x43 },
	{ KEY_KP9,        0x44 },
	{ KEY_KPASTERISK, 0x45 },
	{ KEY_KP4,        0x46 },
	{ KEY_KP5,        0x47 },
	{ KEY_KP6,        0x48 },
	{ KEY_KPPLUS,     0x49 },
	{ KEY_KP1,        0x4a },
	{ KEY_KP2,        0x4b },
	{ KEY_KP3,        0x4c },
	{ KEY_KPEQUAL,    0x4d },
	{ KEY_KP0,        0x4e },
	{ KEY_KPDOT,      0x50 },
	{ KEY_KPENTER,    0x1c },
};

/* 101-key: a US legend as a PC-98 key, with or without Shift */
struct UsKey {
	UINT16 evdev;
	UINT8  plain;          /* the key alone */
	bool   plainShift;
	UINT8  shifted;        /* the key with Shift */
	bool   shiftedShift;
};

const UsKey kUs[] = {
	{ KEY_GRAVE,      0x0c, true,  0x1a, true  },  /* ` ~ */
	{ KEY_2,          0x02, false, 0x1a, false },  /* 2 @ */
	{ KEY_6,          0x06, false, 0x0c, false },  /* 6 ^ */
	{ KEY_7,          0x07, false, 0x06, true  },  /* 7 & */
	{ KEY_8,          0x08, false, 0x27, true  },  /* 8 * */
	{ KEY_9,          0x09, false, 0x08, true  },  /* 9 ( */
	{ KEY_0,          0x0a, false, 0x09, true  },  /* 0 ) */
	{ KEY_MINUS,      0x0b, false, 0x33, true  },  /* - _ */
	{ KEY_EQUAL,      0x0b, true,  0x26, true  },  /* = + */
	{ KEY_LEFTBRACE,  0x1b, false, 0x1b, true  },  /* [ { */
	{ KEY_RIGHTBRACE, 0x28, false, 0x28, true  },  /* ] } */
	{ KEY_BACKSLASH,  0x0d, false, 0x0d, true  },  /* \ | (yen) */
	{ KEY_102ND,      0x0d, false, 0x0d, true  },  /* \ | on ISO keyboards */
	{ KEY_SEMICOLON,  0x26, false, 0x27, false },  /* ; : */
	{ KEY_APOSTROPHE, 0x07, true,  0x02, true  },  /* ' " */
};

/* 106-key: the Japanese keys exist and go where they belong */
const KeyMap k106[] = {
	{ KEY_YEN,              0x0d },  /* Yen | */
	{ KEY_RO,               0x33 },  /* _ */
	{ KEY_KPCOMMA,          0x4f },
	{ KEY_KPJPCOMMA,        0x4f },
	{ KEY_KATAKANAHIRAGANA, 0x82 },  /* KANA, locking */
	{ KEY_HENKAN,           0x35 },  /* XFER */
	{ KEY_MUHENKAN,         0x51 },  /* NFER */
};

const int kMaxEvdev = KEY_MAX + 1;

#else

/* Without evdev codes there is nothing to map from yet. */
const KeyMap kCommon[] = { { 0, NC } };
const UsKey  kUs[]     = { { 0, NC, false, NC, false } };
const KeyMap k106[]    = { { 0, NC } };
const int kMaxEvdev = 1;

#endif

/* PC-98 code sent for each held host key, so the release matches the press
 * even if the keyboard type changes in between. */
UINT8 s_held[kMaxEvdev];

/* What a US key did to the PC-98's Shift, undone on its release */
enum : INT8 { SHIFT_KEPT = 0, SHIFT_PRESSED = 1, SHIFT_RELEASED = -1 };
INT8 s_shiftFix[kMaxEvdev];

/* The host's Shift keys, each of which holds the PC-98's under its own code */
bool s_leftShift;
bool s_rightShift;

void releaseHostShift()
{
	if (s_leftShift) {
		keystat_keyup(0x70);
	}
	if (s_rightShift) {
		keystat_keyup(0x7d);
	}
}

void pressHostShift()
{
	if (s_leftShift) {
		keystat_keydown(0x70);
	}
	if (s_rightShift) {
		keystat_keydown(0x7d);
	}
}

const UsKey *usKey(int evdev)
{
	if (np2oscfg.KEYBOARD != KEY_KEY101) {
		return nullptr;
	}
	for (const UsKey &k : kUs) {
		if (k.evdev == evdev) {
			return &k;
		}
	}
	return nullptr;
}

UINT8 lookup(const KeyMap *table, size_t n, int evdev)
{
	for (size_t i = 0; i < n; i++) {
		if (table[i].evdev == evdev) {
			return table[i].pc98;
		}
	}
	return NC;
}

UINT8 evdev_to_pc98(int evdev)
{
#if defined(__linux__)
	if (np2oscfg.xrollkey) {
		if (evdev == KEY_PAGEUP) {
			evdev = KEY_PAGEDOWN;
		} else if (evdev == KEY_PAGEDOWN) {
			evdev = KEY_PAGEUP;
		}
	}
#endif
	UINT8 kc = lookup(kCommon, NELEMENTS(kCommon), evdev);
	if (kc != NC) {
		return kc;
	}
	if (np2oscfg.KEYBOARD == KEY_KEY101) {
		return NC;
	}
	return lookup(k106, NELEMENTS(k106), evdev);
}

/* XKB keycode -> evdev, or -1 when out of range */
int to_evdev(int rawcode)
{
	const int evdev = rawcode - 8;
	return (evdev > 0 && evdev < kMaxEvdev) ? evdev : -1;
}

} // namespace

extern "C" {

void wxkbd_initialize(void)
{
	std::memset(s_held, NC, sizeof(s_held));
	std::memset(s_shiftFix, SHIFT_KEPT, sizeof(s_shiftFix));
	s_leftShift = false;
	s_rightShift = false;
}

void wxkbd_reset(void)
{
	keystat_allrelease();
	wxkbd_initialize();
}

/* keycode is unused here; rawcode is QKeyEvent::nativeScanCode() */
void wxkbd_keydown(int keycode, int rawcode)
{
	(void)keycode;
	const int evdev = to_evdev(rawcode);
	if (evdev < 0 || s_held[evdev] != NC) {
		return;  /* unknown, or autorepeat of a held key */
	}
#if defined(__linux__)
	if (evdev == KEY_LEFTSHIFT) {
		s_leftShift = true;
	} else if (evdev == KEY_RIGHTSHIFT) {
		s_rightShift = true;
	}
#endif
	if (const UsKey *us = usKey(evdev)) {
		const bool hostShift = s_leftShift || s_rightShift;
		const UINT8 kc = hostShift ? us->shifted : us->plain;
		const bool needShift = hostShift ? us->shiftedShift : us->plainShift;
		if (needShift && !hostShift) {
			keystat_keydown(0x70);
			s_shiftFix[evdev] = SHIFT_PRESSED;
		} else if (!needShift && hostShift) {
			releaseHostShift();
			s_shiftFix[evdev] = SHIFT_RELEASED;
		}
		s_held[evdev] = kc;
		keystat_keydown(kc);
		return;
	}
	const UINT8 kc = evdev_to_pc98(evdev);
	if (kc != NC) {
		s_held[evdev] = kc;
		keystat_keydown(kc);
	}
}

void wxkbd_keyup(int keycode, int rawcode)
{
	(void)keycode;
	const int evdev = to_evdev(rawcode);
	if (evdev < 0) {
		return;
	}
#if defined(__linux__)
	if (evdev == KEY_LEFTSHIFT) {
		s_leftShift = false;
	} else if (evdev == KEY_RIGHTSHIFT) {
		s_rightShift = false;
	}
#endif
	if (s_held[evdev] == NC) {
		return;
	}
	keystat_keyup(s_held[evdev]);
	s_held[evdev] = NC;
	/* Shift back to what the host's Shift keys say now */
	if (s_shiftFix[evdev] == SHIFT_PRESSED && !s_leftShift && !s_rightShift) {
		keystat_keyup(0x70);
	} else if (s_shiftFix[evdev] == SHIFT_RELEASED) {
		pressHostShift();
	}
	s_shiftFix[evdev] = SHIFT_KEPT;
}

} // extern "C"
