/* === TOML-based configuration for wx port ===
 * Uses libtomlplusplus (toml++) for reading/writing TOML files.
 */

#include <compiler.h>
#include "ini.h"
#include "pccore.h"
#include "np2.h"
#include <dosio.h>
#include <sysmng.h>

#include <wx/filefn.h>
#include <wx/stdpaths.h>
#include <wx/filename.h>

#include <toml++/toml.hpp>
#include <fstream>
#include <string>
#include <vector>

#include <sys/types.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <pwd.h>
#include <unistd.h>
#endif

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
	case INITYPE_HEX8:
	case INITYPE_UINT8:
	case INITYPE_SINT8:
		sec.insert_or_assign(p->item, (int64_t)*(const UINT8 *)p->value);
		break;
	case INITYPE_HEX16:
	case INITYPE_UINT16:
	case INITYPE_SINT16:
		sec.insert_or_assign(p->item, (int64_t)*(const UINT16 *)p->value);
		break;
	case INITYPE_HEX32:
	case INITYPE_UINT32:
	case INITYPE_SINT32:
		sec.insert_or_assign(p->item, (int64_t)*(const UINT32 *)p->value);
		break;
	case INITYPE_HEX64:
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
	case INITYPE_HEX8:
		if (got_int) *(UINT8 *)p->value = (UINT8)iv;
		break;
	case INITYPE_UINT8:
	case INITYPE_SINT8:
		if (got_int) *(UINT8 *)p->value = (UINT8)iv;
		if ((p->itemtype & INIFLAG_MAX) && *(UINT8 *)p->value > (UINT8)p->arg)
			*(UINT8 *)p->value = (UINT8)p->arg;
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
	{"HDD1FILE",  INITYPE_STR,    np2cfg.sasihdd[0],   MAX_PATH},
	{"HDD2FILE",  INITYPE_STR,    np2cfg.sasihdd[1],   MAX_PATH},
#if defined(SUPPORT_SCSI)
	{"SCSIHDD0",  INITYPE_STR,    np2cfg.scsihdd[0],   MAX_PATH},
	{"SCSIHDD1",  INITYPE_STR,    np2cfg.scsihdd[1],   MAX_PATH},
	{"SCSIHDD2",  INITYPE_STR,    np2cfg.scsihdd[2],   MAX_PATH},
	{"SCSIHDD3",  INITYPE_STR,    np2cfg.scsihdd[3],   MAX_PATH},
#endif
	{"SampleHz",  INITYPE_UINT32, &np2cfg.samplingrate, 0},
	{"Latencys",  INITYPE_UINT16, &np2cfg.delayms,      0},
	{"SNDboard",  INITYPE_HEX8,   &np2cfg.SOUND_SW,    0},
	{"BEEP_vol",  INITYPE_UINT8,  &np2cfg.BEEP_VOL,    3},
	{"volume_F",  INITYPE_UINT8 | INIFLAG_MAX, &np2cfg.vol_fm,      128},
	{"volume_S",  INITYPE_UINT8 | INIFLAG_MAX, &np2cfg.vol_ssg,     128},
	{"volume_A",  INITYPE_UINT8 | INIFLAG_MAX, &np2cfg.vol_adpcm,   128},
	{"volume_P",  INITYPE_UINT8 | INIFLAG_MAX, &np2cfg.vol_pcm,     128},
	{"volume_R",  INITYPE_UINT8 | INIFLAG_MAX, &np2cfg.vol_rhythm,  128},
	{"DAVOLUME",  INITYPE_UINT8 | INIFLAG_MAX, &np2cfg.davolume,    128},
#if defined(SUPPORT_FMGEN)
	{"USEFMGEN",  INITYPE_BOOL,   &np2cfg.usefmgen,    0},
#endif
	{"Seek_Snd",  INITYPE_BOOL,   &np2cfg.MOTOR,       0},
	{"Seek_Vol",  INITYPE_UINT8,  &np2cfg.MOTORVOL,    0},
	{"btn_mode",  INITYPE_BOOL,   &np2cfg.BTN_MODE,    0},
	{"btn_rapd",  INITYPE_BOOL,   &np2cfg.BTN_RAPID,   0},

	{"keyboard",  INITYPE_UINT8,  &np2oscfg.KEYBOARD,  0},
	{"F12_Copy",  INITYPE_UINT8,  &np2oscfg.F12KEY,    0},
	{"Mouse_sw",  INITYPE_BOOL,   &np2oscfg.MOUSE_SW,  0},
	{"Joypad1",   INITYPE_BOOL,   &np2oscfg.JOYPAD1,   0},
	{"Joy1_Btn",  INITYPE_ARGH8,  np2oscfg.JOY1BTN,    JOY_NBUTTON},

	{"WindPosX",  INITYPE_SINT32, &np2oscfg.winx,       0},
	{"WindPosY",  INITYPE_SINT32, &np2oscfg.winy,       0},

	{"com1port",  INITYPE_UINT8,  &np2oscfg.com[0].port,  0},
	{"com1para",  INITYPE_HEX8,   &np2oscfg.com[0].param, 0},
	{"com1_bps",  INITYPE_UINT32, &np2oscfg.com[0].speed, 0},
	{"com1_m_i",  INITYPE_STR,    np2oscfg.com[0].min,    MAX_PATH},
	{"com1_m_o",  INITYPE_STR,    np2oscfg.com[0].mout,   MAX_PATH},

	{"mpu98en",   INITYPE_BOOL,   &np2cfg.mpuenable,   0},
	{"mpu98opt",  INITYPE_HEX8,   &np2cfg.mpuopt,      0},
	{"mpu98dir",  INITYPE_BOOL,   &np2oscfg.mpu.direct, 0},
	{"mpu98dev",  INITYPE_STR,    np2oscfg.mpu.mdl,    MAX_PATH},

	{"CycleInt",  INITYPE_UINT32, &cycle_shot_interval,     0},
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
#ifdef _WIN32
					// HOME may be unset outside a POSIX shell; use Windows' user directory.
					home = getenv("USERPROFILE");
#else
					struct passwd *pw = getpwuid(getuid());
					home = pw ? pw->pw_dir : "/tmp";
#endif
				}
				if (!home) home = ".";
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
#ifdef _WIN32
	_mkdir(dir);
#else
	mkdir(dir, 0755);
#endif

	ini_write(path, ini_section, np2_tbl, np2_tbl_count);
}
