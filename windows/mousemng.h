
enum {
	uPD8255A_LEFTBIT	= 0x80,
	uPD8255A_RIGHTBIT	= 0x20
};

typedef struct
{
	UINT32	autohide;
} MOUSEMNGSTAT;


#ifdef __cplusplus
extern "C" {
#endif

extern MOUSEMNGSTAT	mousemngstat;

BRESULT mousemng_checkdinput8();

UINT8 mousemng_getstat(SINT16 *x, SINT16 *y, int clear);
void  mousemng_setstat(SINT16 x, SINT16 y, UINT8 btn);

#ifdef __cplusplus
}
#endif


// ---- for windows

enum {
	MOUSEMNG_LEFTDOWN		= 0,
	MOUSEMNG_LEFTUP,
	MOUSEMNG_RIGHTDOWN,
	MOUSEMNG_RIGHTUP
};

enum {
	MOUSEPROC_SYSTEM		= 0,
	MOUSEPROC_WINUI,
	MOUSEPROC_BG
};

void mousemng_initialize(void);
void mousemng_UIThreadSync(void);
void mousemng_sync(void);
BOOL mousemng_buttonevent(UINT event);
void mousemng_enable(UINT proc);
void mousemng_disable(UINT proc);
void mousemng_toggle(UINT proc);
void mousemng_destroy(void);

UINT8 mousemng_supportrawinput(); // 生データ入力サポート
void mousemng_updatespeed(); // 生データ入力サポート

#ifdef __cplusplus
extern "C" {
#endif
void mousemng_updateclip();
UINT8 mousemng_getabspos(int* x, int* y);
void mousemng_setabspos(int x, int y);
void mousemng_reset(void);
void mousemng_setautohidecursor(int autohide);
int mousemng_getautohidecursor(void);
void mousemng_updateautohidecursor(void);
void mousemng_updatemouseon(int mouseon);
#ifdef __cplusplus
}
#endif