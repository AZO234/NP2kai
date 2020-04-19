/**
 * @file	diskdrv.c
 * @brief	Implementation of the disk-drive
 */

#include "compiler.h"
#include "diskdrv.h"
#include "dosio.h"
#include "sysmng.h"
#include "pccore.h"
#include "iocore.h"
#include "diskimage/fddfile.h"
#include "sxsi.h"

#define DISK_DELAY	20			/*!< Delay 0.4sec */

static int diskdrv_delay[4];				/*!< delay */
static OEMCHAR diskdrv_fname[4][MAX_PATH];	/*!< path */
static UINT diskdrv_ftype[4];				/*!< type */
static int diskdrv_ro[4];					/*!< readonly */

/**
 * Set SxSI image
 * @param[in] drv The ID of the drive
 * @param[in] fname The filename
 */
void diskdrv_setsxsi(REG8 drv, const OEMCHAR *fname)
{
	UINT	num;
	OEMCHAR	*p;
	int		leng;

	num = drv & 0x0f;
	p = NULL;
	leng = 0;
	if (!(drv & 0x20))
	{
		/* SASI or IDE */
#if defined(SUPPORT_IDEIO)
		if (num < 4)
		{
			if(sxsi_getdevtype(num)!=SXSIDEV_CDROM){
				p = np2cfg.sasihdd[num];
				leng = NELEMENTS(np2cfg.sasihdd[0]);
			}
		}
#else
		if (num < 2)
		{
			p = np2cfg.sasihdd[num];
			leng = NELEMENTS(np2cfg.sasihdd[0]);
		}
#endif
	}
#if defined(SUPPORT_SCSI)
	else
	{
		/* SCSI */
		if (num < 4)
		{
			p = np2cfg.scsihdd[num];
			leng = NELEMENTS(np2cfg.scsihdd[0]);
		}
	}
#endif
	if (p)
	{
		if (fname)
		{
			file_cpyname(p, fname, leng);
		}
		else
		{
			p[0] = '\0';
		}
		sysmng_update(SYS_UPDATEHDD | SYS_UPDATECFG);
	}
	else
	{
		sxsi_devopen(drv, fname);
	}
}

/**
 * Get current SxSI files
 * @param[in] drv The ID of the drive
 * @return The filename
 */
const OEMCHAR *diskdrv_getsxsi(REG8 drv)
{
	UINT num;

	num = drv & 0x0f;
	if (!(drv & 0x20))
	{
		/* SASI or IDE */
#if defined(SUPPORT_IDEIO)
		if (num < 4)
#else
		if (num < 2)
#endif
		{
			return np2cfg.sasihdd[num];
		}
	}
#if defined(SUPPORT_SCSI)
	else
	{
		/* SCSI */
		if (num < 4)
		{
			return np2cfg.scsihdd[num];
		}
	}
#endif
	return sxsi_getfilename(drv);
}

/**
 * Bind all fixed drives
 */
void diskdrv_hddbind(void)
{
	REG8 drv;

#if defined(SUPPORT_IDEIO)
	for (drv = 0x00; drv < 0x04; drv++)
	{
		sxsi_devclose(drv);
	}
#else
	for (drv = 0x00; drv < 0x02; drv++)
	{
		sxsi_devclose(drv);
	}
#endif
#if defined(SUPPORT_SCSI)
	for (drv = 0x20; drv < 0x24; drv++)
	{
		sxsi_devclose(drv);
	}
#endif /* defined(SUPPORT_SCSI) */

#if defined(SUPPORT_IDEIO)
	for (drv = 0x00; drv < 0x04; drv++)
	{
		sxsi_setdevtype(drv, np2cfg.idetype[drv]);
		if(np2cfg.idetype[drv]==SXSIDEV_HDD)
		{
			if (sxsi_devopen(drv, np2cfg.sasihdd[drv & 0x0f]) != SUCCESS)
			{
				sxsi_setdevtype(drv, SXSIDEV_NC);
				if(np2cfg.sasihdd[drv & 0x0f]!=NULL && np2cfg.sasihdd[drv & 0x0f][0]!='\0')
				{
					msgbox("HD image file open error" ,"file open error"); 
				}
			}
		}
		else if(np2cfg.idetype[drv]==SXSIDEV_CDROM)
		{
			SXSIDEV sxsi = sxsi_getptr(drv);
			if (sxsicd_open(sxsi, np2cfg.idecd[drv & 0x0f]) == SUCCESS) {
				file_cpyname(sxsi->fname, np2cfg.idecd[drv & 0x0f], NELEMENTS(sxsi->fname));
				sxsi->flag = SXSIFLAG_READY | SXSIFLAG_FILEOPENED;
			}
		}
	}
#else
	for (drv = 0x00; drv < 0x02; drv++)
	{
		sxsi_setdevtype(drv, SXSIDEV_HDD);
		if (sxsi_devopen(drv, np2cfg.sasihdd[drv & 0x0f]) != SUCCESS)
		{
			sxsi_setdevtype(drv, SXSIDEV_NC);
			if(np2cfg.sasihdd[drv & 0x0f]!=NULL && np2cfg.sasihdd[drv & 0x0f][0]!='\0')
			{
				msgbox("HD image file open error" ,"file open error"); 
			}
		}
	}
#endif
#if defined(SUPPORT_SCSI)
	for (drv = 0x20; drv < 0x24; drv++)
	{
		sxsi_setdevtype(drv, SXSIDEV_HDD);
		if (sxsi_devopen(drv, np2cfg.scsihdd[drv & 0x0f]) != SUCCESS)
		{
			sxsi_setdevtype(drv, SXSIDEV_NC);
		}
	}
#endif /* defined(SUPPORT_SCSI) */
}

/**
 * Set FDD image (force)
 * @param[in] drv The number of the drive
 * @param[in] fname The filename of the disk
 * @param[in] ftype the type of the disk
 * @param[in] readonly The flags of file
 */
void diskdrv_readyfddex(REG8 drv, const OEMCHAR *fname, UINT ftype, int readonly)
{
	if ((drv < 4) && (fdc.equip & (1 << drv)))
	{
		if ((fname != NULL) && (fname[0] != '\0'))
		{
			short attr;

			attr = file_attr(fname);
			if(attr & FILEATTR_READONLY) {
				readonly = 1;
			}

			fdd_set(drv, fname, ftype, readonly);
			if ((!(fdc.chgreg & 4)) || (fdc.ctrlreg & 0x08)){
				fdc.stat[drv] = FDCRLT_AI | drv;
				fdc.us = drv;
				fdc_interrupt();
			}
			sysmng_update(SYS_UPDATEFDD);
		}
	}
}

/**
 * Set FDD image
 * @param[in] drv The number of the drive
 * @param[in] fname The filename of the disk
 * @param[in] ftype the type of the disk
 * @param[in] readonly The flags of file
 */
void diskdrv_setfddex(REG8 drv, const OEMCHAR *fname, UINT ftype, int readonly)
{
	if ((drv < 4) && (fdc.equip & (1 << drv)))
	{
		fdd_eject(drv);
		diskdrv_delay[drv] = 0;
		diskdrv_fname[drv][0] = '\0';
		np2cfg.fddfile[drv][0] = '\0';
		fdc.stat[drv] = FDCRLT_AI | FDCRLT_NR | drv;
		fdc.us = drv;
		fdc_interrupt();

		if (fname)
		{
			short attr;

			attr = file_attr(fname);
			if(attr & FILEATTR_READONLY) {
				readonly = 1;
			}

			diskdrv_delay[drv] = DISK_DELAY;
			diskdrv_ftype[drv] = ftype;
			diskdrv_ro[drv] = readonly;
			file_cpyname(diskdrv_fname[drv], fname, NELEMENTS(diskdrv_fname[drv]));
			file_cpyname(np2cfg.fddfile[drv], fname, NELEMENTS(np2cfg.fddfile[drv]));
		}
		sysmng_update(SYS_UPDATEFDD);
	}
}

/**
 * Callback
 */
void diskdrv_callback(void)
{
	REG8 drv;

	for (drv = 0; drv < 4; drv++)
	{
		if (diskdrv_delay[drv])
		{
			diskdrv_delay[drv]--;
			if (!diskdrv_delay[drv])
			{
				diskdrv_readyfddex(drv, diskdrv_fname[drv], diskdrv_ftype[drv], diskdrv_ro[drv]);
				diskdrv_fname[drv][0] = '\0';
			}
		}
	}
}
