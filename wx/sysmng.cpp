/* === system management for wx port ===
 * workclock functions copied from x/sysmng.c.
 */

#include <compiler.h>
#include <sysmng.h>
#include <timemng.h>
#include <ini.h>
#include "np2.h"
#include "dosio.h"
#include "np2frame.h"
#include <pccore.h>
#include <cpucore.h>
#include <io/iocore.h>
#include <diskimage/fddfile.h>
#include <cbus/ideio.h>
#include <fdd/sxsi.h>
#include <fdd/diskdrv.h>
#include <common/milstr.h>
#include <time.h>
#include <stdio.h>

#if defined(SUPPORT_IDEIO)
extern REG8 cdchange_drv;
#endif

UINT sys_updates;
SYSMNGMISCINFO sys_miscinfo = {0};

static char titlestr[512];
static char clockstr[64];

static struct {
	UINT32	tick;
	UINT32	clock;
	UINT32	draws;
	SINT32	fps;
	SINT32	khz;
} workclock;

void sysmng_initialize(void)  { sys_updates = 0; }
void sysmng_deinitialize(void) {}

void sysmng_update(UINT update)
{
	sys_updates |= update;
	if (update & (SYS_UPDATECFG | SYS_UPDATEOSCFG)) {
		initsave();
	}
	if (update & SYS_UPDATEHDD) {
		diskdrv_hddbind();
	}
	if (update & (SYS_UPDATEFDD | SYS_UPDATEHDD | SYS_UPDATECLOCK)) {
		sysmng_updatecaption(SYS_UPDATECAPTION_ALL);
		/* Also refresh menu labels (FDD/HDD filenames) on the UI thread */
		if (update & (SYS_UPDATEFDD | SYS_UPDATEHDD)) {
			np2frame_updateCaption(SYS_UPDATECAPTION_ALL);
		}
	}
}

void sysmng_cpureset(void)
{
	sys_updates &= (SYS_UPDATECFG | SYS_UPDATEOSCFG);
	sysmng_workclockreset();
}

void sysmng_workclockreset(void)
{
	workclock.tick  = GETTICK();
	workclock.clock = CPU_CLOCK;
	workclock.draws = drawcount;
	workclock.fps   = 0;
	workclock.khz   = 0;
}

BOOL sysmng_workclockrenewal(void)
{
	SINT32 tick;
	tick = GETTICK() - workclock.tick;
	if (tick >= 2000) {
		workclock.tick  += tick;
		workclock.fps    = ((drawcount - workclock.draws) * 10000) / tick;
		workclock.draws  = drawcount;
		workclock.khz    = (CPU_CLOCK - workclock.clock) / tick;
		workclock.clock  = CPU_CLOCK;
		return TRUE;
	}
	return FALSE;
}

BRESULT timemng_gettime(_SYSTIME *systime)
{
	time_t t = time(NULL);
	struct tm *tm = localtime(&t);
	if (!tm) return FAILURE;
	systime->year   = (UINT16)(tm->tm_year + 1900);
	systime->month  = (UINT16)(tm->tm_mon + 1);
	systime->week   = (UINT16)tm->tm_wday;
	systime->day    = (UINT16)tm->tm_mday;
	systime->hour   = (UINT16)tm->tm_hour;
	systime->minute = (UINT16)tm->tm_min;
	systime->second = (UINT16)tm->tm_sec;
	systime->milli  = 0;
	return SUCCESS;
}

void sysmng_updatecaption(UINT8 flag)
{
	char work[2048];
	int i;
	OEMCHAR fddtext[16] = {0};
#if defined(SUPPORT_IDEIO)
	int cddrvnum = 1;
#endif
	extern int np2_stateslotnow;

	if (flag & SYS_UPDATECAPTION_FDD) {
		titlestr[0] = '\0';
		for (i = 0; i < 4; i++) {
			if (fdd_diskready(i)) {
				OEMSPRINTF(fddtext, OEMTEXT("  FDD%d:"), i + 1);
				milstr_ncat(titlestr, fddtext, sizeof(titlestr));
				milstr_ncat(titlestr, file_getname((char *)fdd_diskname(i)), sizeof(titlestr));
			}
		}
#if defined(SUPPORT_IDEIO)
		for (i = 0; i < 4; i++) {
			if (sxsi_getdevtype(i) == SXSIDEV_CDROM) {
				snprintf(work, sizeof(work), "  CDD%d:", cddrvnum++);
				const OEMCHAR *fname = diskdrv_getsxsi((REG8)i);
				if (fname && fname[0]) {
					milstr_ncat(titlestr, work, NELEMENTS(titlestr));
					milstr_ncat(titlestr, file_getname((char *)fname), NELEMENTS(titlestr));
				} else if (i == cdchange_drv && g_nevent.item[NEVENT_CDWAIT].clock > 0) {
					milstr_ncat(titlestr, work, NELEMENTS(titlestr));
					milstr_ncat(titlestr, OEMTEXT("Now Loading..."), NELEMENTS(titlestr));
				}
			}
		}
#endif
	}
	if (flag & SYS_UPDATECAPTION_CLK) {
		clockstr[0] = '\0';
		if (np2oscfg.DISPCLK & 2) {
			if (workclock.fps) {
				snprintf(clockstr, sizeof(clockstr), " - %u.%1uFPS",
					workclock.fps / 10, workclock.fps % 10);
			} else {
				milstr_ncpy(clockstr, " - 0FPS", sizeof(clockstr));
			}
		}
		if (np2oscfg.DISPCLK & 1) {
			snprintf(work, sizeof(work), " %2u.%03uMHz",
				workclock.khz / 1000, workclock.khz % 1000);
			if (clockstr[0] == '\0') {
				milstr_ncpy(clockstr, " -", sizeof(clockstr));
			}
			milstr_ncat(clockstr, work, sizeof(clockstr));
		}
	}

	milstr_ncpy(work, NP2_WX_APPNAME, sizeof(work));
	milstr_ncat(work, " wx", sizeof(work));
#if defined(SUPPORT_STATSAVE)
	if (np2_stateslotnow > 0) {
		char slot[16];
		snprintf(slot, sizeof(slot), " [Slot %d]", np2_stateslotnow);
		milstr_ncat(work, slot, sizeof(work));
	}
#endif
	milstr_ncat(work, titlestr, sizeof(work));
	milstr_ncat(work, clockstr, sizeof(work));

	np2frame_setTitle(work);
}
