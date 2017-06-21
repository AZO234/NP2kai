/**
 *	@file	sysmng.c
 *	@brief	Implementation of the system
 */

#include "compiler.h"
#include "sysmng.h"
#include "ini.h"

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
void sysmng_update(UINT update)
{
	if (update & (SYS_UPDATECFG | SYS_UPDATEOSCFG))
	{
		initsave();
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
