#include	"compiler.h"
#if defined(OSLANG_UTF8) || defined(OSLANG_UCS2)
#include	"oemtext.h"
#endif
#include	"dosio.h"
#include	"newdisk.h"
#include	"diskimage/fddfile.h"
#include	"sxsi.h"
#include	"hddboot.res"

#ifdef SUPPORT_VPCVHD
#include "hdd_vpc.h"
#endif


// ---- fdd

void newdisk_fdd(const OEMCHAR *fname, REG8 type, const OEMCHAR *label) {

	_D88HEAD	d88head;
	FILEH		fh;

	ZeroMemory(&d88head, sizeof(d88head));
	STOREINTELDWORD(d88head.fd_size, sizeof(d88head));
#if defined(OSLANG_UTF8) || defined(OSLANG_UCS2)
	oemtext_oemtosjis((char *)d88head.fd_name, sizeof(d88head.fd_name),
															label, (UINT)-1);
#else
	milstr_ncpy((char *)d88head.fd_name, label, sizeof(d88head.fd_name));
#endif
	d88head.fd_type = type;
	fh = file_create(fname);
	if (fh != FILEH_INVALID) {
		file_write(fh, &d88head, sizeof(d88head));
		file_close(fh);
	}
}

void newdisk_123mb_fdd(const OEMCHAR *fname) {
	
	FILEH		fh;
	char databuf[8192] = {0};
	int fdsize = 1261568;

	fh = file_create(fname);
	if (fh != FILEH_INVALID) {
		while(fdsize){
			if(fdsize < sizeof(databuf)){
				file_write(fh, &databuf, fdsize);
				break;
			}else{
				file_write(fh, &databuf, sizeof(databuf));
				fdsize -= sizeof(databuf);
			}
		}
		file_close(fh);
	}
}

void newdisk_144mb_fdd(const OEMCHAR *fname) {
	
	FILEH		fh;
	char databuf[8192] = {0};
	int fdsize = 1474560;

	fh = file_create(fname);
	if (fh != FILEH_INVALID) {
		while(fdsize){
			if(fdsize < sizeof(databuf)){
				file_write(fh, &databuf, fdsize);
				break;
			}else{
				file_write(fh, &databuf, sizeof(databuf));
				fdsize -= sizeof(databuf);
			}
		}
		file_close(fh);
	}
}



// ---- hdd

static BRESULT writezero(FILEH fh, FILELEN size) {

	UINT8	work[256];
	FILELEN	wsize;

	ZeroMemory(work, sizeof(work));
	while(size) {
		wsize = MIN(size, sizeof(work));
		if (file_write(fh, work, (UINT)wsize) != wsize) {
			return(FAILURE);
		}
		size -= wsize;
	}
	return(SUCCESS);
}

static BRESULT writehddiplex2(FILEH fh, UINT ssize, FILELEN tsize, int blank, int *progress, int *cancel) {

	UINT8	work[65536];
	FILELEN	size;
	FILELEN	progtotal;

	progtotal = tsize;
	*progress = 0;
	ZeroMemory(work, sizeof(work));
	if(!blank){
		CopyMemory(work, hdddiskboot, sizeof(hdddiskboot));
		if (ssize < 1024) {
			work[ssize - 2] = 0x55;
			work[ssize - 1] = 0xaa;
		}
	}
	if (file_write(fh, work, sizeof(work)) != sizeof(work)) {
		return(FAILURE);
	}
	if (tsize > sizeof(work)) {
		tsize -= sizeof(work);
		ZeroMemory(work, sizeof(work));
		while(tsize) {
			size = MIN(tsize, sizeof(work));
			tsize -= size;
			if (file_write(fh, work, (UINT)size) != size) {
				return(FAILURE);
			}
			*progress = (UINT32)((progtotal - tsize) * 100 / progtotal);
			if(*cancel){
				return(FAILURE);
			}
		}
	}
	return(SUCCESS);
}
static BRESULT writehddiplex(FILEH fh, UINT ssize, FILELEN tsize, int *progress, int *cancel) {
	
	return writehddiplex2(fh, ssize, tsize, 0, progress, cancel);
}
static BRESULT writehddipl(FILEH fh, UINT ssize, FILELEN tsize) {

	int progress;
	int cancel = 0;
	return writehddiplex(fh, ssize, tsize, &progress, &cancel);
}

static void hddsize2CHS(UINT hddsizeMB, UINT32 *C, UINT16 *H, UINT16 *S, UINT16 *SS) {
	
	FILELEN	size;
#ifdef SUPPORT_LARGE_HDD
	if(hddsizeMB <= 4351){
		size = hddsizeMB * 15;
		*C = (UINT32)size;
		*H = 8;
		*S = 17;
		*SS = 512;
	}else if(hddsizeMB <= 32255){
		size = hddsizeMB * 15 * 17 / 2 / 63;
		*C = (UINT32)size;
		*H = 16;
		*S = 63;
		*SS = 512;
	}else{
		size = hddsizeMB * 15 * 17 / 2 / 255;
		*C = (UINT32)size;
		*H = 16;
		*S = 255;
		*SS = 512;
	}
#else
	size = hddsizeMB * 15;
	*C = (UINT32)size;
	*H = 8;
	*S = 17;
	*SS = 512;
#endif
}

void newdisk_thd(const OEMCHAR *fname, UINT hddsize) {
	
	FILEH	fh;
	UINT8	work[256];
	UINT	size;
	BRESULT	r;

	if ((fname == NULL) || (hddsize < 5) || (hddsize > 256)) {
		goto ndthd_err;
	}
	fh = file_create(fname);
	if (fh == FILEH_INVALID) {
		goto ndthd_err;
	}
	ZeroMemory(work, 256);
	size = hddsize * 15;
	STOREINTELWORD(work, size);
	r = (file_write(fh, work, 256) == 256) ? SUCCESS : FAILURE;
	r |= writehddipl(fh, 256, 0);
	file_close(fh);
	if (r != SUCCESS) {
		file_delete(fname);
	}

ndthd_err:
	return;
}

void newdisk_nhd_ex_CHS(const OEMCHAR *fname, UINT32 C, UINT16 H, UINT16 S, UINT16 SS, int blank, int *progress, int *cancel) {

	FILEH	fh;
	NHDHDR	nhd;
	FILELEN	hddsize;
	BRESULT	r;

	hddsize = (FILELEN)C * H * S * SS / 1024 / 1024;
	
	if ((fname == NULL) || (hddsize < 1) || (hddsize > NHD_MAXSIZE2)) {
		goto ndnhd_err;
	}
	fh = file_create(fname);
	if (fh == FILEH_INVALID) {
		goto ndnhd_err;
	}
	ZeroMemory(&nhd, sizeof(nhd));
	CopyMemory(&nhd.sig, sig_nhd, 15);
	STOREINTELDWORD(nhd.headersize, sizeof(nhd));
	STOREINTELDWORD(nhd.cylinders, C);
	STOREINTELWORD(nhd.surfaces, H);
	STOREINTELWORD(nhd.sectors, S);
	STOREINTELWORD(nhd.sectorsize, SS);
	r = (file_write(fh, &nhd, sizeof(nhd)) == sizeof(nhd)) ? SUCCESS : FAILURE;
	r |= writehddiplex2(fh, SS, (FILELEN)C * H * S * SS, blank, progress, cancel);
	file_close(fh);
	if (r != SUCCESS) {
		file_delete(fname);
	}

ndnhd_err:
	return;
}
void newdisk_nhd_ex(const OEMCHAR *fname, UINT hddsize, int blank, int *progress, int *cancel) {
	
	UINT32 C;
	UINT16 H;
	UINT16 S;
	UINT16 SS;
	
	hddsize2CHS(hddsize, &C, &H, &S, &SS);

	newdisk_nhd_ex_CHS(fname, C, H, S, SS, blank, progress, cancel);
}
void newdisk_nhd(const OEMCHAR *fname, UINT hddsize) {
	
	int progress;
	int cancel = 0;
	
	newdisk_nhd_ex(fname, hddsize, 0, &progress, &cancel);
}

// hddtype = 0:5MB / 1:10MB / 2:15MB / 3:20MB / 5:30MB / 6:40MB
void newdisk_hdi(const OEMCHAR *fname, UINT hddtype) {

const SASIHDD	*sasi;
	FILEH		fh;
	HDIHDR		hdi;
	UINT32		size;
	BRESULT		r;

	hddtype &= 7;
	if ((fname == NULL) || (hddtype == 7)) {
		goto ndhdi_err;
	}
	sasi = sasihdd + hddtype;
	fh = file_create(fname);
	if (fh == FILEH_INVALID) {
		goto ndhdi_err;
	}
	ZeroMemory(&hdi, sizeof(hdi));
	size = 256 * sasi->sectors * sasi->surfaces * sasi->cylinders;
//	STOREINTELDWORD(hdi.hddtype, 0);
	STOREINTELDWORD(hdi.headersize, 4096);
	STOREINTELDWORD(hdi.hddsize, size);
	STOREINTELDWORD(hdi.sectorsize, 256);
	STOREINTELDWORD(hdi.sectors, sasi->sectors);
	STOREINTELDWORD(hdi.surfaces, sasi->surfaces);
	STOREINTELDWORD(hdi.cylinders, sasi->cylinders);
	r = (file_write(fh, &hdi, sizeof(hdi)) == sizeof(hdi)) ? SUCCESS : FAILURE;
	r |= writezero(fh, 4096 - sizeof(hdi));
	r |= writehddipl(fh, 256, size);
	file_close(fh);
	if (r != SUCCESS) {
		file_delete(fname);
	}

ndhdi_err:
	return;
}

void newdisk_vhd(const OEMCHAR *fname, UINT hddsize) {

	FILEH	fh;
	VHDHDR	vhd;
	UINT	tmp;
	BRESULT	r;

	if ((fname == NULL) || (hddsize < 2) || (hddsize > 512)) {
		goto ndvhd_err;
	}
	fh = file_create(fname);
	if (fh == FILEH_INVALID) {
		goto ndvhd_err;
	}
	ZeroMemory(&vhd, sizeof(vhd));
	CopyMemory(&vhd.sig, sig_vhd, 7);
	STOREINTELWORD(vhd.mbsize, (UINT16)hddsize);
	STOREINTELWORD(vhd.sectorsize, 256);
	vhd.sectors = 32;
	vhd.surfaces = 8;
	tmp = hddsize *	16;		// = * 1024 * 1024 / (8 * 32 * 256);
	STOREINTELWORD(vhd.cylinders, (UINT16)tmp);
	tmp *= 8 * 32;
	STOREINTELDWORD(vhd.totals, tmp);
	r = (file_write(fh, &vhd, sizeof(vhd)) == sizeof(vhd)) ? SUCCESS : FAILURE;
	r |= writehddipl(fh, 256, 0);
	file_close(fh);
	if (r != SUCCESS) {
		file_delete(fname);
	}

ndvhd_err:
	return;
}

void newdisk_hdn(const OEMCHAR *fname, UINT hddsize) {

	FILEH	fh;
	FILELEN	tmp;
	BRESULT	r;

	// HDN : RaSCSI HD image (suitable for NEC PC-9801-55/92)
	// structure     : flat
	// sectors/track : 25 (fixed)
	// heads         :  8 (fixed)
	// cylinders     : up to 4095 (12bits) (for old BIOS) =  399MiB
	//                      65535 (16bits)                = 6399MiB
	if ((fname == NULL) || (hddsize < 2) || (hddsize > 6399)) {
		goto ndhdn_err;
	}
	fh = file_create(fname);
	if (fh == FILEH_INVALID) {
		goto ndhdn_err;
	}
	tmp = hddsize * 1024 * 1024;
	// round up 
	if ((tmp % (512 * 25 * 8)) != 0) {
		tmp = tmp / (512 * 25 * 8) + 1;
		tmp *= (512 * 25 * 8);
	}
	r = writezero(fh, tmp);
	file_close(fh);
	if (r != SUCCESS) {
		file_delete(fname);
	}

ndhdn_err:
	return;
}

#ifdef SUPPORT_VPCVHD
const char vpcvhd_sig[] = "conectix";
const char vpcvhd_sigDH[] = "cxsparse";
const char vpcvhd_creator[] = "vpc ";
const char vpcvhd_os[] = "Wi2k";

void newdisk_vpcvhd_ex_CHS(const OEMCHAR *fname, UINT32 C, UINT16 H, UINT16 S, UINT16 SS, int dynamic, int blank, int *progress, int *cancel) {

	FILEH   fh;
	VPCVHDFOOTER    vpcvhd;
	VPCVHDDDH		vpcvhd_dh;
	BRESULT r = 0;
	UINT64  origsize;
	UINT32  checksum;
	size_t footerlen;
	UINT	hddsize;
   
	hddsize = (UINT32)((FILELEN)C * H * S * SS / 1024 / 1024);
	
	if ((fname == NULL) || (hddsize < 1) || (hddsize > NHD_MAXSIZE2)) {
		goto vpcvhd_err;
	}
	fh = file_create(fname);
	if (fh == FILEH_INVALID) {
		goto vpcvhd_err;
	}

	footerlen = sizeof(VPCVHDFOOTER);
	ZeroMemory(&vpcvhd, footerlen);

	CopyMemory(&vpcvhd.Cookie, vpcvhd_sig, 8);
	STOREMOTOROLADWORD(vpcvhd.Features, 2);
	STOREMOTOROLADWORD(vpcvhd.FileFormatVersion, 0x10000);
	CopyMemory(&vpcvhd.CreatorApplication, vpcvhd_creator, 4);
	STOREMOTOROLADWORD(vpcvhd.CreatorVersion, 0x50003);
	CopyMemory(&vpcvhd.CreatorHostOS, vpcvhd_os, 4);
	STOREMOTOROLAQWORD(vpcvhd.DataOffset, (SINT64)-1);
	STOREMOTOROLADWORD(vpcvhd.DiskType, 2);

	STOREMOTOROLAWORD(vpcvhd.Cylinder, (UINT16)C);
	vpcvhd.Heads = (UINT8)H;
	vpcvhd.SectorsPerCylinder = (UINT8)S;
	origsize = (UINT64)C * H * S * SS;
	STOREMOTOROLAQWORD(vpcvhd.OriginalSize, origsize);
	CopyMemory(&vpcvhd.CurrentSize, &vpcvhd.OriginalSize, 8);

	if(dynamic){
		UINT32 blockcount; 
		UINT32 i;
		UINT32 blocksize = 0x00200000;
		UINT32 nodata = 0xffffffff;
		
		STOREMOTOROLAQWORD(vpcvhd.DataOffset, (UINT64)(sizeof(VPCVHDFOOTER)));
		STOREMOTOROLADWORD(vpcvhd.DiskType, 3);

		checksum = vpc_calc_checksum((UINT8*)(&vpcvhd), footerlen);
		STOREMOTOROLADWORD(vpcvhd.CheckSum, checksum);
		
		blockcount = (UINT32)((origsize + blocksize - 1) / blocksize);
		ZeroMemory(&vpcvhd_dh, sizeof(VPCVHDDDH));
		CopyMemory(&vpcvhd_dh.Cookie, vpcvhd_sigDH, 8);
		STOREMOTOROLAQWORD(vpcvhd_dh.DataOffset, (SINT64)-1);
		STOREMOTOROLAQWORD(vpcvhd_dh.TableOffset, (UINT64)(sizeof(VPCVHDFOOTER) + sizeof(VPCVHDDDH)));
		STOREMOTOROLADWORD(vpcvhd_dh.HeaderVersion, 0x00010000);
		STOREMOTOROLADWORD(vpcvhd_dh.MaxTableEntries, blockcount);
		STOREMOTOROLADWORD(vpcvhd_dh.BlockSize, blocksize);

		checksum = vpc_calc_checksum((UINT8*)(&vpcvhd_dh), sizeof(VPCVHDDDH));
		STOREMOTOROLADWORD(vpcvhd_dh.CheckSum, checksum);

		r |= (file_write(fh, &vpcvhd, sizeof(vpcvhd)) == sizeof(vpcvhd)) ? SUCCESS : FAILURE;
		r |= (file_write(fh, &vpcvhd_dh, sizeof(vpcvhd_dh)) == sizeof(vpcvhd_dh)) ? SUCCESS : FAILURE;
		for(i=0;i<blockcount;i++){
			r |= (file_write(fh, &nodata, sizeof(UINT32))) ? SUCCESS : FAILURE;
			*progress = 100 * i / blockcount;
		}
		blockcount = 512 - (blockcount*4) % 512;
		nodata = 0;
		for(i=0;i<blockcount;i++){
			r |= (file_write(fh, &nodata, sizeof(UINT8))) ? SUCCESS : FAILURE;
			*progress = 100 * i / blockcount;
		}
	}else{
		checksum = vpc_calc_checksum((UINT8*)(&vpcvhd), footerlen);
		STOREMOTOROLADWORD(vpcvhd.CheckSum, checksum);

		r = writehddiplex2(fh, SS, (FILELEN)origsize, blank, progress, cancel);
	}

	r |= (file_write(fh, &vpcvhd, sizeof(vpcvhd)) == sizeof(vpcvhd)) ? SUCCESS : FAILURE;

	file_close(fh);
	if (r != SUCCESS) {
		file_delete(fname);
	}

vpcvhd_err:
	return;
}
void newdisk_vpcvhd_ex(const OEMCHAR *fname, UINT hddsize, int dynamic, int blank, int *progress, int *cancel) {
	
	UINT32 C;
	UINT16 H;
	UINT16 S;
	UINT16 SS;
	
	hddsize2CHS(hddsize, &C, &H, &S, &SS);

	newdisk_vpcvhd_ex_CHS(fname, C, H, S, SS, dynamic, blank, progress, cancel);
}
void newdisk_vpcvhd(const OEMCHAR *fname, UINT hddsize) {
	
	int progress;
	int cancel = 0;
	
	newdisk_vpcvhd_ex(fname, hddsize, 0, 0, &progress, &cancel);
}
#endif
