#include	"compiler.h"
#include	"dosio.h"
#include	"pccore.h"
#include	"iocore.h"

#include	"diskimage/fddfile.h"
#include	"diskimage/fd/fdd_d88.h"

#define		D88BUFSIZE		0x6000
#define		D88TRACKMAX		10600


#ifdef SUPPORT_KAI_IMAGES

static UINT32 nexttrackptr(FDDFILE fdd, UINT32 fptr, UINT32 last) {

	int		t;
	UINT32	cur;

	for (t=0; t<164; t++) {
		cur = fdd->inf.d88.ptr[t];
		if ((cur > fptr) && (cur < last)) {
			last = cur;
		}
	}
	return(last);
}


// ----

typedef struct {
	FDDFILE	fdd;
	UINT	track;
	UINT	type;
	long	fptr;
	UINT	size;
	BOOL	write;
	UINT8	buf[D88BUFSIZE];
} _D88TRK, *D88TRK;

static	_D88TRK		d88trk;


static BRESULT d88trk_flushdata(D88TRK trk) {

	FDDFILE		fdd;
	FILEH		fh;

	fdd = trk->fdd;
	trk->fdd = NULL;
	if ((fdd == NULL) || (trk->size == 0) || (!trk->write)) {
		goto dtfd_exit;
	}
	if (fdd->protect) {
		goto dtfd_exit;
	}
	fh = file_open(fdd->fname);
	if (fh == FILEH_INVALID) {
		goto dtfd_err1;
	}
	if ((file_seek(fh, trk->fptr, FSEEK_SET) != trk->fptr) ||
		(file_write(fh, trk->buf, trk->size) != trk->size)) {
		goto dtfd_err2;
	}
	file_close(fh);
	trk->write = FALSE;

dtfd_exit:
	return(SUCCESS);

dtfd_err2:
	file_close(fh);

dtfd_err1:
	return(FAILURE);
}

static BRESULT d88trk_read(D88TRK trk, FDDFILE fdd, UINT track, UINT type) {

	UINT8	rpm;
	FILEH	fh;
	UINT32	fptr;
	UINT32	size;

	d88trk_flushdata(trk);
	if (track >= 164) {
		goto dtrd_err1;
	}

	rpm = fdc.rpm[fdc.us];
	switch(fdd->inf.d88.fdtype_major) {
		case DISKTYPE_2D:
			TRACEOUT(("DISKTYPE_2D"));
			if ((rpm) || (type != DISKTYPE_2DD) || (track & 2)) {
				goto dtrd_err1;
			}
			track = ((track >> 1) & 0xfe) | (track & 1);
			break;

		case DISKTYPE_2DD:
			if ((rpm) || (type != DISKTYPE_2DD)) {
				goto dtrd_err1;
			}
			break;

		case DISKTYPE_2HD:
			if (CTRL_FDMEDIA != DISKTYPE_2HD) {
				goto dtrd_err1;
			}
			if ((fdd->inf.d88.fdtype_minor == 0) && (rpm)) {
				goto dtrd_err1;
			}
			break;

		default:
			goto dtrd_err1;
	}

	fptr = fdd->inf.d88.ptr[track];
	if (fptr == 0) {
		goto dtrd_err1;
	}
	size = nexttrackptr(fdd, fptr, fdd->inf.d88.fd_size) - fptr;
	if (size > D88BUFSIZE) {
		size = D88BUFSIZE;
	}
	fh = file_open_rb(fdd->fname);
	if (fh == FILEH_INVALID) {
		goto dtrd_err1;
	}
	if ((file_seek(fh, (long)fptr, FSEEK_SET) != (long)fptr) ||
		(file_read(fh, trk->buf, size) != size)) {
		goto dtrd_err2;
	}
	file_close(fh);

	trk->fdd = fdd;
	trk->track = track;
	trk->type = type;
	trk->fptr = fptr;
	trk->size = size;
	trk->write = FALSE;
	return(SUCCESS);

dtrd_err2:
	file_close(fh);

dtrd_err1:
	return(FAILURE);
}


static BRESULT rpmcheck(D88SEC sec) {

	FDDFILE	fdd = fddfile + fdc.us;
	UINT8	rpm;

	rpm = fdc.rpm[fdc.us];
	switch(fdd->inf.d88.fdtype_major) {
		case DISKTYPE_2D:
		case DISKTYPE_2DD:
			if (rpm) {
				return(FAILURE);
			}
			break;

		case DISKTYPE_2HD:
			if (fdd->inf.d88.fdtype_minor == 0) {
				if (rpm) {
					return(FAILURE);
				}
			}
			else {
				if (sec->rpm_flg != rpm) {
					return(FAILURE);
				}
			}
			break;

		default:
			return(FAILURE);
	}
	return(SUCCESS);
}


// ----

static void drvflush(FDDFILE fdd) {

	D88TRK	trk;

	trk = &d88trk;
	if (trk->fdd == fdd && !fdd->protect) {
		d88trk_flushdata(trk);
		trk->fdd = NULL;
	}
}

static BRESULT trkseek(FDDFILE fdd, UINT track) {

	D88TRK	trk;
	BRESULT	r;

	trk = &d88trk;
	if ((trk->fdd == fdd) && (trk->track == track) &&
		(trk->type == CTRL_FDMEDIA)) {
		r = SUCCESS;
	}
	else {
		r = d88trk_read(trk, fdd, track, CTRL_FDMEDIA);
	}
	return(r);
}


static D88SEC searchsector_d88(BOOL check) {			// ver0.29

	UINT8	*p;
	UINT	sec;
	UINT	pos = 0;
	UINT	nsize;
	UINT	sectors;
	UINT	secsize;

	if (fdc.N < 8) {
		nsize = 128 << fdc.N;
	}
	else {
		nsize = 128 << 8;
	}

	p = d88trk.buf;
	for (sec=0; sec<40; ) {
		if ((pos + nsize + sizeof(_D88SEC)) > D88BUFSIZE) {
			break;
		}

		if ((((D88SEC)p)->c == fdc.C) &&
			(((D88SEC)p)->h == fdc.H) &&
			(((D88SEC)p)->r == fdc.R) &&
			(((D88SEC)p)->n == fdc.N) &&
			(!rpmcheck((D88SEC)p))) {

			// ver0.29
			if (check) {
				if ((fdc.mf != 0xff) &&
					!((fdc.mf ^ (((D88SEC)p)->mfm_flg)) & 0x40)) {
					break;
				}
			}
			return((D88SEC)p);
		}
		sectors = LOADINTELWORD(((D88SEC)p)->sectors);
		if (++sec >= sectors) {
			break;
		}

		secsize = LOADINTELWORD(((D88SEC)p)->size);
		secsize += sizeof(_D88SEC);
		pos += secsize;
		p += secsize;
	}
	return(NULL);
}


// ----

//BRESULT fddd88_set(FDDFILE fdd, const OEMCHAR *fname, int ro) {
BRESULT fdd_set_d88(FDDFILE fdd, FDDFUNC fdd_fn, const OEMCHAR *fname, int ro) {

	short	attr;
	FILEH	fh;
	UINT	rsize;
	int		i;

//	fddd88_eject(fdd);	//	削除(kai9)
	attr = file_attr(fname);
	if (attr & 0x18) {
		goto fdst_err;
	}
	if(attr & FILEATTR_READONLY) {
		ro = 1;
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
		goto fdst_err;
	}
	rsize = file_read(fh, &fdd->inf.d88.head, sizeof(fdd->inf.d88.head));
	file_close(fh);
	if (rsize != sizeof(fdd->inf.d88.head)) {
		goto fdst_err;
	}
	fdd->type = DISKTYPE_D88;
	fdd->protect = ((attr & 1) || (fdd->inf.d88.head.protect & 0x10) ||
															(ro))?TRUE:FALSE;
	fdd->inf.d88.fdtype_major = fdd->inf.d88.head.fd_type >> 4;
	fdd->inf.d88.fdtype_minor = fdd->inf.d88.head.fd_type & 0x0f;
	fdd->inf.d88.fd_size = LOADINTELDWORD(fdd->inf.d88.head.fd_size);
	for (i=0; i<164; i++) {
		fdd->inf.d88.ptr[i] = LOADINTELDWORD(fdd->inf.d88.head.trackp[i]);
	}
	//	処理関数群を登録(kai9)
	fdd_fn->eject = fdd_eject_d88;
	fdd_fn->diskaccess = fdd_diskaccess_d88;
	fdd_fn->seek = fdd_seek_d88;
	fdd_fn->seeksector = fdd_seeksector_d88;
	fdd_fn->read = fdd_read_d88;
	fdd_fn->write = fdd_write_d88;
	fdd_fn->readid = fdd_readid_d88;
	fdd_fn->writeid = fdd_dummy_xxx;
	fdd_fn->formatinit = fdd_formatinit_d88;
	fdd_fn->formating = fdd_formating_d88;
	fdd_fn->isformating = fdd_isformating_d88;
	//
	return(SUCCESS);

fdst_err:
	return(FAILURE);
}

//BRESULT fddd88_eject(FDDFILE fdd) {
BRESULT fdd_eject_d88(FDDFILE fdd) {

	drvflush(fdd);
//	共通関数部へ移動(kai9)
//	fdd->fname[0] = '\0';
//	fdd->type = DISKTYPE_NOTREADY;
//	ZeroMemory(&fdd->inf.d88.head, sizeof(fdd->inf.d88.head));
	return(SUCCESS);
}


//BRESULT fdd_diskaccess_d88(void) {									// ver0.31
BRESULT fdd_diskaccess_d88(FDDFILE fdd) {

//	FDDFILE	fdd = fddfile + fdc.us;
	UINT8	rpm;

	rpm = fdc.rpm[fdc.us];
	switch(fdd->inf.d88.fdtype_major) {
		case DISKTYPE_2D:
		case DISKTYPE_2DD:
			if ((rpm) || (CTRL_FDMEDIA != DISKTYPE_2DD)) {
				return(FAILURE);
			}
			break;

		case DISKTYPE_2HD:
			if (CTRL_FDMEDIA != DISKTYPE_2HD) {
				return(FAILURE);
			}
			if ((fdd->inf.d88.fdtype_minor == 0) && (rpm)) {
				return(FAILURE);
			}
			break;

		default:
			return(FAILURE);

	}
	return(SUCCESS);
}

//BRESULT fdd_seek_d88(void) {
BRESULT fdd_seek_d88(FDDFILE fdd) {

//	FDDFILE	fdd = fddfile + fdc.us;
TRACEOUT(("D88 seek trkseek[%03d]", (fdc.treg[fdc.us] << 1) + fdc.hd));
	return(trkseek(fdd, (fdc.ncn << 1) + fdc.hd));
}

//BRESULT fdd_seeksector_d88(void) {
BRESULT fdd_seeksector_d88(FDDFILE fdd) {

//	FDDFILE	fdd = fddfile + fdc.us;

TRACEOUT(("D88 seeksector trkseek[%03d]", (fdc.treg[fdc.us] << 1) + fdc.hd));
	if (trkseek(fdd, (fdc.treg[fdc.us] << 1) + fdc.hd)) {
TRACEOUT(("D88 seeksector FAILURE trkseek[%03d]", (fdc.treg[fdc.us] << 1) + fdc.hd));
		return(FAILURE);
	}
	if (!searchsector_d88(FALSE)) {
TRACEOUT(("D88 seeksector FAILURE searchsector[%03d]", (fdc.treg[fdc.us] << 1) + fdc.hd));
		return(FAILURE);
	}
	return(SUCCESS);
}

//BRESULT fdd_read_d88(void) {
BRESULT fdd_read_d88(FDDFILE fdd) {

//	FDDFILE		fdd = fddfile + fdc.us;
	D88SEC		p;
	UINT		size;
	UINT		secsize;

TRACEOUT(("D88 read trkseek[%03d]", (fdc.treg[fdc.us] << 1) + fdc.hd));
	fddlasterror = 0x00;
	if (trkseek(fdd, (fdc.treg[fdc.us] << 1) + fdc.hd)) {
TRACEOUT(("D88 read FAILURE trkseek[%03d]", (fdc.treg[fdc.us] << 1) + fdc.hd));
		fddlasterror = 0xe0;
		return(FAILURE);
	}
	p = searchsector_d88(TRUE);
	if (!p) {
TRACEOUT(("D88 read FAILURE searchsector_d88[%03d]", (fdc.treg[fdc.us] << 1) + fdc.hd));
		fddlasterror = 0xc0;
		return(FAILURE);
	}
	if (fdc.N < 8) {
		size = 128 << fdc.N;
	}
	else {
		size = 128 << 8;
	}
	fdc.bufcnt = size;
	ZeroMemory(fdc.buf, size);
	secsize = LOADINTELWORD(p->size);
	if (size > secsize) {
		size = secsize;
	}
	if (size) {
		CopyMemory(fdc.buf, p+1, size);
	}
	fddlasterror = p->stat;
TRACEOUT(("D88 read FDC Result Status[%02x]", p->stat));
TRACEOUT(("D88 read C:%02x,H:%02x,R:%02x,N:%02x", fdc.C, fdc.H, fdc.R, fdc.N));
TRACEOUT(("D88 read dump"));
TRACEOUT(("\t%02x %02x %02x %02x %02x %02x %02x %02x",
		 fdc.buf[0x00], fdc.buf[0x01], fdc.buf[0x02], fdc.buf[0x03], fdc.buf[0x04], fdc.buf[0x05], fdc.buf[0x06], fdc.buf[0x07]));
	return(SUCCESS);
}

//BRESULT fdd_write_d88(void) {
BRESULT fdd_write_d88(FDDFILE fdd) {

//	FDDFILE		fdd = fddfile + fdc.us;
	D88SEC		p;
	UINT		size;
	UINT		secsize;

	if (fdd->protect) {
		return(FAILURE);
	}

	fddlasterror = 0x00;
	if (trkseek(fdd, (fdc.treg[fdc.us] << 1) + fdc.hd)) {
		fddlasterror = 0xe0;
		return(FAILURE);
	}
	p = searchsector_d88(FALSE);
	if (!p) {
		fddlasterror = 0xc0;
		return(FAILURE);
	}
	if (fdc.N < 8) {
		size = 128 << fdc.N;
	}
	else {
		size = 128 << 8;
	}
	secsize = LOADINTELWORD(p->size);
	if (size > secsize) {
		size = secsize;
	}
	if (size) {
		CopyMemory(p+1, fdc.buf, size);
		d88trk.write = TRUE;
	}
	fddlasterror = 0x00;
	return(SUCCESS);
}

//BRESULT fdd_readid_d88(void) {
BRESULT fdd_readid_d88(FDDFILE fdd) {

//	FDDFILE	fdd = fddfile + fdc.us;
	UINT8	*p;
	UINT	sec;
	UINT	pos = 0;
	UINT	sectors;
	UINT	secsize;

	fddlasterror = 0x00;
	if (trkseek(fdd, (fdc.treg[fdc.us] << 1) + fdc.hd)) {
		fddlasterror = 0xe0;
		return(FAILURE);
	}
	p = d88trk.buf;
	for (sec=0; sec<40; ) {
		if (pos > (D88BUFSIZE - sizeof(_D88SEC))) {
			break;
		}
		sectors = LOADINTELWORD(((D88SEC)p)->sectors);
		if ((sec == fdc.crcn) && (!rpmcheck((D88SEC)p))) {			// ver0.31
			/* 170101 ST modified to work on Windows 9x/2000 form ... */
			if (++fdc.crcn >= sectors) {
				fdc.crcn = 0;
				/* np21w rev36 removed */
				//if(fdc.mt) {
				//	fdc.hd ^= 1;
				//	if (fdc.hd == 0) {
				//		fdc.treg[fdc.us]++;
				//	}
				//}
				//else {
				//	fdc.treg[fdc.us]++;
				//}
			}
			fdc.C = fdc.treg[fdc.us];
			fdc.H = fdc.hd;
			fdc.R = ((D88SEC)p)->r;
			fdc.N = ((D88SEC)p)->n;
			//fdc.crcn++;
			//if (fdc.crcn >= sectors) {
			//	fdc.crcn = 0;
			//}
			/* 170101 ST modified to work on Windows 9x/2000 ... to */
			if ((fdc.mf == 0xff) ||
					((fdc.mf ^ (((D88SEC)p)->mfm_flg)) & 0x40)) {
				fddlasterror = 0x00;
				return(SUCCESS);
			}
		}
		if (++sec >= sectors) {
			break;
		}
		secsize = LOADINTELWORD(((D88SEC)p)->size);
		secsize += sizeof(_D88SEC);
		pos += secsize;
		p += secsize;
	}
	fdc.crcn = 0x00;
	fddlasterror = 0xe0;											// ver0.31
	return(FAILURE);
}


// --------------------------------------------------------------------------

// えーと…こんなところにあって大丈夫？
static BOOL formating = FALSE;
static UINT8 formatsec = 0;
static UINT8 formatwrt = 0;
static UINT formatpos = 0;

static int fileappend(FILEH hdl, FDDFILE fdd,
									UINT32 ptr, long last, long apsize) {

	long	length;
	UINT	size;
	UINT	rsize;
	int		t;
	UINT8	tmp[0x400];							// Stack 0x1000->0x400
	UINT32	cur;

	if (fdd->protect) {
		return(0);
	}

	if ((length = last - ptr) <= 0) {			// 書き換える必要なし
		return(0);
	}
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
		file_seek(hdl, ptr + length + apsize, 0);
		file_write(hdl, tmp, rsize);
	}

	for (t=0; t<164; t++) {
		cur = fdd->inf.d88.ptr[t];
		if ((cur != 0) && (cur >= ptr)) {
			cur += apsize;
			fdd->inf.d88.ptr[t] = cur;
			STOREINTELDWORD(fdd->inf.d88.head.trackp[t], cur);
		}
	}
	return(0);
}


static void endoftrack(UINT fmtsize, UINT8 sectors) {

	FDDFILE	fdd = fddfile + fdc.us;

	D88SEC	d88sec;
	FILEH	hdl;
	int		i;
	UINT	trk;
	long	fpointer;
	long	endpointer;
	long	lastpointer;
	long	trksize;
	int		ptr;
	long	apsize;

	if (fdd->protect) {
		return;
	}

	trk = (fdc.treg[fdc.us] << 1) + fdc.hd;

	ptr = 0;
	for (i=0; i<(int)sectors; i++) {
		d88sec = (D88SEC)(d88trk.buf + ptr);
		STOREINTELWORD(d88sec->sectors, sectors);
		ptr += LOADINTELWORD(d88sec->size);
		ptr += sizeof(_D88SEC);
	}

	hdl = file_open(fddfile[fdc.us].fname);
	if (hdl == FILEH_INVALID) {
		return;
	}
	lastpointer = (long)file_getsize(hdl);	/*	lastpointer = file_seek(hdl, 0, FSEEK_END);	*/
	fpointer = fdd->inf.d88.ptr[trk];
	if (fpointer == 0) {
		for (i=trk; i>=0; i--) {					// 新規トラック
			fpointer = fdd->inf.d88.ptr[i];
			if (fpointer) {
				break;
			}
		}
		if (fpointer) {								// ヒットした
			fpointer = nexttrackptr(fdd, fpointer, lastpointer);
		}
		else {
			fpointer = sizeof(_D88HEAD);
		}
		endpointer = fpointer;
	}
	else {										// トラックデータは既にある
		endpointer = nexttrackptr(fdd, fpointer, lastpointer);
	}
	trksize = endpointer - fpointer;
	if ((apsize = (long)fmtsize - trksize) > 0) {
								// 書き込むデータのほーが大きい
		fileappend(hdl, fdd, endpointer, lastpointer, apsize);
		fdd->inf.d88.fd_size += apsize;
		STOREINTELDWORD(fdd->inf.d88.head.fd_size, fdd->inf.d88.fd_size);
	}
	fdd->inf.d88.ptr[trk] = fpointer;
	STOREINTELDWORD(fdd->inf.d88.head.trackp[trk], fpointer);
	file_seek(hdl, fpointer, 0);
	file_write(hdl, d88trk.buf, fmtsize);
	file_seek(hdl, 0, FSEEK_SET);
	file_write(hdl, &fdd->inf.d88.head, sizeof(fdd->inf.d88.head));
	file_close(hdl);
//	TRACEOUT(("fmt %d %d", fpointer, fmtsize));
}


//BRESULT fdd_formatinit_d88(void) {
BRESULT fdd_formatinit_d88(FDDFILE fdd) {

	if (fdc.treg[fdc.us] < 82) {
		formating = TRUE;
		formatsec = 0;
		formatpos = 0;
		formatwrt = 0;
		drvflush(fddfile + fdc.us);
		return(SUCCESS);
	}
	return(FAILURE);
}

	// todo アンフォーマットとか ディスク１周した時の切り捨てとか…
//BRESULT fdd_formating_d88(const UINT8 *ID) {
BRESULT fdd_formating_d88(FDDFILE fdd, const UINT8 *ID) {

//	FDDFILE	fdd = fddfile + fdc.us;

	UINT	size;
	D88SEC	d88sec;

	if (!formating) {
		return(FAILURE);
	}
	if (fdc.N < 8) {
		size = 128 << fdc.N;
	}
	else {
		size = 128 << 8;
	}
	if ((formatpos + sizeof(_D88SEC) + size) < D88TRACKMAX) {
		d88sec = (D88SEC)(d88trk.buf + formatpos);
		ZeroMemory(d88sec, sizeof(_D88SEC));
		d88sec->c = ID[0];
		d88sec->h = ID[1];
		d88sec->r = ID[2];
		d88sec->n = ID[3];
		STOREINTELWORD(d88sec->size, size);
		if ((fdd->inf.d88.fdtype_major == DISKTYPE_2HD) &&
			(fdd->inf.d88.fdtype_minor != 0)) {
			d88sec->rpm_flg = fdc.rpm[fdc.us];
		}
		FillMemory(d88sec + 1, size, fdc.d);
		formatpos += sizeof(_D88SEC);
		formatpos += size;
		formatwrt++;
	}
	formatsec++;
//	TRACE_("format sec", formatsec);
//	TRACE_("format wrt", formatwrt);
//	TRACE_("format max", fdc.sc);
//	TRACE_("format pos", formatpos);
	if (formatsec >= fdc.sc) {
		endoftrack(formatpos, formatwrt);
		formating = FALSE;
	}
	return(SUCCESS);
}

//BOOL fdd_isformating_d88(void) {
BOOL fdd_isformating_d88(FDDFILE fdd) {

	return(formating);
}
#else

static UINT nexttrackptr(FDDFILE fdd, UINT fptr, UINT last) {

	int		t;
	UINT	cur;

	for (t=0; t<164; t++) {
		cur = fdd->inf.d88.ptr[t];
		if ((cur > fptr) && (cur < last)) {
			last = cur;
		}
	}
	return(last);
}


// ----

typedef struct {
	FDDFILE	fdd;
	UINT	track;
	UINT	type;
	long	fptr;
	UINT	size;
	BOOL	write;
	UINT8	buf[D88BUFSIZE];
} _D88TRK, *D88TRK;

static	_D88TRK		d88trk;


static BRESULT d88trk_flushdata(D88TRK trk) {

	FDDFILE		fdd;
	FILEH		fh;

	fdd = trk->fdd;
	trk->fdd = NULL;
	if ((fdd == NULL) || (trk->size == 0) || (!trk->write)) {
		goto dtfd_exit;
	}
	if (fdd->protect) {
		goto dtfd_exit;
	}
	fh = file_open(fdd->fname);
	if (fh == FILEH_INVALID) {
		goto dtfd_err1;
	}
	if ((file_seek(fh, trk->fptr, FSEEK_SET) != trk->fptr) ||
		(file_write(fh, trk->buf, trk->size) != trk->size)) {
		goto dtfd_err2;
	}
	file_close(fh);
	trk->write = FALSE;

dtfd_exit:
	return(SUCCESS);

dtfd_err2:
	file_close(fh);

dtfd_err1:
	return(FAILURE);
}

static BRESULT d88trk_read(D88TRK trk, FDDFILE fdd, UINT track, UINT type) {

	UINT8	rpm;
	FILEH	fh;
	UINT	fptr;
	UINT	size;

	d88trk_flushdata(trk);
	if (track >= 164) {
		goto dtrd_err1;
	}

	rpm = fdc.rpm[fdc.us];
	switch(fdd->inf.d88.fdtype_major) {
		case DISKTYPE_2D:
			TRACEOUT(("DISKTYPE_2D"));
			if ((rpm) || (type != DISKTYPE_2DD) || (track & 2)) {
				goto dtrd_err1;
			}
			track = ((track >> 1) & 0xfe) | (track & 1);
			break;

		case DISKTYPE_2DD:
			if ((rpm) || (type != DISKTYPE_2DD)) {
				goto dtrd_err1;
			}
			break;

		case DISKTYPE_2HD:
			if (CTRL_FDMEDIA != DISKTYPE_2HD) {
				goto dtrd_err1;
			}
			if ((fdd->inf.d88.fdtype_minor == 0) && (rpm)) {
				goto dtrd_err1;
			}
			break;

		default:
			goto dtrd_err1;
	}

	fptr = fdd->inf.d88.ptr[track];
	if (fptr == 0) {
		goto dtrd_err1;
	}
	size = nexttrackptr(fdd, fptr, fdd->inf.d88.fd_size) - fptr;
	if (size > D88BUFSIZE) {
		size = D88BUFSIZE;
	}
	fh = file_open_rb(fdd->fname);
	if (fh == FILEH_INVALID) {
		goto dtrd_err1;
	}
	if ((file_seek(fh, (long)fptr, FSEEK_SET) != (long)fptr) ||
		(file_read(fh, trk->buf, size) != size)) {
		goto dtrd_err2;
	}
	file_close(fh);

	trk->fdd = fdd;
	trk->track = track;
	trk->type = type;
	trk->fptr = fptr;
	trk->size = size;
	trk->write = FALSE;
	return(SUCCESS);

dtrd_err2:
	file_close(fh);

dtrd_err1:
	return(FAILURE);
}


static BRESULT rpmcheck(D88SEC sec) {

	FDDFILE	fdd = fddfile + fdc.us;
	UINT8	rpm;

	rpm = fdc.rpm[fdc.us];
	switch(fdd->inf.d88.fdtype_major) {
		case DISKTYPE_2D:
		case DISKTYPE_2DD:
			if (rpm) {
				return(FAILURE);
			}
			break;

		case DISKTYPE_2HD:
			if (fdd->inf.d88.fdtype_minor == 0) {
				if (rpm) {
					return(FAILURE);
				}
			}
			else {
				if (sec->rpm_flg != rpm) {
					return(FAILURE);
				}
			}
			break;

		default:
			return(FAILURE);
	}
	return(SUCCESS);
}


// ----

static void drvflush(FDDFILE fdd) {

	D88TRK	trk;

	trk = &d88trk;
	if (trk->fdd == fdd) {
		d88trk_flushdata(trk);
		trk->fdd = NULL;
	}
}

static BRESULT trkseek(FDDFILE fdd, UINT track) {

	D88TRK	trk;
	BRESULT	r;

	trk = &d88trk;
	if ((trk->fdd == fdd) && (trk->track == track) &&
		(trk->type == CTRL_FDMEDIA)) {
		r = SUCCESS;
	}
	else {
		r = d88trk_read(trk, fdd, track, CTRL_FDMEDIA);
	}
	return(r);
}


static D88SEC searchsector_d88(BOOL check) {			// ver0.29

	UINT8	*p;
	UINT	sec;
	UINT	pos = 0;
	UINT	nsize;
	UINT	sectors;
	UINT	secsize;

	if (fdc.N < 8) {
		nsize = 128 << fdc.N;
	}
	else {
		nsize = 128 << 8;
	}

	p = d88trk.buf;
	for (sec=0; sec<40; ) {
		if ((pos + nsize + sizeof(_D88SEC)) > D88BUFSIZE) {
			break;
		}

		if ((((D88SEC)p)->c == fdc.C) &&
			(((D88SEC)p)->h == fdc.H) &&
			(((D88SEC)p)->r == fdc.R) &&
			(((D88SEC)p)->n == fdc.N) &&
			(!rpmcheck((D88SEC)p))) {

			// ver0.29
			if (check) {
				if ((fdc.mf != 0xff) &&
					!((fdc.mf ^ (((D88SEC)p)->mfm_flg)) & 0x40)) {
					break;
				}
			}
			return((D88SEC)p);
		}
		sectors = LOADINTELWORD(((D88SEC)p)->sectors);
		if (++sec >= sectors) {
			break;
		}

		secsize = LOADINTELWORD(((D88SEC)p)->size);
		secsize += sizeof(_D88SEC);
		pos += secsize;
		p += secsize;
	}
	return(NULL);
}


// ----

BRESULT fddd88_set(FDDFILE fdd, const OEMCHAR *fname, int ro) {

	short	attr;
	FILEH	fh;
	UINT	rsize;
	int		i;

	fddd88_eject(fdd);
	attr = file_attr(fname);
	if (attr & 0x18) {
		goto fdst_err;
	}
	if(attr & FILEATTR_READONLY) {
		ro = 1;
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
		goto fdst_err;
	}
	rsize = file_read(fh, &fdd->inf.d88.head, sizeof(fdd->inf.d88.head));
	file_close(fh);
	if (rsize != sizeof(fdd->inf.d88.head)) {
		goto fdst_err;
	}
	fdd->type = DISKTYPE_D88;
	fdd->protect = ((attr & 1) || (fdd->inf.d88.head.protect & 0x10) ||
															(ro))?TRUE:FALSE;
	fdd->inf.d88.fdtype_major = fdd->inf.d88.head.fd_type >> 4;
	fdd->inf.d88.fdtype_minor = fdd->inf.d88.head.fd_type & 0x0f;
	fdd->inf.d88.fd_size = LOADINTELDWORD(fdd->inf.d88.head.fd_size);
	for (i=0; i<164; i++) {
		fdd->inf.d88.ptr[i] = LOADINTELDWORD(fdd->inf.d88.head.trackp[i]);
	}
	return(SUCCESS);

fdst_err:
	return(FAILURE);
}

BRESULT fddd88_eject(FDDFILE fdd) {

	drvflush(fdd);
	fdd->fname[0] = '\0';
	fdd->type = DISKTYPE_NOTREADY;
	ZeroMemory(&fdd->inf.d88.head, sizeof(fdd->inf.d88.head));
	return(SUCCESS);
}


BRESULT fdd_diskaccess_d88(void) {									// ver0.31

	FDDFILE	fdd = fddfile + fdc.us;
	UINT8	rpm;

	rpm = fdc.rpm[fdc.us];
	switch(fdd->inf.d88.fdtype_major) {
		case DISKTYPE_2D:
		case DISKTYPE_2DD:
			if ((rpm) || (CTRL_FDMEDIA != DISKTYPE_2DD)) {
				return(FAILURE);
			}
			break;

		case DISKTYPE_2HD:
			if (CTRL_FDMEDIA != DISKTYPE_2HD) {
				return(FAILURE);
			}
			if ((fdd->inf.d88.fdtype_minor == 0) && (rpm)) {
				return(FAILURE);
			}
			break;

		default:
			return(FAILURE);

	}
	return(SUCCESS);
}

BRESULT fdd_seek_d88(void) {

	FDDFILE	fdd = fddfile + fdc.us;

	return(trkseek(fdd, (fdc.ncn << 1) + fdc.hd));
}

BRESULT fdd_seeksector_d88(void) {

	FDDFILE	fdd = fddfile + fdc.us;

	if (trkseek(fdd, (fdc.treg[fdc.us] << 1) + fdc.hd)) {
		return(FAILURE);
	}
	if (!searchsector_d88(FALSE)) {
		return(FAILURE);
	}
	return(SUCCESS);
}

BRESULT fdd_read_d88(void) {

	FDDFILE		fdd = fddfile + fdc.us;
	D88SEC		p;
	UINT		size;
	UINT		secsize;

	fddlasterror = 0x00;
	if (trkseek(fdd, (fdc.treg[fdc.us] << 1) + fdc.hd)) {
		fddlasterror = 0xe0;
		return(FAILURE);
	}
	p = searchsector_d88(TRUE);
	if (!p) {
		fddlasterror = 0xc0;
		return(FAILURE);
	}
	if (fdc.N < 8) {
		size = 128 << fdc.N;
	}
	else {
		size = 128 << 8;
	}
	fdc.bufcnt = size;
	ZeroMemory(fdc.buf, size);
	secsize = LOADINTELWORD(p->size);
	if (size > secsize) {
		size = secsize;
	}
	if (size) {
		CopyMemory(fdc.buf, p+1, size);
	}
	fddlasterror = p->stat;
	return(SUCCESS);
}

BRESULT fdd_write_d88(void) {

	FDDFILE		fdd = fddfile + fdc.us;
	D88SEC		p;
	UINT		size;
	UINT		secsize;

	fddlasterror = 0x00;
	if (trkseek(fdd, (fdc.treg[fdc.us] << 1) + fdc.hd)) {
		fddlasterror = 0xe0;
		return(FAILURE);
	}
	p = searchsector_d88(FALSE);
	if (!p) {
		fddlasterror = 0xc0;
		return(FAILURE);
	}
	if (fdc.N < 8) {
		size = 128 << fdc.N;
	}
	else {
		size = 128 << 8;
	}
	secsize = LOADINTELWORD(p->size);
	if (size > secsize) {
		size = secsize;
	}
	if (size) {
		CopyMemory(p+1, fdc.buf, size);
		d88trk.write = TRUE;
	}
	fddlasterror = 0x00;
	return(SUCCESS);
}

BRESULT fdd_readid_d88(void) {

	FDDFILE	fdd = fddfile + fdc.us;
	UINT8	*p;
	UINT	sec;
	UINT	pos = 0;
	UINT	sectors;
	UINT	secsize;

	fddlasterror = 0x00;
	if (trkseek(fdd, (fdc.treg[fdc.us] << 1) + fdc.hd)) {
		fddlasterror = 0xe0;
		return(FAILURE);
	}
	p = d88trk.buf;
	for (sec=0; sec<40; ) {
		if (pos > (D88BUFSIZE - sizeof(_D88SEC))) {
			break;
		}
		sectors = LOADINTELWORD(((D88SEC)p)->sectors);
		if ((sec == fdc.crcn) && (!rpmcheck((D88SEC)p))) {			// ver0.31
			fdc.C = ((D88SEC)p)->c;
			fdc.H = ((D88SEC)p)->h;
			fdc.R = ((D88SEC)p)->r;
			fdc.N = ((D88SEC)p)->n;
			fdc.crcn++;
			if (fdc.crcn >= sectors) {
				fdc.crcn = 0;
			}
			if ((fdc.mf == 0xff) ||
					((fdc.mf ^ (((D88SEC)p)->mfm_flg)) & 0x40)) {
				fddlasterror = 0x00;
				return(SUCCESS);
			}
		}
		if (++sec >= sectors) {
			break;
		}
		secsize = LOADINTELWORD(((D88SEC)p)->size);
		secsize += sizeof(_D88SEC);
		pos += secsize;
		p += secsize;
	}
	fdc.crcn = 0x00;
	fddlasterror = 0xe0;											// ver0.31
	return(FAILURE);
}


// --------------------------------------------------------------------------

// えーと…こんなところにあって大丈夫？
static BOOL formating = FALSE;
static UINT8 formatsec = 0;
static UINT8 formatwrt = 0;
static UINT formatpos = 0;

static int fileappend(FILEH hdl, FDDFILE fdd, UINT ptr, UINT last, int apsize) {

	int		length;
	UINT	size;
	UINT	rsize;
	int		t;
	UINT8	tmp[0x400];							// Stack 0x1000->0x400
	UINT	cur;

	if (fdd->protect) {
		return(0);
	}

	if ((length = last - ptr) <= 0) {			// 書き換える必要なし
		return(0);
	}
	while(length) {
		if (length >= sizeof(tmp)) {
			size = sizeof(tmp);
		}
		else {
			size = length;
		}
		length -= size;
		file_seek(hdl, ptr + length, 0);
		rsize = file_read(hdl, tmp, size);
		file_seek(hdl, ptr + length + apsize, 0);
		file_write(hdl, tmp, rsize);
	}

	for (t=0; t<164; t++) {
		cur = fdd->inf.d88.ptr[t];
		if ((cur != 0) && (cur >= ptr)) {
			cur += apsize;
			fdd->inf.d88.ptr[t] = cur;
			STOREINTELDWORD(fdd->inf.d88.head.trackp[t], cur);
		}
	}
	return(0);
}


static void endoftrack(UINT fmtsize, UINT8 sectors) {

	FDDFILE	fdd = fddfile + fdc.us;

	D88SEC	d88sec;
	FILEH	hdl;
	int		i;
	UINT	trk;
	UINT	fpointer;
	UINT	endpointer;
	UINT	lastpointer;
	UINT	trksize;
	int		ptr;
	int		apsize;

	if (fdd->protect) {
		return;
	}

	trk = (fdc.treg[fdc.us] << 1) + fdc.hd;

	ptr = 0;
	for (i=0; i<(int)sectors; i++) {
		d88sec = (D88SEC)(d88trk.buf + ptr);
		STOREINTELWORD(d88sec->sectors, sectors);
		ptr += LOADINTELWORD(d88sec->size);
		ptr += sizeof(_D88SEC);
	}

	hdl = file_open(fddfile[fdc.us].fname);
	if (hdl == FILEH_INVALID) {
		return;
	}
	lastpointer = file_getsize(hdl);
	fpointer = fdd->inf.d88.ptr[trk];
	if (fpointer == 0) {
		for (i=trk; i>=0; i--) {					// 新規トラック
			fpointer = fdd->inf.d88.ptr[i];
			if (fpointer) {
				break;
			}
		}
		if (fpointer) {								// ヒットした
			fpointer = nexttrackptr(fdd, fpointer, lastpointer);
		}
		else {
			fpointer = sizeof(_D88HEAD);
		}
		endpointer = fpointer;
	}
	else {										// トラックデータは既にある
		endpointer = nexttrackptr(fdd, fpointer, lastpointer);
	}
	trksize = endpointer - fpointer;
	if ((apsize = fmtsize - trksize) > 0) {
								// 書き込むデータのほーが大きい
		fileappend(hdl, fdd, endpointer, lastpointer, apsize);
		fdd->inf.d88.fd_size += apsize;
		STOREINTELDWORD(fdd->inf.d88.head.fd_size, fdd->inf.d88.fd_size);
	}
	fdd->inf.d88.ptr[trk] = fpointer;
	STOREINTELDWORD(fdd->inf.d88.head.trackp[trk], fpointer);
	file_seek(hdl, (long)fpointer, 0);
	file_write(hdl, d88trk.buf, fmtsize);
	file_seek(hdl, 0, FSEEK_SET);
	file_write(hdl, &fdd->inf.d88.head, sizeof(fdd->inf.d88.head));
	file_close(hdl);
//	TRACEOUT(("fmt %d %d", fpointer, fmtsize));
}


BRESULT fdd_formatinit_d88(void) {

	if (fdc.treg[fdc.us] < 82) {
		formating = TRUE;
		formatsec = 0;
		formatpos = 0;
		formatwrt = 0;
		drvflush(fddfile + fdc.us);
		return(SUCCESS);
	}
	return(FAILURE);
}

	// todo アンフォーマットとか ディスク１周した時の切り捨てとか…
BRESULT fdd_formating_d88(const UINT8 *ID) {

	FDDFILE	fdd = fddfile + fdc.us;

	UINT	size;
	D88SEC	d88sec;

	if (!formating) {
		return(FAILURE);
	}
	if (fdc.N < 8) {
		size = 128 << fdc.N;
	}
	else {
		size = 128 << 8;
	}
	if ((formatpos + sizeof(_D88SEC) + size) < D88TRACKMAX) {
		d88sec = (D88SEC)(d88trk.buf + formatpos);
		ZeroMemory(d88sec, sizeof(_D88SEC));
		d88sec->c = ID[0];
		d88sec->h = ID[1];
		d88sec->r = ID[2];
		d88sec->n = ID[3];
		STOREINTELWORD(d88sec->size, size);
		if ((fdd->inf.d88.fdtype_major == DISKTYPE_2HD) &&
			(fdd->inf.d88.fdtype_minor != 0)) {
			d88sec->rpm_flg = fdc.rpm[fdc.us];
		}
		FillMemory(d88sec + 1, size, fdc.d);
		formatpos += sizeof(_D88SEC);
		formatpos += size;
		formatwrt++;
	}
	formatsec++;
//	TRACE_("format sec", formatsec);
//	TRACE_("format wrt", formatwrt);
//	TRACE_("format max", fdc.sc);
//	TRACE_("format pos", formatpos);
	if (formatsec >= fdc.sc) {
		endoftrack(formatpos, formatwrt);
		formating = FALSE;
	}
	return(SUCCESS);
}

BOOL fdd_isformating_d88(void) {

	return(formating);
}



#endif
