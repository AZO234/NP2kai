
// ‚Ë‚±ê—pƒwƒbƒ_

enum {
	MPU98_EXCVBUFS		= 512,
	MPU98_RECVBUFS		= (1 << 7)
};

#define	MPUTRDATAS	4

typedef struct {
	UINT8	phase;
	UINT8	step;
	UINT8	cmd;
	UINT8	rstat;
	UINT	datapos;
	UINT	datacnt;
	UINT8	data[MPU98_EXCVBUFS];
} MPUCMDS;

typedef struct {
	UINT8	step;
	UINT8	datas;
	UINT8	remain;
	UINT8	rstat;
	UINT8	recv;
	UINT8	padding[2];
	UINT8	data[MPUTRDATAS];
} MPUTR;

typedef struct {
	int		cnt;
	int		pos;
	UINT8	buf[MPU98_RECVBUFS];
} MPURECV;

typedef struct {
	UINT16	port;
	UINT8	irqnum;
	UINT8	data;

	UINT32	xferclock;
	SINT32	stepclock;

	UINT8	intphase;
	UINT8	intreq;
	UINT8	hclk_rem;
	UINT8	hclk_cnt;
	UINT8	hclk_step[4];

	UINT8	acttr;
	UINT8	status;
	UINT8	mode;
	UINT8	flag1;
	UINT8	flag2;

	UINT8	tempo;
	UINT8	reltempo;
	UINT8	curtempo;
	UINT8	inttimebase;

	UINT8	recvevent;
	UINT8	remainstep;
	UINT8	syncmode;
	UINT8	metromode;

	UINT8	midipermetero;
	UINT8	meteropermeas;
	UINT8	sendplaycnt;

	UINT	accch;

	MPURECV	r;

	MPUCMDS	cmd;
	MPUTR	tr[8];
	MPUCMDS	cond;
} _MPU98II, *MPU98II;


#ifdef __cplusplus
extern "C" {
#endif

extern _MPU98II mpu98;

void midiint(NEVENTITEM item);
void midiwaitout(NEVENTITEM item);

void mpu98ii_construct(void);
void mpu98ii_destruct(void);

void mpu98ii_reset(const NP2CFG *pConfig);
void mpu98ii_bind(void);

void mpu98ii_callback(void);
void mpu98ii_midipanic(void);

#ifdef __cplusplus
}
#endif

