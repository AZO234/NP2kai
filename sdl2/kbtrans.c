/**
 * @file   kbtrans.c
 * @brief   Implementation of the keyboard
 */
#include "kbtrans.h"
#include "np2.h"
#include "keystat.h"
#include "kbdmng.h"
#include "vramhdl.h"
#include "menubase.h"
#include "sysmenu.h"

static bool key_states[0x100];

#if defined(__LIBRETRO__)

#define NC 0xff

/* 101 keyboard key table */
static const LRKCNV lrcnv101[] = {
  // test result on Linux Twocode/caNnotpush/Miss/Different
  {RETROK_PAUSE,        0x60},  // STOP
  {RETROK_PRINT,        0x61},  // COPY
  {RETROK_F1,           0x62},  // f.1
  {RETROK_F2,           0x63},  // f.2
  {RETROK_F3,           0x64},  // f.3
  {RETROK_F4,           0x65},  // f.4
  {RETROK_F5,           0x66},  // f.5
  {RETROK_F6,           0x67},  // f.6
  {RETROK_F7,           0x68},  // f.7
  {RETROK_F8,           0x69},  // f.8
  {RETROK_F9,           0x6a},  // f.9
  {RETROK_F10,          0x6b},  // f.10
  // vf.1 - 5
  {RETROK_ESCAPE,       0x00},  // ESC
  {RETROK_1,            0x01},  // 1 !
  {RETROK_2,            0x02},  // 2 " (T 0x02,0x2A)
  {RETROK_3,            0x03},  // 3 #
  {RETROK_4,            0x04},  // 4 $
  {RETROK_5,            0x05},  // 5 %
  {RETROK_6,            0x06},  // 6 &
  {RETROK_7,            0x07},  // 7 '
  {RETROK_8,            0x08},  // 8 (
  {RETROK_9,            0x09},  // 9 )
  {RETROK_0,            0x0a},  // 0 0
  {RETROK_MINUS,        0x0b},  // - =
  {RETROK_EQUALS,       0x0c},  // ^ `
  {RETROK_BACKSLASH,    0x0d},  // Yen |
  {RETROK_BACKSPACE,    0x0e},  // BS
  {RETROK_TAB,          0x0f},  // TAB
  {RETROK_q,            0x10},  // q Q
  {RETROK_w,            0x11},  // w W
  {RETROK_e,            0x12},  // e E
  {RETROK_r,            0x13},  // r R
  {RETROK_t,            0x14},  // t T
  {RETROK_y,            0x15},  // y Y
  {RETROK_u,            0x16},  // u U
  {RETROK_i,            0x17},  // i I
  {RETROK_o,            0x18},  // o O
  {RETROK_p,            0x19},  // p P
  {RETROK_TILDE,        0x1a},  // @ ~ (M)
  {RETROK_LEFTBRACKET,  0x1b},  // [ {
  {RETROK_RETURN,       0x1c},  // Enter
  {RETROK_LCTRL,        0x74},  // CTRL
  {RETROK_CAPSLOCK,     0x71},  // CAPS
  {RETROK_a,            0x1d},  // a A
  {RETROK_s,            0x1e},  // s S
  {RETROK_d,            0x1f},  // d D
  {RETROK_f,            0x20},  // f F
  {RETROK_g,            0x21},  // g G
  {RETROK_h,            0x22},  // h H
  {RETROK_j,            0x23},  // j J
  {RETROK_k,            0x24},  // k K
  {RETROK_l,            0x25},  // l L
  {RETROK_SEMICOLON,    0x26},  // ; +
  {RETROK_QUOTE,        0x27},  // : *
  {RETROK_RIGHTBRACKET, 0x28},  // ] }
  {RETROK_LSHIFT,       0x70},  // LShift
  {RETROK_z,            0x29},  // z Z
  {RETROK_x,            0x2a},  // x X
  {RETROK_c,            0x2b},  // c C
  {RETROK_v,            0x2c},  // v V
  {RETROK_b,            0x2d},  // b B
  {RETROK_n,            0x2e},  // n N
  {RETROK_m,            0x2f},  // m M
  {RETROK_COMMA,        0x30},  // , <
  {RETROK_PERIOD,       0x31},  // . >
  {RETROK_SLASH,        0x32},  // / ?
  // _ _
  {RETROK_RSHIFT,       0x75},  // RShift
  // Kana
  {RETROK_LSUPER,       0x70},  // LSuper
  {RETROK_RCTRL,        0x33},  // GRPH
  {RETROK_LALT,         0x51},  // NFER
  {RETROK_SPACE,        0x34},  // Space
  {RETROK_RALT,         0x35},  // XFER
  {RETROK_RSUPER,       0x78},  // RSuper
  {RETROK_MENU,         0x79},  // Menu
  {RETROK_INSERT,       0x38},  // INS
  {RETROK_DELETE,       0x39},  // DEL
  {RETROK_PAGEUP,       0x36},  // ROLLUP
  {RETROK_PAGEDOWN,     0x37},  // ROLLDOWN
  {RETROK_UP,           0x3a},  // Up
  {RETROK_LEFT,         0x3b},  // Left
  {RETROK_RIGHT,        0x3c},  // Right
  {RETROK_DOWN,         0x3d},  // Down
  {RETROK_HOME,         0x3e},  // HOME/CLR
  {RETROK_END,          0x3f},  // HELP
  {RETROK_KP_MINUS,     0x40},  // KP-
  {RETROK_KP_DIVIDE,    0x41},  // KP/
  {RETROK_KP7,          0x42},  // KP7
  {RETROK_KP8,          0x43},  // KP8
  {RETROK_KP9,          0x44},  // KP9
  {RETROK_KP_MULTIPLY,  0x45},  // KP*
  {RETROK_KP4,          0x46},  // KP4
  {RETROK_KP5,          0x47},  // KP5
  {RETROK_KP6,          0x48},  // KP6
  {RETROK_KP_PLUS,      0x49},  // KP+
  {RETROK_KP1,          0x4a},  // KP1
  {RETROK_KP2,          0x4b},  // KP2
  {RETROK_KP3,          0x4c},  // KP3
  {RETROK_KP_EQUALS,    0x4d},  // KP= (N)
  {RETROK_KP0,          0x4e},  // KP0
  // KP,
  {RETROK_KP_PERIOD,    0x50},  // KP. (M)
  {RETROK_KP_ENTER,     0x1c},  // KPEnter
};

/* 106 keyboard key table */
static const LRKCNV lrcnv106[] = {
  // test result on Linux Twocode/caNnotpush/Miss/Different
  {RETROK_PAUSE,        0x60},  // STOP
  {RETROK_PRINT,        0x61},  // COPY
  {RETROK_F1,           0x62},  // f.1
  {RETROK_F2,           0x63},  // f.2
  {RETROK_F3,           0x64},  // f.3
  {RETROK_F4,           0x65},  // f.4
  {RETROK_F5,           0x66},  // f.5
  {RETROK_F6,           0x67},  // f.6
  {RETROK_F7,           0x68},  // f.7
  {RETROK_F8,           0x69},  // f.8
  {RETROK_F9,           0x6a},  // f.9
  {RETROK_F10,          0x6b},  // f.10
  // vf.1 - 5
  {RETROK_ESCAPE,       0x00},  // ESC
  {RETROK_1,            0x01},  // 1 !
  {RETROK_2,            0x02},  // 2 "
  {RETROK_3,            0x03},  // 3 #
  {RETROK_4,            0x04},  // 4 $
  {RETROK_5,            0x05},  // 5 %
  {RETROK_6,            0x06},  // 6 &
  {RETROK_7,            0x07},  // 7 '
  {RETROK_8,            0x08},  // 8 (
  {RETROK_9,            0x09},  // 9 )
  {RETROK_0,            0x0a},  // 0 0
  {RETROK_MINUS,        0x0b},  // - =
  {RETROK_EQUALS,       0x0c},  // ^ `
  {RETROK_BACKSLASH,    0x0d},  // Yen | (M)
  {RETROK_BACKSPACE,    0x0e},  // BS
  {RETROK_TAB,          0x0f},  // TAB
  {RETROK_q,            0x10},  // q Q
  {RETROK_w,            0x11},  // w W
  {RETROK_e,            0x12},  // e E
  {RETROK_r,            0x13},  // r R
  {RETROK_t,            0x14},  // t T
  {RETROK_y,            0x15},  // y Y
  {RETROK_u,            0x16},  // u U
  {RETROK_i,            0x17},  // i I
  {RETROK_o,            0x18},  // o O
  {RETROK_p,            0x19},  // p P
  {RETROK_AT,           0x1a},  // @ ~
  {RETROK_LEFTBRACKET,  0x1b},  // [ {
  {RETROK_RETURN,       0x1c},  // Enter
  {RETROK_LCTRL,        0x74},  // CTRL
  {RETROK_CAPSLOCK,     0x71},  // CAPS
  {RETROK_a,            0x1d},  // a A
  {RETROK_s,            0x1e},  // s S
  {RETROK_d,            0x1f},  // d D
  {RETROK_f,            0x20},  // f F
  {RETROK_g,            0x21},  // g G
  {RETROK_h,            0x22},  // h H
  {RETROK_j,            0x23},  // j J
  {RETROK_k,            0x24},  // k K
  {RETROK_l,            0x25},  // l L
  {RETROK_SEMICOLON,    0x26},  // ; +
  {RETROK_QUOTE,        0x27},  // : *
  {RETROK_RIGHTBRACKET, 0x28},  // ] }
  {RETROK_LSHIFT,       0x70},  // LShift
  {RETROK_z,            0x29},  // z Z
  {RETROK_x,            0x2a},  // x X
  {RETROK_c,            0x2b},  // c C
  {RETROK_v,            0x2c},  // v V
  {RETROK_b,            0x2d},  // b B
  {RETROK_n,            0x2e},  // n N
  {RETROK_m,            0x2f},  // m M
  {RETROK_COMMA,        0x30},  // , <
  {RETROK_PERIOD,       0x31},  // . >
  {RETROK_SLASH,        0x32},  // / ?
  {2,      0x33},       // _ _ (L2?menu open?)
  {RETROK_RSHIFT,       0x75},  // RShift
  // Kana
  {RETROK_LSUPER,       0x70},  // LSuper
  {RETROK_RCTRL,        0x33},  // GRPH
  {RETROK_LALT,         0x51},  // NFER
  {RETROK_SPACE,        0x34},  // Space
  {RETROK_RALT,         0x35},  // XFER
  {RETROK_RSUPER,       0x78},  // RSuper
  {RETROK_MENU,         0x79},  // Menu
  {RETROK_INSERT,       0x38},  // INS
  {RETROK_DELETE,       0x39},  // DEL
  {RETROK_PAGEUP,       0x36},  // ROLLUP
  {RETROK_PAGEDOWN,     0x37},  // ROLLDOWN
  {RETROK_UP,           0x3a},  // Up
  {RETROK_LEFT,         0x3b},  // Left
  {RETROK_RIGHT,        0x3c},  // Right
  {RETROK_DOWN,         0x3d},  // Down
  {RETROK_HOME,         0x3e},  // HOME/CLR
  {RETROK_END,          0x3f},  // HELP
  {RETROK_KP_MINUS,     0x40},  // KP-
  {RETROK_KP_DIVIDE,    0x41},  // KP/
  {RETROK_KP7,          0x42},  // KP7
  {RETROK_KP8,          0x43},  // KP8
  {RETROK_KP9,          0x44},  // KP9
  {RETROK_KP_MULTIPLY,  0x45},  // KP*
  {RETROK_KP4,          0x46},  // KP4
  {RETROK_KP5,          0x47},  // KP5
  {RETROK_KP6,          0x48},  // KP6
  {RETROK_KP_PLUS,      0x49},  // KP+
  {RETROK_KP1,          0x4a},  // KP1
  {RETROK_KP2,          0x4b},  // KP2
  {RETROK_KP3,          0x4c},  // KP3
  {RETROK_KP_EQUALS,    0x4d},  // KP= (N)
  {RETROK_KP0,          0x4e},  // KP0
  // KP,
  {RETROK_KP_PERIOD,    0x50},  // KP. (M)
  {RETROK_KP_ENTER,     0x1c},  // KPEnter
};

LRKCNV* keys_poll;
uint16_t keys_needed;

void init_lrkey_to_pc98() {
  memset(key_states, 0, sizeof(key_states));
  if(np2oscfg.KEYBOARD == KEY_KEY101) {
    keys_poll = (LRKCNV*)lrcnv101;
    keys_needed = sizeof(lrcnv101) / sizeof(LRKCNV);
  } else if(np2oscfg.KEYBOARD == KEY_KEY106) {
    keys_poll = (LRKCNV*)lrcnv106;
    keys_needed = sizeof(lrcnv106) / sizeof(LRKCNV);
  }
}

void reset_lrkey() {
  memset(key_states, 0, sizeof(key_states));
}

void send_libretro_key_down(UINT lrkey) {
  size_t i;
  uint8_t keycode;

  if(np2oscfg.xrollkey) {
    if(lrkey == RETROK_PAGEUP) {
      lrkey = RETROK_PAGEDOWN;
    } else if(lrkey == RETROK_PAGEDOWN) {
      lrkey = RETROK_PAGEUP;
    }
  }

  for (i = 0; i < keys_needed; i++) {
    if(keys_poll[i].keycode != NC) {
      if(keys_poll[i].lrkey == lrkey) {
        keycode = keys_poll[i].keycode;
        if(!key_states[keycode]) {
          keystat_keydown(keycode);
          key_states[keycode] = true;
        }
        break;
      }
    }
  }
}

void send_libretro_key_up(UINT lrkey) {
  size_t i;
  uint8_t keycode;

  if(np2oscfg.xrollkey) {
    if(lrkey == RETROK_PAGEUP) {
      lrkey = RETROK_PAGEDOWN;
    } else if(lrkey == RETROK_PAGEDOWN) {
      lrkey = RETROK_PAGEUP;
    }
  }

  for (i = 0; i < keys_needed; i++) {
    if(keys_poll[i].keycode != NC && key_states[keys_poll[i].keycode]) {
      if(keys_poll[i].lrkey == lrkey) {
        keycode = keys_poll[i].keycode;
        if(key_states[keycode]) {
          keystat_keyup(keycode);
          key_states[keycode] = false;
        }
        break;
      }
    }
  }
}

#else  // __LIBRETRO__

typedef struct {
#if SDL_MAJOR_VERSION == 1
   SDLKey sdlkey;
#else
   SDL_Scancode sdlkey;
#endif
   UINT8 keycode;
} SDLKCNV;

#define NC 0xff

/* 101 keyboard key table */
static const SDLKCNV sdlcnv101[] = {
#if SDL_MAJOR_VERSION == 1
  // test result on Linux Twocode/caNnotpush/Miss/Different
  {SDLK_PAUSE,        0x60},  // STOP
  {SDLK_PRINT,        0x61},  // COPY (M)
  {SDLK_F1,           0x62},  // f.1
  {SDLK_F2,           0x63},  // f.2
  {SDLK_F3,           0x64},  // f.3
  {SDLK_F4,           0x65},  // f.4
  {SDLK_F5,           0x66},  // f.5
  {SDLK_F6,           0x67},  // f.6
  {SDLK_F7,           0x68},  // f.7
  {SDLK_F8,           0x69},  // f.8
  {SDLK_F9,           0x6a},  // f.9
  {SDLK_F10,          0x6b},  // f.10
  // vf.1 - 5
  {SDLK_ESCAPE,       0x00},  // ESC
  {SDLK_1,            0x01},  // 1 !
  {SDLK_2,            0x02},  // 2 "
  {SDLK_3,            0x03},  // 3 #
  {SDLK_4,            0x04},  // 4 $
  {SDLK_5,            0x05},  // 5 %
  {SDLK_6,            0x06},  // 6 &
  {SDLK_7,            0x07},  // 7 '
  {SDLK_8,            0x08},  // 8 (
  {SDLK_9,            0x09},  // 9 )
  {SDLK_0,            0x0a},  // 0 0
  {SDLK_MINUS,        0x0b},  // - =
  {SDLK_EQUALS,       0x0c},  // ^ `
  {SDLK_BACKSLASH,    0x0d},  // Yen |
  {SDLK_BACKSPACE,    0x0e},  // BS
  {SDLK_TAB,          0x0f},  // TAB
  {SDLK_q,            0x10},  // q Q
  {SDLK_w,            0x11},  // w W
  {SDLK_e,            0x12},  // e E
  {SDLK_r,            0x13},  // r R
  {SDLK_t,            0x14},  // t T
  {SDLK_y,            0x15},  // y Y
  {SDLK_u,            0x16},  // u U
  {SDLK_i,            0x17},  // i I
  {SDLK_o,            0x18},  // o O
  {SDLK_p,            0x19},  // p P
  {SDLK_LEFTBRACKET,  0x1a},  // @ ~
  {SDLK_RIGHTBRACKET, 0x1b},  // [ {
  {SDLK_RETURN,       0x1c},  // Enter
  {SDLK_LCTRL,        0x74},  // CTRL
  {SDLK_CAPSLOCK,     0x71},  // CAPS
  {SDLK_a,            0x1d},  // a A
  {SDLK_s,            0x1e},  // s S
  {SDLK_d,            0x1f},  // d D
  {SDLK_f,            0x20},  // f F
  {SDLK_g,            0x21},  // g G
  {SDLK_h,            0x22},  // h H
  {SDLK_j,            0x23},  // j J
  {SDLK_k,            0x24},  // k K
  {SDLK_l,            0x25},  // l L
  {SDLK_SEMICOLON,    0x26},  // ; +
  {SDLK_QUOTE,        0x27},  // : *
  {SDLK_BACKSLASH,    0x28},  // ] }
  {SDLK_LSHIFT,       0x70},  // LShift
  {SDLK_z,            0x29},  // z Z
  {SDLK_x,            0x2a},  // x X
  {SDLK_c,            0x2b},  // c C
  {SDLK_v,            0x2c},  // v V
  {SDLK_b,            0x2d},  // b B
  {SDLK_n,            0x2e},  // n N
  {SDLK_m,            0x2f},  // m M
  {SDLK_COMMA,        0x30},  // , <
  {SDLK_PERIOD,       0x31},  // . >
  {SDLK_SLASH,        0x32},  // / ?
  // _ _
  {SDLK_RSHIFT,       0x75},  // RShift
  // Kana
  {SDLK_LSUPER,       0x70},  // LSuper (M)
  {SDLK_RCTRL,        0x33},  // GRPH
  {SDLK_LALT,         0x51},  // NFER
  {SDLK_SPACE,        0x34},  // Space
  {SDLK_RALT,         0x35},  // XFER
  {SDLK_RSUPER,       0x78},  // RSuper
  {SDLK_MENU,         0x79},  // Menu
  {SDLK_INSERT,       0x38},  // INS
  {SDLK_DELETE,       0x39},  // DEL
  {SDLK_PAGEUP,       0x36},  // ROLLUP
  {SDLK_PAGEDOWN,     0x37},  // ROLLDOWN
  {SDLK_UP,           0x3a},  // Up
  {SDLK_LEFT,         0x3b},  // Left
  {SDLK_RIGHT,        0x3c},  // Right
  {SDLK_DOWN,         0x3d},  // Down
  {SDLK_HOME,         0x3e},  // HOME/CLR
  {SDLK_END,          0x3f},  // HELP
  {SDLK_KP_MINUS,     0x40},  // KP-
  {SDLK_KP_DIVIDE,    0x41},  // KP/
  {SDLK_KP7,          0x42},  // KP7
  {SDLK_KP8,          0x43},  // KP8
  {SDLK_KP9,          0x44},  // KP9
  {SDLK_KP_MULTIPLY,  0x45},  // KP*
  {SDLK_KP4,          0x46},  // KP4
  {SDLK_KP5,          0x47},  // KP5
  {SDLK_KP6,          0x48},  // KP6
  {SDLK_KP_PLUS,      0x49},  // KP+
  {SDLK_KP1,          0x4a},  // KP1
  {SDLK_KP2,          0x4b},  // KP2
  {SDLK_KP3,          0x4c},  // KP3
  {SDLK_KP_EQUALS,    0x4d},  // KP= (N)
  {SDLK_KP0,          0x4e},  // KP0
//  {SDLK_KP_COMMA,     0x4f},  // KP, (N)
  {SDLK_KP_PERIOD,    0x50},  // KP.
  {SDLK_KP_ENTER,     0x1c},  // KPEnter
#else
  // test result on Linux Twocode/caNnotpush/Miss/Different
  {SDL_SCANCODE_PAUSE,        0x60},  // STOP
  {SDL_SCANCODE_PRINTSCREEN,  0x61},  // COPY (M)
  {SDL_SCANCODE_F1,           0x62},  // f.1
  {SDL_SCANCODE_F2,           0x63},  // f.2
  {SDL_SCANCODE_F3,           0x64},  // f.3
  {SDL_SCANCODE_F4,           0x65},  // f.4
  {SDL_SCANCODE_F5,           0x66},  // f.5
  {SDL_SCANCODE_F6,           0x67},  // f.6
  {SDL_SCANCODE_F7,           0x68},  // f.7
  {SDL_SCANCODE_F8,           0x69},  // f.8
  {SDL_SCANCODE_F9,           0x6a},  // f.9
  {SDL_SCANCODE_F10,          0x6b},  // f.10
  // vf.1 - 5
  {SDL_SCANCODE_ESCAPE,       0x00},  // ESC
  {SDL_SCANCODE_1,            0x01},  // 1 !
  {SDL_SCANCODE_2,            0x02},  // 2 "
  {SDL_SCANCODE_3,            0x03},  // 3 #
  {SDL_SCANCODE_4,            0x04},  // 4 $
  {SDL_SCANCODE_5,            0x05},  // 5 %
  {SDL_SCANCODE_6,            0x06},  // 6 &
  {SDL_SCANCODE_7,            0x07},  // 7 '
  {SDL_SCANCODE_8,            0x08},  // 8 (
  {SDL_SCANCODE_9,            0x09},  // 9 )
  {SDL_SCANCODE_0,            0x0a},  // 0 0
  {SDL_SCANCODE_MINUS,        0x0b},  // - =
  {SDL_SCANCODE_EQUALS,       0x0c},  // ^ `
  {SDL_SCANCODE_GRAVE,        0x0d},  // Yen |
  {SDL_SCANCODE_BACKSPACE,    0x0e},  // BS
  {SDL_SCANCODE_TAB,          0x0f},  // TAB
  {SDL_SCANCODE_Q,            0x10},  // q Q
  {SDL_SCANCODE_W,            0x11},  // w W
  {SDL_SCANCODE_E,            0x12},  // e E
  {SDL_SCANCODE_R,            0x13},  // r R
  {SDL_SCANCODE_T,            0x14},  // t T
  {SDL_SCANCODE_Y,            0x15},  // y Y
  {SDL_SCANCODE_U,            0x16},  // u U
  {SDL_SCANCODE_I,            0x17},  // i I
  {SDL_SCANCODE_O,            0x18},  // o O
  {SDL_SCANCODE_P,            0x19},  // p P
  {SDL_SCANCODE_LEFTBRACKET,  0x1a},  // @ ~
  {SDL_SCANCODE_RIGHTBRACKET, 0x1b},  // [ {
  {SDL_SCANCODE_RETURN,       0x1c},  // Enter
  {SDL_SCANCODE_LCTRL,        0x74},  // CTRL
  {SDL_SCANCODE_CAPSLOCK,     0x71},  // CAPS
  {SDL_SCANCODE_A,            0x1d},  // a A
  {SDL_SCANCODE_S,            0x1e},  // s S
  {SDL_SCANCODE_D,            0x1f},  // d D
  {SDL_SCANCODE_F,            0x20},  // f F
  {SDL_SCANCODE_G,            0x21},  // g G
  {SDL_SCANCODE_H,            0x22},  // h H
  {SDL_SCANCODE_J,            0x23},  // j J
  {SDL_SCANCODE_K,            0x24},  // k K
  {SDL_SCANCODE_L,            0x25},  // l L
  {SDL_SCANCODE_SEMICOLON,    0x26},  // ; +
  {SDL_SCANCODE_APOSTROPHE,   0x27},  // : *
  {SDL_SCANCODE_BACKSLASH,    0x28},  // ] }
  {SDL_SCANCODE_LSHIFT,       0x70},  // LShift
  {SDL_SCANCODE_Z,            0x29},  // z Z
  {SDL_SCANCODE_X,            0x2a},  // x X
  {SDL_SCANCODE_C,            0x2b},  // c C
  {SDL_SCANCODE_V,            0x2c},  // v V
  {SDL_SCANCODE_B,            0x2d},  // b B
  {SDL_SCANCODE_N,            0x2e},  // n N
  {SDL_SCANCODE_M,            0x2f},  // m M
  {SDL_SCANCODE_COMMA,        0x30},  // , <
  {SDL_SCANCODE_PERIOD,       0x31},  // . >
  {SDL_SCANCODE_SLASH,        0x32},  // / ?
  // _ _
  {SDL_SCANCODE_RSHIFT,       0x75},  // RShift
  // Kana
  {SDL_SCANCODE_LGUI,         0x70},  // LSuper (M)
  {SDL_SCANCODE_RCTRL,        0x33},  // GRPH
  {SDL_SCANCODE_LALT,         0x51},  // NFER
  {SDL_SCANCODE_SPACE,        0x34},  // Space
  {SDL_SCANCODE_RALT,         0x35},  // XFER
  {SDL_SCANCODE_RGUI,         0x78},  // RSuper
  {SDL_SCANCODE_APPLICATION,  0x79},  // Menu
  {SDL_SCANCODE_INSERT,       0x38},  // INS
  {SDL_SCANCODE_DELETE,       0x39},  // DEL
  {SDL_SCANCODE_PAGEUP,       0x36},  // ROLLUP
  {SDL_SCANCODE_PAGEDOWN,     0x37},  // ROLLDOWN
  {SDL_SCANCODE_UP,           0x3a},  // Up
  {SDL_SCANCODE_LEFT,         0x3b},  // Left
  {SDL_SCANCODE_RIGHT,        0x3c},  // Right
  {SDL_SCANCODE_DOWN,         0x3d},  // Down
  {SDL_SCANCODE_HOME,         0x3e},  // HOME/CLR
  {SDL_SCANCODE_END,          0x3f},  // HELP
  {SDL_SCANCODE_KP_MINUS,     0x40},  // KP-
  {SDL_SCANCODE_KP_DIVIDE,    0x41},  // KP/
  {SDL_SCANCODE_KP_7,         0x42},  // KP7
  {SDL_SCANCODE_KP_8,         0x43},  // KP8
  {SDL_SCANCODE_KP_9,         0x44},  // KP9
  {SDL_SCANCODE_KP_MULTIPLY,  0x45},  // KP*
  {SDL_SCANCODE_KP_4,         0x46},  // KP4
  {SDL_SCANCODE_KP_5,         0x47},  // KP5
  {SDL_SCANCODE_KP_6,         0x48},  // KP6
  {SDL_SCANCODE_KP_PLUS,      0x49},  // KP+
  {SDL_SCANCODE_KP_1,         0x4a},  // KP1
  {SDL_SCANCODE_KP_2,         0x4b},  // KP2
  {SDL_SCANCODE_KP_3,         0x4c},  // KP3
  {SDL_SCANCODE_KP_EQUALS,    0x4d},  // KP= (N)
  {SDL_SCANCODE_KP_0,         0x4e},  // KP0
  {SDL_SCANCODE_KP_COMMA,     0x4f},  // KP, (N)
  {SDL_SCANCODE_KP_PERIOD,    0x50},  // KP.
  {SDL_SCANCODE_KP_ENTER,     0x1c},  // KPEnter
#endif
};

/* 106 keyboard key table */
static const SDLKCNV sdlcnv106[] = {
#if SDL_MAJOR_VERSION == 1
  // test result on Linux Twocode/caNnotpush/Miss/Different
  {SDLK_PAUSE,        0x60},  // STOP
  {SDLK_PRINT,        0x61},  // COPY (M)
  {SDLK_F1,           0x62},  // f.1
  {SDLK_F2,           0x63},  // f.2
  {SDLK_F3,           0x64},  // f.3
  {SDLK_F4,           0x65},  // f.4
  {SDLK_F5,           0x66},  // f.5
  {SDLK_F6,           0x67},  // f.6
  {SDLK_F7,           0x68},  // f.7
  {SDLK_F8,           0x69},  // f.8
  {SDLK_F9,           0x6a},  // f.9
  {SDLK_F10,          0x6b},  // f.10
  // vf.1 - 5
  {SDLK_ESCAPE,       0x00},  // ESC
  {SDLK_1,            0x01},  // 1 !
  {SDLK_2,            0x02},  // 2 "
  {SDLK_3,            0x03},  // 3 #
  {SDLK_4,            0x04},  // 4 $
  {SDLK_5,            0x05},  // 5 %
  {SDLK_6,            0x06},  // 6 &
  {SDLK_7,            0x07},  // 7 '
  {SDLK_8,            0x08},  // 8 (
  {SDLK_9,            0x09},  // 9 )
  {SDLK_0,            0x0a},  // 0 0
  {SDLK_MINUS,        0x0b},  // - =
  {SDLK_EQUALS,       0x0c},  // ^ `
  {SDLK_BACKSPACE,    0x0e},  // BS
  {SDLK_TAB,          0x0f},  // TAB
  {SDLK_q,            0x10},  // q Q
  {SDLK_w,            0x11},  // w W
  {SDLK_e,            0x12},  // e E
  {SDLK_r,            0x13},  // r R
  {SDLK_t,            0x14},  // t T
  {SDLK_y,            0x15},  // y Y
  {SDLK_u,            0x16},  // u U
  {SDLK_i,            0x17},  // i I
  {SDLK_o,            0x18},  // o O
  {SDLK_p,            0x19},  // p P
  {SDLK_LEFTBRACKET,  0x1a},  // @ ~
  {SDLK_RIGHTBRACKET, 0x1b},  // [ {
  {SDLK_RETURN,       0x1c},  // Enter
  {SDLK_LCTRL,        0x74},  // CTRL
  {SDLK_CAPSLOCK,     0x71},  // CAPS
  {SDLK_a,            0x1d},  // a A
  {SDLK_s,            0x1e},  // s S
  {SDLK_d,            0x1f},  // d D
  {SDLK_f,            0x20},  // f F
  {SDLK_g,            0x21},  // g G
  {SDLK_h,            0x22},  // h H
  {SDLK_j,            0x23},  // j J
  {SDLK_k,            0x24},  // k K
  {SDLK_l,            0x25},  // l L
  {SDLK_SEMICOLON,    0x26},  // ; +
  {SDLK_QUOTE,        0x27},  // : *
  {SDLK_BACKSLASH,    0x28},  // ] }
  {SDLK_LSHIFT,       0x70},  // LShift
  {SDLK_z,            0x29},  // z Z
  {SDLK_x,            0x2a},  // x X
  {SDLK_c,            0x2b},  // c C
  {SDLK_v,            0x2c},  // v V
  {SDLK_b,            0x2d},  // b B
  {SDLK_n,            0x2e},  // n N
  {SDLK_m,            0x2f},  // m M
  {SDLK_COMMA,        0x30},  // , <
  {SDLK_PERIOD,       0x31},  // . >
  {SDLK_SLASH,        0x32},  // / ?
  // _ _
  {SDLK_RSHIFT,       0x75},  // RShift
  // Kana
  {SDLK_LSUPER,       0x70},  // LSuper (M)
  {SDLK_RCTRL,        0x33},  // GRPH
  {SDLK_LALT,         0x51},  // NFER
  {SDLK_SPACE,        0x34},  // Space
  {SDLK_RALT,         0x35},  // XFER
  {SDLK_RSUPER,       0x78},  // RSuper
  {SDLK_MENU,         0x79},  // Menu
  {SDLK_INSERT,       0x38},  // INS
  {SDLK_DELETE,       0x39},  // DEL
  {SDLK_PAGEUP,       0x36},  // ROLLUP
  {SDLK_PAGEDOWN,     0x37},  // ROLLDOWN
  {SDLK_UP,           0x3a},  // Up
  {SDLK_LEFT,         0x3b},  // Left
  {SDLK_RIGHT,        0x3c},  // Right
  {SDLK_DOWN,         0x3d},  // Down
  {SDLK_HOME,         0x3e},  // HOME/CLR
  {SDLK_END,          0x3f},  // HELP
  {SDLK_KP_MINUS,     0x40},  // KP-
  {SDLK_KP_DIVIDE,    0x41},  // KP/
  {SDLK_KP7,          0x42},  // KP7
  {SDLK_KP8,          0x43},  // KP8
  {SDLK_KP9,          0x44},  // KP9
  {SDLK_KP_MULTIPLY,  0x45},  // KP*
  {SDLK_KP4,          0x46},  // KP4
  {SDLK_KP5,          0x47},  // KP5
  {SDLK_KP6,          0x48},  // KP6
  {SDLK_KP_PLUS,      0x49},  // KP+
  {SDLK_KP1,          0x4a},  // KP1
  {SDLK_KP2,          0x4b},  // KP2
  {SDLK_KP3,          0x4c},  // KP3
  {SDLK_KP_EQUALS,    0x4d},  // KP= (N)
  {SDLK_KP0,          0x4e},  // KP0
//  {SDLK_KP_COMMA,     0x4f},  // KP, (N)
  {SDLK_KP_PERIOD,    0x50},  // KP.
  {SDLK_KP_ENTER,     0x1c},  // KPEnter
#else
  // test result on Linux Twocode/caNnotpush/Miss/Different
  {SDL_SCANCODE_PAUSE,          0x60},  // STOP
  {SDL_SCANCODE_PRINTSCREEN,    0x61},  // COPY (M)
  {SDL_SCANCODE_F1,             0x62},  // f.1
  {SDL_SCANCODE_F2,             0x63},  // f.2
  {SDL_SCANCODE_F3,             0x64},  // f.3
  {SDL_SCANCODE_F4,             0x65},  // f.4
  {SDL_SCANCODE_F5,             0x66},  // f.5
  {SDL_SCANCODE_F6,             0x67},  // f.6
  {SDL_SCANCODE_F7,             0x68},  // f.7
  {SDL_SCANCODE_F8,             0x69},  // f.8
  {SDL_SCANCODE_F9,             0x6a},  // f.9
  {SDL_SCANCODE_F10,            0x6b},  // f.10
  // vf.1 - 5
  {SDL_SCANCODE_ESCAPE,         0x00},  // ESC
  {SDL_SCANCODE_1,              0x01},  // 1 !
  {SDL_SCANCODE_2,              0x02},  // 2 "
  {SDL_SCANCODE_3,              0x03},  // 3 #
  {SDL_SCANCODE_4,              0x04},  // 4 $
  {SDL_SCANCODE_5,              0x05},  // 5 %
  {SDL_SCANCODE_6,              0x06},  // 6 &
  {SDL_SCANCODE_7,              0x07},  // 7 '
  {SDL_SCANCODE_8,              0x08},  // 8 (
  {SDL_SCANCODE_9,              0x09},  // 9 )
  {SDL_SCANCODE_0,              0x0a},  // 0 0
  {SDL_SCANCODE_MINUS,          0x0b},  // - =
  {SDL_SCANCODE_EQUALS,         0x0c},  // ^ `
  {SDL_SCANCODE_INTERNATIONAL3, 0x0d},  // Yen |
  {SDL_SCANCODE_BACKSPACE,      0x0e},  // BS
  {SDL_SCANCODE_TAB,            0x0f},  // TAB
  {SDL_SCANCODE_Q,              0x10},  // q Q
  {SDL_SCANCODE_W,              0x11},  // w W
  {SDL_SCANCODE_E,              0x12},  // e E
  {SDL_SCANCODE_R,              0x13},  // r R
  {SDL_SCANCODE_T,              0x14},  // t T
  {SDL_SCANCODE_Y,              0x15},  // y Y
  {SDL_SCANCODE_U,              0x16},  // u U
  {SDL_SCANCODE_I,              0x17},  // i I
  {SDL_SCANCODE_O,              0x18},  // o O
  {SDL_SCANCODE_P,              0x19},  // p P
  {SDL_SCANCODE_LEFTBRACKET,    0x1a},  // @ ~
  {SDL_SCANCODE_RIGHTBRACKET,   0x1b},  // [ {
  {SDL_SCANCODE_RETURN,         0x1c},  // Enter
  {SDL_SCANCODE_LCTRL,          0x74},  // CTRL
  {SDL_SCANCODE_CAPSLOCK,       0x71},  // CAPS
  {SDL_SCANCODE_A,              0x1d},  // a A
  {SDL_SCANCODE_S,              0x1e},  // s S
  {SDL_SCANCODE_D,              0x1f},  // d D
  {SDL_SCANCODE_F,              0x20},  // f F
  {SDL_SCANCODE_G,              0x21},  // g G
  {SDL_SCANCODE_H,              0x22},  // h H
  {SDL_SCANCODE_J,              0x23},  // j J
  {SDL_SCANCODE_K,              0x24},  // k K
  {SDL_SCANCODE_L,              0x25},  // l L
  {SDL_SCANCODE_SEMICOLON,      0x26},  // ; +
  {SDL_SCANCODE_APOSTROPHE,     0x27},  // : *
  {SDL_SCANCODE_BACKSLASH,      0x28},  // ] }
  {SDL_SCANCODE_LSHIFT,         0x70},  // LShift
  {SDL_SCANCODE_Z,              0x29},  // z Z
  {SDL_SCANCODE_X,              0x2a},  // x X
  {SDL_SCANCODE_C,              0x2b},  // c C
  {SDL_SCANCODE_V,              0x2c},  // v V
  {SDL_SCANCODE_B,              0x2d},  // b B
  {SDL_SCANCODE_N,              0x2e},  // n N
  {SDL_SCANCODE_M,              0x2f},  // m M
  {SDL_SCANCODE_COMMA,          0x30},  // , <
  {SDL_SCANCODE_PERIOD,         0x31},  // . >
  {SDL_SCANCODE_SLASH,          0x32},  // / ?
  {SDL_SCANCODE_INTERNATIONAL1, 0x33},  // _ _
  {SDL_SCANCODE_RSHIFT,         0x75},  // RShift
  // Kana
  {SDL_SCANCODE_LGUI,           0x70},  // LSuper (M)
  {SDL_SCANCODE_RCTRL,          0x33},  // GRPH
  {SDL_SCANCODE_LALT,           0x51},  // NFER
  {SDL_SCANCODE_SPACE,          0x34},  // Space
  {SDL_SCANCODE_RALT,           0x35},  // XFER
  {SDL_SCANCODE_RGUI,           0x78},  // RSuper
  {SDL_SCANCODE_APPLICATION,    0x79},  // Menu
  {SDL_SCANCODE_INSERT,         0x38},  // INS
  {SDL_SCANCODE_DELETE,         0x39},  // DEL
  {SDL_SCANCODE_PAGEUP,         0x36},  // ROLLUP
  {SDL_SCANCODE_PAGEDOWN,       0x37},  // ROLLDOWN
  {SDL_SCANCODE_UP,             0x3a},  // Up
  {SDL_SCANCODE_LEFT,           0x3b},  // Left
  {SDL_SCANCODE_RIGHT,          0x3c},  // Right
  {SDL_SCANCODE_DOWN,           0x3d},  // Down
  {SDL_SCANCODE_HOME,           0x3e},  // HOME/CLR
  {SDL_SCANCODE_END,            0x3f},  // HELP
  {SDL_SCANCODE_KP_MINUS,       0x40},  // KP-
  {SDL_SCANCODE_KP_DIVIDE,      0x41},  // KP/
  {SDL_SCANCODE_KP_7,           0x42},  // KP7
  {SDL_SCANCODE_KP_8,           0x43},  // KP8
  {SDL_SCANCODE_KP_9,           0x44},  // KP9
  {SDL_SCANCODE_KP_MULTIPLY,    0x45},  // KP*
  {SDL_SCANCODE_KP_4,           0x46},  // KP4
  {SDL_SCANCODE_KP_5,           0x47},  // KP5
  {SDL_SCANCODE_KP_6,           0x48},  // KP6
  {SDL_SCANCODE_KP_PLUS,        0x49},  // KP+
  {SDL_SCANCODE_KP_1,           0x4a},  // KP1
  {SDL_SCANCODE_KP_2,           0x4b},  // KP2
  {SDL_SCANCODE_KP_3,           0x4c},  // KP3
  {SDL_SCANCODE_KP_EQUALS,      0x4d},  // KP= (N)
  {SDL_SCANCODE_KP_0,           0x4e},  // KP0
  {SDL_SCANCODE_KP_COMMA,       0x4f},  // KP, (N)
  {SDL_SCANCODE_KP_PERIOD,      0x50},  // KP.
  {SDL_SCANCODE_KP_ENTER,       0x1c},  // KPEnter
#endif
};

/**
 * Serializes
 * @param[in] key Key code
 * @return PC-98 data
 */
#if SDL_MAJOR_VERSION == 1
static UINT8 getKey(SDLKey key) {
#else
static UINT8 getKey(SDL_Scancode key) {
#endif
  size_t i;
  size_t imax;
  SDLKCNV* sdlcnv;

  if(np2oscfg.xrollkey) {
#if SDL_MAJOR_VERSION == 1
    if(key == SDLK_PAGEUP) {
      return key = SDLK_PAGEDOWN;
    } else if(key == SDLK_PAGEDOWN) {
      return key = SDLK_PAGEUP;
    }
#else
    if(key == SDL_SCANCODE_PAGEUP) {
      return key = SDL_SCANCODE_PAGEDOWN;
    } else if(key == SDL_SCANCODE_PAGEDOWN) {
      return key = SDL_SCANCODE_PAGEUP;
    }
#endif
  }

  if(np2oscfg.KEYBOARD == KEY_KEY101) {
    sdlcnv = (SDLKCNV*)sdlcnv101;
    imax = SDL_arraysize(sdlcnv101);
  } else {
    sdlcnv = (SDLKCNV*)sdlcnv106;
    imax = SDL_arraysize(sdlcnv106);
  }

  for (i = 0; i < imax; i++) {
    if (sdlcnv[i].sdlkey == key) {
      return sdlcnv[i].keycode;
    }
  }

  return NC;
}

/**
 * reset Key
 */
void sdlkbd_reset() {
  memset(key_states, 0, sizeof(key_states));
}

/**
 * Key down
 * @param[in] key Key code
 */
void sdlkbd_keydown(UINT key) {
  UINT8   data;

  data = getKey(key);
#if SDL_MAJOR_VERSION == 1
  if(key > 0x40000000) {
    key -= 0x40000000;
    key += 0x80;
  }
#endif
  if(data != NC && !key_states[key]) {
     keystat_keydown(data);
     key_states[key] = 1;
  }
}

/**
 * Key up
 * @param[in] key Key code
 */
void sdlkbd_keyup(UINT key) {
  UINT8   data;

  data = getKey(key);
#if SDL_MAJOR_VERSION == 1
  if(key > 0x40000000) {
    key -= 0x40000000;
    key += 0x80;
  }
#endif
  if(data != NC && key_states[key]) {
     keystat_keyup(data);
     key_states[key] = 0;
  }
}

#endif  // __LIBRETRO__
