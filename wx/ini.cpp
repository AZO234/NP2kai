/* === TOML-based configuration for wx port ===
 * Uses libtomlplusplus (toml++) for reading/writing TOML files.
 */

#include <compiler.h>
#include "ini.h"
#include "pccore.h"
#include "np2.h"
#include <dosio.h>
#include <sysmng.h>
#if defined(SUPPORT_WAB)
#include <wab/wab.h>
#endif

#include <sys/stat.h>

#include <toml++/toml.hpp>
#include <fstream>
#include <string>
#include <vector>

#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>

/* ---- helper for memory switches / dipswitches ---- */

static void inisetbmp(UINT8 *ptr, UINT32 pos, bool set)
{
	UINT8 bit = (UINT8)(1 << (pos & 7));
	ptr += (pos >> 3);
	if (set) *ptr |= bit; else *ptr &= ~bit;
}

/* ---- value → TOML node ---- */

static void ini_tbl_write(toml::table &sec, const INITBL *p)
{
	if (p->itemtype & INIFLAG_RO) return;

	switch (p->itemtype & INITYPE_MASK) {
	case INITYPE_STR:
		sec.insert_or_assign(p->item, std::string((const char *)p->value));
		break;
	case INITYPE_BOOL:
		sec.insert_or_assign(p->item, *(const UINT8 *)p->value != 0);
		break;
	case INITYPE_BITMAP:
		sec.insert_or_assign(p->item,
		    ((((const UINT8 *)p->value)[p->arg >> 3] >> (p->arg & 7)) & 1) != 0);
		break;
	case INITYPE_HEX8:
	case INITYPE_UINT8:
		sec.insert_or_assign(p->item, (int64_t)*(const UINT8 *)p->value);
		break;
	case INITYPE_SINT8:
		sec.insert_or_assign(p->item, (int64_t)*(const SINT8 *)p->value);
		break;
	case INITYPE_HEX16:
	case INITYPE_UINT16:
		sec.insert_or_assign(p->item, (int64_t)*(const UINT16 *)p->value);
		break;
	case INITYPE_SINT16:
		sec.insert_or_assign(p->item, (int64_t)*(const SINT16 *)p->value);
		break;
	case INITYPE_HEX32:
	case INITYPE_UINT32:
		sec.insert_or_assign(p->item, (int64_t)*(const UINT32 *)p->value);
		break;
	case INITYPE_SINT32:
		sec.insert_or_assign(p->item, (int64_t)*(const SINT32 *)p->value);
		break;
	case INITYPE_HEX64:
	case INITYPE_UINT64:
	case INITYPE_SINT64:
		sec.insert_or_assign(p->item, (int64_t)*(const UINT64 *)p->value);
		break;
	case INITYPE_ARGU32: {
		const UINT32 *arr = (const UINT32 *)p->value;
		toml::array a;
		for (UINT i = 0; i < p->arg; i++) a.push_back((int64_t)arr[i]);
		sec.insert_or_assign(p->item, std::move(a));
		break;
	}
	case INITYPE_ARGH8: {
		const UINT8 *arr = (const UINT8 *)p->value;
		toml::array a;
		for (UINT i = 0; i < p->arg; i++) a.push_back((int64_t)arr[i]);
		sec.insert_or_assign(p->item, std::move(a));
		break;
	}
	case INITYPE_ARGS16: {
		const int16_t *arr = (const int16_t *)p->value;
		toml::array a;
		for (UINT i = 0; i < p->arg; i++) a.push_back((int64_t)arr[i]);
		sec.insert_or_assign(p->item, std::move(a));
		break;
	}
	case INITYPE_USER:
		if (strcmp(p->item, "INTERPOL") == 0) {
			const char *s;
			switch (*(const UINT8 *)p->value) {
			case INTERP_NEAREST: s = "NEAREST"; break;
			case INTERP_TILES:   s = "TILES";   break;
			case INTERP_HYPER:   s = "HYPER";   break;
			default:             s = "BILINEAR";
			}
			sec.insert_or_assign(p->item, std::string(s));
			return;
		}
		break;
	}
}

/* ---- TOML node → value ---- */

static void ini_tbl_read(const toml::table &sec, const INITBL *p)
{
	auto node = sec.get(p->item);
	if (!node) return;

	std::string sv = "";
	int64_t     iv = 0;
	bool        got_int = false;
	bool        got_str = false;

	if (auto val = node->as_string()) {
		sv = val->get();
		got_str = true;
	} else if (auto val = node->as_integer()) {
		iv = val->get();
		got_int = true;
	} else if (auto val = node->as_boolean()) {
		iv = val->get() ? 1 : 0;
		got_int = true;
	}

	switch (p->itemtype & INITYPE_MASK) {
	case INITYPE_STR:
		if (got_str) milstr_ncpy((char *)p->value, sv.c_str(), p->arg);
		break;
	case INITYPE_BOOL:
		if (got_int) *(UINT8 *)p->value = (UINT8)(iv != 0);
		break;
	case INITYPE_BITMAP:
		if (got_int) inisetbmp((UINT8 *)p->value, p->arg, iv != 0);
		break;
	case INITYPE_HEX8:
		if (got_int) *(UINT8 *)p->value = (UINT8)iv;
		if (p->itemtype & INIFLAG_AND)
			*(UINT8 *)p->value &= (UINT8)p->arg;
		break;
	case INITYPE_UINT8:
	case INITYPE_SINT8:
		if (got_int) *(UINT8 *)p->value = (UINT8)iv;
		if ((p->itemtype & INIFLAG_MAX) && *(UINT8 *)p->value > (UINT8)p->arg)
			*(UINT8 *)p->value = (UINT8)p->arg;
		if (p->itemtype & INIFLAG_AND)
			*(UINT8 *)p->value &= (UINT8)p->arg;
		break;
	case INITYPE_HEX16:
		if (got_int) *(UINT16 *)p->value = (UINT16)iv;
		break;
	case INITYPE_UINT16:
	case INITYPE_SINT16:
		if (got_int) *(UINT16 *)p->value = (UINT16)iv;
		if ((p->itemtype & INIFLAG_MAX) && *(UINT16 *)p->value > (UINT16)p->arg)
			*(UINT16 *)p->value = (UINT16)p->arg;
		break;
	case INITYPE_HEX32:
	case INITYPE_UINT32:
	case INITYPE_SINT32:
		if (got_int) *(UINT32 *)p->value = (UINT32)iv;
		break;
	case INITYPE_HEX64:
	case INITYPE_UINT64:
	case INITYPE_SINT64:
		if (got_int) *(UINT64 *)p->value = (UINT64)iv;
		break;
	case INITYPE_ARGU32: {
		UINT32 *arr = (UINT32 *)p->value;
		if (auto a = node->as_array()) {
			for (UINT i = 0; i < p->arg && i < a->size(); i++)
				if (auto v = (*a)[i].as_integer()) arr[i] = (UINT32)v->get();
		}
		break;
	}
	case INITYPE_ARGH8: {
		UINT8 *arr = (UINT8 *)p->value;
		if (auto a = node->as_array()) {
			for (UINT i = 0; i < p->arg && i < a->size(); i++)
				if (auto v = (*a)[i].as_integer()) arr[i] = (UINT8)v->get();
		}
		break;
	}
	case INITYPE_ARGS16: {
		int16_t *arr = (int16_t *)p->value;
		if (auto a = node->as_array()) {
			for (UINT i = 0; i < p->arg && i < a->size(); i++)
				if (auto v = (*a)[i].as_integer()) arr[i] = (int16_t)v->get();
		}
		break;
	}
	case INITYPE_USER:
		if (strcmp(p->item, "INTERPOL") == 0 && got_str) {
			if (sv == "NEAREST")      *(UINT8 *)p->value = INTERP_NEAREST;
			else if (sv == "TILES")   *(UINT8 *)p->value = INTERP_TILES;
			else if (sv == "HYPER")   *(UINT8 *)p->value = INTERP_HYPER;
			else                      *(UINT8 *)p->value = INTERP_BILINEAR;
		}
		break;
	}
}

/* ---- global config table ---- */

extern UINT32 cycle_shot_interval;
extern char   cycle_shot_path[512];

/* Keys of the Windows and SDL ports where this port had none of its own, so a
 * setting reads the same everywhere. */
static const INITBL np2_tbl[] = {
	{"pc_model",  INITYPE_STR,    np2cfg.model,        sizeof(np2cfg.model)},
	{"clk_base",  INITYPE_UINT32, &np2cfg.baseclock,   0},
	{"clk_mult",  INITYPE_UINT32, &np2cfg.multiple,    0},
	{"EmuSpeed",  INITYPE_UINT32, &np2cfg.emuspeed,    100},
	{"DIPswtch",  INITYPE_ARGH8,  np2cfg.dipsw,        3},
	{"MEMswtch",  INITYPE_ARGH8,  np2cfg.memsw,        8},
#if defined(SUPPORT_LARGE_MEMORY)
	{"ExMemory",  INITYPE_UINT16, &np2cfg.EXTMEM,      1024},
#else
	{"ExMemory",  INITYPE_UINT8 | INIFLAG_MAX, &np2cfg.EXTMEM, 16},
#endif
	{"ITF_WORK",  INITYPE_BOOL | INIFLAG_RO, &np2cfg.ITF_WORK, 1},
#if defined(SUPPORT_FAST_MEMORYCHECK)
	{"MemCheck",  INITYPE_UINT8,  &np2cfg.memcheckspeed, 8},
#endif
#if defined(CPUCORE_IA32)
	{"SYSIOMSK",  INITYPE_HEX16,  &np2cfg.sysiomsk,     0xd1},
#else
	{"SYSIOMSK",  INITYPE_HEX16,  &np2cfg.sysiomsk,     0},
#endif
	{"nousemmx",  INITYPE_BOOL,   &np2oscfg.disablemmx, 0},
#if defined(SUPPORT_WAB)
	{"MTENABLE",  INITYPE_SINT32, &np2wabcfg.multithread, 0},
#endif
	{"TIMERFIX",  INITYPE_BOOL,   &np2cfg.timerfix,     0},
	{"CONSTTSC",  INITYPE_BOOL,   &np2cfg.consttsc,     0},
#if defined(CPUCORE_IA32)
	{"cpu_vend",  INITYPE_STR,    np2cfg.cpu_vendor_o,  sizeof(np2cfg.cpu_vendor_o)},
	{"cpu_fami",  INITYPE_UINT32, &np2cfg.cpu_family,   0},
	{"cpu_mode",  INITYPE_UINT32, &np2cfg.cpu_model,    0},
	{"cpu_step",  INITYPE_UINT32, &np2cfg.cpu_stepping, 0},
	{"cpu_feat",  INITYPE_HEX32,  &np2cfg.cpu_feature,  0},
	{"cpu_f_ex",  INITYPE_HEX32,  &np2cfg.cpu_feature_ex, 0},
	{"cpu_bran",  INITYPE_STR | INIFLAG_RO, np2cfg.cpu_brandstring_o, sizeof(np2cfg.cpu_brandstring_o)},
	{"cpu_brid",  INITYPE_HEX32,  &np2cfg.cpu_brandid,  0},
	{"cpu_fecx",  INITYPE_HEX32,  &np2cfg.cpu_feature_ecx, 0},
	{"cpu_fxcx",  INITYPE_HEX32,  &np2cfg.cpu_feature_ex_ecx, 0},
	{"cpu_eflg",  INITYPE_HEX32,  &np2cfg.cpu_eflags_mask, 0},
	{"FPU_TYPE",  INITYPE_UINT8,  &np2cfg.fpu_type,     0},
#endif

	{"FDfolder",  INITYPE_STR,    fddfolder,           MAX_PATH},
	{"HDfolder",  INITYPE_STR,    hddfolder,           MAX_PATH},
	{"CDfolder",  INITYPE_STR,    cdfolder,            MAX_PATH},
	{"fontfile",  INITYPE_STR,    np2cfg.fontfile,     MAX_PATH},
	{"biospath",  INITYPE_STR | INIFLAG_RO, np2cfg.biospath, MAX_PATH},
	{"FDD1FILE",  INITYPE_STR,    np2cfg.fddfile[0],   MAX_PATH},
	{"FDD2FILE",  INITYPE_STR,    np2cfg.fddfile[1],   MAX_PATH},
	{"FDD3FILE",  INITYPE_STR,    np2cfg.fddfile[2],   MAX_PATH},
	{"FDD4FILE",  INITYPE_STR,    np2cfg.fddfile[3],   MAX_PATH},
	{"FDDRIVE1",  INITYPE_BITMAP, &np2cfg.fddequip,    0},
	{"FDDRIVE2",  INITYPE_BITMAP, &np2cfg.fddequip,    1},
	{"FDDRIVE3",  INITYPE_BITMAP, &np2cfg.fddequip,    2},
	{"FDDRIVE4",  INITYPE_BITMAP, &np2cfg.fddequip,    3},
	{"USE144FD",  INITYPE_BOOL,   &np2cfg.usefd144,    0},
	{"HDD1FILE",  INITYPE_STR,    np2cfg.sasihdd[0],   MAX_PATH},
	{"HDD2FILE",  INITYPE_STR,    np2cfg.sasihdd[1],   MAX_PATH},
#if defined(SUPPORT_IDEIO)
	{"HDD3FILE",  INITYPE_STR,    np2cfg.sasihdd[2],   MAX_PATH},
	{"HDD4FILE",  INITYPE_STR,    np2cfg.sasihdd[3],   MAX_PATH},
	{"IDE1TYPE",  INITYPE_UINT8,  &np2cfg.idetype[0],  0},
	{"IDE2TYPE",  INITYPE_UINT8,  &np2cfg.idetype[1],  0},
	{"IDE3TYPE",  INITYPE_UINT8,  &np2cfg.idetype[2],  0},
	{"IDE4TYPE",  INITYPE_UINT8,  &np2cfg.idetype[3],  0},
	{"CD1_FILE",  INITYPE_STR,    np2cfg.idecd[0],     MAX_PATH},
	{"CD2_FILE",  INITYPE_STR,    np2cfg.idecd[1],     MAX_PATH},
	{"CD3_FILE",  INITYPE_STR,    np2cfg.idecd[2],     MAX_PATH},
	{"CD4_FILE",  INITYPE_STR,    np2cfg.idecd[3],     MAX_PATH},
	{"IDE_BIOS",  INITYPE_BOOL,   &np2cfg.idebios,     0},
	{"AIDEBIOS",  INITYPE_BOOL,   &np2cfg.autoidebios, 0},
	{"IDERWAIT",  INITYPE_UINT32, &np2cfg.iderwait,    0},
	{"IDEWWAIT",  INITYPE_UINT32, &np2cfg.idewwait,    0},
	{"CD_ASYNC",  INITYPE_BOOL,   &np2cfg.useasynccd,  0},
	{"CDTRAYOP",  INITYPE_BOOL,   &np2cfg.allowcdtraycmd, 0},
	{"USECDECC",  INITYPE_BOOL,   &np2cfg.usecdecc,    0},
#endif
#if defined(SUPPORT_SCSI)
	{"SCSIHDD0",  INITYPE_STR,    np2cfg.scsihdd[0],   MAX_PATH},
	{"SCSIHDD1",  INITYPE_STR,    np2cfg.scsihdd[1],   MAX_PATH},
	{"SCSIHDD2",  INITYPE_STR,    np2cfg.scsihdd[2],   MAX_PATH},
	{"SCSIHDD3",  INITYPE_STR,    np2cfg.scsihdd[3],   MAX_PATH},
#endif

	{"DispSync",  INITYPE_BOOL,   &np2cfg.DISPSYNC,    0},
	{"Real_Pal",  INITYPE_BOOL,   &np2cfg.RASTER,      0},
	{"RPal_tim",  INITYPE_UINT8 | INIFLAG_MAX, &np2cfg.realpal, 64},
	{"VRAMwait",  INITYPE_ARGH8,  np2cfg.wait,         6},
	{"uPD72020",  INITYPE_BOOL,   &np2cfg.uPD72020,    0},
	{"GRCG_EGC",  INITYPE_UINT8 | INIFLAG_AND, &np2cfg.grcg, 3},
	{"color16b",  INITYPE_BOOL,   &np2cfg.color16,     0},
	{"USEPEGCP",  INITYPE_BOOL,   &np2cfg.usepegcplane, 0},
	{"GDCWOVRG",  INITYPE_BOOL,   &np2cfg.gdcwovrg,    0},
	{"skipline",  INITYPE_BOOL,   &np2cfg.skipline,    0},
	{"skplight",  INITYPE_UINT16, &np2cfg.skiplight,   0},
	{"LCD_MODE",  INITYPE_UINT8 | INIFLAG_AND, &np2cfg.LCD_MODE, 3},
	{"INTERPOL",  INITYPE_USER,   &np2oscfg.drawinterp, 0},
	{"draw32bit", INITYPE_BOOL,   &draw32bit,          0},
#if defined(SUPPORT_WAB)
	{"WAB_ANSW",  INITYPE_UINT8,  &np2cfg.wabasw,      0},
#endif
#if defined(SUPPORT_CL_GD5430)
	{"USE_CLGD",  INITYPE_BOOL,   &np2cfg.usegd5430,   0},
	{"CLGDTYPE",  INITYPE_UINT16, &np2cfg.gd5430type,  0},
	{"CLGDFCUR",  INITYPE_BOOL,   &np2cfg.gd5430fakecur, 0},
#endif
#if defined(SUPPORT_PCI)
	{"USE98PCI",  INITYPE_BOOL,   &np2cfg.usepci,      0},
	{"PCI_PCMC",  INITYPE_UINT8,  &np2cfg.pci_pcmc,    0},
	{"P_BIOS32",  INITYPE_BOOL,   &np2cfg.pci_bios32,  0},
#endif

	{"SampleHz",  INITYPE_UINT32, &np2cfg.samplingrate, 0},
	{"Latencys",  INITYPE_UINT16, &np2cfg.delayms,      0},
	{"SNDboard",  INITYPE_HEX8,   &np2cfg.SOUND_SW,    0},
	{"BEEP_vol",  INITYPE_UINT8 | INIFLAG_AND, &np2cfg.BEEP_VOL, 3},
	{"volume_M",  INITYPE_UINT8 | INIFLAG_MAX, &np2cfg.vol_master,  100},
	{"volume_F",  INITYPE_UINT8 | INIFLAG_MAX, &np2cfg.vol_fm,      128},
	{"volume_S",  INITYPE_UINT8 | INIFLAG_MAX, &np2cfg.vol_ssg,     128},
	{"volume_A",  INITYPE_UINT8 | INIFLAG_MAX, &np2cfg.vol_adpcm,   128},
	{"volume_P",  INITYPE_UINT8 | INIFLAG_MAX, &np2cfg.vol_pcm,     128},
	{"volume_R",  INITYPE_UINT8 | INIFLAG_MAX, &np2cfg.vol_rhythm,  128},
	{"volume_V",  INITYPE_UINT8 | INIFLAG_MAX, &np2cfg.vol_midi,    128},
	{"DAVOLUME",  INITYPE_UINT8,  &np2cfg.davolume,    0},
	{"SND14vol",  INITYPE_ARGH8,  np2cfg.vol14,        6},
	{"opt26BRD",  INITYPE_HEX8,   &np2cfg.snd26opt,    0},
	{"opt86BRD",  INITYPE_HEX8,   &np2cfg.snd86opt,    0},
	{"optSPBRD",  INITYPE_HEX8,   &np2cfg.spbopt,      0},
	{"optSPBVR",  INITYPE_HEX8,   &np2cfg.spb_vrc,     0},
	{"optSPBVL",  INITYPE_UINT8 | INIFLAG_MAX, &np2cfg.spb_vrl, 24},
	{"optSPB_X",  INITYPE_BOOL,   &np2cfg.spb_x,       0},
	{"opt118io",  INITYPE_HEX16,  &np2cfg.snd118io,    0},
	{"opt118id",  INITYPE_HEX8,   &np2cfg.snd118id,    0},
	{"opt118dm",  INITYPE_UINT8,  &np2cfg.snd118dma,   0},
	{"opt118if",  INITYPE_UINT8,  &np2cfg.snd118irqf,  0},
	{"opt118ip",  INITYPE_UINT8,  &np2cfg.snd118irqp,  0},
	{"opt118im",  INITYPE_UINT8,  &np2cfg.snd118irqm,  0},
	{"opt118ro",  INITYPE_UINT8,  &np2cfg.snd118rom,   0},
	{"optwssid",  INITYPE_HEX8,   &np2cfg.sndwssid,    0},
	{"optwssdm",  INITYPE_UINT8,  &np2cfg.sndwssdma,   0},
	{"optwssip",  INITYPE_UINT8,  &np2cfg.sndwssirq,   0},
	{"optwsio",   INITYPE_HEX16,  &np2cfg.sndwsio,     0},
	{"optwsdm",   INITYPE_UINT8,  &np2cfg.sndwsdma,    0},
	{"optwsirq",  INITYPE_UINT8,  &np2cfg.sndwsirq,    0},
	{"optwspnp",  INITYPE_BOOL,   &np2cfg.sndwspnp,    0},
	{"optwspcm",  INITYPE_BOOL,   &np2cfg.sndwspcm,    0},
#if defined(SUPPORT_SOUND_SB16)
	{"optsb16p",  INITYPE_HEX8,   &np2cfg.sndsb16io,   0},
	{"optsb16d",  INITYPE_UINT8,  &np2cfg.sndsb16dma,  0},
	{"optsb16i",  INITYPE_UINT8,  &np2cfg.sndsb16irq,  0},
#endif
#if defined(SUPPORT_FMGEN)
	{"USEFMGEN",  INITYPE_BOOL,   &np2cfg.usefmgen,    0},
#endif
	{"Seek_Snd",  INITYPE_BOOL,   &np2cfg.MOTOR,       0},
	{"Seek_Vol",  INITYPE_UINT8 | INIFLAG_MAX, &np2cfg.MOTORVOL, 100},
	{"btn_mode",  INITYPE_BOOL,   &np2cfg.BTN_MODE,    0},
	{"btn_rapd",  INITYPE_BOOL,   &np2cfg.BTN_RAPID,   0},
	{"MS_RAPID",  INITYPE_BOOL,   &np2cfg.MOUSERAPID,  0},

	{"keyboard",  INITYPE_UINT8,  &np2oscfg.KEYBOARD,  0},
	{"MOUSEMUL",  INITYPE_SINT16, &np2oscfg.mousemul,  0},
	{"MOUSEDIV",  INITYPE_SINT16, &np2oscfg.mousediv,  0},
	{"RAWMOUSE",  INITYPE_BOOL,   &np2oscfg.rawmouse,  0},
	{"PixelPerfect", INITYPE_BOOL, &np2oscfg.pixelperfect, 0},
	{"Force43",   INITYPE_BOOL,   &np2oscfg.force43,   0},
	{"StatusBar", INITYPE_BOOL,   &np2oscfg.statusbar, 0},
	{"F11_KEY",   INITYPE_UINT8,  &np2oscfg.F11KEY,    0},
	{"F12_Copy",  INITYPE_UINT8,  &np2oscfg.F12KEY,    0},
	{"xrollkey",  INITYPE_BOOL,   &np2oscfg.xrollkey,  0},
	{"keyrepeat_enable",   INITYPE_BOOL,   &np2cfg.keyrepeat_enable,   0},
	{"keyrepeat_delay",    INITYPE_UINT16, &np2cfg.keyrepeat_delay,    0},
	{"keyrepeat_interval", INITYPE_UINT16, &np2cfg.keyrepeat_interval, 0},
	{"Mouse_sw",  INITYPE_BOOL,   &np2oscfg.MOUSE_SW,  0},
	{"Joypad1",   INITYPE_BOOL,   &np2oscfg.JOYPAD1,   0},
	{"Joy1POVC",  INITYPE_BOOL,   &np2oscfg.JOYPAD1POVXY, 0},
	{"Joy1_Btn",  INITYPE_ARGH8,  np2oscfg.JOY1BTN,    JOY_NBUTTON},
	{"Joy1_dev",  INITYPE_STR,    np2oscfg.JOYDEV[0],  MAX_PATH},
#if defined(SUPPORT_GAMEPORT)
	{"GAMEPORT",  INITYPE_BOOL,   &np2cfg.gameport,    0},
	{"ANLG_JOY",  INITYPE_BOOL,   &np2cfg.analogjoy,   0},
#endif

	{"WindPosX",  INITYPE_SINT32, &np2oscfg.winx,       0},
	{"WindPosY",  INITYPE_SINT32, &np2oscfg.winy,       0},

	{"com1port",  INITYPE_UINT8,  &np2oscfg.com[0].port,  0},
	{"com1dir",   INITYPE_BOOL,   &np2oscfg.com[0].direct, 0},
	{"com1para",  INITYPE_HEX8,   &np2oscfg.com[0].param, 0},
	{"com1_bps",  INITYPE_UINT32, &np2oscfg.com[0].speed, 0},
	{"com1_m_i",  INITYPE_STR,    np2oscfg.com[0].min,    MAX_PATH},
	{"com1_m_o",  INITYPE_STR,    np2oscfg.com[0].mout,   MAX_PATH},
	{"com1mmdl",  INITYPE_STR,    np2oscfg.com[0].mdl,    sizeof(np2oscfg.com[0].mdl)},
	{"com1mdef",  INITYPE_STR,    np2oscfg.com[0].def,    MAX_PATH},
	{"com1mdfe",  INITYPE_BOOL,   &np2oscfg.com[0].def_en, 0},
	{"com2port",  INITYPE_UINT8,  &np2oscfg.com[1].port,  0},
	{"com2dir",   INITYPE_BOOL,   &np2oscfg.com[1].direct, 0},
	{"com2para",  INITYPE_HEX8,   &np2oscfg.com[1].param, 0},
	{"com2_bps",  INITYPE_UINT32, &np2oscfg.com[1].speed, 0},
	{"com2_m_i",  INITYPE_STR,    np2oscfg.com[1].min,    MAX_PATH},
	{"com2_m_o",  INITYPE_STR,    np2oscfg.com[1].mout,   MAX_PATH},
	{"com2mmdl",  INITYPE_STR,    np2oscfg.com[1].mdl,    sizeof(np2oscfg.com[1].mdl)},
	{"com2mdef",  INITYPE_STR,    np2oscfg.com[1].def,    MAX_PATH},
	{"com2mdfe",  INITYPE_BOOL,   &np2oscfg.com[1].def_en, 0},
	{"com3port",  INITYPE_UINT8,  &np2oscfg.com[2].port,  0},
	{"com3dir",   INITYPE_BOOL,   &np2oscfg.com[2].direct, 0},
	{"com3para",  INITYPE_HEX8,   &np2oscfg.com[2].param, 0},
	{"com3_bps",  INITYPE_UINT32, &np2oscfg.com[2].speed, 0},
	{"com3_m_i",  INITYPE_STR,    np2oscfg.com[2].min,    MAX_PATH},
	{"com3_m_o",  INITYPE_STR,    np2oscfg.com[2].mout,   MAX_PATH},
	{"com3mmdl",  INITYPE_STR,    np2oscfg.com[2].mdl,    sizeof(np2oscfg.com[2].mdl)},
	{"com3mdef",  INITYPE_STR,    np2oscfg.com[2].def,    MAX_PATH},
	{"com3mdfe",  INITYPE_BOOL,   &np2oscfg.com[2].def_en, 0},
#if defined(SUPPORT_PC9861K)
	{"pc9861_e",  INITYPE_BOOL,   &np2cfg.pc9861enable, 0},
	{"pc9861_s",  INITYPE_ARGH8,  np2cfg.pc9861sw,     3},
	{"pc9861_j",  INITYPE_ARGH8,  np2cfg.pc9861jmp,    6},
#endif

	{"mpu98en",   INITYPE_BOOL,   &np2cfg.mpuenable,   0},
	{"mpu98opt",  INITYPE_HEX8,   &np2cfg.mpuopt,      0},
	{"mpu98dir",  INITYPE_BOOL,   &np2oscfg.mpu.direct, 0},
	{"mpu98dev",  INITYPE_STR,    np2oscfg.mpu.mdl,    sizeof(np2oscfg.mpu.mdl)},
	{"mpu98map",  INITYPE_STR,    np2oscfg.mpu.mout,   MAX_PATH},
	{"mpu98min",  INITYPE_STR,    np2oscfg.mpu.min,    MAX_PATH},
	{"mpu98def",  INITYPE_STR,    np2oscfg.mpu.def,    MAX_PATH},
	{"mpu98dfe",  INITYPE_BOOL,   &np2oscfg.mpu.def_en, 0},
	{"MIDIOUTd",  INITYPE_STR,    np2oscfg.MIDIDEV[0], MAX_PATH},
	{"MIDIIN_d",  INITYPE_STR,    np2oscfg.MIDIDEV[1], MAX_PATH},
#if defined(SUPPORT_SMPU98)
	{"USE_SMPU",  INITYPE_BOOL,   &np2cfg.smpuenable,  0},
	{"opt_SMPU",  INITYPE_HEX8,   &np2cfg.smpuopt,     0},
	{"SMPUMUTB",  INITYPE_BOOL,   &np2cfg.smpumuteB,   0},
	{"smpuAmap",  INITYPE_STR,    np2oscfg.smpuA.mout,   MAX_PATH},
	{"smpuAmin",  INITYPE_STR,    np2oscfg.smpuA.min,    MAX_PATH},
	{"smpuAmdl",  INITYPE_STR,    np2oscfg.smpuA.mdl,    sizeof(np2oscfg.smpuA.mdl)},
	{"smpuAdef",  INITYPE_STR,    np2oscfg.smpuA.def,    MAX_PATH},
	{"smpuAdfe",  INITYPE_BOOL,   &np2oscfg.smpuA.def_en, 0},
	{"MIDIOUAd",  INITYPE_STR,    np2oscfg.MIDIDEVA[0],  MAX_PATH},
	{"MIDIINAd",  INITYPE_STR,    np2oscfg.MIDIDEVA[1],  MAX_PATH},
	{"smpuBmap",  INITYPE_STR,    np2oscfg.smpuB.mout,   MAX_PATH},
	{"smpuBmin",  INITYPE_STR,    np2oscfg.smpuB.min,    MAX_PATH},
	{"smpuBmdl",  INITYPE_STR,    np2oscfg.smpuB.mdl,    sizeof(np2oscfg.smpuB.mdl)},
	{"smpuBdef",  INITYPE_STR,    np2oscfg.smpuB.def,    MAX_PATH},
	{"smpuBdfe",  INITYPE_BOOL,   &np2oscfg.smpuB.def_en, 0},
	{"MIDIOUBd",  INITYPE_STR,    np2oscfg.MIDIDEVB[0],  MAX_PATH},
	{"MIDIINBd",  INITYPE_STR,    np2oscfg.MIDIDEVB[1],  MAX_PATH},
#endif

#if defined(SUPPORT_NET)
	{"NP2NETTAP", INITYPE_STR,    np2cfg.np2nettap,    MAX_PATH},
	{"NP2NETPMM", INITYPE_BOOL,   &np2cfg.np2netpmm,   0},
#endif
#if defined(SUPPORT_LGY98)
	{"USELGY98",  INITYPE_BOOL,   &np2cfg.uselgy98,    0},
	{"LGY98_IO",  INITYPE_UINT16, &np2cfg.lgy98io,     0},
	{"LGY98IRQ",  INITYPE_UINT8,  &np2cfg.lgy98irq,    0},
	{"LGY98MAC",  INITYPE_ARGH8,  np2cfg.lgy98mac,     6},
#endif
#if defined(SUPPORT_HOSTDRV)
	{"use_hdrv",  INITYPE_BOOL,   &np2cfg.hdrvenable,  0},
	{"hdrvroot",  INITYPE_STR,    np2cfg.hdrvroot,     MAX_PATH},
	{"hdrv_acc",  INITYPE_UINT8,  &np2cfg.hdrvacc,     0},
#endif
	{"calendar",  INITYPE_BOOL,   &np2cfg.calendar,    0},
	{"CAL_VOFS",  INITYPE_SINT64, &np2cfg.cal_vofs,    0},

	{"s_NOWAIT",  INITYPE_BOOL,   &np2oscfg.NOWAIT,    0},
	{"SkpFrame",  INITYPE_UINT8,  &np2oscfg.DRAW_SKIP, 0},
	{"e_resume",  INITYPE_BOOL,   &np2oscfg.resume,    0},
#if defined(SUPPORT_STATSAVE)
	{"STATSAVE",  INITYPE_BOOL,   &np2cfg.statsave,    0},
#endif
	{"jast_snd",  INITYPE_BOOL,   &np2oscfg.jastsnd,   0},
	{"CycleInt",  INITYPE_UINT32, &cycle_shot_interval,     0},
	{"CyclePath", INITYPE_STR,    cycle_shot_path,     sizeof(cycle_shot_path)},
	{"CycleEn",   INITYPE_BOOL,   &np2oscfg.cycle_shot_enabled, 0},
	{"bmap_Dir",  INITYPE_STR,    bmpfilefolder,       MAX_PATH},
	{"keydispl",  INITYPE_BOOL,   &np2oscfg.keydisp,   0},
};

static const UINT np2_tbl_count = (UINT)(sizeof(np2_tbl) / sizeof(np2_tbl[0]));

#if defined(CPUCORE_IA32)
static const OEMCHAR ini_section[] = "NP21kai";
#else
static const OEMCHAR ini_section[] = "NP2kai";
#endif

/* ---- config path ---- */

static OEMCHAR s_inipath[MAX_PATH] = "";

void initgetfile(char *lpPath, unsigned int cbPath)
{
	if (s_inipath[0] == '\0') {
		const char *override_path = getenv("NP2KAI_CONFIG");
		if (override_path) {
			milstr_ncpy(s_inipath, override_path, sizeof(s_inipath));
		} else {
			const char *confdir = getenv("XDG_CONFIG_HOME");
			char buf[MAX_PATH];
			if (!confdir) {
				const char *home = getenv("HOME");
				if (!home) {
					struct passwd *pw = getpwuid(getuid());
					home = pw ? pw->pw_dir : "/tmp";
				}
				snprintf(buf, sizeof(buf), "%s/.config", home);
				confdir = buf;
			}
			snprintf(s_inipath, sizeof(s_inipath), "%s/%s/%s.toml",
			         confdir, NP2_WX_APPNAME, NP2_WX_APPNAME);
		}
	}
	milstr_ncpy(lpPath, s_inipath, cbPath);
}

void ini_read(const OEMCHAR *path, const OEMCHAR *title, const INITBL *tbl, UINT count)
{
	if (!path || !path[0]) return;

	try {
		auto doc = toml::parse_file(path);
		auto sec = doc.get_as<toml::table>(title);
		if (!sec) return;

		for (UINT i = 0; i < count; i++) {
			ini_tbl_read(*sec, &tbl[i]);
		}
	} catch (const toml::parse_error &) {
		/* ignore parse errors, use defaults */
	}
}

void ini_write(const OEMCHAR *path, const OEMCHAR *title, const INITBL *tbl, UINT count)
{
	if (!path || !path[0]) return;

	toml::table doc;
	toml::table sec;

	for (UINT i = 0; i < count; i++) {
		ini_tbl_write(sec, &tbl[i]);
	}
	doc.insert_or_assign(title, std::move(sec));

	std::ofstream ofs(path);
	if (ofs) ofs << doc;
}

void initload(void)
{
	extern void pccore_setdefault(void);
	extern void np2oscfg_setdefault(void);
	extern void np2wabcfg_setdefault(void);
	pccore_setdefault();
	np2oscfg_setdefault();
	np2wabcfg_setdefault();

	OEMCHAR path[MAX_PATH];
	initgetfile(path, MAX_PATH);
	ini_read(path, ini_section, np2_tbl, np2_tbl_count);
}

void initsave(void)
{
	OEMCHAR path[MAX_PATH];
	initgetfile(path, MAX_PATH);

	/* ensure config directory exists */
	char dir[MAX_PATH];
	milstr_ncpy(dir, path, MAX_PATH);
	file_cutname(dir);
	mkdir(dir, 0755);

	ini_write(path, ini_section, np2_tbl, np2_tbl_count);
}
