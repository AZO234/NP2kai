/* === TOML-based configuration for wx port ===
 * Uses libtomlplusplus (toml++) for reading/writing TOML files.
 *
 * The config file lives at ~/.config/wxnp2kai/wxnp2kai.toml
 * All settings are stored in a single [NP2kai] section.
 */

#include <compiler.h>
#include "np2.h"
#include "ini.h"
#include <pccore.h>
#include <wab/wab.h>
#include <common/strres.h>
#include <dosio.h>
#include <commng.h>
#include <joymng.h>
#include <soundmng.h>
#include <kbdmng.h>

#define TOML_EXCEPTIONS 0
#include <toml++/toml.hpp>

#include <string>
#include <fstream>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>

/* ---- helpers ---- */

static bool inigetbmp(const UINT8 *ptr, UINT pos)
{
	return ((ptr[pos >> 3] >> (pos & 7)) & 1) != 0;
}

static void inisetbmp(UINT8 *ptr, UINT pos, bool set)
{
	UINT8 bit = (UINT8)(1 << (pos & 7));
	ptr += (pos >> 3);
	if (set) *ptr |= bit; else *ptr &= ~bit;
}

/* ---- value → TOML node ---- */

static void ini_tbl_write(toml::table &sec, const INITBL *p)
{
	if (p->itemtype & INIFLAG_RO) return;

	char buf[512];
	buf[0] = '\0';

	switch (p->itemtype & INITYPE_MASK) {
	case INITYPE_STR:
		sec.insert_or_assign(p->item, std::string((const char *)p->value));
		return;

	case INITYPE_BOOL:
		sec.insert_or_assign(p->item, (bool)(*(const UINT8 *)p->value != 0));
		return;

	case INITYPE_BITMAP:
		sec.insert_or_assign(p->item,
		    (bool)inigetbmp((const UINT8 *)p->value, p->arg));
		return;

	case INITYPE_SINT8:
	case INITYPE_UINT8:
		sec.insert_or_assign(p->item, (int64_t)*(const UINT8 *)p->value);
		return;
	case INITYPE_SINT16:
	case INITYPE_UINT16:
		sec.insert_or_assign(p->item, (int64_t)*(const UINT16 *)p->value);
		return;
	case INITYPE_SINT32:
	case INITYPE_UINT32:
		sec.insert_or_assign(p->item, (int64_t)*(const UINT32 *)p->value);
		return;
	case INITYPE_SINT64:
	case INITYPE_UINT64:
		sec.insert_or_assign(p->item, (int64_t)*(const INT64 *)p->value);
		return;

	case INITYPE_HEX8:
		snprintf(buf, sizeof(buf), "0x%02x", (unsigned)*(const UINT8 *)p->value);
		break;
	case INITYPE_HEX16:
		snprintf(buf, sizeof(buf), "0x%04x", (unsigned)*(const UINT16 *)p->value);
		break;
	case INITYPE_HEX32:
		snprintf(buf, sizeof(buf), "0x%08x", (unsigned)*(const UINT32 *)p->value);
		break;
	case INITYPE_HEX64:
		snprintf(buf, sizeof(buf), "0x%016llx",
		    (unsigned long long)*(const UINT64 *)p->value);
		break;

	case INITYPE_ARGU32: {
		const uint32_t *arr = (const uint32_t *)p->value;
		int pos = 0;
		for (UINT i = 0; i < p->arg && pos < (int)sizeof(buf) - 16; i++) {
			pos += snprintf(buf + pos, sizeof(buf) - pos,
			                i ? ",%u" : "%u", arr[i]);
		}
		break;
	}
	case INITYPE_ARGS16: {
		const int16_t *arr = (const int16_t *)p->value;
		int pos = 0;
		for (UINT i = 0; i < p->arg && pos < (int)sizeof(buf) - 16; i++) {
			pos += snprintf(buf + pos, sizeof(buf) - pos,
			                i ? ",%d" : "%d", (int)arr[i]);
		}
		break;
	}
	case INITYPE_ARGH8: {
		const UINT8 *arr = (const UINT8 *)p->value;
		int pos = 0;
		for (UINT i = 0; i < p->arg && pos < (int)sizeof(buf) - 8; i++) {
			pos += snprintf(buf + pos, sizeof(buf) - pos, "%.2x ", arr[i]);
		}
		break;
	}
	case INITYPE_BYTE3: {
		const UINT8 *b = (const UINT8 *)p->value;
		snprintf(buf, sizeof(buf), "%c%c%c", b[0], b[1], b[2]);
		break;
	}
	case INITYPE_KB:
		sec.insert_or_assign(p->item,
		    std::string(*(const UINT8 *)p->value == KEY_KEY106 ? "KEY106" : "KEY101"));
		return;

	case INITYPE_SNDDRV:
		sec.insert_or_assign(p->item,
		    std::string(snddrv_num2drv(*(const UINT8 *)p->value)));
		return;

	case INITYPE_INTERP: {
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
	default:
		return;
	}
	sec.insert_or_assign(p->item, std::string(buf));
}

/* ---- TOML node → value ---- */

static void ini_tbl_read(const toml::table &sec, INITBL *p)
{
	auto node = sec.get(p->item);
	if (!node) return;

	auto type = node->type();
	std::string sv;
	int64_t iv = 0;
	bool    bv = false;
	bool    got_int = false, got_bool = false;

	if (type == toml::node_type::string) {
		sv = **node->as_string();
	} else if (type == toml::node_type::integer) {
		iv = **node->as_integer();
		snprintf(&sv[0], 0, "");  /* not used */
		got_int = true;
	} else if (type == toml::node_type::boolean) {
		bv = **node->as_boolean();
		got_bool = true;
	} else {
		return;
	}

	switch (p->itemtype & INITYPE_MASK) {
	case INITYPE_STR:
		if (!sv.empty()) milstr_ncpy((OEMCHAR *)p->value, sv.c_str(), p->arg);
		break;
	case INITYPE_BOOL:
		if (got_bool)  *(UINT8 *)p->value = bv ? 1 : 0;
		else if (got_int) *(UINT8 *)p->value = (iv != 0) ? 1 : 0;
		else           *(UINT8 *)p->value = (sv == "true" || sv == "1") ? 1 : 0;
		break;
	case INITYPE_BITMAP:
		if (got_bool)  inisetbmp((UINT8 *)p->value, p->arg, bv);
		else if (got_int) inisetbmp((UINT8 *)p->value, p->arg, iv != 0);
		else           inisetbmp((UINT8 *)p->value, p->arg, sv == "true" || sv == "1");
		break;
	case INITYPE_SINT8:
	case INITYPE_UINT8:
		*(UINT8 *)p->value = (UINT8)(got_int ? iv : strtol(sv.c_str(), nullptr, 0));
		if ((p->itemtype & INIFLAG_MAX) && *(UINT8 *)p->value > p->arg)
			*(UINT8 *)p->value = (UINT8)p->arg;
		if ((p->itemtype & INIFLAG_AND))
			*(UINT8 *)p->value &= (UINT8)p->arg;
		break;
	case INITYPE_SINT16:
	case INITYPE_UINT16:
		*(UINT16 *)p->value = (UINT16)(got_int ? iv : strtol(sv.c_str(), nullptr, 0));
		break;
	case INITYPE_SINT32:
	case INITYPE_UINT32:
		*(UINT32 *)p->value = (UINT32)(got_int ? iv : strtol(sv.c_str(), nullptr, 0));
		break;
	case INITYPE_SINT64:
	case INITYPE_UINT64:
		*(INT64 *)p->value = got_int ? iv : (INT64)strtoll(sv.c_str(), nullptr, 0);
		break;
	case INITYPE_HEX8:
		*(UINT8 *)p->value = (UINT8)strtoul(sv.c_str(), nullptr, 16);
		if ((p->itemtype & INIFLAG_AND)) *(UINT8 *)p->value &= (UINT8)p->arg;
		break;
	case INITYPE_HEX16:
		*(UINT16 *)p->value = (UINT16)strtoul(sv.c_str(), nullptr, 16);
		if ((p->itemtype & INIFLAG_AND)) *(UINT16 *)p->value &= (UINT16)p->arg;
		break;
	case INITYPE_HEX32:
		*(UINT32 *)p->value = (UINT32)strtoul(sv.c_str(), nullptr, 16);
		if ((p->itemtype & INIFLAG_AND)) *(UINT32 *)p->value &= (UINT32)p->arg;
		break;
	case INITYPE_HEX64:
		*(UINT64 *)p->value = strtoull(sv.c_str(), nullptr, 16);
		break;
	case INITYPE_ARGU32: {
		uint32_t *arr = (uint32_t *)p->value;
		const char *cur = sv.c_str();
		for (UINT i = 0; i < p->arg; i++) {
			while (*cur == ' ') cur++;
			if (!*cur) break;
			arr[i] = (uint32_t)strtoul(cur, nullptr, 0);
			while (*cur && *cur != ',') cur++;
			if (*cur == ',') cur++;
		}
		break;
	}
	case INITYPE_ARGS16: {
		int16_t *arr = (int16_t *)p->value;
		const char *cur = sv.c_str();
		for (UINT i = 0; i < p->arg; i++) {
			while (*cur == ' ') cur++;
			if (!*cur) break;
			arr[i] = (int16_t)strtol(cur, nullptr, 0);
			while (*cur && *cur != ',') cur++;
			if (*cur == ',') cur++;
		}
		break;
	}
	case INITYPE_ARGH8: {
		UINT8 *arr = (UINT8 *)p->value;
		const char *cur = sv.c_str();
		for (UINT i = 0; i < p->arg; i++) {
			while (*cur == ' ') cur++;
			if (!*cur) break;
			arr[i] = (UINT8)strtoul(cur, nullptr, 16);
			while (*cur && *cur != ' ') cur++;
		}
		break;
	}
	case INITYPE_BYTE3:
		if (sv.length() >= 3) {
			((UINT8 *)p->value)[0] = sv[0];
			((UINT8 *)p->value)[1] = sv[1];
			((UINT8 *)p->value)[2] = sv[2];
		}
		break;
	case INITYPE_KB:
		*(UINT8 *)p->value = (sv == "KEY101" || sv == "101") ? KEY_KEY101 : KEY_KEY106;
		break;
	case INITYPE_SNDDRV:
		*(UINT8 *)p->value = snddrv_drv2num(sv.c_str());
		break;
	case INITYPE_INTERP:
		if      (sv == "NEAREST") *(UINT8 *)p->value = INTERP_NEAREST;
		else if (sv == "TILES")   *(UINT8 *)p->value = INTERP_TILES;
		else if (sv == "HYPER")   *(UINT8 *)p->value = INTERP_HYPER;
		else                      *(UINT8 *)p->value = INTERP_BILINEAR;
		break;
	}
}

/* ---- public API ---- */

void ini_read(const OEMCHAR *path, const OEMCHAR *title, const INITBL *tbl, UINT count)
{
	auto result = toml::parse_file(path);
	if (!result) return;

	auto *sec = result.table()[title].as_table();
	if (!sec) return;

	for (UINT i = 0; i < count; i++) {
		INITBL tmp = tbl[i];
		ini_tbl_read(*sec, &tmp);
	}
}

void ini_write(const OEMCHAR *path, const OEMCHAR *title, const INITBL *tbl, UINT count)
{
	toml::table doc;
	{
		auto result = toml::parse_file(path);
		if (result) doc = std::move(result.table());
	}

	toml::table sec;
	/* preserve existing keys not in tbl */
	if (auto *old = doc[title].as_table()) sec = *old;

	for (UINT i = 0; i < count; i++) {
		ini_tbl_write(sec, &tbl[i]);
	}
	doc.insert_or_assign(title, std::move(sec));

	std::ofstream ofs(path);
	if (ofs) ofs << doc;
}

/* ---- config path ---- */

static OEMCHAR s_inipath[MAX_PATH] = "";

void initgetfile(OEMCHAR *lpPath, unsigned int cchPath)
{
	if (s_inipath[0] == '\0') {
		/* Command-line override via env */
		const char *override_path = getenv("NP2KAI_CONFIG");
		if (override_path && override_path[0]) {
			milstr_ncpy(s_inipath, override_path, sizeof(s_inipath));
		} else {
			const char *xdg = getenv("XDG_CONFIG_HOME");
			char confdir[MAX_PATH];
			if (xdg && xdg[0]) {
				snprintf(confdir, sizeof(confdir), "%s", xdg);
			} else {
				const char *home = getenv("HOME");
				if (!home) {
					struct passwd *pw = getpwuid(getuid());
					home = pw ? pw->pw_dir : "/tmp";
				}
				snprintf(confdir, sizeof(confdir), "%s/.config", home);
			}
			snprintf(s_inipath, sizeof(s_inipath), "%s/%s/%s.toml",
			         confdir, NP2_WX_APPNAME, NP2_WX_APPNAME);
		}
	}
	milstr_ncpy(lpPath, s_inipath, cchPath);
}

/* ---- NP2 config INITBL tables ---- */

/* Convenience macros matching SDL port */
enum {
	INIRO_STR    = INIFLAG_RO | INITYPE_STR,
	INIRO_BOOL   = INIFLAG_RO | INITYPE_BOOL,
	INIRO_BITMAP = INIFLAG_RO | INITYPE_BITMAP,
	INIRO_UINT8  = INIFLAG_RO | INITYPE_UINT8,
	INIRO_HEX8   = INIFLAG_RO | INITYPE_HEX8,
	INIRO_STR_HEX32 = INIFLAG_RO | INITYPE_HEX32,
	INIMAX_UINT8 = INIFLAG_MAX | INITYPE_UINT8,
	INIAND_UINT8 = INIFLAG_AND | INITYPE_UINT8,
	INIROMAX_SINT32 = INIFLAG_RO | INIFLAG_MAX | INITYPE_SINT32,
	INIROAND_HEX32  = INIFLAG_RO | INIFLAG_AND | INITYPE_HEX32,
	INIRO_BYTE3  = INIFLAG_RO | INITYPE_BYTE3,
	INIRO_KB     = INIFLAG_RO | INITYPE_KB
};

static INITBL np2_tbl[] = {
	{"FDfolder",  INITYPE_STR,    fddfolder,           MAX_PATH},
	{"HDfolder",  INITYPE_STR,    hddfolder,           MAX_PATH},
	{"bmap_Dir",  INITYPE_STR,    bmpfilefolder,       MAX_PATH},
	{"bmap_Num",  INITYPE_UINT32, &bmpfilenumber,      0},
	{"fontfile",  INITYPE_STR,    np2cfg.fontfile,     MAX_PATH},
	{"biospath",  INIRO_STR,      np2cfg.biospath,     MAX_PATH},
#if defined(SUPPORT_HOSTDRV)
	{"use_hdrv",  INITYPE_BOOL,   &np2cfg.hdrvenable,  0},
	{"hdrvroot",  INITYPE_STR,    &np2cfg.hdrvroot,    MAX_PATH},
	{"hdrv_acc",  INITYPE_UINT8,  &np2cfg.hdrvacc,     0},
#endif
	{"pc_model",  INITYPE_STR,    np2cfg.model,        sizeof(np2cfg.model)},
	{"clk_base",  INITYPE_UINT32, &np2cfg.baseclock,   0},
	{"clk_mult",  INITYPE_UINT32, &np2cfg.multiple,    0},
	{"EmuSpeed",  INITYPE_UINT32, &np2cfg.emuspeed,    0},
	{"DIPswtch",  INITYPE_ARGH8,  np2cfg.dipsw,        3},
	{"MEMswtch",  INITYPE_ARGH8,  np2cfg.memsw,        8},
#if defined(SUPPORT_LARGE_MEMORY)
	{"ExMemory",  INITYPE_UINT16, &np2cfg.EXTMEM,      13},
#else
	{"ExMemory",  INIMAX_UINT8,   &np2cfg.EXTMEM,      13},
#endif
	{"ITF_WORK",  INIRO_BOOL,     &np2cfg.ITF_WORK,    0},
#if defined(SUPPORT_FAST_MEMORYCHECK)
	{"MemCheck",  INITYPE_UINT8,  &np2cfg.memcheckspeed, 0},
#endif
	{"HDD1FILE",  INITYPE_STR,    np2cfg.sasihdd[0],   MAX_PATH},
	{"HDD2FILE",  INITYPE_STR,    np2cfg.sasihdd[1],   MAX_PATH},
#if defined(SUPPORT_SCSI)
	{"SCSIHDD0",  INITYPE_STR,    np2cfg.scsihdd[0],   MAX_PATH},
	{"SCSIHDD1",  INITYPE_STR,    np2cfg.scsihdd[1],   MAX_PATH},
	{"SCSIHDD2",  INITYPE_STR,    np2cfg.scsihdd[2],   MAX_PATH},
	{"SCSIHDD3",  INITYPE_STR,    np2cfg.scsihdd[3],   MAX_PATH},
#endif
#if defined(SUPPORT_IDEIO)
	{"HDD3FILE",  INITYPE_STR,    np2cfg.sasihdd[2],   MAX_PATH},
	{"HDD4FILE",  INITYPE_STR,    np2cfg.sasihdd[3],   MAX_PATH},
	{"IDE1TYPE",  INITYPE_UINT8,  &np2cfg.idetype[0],  0},
	{"IDE2TYPE",  INITYPE_UINT8,  &np2cfg.idetype[1],  0},
	{"IDE3TYPE",  INITYPE_UINT8,  &np2cfg.idetype[2],  0},
	{"IDE4TYPE",  INITYPE_UINT8,  &np2cfg.idetype[3],  0},
	{"IDE_CD1",   INITYPE_STR,    np2cfg.idecd[0],     MAX_PATH},
	{"IDE_CD2",   INITYPE_STR,    np2cfg.idecd[1],     MAX_PATH},
	{"IDE_CD3",   INITYPE_STR,    np2cfg.idecd[2],     MAX_PATH},
	{"IDE_CD4",   INITYPE_STR,    np2cfg.idecd[3],     MAX_PATH},
	{"IDERWAIT",  INITYPE_UINT32, &np2cfg.iderwait,    0},
	{"IDEWWAIT",  INITYPE_UINT32, &np2cfg.idewwait,    0},
	{"IDEMWAIT",  INITYPE_UINT32, &np2cfg.idemwait,    0},
#endif
	{"SampleHz",  INITYPE_UINT32, &np2cfg.samplingrate,0},
	{"Latencys",  INITYPE_UINT16, &np2cfg.delayms,     0},
	{"SNDboard",  INITYPE_HEX8,   &np2cfg.SOUND_SW,    0},
	{"BEEP_vol",  INIAND_UINT8,   &np2cfg.BEEP_VOL,    3},
	{"opt26BRD",  INITYPE_HEX8,   &np2cfg.snd26opt,    0},
	{"opt86BRD",  INITYPE_HEX8,   &np2cfg.snd86opt,    0},
	{"optSPBRD",  INITYPE_HEX8,   &np2cfg.spbopt,      0},
	{"optSPBVR",  INITYPE_HEX8,   &np2cfg.spb_vrc,     0},
	{"optSPBVL",  INIMAX_UINT8,   &np2cfg.spb_vrl,     24},
	{"optSPB_X",  INITYPE_BOOL,   &np2cfg.spb_x,       0},
	{"USEMPU98",  INITYPE_BOOL,   &np2cfg.mpuenable,   0},
	{"optMPU98",  INITYPE_HEX8,   &np2cfg.mpuopt,      0},
	{"optMPUAT",  INITYPE_BOOL,   &np2cfg.mpu_at,      0},
#if defined(SUPPORT_SMPU98)
	{"USE_SMPU",  INITYPE_BOOL,   &np2cfg.smpuenable,  0},
	{"opt_SMPU",  INITYPE_HEX8,   &np2cfg.smpuopt,     0},
	{"SMPUMUTB",  INITYPE_BOOL,   &np2cfg.smpumuteB,   0},
#endif
	{"volume_F",  INIMAX_UINT8,   &np2cfg.vol_fm,      128},
	{"volume_S",  INIMAX_UINT8,   &np2cfg.vol_ssg,     128},
	{"volume_A",  INIMAX_UINT8,   &np2cfg.vol_adpcm,   128},
	{"volume_P",  INIMAX_UINT8,   &np2cfg.vol_pcm,     128},
	{"volume_R",  INIMAX_UINT8,   &np2cfg.vol_rhythm,  128},
	{"DAVOLUME",  INIMAX_UINT8,   &np2cfg.davolume,    128},
#if defined(SUPPORT_FMGEN)
	{"USEFMGEN",  INITYPE_BOOL,   &np2cfg.usefmgen,    0},
#endif
	{"Seek_Snd",  INITYPE_BOOL,   &np2cfg.MOTOR,       0},
	{"Seek_Vol",  INIMAX_UINT8,   &np2cfg.MOTORVOL,    100},
	{"btnRAPID",  INITYPE_BOOL,   &np2cfg.BTN_RAPID,   0},
	{"btn_MODE",  INITYPE_BOOL,   &np2cfg.BTN_MODE,    0},
	{"MS_RAPID",  INITYPE_BOOL,   &np2cfg.MOUSERAPID,  0},
	{"VRAMwait",  INITYPE_ARGH8,  np2cfg.wait,         6},
	{"DispSync",  INITYPE_BOOL,   &np2cfg.DISPSYNC,    0},
	{"Real_Pal",  INITYPE_BOOL,   &np2cfg.RASTER,      0},
	{"RPal_tim",  INIMAX_UINT8,   &np2cfg.realpal,     64},
	{"uPD72020",  INITYPE_BOOL,   &np2cfg.uPD72020,    0},
	{"GRCG_EGC",  INIAND_UINT8,   &np2cfg.grcg,        3},
	{"color16b",  INITYPE_BOOL,   &np2cfg.color16,     0},
	{"skipline",  INITYPE_BOOL,   &np2cfg.skipline,    0},
	{"skplight",  INITYPE_SINT16, &np2cfg.skiplight,   0},
	{"LCD_MODE",  INIAND_UINT8,   &np2cfg.LCD_MODE,    0x03},
	{"calendar",  INITYPE_BOOL,   &np2cfg.calendar,    0},
	{"USE144FD",  INITYPE_BOOL,   &np2cfg.usefd144,    0},
	{"FDD1FILE",  INITYPE_STR,    np2cfg.fddfile[0],   MAX_PATH},
	{"FDD2FILE",  INITYPE_STR,    np2cfg.fddfile[1],   MAX_PATH},
	{"FDD3FILE",  INITYPE_STR,    np2cfg.fddfile[2],   MAX_PATH},
	{"FDD4FILE",  INITYPE_STR,    np2cfg.fddfile[3],   MAX_PATH},
	{"FDDRIVE1",  INITYPE_BITMAP, &np2cfg.fddequip,    0},
	{"FDDRIVE2",  INITYPE_BITMAP, &np2cfg.fddequip,    1},
	{"FDDRIVE3",  INITYPE_BITMAP, &np2cfg.fddequip,    2},
	{"FDDRIVE4",  INITYPE_BITMAP, &np2cfg.fddequip,    3},
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
	{"TIMERFIX",  INITYPE_BOOL,   &np2cfg.timerfix,    0},
	{"WINNTFIX",  INITYPE_BOOL,   &np2cfg.winntfix,    0},
	{"FPU_TYPE",  INITYPE_UINT8,  &np2cfg.fpu_type,    0},
#if defined(SUPPORT_ASYNC_CPU)
	{"ASYNCCPU",  INITYPE_BOOL,   &np2cfg.asynccpu,    0},
#endif
#if defined(SUPPORT_WAB)
	{"wabasw  ",  INITYPE_BOOL,   &np2cfg.wabasw,      0},
	{"MULTIWND",  INITYPE_BOOL,   &np2wabcfg.multiwindow, 0},
#endif
#if defined(SUPPORT_CL_GD5430)
	{"USE_CLGD",  INITYPE_BOOL,   &np2cfg.usegd5430,   0},
	{"CLGDTYPE",  INITYPE_UINT16, &np2cfg.gd5430type,  0},
	{"CLGDFCUR",  INITYPE_BOOL,   &np2cfg.gd5430fakecur, 0},
	{"GDMELOFS",  INITYPE_UINT8,  &np2cfg.gd5430melofs,0},
#endif
	{"pegcplane", INITYPE_BOOL,   &np2cfg.usepegcplane,0},
#if defined(SUPPORT_HOSTDRV)
	{"hdrvnt",    INITYPE_BOOL,   &np2cfg.hdrvntenable,0},
#endif
#if defined(SUPPORT_MULTITHREAD)
	{"MULTHREAD", INITYPE_BOOL,   &np2wabcfg.multithread, 1},
#endif
	/* OS-level settings */
	{"np2title",  INIRO_STR,      np2oscfg.titles,         sizeof(np2oscfg.titles)},
	{"keyboard",  INITYPE_KB,     &np2oscfg.KEYBOARD,      0},
	{"F12_COPY",  INITYPE_UINT8,  &np2oscfg.F12KEY,        0},
	{"Mouse_sw",  INITYPE_BOOL,   &np2oscfg.MOUSE_SW,      0},
	{"Joystick",  INITYPE_BOOL,   &np2oscfg.JOYPAD1,       0},
	{"Joy1_btn",  INITYPE_ARGH8,  np2oscfg.JOY1BTN,        JOY_NBUTTON},
	{"Joy1_dev",  INITYPE_STR,    &np2oscfg.JOYDEV[0],     MAX_PATH},
	{"mpu98port", INIMAX_UINT8,   &np2oscfg.mpu.port,      COMPORT_MIDI},
	{"mpu98dir",  INITYPE_BOOL,   &np2oscfg.mpu.direct,    0},
	{"mpu98map",  INITYPE_STR,    np2oscfg.mpu.mout,       MAX_PATH},
	{"mpu98min",  INITYPE_STR,    np2oscfg.mpu.min,        MAX_PATH},
	{"mpu98mdl",  INITYPE_STR,    np2oscfg.mpu.mdl,        64},
	{"mpu98def",  INITYPE_STR,    np2oscfg.mpu.def,        MAX_PATH},
	{"com1port",  INIMAX_UINT8,   &np2oscfg.com[0].port,   COMPORT_NONE},
	{"com1dir",   INITYPE_BOOL,   &np2oscfg.com[0].direct, 1},
	{"com1_bps",  INITYPE_UINT32, &np2oscfg.com[0].speed,  0},
	{"com1para",  INITYPE_UINT8,  &np2oscfg.com[0].param,  0},
	{"com1mmap",  INITYPE_STR,    np2oscfg.com[0].mout,    MAX_PATH},
	{"com2port",  INIMAX_UINT8,   &np2oscfg.com[1].port,   COMPORT_NONE},
	{"com2dir",   INITYPE_BOOL,   &np2oscfg.com[1].direct, 1},
	{"com2_bps",  INITYPE_UINT32, &np2oscfg.com[1].speed,  0},
	{"com2para",  INITYPE_UINT8,  &np2oscfg.com[1].param,  0},
	{"com2mmap",  INITYPE_STR,    np2oscfg.com[1].mout,    MAX_PATH},
	{"com3port",  INIMAX_UINT8,   &np2oscfg.com[2].port,   COMPORT_NONE},
	{"com3dir",   INITYPE_BOOL,   &np2oscfg.com[2].direct, 1},
	{"com3_bps",  INITYPE_UINT32, &np2oscfg.com[2].speed,  0},
	{"com3para",  INITYPE_UINT8,  &np2oscfg.com[2].param,  0},
	{"com3mmap",  INITYPE_STR,    np2oscfg.com[2].mout,    MAX_PATH},
	{"Mouse_sp",  INITYPE_UINT8,  &np2oscfg.mouse_move_ratio, 0},
#if defined(SUPPORT_RESUME)
	{"e_resume",  INITYPE_BOOL,   &np2oscfg.resume,        0},
#endif
#if defined(SUPPORT_STATSAVE)
	{"STATSAVE",  INITYPE_BOOL,   &np2cfg.statsave,        0},
#endif
	{"xrollkey",  INITYPE_BOOL,   &np2oscfg.xrollkey,      0},
	{"sounddrv",  INITYPE_SNDDRV, &np2oscfg.snddrv,        0},
	{"s_NOWAIT",  INITYPE_BOOL,   &np2oscfg.NOWAIT,        0},
	{"SkpFrame",  INITYPE_UINT8,  &np2oscfg.DRAW_SKIP,     0},
	{"DspClock",  INIAND_UINT8,   &np2oscfg.DISPCLK,       3},
	{"jast_snd",  INITYPE_BOOL,   &np2oscfg.jastsnd,       0},
	{"hostdrv_w", INITYPE_BOOL,   &np2oscfg.hostdrv_write, 0},
	{"mpu98mdw",  INITYPE_UINT32, &np2oscfg.MIDIWAIT,      0},
	{"mpu98out",  INITYPE_STR,    np2oscfg.MIDIDEV[0],     MAX_PATH},
	{"mpu98in",   INITYPE_STR,    np2oscfg.MIDIDEV[1],     MAX_PATH},
	/* Window geometry */
	{"win_x",     INITYPE_SINT32, &np2oscfg.winx,          0},
	{"win_y",     INITYPE_SINT32, &np2oscfg.winy,          0},
	{"win_w",     INITYPE_UINT32, &np2oscfg.winwidth,      0},
	{"win_h",     INITYPE_UINT32, &np2oscfg.winheight,     0},
};

static const UINT np2_tbl_count = (UINT)(sizeof(np2_tbl) / sizeof(np2_tbl[0]));

static const OEMCHAR ini_section[] = "NP2kai";

void initload(void)
{
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
