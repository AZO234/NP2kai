/**
 * @file	kbtrans.c
 * @brief	Implementation of the keyboard
 */

#include "compiler.h"
#include "np2.h"
#include "kbtrans.h"
#include "keystat.h"
#include "vramhdl.h"
#include "menubase.h"
#include "sysmenu.h"

#if defined(__LIBRETRO__)
typedef struct {
   uint16_t lrkey;
   UINT8 keycode;
} LRKCNV;

#define		NC		0xff

/*! 101 keyboard key table */
static const LRKCNV lrcnv101[] =
{
			{RETROK_ESCAPE,		0x00},	{RETROK_1,           0x01},
			{RETROK_2,           0x02},	{RETROK_3,           0x03},
			{RETROK_4,           0x04},	{RETROK_5,           0x05},
			{RETROK_6,           0x06},	{RETROK_7,           0x07},
   
			{RETROK_8,           0x08},	{RETROK_9,           0x09},
			{RETROK_0,           0x0a},	{RETROK_MINUS,       0x0b},
			{RETROK_CARET,       0x0c},	{RETROK_BACKSLASH,	0x0d},
			{RETROK_BACKSPACE,	0x0e},	{RETROK_TAB,			0x0f},
   
			{RETROK_q,           0x10},	{RETROK_w,           0x11},
			{RETROK_e,           0x12},	{RETROK_r,           0x13},
			{RETROK_t,           0x14},	{RETROK_y,           0x15},
			{RETROK_u,           0x16},	{RETROK_i,           0x17},
   
			{RETROK_o,           0x18},	{RETROK_p,           0x19},
			{RETROK_AT,          0x1a},	{RETROK_LEFTBRACKET,	0x1b},
			{RETROK_RETURN,		0x1c},	{RETROK_a,           0x1d},
			{RETROK_s,           0x1e},	{RETROK_d,           0x1f},
   
			{RETROK_f,           0x20},	{RETROK_g,           0x21},
			{RETROK_h,           0x22},	{RETROK_j,           0x23},
			{RETROK_k,           0x24},	{RETROK_l,           0x25},
			{RETROK_SEMICOLON,	0x26},	{RETROK_COLON,       0x27},
   
			{RETROK_RIGHTBRACKET,0x28},	{RETROK_z,           0x29},
			{RETROK_x,           0x2a},	{RETROK_c,           0x2b},
			{RETROK_v,           0x2c},	{RETROK_b,           0x2d},
			{RETROK_n,           0x2e},	{RETROK_m,           0x2f},
   
			{RETROK_COMMA,       0x30},	{RETROK_PERIOD,		0x31},
			{RETROK_SLASH,       0x32},	{RETROK_UNDERSCORE,	0x33},
			{RETROK_SPACE,       0x34},
			{RETROK_PAGEUP,      0x36},	{RETROK_PAGEDOWN,		0x37},
   
			{RETROK_INSERT,		0x38},	{RETROK_DELETE,		0x39},
			{RETROK_UP,          0x3a},	{RETROK_LEFT,			0x3b},
			{RETROK_RIGHT,       0x3c},	{RETROK_DOWN,			0x3d},
			{RETROK_HOME,			0x3e},	{RETROK_END,			0x3f},
   
			{RETROK_KP_MINUS,		0x40},	{RETROK_KP_DIVIDE,	0x41},
			{RETROK_KP7,			0x42},	{RETROK_KP8,			0x43},
			{RETROK_KP9,			0x44},	{RETROK_KP_MULTIPLY,	0x45},
			{RETROK_KP4,			0x46},	{RETROK_KP5,			0x47},
   
			{RETROK_KP6,			0x48},	{RETROK_KP_PLUS,		0x49},
			{RETROK_KP1,			0x4a},	{RETROK_KP2,			0x4b},
			{RETROK_KP3,			0x4c},	{RETROK_KP_EQUALS,	0x4d},
			{RETROK_KP0,			0x4e},
   
			{RETROK_KP_PERIOD,	0x50},	{RETROK_KP_ENTER,	   0x1c},
   
			{RETROK_PAUSE,       0x60},	{RETROK_PRINT,       0x61},
			{RETROK_F1,          0x62},	{RETROK_F2,          0x63},
			{RETROK_F3,          0x64},	{RETROK_F4,          0x65},
			{RETROK_F5,          0x66},	{RETROK_F6,          0x67},
   
			{RETROK_F7,          0x68},	{RETROK_F8,          0x69},
			{RETROK_F9,          0x6a},	{RETROK_F10,			0x6b},
   
			{RETROK_RSHIFT,		0x70},	{RETROK_LSHIFT,		0x70},
			{RETROK_CAPSLOCK,		0x71},
			{RETROK_RALT,			0x73},	{RETROK_LALT,			0x73},
			{RETROK_RCTRL,       0x74},	{RETROK_LCTRL,       0x74},
   
			/* = */
			{RETROK_EQUALS,		0x0c},
   
         /* @ and : keys for western qwerty kb */
         {RETROK_BACKQUOTE,   0x1a},   {RETROK_QUOTE,       0x27},
   
         /* _ as shift+F11 for western qwerty kb */
         {RETROK_F11,         0x33},
   
			/* MacOS Yen */
			//{0xa5,				0x0d}
};

static UINT8 pc98key[0xFFFF];
static bool key_states[0xFFFF];
uint16_t keys_to_poll[500];
uint16_t keys_needed;

void init_lr_key_to_pc98(){
   memset(pc98key, 0xFF, 0xFFFF);
   
   size_t i;
   for (i = 0; i < SDL_arraysize(lrcnv101); i++)
   {
      pc98key[lrcnv101[i].lrkey] = lrcnv101[i].keycode;
      keys_to_poll[i] = lrcnv101[i].lrkey;
   }
   
   for (i = 0; i < 0xFFFF; i++)
   {
      key_states[i] = false;
   }
   
   keys_needed = SDL_arraysize(lrcnv101);
}

void send_libretro_key_down(uint16_t key){

   UINT8	data = pc98key[key];

   if (data != NC && !key_states[key])
   {
      keystat_senddata(data);//keystat_keydown(data);
      key_states[key] = true;
   }
}

void send_libretro_key_up(uint16_t key){
   UINT8	data = pc98key[key];

   if (data != NC && key_states[key])
   {
      keystat_senddata((UINT8)(data | 0x80));//keystat_keyup(data);
      key_states[key] = false;
   }
}
#else	/* __LIBRETRO__ */
typedef struct {
	SDL_Scancode sdlkey;
	UINT8 keycode;
} SDLKCNV;

#define		NC		0xff

/*! 101 keyboard key table */
static const SDLKCNV sdlcnv101[] =
{
			{SDL_SCANCODE_ESCAPE,		0x00},	{SDL_SCANCODE_1,			0x01},
			{SDL_SCANCODE_2,			0x02},	{SDL_SCANCODE_3,			0x03},
			{SDL_SCANCODE_4,			0x04},	{SDL_SCANCODE_5,			0x05},
			{SDL_SCANCODE_6,			0x06},	{SDL_SCANCODE_7,			0x07},

			{SDL_SCANCODE_8,			0x08},	{SDL_SCANCODE_9,			0x09},
			{SDL_SCANCODE_0,			0x0a},	{SDL_SCANCODE_MINUS,		0x0b},
			{SDL_SCANCODE_EQUALS,		0x0c},	{SDL_SCANCODE_INTERNATIONAL3,	0x0d},
			{SDL_SCANCODE_BACKSPACE,	0x0e},	{SDL_SCANCODE_TAB,			0x0f},

			{SDL_SCANCODE_Q,			0x10},	{SDL_SCANCODE_W,			0x11},
			{SDL_SCANCODE_E,			0x12},	{SDL_SCANCODE_R,			0x13},
			{SDL_SCANCODE_T,			0x14},	{SDL_SCANCODE_Y,			0x15},
			{SDL_SCANCODE_U,			0x16},	{SDL_SCANCODE_I,			0x17},

			{SDL_SCANCODE_O,			0x18},	{SDL_SCANCODE_P,			0x19},
			{SDL_SCANCODE_LEFTBRACKET,			0x1a},	{SDL_SCANCODE_RIGHTBRACKET,	0x1b},
			{SDL_SCANCODE_RETURN,		0x1c},	{SDL_SCANCODE_A,			0x1d},
			{SDL_SCANCODE_S,			0x1e},	{SDL_SCANCODE_D,			0x1f},

			{SDL_SCANCODE_F,			0x20},	{SDL_SCANCODE_G,			0x21},
			{SDL_SCANCODE_H,			0x22},	{SDL_SCANCODE_J,			0x23},
			{SDL_SCANCODE_K,			0x24},	{SDL_SCANCODE_L,			0x25},
			{SDL_SCANCODE_SEMICOLON,	0x26},	{SDL_SCANCODE_APOSTROPHE,		0x27},

			{SDL_SCANCODE_BACKSLASH,	0x28},	{SDL_SCANCODE_Z,			0x29},
			{SDL_SCANCODE_X,			0x2a},	{SDL_SCANCODE_C,			0x2b},
			{SDL_SCANCODE_V,			0x2c},	{SDL_SCANCODE_B,			0x2d},
			{SDL_SCANCODE_N,			0x2e},	{SDL_SCANCODE_M,			0x2f},

			{SDL_SCANCODE_COMMA,		0x30},	{SDL_SCANCODE_PERIOD,		0x31},
			{SDL_SCANCODE_SLASH,		0x32},	{SDL_SCANCODE_INTERNATIONAL1,	0x33},
			{SDL_SCANCODE_SPACE,		0x34},	{SDL_SCANCODE_INTERNATIONAL4,	0x35},
			{SDL_SCANCODE_PAGEUP,		0x36},	{SDL_SCANCODE_PAGEDOWN,		0x37},

			{SDL_SCANCODE_INSERT,		0x38},	{SDL_SCANCODE_DELETE,		0x39},
			{SDL_SCANCODE_UP,			0x3a},	{SDL_SCANCODE_LEFT,			0x3b},
			{SDL_SCANCODE_RIGHT,		0x3c},	{SDL_SCANCODE_DOWN,			0x3d},
			{SDL_SCANCODE_HOME,			0x3e},	{SDL_SCANCODE_END,			0x3f},

			{SDL_SCANCODE_KP_MINUS,		0x40},	{SDL_SCANCODE_KP_DIVIDE,	0x41},
			{SDL_SCANCODE_KP_7,			0x42},	{SDL_SCANCODE_KP_8,			0x43},
			{SDL_SCANCODE_KP_9,			0x44},	{SDL_SCANCODE_KP_MULTIPLY,	0x45},
			{SDL_SCANCODE_KP_4,			0x46},	{SDL_SCANCODE_KP_5,			0x47},

			{SDL_SCANCODE_KP_6,			0x48},	{SDL_SCANCODE_KP_PLUS,		0x49},
			{SDL_SCANCODE_KP_1,			0x4a},	{SDL_SCANCODE_KP_2,			0x4b},
			{SDL_SCANCODE_KP_3,			0x4c},	{SDL_SCANCODE_KP_EQUALS,	0x4d},
			{SDL_SCANCODE_KP_0,			0x4e},	{SDL_SCANCODE_KP_ENTER,		0x1c},

			{SDL_SCANCODE_KP_PERIOD,	0x50},		{SDL_SCANCODE_INTERNATIONAL5,	0x51},

			{SDL_SCANCODE_PAUSE,		0x60},	{SDL_SCANCODE_PRINTSCREEN,	0x61},
			{SDL_SCANCODE_F1,			0x62},	{SDL_SCANCODE_F2,			0x63},
			{SDL_SCANCODE_F3,			0x64},	{SDL_SCANCODE_F4,			0x65},
			{SDL_SCANCODE_F5,			0x66},	{SDL_SCANCODE_F6,			0x67},

			{SDL_SCANCODE_F7,			0x68},	{SDL_SCANCODE_F8,			0x69},
			{SDL_SCANCODE_F9,			0x6a},	{SDL_SCANCODE_F10,			0x6b},

			{SDL_SCANCODE_LSHIFT,		0x70},	{SDL_SCANCODE_RSHIFT,		0x75},
			{SDL_SCANCODE_CAPSLOCK,		0x71},
			{SDL_SCANCODE_LALT,			0x72},	{SDL_SCANCODE_RALT,			0x73},
			{SDL_SCANCODE_LCTRL,		0x74},	{SDL_SCANCODE_RCTRL,		0x74},
			{SDL_SCANCODE_LGUI,		0x77},	{SDL_SCANCODE_RGUI,		0x78},
			{SDL_SCANCODE_APPLICATION,	0x79},

			/* = */
//			{SDLK_EQUALS,		0x0c},

			/* MacOS Yen */
//			{0xa5,				0x0d},
};

/**
 * Serializes
 * @param[in] key Key code
 * @return PC-98 data
 */
static UINT8 getKey(SDL_Scancode key)
{
	size_t i;

	for (i = 0; i < SDL_arraysize(sdlcnv101); i++)
	{
		if (sdlcnv101[i].sdlkey == key)
		{
			return sdlcnv101[i].keycode;
		}
	}
	return NC;
}

/**
 * Key down
 * @param[in] key Key code
 */
void sdlkbd_keydown(UINT key)
{
	UINT8	data;

	data = getKey(key);
	if (data != NC)
	{
		keystat_senddata(data);
	}
}

/**
 * Key up
 * @param[in] key Key code
 */
void sdlkbd_keyup(UINT key)
{
	UINT8	data;

	data = getKey(key);
	if (data != NC)
	{
		keystat_senddata((UINT8)(data | 0x80));
	}
}
#endif	/* __LIBRETRO__ */

