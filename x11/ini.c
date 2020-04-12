#include "compiler.h"

#include "np2.h"
#include "dosio.h"
#include "ini.h"
#include "pccore.h"
#include "profile.h"
#include "strres.h"
#if defined(SUPPORT_BMS)
#include "bmsio.h"
#endif

#include "commng.h"
#include "joymng.h"
#include "kbdmng.h"
#include "soundmng.h"


typedef struct {
	const char	*title;
	INITBL		*tbl;
	INITBL		*tblterm;
	UINT		count;
} _INIARG, *INIARG;


static BOOL
inigetbmp(const UINT8 *ptr, UINT pos)
{

	return ((ptr[pos >> 3] >> (pos & 7)) & 1);
}

static void
inisetbmp(UINT8 *ptr, UINT pos, BOOL set)
{
	UINT8 bit;

	ptr += (pos >> 3);
	bit = 1 << (pos & 7);
	if (set) {
		*ptr |= bit;
	} else {
		*ptr &= ~bit;
	}
}

static void
inirdargs16(const char *src, INITBL *ini)
{
	SINT16 *dst;
	int dsize;
	int i;
	char c;

	dst = (SINT16 *)ini->value;
	dsize = ini->arg;

	for (i = 0; i < dsize; i++) {
		while (*src == ' ') {
			src++;
		}
		if (*src == '\0') {
			break;
		}
		dst[i] = (SINT16)milstr_solveINT(src);
		while (*src != '\0') {
			c = *src++;
			if (c == ',') {
				break;
			}
		}
	}
}

static void
inirdargh8(const char *src, INITBL *ini)
{
	UINT8 *dst;
	int dsize;
	int i;
	UINT8 val;
	BOOL set;
	char c;

	dst = (UINT8 *)ini->value;
	dsize = ini->arg;

	for (i = 0; i < dsize; i++) {
		val = 0;
		set = FALSE;
		while (*src == ' ') {
			src++;
		}
		for (;;) {
			c = *src;
			if ((c == '\0') || (c == ' ')) {
				break;
			}
			else if ((c >= '0') && (c <= '9')) {
				val <<= 4;
				val += c - '0';
				set = TRUE;
			}
			else {
				c |= 0x20;
				if ((c >= 'a') && (c <= 'f')) {
					val <<= 4;
					val += c - 'a' + 10;
					set = TRUE;
				}
			}
			src++;
		}
		if (set == FALSE) {
			break;
		}
		dst[i] = val;
	}
}

static void
iniwrsetargh8(char *work, int size, INITBL *ini)
{
	char tmp[8];
	const UINT8 *ptr;
	UINT arg;
	UINT i;

	ptr = (UINT8 *)(ini->value);
	arg = ini->arg;
	if (arg > 0) {
		g_snprintf(tmp, sizeof(tmp), "%.2x ", ptr[0]);
		milstr_ncpy(work, tmp, size);
	}
	for (i = 1; i < arg; i++) {
		g_snprintf(tmp, sizeof(tmp), "%.2x ", ptr[i]);
		milstr_ncat(work, tmp, size);
	}
}

/* ----- user */

static void
inirdbyte3(const char *src, INITBL *ini)
{
	UINT i;

	for (i = 0; i < 3; i++) {
		if (src[i] == '\0') {
			break;
		}
		if ((((src[i] - '0') & 0xff) < 9) ||
		    (((src[i] - 'A') & 0xdf) < 26)) {
			((UINT8 *)ini->value)[i] = src[i];
		}
	}
}

static void
inirdkb(const char *src, INITBL *ini)
{

	if ((!milstr_extendcmp(src, "DOS"))
	 || (!milstr_cmp(src, "JIS"))
	 || (!milstr_cmp(src, "106"))
	 || (!milstr_cmp(src, "JP"))
	 || (!milstr_cmp(src, "PCAT"))
	 || (!milstr_cmp(src, "AT"))) {
		*(UINT8 *)ini->value = KEY_KEY106;
	} else if ((!milstr_extendcmp(src, "KEY101"))
	        || (!milstr_cmp(src, "ASCII"))
	        || (!milstr_cmp(src, "EN"))
	        || (!milstr_cmp(src, "US"))
	        || (!milstr_cmp(src, "101"))) {
		*(UINT8 *)ini->value = KEY_KEY101;
	}
}

static void
inirdsnddrv(const char *src, INITBL *ini)
{

	*(UINT8 *)ini->value = snddrv_drv2num(src);
}

static void
inirdinterp(const char *src, INITBL *ini)
{

	if (!milstr_cmp(src, "NEAREST")) {
		*(UINT8 *)ini->value = INTERP_NEAREST;
	} else if (!milstr_cmp(src, "TILES")) {
		*(UINT8 *)ini->value = INTERP_TILES;
	} else if (!milstr_cmp(src, "HYPER")) {
		*(UINT8 *)ini->value = INTERP_HYPER;
	} else {
		*(UINT8 *)ini->value = INTERP_BILINEAR;
	}
}

static void update_iniread_flag(const INITBL *p);

static BRESULT
inireadcb(void *arg, const char *para, const char *key, const char *data)
{
	char work[512];
	INITBL *p;
	BOOL rv;

	if (arg == NULL) {
		return(FAILURE);
	}
	if (milstr_cmp(para, ((INIARG)arg)->title)) {
		return(SUCCESS);
	}
	p = ((INIARG)arg)->tbl;
	while (p < ((INIARG)arg)->tblterm) {
		if (!milstr_cmp(key, p->item)) {
			rv = TRUE;
			switch (p->itemtype & INITYPE_MASK) {
			case INITYPE_STR:
				milstr_ncpy((char *)p->value, data, p->arg);
				break;

			case INITYPE_BOOL:
				*((UINT8 *)p->value) = (!milstr_cmp(data, str_true))?1:0;
				break;

			case INITYPE_BITMAP:
				inisetbmp((UINT8 *)p->value, p->arg, milstr_cmp(data, str_true) == 0);
				break;

			case INITYPE_ARGS16:
				milstr_ncpy(work, data, 512);
				inirdargs16(work, p);
				break;

			case INITYPE_ARGH8:
				milstr_ncpy(work, data, 512);
				inirdargh8(work, p);
				break;

			case INITYPE_SINT8:
			case INITYPE_UINT8:
				*((UINT8 *)p->value) = (UINT8)milstr_solveINT(data);
				break;

			case INITYPE_SINT16:
			case INITYPE_UINT16:
				*((UINT16 *)p->value) = (UINT16)milstr_solveINT(data);
				break;

			case INITYPE_SINT32:
			case INITYPE_UINT32:
				*((UINT32 *)p->value) = (UINT32)milstr_solveINT(data);
				break;

			case INITYPE_HEX8:
				*((UINT8 *)p->value) = (UINT8)milstr_solveHEX(data);
				break;

			case INITYPE_HEX16:
				*((UINT16 *)p->value) = (UINT16)milstr_solveHEX(data);
				break;

			case INITYPE_HEX32:
				*((UINT32 *)p->value) = (UINT32)milstr_solveHEX(data);
				break;

			case INITYPE_BYTE3:
				milstr_ncpy(work, data, 512);
				inirdbyte3(work, p);
				break;

			case INITYPE_KB:
				milstr_ncpy(work, data, 512);
				inirdkb(work, p);
				break;

			case INITYPE_SNDDRV:
				milstr_ncpy(work, data, 512);
				inirdsnddrv(work, p);
				break;

			case INITYPE_INTERP:
				milstr_ncpy(work, data, 512);
				inirdinterp(work, p);
				break;

			default:
				rv = FALSE;
				break;
			}
			if (rv) {
				update_iniread_flag(p);
			}
		}
		p++;
	}
	return (SUCCESS);
}

void
ini_read(const char *path, const char *title, INITBL *tbl, UINT count)
{
	_INIARG	iniarg;

	if (path == NULL) {
		return;
	}
	iniarg.title = title;
	iniarg.tbl = tbl;
	iniarg.tblterm = tbl + count;
	profile_enum(path, &iniarg, inireadcb);
}


static void
iniwrsetstr(char *work, int size, const char *ptr)
{
	int i;
	char c;

	if (ptr[0] == ' ') {
		goto iwss_extend;
	}
	i = strlen(ptr);
	if ((i) && (ptr[i-1] == ' ')) {
		goto iwss_extend;
	}
	while (i > 0) {
		i--;
		if (ptr[i] == '\"') {
			goto iwss_extend;
		}
	}
	milstr_ncpy(work, ptr, size);
	return;

iwss_extend:
	if (size > 3) {
		size -= 3;
		*work++ = '\"';
		while (size > 0) {
			size--;
			c = *ptr++;
			if (c == '\"') {
				if (size > 0) {
					size--;
					work[0] = c;
					work[1] = c;
					work += 2;
				}
			}
			else {
				*work++ = c;
			}
		}
		work[0] = '\"';
		work[1] = '\0';
	}
}

static const char *
iniwrinterp(UINT8 interp)
{

	if (interp == INTERP_NEAREST)
		return "NEAREST";
	else if (interp == INTERP_TILES)
		return "TILES";
	else if (interp == INTERP_HYPER)
		return "HYPER";
	else
		return "BILINEAR";
}

static BOOL read_iniread_flag(const INITBL *p);

void
ini_write(const char *path, const char *title, INITBL *tbl, UINT count, BOOL create)
{
	char	work[512];
	INITBL	*p;
	INITBL	*pterm;
	FILEH	fh;
	BRESULT	set;

	fh = FILEH_INVALID;
	if (!create) {
		fh = file_open(path);
		if (fh != FILEH_INVALID)
			file_seek(fh, 0L, FSEEK_END);
	}
	if (fh == FILEH_INVALID) {
		fh = file_create(path);
		if (fh == FILEH_INVALID)
			return;
	}

	milstr_ncpy(work, "[", sizeof(work));
	milstr_ncat(work, title, sizeof(work));
	milstr_ncat(work, "]\n", sizeof(work));
	file_write(fh, work, strlen(work));

	p = tbl;
	pterm = tbl + count;
	while (p < pterm) {
		if (!(p->itemtype & INIFLAG_RO) || read_iniread_flag(p)) {
			work[0] = '\0';
			set = SUCCESS;
			switch (p->itemtype & INITYPE_MASK) {
			case INITYPE_STR:
				iniwrsetstr(work, sizeof(work), (char *)p->value);
				break;

			case INITYPE_BOOL:
				milstr_ncpy(work, (*((UINT8 *)p->value)) ? str_true : str_false, sizeof(work));
				break;

			case INITYPE_BITMAP:
				milstr_ncpy(work, inigetbmp((UINT8 *)p->value, p->arg) ? str_true : str_false, sizeof(work));
				break;

			case INITYPE_ARGH8:
				iniwrsetargh8(work, sizeof(work), p);
				break;

			case INITYPE_SINT8:
				g_snprintf(work, sizeof(work), "%d", *((char *)p->value));
				break;

			case INITYPE_SINT16:
				g_snprintf(work, sizeof(work), "%d", *((SINT16 *)p->value));
				break;

			case INITYPE_SINT32:
				g_snprintf(work, sizeof(work), "%d", *((SINT32 *)p->value));
				break;

			case INITYPE_UINT8:
				g_snprintf(work, sizeof(work), "%u", *((UINT8 *)p->value));
				break;

			case INITYPE_UINT16:
				g_snprintf(work, sizeof(work), "%u", *((UINT16 *)p->value));
				break;

			case INITYPE_UINT32:
				g_snprintf(work, sizeof(work), "%u", *((UINT32 *)p->value));
				break;

			case INITYPE_HEX8:
				g_snprintf(work, sizeof(work), "%x", *((UINT8 *)p->value));
				break;

			case INITYPE_HEX16:
				g_snprintf(work, sizeof(work), "%x", *((UINT16 *)p->value));
				break;

			case INITYPE_HEX32:
				g_snprintf(work, sizeof(work), "%x", *((UINT32 *)p->value));
				break;

			case INITYPE_KB:
				if (*(UINT8 *)p->value == KEY_KEY101)
					milstr_ncpy(work, "101", sizeof(work));
				else
					milstr_ncpy(work, "106", sizeof(work));
				break;

			case INITYPE_SNDDRV:
				g_snprintf(work, sizeof(work), "%s", snddrv_num2drv(*(UINT8 *)p->value));
				break;

			case INITYPE_INTERP:
				g_snprintf(work, sizeof(work), "%s", iniwrinterp(*(UINT8 *)p->value));
				break;

			default:
				set = FAILURE;
				break;
			}
			if (set == SUCCESS) {
				file_write(fh, p->item, strlen(p->item));
				file_write(fh, " = ", 3);
				file_write(fh, work, strlen(work));
				file_write(fh, "\n", 1);
			}
		}
		p++;
	}
	file_close(fh);
}

static const char ini_title[] =
#if !defined(CPUCORE_IA32)
	"NekoProjectIIkai";
#else
	"NekoProjectIIkai_IA32";
#endif
static const char inifile[] = "np2.cfg";

enum {
	INIRO_STR	= INIFLAG_RO | INITYPE_STR,
	INIRO_BOOL	= INIFLAG_RO | INITYPE_BOOL,
	INIRO_BITMAP	= INIFLAG_RO | INITYPE_BITMAP,
	INIRO_UINT8	= INIFLAG_RO | INITYPE_UINT8,
	INIRO_HEX8	= INIFLAG_RO | INITYPE_HEX8,
	INIMAX_UINT8	= INIFLAG_MAX | INITYPE_UINT8,
	INIAND_UINT8	= INIFLAG_AND | INITYPE_UINT8,
	INIROMAX_SINT32	= INIFLAG_RO | INIFLAG_MAX | INITYPE_SINT32,
	INIROAND_HEX32	= INIFLAG_RO | INIFLAG_AND | INITYPE_HEX32,

	INIRO_BYTE3	= INIFLAG_RO | INITYPE_BYTE3,
	INIRO_KB	= INIFLAG_RO | INITYPE_KB
};

static INITBL iniitem[] = {
	{"np2title", INIRO_STR,		np2oscfg.titles,	sizeof(np2oscfg.titles)},
	{"paddingx", INIROMAX_SINT32,	&np2oscfg.paddingx,	32},
	{"paddingy", INIROMAX_SINT32,	&np2oscfg.paddingy,	32},

	{"FDfolder", INITYPE_STR,	fddfolder,		MAX_PATH},
	{"HDfolder", INITYPE_STR,	hddfolder,		MAX_PATH},
	{"bmap_Dir", INITYPE_STR,	bmpfilefolder,		MAX_PATH},
	{"fontfile", INITYPE_STR,	np2cfg.fontfile,	MAX_PATH},
	{"biospath", INIRO_STR,		np2cfg.biospath,	MAX_PATH},
	{"hdrvroot", INIRO_STR,		np2cfg.hdrvroot,	MAX_PATH},
	{"hdrv_acc", INIRO_UINT8,	&np2cfg.hdrvacc,	0},

#if defined(SUPPORT_HOSTDRV)
	{"use_hdrv", INITYPE_BOOL,	&np2cfg.hdrvenable,	0},
	{"hdrvroot", INITYPE_STR,	&np2cfg.hdrvroot,	MAX_PATH},
	{"hdrv_acc", INITYPE_UINT8,	&np2cfg.hdrvacc,	0},
#endif

	{"pc_model", INITYPE_STR,	np2cfg.model,		sizeof(np2cfg.model)},

	{"clk_base", INITYPE_UINT32,	&np2cfg.baseclock,	0},
	{"clk_mult", INITYPE_UINT32,	&np2cfg.multiple,	0},

	{"DIPswtch", INITYPE_ARGH8,	np2cfg.dipsw,		3},
	{"MEMswtch", INITYPE_ARGH8,	np2cfg.memsw,		8},
	{"ExMemory", INIMAX_UINT8,	&np2cfg.EXTMEM,		13},
	{"ITF_WORK", INIRO_BOOL,	&np2cfg.ITF_WORK,	0},

	{"HDD1FILE", INITYPE_STR,	np2cfg.sasihdd[0],	MAX_PATH},
	{"HDD2FILE", INITYPE_STR,	np2cfg.sasihdd[1],	MAX_PATH},
#if defined(SUPPORT_IDEIO)
	{"HDD3FILE", INIRO_STR,		np2cfg.sasihdd[2],	MAX_PATH},
	{"HDD4FILE", INIRO_STR,		np2cfg.sasihdd[3],	MAX_PATH},
	{"IDE1TYPE", INIRO_UINT8,	&np2cfg.idetype[0],	0},
	{"IDE2TYPE", INIRO_UINT8,	&np2cfg.idetype[1],	0},
	{"IDE3TYPE", INIRO_UINT8,	&np2cfg.idetype[2],	0},
	{"IDE4TYPE", INIRO_UINT8,	&np2cfg.idetype[3],	0},
	{"IDE_BIOS", INIRO_BOOL,	&np2cfg.idebios,	0},
	{"AIDEBIOS", INIRO_BOOL,	&np2cfg.autoidebios,	0},
	{"IDERWAIT", INITYPE_UINT32,	&np2cfg.iderwait,	0},
	{"IDEWWAIT", INITYPE_UINT32,	&np2cfg.idewwait,	0},
	{"IDEMWAIT", INITYPE_UINT32,	&np2cfg.idemwait,	0},
#endif

	{"SampleHz", INITYPE_UINT32,	&np2cfg.samplingrate,	0},
	{"Latencys", INITYPE_UINT16,	&np2cfg.delayms,	0},
	{"SNDboard", INITYPE_HEX8,	&np2cfg.SOUND_SW,	0},
	{"BEEP_vol", INIAND_UINT8,	&np2cfg.BEEP_VOL,	3},
	{"xspeaker", INIRO_BOOL,	&np2cfg.snd_x,		0},

	{"SND14vol", INITYPE_ARGH8,	np2cfg.vol14,		6},
//	{"opt14BRD", INITYPE_ARGH8,	np2cfg.snd14opt,	3},
	{"opt26BRD", INITYPE_HEX8,	&np2cfg.snd26opt,	0},
	{"opt86BRD", INITYPE_HEX8,	&np2cfg.snd86opt,	0},
	{"optSPBRD", INITYPE_HEX8,	&np2cfg.spbopt,		0},
	{"optSPBVR", INITYPE_HEX8,	&np2cfg.spb_vrc,	0},
	{"optSPBVL", INIMAX_UINT8,	&np2cfg.spb_vrl,	24},
	{"optSPB_X", INITYPE_BOOL,	&np2cfg.spb_x,		0},
	{"USEMPU98", INITYPE_BOOL,	&np2cfg.mpuenable,		0},
	{"optMPU98", INITYPE_HEX8,	&np2cfg.mpuopt,		0},
	{"optMPUAT", INITYPE_BOOL,	&np2cfg.mpu_at,		0},
#if defined(SUPPORT_SMPU98)
	{"USE_SMPU", INITYPE_BOOL,	&np2cfg.smpuenable,		0},
	{"opt_SMPU", INITYPE_HEX8,	&np2cfg.smpuopt,		0},
	{"SMPUMUTB", INITYPE_BOOL,	&np2cfg.smpumuteB,		0},
#endif
	{"opt118io", INITYPE_HEX16,	&np2cfg.snd118io,	0},
	{"opt118id", INITYPE_HEX8,	&np2cfg.snd118id,	0},
	{"opt118dm", INITYPE_UINT8,	&np2cfg.snd118dma,	0},
	{"opt118if", INITYPE_UINT8,	&np2cfg.snd118irqf,	0},
	{"opt118ip", INITYPE_UINT8,	&np2cfg.snd118irqp,	0},
	{"opt118im", INITYPE_UINT8,	&np2cfg.snd118irqm,	0},
	{"opt118ro", INITYPE_UINT8,	&np2cfg.snd118rom,	0},

	{"optwssid", INITYPE_HEX8,	&np2cfg.sndwssid,	0},
	{"optwssdm", INITYPE_UINT8,	&np2cfg.sndwssdma,	0},
	{"optwssip", INITYPE_UINT8,	&np2cfg.sndwssirq,	0},

#if defined(SUPPORT_SOUND_SB16)
	{"optsb16p", INITYPE_HEX8,	&np2cfg.sndsb16io,	0},
	{"optsb16d", INITYPE_UINT8,	&np2cfg.sndsb16dma,	0},
	{"optsb16i", INITYPE_UINT8,	&np2cfg.sndsb16irq,	0},
	{"optsb16A", INITYPE_BOOL,	&np2cfg.sndsb16at,	0},
#endif	/* SUPPORT_SOUND_SB16 */

	{"volume_F", INIMAX_UINT8,	&np2cfg.vol_fm,		128},
	{"volume_S", INIMAX_UINT8,	&np2cfg.vol_ssg,	128},
	{"volume_A", INIMAX_UINT8,	&np2cfg.vol_adpcm,	128},
	{"volume_P", INIMAX_UINT8,	&np2cfg.vol_pcm,	128},
	{"volume_R", INIMAX_UINT8,	&np2cfg.vol_rhythm,	128},
	{"DAVOLUME", INIMAX_UINT8,	&np2cfg.davolume,	128},

#if defined(SUPPORT_FMGEN)
	{"USEFMGEN", INITYPE_BOOL,	&np2cfg.usefmgen,	0},
#endif	/* SUPPORT_FMGEN */

	{"Seek_Snd", INITYPE_BOOL,	&np2cfg.MOTOR,		0},
	{"Seek_Vol", INIMAX_UINT8,	&np2cfg.MOTORVOL,	100},

	{"btnRAPID", INITYPE_BOOL,	&np2cfg.BTN_RAPID,	0},
	{"btn_MODE", INITYPE_BOOL,	&np2cfg.BTN_MODE,	0},
	{"Mouse_sw", INITYPE_BOOL,	&np2oscfg.MOUSE_SW,	0},
	{"MS_RAPID", INITYPE_BOOL,	&np2cfg.MOUSERAPID,	0},

	{"VRAMwait", INITYPE_ARGH8,	np2cfg.wait,		6},
	{"DspClock", INIAND_UINT8,	&np2oscfg.DISPCLK,	3},
	{"DispSync", INITYPE_BOOL,	&np2cfg.DISPSYNC,	0},
	{"Real_Pal", INITYPE_BOOL,	&np2cfg.RASTER,		0},
	{"RPal_tim", INIMAX_UINT8,	&np2cfg.realpal,	64},
	{"s_NOWAIT", INITYPE_BOOL,	&np2oscfg.NOWAIT,	0},
	{"SkpFrame", INITYPE_UINT8,	&np2oscfg.DRAW_SKIP,	0},
	{"uPD72020", INITYPE_BOOL,	&np2cfg.uPD72020,	0},
	{"GRCG_EGC", INIAND_UINT8,	&np2cfg.grcg,		3},
	{"color16b", INITYPE_BOOL,	&np2cfg.color16,	0},
	{"skipline", INITYPE_BOOL,	&np2cfg.skipline,	0},
	{"skplight", INITYPE_SINT16,	&np2cfg.skiplight,	0},
	{"LCD_MODE", INIAND_UINT8,	&np2cfg.LCD_MODE,	0x03},
	{"BG_COLOR", INIROAND_HEX32,	&np2cfg.BG_COLOR,	0xffffff},
	{"FG_COLOR", INIROAND_HEX32,	&np2cfg.FG_COLOR,	0xffffff},

	{"pc9861_e", INITYPE_BOOL,	&np2cfg.pc9861enable,	0},
	{"pc9861_s", INITYPE_ARGH8,	np2cfg.pc9861sw,	3},
	{"pc9861_j", INITYPE_ARGH8,	np2cfg.pc9861jmp,	6},

	{"calendar", INITYPE_BOOL,	&np2cfg.calendar,	0},
	{"USE144FD", INITYPE_BOOL,	&np2cfg.usefd144,	0},
	{"FDDRIVE1", INIRO_BITMAP,	&np2cfg.fddequip,	0},
	{"FDDRIVE2", INIRO_BITMAP,	&np2cfg.fddequip,	1},
	{"FDDRIVE3", INIRO_BITMAP,	&np2cfg.fddequip,	2},
	{"FDDRIVE4", INIRO_BITMAP,	&np2cfg.fddequip,	3},

#if defined(SUPPORT_BMS)
	{"Use_BMS_", INITYPE_BOOL,	&bmsiocfg.enabled,	0},
	{"BMS_Port", INIROAND_HEX32,	&bmsiocfg.port,		0},
	{"BMS_Size", INIMAX_UINT8,	&bmsiocfg.numbanks,	256},
#endif

#if defined(SUPPORT_NET)
	{"NP2NETTAP", INITYPE_STR,	&np2cfg.np2nettap,	0},
	{"NP2NETPMM", INITYPE_BOOL,	&np2cfg.np2netpmm,	0},
#endif
#if defined(SUPPORT_LGY98)
	{"USELGY98", INITYPE_BOOL,	&np2cfg.uselgy98,	0},
	{"LGY98_IO", INITYPE_UINT16,	&np2cfg.lgy98io,	0},
	{"LGY98IRQ", INITYPE_UINT8,	&np2cfg.lgy98irq,	0},
	{"LGY98MAC", INITYPE_ARGH8,	np2cfg.lgy98mac,	6},
#endif
#if defined(SUPPORT_WAB)
	{"WAB_ANSW", INITYPE_UINT8,	&np2cfg.wabasw,		0},
#endif
#if defined(SUPPORT_CL_GD5430)
	{"USE_CLGD", INITYPE_BOOL,	&np2cfg.usegd5430,	0},
	{"CLGDTYPE", INITYPE_UINT16,	&np2cfg.gd5430type,	0},
	{"CLGDFCUR", INITYPE_BOOL,	&np2cfg.gd5430fakecur,	0},
	{"GDMELOFS", INITYPE_UINT8,	&np2cfg.gd5430melofs,	0},
	{"GANBBSEX", INITYPE_BOOL,	&np2cfg.ga98nb_bigscrn_ex,	0},
#endif
	{"TIMERFIX", INITYPE_BOOL,	&np2cfg.timerfix,	0},

	{"WINNTFIX", INITYPE_BOOL,	&np2cfg.winntfix,	0},

	{"SYSIOMSK", INITYPE_HEX16,	&np2cfg.sysiomsk,	0},

	{"MEMCHKMX", INITYPE_UINT8,	&np2cfg.memchkmx,	0},
	{"SBEEPLEN", INITYPE_UINT8,	&np2cfg.sbeeplen,	0},
	{"SBEEPLEN", INITYPE_BOOL,	&np2cfg.sbeepadj,	0},
	
	{"cpu_vend", INITYPE_STR,	np2cfg.cpu_vendor_o,	15},
	{"cpu_fami", INITYPE_UINT32,	&np2cfg.cpu_family,	0},
	{"cpu_mode", INITYPE_UINT32,	&np2cfg.cpu_model,	0},
	{"cpu_step", INITYPE_UINT32,	&np2cfg.cpu_stepping,	0},
	{"cpu_feat", INITYPE_HEX32,	&np2cfg.cpu_feature,	0},
	{"cpu_f_ex", INITYPE_HEX32,	&np2cfg.cpu_feature_ex,	0},
	{"cpu_bran", INIRO_STR,		np2cfg.cpu_brandstring_o, 63},
	{"cpu_brid", INITYPE_HEX32,	&np2cfg.cpu_brandid,	0},
	{"cpu_fecx", INITYPE_HEX32,	&np2cfg.cpu_feature_ecx,	0},
	{"cpu_eflg", INITYPE_HEX32,	&np2cfg.cpu_eflags_mask,	0},

	{"FPU_TYPE", INITYPE_UINT8,	&np2cfg.fpu_type,	0},
#if defined(SUPPORT_FAST_MEMORYCHECK)
	{"memckspd", INITYPE_UINT8,	&np2cfg.memcheckspeed,	0},
#endif
	{"USERAM_D", INITYPE_BOOL,	&np2cfg.useram_d,	0},
#if defined(SUPPORT_ASYNC_CPU)
	{"ASYNCCPU", INITYPE_BOOL,	&np2cfg.asynccpu,	0},
#endif
#if defined(SUPPORT_IDEIO)
	{"IDEBADDR", INIRO_HEX8,	&np2cfg.idebaddr,	0},
#endif
#if defined(SUPPORT_GAMEPORT)
	{"GAMEPORT", INITYPE_BOOL,	&np2cfg.gameport,	0},
#endif

	{"keyboard", INITYPE_KB,	&np2oscfg.KEYBOARD,	0},
	{"F12_COPY", INITYPE_UINT8,	&np2oscfg.F12KEY,	0},
	{"Joystick", INITYPE_BOOL,	&np2oscfg.JOYPAD1,	0},
	{"Joy1_btn", INITYPE_ARGH8,	np2oscfg.JOY1BTN,	JOY_NBUTTON},
	{"Joy1_dev", INITYPE_STR,	&np2oscfg.JOYDEV[0],	MAX_PATH},
	{"Joy1amap", INITYPE_ARGH8,	np2oscfg.JOYAXISMAP[0],	JOY_NAXIS},
	{"Joy1bmap", INITYPE_ARGH8,	np2oscfg.JOYBTNMAP[0],	JOY_NBUTTON},

	{"confirm_", INITYPE_BOOL,	&np2oscfg.confirm,	0},

	{"mpu98map", INITYPE_STR,	np2oscfg.mpu.mout,	MAX_PATH},
	{"mpu98min", INITYPE_STR,	np2oscfg.mpu.min,	MAX_PATH},
	{"mpu98mdl", INITYPE_STR,	np2oscfg.mpu.mdl,	64},
	{"mpu98def", INITYPE_STR,	np2oscfg.mpu.def,	MAX_PATH},

#if defined(SUPPORT_SMPU98)
	{"smpuAmap", INITYPE_STR,	np2oscfg.smpuA.mout,	MAX_PATH},
	{"smpuAmin", INITYPE_STR,	np2oscfg.smpuA.min,	MAX_PATH},
	{"smpuAmdl", INITYPE_STR,	np2oscfg.smpuA.mdl,	64},
	{"smpuAdef", INITYPE_STR,	np2oscfg.smpuA.def,	MAX_PATH},
	{"smpuBmap", INITYPE_STR,	np2oscfg.smpuB.mout,	MAX_PATH},
	{"smpuBmin", INITYPE_STR,	np2oscfg.smpuB.min,	MAX_PATH},
	{"smpuBmdl", INITYPE_STR,	np2oscfg.smpuB.mdl,	64},
	{"smpuBdef", INITYPE_STR,	np2oscfg.smpuB.def,	MAX_PATH},
#endif

	{"com1port", INIMAX_UINT8,	&np2oscfg.com[0].port,	COMPORT_MIDI},
	{"com1dir",  INITYPE_BOOL,	&np2oscfg.com[0].direct,	1},
	{"com1para", INITYPE_UINT8,	&np2oscfg.com[0].param,	0},
	{"com1_bps", INITYPE_UINT32,	&np2oscfg.com[0].speed,	0},
	{"com1mmap", INITYPE_STR,	np2oscfg.com[0].mout,	MAX_PATH},
	{"com1mmdl", INITYPE_STR,	np2oscfg.com[0].mdl,	64},
	{"com1mdef", INITYPE_STR,	np2oscfg.com[0].def,	MAX_PATH},

	{"com2port", INIMAX_UINT8,	&np2oscfg.com[1].port,	COMPORT_MIDI},
	{"com2dir",  INITYPE_BOOL,	&np2oscfg.com[1].direct,	1},
	{"com2para", INITYPE_UINT8,	&np2oscfg.com[1].param,	0},
	{"com2_bps", INITYPE_UINT32,	&np2oscfg.com[1].speed,	0},
	{"com2mmap", INITYPE_STR,	np2oscfg.com[1].mout,	MAX_PATH},
	{"com2mmdl", INITYPE_STR,	np2oscfg.com[1].mdl,	64},
	{"com2mdef", INITYPE_STR,	np2oscfg.com[1].def,	MAX_PATH},

	{"com3port", INIMAX_UINT8,	&np2oscfg.com[2].port,	COMPORT_MIDI},
	{"com3dir",  INITYPE_BOOL,	&np2oscfg.com[2].direct,	1},
	{"com3para", INITYPE_UINT8,	&np2oscfg.com[2].param,	0},
	{"com3_bps", INITYPE_UINT32,	&np2oscfg.com[2].speed,	0},
	{"com3mmap", INITYPE_STR,	np2oscfg.com[2].mout,	MAX_PATH},
	{"com3mmdl", INITYPE_STR,	np2oscfg.com[2].mdl,	64},
	{"com3mdef", INITYPE_STR,	np2oscfg.com[2].def,	MAX_PATH},

#if defined(SUPPORT_RESUME)
	{"e_resume", INITYPE_BOOL,	&np2oscfg.resume,	0},
#endif
#if defined(SUPPORT_STATSAVE)
	{"STATSAVE", INIRO_BOOL,	&np2oscfg.statsave,	0},
#endif
#if defined(GCC_CPU_ARCH_IA32)
	{"nousemmx", INITYPE_BOOL,	&np2oscfg.disablemmx,	0},
#endif
	{"toolwind", INITYPE_BOOL,	&np2oscfg.toolwin,	0},
	{"keydispl", INITYPE_BOOL,	&np2oscfg.keydisp,	0},
	{"soft_kbd", INITYPE_BOOL,	&np2oscfg.softkbd,	0},
	{"jast_snd", INITYPE_BOOL,	&np2oscfg.jastsnd,	0},
	{"xrollkey", INITYPE_BOOL,	&np2oscfg.xrollkey,	0},

	{"sounddrv", INITYPE_SNDDRV,	&np2oscfg.snddrv,	0},
	{"MIDIOUTd", INITYPE_STR,	&np2oscfg.MIDIDEV[0],	MAX_PATH},
	{"MIDIIN_d", INITYPE_STR,	&np2oscfg.MIDIDEV[1],	MAX_PATH},
#if defined(SUPPORT_SMPU98)
	{"MIDIOUAd", INITYPE_STR,	&np2oscfg.MIDIDEVA[0],	MAX_PATH},
	{"MIDIINAd", INITYPE_STR,	&np2oscfg.MIDIDEVA[1],	MAX_PATH},
	{"MIDIOUBd", INITYPE_STR,	&np2oscfg.MIDIDEVB[0],	MAX_PATH},
	{"MIDIINBd", INITYPE_STR,	&np2oscfg.MIDIDEVB[1],	MAX_PATH},
#endif
	{"MIDIWAIT", INITYPE_UINT32,	&np2oscfg.MIDIWAIT,	0},

	{"dinterp_", INITYPE_INTERP,	&np2oscfg.drawinterp,	0},
	{"fullscrn", INITYPE_UINT32,	&ignore_fullscreen_mode,0},
	{"F11_KEY_", INITYPE_UINT8,	&np2oscfg.F11KEY,	0},

	{"READONLY", INIRO_BOOL,	&np2oscfg.readonly,	0},
	{"I286SAVE", INIRO_BOOL,	&np2oscfg.I286SAVE,	0},
};
#define	INIITEMS	(sizeof(iniitem) / sizeof(iniitem[0]))


static BOOL iniread_flag[INIITEMS];

static int
calc_index(const INITBL *p)
{
	UINT offset;
	UINT idx;

	if (p) {
		offset = (const char *)p - (const char *)iniitem;
		if ((offset % sizeof(iniitem[0])) == 0) {
			idx = offset / sizeof(iniitem[0]);
			if (idx < INIITEMS) {
				return idx;
			}
		}
	}
	return -1;
}

static void
update_iniread_flag(const INITBL *p)
{
	int idx;

	idx = calc_index(p);
	if (idx >= 0) {
		iniread_flag[idx] = TRUE;
	}
}

static BOOL
read_iniread_flag(const INITBL *p)
{
	int idx;

	idx = calc_index(p);
	if (idx >= 0) {
		return iniread_flag[idx];
	}
	return FALSE;
}

NP2CFG np2cfg_default;
NP2OSCFG np2oscfg_default;

void
initload(void)
{
	char path[MAX_PATH];

	np2cfg_default = np2cfg;
	np2oscfg_default = np2oscfg;

	milstr_ncpy(path, modulefile, sizeof(path));
	ini_read(path, ini_title, iniitem, INIITEMS);
}

void
initsave(void)
{
	char path[MAX_PATH];

	milstr_ncpy(path, modulefile, sizeof(path));
	ini_write(path, ini_title, iniitem, INIITEMS, TRUE);
}

static const char s_szExt[] = ".ini";

void initgetfile(char *lpPath, unsigned int cchPath)
{
	const char *lpIni = inifile;
	if (lpIni)
	{
		file_cpyname(lpPath, lpIni, cchPath);
		//LPCTSTR lpExt = file_getext(lpPath);
		//if (lpExt[0] != '\0')
		//{
		//	file_catname(lpPath, s_szExt, cchPath);
		//}
	}
	else
	{
		file_cpyname(lpPath, modulefile, cchPath);
		file_cutext(lpPath);
		file_catname(lpPath, s_szExt, cchPath);
	}
}

