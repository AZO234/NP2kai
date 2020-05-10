#include	"compiler.h"
#include	"strres.h"
#if defined(OSLANG_UCS2)
#include	"oemtext.h"
#endif
#include	"taskmng.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"bios/sxsibios.h"
#if defined(SUPPORT_HOSTDRV)
#include	"hostdrv.h"
#endif
#include	"sound/sound.h"
#include	"sound/beep.h"
#include	"sound/fmboard.h"
#include	"sound/soundrom.h"
#include	"cbus/mpu98ii.h"
#if defined(SUPPORT_SMPU98)
#include	"cbus/smpu98.h"
#endif
#if defined(SUPPORT_IDEIO)
#include	"cbus/ideio.h"
#endif
#if defined(SUPPORT_WAB) && defined(SUPPORT_CL_GD5430)
#include	"wab/cirrus_vga_extern.h"
#include	"wab/wab.h"
#endif
#include	"np2.h"
#if defined(SUPPORT_HRTIMER)
#include	"timemng.h"
#endif


#define		NP2SYSP_VER			"C"
// #define	NP2SYSP_CREDIT		""					// 要るなら・・・

// NP2依存ポート
// port:07edh	np2 value comm
// port:07efh	np2 string comm

// 基本的に STRINGでやり取りする
// ポート 7efh に 'NP2' と出力で "NP2"が返ってきたら NP2である

// verA
//		out->str: 'ver'				in->str:	ver番号 A〜
//		out->str: 'poweroff'		NP2を終了

// verB
//		out->str: 'cpu'				in->str:	CPU型番
//		out->str: 'clock'			in->str:	動作クロック数


// ----

typedef struct {
const char	*key;
	void	(*func)(const void *arg1, long arg2);
const void	*arg1;
	long	arg2;
} SYSPCMD;

static const OEMCHAR str_80286[] = OEMTEXT("80286");
static const OEMCHAR str_v30[] = OEMTEXT("V30");
#if defined(CPUCORE_IA32)
static const OEMCHAR str_pentium[] = OEMTEXT("PENTIUM");
#endif	/* 0 */
static const OEMCHAR str_mhz[] = OEMTEXT("%uMHz");

// np21/w dynamic config
#define NP21W_SWITCH_DUMMY				0
#define NP21W_SWITCH_GD54XXTYPE			1
#define NP21W_SWITCH_SOUNDBOARD			2
#define NP21W_SWITCH_SYNCCLOCK			3
#define NP21W_SWITCH_PCIENABLE			4
#define NP21W_SWITCH_ASYNCCPU			5
#define NP21W_SWITCH_DISABLESOUNDROM	6


static void setoutstr(const OEMCHAR *str) {

#if defined(OSLANG_UCS2)
	oemtext_oemtosjis(np2sysp.outstr, sizeof(np2sysp.outstr), str, -1);
#else
	milstr_ncpy(np2sysp.outstr, str, sizeof(np2sysp.outstr));
#endif
	np2sysp.outpos = 0;
}

void np2sysp_outstr(const void *arg1, long arg2) {

	setoutstr((OEMCHAR *)arg1);
	(void)arg2;
}

static void np2sysp_poweroff(const void *arg1, long arg2) {

	taskmng_exit();
	(void)arg1;
	(void)arg2;
}

static void np2sysp_cpu(const void *arg1, long arg2) {

	// CPUを返す
#if !defined(CPUCORE_IA32)							// 80286 or V30
	if (!(CPU_TYPE & CPUTYPE_V30)) {
		setoutstr(str_80286);
	}
	else {
		setoutstr(str_v30);
	}
#else
	// 386機以降の場合 V30モードはエミュレーションだから固定(?)
	setoutstr(str_pentium);
#endif
	(void)arg1;
	(void)arg2;
}

static void np2sysp_clock(const void *arg1, long arg2) {

	OEMCHAR	str[64];

	OEMSNPRINTF(str, sizeof(str), str_mhz, (pccore.realclock + 500000) / 1000000);
	setoutstr(str);
	(void)arg1;
	(void)arg2;
}

static void np2sysp_multiple(const void *arg1, long arg2) {

	OEMCHAR	str[64];

	OEMSNPRINTF(str, sizeof(str), str_u, pccore.multiple);
	setoutstr(str);
	(void)arg1;
	(void)arg2;
}

static void np2sysp_hwreset(const void *arg1, long arg2) {

	pcstat.hardwarereset = TRUE;
	(void)arg1;
	(void)arg2;
}

static void np2sysp_changeclock(const void *arg1, long arg2) {

	OEMCHAR	str[64];

	OEMSNPRINTF(str, sizeof(str), str_mhz, (pccore.realclock + 500000) / 1000000);
	setoutstr(str);
	(void)arg1;
	(void)arg2;
}

// np21/w extensions
static void np2sysp_cngclkmul(const void *arg1, long arg2) {

	OEMCHAR	str[64];
	UINT8 oldclockmul = pccore.maxmultiple;
	UINT8 newclockmul = (np2sysp.outval >> 24);
	
	if(newclockmul > 0) {
		np2sysp.outval = (oldclockmul << 24) + (np2sysp.outval >> 8);

		pccore.multiple = newclockmul;
		pccore.maxmultiple = newclockmul;
		pccore.realclock = pccore.baseclock * pccore.multiple;
		
		sound_changeclock();
		beep_changeclock();
		mpu98ii_changeclock();
#if defined(SUPPORT_SMPU98)
		smpu98_changeclock();
#endif
		keyboard_changeclock();
		mouseif_changeclock();
		gdc_updateclock();
	}

	OEMSPRINTF(str, OEMTEXT("%d"), pccore.maxmultiple);
	setoutstr(str);
	(void)arg1;
	(void)arg2;
}

static void np2sysp_getconfig(const void *arg1, long arg2) {

	OEMCHAR	str[64];
	UINT8 configid = (np2sysp.outval >> 24) & 0xff;
	UINT8 configvalue = 0;
	UINT16 configvalue16 = 0;
	
	switch(configid){
	case NP21W_SWITCH_SOUNDBOARD:
		configvalue = g_nSoundID;
		break;
	case NP21W_SWITCH_GD54XXTYPE:
#if defined(SUPPORT_WAB) && defined(SUPPORT_CL_GD5430)
		configvalue16 = np2clvga.gd54xxtype;
#endif
		OEMSNPRINTF(str, sizeof(str), OEMTEXT("%d"), configvalue16);
		setoutstr(str);
		return;
	case NP21W_SWITCH_PCIENABLE:
#if defined(SUPPORT_PCI)
		configvalue = pcidev.enable ? 1 : 0;
#endif
		return;
	case NP21W_SWITCH_ASYNCCPU:
#if defined(SUPPORT_ASYNC_CPU)
		configvalue = np2cfg.asynccpu ? 1 : 0;
#endif	/* defined(SUPPORT_ASYNC_CPU) */
		break;
	case NP21W_SWITCH_DISABLESOUNDROM:
		configvalue = 0; // 常時0
		break;
	case NP21W_SWITCH_DUMMY:
	default:
		break;
	}

	OEMSNPRINTF(str, sizeof(str), OEMTEXT("%d"), configvalue);
	setoutstr(str);
	(void)arg1;
	(void)arg2;
}
static void np2sysp_cngconfig(const void *arg1, long arg2) {

	OEMCHAR	str[64];
	UINT8 configid = (np2sysp.outval >> 24) & 0xff;
	UINT8 configvalue = (np2sysp.outval >> 16) & 0xff;
	UINT16 configvalue16 = 0;
	
	switch(configid){
	case NP21W_SWITCH_SYNCCLOCK:
#if defined(SUPPORT_HRTIMER)
		if(configvalue == 1){
			_SYSTIME hrtimertime;
			UINT32 hrtimertimeuint;

			timemng_gettime(&hrtimertime);
			hrtimertimeuint = (((UINT32)hrtimertime.hour*60 + (UINT32)hrtimertime.minute)*60 + (UINT32)hrtimertime.second)*32 + ((UINT32)hrtimertime.milli*32)/1000;
			hrtimertimeuint |= 0x400000; // こうしないとWin98の時計が1日ずれる?
			STOREINTELDWORD(mem+0x04F1, hrtimertimeuint); // XXX: 04F4にも書いちゃってるけど差し当たっては問題なさそうなので･･･
		}
#endif	/* defined(SUPPORT_HRTIMER) */
		break;
	case NP21W_SWITCH_SOUNDBOARD:
		if(configvalue != (UINT8)g_nSoundID){
			sound_reset();
			fmboard_unbind();
			if(g_nSoundID == SOUNDID_PC_9801_118 || g_nSoundID == SOUNDID_PC_9801_118_SB16){
				iocore_detachout(cs4231.port[10]);
				iocore_detachinp(cs4231.port[10]);
				iocore_detachout(cs4231.port[10]+1);
				iocore_detachinp(cs4231.port[10]+1);
			}
			soundrom_reset();
			fmboard_reset(&np2cfg, (SOUNDID)configvalue);
			fmboard_bind();
			if (((pccore.model & PCMODELMASK) >= PCMODEL_VX) &&
				(pccore.sound & 0x7e)) {
				if(g_nSoundID == SOUNDID_MATE_X_PCM || ((g_nSoundID == SOUNDID_PC_9801_118 || g_nSoundID == SOUNDID_PC_9801_86_118 || g_nSoundID == SOUNDID_PC_9801_118_SB16 || g_nSoundID == SOUNDID_PC_9801_86_118_SB16) && np2cfg.snd118irqf == np2cfg.snd118irqp) || g_nSoundID == SOUNDID_PC_9801_86_WSS || g_nSoundID == SOUNDID_WAVESTAR || g_nSoundID==SOUNDID_PC_9801_86_WSS_SB16){
					iocore_out8(0x188, 0x27);
					iocore_out8(0x18a, 0x30);
					if(g_nSoundID == SOUNDID_PC_9801_118 || g_nSoundID == SOUNDID_PC_9801_86_118 || g_nSoundID==SOUNDID_WAVESTAR || g_nSoundID == SOUNDID_PC_9801_118_SB16 || g_nSoundID == SOUNDID_PC_9801_86_118_SB16){
						iocore_out8(cs4231.port[4], 0x27);
						iocore_out8(cs4231.port[4]+2, 0x30);
					}
				}else{
					iocore_out8(0x188, 0x27);
					iocore_out8(0x18a, 0x3f);
				}
			}
			if(g_nSoundID == SOUNDID_PC_9801_118 || g_nSoundID == SOUNDID_PC_9801_86_118 || g_nSoundID == SOUNDID_PC_9801_118_SB16 || g_nSoundID == SOUNDID_PC_9801_86_118_SB16){
				iocore_attachout(cs4231.port[10], mpu98ii_o0);
				iocore_attachinp(cs4231.port[10], mpu98ii_i0);
				iocore_attachout(cs4231.port[10]+1, mpu98ii_o2);
				iocore_attachinp(cs4231.port[10]+1, mpu98ii_i2);
				switch(np2cfg.snd118irqm){
				case 10:
					mpu98.irqnum = 10;
					break;
				}
			}
#if defined(SUPPORT_IDEIO)
			ideio_bindCDDA();
#endif	/* defined(SUPPORT_IDEIO) */
		}
		break;
	case NP21W_SWITCH_GD54XXTYPE:
#if defined(SUPPORT_WAB) && defined(SUPPORT_CL_GD5430)
		pc98_cirrus_vga_unbind();
		configvalue16 = (np2sysp.outval >> 8) & 0xffff;
		if(configvalue16 != 0){
			np2clvga.gd54xxtype = configvalue16;
		}else{
			np2clvga.gd54xxtype = np2clvga.defgd54xxtype;
		}
		pc98_cirrus_vga_bind();
		np2clvga.VRAMWindowAddr2 = 0;
		np2clvga.VRAMWindowAddr3 = 0;
		pc98_cirrus_vga_initVRAMWindowAddr();
		np2clvga.mmioenable = 0;
		np2wab.paletteChanged = 1;
		pc98_cirrus_vga_resetresolution();
#endif
		OEMSNPRINTF(str, sizeof(str), OEMTEXT("%d"), configvalue16);
		setoutstr(str);
		return;
	case NP21W_SWITCH_PCIENABLE:
#if defined(SUPPORT_PCI)
		if(configvalue==0){
			pcidev.enable = configvalue;
		}else{
			// TODO: 無効→有効にする場合のコードを書く
		}
		configvalue = pcidev.enable;
#endif
		return;
	case NP21W_SWITCH_ASYNCCPU:
#if defined(SUPPORT_ASYNC_CPU)
		np2cfg.asynccpu = configvalue ? 1 : 0;
		configvalue = np2cfg.asynccpu ? 1 : 0;
#endif	/* defined(SUPPORT_ASYNC_CPU) */
		break;
	case NP21W_SWITCH_DISABLESOUNDROM:
		if(configvalue == 1){
			soundrom_reset(); // サウンドROMを消す
		}
		break;
	case NP21W_SWITCH_DUMMY:
	default:
		break;
	}

	OEMSNPRINTF(str, sizeof(str), OEMTEXT("%d"), configvalue);
	setoutstr(str);
	(void)arg1;
	(void)arg2;
}


// ----

static const char cmd_np2[] = "NP2";
static const OEMCHAR rep_np2[] = OEMTEXT("NP2");

static const char cmd_ver[] = "ver";
static const char cmd_poweroff[] = "poweroff";
static const char cmd_credit[] = "credit";
static const char cmd_cpu[] = "cpu";
static const char cmd_clock[] = "clock";
static const char cmd_multiple[] = "multiple";
static const char cmd_hwreset[] = "hardwarereset";
#if defined(SUPPORT_IDEIO) || defined(SUPPORT_SASI)
static const char cmd_sasibios[] = "sasibios";
#endif
#if defined(SUPPORT_SCSI)
static const char cmd_scsibios[] = "scsibios";
static const char cmd_scsidev[] = "scsi_dev";
#endif
#if defined(SUPPORT_HOSTDRV)
static const char cmd_hdrvcheck[] = "check_hostdrv";
static const char cmd_hdrvopen[] = "open_hostdrv";
static const char cmd_hdrvclose[] = "close_hostdrv";
static const char cmd_hdrvintr[] = "intr_hostdrv";
static const OEMCHAR rep_hdrvcheck[] = OEMTEXT("0.74");
#endif

// np21/w extensions
static const char cmd_cngclkmul[] = "changeclockmul"; // np21w ver0.86 rev44
static const char cmd_cngconfig[] = "changeconfig"; // np21w ver0.86 rev44
static const char cmd_getconfig[] = "getconfig"; // np21w ver0.86 rev44

#if defined(NP2SYSP_VER)
static const OEMCHAR str_syspver[] = OEMTEXT(NP2SYSP_VER);
#else
#define	str_syspver		str_null
#endif

#if defined(NP2SYSP_CREDIT)
static const OEMCHAR str_syspcredit[] = OEMTEXT(NP2SYSP_CREDIT);
#else
#define	str_syspcredit	str_null
#endif


static const SYSPCMD np2spcmd[] = {
			{cmd_np2,		np2sysp_outstr,		rep_np2,		0},
			{cmd_ver,		np2sysp_outstr,		str_syspver,	0},

// version:A
			{cmd_poweroff,	np2sysp_poweroff,	NULL,			0},

// version:B
			{cmd_credit,	np2sysp_outstr,		str_syspcredit,	0},
			{cmd_cpu,		np2sysp_cpu,		NULL,			0},
			{cmd_clock,		np2sysp_clock,		NULL,			0},
			{cmd_multiple,	np2sysp_multiple,	NULL,			0},

// version:C
			{cmd_hwreset,	np2sysp_hwreset,	NULL,			0},

// extension
#if defined(SUPPORT_IDEIO) || defined(SUPPORT_SASI)
			{cmd_sasibios,	np2sysp_sasi,		NULL,			0},
#endif
#if defined(SUPPORT_SCSI)
			{cmd_scsibios,	np2sysp_scsi,		NULL,			0},
			{cmd_scsidev,	np2sysp_scsidev,	NULL,			0},
#endif

#if defined(SUPPORT_HOSTDRV)
			{cmd_hdrvcheck,	np2sysp_outstr,		rep_hdrvcheck,	0},
			{cmd_hdrvopen,	hostdrv_mount,		NULL,			0},
			{cmd_hdrvclose,	hostdrv_unmount,	NULL,			0},
			{cmd_hdrvintr,	hostdrv_intr,		NULL,			0},
#endif
			{cmd_cngclkmul,	np2sysp_cngclkmul,	NULL,			0},
			{cmd_getconfig,	np2sysp_getconfig,	NULL,			0},
			{cmd_cngconfig,	np2sysp_cngconfig,	NULL,			0},
};


static BRESULT np2syspcmp(const char *p) {

	int		len;
	int		pos;

	len = (int)STRLEN(p);
	if (!len) {
		return(FAILURE);
	}
	pos = np2sysp.strpos;
	while(len--) {
		if (p[len] != np2sysp.substr[pos]) {
			return(FAILURE);
		}
		pos--;
		pos &= NP2SYSP_MASK;
	}
	return(SUCCESS);
}

static void IOOUTCALL np2sysp_o7ed(UINT port, REG8 dat) {

	np2sysp.outval = (dat << 24) | (np2sysp.outval >> 8);
	(void)port;
}

static void IOOUTCALL np2sysp_o7ef(UINT port, REG8 dat) {

const SYSPCMD	*cmd;
const SYSPCMD	*cmdterm;

	np2sysp.substr[np2sysp.strpos] = (char)dat;
	cmd = np2spcmd;
	cmdterm = cmd + NELEMENTS(np2spcmd);
	while(cmd < cmdterm) {
		if (!np2syspcmp(cmd->key)) {
			cmd->func(cmd->arg1, cmd->arg2);
			break;
		}
		cmd++;
	}
	np2sysp.strpos++;
	np2sysp.strpos &= NP2SYSP_MASK;
	(void)port;
}

static REG8 IOINPCALL np2sysp_i7ed(UINT port) {

	REG8	ret;

	ret = (REG8)(np2sysp.inpval & 0xff);
	np2sysp.inpval = (ret << 24) | (np2sysp.inpval >> 8);
	(void)port;
	return(ret);
}

static REG8 IOINPCALL np2sysp_i7ef(UINT port) {

	REG8	ret;

	ret = (UINT8)np2sysp.outstr[np2sysp.outpos];
	if (ret) {
		np2sysp.outpos++;
		np2sysp.outpos &= NP2SYSP_MASK;
	}
	(void)port;
	return(ret);
}

#if defined(NP2APPDEV)
static void IOOUTCALL np2sysp_o0e9(UINT port, REG8 dat) {

	APPDEVOUT(dat);
	(void)port;
}

static REG8 IOINPCALL np2sysp_i0e9(UINT port) {

	return((UINT8)port);
}
#endif


// ---- I/F

void np2sysp_reset(const NP2CFG *pConfig) {

	ZeroMemory(&np2sysp, sizeof(np2sysp));

	(void)pConfig;
}

void np2sysp_bind(void) {

	iocore_attachout(0x07ed, np2sysp_o7ed);
	iocore_attachout(0x07ef, np2sysp_o7ef);
	iocore_attachinp(0x07ed, np2sysp_i7ed);
	iocore_attachinp(0x07ef, np2sysp_i7ef);

#if defined(NP2APPDEV)
	iocore_attachout(0x00e9, np2sysp_o0e9);
	iocore_attachinp(0x00e9, np2sysp_i0e9);
#endif
}

