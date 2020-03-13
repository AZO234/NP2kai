/**
 * @file	nevent.h
 * @brief	Interface of the event
 */

#pragma once

enum
{
	NEVENT_MAXCLOCK		= 0x400000,
};

/**
 * NEvent ID
 */
enum tagNEventId
{
	NEVENT_FLAMES		= 0,
	NEVENT_ITIMER		= 1,
	NEVENT_BEEP			= 2,
	NEVENT_RS232C		= 3,
	NEVENT_MUSICGEN		= 4,
	NEVENT_FMTIMERA		= 5,
	NEVENT_FMTIMERB		= 6,
	NEVENT_FMTIMER2A	= 7,
	NEVENT_FMTIMER2B	= 8,
	//	NEVENT_FMTIMER3A	= 9,
	//	NEVENT_FMTIMER3B	= 10,
	//	NEVENT_FMTIMER4A	= 11,
	//	NEVENT_FMTIMER4B	= 12,
	NEVENT_MOUSE		= 13,
	NEVENT_KEYBOARD		= 14,
	NEVENT_MIDIWAIT		= 15,
	NEVENT_MIDIINT		= 16,
	NEVENT_PICMASK		= 17,
	NEVENT_S98TIMER		= 18,
	NEVENT_CS4231		= 19,
	NEVENT_GDCSLAVE		= 20,
	NEVENT_FDBIOSBUSY	= 21,
	NEVENT_FDCINT		= 22,
	NEVENT_PC9861CH1	= 23,
	NEVENT_PC9861CH2	= 24,
	NEVENT_86PCM		= 25,
	NEVENT_SASIIO		= 26,
	NEVENT_SCSIIO		= 27,
	NEVENT_CDWAIT		= 28, // XXX: 勝手に使ってOK?
	NEVENT_CT1741		= 29, // np2sより 28を使っちゃったので29に np21w ver0.86 rev29
#if defined(VAEG_EXT)
	NEVENT_FDCTIMER		= 29,
	NEVENT_FDDMOTOR		= 30,
	NEVENT_FDCSTEPWAIT	= 31,
#endif
#if defined(VAEG_FIX)
	NEVENT_FDCSTATE		= 32,
#endif
#if defined(SUPPORT_WAB)
	NEVENT_WABSNDOFF	= 33,
#endif
	/* ---- */
	NEVENT_MAXEVENTS	= 34
};
typedef enum tagNEventId NEVENTID;

enum
{
	NEVENT_ENABLE		= 0x0001,
	NEVENT_SETEVENT		= 0x0002,
	NEVENT_WAIT			= 0x0004
};

/**
 * event position
 */
enum tagNEventPosition
{
	NEVENT_RELATIVE		= 0,		/*!< relative */
	NEVENT_ABSOLUTE		= 1			/*!< absolute */
};
typedef enum tagNEventPosition NEVENTPOSITION;		/*!< the defines of position */

struct _neventitem;
typedef	struct _neventitem	_NEVENTITEM;
typedef	struct _neventitem	*NEVENTITEM;
typedef void (*NEVENTCB)(NEVENTITEM item);

struct _neventitem
{
	SINT32		clock;
	UINT32		flag;
	NEVENTCB	proc;
	INTPTR		userData;
};

typedef struct {
	UINT		readyevents;
	UINT		waitevents;
	NEVENTID	level[NEVENT_MAXEVENTS];
	NEVENTID	waitevent[NEVENT_MAXEVENTS];
	_NEVENTITEM	item[NEVENT_MAXEVENTS];
} _NEVENT, *NEVENT;


#ifdef __cplusplus
extern "C" {
#endif

extern	_NEVENT		g_nevent;

// 初期化
void nevent_allreset(void);

// 最短イベントのセット
void nevent_get1stevent(void);

// 時間を進める
void nevent_progress(void);

// イベントの実行
void nevent_execule(void);

// イベントの追加
void nevent_set(NEVENTID id, SINT32 eventclock, NEVENTCB proc, NEVENTPOSITION absolute);
void nevent_setbyms(NEVENTID id, SINT32 ms, NEVENTCB proc, NEVENTPOSITION absolute);

// イベントの削除
void nevent_reset(NEVENTID id);
void nevent_waitreset(NEVENTID id);

// イベントの動作状態取得
BOOL nevent_iswork(NEVENTID id);

// イベント実行までのクロック数の取得
SINT32 nevent_getremain(NEVENTID id);

// NEVENTの強制脱出
void nevent_forceexit(void);

void nevent_changeclock(UINT32 oldclock, UINT32 newclock);

#ifdef __cplusplus
}
#endif
