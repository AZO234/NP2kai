#include	"compiler.h"

#if defined(SUPPORT_HOSTDRV)

/*
	ゲストOS(DOS)からホストOS(Win)にアクセスするの〜
	完全にDOS(3.1以上)依存だお(汗
	ネットワークインタフェイス搭載前の繋ぎだけど
	更に、手抜き版だし(マテ
*/

#include	"dosio.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"hostdrv.h"
#include	"hostdrvs.h"
#include	"hostdrv.tbl"


#define IS_PERMITWRITE		(np2cfg.hdrvacc & HDFMODE_WRITE)
#define IS_PERMITDELETE		(np2cfg.hdrvacc & HDFMODE_DELETE)

#define ROOTPATH_NAME		"\\\\HOSTDRV\\"
#define ROOTPATH_SIZE		(sizeof(ROOTPATH_NAME) - 1)

static const char ROOTPATH[ROOTPATH_SIZE + 1] = ROOTPATH_NAME;
static const HDRVFILE hdd_volume = {{'_','H','O','S','T','D','R','I','V','E','_'}, 0, 0, 0x08, {0}, {0}};

	HOSTDRV		hostdrv;

// ---- i/f

static void succeed(INTRST intrst) {

	intrst->r.b.flag_l &= ~C_FLAG;
	STOREINTELWORD(intrst->r.w.ax, ERR_NOERROR);
}

static void fail(INTRST intrst, UINT16 err_code) {

	intrst->r.b.flag_l |= C_FLAG;
	STOREINTELWORD(intrst->r.w.ax, err_code);
}


static void fetch_if4dos(void) {

	REG16	off;
	REG16	seg;
	IF4DOS	if4dos;

	off = MEMR_READ16(IF4DOSPTR_SEG, IF4DOSPTR_OFF + 0);
	seg = MEMR_READ16(IF4DOSPTR_SEG, IF4DOSPTR_OFF + 2);
	MEMR_READS(seg, off, &if4dos, sizeof(if4dos));
	hostdrv.stat.drive_no = if4dos.drive_no;
	hostdrv.stat.dosver_major = if4dos.dosver_major;
	hostdrv.stat.dosver_minor = if4dos.dosver_minor;
	hostdrv.stat.sda_off = LOADINTELWORD(if4dos.sda_off);
	hostdrv.stat.sda_seg = LOADINTELWORD(if4dos.sda_seg);

	TRACEOUT(("hostdrv:drive_no = %d", if4dos.drive_no));
	TRACEOUT(("hostdrv:dosver = %d.%.2d", if4dos.dosver_major, if4dos.dosver_minor));
	TRACEOUT(("hostdrv.sda = %.4x:%.4x", hostdrv.stat.sda_seg, hostdrv.stat.sda_off));
}


static void fetch_intr_regs(INTRST is) {

	MEMR_READS(CPU_SS, CPU_BP, &is->r, sizeof(is->r));
}

static void store_intr_regs(INTRST is) {

	MEMR_WRITES(CPU_SS, CPU_BP, &is->r, sizeof(is->r));
}


static void fetch_sda_currcds(SDACDS sc) {

	REG16	off;
	REG16	seg;

	if (hostdrv.stat.dosver_major == 3) {
		MEMR_READS(hostdrv.stat.sda_seg, hostdrv.stat.sda_off,
										&sc->ver3.sda, sizeof(sc->ver3.sda));
		off = LOADINTELWORD(sc->ver3.sda.cdsptr.off);
		seg = LOADINTELWORD(sc->ver3.sda.cdsptr.seg);
		MEMR_READS(seg, off, &sc->ver3.cds, sizeof(sc->ver3.cds));
	}
	else {
		MEMR_READS(hostdrv.stat.sda_seg, hostdrv.stat.sda_off,
										&sc->ver4.sda, sizeof(sc->ver4.sda));
		off = LOADINTELWORD(sc->ver4.sda.cdsptr.off);
		seg = LOADINTELWORD(sc->ver4.sda.cdsptr.seg);
		MEMR_READS(seg, off, &sc->ver4.cds, sizeof(sc->ver4.cds));
	}
}

static void store_sda_currcds(SDACDS sc) {

	REG16	off;
	REG16	seg;

	if (hostdrv.stat.dosver_major == 3) {
		MEMR_WRITES(hostdrv.stat.sda_seg, hostdrv.stat.sda_off,
										&sc->ver3.sda, sizeof(sc->ver3.sda));
		off = LOADINTELWORD(sc->ver3.sda.cdsptr.off);
		seg = LOADINTELWORD(sc->ver3.sda.cdsptr.seg);
		MEMR_WRITES(seg, off, &sc->ver3.cds, sizeof(sc->ver3.cds));
	}
	else {
		MEMR_WRITES(hostdrv.stat.sda_seg, hostdrv.stat.sda_off,
										&sc->ver4.sda, sizeof(sc->ver4.sda));
		off = LOADINTELWORD(sc->ver4.sda.cdsptr.off);
		seg = LOADINTELWORD(sc->ver4.sda.cdsptr.seg);
		MEMR_WRITES(seg, off, &sc->ver4.cds, sizeof(sc->ver4.cds));
	}
}


static void fetch_sft(INTRST is, SFTREC sft) {

	REG16	off;
	REG16	seg;

	off = LOADINTELWORD(is->r.w.di);
	seg = LOADINTELWORD(is->r.w.es);
	MEMR_READS(seg, off, sft, sizeof(_SFTREC));
}

static void store_sft(INTRST is, SFTREC sft) {

	REG16	off;
	REG16	seg;

	off = LOADINTELWORD(is->r.w.di);
	seg = LOADINTELWORD(is->r.w.es);
	MEMR_WRITES(seg, off, sft, sizeof(_SFTREC));
}


static void store_srch(INTRST is) {

	SRCHREC	srchrec;

	// SDA内のSRCHRECにセット
	srchrec = is->srchrec_ptr;
	srchrec->drive_no = 0xc0 | hostdrv.stat.drive_no;
	CopyMemory(srchrec->srch_mask, is->fcbname_ptr, 11);
	srchrec->attr_mask = *is->srch_attr_ptr;
	STOREINTELWORD(srchrec->dir_entry_no, ((UINT16)-1));
	STOREINTELWORD(srchrec->dir_sector, ((UINT16)-1));
}

static void store_dir(INTRST is, const HDRVFILE *phdf) {

	DIRREC	dirrec;
	UINT8	attr;
	UINT16	reg;

	// SDA内のDIRRECにセット
	dirrec = is->dirrec_ptr;
	CopyMemory(dirrec->file_name, phdf->fcbname, 11);
	attr = (UINT8)(phdf->attr & 0x3f);
	if (!IS_PERMITWRITE) {
		attr |= 0x01;
	}
	dirrec->file_attr = attr;
	reg = 0;
	if (phdf->caps & FLICAPS_TIME) {
		reg |= (phdf->time.hour & 0x1f) << 11;
		reg |= (phdf->time.minute & 0x3f) << 5;
		reg |= (phdf->time.second & 0x3e) >> 1;
	}
	STOREINTELWORD(dirrec->file_time, reg);
	reg = 0;
	if (phdf->caps & FLICAPS_DATE) {
		reg |= ((phdf->date.year - 1980) & 0x7f) << 9;
		reg |= (phdf->date.month & 0x0f) << 5;
		reg |= phdf->date.day & 0x1f;
	}
	STOREINTELWORD(dirrec->file_date, reg);
	STOREINTELWORD(dirrec->start_sector, ((UINT16)-1));
	STOREINTELDWORD(dirrec->file_size, phdf->size);
}

static void fill_sft(INTRST is, SFTREC sft, UINT num, const HDRVFILE *phdf) {

	UINT8	attr;
	UINT16	reg;

	attr = phdf->attr;
	if (!IS_PERMITWRITE) {
		attr |= 0x01;
	}
	sft->file_attr = attr;
	STOREINTELWORD(sft->start_sector, (UINT16)num);

	reg = 0;
	if (phdf->caps & FLICAPS_TIME) {
		reg |= (phdf->time.hour & 0x1f) << 11;
		reg |= (phdf->time.minute & 0x3f) << 5;
		reg |= (phdf->time.second & 0x3e) >> 1;
	}
	STOREINTELWORD(sft->file_time, reg);
	reg = 0;
	if (phdf->caps & FLICAPS_DATE) {
		reg |= ((phdf->date.year - 1980) & 0x7f) << 9;
		reg |= (phdf->date.month & 0x0f) << 5;
		reg |= phdf->date.day & 0x1f;
	}
	STOREINTELWORD(sft->file_date, reg);
	STOREINTELDWORD(sft->file_size, phdf->size);
	STOREINTELWORD(sft->dir_sector, (UINT16)-1);
	sft->dir_entry_no = (UINT8)-1;
	CopyMemory(sft->file_name, is->fcbname_ptr, 11);
	TRACEOUT(("open -> size %d", phdf->size));
}

static void init_sft(SFTREC sft) {

	if (sft->open_mode[1] & 0x80) {	// fcb mode
		sft->open_mode[0] |= 0xf0;
	}
	else {
		sft->open_mode[0] &= 0x0f;
	}
	sft->dev_info_word[0] = (UINT8)(0x40 | hostdrv.stat.drive_no);
	sft->dev_info_word[1] = 0x80;
	STOREINTELDWORD(sft->dev_drvr_ptr, 0);
	STOREINTELDWORD(sft->file_pos, 0);
	STOREINTELWORD(sft->rel_sector, (UINT16)-1);
	STOREINTELWORD(sft->abs_sector, (UINT16)-1);
	if (sft->open_mode[1] & 0x80) {	// fcb mode
		CPU_FLAG |= C_FLAG;
	}
}


static BOOL is_wildcards(const char *path) {

	int		i;

	for (i=0; i<11; i++) {
		if (path[i] == '?') {
			return(TRUE);
		}
	}
	return(FALSE);
}


// ぽいんた初期化
static void setup_ptrs(INTRST is, SDACDS sc) {

	char	*rootpath;
	int		off;

	if (hostdrv.stat.dosver_major == 3) {
		is->fcbname_ptr = sc->ver3.sda.fcb_name;
		is->filename_ptr = sc->ver3.sda.file_name + ROOTPATH_SIZE - 1;
		is->fcbname_ptr_2 = sc->ver3.sda.fcb_name_2;
		is->filename_ptr_2 = sc->ver3.sda.file_name_2 + ROOTPATH_SIZE - 1;

		is->srchrec_ptr = &sc->ver3.sda.srchrec;
		is->dirrec_ptr = &sc->ver3.sda.dirrec;
		is->srchrec_ptr_2 = &sc->ver3.sda.rename_srchrec;
		is->dirrec_ptr_2 = &sc->ver3.sda.rename_dirrec;
		is->srch_attr_ptr = &sc->ver3.sda.srch_attr;

		rootpath = sc->ver3.cds.current_path;
		off = LOADINTELWORD(sc->ver3.cds.root_ofs);
		is->root_path = rootpath;
		is->current_path = rootpath + off;
	}
	else {
		is->fcbname_ptr = sc->ver4.sda.fcb_name;
		is->filename_ptr = sc->ver4.sda.file_name + ROOTPATH_SIZE - 1;
		is->fcbname_ptr_2 = sc->ver4.sda.fcb_name_2;
		is->filename_ptr_2 = sc->ver4.sda.file_name_2 + ROOTPATH_SIZE - 1;

		is->srchrec_ptr = &sc->ver4.sda.srchrec;
		is->dirrec_ptr = &sc->ver4.sda.dirrec;
		is->srchrec_ptr_2 = &sc->ver4.sda.rename_srchrec;
		is->dirrec_ptr_2 = &sc->ver4.sda.rename_dirrec;
		is->srch_attr_ptr = &sc->ver4.sda.srch_attr;

		rootpath = sc->ver4.cds.current_path;
		off = LOADINTELWORD(sc->ver4.cds.root_ofs);
		is->root_path = rootpath;
		is->current_path = rootpath + off;
	}
}


static BRESULT pathishostdrv(INTRST is, SDACDS sc) {

	fetch_sda_currcds(sc);
	setup_ptrs(is, sc);

	if (memcmp(is->root_path, ROOTPATH, ROOTPATH_SIZE)) {
		CPU_FLAG &= ~Z_FLAG;	// chain
		return(FAILURE);
	}
	if (is->is_chardev) {
		fail(is, ERR_ACCESSDENIED);
		return(FAILURE);
	}
	return(SUCCESS);
}


static BRESULT read_data(UINT num, UINT32 pos, UINT size, UINT seg, UINT off) {

	HDRVHANDLE	hdf;
	FILEH		fh;
	UINT8		work[1024];
	UINT		r;

	hdf = (HDRVHANDLE)listarray_getitem(hostdrv.fhdl, num);
	if (hdf == NULL) {
		return(FAILURE);
	}
	fh = (FILEH)hdf->hdl;
	if (file_seek(fh, (long)pos, FSEEK_SET) != (long)pos) {
		return(FAILURE);
	}
	while(size) {
		r = MIN(size, sizeof(work));
		if (file_read(fh, work, r) != r) {
			return(FAILURE);
		}
		MEMR_WRITES(seg, off, work, r);
		off += r;
		size -= r;
	}
	return(SUCCESS);
}

static BRESULT write_data(UINT num, UINT32 pos, UINT size, UINT seg, UINT off) {

	HDRVHANDLE	hdf;
	FILEH		fh;
	UINT8		work[1024];
	UINT		r;

	hdf = (HDRVHANDLE)listarray_getitem(hostdrv.fhdl, num);
	if (hdf == NULL) {
		return(FAILURE);
	}
	fh = (FILEH)hdf->hdl;
	if (file_seek(fh, (long)pos, FSEEK_SET) != (long)pos) {
		return(FAILURE);
	}
	if (!size) {
		file_write(fh, work, 0);
	}
	else {
		do {
			r = MIN(size, sizeof(work));
			MEMR_READS(seg, off, work, r);
			if (file_write(fh, work, r) != r) {
				return(FAILURE);
			}
			off += r;
			size -= r;
		} while(size);
	}
	return(SUCCESS);
}


static BRESULT find_file(INTRST is)
{
	HDRVLST hdl;

	hdl = (HDRVLST)listarray_getitem(hostdrv.flist, hostdrv.stat.flistpos);
	if (hdl != NULL)
	{
		store_srch(is);
		store_dir(is, &hdl->file);
		hostdrv.stat.flistpos++;
		return SUCCESS;
	}
	else
	{
		listarray_destroy(hostdrv.flist);
		hostdrv.flist = NULL;
		return FAILURE;
	}
}


// ----

/* cmd in int2f11 */
/* 00 */
static void inst_check(INTRST intrst) {

	intrst->r.b.flag_l &= ~C_FLAG;
	intrst->r.b.al = 0xff;					// インストール済み。追加OKだお
}

/* 01 */
static void remove_dir(INTRST intrst)
{
	_SDACDS sc;
	UINT nResult;
	HDRVPATH hdp;

	if (pathishostdrv(intrst, &sc) != SUCCESS)
	{
		return;
	}

	nResult = ERR_NOERROR;
	do
	{
		if (is_wildcards(intrst->fcbname_ptr))
		{
			nResult = ERR_PATHNOTFOUND;
			break;
		}

		if (hostdrvs_getrealpath(&hdp, intrst->filename_ptr) != ERR_NOERROR)
		{
			nResult = ERR_PATHNOTFOUND;
			break;
		}
		if ((hdp.file.attr & 0x10) == 0)
		{
			nResult = ERR_ACCESSDENIED;
			break;
		}
		TRACEOUT(("remove_dir: %s -> %s", intrst->filename_ptr, hdp.szPath));

		if (!IS_PERMITDELETE)
		{
			nResult = ERR_ACCESSDENIED;
			break;
		}

		if (file_dirdelete(hdp.szPath))
		{
			nResult = ERR_ACCESSDENIED;
			break;
		}
		succeed(intrst);
		return;
	} while (FALSE /*CONSTCOND*/);

	fail(intrst, (UINT16)nResult);
}

/* 03 */
static void make_dir(INTRST intrst)
{
	_SDACDS sc;
	UINT nResult;
	HDRVPATH hdp;

	if (pathishostdrv(intrst, &sc) != SUCCESS)
	{
		return;
	}

	nResult = ERR_NOERROR;
	do
	{
		if (is_wildcards(intrst->fcbname_ptr))
		{
			nResult = ERR_PATHNOTFOUND;
			break;
		}

		nResult = hostdrvs_getrealpath(&hdp, intrst->filename_ptr);
		if (nResult == ERR_NOERROR)
		{
			nResult = ERR_ACCESSDENIED;
			break;
		}
		else if (nResult != ERR_FILENOTFOUND)
		{
			break;
		}
		TRACEOUT(("make_dir: %s -> %s", intrst->filename_ptr, hdp.szPath));

		if (!IS_PERMITWRITE)
		{
			nResult = ERR_ACCESSDENIED;
			break;
		}

		if (file_dircreate(hdp.szPath))
		{
			nResult = ERR_ACCESSDENIED;
			break;
		}
		nResult = ERR_NOERROR;
	} while (FALSE /*CONSTCOND*/);

	if (nResult == ERR_NOERROR)
	{
		succeed(intrst);
	}
	else
	{
		fail(intrst, (UINT16)nResult);
	}
}

/* 05 */
static void change_currdir(INTRST intrst) {

	_SDACDS		sc;
	char		*ptr;
	HDRVPATH	hdp;

	if (pathishostdrv(intrst, &sc) != SUCCESS) {
		return;
	}

	ptr = intrst->filename_ptr;
	TRACEOUT(("change_currdir %s", intrst->filename_ptr));
	if (ptr[0] == '\0') {							// るーと
		strcpy(intrst->filename_ptr, "\\");
		strcpy(intrst->current_path, intrst->filename_ptr);
		store_sda_currcds(&sc);
		succeed(intrst);
		return;
	}
	if ((strlen(intrst->filename_ptr) >= (67 - ROOTPATH_SIZE)) ||
		(is_wildcards(intrst->fcbname_ptr) != FALSE) ||
		(hostdrvs_getrealpath(&hdp, ptr) != ERR_NOERROR) ||
		(hdp.file.fcbname[0] == ' ') || (!(hdp.file.attr & 0x10))) {
		fail(intrst, ERR_PATHNOTFOUND);
		return;
	}
	strcpy(intrst->current_path, intrst->filename_ptr);
	store_sda_currcds(&sc);
	succeed(intrst);
}

/* 06 */
static void close_file(INTRST intrst) {

	_SDACDS		sc;
	_SFTREC		sft;
	UINT16		handle_count;
	UINT16		start_sector;
	HDRVHANDLE	hdf;

	fetch_sda_currcds(&sc);
	fetch_sft(intrst, &sft);
	setup_ptrs(intrst, &sc);

	if ((sft.dev_info_word[0] & 0x3f) != hostdrv.stat.drive_no) {
		CPU_FLAG &= ~Z_FLAG;	// chain
		return;
	}
	handle_count = LOADINTELWORD(sft.handle_count);

	if (handle_count) {
		handle_count--;
	}
	if (handle_count == 0) {
		start_sector = LOADINTELWORD(sft.start_sector);
		hdf = (HDRVHANDLE)listarray_getitem(hostdrv.fhdl, start_sector);
		if (hdf) {
			file_close((FILEH)hdf->hdl);
			hdf->hdl = (INTPTR)FILEH_INVALID;
			hdf->path[0] = '\0';
		}
	}
	STOREINTELWORD(sft.handle_count, handle_count);
	store_sft(intrst, &sft);
	succeed(intrst);
}

/* 07 */
static void commit_file(INTRST intrst) {

	_SDACDS		sc;
	_SFTREC		sft;

	fetch_sda_currcds(&sc);
	fetch_sft(intrst, &sft);

	if ((sft.dev_info_word[0] & 0x3f) != hostdrv.stat.drive_no) {
		CPU_FLAG &= ~Z_FLAG;	// chain
		return;
	}

	// なんもしないよー
	succeed(intrst);
}

/* 08 */
static void read_file(INTRST intrst) {

	_SDACDS		sc;
	_SFTREC		sft;
	UINT16		cx;
	UINT		file_size;
	UINT32		file_pos;

	fetch_sda_currcds(&sc);
	fetch_sft(intrst, &sft);
	setup_ptrs(intrst, &sc);

	if ((sft.dev_info_word[0] & 0x3f) != hostdrv.stat.drive_no) {
		CPU_FLAG &= ~Z_FLAG;	// chain
		return;
	}
	if (sft.open_mode[0] & 1) {
		fail(intrst, ERR_ACCESSDENIED);
		return;
	}

	cx = LOADINTELWORD(intrst->r.w.cx);
	file_size = LOADINTELDWORD(sft.file_size);
	file_pos = LOADINTELDWORD(sft.file_pos);
	if (cx > (file_size - file_pos)) {
		cx = (UINT16)(file_size - file_pos);
		STOREINTELWORD(intrst->r.w.cx, cx);
	}
	if (cx == 0) {
		succeed(intrst);
		return;
	}
	if (read_data(LOADINTELWORD(sft.start_sector), file_pos, cx,
					LOADINTELWORD(sc.ver3.sda.current_dta.seg),
					LOADINTELWORD(sc.ver3.sda.current_dta.off)) != SUCCESS) {
		fail(intrst, ERR_READFAULT);
		return;
	}

	file_pos += cx;
	STOREINTELDWORD(sft.file_pos, file_pos);

	store_sft(intrst, &sft);
//	store_sda_currcds(&sc);						// ver0.74 Yui / sdaは変更無し
	succeed(intrst);
}

/* 09 */
static void write_file(INTRST intrst) {

	_SDACDS		sc;
	_SFTREC		sft;
	UINT16		cx;
	UINT		file_size;
	UINT32		file_pos;

	fetch_sda_currcds(&sc);
	fetch_sft(intrst, &sft);
	setup_ptrs(intrst, &sc);

	if ((sft.dev_info_word[0] & 0x3f) != hostdrv.stat.drive_no) {
		CPU_FLAG &= ~Z_FLAG;	// chain
		return;
	}

	if ((!IS_PERMITWRITE) ||
		(!(sft.open_mode[0] & 3))) {	// read only
		fail(intrst, ERR_ACCESSDENIED);
		return;
	}

	cx = LOADINTELWORD(intrst->r.w.cx);
	file_size = LOADINTELDWORD(sft.file_size);
	file_pos = LOADINTELDWORD(sft.file_pos);
	if (write_data(LOADINTELWORD(sft.start_sector), file_pos, cx,
					LOADINTELWORD(sc.ver3.sda.current_dta.seg),
					LOADINTELWORD(sc.ver3.sda.current_dta.off)) != SUCCESS) {
		fail(intrst, ERR_WRITEFAULT);
		return;
	}
	if (cx) {
		file_pos += cx;
		if (file_size < file_pos) {
			file_size = file_pos;
		}
	}
	else {
		file_size = file_pos;
	}

	STOREINTELDWORD(sft.file_size, file_size);
	STOREINTELDWORD(sft.file_pos, file_pos);
	store_sft(intrst, &sft);
	succeed(intrst);
}

/* 0A */
static void lock_file(INTRST intrst) {

	_SDACDS		sc;
	_SFTREC		sft;

	fetch_sda_currcds(&sc);
	fetch_sft(intrst, &sft);

	if ((sft.dev_info_word[0] & 0x3f) != hostdrv.stat.drive_no) {
		CPU_FLAG &= ~Z_FLAG;	// chain
		return;
	}
	// 未実装
	TRACEOUT(("hostdrv: lock_file"));
}

/* 0B */
static void unlock_file(INTRST intrst) {

	_SDACDS		sc;
	_SFTREC		sft;

	fetch_sda_currcds(&sc);
	fetch_sft(intrst, &sft);

	if ((sft.dev_info_word[0] & 0x3f) != hostdrv.stat.drive_no) {
		CPU_FLAG &= ~Z_FLAG;	// chain
		return;
	}
	// 未実装
	TRACEOUT(("hostdrv: unlock_file"));
}

/* 0C */
static void get_diskspace(INTRST intrst)
{
	_SDACDS sc;

	if (pathishostdrv(intrst, &sc) != SUCCESS)
	{
		return;
	}

	intrst->r.b.flag_l &= ~C_FLAG;
	STOREINTELWORD(intrst->r.w.ax, 0xf840);
	STOREINTELWORD(intrst->r.w.bx, 0x8000);
	STOREINTELWORD(intrst->r.w.cx, 0x0200);
	STOREINTELWORD(intrst->r.w.dx, 0x8000);
}

/* 0E */
static void set_fileattr(INTRST intrst) {

	_SDACDS		sc;
	HDRVPATH	hdp;
	REG16		attr;

	if (pathishostdrv(intrst, &sc) != SUCCESS) {
		return;
	}
	if ((is_wildcards(intrst->fcbname_ptr)) ||
		(hostdrvs_getrealpath(&hdp, intrst->filename_ptr) != ERR_NOERROR)) {
		fail(intrst, ERR_FILENOTFOUND);
		return;
	}
	if (!IS_PERMITWRITE) {
		fail(intrst, ERR_ACCESSDENIED);
		return;
	}
	attr = MEMR_READ16(CPU_SS, CPU_BP + sizeof(IF4INTR)) & 0x37;

	// 成功したことにする...
	succeed(intrst);
}

/* 0F */
static void get_fileattr(INTRST intrst) {

	_SDACDS		sc;
	HDRVPATH	hdp;
	UINT16		ax;

	if (pathishostdrv(intrst, &sc) != SUCCESS) {
		return;
	}

	TRACEOUT(("get_fileattr: ->%s", intrst->fcbname_ptr));
	if(strcmp(intrst->fcbname_ptr, "???????????") || intrst->filename_ptr[0]){ // XXX: Win用特例
		if ((is_wildcards(intrst->fcbname_ptr)) ||
			(hostdrvs_getrealpath(&hdp, intrst->filename_ptr) != ERR_NOERROR)) {
			fail(intrst, ERR_FILENOTFOUND);
			return;
		}
	}else{
		if (hostdrvs_getrealpath(&hdp, intrst->filename_ptr) != ERR_NOERROR) {
			fail(intrst, ERR_FILENOTFOUND);
			return;
		}
	}
	TRACEOUT(("get_fileattr: %s - %x", hdp.szPath, hdp.file.attr));
	ax = hdp.file.attr & 0x37;
	if (!IS_PERMITWRITE) {
		ax |= 0x01;
	}
	intrst->r.b.flag_l &= ~C_FLAG;
	STOREINTELWORD(intrst->r.w.ax, ax);
}

/* 11 */
static void rename_file(INTRST intrst)
{
	_SDACDS sc;
	UINT nResult;
	HDRVPATH hdp1;
	char fcbname1[11];
	HDRVPATH hdp2;
	char fcbname2[11];
	LISTARRAY lst;
	UINT nIndex;
	HDRVLST phdl;
	OEMCHAR szPath[MAX_PATH];
	HDRVPATH hdp;
	UINT i;
	char fcbname[11];

	if (pathishostdrv(intrst, &sc) != SUCCESS)
	{
		return;
	}

	nResult = ERR_NOERROR;
	lst = NULL;
	do
	{
		TRACEOUT(("rename_file: %s -> %s", intrst->filename_ptr, intrst->filename_ptr_2));
		nResult = hostdrvs_getrealdir(&hdp1, fcbname1, intrst->filename_ptr);
		if (nResult != ERR_NOERROR)
		{
			break;
		}

		nResult = hostdrvs_getrealdir(&hdp2, fcbname2, intrst->filename_ptr_2);
		if (nResult != ERR_NOERROR)
		{
			break;
		}

		lst = hostdrvs_getpathlist(&hdp1, fcbname1, 0x37);
		if (lst == NULL)
		{
			nResult = ERR_FILENOTFOUND;
			break;
		}

		if (!IS_PERMITDELETE)
		{
			nResult = ERR_ACCESSDENIED;
			break;
		}

		nIndex = 0;
		while (TRUE /*CONSTCOND*/)
		{
			phdl = (HDRVLST)listarray_getitem(lst, nIndex++);
			if (phdl == NULL)
			{
				break;
			}
			file_cpyname(szPath, hdp1.szPath, NELEMENTS(szPath));
			file_setseparator(szPath, NELEMENTS(szPath));
			file_catname(szPath, phdl->szFilename, NELEMENTS(szPath));

			hdp = hdp2;
			for (i = 0; i < 11; i++)
			{
				fcbname[i] = (fcbname2[i] != '?') ? fcbname2[i] : phdl->file.fcbname[i];
			}
			if (hostdrvs_appendname(&hdp, fcbname) != ERR_FILENOTFOUND)
			{
				nResult = ERR_ACCESSDENIED;
				break;
			}

			TRACEOUT(("renamed: %s -> %s", szPath, hdp.szPath));
			if (file_rename(szPath, hdp.szPath) != 0)
			{
				nResult = ERR_ACCESSDENIED;
				break;
			}
		}
	} while (FALSE /*CONSTCOND*/);

	if (lst != NULL)
	{
		listarray_destroy(lst);
	}
	if (nResult == ERR_NOERROR)
	{
		succeed(intrst);
	}
	else
	{
		fail(intrst, (UINT16)nResult);
	}
}

/* 13 */
static void delete_file(INTRST intrst)
{
	_SDACDS sc;
	UINT nResult;
	HDRVPATH hdp;
	char fcbname[11];
	LISTARRAY lst;
	UINT nIndex;
	HDRVLST phdl;
	OEMCHAR szPath[MAX_PATH];

	if (pathishostdrv(intrst, &sc) != SUCCESS)
	{
		return;
	}

	nResult = ERR_NOERROR;
	lst = NULL;
	do
	{
		nResult = hostdrvs_getrealdir(&hdp, fcbname, intrst->filename_ptr);
		if (nResult != ERR_NOERROR)
		{
			break;
		}

		if (!is_wildcards(fcbname))
		{
			nResult = hostdrvs_appendname(&hdp, fcbname);
			if (nResult != ERR_NOERROR)
			{
				break;
			}

			if (hdp.file.attr & 0x10)
			{
				nResult = ERR_ACCESSDENIED;
				break;
			}
			TRACEOUT(("delete_file: %s -> %s", intrst->filename_ptr, hdp.szPath));

			if (!IS_PERMITDELETE)
			{
				nResult = ERR_ACCESSDENIED;
				break;
			}

			if (file_delete(hdp.szPath))
			{
				nResult = ERR_ACCESSDENIED;
				break;
			}
		}
		else
		{
			lst = hostdrvs_getpathlist(&hdp, fcbname, 0x27);
			if (lst == NULL)
			{
				nResult = ERR_FILENOTFOUND;
				break;
			}

			if (!IS_PERMITDELETE)
			{
				nResult = ERR_ACCESSDENIED;
				break;
			}

			nIndex = 0;
			while (TRUE /*CONSTCOND*/)
			{
				phdl = (HDRVLST)listarray_getitem(lst, nIndex++);
				if (phdl == NULL)
				{
					break;
				}
				file_cpyname(szPath, hdp.szPath, NELEMENTS(szPath));
				file_setseparator(szPath, NELEMENTS(szPath));
				file_catname(szPath, phdl->szFilename, NELEMENTS(szPath));

				TRACEOUT(("delete_file: %s -> %s", intrst->filename_ptr, szPath));
				if (file_delete(szPath))
				{
					nResult = ERR_ACCESSDENIED;
					break;
				}
			}
		}
	} while (FALSE /*CONSTCOND*/);

	if (lst != NULL)
	{
		listarray_destroy(lst);
	}
	if (nResult == ERR_NOERROR)
	{
		succeed(intrst);
	}
	else
	{
		fail(intrst, (UINT16)nResult);
	}
}

/* 16 */
static void open_file(INTRST intrst)
{
	_SDACDS sc;
	_SFTREC sft;
	UINT nResult;
	HDRVPATH hdp;
	UINT nMode;
	FILEH fh = FILEH_INVALID;
	HDRVHANDLE hdf;

	if (pathishostdrv(intrst, &sc) != SUCCESS)
	{
		return;
	}
	fetch_sft(intrst, &sft);

	nResult = ERR_NOERROR;
	do
	{
		if (is_wildcards(intrst->fcbname_ptr))
		{
			nResult = ERR_FILENOTFOUND;
			break;
		}

		nResult = hostdrvs_getrealpath(&hdp, intrst->filename_ptr);
		if (nResult != ERR_NOERROR)
		{
			break;
		}
		if (hdp.file.attr & 0x10)
		{
			nResult = ERR_ACCESSDENIED;
			break;
		}
		TRACEOUT(("open_file: %s -> %s %d", intrst->filename_ptr, hdp.szPath, sft.open_mode[0] & 7));
		switch (sft.open_mode[0] & 7)
		{
			case 0:	/* read only */
				nMode = HDFMODE_READ;
				break;

			case 1:	/* write only */
				nMode = HDFMODE_WRITE;
				break;

			case 2:	/* read/write */
				nMode = HDFMODE_READ | HDFMODE_WRITE;
				break;

			default:
				nMode = 0;
				break;
		}
		if (nMode == 0)
		{
			nResult = ERR_INVALDACCESSMODE;
			break;
		}

		if (nMode & HDFMODE_WRITE)
		{
			if (!IS_PERMITWRITE)
			{
				nResult = ERR_ACCESSDENIED;
				break;
			}
			fh = file_open(hdp.szPath);
		}
		else
		{
			fh = file_open_rb(hdp.szPath);
		}
		if (fh == FILEH_INVALID)
		{
			TRACEOUT(("file open error!"));
			nResult = ERR_FILENOTFOUND;
			break;
		}

		hdf = hostdrvs_fhdlsea(hostdrv.fhdl);
		if (hdf == NULL)
		{
			nResult = ERR_NOHANDLESLEFT;
			break;
		}

		hdf->hdl = (INTPTR)fh;
		hdf->mode = nMode;
		file_cpyname(hdf->path, hdp.szPath, NELEMENTS(hdf->path));

		fill_sft(intrst, &sft, listarray_getpos(hostdrv.fhdl, hdf), &hdp.file);
		init_sft(&sft);

		store_sft(intrst, &sft);
		store_sda_currcds(&sc);
		succeed(intrst);
		return;
	} while (FALSE /*CONSTCOND*/);

	if (fh != FILEH_INVALID)
	{
		file_close(fh);
	}
	fail(intrst, (UINT16)nResult);
}

/* 17 */
static void create_file(INTRST intrst)
{
	_SDACDS sc;
	_SFTREC sft;
	UINT nResult;
	HDRVPATH hdp;
	HDRVHANDLE hdf;
	FILEH fh;

	if (pathishostdrv(intrst, &sc) != SUCCESS)
	{
		return;
	}
	fetch_sft(intrst, &sft);

	nResult = ERR_NOERROR;
	do
	{
		if (is_wildcards(intrst->fcbname_ptr))
		{
			nResult = ERR_FILENOTFOUND;
			break;
		}

		nResult = hostdrvs_getrealpath(&hdp, intrst->filename_ptr);
		if (nResult == ERR_NOERROR)
		{
			if (hdp.file.attr & 0x10)
			{
				nResult = ERR_ACCESSDENIED;
				break;
			}
		}
		else if (nResult == ERR_FILENOTFOUND)
		{
			nResult = ERR_NOERROR;
		}
		else
		{
			break;
		}

		TRACEOUT(("create_file: %s -> %s %d", intrst->filename_ptr, hdp.szPath, sft.open_mode[0] & 7));

		if (!IS_PERMITWRITE)
		{
			nResult = ERR_ACCESSDENIED;
			break;
		}

		hdf = hostdrvs_fhdlsea(hostdrv.fhdl);
		if (hdf == NULL)
		{
			nResult = ERR_ACCESSDENIED;
			break;
		}
		fh = file_create(hdp.szPath);
		if (fh == FILEH_INVALID)
		{
			TRACEOUT(("file create error!"));
			nResult = ERR_ACCESSDENIED;
			break;
		}

		hdf->hdl = (INTPTR)fh;
		hdf->mode = HDFMODE_READ | HDFMODE_WRITE;
		file_cpyname(hdf->path, hdp.szPath, NELEMENTS(hdf->path));

		fill_sft(intrst, &sft, listarray_getpos(hostdrv.fhdl, hdf), &hdp.file);
		init_sft(&sft);

		store_sft(intrst, &sft);
		store_sda_currcds(&sc);
	} while (FALSE /*CONSTCOND*/);

	if (nResult == ERR_NOERROR)
	{
		succeed(intrst);
	}
	else
	{
		fail(intrst, (UINT16)nResult);
	}
}

/* 1B */
static void find_first(INTRST intrst) {

	LISTARRAY flist;
	_SDACDS sc;
	HDRVPATH hdp;
	char fcbname[11];

	flist = hostdrv.flist;
	if (flist)
	{
		hostdrv.flist = NULL;
		hostdrv.stat.flistpos = 0;
		listarray_destroy(flist);
	}

	if (pathishostdrv(intrst, &sc) != SUCCESS)
	{
		return;
	}

	if (*intrst->srch_attr_ptr == 0x08)		/* ボリュームラベル */
	{
		store_srch(intrst);
		store_dir(intrst, &hdd_volume);
	}
	else
	{
		if (hostdrvs_getrealdir(&hdp, fcbname, intrst->filename_ptr) != ERR_NOERROR)
		{
			fail(intrst, ERR_PATHNOTFOUND);
			return;
		}

		TRACEOUT(("find_first %s -> %s", intrst->filename_ptr, hdp.szPath));
		hostdrv.flist = hostdrvs_getpathlist(&hdp, intrst->fcbname_ptr, *intrst->srch_attr_ptr);
		hostdrv.stat.flistpos = 0;
		if (find_file(intrst) != SUCCESS)
		{
			fail(intrst, ERR_PATHNOTFOUND);
			return;
		}
	}
	store_sda_currcds(&sc);
	succeed(intrst);
}

/* 1C */
static void find_next(INTRST intrst) {

	_SDACDS		sc;
	SRCHREC		srchrec;

	fetch_sda_currcds(&sc);
	setup_ptrs(intrst, &sc);

	srchrec = intrst->srchrec_ptr;
	if ((!(srchrec->drive_no & 0x40)) ||
		((srchrec->drive_no & 0x1f) != hostdrv.stat.drive_no)) {
		CPU_FLAG &= ~Z_FLAG;	// chain
		return;
	}
	if (find_file(intrst) != SUCCESS) {
		fail(intrst, ERR_NOMOREFILES);
		return;
	}
	store_sda_currcds(&sc);
	succeed(intrst);
}

#if 1
/* 1E */
static void do_redir(INTRST intrst) {

	_SDACDS		sc;
	REG16		mode;
	REG16		bx;
	char		tmp[4];

	TRACEOUT(("do_redir"));
	if (pathishostdrv(intrst, &sc) != SUCCESS) {
		return;
	}
	mode = MEMR_READ16(CPU_SS, CPU_BP + sizeof(IF4INTR));
	TRACEOUT(("do_redir: %.4x", mode));
	switch(mode) {
		case 0x5f02:
			bx = LOADINTELWORD(intrst->r.w.bx);
			if (bx) {
				fail(intrst, 0x12);
				return;
			}
			MEMR_WRITE16(CPU_DS, CPU_BX + 2, 4);
			MEMR_WRITE16(CPU_DS, CPU_BX + 4, 1);
			tmp[0] = (char)('A' + hostdrv.stat.drive_no);
			tmp[1] = ':';
			tmp[2] = '\0';
			MEMR_WRITES(LOADINTELWORD(intrst->r.w.ds),
							LOADINTELWORD(intrst->r.w.si), tmp, 3);
			MEMR_WRITES(LOADINTELWORD(intrst->r.w.es),
							LOADINTELWORD(intrst->r.w.di),
							ROOTPATH, ROOTPATH_SIZE + 1);
			break;

		default:
			CPU_FLAG &= ~Z_FLAG;	// chain
			return;
	}
	succeed(intrst);
}
#endif

/* 21 */
// dos4以降呼ばれることはあんまない・・・
static void seek_fromend(INTRST intrst) {

	_SDACDS		sc;
	_SFTREC		sft;
	UINT16		reg;
	UINT32 		pos;
	UINT		file_size;

	fetch_sda_currcds(&sc);
	fetch_sft(intrst, &sft);

	if ((sft.dev_info_word[0] & 0x3f) != hostdrv.stat.drive_no) {
		CPU_FLAG &= ~Z_FLAG;	// chain
		return;
	}
	reg = LOADINTELWORD(intrst->r.w.cx);
	pos = reg << 16;
	reg = LOADINTELWORD(intrst->r.w.dx);
	pos += reg;
	file_size = LOADINTELDWORD(sft.file_size);
	if (pos > file_size) {
		pos = file_size;
	}
	reg = (UINT16)(pos >> 16);
	STOREINTELWORD(intrst->r.w.dx, reg);
	reg = (UINT16)pos;
	STOREINTELWORD(intrst->r.w.ax, reg);
	pos = file_size - pos;
	STOREINTELDWORD(sft.file_pos, pos);

	store_sft(intrst, &sft);
	intrst->r.b.flag_l &= ~C_FLAG;
}

/* 2D */
static void unknownfunc_2d(INTRST intrst) {

	_SDACDS		sc;
	_SFTREC		sft;

	fetch_sda_currcds(&sc);
	fetch_sft(intrst, &sft);
	if ((sft.dev_info_word[0] & 0x3f) != hostdrv.stat.drive_no) {
		CPU_FLAG &= ~Z_FLAG;	// chain
		return;
	}
#if 1
	TRACEOUT(("unknownfunc_2d"));
#else
	intr_regs.flags &= ~C_FLAG;
	intr_regs.ax = 2;
#endif
}

/* 2E */
// for dos4+
static void ext_openfile(INTRST intrst)
{
	_SDACDS		sc;
	_SFTREC		sft;
	UINT nResult;
	HDRVPATH	hdp;
	UINT		mode;
	BOOL		create;
	REG16 		act;
	REG16		cx;
	FILEH		fh;
	HDRVHANDLE	hdf;

	if (pathishostdrv(intrst, &sc) != SUCCESS)
	{
		return;
	}
	fetch_sft(intrst, &sft);

	nResult = ERR_NOERROR;
	do
	{
		if (is_wildcards(intrst->fcbname_ptr))
		{
			nResult = ERR_FILENOTFOUND;
			break;
		}

		sft.open_mode[0] = sc.ver4.sda.mode_2E[0] & 0x7f;
		sft.open_mode[1] = sc.ver4.sda.mode_2E[1] & 0x00;
		switch (sft.open_mode[0] & 7)
		{
			case 1:	// write only
				mode = HDFMODE_WRITE;
				break;

			case 2:	// read/write
				mode = HDFMODE_READ | HDFMODE_WRITE;
				break;

			default:
				mode = HDFMODE_READ;
				break;
		}
		act = LOADINTELWORD(sc.ver4.sda.action_2E);
		create = FALSE;

		nResult = hostdrvs_getrealpath(&hdp, intrst->filename_ptr);
		if (nResult == ERR_NOERROR)				/* ファイルが存在 */
		{
			if (hdp.file.attr & 0x10)
			{
				nResult = ERR_ACCESSDENIED;
			}
			switch (act & 3)
			{
				case 1:
					cx = 1;
					break;

				case 2:
					create = TRUE;
					cx = 3;
					break;

				default:
					nResult = ERR_ACCESSDENIED;
					return;
			}
		}
		else if (nResult == ERR_FILENOTFOUND)	/* 新規ファイル */
		{
			if (act & 0x10)
			{
				create = TRUE;
				cx = 2;
				nResult = ERR_NOERROR;
			}
		}
		if (nResult != ERR_NOERROR)
		{
			break;
		}

		fh = FILEH_INVALID;
		if (create)
		{
			if (IS_PERMITWRITE)
			{
				fh = file_create(hdp.szPath);
			}
		}
		else if (mode & HDFMODE_WRITE)
		{
			if (IS_PERMITWRITE)
			{
				fh = file_open(hdp.szPath);
			}
		}
		else
		{
			fh = file_open_rb(hdp.szPath);
		}

		if (fh == FILEH_INVALID)
		{
			TRACEOUT(("file open error!"));
			nResult = ERR_ACCESSDENIED;
			break;
		}

		hdf = hostdrvs_fhdlsea(hostdrv.fhdl);
		if (hdf == NULL)
		{
			file_close(fh);
			nResult = ERR_ACCESSDENIED;
			break;
		}

		hdf->hdl = (INTPTR)fh;
		hdf->mode = mode;
		file_cpyname(hdf->path, hdp.szPath, NELEMENTS(hdf->path));

		STOREINTELWORD(intrst->r.w.cx, cx);
		fill_sft(intrst, &sft, listarray_getpos(hostdrv.fhdl, hdf), &hdp.file);
		init_sft(&sft);
		store_sft(intrst, &sft);

		store_sda_currcds(&sc);
	} while (FALSE /*CONSTCOND*/);

	if (nResult == ERR_NOERROR)
	{
		succeed(intrst);
	}
	else
	{
		fail(intrst, (UINT16)nResult);
	}
}


// ----

typedef void (*HDINTRFN)(INTRST intrst);

static const HDINTRFN intr_func[] = {
		inst_check,			/* 00 */
		remove_dir,			/* 01 */
		NULL,
		make_dir,			/* 03 */
		NULL,
		change_currdir,		/* 05 */
		close_file,			/* 06 */
		commit_file,		/* 07 */
		read_file,			/* 08 */
		write_file,			/* 09 */
		lock_file,			/* 0A */
		unlock_file,		/* 0B */
		get_diskspace,		/* 0C */
		NULL,
		set_fileattr,		/* 0E */
		get_fileattr,		/* 0F */
		NULL,
		rename_file,		/* 11 */
		NULL,
		delete_file,		/* 13 */
		NULL,
		NULL,
		open_file,			/* 16 */
		create_file,		/* 17 */
		NULL,
		NULL,
		NULL,
		find_first,			/* 1B */
		find_next,			/* 1C */
		NULL,
		do_redir,
		NULL,
		NULL,
		seek_fromend,		/* 21 */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		unknownfunc_2d,		/* 2D */
		ext_openfile		/* 2E */
};


// ----

// 始めに一回だけ呼んでね(はーと
void hostdrv_initialize(void) {

	ZeroMemory(&hostdrv, sizeof(hostdrv));
	hostdrv.fhdl = listarray_new(sizeof(_HDRVHANDLE), 16);
	TRACEOUT(("hostdrv_initialize"));
}

// 終わりに一回だけ呼んでね(はーと
void hostdrv_deinitialize(void) {

	listarray_destroy(hostdrv.flist);
	hostdrvs_fhdlallclose(hostdrv.fhdl);
	listarray_destroy(hostdrv.fhdl);
	TRACEOUT(("hostdrv_deinitialize"));
}

// リセットルーチンで呼ぶべしべし
void hostdrv_reset(void) {

	hostdrv_deinitialize();
	hostdrv_initialize();
}


// ---- for np2sysp

void hostdrv_mount(const void *arg1, long arg2) {

	if ((np2cfg.hdrvroot[0] == '\0') || (!np2cfg.hdrvenable) || (hostdrv.stat.is_mount)) {
		np2sysp_outstr(OEMTEXT("ng"), 0);
		return;
	}
	hostdrv.stat.is_mount = TRUE;
	fetch_if4dos();
	np2sysp_outstr(OEMTEXT("ok"), 0);
	(void)arg1;
	(void)arg2;
}

void hostdrv_unmount(const void *arg1, long arg2) {

	if (hostdrv.stat.is_mount) {
		hostdrv_reset();
	}
	(void)arg1;
	(void)arg2;
}

void hostdrv_intr(const void *arg1, long arg2) {

	_INTRST	intrst;

	ZeroMemory(&intrst, sizeof(intrst));
	intrst.is_chardev = (CPU_FLAG & C_FLAG) == 0;
	CPU_FLAG &= ~(C_FLAG | Z_FLAG);				// not fcb / chain
	
	if (!np2cfg.hdrvenable) {
		return;
	}
	if (!hostdrv.stat.is_mount) {
		return;
	}

	fetch_intr_regs(&intrst);

	TRACEOUT(("hostdrv: AL=%.2x", intrst.r.b.al));

	if ((intrst.r.b.al >= NELEMENTS(intr_func)) ||
		(intr_func[intrst.r.b.al] == NULL)) {
		return;
	}

	CPU_FLAG |= Z_FLAG;							// not chain
	(*intr_func[intrst.r.b.al])(&intrst);

	store_intr_regs(&intrst);

	(void)arg1;
	(void)arg2;
}


// ---- for statsave

typedef struct {
	UINT	stat;
	UINT	files;
	UINT	flists;
} SFHDRV;

static BOOL fhdl_wr(void *vpItem, void *vpArg) {

	OEMCHAR	*p;
	UINT	len;

	p = ((HDRVHANDLE)vpItem)->path;
	len = (UINT)OEMSTRLEN(p);
	statflag_write((STFLAGH)vpArg, &len, sizeof(len));
	if (len) {
		if (len < MAX_PATH) {
			ZeroMemory(p + len, (MAX_PATH - len) * sizeof(OEMCHAR));
		}
		statflag_write((STFLAGH)vpArg, vpItem, sizeof(_HDRVHANDLE));
	}
	return(FALSE);
}

static BOOL flist_wr(void *vpItem, void *vpArg) {

	OEMCHAR	*p;
	UINT	len;

	p = ((HDRVLST)vpItem)->szFilename;
	len = (UINT)OEMSTRLEN(p);
	if (len < MAX_PATH) {
		ZeroMemory(p + len, (MAX_PATH - len) * sizeof(OEMCHAR));
	}
	statflag_write((STFLAGH)vpArg, vpItem, sizeof(_HDRVLST));
	return(FALSE);
}

int hostdrv_sfsave(STFLAGH sfh, const SFENTRY *tbl) {

	SFHDRV	sfhdrv;
	int		ret;

	if (!hostdrv.stat.is_mount) {
		return(STATFLAG_SUCCESS);
	}
	sfhdrv.stat = sizeof(hostdrv.stat);
	sfhdrv.files = listarray_getitems(hostdrv.fhdl);
	sfhdrv.flists = listarray_getitems(hostdrv.flist);
	ret = statflag_write(sfh, &sfhdrv, sizeof(sfhdrv));
	ret |= statflag_write(sfh, &hostdrv.stat, sizeof(hostdrv.stat));
	listarray_enum(hostdrv.fhdl, fhdl_wr, sfh);
	listarray_enum(hostdrv.flist, flist_wr, sfh);
	(void)tbl;
	return(ret);
}

int hostdrv_sfload(STFLAGH sfh, const SFENTRY *tbl) {

	SFHDRV		sfhdrv;
	int			ret;
	UINT		i;
	UINT		len;
	HDRVHANDLE	hdf;
	FILEH		fh;
	HDRVLST		hdl;

	listarray_clr(hostdrv.fhdl);
	listarray_clr(hostdrv.flist);

	ret = statflag_read(sfh, &sfhdrv, sizeof(sfhdrv));
	if (sfhdrv.stat != sizeof(hostdrv.stat)) {
		return(STATFLAG_FAILURE);
	}
	ret |= statflag_read(sfh, &hostdrv.stat, sizeof(hostdrv.stat));
	for (i=0; i<sfhdrv.files; i++) {
		hdf = (HDRVHANDLE)listarray_append(hostdrv.fhdl, NULL);
		if (hdf == NULL) {
			return(STATFLAG_FAILURE);
		}
		ret |= statflag_read(sfh, &len, sizeof(len));
		if (len) {
			ret |= statflag_read(sfh, hdf, sizeof(_HDRVHANDLE));
			if (hdf->mode & HDFMODE_WRITE) {
				fh = file_open(hdf->path);
			}
			else {
				fh = file_open_rb(hdf->path);
			}
			hdf->hdl = (INTPTR)fh;
		}
	}
	for (i=0; i<sfhdrv.flists; i++) {
		hdl = (HDRVLST)listarray_append(hostdrv.flist, NULL);
		if (hdl == NULL) {
			return(STATFLAG_FAILURE);
		}
		ret |= statflag_read(sfh, hdl, sizeof(_HDRVLST));
	}
	(void)tbl;
	return(ret);
}
#endif

