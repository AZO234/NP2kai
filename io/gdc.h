
#define	GDCCMD_MAX	32

typedef struct {
	UINT8	para[256];
	UINT16	fifo[GDCCMD_MAX];
	UINT16	cnt;
	UINT8	ptr;
	UINT8	rcv;
	UINT8	snd;
	UINT8	cmd;
	UINT8	paracb;
	UINT8	reserved;
} _GDCDATA, *GDCDATA;

typedef struct {
	_GDCDATA	m;
	_GDCDATA	s;
	UINT8		mode1;
	UINT8		mode2;
	UINT8		clock;
	UINT8		crt15khz;
	UINT8		m_drawing;
	UINT8		s_drawing;
	UINT8		vsync;
	UINT8		vsyncint;
	UINT8		display;
	UINT8		bitac;
	UINT8		ff2;
	UINT8		reserved;
	int			analog;
	int			palnum;
	UINT8		degpal[4];
	RGB32		anapal[16];
	UINT32		dispclock;
	UINT32		vsyncclock;
	UINT32		rasterclock;
	UINT32		hsyncclock;

	UINT32		hclock;
	UINT32		vclock;

#if defined(SUPPORT_PC9821)
	UINT8		anareg[16*3 + 256*4];
#endif
} _GDC, *GDC;

typedef struct {
	UINT8	access;
	UINT8	disp;
	UINT8	textdisp;
	UINT8	msw_accessable;
	UINT8	grphdisp;
	UINT8	palchange;
	UINT8	mode2;
} _GDCS, *GDCS;

enum {
	GDC_DEGBBIT			= 0x01,
	GDC_DEGRBIT			= 0x02,
	GDC_DEGGBIT			= 0x04,

	GDCSCRN_ENABLE		= 0x80,
	GDCSCRN_EXT			= 0x40,
	GDCSCRN_ALLDRAW		= 0x04,
	GDCSCRN_REDRAW		= 0x01,
	GDCSCRN_ALLDRAW2	= 0x0c,
	GDCSCRN_REDRAW2		= 0x03,
	GDCSCRN_MAKE		= (GDCSCRN_ALLDRAW | GDCSCRN_REDRAW),

	GDCWORK_MASTER		= 0,
	GDCWORK_SLAVE		= 1,

	GDCANALOG_16		= 0,
	GDCANALOG_256		= 1,
	GDCANALOG_256E		= 2,

	GDCDISP_PLAZMA		= 0,
	GDCDISP_ANALOG		= 1,
	GDCDISP_PLAZMA2		= 2,
	GDCDISP_15			= 6,
	GDCDISP_31			= 7
};


#ifdef __cplusplus
extern "C" {
#endif

void gdc_reset(const NP2CFG *pConfig);
void gdc_bind(void);

void gdc_vectreset(GDCDATA item);
void gdc_work(int id);
void gdc_forceready(int id);
void gdc_paletteinit(void);

void gdc_setdegitalpal(int color, REG8 value);
void gdc_setdegpalpack(int color, REG8 value);
void gdc_setanalogpal(int color, int rgb, REG8 value);
void gdc_setanalogpalall(const UINT16 *paltbl);

#if defined(SUPPORT_PC9821)
void gdc_analogext(BOOL extend);
#endif

void gdc_biosreset(void);
void gdc_updateclock(void);
void gdc_restorekacmode(void);

#ifdef __cplusplus
}
#endif

