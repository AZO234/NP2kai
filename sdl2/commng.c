#include "compiler.h"

#include "np2.h"
#include "commng.h"
#include "cmver.h"
#include "cmjasts.h"


// ---- non connect

static UINT
ncread(COMMNG self, UINT8 *data)
{

	return 0;
}

static UINT
ncwrite(COMMNG self, UINT8 data)
{

	return 0;
}

static UINT8
ncgetstat(COMMNG self)
{

	return 0xf0;
}

static INTPTR
ncmsg(COMMNG self, UINT msg, INTPTR param)
{

	return 0;
}

static void
ncrelease(COMMNG self)
{

	/* Nothing to do */
}

static _COMMNG com_nc = {
	COMCONNECT_OFF, ncread, ncwrite, ncgetstat, ncmsg, ncrelease
};


// ----

void
commng_initialize(void)
{

	cmvermouth_initialize();
#if !defined(__LIBRETRO__)
#if !defined(_WIN32)
//	cmmidi_initailize();
#endif	/* _WIN32 */
#endif	/* __LIBRETRO__ */
}

COMMNG
commng_create(UINT device)
{
	COMMNG ret;

#if !defined(__LIBRETRO__)
	COMCFG *cfg;

	ret = NULL;

	switch (device) {
	case COMCREATE_SERIAL:
		cfg = &np2oscfg.com[0];
		break;

	case COMCREATE_PC9861K1:
		cfg = &np2oscfg.com[1];
		break;

	case COMCREATE_PC9861K2:
		cfg = &np2oscfg.com[2];
		break;

	case COMCREATE_MPU98II:
		cfg = &np2oscfg.mpu;
		break;

	case COMCREATE_PRINTER:
		cfg = NULL;
		if (np2oscfg.jastsnd) {
			ret = cmjasts_create();
		}
		break;

	default:
		cfg = NULL;
		break;
	}
	if (cfg) {
		if ((cfg->port >= COMPORT_COM1)
		 && (cfg->port <= COMPORT_COM4)) {
#if !defined(_WIN32)
//		 	ret = cmserial_create(cfg->port - COMPORT_COM1 + 1, cfg->param, cfg->speed);
#endif	/* _WIN32 */
		} else if (cfg->port == COMPORT_MIDI) {
#if !defined(_WIN32)
//			ret = cmmidi_create(cfg->mout, cfg->min, cfg->mdl);
//			if (ret) {
//				(*ret->msg)(ret, COMMSG_MIMPIDEFFILE, (INTPTR)cfg->def);
//				(*ret->msg)(ret, COMMSG_MIMPIDEFEN, (INTPTR)cfg->def_en);
//			}
#endif	/* _WIN32 */
		}
	}
	if (ret)
		return ret;
	return (COMMNG)&com_nc;
#else	/* __LIBRETRO__ */
	ret = NULL;
	if (device == COMCREATE_MPU98II) {
		ret = cmvermouth_create();
	}
	else if (device == COMCREATE_PRINTER) {
		if (np2oscfg.jastsnd) {
			ret = cmjasts_create();
		}
	}
	if (ret == NULL) {
		ret = (COMMNG)&com_nc;
	}
	return(ret);
#endif	/* __LIBRETRO__ */
}

void
commng_destroy(COMMNG hdl)
{

	if (hdl) {
		hdl->release(hdl);
	}
}
