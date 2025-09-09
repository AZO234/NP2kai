/**
 *	@file	sysmng.c
 *	@brief	Implementation of the system
 */

#include <compiler.h>
#include <sysmng.h>
#include <ini.h>

extern REG8 cdchange_drv;

UINT	sys_updates;

SYSMNGMISCINFO	sys_miscinfo = {0};

/**
 * Initialize
 */
void sysmng_initialize(void)
{
}

/**
 * Deinitialize
 */
void sysmng_deinitialize(void)
{
}

/**
 * Notifies flags
 * @param[in] update update flags
 */
#if defined(__LIBRETRO__)
extern int lr_init;
#endif

void sysmng_update(UINT update)
{
	if (update & (SYS_UPDATECFG | SYS_UPDATEOSCFG))
	{
#if defined(__LIBRETRO__)
		if(lr_init != 0)
			initsave();
#else
		initsave();
#endif
	}
}

/**
 * Notifies CPU Reset
 */
void sysmng_cpureset(void)
{
}

void sysmng_updatecaption(UINT8 flag)
{
}
