
typedef union {
	UINT8	_b[2];
	UINT16	w;
} EGCWORD;

typedef union {
	UINT8	_b[4][2];
	UINT16	w[4];
	UINT32	d[2];
} EGCQUAD;

typedef struct {
	UINT16	access;
	UINT16	fgbg;
	UINT16	ope;
	UINT16	fg;
	EGCWORD	mask;
	UINT16	bg;
	UINT16	sft;
	UINT16	leng;
	EGCQUAD	lastvram;
	EGCQUAD	patreg;
	EGCQUAD	fgc;
	EGCQUAD	bgc;

	int		func;
	UINT	remain;
	UINT	stack;
	UINT8	*inptr;
	UINT8	*outptr;
	EGCWORD	mask2;
	EGCWORD	srcmask;
	UINT8	srcbit;
	UINT8	dstbit;
	UINT8	sft8bitl;
	UINT8	sft8bitr;

	UINT	padding_b[4];
	UINT8	buf[4096/8 + 4*4];
	UINT	padding_a[4];
} _EGC, *EGC;


#ifdef __cplusplus
extern "C" {
#endif

void egc_reset(const NP2CFG *pConfig);
void egc_bind(void);
void IOOUTCALL egc_w16(UINT port, REG16 value);

#ifdef __cplusplus
}
#endif

