/**
 * @file	fddsndout.cpp
 * @brief	FDD / board(relay) operation-sound separate output-device manager.
 *
 * Design notes (addressing concurrency / lifecycle hazards):
 *  - Slow device work (DirectSoundCreate/Open) runs OUTSIDE m_cs; a freshly
 *    built device is swapped into place under a short lock, and the old device
 *    is torn down after the lock is released. The emulation thread's
 *    Play/Stop therefore never blocks on a wedged audio endpoint.
 *  - Every teardown stops the group's sounds first, so a looping seek buffer is
 *    never orphaned. On a routing change the group is also silenced on the main
 *    device, so a loop started there before separation cannot get stuck.
 *  - Aux playback honours the sound manager's mute state; Disable()/mute stop
 *    aux sounds too (fddsndout_stopall).
 */

#include "compiler.h"

#if defined(SUPPORT_FDDSNDDEV)

#include "soundmng.h"
#include "sddsound3.h"
#include "fddsndout.h"
#include "pccore.h"

namespace
{

/**
 * @brief PCM entry (sound number + embedded WAVE resource name)
 */
struct PcmDef
{
	UINT	num;
	LPCTSTR	res;
};

const PcmDef s_fddPcm[] =
{
	{ SOUND_PCMSEEK,	TEXT("SEEKWAV") },
	{ SOUND_PCMSEEK1,	TEXT("SEEK1WAV") },
};

const PcmDef s_boardPcm[] =
{
	{ SOUND_RELAY1,		TEXT("RELAY1WAV") },
};

/**
 * @brief group definition
 */
struct GroupDef
{
	const PcmDef*	pcm;
	UINT			count;
};

const GroupDef s_group[FDDSND_GROUP_COUNT] =
{
	{ s_fddPcm,		_countof(s_fddPcm) },
	{ s_boardPcm,	_countof(s_boardPcm) },
};

enum
{
	VOL_MAX = 200,				//!< separate-output volume ceiling (0-200%)
	ITEMDATA_KEEP = 0xFFFF		//!< (shared with d_sound.cpp) missing-device sentinel
};

/**
 * @brief aux output manager (up to one DirectSound device per group)
 */
class CFddSndOut
{
public:
	static CFddSndOut& GetInstance();

	CFddSndOut();
	~CFddSndOut();

	void SetWindow(HWND hWnd);
	void Reconfig();
	void Close();
	void StopAll();
	BOOL Play(UINT nNum, BOOL bLoop);
	BOOL Stop(UINT nNum);

private:
	CSoundDeviceDSound3* BuildGroup(int nGroup, HWND hWnd, const OEMCHAR* lpDevice, int nVolume);
	void StopGroupLocked(int nGroup);	//!< stop this group's sounds on the aux device (m_cs held)
	void StopGroupOnMain(int nGroup);	//!< stop this group's sounds on the main device (no lock)
	int GroupOfPcm(UINT nNum) const;

	HWND					m_hWnd;
	bool					m_enabled[FDDSND_GROUP_COUNT];
	CSoundDeviceDSound3*	m_pDevice[FDDSND_GROUP_COUNT];
	CRITICAL_SECTION		m_cs;
};

CFddSndOut& CFddSndOut::GetInstance()
{
	static CFddSndOut s_instance;
	return s_instance;
}

CFddSndOut::CFddSndOut()
	: m_hWnd(NULL)
{
	for (int i = 0; i < FDDSND_GROUP_COUNT; i++)
	{
		m_enabled[i] = false;
		m_pDevice[i] = NULL;
	}
	::InitializeCriticalSection(&m_cs);
}

CFddSndOut::~CFddSndOut()
{
	Close();
	::DeleteCriticalSection(&m_cs);
}

void CFddSndOut::SetWindow(HWND hWnd)
{
	::EnterCriticalSection(&m_cs);
	m_hWnd = hWnd;
	::LeaveCriticalSection(&m_cs);
}

int CFddSndOut::GroupOfPcm(UINT nNum) const
{
	switch (nNum)
	{
		case SOUND_PCMSEEK:
		case SOUND_PCMSEEK1:
			return FDDSND_GROUP_FDD;

		case SOUND_RELAY1:
			return FDDSND_GROUP_BOARD;

		default:
			return -1;
	}
}

/**
 * Build a group's aux device. Runs WITHOUT m_cs (Open() can block).
 * lpDevice == NULL or "" -> not separated (returns NULL => fall back to main).
 */
CSoundDeviceDSound3* CFddSndOut::BuildGroup(int nGroup, HWND hWnd, const OEMCHAR* lpDevice, int nVolume)
{
	if ((hWnd == NULL) || (lpDevice == NULL) || (lpDevice[0] == '\0'))
	{
		return NULL;
	}

	CSoundDeviceDSound3* pDevice = new CSoundDeviceDSound3;
	if (!pDevice->Open(lpDevice, hWnd))
	{
		/* device missing / open failed -> fall back to the main device */
		delete pDevice;
		return NULL;
	}

	const GroupDef& g = s_group[nGroup];
	for (UINT i = 0; i < g.count; i++)
	{
		/* set the volume first so LoadPCM bakes it in a single buffer build */
		pDevice->SetPCMVolume(g.pcm[i].num, nVolume);
		pDevice->LoadPCM(g.pcm[i].num, g.pcm[i].res);
	}
	return pDevice;
}

void CFddSndOut::StopGroupLocked(int nGroup)
{
	if (m_pDevice[nGroup])
	{
		const GroupDef& g = s_group[nGroup];
		for (UINT i = 0; i < g.count; i++)
		{
			m_pDevice[nGroup]->StopPCM(g.pcm[i].num);
		}
	}
}

void CFddSndOut::StopGroupOnMain(int nGroup)
{
	const GroupDef& g = s_group[nGroup];
	for (UINT i = 0; i < g.count; i++)
	{
		CSoundMng::GetInstance()->StopPCM(static_cast<SoundPCMNumber>(g.pcm[i].num));
	}
}

void CFddSndOut::Reconfig()
{
	HWND hWnd;
	OEMCHAR fddDev[MAX_PATH];
	OEMCHAR brdDev[MAX_PATH];
	int fddVol;
	int brdVol;

	/* snapshot the desired configuration */
	::EnterCriticalSection(&m_cs);
	hWnd = m_hWnd;
	::lstrcpyn(fddDev, np2cfg.fddSndDevice, _countof(fddDev));
	::lstrcpyn(brdDev, np2cfg.boardSndDevice, _countof(brdDev));
	fddVol = np2cfg.fddSndVol;
	brdVol = np2cfg.boardSndVol;
	::LeaveCriticalSection(&m_cs);

	if (fddVol < 0) fddVol = 0; else if (fddVol > VOL_MAX) fddVol = VOL_MAX;
	if (brdVol < 0) brdVol = 0; else if (brdVol > VOL_MAX) brdVol = VOL_MAX;

	/* build the new devices OUTSIDE the lock */
	CSoundDeviceDSound3* pNewFdd = BuildGroup(FDDSND_GROUP_FDD, hWnd, fddDev, fddVol);
	CSoundDeviceDSound3* pNewBrd = BuildGroup(FDDSND_GROUP_BOARD, hWnd, brdDev, brdVol);

	CSoundDeviceDSound3* pOldFdd;
	CSoundDeviceDSound3* pOldBrd;

	/* swap atomically */
	::EnterCriticalSection(&m_cs);
	StopGroupLocked(FDDSND_GROUP_FDD);
	pOldFdd = m_pDevice[FDDSND_GROUP_FDD];
	m_pDevice[FDDSND_GROUP_FDD] = pNewFdd;
	m_enabled[FDDSND_GROUP_FDD] = (pNewFdd != NULL);
	StopGroupLocked(FDDSND_GROUP_BOARD);
	pOldBrd = m_pDevice[FDDSND_GROUP_BOARD];
	m_pDevice[FDDSND_GROUP_BOARD] = pNewBrd;
	m_enabled[FDDSND_GROUP_BOARD] = (pNewBrd != NULL);
	::LeaveCriticalSection(&m_cs);

	/* tear down old devices outside the lock */
	if (pOldFdd)
	{
		pOldFdd->Close();
		delete pOldFdd;
	}
	if (pOldBrd)
	{
		pOldBrd->Close();
		delete pOldBrd;
	}

	/* a loop started on the MAIN device before this group became separated
	   would otherwise never be stopped -> silence it on the main device */
	if (pNewFdd)
	{
		StopGroupOnMain(FDDSND_GROUP_FDD);
	}
	if (pNewBrd)
	{
		StopGroupOnMain(FDDSND_GROUP_BOARD);
	}
}

void CFddSndOut::Close()
{
	CSoundDeviceDSound3* pDev[FDDSND_GROUP_COUNT];

	::EnterCriticalSection(&m_cs);
	for (int i = 0; i < FDDSND_GROUP_COUNT; i++)
	{
		StopGroupLocked(i);
		pDev[i] = m_pDevice[i];
		m_pDevice[i] = NULL;
		m_enabled[i] = false;
	}
	::LeaveCriticalSection(&m_cs);

	for (int i = 0; i < FDDSND_GROUP_COUNT; i++)
	{
		if (pDev[i])
		{
			pDev[i]->Close();
			delete pDev[i];
		}
	}
}

void CFddSndOut::StopAll()
{
	::EnterCriticalSection(&m_cs);
	for (int i = 0; i < FDDSND_GROUP_COUNT; i++)
	{
		StopGroupLocked(i);
	}
	::LeaveCriticalSection(&m_cs);
}

BOOL CFddSndOut::Play(UINT nNum, BOOL bLoop)
{
	const int nGroup = GroupOfPcm(nNum);
	if (nGroup < 0)
	{
		return FALSE;
	}

	BOOL bHandled = FALSE;
	::EnterCriticalSection(&m_cs);
	if ((m_enabled[nGroup]) && (m_pDevice[nGroup]))
	{
		/* routed to the aux device: the caller must not use the main device.
		   Honour mute -- report handled but stay silent when muted. */
		bHandled = TRUE;
		if (!CSoundMng::GetInstance()->IsMuted())
		{
			m_pDevice[nGroup]->PlayPCM(nNum, bLoop);
		}
	}
	::LeaveCriticalSection(&m_cs);
	return bHandled;
}

BOOL CFddSndOut::Stop(UINT nNum)
{
	const int nGroup = GroupOfPcm(nNum);
	if (nGroup < 0)
	{
		return FALSE;
	}

	BOOL bHandled = FALSE;
	::EnterCriticalSection(&m_cs);
	if ((m_enabled[nGroup]) && (m_pDevice[nGroup]))
	{
		m_pDevice[nGroup]->StopPCM(nNum);
		bHandled = TRUE;
	}
	::LeaveCriticalSection(&m_cs);
	return bHandled;
}

} // anonymous namespace


// ---- C interface

extern "C"
{

void fddsndout_setwindow(HWND hWnd)
{
	CFddSndOut::GetInstance().SetWindow(hWnd);
}

void fddsndout_reconfig(void)
{
	CFddSndOut::GetInstance().Reconfig();
}

void fddsndout_close(void)
{
	CFddSndOut::GetInstance().Close();
}

void fddsndout_stopall(void)
{
	CFddSndOut::GetInstance().StopAll();
}

BOOL fddsndout_pcmplay(UINT nNum, BOOL bLoop)
{
	return CFddSndOut::GetInstance().Play(nNum, bLoop);
}

BOOL fddsndout_pcmstop(UINT nNum)
{
	return CFddSndOut::GetInstance().Stop(nNum);
}

} // extern "C"

#endif	/* SUPPORT_FDDSNDDEV */
