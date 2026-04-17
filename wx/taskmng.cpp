/* === task management for wx port ===
 * Follows the x port pattern: uses task_avail (analogous to np2running).
 * taskmng_sleep() uses SDL_Delay while checking task_avail, matching
 * the x port's wait-then-check loop.
 */

#include <compiler.h>
#include <taskmng.h>

BOOL task_avail = FALSE;

void taskmng_initialize(void) { task_avail = TRUE; }
void taskmng_exit(void)       { task_avail = FALSE; }
void taskmng_rol(void)        { /* nothing */ }

BOOL taskmng_sleep(UINT32 tick)
{
	UINT32 base;
	UINT32 now;

	base = GETTICK();
	while (task_avail && (((now = GETTICK()) - base) < tick)) {
		/* Allow the OS scheduler to run; sleep half the remaining time */
		UINT32 elapsed = now - base;
		UINT32 remain  = tick - elapsed;
		SDL_Delay(remain / 2 > 1 ? remain / 2 : 1);
	}
	return task_avail;
}
