#include	"compiler.h"
#include	"dosio.h"
#include	"textfile.h"
#include	"cpucore.h"
#include	"fdd/sxsi.h"
#include	"cddfile.h"

#ifdef SUPPORT_KAI_IMAGES

#include	"diskimage/img_strres.h"
#include	"diskimage/win9x/img_dosio.h"
#include	"diskimage/cd/cdd_iso.h"

//	ISO9660のボリューム記述子によるチェックを有効にする場合はコメントを外す
//	※有効にした場合、CD-ROM以外がマウントできなくなる
//#define	CHECK_ISO9660

#ifdef	CHECK_ISO9660
static const UINT8 cd001[7] = {0x01,'C','D','0','0','1',0x01};
#endif

#define CD_EDC_POLYNOMIAL	0xD8018001 // Reverse 0x8001801B

UINT32 crcTable[256];

void makeCRCTable( void)
{
	UINT32 i, j;
    for( i=0; i<256; i++){
        UINT32 crc = i;
        for( j=0; j<8; j++){
            crc = ( crc >> 1) ^ ( ( crc & 0x1) ? CD_EDC_POLYNOMIAL : 0);
        }
        crcTable[i] = crc;
    }
}

//	追加(kaiA)
BOOL isCDImage(const OEMCHAR *fname) {

const OEMCHAR	*ext;

	ext = file_getext(fname);
	if ((!file_cmpname(ext, str_cue)) ||
		(!file_cmpname(ext, str_ccd)) ||
		(!file_cmpname(ext, str_cdm)) ||
		(!file_cmpname(ext, str_mds)) ||
		(!file_cmpname(ext, str_nrg)) ||
		(!file_cmpname(ext, str_iso))) {
		return TRUE;
	}
	return FALSE;
}
//

long issec2048(FILEH fh) {

#ifdef	CHECK_ISO9660
	FILEPOS	fpos;
	UINT8	buf[2048];
	UINT	secsize;
#endif
	FILELEN	fsize;

#ifdef	CHECK_ISO9660
	fpos = 16 * 2048;
	if (file_seek(fh, fpos, FSEEK_SET) != fpos) {
		goto sec2048_err;
	}
	if (file_read(fh, buf, sizeof(buf)) != sizeof(buf)) {
		goto sec2048_err;
	}
	if (memcmp(buf, cd001, 7) != 0) {
		goto sec2048_err;
	}
	secsize = LOADINTELWORD(buf + 128);
	if (secsize != 2048) {
		goto sec2048_err;
	}
#endif
	fsize = file_getsize(fh);
	if ((fsize % 2048) != 0) {
		goto sec2048_err;
	}
	return((long)(fsize / 2048));

sec2048_err:
	return(-1);
}

long issec2352(FILEH fh) {

#ifdef	CHECK_ISO9660
	FILEPOS	fpos;
	UINT8	buf[2048];
	UINT	secsize;
#endif
	FILELEN	fsize;

#ifdef	CHECK_ISO9660
	fpos = (16 * 2352) + 16;
	if (file_seek(fh, fpos, FSEEK_SET) != fpos) {
		goto sec2352_err;
	}
	if (file_read(fh, buf, sizeof(buf)) != sizeof(buf)) {
		goto sec2352_err;
	}
	if (memcmp(buf, cd001, 7) != 0) {
		goto sec2352_err;
	}
	secsize = LOADINTELWORD(buf + 128);
	if (secsize != 2048) {
		goto sec2352_err;
	}
#endif
	fsize = file_getsize(fh);
	if ((fsize % 2352) != 0) {
		goto sec2352_err;
	}
	return((long)(fsize / 2352));

sec2352_err:
	return(-1);
}

long issec2448(FILEH fh) {

#ifdef	CHECK_ISO9660
	FILEPOS	fpos;
	UINT8	buf[2048];
	UINT	secsize;
#endif
	FILELEN	fsize;

#ifdef	CHECK_ISO9660
	fpos = (16 * 2448) + 16;
	if (file_seek(fh, fpos, FSEEK_SET) != fpos) {
		goto sec2448_err;
	}
	if (file_read(fh, buf, sizeof(buf)) != sizeof(buf)) {
		goto sec2448_err;
	}
	if (memcmp(buf, cd001, 7) != 0) {
		goto sec2448_err;
	}
	secsize = LOADINTELWORD(buf + 128);
	if (secsize != 2048) {
		goto sec2448_err;
	}
#endif
	fsize = file_getsize(fh);
	if ((fsize % 2448) != 0) {
		goto sec2448_err;
	}
	return((long)(fsize / 2448));

sec2448_err:
	return(-1);
}

long issec(FILEH fh, _CDTRK *trk, UINT trks) {

#ifdef	CHECK_ISO9660
	FILEPOS	fpos;
	UINT8	buf[2048];
	UINT	secsize;
#endif
	UINT	i;
	FILELEN	fsize;
	long	total;

	total = 0;

#ifdef	CHECK_ISO9660
	fpos = 16 * trk[0].sector_size;
	if (trk[0].sector_size != 2048) {
		fpos += 16;
	}
	if (file_seek(fh, fpos, FSEEK_SET) != fpos) {
		goto sec_err;
	}
	if (file_read(fh, buf, sizeof(buf)) != sizeof(buf)) {
		goto sec_err;
	}
	if (memcmp(buf, cd001, 7) != 0) {
		goto sec_err;
	}
	secsize = LOADINTELWORD(buf + 128);
	if (secsize != 2048) {
		goto sec_err;
	}
#endif

	if (trks == 1) {
		trk[0].sector_size = 2048;
		trk[0].str_sec = 0;
		total = issec2048(fh);
		if (total < 0) {
			trk[0].sector_size = 2352;
			total = issec2352(fh);
		}
		if (total < 0) {
			trk[0].sector_size = 2448;
			total = issec2448(fh);
		}
		if (total < 0) {
			return(-1);
		}
		else {
			trk[0].end_sec = total - 1;
			trk[0].sectors = total;
			return(total);
		}
	}

	fsize = file_getsize(fh);
	if (trk[0].pos0 == 0) {
		trk[0].str_sec = trk[0].pos;
	}
	else {
		trk[0].str_sec = trk[0].pos0;
	}
	for (i = 1; i < trks; i++) {
		if (trk[i].pos0 == 0) {
			trk[i].str_sec = trk[i].pos;
		}
		else {
			trk[i].str_sec = trk[i].pos0;
		}
		trk[i-1].end_sec = trk[i].str_sec - 1;
		trk[i-1].sectors = trk[i-1].end_sec - trk[i-1].str_sec + 1;
		total += trk[i-1].sectors;
		fsize -= trk[i-1].sectors * trk[i-1].sector_size;
	}
	if (fsize % trk[trks-1].sector_size != 0) {
		return(-1);
	}
	if (trk[trks-1].pos0 == 0) {
		trk[trks-1].str_sec = trk[trks-1].pos;
	}
	else {
		trk[trks-1].str_sec = trk[trks-1].pos0;
	}
	trk[trks-1].end_sec = (UINT32)(trk[trks-1].str_sec + (fsize / trk[trks-1].sector_size));
	trk[trks-1].sectors = trk[trks-1].end_sec - trk[trks-1].str_sec + 1;
	total += trk[trks-1].sectors;

	return(total);

#ifdef	CHECK_ISO9660
sec_err:
	return(-1);
#endif
}

//	※CDTRK構造体内の
//		UINT32	str_sec;
//		UINT32	end_sec;
//		UINT32	sectors;
//		等のメンバの設定
long set_trkinfo(FILEH fh, _CDTRK *trk, UINT trks, FILELEN imagesize) {

	UINT	i;
	FILELEN	fsize;
	long	total;

	total = 0;

	if (trks == 1) {
		trk[0].sector_size = 2048;
		trk[0].str_sec = 0;
		total = issec2048(fh);
		if (total < 0) {
			trk[0].sector_size = 2352;
			total = issec2352(fh);
		}
		if (total < 0) {
			trk[0].sector_size = 2448;
			total = issec2448(fh);
		}
		if (total < 0) {
			return(-1);
		}
		else {
			trk[0].end_sec = total - 1;
			trk[0].sectors = total;
			return(total);
		}
	}

	if (imagesize == 0) {
		fsize = file_getsize(fh);
	}
	else {
		fsize = imagesize;
	}
	if (trk[0].pos0 == 0) {
		trk[0].str_sec = trk[0].pos;
	}
	else {
		trk[0].str_sec = trk[0].pos0;
	}
	for (i = 1; i < trks; i++) {
		if (trk[i].pos0 == 0) {
			trk[i].str_sec = trk[i].pos;
		}
		else {
			trk[i].str_sec = trk[i].pos0;
		}
		trk[i-1].end_sec = trk[i].str_sec - 1;
		trk[i-1].sectors = trk[i-1].end_sec - trk[i-1].str_sec + 1;
		total += trk[i-1].sectors;
		fsize -= trk[i-1].sectors * trk[i-1].sector_size;
	}
	if (fsize % trk[trks-1].sector_size != 0) {
		return(-1);
	}
	if (trk[trks-1].pos0 == 0) {
		trk[trks-1].str_sec = trk[trks-1].pos;
	}
	else {
		trk[trks-1].str_sec = trk[trks-1].pos0;
	}
	trk[trks-1].end_sec = (UINT32)(trk[trks-1].str_sec + (fsize / trk[trks-1].sector_size));
	trk[trks-1].sectors = trk[trks-1].end_sec - trk[trks-1].str_sec + 1;
	total += trk[trks-1].sectors;

	return(total);
}


//	----
//	イメージファイル内全トラックセクタ長2048byte用
REG8 sec2048_read(SXSIDEV sxsi, FILEPOS pos, UINT8 *buf, UINT size) {

	CDINFO	cdinfo;
	FILEH	fh;
	UINT	rsize;

	if (sxsi_prepare(sxsi) != SUCCESS) {
		return(0x60);
	}
	if ((pos < 0) || (pos >= sxsi->totals)) {
		return(0x40);
	}

	cdinfo = (CDINFO)sxsi->hdl;
	fh = cdinfo->fh;

	pos = (FILEPOS)(pos * 2048 + cdinfo->trk[0].start_offset);
	if (file_seek(fh, pos, FSEEK_SET) != pos) {
		return(0xd0);
	}

	while(size) {
		rsize = MIN(size, 2048);
		CPU_REMCLOCK -= rsize;
		if (file_read(fh, buf, rsize) != rsize) {
			return(0xd0);
		}
		buf += rsize;
		size -= rsize;
	}
	return(0x00);
}


//	イメージファイル内全トラックセクタ長2352byte用
REG8 sec2352_read(SXSIDEV sxsi, FILEPOS pos, UINT8 *buf, UINT size) {

	CDINFO	cdinfo;
	FILEH	fh;
	FILEPOS	fpos;
	UINT	rsize;

	if (sxsi_prepare(sxsi) != SUCCESS) {
		return(0x60);
	}
	if ((pos < 0) || (pos >= sxsi->totals)) {
		return(0x40);
	}

	cdinfo = (CDINFO)sxsi->hdl;
	fh = cdinfo->fh;

	while(size) {
		fpos = (FILEPOS)((pos * 2352) + 16 + cdinfo->trk[0].start_offset);
		if (file_seek(fh, fpos, FSEEK_SET) != fpos) {
			return(0xd0);
		}
		rsize = MIN(size, 2048);
		CPU_REMCLOCK -= rsize;
		if (file_read(fh, buf, rsize) != rsize) {
			return(0xd0);
		}
		buf += rsize;
		size -= rsize;
		pos++;
	}
	return(0x00);
}

UINT32 calcCRC(UINT8 *buf, int len)
{
	int i;
    UINT32 crc = 0x00000000;
    for( i=0; i<len; i++){
        crc = (crc >> 8) ^ crcTable[(crc^buf[i]) & 0xff];
    }
    return crc;
}

//	イメージファイル内全トラックセクタ長2352byte用(ECCチェック有効)
REG8 sec2352_read_with_ecc(SXSIDEV sxsi, FILEPOS pos, UINT8 *buf, UINT size) {
	
	CDINFO	cdinfo;
	FILEH	fh;
	FILEPOS	fpos;
	UINT	rsize;
	UINT8	bufedc[4];
	UINT8	bufecc[276];
	UINT8	bufdata[2352];

	if (sxsi_prepare(sxsi) != SUCCESS) {
		return(0x60);
	}
	if ((pos < 0) || (pos >= sxsi->totals)) {
		return(0x40);
	}

	cdinfo = (CDINFO)sxsi->hdl;
	fh = cdinfo->fh;

	while(size) {
		fpos = (FILEPOS)((pos * 2352) + cdinfo->trk[0].start_offset);
		if (file_seek(fh, fpos, FSEEK_SET) != fpos) {
			return(0xd0);
		}
		rsize = 2352;
		CPU_REMCLOCK -= rsize;
		if (file_read(fh, bufdata, rsize) != rsize) {
			return(0xd0);
		}
		memcpy(buf, bufdata+16, 2048);
		memcpy(bufedc, bufdata+16+2048, 4);
		memcpy(bufecc, bufdata+16+2048+4+8, 276);

		// Check EDC
		if(calcCRC(bufdata, 2064) != LOADINTELDWORD(bufedc)){
			// EDC Error
			// TODO: Check ECC
			//sxsi->cdflag_ecc = (sxsi->cdflag_ecc & ~CD_ECC_BITMASK) | CD_ECC_RECOVERED; // ECC recovered
			sxsi->cdflag_ecc = (sxsi->cdflag_ecc & ~CD_ECC_BITMASK) | CD_ECC_ERROR; // ECC error
			//return(0xd0);
		}

		rsize = MIN(size, 2048);
		buf += rsize;
		size -= rsize;
		pos++;
	}
	return(0x00);
}


//	イメージファイル内全トラックセクタ長2448(2352+96)用
REG8 sec2448_read(SXSIDEV sxsi, FILEPOS pos, UINT8 *buf, UINT size) {

	CDINFO	cdinfo;
	FILEH	fh;
	FILEPOS	fpos;
	UINT	rsize;

	if (sxsi_prepare(sxsi) != SUCCESS) {
		return(0x60);
	}
	if ((pos < 0) || (pos >= sxsi->totals)) {
		return(0x40);
	}

	cdinfo = (CDINFO)sxsi->hdl;
	fh = cdinfo->fh;
	while(size) {
		fpos = (FILEPOS)((pos * 2448) + 16 + cdinfo->trk[0].start_offset);
		if (file_seek(fh, fpos, FSEEK_SET) != fpos) {
			return(0xd0);
		}
		rsize = MIN(size, 2048);
		CPU_REMCLOCK -= rsize;
		if (file_read(fh, buf, rsize) != rsize) {
			return(0xd0);
		}
		buf += rsize;
		size -= rsize;
		pos++;
	}
	return(0x00);
}


//	イメージファイル内セクタ長混在用
//		非RAW(2048byte)＋Audio(2352byte)等
REG8 sec_read(SXSIDEV sxsi, FILEPOS pos, UINT8 *buf, UINT size) {

	CDINFO	cdinfo;
	FILEH	fh;
	FILEPOS	fpos;
	UINT	rsize;
	UINT	i;
	UINT32	secs;

	if (sxsi_prepare(sxsi) != SUCCESS) {
		return(0x60);
	}
	if ((pos < 0) || (pos >= sxsi->totals)) {
		return(0x40);
	}

	cdinfo = (CDINFO)sxsi->hdl;
	fh = cdinfo->fh;

	while (size) {
		fpos = 0;
		secs = 0;
		for (i = 0; i < cdinfo->trks; i++) {
			if (cdinfo->trk[i].str_sec <= (UINT32)pos && (UINT32)pos <= cdinfo->trk[i].end_sec) {
				fpos += (pos - secs) * cdinfo->trk[i].sector_size;
				if (cdinfo->trk[i].sector_size != 2048) {
					fpos += 16;
				}
				break;
			}
			fpos += cdinfo->trk[i].sectors * cdinfo->trk[i].sector_size;
			secs += cdinfo->trk[i].sectors;
		}
		fpos += (FILEPOS)cdinfo->trk[0].start_offset;
		if (file_seek(fh, fpos, FSEEK_SET) != fpos) {
			return(0xd0);
		}
		rsize = MIN(size, 2048);
		CPU_REMCLOCK -= rsize;
		if (file_read(fh, buf, rsize) != rsize) {
			return(0xd0);
		}
		buf += rsize;
		size -= rsize;
		pos++;
	}
	return(0x00);
}

//	----
BRESULT cd_reopen(SXSIDEV sxsi) {

	CDINFO	cdinfo;
	FILEH	fh;

	cdinfo = (CDINFO)sxsi->hdl;
	fh = file_open_rb(cdinfo->path);
	if (fh != FILEH_INVALID) {
		cdinfo->fh = fh;
		return(SUCCESS);
	}
	else {
		return(FAILURE);
	}
}

void cd_close(SXSIDEV sxsi) {

	CDINFO	cdinfo;

	cdinfo = (CDINFO)sxsi->hdl;
	file_close(cdinfo->fh);
}

void cd_destroy(SXSIDEV sxsi) {

	if(sxsi->hdl){
		_MFREE((CDINFO)sxsi->hdl);
		sxsi->hdl = (INTPTR)NULL;
	}
}
//	----

void set_secread(SXSIDEV sxsi, const _CDTRK *trk, UINT trks) {

	UINT		i;
	UINT16		secsize;

	secsize = trk[0].sector_size;
	for (i = 1; i < trks; i++) {
		if (secsize != trk[i].sector_size) {
			secsize = 0;
			break;
		}
	}
	if (secsize != 0) {
		switch (secsize) {
			case	2048:
				sxsi->read = sec2048_read;
				break;
			case	2352:
				sxsi->read = sec2352_read_with_ecc; // sec2352_read;
				break;
			case	2448:
				sxsi->read = sec2448_read;
				break;
		}
	}
	else {
		sxsi->read = sec_read;
	}
}

//
//#define	TOCLOGOUT
#ifdef	TOCLOGOUT
#define	TOCLOG(fmt, val)	\
			_stprintf(logbuf, fmt, val);	\
			textfile_write(tfh, logbuf);
static const OEMCHAR str_logB[] = OEMTEXT("._CDTRK.Before.log");
static const OEMCHAR str_logA[] = OEMTEXT("._CDTRK.After.log");
#endif
//

//	イメージファイルの実体を開き、各種情報構築
BRESULT setsxsidev(SXSIDEV sxsi, const OEMCHAR *path, const _CDTRK *trk, UINT trks) {

	FILEH	fh;
	long	totals;
	CDINFO	cdinfo;
	UINT	mediatype;
	UINT	i;
#ifdef	TOCLOGOUT
	OEMCHAR		logpath[MAX_PATH];
	OEMCHAR		logbuf[2048];
	TEXTFILEH	tfh;
#endif

	makeCRCTable();

	//	trk、trksは有効な値が設定済みなのが前提
	if ((trk == NULL) || (trks == 0)) {
		goto sxsiope_err1;
	}

	fh = file_open_rb(path);
	if (fh == FILEH_INVALID) {
		goto sxsiope_err1;
	}

	cdinfo = (CDINFO)_MALLOC(sizeof(_CDINFO), path);
	if (cdinfo == NULL) {
		goto sxsiope_err2;
	}
	ZeroMemory(cdinfo, sizeof(_CDINFO));
	cdinfo->fh = fh;
	trks = MIN(trks, NELEMENTS(cdinfo->trk) - 1);
	CopyMemory(cdinfo->trk, trk, trks * sizeof(_CDTRK));

#ifdef	TOCLOGOUT
	file_cpyname(logpath, path, NELEMENTS(logpath));
	file_cutext(logpath);
	file_catname(logpath, str_logB, NELEMENTS(logpath));

	tfh = textfile_create(logpath, 0x800);
	if (tfh == NULL) {
		return(FAILURE);
	}

	TOCLOG(OEMTEXT("STR _CDTRK LOG\r\n"), 0);
	for (i = 0; i < trks; i++) {
		TOCLOG(OEMTEXT("trk[%02d]\r\n"), i);
		TOCLOG(OEMTEXT("  adr_ctl        = 0x%02X\r\n"),     cdinfo->trk[i].adr_ctl);
		TOCLOG(OEMTEXT("  point          = %02d\r\n"),       cdinfo->trk[i].point);
		TOCLOG(OEMTEXT("  [pos0][pos][ ]              = [%18I32d]"), cdinfo->trk[i].pos0);
		TOCLOG(OEMTEXT("[%18I32d][                  ]\r\n"),         cdinfo->trk[i].pos);
		TOCLOG(OEMTEXT("  sec[ ][str][end]            = [                  ][%18I32d]"), cdinfo->trk[i].str_sec);
		TOCLOG(OEMTEXT("[%18I32d]\r\n"), cdinfo->trk[i].end_sec);
		TOCLOG(OEMTEXT("  sectors        = %I32d\r\n"),      cdinfo->trk[i].sectors);
		TOCLOG(OEMTEXT("  sector_size    = %d\r\n"),         cdinfo->trk[i].sector_size);
		TOCLOG(OEMTEXT("  sector [pregap][start][end] = [%18I32d]"), cdinfo->trk[i].pregap_sector);
		TOCLOG(OEMTEXT("[%18I32d]"),     cdinfo->trk[i].start_sector);
		TOCLOG(OEMTEXT("[%18I32d]\r\n"), cdinfo->trk[i].end_sector);
		TOCLOG(OEMTEXT("  img_sec[pregap][start][end] = [%18I32d]"), cdinfo->trk[i].img_pregap_sec);
		TOCLOG(OEMTEXT("[%18I32d]"),     cdinfo->trk[i].img_start_sec);
		TOCLOG(OEMTEXT("[%18I32d]\r\n"), cdinfo->trk[i].img_end_sec);
		TOCLOG(OEMTEXT("  offset [pregap][start][end] = [0x%016I64X]"), cdinfo->trk[i].pregap_offset);
		TOCLOG(OEMTEXT("[0x%016I64X]"),     cdinfo->trk[i].start_offset);
		TOCLOG(OEMTEXT("[0x%016I64X]\r\n"), cdinfo->trk[i].end_offset);
		TOCLOG(OEMTEXT("  pregap_sectors = %I32d\r\n"),      cdinfo->trk[i].pregap_sectors);
		TOCLOG(OEMTEXT("  track_sectors  = %I32d\r\n"),      cdinfo->trk[i].track_sectors);
	}
	TOCLOG(OEMTEXT("END _CDTRK LOG\r\n"), 0);

	textfile_close(tfh);
#endif

#if 1
	if (sxsi->totals == -1) {
		totals = set_trkinfo(fh, cdinfo->trk, trks, 0);
		if (totals < 0) {
			goto sxsiope_err3;
		}
		sxsi->totals = totals;
	}
#else
	totals = issec(fh, cdinfo->trk, trks);	//	とりあえず
	sxsi->read = sec2048_read;
	totals = issec2048(cdinfo->fh);
	if (totals < 0) {
		sxsi->read = sec2352_read;
		totals = issec2352(cdinfo->fh);
	}
	if (totals < 0) {
		sxsi->read = sec2448_read;
		totals = issec2448(cdinfo->fh);
	}
	if (totals < 0) {
		sxsi->read = sec_read;
		totals = issec(cdinfo->fh, cdinfo->trk, trks);
	}
	if (totals < 0) {
		goto sxsiope_err3;
	}
#endif

	mediatype = 0;
	for (i = 0; i < trks; i++) {
		if (cdinfo->trk[i].adr_ctl == TRACKTYPE_DATA) {
			mediatype |= SXSIMEDIA_DATA;
		}
		else if (cdinfo->trk[i].adr_ctl == TRACKTYPE_AUDIO) {
			mediatype |= SXSIMEDIA_AUDIO;
		}
	}

	//	リードアウトトラックを生成
	cdinfo->trk[trks].adr_ctl	= 0x10;
	cdinfo->trk[trks].point		= 0xaa;
//	cdinfo->trk[trks].pos		= totals;
	cdinfo->trk[trks].pos		= (UINT32)sxsi->totals;

	cdinfo->trks = trks;
	file_cpyname(cdinfo->path, path, NELEMENTS(cdinfo->path));

	sxsi->reopen		= cd_reopen;
	sxsi->close			= cd_close;
	sxsi->destroy		= cd_destroy;
	sxsi->hdl			= (INTPTR)cdinfo;
//	sxsi->totals		= totals;
	sxsi->cylinders		= 0;
	sxsi->size			= 2048;
	sxsi->sectors		= 1;
	sxsi->surfaces		= 1;
	sxsi->headersize	= 0;
	sxsi->mediatype		= mediatype;

#ifdef	TOCLOGOUT
	file_cpyname(logpath, path, NELEMENTS(logpath));
	file_cutext(logpath);
	file_catname(logpath, str_logA, NELEMENTS(logpath));

	tfh = textfile_create(logpath, 0x800);
	if (tfh == NULL) {
		return(FAILURE);
	}

	TOCLOG(OEMTEXT("STR _CDTRK LOG\r\n"), 0);
	for (i = 0; i < trks; i++) {
		TOCLOG(OEMTEXT("trk[%02d]\r\n"), i);
		TOCLOG(OEMTEXT("  adr_ctl        = 0x%02X\r\n"),     cdinfo->trk[i].adr_ctl);
		TOCLOG(OEMTEXT("  point          = %02d\r\n"),       cdinfo->trk[i].point);
		TOCLOG(OEMTEXT("  [pos0][pos][ ]              = [%18I32d]"), cdinfo->trk[i].pos0);
		TOCLOG(OEMTEXT("[%18I32d][                  ]\r\n"),         cdinfo->trk[i].pos);
		TOCLOG(OEMTEXT("  sec[ ][str][end]            = [                  ][%18I32d]"), cdinfo->trk[i].str_sec);
		TOCLOG(OEMTEXT("[%18I32d]\r\n"), cdinfo->trk[i].end_sec);
		TOCLOG(OEMTEXT("  sectors        = %I32d\r\n"),      cdinfo->trk[i].sectors);
		TOCLOG(OEMTEXT("  sector_size    = %d\r\n"),         cdinfo->trk[i].sector_size);
		TOCLOG(OEMTEXT("  sector [pregap][start][end] = [%18I32d]"), cdinfo->trk[i].pregap_sector);
		TOCLOG(OEMTEXT("[%18I32d]"),     cdinfo->trk[i].start_sector);
		TOCLOG(OEMTEXT("[%18I32d]\r\n"), cdinfo->trk[i].end_sector);
		TOCLOG(OEMTEXT("  img_sec[pregap][start][end] = [%18I32d]"), cdinfo->trk[i].img_pregap_sec);
		TOCLOG(OEMTEXT("[%18I32d]"),     cdinfo->trk[i].img_start_sec);
		TOCLOG(OEMTEXT("[%18I32d]\r\n"), cdinfo->trk[i].img_end_sec);
		TOCLOG(OEMTEXT("  offset [pregap][start][end] = [0x%016I64X]"), cdinfo->trk[i].pregap_offset);
		TOCLOG(OEMTEXT("[0x%016I64X]"),     cdinfo->trk[i].start_offset);
		TOCLOG(OEMTEXT("[0x%016I64X]\r\n"), cdinfo->trk[i].end_offset);
		TOCLOG(OEMTEXT("  pregap_sectors = %I32d\r\n"),      cdinfo->trk[i].pregap_sectors);
		TOCLOG(OEMTEXT("  track_sectors  = %I32d\r\n"),      cdinfo->trk[i].track_sectors);
	}
	TOCLOG(OEMTEXT("END _CDTRK LOG\r\n"), 0);

	textfile_close(tfh);
#endif

	return(SUCCESS);

sxsiope_err3:
	_MFREE(cdinfo);

sxsiope_err2:
	file_close(fh);

sxsiope_err1:
	return(FAILURE);
}

#endif
