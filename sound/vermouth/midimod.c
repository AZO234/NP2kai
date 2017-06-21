#include	"compiler.h"
#include	"strres.h"
#include	"dosio.h"
#include	"textfile.h"
#include	"midiout.h"
#if defined(SUPPORT_ARC)
#include	"arc.h"
#endif


#define	CFG_MAXAMP		400
#define	MAX_NAME		64

enum {
	CFG_DIR		 = 0,
	CFG_SOURCE,
	CFG_DEFAULT,
	CFG_BANK,
	CFG_DRUM
};

static const OEMCHAR str_dir[] = OEMTEXT("dir");
static const OEMCHAR str_source[] = OEMTEXT("source");
static const OEMCHAR str_default[] = OEMTEXT("default");
static const OEMCHAR str_bank[] = OEMTEXT("bank");
static const OEMCHAR str_drumset[] = OEMTEXT("drumset");
static const OEMCHAR *cfgstr[] = {str_dir, str_source, str_default,
								str_bank, str_drumset};

static const OEMCHAR str_amp[] = OEMTEXT("amp");
static const OEMCHAR str_keep[] = OEMTEXT("keep");
static const OEMCHAR str_note[] = OEMTEXT("note");
static const OEMCHAR str_pan[] = OEMTEXT("pan");
static const OEMCHAR str_strip[] = OEMTEXT("strip");
static const OEMCHAR str_left[] = OEMTEXT("left");
static const OEMCHAR str_center[] = OEMTEXT("center");
static const OEMCHAR str_right[] = OEMTEXT("right");
static const OEMCHAR str_env[] = OEMTEXT("env");
static const OEMCHAR str_loop[] = OEMTEXT("loop");
static const OEMCHAR str_tail[] = OEMTEXT("tail");
static const OEMCHAR file_timiditycfg[] = OEMTEXT("timidity.cfg");
static const OEMCHAR str_basedir[] = OEMTEXT("${basedir}");


static void VERMOUTHCL pathadd(MIDIMOD mod, const OEMCHAR *path) {

	_PATHLIST	pl;
	PATHLIST	p;

	ZeroMemory(&pl, sizeof(pl));
	if (path) {
		pl.path[0] = '\0';
		// separator change!
		file_catname(pl.path, path, NELEMENTS(pl.path));
		if (path[0]) {
#if defined(SUPPORT_ARC)
			if (milstr_chr(pl.path, '#') == NULL)
#endif
				file_setseparator(pl.path, NELEMENTS(pl.path));
		}
	}

	pl.next = mod->pathlist;
	p = pl.next;
	while(p) {
		if (!file_cmpname(p->path, pl.path)) {
			return;
		}
		p = p->next;
	}
	p = (PATHLIST)listarray_append(mod->pathtbl, &pl);
	if (p) {
		mod->pathlist = p;
	}
}

static void VERMOUTHCL pathaddex(MIDIMOD mod, const OEMCHAR *path) {

	OEMCHAR	_path[MAX_PATH];

	if (milstr_memcmp(path, str_basedir)) {
		pathadd(mod, path);
	}
	else {
		file_cpyname(_path, file_getcd(str_null), NELEMENTS(_path));
		file_cutseparator(_path);
		file_catname(_path, path + 10, NELEMENTS(_path));
		pathadd(mod, _path);
	}
}

static int VERMOUTHCL cfggetarg(OEMCHAR *str, OEMCHAR *arg[], int maxarg) {

	int		ret;
	BOOL	quot;
	OEMCHAR	*p;
	OEMCHAR	c;

	ret = 0;
	while(maxarg--) {
		quot = FALSE;
		while(1) {
			c = *str;
			if ((c == 0) || (c == 0x23)) {
				goto cga_done;
			}
			if ((c < 0) || (c > 0x20)) {
				break;
			}
			str++;
		}
		arg[ret++] = str;
		p = str;
		while(1) {
			c = *str;
			if (c == 0) {
				break;
			}
			str++;
			if (c == 0x22) {
				quot = !quot;
			}
			else if (quot) {
				*p++ = c;
			}
			else if (c == 0x23) {
				*p = '\0';
				goto cga_done;
			}
			else if ((c < 0) || (c > 0x20)) {
				*p++ = c;
			}
			else {
				break;
			}
		}
		*p = '\0';
	}

cga_done:
	return(ret);
}

static OEMCHAR *VERMOUTHCL seachr(const OEMCHAR *str, OEMCHAR sepa) {

	OEMCHAR	c;

	while(1) {
		c = *str;
		if (c == '\0') {
			break;
		}
		if (c == sepa) {
			return((OEMCHAR *)str);
		}
		str++;
	}
	return(NULL);
}

enum {
	VAL_EXIST	= 1,
	VAL_SIGN	= 2
};

static BRESULT VERMOUTHCL cfggetval(const OEMCHAR *str, int *val) {

	int		ret;
	int		flag;
	int		c;

	ret = 0;
	flag = 0;
	c = *str;
	if (c == '+') {
		str++;
	}
	else if (c == '-') {
		str++;
		flag |= VAL_SIGN;
	}
	while(1) {
		c = *str++;
		c -= '0';
		if ((unsigned)c < 10) {
			ret *= 10;
			ret += c;
			flag |= VAL_EXIST;
		}
		else {
			break;
		}
	}
	if (flag & VAL_EXIST) {
		if (flag & VAL_SIGN) {
			ret *= -1;
		}
		if (val) {
			*val = ret;
		}
		return(SUCCESS);
	}
	else {
		return(FAILURE);
	}
}


// ----

static void VERMOUTHCL settone(MIDIMOD mod, int bank, int argc,
															OEMCHAR *argv[]) {

	int		val;
	TONECFG	tone;
	OEMCHAR	*name;
	int		i;
	OEMCHAR	*key;
	OEMCHAR	*data;
	UINT8	flag;

	if ((bank < 0) || (bank >= (MIDI_BANKS * 2)) || (argc < 2) ||
		(cfggetval(argv[0], &val) != SUCCESS) || (val < 0) || (val >= 128)) {
		return;
	}
	tone = mod->tonecfg[bank];
	if (tone == NULL) {
		tone = (TONECFG)_MALLOC(sizeof(_TONECFG) * 128, "tone cfg");
		if (tone == NULL) {
			return;
		}
		mod->tonecfg[bank] = tone;
		ZeroMemory(tone, sizeof(_TONECFG) * 128);
	}
	tone += val;
	name = tone->name;
	if (name == NULL) {
		name = (OEMCHAR *)listarray_append(mod->namelist, NULL);
		tone->name = name;
	}
	if (name) {
		name[0] = '\0';
		file_catname(name, argv[1], MAX_NAME);		// separator change!
	}
	flag = TONECFG_EXIST;
	tone->amp = TONECFG_AUTOAMP;
	tone->pan = TONECFG_VARIABLE;

	if (!(bank & 1)) {					// for tone
		tone->note = TONECFG_VARIABLE;
	}
	else {								// for drums
		flag |= TONECFG_NOLOOP | TONECFG_NOENV;
		tone->note = (UINT8)val;
	}

	for (i=2; i<argc; i++) {
		key = argv[i];
		data = seachr(key, '=');
		if (data == NULL) {
			continue;
		}
		*data++ = '\0';
		if (!milstr_cmp(key, str_amp)) {
			if (cfggetval(data, &val) == SUCCESS) {
				if (val < 0) {
					val = 0;
				}
				else if (val > CFG_MAXAMP) {
					val = CFG_MAXAMP;
				}
				tone->amp = val;
			}
		}
		else if (!milstr_cmp(key, str_keep)) {
			if (!milstr_cmp(data, str_env)) {
				flag &= ~TONECFG_NOENV;
				flag |= TONECFG_KEEPENV;
			}
			else if (!milstr_cmp(data, str_loop)) {
				flag &= ~TONECFG_NOLOOP;
			}
		}
		else if (!milstr_cmp(key, str_note)) {
			if ((cfggetval(data, &val) == SUCCESS) &&
				(val >= 0) && (val < 128)) {
				tone->note = (UINT8)val;
			}
		}
		else if (!milstr_cmp(key, str_pan)) {
			if (!milstr_cmp(data, str_left)) {
				val = 0;
			}
			else if (!milstr_cmp(data, str_center)) {
				val = 64;
			}
			else if (!milstr_cmp(data, str_right)) {
				val = 127;
			}
			else if (cfggetval(data, &val) == SUCCESS) {
				if (val < -100) {
					val = -100;
				}
				else if (val > 100) {
					val = 100;
				}
				val = val + 100;
				val *= 127;
				val += 100;
				val /= 200;
			}
			else {
				continue;
			}
			tone->pan = (UINT8)val;
		}
		else if (!milstr_cmp(key, str_strip)) {
			if (!milstr_cmp(data, str_env)) {
				flag &= ~TONECFG_KEEPENV;
				flag |= TONECFG_NOENV;
			}
			else if (!milstr_cmp(data, str_loop)) {
				flag |= TONECFG_NOLOOP;
			}
			else if (!milstr_cmp(data, str_tail)) {
				flag |= TONECFG_NOTAIL;
			}
		}
	}
	tone->flag = flag;
}


// ----

BRESULT VERMOUTHCL midimod_getfile(MIDIMOD mod, const OEMCHAR *filename,
													OEMCHAR *path, int size) {

	PATHLIST	p;
	short		attr;

	if ((filename == NULL) || (filename[0] == '\0') ||
		(path == NULL) || (size == 0)) {
		goto fpgf_exit;
	}
	p = mod->pathlist;
	while(p) {
		file_cpyname(path, p->path, size);
		file_catname(path, filename, size);
#if defined(SUPPORT_ARC)
		attr = arcex_attr(path);
#else
		attr = file_attr(path);
#endif
		if (attr != -1) {
			return(SUCCESS);
		}
		p = p->next;
	}

fpgf_exit:
	return(FAILURE);
}

static BRESULT VERMOUTHCL cfgfile_load(MIDIMOD mod, const OEMCHAR *filename,
																int depth) {

	TEXTFILEH	tfh;
	OEMCHAR		buf[1024];
	int			bank;
	int			i;
	int			argc;
	OEMCHAR		*argv[16];
	int			val;
	UINT		cfg;

	bank = -1;

	if ((depth >= 16) ||
		(midimod_getfile(mod, filename, buf, NELEMENTS(buf)) != SUCCESS)) {
		goto cfl_err;
	}
// TRACEOUT(("open: %s", buf));
	tfh = textfile_open(buf, 0x1000);
	if (tfh == NULL) {
		goto cfl_err;
	}
	while(textfile_read(tfh, buf, NELEMENTS(buf)) == SUCCESS) {
		argc = cfggetarg(buf, argv, NELEMENTS(argv));
		if (argc < 2) {
			continue;
		}
		cfg = 0;
		while(cfg < NELEMENTS(cfgstr)) {
			if (!milstr_cmp(argv[0], cfgstr[cfg])) {
				break;
			}
			cfg++;
		}
		switch(cfg) {
			case CFG_DIR:
				for (i=1; i<argc; i++) {
					pathaddex(mod, argv[i]);
				}
				break;

			case CFG_SOURCE:
				for (i=1; i<argc; i++) {
					depth++;
					cfgfile_load(mod, argv[i], depth);
					depth--;
				}
				break;

			case CFG_DEFAULT:
				break;

			case CFG_BANK:
			case CFG_DRUM:
				if ((cfggetval(argv[1], &val) == SUCCESS) &&
					(val >= 0) && (val < 128)) {
					val <<= 1;
					if (cfg == CFG_DRUM) {
						val++;
					}
					bank = val;
				}
				break;

			default:
				settone(mod, bank, argc, argv);
				break;
		}
	}
	textfile_close(tfh);
	return(SUCCESS);

cfl_err:
	return(FAILURE);
}


// ----

VEXTERN MIDIMOD VEXPORT midimod_create(UINT samprate) {

	UINT	size;
	MIDIMOD	ret;
	BRESULT	r;

	size = sizeof(_MIDIMOD);
	size += sizeof(INSTRUMENT) * 128 * 2;
	size += sizeof(_TONECFG) * 128 * 2;
	ret = (MIDIMOD)_MALLOC(size, "MIDIMOD");
	if (ret == NULL) {
		goto mmcre_err1;
	}
	ZeroMemory(ret, size);
	ret->samprate = samprate;
	ret->tone[0] = (INSTRUMENT *)(ret + 1);
	ret->tone[1] = ret->tone[0] + 128;
	ret->tonecfg[0] = (TONECFG)(ret->tone[1] + 128);
	ret->tonecfg[1] = ret->tonecfg[0] + 128;
	ret->pathtbl = listarray_new(sizeof(_PATHLIST), 16);
	pathadd(ret, NULL);
	pathadd(ret, file_getcd(str_null));
	ret->namelist = listarray_new(MAX_NAME, 128);
	r = cfgfile_load(ret, file_timiditycfg, 0);
#if defined(TIMIDITY_CFGFILE)
	if (r != SUCCESS) {
		r = cfgfile_load(ret, TIMIDITY_CFGFILE, 0);
	}
#endif
	if (r != SUCCESS) {
		goto mmcre_err2;
	}
	midimod_lock(ret);
	return(ret);

mmcre_err2:
	listarray_destroy(ret->namelist);
	listarray_destroy(ret->pathtbl);
	_MFREE(ret);

mmcre_err1:
	return(NULL);
}

void VERMOUTHCL midimod_lock(MIDIMOD mod) {

	mod->lockcount++;
}

void VERMOUTHCL midimod_unlock(MIDIMOD mod) {

	UINT	r;
	TONECFG	bank;

	if (!mod->lockcount) {
		return;
	}
	mod->lockcount--;
	if (mod->lockcount) {
		return;
	}

	r = 128;
	do {
		r--;
		inst_bankfree(mod, r);
	} while(r > 0);
	for (r=2; r<(MIDI_BANKS*2); r++) {
		bank = mod->tonecfg[r];
		if (bank) {
			_MFREE(bank);
		}
	}
	listarray_destroy(mod->namelist);
	listarray_destroy(mod->pathtbl);
	_MFREE(mod);
}

VEXTERN void VEXPORT midimod_destroy(MIDIMOD mod) {

	if (mod) {
		midimod_unlock(mod);
	}
}

VEXTERN BRESULT VEXPORT midimod_cfgload(MIDIMOD mod,
												const OEMCHAR *filename) {

	return(cfgfile_load(mod, filename, 0));
}

VEXTERN void VEXPORT midimod_loadprogram(MIDIMOD mod, UINT num) {

	UINT	bank;

	if (mod != NULL) {
		bank = (num >> 8) & 0x7f;
		num &= 0x7f;
		if (inst_singleload(mod, bank << 1, num) != MIDIOUT_SUCCESS) {
			inst_singleload(mod, 0, num);
		}
	}
}

VEXTERN void VEXPORT midimod_loadrhythm(MIDIMOD mod, UINT num) {

	UINT	bank;

	if (mod != NULL) {
		bank = (num >> 8) & 0x7f;
		num &= 0x7f;
		if (inst_singleload(mod, (bank << 1) + 1, num) != MIDIOUT_SUCCESS) {
			inst_singleload(mod, 1, num);
		}
	}
}

VEXTERN void VEXPORT midimod_loadgm(MIDIMOD mod) {

	if (mod) {
		inst_bankload(mod, 0);
		inst_bankload(mod, 1);
	}
}

VEXTERN void VEXPORT midimod_loadall(MIDIMOD mod) {

	UINT	b;

	if (mod) {
		for (b=0; b<(MIDI_BANKS*2); b++) {
			inst_bankload(mod, b);
		}
	}
}


VEXTERN int VEXPORT midimod_loadallex(MIDIMOD mod, FNMIDIOUTLAEXCB cb,
															void *userdata) {

	int					result;
	MIDIOUTLAEXPARAM	param;
	UINT				b;

	result = MIDIOUT_SUCCESS;
	if (mod) {
		ZeroMemory(&param, sizeof(param));
		param.userdata = userdata;
		for (b=0; b<(MIDI_BANKS*2); b++) {
			param.totaltones += inst_gettones(mod, b);
		}
		for (b=0; b<(MIDI_BANKS*2); b++) {
			param.bank = b;
			result = inst_bankloadex(mod, b, cb, &param);
			if (result != MIDIOUT_SUCCESS)
			{
				break;
			}
		}
	}
	return result;
}

