#include	"compiler.h"
#include	"dosio.h"
#include	"pccore.h"
#include	"iocore.h"

#ifdef SUPPORT_KAI_IMAGES

#include	"diskimage/fddfile.h"
#include	"diskimage/fd/fdd_nfd.h"

static const UINT8 nfd_FileID_r0[15] =
						{'T','9','8','F','D','D','I','M','A','G','E','.','R','0',0x00};
static const UINT8 nfd_FileID_r1[15] =
						{'T','9','8','F','D','D','I','M','A','G','E','.','R','1',0x00};

BRESULT fdd_set_nfd(FDDFILE fdd, FDDFUNC fdd_fn, const OEMCHAR *fname, int ro) {

	short		attr;
	FILEH		fh;
	UINT		rsize;
	UINT32		ptr;
	UINT		i;
	UINT		j;

	attr = file_attr(fname);
	if (attr & 0x18) {
		return(FAILURE);
	}
	fh = file_open(fname);
	if (fh == FILEH_INVALID) {
		return(FAILURE);
	}
	rsize = file_read(fh, &fdd->inf.nfd.head, NFD_HEADERSIZE);	//	nfdヘッダ読込(とりあえずr0で)
	file_close(fh);
	if (rsize != NFD_HEADERSIZE) {
		return(FAILURE);
	}

	//	識別IDチェック
	if (memcmp(fdd->inf.nfd.head.r0.szFileID, nfd_FileID_r0, 15) == 0) {
		//	NFD r0形式
		const	NFD_SECT_ID	*sec_nfd;

		fdd->type = DISKTYPE_NFD;
		fdd->protect = ((attr & 0x01) || (ro)) ? TRUE : FALSE;
		if (fdd->inf.nfd.head.r0.flProtect) {
			fdd->protect = TRUE;
		}

		/* 170101 ST modified to work on Windows 9x/2000 from ... */
		//	最大値入れて平気？
		// fdd->inf.xdf.tracks		= NFD_TRKMAX;
		// fdd->inf.xdf.sectors	= NFD_SECMAX;
		/* 170101 ST modified to work on Windows 9x/2000 ... to */
			
		//	期待した値が入ってないことが…orz
		ptr = LOADINTELDWORD(&fdd->inf.nfd.head.r0.dwHeadSize);
//		ptr = NFD_HEADERSIZE;
//TRACEOUT(("NFD(r0) TopSec Offset[%08x]", ptr));
		//	先頭格納セクタを見て決め打ち
		//	…2DD/2HD混在フォーマットでまずい気がする
		sec_nfd = &fdd->inf.nfd.head.r0.si[0][0];
//TRACEOUT(("NFD(r0) TopSec PDA[%02x]", sec_nfd->byPDA));
		switch (sec_nfd->byPDA) {
			case	0x10:	//	640K
				fdd->inf.xdf.disktype = DISKTYPE_2DD;
				fdd->inf.xdf.rpm = 0;
				break;
			case	0x30:	//	1.44M
				fdd->inf.xdf.disktype = DISKTYPE_2HD;
				fdd->inf.xdf.rpm = 1;
				break;
			case	0x90:	//	1.2M
				fdd->inf.xdf.disktype = DISKTYPE_2HD;
				fdd->inf.xdf.rpm = 0;
				break;
			default:
				return(FAILURE);
				break;
		}
		/* 170101 ST modified to work on Windows 9x/2000 from ... */
		fdd->inf.xdf.tracks		= 0;
		fdd->inf.xdf.sectors	= 0;
		ZeroMemory(fdd->inf.nfd.ptr, sizeof(fdd->inf.nfd.ptr));
		fdd->inf.xdf.headersize = ptr;
		/* 170101 ST modified to work on Windows 9x/2000 ... to */
		//	ディスクアクセス時用に各セクタのオフセットを算出し格納
		for (i = 0; i < NFD_TRKMAX; i++) {
			for (j = 0; j < NFD_SECMAX; j++) {
TRACEOUT(("NFD(r0) C[%02x]:H[%02x]:R[%02x]:N[%02x]",
	sec_nfd->C, sec_nfd->H, sec_nfd->R, sec_nfd->N));
				if (sec_nfd->C != 0xff) {
TRACEOUT(("\tSetOffset Trk[%03d]Sec[%02x] = Offset[%08x]", i, j, ptr));
					fdd->inf.nfd.ptr[i][j] = ptr;
					ptr += 128 << sec_nfd->N;
					/* 170101 ST modified to work on Windows 9x/2000 from ... */
					fdd->inf.xdf.tracks = i;
					if (fdd->inf.xdf.sectors < j) {
						fdd->inf.xdf.sectors = j;
					}
					/* 170101 ST modified to work on Windows 9x/2000 ... to */
				}
				sec_nfd++;
			}
		}
		/* 170101 ST modified to work on Windows 9x/2000 from ... */
		fdd->inf.xdf.tracks++;
		fdd->inf.xdf.sectors++;
		/* 170101 ST modified to work on Windows 9x/2000 ... to */

		//	処理関数群を登録
		fdd_fn->eject		= fdd_eject_xxx;
		fdd_fn->diskaccess	= fdd_diskaccess_common;
		fdd_fn->seek		= fdd_seek_common;
		fdd_fn->seeksector	= fdd_seeksector_nfd;	//	変更(kaiE)
		fdd_fn->read		= fdd_read_nfd;
		fdd_fn->write		= fdd_write_nfd;
		fdd_fn->readid		= fdd_readid_nfd;
		fdd_fn->writeid		= fdd_dummy_xxx;
		fdd_fn->formatinit	= fdd_formatinit_nfd;	/* 170107 to support format command */
		fdd_fn->formating	= fdd_formating_xxx;
		fdd_fn->isformating	= fdd_isformating_xxx;
		fdd_fn->fdcresult	= TRUE;
	}
	else if (memcmp(fdd->inf.nfd.head.r1.szFileID, nfd_FileID_r1, 15) == 0) {
		//	NFD r1形式
		NFD_TRACK_ID1	trk_id;
		NFD_SECT_ID1	sec_id;
		NFD_DIAG_ID1	dia_id;
		UINT32			trksize;

TRACEOUT(("This is NFD(r1) IMAGE!"));
		//	ヘッダ再読込
		fh = file_open(fname);
		if (fh == FILEH_INVALID) {
			return(FAILURE);
		}
		rsize = file_read(fh, &fdd->inf.nfd.head, sizeof(NFD_FILE_HEAD1));

		fdd->type = DISKTYPE_NFD;
		fdd->protect = ((attr & 0x01) || (ro)) ? TRUE : FALSE;
		if (fdd->inf.nfd.head.r1.flProtect) {
			fdd->protect = TRUE;
		}

		/* 170101 ST modified to work on Windows 9x/2000 from ... */
		//	最大値入れて平気？
		//fdd->inf.xdf.tracks		= NFD_TRKMAX;
		//fdd->inf.xdf.sectors	= NFD_SECMAX;
		/* 170101 modified to work on Windows 9x/2000 ... to */

		ptr = LOADINTELDWORD(&fdd->inf.nfd.head.r1.dwHeadSize);
		/* 170107 modified to work on Windows 9x/2000 from ... */
		fdd->inf.xdf.tracks		= 0;
		fdd->inf.xdf.sectors	= 0;
		ZeroMemory(fdd->inf.nfd.ptr, sizeof(fdd->inf.nfd.ptr));
		fdd->inf.xdf.headersize = ptr;
		/* 170107 modified to work on Windows 9x/2000 ... to */
		for (i = 0; i < NFD_TRKMAX1; i++) {
			//	トラック情報ヘッダ読込
			/* 170107 modified to work on Windows 9x/2000 from ... */
			if (LOADINTELDWORD(&(fdd->inf.nfd.head.r1.dwTrackHead[i])) == 0) {
				continue;
			}
			/* 170107 modified to work on Windows 9x/2000 ... to */
			if ((file_seek(fh, LOADINTELDWORD(&(fdd->inf.nfd.head.r1.dwTrackHead[i])), FSEEK_SET) != LOADINTELDWORD(&(fdd->inf.nfd.head.r1.dwTrackHead[i]))) ||
				(file_read(fh, &trk_id, sizeof(NFD_TRACK_ID1)) != sizeof(NFD_TRACK_ID1))) {
				file_close(fh);
				return(FAILURE);
			}
			trksize = 0;
			//	セクタ情報ヘッダ読込
			for (j = 0; j < LOADINTELWORD(&(trk_id.wSector)); j++) {
				file_read(fh, &sec_id, sizeof(NFD_SECT_ID1));
//				if (sec_id.R < NFD_SECMAX) {
					fdd->inf.nfd.ptr[i][sec_id.R - 1] = ptr;
//					fdd->inf.nfd.ptr[i][j] = ptr;
//				}
				ptr += 128 << sec_id.N;
				trksize += 128 << sec_id.N;
				if (sec_id.byRetry > 0) {
					ptr += (128 << sec_id.N) * sec_id.byRetry;
					trksize += (128 << sec_id.N) * sec_id.byRetry;
				}
				if (i == 0 && j == 0) {
					//	先頭格納セクタを見て決め打ち
					//	…2DD/2HD混在フォーマットでまずい気がする
TRACEOUT(("NFD(r1) TopSec PDA[%02x]", sec_id.byPDA));
					switch (sec_id.byPDA) {
						case	0x10:	//	640K
							fdd->inf.xdf.disktype = DISKTYPE_2DD;
							fdd->inf.xdf.rpm = 0;
							break;
						case	0x30:	//	1.44M
							fdd->inf.xdf.disktype = DISKTYPE_2HD;
							fdd->inf.xdf.rpm = 1;
							break;
						case	0x00:	//	1.2M…？
						case	0x90:	//	1.2M
							fdd->inf.xdf.disktype = DISKTYPE_2HD;
							fdd->inf.xdf.rpm = 0;
							break;
						default:
							file_close(fh);
							return(FAILURE);
							break;
					}
				}
			}
			fdd->inf.nfd.trksize[i] = trksize;
			//	特殊読み込み情報ヘッダ読込
			for (j = 0; j < LOADINTELWORD(&(trk_id.wDiag)); j++) {
				file_read(fh, &dia_id, sizeof(NFD_DIAG_ID1));
				ptr += LOADINTELDWORD(&dia_id.dwDataLen);
				if (dia_id.byRetry > 0) {
					ptr += LOADINTELDWORD(&dia_id.dwDataLen) * dia_id.byRetry;
				}
			}
			/* 170107 modified to work on Windows 9x/2000 from ... */
			if(fdd->inf.xdf.tracks == 0) {
				fdd->inf.xdf.sectors = (UINT8)trk_id.wSector;
			}
			fdd->inf.xdf.tracks++;
			/* 170107 modified to work on Windows 9x/2000 ... to */
		}

		file_close(fh);

		//	処理関数群を登録
		fdd_fn->eject		= fdd_eject_xxx;
		fdd_fn->diskaccess	= fdd_diskaccess_common;
		fdd_fn->seek		= fdd_seek_common;
		fdd_fn->seeksector	= fdd_seeksector_nfd1;
		fdd_fn->read		= fdd_read_nfd1;
		fdd_fn->write		= fdd_write_nfd1;
		fdd_fn->readid		= fdd_readid_nfd1;
		fdd_fn->writeid		= fdd_dummy_xxx;
		fdd_fn->formatinit	= fdd_dummy_xxx;
		fdd_fn->formating	= fdd_formating_xxx;
		fdd_fn->isformating	= fdd_isformating_xxx;
		fdd_fn->fdcresult	= TRUE;
TRACEOUT(("Build ImageAccess Info OK ... maybe"));
	}
	else {
		return(FAILURE);
	}

	return(SUCCESS);
}

//	追加(kaiE)
BRESULT fdd_seeksector_nfd(FDDFILE fdd) {

	UINT	trk;
	BYTE	MaxR;
	UINT	i;

	TRACEOUT(("NFD(r0) seeksector [%03d]", (fdc.treg[fdc.us] << 1) + fdc.hd));

	if ((CTRL_FDMEDIA != fdd->inf.xdf.disktype) ||
		(fdc.rpm[fdc.us] != fdd->inf.xdf.rpm) ||
		(fdc.treg[fdc.us] >= (fdd->inf.xdf.tracks >> 1))) {
		TRACEOUT(("fdd_seek_nfd FAILURE CTRL_FDMEDIA[%02x], DISKTYPE[%02x]", CTRL_FDMEDIA, fdd->inf.xdf.disktype));
		TRACEOUT(("fdd_seek_nfd FAILURE fdc.rpm[%02x], fdd->rpm[%02x]", fdc.rpm[fdc.us], fdd->inf.xdf.rpm));
		TRACEOUT(("fdd_seek_nfd FAILURE fdc.treg[%02x], fdd->trk[%02x]", fdc.treg[fdc.us], (fdd->inf.xdf.tracks >> 1)));
		fddlasterror = 0xe0;
		return(FAILURE);
	}
	if (!fdc.R) {
		TRACEOUT(("fdd_seek_nfd FAILURE fdc.R[%02x]", fdc.R));
		fddlasterror = 0xc0;
		return(FAILURE);
	}

	trk = (fdc.treg[fdc.us] << 1) + fdc.hd;
	MaxR = 0;
	for (i = 0; i < NFD_SECMAX; i++) {
//		TRACEOUT(("fdd_seek_nfd1 read sector_id[C:%02x,H:%02x,R:%02x,N:%02x]", sec_id.C, sec_id.H, sec_id.R, sec_id.N));
		if (fdd->inf.nfd.head.r0.si[trk][i].R > MaxR) {
			MaxR = fdd->inf.nfd.head.r0.si[trk][i].R;
		}
	}

	if (fdc.R > MaxR) {
		TRACEOUT(("fdd_seek_nfd FAILURE fdc.R[%02x],MaxR[%02x]", fdc.R, MaxR));
		fddlasterror = 0xc0;
		return(FAILURE);
	}
	if ((fdc.mf != 0xff) && (fdc.mf != 0x40)) {
		TRACEOUT(("fdd_seek_nfd FAILURE fdc.mf[%02x]", fdc.mf));
		fddlasterror = 0xc0;
		return(FAILURE);
	}
	return(SUCCESS);
}
//

BRESULT fdd_read_nfd(FDDFILE fdd) {

	FILEH	hdl;
	UINT	trk;
	UINT	sec;
	UINT	secR;
	UINT	secsize;
	long	seekp;
	UINT	i;

	fddlasterror = 0x00;
//	変更(kaiE)
//	if (fdd_seeksector_common(fdd)) {
	if (fdd_seeksector_nfd(fdd)) {
		TRACEOUT(("NFD(r0) read FAILURE seeksector"));
		return(FAILURE);
	}
	trk = (fdc.treg[fdc.us] << 1) + fdc.hd;
	/* 170107 modified to work on Windows 9x/2000 form ... */
	if (fdc.eot && !fdd->inf.nfd.ptr[trk][fdc.eot - 1]) {
		fddlasterror = 0xc0;
		return(FAILURE);
	}
	/* 170107 modified to work on Windows 9x/2000 ... to */
	sec = fdc.R - 1;
	secR = 0xff;
	for (i = 0; i < NFD_SECMAX; i++) {
#if 0
		TRACEOUT(("NFD(r0) read NOR trk[%03d]:        C[%02x]:H[%02x]:R[%02x]:N[%02x]:MF[%02x]",
			trk,
			fdd->inf.nfd.head.r0.si[trk][i].C,
			fdd->inf.nfd.head.r0.si[trk][i].H,
			fdd->inf.nfd.head.r0.si[trk][i].R,
			fdd->inf.nfd.head.r0.si[trk][i].N,
			fdd->inf.nfd.head.r0.si[trk][i].flMFM));
#endif
		if (fdd->inf.nfd.head.r0.si[trk][i].R == fdc.R) {
			secR = i;
//			break;
		}
	}
	if (secR == 0xff) {
		TRACEOUT(("NFD(r0) read FAILURE R[%02x] not found", fdc.R));
		fddlasterror = 0xc0;	//	抜けてた…追加(kaiE)
		return(FAILURE);
	}
	if (fdc.N != fdd->inf.nfd.head.r0.si[trk][secR].N) {
		TRACEOUT(("NFD(r0) read FAILURE N not match FDC[%02x],DSK[%02x]", fdc.N, fdd->inf.nfd.head.r0.si[trk][secR].N));
		fddlasterror = 0xc0;
		return(FAILURE);
	}

	if (fdd->type == DISKTYPE_NFD) {
		secsize = 128 << fdd->inf.nfd.head.r0.si[trk][secR].N;
		seekp = fdd->inf.nfd.ptr[trk][secR];

		hdl = file_open_rb(fdd->fname);
		if (hdl == FILEH_INVALID) {
			fddlasterror = 0xe0;
			return(FAILURE);
		}
		TRACEOUT(("NFD(r0) read seek to ... [%08x]", seekp));
		if ((file_seek(hdl, seekp, FSEEK_SET) != seekp) ||
			(file_read(hdl, fdc.buf, secsize) != secsize)) {
			file_close(hdl);
			fddlasterror = 0xe0;
			return(FAILURE);
		}
		file_close(hdl);
	}

	fdc.bufcnt = secsize;
//	変更(kaiD)
//	fddlasterror = 0x00;
	//	イメージ内情報のREAD DATA(FDDBIOS)の結果を反映
	fdc.stat[fdc.us] = fdd->inf.nfd.head.r0.si[trk][secR].byST0 + (fdd->inf.nfd.head.r0.si[trk][secR].byST1 *256) + (fdd->inf.nfd.head.r0.si[trk][secR].byST2 * 256 * 256);
	fddlasterror = fdd->inf.nfd.head.r0.si[trk][secR].byStatus;
	TRACEOUT(("NFD(r0) read FDC Result Status[%02x],STS0[%02x],STS1[%02x],STS2[%02x]",
			fdd->inf.nfd.head.r0.si[trk][secR].byStatus,
			fdd->inf.nfd.head.r0.si[trk][secR].byST0,
			fdd->inf.nfd.head.r0.si[trk][secR].byST1,
			fdd->inf.nfd.head.r0.si[trk][secR].byST2));
	TRACEOUT(("NFD(r0) read C:%02x,H:%02x,R:%02x,N:%02x", fdc.C, fdc.H, fdc.R, fdc.N));
	TRACEOUT(("NFD(r0) read dump"));
	TRACEOUT(("\t%02x %02x %02x %02x %02x %02x %02x %02x",
			fdc.buf[0x00], fdc.buf[0x01], fdc.buf[0x02], fdc.buf[0x03], fdc.buf[0x04], fdc.buf[0x05], fdc.buf[0x06], fdc.buf[0x07]));
//
	return(SUCCESS);
}

BRESULT fdd_write_nfd(FDDFILE fdd) {

	FILEH	hdl;
	UINT	trk;
	UINT	sec;
	UINT	secR;
	UINT	secsize;
	long	seekp;
	UINT	i;

	fddlasterror = 0x00;
//	変更(kaiE)
//	if (fdd_seeksector_common(fdd)) {
	if (fdd_seeksector_nfd(fdd)) {
		fddlasterror = 0xe0;
		return(FAILURE);
	}
	if (fdd->protect) {
		fddlasterror = 0x70;
		return(FAILURE);
	}
	trk = (fdc.treg[fdc.us] << 1) + fdc.hd;
	/* 170107 modified to work on Windows 9x/2000 form ... */
	if (fdc.eot && !fdd->inf.nfd.ptr[trk][fdc.eot - 1]) {
		fddlasterror = 0xc0;
		return(FAILURE);
	}
	/* 170107 modified to work on Windows 9x/2000 ... to */
	sec = fdc.R - 1;
	secR = 0xff;
	for (i = 0; i < NFD_SECMAX; i++) {
		if (fdd->inf.nfd.head.r0.si[trk][i].R == fdc.R) {
			secR = i;
			break;
		}
	}
	if (secR == 0xff) {
		return(FAILURE);
	}
	if (fdc.N != fdd->inf.nfd.head.r0.si[trk][secR].N) {
		fddlasterror = 0xc0;
		return(FAILURE);
	}

	if (fdd->type == DISKTYPE_NFD) {
		secsize = 128 << fdd->inf.nfd.head.r0.si[trk][secR].N;
		seekp = fdd->inf.nfd.ptr[trk][secR];

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

	fdc.bufcnt = secsize;
	fddlasterror = 0x00;

	return(SUCCESS);
}

BRESULT fdd_readid_nfd(FDDFILE fdd) {

	UINT	trk;
	UINT	sec;
	UINT	i;

	/* 170101 ST modified to work on Windows 9x/2000 form ... */
	if (fdc.crcn >= fdd->inf.xdf.sectors) {
		fdc.crcn = 0;
		if(fdc.mt) {
			fdc.hd ^= 1;
			if (fdc.hd == 0) {
				fdc.treg[fdc.us]++;
			}
		}
		else {
			fdc.treg[fdc.us]++;
		}
	}
	/* 170101 ST modified to work on Windows 9x/2000 ... to */
	fddlasterror = 0x00;
	if ((!fdc.mf) ||
		(fdc.rpm[fdc.us] != fdd->inf.xdf.rpm) ||
		(CTRL_FDMEDIA != fdd->inf.xdf.disktype)) {
		//(fdc.crcn >= fdd->inf.xdf.sectors)) {
		fddlasterror = 0xe0;
		return(FAILURE);
	}
	fdc.C = fdc.treg[fdc.us];
	fdc.H = fdc.hd;
	fdc.R = ++fdc.crcn;
	trk = (fdc.C << 1) + fdc.H;
	sec = 0xff;
	for (i = 0; i < NFD_SECMAX; i++) {
		if (fdd->inf.nfd.head.r0.si[trk][i].R == fdc.R) {
			sec = i;
			break;
		}
	}
	if (sec == 0xff) {
		fddlasterror = 0xe0;
		return(FAILURE);
	}
	fdc.N = fdd->inf.nfd.head.r0.si[trk][sec].N;
	return(SUCCESS);
}

/* 170107 to supprt format command form ... */
/* ヘッダの更新頻度が多いのが気になります */
BRESULT fdd_formatinit_nfd(FDDFILE fdd) {
	FILEH	hdl;
	UINT32	fddtype;
	UINT32	cylinders;
	UINT32  offset;
	UINT	trk;
	UINT    secsize;
	UINT    size;
	UINT	i;

	if (fdd->protect) {
		fddlasterror = 0x70;
		return(FAILURE);
	}

	hdl = file_open(fdd->fname);
	if (hdl == FILEH_INVALID) {
		fddlasterror = 0xc0;
		return(FAILURE);
	}

	secsize = 128 << fdc.N;
	size = secsize * fdc.sc;
	trk = (fdc.treg[fdc.us] << 1) + fdc.hd;
	memset(fdc.buf, fdc.d, size);

	fddtype = 0x90;
	if (fdc.N == 2) {
		if (fdc.sc < 10) {
			fddtype = 0x10;
		}
		else if (fdc.sc > 16) {
			fddtype = 0x30;
		}
	}

	if (trk == 0) {
		ZeroMemory(fdd->inf.nfd.head.r0.si, sizeof(fdd->inf.nfd.head.r0.si));
		offset = fdd->inf.xdf.headersize;
	} else {
		offset = fdd->inf.nfd.tptr[trk];
	}
	fdd->inf.nfd.tptr[trk + 1] = offset + size;

	for (i = 0; i < fdc.sc; i++) {
		fdd->inf.nfd.head.r0.si[trk][i].C = fdc.treg[fdc.us];
		fdd->inf.nfd.head.r0.si[trk][i].H = fdc.hd;
		fdd->inf.nfd.head.r0.si[trk][i].R = i + 1;
		fdd->inf.nfd.head.r0.si[trk][i].N = fdc.N;
		fdd->inf.nfd.head.r0.si[trk][i].flMFM = 1;
		fdd->inf.nfd.head.r0.si[trk][i].flDDAM = 0;
		fdd->inf.nfd.head.r0.si[trk][i].byStatus = 0;
		fdd->inf.nfd.head.r0.si[trk][i].byST0 = fdc.hd << 2;
		fdd->inf.nfd.head.r0.si[trk][i].byST1 = 0;
		fdd->inf.nfd.head.r0.si[trk][i].byST2 = 0;
		fdd->inf.nfd.head.r0.si[trk][i].byPDA = fddtype;
		fdd->inf.nfd.ptr[trk][i] = offset;
		offset += secsize;
	}
	if (trk == 0) {
		for (; i < NFD_TRKMAX * NFD_SECMAX; i++) {
			fdd->inf.nfd.head.r0.si[trk][i].C = 0xff;
			fdd->inf.nfd.head.r0.si[trk][i].H = 0xff;
			fdd->inf.nfd.head.r0.si[trk][i].R = 0xff;
			fdd->inf.nfd.head.r0.si[trk][i].N = 0xff;
			fdd->inf.nfd.head.r0.si[trk][i].flMFM = 0xff;
			fdd->inf.nfd.head.r0.si[trk][i].flDDAM = 0xff;
			fdd->inf.nfd.head.r0.si[trk][i].byStatus = 0xe0;
			fdd->inf.nfd.head.r0.si[trk][i].byST0 = 0x40 | (fdc.hd << 2);
			fdd->inf.nfd.head.r0.si[trk][i].byST1 = 1;
			fdd->inf.nfd.head.r0.si[trk][i].byST2 = 0;
			fdd->inf.nfd.head.r0.si[trk][i].byPDA = 0;
		}
	}

	if ((file_seek(hdl, 0, FSEEK_SET) != 0) ||
		(file_write(hdl, &fdd->inf.nfd.head, NFD_HEADERSIZE) != NFD_HEADERSIZE)) {
		file_close(hdl);
		fddlasterror = 0xc0;
		return(FAILURE);
	}

	offset = fdd->inf.nfd.ptr[trk][0];
	if ((file_seek(hdl, offset, FSEEK_SET) != offset) ||
		(file_write(hdl, fdc.buf, size) != size)) {
		file_close(hdl);
		fddlasterror = 0xc0;
		return(FAILURE);
	}

	if (trk == 0) {
		fdd->inf.xdf.disktype = DISKTYPE_2HD;
		cylinders = 77;
		switch(fdc.N) {
			case 1:		// BASIC (sector size = 256)
				break;
			case 2:		// 1.44M/1.21M/2DD
				if (fdc.sc < 10) {
					fdd->inf.xdf.disktype = DISKTYPE_2DD;
				}
				cylinders = 80;
				break;
			default:	// 1.25M
				break;
		}

		fdd->inf.xdf.tracks = (UINT8)(cylinders * 2);
		fdd->inf.xdf.sectors = fdc.sc;
		fdd->inf.xdf.n = fdc.N;
		fdd->inf.xdf.rpm = fdc.rpm[fdc.us];

		// trim media image
		offset = fdd->inf.xdf.tracks * size + fdd->inf.xdf.headersize;
		file_seek(hdl, offset, FSEEK_SET);
		file_write(hdl, fdc.buf, 0);
	}

	file_close(hdl);
	fddlasterror = 0x00;
	return(SUCCESS);
}
/* 170107 to supprt format command ... to */

//	追加(kaiD)
BRESULT fdd_seeksector_nfd1(FDDFILE fdd) {

	FILEH	hdl;
	UINT	trk;
	BYTE	MaxR;
	UINT	i;
	NFD_TRACK_ID1	trk_id;
	NFD_SECT_ID1	sec_id;

	if ((CTRL_FDMEDIA != fdd->inf.xdf.disktype) ||
		(fdc.rpm[fdc.us] != fdd->inf.xdf.rpm) ||
		(fdc.treg[fdc.us] >= (fdd->inf.xdf.tracks >> 1))) {
		TRACEOUT(("fdd_seek_nfd1 FAILURE CTRL_FDMEDIA[%02x], DISKTYPE[%02x]", CTRL_FDMEDIA, fdd->inf.xdf.disktype));
		TRACEOUT(("fdd_seek_nfd1 FAILURE fdc.rpm[%02x], fdd->rpm[%02x]", fdc.rpm[fdc.us], fdd->inf.xdf.rpm));
		TRACEOUT(("fdd_seek_nfd1 FAILURE fdc.treg[%02x], fdd->trk[%02x]", fdc.treg[fdc.us], (fdd->inf.xdf.tracks >> 1)));
		fddlasterror = 0xe0;
		return(FAILURE);
	}
	if (!fdc.R) {
		TRACEOUT(("fdd_seek_nfd1 FAILURE fdc.R[%02x]", fdc.R));
		fddlasterror = 0xc0;
		return(FAILURE);
	}

	hdl = file_open_rb(fdd->fname);
	if (hdl == FILEH_INVALID) {
		TRACEOUT(("fdd_seek_nfd1 FAILURE ... 1"));
		fddlasterror = 0xe0;
		return(FAILURE);
	}
	trk = (fdc.treg[fdc.us] << 1) + fdc.hd;
	file_seek(hdl, LOADINTELDWORD(&(fdd->inf.nfd.head.r1.dwTrackHead[trk])), FSEEK_SET);
	file_read(hdl, &trk_id, sizeof(NFD_TRACK_ID1));
//	TRACEOUT(("fdd_seek_nfd1 read track_id[%03d:%08x]", trk, fdd->inf.nfd.head.r1.dwTrackHead[trk]));
	MaxR = 0;
	for (i = 0; i < LOADINTELWORD(&(trk_id.wSector)); i++) {
		file_read(hdl, &sec_id, sizeof(NFD_SECT_ID1));
//		TRACEOUT(("fdd_seek_nfd1 read sector_id[C:%02x,H:%02x,R:%02x,N:%02x]", sec_id.C, sec_id.H, sec_id.R, sec_id.N));
		if (sec_id.R > MaxR) {
			MaxR = sec_id.R;
		}
	}
	file_close(hdl);

	if (fdc.R > MaxR) {
		TRACEOUT(("fdd_seek_nfd1 FAILURE fdc.R[%02x],MaxR[%02x]", fdc.R, MaxR));
		fddlasterror = 0xc0;
		return(FAILURE);
	}
	if ((fdc.mf != 0xff) && (fdc.mf != 0x40)) {
		TRACEOUT(("fdd_seek_nfd1 FAILURE fdc.mf[%02x]", fdc.mf));
		fddlasterror = 0xc0;
		return(FAILURE);
	}
	return(SUCCESS);
}

//	RetryData有の対応…いいのか？これで
static	UINT8	rcnt = 0;

BRESULT fdd_read_nfd1(FDDFILE fdd) {

	FILEH	hdl;
	UINT	trk;
	UINT	sec;
	UINT	secR;
	UINT	secsize;
	long	seekp;
	long	ex_offset;
	long	ex_offset2;
	UINT	i;
	UINT8	MaxRetry;
	NFD_TRACK_ID1	trk_id;
	NFD_SECT_ID1	sec_id;
	NFD_SECT_ID1	sec_idx;
	NFD_DIAG_ID1	dia_id;

	//	RetryData有の対応…いいのか？これで
	rcnt++;

	fddlasterror = 0x00;
	if (fdd_seeksector_nfd1(fdd)) {
		TRACEOUT(("NFD(r1) read failure ... seeksector"));
		return(FAILURE);
	}
	trk = (fdc.treg[fdc.us] << 1) + fdc.hd;
	/* 170107 modified to work on Windows 9x/2000 form ... */
	if (fdc.eot && !fdd->inf.nfd.ptr[trk][fdc.eot - 1]) {
		fddlasterror = 0xc0;
		return(FAILURE);
	}
	/* 170107 modified to work on Windows 9x/2000 ... to */
	sec = fdc.R - 1;
	secR = 0xff;
	ex_offset = 0;
	ex_offset2 = 0;
	MaxRetry = 0;
	hdl = file_open_rb(fdd->fname);
	if (hdl == FILEH_INVALID) {
		TRACEOUT(("NFD(r1) read failure ... FILE OPEN"));
		fddlasterror = 0xe0;
		return(FAILURE);
	}
	file_seek(hdl, LOADINTELDWORD(&(fdd->inf.nfd.head.r1.dwTrackHead[trk])), FSEEK_SET);
	file_read(hdl, &trk_id, sizeof(NFD_TRACK_ID1));
	for (i = 0; i < LOADINTELWORD(&(trk_id.wSector)); i++) {
		file_read(hdl, &sec_idx, sizeof(NFD_SECT_ID1));
		TRACEOUT(("NFD(r1) read NOR trk[%03d]:        C[%02x]:H[%02x]:R[%02x]:N[%02x]:MFM[%02x]:DDAM[%02x]:Retry[%02x]:PDA[%02x]",
			trk, sec_idx.C, sec_idx.H, sec_idx.R, sec_idx.N, sec_idx.flMFM, sec_idx.flDDAM, sec_idx.byRetry, sec_idx.byPDA));
		if (sec_idx.R == fdc.R) {
			secR = i;
			memcpy(&sec_id, &sec_idx, sizeof(NFD_SECT_ID1));
			MaxRetry = sec_idx.byRetry;
//			break;
		}
//		ex_offset += 128 << sec_id.N;
	}
	if (secR == 0xff) {
		TRACEOUT(("NFD(r1) read failure ... R[%0x2] not found", fdc.R));
		file_close(hdl);
		return(FAILURE);
	}

	if (fdc.N != sec_id.N) {
		BOOL	flg;
		TRACEOUT(("NFD(r1) read failure ... N not match : fdc.n:[%02x],sec_id.N:[%02x]", fdc.N, sec_id.N));
#if 1
		flg = FALSE;
		for (i = 0; i < LOADINTELWORD(&(trk_id.wDiag)); i++) {
			file_read(hdl, &dia_id, sizeof(NFD_DIAG_ID1));
			TRACEOUT(("NFD(r1) read DIA trk[%03d]:Cmd[%02x]:C[%02x]:H[%02x]:R[%02x]:N[%02x]:Len[%08x]:Retry[%02x]",
			 trk, dia_id.Cmd, dia_id.C, dia_id.H, dia_id.R, dia_id.N, dia_id.dwDataLen, dia_id.byRetry));
			if (fdc.N == dia_id.N) {
				TRACEOUT(("\tfound diag data"));
				flg = TRUE;
				break;
			}
			ex_offset2 += LOADINTELDWORD(&(dia_id.dwDataLen));
		}
#endif
		if (flg == FALSE) {
			fddlasterror = 0xc0;
			file_close(hdl);
			return(FAILURE);
		}
	}

	if (fdd->type == DISKTYPE_NFD) {
		if (fdc.N == sec_id.N) {
			secsize = 128 << sec_id.N;
			seekp = fdd->inf.nfd.ptr[trk][sec];
			//	RetryData有の対応…いいのか？これで
			seekp += secsize * (MaxRetry ? (rcnt % MaxRetry) : 0);
		}
		else {
			secsize = LOADINTELDWORD(&(dia_id.dwDataLen));
			seekp = fdd->inf.nfd.ptr[trk][sec] + fdd->inf.nfd.trksize[trk] + ex_offset2;
		}

		TRACEOUT(("NFD(r1) read seek to ... [%08x]", seekp));
		if ((file_seek(hdl, seekp, FSEEK_SET) != seekp) ||
			(file_read(hdl, fdc.buf, secsize) != secsize)) {
			file_close(hdl);
			fddlasterror = 0xe0;
			TRACEOUT(("NFD(r1) read failure ... FILE SEEK or READ"));
			return(FAILURE);
		}
	}
	file_close(hdl);

	//	イメージ内情報のREAD DATA RESULTを反映
	fdc.stat[fdc.us] = sec_id.bySTS0 | (sec_id.bySTS1 << 8) | (sec_id.bySTS2 << 16);
	fddlasterror = sec_id.byStatus;
	fdc.bufcnt = secsize;
	TRACEOUT(("NFD(r1) read FDC Result Status[%02x],STS0[%02x],STS1[%02x],STS2[%02x]",
			sec_id.byStatus, sec_id.bySTS0, sec_id.bySTS1, sec_id.bySTS2));
	TRACEOUT(("NFD(r1) read C:%02x,H:%02x,R:%02x,N:%02x", fdc.C, fdc.H, fdc.R, fdc.N));
	TRACEOUT(("NFD(r1) read dump"));
	TRACEOUT(("\t%02x %02x %02x %02x %02x %02x %02x %02x",
			fdc.buf[0x00], fdc.buf[0x01], fdc.buf[0x02], fdc.buf[0x03], fdc.buf[0x04], fdc.buf[0x05], fdc.buf[0x06], fdc.buf[0x07]));
	return(SUCCESS);
}

BRESULT fdd_write_nfd1(FDDFILE fdd) {

	FILEH	hdl;
	UINT	trk;
	UINT	sec;
	UINT	secR;
	UINT	secsize;
	long	seekp;
	UINT	i;
	NFD_TRACK_ID1	trk_id;
	NFD_SECT_ID1	sec_id;

	fddlasterror = 0x00;
	if (fdd_seeksector_nfd1(fdd)) {
		fddlasterror = 0xe0;
		return(FAILURE);
	}
	if (fdd->protect) {
		fddlasterror = 0x70;
		return(FAILURE);
	}
	trk = (fdc.treg[fdc.us] << 1) + fdc.hd;
	/* 170107 modified to work on Windows 9x/2000 form ... */
	if (fdc.eot && !fdd->inf.nfd.ptr[trk][fdc.eot - 1]) {
		fddlasterror = 0xc0;
		return(FAILURE);
	}
	/* 170107 modified to work on Windows 9x/2000 ... to */
	sec = fdc.R - 1;
	secR = 0xff;
	hdl = file_open(fdd->fname);
	if (hdl == FILEH_INVALID) {
		fddlasterror = 0xe0;
		return(FAILURE);
	}
	file_seek(hdl, LOADINTELDWORD(&(fdd->inf.nfd.head.r1.dwTrackHead[trk])), FSEEK_SET);
	file_read(hdl, &trk_id, sizeof(NFD_TRACK_ID1));
	for (i = 0; i < LOADINTELWORD(&(trk_id.wSector)); i++) {
		file_read(hdl, &sec_id, sizeof(NFD_SECT_ID1));
		if (sec_id.R == fdc.R) {
			secR = i;
			break;
		}
	}
	if (secR == 0xff) {
		file_close(hdl);
		return(FAILURE);
	}
	if (fdc.N != sec_id.N) {
		fddlasterror = 0xc0;
		file_close(hdl);
		return(FAILURE);
	}

	if (fdd->type == DISKTYPE_NFD) {
		secsize = 128 << sec_id.N;
		seekp = fdd->inf.nfd.ptr[trk][sec];
		if ((file_seek(hdl, seekp, FSEEK_SET) != seekp) ||
			(file_write(hdl, fdc.buf, secsize) != secsize)) {
			file_close(hdl);
			fddlasterror = 0xc0;
			return(FAILURE);
		}
	}

	file_close(hdl);
	fdc.bufcnt = secsize;
	fddlasterror = 0x00;

	return(SUCCESS);
}

BRESULT fdd_readid_nfd1(FDDFILE fdd) {

	FILEH	hdl;
	UINT	trk;
	UINT	sec;
	UINT	i;
	NFD_TRACK_ID1	trk_id;
	NFD_SECT_ID1	sec_id;

	/* 170107 modified to work on Windows 9x/2000 form ... */
	if (fdc.crcn >= fdd->inf.xdf.sectors) {
		fdc.crcn = 0;
		if(fdc.mt) {
			fdc.hd ^= 1;
			if (fdc.hd == 0) {
				fdc.treg[fdc.us]++;
			}
		}
		else {
			fdc.treg[fdc.us]++;
		}
	}
	/* 170107 modified to work on Windows 9x/2000 ... to */
	fddlasterror = 0x00;
	if ((!fdc.mf) ||
		(fdc.rpm[fdc.us] != fdd->inf.xdf.rpm) ||
		(CTRL_FDMEDIA != fdd->inf.xdf.disktype)) {
		//(fdc.crcn >= fdd->inf.xdf.sectors)) {
		fddlasterror = 0xe0;
		return(FAILURE);
	}
	fdc.C = fdc.treg[fdc.us];
	fdc.H = fdc.hd;
	fdc.R = ++fdc.crcn;
	trk = (fdc.C << 1) + fdc.H;
	sec = 0xff;
	hdl = file_open_rb(fdd->fname);
	if (hdl == FILEH_INVALID) {
		fddlasterror = 0xe0;
		return(FAILURE);
	}
	file_seek(hdl, LOADINTELDWORD(&(fdd->inf.nfd.head.r1.dwTrackHead[trk])), FSEEK_SET);
	file_read(hdl, &trk_id, sizeof(NFD_TRACK_ID1));
	for (i = 0; i < LOADINTELWORD(&(trk_id.wSector)); i++) {
		file_read(hdl, &sec_id, sizeof(NFD_SECT_ID1));
		if (sec_id.R == fdc.R) {
			sec = i;
			break;
		}
	}
	file_close(hdl);
	if (sec == 0xff) {
		fddlasterror = 0xe0;
		return(FAILURE);
	}
	fdc.N = sec_id.N;
	return(SUCCESS);
}
//

#endif
