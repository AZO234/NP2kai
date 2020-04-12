#include	<string.h>
#include	"compiler.h"
#include	"strres.h"
#include	"dosio.h"
#include	"sysmng.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"sxsi.h"
#include	"timemng.h"
#if defined(SUPPORT_IDEIO)
#include	"ideio.h"
#endif
#if !defined(_MSC_VER)
#include <sys/time.h>
#endif

#if defined(_MSC_VER)
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#endif

	_SXSIDEV	sxsi_dev[SASIHDD_MAX + SCSIHDD_MAX];

#if !defined(_WIN32)
unsigned GetTickCount()
{
        struct timeval tv;
        if(gettimeofday(&tv, NULL) != 0)
                return 0;

        return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}
#endif	/* _WIN32 */

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
DWORD cdchange_reqtime = 0;
REG8 cdchange_drv;
OEMCHAR cdchange_fname[MAX_PATH];
void cdchange_timeoutproc(NEVENTITEM item) {

	if(!cdchange_flag) return;
	cdchange_flag = 0;
	sxsi_devopen(cdchange_drv, cdchange_fname);
#if defined(SUPPORT_IDEIO)
	ideio_mediachange(cdchange_drv);
#endif
	sysmng_updatecaption(1); // SYS_UPDATECAPTION_FDD
}
static void cdchange_timeoutset(void) {

	nevent_setbyms(NEVENT_CDWAIT, 6000, cdchange_timeoutproc, NEVENT_ABSOLUTE);
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
				if (sxsi->flag & SXSIFLAG_FILEOPENED) {
					ideio_notify(sxsi->drv, 0);
					(*sxsi->close)(sxsi);
				}
				if (sxsi->flag & SXSIFLAG_READY) {
					(*sxsi->destroy)(sxsi);
				}
				file_cpyname(sxsi->fname, _T("\0\0\0\0"), 1);
				sxsi->flag = 0;
				file_cpyname(np2cfg.idecd[num], _T("\0\0\0\0"), 1);
				sysmng_updatecaption(1); // SYS_UPDATECAPTION_FDD
				return(SUCCESS);
			}
			else {
				if((sxsi->flag & SXSIFLAG_READY) && (_tcsnicmp(sxsi->fname, OEMTEXT("\\\\.\\"), 4)!=0 || _tcsicmp(sxsi->fname, np2cfg.idecd[drv & 0x0f])==0) ){
					// いったん取り出す
					if (sxsi->flag & SXSIFLAG_FILEOPENED) {
						ideio_notify(sxsi->drv, 0);
						(*sxsi->close)(sxsi);
					}
					if (sxsi->flag & SXSIFLAG_READY) {
						(*sxsi->destroy)(sxsi);
					}
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
				if (r == SUCCESS || _tcsnicmp(fname, OEMTEXT("\\\\.\\"), 4)==0) {
					int num = drv & 0x0f;
					file_cpyname(np2cfg.idecd[num], fname, NELEMENTS(cdchange_fname));
					if(r != SUCCESS && _tcsnicmp(fname, OEMTEXT("\\\\.\\"), 4)==0){
						ideio_notify(sxsi->drv, 0);
					}
				}else{
					int num = drv & 0x0f;
					file_cpyname(np2cfg.idecd[num], _T("\0\0\0\0"), 1);
				}
				sysmng_updatecaption(1); // SYS_UPDATECAPTION_FDD
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

BRESULT sxsi_state_save(const OEMCHAR *ext) {
	SXSIDEV	sxsi;
	SXSIDEV	sxsiterm;

	sxsi = sxsi_dev;
	sxsiterm = sxsi + NELEMENTS(sxsi_dev);
	while(sxsi < sxsiterm) {
		if (sxsi->state_save != NULL) {
			_SYSTIME st;
			OEMCHAR dt[64];
			OEMCHAR	sfname[MAX_PATH];
			BRESULT r;

			timemng_gettime(&st);
			OEMSNPRINTF(dt, sizeof(dt), 
				OEMTEXT("%04d%02d%02d%02d%02d%02d%03d"),
				st.year, st.month, st.day,
				st.hour, st.minute, st.second,
				st.milli);

			file_cpyname(sfname, sxsi->fname, NELEMENTS(sfname));
			file_catname(sfname, OEMTEXT("_"), NELEMENTS(sfname));
			file_catname(sfname, ext, NELEMENTS(sfname));
			file_catname(sfname, OEMTEXT("_"), NELEMENTS(sfname));
			file_catname(sfname, dt, NELEMENTS(sfname));

			r = (*sxsi->state_save)(sxsi, sfname);
			if (r != SUCCESS) {
				return(r);
			}
		}

		sxsi++;
	}
}

static int str_get_mem_size(const OEMCHAR *str)
{
	return ((int)((OEMCHAR *)milstr_chr(str, 0) - str));
}

static BRESULT state_load(SXSIDEV sxsi, const OEMCHAR *ext)
{
	OEMCHAR	dir[MAX_PATH];
	FLINFO fli;
	FLISTH flh;
	OEMCHAR rname[MAX_PATH];
	int rnamesize;
	OEMCHAR	tname[MAX_PATH];

	file_cpyname(dir, sxsi->fname, NELEMENTS(dir));
	file_cutname(dir);
	file_cutseparator(dir);

	flh = file_list1st(dir, &fli);
	if (flh == FLISTH_INVALID)
	{
		return (SUCCESS);
	}

	file_cpyname(rname, file_getname(sxsi->fname), NELEMENTS(rname));
	file_catname(rname, OEMTEXT("_"), NELEMENTS(rname));
	file_catname(rname, ext, NELEMENTS(rname));
	file_catname(rname, OEMTEXT("_"), NELEMENTS(rname));
	rnamesize = str_get_mem_size(rname);

	ZeroMemory(tname, sizeof(OEMCHAR) * MAX_PATH);

	do
	{
		int namesize;

		if ((fli.attr & FILEATTR_VOLUME) ||
			(fli.attr & FILEATTR_DIRECTORY))
		{
			continue;
		}

		namesize = str_get_mem_size(fli.path);

		if (namesize <= rnamesize)
		{
			continue;
		}

		if (milstr_memcmp(fli.path, rname) != 0)
		{
			continue;
		}

		if (file_cmpname(fli.path, tname) > 0)
		{
			file_cpyname(tname, fli.path, NELEMENTS(tname));
		}
	} while (file_listnext(flh, &fli) == SUCCESS);

	if (OEMSTRLEN(tname) == 0)
	{
		return (SUCCESS);
	}

	file_setseparator(dir, NELEMENTS(dir));
	file_catname(dir, tname, NELEMENTS(dir));

	return ((*sxsi->state_load)(sxsi, dir));
}

BRESULT sxsi_state_load(const OEMCHAR *ext)
{
	SXSIDEV	sxsi;
	SXSIDEV	sxsiterm;

	sxsi = sxsi_dev;
	sxsiterm = sxsi + NELEMENTS(sxsi_dev);
	while (sxsi < sxsiterm) {
		if (sxsi->state_load != NULL) {
			BRESULT r;

			r = state_load(sxsi, ext);
			if (r != SUCCESS)
			{
				return(r);
			}
		}

		sxsi++;
	}
}

