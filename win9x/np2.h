
#if !defined(SUPPORT_PC9821)
#define PROJECTNAME			"Neko Project II/W"
#else
#define PROJECTNAME			"Neko Project 21/W"
#endif

#if !defined(_WIN64)
#define PROJECTSUBNAME		""
#else
#define PROJECTSUBNAME		" x64"
#endif

typedef struct {
	UINT8	port;
	UINT8	def_en;
	UINT8	param;
	UINT32	speed;
	OEMCHAR	mout[MAXPNAMELEN];
	OEMCHAR	min[MAXPNAMELEN];
	OEMCHAR	mdl[64];
	OEMCHAR	def[MAX_PATH];
	UINT8	fixedspeed;
#if defined(SUPPORT_NAMED_PIPE)
	OEMCHAR	pipename[MAX_PATH]; // The name of the named-pipe
	OEMCHAR	pipeserv[MAX_PATH]; // The server name of the named-pipe
#endif
} COMCFG;

typedef struct {
	OEMCHAR	titles[256];
	OEMCHAR	winid[4];

	int		winx;
	int		winy;
	UINT	paddingx;
	UINT	paddingy;
	UINT	fscrn_cx;
	UINT	fscrn_cy;
	UINT8	force400;
	UINT8	WINSNAP;
	UINT8	NOWAIT;
	UINT8	DRAW_SKIP;

	UINT8	background;
	UINT8	DISPCLK;
	UINT8	KEYBOARD;
	UINT8	F12COPY;
	UINT8	USENUMLOCK;

	UINT8	MOUSE_SW;
	UINT8	JOYPAD1;
	UINT8	JOYPAD2;
	UINT8	JOY1BTN[4];

	COMCFG	mpu;
#if defined(SUPPORT_SMPU98)
	COMCFG	smpuA;
	COMCFG	smpuB;
#endif
	COMCFG	com1;
	COMCFG	com2;
	COMCFG	com3;

	UINT32	clk_color1;
	UINT32	clk_color2;
	UINT8	clk_x;
	UINT8	clk_fnt;

	UINT8	comfirm;
	UINT8	shortcut;												// ver0.30

	UINT8	resume;													// ver0.30
	UINT8	statsave;
#if !defined(_WIN64)
	UINT8	disablemmx;
#endif
	UINT8	wintype;
	UINT8	toolwin;
	UINT8	keydisp;
	UINT8	skbdwin;
	UINT8	I286SAVE;
	UINT8	hostdrv_write;
	UINT8	jastsnd;
	UINT8	useromeo;
	UINT8	thickframe;
	UINT8	xrollkey;
	UINT8	fscrnbpp;
	UINT8	fscrnmod;
	UINT8	fsrescfg;

#if defined(SUPPORT_SCRN_DIRECT3D)
	UINT8	d3d_imode; // Direct3D interpolation mode
	UINT8	d3d_exclusive; // Direct3D fullscreen exclusive mode
#endif

	UINT8	cSoundDeviceType;
	TCHAR	szSoundDeviceName[MAX_PATH];

#if defined(SUPPORT_VSTi)
	TCHAR	szVSTiFile[MAX_PATH];
#endif	// defined(SUPPORT_VSTi)

	UINT8	emuddraw; // DirectDraw Emulation Only
	UINT8	drawtype; // Screen renderer type (0: DirectDraw, 1: reserved(DirecrDraw), 2: Direct3D)
	UINT8	dragdrop; // Drag and drop support
	UINT8	makelhdd; // Large HDD creation support
	UINT8	syskhook; // Low-level keyboard hook support
	UINT8	rawmouse; // Raw mouse input support
	SINT16	mousemul; // Mouse speed (mul)
	SINT16	mousediv; // Mouse speed (div)
	
	UINT8	scrnmode; // Screen Mode
	UINT8	savescrn; // Save ScreenMode
	
	UINT8	svscrmul; // Save Screen Size Multiplying Value 
	UINT8	scrn_mul; // Screen Size Multiplying Value (8: default)
	
	UINT8	mouse_nc; // Always notify mouse event
	UINT16	cpustabf; // CPU clock stabilizer frame
	UINT8	readonly; // No save changed settings
	UINT8	usewheel; // Use mouse wheel
	UINT8	tickmode; // Force Set Tick Counter Mode
	UINT8	usemastervolume; // Use Master Volume
	
	UINT8	toolwndhistory; // Number of data of recently opened FD image list in Tool Window
	
#ifdef SUPPORT_WACOM_TABLET
	UINT8	pentabfa; // Pen tablet fixed aspect mode
#endif	// defined(SUPPORT_WACOM_TABLET)
} NP2OSCFG;


enum {
	FULLSCREEN_WIDTH	= 640,
	FULLSCREEN_HEIGHT	= 480
};

enum {
	NP2BREAK_MAIN		= 0x01,
	NP2BREAK_DEBUG		= 0x02
};

enum {
	WM_NP2CMD			= (WM_USER + 200)
};

enum {
	NP2CMD_EXIT			= 0,
	NP2CMD_RESET		= 1,
	NP2CMD_EXIT2		= 0x0100,
	NP2CMD_DUMMY		= 0xffff
};

enum {
	MMXFLAG_DISABLE		= 1,
	MMXFLAG_NOTSUPPORT	= 2
};


extern	NP2OSCFG	np2oscfg;
extern	HWND		g_hWndMain;
extern	UINT8		np2break;
extern	BOOL		winui_en;
extern	UINT8		g_scrnmode;
#if !defined(_WIN64)
extern	int			mmxflag;
#endif

extern	OEMCHAR		modulefile[MAX_PATH];
extern	OEMCHAR		fddfolder[MAX_PATH];
extern	OEMCHAR		hddfolder[MAX_PATH];
extern	OEMCHAR		cdfolder[MAX_PATH];
extern	OEMCHAR		bmpfilefolder[MAX_PATH];
extern	OEMCHAR		npcfgfilefolder[MAX_PATH];

void np2active_renewal(void);
void unloadNP2INI();
void loadNP2INI(const OEMCHAR *fname);

