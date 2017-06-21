#include "compiler.h"

#include "np2.h"
#include "dosio.h"
#include "cpucore.h"
#include "pccore.h"
#include "iocore.h"
#include "fdd/fddfile.h"
#include "ideio.h"
#include "fdd/sxsi.h"
#include "sysmng.h"
#include "toolkit.h"

UINT	sys_updates;

static char titlestr[512];
static char clockstr[64];

static struct {
	UINT32	tick;
	UINT32	clock;
	UINT32	draws;
	SINT32	fps;
	SINT32	khz;
} workclock;

void
sysmng_workclockreset(void)
{

	workclock.tick = GETTICK();
	workclock.clock = CPU_CLOCK;
	workclock.draws = drawcount;
	workclock.fps = 0;
	workclock.khz = 0;
}

BOOL
sysmng_workclockrenewal(void)
{
	SINT32	tick;
	tick = GETTICK() - workclock.tick;
	if (tick >= 2000) {
		workclock.tick += tick;
		workclock.fps = ((drawcount - workclock.draws) * 10000) / tick;
		workclock.draws = drawcount;
		workclock.khz = (CPU_CLOCK - workclock.clock) / tick;
		workclock.clock = CPU_CLOCK;
		return TRUE;
	}
	return FALSE;
}

void
sysmng_updatecaption(UINT8 flag)
{
	char work[512];

	if (flag & 1) {
		titlestr[0] = '\0';
		if (fdd_diskready(0)) {
			milstr_ncat(titlestr, "  FDD1:", sizeof(titlestr));
			milstr_ncat(titlestr, file_getname((char *)fdd_diskname(0)), sizeof(titlestr));
		}
		if (fdd_diskready(1)) {
			milstr_ncat(titlestr, "  FDD2:", sizeof(titlestr));
			milstr_ncat(titlestr, file_getname((char *)fdd_diskname(1)), sizeof(titlestr));
		}
#if defined(SUPPORT_IDEIO)
		if (sxsi_getfilename(0x02)) {
			milstr_ncat(titlestr, "  CD-ROM:", sizeof(titlestr));
			milstr_ncat(titlestr, file_getname((OEMCHAR *)sxsi_getfilename(0x02)), sizeof(titlestr));
		}
#endif
	}
	if (flag & 2) {
		clockstr[0] = '\0';
		if (np2oscfg.DISPCLK & 2) {
			if (workclock.fps) {
				g_snprintf(clockstr, sizeof(clockstr), " - %u.%1uFPS", workclock.fps / 10, workclock.fps % 10);
			}
			else {
				milstr_ncpy(clockstr, " - 0FPS", sizeof(clockstr));
			}
		}
		if (np2oscfg.DISPCLK & 1) {
			g_snprintf(work, sizeof(work), " %2u.%03uMHz", workclock.khz / 1000, workclock.khz % 1000);
			if (clockstr[0] == '\0') {
				milstr_ncpy(clockstr, " -", sizeof(clockstr));
			}
			milstr_ncat(clockstr, work, sizeof(clockstr));
		}
	}
	milstr_ncpy(work, np2oscfg.titles, sizeof(work));
	milstr_ncat(work, titlestr, sizeof(work));
	milstr_ncat(work, clockstr, sizeof(work));
	toolkit_set_window_title(work);
}
