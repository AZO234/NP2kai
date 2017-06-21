
#if defined(SUPPORT_SCSI)

typedef struct {
	UINT	port;
	UINT	phase;
	UINT8	reg[0x30];
	UINT8	auxstatus;
	UINT8	scsistatus;
	UINT8	membank;
	UINT8	memwnd;
	UINT8	resent;
	UINT8	datmap;
	UINT	cmdpos;
	UINT	wrdatpos;
	UINT	rddatpos;
	UINT8	cmd[12];
	UINT8	data[0x10000];
	UINT8	bios[2][0x2000];
} _SCSIIO, *SCSIIO;


#ifdef __cplusplus
extern "C" {
#endif

extern	_SCSIIO		scsiio;

void scsiioint(NEVENTITEM item);

void scsiio_reset(const NP2CFG *pConfig);
void scsiio_bind(void);

#ifdef __cplusplus
}
#endif

#endif

