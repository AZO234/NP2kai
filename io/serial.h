
enum {
	KB_CTR			= (1 << 3),
	KB_CTRMASK		= (KB_CTR - 1),

	KB_BUF			= (1 << 7),
	KB_BUFMASK		= (KB_BUF - 1)
};

typedef struct {
	UINT32	xferclock;
	UINT8	data;
	UINT8	cmd;
	UINT8	mode;
	UINT8	status;
	UINT	ctrls;
	UINT	ctrpos;
	UINT	buffers;
	UINT	bufpos;
	UINT8	ctr[KB_CTR];
	UINT8	buf[KB_BUF];
} _KEYBRD, *KEYBRD;

typedef struct {
	UINT8	result;
	UINT8	data;
	UINT8	send;
	UINT8	pad;
	UINT	pos;
	UINT	dummyinst;
	UINT	mul;
} _RS232C, *RS232C;



#ifdef __cplusplus
extern "C" {
#endif

void keyboard_callback(NEVENTITEM item);

void keyboard_reset(const NP2CFG *pConfig);
void keyboard_bind(void);
void keyboard_resetsignal(void);
void keyboard_ctrl(REG8 data);
void keyboard_send(REG8 data);



void rs232c_construct(void);
void rs232c_destruct(void);

void rs232c_reset(const NP2CFG *pConfig);
void rs232c_bind(void);
void rs232c_open(void);
void rs232c_callback(void);

UINT8 rs232c_stat(void);
void rs232c_midipanic(void);

#ifdef __cplusplus
}
#endif

