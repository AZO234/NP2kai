#ifndef _NP2_KEYSTAT_H_
#define _NP2_KEYSTAT_H_

#if 0
enum {
	NKEY_ESC			= 0x00,
	NKEY_1				= 0x01,
	NKEY_2				= 0x02,
	NKEY_3				= 0x03,
	NKEY_4				= 0x04,
	NKEY_5				= 0x05,
	NKEY_6				= 0x06,
	NKEY_7				= 0x07,

	NKEY_8				= 0x08,
	NKEY_9				= 0x09,
	NKEY_0				= 0x0a,
	NKEY_MINUS			= 0x0b,
	NKEY_CIRCUMFLEX		= 0x0c,
	NKEY_YEN			= 0x0d,
	NKEY_BACKSPACE		= 0x0e,
	NKEY_TAB			= 0x0f,

	NKEY_Q				= 0x10,
	NKEY_W				= 0x11,
	NKEY_E				= 0x12,
	NKEY_R				= 0x13,
	NKEY_T				= 0x14,
	NKEY_Y				= 0x15,
	NKEY_U				= 0x16,
	NKEY_I				= 0x17,

	NKEY_O				= 0x18,
	NKEY_P				= 0x19,
	NKEY_ATMARK			= 0x1a,
	NKEY_LEFTSBRACKET	= 0x1b,
	NKEY_RETURN			= 0x1c,
	NKEY_A				= 0x1d,
	NKEY_S				= 0x1e,
	NKEY_D				= 0x1f,

	NKEY_F				= 0x20,
	NKEY_G				= 0x21,
	NKEY_H				= 0x22,
	NKEY_J				= 0x23,
	NKEY_K				= 0x24,
	NKEY_L				= 0x25,
	NKEY_SEMICOLON		= 0x26,
	NKEY_COLON			= 0x27,

	NKEY_RIGHTSBRACKET	= 0x28,
	NKEY_Z				= 0x29,
	NKEY_X				= 0x2a,
	NKEY_C				= 0x2b,
	NKEY_V				= 0x2c,
	NKEY_B				= 0x2d,
	NKEY_N				= 0x2e,
	NKEY_M				= 0x2f,

	NKEY_COMMA			= 0x30,
	NKEY_DOT			= 0x31,
	NKEY_SLASH			= 0x32,
	NKEY_UNDERSCORE		= 0x33,
	NKEY_SPACE			= 0x34,
	NKEY_XFER			= 0x35,
	NKEY_ROLLUP			= 0x36,
	NKEY_ROLLDOWN		= 0x37,

	NKEY_INS			= 0x38,
	NKEY_DEL			= 0x39,
	NKEY_UP				= 0x3a,
	NKEY_LEFT			= 0x3b,
	NKEY_RIGHT			= 0x3c,
	NKEY_DOWN			= 0x3d,
	NKEY_HOMECLR		= 0x3e,
	NKEY_HELP			= 0x3f,

	NKEY_KP_MINUS		= 0x40,
	NKEY_KP_SLASH		= 0x41,
	NKEY_KP_7			= 0x42,
	NKEY_KP_8			= 0x43,
	NKEY_KP_9			= 0x44,
	NKEY_KP_ASTERISK	= 0x45,
	NKEY_KP_4			= 0x46,
	NKEY_KP_5			= 0x47,

	NKEY_KP_6			= 0x48,
	NKEY_KP_PLUS		= 0x49,
	NKEY_KP_1			= 0x4a,
	NKEY_KP_2			= 0x4b,
	NKEY_KP_3			= 0x4c,
	NKEY_KP_EQUAL		= 0x4d,
	NKEY_KP_0			= 0x4e,
	NKEY_KP_COMMA		= 0x4f,

	NKEY_KP_DOT			= 0x50,
	NKEY_NFER			= 0x51,
	NKEY_VF1			= 0x52,
	NKEY_VF2			= 0x53,
	NKEY_VF3			= 0x54,
	NKEY_VF4			= 0x55,
	NKEY_VF5			= 0x56,

	NKEY_STOP			= 0x60,
	NKEY_COPY			= 0x61,
	NKEY_F1				= 0x62,
	NKEY_F2				= 0x63,
	NKEY_F3				= 0x64,
	NKEY_F4				= 0x65,
	NKEY_F5				= 0x66,
	NKEY_F6				= 0x67,

	NKEY_F7				= 0x68,
	NKEY_F8				= 0x69,
	NKEY_F9				= 0x6a,
	NKEY_F10			= 0x6b,

	NKEY_SHIFT			= 0x70,
	NKEY_CAPS			= 0x71,
	NKEY_KANA			= 0x72,
	NKEY_GRPH			= 0x73,
	NKEY_CTRL			= 0x74
};
#endif

enum {
	NKEY_SYSTEM			= 0x90,

	NKEY_USER			= 0x90,
	NKEY_USERKEYS		= 2,

	NKEYREF_uPD8255		= 0xf7,
	NKEYREF_USER		= 0xf8,
	NKEYREF_SOFTKBD		= 0xf9,
	NKEYREF_NC			= 0xff
};


typedef struct {
	UINT8	keys;
	UINT8	key[1];
} NKEYM;

typedef struct {
	UINT8	keys;
	UINT8	key[3];
} NKEYM3;

typedef struct {
	UINT8	keys;
	UINT8	key[15];
} NKEYM15;

typedef struct {
	NKEYM3	key[NKEY_SYSTEM];
	NKEYM15	user[NKEY_USERKEYS];
} NKEYTBL;

typedef struct {
	UINT8	reqparam;
	UINT8	mode;
	UINT8	kbdtype;
	UINT8	keyrep;
	UINT8	capsref;
	UINT8	kanaref;
} KEYCTRL;

typedef struct {
	UINT8	ref[0x80];
	UINT8	extkey;
	UINT8	mouselast;
	UINT8	padding;
	UINT8	d_up;
	UINT8	d_dn;
	UINT8	d_lt;
	UINT8	d_rt;
} KEYSTAT;


#ifdef __cplusplus
extern "C" {
#endif

extern	NKEYTBL		nkeytbl;
extern	KEYCTRL		keyctrl;
extern	KEYSTAT		keystat;


void keystat_initialize(void);

void keystat_tblreset(void);
void keystat_tblset(REG8 ref, const UINT8 *key, UINT cnt);
void keystat_tblload(const OEMCHAR *filename);

void keystat_ctrlreset(void);
void keystat_ctrlsend(REG8 dat);

void keystat_keydown(REG8 ref);
void keystat_keyup(REG8 ref);
void keystat_allrelease(void);
void keystat_releaseref(REG8 ref);
void keystat_releasekey(REG8 key);
void keystat_resetjoykey(void);


// ---- I/O

void keystat_down(const UINT8 *key, REG8 keys, REG8 ref);
void keystat_up(const UINT8 *key, REG8 keys, REG8 ref);
void keystat_resendstat(void);
REG8 keystat_getjoy(void);
REG8 keystat_getmouse(SINT16 *x, SINT16 *y);



// ---- 廃止関数

void keystat_senddata(REG8 data);
void keystat_forcerelease(REG8 data);

#ifdef __cplusplus
}
#endif

#endif	/* _NP2_KEYSTAT_H_ */

