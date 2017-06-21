
typedef struct {
	uPD8255	upd8255;
	UINT32	lastc;
	UINT32	intrclock;
	UINT32	moveclock;
	SINT16	x;
	SINT16	y;
	SINT16	rx;
	SINT16	ry;
	SINT16	sx;
	SINT16	sy;
	SINT16	latch_x;
	SINT16	latch_y;
	UINT8	timing;
	UINT8	rapid;
	UINT8	b;
} _MOUSEIF, *MOUSEIF;


#ifdef __cplusplus
extern "C" {
#endif

void mouseif_reset(const NP2CFG *pConfig);
void mouseif_bind(void);
void mouseif_sync(void);
void mouseint(NEVENTITEM item);

#ifdef __cplusplus
}
#endif

