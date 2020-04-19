#include	"compiler.h"
#include	"dosio.h"
#include	"pccore.h"
#include	"iocore.h"

#include	"diskimage/fddfile.h"
#include	"diskimage/fd/fdd_xdf.h"

#ifdef SUPPORT_KAI_IMAGES

//	BKDSK(DD6)、BKDSK(DDB)判定用
//static const OEMCHAR str_dd6[] = OEMTEXT("dd6");	//	未使用なのでコメント(kai9)
static const OEMCHAR str_ddb[] = OEMTEXT("ddb");
//

static const _XDFINFO supportxdf[] = {
#if 0
			// 256
			{0, 154, 26, 1, DISKTYPE_2HD, 0},
			// 512
			{0, 154, 15, 2, DISKTYPE_2HD, 0},
#endif
#if 1
			// 512
			{0, 160, 15, 2, DISKTYPE_2HD, 0},	//	BKDSK(HD5)	MS-DOS 1.21M(2HC)
#endif
			// 1024
			{0, 154,  8, 3, DISKTYPE_2HD, 0},	//	XDF他2HD、BIN、FLP等
												//	BKDSK(HDM)	MS-DOS 1.25M(2HD)
			// 1.44MB
			{0, 160, 18, 2, DISKTYPE_2HD, 1},	//	BKDSK(HD4)	MS-DOS 1.44M(2HD)

			//	追加でいくつかのBKDSK形式に対応
			{0, 154, 26, 1, DISKTYPE_2HD, 0},	//	BKDSK(H01)	2HD:256byte*26sec (0-154)track
			{0, 154,  9, 3, DISKTYPE_2HD, 0},	//	BKDSK(HD9)	MS-DOS 2HD(9sec)
			{0, 160,  8, 2, DISKTYPE_2DD, 0},	//	BKDSK(DD6)	MS-DOS  640K(2DD)
			{0, 160,  9, 2, DISKTYPE_2DD, 0},	//	BKDSK(DD9)	MS-DOS  720K(2DD)
			//	track 0とそれ以外でセクタサイズが違うため未対応
//			{0, 154, 26, 1, DISKTYPE_2HD, 0},	//	BKDSK(HDB)	BASIC 2HD
			//
			{0, 160, 16, 1, DISKTYPE_2DD, 0},	//	BKDSK(DDB)	BASIC 2DD
			//
			//	ヘッダサイズを指定し、判定時に考慮するようにすれば
			//	DIPやDCP／DCU(全トラック格納イメージ)とかもいける
			{256, 154,  8, 3, DISKTYPE_2HD, 0},	//	DIP			2HD-8セクタ(1.25MB)
												//	FIM			2HD-8セクタ(1.25MB)
			{256, 154, 26, 1, DISKTYPE_2HD, 0},	//	FIM			2HD-26セクタ
#if 0	//	fdd_dcp.cへ移動(kai9)
			{162, 154,  8, 3, DISKTYPE_2HD, 0},	//	DCP／DCU	01h	2HD- 8セクタ(1.25MB)
			{162, 160, 15, 2, DISKTYPE_2HD, 0},	//	DCP／DCU	02h	2HD-15セクタ(1.21MB)
			{162, 160, 18, 2, DISKTYPE_2HD, 1},	//	DCP／DCU	03h	2HQ-18セクタ(1.44MB)
			{162, 160,  8, 2, DISKTYPE_2DD, 0},	//	DCP／DCU	04h	2DD- 8セクタ( 640KB)
			{162, 160,  9, 2, DISKTYPE_2DD, 0},	//	DCP／DCU	05h	2DD- 9セクタ( 720KB)
			{162, 154,  9, 3, DISKTYPE_2HD, 0},	//	DCP／DCU	08h	2HD- 9セクタ(1.44MB)
//			{162, 154, 26, 1, DISKTYPE_2HD, 0},	//	DCP／DCU	11h	BASIC-2HD
			{162, 160, 16, 1, DISKTYPE_2DD, 0},	//	DCP／DCU	19h	BASIC-2DD
			{162, 154, 26, 1, DISKTYPE_2HD, 0},	//	DCP／DCU	21h	2HD-26セクタ
#endif
			//
};

//	FDIヘッダ
typedef struct {
	UINT8	dummy[4];
	UINT8	fddtype[4];
	UINT8	headersize[4];
	UINT8	fddsize[4];
	UINT8	sectorsize[4];
	UINT8	sectors[4];
	UINT8	surfaces[4];
	UINT8	cylinders[4];
} FDIHDR;

//BRESULT fddxdf_set(FDDFILE fdd, const OEMCHAR *fname, int ro) {
BRESULT fdd_set_xdf(FDDFILE fdd, FDDFUNC fdd_fn, const OEMCHAR *fname, int ro) {

const _XDFINFO	*xdf;
	short		attr;
	FILEH		fh;
	UINT32		fdsize;
	UINT		size;
const OEMCHAR	*p;			//	BKDSK(DD6) or BKDSK(DDB)判定用

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
	file_close(fh);

	p = file_getext(fname);	//	BKDSK(DD6) or BKDSK(DDB)判定用

	xdf = supportxdf;
	while(xdf < (supportxdf + NELEMENTS(supportxdf))) {
		size = xdf->tracks;
		size *= xdf->sectors;
		size <<= (7 + xdf->n);
		//	ヘッダサイズを考慮するように
		size += xdf->headersize;
		//
		if (size == fdsize) {
			//	BKDSK(DD6)とBKDSK(DDB)が同一サイズのため、拡張子で判定
			if (!milstr_cmp(p, str_ddb) && xdf->sectors == 8) {
				xdf++;
				continue;
			}
			//
			fdd->type = DISKTYPE_BETA;
			fdd->protect = ((attr & 1) || (ro))?TRUE:FALSE;
			fdd->inf.xdf = *xdf;
			//	処理関数群を登録(kai9)
			fdd_fn->eject		= fdd_eject_xxx;
//			fdd_fn->diskaccess	= fdd_diskaccess_xdf;
//			fdd_fn->seek		= fdd_seek_xdf;
//			fdd_fn->seeksector	= fdd_seeksector_xdf;
			fdd_fn->diskaccess	= fdd_diskaccess_common;
			fdd_fn->seek		= fdd_seek_common;
			fdd_fn->seeksector	= fdd_seeksector_common;
			fdd_fn->read		= fdd_read_xdf;
			fdd_fn->write		= fdd_write_xdf;
//			fdd_fn->readid		= fdd_readid_xdf;
			fdd_fn->readid		= fdd_readid_common;
			fdd_fn->writeid		= fdd_dummy_xxx;
			fdd_fn->formatinit	= fdd_formatinit_xdf;	/* 170107 to support format command */
			fdd_fn->formating	= fdd_formating_xxx;
			fdd_fn->isformating	= fdd_isformating_xxx;
			//
			return(SUCCESS);
		}
		xdf++;
	}
	return(FAILURE);
}

// こっそり対応したりして
//BRESULT fddxdf_setfdi(FDDFILE fdd, const OEMCHAR *fname, int ro) {
BRESULT fdd_set_fdi(FDDFILE fdd, FDDFUNC fdd_fn, const OEMCHAR *fname, int ro) {

	short	attr;
	FILEH	fh;
	UINT32	fdsize;
	UINT	r;
	FDIHDR	fdi;
	UINT32	fddtype;
	UINT32	headersize;
	UINT32	size;
	UINT32	sectors;
	UINT32	surfaces;
	UINT32	cylinders;
	UINT8	n;
	UINT8	disktype;
	UINT8	rpm;

	attr = file_attr(fname);
	if (attr & 0x18) {
		return(FAILURE);
	}
	fdsize = 0;
	r = 0;
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
	if (fh != FILEH_INVALID) {
		fdsize = (UINT32)file_getsize(fh);
		r = file_read(fh, &fdi, sizeof(fdi));
		file_close(fh);
	}
	if (r != sizeof(fdi)) {
		return(FAILURE);
	}
	fddtype = LOADINTELDWORD(fdi.fddtype);
	headersize = LOADINTELDWORD(fdi.headersize);
	size = LOADINTELDWORD(fdi.sectorsize);
	sectors = LOADINTELDWORD(fdi.sectors);
	surfaces = LOADINTELDWORD(fdi.surfaces);
	cylinders = LOADINTELDWORD(fdi.cylinders);
	if (((size & (size - 1)) != 0) || (!(size & 0x7f80)) ||
		(sectors == 0) || (sectors >= 256) ||
		(surfaces != 2) ||
		(cylinders == 0) || (cylinders >= 128)) {
		return(FAILURE);
	}
	if (fdsize != (headersize + (size * sectors * surfaces * cylinders))) {
		return(FAILURE);
	}
	size >>= 8;
	n = 0;
	while(size) {
		size >>= 1;
		n++;
	}
	disktype = DISKTYPE_2HD;
	rpm = 0;
	switch(fddtype & 0xf0) {
		case 0x10:				// 1MB/640KB - 2DD
		case 0x70:				// 640KB - 2DD
		case 0xf0:
			disktype = DISKTYPE_2DD;
			break;

		case 0x30:				// 1.44MB - 2HD
		case 0xb0:
			rpm = 1;
			break;

		case 0x50:				// 320KB - 2D
		case 0xd0:				// 
			disktype = DISKTYPE_2D;
			break;

		case 0x90:				// 2HD
			break;

		default:
			return(FAILURE);
	}
	fdd->type = DISKTYPE_BETA;
	fdd->protect = ((attr & 1) || (ro))?TRUE:FALSE;
	fdd->inf.xdf.headersize = headersize;
	fdd->inf.xdf.tracks = (UINT8)(cylinders * 2);
	fdd->inf.xdf.sectors = (UINT8)sectors;
	fdd->inf.xdf.n = n;
	fdd->inf.xdf.disktype = disktype;
	fdd->inf.xdf.rpm = rpm;
	//	処理関数群を登録(kai9)
	fdd_fn->eject		= fdd_eject_xxx;
	fdd_fn->diskaccess	= fdd_diskaccess_common;
	fdd_fn->seek		= fdd_seek_common;
	fdd_fn->seeksector	= fdd_seeksector_common;
	fdd_fn->read		= fdd_read_xdf;
	fdd_fn->write		= fdd_write_xdf;
	fdd_fn->readid		= fdd_readid_common;
	fdd_fn->writeid		= fdd_dummy_xxx;
	fdd_fn->formatinit	= fdd_formatinit_xdf;	/* 170107 modified to work on Windows 9x/2000 */
	fdd_fn->formating	= fdd_formating_xxx;
	fdd_fn->isformating	= fdd_isformating_xxx;
	//
	return(SUCCESS);
}

#if 0	//	共通関数化したので廃止(fddfile.cへ)(kai9)
//BRESULT fddxdf_eject(FDDFILE fdd) {
BRESULT fdd_eject_xdf(FDDFILE fdd) {

//	fdd->fname[0] = '\0';
//	fdd->type = DISKTYPE_NOTREADY;
	return(SUCCESS);
}

//BRESULT fddxdf_diskaccess(FDDFILE fdd) {
BRESULT fdd_diskaccess_xdf(FDDFILE fdd) {

	if ((fdd->type != DISKTYPE_BETA) ||
		(CTRL_FDMEDIA != fdd->inf.xdf.disktype)) {
		return(FAILURE);
	}
	return(SUCCESS);
}

//BRESULT fddxdf_seek(FDDFILE fdd) {
BRESULT fdd_seek_xdf(FDDFILE fdd) {

	if ((fdd->type != DISKTYPE_BETA) ||
		(CTRL_FDMEDIA != fdd->inf.xdf.disktype) ||
		(fdc.rpm[fdc.us] != fdd->inf.xdf.rpm) ||
		(fdc.ncn >= (fdd->inf.xdf.tracks >> 1))) {
		return(FAILURE);
	}
	return(SUCCESS);
}

//BRESULT fddxdf_seeksector(FDDFILE fdd) {
BRESULT fdd_seeksector_xdf(FDDFILE fdd) {

	if ((fdd->type != DISKTYPE_BETA) ||
		(CTRL_FDMEDIA != fdd->inf.xdf.disktype) ||
		(fdc.rpm[fdc.us] != fdd->inf.xdf.rpm) ||
		(fdc.treg[fdc.us] >= (fdd->inf.xdf.tracks >> 1))) {
		fddlasterror = 0xe0;
		return(FAILURE);
	}
	if ((!fdc.R) || (fdc.R > fdd->inf.xdf.sectors)) {
		fddlasterror = 0xc0;
		return(FAILURE);
	}
	if ((fdc.mf != 0xff) && (fdc.mf != 0x40)) {
		fddlasterror = 0xc0;
		return(FAILURE);
	}
	return(SUCCESS);
}
#endif

//BRESULT fddxdf_read(FDDFILE fdd) {
BRESULT fdd_read_xdf(FDDFILE fdd) {

	FILEH	hdl;
	long	seekp;
	UINT	secsize;

	fddlasterror = 0x00;
//	if (fddxdf_seeksector(fdd)) {
	if (fdd_seeksector_common(fdd)) {
		return(FAILURE);
	}
	if (fdc.N != fdd->inf.xdf.n) {
		fddlasterror = 0xc0;
		return(FAILURE);
	}
//	/* 170101 ST modified to work on Windows 9x/2000 form ... */
//	if (fdc.eot < fdd->inf.xdf.sectors) {
//		fddlasterror = 0xc0;
//		return(FAILURE);
//	}
//	/* 170101 ST modified to work on Windows 9x/2000 ... to */

	seekp = (fdc.treg[fdc.us] << 1) + fdc.hd;
	seekp *= fdd->inf.xdf.sectors;
	seekp += fdc.R - 1;
	seekp <<= (7 + fdd->inf.xdf.n);
	seekp += fdd->inf.xdf.headersize;
	secsize = 128 << fdd->inf.xdf.n;

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
	fdc.bufcnt = secsize;
	fddlasterror = 0x00;
	return(SUCCESS);
}

//BRESULT fddxdf_write(FDDFILE fdd) {
BRESULT fdd_write_xdf(FDDFILE fdd) {

	FILEH	hdl;
	long	seekp;
	UINT	secsize;

	fddlasterror = 0x00;
//	if (fddxdf_seeksector(fdd)) {
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

	seekp = (fdc.treg[fdc.us] << 1) + fdc.hd;
	seekp *= fdd->inf.xdf.sectors;
	seekp += fdc.R - 1;
	seekp <<= (7 + fdd->inf.xdf.n);
	seekp += fdd->inf.xdf.headersize;
	secsize = 128 << fdd->inf.xdf.n;

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
	fdc.bufcnt = secsize;
	fddlasterror = 0x00;
	return(SUCCESS);
}

/* 170107 modified to work on Windows 9x/2000 form ... */
/* 実装補足 先頭からのオフセット値は単純にトラック数ｘ1トラック当たりのセクタ数ｘセクタサイズ
   で行っているため、トラックごとに異なるセクタ構造を持つフォーマットには対応できません 
   元々ファイルのデータ構造的にサポートできません */
BRESULT fdd_formatinit_xdf(FDDFILE fdd) {
	FILEH	hdl;
	FDIHDR	fdi;
	UINT32	fddtype;
	UINT32	cylinders;
	long	seekp;
	UINT    size;

	if (fdd->protect) {
		fddlasterror = 0x70;
		return(FAILURE);
	}

	hdl = file_open(fdd->fname);
	if (hdl == FILEH_INVALID) {
		fddlasterror = 0xc0;
		return(FAILURE);
	}

	size = (128 << fdc.N) * fdc.sc;
	seekp = (fdc.treg[fdc.us] << 1) + fdc.hd;
	seekp *= size;
	seekp += fdd->inf.xdf.headersize;
	memset(fdc.buf, fdc.d, size);

	if ((file_seek(hdl, seekp, FSEEK_SET) != seekp) ||
		(file_write(hdl, fdc.buf, size) != size)) {
		file_close(hdl);
		fddlasterror = 0xc0;
		return(FAILURE);
	}

	if (seekp == fdd->inf.xdf.headersize) {
		
		fdd->inf.xdf.disktype = DISKTYPE_2HD;
		cylinders = 77;
		switch(fdc.N) {
			case 1:		// BASIC (sector size = 256)
				fddtype = 0x10;
				break;
			case 2:		// 1.44M/1.21M/2DD
				fddtype = 0x30;
				if (fdc.sc < 10) {
					fddtype = 0x70;
					fdd->inf.xdf.disktype = DISKTYPE_2DD;
				}
				else if (fdc.sc < 18) {
					fddtype = 0x90;
				}
				cylinders = 80;
				break;
			default:	// 1.25M
				fddtype = 0x90;
				break;
		}

		fdd->inf.xdf.tracks = (UINT8)(cylinders * 2);
		fdd->inf.xdf.sectors = fdc.sc;
		fdd->inf.xdf.n = fdc.N;
		fdd->inf.xdf.rpm = fdc.rpm[fdc.us];

		if(fdd->inf.xdf.headersize) {			/* for FDI */
			if ((file_seek(hdl, 0, FSEEK_SET) == 0) &&
				(file_read(hdl, &fdi, sizeof(fdi)) == sizeof(fdi))) {
				if (fdd->inf.xdf.headersize == LOADINTELDWORD(fdi.headersize)) {
					STOREINTELDWORD(fdi.fddtype, fddtype);
					STOREINTELDWORD(fdi.fddsize, size * cylinders * 2);
					STOREINTELDWORD(fdi.sectorsize, 128 << fdc.N);
					STOREINTELDWORD(fdi.sectors, fdc.sc);
					STOREINTELDWORD(fdi.surfaces, 2);
					STOREINTELDWORD(fdi.cylinders, cylinders);
					file_seek(hdl, 0, FSEEK_SET);
					file_write(hdl, &fdi, sizeof(fdi));
				}
			}
		}
		// trim media image
		seekp = fdd->inf.xdf.tracks * size + fdd->inf.xdf.headersize;
		file_seek(hdl, seekp, FSEEK_SET);
		file_write(hdl, fdc.buf, 0);
	}

	file_close(hdl);
	fddlasterror = 0x00;
	return(SUCCESS);
}
/* 170107 modified to work on Windows 9x/2000 ... to */

#if 0	//	共通関数化したので廃止(fddfile.cへ)(kai9)
//BRESULT fddxdf_readid(FDDFILE fdd) {
BRESULT fdd_readid_xdf(FDDFILE fdd) {

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
	fdc.N = fdd->inf.xdf.n;
	return(SUCCESS);
}
#endif

#else


static const _XDFINFO supportxdf[] = {
#if 0
			// 256
			{0, 154, 26, 1, DISKTYPE_2HD, 0},
			// 512
			{0, 154, 15, 2, DISKTYPE_2HD, 0},
#endif
#if 1
			// 512
			{0, 160, 15, 2, DISKTYPE_2HD, 0},
#endif
			// 1024
			{0, 154,  8, 3, DISKTYPE_2HD, 0},
			// 1.44MB
			{0, 160, 18, 2, DISKTYPE_2HD, 1},
};

typedef struct {
	UINT8	dummy[4];
	UINT8	fddtype[4];
	UINT8	headersize[4];
	UINT8	fddsize[4];
	UINT8	sectorsize[4];
	UINT8	sectors[4];
	UINT8	surfaces[4];
	UINT8	cylinders[4];
} FDIHDR;


BRESULT fddxdf_set(FDDFILE fdd, const OEMCHAR *fname, int ro) {

const _XDFINFO	*xdf;
	short		attr;
	FILEH		fh;
	UINT32		fdsize;
	UINT		size;

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
	fdsize = file_getsize(fh);
	file_close(fh);

	xdf = supportxdf;
	while(xdf < (supportxdf + NELEMENTS(supportxdf))) {
		size = xdf->tracks;
		size *= xdf->sectors;
		size <<= (7 + xdf->n);
		if (size == fdsize) {
			fdd->type = DISKTYPE_BETA;
			fdd->protect = ((attr & 1) || (ro))?TRUE:FALSE;
			fdd->inf.xdf = *xdf;
			return(SUCCESS);
		}
		xdf++;
	}
	return(FAILURE);
}

// こっそり対応したりして
BRESULT fddxdf_setfdi(FDDFILE fdd, const OEMCHAR *fname, int ro) {

	short	attr;
	FILEH	fh;
	UINT32	fdsize;
	UINT	r;
	FDIHDR	fdi;
	UINT32	fddtype;
	UINT32	headersize;
	UINT32	size;
	UINT32	sectors;
	UINT32	surfaces;
	UINT32	cylinders;
	UINT8	n;
	UINT8	disktype;
	UINT8	rpm;

	attr = file_attr(fname);
	if (attr & 0x18) {
		return(FAILURE);
	}
	fdsize = 0;
	r = 0;
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
	if (fh != FILEH_INVALID) {
		fdsize = file_getsize(fh);
		r = file_read(fh, &fdi, sizeof(fdi));
		file_close(fh);
	}
	if (r != sizeof(fdi)) {
		return(FAILURE);
	}
	fddtype = LOADINTELDWORD(fdi.fddtype);
	headersize = LOADINTELDWORD(fdi.headersize);
	size = LOADINTELDWORD(fdi.sectorsize);
	sectors = LOADINTELDWORD(fdi.sectors);
	surfaces = LOADINTELDWORD(fdi.surfaces);
	cylinders = LOADINTELDWORD(fdi.cylinders);
	if (((size & (size - 1)) != 0) || (!(size & 0x7f80)) ||
		(sectors == 0) || (sectors >= 256) ||
		(surfaces != 2) ||
		(cylinders == 0) || (cylinders >= 128)) {
		return(FAILURE);
	}
	if (fdsize != (headersize + (size * sectors * surfaces * cylinders))) {
		return(FAILURE);
	}
	size >>= 8;
	n = 0;
	while(size) {
		size >>= 1;
		n++;
	}
	disktype = DISKTYPE_2HD;
	rpm = 0;
	switch(fddtype & 0xf0) {
		case 0x10:				// 1MB/640KB - 2DD
		case 0x70:				// 640KB - 2DD
		case 0xf0:
			disktype = DISKTYPE_2DD;
			break;

		case 0x30:				// 1.44MB - 2HD
		case 0xb0:
			rpm = 1;
			break;

		case 0x50:				// 320KB - 2D
		case 0xd0:				// 
			disktype = DISKTYPE_2D;
			break;

		case 0x90:				// 2HD
			break;

		default:
			return(FAILURE);
	}
	fdd->type = DISKTYPE_BETA;
	fdd->protect = ((attr & 1) || (ro))?TRUE:FALSE;
	fdd->inf.xdf.headersize = headersize;
	fdd->inf.xdf.tracks = (UINT8)(cylinders * 2);
	fdd->inf.xdf.sectors = (UINT8)sectors;
	fdd->inf.xdf.n = n;
	fdd->inf.xdf.disktype = disktype;
	fdd->inf.xdf.rpm = rpm;
	return(SUCCESS);
}

BRESULT fddxdf_eject(FDDFILE fdd) {

	fdd->fname[0] = '\0';
	fdd->type = DISKTYPE_NOTREADY;
	return(SUCCESS);
}


BRESULT fddxdf_diskaccess(FDDFILE fdd) {

	if ((fdd->type != DISKTYPE_BETA) ||
		(CTRL_FDMEDIA != fdd->inf.xdf.disktype)) {
		return(FAILURE);
	}
	return(SUCCESS);
}

BRESULT fddxdf_seek(FDDFILE fdd) {

	if ((fdd->type != DISKTYPE_BETA) ||
		(CTRL_FDMEDIA != fdd->inf.xdf.disktype) ||
		(fdc.rpm[fdc.us] != fdd->inf.xdf.rpm) ||
		(fdc.ncn >= (fdd->inf.xdf.tracks >> 1))) {
		return(FAILURE);
	}
	return(SUCCESS);
}

BRESULT fddxdf_seeksector(FDDFILE fdd) {

	if ((fdd->type != DISKTYPE_BETA) ||
		(CTRL_FDMEDIA != fdd->inf.xdf.disktype) ||
		(fdc.rpm[fdc.us] != fdd->inf.xdf.rpm) ||
		(fdc.treg[fdc.us] >= (fdd->inf.xdf.tracks >> 1))) {
		fddlasterror = 0xe0;
		return(FAILURE);
	}
	if ((!fdc.R) || (fdc.R > fdd->inf.xdf.sectors)) {
		fddlasterror = 0xc0;
		return(FAILURE);
	}
	if ((fdc.mf != 0xff) && (fdc.mf != 0x40)) {
		fddlasterror = 0xc0;
		return(FAILURE);
	}
	return(SUCCESS);
}

BRESULT fddxdf_read(FDDFILE fdd) {

	FILEH	hdl;
	long	seekp;
	UINT	secsize;

	fddlasterror = 0x00;
	if (fddxdf_seeksector(fdd)) {
		return(FAILURE);
	}
	if (fdc.N != fdd->inf.xdf.n) {
		fddlasterror = 0xc0;
		return(FAILURE);
	}

	seekp = (fdc.treg[fdc.us] << 1) + fdc.hd;
	seekp *= fdd->inf.xdf.sectors;
	seekp += fdc.R - 1;
	seekp <<= (7 + fdd->inf.xdf.n);
	seekp += fdd->inf.xdf.headersize;
	secsize = 128 << fdd->inf.xdf.n;

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
	fdc.bufcnt = secsize;
	fddlasterror = 0x00;
	return(SUCCESS);
}

BRESULT fddxdf_write(FDDFILE fdd) {

	FILEH	hdl;
	long	seekp;
	UINT	secsize;

	fddlasterror = 0x00;
	if (fddxdf_seeksector(fdd)) {
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

	seekp = (fdc.treg[fdc.us] << 1) + fdc.hd;
	seekp *= fdd->inf.xdf.sectors;
	seekp += fdc.R - 1;
	seekp <<= (7 + fdd->inf.xdf.n);
	seekp += fdd->inf.xdf.headersize;
	secsize = 128 << fdd->inf.xdf.n;

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
	fdc.bufcnt = secsize;
	fddlasterror = 0x00;
	return(SUCCESS);
}

BRESULT fddxdf_readid(FDDFILE fdd) {

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
	fdc.N = fdd->inf.xdf.n;
	return(SUCCESS);
}

#endif
