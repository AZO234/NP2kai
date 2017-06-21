/**
 * @file	nevent.c
 * @brief	Implementation of the event
 */

#include "compiler.h"
#include "nevent.h"
#include "cpucore.h"
#include "pccore.h"

	_NEVENT g_nevent;

void nevent_allreset(void)
{
	/* すべてをリセット */
	memset(&g_nevent, 0, sizeof(g_nevent));
}

void nevent_get1stevent(void)
{
	/* 最短のイベントのクロック数をセット */
	if (g_nevent.readyevents)
	{
		CPU_BASECLOCK = g_nevent.item[g_nevent.level[0]].clock;
	}
	else
	{
		/* イベントがない場合のクロック数をセット */
		CPU_BASECLOCK = NEVENT_MAXCLOCK;
	}

	/* カウンタへセット */
	CPU_REMCLOCK = CPU_BASECLOCK;
}

static void nevent_execute(void)
{
	UINT nEvents;
	UINT i;
	NEVENTID id;
	NEVENTITEM item;

	nEvents = 0;
	for (i = 0; i < g_nevent.waitevents; i++)
	{
		id = g_nevent.waitevent[i];
		item = &g_nevent.item[id];

		/* コールバックの実行 */
		if (item->proc != NULL)
		{
			item->proc(item);

			/* 次回に持ち越しのイベントのチェック */
			if (item->flag & NEVENT_WAIT)
			{
				g_nevent.waitevent[nEvents++] = id;
			}
		}
		else {
			item->flag &= ~(NEVENT_WAIT);
		}
		item->flag &= ~(NEVENT_SETEVENT);
	}
	g_nevent.waitevents = nEvents;
}

void nevent_progress(void)
{
	UINT nEvents;
	SINT32 nextbase;
	UINT i;
	NEVENTID id;
	NEVENTITEM item;

	CPU_CLOCK += CPU_BASECLOCK;
	nEvents = 0;
	nextbase = NEVENT_MAXCLOCK;
	for (i = 0; i < g_nevent.readyevents; i++)
	{
		id = g_nevent.level[i];
		item = &g_nevent.item[id];
		item->clock -= CPU_BASECLOCK;
		if (item->clock > 0)
		{
			/* イベント待ち中 */
			g_nevent.level[nEvents++] = id;
			if (nextbase >= item->clock)
			{
				nextbase = item->clock;
			}
		}
		else
		{
			/* イベント発生 */
			if (!(item->flag & (NEVENT_SETEVENT | NEVENT_WAIT)))
			{
				g_nevent.waitevent[g_nevent.waitevents++] = id;
			}
			item->flag |= NEVENT_SETEVENT;
			item->flag &= ~(NEVENT_ENABLE);
//			TRACEOUT(("event = %x", id));
		}
	}
	g_nevent.readyevents = nEvents;
	CPU_BASECLOCK = nextbase;
	CPU_REMCLOCK += nextbase;
	nevent_execute();
//	TRACEOUT(("nextbase = %d (%d)", nextbase, CPU_REMCLOCK));
}


void nevent_reset(NEVENTID id)
{
	UINT i;

	/* 現在進行してるイベントを検索 */
	for (i = 0; i < g_nevent.readyevents; i++)
	{
		if (g_nevent.level[i] == id)
		{
			break;
		}
	}
	/* イベントは存在した？ */
	if (i < g_nevent.readyevents)
	{
		/* 存在していたら削る */
		g_nevent.readyevents--;
		for (; i < g_nevent.readyevents; i++)
		{
			g_nevent.level[i] = g_nevent.level[i + 1];
		}
	}
}

void nevent_waitreset(NEVENTID id)
{
	UINT i;

	/* 現在進行してるイベントを検索 */
	for (i = 0; i < g_nevent.waitevents; i++)
	{
		if (g_nevent.waitevent[i] == id)
		{
			break;
		}
	}
	/* イベントは存在した？ */
	if (i < g_nevent.waitevents)
	{
		/* 存在していたら削る */
		g_nevent.waitevents--;
		for (; i < g_nevent.waitevents; i++)
		{
			g_nevent.waitevent[i] = g_nevent.waitevent[i + 1];
		}
	}
}

void nevent_set(NEVENTID id, SINT32 eventclock, NEVENTCB proc, NEVENTPOSITION absolute)
{
	SINT32 clk;
	NEVENTITEM item;
	UINT eventId;
	UINT i;

//	TRACEOUT(("event %d - %xclocks", id, eventclock));

	clk = CPU_BASECLOCK - CPU_REMCLOCK;
	item = &g_nevent.item[id];
	item->proc = proc;
	item->flag = 0;
	if (absolute)
	{
		item->clock = eventclock + clk;
	}
	else
	{
		item->clock += eventclock;
	}
#if 0
	if (item->clock < clk)
	{
		item->clock = clk;
	}
#endif
	/* イベントの削除 */
	nevent_reset(id);

	/* イベントの挿入位置の検索 */
	for (eventId = 0; eventId < g_nevent.readyevents; eventId++)
	{
		if (item->clock < g_nevent.item[g_nevent.level[eventId]].clock)
		{
			break;
		}
	}

	/* イベントの挿入 */
	for (i = g_nevent.readyevents; i > eventId; i--)
	{
		g_nevent.level[i] = g_nevent.level[i - 1];
	}
	g_nevent.level[eventId] = id;
	g_nevent.readyevents++;

	/* もし最短イベントだったら... */
	if (eventId == 0)
	{
		clk = CPU_BASECLOCK - item->clock;
		CPU_BASECLOCK -= clk;
		CPU_REMCLOCK -= clk;
//		TRACEOUT(("reset nextbase -%d (%d)", clock, CPU_REMCLOCK));
	}
}

void nevent_setbyms(NEVENTID id, SINT32 ms, NEVENTCB proc, NEVENTPOSITION absolute)
{
	nevent_set(id, (pccore.realclock / 1000) * ms, proc, absolute);
}

BOOL nevent_iswork(NEVENTID id)
{
	UINT i;

	/* 現在進行してるイベントを検索 */
	for (i = 0; i < g_nevent.readyevents; i++)
	{
		if (g_nevent.level[i] == id)
		{
			return TRUE;
		}
	}
	return FALSE;
}

SINT32 nevent_getremain(NEVENTID id)
{
	UINT i;

	/* 現在進行してるイベントを検索 */
	for (i = 0; i < g_nevent.readyevents; i++)
	{
		if (g_nevent.level[i] == id)
		{
			return (g_nevent.item[id].clock - (CPU_BASECLOCK - CPU_REMCLOCK));
		}
	}
	return -1;
}

void nevent_forceexit(void)
{
	if (CPU_REMCLOCK > 0)
	{
		CPU_BASECLOCK -= CPU_REMCLOCK;
		CPU_REMCLOCK = 0;
	}
}
