#include	"compiler.h"
#include	"strres.h"
#include	"np2.h"
#include	"scrnmng.h"
#include	"sysmng.h"
#include	"sstp.h"
#include	"sstpres.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"sound.h"
#include	"fmboard.h"
#include	"soundrom.h"
#include	"np2info.h"
#if defined(OSLANG_UCS2)
#include	"oemtext.h"
#endif	// defined(OSLANG_UCS2)


static const OEMCHAR cr[] = OEMTEXT("\\n");


// ---- np2info extend

static const OEMCHAR str_jwinclr[] =
					OEMTEXT("256色\0ハイカラー\0フルカラー\0トゥルーカラー");
static const OEMCHAR str_jwinmode[] =
					OEMTEXT(" (窓モード)\0 (フルスクリーン)");


static void info_progtitle(OEMCHAR *str, int maxlen, const NP2INFOEX *ex) {

	milstr_ncpy(str, np2oscfg.titles, maxlen);
}

static void info_jsound(OEMCHAR *str, int maxlen, const NP2INFOEX *ex) {

const OEMCHAR	*p;

	switch(np2cfg.SOUND_SW) {
		case SOUNDID_NONE:
			p = OEMTEXT("なし");
			break;

		case SOUNDID_PC_9801_14:
			p = OEMTEXT("14ボード");
			break;

		case SOUNDID_PC_9801_26K:
			p = OEMTEXT("26K音源");
			break;

		case SOUNDID_PC_9801_86:
			p = OEMTEXT("86音源");
			break;

		case SOUNDID_PC_9801_86_26K:
			p = OEMTEXT("２枚刺し");
			break;

		case SOUNDID_PC_9801_118:
			p = OEMTEXT("118音源");
			break;

		case SOUNDID_PC_9801_86_ADPCM:
			p = OEMTEXT("86音源(ちびおと付)");
			break;

		case SOUNDID_SPEAKBOARD:
			p = OEMTEXT("スピークボード");
			break;

		case SOUNDID_SPARKBOARD:
			p = OEMTEXT("スパークボード");
			break;

#if defined(SUPPORT_SOUND_SB16)
		case SOUNDID_SB16:
			p = OEMTEXT("サウンドブラスター16");
			break;
#endif

		case SOUNDID_WAVESTAR:
			p = OEMTEXT("ウェーブスター");
			break;

		case SOUNDID_AMD98:
			p = OEMTEXT("AMD-98");
			break;

		case SOUNDID_SOUNDORCHESTRA:
			p = OEMTEXT("サウンドオーケストラ");
			break;

		case SOUNDID_SOUNDORCHESTRAV:
			p = OEMTEXT("安いサウンドオーケストラ");
			break;

		case SOUNDID_LITTLEORCHESTRAL:
			p = OEMTEXT("リトルオーケストラ");
			break;

		case SOUNDID_MMORCHESTRA:
			p = OEMTEXT("マルチメディアオーケストラ");
			break;

#if defined(SUPPORT_PX)
		case SOUNDID_PX1:
			p = OEMTEXT("音美ちゃん");
			break;

		case SOUNDID_PX2:
			p = OEMTEXT("音美ちゃん2");
			break;
#endif

		default:
			p = OEMTEXT("よくわかんない");
			break;
	}
	milstr_ncpy(str, p, maxlen);
}

static void info_jdisp(OEMCHAR *str, int maxlen, const NP2INFOEX *ex) {

	UINT	bpp;

	bpp = scrnmng_getbpp();
	milstr_ncpy(str, milstr_list(str_jwinclr, ((bpp >> 3) - 1) & 3), maxlen);
	milstr_ncat(str, milstr_list(str_jwinmode, (scrnmng_isfullscreen())?1:0),
																	maxlen);
	(void)ex;
}

static void info_jbios(OEMCHAR *str, int maxlen, const NP2INFOEX *ex) {

	str[0] = '\0';
	if (pccore.rom & PCROM_BIOS) {
		milstr_ncat(str, str_biosrom, maxlen);
	}
	if (soundrom.name[0]) {
		if (str[0]) {
			milstr_ncat(str, OEMTEXT("と"), maxlen);
		}
		milstr_ncat(str, soundrom.name, maxlen);
	}
	if (str[0] == '\0') {
		milstr_ncat(str, OEMTEXT("なし"), maxlen);
	}
}

static void info_jrhythm(OEMCHAR *str, int maxlen, const NP2INFOEX *ex) {

const OEMCHAR	*p;
	OEMCHAR		jrhythmstr[16];
	UINT		exist;
	UINT		i;

	if (!(np2cfg.SOUND_SW & 0x6c)) {
		p = OEMTEXT("不要");
	}
	else {
		exist = rhythm_getcaps();
		if (exist == 0) {
			p = OEMTEXT("用意されてない…");
		}
		else if (exist == 0x3f) {
			p = OEMTEXT("全部ある");
		}
		else {
			milstr_ncpy(jrhythmstr, OEMTEXT("BSCHTRです"), NELEMENTS(jrhythmstr));
			for (i=0; i<6; i++) {
				if (!(exist & (1 << i))) {
					jrhythmstr[i] = '_';
				}
			}
			p = jrhythmstr;
		}
	}
	milstr_ncpy(str, p, maxlen);
}

typedef struct {
	OEMCHAR	key[8];
	void	(*proc)(OEMCHAR *str, int maxlen, const NP2INFOEX *ex);
} INFOPROC;

static const INFOPROC infoproc[] = {
			{OEMTEXT("PROG"),		info_progtitle},
			{OEMTEXT("JSND"),		info_jsound},
			{OEMTEXT("JBIOS"),		info_jbios},
			{OEMTEXT("JDISP"),		info_jdisp},
			{OEMTEXT("JRHYTHM"),	info_jrhythm}};

static BOOL sstpext(OEMCHAR *dst, const OEMCHAR *key, int maxlen,
														const NP2INFOEX *ex) {

const INFOPROC	*inf;
const INFOPROC	*infterm;

	inf = infoproc;
	infterm = infoproc + NELEMENTS(infoproc);
	while(inf < infterm) {
		if (!milstr_cmp(key, inf->key)) {
			inf->proc(dst, maxlen, ex);
			return(TRUE);
		}
		inf++;
	}
	return(FALSE);
}

static const NP2INFOEX sstpex = {OEMTEXT("\\n"), sstpext};


// ----

static const UINT8 prs2[] = {0xaa,0xac,0xae,0xb0,0xb2,0xbe,0xf0,0x9f,
							 0xa1,0xa3,0xa5,0xa7,0xe1,0xe3,0xe5,0xc1,
							 0xb8,0xa0,0xa2,0xa4,0xa6,0xa8,0xa9,0xab,
							 0xad,0xaf,0xb1,0xb3,0xb5,0xb7,0xb9,0xbb,
							 0xbd,0xbf,0xc2,0xc4,0xc6,0xc8,0xc9,0xca,
							 0xcb,0xcc,0xcd,0xd0,0xd3,0xd6,0xd9,0xdc,
							 0xdd,0xde,0xdf,0xe0,0xe2,0xe4,0xe6,0xe7,
							 0xe8,0xe9,0xea,0xeb,0xed,0xf1,0xb4,0xb8};

#define	GETSSTPDAT1(a) {								\
				(a) = last;								\
				last = *dat++;							\
				(a) += last;							\
				(a) = ((a) << 2) | ((a) >> 6);			\
		}

static OEMCHAR *sstpsolve(OEMCHAR *buf, const UINT8 *dat) {

	UINT8	c;
	UINT8	last;
#if defined(OSLANG_UCS2)
	char	sjis[4];
#endif

	last = 0x80;
	while(1) {
		GETSSTPDAT1(c);
		if (!c) {
			break;
		}
		else if (c < 0x20) {
			if (c == 0x12) {
				*buf++ = 13;
				*buf++ = 10;
			}
			else {
				*buf++ = '\\';
				if (c < 0x1b) {
					*buf++ = (c + 0x60);
				}
				else {
					*buf++ = 's';
					*buf++ = (c + 0x30 - 0x1b);
				}
			}
		}
		else if (c < 0x7f) {
			*buf++ = c;
		}
		else if (c == 0x7f) {
			UINT8 ms;
			GETSSTPDAT1(ms);
			if (!ms) {
				break;
			}
			while(ms > 10) {
				CopyMemory(buf, OEMTEXT("\\w9"), 3 * sizeof(OEMCHAR));
				buf += 3;
				ms -= 10;
			}
			if (ms) {
				OEMSPRINTF(buf, OEMTEXT("\\w%1u"), ms);
				buf += 3;
			}
		}
		else if (c == 0x80) {
			UINT8 c2;
			GETSSTPDAT1(c2);
			if (c2) {
#if defined(OSLANG_UCS2)
				sjis[0] = c2;
				sjis[1] = '\0';
				buf += oemtext_sjistooem(buf, 4, sjis, 1);
#else
				*buf++ = c2;
#endif
			}
			else {
				break;
			}
		}
		else if (c >= 0xf0) {
			int i;
			const UINT8 *p;
			i = c - 0xf0;
			if (c == 0xff) {
				UINT8 c2;
				GETSSTPDAT1(c2);
				if (!c2) {
					break;
				}
				i += (c2 - 1);
			}
			p = xitems;
			while(i--) {
				p += xitems2[i];
			}
			buf = sstpsolve(buf, p);
		}
		else if ((c >= 0xa0) && (c < 0xe0)) {
#if defined(OSLANG_UCS2)
			sjis[0] = (UINT8)0x82;
			sjis[1] = prs2[c-0xa0];
			sjis[2] = '\0';
			buf += oemtext_sjistooem(buf, 4, sjis, 2);
#else
			buf[0] = (UINT8)0x82;
			buf[1] = prs2[c-0xa0];
			buf += 2;
#endif
		}
		else {
			UINT8 c2;
			GETSSTPDAT1(c2);
			if (c2) {
#if defined(OSLANG_UCS2)
				sjis[0] = c;
				sjis[1] = c2;
				sjis[2] = '\0';
				buf += oemtext_sjistooem(buf, 4, sjis, 2);
#else
				buf[0] = c;
				buf[1] = c2;
				buf += 2;
#endif
			}
			else {
				break;
			}
		}
	}
	*buf = '\0';
	return(buf);
}


// -------------------------------

static const UINT8 *prcs[4] = {k_keropi, k_winx68k, k_t98next, k_anex86};

static int check_emuprcs(void) {

	UINT	i;

	for (i=0; i<NELEMENTS(prcs); i++) {
		OEMCHAR	buf[64];
		sstpsolve(buf, prcs[i]);
		if (FindWindow(buf, NULL)) {
			return(i + 1);
		}
	}
	return(0);
}


// ------------------------------------------------------------------------

void sstpmsg_welcome(void) {

	UINT	emu;
	OEMCHAR	*p;
	OEMCHAR	buf[512];

	p = buf;
	emu = check_emuprcs();
	if (!emu) {
		switch(rand() & 15) {
			case 0x00:
			case 0x01:
			case 0x02:
			case 0x03:
				p = sstpsolve(p, s_welcome1);
				break;

			case 0x04:
			case 0x05:
			case 0x06:
				p = sstpsolve(p, s_welcome2);
				break;

			case 0x07:
			case 0x08:
				p = sstpsolve(p, s_welcome3);
				break;

			case 0x09:
				p = sstpsolve(p, s_welcome4);
				break;

			case 0x0a:
				p = sstpsolve(p, s_welcome5);
				break;

			default:
				p = sstpsolve(p, s_welcome6);
				break;
		}
	}
	else {
		switch(emu) {
			case 1:
				p = sstpsolve(p, s_keropi0);
				switch(rand() & 3) {
					case 0:
						p = sstpsolve(p, s_keropi1);
						break;

					case 1:
						p = sstpsolve(p, s_keropi2);
						break;

					default:
						p = sstpsolve(p, s_keropi3);
						break;
				}
				break;

			case 2:
				p = sstpsolve(p, s_winx68k);
				break;

			case 3:
				p = sstpsolve(p, s_t98next);
				break;

			case 4:
				p = sstpsolve(p, s_anex86);
				break;

			default:
				p = sstpsolve(p, s_error);
				break;
		}
	}
	sstp_send(buf, NULL);
}

void sstpmsg_reset(void) {

	OEMCHAR	str[1024];
	UINT	update;

	str[0] = '\0';
	update = sys_updates;
	if (update & SYS_UPDATECLOCK) {
		milstr_ncat(str, OEMTEXT("ＣＰＵクロックを %CLOCK%に"), NELEMENTS(str));
	}
	if (update & SYS_UPDATEMEMORY) {
		if (str[0]) {
			milstr_ncat(str, cr, NELEMENTS(str));
		}
		milstr_ncat(str, OEMTEXT("メモリを %MEM3%に"), NELEMENTS(str));
	}
	if (update & SYS_UPDATESBOARD) {
		if (str[0]) {
			milstr_ncat(str, cr, NELEMENTS(str));
		}
		milstr_ncat(str, OEMTEXT("音源を %JSND%に"), NELEMENTS(str));
	}
	if (update & (SYS_UPDATERATE | SYS_UPDATESBUF | SYS_UPDATEMIDI |
					SYS_UPDATEHDD | SYS_UPDATESERIAL1)) {
		BOOL hit = FALSE;
		if (str[0]) {
			milstr_ncat(str, OEMTEXT("\\nあと…\\w5"), NELEMENTS(str));
		}
		if (update & SYS_UPDATEMIDI) {
			hit = TRUE;
			milstr_ncat(str, OEMTEXT("MIDI"), NELEMENTS(str));
		}
		if (update & (SYS_UPDATERATE | SYS_UPDATESBUF)) {
			if (hit) {
				milstr_ncat(str, str_space, NELEMENTS(str));
			}
			hit = TRUE;
			milstr_ncat(str, OEMTEXT("サウンド設定"), NELEMENTS(str));
		}
		if (update & SYS_UPDATEHDD) {
			if (hit) {
				milstr_ncat(str, str_space, NELEMENTS(str));
			}
			hit = TRUE;
			milstr_ncat(str, OEMTEXT("ハードディスク"), NELEMENTS(str));
		}
		if (update & SYS_UPDATESERIAL1) {
			if (hit) {
				milstr_ncat(str, str_space, NELEMENTS(str));
			}
			hit = TRUE;
			milstr_ncat(str, OEMTEXT("シリアル"), NELEMENTS(str));
		}
		milstr_ncat(str, OEMTEXT("の設定を"), NELEMENTS(str));
	}
	if (str[0]) {
		OEMCHAR out[1024];
		milstr_ncat(str, OEMTEXT("変更しました。"), NELEMENTS(str));
		np2info(out, str, NELEMENTS(out), &sstpex);
		sstp_send(out, NULL);
	}
}

void sstpmsg_about(void) {

	OEMCHAR	str[1024];
	OEMCHAR	out[1024];
	OEMCHAR	*p;
	int		nostat = FALSE;

	p = str;
	switch(rand() & 7) {
		case 0:
		case 1:
		case 2:
		case 3:
			p = sstpsolve(p, s_ver0);
			break;

		case 4:
		case 5:
			p = sstpsolve(p, s_ver1);
			break;

		case 6:
			p = sstpsolve(p, s_ver2);
			break;

		case 7:
			p = sstpsolve(p, s_ver3);
			nostat = TRUE;
			break;
	}
	if (!nostat) {
		p = sstpsolve(p, s_info);
	}
	np2info(out, str, NELEMENTS(out), &sstpex);
	sstp_send(out, NULL);
}


void sstpmsg_config(void) {

	OEMCHAR	str[1024];
	OEMCHAR	*p;

	p = sstpsolve(str, s_config0);
	switch(rand() & 7) {
		case 0:
			p = sstpsolve(p, s_config1);
			break;

		case 1:
			p = sstpsolve(p, s_config2);
			break;

		case 2:
		case 3:
			p = sstpsolve(p, s_config3);
			break;

		default:
			p = sstpsolve(p, s_config4);
			break;
	}
	sstp_send(str, NULL);
}


// ----

static char *get_code(char *buf, int *ret) {

	int		stat;

	stat = 0;
	if (!memcmp(buf, "SSTP", 4)) {
		while(*buf) {
			if (*buf++ == ' ') {
				break;
			}
		}
		while((*buf >= '0') && (*buf <= '9')) {
			stat *= 10;
			stat += (*buf++ - '0');
		}
		while(*buf) {
			if (*buf++ == '\n') {
				break;
			}
		}
	}
	if (ret) {
		*ret = stat;
	}
	return(buf);
}

static void e_sstpreset(HWND hWnd, char *buf) {

	char	*p;
	int		ret;

	p = get_code(buf, &ret);
	if (ret == 200) {
		if (!memcmp(p, "いい", 4)) {
			SendMessage(hWnd, WM_NP2CMD, 0, NP2CMD_RESET);
		}
	}
}

BOOL sstpconfirm_reset(void) {

	OEMCHAR	str[256];

	sstpsolve(str, s_reset);
	return(sstp_send(str, e_sstpreset));
}

static void e_sstpexit(HWND hWnd, char *buf) {

	char	*p;
	int		ret;

	p = get_code(buf, &ret);
	if (ret == 200) {
		if (!memcmp(p, "いい", 4)) {
			SendMessage(hWnd, WM_NP2CMD, 0, NP2CMD_EXIT);
		}
	}
}

BOOL sstpconfirm_exit(void) {

	OEMCHAR	str[512];

	sstpsolve(str, s_exit);
	return(sstp_send(str, e_sstpexit));
}


// ------------------------------------------------------------ 単発。

BOOL sstpmsg_running(void) {

	OEMCHAR	buf[256];
	OEMCHAR	*p;

	p = buf;
	switch(rand() & 7) {
		case 0:
			p = sstpsolve(p, s_running1);
			break;

		case 1:
			p = sstpsolve(p, s_running2);
			break;

		case 2:
			p = sstpsolve(p, s_running3);
			break;

		default:
			p = sstpsolve(p, s_running4);
			break;
	}
	return(sstp_sendonly(buf));
}


BOOL sstpmsg_dxerror(void) {

	OEMCHAR	buf[256];

	sstpsolve(buf, s_dxerror);
	return(sstp_sendonly(buf));
}

