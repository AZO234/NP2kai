#include	<compiler.h>
#include	<dosio.h>
#include	<pccore.h>
#include	<io/iocore.h>

#ifdef SUPPORT_KAI_IMAGES

#include	<diskimage/fddfile.h>
#include	"diskimage/fd/fdd_nfd.h"

static const UINT8 nfd_FileID_r0[15] =
						{'T','9','8','F','D','D','I','M','A','G','E','.','R','0',0x00};
static const UINT8 nfd_FileID_r1[15] =
						{'T','9','8','F','D','D','I','M','A','G','E','.','R','1',0x00};

#define	NFD_FDCBUFSIZE	0x8000U

static UINT16 nfd_load_le16(const void *ptr) {

	const UINT8	*p;

	p = (const UINT8 *)ptr;
	return((UINT16)(p[0] | ((UINT16)p[1] << 8)));
}

static UINT32 nfd_load_le32(const void *ptr) {

	const UINT8	*p;

	p = (const UINT8 *)ptr;
	return((UINT32)p[0] | ((UINT32)p[1] << 8) |
		((UINT32)p[2] << 16) | ((UINT32)p[3] << 24));
}

//	保存されているデータ数の範囲で順番に選択する
static UINT32 nfd_retry_select(UINT8 *counter, UINT32 copies) {

	UINT32	select;

	select = *counter;
	if (select >= copies) {
		select = 0;
	}
	*counter = (UINT8)((select + 1U) % copies);
	return(select);
}

static BRESULT nfd_get_secsize(BYTE n, UINT32 *size) {

	if (n > 8) {
		return(FAILURE);
	}
	*size = ((UINT32)128) << n;
	return(SUCCESS);
}

static BYTE nfd_media_pda(FDDFILE fdd) {

	if (fdd->inf.xdf.disktype == DISKTYPE_2DD) {
		return(0x10);
	}
	return(fdd->inf.xdf.rpm ? 0x30 : 0x90);
}

static BYTE nfd_current_pda(void);

static BRESULT nfd_set_media(FDDFILE fdd, BYTE pda, BYTE n, UINT sectors) {

	if (pda == 0) {
		if (n == 2) {
			if (sectors <= 9) {
				pda = 0x10;
			}
			else if (sectors >= 17) {
				pda = 0x30;
			}
			else {
				pda = 0x90;
			}
		}
		else {
			pda = 0x90;
		}
	}
	switch (pda) {
		case	0x10:
			fdd->inf.xdf.disktype = DISKTYPE_2DD;
			fdd->inf.xdf.rpm = 0;
			break;
		case	0x30:
			fdd->inf.xdf.disktype = DISKTYPE_2HD;
			fdd->inf.xdf.rpm = 1;
			break;
		case	0x90:
			fdd->inf.xdf.disktype = DISKTYPE_2HD;
			fdd->inf.xdf.rpm = 0;
			break;
		default:
			return(FAILURE);
	}
	return(SUCCESS);
}

static BRESULT nfd_range_ok(UINT32 pos, UINT32 len, UINT32 limit) {

	if (pos > limit || len > (limit - pos)) {
		return(FAILURE);
	}
	return(SUCCESS);
}

static BRESULT nfd_track_index(FDDFILE fdd, UINT cylinder, UINT head, UINT *trk) {

	UINT	heads;

	heads = fdd->inf.nfd.revision ? fdd->inf.nfd.head.r1.byHead : fdd->inf.nfd.head.r0.byHead;
	if (head >= heads) {
		return(FAILURE);
	}
	//	NFDのトラック番号はC/Hの物理スロット。片面でもC*2+Hの番号を維持する
	*trk = cylinder * 2U + head;
	if (*trk >= fdd->inf.nfd.trackcount) {
		return(FAILURE);
	}
	return(SUCCESS);
}

static BRESULT fdd_seek_nfd_common(FDDFILE fdd) {

	UINT	trk;
	UINT	i;

	if (!nfd_current_pda() || nfd_track_index(fdd, fdc.ncn, fdc.hd, &trk)) {
		return(FAILURE);
	}
	if (fdd->inf.nfd.revision) {
		return(nfd_load_le32(&fdd->inf.nfd.head.r1.dwTrackHead[trk]) ? SUCCESS : FAILURE);
	}
	for (i = 0; i < NFD_SECMAX; i++) {
		if (fdd->inf.nfd.head.r0.si[trk][i].C != 0xff) {
			return(SUCCESS);
		}
	}
	return(FAILURE);
}

static BRESULT nfd1_read_track(FILEH hdl, FDDFILE fdd, UINT trk, NFD_TRACK_ID1 *trk_id) {

	UINT32	pos;
	UINT32	headsize;
	UINT32	infosize;
	UINT	sectors;
	UINT	diags;

	pos = nfd_load_le32(&fdd->inf.nfd.head.r1.dwTrackHead[trk]);
	if (!pos) {
		return(FAILURE);
	}
	headsize = nfd_load_le32(&fdd->inf.nfd.head.r1.dwHeadSize);
	//	トラック情報はr1固定ヘッダ以降、dwHeadSizeより前に存在する
	if (pos < sizeof(NFD_FILE_HEAD1) ||
		nfd_range_ok(pos, sizeof(*trk_id), headsize)) {
		return(FAILURE);
	}
	if ((file_seek(hdl, pos, FSEEK_SET) != (FILEPOS)pos) ||
		(file_read(hdl, trk_id, sizeof(*trk_id)) != sizeof(*trk_id))) {
		return(FAILURE);
	}
	sectors = nfd_load_le16(&trk_id->wSector);
	diags = nfd_load_le16(&trk_id->wDiag);
	infosize = sizeof(*trk_id) + ((UINT32)sectors + (UINT32)diags) * 16U;
	if (nfd_range_ok(pos, infosize, headsize)) {
		return(FAILURE);
	}
	return(SUCCESS);
}

static BYTE nfd_current_cmd(void) {

	//	BIOS INT 1Bh経由ではfdc.cmdが更新されないため、BIOS用コマンド値を参照する
	if (fdc.mf == 0xff) {
		return((BYTE)(fddbioscmd & 0x0f));
	}
	return((BYTE)(fdc.cmd & 0x1f));
}

static BOOL nfd_mfm_match(BYTE image_mfm) {

	if (fdc.mf == 0xff) {
		return(TRUE);
	}
	return(image_mfm ? (fdc.mf == 0x40) : (fdc.mf == 0));
}

static BOOL nfd_mark_match(BYTE image_ddam) {

	BYTE	cmd;

	cmd = nfd_current_cmd();
	if (cmd == 0x0c) {
		return(image_ddam != 0);
	}
	if (cmd == 0x06) {
		return(image_ddam == 0);
	}
	return(TRUE);
}

static BYTE nfd_current_pda(void) {

	if (CTRL_FDMEDIA == DISKTYPE_2DD) {
		return(0x10);
	}
	if (CTRL_FDMEDIA == DISKTYPE_2HD) {
		return(fdc.rpm[fdc.us] ? 0x30 : 0x90);
	}
	return(0);
}

static BOOL nfd_pda_match(FDDFILE fdd, BYTE pda) {

	BYTE	current;

	current = nfd_current_pda();
	if (!current) {
		return(FALSE);
	}
	//	PDAはイメージ全体ではなく各エントリの属性。0の場合はマウント時の判定値を使用する
	if (!pda) {
		pda = nfd_media_pda(fdd);
	}
	return(pda == current);
}

static UINT nfd0_find_sector(FDDFILE fdd, UINT trk, BOOL exact_mark) {

	NFD_SECT_ID	*id;
	UINT		i;

	for (i = 0; i < NFD_SECMAX; i++) {
		id = &fdd->inf.nfd.head.r0.si[trk][i];
		if (id->C == fdc.C && id->H == fdc.H && id->R == fdc.R && id->N == fdc.N &&
			nfd_mfm_match(id->flMFM) && nfd_pda_match(fdd, id->byPDA) &&
			(!exact_mark || nfd_mark_match(id->flDDAM))) {
			return(i);
		}
	}
	return(NFD_SECMAX);
}

static BRESULT nfd1_find_sector(FILEH hdl, FDDFILE fdd, UINT trk, NFD_SECT_ID1 *found,
	UINT32 *datapos, UINT *secindex, BOOL exact_mark) {
	NFD_TRACK_ID1	trk_id;
	NFD_SECT_ID1	sec;
	UINT32	pos;
	UINT32	data;
	UINT32	size;
	UINT32	copies;
	UINT	sectors;
	UINT	i;

	if (nfd1_read_track(hdl, fdd, trk, &trk_id)) {
		return(FAILURE);
	}
	pos = nfd_load_le32(&fdd->inf.nfd.head.r1.dwTrackHead[trk]) + sizeof(trk_id);
	data = fdd->inf.nfd.tptr[trk];
	sectors = nfd_load_le16(&trk_id.wSector);
	for (i = 0; i < sectors; i++) {
		if ((file_seek(hdl, pos, FSEEK_SET) != (FILEPOS)pos) ||
			(file_read(hdl, &sec, sizeof(sec)) != sizeof(sec)) ||
			nfd_get_secsize(sec.N, &size)) {
			return(FAILURE);
		}
		copies = (UINT32)sec.byRetry + 1U;
		if (sec.C == fdc.C && sec.H == fdc.H && sec.R == fdc.R && sec.N == fdc.N &&
			nfd_mfm_match(sec.flMFM) && nfd_pda_match(fdd, sec.byPDA) &&
			(!exact_mark || nfd_mark_match(sec.flDDAM))) {
			*found = sec;
			*datapos = data;
			*secindex = i;
			return(SUCCESS);
		}
		data += size * copies;
		pos += sizeof(sec);
	}
	return(FAILURE);
}

static BRESULT nfd1_find_diag(FILEH hdl, FDDFILE fdd, UINT trk, NFD_DIAG_ID1 *found,
	UINT32 *datapos, UINT *diagindex) {
	NFD_TRACK_ID1	trk_id;
	NFD_SECT_ID1	sec;
	NFD_DIAG_ID1	dia;
	UINT32	pos;
	UINT32	data;
	UINT32	size;
	UINT32	copies;
	UINT	sectors;
	UINT	diags;
	UINT	i;
	BYTE	cmd;

	if (nfd1_read_track(hdl, fdd, trk, &trk_id)) {
		return(FAILURE);
	}
	pos = nfd_load_le32(&fdd->inf.nfd.head.r1.dwTrackHead[trk]) + sizeof(trk_id);
	data = fdd->inf.nfd.tptr[trk];
	sectors = nfd_load_le16(&trk_id.wSector);
	diags = nfd_load_le16(&trk_id.wDiag);
	for (i = 0; i < sectors; i++) {
		if ((file_seek(hdl, pos, FSEEK_SET) != (FILEPOS)pos) ||
			(file_read(hdl, &sec, sizeof(sec)) != sizeof(sec)) ||
			nfd_get_secsize(sec.N, &size)) {
			return(FAILURE);
		}
		copies = (UINT32)sec.byRetry + 1U;
		data += size * copies;
		pos += sizeof(sec);
	}
	cmd = (BYTE)(nfd_current_cmd() & 0x0f);
	for (i = 0; i < diags; i++) {
		if ((file_seek(hdl, pos, FSEEK_SET) != (FILEPOS)pos) ||
			(file_read(hdl, &dia, sizeof(dia)) != sizeof(dia))) {
			return(FAILURE);
		}
		size = nfd_load_le32(&dia.dwDataLen);
		copies = (UINT32)dia.byRetry + 1U;
		if (!size || size > NFD_FDCBUFSIZE) {
			return(FAILURE);
		}
		if ((dia.Cmd & 0x0f) == cmd && dia.C == fdc.C && dia.H == fdc.H &&
			dia.R == fdc.R && dia.N == fdc.N && nfd_pda_match(fdd, dia.byPDA)) {
			*found = dia;
			*datapos = data;
			*diagindex = i;
			return(SUCCESS);
		}
		data += size * copies;
		pos += sizeof(dia);
	}
	return(FAILURE);
}

BRESULT fdd_set_nfd(FDDFILE fdd, FDDFUNC fdd_fn, const OEMCHAR *fname, int ro) {

	short	attr;
	FILEH	fh;
	FILELEN	flen;
	UINT32	filesize;
	UINT32	headsize;
	UINT32	data;
	UINT32	tpos;
	UINT32	infosize;
	UINT32	secsize;
	UINT32	copies;
	UINT	i;
	UINT	j;
	UINT	k;
	UINT	order[NFD_TRKMAX1];
	UINT	ordercnt;
	UINT	best;
	UINT32	bestpos;
	UINT32	prevend;
	UINT	maxtrk;
	UINT	maxsec;
	UINT	sectors;
	UINT	diags;
	BOOL	media_set;
	BYTE	id[15];
	NFD_TRACK_ID1	trk_id;
	NFD_SECT_ID1	sec_id;
	NFD_DIAG_ID1	dia_id;

	attr = file_attr(fname);
	if (attr & 0x18) {
		return(FAILURE);
	}
	fh = file_open_rb(fname);
	if (fh == FILEH_INVALID) {
		return(FAILURE);
	}
	flen = file_getsize(fh);
	if (flen < 15 || flen > (FILELEN)0xffffffffUL) {
		file_close(fh);
		return(FAILURE);
	}
	filesize = (UINT32)flen;
	if ((file_seek(fh, 0, FSEEK_SET) != 0) || file_read(fh, id, sizeof(id)) != sizeof(id)) {
		file_close(fh);
		return(FAILURE);
	}

	ZeroMemory(&fdd->inf.nfd, sizeof(fdd->inf.nfd));
	fdd->type = DISKTYPE_NFD;
	fdd->protect = ((attr & 0x01) || ro) ? TRUE : FALSE;

	if (memcmp(id, nfd_FileID_r0, sizeof(id)) == 0) {
		NFD_SECT_ID	*sec;
		if (filesize < sizeof(NFD_FILE_HEAD) ||
			(file_seek(fh, 0, FSEEK_SET) != 0) ||
			(file_read(fh, &fdd->inf.nfd.head.r0, sizeof(NFD_FILE_HEAD)) != sizeof(NFD_FILE_HEAD))) {
			file_close(fh);
			return(FAILURE);
		}
		headsize = nfd_load_le32(&fdd->inf.nfd.head.r0.dwHeadSize);
		if (headsize < sizeof(NFD_FILE_HEAD) || headsize > filesize) {
			file_close(fh);
			return(FAILURE);
		}
		if (fdd->inf.nfd.head.r0.byHead < 1 || fdd->inf.nfd.head.r0.byHead > 2) {
			file_close(fh);
			return(FAILURE);
		}
		if (fdd->inf.nfd.head.r0.flProtect) {
			fdd->protect = TRUE;
		}
		fdd->inf.nfd.revision = 0;
		fdd->inf.xdf.headersize = headsize;
		data = headsize;
		maxtrk = 0;
		maxsec = 0;
		media_set = FALSE;
		for (i = 0; i < NFD_TRKMAX; i++) {
			fdd->inf.nfd.tptr[i] = data;
			for (j = 0; j < NFD_SECMAX; j++) {
				sec = &fdd->inf.nfd.head.r0.si[i][j];
				if (sec->C == 0xff) {
					continue;
				}
				if (nfd_get_secsize(sec->N, &secsize) || nfd_range_ok(data, secsize, filesize)) {
					file_close(fh);
					return(FAILURE);
				}
				fdd->inf.nfd.ptr[i][j] = data;
				data += secsize;
				if (i + 1 > maxtrk) {
					maxtrk = i + 1;
				}
				if (j + 1 > maxsec) {
					maxsec = j + 1;
				}
				if (!media_set) {
					if (nfd_set_media(fdd, sec->byPDA, sec->N, NFD_SECMAX)) {
						file_close(fh);
						return(FAILURE);
					}
					media_set = TRUE;
				}
			}
			fdd->inf.nfd.trksize[i] = data - fdd->inf.nfd.tptr[i];
		}
		if (!media_set || !maxtrk) {
			file_close(fh);
			return(FAILURE);
		}
		fdd->inf.nfd.trackcount = (UINT16)maxtrk;
		fdd->inf.xdf.tracks = (UINT8)maxtrk;
		fdd->inf.xdf.sectors = (UINT8)maxsec;

		fdd_fn->eject		= fdd_eject_xxx;
		fdd_fn->diskaccess	= fdd_diskaccess_common;
		fdd_fn->seek		= fdd_seek_nfd_common;
		fdd_fn->seeksector	= fdd_seeksector_nfd;
		fdd_fn->read		= fdd_read_nfd;
		fdd_fn->write		= fdd_write_nfd;
		fdd_fn->readid		= fdd_readid_nfd;
		fdd_fn->writeid		= fdd_dummy_xxx;
		fdd_fn->formatinit	= fdd_formatinit_nfd;
		fdd_fn->formating	= fdd_formating_xxx;
		fdd_fn->isformating	= fdd_isformating_xxx;
		fdd_fn->fdcresult	= TRUE;
	}
	else if (memcmp(id, nfd_FileID_r1, sizeof(id)) == 0) {
		if (filesize < sizeof(NFD_FILE_HEAD1) ||
			(file_seek(fh, 0, FSEEK_SET) != 0) ||
			(file_read(fh, &fdd->inf.nfd.head.r1, sizeof(NFD_FILE_HEAD1)) != sizeof(NFD_FILE_HEAD1))) {
			file_close(fh);
			return(FAILURE);
		}
		headsize = nfd_load_le32(&fdd->inf.nfd.head.r1.dwHeadSize);
		//	dwAddInfoはr1仕様では予約(0固定)。未知の拡張形式は誤認防止のため拒否する
		if (headsize < sizeof(NFD_FILE_HEAD1) || headsize > filesize ||
			nfd_load_le32(&fdd->inf.nfd.head.r1.dwAddInfo) != 0 ||
			fdd->inf.nfd.head.r1.byHead < 1 || fdd->inf.nfd.head.r1.byHead > 2) {
			file_close(fh);
			return(FAILURE);
		}
		if (fdd->inf.nfd.head.r1.flProtect) {
			fdd->protect = TRUE;
		}
		fdd->inf.nfd.revision = 1;
		fdd->inf.xdf.headersize = headsize;
		ordercnt = 0;
		for (i = 0; i < NFD_TRKMAX1; i++) {
			tpos = nfd_load_le32(&fdd->inf.nfd.head.r1.dwTrackHead[i]);
			if (tpos) {
				order[ordercnt++] = i;
			}
		}
		//	データ部はトラック情報がヘッダに格納された物理順に並ぶ
		for (i = 0; i < ordercnt; i++) {
			best = i;
			bestpos = nfd_load_le32(&fdd->inf.nfd.head.r1.dwTrackHead[order[i]]);
			for (j = i + 1; j < ordercnt; j++) {
				tpos = nfd_load_le32(&fdd->inf.nfd.head.r1.dwTrackHead[order[j]]);
				if (tpos < bestpos) {
					best = j;
					bestpos = tpos;
				}
			}
			if (best != i) {
				k = order[i];
				order[i] = order[best];
				order[best] = k;
			}
		}
		data = headsize;
		maxtrk = 0;
		maxsec = 0;
		media_set = FALSE;
		prevend = sizeof(NFD_FILE_HEAD1);
		for (k = 0; k < ordercnt; k++) {
			i = order[k];
			tpos = nfd_load_le32(&fdd->inf.nfd.head.r1.dwTrackHead[i]);
			if (nfd1_read_track(fh, fdd, i, &trk_id)) {
				file_close(fh);
				return(FAILURE);
			}
			sectors = nfd_load_le16(&trk_id.wSector);
			diags = nfd_load_le16(&trk_id.wDiag);
			//	トラック情報の重複・オーバーラップを拒否する
			infosize = sizeof(trk_id) + ((UINT32)sectors + diags) * 16U;
			if (tpos < prevend || nfd_range_ok(tpos, infosize, headsize)) {
				file_close(fh);
				return(FAILURE);
			}
			prevend = tpos + infosize;
			fdd->inf.nfd.tptr[i] = data;
			infosize = sizeof(trk_id);
			for (j = 0; j < sectors; j++) {
				if ((file_seek(fh, tpos + infosize, FSEEK_SET) != (FILEPOS)(tpos + infosize)) ||
					file_read(fh, &sec_id, sizeof(sec_id)) != sizeof(sec_id) ||
					nfd_get_secsize(sec_id.N, &secsize)) {
					file_close(fh);
					return(FAILURE);
				}
				copies = (UINT32)sec_id.byRetry + 1U;
				if (nfd_range_ok(data, secsize * copies, filesize)) {
					file_close(fh);
					return(FAILURE);
				}
				if (sec_id.R && !fdd->inf.nfd.ptr[i][sec_id.R - 1]) {
					fdd->inf.nfd.ptr[i][sec_id.R - 1] = data;
				}
				data += secsize * copies;
				infosize += sizeof(sec_id);
				if (!media_set) {
					if (nfd_set_media(fdd, sec_id.byPDA, sec_id.N, sectors)) {
						file_close(fh);
						return(FAILURE);
					}
					media_set = TRUE;
				}
			}
			for (j = 0; j < diags; j++) {
				if ((file_seek(fh, tpos + infosize, FSEEK_SET) != (FILEPOS)(tpos + infosize)) ||
					file_read(fh, &dia_id, sizeof(dia_id)) != sizeof(dia_id)) {
					file_close(fh);
					return(FAILURE);
				}
				secsize = nfd_load_le32(&dia_id.dwDataLen);
				copies = (UINT32)dia_id.byRetry + 1U;
				if (!secsize || secsize > NFD_FDCBUFSIZE ||
					nfd_range_ok(data, secsize * copies, filesize)) {
					file_close(fh);
					return(FAILURE);
				}
				data += secsize * copies;
				infosize += sizeof(dia_id);
				//	通常セクタが無くても特殊読み込み情報のPDAからメディアを判定できる
				if (!media_set && dia_id.byPDA) {
					if (nfd_set_media(fdd, dia_id.byPDA, dia_id.N, 0)) {
						file_close(fh);
						return(FAILURE);
					}
					media_set = TRUE;
				}
			}
			fdd->inf.nfd.trksize[i] = data - fdd->inf.nfd.tptr[i];
			if (i + 1 > maxtrk) {
				maxtrk = i + 1;
			}
			if (sectors > maxsec) {
				maxsec = sectors;
			}
		}
		if (!media_set || !maxtrk) {
			file_close(fh);
			return(FAILURE);
		}
		fdd->inf.nfd.trackcount = (UINT16)maxtrk;
		fdd->inf.xdf.tracks = (UINT8)maxtrk;
		fdd->inf.xdf.sectors = (UINT8)((maxsec > 255) ? 255 : maxsec);

		fdd_fn->eject		= fdd_eject_xxx;
		fdd_fn->diskaccess	= fdd_diskaccess_common;
		fdd_fn->seek		= fdd_seek_nfd_common;
		fdd_fn->seeksector	= fdd_seeksector_nfd1;
		fdd_fn->readdiag	= fdd_readdiag_nfd1;
		fdd_fn->read		= fdd_read_nfd1;
		fdd_fn->write		= fdd_write_nfd1;
		fdd_fn->readid		= fdd_readid_nfd1;
		fdd_fn->writeid		= fdd_dummy_xxx;
		fdd_fn->formatinit	= fdd_dummy_xxx;
		fdd_fn->formating	= fdd_formating_xxx;
		fdd_fn->isformating	= fdd_isformating_xxx;
		fdd_fn->fdcresult	= TRUE;
	}
	else {
		file_close(fh);
		return(FAILURE);
	}
	file_close(fh);
	return(SUCCESS);
}

//	追加(kaiE)
BRESULT fdd_seeksector_nfd(FDDFILE fdd) {

	UINT	trk;
	UINT	i;

	if (!nfd_current_pda()) {
		fddlasterror = 0xe0;
		return(FAILURE);
	}
	if (nfd_track_index(fdd, fdc.treg[fdc.us], fdc.hd, &trk)) {
		fddlasterror = 0xe0;
		return(FAILURE);
	}
	for (i = 0; i < NFD_SECMAX; i++) {
		if (fdd->inf.nfd.head.r0.si[trk][i].C != 0xff) {
			return(SUCCESS);
		}
	}
	fddlasterror = 0xc0;
	return(FAILURE);
}

BRESULT fdd_read_nfd(FDDFILE fdd) {

	FILEH	hdl;
	UINT	trk;
	UINT	sec;
	UINT32	secsize;
	UINT32	seekp;
	NFD_SECT_ID	*id;

	fddlasterror = 0;
	if (fdd_seeksector_nfd(fdd)) {
		return(FAILURE);
	}
	if (nfd_track_index(fdd, fdc.treg[fdc.us], fdc.hd, &trk)) {
		fddlasterror = 0xe0;
		return(FAILURE);
	}
	sec = nfd0_find_sector(fdd, trk, TRUE);
	if (sec == NFD_SECMAX) {
		//	DAM/DDAM不一致でもデータは読み出し、保存されているリザルトを返す
		sec = nfd0_find_sector(fdd, trk, FALSE);
	}
	if (sec == NFD_SECMAX || nfd_get_secsize(fdd->inf.nfd.head.r0.si[trk][sec].N, &secsize)) {
		fddlasterror = 0xc0;
		return(FAILURE);
	}
	seekp = fdd->inf.nfd.ptr[trk][sec];
	hdl = file_open_rb(fdd->fname);
	if (hdl == FILEH_INVALID) {
		fddlasterror = 0xe0;
		return(FAILURE);
	}
	if ((file_seek(hdl, seekp, FSEEK_SET) != (FILEPOS)seekp) || file_read(hdl, fdc.buf, secsize) != secsize) {
		file_close(hdl);
		fddlasterror = 0xe0;
		return(FAILURE);
	}
	file_close(hdl);
	id = &fdd->inf.nfd.head.r0.si[trk][sec];
	fdc.stat[fdc.us] = id->byST0 | (id->byST1 << 8) | (id->byST2 << 16);
	fddlasterror = id->byStatus;
	fdc.bufcnt = secsize;
	return(SUCCESS);
}

BRESULT fdd_write_nfd(FDDFILE fdd) {

	FILEH	hdl;
	UINT	trk;
	UINT	sec;
	UINT32	secsize;
	UINT32	seekp;
	UINT32	idpos;
	NFD_SECT_ID	*id;
	NFD_SECT_ID	newid;

	fddlasterror = 0;
	if (fdd_seeksector_nfd(fdd)) {
		return(FAILURE);
	}
	if (fdd->protect) {
		fddlasterror = 0x70;
		return(FAILURE);
	}
	if (nfd_track_index(fdd, fdc.treg[fdc.us], fdc.hd, &trk)) {
		fddlasterror = 0xe0;
		return(FAILURE);
	}
	sec = nfd0_find_sector(fdd, trk, FALSE);
	if (sec == NFD_SECMAX || nfd_get_secsize(fdd->inf.nfd.head.r0.si[trk][sec].N, &secsize)) {
		fddlasterror = 0xc0;
		return(FAILURE);
	}
	seekp = fdd->inf.nfd.ptr[trk][sec];
	hdl = file_open(fdd->fname);
	if (hdl == FILEH_INVALID) {
		fddlasterror = 0xc0;
		return(FAILURE);
	}
	if ((file_seek(hdl, seekp, FSEEK_SET) != (FILEPOS)seekp) || file_write(hdl, fdc.buf, secsize) != secsize) {
		file_close(hdl);
		fddlasterror = 0xc0;
		return(FAILURE);
	}
	id = &fdd->inf.nfd.head.r0.si[trk][sec];
	newid = *id;
	newid.flDDAM = (nfd_current_cmd() == 0x09) ? 1 : 0;
	newid.byStatus = 0;
	newid.byST0 = (BYTE)((fdc.hd << 2) | fdc.us);
	newid.byST1 = 0;
	newid.byST2 = 0;
	idpos = (UINT32)((const UINT8 *)id - (const UINT8 *)&fdd->inf.nfd.head.r0);
	if ((file_seek(hdl, idpos, FSEEK_SET) != (FILEPOS)idpos) ||
		file_write(hdl, &newid, sizeof(newid)) != sizeof(newid)) {
		file_close(hdl);
		fddlasterror = 0xc0;
		return(FAILURE);
	}
	file_close(hdl);
	*id = newid;
	fdc.bufcnt = secsize;
	return(SUCCESS);
}

BRESULT fdd_readid_nfd(FDDFILE fdd) {

	UINT	trk;
	UINT	total;
	UINT	start;
	UINT	ordinal;
	UINT	step;
	UINT	i;
	UINT	seen;
	NFD_SECT_ID	*id;

	fddlasterror = 0;
	if (!nfd_current_pda()) {
		fddlasterror = 0xe0;
		return(FAILURE);
	}
	if (nfd_track_index(fdd, fdc.treg[fdc.us], fdc.hd, &trk)) {
		fddlasterror = 0xe0;
		return(FAILURE);
	}
	total = 0;
	for (i = 0; i < NFD_SECMAX; i++) {
		if (fdd->inf.nfd.head.r0.si[trk][i].C != 0xff) {
			total++;
		}
	}
	if (!total) {
		fddlasterror = 0xe0;
		return(FAILURE);
	}

	//	FM/MFM混在時は指定密度と一致する次のIDまで読み飛ばす
	start = fdc.crcn % total;
	for (step = 0; step < total; step++) {
		ordinal = (start + step) % total;
		seen = 0;
		id = NULL;
		for (i = 0; i < NFD_SECMAX; i++) {
			if (fdd->inf.nfd.head.r0.si[trk][i].C == 0xff) {
				continue;
			}
			if (seen++ == ordinal) {
				id = &fdd->inf.nfd.head.r0.si[trk][i];
				break;
			}
		}
		if (id && nfd_mfm_match(id->flMFM) && nfd_pda_match(fdd, id->byPDA)) {
			fdc.crcn = (UINT8)((ordinal + 1) % total);
			fdc.C = id->C;
			fdc.H = id->H;
			fdc.R = id->R;
			fdc.N = id->N;
			fdc.stat[fdc.us] = id->byST0 | (id->byST1 << 8) | (id->byST2 << 16);
			fddlasterror = id->byStatus;
			return(SUCCESS);
		}
	}
	fddlasterror = 0xe0;
	return(FAILURE);
}

BRESULT fdd_formatinit_nfd(FDDFILE fdd) {

	FILEH	hdl;
	UINT32	offset;
	UINT	trk;
	UINT	secsize;
	UINT	size;
	UINT	i;
	BOOL	tailresize;
	BYTE	fddtype;

	if (fdd->protect) {
		fddlasterror = 0x70;
		return(FAILURE);
	}
	if (fdc.sc == 0 || fdc.sc > NFD_SECMAX || nfd_get_secsize(fdc.N, &offset)) {
		fddlasterror = 0xc0;
		return(FAILURE);
	}
	secsize = (UINT)offset;
	if (secsize > NFD_FDCBUFSIZE / fdc.sc) {
		fddlasterror = 0xc0;
		return(FAILURE);
	}
	size = secsize * fdc.sc;
	if (nfd_track_index(fdd, fdc.treg[fdc.us], fdc.hd, &trk)) {
		fddlasterror = 0xc0;
		return(FAILURE);
	}

	//	r0はデータ部が連続配置されるため、後続データがあるトラックのサイズ変更は行わない
	tailresize = (fdd->inf.nfd.trksize[trk] != size);
	if (tailresize) {
		for (i = trk + 1; i < NFD_TRKMAX; i++) {
			if (fdd->inf.nfd.trksize[i]) {
				fddlasterror = 0xc0;
				return(FAILURE);
			}
		}
		tailresize = TRUE;
	}

	hdl = file_open(fdd->fname);
	if (hdl == FILEH_INVALID) {
		fddlasterror = 0xc0;
		return(FAILURE);
	}

	offset = fdd->inf.nfd.tptr[trk];
	if (offset < fdd->inf.xdf.headersize) {
		file_close(hdl);
		fddlasterror = 0xc0;
		return(FAILURE);
	}
	memset(fdc.buf, fdc.d, size);
	fddtype = nfd_current_pda();
	if (!fddtype) {
		fddtype = nfd_media_pda(fdd);
	}

	//	古いセクタIDを残すと次回マウント時のオフセット計算が狂うため、対象トラックを先に消去する
	for (i = 0; i < NFD_SECMAX; i++) {
		NFD_SECT_ID	*id = &fdd->inf.nfd.head.r0.si[trk][i];
		memset(id, 0, sizeof(*id));
		id->C = 0xff;
		fdd->inf.nfd.ptr[trk][i] = 0;
	}

	fdd->inf.nfd.tptr[trk] = offset;
	fdd->inf.nfd.trksize[trk] = size;
	for (i = 0; i < fdc.sc; i++) {
		NFD_SECT_ID	*id = &fdd->inf.nfd.head.r0.si[trk][i];
		id->C = fdc.treg[fdc.us];
		id->H = fdc.hd;
		id->R = (BYTE)(i + 1);
		id->N = fdc.N;
		id->flMFM = (fdc.mf != 0);
		id->flDDAM = 0;
		id->byStatus = 0;
		id->byST0 = (BYTE)(fdc.hd << 2);
		id->byST1 = 0;
		id->byST2 = 0;
		id->byPDA = fddtype;
		fdd->inf.nfd.ptr[trk][i] = offset + secsize * i;
	}

	//	書き込み失敗時にヘッダだけ更新されないよう、データを書いてからヘッダを更新する
	if ((file_seek(hdl, (FILEPOS)offset, FSEEK_SET) != (FILEPOS)offset) ||
		(file_write(hdl, fdc.buf, size) != size) ||
		(file_seek(hdl, 0, FSEEK_SET) != 0) ||
		(file_write(hdl, &fdd->inf.nfd.head.r0, NFD_HEADERSIZE) != NFD_HEADERSIZE)) {
		file_close(hdl);
		fddlasterror = 0xc0;
		return(FAILURE);
	}

	if (tailresize) {
		UINT32 newend = offset + size;
		if (file_setsize(hdl, newend) != 0) {
			file_close(hdl);
			fddlasterror = 0xc0;
			return(FAILURE);
		}
		//	末尾の空きトラックはすべてEOFを指す
		for (i = trk + 1; i < NFD_TRKMAX1; i++) {
			fdd->inf.nfd.tptr[i] = newend;
		}
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
	NFD_TRACK_ID1	trk_id;

	if (!nfd_current_pda()) {
		fddlasterror = 0xe0;
		return(FAILURE);
	}
	if (nfd_track_index(fdd, fdc.treg[fdc.us], fdc.hd, &trk) ||
		!nfd_load_le32(&fdd->inf.nfd.head.r1.dwTrackHead[trk])) {
		fddlasterror = 0xe0;
		return(FAILURE);
	}
	hdl = file_open_rb(fdd->fname);
	if (hdl == FILEH_INVALID) {
		fddlasterror = 0xe0;
		return(FAILURE);
	}
	if (nfd1_read_track(hdl, fdd, trk, &trk_id)) {
		file_close(hdl);
		fddlasterror = 0xe0;
		return(FAILURE);
	}
	file_close(hdl);
	if (!nfd_load_le16(&trk_id.wSector) && !nfd_load_le16(&trk_id.wDiag)) {
		fddlasterror = 0xc0;
		return(FAILURE);
	}
	return(SUCCESS);
}

BRESULT fdd_read_nfd1(FDDFILE fdd) {

	FILEH	hdl;
	UINT	trk;
	UINT32	seekp;
	UINT32	secsize;
	UINT32	copies;
	UINT32	select;
	UINT	secindex;
	UINT	diagindex;
	NFD_SECT_ID1	sec_id;
	NFD_DIAG_ID1	dia_id;

	fddlasterror = 0x00;
	if (fdd_seeksector_nfd1(fdd)) {
		return(FAILURE);
	}
	if (nfd_track_index(fdd, fdc.treg[fdc.us], fdc.hd, &trk)) {
		fddlasterror = 0xe0;
		return(FAILURE);
	}
	hdl = file_open_rb(fdd->fname);
	if (hdl == FILEH_INVALID) {
		fddlasterror = 0xe0;
		return(FAILURE);
	}

	//	r1では特殊読み込み情報を通常セクタ情報より優先する
	if (nfd1_find_diag(hdl, fdd, trk, &dia_id, &seekp, &diagindex) == SUCCESS) {
		secsize = nfd_load_le32(&dia_id.dwDataLen);
		copies = (UINT32)dia_id.byRetry + 1U;
		select = nfd_retry_select(&fdd->inf.nfd.diagretry[trk][diagindex & 0xff], copies);
		seekp += secsize * select;
		if ((file_seek(hdl, seekp, FSEEK_SET) != (FILEPOS)seekp) ||
			(file_read(hdl, fdc.buf, secsize) != secsize)) {
			file_close(hdl);
			fddlasterror = 0xe0;
			return(FAILURE);
		}
		fdc.stat[fdc.us] = dia_id.bySTS0 | (dia_id.bySTS1 << 8) | (dia_id.bySTS2 << 16);
		fddlasterror = dia_id.byStatus;
		fdc.bufcnt = secsize;
		file_close(hdl);
		return(SUCCESS);
	}
	//	READ DATA / READ DELETED DATAで要求されたデータマークを優先する
	if (nfd1_find_sector(hdl, fdd, trk, &sec_id, &seekp, &secindex, TRUE) &&
		nfd1_find_sector(hdl, fdd, trk, &sec_id, &seekp, &secindex, FALSE)) {
		file_close(hdl);
		fddlasterror = 0xc0;
		return(FAILURE);
	}
	if (nfd_get_secsize(sec_id.N, &secsize)) {
		file_close(hdl);
		fddlasterror = 0xc0;
		return(FAILURE);
	}
	copies = (UINT32)sec_id.byRetry + 1U;
	select = nfd_retry_select(&fdd->inf.nfd.retrycnt[trk][secindex & 0xff], copies);
	seekp += secsize * select;
	if ((file_seek(hdl, seekp, FSEEK_SET) != (FILEPOS)seekp) ||
		(file_read(hdl, fdc.buf, secsize) != secsize)) {
		file_close(hdl);
		fddlasterror = 0xe0;
		return(FAILURE);
	}
	file_close(hdl);
	fdc.stat[fdc.us] = sec_id.bySTS0 | (sec_id.bySTS1 << 8) | (sec_id.bySTS2 << 16);
	fddlasterror = sec_id.byStatus;
	fdc.bufcnt = secsize;
	return(SUCCESS);
}

BRESULT fdd_readdiag_nfd1(FDDFILE fdd) {

	FILEH	hdl;
	UINT	trk;
	UINT32	seekp;
	UINT32	size;
	UINT32	copies;
	UINT32	select;
	UINT	diagindex;
	NFD_DIAG_ID1	dia_id;

	fddlasterror = 0xc0;
	if (fdd_seeksector_nfd1(fdd)) {
		return(FAILURE);
	}
	if (nfd_track_index(fdd, fdc.treg[fdc.us], fdc.hd, &trk)) {
		fddlasterror = 0xe0;
		return(FAILURE);
	}
	hdl = file_open_rb(fdd->fname);
	if (hdl == FILEH_INVALID) {
		fddlasterror = 0xe0;
		return(FAILURE);
	}
	if (nfd1_find_diag(hdl, fdd, trk, &dia_id, &seekp, &diagindex)) {
		file_close(hdl);
		fddlasterror = 0xc0;
		return(FAILURE);
	}
	size = nfd_load_le32(&dia_id.dwDataLen);
	copies = (UINT32)dia_id.byRetry + 1U;
	select = nfd_retry_select(&fdd->inf.nfd.diagretry[trk][diagindex & 0xff], copies);
	seekp += size * select;
	if ((file_seek(hdl, seekp, FSEEK_SET) != (FILEPOS)seekp) ||
		(file_read(hdl, fdc.buf, size) != size)) {
		file_close(hdl);
		fddlasterror = 0xe0;
		return(FAILURE);
	}
	file_close(hdl);
	fdc.stat[fdc.us] = dia_id.bySTS0 | (dia_id.bySTS1 << 8) | (dia_id.bySTS2 << 16);
	fddlasterror = dia_id.byStatus;
	fdc.bufcnt = size;
	return(SUCCESS);
}

BRESULT fdd_write_nfd1(FDDFILE fdd) {

	FILEH	hdl;
	UINT	trk;
	UINT32	seekp;
	UINT32	idpos;
	UINT	secindex;
	UINT32	secsize;
	UINT32	copies;
	UINT32	i;
	UINT32	pos;
	NFD_SECT_ID1	sec_id;

	fddlasterror = 0x00;
	if (fdd_seeksector_nfd1(fdd)) {
		return(FAILURE);
	}
	if (fdd->protect) {
		fddlasterror = 0x70;
		return(FAILURE);
	}
	if (nfd_track_index(fdd, fdc.treg[fdc.us], fdc.hd, &trk)) {
		fddlasterror = 0xe0;
		return(FAILURE);
	}
	hdl = file_open(fdd->fname);
	if (hdl == FILEH_INVALID) {
		fddlasterror = 0xe0;
		return(FAILURE);
	}
	if (nfd1_find_sector(hdl, fdd, trk, &sec_id, &seekp, &secindex, FALSE) ||
		nfd_get_secsize(sec_id.N, &secsize)) {
		file_close(hdl);
		fddlasterror = 0xc0;
		return(FAILURE);
	}

	//	書き込み後に古いRetryDataが再出現しないよう全コピーを更新する
	copies = (UINT32)sec_id.byRetry + 1U;
	for (i = 0; i < copies; i++) {
		pos = seekp + secsize * i;
		if ((file_seek(hdl, pos, FSEEK_SET) != (FILEPOS)pos) ||
			(file_write(hdl, fdc.buf, secsize) != secsize)) {
			file_close(hdl);
			fddlasterror = 0xc0;
			return(FAILURE);
		}
	}

	//	CHRNは維持し、データマークと正常終了ステータスを更新する
	idpos = nfd_load_le32(&fdd->inf.nfd.head.r1.dwTrackHead[trk]) +
		sizeof(NFD_TRACK_ID1) + (UINT32)secindex * sizeof(NFD_SECT_ID1);
	sec_id.flDDAM = (nfd_current_cmd() == 0x09) ? 1 : 0;
	sec_id.byStatus = 0;
	sec_id.bySTS0 = (BYTE)((fdc.hd << 2) | fdc.us);
	sec_id.bySTS1 = 0;
	sec_id.bySTS2 = 0;
	if ((file_seek(hdl, idpos, FSEEK_SET) != (FILEPOS)idpos) ||
		(file_write(hdl, &sec_id, sizeof(sec_id)) != sizeof(sec_id))) {
		file_close(hdl);
		fddlasterror = 0xc0;
		return(FAILURE);
	}
	file_close(hdl);
	fdd->inf.nfd.retrycnt[trk][secindex & 0xff] = 0;
	fdc.bufcnt = secsize;
	return(SUCCESS);
}

BRESULT fdd_readid_nfd1(FDDFILE fdd) {

	FILEH	hdl;
	UINT	trk;
	UINT	count;
	UINT	start;
	UINT	index;
	UINT	step;
	UINT32	pos;
	NFD_TRACK_ID1	trk_id;
	NFD_SECT_ID1	sec_id;

	fddlasterror = 0x00;
	if (!nfd_current_pda()) {
		fddlasterror = 0xe0;
		return(FAILURE);
	}
	if (nfd_track_index(fdd, fdc.treg[fdc.us], fdc.hd, &trk) ||
		!nfd_load_le32(&fdd->inf.nfd.head.r1.dwTrackHead[trk])) {
		fddlasterror = 0xe0;
		return(FAILURE);
	}
	hdl = file_open_rb(fdd->fname);
	if (hdl == FILEH_INVALID) {
		fddlasterror = 0xe0;
		return(FAILURE);
	}
	if (nfd1_read_track(hdl, fdd, trk, &trk_id)) {
		file_close(hdl);
		fddlasterror = 0xe0;
		return(FAILURE);
	}
	count = nfd_load_le16(&trk_id.wSector);
	if (!count) {
		file_close(hdl);
		fddlasterror = 0xe0;
		return(FAILURE);
	}
	start = fdc.crcn % count;
	for (step = 0; step < count; step++) {
		index = (start + step) % count;
		pos = nfd_load_le32(&fdd->inf.nfd.head.r1.dwTrackHead[trk]) +
			sizeof(trk_id) + (UINT32)index * sizeof(sec_id);
		if ((file_seek(hdl, pos, FSEEK_SET) != (FILEPOS)pos) ||
			(file_read(hdl, &sec_id, sizeof(sec_id)) != sizeof(sec_id))) {
			file_close(hdl);
			fddlasterror = 0xe0;
			return(FAILURE);
		}
		if (nfd_mfm_match(sec_id.flMFM) && nfd_pda_match(fdd, sec_id.byPDA)) {
			file_close(hdl);
			fdc.crcn = (UINT8)((index + 1) % count);
			fdc.C = sec_id.C;
			fdc.H = sec_id.H;
			fdc.R = sec_id.R;
			fdc.N = sec_id.N;
			fdc.stat[fdc.us] = sec_id.bySTS0 | (sec_id.bySTS1 << 8) | (sec_id.bySTS2 << 16);
			fddlasterror = sec_id.byStatus;
			return(SUCCESS);
		}
	}
	file_close(hdl);
	fddlasterror = 0xe0;
	return(FAILURE);
}

//

#endif
