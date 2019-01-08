#include	"compiler.h"
#include	"textfile.h"
#include	"dosio.h"
#include	"cpucore.h"
#include	"fdd/sxsi.h"

#ifdef SUPPORT_KAI_IMAGES

#include	"diskimage/cddfile.h"
#include	"diskimage/cd/cdd_cue.h"

static const OEMCHAR str_img[] = OEMTEXT(".img");

static const OEMCHAR str__mode1[] = OEMTEXT("MODE=1");
static const OEMCHAR str__mode0[] = OEMTEXT("MODE=0");

#if 0
"Entry"
"Point"
"ADR"
"Control"
"AMin"
"ASec"
"AFrame"
"ALBA"
"Zero"
"PMin"
"PSec"
"PFrame"
"PLBA"
#endif

//	CCD(&CDM)読み込み
BRESULT openccd(SXSIDEV sxsi, const OEMCHAR *fname) {

	_CDTRK		trk[99];
	OEMCHAR		path[MAX_PATH];
	UINT		index;
	UINT16		curssize;
	UINT8		curtrk;
	UINT		curtype;
	UINT32		curpos0;
	TEXTFILEH	tfh;
	OEMCHAR		buf[512];
	OEMCHAR		*argv[8];
	int			argc;

	ZeroMemory(trk, sizeof(trk));
	path[0] = '\0';
	index = 0;
	curssize = 2352;	//	セクタサイズは2352byte固定…でいいのかな？
	curtrk = 1;
	curtype = 0x14;
	curpos0 = 0;

	//	イメージファイルの実体は"*.img"固定…でいいのかな？
	file_cpyname(path, fname, NELEMENTS(path));
	file_cutext(path);
	file_catname(path, str_img, NELEMENTS(path));

	tfh = textfile_open(fname, 0x800);
	if (tfh == NULL) {
		goto openccd_err2;
	}
	while (textfile_read(tfh, buf, NELEMENTS(buf)) == SUCCESS) {
		if (!milstr_cmp(buf, str__mode1)) {
			curtype = 0x14;
		}
		else if (!milstr_cmp(buf, str__mode0)) {
			curtype = 0x10;
		}
		argc = milstr_getarg(buf, argv, NELEMENTS(argv));
		if ((argc >= 2) && (!milstr_cmp(argv[0] + 1, str_track))) {
			curtrk = (UINT8)milstr_solveINT(argv[1]);
		}
		else if ((argc >= 2) && (!milstr_cmp(argv[0], str_index))) {
			if (index < NELEMENTS(trk)) {
				//	"INDEX 0"、セクタを記録
				if ((UINT8)milstr_solveINT(argv[1]) == 0) {
					curpos0 = (UINT32)milstr_solveINT(argv[1] + 2);
					continue;
				}
				trk[index].adr_ctl		= curtype;
				trk[index].point		= curtrk;
				trk[index].pos			= (UINT32)milstr_solveINT(argv[1] + 2);
				trk[index].pos0			= curpos0;

				trk[index].sector_size	= curssize;

				index++;

				curpos0 = 0;
			}
		}
	}

	if (index == 0) {
		goto openccd_err1;
	}
	
	sxsi->read = sec2352_read_with_ecc; //sec2352_read;
	sxsi->totals = -1;

	textfile_close(tfh);

	return(setsxsidev(sxsi, path, trk, index));

openccd_err1:
	textfile_close(tfh);

openccd_err2:
	return(FAILURE);
}

//	参照部分の記述内容がCloneCD(*.ccd)と同じため、とりあえず封印
#if 0
static BRESULT opencdm(SXSIDEV sxsi, const OEMCHAR *fname) {

	_CDTRK		trk[99];
	OEMCHAR		path[MAX_PATH];
	UINT		index;
	UINT8		curtrk;
	UINT		curtype;
	TEXTFILEH	tfh;
	OEMCHAR		buf[512];
	OEMCHAR		*argv[8];
	int			argc;

	ZeroMemory(trk, sizeof(trk));
	path[0] = '\0';
	index = -1;
//	index = 0;
	curtrk = 1;
	curtype = 0x14;

	file_cpyname(path, fname, NELEMENTS(path));
	file_cutext(path);
	file_catname(path, str_img, NELEMENTS(path));

	tfh = textfile_open(fname, 0x800);
	if (tfh == NULL) {
		return(FAILURE);
	}
	while(textfile_read(tfh, buf, NELEMENTS(buf)) == SUCCESS) {
		if (!milstr_cmp(buf, str__mode1)) {
			curtype = 0x14;
		}
		else if (!milstr_cmp(buf, str__mode0)) {
			curtype = 0x10;
		}
		argc = milstr_getarg(buf, argv, NELEMENTS(argv));
		if ((argc >= 2) && (!milstr_cmp(argv[0] + 1, str_track))) {
			index++;
			//curtrk++;
			curtrk = (UINT8)milstr_solveINT(argv[1]);
		}
		else if ((argc >= 2) && (!milstr_cmp(argv[0], str_index))) {
			if (index < NELEMENTS(trk)) {
				trk[index].type = curtype;
				trk[index].track = curtrk;
				trk[index].pos = (UINT32)milstr_solveINT(argv[1] + 2);
//				index++;
			}
		}
	}
	textfile_close(tfh);

	return(setsxsidev(sxsi, path, trk, index));
}
#endif

#endif
