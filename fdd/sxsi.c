#include	"compiler.h"
#include	"strres.h"
#include	"dosio.h"
#include	"sysmng.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"sxsi.h"
#if defined(SUPPORT_IDEIO)
#include	"ideio.h"
#endif

	_SXSIDEV	sxsi_dev[SASIHDD_MAX + SCSIHDD_MAX];

#if !defined(__WIN32__)
unsigned GetTickCount()
{
        struct timeval tv;
        if(gettimeofday(&tv, NULL) != 0)
                return 0;

        return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}
#endif	/* __WIN32__ */

// ----

static BRESULT nc_reopen(SXSIDEV sxsi) {

	(void)sxsi;
	return(FAILURE);
}

static REG8	nc_read(SXSIDEV sxsi, FILEPOS pos, UINT8 *buf, UINT size) {

	(void)sxsi;
	(void)pos;
	(void)buf;
	(void)size;
	return(0x60);
}

static REG8 nc_write(SXSIDEV sxsi, FILEPOS pos, const UINT8 *buf, UINT size) {

	(void)sxsi;
	(void)pos;
	(void)buf;
	(void)size;
	return(0x60);
}

static REG8 nc_format(SXSIDEV sxsi, FILEPOS pos) {

	(void)sxsi;
	(void)pos;
	return(0x60);
}

static void nc_close(SXSIDEV sxsi) {

	(void)sxsi;
}

static void nc_destroy(SXSIDEV sxsi) {

	(void)sxsi;
}


static void sxsi_disconnect(SXSIDEV sxsi) {

	if (sxsi) {
		if (sxsi->flag & SXSIFLAG_FILEOPENED) {
#if defined(SUPPORT_IDEIO)
			ideio_notify(sxsi->drv, 0);
#endif
			(*sxsi->close)(sxsi);
		}
		if (sxsi->flag & SXSIFLAG_READY) {
			(*sxsi->destroy)(sxsi);
		}
		sxsi->flag = 0;
		sxsi->reopen = nc_reopen;
		sxsi->read = nc_read;
		sxsi->write = nc_write;
		sxsi->format = nc_format;
		sxsi->close = nc_close;
		sxsi->destroy = nc_destroy;
	}
}


// ----

void sxsi_initialize(void) {

	UINT	i;

	ZeroMemory(sxsi_dev, sizeof(sxsi_dev));
	for (i=0; i<SASIHDD_MAX; i++) {
		sxsi_dev[i].drv = (UINT8)(SXSIDRV_SASI + i);
	}
#if defined(SUPPORT_SCSI)
	for (i=0; i<SCSIHDD_MAX; i++) {
		sxsi_dev[SASIHDD_MAX + i].drv = (UINT8)(SXSIDRV_SCSI + i);
	}
#endif
	for (i=0; i<NELEMENTS(sxsi_dev); i++) {
		sxsi_disconnect(sxsi_dev + i);
	}
}

void sxsi_allflash(void) {

	SXSIDEV	sxsi;
	SXSIDEV	sxsiterm;

	sxsi = sxsi_dev;
	sxsiterm = sxsi + NELEMENTS(sxsi_dev);
	while(sxsi < sxsiterm) {
		if (sxsi->flag & SXSIFLAG_FILEOPENED) {
			sxsi->flag &= ~SXSIFLAG_FILEOPENED;
			(*sxsi->close)(sxsi);
		}
		sxsi++;
	}
}

void sxsi_alltrash(void) {

	SXSIDEV	sxsi;
	SXSIDEV	sxsiterm;

	sxsi = sxsi_dev;
	sxsiterm = sxsi + NELEMENTS(sxsi_dev);
	while(sxsi < sxsiterm) {
		sxsi_disconnect(sxsi);
		sxsi++;
	}
}

BOOL sxsi_isconnect(SXSIDEV sxsi) {

	if (sxsi) {
		switch(sxsi->devtype) {
			case SXSIDEV_HDD:
				if (sxsi->flag & SXSIFLAG_READY) {
					return(TRUE);
				}
				break;

			case SXSIDEV_CDROM:
				return(TRUE);
		}
	}
	return(FALSE);
}

BRESULT sxsi_prepare(SXSIDEV sxsi) {

	if ((sxsi == NULL) || (!(sxsi->flag & SXSIFLAG_READY))) {
		return(FAILURE);
	}
	if (!(sxsi->flag & SXSIFLAG_FILEOPENED)) {
		if ((*sxsi->reopen)(sxsi) == SUCCESS) {
			sxsi->flag |= SXSIFLAG_FILEOPENED;
		}
		else {
			return(FAILURE);
		}
	}
	sysmng_hddaccess(sxsi->drv);
	return(SUCCESS);
}


// ----

SXSIDEV sxsi_getptr(REG8 drv) {

	UINT	num;

	num = drv & 0x0f;
	if (!(drv & 0x20)) {					// SASI or IDE
		if (num < SASIHDD_MAX) {
			return(sxsi_dev + num);
		}
	}
#if defined(SUPPORT_SCSI)
	else {
		if (num < SCSIHDD_MAX) {			// SCSI
			return(sxsi_dev + SASIHDD_MAX + num);
		}
	}
#endif
	return(NULL);
}

OEMCHAR *sxsi_getfilename(REG8 drv) {

	SXSIDEV	sxsi;

	sxsi = sxsi_getptr(drv);
	if ((sxsi) && (sxsi->flag & SXSIFLAG_READY)) {
		return(sxsi->fname);
	}
	return(NULL);
}

BRESULT sxsi_setdevtype(REG8 drv, UINT8 dev) {

	SXSIDEV	sxsi;

	sxsi = sxsi_getptr(drv);
	if (sxsi) {
		if (sxsi->devtype != dev) {
			sxsi_disconnect(sxsi);
			sxsi->devtype = dev;
		}
		return(SUCCESS);
	}
	else {
		return(FAILURE);
	}
}

UINT8 sxsi_getdevtype(REG8 drv) {

	SXSIDEV	sxsi;

	sxsi = sxsi_getptr(drv);
	if (sxsi) {
		return(sxsi->devtype);
	}
	else {
		return(SXSIDEV_NC);
	}
}

// CD入れ替えのタイムアウト（投げやり）
char cdchange_flag = 0;
UINT32 cdchange_reqtime = 0;
REG8 cdchange_drv;
OEMCHAR cdchange_fname[MAX_PATH];
void cdchange_timeoutproc(NEVENTITEM item) {

	if(!cdchange_flag) return;
	cdchange_flag = 0;
	sxsi_devopen(cdchange_drv, cdchange_fname);
#if defined(SUPPORT_IDEIO)
	ideio_mediachange(cdchange_drv);
#endif
	sysmng_updatecaption(1);
}
static void cdchange_timeoutset(void) {

	nevent_setbyms(NEVENT_CDWAIT, 5000, cdchange_timeoutproc, NEVENT_ABSOLUTE);
}

#ifdef SUPPORT_NVL_IMAGES
BRESULT sxsihdd_nvl_open(SXSIDEV sxsi, const OEMCHAR *fname);
#endif

BRESULT sxsi_devopen(REG8 drv, const OEMCHAR *fname) {

	SXSIDEV		sxsi;
	BRESULT		r;

	sxsi = sxsi_getptr(drv);
	if (sxsi == NULL) {
		goto sxsiope_err;
	}
	switch(sxsi->devtype) {
		case SXSIDEV_HDD:
			if ((fname == NULL) || (fname[0] == '\0')) {
				goto sxsiope_err;
			}
			r = sxsihdd_open(sxsi, fname);
#ifdef SUPPORT_NVL_IMAGES
			if (r == FAILURE)
			{
				r = sxsihdd_nvl_open(sxsi, fname);
			}
#endif
			break;

		case SXSIDEV_CDROM:
#if defined(SUPPORT_IDEIO)
			if (cdchange_flag) {
				// CD交換中
				if(GetTickCount()-cdchange_reqtime>5000){
					// 強制交換
					cdchange_timeoutproc(NULL);
				}
				return(FAILURE);
			}
			if ((fname == NULL) || (fname[0] == '\0')) {
				int num = drv & 0x0f;
				ideio_notify(sxsi->drv, 0);
				file_cpyname(sxsi->fname, _T("\0\0\0\0"), 1);
				sxsi->flag = 0;
				file_cpyname(np2cfg.idecd[num], _T("\0\0\0\0"), 1);
				sysmng_updatecaption(1);
				return(SUCCESS);
			}
			else {
				if(sxsi->flag & SXSIFLAG_READY){
					// いったん取り出す
					ideio_notify(sxsi->drv, 0);
					sxsi->flag = 0;
					cdchange_drv = drv;
					file_cpyname(sxsi->fname, _T("\0\0\0\0"), 1);
					file_cpyname(np2cfg.idecd[drv & 0x0f], _T("\0\0\0\0"), NELEMENTS(cdchange_fname));
					file_cpyname(cdchange_fname, fname, NELEMENTS(cdchange_fname));
					cdchange_flag = 1;
					cdchange_timeoutset();
					cdchange_reqtime = GetTickCount();
					return(FAILURE); // XXX: ここで失敗返してええの？
				}
				r = sxsicd_open(sxsi, fname);
				if (r == SUCCESS) {
					int num = drv & 0x0f;
					file_cpyname(np2cfg.idecd[num], fname, NELEMENTS(cdchange_fname));
				}else{
					int num = drv & 0x0f;
					file_cpyname(np2cfg.idecd[num], _T("\0\0\0\0"), 1);
				}
				sysmng_updatecaption(1);
				ideio_mediachange(cdchange_drv);
			}
#endif
			break;

		default:
			r = FAILURE;
			break;
	}
	if (r != SUCCESS) {
		goto sxsiope_err;
	}
	file_cpyname(sxsi->fname, fname, NELEMENTS(sxsi->fname));
	sxsi->flag = SXSIFLAG_READY | SXSIFLAG_FILEOPENED;
#if defined(SUPPORT_IDEIO)
	ideio_notify(sxsi->drv, 1);
#endif
	return(SUCCESS);

sxsiope_err:
	return(FAILURE);
}

void sxsi_devclose(REG8 drv) {

	SXSIDEV		sxsi;

	sxsi = sxsi_getptr(drv);
	sxsi_disconnect(sxsi);
}

BOOL sxsi_issasi(void) {

	REG8	drv;
	SXSIDEV	sxsi;
	BOOL	ret;

	ret = FALSE;
	for (drv=0x00; drv<0x04; drv++) {
		sxsi = sxsi_getptr(drv);
		if (sxsi) {
			if ((drv < 0x02) && (sxsi->devtype == SXSIDEV_HDD)) {
				if (sxsi->flag & SXSIFLAG_READY) {
					if (sxsi->mediatype & SXSIMEDIA_INVSASI) {
						return(FALSE);
					}
					ret = TRUE;
				}
			}
			else {
				return(FALSE);
			}
		}
	}
	return(ret);
}

BOOL sxsi_isscsi(void) {

	REG8	drv;
	SXSIDEV	sxsi;

	for (drv=0x20; drv<0x28; drv++) {
		sxsi = sxsi_getptr(drv);
		if (sxsi_isconnect(sxsi)) {
			return(TRUE);
		}
	}
	return(FALSE);
}

BOOL sxsi_iside(void) {

	REG8	drv;
	SXSIDEV	sxsi;

	for (drv=0x00; drv<0x04; drv++) {
		sxsi = sxsi_getptr(drv);
		if (sxsi_isconnect(sxsi)) {
			return(TRUE);
		}
	}
	return(FALSE);
}



REG8 sxsi_read(REG8 drv, FILEPOS pos, UINT8 *buf, UINT size) {

	SXSIDEV	sxsi;

	sxsi = sxsi_getptr(drv);
	if (sxsi != NULL) {
		return(sxsi->read(sxsi, pos, buf, size));
	}
	else {
		return(0x60);
	}
}

REG8 sxsi_write(REG8 drv, FILEPOS pos, const UINT8 *buf, UINT size) {

	SXSIDEV	sxsi;

	sxsi = sxsi_getptr(drv);
	if (sxsi != NULL) {
		return(sxsi->write(sxsi, pos, buf, size));
	}
	else {
		return(0x60);
	}
}

REG8 sxsi_format(REG8 drv, FILEPOS pos) {

	SXSIDEV	sxsi;

	sxsi = sxsi_getptr(drv);
	if (sxsi != NULL) {
		return(sxsi->format(sxsi, pos));
	}
	else {
		return(0x60);
	}
}

