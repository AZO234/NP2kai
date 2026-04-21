/* === NP2kai wx port - main emulation module === */

#include <compiler.h>
#include "np2.h"
#include "dosio.h"
#include "commng.h"
#include "fontmng.h"
#include "inputmng.h"
#include "joymng.h"
#include "kbdmng.h"
#include "kbtrans.h"
#include "mousemng.h"
#include "scrnmng.h"
#include "soundmng.h"
#include "sysmng.h"
#include "taskmng.h"
#include "ini.h"
#include <pccore.h>
#include <statsave.h>
#include <io/iocore.h>
#include <vram/scrndraw.h>
#include <sound/s98.h>
#include <fdd/sxsi.h>
#include <fdd/diskdrv.h>
#include <timing.h>
#include <keystat.h>
#include <np2_tickcount.h>
#include <codecnv/codecnv.h>
#include <string.h>
#include <stdlib.h>
#include <embed/menubase/menubase.h>
#if defined(SUPPORT_NET)
#include <network/net.h>
#endif
#if defined(SUPPORT_WAB)
#include <wab/wab.h>
#endif
#if defined(SUPPORT_CL_GD5430)
#include <wab/cirrus_vga_extern.h>
#endif

/* ---- globals ---- */

NP2OSCFG np2oscfg = {
	"",         /* titles */
	0, 0,       /* paddingx, paddingy */
	0,          /* NOWAIT */
	0,          /* DRAW_SKIP */
	0,          /* DISPCLK */
	KEY_KEY106, /* KEYBOARD */
	0,          /* F12KEY */
	0,          /* MOUSE_SW */
	0, 0,       /* JOYPAD1, JOYPAD2 */
	{1,2,5,6},  /* JOY1BTN */
	{{0,1},{0,1}}, /* JOYAXISMAP */
	{{0,1,0xff,0xff},{0,1,0xff,0xff}}, /* JOYBTNMAP */
	{"",""},    /* JOYDEV */
	{FALSE, COMPORT_MIDI, 0, 0x3e, 19200, "", "", "", ""},  /* mpu */
#if defined(SUPPORT_SMPU98)
	{FALSE, COMPORT_MIDI, 0, 0x3e, 19200, "", "", "", ""},  /* smpuA */
	{FALSE, COMPORT_MIDI, 0, 0x3e, 19200, "", "", "", ""},  /* smpuB */
#endif
	{
		{TRUE, COMPORT_NONE, 0, 0x3e, 19200, "", "", "", ""},
		{TRUE, COMPORT_NONE, 0, 0x3e, 19200, "", "", "", ""},
		{TRUE, COMPORT_NONE, 0, 0x3e, 19200, "", "", "", ""},
	}, /* com[3] */
	0,          /* confirm */
	0,          /* resume */
	0, 0, 0, 0, 0, /* statsave, toolwin, keydisp, softkbd, hostdrv_write */
	0, 0, 1,    /* jastsnd, I286SAVE, xrollkey */
	SNDDRV_SDL, /* snddrv */
	{"",""},    /* MIDIDEV */
#if defined(SUPPORT_SMPU98)
	{"",""},    /* MIDIDEVA */
	{"",""},    /* MIDIDEVB */
#endif
	0,          /* MIDIWAIT */
	100,        /* mouse_move_ratio */
	0, INTERP_NEAREST, 0, /* disablemmx, drawinterp, F11KEY */
	0,          /* readonly */
	-1, -1,     /* winx, winy */
	640, 400,   /* winwidth, winheight */
};

BOOL  s98logging   = FALSE;
int   s98log_count = 0;

char  hddfolder[MAX_PATH]    = "";
char  fddfolder[MAX_PATH]    = "";
char  cdfolder[MAX_PATH]     = "";
char  bmpfilefolder[MAX_PATH]= "";
UINT  bmpfilenumber          = 0;
char  modulefile[MAX_PATH]   = "";
char  draw32bit              = 0;
UINT8 scrnmode               = 0;
UINT8 changescreeninit       = 0;

int   np2_stateslotnow       = 0;
int   mmxflag                = 0;

int havemmx(void) { return 0; }

void np2oscfg_setdefault(void)
{
	static const NP2OSCFG def = {
		"",
		0, 0,
		0,
		0,
		0,
		KEY_KEY106,
		0,
		0,
		0, 0,
		{1,2,5,6},
		{{0,1},{0,1}},
		{{0,1,0xff,0xff},{0,1,0xff,0xff}},
		{"",""},
		{FALSE, COMPORT_MIDI, 0, 0x3e, 19200, "", "", "", ""},
#if defined(SUPPORT_SMPU98)
		{FALSE, COMPORT_MIDI, 0, 0x3e, 19200, "", "", "", ""},
		{FALSE, COMPORT_MIDI, 0, 0x3e, 19200, "", "", "", ""},
#endif
		{
			{TRUE, COMPORT_NONE, 0, 0x3e, 19200, "", "", "", ""},
			{TRUE, COMPORT_NONE, 0, 0x3e, 19200, "", "", "", ""},
			{TRUE, COMPORT_NONE, 0, 0x3e, 19200, "", "", "", ""},
		},
		0,
		0,
		0, 0, 0, 0, 0,
		0, 0, 1,
		SNDDRV_SDL,
		{"",""},
#if defined(SUPPORT_SMPU98)
		{"",""},
		{"",""},
#endif
		0,
		100,
		0, INTERP_NEAREST, 0,
		0,
		-1, -1,
		640, 400,
	};
	np2oscfg = def;
}

extern "C" {
#if defined(SUPPORT_WAB)
#include <wab/wab.h>
#endif
}

void np2wabcfg_setdefault(void)
{
#if defined(SUPPORT_WAB)
	memset(&np2wabcfg, 0, sizeof(np2wabcfg));
	np2wabcfg.posx = 0;
	np2wabcfg.posy = 0;
	np2wabcfg.multithread = 1;
	np2wabcfg.multiwindow = 0;
	np2wabcfg.halftone = 0;
#endif
}

/* ---- state save ---- */

static void getstatfilename(OEMCHAR *path, const OEMCHAR *ext, int size)
{
	OEMCHAR filename[64];
	OEMSNPRINTF(filename, sizeof(filename), "%s.%s", NP2_WX_APPNAME, ext);
	file_cpyname(path, file_getcd(filename), size);
}

int flagsave(const OEMCHAR *ext)
{
	OEMCHAR path[MAX_PATH];
	getstatfilename(path, ext, sizeof(path));
	int ret = statsave_save(path);
	if (ret) file_delete(path);
	return ret;
}

void flagdelete(const OEMCHAR *ext)
{
	OEMCHAR path[MAX_PATH];
	getstatfilename(path, ext, sizeof(path));
	file_delete(path);
}

int flagload(const OEMCHAR *ext, const OEMCHAR *title, BOOL force)
{
	OEMCHAR path[MAX_PATH];
	OEMCHAR buf[1024];
	OEMCHAR buf2[1024 + 256];

	getstatfilename(path, ext, sizeof(path));
	int id  = DID_YES;
	int ret = statsave_check(path, buf, sizeof(buf));
	if (ret & (~STATFLAG_DISKCHG)) {
		/* failed */
		id = DID_NO;
	} else if (!force && (ret & STATFLAG_DISKCHG)) {
		OEMSNPRINTF(buf2, sizeof(buf2), "Conflict!\n\n%s\nContinue?", buf);
		/* show modal question from UI thread - simplified: always yes */
		id = DID_YES;
	}
	if (id == DID_YES) {
		statsave_load(path);
	}
	return id;
}

/* ---- screen mode change ---- */

void changescreen(UINT8 newmode)
{
	UINT8 change  = scrnmode ^ newmode;
	UINT8 renewal = change & SCRNMODE_FULLSCREEN;
	if (newmode & SCRNMODE_FULLSCREEN) {
		renewal |= change & SCRNMODE_HIGHCOLOR;
	} else {
		renewal |= change & SCRNMODE_ROTATEMASK;
	}
	if (renewal) {
		changescreeninit = 1;
		soundmng_stop();
		scrnmng_destroy();
		if (scrnmng_create(newmode) == SUCCESS) {
			scrnmode = newmode;
		} else {
			scrnmng_create(scrnmode);
		}
		changescreeninit = 0;
		scrndraw_redraw();
		soundmng_play();
	} else {
		scrnmode = newmode;
	}
}

/* ---- autokey: clipboard text paste (Shift_JIS byte stream) ---- */

/* PC-98 scan codes for ASCII characters, mirrors Windows port's vkeylist */
static UINT8 s_vkeylist[256];
static UINT8 s_shift_on[256];

static void autokey_init_table(void)
{
	static int s_done = 0;
	if (s_done) return;
	s_done = 1;

	/* digits */
	static const char numkeys[] = {0,'!','"','#','$','%','&','\'','(',')' };
	for (int i = '0'; i <= '9'; i++) {
		s_vkeylist[(UINT8)i] = (UINT8)(i - '0');
		if (i == '0') s_vkeylist[(UINT8)i] = 0x0a;
		s_vkeylist[(UINT8)numkeys[i-'0']] = s_vkeylist[(UINT8)i];
		s_shift_on[(UINT8)numkeys[i-'0']] = 1;
	}

	/* A-Z / a-z */
	static const UINT8 asckeycode[] = {
		0x1d,0x2d,0x2b,0x1f,0x12,0x20,0x21,0x22,0x17,0x23,
		0x24,0x25,0x2f,0x2e,0x18,0x19,0x10,0x13,0x1e,0x14,
		0x16,0x2c,0x11,0x2a,0x15,0x29
	};
	for (int i = 'A'; i <= 'Z'; i++) {
		s_vkeylist[(UINT8)i]        = asckeycode[i - 'A'];
		s_shift_on[(UINT8)i]        = 1;
		s_vkeylist[(UINT8)(i+0x20)] = asckeycode[i - 'A'];
	}

	/* punctuation */
	static const char spkeyascii[] = { '-','^','\\','@','[',';',':',']',',','.','/', '_' };
	static const char spshascii[]  = { '=','`', '|','~','{','+','*','}','<','>','?', '_' };
	static const UINT8 spkeycode[] = { 0x0B,0x0C,0x0D,0x1A,0x1B,0x26,0x27,0x28,0x30,0x31,0x32,0x33 };
	s_vkeylist[(UINT8)' ']  = 0x34;
	s_vkeylist[(UINT8)'\t'] = 0x0f;
	s_vkeylist[(UINT8)'\n'] = 0x1c;
	for (int i = 0; i < (int)(sizeof(spkeyascii)/sizeof(spkeyascii[0])); i++) {
		s_vkeylist[(UINT8)spkeyascii[i]] = spkeycode[i];
		s_vkeylist[(UINT8)spshascii[i]]  = spkeycode[i];
		s_shift_on[(UINT8)spshascii[i]]  = 1;
	}

	/* half-width kana (0xA1-0xDF) */
	static const UINT8 kanakeycode[] = {
		0x31,0x1b,0x28,0x30,0x32,0x0a,0x03,0x12,0x04,0x05,
		0x06,0x07,0x08,0x09,0x29,0x0d,0x03,0x12,0x04,0x05,
		0x06,0x14,0x21,0x22,0x27,0x2d,0x2a,0x1f,0x13,0x19,
		0x2b,0x10,0x1d,0x29,0x11,0x1e,0x16,0x17,0x01,0x30,
		0x24,0x20,0x2c,0x02,0x0c,0x0b,0x23,0x2e,0x28,0x32,
		0x2f,0x07,0x08,0x09,0x18,0x25,0x31,0x26,0x33,0x0a,
		0x15,0x1a,0x1b
	};
	for (int i = 0xa1; i <= 0xdf; i++) {
		s_vkeylist[i] = kanakeycode[i - 0xa1];
		s_shift_on[i] = (i <= 0xaf) ? 1 : 0;
	}
}

static unsigned short sjis_to_jis(unsigned short sjis)
{
	int h = sjis >> 8;
	int l = sjis & 0xff;
	if (h <= 0x9f) {
		h = (l < 0x9f) ? (h << 1) - 0xe1 : (h << 1) - 0xe0;
	} else {
		h = (l < 0x9f) ? (h << 1) - 0x161 : (h << 1) - 0x160;
	}
	if      (l < 0x7f) l -= 0x1f;
	else if (l < 0x9f) l -= 0x20;
	else               l -= 0x7e;
	return (unsigned short)((h << 8) | l);
}

static char   *s_autokey_buf    = NULL;
static int     s_autokey_len    = 0;
static int     s_autokey_pos    = 0;
static int     s_autokey_shift  = 0;
static int     s_autokey_kanji  = 0;  /* PC-98 kanji input mode active */
static int     s_autokey_kana   = 0;  /* saved kana state at start */

void autokey_start(const char *sjis_text)
{
	autokey_init_table();
	if (s_autokey_buf) {
		free(s_autokey_buf);
		s_autokey_buf = NULL;
	}
	int len = (int)strlen(sjis_text);
	s_autokey_buf = (char *)malloc(len + 1);
	if (!s_autokey_buf) return;
	memcpy(s_autokey_buf, sjis_text, len + 1);
	s_autokey_len   = len;
	s_autokey_pos   = 0;
	s_autokey_shift = 0;
	s_autokey_kanji = 0;
	s_autokey_kana  = keyctrl.kanaref;
	/* release any held shift first */
	keystat_senddata(0x80 | 0x70);
}

static void autokey_exec(void)
{
	if (!s_autokey_buf || s_autokey_pos >= s_autokey_len) return;
	if (keybrd.buffers >= KB_BUF / 2) return;  /* buffer nearly full, wait */

	UINT8 c = (UINT8)s_autokey_buf[s_autokey_pos];
	if (c == 0) { s_autokey_pos++; return; }

	static const UINT8 hexToAsc[] = {
		'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'
	};

	if (c <= 0x7f) {
		/* ASCII */
		if (s_autokey_kanji) {
			/* exit kanji mode: CTRL+XFER off */
			keystat_senddata(0x00 | 0x74);
			keystat_senddata(0x00 | 0x35);
			keystat_senddata(0x80 | 0x35);
			keystat_senddata(0x80 | 0x74);
			s_autokey_kanji = 0;
		}
		if (keyctrl.kanaref != 0xff) {
			/* turn off kana */
			keystat_senddata(0x00 | 0x72);
			keystat_senddata(0x80 | 0x72);
		}
		if (s_vkeylist[c]) {
			if (s_shift_on[c] && !s_autokey_shift) {
				keystat_senddata(0x00 | 0x70);
				s_autokey_shift = 1;
			}
			if (!s_shift_on[c] && s_autokey_shift) {
				keystat_senddata(0x80 | 0x70);
				s_autokey_shift = 0;
			}
			keystat_senddata(0x00 | s_vkeylist[c]);
			keystat_senddata(0x80 | s_vkeylist[c]);
		}
		s_autokey_pos++;
	} else if (c >= 0xa1 && c <= 0xdf) {
		/* half-width kana */
		if (s_autokey_kanji) {
			keystat_senddata(0x00 | 0x74);
			keystat_senddata(0x00 | 0x35);
			keystat_senddata(0x80 | 0x35);
			keystat_senddata(0x80 | 0x74);
			s_autokey_kanji = 0;
		}
		if (keyctrl.kanaref == 0xff) {
			/* turn on kana */
			keystat_senddata(0x00 | 0x72);
			keystat_senddata(0x80 | 0x72);
		}
		if (s_vkeylist[c]) {
			if (s_shift_on[c] && !s_autokey_shift) {
				keystat_senddata(0x00 | 0x70);
				s_autokey_shift = 1;
			}
			if (!s_shift_on[c] && s_autokey_shift) {
				keystat_senddata(0x80 | 0x70);
				s_autokey_shift = 0;
			}
			keystat_senddata(0x00 | s_vkeylist[c]);
			keystat_senddata(0x80 | s_vkeylist[c]);
		}
		s_autokey_pos++;
	} else if (c >= 0x80 && s_autokey_pos + 1 < s_autokey_len) {
		/* 2-byte Shift_JIS kanji */
		UINT8 c2 = (UINT8)s_autokey_buf[s_autokey_pos + 1];
		if (s_autokey_shift) {
			keystat_senddata(0x80 | 0x70);
			s_autokey_shift = 0;
		}
		if (keyctrl.kanaref != 0xff) {
			keystat_senddata(0x00 | 0x72);
			keystat_senddata(0x80 | 0x72);
		}
		if (!s_autokey_kanji) {
			/* enter kanji mode: CTRL+XFER */
			keystat_senddata(0x00 | 0x74);
			keystat_senddata(0x00 | 0x35);
			keystat_senddata(0x80 | 0x35);
			keystat_senddata(0x80 | 0x74);
			s_autokey_kanji = 1;
		}
		unsigned short jis = sjis_to_jis(((unsigned short)c << 8) | c2);
		/* type 4 hex digits of JIS code */
		for (int b = 12; b >= 0; b -= 4) {
			UINT8 hc = hexToAsc[(jis >> b) & 0xf];
			keystat_senddata(0x00 | s_vkeylist[hc]);
			keystat_senddata(0x80 | s_vkeylist[hc]);
		}
		s_autokey_pos += 2;
	} else {
		s_autokey_pos++;
	}

	/* check completion */
	if (s_autokey_pos >= s_autokey_len) {
		if (s_autokey_kanji) {
			keystat_senddata(0x00 | 0x74);
			keystat_senddata(0x00 | 0x35);
			keystat_senddata(0x80 | 0x35);
			keystat_senddata(0x80 | 0x74);
			s_autokey_kanji = 0;
		}
		/* restore kana state if it changed */
		if ((s_autokey_kana != 0xff) != (keyctrl.kanaref != 0xff)) {
			keystat_senddata(0x00 | 0x72);
			keystat_senddata(0x80 | 0x72);
		}
		keystat_senddata(0x80 | 0x70);  /* release shift */
		s_autokey_len = 0;
		free(s_autokey_buf);
		s_autokey_buf = NULL;
	}
}

/* ---- emulation step (called periodically from np2frame timer) ---- */

static UINT framecnt = 0;
static UINT waitcnt  = 0;
static UINT framemax = 1;

static void processwait(UINT cnt)
{
	if (timing_getcount() >= cnt) {
		timing_setcount(0);
		framecnt = 0;
	} else {
		taskmng_sleep(1);
	}
}

void np2_exec(void)
{
	taskmng_rol();
	autokey_exec();

	if (np2oscfg.NOWAIT) {
		joymng_sync();
		pccore_exec(framecnt == 0);
		if (np2oscfg.DRAW_SKIP) {				// nowait frame skip
			framecnt++;
			if (framecnt >= (UINT)np2oscfg.DRAW_SKIP) {
				processwait(0);
			}
		}
		else {									// nowait auto skip
			framecnt = 1;
			if (timing_getcount()) {
				processwait(0);
			}
		}
	}
	else if (np2oscfg.DRAW_SKIP) {				// frame skip
		if (framecnt < (UINT)np2oscfg.DRAW_SKIP) {
			joymng_sync();
			pccore_exec(framecnt == 0);
			framecnt++;
		}
		else {
			processwait(np2oscfg.DRAW_SKIP);
		}
	}
	else {										// auto skip
		if (!waitcnt) {
			UINT cnt;
			joymng_sync();
			pccore_exec(framecnt == 0);
			framecnt++;
			cnt = timing_getcount();
			if (framecnt > cnt) {
				waitcnt = framecnt;
				if (framemax > 1) {
					framemax--;
				}
			}
			else if (framecnt >= framemax) {
				if (framemax < 12) {
					framemax++;
				}
				if (cnt >= 12) {
					timing_reset();
				}
				else {
					timing_setcount(cnt - framecnt);
				}
				framecnt = 0;
				soundmng_sync();
				joymng_sync();
			}
		}
		else {
			processwait(waitcnt);
			waitcnt = framecnt;
		}
	}

#if defined(SUPPORT_S98)
	if (s98logging) S98_sync();
#endif
}

/* ---- initialization / termination ---- */

BRESULT np2_initialize(const char *argv0)
{
	/* Initialize subsystems */
#if defined(SUPPORT_NP2_TICKCOUNT)
	NP2_TickCount_Initialize();
#endif

	dosio_init();

	/* determine base path from executable location */
	char base[MAX_PATH];
	milstr_ncpy(base, argv0, MAX_PATH);
	file_cutname(base);
	file_setcd(base);

	/* load configuration */
	initload();

	/* If biospath not configured, default it to the config directory
	 * (where the user places ROM files alongside wxnp21kai.toml) */
	if (np2cfg.biospath[0] == '\0') {
		char inipath[MAX_PATH];
		initgetfile(inipath, sizeof(inipath));
		milstr_ncpy(np2cfg.biospath, inipath, MAX_PATH);
		file_cutname(np2cfg.biospath);
		/* Also redirect file_setcd so font_load finds ROM files there */
		file_setcd(np2cfg.biospath);
	}

	/* screen */
	scrnmng_initialize();
	if (scrnmng_create(scrnmode) != SUCCESS) {
		fprintf(stderr, "scrnmng_create failed\n");
		return FAILURE;
	}

	/* font */
	fontmng_init();
	fontmng_setdeffontname(NULL);

	/* keyboard / mouse */
	kbdmng_initialize();
	wxkbd_initialize();
	mousemng_initialize();
	inputmng_init();

	/* joystick */
	joymng_initialize();

	/* sound */
	soundmng_initialize();

	/* task/system */
	taskmng_initialize();
	sysmng_initialize();

	/* PC core — pccore_init() internally calls sound_create() → soundmng_create() */
	pccore_init();
#if defined(SUPPORT_WAB)
	np2wab_init();
#endif
#if defined(SUPPORT_CL_GD5430)
	pc98_cirrus_vga_init();
#endif
	pccore_reset();
	/* pccore_reset() calls diskdrv_hddbind() internally, which reads
	 * np2cfg.idetype[], np2cfg.sasihdd[], and np2cfg.idecd[] to open
	 * all HDD/CD drives.  No extra diskdrv_setsxsi calls needed here. */

	/* mount FDD images saved in config */
	for (int _i = 0; _i < 4; _i++) {
		if (np2cfg.fddfile[_i][0])
			diskdrv_readyfdd((REG8)_i, np2cfg.fddfile[_i], 0);
	}

	soundmng_play();

	/* timing */
	timing_reset();

	/* load resume state */
	if (np2oscfg.resume) {
		flagload("s0", NP2_WX_APPNAME, TRUE);
	}

	return SUCCESS;
}

void np2_terminate(void)
{
	/* save resume state */
	if (np2oscfg.resume) {
		flagsave("s0");
	}

	pccore_term();
	soundmng_deinitialize();
	joymng_deinitialize();
	fontmng_term();
	scrnmng_destroy();
	taskmng_exit();
	sysmng_deinitialize();
	dosio_term();
	initsave();
}

/* ---- CD-ROM helpers ---- */

int findCdromDrive(void)
{
	for (int i = 0; i < 4; i++) {
		if (sxsi_getdevtype(i) == SXSIDEV_CDROM) return i;
	}
	/* fall back: check idetype config */
#if defined(SUPPORT_IDEIO)
	for (int i = 0; i < 4; i++) {
		if (np2cfg.idetype[i] == SXSIDEV_CDROM) return i;
	}
#endif
	return -1;
}

/* ---- disk image helpers ---- */

static char fdimage_ext[][5] = {
	".d88", ".d98", ".fdi", ".hdm", ".xdf", ".dup", ".2hd",
	".nfd", ".fdd", ".hd4", ".hd5", ".hd9", ".h01", ".hdb",
	".ddb", ".dd6", ".dd9", ".dcp", ".dcu", ".flp", ".bin",
	".tfd", ".fim", ".img", ".ima", ""
};

BOOL np2_isfdimage(const char *path)
{
	const char *ext = path + strlen(path) - 4;
	if (strlen(path) < 5) return FALSE;
	for (int i = 0; fdimage_ext[i][0]; i++) {
		if (strcasecmp(ext, fdimage_ext[i]) == 0) return TRUE;
	}
	return FALSE;
}
