#include	<compiler.h>
#include	<dosio.h>
#include	<cpucore.h>
#include	<fdd/sxsi.h>

#ifdef SUPPORT_PHYSICAL_CDDRV

#include	<winioctl.h>
#include	<ntddcdrm.h>

#pragma warning (push)
#pragma warning (disable: 4091)
#include <ntddscsi.h>
#pragma warning (pop)

#include	"diskimage/cddfile.h"
#include	"diskimage/cd/cdd_real.h"

UINT64 msf2lba(UINT64 msf){
	UINT64 m,s,f;
	m = (msf >> 16) & 0xffffffffffff;
	s = (msf >>  8) & 0xff;
	f = (msf >>  0) & 0xff;
	return (((m * 60) + s) * 75) + f;
}

struct sptdinfo
{
	SCSI_PASS_THROUGH info;
	char sense_buffer[20];
	UCHAR ucDataBuf[16384];
};

struct sptdreadcapacityinfo
{
	SCSI_PASS_THROUGH info;
	char sense_buffer[20];
	UCHAR ucDataBuf[8];
};


//	----
//	イメージファイル内全トラックセクタ長2048byte用
REG8 sec2048_read_SPTI(SXSIDEV sxsi, FILEPOS pos, UINT8 *buf, UINT size) {

	CDINFO	cdinfo;
	FILEH	fh;
	//UINT	rsize;

	if (sxsi_prepare(sxsi) != SUCCESS) {
		return(0x60);
	}
	if ((pos < 0) || (pos >= sxsi->totals)) {
		return(0x40);
	}

	cdinfo = (CDINFO)sxsi->hdl;
	fh = cdinfo->fh;

	size /= 2048;

	if (1 || size != 2048) {
		DWORD sptisize = 0;
		struct sptdinfo* psptd = (struct sptdinfo*)malloc(sizeof(struct sptdinfo));
		if (!psptd) {
			return(0x60);
		}
		memset(psptd, 0, sizeof(struct sptdinfo));
		//UCHAR ucDataBuf[2048*8];

		// READ command
		psptd->info.Cdb[0] = 0x28;//0xBE;//0x3C;//0xBE;
		// Don't care about sector type.
		//psptd->info.Cdb[1] = 0;
		//psptd->info.Cdb[2] = (pos >> 24) & 0xFF;
		//psptd->info.Cdb[3] = (pos >> 16) & 0xFF;
		//psptd->info.Cdb[4] = (pos >> 8) & 0xFF;
		//psptd->info.Cdb[5] = pos & 0xFF;
		//psptd->info.Cdb[6] = (size >> 16) & 0xFF;
		//psptd->info.Cdb[7] = (size >> 8) & 0xFF;
		//psptd->info.Cdb[8] = size & 0xFF;
		//// Sync + all headers + user data + EDC/ECC. Excludes C2 + subchannel
		psptd->info.Cdb[1] = 2;                         // Data mode
		psptd->info.Cdb[2] = (pos >> 24) & 0xFF;
		psptd->info.Cdb[3] = (pos >> 16) & 0xFF;
		psptd->info.Cdb[4] = (pos >> 8) & 0xFF;
		psptd->info.Cdb[5] = pos & 0xFF;
		psptd->info.Cdb[6] = (size >> 16) & 0xFF;
		psptd->info.Cdb[7] = (size >> 8) & 0xFF;
		psptd->info.Cdb[8] = size & 0xFF;
		psptd->info.Cdb[9] = 0xF8;
		psptd->info.Cdb[10] = 0;
		psptd->info.Cdb[11] = 0;

		psptd->info.CdbLength = 12;
		psptd->info.Length = sizeof(SCSI_PASS_THROUGH);
		psptd->info.DataIn = SCSI_IOCTL_DATA_IN;
		psptd->info.DataTransferLength = 2048 * size;
		psptd->info.DataBufferOffset = offsetof(struct sptdinfo, ucDataBuf);
		//psptd->info.DataBuffer = ucDataBuf;
		psptd->info.SenseInfoLength = sizeof(psptd->sense_buffer);
		psptd->info.SenseInfoOffset = offsetof(struct sptdinfo, sense_buffer);
		psptd->info.TimeOutValue = 5;

		if (psptd->info.DataTransferLength > sizeof(psptd->ucDataBuf)) return 0xd0;

		if (DeviceIoControl(fh, IOCTL_SCSI_PASS_THROUGH, psptd, offsetof(struct sptdinfo, sense_buffer) + sizeof(psptd->sense_buffer), psptd, offsetof(struct sptdinfo, ucDataBuf) + 2048 * size, &sptisize, FALSE))
		{
			if (psptd->info.DataTransferLength != 0) {
				memcpy(buf, psptd->ucDataBuf, psptd->info.DataTransferLength);
				free(psptd);
				return(0x00);
			}
		}
		free(psptd);
	}

	return(0xd0);
}

//	イメージファイルの実体を開き、各種情報構築
BRESULT setsxsidev_SPTI(SXSIDEV sxsi, const OEMCHAR *path, const _CDTRK *trk, UINT trks) {

	FILEH	fh;
	long	totals;
	CDINFO	cdinfo;
	UINT	mediatype;
	UINT	i;
#ifdef	TOCLOGOUT
	OEMCHAR		logpath[MAX_PATH];
	OEMCHAR		logbuf[2048];
	TEXTFILEH	tfh;
#endif

	//	trk、trksは有効な値が設定済みなのが前提
	if ((trk == NULL) || (trks == 0)) {
		goto sxsiope_err1;
	}

	fh = CreateFile(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (fh == FILEH_INVALID) {
		goto sxsiope_err1;
	}

	cdinfo = (CDINFO)_MALLOC(sizeof(_CDINFO), path);
	if (cdinfo == NULL) {
		goto sxsiope_err2;
	}
	ZeroMemory(cdinfo, sizeof(_CDINFO));
	cdinfo->fh = fh;
	trks = min(trks, NELEMENTS(cdinfo->trk) - 1);
	CopyMemory(cdinfo->trk, trk, trks * sizeof(_CDTRK));

#ifdef	TOCLOGOUT
	file_cpyname(logpath, path, NELEMENTS(logpath));
	file_cutext(logpath);
	file_catname(logpath, str_logB, NELEMENTS(logpath));

	tfh = textfile_create(logpath, 0x800);
	if (tfh == NULL) {
		return(FAILURE);
	}

	TOCLOG(OEMTEXT("STR _CDTRK LOG\r\n"), 0);
	for (i = 0; i < trks; i++) {
		TOCLOG(OEMTEXT("trk[%02d]\r\n"), i);
		TOCLOG(OEMTEXT("  adr_ctl        = 0x%02X\r\n"),     cdinfo->trk[i].adr_ctl);
		TOCLOG(OEMTEXT("  point          = %02d\r\n"),       cdinfo->trk[i].point);
		TOCLOG(OEMTEXT("  [pos0][pos][ ]              = [%18I32d]"), cdinfo->trk[i].pos0);
		TOCLOG(OEMTEXT("[%18I32d][                  ]\r\n"),         cdinfo->trk[i].pos);
		TOCLOG(OEMTEXT("  sec[ ][str][end]            = [                  ][%18I32d]"), cdinfo->trk[i].str_sec);
		TOCLOG(OEMTEXT("[%18I32d]\r\n"), cdinfo->trk[i].end_sec);
		TOCLOG(OEMTEXT("  sectors        = %I32d\r\n"),      cdinfo->trk[i].sectors);
		TOCLOG(OEMTEXT("  sector_size    = %d\r\n"),         cdinfo->trk[i].sector_size);
		TOCLOG(OEMTEXT("  sector [pregap][start][end] = [%18I32d]"), cdinfo->trk[i].pregap_sector);
		TOCLOG(OEMTEXT("[%18I32d]"),     cdinfo->trk[i].start_sector);
		TOCLOG(OEMTEXT("[%18I32d]\r\n"), cdinfo->trk[i].end_sector);
		TOCLOG(OEMTEXT("  img_sec[pregap][start][end] = [%18I32d]"), cdinfo->trk[i].img_pregap_sec);
		TOCLOG(OEMTEXT("[%18I32d]"),     cdinfo->trk[i].img_start_sec);
		TOCLOG(OEMTEXT("[%18I32d]\r\n"), cdinfo->trk[i].img_end_sec);
		TOCLOG(OEMTEXT("  offset [pregap][start][end] = [0x%016I64X]"), cdinfo->trk[i].pregap_offset);
		TOCLOG(OEMTEXT("[0x%016I64X]"),     cdinfo->trk[i].start_offset);
		TOCLOG(OEMTEXT("[0x%016I64X]\r\n"), cdinfo->trk[i].end_offset);
		TOCLOG(OEMTEXT("  pregap_sectors = %I32d\r\n"),      cdinfo->trk[i].pregap_sectors);
		TOCLOG(OEMTEXT("  track_sectors  = %I32d\r\n"),      cdinfo->trk[i].track_sectors);
	}
	TOCLOG(OEMTEXT("END _CDTRK LOG\r\n"), 0);

	textfile_close(tfh);
#endif

#if 1
	if (sxsi->totals == -1) {
		totals = set_trkinfo(fh, cdinfo->trk, trks, 0);
		if (totals < 0) {
			goto sxsiope_err3;
		}
		sxsi->totals = totals;
	}
#else
	totals = issec(fh, cdinfo->trk, trks);	//	とりあえず
	sxsi->read = sec2048_read;
	totals = issec2048(cdinfo->fh);
	if (totals < 0) {
		sxsi->read = sec2352_read;
		totals = issec2352(cdinfo->fh);
	}
	if (totals < 0) {
		sxsi->read = sec2448_read;
		totals = issec2448(cdinfo->fh);
	}
	if (totals < 0) {
		sxsi->read = sec_read;
		totals = issec(cdinfo->fh, cdinfo->trk, trks);
	}
	if (totals < 0) {
		goto sxsiope_err3;
	}
#endif

	mediatype = 0;
	for (i = 0; i < trks; i++) {
		if (cdinfo->trk[i].adr_ctl == TRACKTYPE_DATA) {
			mediatype |= SXSIMEDIA_DATA;
		}
		else if (cdinfo->trk[i].adr_ctl == TRACKTYPE_AUDIO) {
			mediatype |= SXSIMEDIA_AUDIO;
		}
	}

	//	リードアウトトラックを生成
	cdinfo->trk[trks].adr_ctl	= 0x10;
	cdinfo->trk[trks].point		= 0xaa;
//	cdinfo->trk[trks].pos		= totals;
	cdinfo->trk[trks].pos		= (UINT32)sxsi->totals;

	cdinfo->trks = trks;
	file_cpyname(cdinfo->path, path, NELEMENTS(cdinfo->path));

	sxsi->reopen		= cd_reopen;
	sxsi->close			= cd_close;
	sxsi->destroy		= cd_destroy;
	sxsi->hdl			= (INTPTR)cdinfo;
//	sxsi->totals		= totals;
	sxsi->cylinders		= 0;
	sxsi->size			= 2352;
	sxsi->sectors		= 1;
	sxsi->surfaces		= 1;
	sxsi->headersize	= 0;
	sxsi->mediatype		= mediatype;

#ifdef	TOCLOGOUT
	file_cpyname(logpath, path, NELEMENTS(logpath));
	file_cutext(logpath);
	file_catname(logpath, str_logA, NELEMENTS(logpath));

	tfh = textfile_create(logpath, 0x800);
	if (tfh == NULL) {
		return(FAILURE);
	}

	TOCLOG(OEMTEXT("STR _CDTRK LOG\r\n"), 0);
	for (i = 0; i < trks; i++) {
		TOCLOG(OEMTEXT("trk[%02d]\r\n"), i);
		TOCLOG(OEMTEXT("  adr_ctl        = 0x%02X\r\n"),     cdinfo->trk[i].adr_ctl);
		TOCLOG(OEMTEXT("  point          = %02d\r\n"),       cdinfo->trk[i].point);
		TOCLOG(OEMTEXT("  [pos0][pos][ ]              = [%18I32d]"), cdinfo->trk[i].pos0);
		TOCLOG(OEMTEXT("[%18I32d][                  ]\r\n"),         cdinfo->trk[i].pos);
		TOCLOG(OEMTEXT("  sec[ ][str][end]            = [                  ][%18I32d]"), cdinfo->trk[i].str_sec);
		TOCLOG(OEMTEXT("[%18I32d]\r\n"), cdinfo->trk[i].end_sec);
		TOCLOG(OEMTEXT("  sectors        = %I32d\r\n"),      cdinfo->trk[i].sectors);
		TOCLOG(OEMTEXT("  sector_size    = %d\r\n"),         cdinfo->trk[i].sector_size);
		TOCLOG(OEMTEXT("  sector [pregap][start][end] = [%18I32d]"), cdinfo->trk[i].pregap_sector);
		TOCLOG(OEMTEXT("[%18I32d]"),     cdinfo->trk[i].start_sector);
		TOCLOG(OEMTEXT("[%18I32d]\r\n"), cdinfo->trk[i].end_sector);
		TOCLOG(OEMTEXT("  img_sec[pregap][start][end] = [%18I32d]"), cdinfo->trk[i].img_pregap_sec);
		TOCLOG(OEMTEXT("[%18I32d]"),     cdinfo->trk[i].img_start_sec);
		TOCLOG(OEMTEXT("[%18I32d]\r\n"), cdinfo->trk[i].img_end_sec);
		TOCLOG(OEMTEXT("  offset [pregap][start][end] = [0x%016I64X]"), cdinfo->trk[i].pregap_offset);
		TOCLOG(OEMTEXT("[0x%016I64X]"),     cdinfo->trk[i].start_offset);
		TOCLOG(OEMTEXT("[0x%016I64X]\r\n"), cdinfo->trk[i].end_offset);
		TOCLOG(OEMTEXT("  pregap_sectors = %I32d\r\n"),      cdinfo->trk[i].pregap_sectors);
		TOCLOG(OEMTEXT("  track_sectors  = %I32d\r\n"),      cdinfo->trk[i].track_sectors);
	}
	TOCLOG(OEMTEXT("END _CDTRK LOG\r\n"), 0);

	textfile_close(tfh);
#endif

	return(SUCCESS);

sxsiope_err3:
	_MFREE(cdinfo);

sxsiope_err2:
	file_close(fh);

sxsiope_err1:
	return(FAILURE);
}

//	----
//	セクタ長取得用（でもREAD CAPACITYコマンドに返事してくれない場合があるような･･･）
UINT32 readcapacity_SPTI(FILEH fh) {

	//CDINFO	cdinfo;
	UINT	rsize;

	{
		DWORD sptisize=0;
		struct sptdreadcapacityinfo sptd = {0};
		// READ_CAPACITY command
		sptd.info.Cdb[0] = 0x25;
		sptd.info.Cdb[9] = 0xF8;

		sptd.info.CdbLength = 10;
		sptd.info.Length = sizeof(SCSI_PASS_THROUGH);
		sptd.info.DataIn = SCSI_IOCTL_DATA_IN;
		sptd.info.DataTransferLength = 8;
		sptd.info.DataBufferOffset = offsetof(struct sptdreadcapacityinfo, ucDataBuf);
		//sptd.info.DataBuffer = ucDataBuf;
		sptd.info.SenseInfoLength = sizeof(sptd.sense_buffer);
		sptd.info.SenseInfoOffset = offsetof(struct sptdreadcapacityinfo, sense_buffer);
		sptd.info.TimeOutValue = 5;
		
		//memset(buf, 0, 2048 * size);
		//memset(ucDataBuf, 0, 16384);
		//SetLastError(0);
		if (DeviceIoControl(fh, IOCTL_SCSI_PASS_THROUGH, &sptd, offsetof(struct sptdreadcapacityinfo, sense_buffer)+sizeof(sptd.sense_buffer), &sptd, offsetof(struct sptdreadcapacityinfo, ucDataBuf)+8, &sptisize, FALSE))
		{
			if (sptd.info.DataTransferLength != 0){
				int secsize = LOADMOTOROLADWORD(sptd.ucDataBuf+4);
				return(secsize);
			}
		}
	}


	//if (file_seek(fh, pos, FSEEK_SET) != pos) {
	//	return(0xd0);
	//}

	//while(size) {
	//	rsize = min(size, 2048);
	//	CPU_REMCLOCK -= rsize;
	//	if (file_read(fh, buf, rsize) != rsize) {
	//		return(0xd0);
	//	}
	//	buf += rsize;
	//	size -= rsize;
	//}
	//
	rsize = GetLastError();
	return 0;
}

//	実ドライブを開く
BRESULT openrealcdd(SXSIDEV sxsi, const OEMCHAR *path) {

	_CDTRK	trk[99];
	UINT	trks;
	FILEH	fh;
	UINT16	sector_size;
	FILELEN	totals;
	DISK_GEOMETRY dgCDROM;
	CDROM_TOC tocCDROM;
	DWORD dwNotUsed;
	UINT i;

	ZeroMemory(trk, sizeof(trk));
	trks = 0;
	
	//fh = file_open_rb(path);
	fh = CreateFile(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	//fh = file_open(path);
	if (fh == FILEH_INVALID) {
		goto openiso_err1;
	}
	
	//	セクタサイズが2048byte、2352byte、2448byteのどれかをチェック
	DeviceIoControl(fh, IOCTL_CDROM_GET_DRIVE_GEOMETRY,
				NULL, 0, &dgCDROM, sizeof(DISK_GEOMETRY),
				&dwNotUsed, NULL);

	if(dgCDROM.MediaType != 11){ // Removable Media Check
		goto openiso_err2;
	}
	
	sector_size = (UINT16)dgCDROM.BytesPerSector;
	totals = (FILELEN)dgCDROM.SectorsPerTrack*dgCDROM.TracksPerCylinder*dgCDROM.Cylinders.QuadPart;
	switch(sector_size){
	case 2048:
		if(readcapacity_SPTI(fh) == 2048){
			sxsi->read = sec2048_read_SPTI;
		}else{
			sxsi->read = sec2048_read;
		}
		break;
	case 2352:
		sxsi->read = sec2352_read;
		break;
	case 2448:
		sxsi->read = sec2448_read;
		break;
	default:
		goto openiso_err2;
	}
	
	//	トラック情報を拾う
	DeviceIoControl(fh, IOCTL_CDROM_READ_TOC,
				NULL, 0, &tocCDROM, sizeof(tocCDROM),
				&dwNotUsed, NULL);

	trks = tocCDROM.LastTrack - tocCDROM.FirstTrack + 1;
	if (trks <= 0) goto openiso_err2;
	for(i=0;i<trks;i++){
		if((tocCDROM.TrackData[i].Control & 0xC) == 0x4){
			trk[i].adr_ctl		= TRACKTYPE_DATA;
		}else{
			trk[i].adr_ctl		= TRACKTYPE_AUDIO;
		}
		trk[i].point			= tocCDROM.TrackData[i].TrackNumber;
		trk[i].pos				= (UINT32)(msf2lba(LOADMOTOROLADWORD(tocCDROM.TrackData[i].Address)) - 150);
		trk[i].pos0				= trk[i].pos;

		trk[i].sector_size	= sector_size;

		trk[i].pregap_sector	= trk[i].pos;
		//trk[i].start_sector	= trk[i].pos;
		if(i==trks-1){
			trk[i].end_sector	= (UINT32)totals;
		}else{
			trk[i].end_sector	= (UINT32)(msf2lba(LOADMOTOROLADWORD(tocCDROM.TrackData[i+1].Address)) - 150 - 1);
		}

		trk[i].img_pregap_sec	= trk[i].pregap_sector;
		trk[i].img_start_sec	= trk[i].start_sector;
		trk[i].img_end_sec		= trk[i].end_sector;

		trk[i].pregap_sectors	= 0;
		trk[i].track_sectors	= trk[i].end_sector - trk[i].start_sector + 1;
		
		trk[i].str_sec		= trk[i].start_sector;
		trk[i].end_sec		= trk[i].end_sector;
		trk[i].sectors		= trk[i].track_sectors;
		
		trk[i].pregap_offset	= (UINT64)trk[i].start_sector * trk[i].sector_size;
		trk[i].start_offset		= (UINT64)trk[i].start_sector * trk[i].sector_size;
		trk[i].end_offset		= (UINT64)trk[i].end_sector * trk[i].sector_size;
	}

	//trk[0].adr_ctl			= TRACKTYPE_DATA;
	//trk[0].point			= 1;
	//trk[0].pos				= 0;
	//trk[0].pos0				= 0;

	//trk[0].sector_size		= sector_size;

	//trk[0].pregap_sector	= 0;
	//trk[0].start_sector		= 0;
	//trk[0].end_sector		= totals;

	//trk[0].img_pregap_sec	= 0;
	//trk[0].img_start_sec	= 0;
	//trk[0].img_end_sec		= totals;

	//trk[0].pregap_offset	= 0;
	//trk[0].start_offset		= 0;
	//trk[0].end_offset		= totals * sector_size;

	//trk[0].pregap_sectors	= 0;
	//trk[0].track_sectors	= totals;
	//trks = 1;

	sxsi->totals = trk[trks-1].end_sector;

	file_close(fh);

	return(setsxsidev_SPTI(sxsi, path, trk, trks));

openiso_err2:
	file_close(fh);

openiso_err1:
	return(FAILURE);
}

#endif

//	_CDTRK	trk[99];
//	UINT	trks;
//	FILEH	fh;
//	UINT16	sector_size;
//	FILELEN	totals;
//	DISK_GEOMETRY dgCDROM;
//	CDROM_TOC_FULL_TOC_DATA tocCDROMtmp;
//	CDROM_TOC_FULL_TOC_DATA *lptocCDROM;
//	DWORD dwNotUsed;
//	int i;
//	WORD  lentmp;
//	CDROM_READ_TOC_EX TOCEx={0};
//
//	TOCEx.Format=CDROM_READ_TOC_EX_FORMAT_FULL_TOC;
//	TOCEx.Msf=1;
//
//	ZeroMemory(trk, sizeof(trk));
//	trks = 0;
//	
//	//fh = CreateFile(path, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
//	fh = file_open_rb(path);
//	if (fh == FILEH_INVALID) {
//		goto openiso_err1;
//	}
//
//	//	セクタサイズが2048byte、2352byte、2448byteのどれかをチェック
//	DeviceIoControl(fh, IOCTL_CDROM_GET_DRIVE_GEOMETRY,
//				NULL, 0, &dgCDROM, sizeof(DISK_GEOMETRY),
//				&dwNotUsed, NULL);
//
//	if(dgCDROM.MediaType != 11){ // Removable Media Check
//		goto openiso_err2;
//	}
//	sector_size = dgCDROM.BytesPerSector;
//	totals = dgCDROM.SectorsPerTrack*dgCDROM.TracksPerCylinder*dgCDROM.Cylinders.QuadPart;
//	switch(sector_size){
//	case 2048:
//		sxsi->read = sec2048_read;
//		break;
//	case 2352:
//		sxsi->read = sec2352_read;
//		break;
//	case 2448:
//		sxsi->read = sec2448_read;
//		break;
//	default:
//		goto openiso_err2;
//	}
//	
//	//	トラック情報を拾う
//	DeviceIoControl(fh, IOCTL_CDROM_READ_TOC_EX,
//				&TOCEx, sizeof(TOCEx), &tocCDROMtmp, sizeof(tocCDROMtmp),
//				&dwNotUsed, NULL);
//	trks = (LOADMOTOROLAWORD(tocCDROMtmp.Length) - sizeof(UCHAR)*2) / sizeof(CDROM_TOC_FULL_TOC_DATA_BLOCK);
//	lentmp = sizeof(CDROM_TOC_FULL_TOC_DATA) + trks * sizeof(CDROM_TOC_FULL_TOC_DATA_BLOCK);
//	lptocCDROM = (CDROM_TOC_FULL_TOC_DATA*)calloc(lentmp, 1);
//	
//	DeviceIoControl(fh, IOCTL_CDROM_READ_TOC_EX,
//				&TOCEx, sizeof(TOCEx), lptocCDROM, lentmp,
//				&dwNotUsed, NULL);
//
//	//trks = tocCDROM.LastTrack - tocCDROM.FirstTrack + 1;
//	for(i=0;i<trks;i++){
//		if((lptocCDROM->Descriptors[i].Control & 0xC) == 0x4){
//			trk[i].adr_ctl		= TRACKTYPE_DATA;
//		}else{
//			trk[i].adr_ctl		= TRACKTYPE_AUDIO;
//		}
//		trk[i].point			= lptocCDROM->Descriptors[i].SessionNumber;
//		trk[i].pos				= (msf2lba(LOADMOTOROLADWORD(lptocCDROM->Descriptors[i].Msf)) - 150);
//		trk[i].pos0				= trk[i].pos;
//
//		trk[i].sector_size	= sector_size;
//
//		trk[i].pregap_sector	= trk[i].pos;
//		//trk[i].start_sector		= trk[i].pos;
//		if(i==trks-1){
//			trk[i].end_sector		= totals;
//		}else{
//			trk[i].end_sector		= (msf2lba(LOADMOTOROLADWORD(lptocCDROM->Descriptors[i].Msf)) - 150 - 1);
//		}
//
//		trk[i].img_pregap_sec	= trk[i].pregap_sector;
//		trk[i].img_start_sec	= trk[i].start_sector;
//		trk[i].img_end_sec		= trk[i].end_sector;
//
//		trk[i].pregap_sectors	= 0;
//		trk[i].track_sectors	= trk[i].end_sector - trk[i].start_sector + 1;
//		
//		trk[i].str_sec		= trk[i].start_sector;
//		trk[i].end_sec		= trk[i].end_sector;
//		trk[i].sectors		= trk[i].track_sectors;
//		
//		trk[i].pregap_offset	= trk[i].start_sector * trk[i].sector_size;
//		trk[i].start_offset		= trk[i].start_sector * trk[i].sector_size;
//		trk[i].end_offset		= trk[i].end_sector * trk[i].sector_size;
//	}
//
//	//trk[0].adr_ctl			= TRACKTYPE_DATA;
//	//trk[0].point			= 1;
//	//trk[0].pos				= 0;
//	//trk[0].pos0				= 0;
//
//	//trk[0].sector_size		= sector_size;
//
//	//trk[0].pregap_sector	= 0;
//	//trk[0].start_sector		= 0;
//	//trk[0].end_sector		= totals;
//
//	//trk[0].img_pregap_sec	= 0;
//	//trk[0].img_start_sec	= 0;
//	//trk[0].img_end_sec		= totals;
//
//	//trk[0].pregap_offset	= 0;
//	//trk[0].start_offset		= 0;
//	//trk[0].end_offset		= totals * sector_size;
//
//	//trk[0].pregap_sectors	= 0;
//	//trk[0].track_sectors	= totals;
//	//trks = 1;
//
//	sxsi->totals = trk[trks-1].end_sector;
//
//	file_close(fh);
//
//	free(lptocCDROM);
//
//	return(setsxsidev(sxsi, path, trk, trks));
//
//openiso_err2:
//	file_close(fh);
//
//openiso_err1:
//	return(FAILURE);
