#include	"compiler.h"
#include	"strres.h"
#include	"dosio.h"
#include	"sysmng.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"sxsi.h"
#ifdef SUPPORT_VPCVHD
#include "hdd_vpc.h"
#endif

const char sig_vhd[8] = "VHD1.00";
const char sig_nhd[15] = "T98HDDIMAGE.R0";
const char sig_slh[] = "HDIM";

const SASIHDD sasihdd[7] = {
				{33, 4, 153},			// 5MB
				{33, 4, 310},			// 10MB
				{33, 6, 310},			// 15MB
				{33, 8, 310},			// 20MB
				{33, 4, 615},			// 20MB (not used!)
				{33, 6, 615},			// 30MB
				{33, 8, 615}};			// 40MB


// ----

static BRESULT hdd_reopen(SXSIDEV sxsi) {

	FILEH	fh;

	fh = file_open(sxsi->fname);
	if (fh != FILEH_INVALID) {
		sxsi->hdl = (INTPTR)fh;
		return(SUCCESS);
	}
	else {
		return(FAILURE);
	}
}

static REG8 hdd_read(SXSIDEV sxsi, FILEPOS pos, UINT8 *buf, UINT size) {

	FILEH	fh;
	FILEPOS	r;
	UINT	rsize;

	if (sxsi_prepare(sxsi) != SUCCESS) {
		return(0x60);
	}
	if ((pos < 0) || (pos >= sxsi->totals)) {
		return(0x40);
	}
	pos = pos * sxsi->size + sxsi->headersize;
	fh = (FILEH)sxsi->hdl;
	r = file_seek(fh, pos, FSEEK_SET);
	if (pos != r) {
		return(0xd0);
	}
	while(size) {
		rsize = MIN(size, sxsi->size);
		CPU_REMCLOCK -= rsize;
		if (file_read(fh, buf, rsize) != rsize) {
			return(0xd0);
		}
		buf += rsize;
		size -= rsize;
	}
	return(0x00);
}

static REG8 hdd_write(SXSIDEV sxsi, FILEPOS pos, const UINT8 *buf, UINT size) {

	FILEH	fh;
	FILEPOS	r;
	UINT	wsize;

	if (sxsi_prepare(sxsi) != SUCCESS) {
		return(0x60);
	}
	if ((pos < 0) || (pos >= sxsi->totals)) {
		return(0x40);
	}
	pos = pos * sxsi->size + sxsi->headersize;
	fh = (FILEH)sxsi->hdl;
	r = file_seek(fh, pos, FSEEK_SET);
	if (pos != r) {
		return(0xd0);
	}
	while(size) {
		wsize = MIN(size, sxsi->size);
		CPU_REMCLOCK -= wsize;
		if (file_write(fh, buf, wsize) != wsize) {
			return(0x70);
		}
		buf += wsize;
		size -= wsize;
	}
	return(0x00);
}

static REG8 hdd_format(SXSIDEV sxsi, FILEPOS pos) {

	FILEH	fh;
	FILEPOS	r;
	UINT16	i;
	UINT8	work[256];
	UINT	size;
	UINT	wsize;

	if (sxsi_prepare(sxsi) != SUCCESS) {
		return(0x60);
	}
	if ((pos < 0) || (pos >= sxsi->totals)) {
		return(0x40);
	}
	pos = pos * sxsi->size + sxsi->headersize;
	fh = (FILEH)sxsi->hdl;
	r = file_seek(fh, pos, FSEEK_SET);
	if (pos != r) {
		return(0xd0);
	}
	FillMemory(work, sizeof(work), 0xe5);
	for (i=0; i<sxsi->sectors; i++) {
		size = sxsi->size;
		while(size) {
			wsize = MIN(size, sizeof(work));
			size -= wsize;
			CPU_REMCLOCK -= wsize;
			if (file_write(fh, work, wsize) != wsize) {
				return(0x70);
			}
		}
	}
	return(0x00);
}

static void hdd_close(SXSIDEV sxsi) {

	file_close((FILEH)sxsi->hdl);
}


// ----

// SASI規格HDDかチェック
static UINT8 gethddtype(SXSIDEV sxsi) {

const SASIHDD	*sasi;
	UINT		i;

	if (sxsi->size == 256) {
		sasi = sasihdd;
		for (i=0; i<NELEMENTS(sasihdd); i++, sasi++) {
			if ((sxsi->sectors == sasi->sectors) &&
				(sxsi->surfaces == sasi->surfaces) &&
				(sxsi->cylinders == sasi->cylinders)) {
				return((UINT8)i);
			}
		}
	}
	return(SXSIMEDIA_INVSASI + 7);
}

BRESULT sxsihdd_open(SXSIDEV sxsi, const OEMCHAR *fname) {

	FILEH		fh;
const OEMCHAR	*ext;
	REG8		iftype;
	FILEPOS		totals;
	UINT32		headersize;
	UINT32		surfaces;
	UINT32		cylinders;
	UINT32		sectors;
	UINT32		size;

	fh = file_open(fname);
	if (fh == FILEH_INVALID) {
		goto sxsiope_err1;
	}
	ext = file_getext(fname);
	iftype = sxsi->drv & SXSIDRV_IFMASK;
	if ((iftype == SXSIDRV_SASI) && (!file_cmpname(ext, str_thd))) {
		THDHDR thd;						// T98 HDD (IDE)
		if (file_read(fh, &thd, sizeof(thd)) != sizeof(thd)) {
			goto sxsiope_err2;
		}
		headersize = 256;
		surfaces = 8;
		cylinders = LOADINTELWORD(thd.cylinders);
		sectors = 33;
		size = 256;
		totals = cylinders * sectors * surfaces;
	}
	else if ((iftype == SXSIDRV_SASI) && (!file_cmpname(ext, str_nhd))) {
		NHDHDR nhd;						// T98Next HDD (IDE)
		if ((file_read(fh, &nhd, sizeof(nhd)) != sizeof(nhd)) ||
			(memcmp(nhd.sig, sig_nhd, 15))) {
			goto sxsiope_err2;
		}
		headersize = LOADINTELDWORD(nhd.headersize);
		surfaces = LOADINTELWORD(nhd.surfaces);
		cylinders = LOADINTELDWORD(nhd.cylinders);
		sectors = LOADINTELWORD(nhd.sectors);
		size = LOADINTELWORD(nhd.sectorsize);
		totals = (FILEPOS)cylinders * sectors * surfaces;
	}
	else if ((iftype == SXSIDRV_SASI) && (!file_cmpname(ext, str_hdi))) {
		HDIHDR hdi;						// ANEX86 HDD (SASI) thanx Mamiya
		if (file_read(fh, &hdi, sizeof(hdi)) != sizeof(hdi)) {
			goto sxsiope_err2;
		}
		headersize = LOADINTELDWORD(hdi.headersize);
		surfaces = LOADINTELDWORD(hdi.surfaces);
		cylinders = LOADINTELDWORD(hdi.cylinders);
		sectors = LOADINTELDWORD(hdi.sectors);
		size = LOADINTELDWORD(hdi.sectorsize);
		totals = cylinders * sectors * surfaces;
	}
	else if ((iftype == SXSIDRV_SCSI) && (!file_cmpname(ext, str_hdd))) {
		VHDHDR vhd;						// Virtual98 HDD (SCSI)
		if ((file_read(fh, &vhd, sizeof(vhd)) != sizeof(vhd)) ||
			(memcmp(vhd.sig, sig_vhd, 5))) {
			goto sxsiope_err2;
		}
		headersize = sizeof(vhd);
		surfaces = vhd.surfaces;
		cylinders = LOADINTELWORD(vhd.cylinders);
		sectors = vhd.sectors;
		size = LOADINTELWORD(vhd.sectorsize);
		totals = (SINT32)LOADINTELDWORD(vhd.totals);
	}
	else if ((iftype == SXSIDRV_SCSI) && (!file_cmpname(ext, str_hdn))) {
		// RaSCSI flat disk image (NEC PC-9801-55)
		FILELEN fsize = file_getsize(fh);
		headersize = 0;
		size = 512;
		surfaces = 8;
		sectors = 25;
		cylinders = (UINT32)(fsize / (sectors * surfaces * size));
		totals = fsize / size;
		// totals = (FILEPOS)cylinders * sectors * surfaces;
	}
	else if ((iftype == SXSIDRV_SASI) && (!file_cmpname(ext, str_slh))) {
		SLHHDR slh;						// SL9821 HDD (IDE)
		if ((file_read(fh, &slh, sizeof(slh)) != sizeof(slh)) ||
			(memcmp(slh.sig, sig_slh, 4))) {
			goto sxsiope_err2;
		}
		headersize = 512;
		surfaces = LOADINTELDWORD(slh.surfaces);
		cylinders = LOADINTELDWORD(slh.cylinders);
		sectors = LOADINTELDWORD(slh.sectors);
		size = LOADINTELDWORD(slh.sectorsize);
		totals = (FILEPOS)cylinders * sectors * surfaces;
	}
#ifdef SUPPORT_VPCVHD
	else if ((iftype == SXSIDRV_SASI) && (!file_cmpname(ext, str_vhd))) {
		// Microsoft VirtualPC VHD (SASI/IDE)
		BRESULT rc = sxsihdd_vpcvhd_mount(sxsi, fh);
		if (rc == FAILURE) {
			file_close(fh);
		}
		return (rc);
	}
#endif
	else {
		goto sxsiope_err2;
	}

	// フォーマット確認〜
	if ((surfaces == 0) || (surfaces >= 256) ||
		(cylinders == 0) || (cylinders >= 65536) ||
		(sectors == 0) || (sectors >= 256) ||
		(size == 0) || ((size & (size - 1)) != 0)) {
		goto sxsiope_err2;
	}
	if (iftype == SXSIDRV_SCSI) {
		if (!(size & 0x700)) {			// not 256,512,1024
			goto sxsiope_err2;
		}
	}
	sxsi->reopen = hdd_reopen;
	sxsi->read = hdd_read;
	sxsi->write = hdd_write;
	sxsi->format = hdd_format;
	sxsi->close = hdd_close;

	sxsi->hdl = (INTPTR)fh;
	sxsi->totals = totals;
	sxsi->cylinders = (UINT16)cylinders;
	sxsi->size = (UINT16)size;
	sxsi->sectors = (UINT8)sectors;
	sxsi->surfaces = (UINT8)surfaces;
	sxsi->headersize = headersize;
	sxsi->mediatype = gethddtype(sxsi);
	return(SUCCESS);

sxsiope_err2:
	file_close(fh);

sxsiope_err1:
	return(FAILURE);
}

#ifdef SUPPORT_VPCVHD
#define HDD_VPC_INCLUDE
#include "hdd_vhd.h"
#endif
