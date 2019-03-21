#include	"compiler.h"
#include	"strres.h"
#include	"dosio.h"
#include	"sysmng.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"fddfile.h"

#ifdef SUPPORT_KAI_IMAGES

#include	"diskimage/img_common.h"	//	追加(Kai1)
#include	"diskimage/img_strres.h"	//	追加(Kai1)
#include	"diskimage/fd/fdd_xdf.h"	//	追加(Kai1)
#include	"diskimage/fd/fdd_d88.h"	//	追加(Kai1)
#include	"diskimage/fd/fdd_dcp.h"	//	追加(Kai1)
#include	"diskimage/fd/fdd_bkdsk.h"	//	追加(Kai1)
#include	"diskimage/fd/fdd_nfd.h"	//	追加(Kai1)
#include	"diskimage/fd/fdd_vfdd.h"	//	追加(Kai1)


	_FDDFILE	fddfile[MAX_FDDFILE];
	_FDDFUNC	fddfunc[MAX_FDDFILE];	//	追加(Kai1)
	UINT8		fddlasterror;

//	追加(kai9)
void fddfunc_init(FDDFUNC fdd_fn) {

	fdd_fn->eject		= fdd_eject_xxx;
	fdd_fn->diskaccess	= fdd_dummy_xxx;
	fdd_fn->seek		= fdd_dummy_xxx;
	fdd_fn->seeksector	= fdd_dummy_xxx;
	fdd_fn->read		= fdd_dummy_xxx;
	fdd_fn->write		= fdd_dummy_xxx;
	fdd_fn->readid		= fdd_dummy_xxx;
	fdd_fn->writeid		= fdd_dummy_xxx;
	fdd_fn->formatinit	= fdd_dummy_xxx;
	fdd_fn->formating	= fdd_formating_xxx;
	fdd_fn->isformating	= fdd_isformating_xxx;
	fdd_fn->fdcresult	= FALSE;	//	追加(Kai1)
}
//

// ----

void fddfile_initialize(void) {

	UINT	i;

	//	とりあえずダミーで埋めておく(Kai1)
	for (i = 0; i < MAX_FDDFILE; i++) {
		fddfunc_init(&fddfunc[i]);
	}
	//
	ZeroMemory(fddfile, sizeof(fddfile));
}

void fddfile_reset2dmode(void) { 			// ver0.29
#if 0
	int		i;

	for (i=0; i<4; i++) {
		fddfile[i].mode2d = 0;
	}
#endif
}

OEMCHAR *fdd_diskname(REG8 drv) {

	if (drv >= MAX_FDDFILE) {
		return(NULL);
	}
	return(fddfile[drv].fname);
}

OEMCHAR *fdd_getfileex(REG8 drv, UINT *ftype, int *ro) {

	FDDFILE	fdd;

	if (drv >= MAX_FDDFILE) {
		return((OEMCHAR *)str_null);
	}
	fdd = fddfile + drv;
	if (ftype) {
		*ftype = fdd->ftype;
	}
	if (ro) {
		*ro = fdd->ro;
	}
	return(fdd->fname);
}

BOOL fdd_diskready(REG8 drv) {

	if ((drv >= MAX_FDDFILE) || (!fddfile[drv].fname[0])) {
		return(FALSE);
	}
	return(TRUE);
}

BOOL fdd_diskprotect(REG8 drv) {

	if ((drv >= MAX_FDDFILE) || (!fddfile[drv].protect)) {
		return(FALSE);
	}
	return(TRUE);
}


// --------------------------------------------------------------------------

BRESULT fdd_set(REG8 drv, const OEMCHAR *fname, UINT ftype, int ro) {

	FDDFILE		fdd;
	FDDFUNC		fdd_fn;	//	追加(Kai1)
	UINT		fddtype;
const OEMCHAR	*p;
	BRESULT		r;

	if (drv >= MAX_FDDFILE) {
		return(FAILURE);
	}
	fddtype = ftype;
	if (fddtype == FTYPE_NONE) {
		p = file_getext(fname);
		if ((!milstr_cmp(p, str_d88)) || (!milstr_cmp(p, str_88d)) ||
			(!milstr_cmp(p, str_d98)) || (!milstr_cmp(p, str_98d))) {
			fddtype = FTYPE_D88;
		}
		else if (!milstr_cmp(p, str_fdi)) {
			fddtype = FTYPE_FDI;
		}
//	厳密な対応のためにFTYPE_BETAより分離(Kai1)
		else if ((!milstr_cmp(p, str_dcp)) || (!milstr_cmp(p, str_dcu))) {
			fddtype = FTYPE_DCP;
		}
//
//	追加(Kai1)
		else if (!milstr_cmp(p, str_nfd)) {
			fddtype = FTYPE_NFD;
		}
		else if (!milstr_cmp(p, str_vfdd)) {
			fddtype = FTYPE_VFDD;
		}
//
		else {
			fddtype = FTYPE_BETA;
		}
	}
	fdd = fddfile + drv;
	fdd_fn = fddfunc + drv;	//	追加(Kai1)
	fdd_fn->eject(fdd);		//	念のためイジェクト(Kai1)
	switch(fddtype) {
		case FTYPE_FDI:
			r = fdd_set_fdi(fdd, fdd_fn, fname, ro);
			if (r == SUCCESS) {
				break;
			}
			/* FALLTHROUGH */

		case FTYPE_BETA:
			r = fdd_set_xdf(fdd, fdd_fn, fname, ro);
			//	追加(Kai1)
			if (r != SUCCESS) {
				//	BKDSK(HDB)	BASIC 2HDかな？かな？
				r = fdd_set_bkdsk(fdd, fdd_fn, fname, ro);
				break;
			}
			//
			break;

		case FTYPE_D88:
			r = fdd_set_d88(fdd, fdd_fn, fname, ro);
			break;
//	厳密な対応(Kai1)
		case FTYPE_DCP:
			r = fdd_set_dcp(fdd, fdd_fn, fname, ro);
			break;
//
//	追加(Kai1)
		case FTYPE_NFD:
			r = fdd_set_nfd(fdd, fdd_fn, fname, ro);
			break;
		case FTYPE_VFDD:
			r = fdd_set_vfdd(fdd, fdd_fn, fname, ro);
			break;
//
		default:
			r = fdd_set_xdf(fdd, fdd_fn, fname, ro);
			//	追加(Kai1)
			if (r != SUCCESS) {
				//	BKDSK(HDB)	BASIC 2HDかな？かな？
				r = fdd_set_bkdsk(fdd, fdd_fn, fname, ro);
				break;
			}
			//
			break;
			//r = FAILURE;
	}
	if (r == SUCCESS) {
		file_cpyname(fdd->fname, fname, NELEMENTS(fdd->fname));
		file_cpyname(np2cfg.fddfile[drv], fname, NELEMENTS(np2cfg.fddfile[drv]));
		fdd->ftype = ftype;
		fdd->ro = ro;
	}
	return(FAILURE);
}

BRESULT fdd_eject(REG8 drv) {

	BRESULT		ret;	//	追加(Kai1)
	FDDFILE		fdd;
	FDDFUNC		fdd_fn;	//	追加(Kai1)

	if (drv >= MAX_FDDFILE) {
		return(FAILURE);
	}
	fdd = fddfile + drv;
	fdd_fn = fddfunc + drv;	//	追加(Kai1)
#if 1						//	変更(Kai1)
	ret = fdd_fn->eject(fdd);

	ZeroMemory(fdd, sizeof(_FDDFILE));

	fdd->fname[0] = '\0';
	fdd->type = DISKTYPE_NOTREADY;

	fddfunc_init(fdd_fn);

	return ret;
#else
	switch(fdd->type) {
		case DISKTYPE_BETA:
//			return(fddxdf_eject(fdd));
			return(fdd_eject_xdf(fdd));

		case DISKTYPE_D88:
//			return(fddd88_eject(fdd));
			return(fdd_eject_d88(fdd));
	}
	return(FAILURE);
#endif
}

//	----
//	未実装、未対応用ダミー関数群(Kai1)
BRESULT fdd_dummy_xxx(FDDFILE fdd) {

	(void)fdd;
	return(FAILURE);
}

BRESULT fdd_eject_xxx(FDDFILE fdd) {

	(void)fdd;
	return(SUCCESS);
}

BRESULT fdd_formating_xxx(FDDFILE fdd, const UINT8 *ID) {

	(void)fdd;
	(void)ID;
	return(FAILURE);
}

BOOL fdd_isformating_xxx(FDDFILE fdd) {

	(void)fdd;
	/* 170107 to support format command form ... */
	//return(FAILURE);
	return FALSE;
	/* 170107 to support format command ... to */
}
// ----
//	ベタ系イメージ用共通処理関数群(Kai1)
BRESULT fdd_diskaccess_common(FDDFILE fdd) {

	if (CTRL_FDMEDIA != fdd->inf.xdf.disktype) {
		return(FAILURE);
	}
	return(SUCCESS);
}

BRESULT fdd_seek_common(FDDFILE fdd) {

	if ((CTRL_FDMEDIA != fdd->inf.xdf.disktype) ||
		(fdc.rpm[fdc.us] != fdd->inf.xdf.rpm) ||
		//!(fdc.chgreg & fdd->inf.xdf.disktype) ||  // np21w ver0.86 rev20
		(fdc.ncn >= (fdd->inf.xdf.tracks >> 1))) {
		return(FAILURE);
	}
	return(SUCCESS);
}

BRESULT fdd_seeksector_common(FDDFILE fdd) {

	if ((CTRL_FDMEDIA != fdd->inf.xdf.disktype) ||
		(fdc.rpm[fdc.us] != fdd->inf.xdf.rpm) ||
		//!(fdc.chgreg & fdd->inf.xdf.disktype) ||  // np21w ver0.86 rev20
		(fdc.treg[fdc.us] >= (fdd->inf.xdf.tracks >> 1))) {
TRACEOUT(("fdd_seek_common FAILURE CTRL_FDMEDIA[%02x], DISKTYPE[%02x]", CTRL_FDMEDIA, fdd->inf.xdf.disktype));
TRACEOUT(("fdd_seek_common FAILURE fdc.rpm[%02x], fdd->rpm[%02x]", fdc.rpm[fdc.us], fdd->inf.xdf.rpm));
TRACEOUT(("fdd_seek_common FAILURE fdc.treg[%02x], fdd->trk[%02x]", fdc.treg[fdc.us], (fdd->inf.xdf.tracks >> 1)));
		fddlasterror = 0xe0;
		return(FAILURE);
	}
	if ((!fdc.R) || (fdc.R > fdd->inf.xdf.sectors)) {
TRACEOUT(("fdd_seek_common FAILURE fdc.R[%02x], Secters[%02x]", fdc.R, fdd->inf.xdf.sectors));
		fddlasterror = 0xc0;
		return(FAILURE);
	}
	if ((fdc.mf != 0xff) && (fdc.mf != 0x40)) {
TRACEOUT(("fdd_seek_common FAILURE fdc.mf[%02x]", fdc.mf));
		fddlasterror = 0xc0;
		return(FAILURE);
	}
	return(SUCCESS);
}

BRESULT fdd_readid_common(FDDFILE fdd) {

	fddlasterror = 0x00;
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
	if ((!fdc.mf) ||
		(fdc.rpm[fdc.us] != fdd->inf.xdf.rpm) ||
		(CTRL_FDMEDIA != fdd->inf.xdf.disktype)) {
		//!(fdc.chgreg & fdd->inf.xdf.disktype) ||  // np21w ver0.86 rev20
		//(fdc.crcn >= fdd->inf.xdf.sectors)) {
		fddlasterror = 0xe0;
		return(FAILURE);
	}
	fdc.C = fdc.treg[fdc.us];
	fdc.H = fdc.hd;
	fdc.R = ++fdc.crcn;
	fdc.N = fdd->inf.xdf.n;
	return(SUCCESS);
}
// ----

BRESULT fdd_diskaccess(void) {

	FDDFILE		fdd;
	FDDFUNC		fdd_fn;			//	追加(Kai1)

	fdd = fddfile + fdc.us;
	fdd_fn = fddfunc + fdc.us;	//	追加(Kai1)
#if 1							//	変更(Kai1)
	return(fdd_fn->diskaccess(fdd));
#else
	switch(fdd->type) {
		case DISKTYPE_BETA:
//			return(fddxdf_diskaccess(fdd));
			return(fdd_diskaccess_xdf(fdd));

		case DISKTYPE_D88:
//			return(fdd_diskaccess_d88());
			return(fdd_diskaccess_d88(fdd));
	}
	return(FAILURE);
#endif
}

BRESULT fdd_seek(void) {

	BRESULT		ret;
	FDDFILE		fdd;
	FDDFUNC		fdd_fn;			//	追加(Kai1)

	ret = FAILURE;
	fdd = fddfile + fdc.us;
	fdd_fn = fddfunc + fdc.us;	//	追加(Kai1)
#if 1							//	変更(Kai1)
	ret = fdd_fn->seek(fdd);
#else
	switch(fdd->type) {
		case DISKTYPE_BETA:
//			ret = fddxdf_seek(fdd);
			ret = fdd_seek_xdf(fdd);
			break;

		case DISKTYPE_D88:
//			ret = fdd_seek_d88();
			ret = fdd_seek_d88(fdd);
			break;
	}
#endif
	fdc.treg[fdc.us] = fdc.ncn;
	return(ret);
}

BRESULT fdd_seeksector(void) {

	FDDFILE		fdd;
	FDDFUNC		fdd_fn;			//	追加(Kai1)

	fdd = fddfile + fdc.us;
	fdd_fn = fddfunc + fdc.us;	//	追加(Kai1)
#if 1							//	変更(Kai1)
	return(fdd_fn->seeksector(fdd));
#else
	switch(fdd->type) {
		case DISKTYPE_BETA:
//			return(fddxdf_seeksector(fdd));
			return(fdd_seeksector_xdf(fdd));

		case DISKTYPE_D88:
//			return(fdd_seeksector_d88());
			return(fdd_seeksector_d88(fdd));
	}
	return(FAILURE);
#endif
}


BRESULT fdd_read(void) {

	FDDFILE		fdd;
	FDDFUNC		fdd_fn;			//	追加(Kai1)

	sysmng_fddaccess(fdc.us);
	fdd = fddfile + fdc.us;
	fdd_fn = fddfunc + fdc.us;	//	追加(Kai1)
#if 1							//	変更(Kai1)
	return(fdd_fn->read(fdd));
#else
	switch(fdd->type) {
		case DISKTYPE_BETA:
//			return(fddxdf_read(fdd));
			return(fdd_read_xdf(fdd));

		case DISKTYPE_D88:
//			return(fdd_read_d88());
			return(fdd_read_d88(fdd));
	}
	return(FAILURE);
#endif
}

BRESULT fdd_write(void) {

	FDDFILE		fdd;
	FDDFUNC		fdd_fn;			//	追加(Kai1)

	sysmng_fddaccess(fdc.us);
	fdd = fddfile + fdc.us;
	fdd_fn = fddfunc + fdc.us;	//	追加(Kai1)
#if 1							//	変更(Kai1)
	return(fdd_fn->write(fdd));
#else
	switch(fdd->type) {
		case DISKTYPE_BETA:
//			return(fddxdf_write(fdd));
			return(fdd_write_xdf(fdd));

		case DISKTYPE_D88:
//			return(fdd_write_d88());
			return(fdd_write_d88(fdd));
	}
	return(FAILURE);
#endif
}

BRESULT fdd_readid(void) {

	FDDFILE		fdd;
	FDDFUNC		fdd_fn;			//	追加(Kai1)

	sysmng_fddaccess(fdc.us);
	fdd = fddfile + fdc.us;
	fdd_fn = fddfunc + fdc.us;	//	追加(Kai1)
#if 1							//	変更(Kai1)
	return(fdd_fn->readid(fdd));
#else
	switch(fdd->type) {
		case DISKTYPE_BETA:
//			return(fddxdf_readid(fdd));
			return(fdd_readid_xdf(fdd));

		case DISKTYPE_D88:
//			return(fdd_readid_d88());
			return(fdd_readid_d88(fdd));
	}
	return(FAILURE);
#endif
}

BRESULT fdd_formatinit(void) {

	FDDFILE		fdd;			//	追加(Kai1)
	FDDFUNC		fdd_fn;			//	追加(Kai1)

	fdd = fddfile + fdc.us;		//	追加(Kai1)
	fdd_fn = fddfunc + fdc.us;	//	追加(Kai1)
#if 1							//	変更(Kai1)
	return(fdd_fn->formatinit(fdd));
#else
	if (fddfile[fdc.us].type == DISKTYPE_D88) {
//		return(fdd_formatinit_d88());
		return(fdd_formatinit_d88(fdd));
	}
	return(FAILURE);
#endif
}

BRESULT fdd_formating(const UINT8 *ID) {

	FDDFILE		fdd;			//	追加(Kai1)
	FDDFUNC		fdd_fn;			//	追加(Kai1)

	sysmng_fddaccess(fdc.us);
	fdd = fddfile + fdc.us;		//	追加(Kai1)
	fdd_fn = fddfunc + fdc.us;	//	追加(Kai1)
#if 1							//	変更(Kai1)
	return(fdd_fn->formating(fdd, ID));
#else
	if (fddfile[fdc.us].type == DISKTYPE_D88) {
//		return(fdd_formating_d88(ID));
		return(fdd_formating_d88(fdd, ID));
	}
	return(FAILURE);
#endif
}

BOOL fdd_isformating(void) {

	FDDFILE		fdd;			//	追加(Kai1)
	FDDFUNC		fdd_fn;			//	追加(Kai1)

	fdd = fddfile + fdc.us;		//	追加(Kai1)
	fdd_fn = fddfunc + fdc.us;	//	追加(Kai1)
#if 1							//	変更(Kai1)
	/* 170107 to support format command form ... */
	//return(fdd_fn->formatinit(fdd));
	return(fdd_fn->isformating(fdd));
	/* 170107 to support format command ... to */
#else
	if (fddfile[fdc.us].type == DISKTYPE_D88) {
//		return(fdd_isformating_d88());
		return(fdd_isformating_d88(fdd));
	}
	return(FALSE);
#endif
}

//	追加(Kai1)
BOOL fdd_fdcresult(void) {

	FDDFUNC		fdd_fn;

	fdd_fn = fddfunc + fdc.us;

	return(fdd_fn->fdcresult);
}
//

#else
#include	"diskimage/fd/fdd_xdf.h"
#include	"diskimage/fd/fdd_d88.h"


	_FDDFILE	fddfile[MAX_FDDFILE];
	UINT8		fddlasterror;


// ----

void fddfile_initialize(void) {

	ZeroMemory(fddfile, sizeof(fddfile));
}

void fddfile_reset2dmode(void) { 			// ver0.29
#if 0
	int		i;

	for (i=0; i<4; i++) {
		fddfile[i].mode2d = 0;
	}
#endif
}

OEMCHAR *fdd_diskname(REG8 drv) {

	if (drv >= MAX_FDDFILE) {
		return(NULL);
	}
	return(fddfile[drv].fname);
}

OEMCHAR *fdd_getfileex(REG8 drv, UINT *ftype, int *ro) {

	FDDFILE	fdd;

	if (drv >= MAX_FDDFILE) {
		return((OEMCHAR *)str_null);
	}
	fdd = fddfile + drv;
	if (ftype) {
		*ftype = fdd->ftype;
	}
	if (ro) {
		*ro = fdd->ro;
	}
	return(fdd->fname);
}

BOOL fdd_diskready(REG8 drv) {

	if ((drv >= MAX_FDDFILE) || (!fddfile[drv].fname[0])) {
		return(FALSE);
	}
	return(TRUE);
}

BOOL fdd_diskprotect(REG8 drv) {

	if ((drv >= MAX_FDDFILE) || (!fddfile[drv].protect)) {
		return(FALSE);
	}
	return(TRUE);
}


// --------------------------------------------------------------------------

BRESULT fdd_set(REG8 drv, const OEMCHAR *fname, UINT ftype, int ro) {

	FDDFILE		fdd;
	UINT		fddtype;
const OEMCHAR	*p;
	BRESULT		r;

	if (drv >= MAX_FDDFILE) {
		return(FAILURE);
	}
	fddtype = ftype;
	if (fddtype == FTYPE_NONE) {
		p = file_getext(fname);
		if ((!milstr_cmp(p, str_d88)) || (!milstr_cmp(p, str_88d)) ||
			(!milstr_cmp(p, str_d98)) || (!milstr_cmp(p, str_98d))) {
			fddtype = FTYPE_D88;
		}
		else if (!milstr_cmp(p, str_fdi)) {
			fddtype = FTYPE_FDI;
		}
		else {
			fddtype = FTYPE_BETA;
		}
	}
	fdd = fddfile + drv;
	switch(fddtype) {
		case FTYPE_FDI:
			r = fddxdf_setfdi(fdd, fname, ro);
			if (r == SUCCESS) {
				break;
			}
			/* FALLTHROUGH */

		case FTYPE_BETA:
			r = fddxdf_set(fdd, fname, ro);
			break;

		case FTYPE_D88:
			r = fddd88_set(fdd, fname, ro);
			break;

		default:
			r = FAILURE;
	}
	if (r == SUCCESS) {
		file_cpyname(fdd->fname, fname, NELEMENTS(fdd->fname));
		fdd->ftype = ftype;
		fdd->ro = ro;
	}
	return(FAILURE);
}

BRESULT fdd_eject(REG8 drv) {

	FDDFILE		fdd;

	if (drv >= MAX_FDDFILE) {
		return(FAILURE);
	}
	fdd = fddfile + drv;
	switch(fdd->type) {
		case DISKTYPE_BETA:
			return(fddxdf_eject(fdd));

		case DISKTYPE_D88:
			return(fddd88_eject(fdd));
	}
	return(FAILURE);
}


// ----

BRESULT fdd_diskaccess(void) {

	FDDFILE		fdd;

	fdd = fddfile + fdc.us;
	switch(fdd->type) {
		case DISKTYPE_BETA:
			return(fddxdf_diskaccess(fdd));

		case DISKTYPE_D88:
			return(fdd_diskaccess_d88());
	}
	return(FAILURE);
}

BRESULT fdd_seek(void) {

	BRESULT		ret;
	FDDFILE		fdd;

	ret = FAILURE;
	fdd = fddfile + fdc.us;
	switch(fdd->type) {
		case DISKTYPE_BETA:
			ret = fddxdf_seek(fdd);
			break;

		case DISKTYPE_D88:
			ret = fdd_seek_d88();
			break;
	}
	fdc.treg[fdc.us] = fdc.ncn;
	return(ret);
}

BRESULT fdd_seeksector(void) {

	FDDFILE		fdd;

	fdd = fddfile + fdc.us;
	switch(fdd->type) {
		case DISKTYPE_BETA:
			return(fddxdf_seeksector(fdd));

		case DISKTYPE_D88:
			return(fdd_seeksector_d88());
	}
	return(FAILURE);
}


BRESULT fdd_read(void) {

	FDDFILE		fdd;

	sysmng_fddaccess(fdc.us);
	fdd = fddfile + fdc.us;
	switch(fdd->type) {
		case DISKTYPE_BETA:
			return(fddxdf_read(fdd));

		case DISKTYPE_D88:
			return(fdd_read_d88());
	}
	return(FAILURE);
}

BRESULT fdd_write(void) {

	FDDFILE		fdd;

	sysmng_fddaccess(fdc.us);
	fdd = fddfile + fdc.us;
	switch(fdd->type) {
		case DISKTYPE_BETA:
			return(fddxdf_write(fdd));

		case DISKTYPE_D88:
			return(fdd_write_d88());
	}
	return(FAILURE);
}

BRESULT fdd_readid(void) {

	FDDFILE		fdd;

	sysmng_fddaccess(fdc.us);
	fdd = fddfile + fdc.us;
	switch(fdd->type) {
		case DISKTYPE_BETA:
			return(fddxdf_readid(fdd));

		case DISKTYPE_D88:
			return(fdd_readid_d88());
	}
	return(FAILURE);
}

BRESULT fdd_formatinit(void) {

	if (fddfile[fdc.us].type == DISKTYPE_D88) {
		return(fdd_formatinit_d88());
	}
	return(FAILURE);
}

BRESULT fdd_formating(const UINT8 *ID) {

	sysmng_fddaccess(fdc.us);
	if (fddfile[fdc.us].type == DISKTYPE_D88) {
		return(fdd_formating_d88(ID));
	}
	return(FAILURE);
}

BOOL fdd_isformating(void) {

	if (fddfile[fdc.us].type == DISKTYPE_D88) {
		return(fdd_isformating_d88());
	}
	return(FALSE);
}

#endif