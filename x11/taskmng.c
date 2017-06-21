#include "compiler.h"

#include "np2.h"
#include "toolkit.h"

#include "taskmng.h"


void
taskmng_initialize(void)
{

	np2running = 1;
}

BOOL
taskmng_sleep(UINT32 tick)
{
	UINT32 base;
	UINT32 now;

	base = GETTICK();
	while (taskmng_isavail() && (((now = GETTICK()) - base) < tick)) {
		toolkit_event_process();
		usleep((tick - (now - base) / 2) * 1000);
	}
	return taskmng_isavail();
}

void
taskmng_exit(void)
{

	np2running = 0;
}
