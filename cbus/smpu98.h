
// ねこ専用ヘッダ

#define SMPU_IMMBUF_W_LEN	32
#define SMPU_IMMBUF_R_LEN	16
#define SMPU_SEQBUF_W_LEN	32
#define SMPU_SEQBUF_R_LEN	32
#define SMPU_MONBUF_LEN		16

typedef struct {
	UINT8	enable;
	UINT8	portBready; // ポートBの使用準備ができていれば1
	UINT8	muteB; // MPU-PC98II互換モードの時、ポートBに出力しない

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

	// for S-MPU Native Mode
	UINT16 native_status;
	UINT16 native_intmask;

	UINT16 native_immbuf_w[SMPU_IMMBUF_W_LEN];
	int native_immbuf_w_pos;
	int native_immbuf_w_len;
	UINT16 native_immbuf_r[SMPU_IMMBUF_R_LEN];
	int native_immbuf_r_pos;
	int native_immbuf_r_len;
	UINT16 native_seqbuf_w[SMPU_SEQBUF_W_LEN];
	int native_seqbuf_w_pos;
	int native_seqbuf_w_len;
	UINT16 native_seqbuf_r[SMPU_SEQBUF_R_LEN];
	int native_seqbuf_r_pos;
	int native_seqbuf_r_len;
	UINT16 native_monbuf[SMPU_MONBUF_LEN];
	int native_monbuf_pos;
	int native_monbuf_len;
	
	UINT8 native_lastmsg;
	UINT8 native_runningmsg;
	UINT8 native_linenum;
	UINT8 native_portnum;
	UINT8 native_tmpbuf[1024];
	
	UINT8 native_immread_lastmsg[2];
	UINT16 native_immread_portbuf[2];
	UINT16 native_immread_phase[2];
} _SMPU98, *SMPU98;


#ifdef __cplusplus
extern "C" {
#endif

extern _SMPU98 smpu98;

void smpu98_midiint(NEVENTITEM item);
void smpu98_midiwaitout(NEVENTITEM item);

void smpu98_construct(void);
void smpu98_destruct(void);

void smpu98_reset(const NP2CFG *pConfig);
void smpu98_bind(void);

void smpu98_callback(void);
void smpu98_midipanic(void);

void smpu98_changeclock(void);

void IOOUTCALL smpu98_o0(UINT port, REG8 dat);
void IOOUTCALL smpu98_o2(UINT port, REG8 dat);
REG8 IOINPCALL smpu98_i0(UINT port);
REG8 IOINPCALL smpu98_i2(UINT port);

void IOOUTCALL smpu98_o4(UINT port, UINT16 dat);
void IOOUTCALL smpu98_o6(UINT port, UINT16 dat);
void IOOUTCALL smpu98_o8(UINT port, UINT16 dat);
void IOOUTCALL smpu98_oa(UINT port, UINT16 dat);
UINT16 IOINPCALL smpu98_i4(UINT port);
UINT16 IOINPCALL smpu98_i6(UINT port);
UINT16 IOINPCALL smpu98_i8(UINT port);
UINT16 IOINPCALL smpu98_ia(UINT port);

extern void (IOOUTCALL *smpu98_io16outfunc[])(UINT port, UINT16 dat);
extern UINT16 (IOINPCALL *smpu98_io16inpfunc[])(UINT port);

#ifdef __cplusplus
}
#endif

