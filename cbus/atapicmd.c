#include	<compiler.h>

#if 0
#undef	TRACEOUT
#define	TRACEOUT(s)	(void)(s)
#endif	/* 1 */

// これ、scsicmdとどう統合するのよ？

#if defined(SUPPORT_IDEIO)
#if defined(_WINDOWS) && !defined(__LIBRETRO__)
#include	<process.h>
#endif

#include	<dosio.h>
#include	<cpucore.h>
#include	<pccore.h>
#include	<io/iocore.h>
#include	<cbus/ideio.h>
#include	<cbus/atapicmd.h>
#include	<fdd/sxsi.h>
#include	<codecnv/codecnv.h>

#define	YUIDEBUG

#define	SUPPORT_NECCDD

#define HEX2BCD(hex)	( (((hex/10)%10)<<4)|((hex)%10) )
#define BCD2HEX(bcd)	( (((bcd>>4)&0xf)*10)+((bcd)&0xf) )

#if defined(_WINDOWS) && !defined(__LIBRETRO__)
static int atapi_thread_initialized = 0;
static HANDLE atapi_thread = NULL;
static IDEDRV atapi_thread_drv = NULL;
static HANDLE atapi_thread_event_request = NULL;
static HANDLE atapi_thread_event_complete = NULL;
	// TODO: 非Windows用コードを書く
#endif

// INQUIRY
static const UINT8 cdrom_inquiry[] = {
#ifdef YUIDEBUG
	// うちのドライブの奴 NECCDは Product Level 3.00以上で modesense10のコードがちげー
	0x05,	// CD-ROM
	0x80,	// bit7: Removable Medium Bit, other: Reserved
	0x00,	// version [7-6: ISO, ECMA: 5-3, 2-0: ANSI(00)]
	0x21,	// 7-4: ATAPI version, 3-0: Response Data Format
	0x1f,	// Additional length
	0x00,0x00,0x00,	// Reserved
	'N', 'E', 'C', ' ', ' ', ' ', ' ', ' ',	// Vendor ID
	'C', 'D', '-', 'R', 'O', 'M', ' ', 'D',	// Product ID
	'R', 'I', 'V', 'E', ':', '9', '8', ' ',	// Product ID
	'1', '.', '0', ' '	// Product Revision Level
#else
	0x05,	// CD-ROM
	0x80,	// bit7: Removable Medium Bit, other: Reserved
	0x00,	// version [7-6: ISO, ECMA: 5-3, 2-0: ANSI(00)]
	0x21,	// 7-4: ATAPI version, 3-0: Response Data Format
	0x1f,	// Additional length
	0x00,0x00,0x00,	// Reserved
	'N', 'E', 'C', ' ', ' ', ' ', ' ', ' ',	// Vendor ID
	'C', 'D', '-', 'R', 'O', 'M', ' ', 'D',	// Product ID
	'R', 'I', 'V', 'E', ' ', ' ', ' ', ' ',	// Product ID
	'1', '.', '0', ' '	// Product Revision Level
#endif
};

static void senddata(IDEDRV drv, UINT size, UINT limit) {

	size = MIN(size, limit);
	drv->sc = IDEINTR_IO;
	drv->cy = size;
	drv->status &= ~(IDESTAT_BSY|IDESTAT_DMRD|IDESTAT_SERV|IDESTAT_CHK);
	drv->status |= IDESTAT_DRQ|IDESTAT_DSC; // XXX: set Drive Seek Complete bit np21w ver0.86 rev29
	drv->error = 0;
	ATAPI_SET_SENSE_KEY(drv, ATAPI_SK_NO_SENSE);
	drv->asc = ATAPI_ASC_NO_ADDITIONAL_SENSE_INFORMATION;
	drv->bufdir = IDEDIR_IN;
	drv->buftc = IDETC_TRANSFEREND;
	drv->bufpos = 0;
	drv->bufsize = size;

	if (!(drv->ctrl & IDECTRL_NIEN)) {
		//TRACEOUT(("atapicmd: senddata()"));
		ideio.bank[0] = ideio.bank[1] | 0x80;			// ????
		pic_setirq(IDE_IRQ);
	}
}

static void cmddone(IDEDRV drv) {

	drv->sc = IDEINTR_IO|IDEINTR_CD;
	drv->status &= ~(IDESTAT_BSY|IDESTAT_DRQ|IDESTAT_SERV|IDESTAT_CHK);
	drv->status |= IDESTAT_DRDY|IDESTAT_DSC; // XXX: set Drive Seek Complete bit np21w ver0.86 rev13
	drv->error = 0;
	ATAPI_SET_SENSE_KEY(drv, ATAPI_SK_NO_SENSE);
	drv->asc = ATAPI_ASC_NO_ADDITIONAL_SENSE_INFORMATION;

	if (!(drv->ctrl & IDECTRL_NIEN)) {
		//TRACEOUT(("atapicmd: cmddone()"));
		ideio.bank[0] = ideio.bank[1] | 0x80;			// ????
		pic_setirq(IDE_IRQ);
	}
}

static void senderror(IDEDRV drv) {

	//drv->sc = IDEINTR_IO;
	drv->sc = IDEINTR_IO | IDEINTR_CD; // set Command or Data bit np21w ver0.86 rev38
	drv->status &= ~(IDESTAT_BSY|IDESTAT_DMRD|IDESTAT_SERV|IDESTAT_DRQ); // clear DRQ bit np21w ver0.86 rev38
	drv->status |= IDESTAT_CHK|IDESTAT_DSC;

	if (!(drv->ctrl & IDECTRL_NIEN)) {
		//TRACEOUT(("atapicmd: senderror()"));
		ideio.bank[0] = ideio.bank[1] | 0x80;			// ????
		pic_setirq(IDE_IRQ);
	}
}

static void sendabort(IDEDRV drv) {

	drv->sk = ATAPI_SK_ABORTED_COMMAND;
	drv->error = IDEERR_ABRT;
	senderror(drv);
}

static void stop_daplay(IDEDRV drv)
{

	/*
		"Play operation SHALL stopped" commands (INF-8090)
		0xA1 blank
		0x5B close track/session
		0x04 format unit
		0xA6 load/unload medium
		0x28 read(10)
		0xA8 read(12)
		0x58 repair rzone
		0x2B seek
		0x1B start/stop unit
		0x4E stop play/scan
		0x2F verify(10)
		0x2A write(10)
		0xAA write(12)
		0x2E write and verify(10)
		
		"Play operation SHALL NOT stopped" commands (INF-8090)
		0x46 get configuration
		0x4A get event/status notification
		0x12 inquiry
		0xBD mechanism status
		0x55 mode select
		0x5A mode sense
		0x1E prevent allow medium removal
		0x5C read buffer capacity
		0x25 read capacity
		0x03 request sense
		0xA7 set read ahead
		0x35 synchronize cache(10)
		0x00 test unit ready
	*/
	if (ideio.daplaying & (1 << (drv->sxsidrv & 3))) {
		/* stop playing audio */
		ideio.daplaying &= ~(1 << (drv->sxsidrv & 3));
		drv->daflag = 0x13;
		drv->dacurpos = 0;
		drv->dalength = 0;
	}
}

// ----- ATAPI packet command

static void atapi_cmd_start_stop_unit(IDEDRV drv);
static void atapi_cmd_prevent_allow_medium_removal(IDEDRV drv);
static void atapi_cmd_read_capacity(IDEDRV drv);
static void atapi_cmd_read(IDEDRV drv, UINT32 lba, UINT32 leng);
static void atapi_cmd_read_cd(IDEDRV drv, UINT32 lba, UINT32 leng);
static void atapi_cmd_read_cd_msf(IDEDRV drv);
static void atapi_cmd_mode_select(IDEDRV drv);
static void atapi_cmd_mode_sense(IDEDRV drv);
static void atapi_cmd_readsubch(IDEDRV drv);
static void atapi_cmd_readtoc(IDEDRV drv);
static void atapi_cmd_playaudio(IDEDRV drv);
static void atapi_cmd_playaudiomsf(IDEDRV drv);
static void atapi_cmd_pauseresume(IDEDRV drv);
static void atapi_cmd_seek(IDEDRV drv, UINT32 lba);
static void atapi_cmd_mechanismstatus(IDEDRV drv);

#define MEDIA_CHANGE_WAIT	6	// Waitを入れないとWinNT系で正しくメディア交換出来ない
static int mediachangeflag = 0;

extern REG8 cdchange_drv;
void cdchange_timeoutproc(NEVENTITEM item);

void atapicmd_a0(IDEDRV drv) {

	UINT32	lba, leng;
	UINT8	cmd;

	cmd = drv->buf[0];
	switch (cmd) {
	case 0x00:		// test unit ready
		TRACEOUT(("atapicmd: test unit ready"));
		if (!(drv->media & IDEIO_MEDIA_LOADED)) {
			/* medium not present */
			ATAPI_SET_SENSE_KEY(drv, ATAPI_SK_NOT_READY);
			drv->asc = ATAPI_ASC_MEDIUM_NOT_PRESENT;
			if(drv->sxsidrv==cdchange_drv && g_nevent.item[NEVENT_CDWAIT].clock > 0){
				if(mediachangeflag==MEDIA_CHANGE_WAIT){
					nevent_set(NEVENT_CDWAIT, 500, cdchange_timeoutproc, NEVENT_ABSOLUTE); // OS側がCDを催促しているようなので更に急いで交換
				}else if(mediachangeflag==0){
					//nevent_setbyms(NEVENT_CDWAIT, 1000, cdchange_timeoutproc, NEVENT_ABSOLUTE); // OS側がCDが無いと認識したようなので急いで交換
				}
			}
			if(mediachangeflag < MEDIA_CHANGE_WAIT) mediachangeflag++;
			//drv->status |= IDESTAT_ERR;
			//drv->error = IDEERR_MCNG;
			senderror(drv);
			break;
		}
		if (drv->media & IDEIO_MEDIA_CHANGED) {
			UINT8 olderror = drv->error;
			ATAPI_SET_SENSE_KEY(drv, ATAPI_SK_NOT_READY);
			if(drv->damsfbcd){
				// NECCDD.SYS
				//if(mediachangeflag){
				//if(mediachangeflag >= MEDIA_CHANGE_WAIT){
					drv->media &= ~IDEIO_MEDIA_CHANGED;
					drv->asc = ATAPI_ASC_NOT_READY_TO_READY_TRANSITION;
					//drv->error &= ~IDEERR_MCRQ;
				//}else{
				//	drv->asc = ATAPI_ASC_MEDIUM_NOT_PRESENT;
				//	mediachangeflag++;
				//	//drv->status |= IDESTAT_ERR;
				//	//drv->error |= IDEERR_MCRQ;
				//}
			}else{
				// for WinNT,2000 setup
				if(mediachangeflag >= MEDIA_CHANGE_WAIT){
					drv->media &= ~IDEIO_MEDIA_CHANGED;
					drv->asc = 0x0204; // LOGICAL DRIVE NOT READY - INITIALIZING COMMAND REQUIRED
				//}else if(mediachangeflag >= 1){
				//	drv->asc = 0x0204; // LOGICAL DRIVE NOT READY - INITIALIZING COMMAND REQUIRED
				//	mediachangeflag++;
				}else{
					drv->asc = ATAPI_ASC_MEDIUM_NOT_PRESENT;
//#if defined(CPUCORE_IA32)
//					// Workaround for WinNT
//					if (CPU_STAT_PM && !CPU_STAT_VM86) {
						//mediachangeflag++;
//					} else
//#endif
//					{
						mediachangeflag = MEDIA_CHANGE_WAIT;
					//}
				}
			}
			senderror(drv);
			break;
		}
		mediachangeflag = 0;
		//if(drv->error & IDEERR_MCNG){
		//	drv->status &= ~IDESTAT_ERR;
		//	drv->error &= ~IDEERR_MCNG;
		//}

		cmddone(drv);
		break;

	case 0x03:		// request sense
		TRACEOUT(("atapicmd: request sense"));
		leng = drv->buf[4];
		ZeroMemory(drv->buf, 18);
		drv->buf[0] = 0x70;
		drv->buf[2] = drv->sk;
		drv->buf[7] = 11;	// length
		drv->buf[12] = (UINT8)(drv->asc & 0xff); // Additional Sense Code
		drv->buf[13] = (UINT8)((drv->asc>>8) & 0xff); // Additional Sense Code Qualifier (Optional)
		senddata(drv, 18, leng);
		break;

	case 0x12:		// inquiry
		TRACEOUT(("atapicmd: inquiry"));
		leng = drv->buf[4];
		CopyMemory(drv->buf, cdrom_inquiry, sizeof(cdrom_inquiry));
		senddata(drv, sizeof(cdrom_inquiry), leng);
		break;

	case 0x1b:		// start stop unit
		TRACEOUT(("atapicmd: start stop unit"));
		atapi_cmd_start_stop_unit(drv);
		break;

	case 0x1e:		// prevent allow medium removal
		TRACEOUT(("atapicmd: prevent allow medium removal"));
		atapi_cmd_prevent_allow_medium_removal(drv);
		break;

	case 0x25:		// read capacity
		TRACEOUT(("atapicmd: read capacity"));
		atapi_cmd_read_capacity(drv);
		break;

	case 0x28:		// read(10)
		//TRACEOUT(("atapicmd: read(10)"));
		lba = (drv->buf[2] << 24) + (drv->buf[3] << 16) + (drv->buf[4] << 8) + drv->buf[5];
		leng = (drv->buf[7] << 8) + drv->buf[8];
		atapi_cmd_read(drv, lba, leng);
		break;
		
	case 0xbe:		// read cd
		lba = (drv->buf[2] << 24) + (drv->buf[3] << 16) + (drv->buf[4] << 8) + drv->buf[5];
		leng = (drv->buf[6] << 16) + (drv->buf[7] << 8) + drv->buf[8];
		atapi_cmd_read_cd(drv, lba, leng);
		break;
		
	case 0xb9:		// read cd msf
		atapi_cmd_read_cd_msf(drv);
		break;
		
	case 0x2b:		// Seek
		lba = (drv->buf[2] << 24) + (drv->buf[3] << 16) + (drv->buf[4] << 8) + drv->buf[5];
		atapi_cmd_seek(drv, lba);
		break;
		
	case 0x55:		// mode select
		TRACEOUT(("atapicmd: mode select"));
		atapi_cmd_mode_select(drv);
		break;

	case 0x5a:		// mode sense(10)
		TRACEOUT(("atapicmd: mode sense(10)"));
		atapi_cmd_mode_sense(drv);
		break;

	case 0x42:
		TRACEOUT(("atapicmd: read sub channel"));
		atapi_cmd_readsubch(drv);
		break;

	case 0x43:		// read TOC
		TRACEOUT(("atapicmd: read TOC"));
		atapi_cmd_readtoc(drv);
		break;

	case 0x45:		// Play Audio
		TRACEOUT(("atapicmd: Play Audio"));
		atapi_cmd_playaudio(drv);
		break;

	case 0x46:		// get config?
		TRACEOUT(("atapicmd: get config"));
		leng = drv->buf[7]|(drv->buf[8] << 8);
		ZeroMemory(drv->buf, 512);
		drv->buf[10] = 3;
		drv->buf[6] = 8;
		if(leng == 0) leng = 12;
		senddata(drv, 512, leng);
		break;

	case 0x47:		// Play Audio MSF
		TRACEOUT(("atapicmd: Play Audio MSF"));
		atapi_cmd_playaudiomsf(drv);
		break;

	case 0x4b:
		TRACEOUT(("atapicmd: pause resume"));
		atapi_cmd_pauseresume(drv);
		break;
		
	case 0xbd:		// mechanism status
		TRACEOUT(("atapicmd: mechanism status"));
		atapi_cmd_mechanismstatus(drv);
		break;
		
	default:
		TRACEOUT(("atapicmd: unknown command = %.2x", cmd));
		sendabort(drv);
		break;
	}
}


//-- command

// 0x1b: START/STOP UNIT
#ifdef SUPPORT_PHYSICAL_CDDRV
void atapi_cmd_traycmd_eject_threadfunc(void* vdParam) {
#if defined(_WINDOWS)
	HANDLE handle;
	DWORD dwRet = 0;
	wchar_t	wpath[MAX_PATH];
	codecnv_utf8toucs2(wpath, MAX_PATH, np2cfg.idecd[(int)vdParam], -1);
	handle = CreateFileW(wpath, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if(handle != INVALID_HANDLE_VALUE){
		if(DeviceIoControl(handle, FSCTL_LOCK_VOLUME, 0, 0, 0, 0, &dwRet, 0)){
			if(DeviceIoControl(handle, FSCTL_DISMOUNT_VOLUME, 0, 0, 0, 0, &dwRet, 0)){
				DeviceIoControl(handle, IOCTL_STORAGE_EJECT_MEDIA, 0, 0, 0, 0, &dwRet, 0);
			}
		}
		CloseHandle(handle);
	}
#else
	// TODO: Windows以外のコードを書く
#endif
}
void atapi_cmd_traycmd_close_threadfunc(void* vdParam) {
#if defined(_WINDOWS)
	HANDLE handle;
	DWORD dwRet = 0;
	wchar_t	wpath[MAX_PATH];
	codecnv_utf8toucs2(wpath, MAX_PATH, np2cfg.idecd[(int)vdParam], -1);
	handle = CreateFileW(wpath, GENERIC_READ, FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
	if(handle != INVALID_HANDLE_VALUE){
		DeviceIoControl(handle, IOCTL_STORAGE_LOAD_MEDIA, 0, 0, 0, 0, &dwRet, 0);
		CloseHandle(handle);
	}
#else
	// TODO: Windows以外のコードを書く
#endif
}
#endif
static void atapi_cmd_start_stop_unit(IDEDRV drv) {

	UINT	power;
	SXSIDEV		sxsi;

	sxsi = sxsi_getptr(drv->sxsidrv);

	stop_daplay(drv);

	power = (drv->buf[4] >> 4);
	if (power != 0) {
		/* power control is not supported */
		ATAPI_SET_SENSE_KEY(drv, ATAPI_SK_ILLEGAL_REQUEST);
		drv->asc = ATAPI_ASC_INVALID_FIELD_IN_CDB;
		goto send_error;
	}
	switch(drv->buf[4] & 3){
	case 0: // Stop the Disc
		break;
	case 1: // Start the Disc and read the TOC
		if (!(drv->media & IDEIO_MEDIA_LOADED)) {
			atapi_cmd_readtoc(drv);
			return;
		}
		break;
	case 2: // Eject the Disc if possible
#ifdef SUPPORT_PHYSICAL_CDDRV
		if(np2cfg.allowcdtraycmd && _tcsnicmp(np2cfg.idecd[sxsi->drv], OEMTEXT("\\\\.\\"), 4)==0){
#if defined(_WINDOWS) && !defined(__LIBRETRO__)
			_beginthread(atapi_cmd_traycmd_eject_threadfunc, 0, (void*)sxsi->drv);
#else
			// TODO: Windows以外のコードを書く
#endif
		}else
#endif
		{
			ATAPI_SET_SENSE_KEY(drv, ATAPI_SK_ILLEGAL_REQUEST);
			drv->asc = ATAPI_ASC_INVALID_FIELD_IN_CDB;
			goto send_error;
		}
		break;
	case 3: // Load the Disc (Close Tray)
#ifdef SUPPORT_PHYSICAL_CDDRV
		if(np2cfg.allowcdtraycmd && _tcsnicmp(np2cfg.idecd[sxsi->drv], OEMTEXT("\\\\.\\"), 4)==0){
#if defined(_WINDOWS) && !defined(__LIBRETRO__)
			_beginthread(atapi_cmd_traycmd_close_threadfunc, 0, (void*)sxsi->drv);
#else
			// TODO: Windows以外のコードを書く
#endif
		}else
#endif
		{
			ATAPI_SET_SENSE_KEY(drv, ATAPI_SK_ILLEGAL_REQUEST);
			drv->asc = ATAPI_ASC_INVALID_FIELD_IN_CDB;
			goto send_error;
		}
		break;
	}
	if (!(drv->media & IDEIO_MEDIA_LOADED)) {
		/* medium not present */
		ATAPI_SET_SENSE_KEY(drv, ATAPI_SK_NOT_READY);
		drv->asc = ATAPI_ASC_MEDIUM_NOT_PRESENT;
		goto send_error;
	}

	/* XXX play/read TOC, stop */

	cmddone(drv);

	return;

send_error:
	senderror(drv);
}

// 0x1e: PREVENT/ALLOW MEDIUM REMOVAL
static void atapi_cmd_prevent_allow_medium_removal(IDEDRV drv) {

	/* XXX */
	cmddone(drv);
}

// 0x25: READ CAPACITY
static void atapi_cmd_read_capacity(IDEDRV drv) {

	/* XXX */
	UINT32 blklen = 2048; // drv->secsize;
	UINT8 *b = drv->buf;
	SXSIDEV sxsi;
	UINT32 totals;

	sxsi = sxsi_getptr(drv->sxsidrv);
	if ((sxsi == NULL) || (!(sxsi->flag & SXSIFLAG_READY) && drv->device != IDETYPE_CDROM)) {
		senderror(drv);
		return;
	}
	totals = (UINT32)sxsi->totals;

	b[0] = (UINT8)(totals >> 24);
	b[1] = (UINT8)(totals >> 16);
	b[2] = (UINT8)(totals >> 8);
	b[3] = (UINT8)(totals);
	b[4] = (UINT8)(blklen >> 24);
	b[5] = (UINT8)(blklen >> 16);
	b[6] = (UINT8)(blklen >> 8);
	b[7] = (UINT8)(blklen);
	
	senddata(drv, 8, 8);

	//cmddone(drv);
}

// 0x28: READ(10)
#if defined(_WINDOWS) && !defined(__LIBRETRO__)
static int atapi_dataread_error = -1;
void atapi_dataread_threadfunc_part(IDEDRV drv) {

	SXSIDEV	sxsi;
	sxsi = sxsi_getptr(drv->sxsidrv);
	sxsi->cdflag_ecc = (sxsi->cdflag_ecc & ~CD_ECC_BITMASK) | CD_ECC_NOERROR;

	if (sxsi_read(drv->sxsidrv, drv->sector, drv->buf, 2048) != 0) {
		atapi_dataread_error = 1;
		return;
	}

	// EDC/ECC check
	if(np2cfg.usecdecc && (sxsi->cdflag_ecc & CD_ECC_BITMASK)==CD_ECC_ERROR){
		atapi_dataread_error = 2;
		return;
	}
	
	atapi_dataread_error = 0;
}
void atapi_dataread_asyncwait(int wait) {
	if(atapi_dataread_error!=-1 && atapi_thread_drv && (!np2cfg.useasynccd || !atapi_thread || WaitForSingleObject(atapi_thread_event_complete, wait) == WAIT_OBJECT_0)){
		IDEDRV drv = atapi_thread_drv;
		SXSIDEV	sxsi;
		sxsi = sxsi_getptr(drv->sxsidrv);
		drv->status &= ~(IDESTAT_DRQ);

		switch(atapi_dataread_error){
		case 1:
			ATAPI_SET_SENSE_KEY(drv, ATAPI_SK_ILLEGAL_REQUEST);
			drv->asc = 0x21;
			sxsi->cdflag_ecc = (sxsi->cdflag_ecc & ~CD_ECC_BITMASK) | CD_ECC_NOERROR;
			senderror(drv);
			TRACEOUT(("atapicmd: read error at sector %d", drv->sector));
			break;
		case 2:
			ATAPI_SET_SENSE_KEY(drv, ATAPI_SK_MEDIUM_ERROR);
			drv->sk = 0x03;
			drv->asc = 0x11;
			drv->status |= IDESTAT_ERR;
			drv->error |= IDEERR_UNC;
			sxsi->cdflag_ecc = (sxsi->cdflag_ecc & ~CD_ECC_BITMASK) | CD_ECC_NOERROR;
			senderror(drv);
			TRACEOUT(("atapicmd: EDC/ECC error detected at sector %d", drv->sector));
			break;
		case 0:
			drv->sector++;
			drv->nsectors--;

			drv->sc = IDEINTR_IO;
			drv->cy = 2048;
			drv->status &= ~(IDESTAT_DMRD|IDESTAT_SERV|IDESTAT_CHK);
			drv->status |= IDESTAT_DRQ;
			drv->error = 0;
			ATAPI_SET_SENSE_KEY(drv, ATAPI_SK_NO_SENSE);
			drv->asc = ATAPI_ASC_NO_ADDITIONAL_SENSE_INFORMATION;
			drv->bufdir = IDEDIR_IN;
			drv->buftc = (drv->nsectors)?IDETC_ATAPIREAD:IDETC_TRANSFEREND;
			drv->bufpos = 0;
			drv->bufsize = 2048;
	
			if(np2cfg.usecdecc && (sxsi->cdflag_ecc & CD_ECC_BITMASK)==CD_ECC_RECOVERED){
				drv->status |= IDESTAT_CORR;
				ATAPI_SET_SENSE_KEY(drv, ATAPI_SK_RECOVERED_ERROR);
				drv->asc = 0x18;
			}
			sxsi->cdflag_ecc = (sxsi->cdflag_ecc & ~CD_ECC_BITMASK) | CD_ECC_NOERROR;

			drv->status &= ~(IDESTAT_BSY); // 念のため直前で解除
			if (!(drv->ctrl & IDECTRL_NIEN)) {
				//TRACEOUT(("atapicmd: senddata()"));
				ideio.bank[0] = ideio.bank[1] | 0x80;			// ????
				pic_setirq(IDE_IRQ);
			}

			break;
		}
		atapi_dataread_error = -1;
	}
}
unsigned int __stdcall atapi_dataread_threadfunc(void* vdParam) {
	IDEDRV drv = NULL;
	
	SetEvent(atapi_thread_event_complete);
	while(WaitForSingleObject(atapi_thread_event_request, INFINITE) == WAIT_OBJECT_0){
		if(!atapi_thread_initialized) break;
		drv = atapi_thread_drv;
		atapi_dataread_threadfunc_part(drv);
		SetEvent(atapi_thread_event_complete);
	}
	SetEvent(atapi_thread_event_complete);
    _endthreadex(0);
	return 0;

}
void atapi_dataread(IDEDRV drv) {
	
	if(drv->status & IDESTAT_BSY) {
		return;
	}

	// エラー処理目茶苦茶〜
	if (drv->nsectors == 0) {
		sendabort(drv);
		return;
	}

	drv->status |= IDESTAT_BSY;
	
	if(np2cfg.useasynccd){
		if(atapi_thread){
			atapi_dataread_asyncwait(INFINITE);
			ResetEvent(atapi_thread_event_complete);
			atapi_dataread_error = -1;
			atapi_thread_drv = drv;
			SetEvent(atapi_thread_event_request);
			atapi_dataread_asyncwait(2);
		}else{
			atapi_dataread_error = -1;
			atapi_thread_drv = drv;
			atapi_dataread_threadfunc_part(drv);
			atapi_dataread_asyncwait(0);
		}
	}else{
		if(atapi_thread){
			atapi_dataread_asyncwait(INFINITE);
		}
		atapi_dataread_error = -1;
		atapi_thread_drv = drv;
		atapi_dataread_threadfunc_part(drv);
		atapi_dataread_asyncwait(0);
	}
}
#else
void atapi_dataread(IDEDRV drv) {

	SXSIDEV	sxsi;
	sxsi = sxsi_getptr(drv->sxsidrv);

	// エラー処理目茶苦茶〜
	if (drv->nsectors == 0) {
		sendabort(drv);
		return;
	}
	
	sxsi->cdflag_ecc = (sxsi->cdflag_ecc & ~CD_ECC_BITMASK) | CD_ECC_NOERROR;

	if (sxsi_read(drv->sxsidrv, drv->sector, drv->buf, 2048) != 0) {
		ATAPI_SET_SENSE_KEY(drv, ATAPI_SK_ILLEGAL_REQUEST);
		drv->asc = 0x21;
		senderror(drv);
		sxsi->cdflag_ecc = (sxsi->cdflag_ecc & ~CD_ECC_BITMASK) | CD_ECC_NOERROR;
		return;
	}

	// EDC/ECC check
	if(np2cfg.usecdecc && (sxsi->cdflag_ecc & CD_ECC_BITMASK)==CD_ECC_ERROR){
		ATAPI_SET_SENSE_KEY(drv, ATAPI_SK_MEDIUM_ERROR);
		drv->sk = 0x03;
		drv->asc = 0x11;
		drv->status |= IDESTAT_ERR;
		drv->error |= IDEERR_UNC;
		sxsi->cdflag_ecc = (sxsi->cdflag_ecc & ~CD_ECC_BITMASK) | CD_ECC_NOERROR;
		senderror(drv);
		return;
	}

	drv->sector++;
	drv->nsectors--;

	drv->sc = IDEINTR_IO;
	drv->cy = 2048;
	drv->status &= ~(IDESTAT_BSY|IDESTAT_DMRD|IDESTAT_SERV|IDESTAT_CHK);
	drv->status |= IDESTAT_DRQ;
	drv->error = 0;
	ATAPI_SET_SENSE_KEY(drv, ATAPI_SK_NO_SENSE);
	drv->asc = ATAPI_ASC_NO_ADDITIONAL_SENSE_INFORMATION;
	drv->bufdir = IDEDIR_IN;
	drv->buftc = (drv->nsectors)?IDETC_ATAPIREAD:IDETC_TRANSFEREND;
	drv->bufpos = 0;
	drv->bufsize = 2048;
	
	if(np2cfg.usecdecc && (sxsi->cdflag_ecc & CD_ECC_BITMASK)==CD_ECC_RECOVERED){
		drv->status |= IDESTAT_CORR;
		ATAPI_SET_SENSE_KEY(drv, ATAPI_SK_RECOVERED_ERROR);
		drv->asc = 0x18;
	}
	sxsi->cdflag_ecc = (sxsi->cdflag_ecc & ~CD_ECC_BITMASK) | CD_ECC_NOERROR;

	if (!(drv->ctrl & IDECTRL_NIEN)) {
		//TRACEOUT(("atapicmd: senddata()"));
		ideio.bank[0] = ideio.bank[1] | 0x80;			// ????
		pic_setirq(IDE_IRQ);
	}
}
#endif

void atapi_dataread_end(IDEDRV drv) {
	SXSIDEV	sxsi;
	sxsi = sxsi_getptr(drv->sxsidrv);

	drv->sector++;
	drv->nsectors--;

	drv->sc = IDEINTR_IO;
	drv->cy = 2048;
	drv->status &= ~(IDESTAT_DMRD|IDESTAT_SERV|IDESTAT_CHK);
	drv->status |= IDESTAT_DRQ;
	drv->error = 0;
	ATAPI_SET_SENSE_KEY(drv, ATAPI_SK_NO_SENSE);
	drv->asc = ATAPI_ASC_NO_ADDITIONAL_SENSE_INFORMATION;
	drv->bufdir = IDEDIR_IN;
	drv->buftc = (drv->nsectors)?IDETC_ATAPIREAD:IDETC_TRANSFEREND;
	drv->bufpos = 0;
	drv->bufsize = 2048;
	
	if(np2cfg.usecdecc && (sxsi->cdflag_ecc & CD_ECC_BITMASK)==CD_ECC_RECOVERED){
		drv->status |= IDESTAT_CORR;
		ATAPI_SET_SENSE_KEY(drv, ATAPI_SK_RECOVERED_ERROR);
		drv->asc = 0x18;
	}
	sxsi->cdflag_ecc = (sxsi->cdflag_ecc & ~CD_ECC_BITMASK) | CD_ECC_NOERROR;

	drv->status &= ~(IDESTAT_BSY); // 念のため直前で解除
	if (!(drv->ctrl & IDECTRL_NIEN)) {
		//TRACEOUT(("atapicmd: senddata()"));
		ideio.bank[0] = ideio.bank[1] | 0x80;			// ????
		pic_setirq(IDE_IRQ);
	}
#if defined(_WINDOWS)
	atapi_dataread_error = -1;
#endif
}
void atapi_dataread_errorend(IDEDRV drv) {
	SXSIDEV	sxsi;
	sxsi = sxsi_getptr(drv->sxsidrv);
	
	drv->status &= ~(IDESTAT_DRQ);

	ATAPI_SET_SENSE_KEY(drv, ATAPI_SK_ILLEGAL_REQUEST);
	drv->asc = 0x21;
	sxsi->cdflag_ecc = (sxsi->cdflag_ecc & ~CD_ECC_BITMASK) | CD_ECC_NOERROR;
	senderror(drv);
	TRACEOUT(("atapicmd: read error at sector %d", drv->sector));
#if defined(_WINDOWS)
	atapi_dataread_error = -1;
#endif
}

static void atapi_cmd_read(IDEDRV drv, UINT32 lba, UINT32 nsec) {

	drv->sector = lba;
	drv->nsectors = nsec;
	atapi_dataread(drv);
}
static void atapi_cmd_read_cd(IDEDRV drv, UINT32 lba, UINT32 nsec) {
	
	int i;
	SXSIDEV	sxsi;
	CDTRK	trk;
	UINT	tracks;
	UINT8 *bufptr;
	UINT bufsize;

	UINT8 rawdata[2352];

	UINT8 hassync;
	UINT8 hashead;
	UINT8 hassubhead;
	UINT8 hasdata;
	UINT8 hasedcecc;

	UINT16 isCDDA = 1;
	
#if defined(_WINDOWS)
	atapi_thread_drv = drv;
#endif
	sxsi = sxsi_getptr(drv->sxsidrv);

	hassync = (drv->buf[9] & 0x80) ? 1 : 0;
	hassubhead = (drv->buf[9] & 0x40) ? 1 : 0;
	hashead = (drv->buf[9] & 0x20) ? 1 : 0;
	hasdata = (drv->buf[9] & 0x10) ? 1 : 0;
	hasedcecc = (drv->buf[9] & 0x08) ? 1 : 0;

	drv->sector = lba;
	drv->nsectors = nsec;
	
	// エラー処理目茶苦茶〜
	if (drv->nsectors == 0) {
		cmddone(drv);
		return;
	}

	sxsi->cdflag_ecc = (sxsi->cdflag_ecc & ~CD_ECC_BITMASK) | CD_ECC_NOERROR;
	
	trk = sxsicd_gettrk(sxsi, &tracks);
	for (i = 0; i < tracks; i++) {
		if (trk[i].str_sec <= (UINT32)drv->sector && (UINT32)drv->sector <= trk[i].end_sec) {
			isCDDA = (trk[i].adr_ctl == TRACKTYPE_AUDIO);
			break;
		}
	}
	
	if(isCDDA){
		// Audio
		if (sxsicd_readraw(sxsi, drv->sector, drv->buf) != SUCCESS) {
			atapi_dataread_errorend(0);
			return;
		}
		bufsize = 2352;
	}else{
		// 条件がかなり複雑。
		// ATAPI CD-ROM Specificationの
		// Table 99 - Number of Bytes Returned Based on Data Selection Field
		// を参照

		// MODE1決め打ち
		if (sxsicd_readraw(sxsi, drv->sector, rawdata) != SUCCESS) {
			atapi_dataread_errorend(0);
			return;
		}

		bufsize = 0;
		bufptr = drv->buf;
		if (hassync){
			if(hashead){
				// Headerがいるときだけ有効
				memcpy(bufptr, rawdata, 12);
				bufptr += 12;
				bufsize += 12;
			}
		}
		if (hashead){
			memcpy(bufptr, rawdata + 12, 4);
			bufptr += 4;
			bufsize += 4;
		}
		if (hassubhead){
			// MODE1（本来ないが、User Dataが無いときだけ特例で書く）
			if(!hasdata){
				memset(bufptr, 0, 8);
				bufptr += 8;
				bufsize += 8;

			}

			//// XA
			//memcpy(bufptr, rawdata + 12 + 4, 8);

			//bufptr += 8;
			//bufsize += 8;
		}
		if (hasdata){
			memcpy(bufptr, rawdata + 12 + 4 + 8, 2048);
			bufptr += 2048;
			bufsize += 2048;
		}
		if (hasedcecc){
			//// MODE1
			//memcpy(bufptr, rawdata + 12 + 4 + 8 + 2048, 4);
			//memcpy(bufptr + 4, rawdata + 12 + 4 + 8 + 2048 + 12, 276);

			////// XA
			////memcpy(bufptr, rawdata + 12 + 4 + 8 + 2048, 280);
			
			//bufptr += 280;
			//bufsize += 280;

			memcpy(bufptr, rawdata + 12 + 4 + 8 + 2048, 288);
			bufptr += 288;
			bufsize += 288;
		}
	}
	
	atapi_dataread_end(drv);
	
	drv->bufsize = bufsize;
	drv->cy = bufsize;
}

static void atapi_cmd_read_cd_msf(IDEDRV drv) {

	UINT32	pos;
	UINT32	leng;

	int M, S, F;
	if(drv->damsfbcd){
		M = BCD2HEX(drv->buf[3]);
		S = BCD2HEX(drv->buf[4]);
		F = BCD2HEX(drv->buf[5]);
		pos = (((M * 60) + S) * 75) + F;
		M = BCD2HEX(drv->buf[6]);
		S = BCD2HEX(drv->buf[7]);
		F = BCD2HEX(drv->buf[8]);
		leng = (((M * 60) + S) * 75) + F;
	}else{
		M = drv->buf[3];
		S = drv->buf[4];
		F = drv->buf[5];
		pos = (((M * 60) + S) * 75) + F;
		M = drv->buf[6];
		S = drv->buf[7];
		F = drv->buf[8];
		leng = (((M * 60) + S) * 75) + F;
	}
	if (leng > pos) {
		leng -= pos;
	}
	else {
		leng = 0;
	}
	if (pos >= 150) {
		pos -= 150;
	}
	else {
		pos = 0;
	}
	atapi_cmd_read_cd(drv, pos, leng);
}

// -- MODE SELECT/SENSE
#define	PC_01_SIZE	8
#define	PC_0D_SIZE	8
#define	PC_0E_SIZE	16
#define	PC_2A_SIZE	20

// page code changeable value
static const UINT8 chgval_pagecode_01[PC_01_SIZE] = {
	0x00, 0x00, 0x37, 0xff, 0x00, 0x00, 0x00, 0x00,
};
static const UINT8 chgval_pagecode_0d[PC_0D_SIZE] = {
	0x00, 0x00, 0x00, 0x0f, 0xff, 0xff, 0xff, 0xff,
};
static const UINT8 chgval_pagecode_0e[PC_0E_SIZE] = {
	0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0xff, 0xff,
	0x0f, 0xff, 0x0f, 0xff, 0x00, 0x00, 0x00, 0x00,
};
static const UINT8 chgval_pagecode_2a[PC_2A_SIZE] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x02, 0xc2, 0x00, 0x02, 0x00, 0x00, 0x02, 0xc2,
	0x00, 0x00, 0x00, 0x00,
};

// page code default value
static const UINT8 defval_pagecode_01[PC_01_SIZE] = {
	0x01, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
static const UINT8 defval_pagecode_0d[PC_0D_SIZE] = {
	0x0d, 0x06, 0x00, 0x00, 0x00, 0x3c, 0x00, 0x4b,
};
static const UINT8 defval_pagecode_0e[PC_0E_SIZE] = {
	0x0e, 0x0e, 0x04, 0x00, 0x00, 0x00, 0x00, 0x4b,
	0x01, 0xff, 0x02, 0xff, 0x00, 0x00, 0x00, 0x00,
};

static const UINT8 defval_pagecode_2a[PC_2A_SIZE] = {
//#ifdef YUIDEBUG
//	0x2a, 0x12, 0x00, 0x00, 0x71, 0x65, 0x89, 0x07,
//	0x02, 0xc2, 0x00, 0xff, 0x00, 0x80, 0x02, 0xc2,
//	0x00, 0x00, 0x00, 0x00,
//#else
	0x2a, 0x12, 0x00, 0x00, 0x71, 0x65, 0x29, 0x07,
	0x02, 0xc2, 0x00, 0xff, 0x00, 0x80, 0x02, 0xc2,
	0x00, 0x00, 0x00, 0x00,
//#endif
}; 

#if defined(SUPPORT_NECCDD)
#define	PC_0F_SIZE	16

// "NEC CD-ROM" unique? (for neccdd.sys)
// It's just a stub, all values are unknown...
static const UINT8 chgval_pagecode_0f[PC_0F_SIZE] = {
	0x0f, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
static const UINT8 defval_pagecode_0f[PC_0F_SIZE] = {
	0x0f, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
#endif


// 0x55: MODE SELECT
static void atapi_cmd_mode_select(IDEDRV drv) {
	
	UINT leng;

	leng = (drv->buf[7] << 8) + drv->buf[8];
	TRACEOUT(("atapi_cmd_mode_select: leng=%u SP=%u", leng, drv->buf[1] & 1));

	if (drv->buf[1] & 1) {
		/* Saved Page is not supported */
		ATAPI_SET_SENSE_KEY(drv, ATAPI_SK_ILLEGAL_REQUEST);
		drv->asc = ATAPI_ASC_INVALID_FIELD_IN_CDB;
		senderror(drv);
		return;
	}

#if 1
	cmddone(drv);	/* workaround */
#else
	sendabort(drv);	/* XXX */
#endif

}

// 0x5a: MODE SENSE
static void atapi_cmd_mode_sense(IDEDRV drv) {

	const UINT8	*ptr;
	UINT		leng;
	UINT		cnt;
	UINT8		pctrl, pcode;

	leng = (drv->buf[7] << 8) + drv->buf[8];
	pctrl = ((drv->buf[2] >> 6) & 3) & ~0x2;	// 0: current, 1: changeable, 2: default
	pcode = drv->buf[2] & 0x3f;

	if (pctrl == 3) {
		/* Saved Page is not supported */
		//TRACEOUT(("Saved Page is not supported"));
		ATAPI_SET_SENSE_KEY(drv, ATAPI_SK_ILLEGAL_REQUEST);
		drv->asc = ATAPI_ASC_SAVING_PARAMETERS_NOT_SUPPORTED;
		senderror(drv);
		return;
	}

	/* Mode Parameter Header */
	ZeroMemory(drv->buf, 8);
	if (!(drv->media & IDEIO_MEDIA_LOADED)) {
		drv->buf[2] = 0x70;	// Door closed, no disc present
	}
	else if ((drv->media & (IDEIO_MEDIA_COMBINE)) == IDEIO_MEDIA_AUDIO) {
		drv->buf[2] = 0x02;	// 120mm CD-ROM audio only
	}
	else if ((drv->media & (IDEIO_MEDIA_COMBINE)) == IDEIO_MEDIA_COMBINE) {
		drv->buf[2] = 0x03;	// 120mm CD-ROM data & audio combined
	}
	else {
		drv->buf[2] = 0x01;	// 120mm CD-ROM data only
	}
	cnt = 8;
	if (cnt > leng) {
		goto length_exceeded;
	}

	/* Mode Page */
	//TRACEOUT(("pcode = %.2x", pcode));
	switch (pcode) {
#if defined(SUPPORT_NECCDD)
	case 0x0f:
	{
		// some NEC CD-ROM specific support (required for neccdd.sys)
		const UINT8 *p2a;
		UINT8 *p = drv->buf + cnt;
		if (pctrl == 1) {
			p2a = chgval_pagecode_2a;
			ptr = chgval_pagecode_0f;
		}
		else {
			p2a = defval_pagecode_2a;
			ptr = defval_pagecode_0f;
		}
		CopyMemory(p, ptr, MIN((leng - cnt), PC_0F_SIZE));
		p[4] = (p2a[4] & 1);				// byte04 bit0 = Audioplay supported?
		p[4] |= ((p2a[6] & 2) << 6);		// byte04 bit7 = lock state?
		p[4] |= ((p2a[5] & 4) << 2);		// byte04 bit4 = R-W supported?
		p[5] = ((p2a[7] & 3) << 3);		// byte05 bit4,3 = audio manipulation?
		// note: When byte04 bit1 is set, neccdd.sys will set bit10 in device status call.
		// (bit10 is "reserved" in the Microsoft's reference)
		// for drive specific applications? or simply a bug?)
	}
		cnt += PC_0F_SIZE;
		if (cnt > leng) {
			goto length_exceeded;
		}
		drv->damsfbcd = 1; // XXX: NEC CD-ROMコマンドを飛ばしてきたらBCDモードにする･･･（暫定neccdd.sys判定）
		break;
#endif
	case 0x3f:
		/*FALLTHROUGH*/

	case 0x01:	/* Read Error Recovery Parameters Page */
		if (pctrl == 1) {
			ptr = chgval_pagecode_01;
		}
		else {
			ptr = defval_pagecode_01;
		}
		CopyMemory(drv->buf + cnt, ptr, MIN((leng - cnt), PC_01_SIZE));
		cnt += PC_01_SIZE;
		if (cnt > leng) {
			goto length_exceeded;
		}
		if (pcode == 0x01) {
			break;
		}
		/*FALLTHROUGH*/

	case 0x0d:	/* CD-ROM Device Parameters Page */
		if (pctrl == 1) {
			ptr = chgval_pagecode_0d;
		}
		else {
			ptr = defval_pagecode_0d;
		}
		CopyMemory(drv->buf + cnt, ptr, MIN((leng - cnt), PC_0D_SIZE));
		cnt += PC_0D_SIZE;
		if (cnt > leng) {
			goto length_exceeded;
		}
		if (pcode == 0x0d) {
			break;
		}
		/*FALLTHROUGH*/

	case 0x0e:	/* CD-ROM Audio Control Paramater Page */
		if (pctrl == 1) {
			ptr = chgval_pagecode_0e;
		}
		else {
			ptr = defval_pagecode_0e;
		}
		CopyMemory(drv->buf + cnt, ptr, MIN((leng - cnt), PC_0E_SIZE));
		cnt += PC_0E_SIZE;
		if (cnt > leng) {
			goto length_exceeded;
		}
		if (pcode == 0x0e) {
			break;
		}
		//np2cfg.davolume = drv->buf[11];
		/*FALLTHROUGH*/

	case 0x2a:	/* CD-ROM Capabilities & Mechanical Status Page */
		if (pctrl == 1) {
			ptr = chgval_pagecode_2a;
		}
		else {
			ptr = defval_pagecode_2a;
		}
		CopyMemory(drv->buf + cnt, ptr, MIN((leng - cnt), PC_2A_SIZE));
		cnt += PC_2A_SIZE;
		if (cnt > leng) {
			goto length_exceeded;
		}
#if 0
		/*FALLTHROUGH*/

	case 0x00:
#endif
		break;

	default:
		ATAPI_SET_SENSE_KEY(drv, ATAPI_SK_ILLEGAL_REQUEST);
		drv->asc = ATAPI_ASC_INVALID_FIELD_IN_CDB;
		senderror(drv);
		return;
	}

	drv->buf[0] = (UINT8)((cnt - 2) >> 8);
	drv->buf[1] = (UINT8)(cnt - 2);
	senddata(drv, cnt, leng);
	return;

length_exceeded:
	if (cnt >= 65536) {
		ATAPI_SET_SENSE_KEY(drv, ATAPI_SK_ILLEGAL_REQUEST);
		drv->asc = ATAPI_ASC_INVALID_FIELD_IN_CDB;
		senderror(drv);
		return;
	}

	drv->buf[0] = (UINT8)((leng - 2) >> 8);
	drv->buf[1] = (UINT8)(leng - 2);
	senddata(drv, cnt, leng);
}


// ---- Audio

static void storemsf(UINT8 *ptr, UINT32 pos, int isBCD) {

	UINT	f;
	UINT	m;

	f = pos % 75;
	pos = pos / 75;
	m = pos % 60;
	pos = pos / 60;
	ptr[0] = 0;
	if(isBCD){
		if (pos > 99) {
			ptr[1] = 0xff;
			ptr[2] = 0x59;
			ptr[3] = 0x74;
			return;
		}
		ptr[1] = (UINT8)HEX2BCD(pos);
		ptr[2] = (UINT8)HEX2BCD(m);
		ptr[3] = (UINT8)HEX2BCD(f);
	}else{
		if (pos > 0xff) {
			pos = 0xff;
			m = 59;
			f = 74;
		}
		ptr[1] = (UINT8)pos;
		ptr[2] = (UINT8)m;
		ptr[3] = (UINT8)f;
	}
}

static void storelba(UINT8 *ptr, UINT32 pos) {

	ptr[0] = (UINT8)(pos >> 24);
	ptr[1] = (UINT8)(pos >> 16);
	ptr[2] = (UINT8)(pos >> 8);
	ptr[3] = (UINT8)(pos);
}

// 0x42: READ SUB CHANNEL
static void atapi_cmd_readsubch(IDEDRV drv) {

	SXSIDEV	sxsi;
	UINT	subq;
	UINT	cmd;
	UINT	leng;
	CDTRK	trk;
	UINT	tracks;
	UINT	r;
	UINT32	pos;

	sxsi = sxsi_getptr(drv->sxsidrv);
	if ((sxsi == NULL) || (sxsi->devtype != SXSIDEV_CDROM) ||
		(!(sxsi->flag & SXSIFLAG_READY))) {
		senderror(drv);
		return;
	}
	trk = sxsicd_gettrk(sxsi, &tracks);
	leng = (drv->buf[7] << 8) + drv->buf[8];
	subq = drv->buf[2] & 0x40;
	cmd = drv->buf[3];

	drv->buf[0] = 0;
	drv->buf[1] = (UINT8)drv->daflag;
	drv->buf[2] = 0;
	drv->buf[3] = 0;
	if (!subq) {
		senddata(drv, 4, leng);
		return;
	}
	switch(cmd) {
		case 0x01:			// CD-ROM current pos
			ZeroMemory(drv->buf + 4, 12);
			drv->buf[3] = 0x12;
			drv->buf[4] = 0x01;
			if (drv->daflag != 0x15) {
				pos = drv->dacurpos;
				if ((drv->daflag == 0x12) || (drv->daflag == 0x13)) {
					pos += (rand() & 7);
				}
				r = tracks;
				while(r) {
					r--;
					if (trk[r].pos <= pos) {
						break;
					}
				}
#ifdef SUPPORT_KAI_IMAGES
				drv->buf[5] = trk[r].adr_ctl;
				drv->buf[6] = trk[r].point;
#else
				drv->buf[5] = trk[r].type;
				drv->buf[6] = trk[r].track;
#endif
				drv->buf[7] = 1;

				storemsf(drv->buf + 8, pos + 150, drv->damsfbcd);
				storemsf(drv->buf + 12, (UINT32)(pos - trk[r].pos), drv->damsfbcd);
			}
			senddata(drv, 16, leng);
			break;

		default:
			senderror(drv);
			break;
	}
}

// 0x43: READ TOC
static void atapi_cmd_readtoc(IDEDRV drv) {

	SXSIDEV	sxsi;
	UINT	leng;
	UINT	format;
	CDTRK	trk;
	UINT	tracks;
	UINT	datasize;
	UINT8	*ptr;
	UINT	i;
#ifdef SUPPORT_KAI_IMAGES
	UINT8	time;	//	追加(kaiD)
#endif
	UINT8	strack;

	sxsi = sxsi_getptr(drv->sxsidrv);
	if ((sxsi == NULL) || (sxsi->devtype != SXSIDEV_CDROM) ||
		(!(sxsi->flag & SXSIFLAG_READY))) {
		senderror(drv);
		return;
	}
	trk = sxsicd_gettrk(sxsi, &tracks);
	
#ifdef SUPPORT_KAI_IMAGES

#if 0	//	修正(kaiD)
	leng = (drv->buf[7] << 8) + drv->buf[8];
	format = (drv->buf[9] >> 6);
	TRACEOUT(("atapi_cmd_readtoc fmt=%d leng=%d", format, leng));
#else
#if 0
	//	こっちが正しいと思うんだけど…ドライバの違い？
	time = (drv->buf[1] & 0x02) >> 0x01;
	format = (drv->buf[2] & 0x0f);
	leng = (drv->buf[6] << 8) + drv->buf[7];
#else
	time = (drv->buf[1] & 0x02) >> 0x01;
	// format = (drv->buf[2] & 0x0f);
	// if (format == 0)		// "When Format in Byte 2 is zero, then Byte 9 is used" (SFF8020)
	format = (drv->buf[9] >> 6);
	leng = (drv->buf[7] << 8) + drv->buf[8];
#endif
	TRACEOUT(("ATAPI CMD: read TOC : time=%d fmt=%d leng=%d", time, format, leng));
	TRACEOUT(("\t[%02x %02x %02x %02x %02x %02x %02x %02x]",
			drv->buf[0x00], drv->buf[0x01], drv->buf[0x02], drv->buf[0x03], drv->buf[0x04], drv->buf[0x05], drv->buf[0x06], drv->buf[0x07]));
	TRACEOUT(("\t[%02x %02x %02x %02x %02x %02x %02x %02x]",
			drv->buf[0x08], drv->buf[0x09], drv->buf[0x0a], drv->buf[0x0b], drv->buf[0x0c], drv->buf[0x0d], drv->buf[0x0e], drv->buf[0x0f]));
#endif

#else /* SUPPORT_KAI_IMAGES */
	leng = (drv->buf[7] << 8) + drv->buf[8];
	format = (drv->buf[9] >> 6);
	//TRACEOUT(("atapi_cmd_readtoc fmt=%d leng=%d", format, leng));
#endif /* SUPPORT_KAI_IMAGES */
	strack = drv->buf[6];

	switch (format) {
	case 0: // track info
		//datasize = (tracks * 8) + 10;
		strack = MIN(MAX(1U, strack), (tracks+1));		// special case: 0 = 1sttrack, 0xaa = leadout
		datasize = ((tracks - strack + 1U) * 8U) + 10;
		drv->buf[0] = (UINT8)(datasize >> 8);
		drv->buf[1] = (UINT8)(datasize >> 0);
		drv->buf[2] = 1;
		drv->buf[3] = (UINT8)tracks;
		ptr = drv->buf + 4;
		////for (i=0; i<=tracks; i++) {
		//i = drv->buf[6];
		//if (i > 0) --i;
		//for (/* i=0 */; i<=tracks; i++) {
		for (i=strack-1; i<=tracks; i++) {
			ptr[0] = 0;
#ifdef SUPPORT_KAI_IMAGES
			ptr[1] = trk[i].adr_ctl;
			ptr[2] = trk[i].point;
#else
			ptr[1] = trk[i].type;
			ptr[2] = trk[i].track;
#endif
			ptr[3] = 0;
			//storemsf(ptr + 4, (UINT32)(trk[i].pos + 150), drv->damsfbcd);
			if (time)
				storemsf(ptr + 4, (UINT32)(trk[i].pos + 150), drv->damsfbcd);
			else
				storelba(ptr + 4, (UINT32)(trk[i].pos));
			ptr += 8;
		}
		//senddata(drv, (tracks * 8) + 12, leng);
		senddata(drv, 2 + datasize, leng);
		drv->media &= ~IDEIO_MEDIA_CHANGED;
		break;

	case 1:	// multi session
		ZeroMemory(drv->buf, 12);
		drv->buf[1] = 0x0a;
		drv->buf[2] = 0x01;
		drv->buf[3] = 0x01;
		drv->buf[5] = 0x14;
		drv->buf[6] = 0x01;
		//drv->buf[10] = 0x02;
		if (time)
			storemsf(drv->buf + 8, (UINT32)(150), drv->damsfbcd);
		else
			storelba(drv->buf + 8, (UINT32)(0));
		senddata(drv, 12, leng);
		drv->media &= ~IDEIO_MEDIA_CHANGED;
		break;

	default:
		// time = 1;		// 0010b~0101b: MSF(TIME) Field "Ignored by Drive" (SCSI MMC)
		senderror(drv);
		break;
	}
}
 
static void atapi_cmd_playaudio_sub(IDEDRV drv, UINT32 pos, UINT32 leng) {

	ideio.daplaying |= 1 << (drv->sxsidrv & 3);
	drv->daflag = 0x11;
	drv->dacurpos = pos;
	drv->dalength = leng;
	drv->dabufrem = 0;
	cmddone(drv);
}
// 0x45: Play Audio
static void atapi_cmd_playaudio(IDEDRV drv) {
	UINT32	pos;
	UINT32	leng;
	
	pos = (drv->buf[2] << 24) | (drv->buf[3] << 16) | (drv->buf[4] << 8) | drv->buf[5];
	leng = (drv->buf[7] << 16) | drv->buf[8];
	atapi_cmd_playaudio_sub(drv, pos, leng);
}

// 0x47: Play Audio MSF
static void atapi_cmd_playaudiomsf(IDEDRV drv) {

	UINT32	pos;
	UINT32	leng;

	int M, S, F;
	if(drv->damsfbcd){
		M = BCD2HEX(drv->buf[3]);
		S = BCD2HEX(drv->buf[4]);
		F = BCD2HEX(drv->buf[5]);
		pos = (((M * 60) + S) * 75) + F;
		M = BCD2HEX(drv->buf[6]);
		S = BCD2HEX(drv->buf[7]);
		F = BCD2HEX(drv->buf[8]);
		leng = (((M * 60) + S) * 75) + F;
	}else{
		M = drv->buf[3];
		S = drv->buf[4];
		F = drv->buf[5];
		pos = (((M * 60) + S) * 75) + F;
		M = drv->buf[6];
		S = drv->buf[7];
		F = drv->buf[8];
		leng = (((M * 60) + S) * 75) + F;
	}
	if (leng > pos) {
		leng -= pos;
	}
	else {
		leng = 0;
	}
	if (pos >= 150) {
		pos -= 150;
	}
	else {
		pos = 0;
	}
	atapi_cmd_playaudio_sub(drv, pos, leng);
}

// 0x4B: PAUSE RESUME
static void atapi_cmd_pauseresume(IDEDRV drv) {

	if (drv->buf[8] & 1) {
		// resume
		if (drv->daflag == 0x12) {
			ideio.daplaying |= 1 << (drv->sxsidrv & 3);
			drv->daflag = 0x11;
		}
	}
	else {
		// pause
		if (drv->daflag == 0x11) {
			ideio.daplaying &= ~(1 << (drv->sxsidrv & 3));
			drv->daflag = 0x12;
		}
	}
	cmddone(drv);
}

// 0x2B: SEEK
static void atapi_cmd_seek(IDEDRV drv, UINT32 lba)
{
	CDTRK	trk;
	UINT	tracks;
	SXSIDEV sxsi;

	stop_daplay(drv);

	sxsi = sxsi_getptr(drv->sxsidrv);
	trk = sxsicd_gettrk(sxsi, &tracks);
	TRACEOUT(("atapicmd: seek LBA=%d NSEC=%d", lba, trk[tracks-1].pos + trk[tracks-1].sectors));
	if (lba < trk[tracks-1].pos + trk[tracks-1].sectors) {
		drv->dacurpos = lba;
	}
	cmddone(drv);
}

// 0xBD: MECHANISM STATUS
static void atapi_cmd_mechanismstatus(IDEDRV drv) {

	//SXSIDEV	sxsi;

	//sxsi = sxsi_getptr(drv->sxsidrv);
	//ZeroMemory(drv->buf, 12);
	//drv->buf[0] = 0x00;
	//drv->buf[1] = 0x00;
	//drv->buf[2] = drv->cy & 0xff; // LBA(MSB)
	//drv->buf[3] = (drv->cy >> 8) & 0xff; // LBA
	//drv->buf[4] = drv->sn; // LBA(LSB)
	//drv->buf[5] = 0x01;
	//drv->buf[6] = 0x00;
	//drv->buf[7] = 0x04;
	//drv->buf[8] = (sxsi->flag & SXSIFLAG_READY) ? 0x80 : 0x00; // XXX: CD挿入状態とか入れてやるべき？
	//drv->buf[9] = 0x00;
	//drv->buf[10] = 0x00;
	//drv->buf[11] = 0x00;
	//senddata(drv, 12, 12);
	sendabort(drv);
}

void atapi_initialize(void) {
#if defined(_WINDOWS) && !defined(__LIBRETRO__)
	UINT32 dwID = 0;
	//if(!pic_cs_initialized){
	//	memset(&pic_cs, 0, sizeof(pic_cs));
	//	InitializeCriticalSection(&pic_cs);
	//	pic_cs_initialized = 1;
	//}
	if(!atapi_thread_initialized){
		atapi_thread_initialized = 1;
		atapi_thread_event_complete = CreateEvent(NULL, TRUE, TRUE, NULL);
		atapi_thread_event_request = CreateEvent(NULL, FALSE, FALSE, NULL);
		atapi_thread = (HANDLE)_beginthreadex(NULL, 0, atapi_dataread_threadfunc, NULL, 0, &dwID);
	}
#else
	// TODO: 非Windows用コードを書く
#endif
}

void atapi_deinitialize(void) {
#if defined(_WINDOWS) && !defined(__LIBRETRO__)
	if(atapi_thread_initialized){
		atapi_thread_initialized = 0;
		SetEvent(atapi_thread_event_request);
		if(WaitForSingleObject(atapi_thread, 5000) == WAIT_TIMEOUT){
			TerminateThread(atapi_thread, 0);
		}
		CloseHandle(atapi_thread);
		CloseHandle(atapi_thread_event_complete);
		CloseHandle(atapi_thread_event_request);
		atapi_thread = NULL;
	}
#else
	// TODO: 非Windows用コードを書く
#endif
}
#endif	/* SUPPORT_IDEIO */

