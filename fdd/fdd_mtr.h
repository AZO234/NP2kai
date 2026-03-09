
typedef struct {
	int		busy;
	UINT8	head[4];
	UINT	nextevent;
	UINT8	curevent;
} _FDDMTR, *FDDMTR;


#ifdef __cplusplus
extern "C" {
#endif

extern	_FDDMTR		fddmtr;

void fdbiosout(NEVENTITEM item);

void fddmtr_initialize(void);
void fddmtr_callback(UINT time);
void fddmtr_seek(REG8 drv, REG8 c, UINT size);
void fddmtr_reset(void);


#if defined(SUPPORT_SWSEEKSND)
void fddmtrsnd_initialize(UINT rate);
void fddmtrsnd_bind(void);
void fddmtrsnd_deinitialize(void);
#else
#define	fddmtrsnd_initialize(r)
#define	fddmtrsnd_bind()
#define	fddmtrsnd_deinitialize()
#endif

#ifdef __cplusplus
}
#endif

