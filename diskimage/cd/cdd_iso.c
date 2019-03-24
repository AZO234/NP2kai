#include	"compiler.h"
#include	"dosio.h"
#include	"fdd/sxsi.h"

#ifdef SUPPORT_KAI_IMAGES

#include	"diskimage/cddfile.h"

//	指定ファイルをISOイメージとみなして開く
BRESULT openiso(SXSIDEV sxsi, const OEMCHAR *path) {

	_CDTRK	trk[99];
	UINT	trks;
	FILEH	fh;
	UINT16	sector_size;
	long	totals;

	ZeroMemory(trk, sizeof(trk));
	trks = 0;

	fh = file_open_rb(path);
	if (fh == FILEH_INVALID) {
		goto openiso_err1;
	}

	//	ファイルサイズが2048byte、2352byte、2448byteのどれかで割り切れるかチェック
	sxsi->read = sec2048_read;
	sector_size = 2048;
	totals = issec2048(fh);
	if (totals < 0) {
		sxsi->read = sec2352_read_with_ecc;
		sector_size = 2352;
		totals = issec2352(fh);
	}
	if (totals < 0) {
		sxsi->read = sec2448_read;
		sector_size = 2448;
		totals = issec2448(fh);
	}
	if (totals < 0) {
		goto openiso_err2;
	}

	trk[0].adr_ctl			= TRACKTYPE_DATA;
	trk[0].point			= 1;
	trk[0].pos				= 0;
	trk[0].pos0				= 0;

	trk[0].sector_size		= sector_size;

	trk[0].pregap_sector	= 0;
	trk[0].start_sector		= 0;
	trk[0].end_sector		= totals;

	trk[0].img_pregap_sec	= 0;
	trk[0].img_start_sec	= 0;
	trk[0].img_end_sec		= totals;

	trk[0].pregap_offset	= 0;
	trk[0].start_offset		= 0;
	trk[0].end_offset		= totals * sector_size;

	trk[0].pregap_sectors	= 0;
	trk[0].track_sectors	= totals;
	trks = 1;

	sxsi->totals = totals;

	file_close(fh);

	return(setsxsidev(sxsi, path, trk, trks));

openiso_err2:
	file_close(fh);

openiso_err1:
	return(FAILURE);
}

#endif
