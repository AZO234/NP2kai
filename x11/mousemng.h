#ifndef	NP2_X11_MOUSEMNG_H__
#define	NP2_X11_MOUSEMNG_H__

G_BEGIN_DECLS

#define	MOUSE_MASK	0x07

#define	M_RES		0x00
#define	M_XOR		0x40
#define	M_SET		0x80


#define	MOUSE_OFF	(M_RES | 0)
#define MOUSE_ON	(M_SET | 0)
#define	MOUSE_XOR	(M_XOR | 0)
#define	MOUSE_CONT	(M_RES | 1)
#define	MOUSE_STOP	(M_SET | 1)
#define	MOUSE_CONT_M	(M_RES | 2)
#define	MOUSE_STOP_M	(M_SET | 2)


#define	MOUSE_LEFTDOWN	0
#define	MOUSE_LEFTUP	1
#define	MOUSE_RIGHTDOWN	2
#define	MOUSE_RIGHTUP	3


UINT8 mousemng_getstat(short *x, short *y, int clear);
void mousemng_callback(void);

UINT8 mouse_flag(void);
void mouse_running(UINT8 flg);
UINT8 mouse_btn(UINT8 btn);

/* for X11 */
BRESULT mousemng_initialize(void);
void mousemng_term(void);
void mousemng_set_ratio(UINT8);

enum {
	MOUSE_RATIO_050 = 0x12,
	MOUSE_RATIO_075 = 0x34,
	MOUSE_RATIO_150 = 0x32,
	MOUSE_RATIO_200 = 0x21,
	MOUSE_RATIO_400 = 0x41,
	MOUSE_RATIO_800 = 0x81,
	MOUSE_RATIO_100 = 0
};

G_END_DECLS

#endif	/* NP2_X11_MOUSEMNG_H__ */
