
typedef struct {
	UINT8	ctrl;
	UINT8	ch;
	UINT8	flag;
	UINT8	stat;
	UINT16	value;
	UINT16	latch;
} _PITCH, *PITCH;

#if !defined(DISABLE_SOUND)
typedef struct {
	_PITCH	ch[5];
} _PIT, *PIT;
#else
typedef struct {
	_PITCH	ch[3];
} _PIT, *PIT;
#endif

enum {
	PIT_CTRL_BCD	= 0x01,
	PIT_CTRL_MODE	= 0x0e,
	PIT_CTRL_RL		= 0x30,
	PIT_CTRL_SC		= 0xc0,

	PIT_RL_L		= 0x10,
	PIT_RL_H		= 0x20,
	PIT_RL_ALL		= 0x30,

	PIT_STAT_CMD	= 0x40,
	PIT_STAT_INT	= 0x80,

	PIT_FLAG_R		= 0x01,
	PIT_FLAG_W		= 0x02,
	PIT_FLAG_L		= 0x04,
	PIT_FLAG_S		= 0x08,
	PIT_FLAG_C		= 0x10,
	PIT_FLAG_I		= 0x20,

	PIT_LATCH_S		= 0x10,
	PIT_LATCH_C		= 0x20
};


#ifdef __cplusplus
extern "C" {
#endif

void systimer(NEVENTITEM item);
void beeponeshot(NEVENTITEM item);
void rs232ctimer(NEVENTITEM item);

void pit_setflag(PITCH pitch, REG8 value);
BOOL pit_setcount(PITCH pitch, REG8 value);
UINT pit_getcount(PITCH pitch);
REG8 pit_getstat(PITCH pitch);

void itimer_reset(const NP2CFG *pConfig);
void itimer_bind(void);

#ifdef __cplusplus
}
#endif

