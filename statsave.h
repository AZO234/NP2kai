/**
 * @file	statsave.h
 * @brief	Interface of state save
 */

#pragma once

/**
 * Result code
 */
enum
{
	STATFLAG_SUCCESS	= 0,
	STATFLAG_DISKCHG	= 0x0001,
	STATFLAG_VERCHG		= 0x0002,
	STATFLAG_WARNING	= 0x0080,
	STATFLAG_VERSION	= 0x0100,
	STATFLAG_FAILURE	= -1
};

struct TagStatFlagHandle;
typedef struct TagStatFlagHandle *STFLAGH;

/**
 * @brief The entry of state flag
 */
struct TagStatFlagEntry
{
	char	index[12];
	UINT16	ver;
	UINT16	type;
	void	*arg1;
	UINT	arg2;
};
typedef struct TagStatFlagEntry SFENTRY;

#ifdef __cplusplus
extern "C"
{
#endif

extern uint8_t g_u8ControlState;

int statflag_read(STFLAGH sfh, void *ptr, UINT size);
int statflag_write(STFLAGH sfh, const void *ptr, UINT size);
void statflag_seterr(STFLAGH sfh, const OEMCHAR *str);

int statsave_save(const OEMCHAR *filename);
int statsave_check(const OEMCHAR *filename, OEMCHAR *buf, int size);
int statsave_load(const OEMCHAR *filename);
int statsave_save_hdd(const OEMCHAR *ext);
int statsave_load_hdd(const OEMCHAR *ext);

#if defined(__LIBRETRO__)
int statsave_save_d(const OEMCHAR *filename);
int statsave_load_d(const OEMCHAR *filename);
#else
int statsave_save_d(void);
int statsave_load_d(void);
#endif

#ifdef __cplusplus
}
#endif
