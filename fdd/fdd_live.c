/**
 * @file	fdd_live.c
 * @brief	Floppy images served from a real drive
 *
 * Turned on with -l, for D88 images. A BIOS read has a worker thread fetch the track
 * first, so the emulation keeps running while the drive reads; a changed track is written
 * back to its file soon after, not only when the head moves on; and a file that goes away
 * and comes back is passed to the guest as a disk change.
 */

#include <compiler.h>
#include <dosio.h>
#include <cpucore.h>
#include <pccore.h>
#include <io/iocore.h>
#include <fdd/diskdrv.h>
#include <fdd/fdd_live.h>
#include <diskimage/fddfile.h>
#include <diskimage/fd/fdd_d88.h>
#if defined(SUPPORT_FDD_LIVE)
#include <np2_thread.h>
#include <file/file_path.h>
#endif

#define LOOK_TICKS	25			/*!< Look at the files every 0.5sec */
#define D88_TRACKREAD	0x6000		/*!< What fdd_d88.c reads of one track at most */

BOOL fddlive_enable = FALSE;

#if defined(SUPPORT_FDD_LIVE)

enum {
	READ_IDLE = 0,
	READ_RUNNING,
	READ_DONE
};

static NP2_Thread_t s_thread;
static NP2_Semaphore_t s_wake;
static NP2_Semaphore_t s_lock;				/*!< guards what the two threads share, below */
static BOOL s_started = FALSE;
static BOOL s_quit = FALSE;

static int s_read = READ_IDLE;
static OEMCHAR s_readname[MAX_PATH];
static long s_readpos;
static UINT s_readsize;

static BOOL s_looking = FALSE;
static OEMCHAR s_name[MAX_FDDFILE][MAX_PATH];
static BOOL s_gone[MAX_FDDFILE];

static BOOL s_out[MAX_FDDFILE];
static int s_ticks = 0;

static void *worker(void *param)
{
	static UINT8 buffer[0x10000];
	static OEMCHAR readname[MAX_PATH];
	static OEMCHAR name[MAX_FDDFILE][MAX_PATH];
	BOOL quit, fetch, look, gone[MAX_FDDFILE];
	long readpos = 0;
	UINT readsize = 0;
	FILEH hdl;
	UINT drv;

	(void)param;
	while (1)
	{
		NP2_Semaphore_Wait(&s_wake);
		NP2_Semaphore_Wait(&s_lock);
		quit = s_quit;
		fetch = (s_read == READ_RUNNING);
		if (fetch)
		{
			file_cpyname(readname, s_readname, NELEMENTS(readname));
			readpos = s_readpos;
			readsize = s_readsize;
		}
		look = s_looking;
		if (look)
		{
			CopyMemory(name, s_name, sizeof(name));
		}
		NP2_Semaphore_Release(&s_lock);
		if (quit)
		{
			break;
		}
		if (fetch)
		{
			hdl = file_open_rb(readname);
			if (hdl != FILEH_INVALID)
			{
				if (file_seek(hdl, readpos, FSEEK_SET) == readpos)
				{
					file_read(hdl, buffer, readsize);
				}
				file_close(hdl);
			}
			NP2_Semaphore_Wait(&s_lock);
			s_read = READ_DONE;
			NP2_Semaphore_Release(&s_lock);
		}
		if (look)
		{
			for (drv = 0; drv < MAX_FDDFILE; drv++)
			{
				gone[drv] = (name[drv][0]) && (!path_is_valid(name[drv]));
			}
			NP2_Semaphore_Wait(&s_lock);
			CopyMemory(s_gone, gone, sizeof(gone));
			s_looking = FALSE;
			NP2_Semaphore_Release(&s_lock);
		}
	}
	return NULL;
}

static BOOL start(void)
{
	if (!s_started)
	{
		s_started = TRUE;
		s_quit = FALSE;
		NP2_Semaphore_Create(&s_wake, 0);
		NP2_Semaphore_Create(&s_lock, 1);
		NP2_Thread_Create(&s_thread, worker, NULL);
		if (!s_thread)
		{
			fddlive_enable = FALSE;		/* no worker: read the slow way */
		}
	}
	return fddlive_enable;
}

/**
 * Called on entry to INT 1Bh, before the disk is touched
 * @return TRUE while the guest has to wait for the bytes it asked for
 */
BOOL fddlive_wait(void)
{
	FDDFILE fdd;
	UINT32 pos;
	UINT32 next;
	UINT size;
	UINT track;
	UINT i;
	int state;

	if ((!fddlive_enable) || (!start()))
	{
		return FALSE;
	}
	NP2_Semaphore_Wait(&s_lock);
	state = s_read;
	if (state == READ_DONE)
	{
		s_read = READ_IDLE;
	}
	NP2_Semaphore_Release(&s_lock);
	if (state != READ_IDLE)
	{
		return (state == READ_RUNNING);
	}

	fdd = fddfile + (CPU_AL & 0x03);
	track = (CPU_AH & 0x10) ? CPU_CL : fdc.treg[CPU_AL & 0x03];	/* as bios1b.c seeks */
	track = (track << 1) + ((CPU_DH ^ (CPU_AL >> 2)) & 1);
	if (((CPU_AH & 0x0f) != 0x06) || ((CPU_AL & 0xf0) != 0x90) ||	/* read data, 2HD */
		(!fdd->fname[0]) || (fdd->type != DISKTYPE_D88) ||
		(track >= (UINT)NELEMENTS(fdd->inf.d88.ptr)) || (!fdd->inf.d88.ptr[track]) ||
		(fdd->inf.d88.ptr[track] >= fdd->inf.d88.fd_size))
	{
		return FALSE;
	}
	pos = fdd->inf.d88.ptr[track];
	next = fdd->inf.d88.fd_size;				/* the track ends where the next begins */
	for (i = 0; i < (UINT)NELEMENTS(fdd->inf.d88.ptr); i++)
	{
		if ((fdd->inf.d88.ptr[i] > pos) && (fdd->inf.d88.ptr[i] < next))
		{
			next = fdd->inf.d88.ptr[i];
		}
	}
	size = next - pos;
	if (size > D88_TRACKREAD)
	{
		size = D88_TRACKREAD;
	}

	NP2_Semaphore_Wait(&s_lock);
	file_cpyname(s_readname, fdd->fname, NELEMENTS(s_readname));
	s_readpos = pos;
	s_readsize = size;
	s_read = READ_RUNNING;
	NP2_Semaphore_Release(&s_lock);
	NP2_Semaphore_Release(&s_wake);
	return TRUE;
}

/**
 * Called on every disk timer tick
 */
void fddlive_callback(void)
{
	REG8 drv;
	BOOL gone[MAX_FDDFILE];

	if ((!fddlive_enable) || (++s_ticks < LOOK_TICKS) || (!start()))
	{
		return;
	}
	fdd_writeback_d88();		/* a save must be in the file before the disk can come out */
	NP2_Semaphore_Wait(&s_lock);
	if (s_looking)
	{
		NP2_Semaphore_Release(&s_lock);
		return;
	}
	for (drv = 0; drv < MAX_FDDFILE; drv++)
	{
		if (np2cfg.fddfile[drv][0])
		{
			file_cpyname(s_name[drv], np2cfg.fddfile[drv], NELEMENTS(s_name[drv]));
		}
		gone[drv] = s_gone[drv];
	}
	s_looking = TRUE;
	NP2_Semaphore_Release(&s_lock);
	NP2_Semaphore_Release(&s_wake);
	s_ticks = 0;

	for (drv = 0; drv < MAX_FDDFILE; drv++)
	{
		if (gone[drv])
		{
			s_out[drv] = TRUE;
			if (fdd_diskready(drv))
			{
				diskdrv_setfdd(drv, NULL, 0);
			}
		}
		else if (s_out[drv])
		{
			if (fdd_diskready(drv))
			{
				s_out[drv] = FALSE;
			}
			else
			{
				diskdrv_setfdd(drv, s_name[drv], 0);
			}
		}
	}
}

/**
 * Stop the worker
 */
void fddlive_shutdown(void)
{
	if (s_started)
	{
		NP2_Semaphore_Wait(&s_lock);
		s_quit = TRUE;
		NP2_Semaphore_Release(&s_lock);
		NP2_Semaphore_Release(&s_wake);
		NP2_Thread_Wait(&s_thread, NULL);
		NP2_Thread_Destroy(&s_thread);
		NP2_Semaphore_Destroy(&s_wake);
		NP2_Semaphore_Destroy(&s_lock);
		s_started = FALSE;
	}
	fddlive_enable = FALSE;
	s_read = READ_IDLE;
	s_looking = FALSE;
	ZeroMemory(s_name, sizeof(s_name));
	ZeroMemory(s_gone, sizeof(s_gone));
	ZeroMemory(s_out, sizeof(s_out));
}

#else	/* SUPPORT_FDD_LIVE */

BOOL fddlive_wait(void)
{
	return FALSE;
}

void fddlive_callback(void)
{
}

void fddlive_shutdown(void)
{
}

#endif	/* SUPPORT_FDD_LIVE */
