
typedef union {
	struct {
		UINT8	pl;
		UINT8	bl;
		UINT8	cl;
		UINT8	ssl;
		UINT8	sur;
		UINT8	sdr;
	} reg;
	UINT8	b[6];
} _CRTC, *CRTC;

typedef union {
	UINT8	b[2];
	UINT16	w;
} PAIR16;

typedef struct {
	UINT32	counter;
	UINT16	mode;
	UINT8	modereg;
	UINT8	padding;
	PAIR16	tile[4];
	UINT32	gdcwithgrcg;
	UINT8	chip;
} _GRCG, *GRCG;


#ifdef __cplusplus
extern "C" {
#endif

void crtc_reset(const NP2CFG *pConfig);
void crtc_bind(void);

void crtc_biosreset(void);

#ifdef __cplusplus
}
#endif

