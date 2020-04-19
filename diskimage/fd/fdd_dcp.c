#include	"compiler.h"
#include	"dosio.h"
#include	"pccore.h"
#include	"iocore.h"

#ifdef SUPPORT_KAI_IMAGES

#include	"diskimage/fddfile.h"
#include	"diskimage/fd/fdd_xdf.h"
#include	"diskimage/fd/fdd_dcp.h"

typedef struct {
	UINT8		mediatype;
	_XDFINFO	xdf;
} __DCPINFO;

//	全トラック格納イメージチェック用
static const __DCPINFO supportdcp[] = {
	{0x01, {162, 154,  8, 3, DISKTYPE_2HD, 0}},	//	01h	2HD- 8セクタ(1.25MB)
	{0x02, {162, 160, 15, 2, DISKTYPE_2HD, 0}},	//	02h	2HD-15セクタ(1.21MB)
	{0x03, {162, 160, 18, 2, DISKTYPE_2HD, 1}},	//	03h	2HQ-18セクタ(1.44MB)
	{0x04, {162, 160,  8, 2, DISKTYPE_2DD, 0}},	//	04h	2DD- 8セクタ( 640KB)
	{0x05, {162, 160,  9, 2, DISKTYPE_2DD, 0}},	//	05h	2DD- 9セクタ( 720KB)
	{0x08, {162, 154,  9, 3, DISKTYPE_2HD, 0}},	//	08h	2HD- 9セクタ(1.44MB)
	{0x11, {162, 154, 26, 1, DISKTYPE_2HD, 0}},	//	11h	BASIC-2HD
	{0x19, {162, 160, 16, 1, DISKTYPE_2DD, 0}},	//	19h	BASIC-2DD
	{0x21, {162, 154, 26, 1, DISKTYPE_2HD, 0}},	//	21h	2HD-26セクタ
};

BRESULT fdd_set_dcp(FDDFILE fdd, FDDFUNC fdd_fn, const OEMCHAR *fname, int ro) {

const __DCPINFO	*dcp;
	short		attr;
	FILEH		fh;
	UINT32		fdsize;
	UINT		size;
	UINT		rsize;
	UINT		tracksize;
	UINT32		trackptr;
	UINT		i;

	attr = file_attr(fname);
	if (attr & 0x18) {
		return(FAILURE);
	}
	if(!ro) {
		if(attr & FILEATTR_READONLY) {
			ro = 1;
		}
	}
	if(ro) {
		fh = file_open_rb(fname);
	} else {
		fh = file_open(fname);
	}
	if (fh == FILEH_INVALID) {
		return(FAILURE);
	}
	fdsize = (UINT32)file_getsize(fh);
	rsize = file_read(fh, &fdd->inf.dcp.head, DCP_HEADERSIZE);	//	DCPヘッダ読込
	file_close(fh);
	if (rsize != DCP_HEADERSIZE) {
		return(FAILURE);
	}

	//	全トラック格納イメージチェック
	dcp = supportdcp;
	while(dcp < (supportdcp + NELEMENTS(supportdcp))) {
		if (fdd->inf.dcp.head.mediatype == dcp->mediatype) {
			if (fdd->inf.dcp.head.alltrackflg == 0x01) {
				//	全トラック格納フラグが0x01の場合、ファイルサイズチェック
				size = dcp->xdf.tracks;
				size *= dcp->xdf.sectors;
				size <<= (7 + dcp->xdf.n);
				size += dcp->xdf.headersize;
				if (size != fdsize) {
					return(FAILURE);
				}
			}
			fdd->type = DISKTYPE_DCP;
			fdd->protect = ((attr & 0x01) || (ro)) ? TRUE : FALSE;
			fdd->inf.xdf = dcp->xdf;

			//	ディスクアクセス時用に各トラックのオフセットを算出
			tracksize = fdd->inf.xdf.sectors * (128 << fdd->inf.xdf.n);
//			trackptr = 0;
			trackptr = DCP_HEADERSIZE;
			for(i = 0; i < fdd->inf.xdf.tracks; i++) {
				if (fdd->inf.dcp.head.trackmap[i] == 0x01 || fdd->inf.dcp.head.alltrackflg == 0x01) {
					//	トラックデータが存在する(trackmap[i] = 0x01)
					//	or 全トラック格納フラグが0x01
					fdd->inf.dcp.ptr[i] = trackptr;
					if (i == 0 && fdd->inf.dcp.head.mediatype == DCP_DISK_2HD_BAS) {
						trackptr += tracksize / 2;	//	BASIC-2HD、track 0用小細工
					}
					else {
						trackptr += tracksize;
					}
				}
				else {
					//	イメージファイル上に存在しないトラック
					fdd->inf.dcp.ptr[i] = 0;
				}
			}

			//	処理関数群を登録
			//	※read、write以外は構造体の小細工でxdf系と共用
			fdd_fn->eject		= fdd_eject_xxx;
			fdd_fn->diskaccess	= fdd_diskaccess_common;
			fdd_fn->seek		= fdd_seek_common;
			fdd_fn->seeksector	= fdd_seeksector_common;
			fdd_fn->read		= fdd_read_dcp;
			fdd_fn->write		= fdd_write_dcp;
			fdd_fn->readid		= fdd_readid_common;
			fdd_fn->writeid		= fdd_dummy_xxx;
			fdd_fn->formatinit	= fdd_dummy_xxx;
			fdd_fn->formating	= fdd_formating_xxx;
			fdd_fn->isformating	= fdd_isformating_xxx;

			return(SUCCESS);
		}
		dcp++;
	}

	return(FAILURE);
}

BRESULT fdd_read_dcp(FDDFILE fdd) {

	FILEH	hdl;
	UINT	track;
	UINT	secsize;
	long	seekp;

	fddlasterror = 0x00;
	if (fdd_seeksector_common(fdd)) {
		return(FAILURE);
	}
	if (fdc.N != fdd->inf.xdf.n) {
		fddlasterror = 0xc0;
		return(FAILURE);
	}
	/* 170101 ST modified to work on Windows 9x/2000 form ... */
	if (fdc.eot > fdd->inf.xdf.sectors) {
		fddlasterror = 0xc0;
		return(FAILURE);
	}
	/* 170101 ST modified to work on Windows 9x/2000 ... to */

	track = (fdc.treg[fdc.us] << 1) + fdc.hd;
	secsize = 128 << fdd->inf.xdf.n;
	if (fdd->inf.dcp.head.mediatype == DCP_DISK_2HD_BAS && track == 0) {
		//	BASIC-2HD、track 0用小細工
		secsize /= 2;
	}
	if ((fdd->type == DISKTYPE_BETA) ||
		(fdd->type == DISKTYPE_DCP && fdd->inf.dcp.head.trackmap[track] == 0x01) ||
		(fdd->type == DISKTYPE_DCP && fdd->inf.dcp.head.alltrackflg == 0x01)) {

		seekp = fdc.R - 1;
		seekp <<= (7 + fdd->inf.xdf.n);
		if (fdd->inf.dcp.head.mediatype == DCP_DISK_2HD_BAS && track == 0) {
			//	BASIC-2HD、track 0用小細工
			seekp /= 2;
		}
		seekp += fdd->inf.dcp.ptr[track];
//		seekp += fdd->inf.xdf.headersize;

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
	else {
		//	ファイル上にデータの存在しないセクタは0xE5で埋めて返す
		//	※DCUだと違う？
		FillMemory(fdc.buf, secsize, 0xe5);
	}
#if 0
	if (fdd->inf.dcp.head.trackmap[track] != 0x01) {
//	if (fdd->inf.dcp.ptr[track] == 0) {
		//	ファイル上にデータの存在しないセクタは0xE5で埋めて返す
		//	※DCUだと違う？
		FillMemory(fdc.buf, secsize, 0xe5);
	}
	else {
		seekp = fdc.R - 1;
		seekp <<= (7 + fdd->inf.xdf.n);
		if (fdd->inf.dcp.head.mediatype == DCP_DISK_2HD_BAS && track == 0) {
			//	BASIC-2HD、track 0用小細工
			seekp /= 2;
		}
		seekp += fdd->inf.dcp.ptr[track];
		seekp += fdd->inf.xdf.headersize;

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
#endif
	fdc.bufcnt = secsize;
	fddlasterror = 0x00;
	return(SUCCESS);
}

BRESULT makenewtrack_dcp(FDDFILE fdd) {

#if 1
	FILEH	hdl;
//	UINT	curtrack;
	UINT	newtrack;

	UINT32	tracksize;
	UINT32	length;
	UINT	size;
	UINT	rsize;
//	int		t;
	UINT8	tmp[0x0400];
//	UINT32	cur;

	int		i;
	UINT32	ptr;
	UINT32	fdsize;

	if (fdd->protect) {
		fddlasterror = 0x70;
		return(FAILURE);
	}

	hdl = file_open(fdd->fname);
	if (hdl == FILEH_INVALID) {
		fddlasterror = 0xc0;
		return(FAILURE);
	}

	fdsize = (UINT32)file_getsize(hdl);

	newtrack = (fdc.treg[fdc.us] << 1) + fdc.hd;
	tracksize = fdd->inf.xdf.sectors * (128 << fdd->inf.xdf.n);
	if (fdd->inf.dcp.head.mediatype == DCP_DISK_2HD_BAS && newtrack == 0) {
		tracksize /= 2;
	}
	//	ずらし始めるオフセット取得
	ptr = 0;
	for (i = newtrack; i < DCP_TRACKMAX; i++) {
		if (fdd->inf.dcp.ptr[i] != 0) {
			ptr = fdd->inf.dcp.ptr[i];
			break;
		}
	}
	if (ptr != 0) {
//		ptr += fdd->inf.xdf.headersize;
		length = fdsize - ptr;

		while(length) {
			if (length >= (long)(sizeof(tmp))) {
				size = sizeof(tmp);
			}
			else {
				size = length;
			}
			length -= size;
			file_seek(hdl, ptr + length, 0);
			rsize = file_read(hdl, tmp, size);
			file_seek(hdl, ptr + length + tracksize, 0);
			file_write(hdl, tmp, rsize);
		}

		//	各トラックのオフセット再計算
		fdd->inf.dcp.ptr[newtrack] = ptr;
		ptr += tracksize;
		for (i = newtrack+1; i < DCP_TRACKMAX; i++) {
			if (fdd->inf.dcp.ptr[i] != 0) {
				fdd->inf.dcp.ptr[i] = ptr;
				ptr += tracksize;
			}
		}
	}
	else {
		fdd->inf.dcp.ptr[newtrack] = fdsize;
	}

	file_close(hdl);

	return(SUCCESS);
#else
	return(FAILURE);
#endif
}

BRESULT refreshheader_dcp(FDDFILE fdd) {

	FILEH	hdl;

	if (fdd->protect) {
		hdl = file_open_rb(fdd->fname);
	} else {
		hdl = file_open(fdd->fname);
	}
	if (hdl == FILEH_INVALID) {
		fddlasterror = 0xc0;
		return(FAILURE);
	}
	file_seek(hdl, 0, 0);
	file_write(hdl, &fdd->inf.dcp.head, DCP_HEADERSIZE);
	file_close(hdl);

	return(SUCCESS);
}


BRESULT fdd_write_dcp(FDDFILE fdd) {

	FILEH	hdl;
	UINT	track;
	UINT	secsize;
	long	seekp;

	fddlasterror = 0x00;
	if (fdd_seeksector_common(fdd)) {
		fddlasterror = 0xe0;
		return(FAILURE);
	}
	if (fdd->protect) {
		fddlasterror = 0x70;
		return(FAILURE);
	}
	if (fdc.N != fdd->inf.xdf.n) {
		fddlasterror = 0xc0;
		return(FAILURE);
	}
	/* 170101 ST modified to work on Windows 9x/2000 form ... */
	if (fdc.eot > fdd->inf.xdf.sectors) {
		fddlasterror = 0xc0;
		return(FAILURE);
	}
	/* 170101 ST modified to work on Windows 9x/2000 ... to */

	track = (fdc.treg[fdc.us] << 1) + fdc.hd;

	if ((fdd->type == DISKTYPE_BETA) ||
		(fdd->type == DISKTYPE_DCP && fdd->inf.dcp.head.trackmap[track] == 0x01) ||
		(fdd->type == DISKTYPE_DCP && fdd->inf.dcp.head.alltrackflg == 0x01)) {

		secsize = 128 << fdd->inf.xdf.n;
		seekp = fdc.R - 1;
		seekp <<= (7 + fdd->inf.xdf.n);
		if (fdd->inf.dcp.head.mediatype == DCP_DISK_2HD_BAS && track == 0) {
			//	BASIC-2HD、track 0用小細工
			secsize /= 2;
			seekp /= 2;
		}
		seekp += fdd->inf.dcp.ptr[track];
//		seekp += fdd->inf.xdf.headersize;

		hdl = file_open(fdd->fname);
		if (hdl == FILEH_INVALID) {
			fddlasterror = 0xc0;
			return(FAILURE);
		}
		if ((file_seek(hdl, seekp, FSEEK_SET) != seekp) ||
			(file_write(hdl, fdc.buf, secsize) != secsize)) {
			file_close(hdl);
			fddlasterror = 0xc0;
			return(FAILURE);
		}
		file_close(hdl);
	}
	else {
		//	新規トラック挿入後、再帰呼び出し
		BRESULT	r;
		r = makenewtrack_dcp(fdd);
		if (r != SUCCESS) {
			return r;
		}
		fdd->inf.dcp.head.trackmap[track] = 0x01;
		r = refreshheader_dcp(fdd);
		if (r != SUCCESS) {
			return r;
		}
		return(fdd_write_dcp(fdd));
	}
#if 0
	if (fdd->inf.dcp.head.trackmap[track] != 0x01) {
//	if (fdd->inf.dcp.ptr[track] == 0) {
		//	データの存在しないトラックはエラーにしとく
//		fddlasterror = 0xc0;
//		return(FAILURE);
		//	新規トラック挿入＆ヘッダ部更新後、
		//	再帰呼び出し
		BRESULT	r;
		r = makenewtrack_dcp(fdd);
		if (r != SUCCESS) {
			return r;
		}
		//	r = refreshheader_dcp(fdd);
		//	if (r != SUCCESS) {
		//		return r;
		//	}
		return(fdd_write_dcp(fdd));
	}

	secsize = 128 << fdd->inf.xdf.n;
	seekp = fdc.R - 1;
	seekp <<= (7 + fdd->inf.xdf.n);
	if (fdd->inf.dcp.head.mediatype == DCP_DISK_2HD_BAS && track == 0) {
		//	BASIC-2HD、track 0用小細工
		secsize /= 2;
		seekp /= 2;
	}
	seekp += fdd->inf.dcp.ptr[track];
	seekp += fdd->inf.xdf.headersize;

	hdl = file_open(fdd->fname);
	if (hdl == FILEH_INVALID) {
		fddlasterror = 0xc0;
		return(FAILURE);
	}
	if ((file_seek(hdl, seekp, FSEEK_SET) != seekp) ||
		(file_write(hdl, fdc.buf, secsize) != secsize)) {
		file_close(hdl);
		fddlasterror = 0xc0;
		return(FAILURE);
	}
	file_close(hdl);
#endif
	fdc.bufcnt = secsize;
	fddlasterror = 0x00;
	if (fdd->type == DISKTYPE_DCP ) {
		if (fdd->inf.dcp.head.trackmap[track] != 0x01) {
			fdd->inf.dcp.head.trackmap[track] = 0x01;
			refreshheader_dcp(fdd);
		}
	}
	return(SUCCESS);
}

#endif
