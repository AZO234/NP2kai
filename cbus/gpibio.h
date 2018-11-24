
#if defined(SUPPORT_GPIB)

enum {
	GPIB_MODE_MASTER	= 1,
	GPIB_MODE_SLAVE		= 0,
};

typedef struct {
	UINT8 enable;
	UINT8 irq; // 割り込み
	UINT8 mode; // マスタ=1, スレーブ=0
	UINT8 gpibaddr; // GP-IBアドレス(0x0～0x1F)
	UINT8 ifcflag; // IFC# 1=アクティブ, 0=非アクティブ

	UINT16 exiobase;
} _GPIB, *GPIB;

#ifdef __cplusplus
extern "C" {
#endif

extern	_GPIB		gpib;

void gpibint(NEVENTITEM item);

void gpibio_initialize(void);
void gpibio_shutdown(void);

void gpibio_reset(const NP2CFG *pConfig);
void gpibio_bind(void);

#ifdef __cplusplus
}
#endif

#endif

