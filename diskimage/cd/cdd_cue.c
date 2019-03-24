#include	"compiler.h"
#include	"textfile.h"
#include	"dosio.h"
#include	"fdd/sxsi.h"

#ifdef SUPPORT_KAI_IMAGES

#include	"diskimage/cddfile.h"

//const OEMCHAR str_cue[] = OEMTEXT("cue");	//	CUEシート

//	CDD_CCD.Cで共有するための暫定処置
const OEMCHAR str_track[] = OEMTEXT("TRACK");
const OEMCHAR str_index[] = OEMTEXT("INDEX");
//

static const OEMCHAR str_file[] = OEMTEXT("FILE");
static const OEMCHAR str_binary[] = OEMTEXT("BINARY");
static const OEMCHAR str_wave[] = OEMTEXT("WAVE");
//static const OEMCHAR str_track[] = OEMTEXT("TRACK");
static const OEMCHAR str_pregap[] = OEMTEXT("PREGAP");
//static const OEMCHAR str_index[] = OEMTEXT("INDEX");
static const OEMCHAR str_mode1[] = OEMTEXT("MODE1");
static const OEMCHAR str_mode2[] = OEMTEXT("MODE2");	//	暫定対応
static const OEMCHAR str_audio[] = OEMTEXT("AUDIO");

//static BRESULT getint2(const OEMCHAR *str, UINT *val) {
//
//	if ((str[0] < '0') || (str[0] > '9') ||
//		(str[1] < '0') || (str[1] > '9')) {
//		return(FAILURE);
//	}
//	if (val) {
//		*val = ((str[0] - '0') * 10) + (str[1] - '0');
//	}
//	return(SUCCESS);
//}

static UINT32 getpos(const OEMCHAR *str) {

	UINT	m = 0;
	UINT	s = 0;
	UINT	f = 0;
	
	int idx = 0; // 文字位置
	UINT sdata[3] = {0}; // 数値バッファ
	UINT sdatapos = 0; // 数値バッファの格納位置
	int numdig = 0; // 通知の桁数

	while(str[idx]){ // NULL文字まで続ける
		if('0' <= str[idx] && str[idx] <= '9'){ // 0から9の数字ならバッファに入れる
			sdata[sdatapos] *= 10; // 桁上げ
			sdata[sdatapos] += (str[idx] - '0');
			numdig++; // 桁数カウント
		}else if(str[idx] == ':' && numdig >= 1){ // 「:」なら区切り文字（ただし、数字が一桁も無い場合は不正扱い）
			if(sdatapos == 2){
				break; // 3個目のデータならループを抜ける（配列を3つ分しか確保していない＆そもそも4つ以上があり得ないので）
			}
			sdatapos++; // 格納位置変更
			numdig = 0; // 桁数カウントリセット
		}else{
			return(0); // 不正な文字
		}
		idx++;
	}
	sdatapos++; // データ数にするために1を足す
	if(sdatapos!=3){
		return(0); // データ数が変（3個ではない）
	}
	m = sdata[0];
	s = sdata[1];
	f = sdata[2];
	return((((m * 60) + s) * 75) + f);
}

//	CUEシート読み込み
BRESULT opencue(SXSIDEV sxsi, const OEMCHAR *fname) {

	_CDTRK		trk[99];
	OEMCHAR		path[MAX_PATH];
	UINT		index;
	UINT8		curtrk;
	UINT		curtype;
	TEXTFILEH	tfh;
	OEMCHAR		buf_mode[10];
	OEMCHAR		buf[512];
	OEMCHAR		*argv[8];
	int			argc;
//	--------
	UINT16		curssize = 0;
	UINT32		curpos0;
	UINT32		curpregap;

	ZeroMemory(trk, sizeof(trk));
	path[0] = '\0';
	index = 0;
	curtrk = 1;
	curtype = 0x14;
//	--------
	curpos0 = 0;
	curpregap = 0;
//	--------

	tfh = textfile_open(fname, 0x800);
	if (tfh == NULL) {
		goto opencue_err2;
	}
	while (textfile_read(tfh, buf, NELEMENTS(buf)) == SUCCESS) {
		argc = milstr_getarg(buf, argv, NELEMENTS(argv));
		if ((argc >= 3) && (!milstr_cmp(argv[0], str_file))) {				//	FILE
			if (!milstr_cmp(argv[argc-1], str_binary) && path[0] == '\0') {	//		BINARY
				file_cpyname(path, fname, NELEMENTS(path));
				file_cutname(path);
				file_catname(path, argv[1], NELEMENTS(path));
			}
		}
		else if ((argc >= 3) && (!milstr_cmp(argv[0], str_track))) {		//	TRACK
			curtrk = (UINT8)milstr_solveINT(argv[1]);
			milstr_ncpy(buf_mode, argv[2], NELEMENTS(str_mode1));
			if (!milstr_cmp(buf_mode, str_mode1)) {							//		MODE1/????
				curtype = 0x14;
				curssize = (UINT16)milstr_solveINT(argv[2] + 6);
			}
			else if (!milstr_cmp(buf_mode, str_mode2)) {					//		MODE2/????
				curtype = 0x14;
				curssize = (UINT16)milstr_solveINT(argv[2] + 6);
			}
			else if (!milstr_cmp(argv[2], str_audio)) {						//		AUDIO
				curtype = 0x10;
				curssize = 2352;
			}
		}
		else if ((argc >= 2) && (!milstr_cmp(argv[0], str_pregap))) {		//	PREGAP
			curpregap = getpos(argv[1]);
		}
		else if ((argc >= 3) && (!milstr_cmp(argv[0], str_index))) {		//	INDEX ??
			if (index < NELEMENTS(trk)) {
				if ((UINT8)milstr_solveINT(argv[1]) == 0) {					//	INDEX 00
					curpos0 = getpos(argv[2]);
					continue;
				}
				if ((UINT8)milstr_solveINT(argv[1]) != 1) {					//	INDEX 01以外
					continue;
				}

				trk[index].adr_ctl			= curtype;
				trk[index].point			= curtrk;
				trk[index].pos				= getpos(argv[2]);
				trk[index].pos0				= (curpos0 == 0) ? trk[index].pos : curpos0;

				trk[index].sector_size		= curssize;

				trk[index].pregap_sectors	= curpregap + (trk[index].pos - trk[index].pos0);

				trk[index].img_pregap_sec	= (trk[index].pos0 == 0) ? trk[index].pos : trk[index].pos0;
				trk[index].img_start_sec	= trk[index].pos;

//				trk[index].pregap_sector	= trk[index].start_sector - trk[index].pregap_sectors;

				index++;
				curpregap = 0;
				curpos0 = 0;
			}
		}
	}

	if (index == 0) {
		goto opencue_err1;
	}

	set_secread(sxsi, trk, index);
	sxsi->totals = -1;

	textfile_close(tfh);

	return(setsxsidev(sxsi, path, trk, index));

opencue_err1:
	textfile_close(tfh);

opencue_err2:
	return(FAILURE);
}

#endif
