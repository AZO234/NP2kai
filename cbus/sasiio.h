
#if defined(SUPPORT_SASI)

typedef struct {
	UINT8	phase;
	UINT8	ocr;
	UINT8	stat;
	UINT8	error;
	UINT8	unit;
	UINT8	isrint;
	UINT8	cmd[6];
	UINT	cmdpos;
	UINT8	sens[4];
	UINT	senspos;
	UINT	c2pos;
	UINT32	sector;
	UINT	blocks;
	UINT	datpos;
	UINT	datsize;
	UINT8	dat[256];
} _SASIIO, *SASIIO;

#ifdef __cplusplus
extern "C" {
#endif

extern	_SASIIO		sasiio;

void sasiioint(NEVENTITEM item);

REG8 DMACCALL sasi_dmafunc(REG8 func);
REG8 DMACCALL sasi_dataread(void);
void DMACCALL sasi_datawrite(REG8 data);

void sasiio_reset(const NP2CFG *pConfig);
void sasiio_bind(void);

#ifdef __cplusplus
}
#endif

#endif

