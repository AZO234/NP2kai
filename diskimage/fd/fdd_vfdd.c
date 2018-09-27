#include	"compiler.h"
#include	"dosio.h"
#include	"pccore.h"
#include	"iocore.h"

#ifdef SUPPORT_KAI_IMAGES

#include	"diskimage/fddfile.h"
#include	"diskimage/fd/fdd_vfdd.h"

static const UINT8 vfdd_verID_100[8] =
						{'V','F','D','1','.','0','0', 0x00};
static const UINT8 vfdd_verID_101[8] =
						{'V','F','D','1','.','0','1', 0x00};	//	1.01もとりあえず対象に(kaiF)

BRESULT fdd_set_vfdd(FDDFILE fdd, FDDFUNC fdd_fn, const OEMCHAR *fname, int ro) {

const _VFDD_ID	*sec_vfdd;
	short		attr;
	FILEH		fh;
	UINT		rsize;
	UINT		i;

	attr = file_attr(fname);
	if (attr & 0x18) {
		return(FAILURE);
	}
	fh = file_open(fname);
	if (fh == FILEH_INVALID) {
		return(FAILURE);
	}
	rsize = file_read(fh, &fdd->inf.vfdd.head, VFDD_HEADERSIZE);	//	VFDDヘッダ読込
	file_close(fh);
	if (rsize != VFDD_HEADERSIZE) {
		return(FAILURE);
	}

	//	バージョンＩＤチェック
	if (memcmp(fdd->inf.vfdd.head.verID, vfdd_verID_100, 8)) {
		if (memcmp(fdd->inf.vfdd.head.verID, vfdd_verID_101, 8)) {	//	1.01もとりあえず対象に(kaiF)
			return(FAILURE);
		}
	}

	fdd->type = DISKTYPE_VFDD;
	fdd->protect = ((attr & 0x01) || (ro)) ? TRUE : FALSE;
	if (LOADINTELWORD(&fdd->inf.vfdd.head.write_protect)) {
		fdd->protect = TRUE;
	}

	//	最大値入れて平気？
	fdd->inf.xdf.tracks		= VFDD_TRKMAX;
	fdd->inf.xdf.sectors	= VFDD_SECMAX;

	sec_vfdd = &fdd->inf.vfdd.id[0][0];
	//	ディスクアクセス時用に各セクタのオフセットを算出
	for (i = 0; i < VFDD_TRKMAX * VFDD_SECMAX; i++) {
		if (sec_vfdd->C != 0xff) {
			fdd->inf.vfdd.ptr[(sec_vfdd->C << 1) + sec_vfdd->H][sec_vfdd->R - 1] = LOADINTELDWORD(&sec_vfdd->dataPoint);
		}
		sec_vfdd++;
	}

	//	先頭格納セクタを見て決め打ち
	//	…2DD/2HD混在フォーマットでまずい気がする
	sec_vfdd = &fdd->inf.vfdd.id[0][0];
	if (sec_vfdd->flHD) {
		/* 1.2M */
		fdd->inf.xdf.disktype = DISKTYPE_2HD;
		fdd->inf.xdf.rpm = 0;
		sec_vfdd = &fdd->inf.vfdd.id[0][17];
		if (sec_vfdd->flMF == 0x01 && sec_vfdd->flHD == 0x01) {
			/* 1.44M(暫定) */
			fdd->inf.xdf.rpm = 1;
		}
	}
	else {
		//	640K
		fdd->inf.xdf.disktype = DISKTYPE_2DD;
		fdd->inf.xdf.rpm = 0;
	}

	//	処理関数群を登録
	fdd_fn->eject		= fdd_eject_xxx;
	fdd_fn->diskaccess	= fdd_diskaccess_common;
	fdd_fn->seek		= fdd_seek_common;
	fdd_fn->seeksector	= fdd_seeksector_common;
	fdd_fn->read		= fdd_read_vfdd;
	fdd_fn->write		= fdd_write_vfdd;
	fdd_fn->readid		= fdd_readid_vfdd;
	fdd_fn->writeid		= fdd_dummy_xxx;
	fdd_fn->formatinit	= fdd_dummy_xxx;
	fdd_fn->formating	= fdd_formating_xxx;
	fdd_fn->isformating	= fdd_isformating_xxx;

	return(SUCCESS);
}

BRESULT fdd_read_vfdd(FDDFILE fdd) {

	FILEH	hdl;
	UINT	trk;
	UINT	sec;
	UINT	secR;
	UINT	secsize;
	long	seekp;
	UINT	i;

	fddlasterror = 0x00;
	if (fdd_seeksector_common(fdd)) {
		return(FAILURE);
	}
	trk = (fdc.treg[fdc.us] << 1) + fdc.hd;
	sec = fdc.R - 1;
	secR = 0xff;
	for (i = 0; i < VFDD_SECMAX; i++) {
		if (fdd->inf.vfdd.id[trk][i].R == fdc.R) {
			secR = i;
			break;
		}
	}
	if (secR == 0xff) {
		return(FAILURE);
	}
	if (fdc.N != fdd->inf.vfdd.id[trk][secR].N) {
		fddlasterror = 0xc0;
		return(FAILURE);
	}

	if (fdd->type == DISKTYPE_VFDD) {
		secsize = 128 << fdd->inf.vfdd.id[trk][secR].N;
//		seekp = (long)fdd->inf.vfdd.ptr[trk][sec];
		if (fdd->inf.vfdd.ptr[trk][sec] == 0xffffffff || fdd->inf.vfdd.ptr[trk][sec] == 0x00000000) {
			FillMemory(fdc.buf, secsize, fdd->inf.vfdd.id[trk][secR].D);
		}
		else {
			seekp = fdd->inf.vfdd.ptr[trk][sec];
			hdl = file_open_rb(fdd->fname);
			if (hdl == FILEH_INVALID) {
				fddlasterror = 0xe0;
				return(FAILURE);
			}
			if ((file_seek(hdl, seekp, FSEEK_SET) != seekp) ||
				(file_read(hdl, fdc.buf, secsize) != secsize)) {
				file_close(hdl);
				fddlasterror = 0xe0;
				return(FAILURE);
			}
			file_close(hdl);
		}
	}

	fdc.bufcnt = secsize;
	fddlasterror = 0x00;
	return(SUCCESS);
}

BRESULT fdd_write_vfdd(FDDFILE fdd) {

	FILEH	hdl;
	UINT	trk;
	UINT	sec;
	UINT	secR;
	UINT	secsize;
	long	seekp;
	UINT	i;

	fddlasterror = 0x00;
	if (fdd_seeksector_common(fdd)) {
		fddlasterror = 0xe0;
		return(FAILURE);
	}
	if (fdd->protect) {
		fddlasterror = 0x70;
		return(FAILURE);
	}
	trk = (fdc.treg[fdc.us] << 1) + fdc.hd;
	sec = fdc.R - 1;
	secR = 0xff;
	for (i = 0; i < VFDD_SECMAX; i++) {
		if (fdd->inf.vfdd.id[trk][i].R == fdc.R) {
			secR = i;
			break;
		}
	}
	if (secR == 0xff) {
		return(FAILURE);
	}
	if (fdc.N != fdd->inf.vfdd.id[trk][secR].N) {
		fddlasterror = 0xc0;
		return(FAILURE);
	}

	if (fdd->type == DISKTYPE_VFDD) {
		hdl = file_open(fdd->fname);
		if (hdl == FILEH_INVALID) {
			fddlasterror = 0xc0;
			return(FAILURE);
		}
		secsize = 128 << fdd->inf.vfdd.id[trk][secR].N;
		seekp = fdd->inf.vfdd.ptr[trk][sec];
		if (seekp == -1 || seekp == 0) {
			UINT32	fdsize;

			fdsize = (UINT32)file_getsize(hdl);
			STOREINTELDWORD(&fdd->inf.vfdd.id[trk][secR].dataPoint, fdsize);
			fdd->inf.vfdd.ptr[trk][sec] = fdsize;
			file_seek(hdl, 0, 0);
			file_write(hdl, &fdd->inf.vfdd.head, VFDD_HEADERSIZE);
			seekp = fdsize;
		}
		if ((file_seek(hdl, seekp, FSEEK_SET) != seekp) ||
			(file_write(hdl, fdc.buf, secsize) != secsize)) {
			file_close(hdl);
			fddlasterror = 0xc0;
			return(FAILURE);
		}
		file_close(hdl);
	}

	fdc.bufcnt = secsize;
	fddlasterror = 0x00;

	return(SUCCESS);
}

BRESULT fdd_readid_vfdd(FDDFILE fdd) {

	UINT	trk;
	UINT	sec;
	UINT	i;

	fddlasterror = 0x00;
	if ((!fdc.mf) ||
		(fdc.rpm[fdc.us] != fdd->inf.xdf.rpm) ||
		(fdc.crcn >= fdd->inf.xdf.sectors)) {
		fddlasterror = 0xe0;
		return(FAILURE);
	}
	fdc.C = fdc.treg[fdc.us];
	fdc.H = fdc.hd;
	fdc.R = ++fdc.crcn;
	trk = (fdc.C << 1) + fdc.H;
	sec = 0xff;
	for (i = 0; i < VFDD_SECMAX; i++) {
		if (fdd->inf.vfdd.id[trk][i].R == fdc.R) {
			sec = i;
			break;
		}
	}
	if (sec == 0xff) {
		fddlasterror = 0xe0;
		return(FAILURE);
	}
	fdc.N = fdd->inf.vfdd.id[trk][sec].N;
	return(SUCCESS);
}

#endif
