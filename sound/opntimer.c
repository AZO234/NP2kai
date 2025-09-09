/**
 * @file	opntimer.c
 * @brief	Implementation of OPN timer
 */

#include <compiler.h>
#include "opntimer.h"
#include <pccore.h>
#include <io/iocore.h>
#include <sound/fmboard.h>

/** IRQ */
static const UINT8 s_irqtable[4] = {0x03, 0x0d, 0x0a, 0x0c};

/**
 * Rise Timer-A
 * @param[in] opna The instance
 * @param[in] absolute If ture is absolute
 */
static void set_fmtimeraevent(POPNA opna, NEVENTPOSITION absolute)
{
	SINT32 l;
	int OPNAidx;
	int v_NEVENT_FMTIMERA, v_NEVENT_FMTIMERB;

	OPNAidx = (opna == &(g_opna[1])) ? 1 : 0;
	v_NEVENT_FMTIMERA = (OPNAidx==0) ? NEVENT_FMTIMERA : NEVENT_FMTIMER2A;
	v_NEVENT_FMTIMERB = (OPNAidx==0) ? NEVENT_FMTIMERB : NEVENT_FMTIMER2B;

	l = 18 * (1024 - ((opna->s.reg[0x24] << 2) | (opna->s.reg[0x25] & 3)));
	if (pccore.cpumode & CPUMODE_8MHZ)			/* 4MHz */
	{
		l = (l * 1248) / 625;
	}
	else										/* 5MHz */
	{
		l = (l * 1536) / 625;
	}
	l *= pccore.multiple;
//	TRACEOUT(("FMTIMER-A: %08x-%d", l, absolute));
	nevent_set((NEVENTID)v_NEVENT_FMTIMERA, l, fmport_a, absolute);
}

/**
 * Rise Timer-B
 * @param[in] opna The instance
 * @param[in] absolute If ture is absolute
 */
static void set_fmtimerbevent(POPNA opna, NEVENTPOSITION absolute)
{
	SINT32 l;
	int OPNAidx;
	int v_NEVENT_FMTIMERA, v_NEVENT_FMTIMERB;

	OPNAidx = (opna == &(g_opna[1])) ? 1 : 0;
	v_NEVENT_FMTIMERA = (OPNAidx==0) ? NEVENT_FMTIMERA : NEVENT_FMTIMER2A;
	v_NEVENT_FMTIMERB = (OPNAidx==0) ? NEVENT_FMTIMERB : NEVENT_FMTIMER2B;

	l = 288 * (256 - opna->s.reg[0x26]);
	if (pccore.cpumode & CPUMODE_8MHZ)			/* 4MHz */
	{
		l = (l * 1248) / 625;
	}
	else
	{											/* 5MHz */
		l = (l * 1536) / 625;
	}
	l *= pccore.multiple;
//	TRACEOUT(("FMTIMER-B: %08x-%d", l, absolute));
	nevent_set((NEVENTID)v_NEVENT_FMTIMERB, l, fmport_b, absolute);
}

/**
 * Timer-A event
 * @param[in] item The item of event
 */
void fmport_a(NEVENTITEM item)
{
	POPNA opna = (POPNA)item->userData;
	BOOL intreq = FALSE;

	if(!opna) return;

	if (item->flag & NEVENT_SETEVENT)
	{
		if(g_pcm86.irq==opna->s.irq){
			intreq = pcm86gen_intrq();
			if(!(opna->s.status & 0x01) && g_pcm86.irqflag){
				intreq = TRUE;
			}
		}
		if ((opna->s.reg[0x27] & 0x04) && !(opna->s.status & 0x01))
		{
			opna->s.status |= 0x01;
			intreq = TRUE;
		}
		if ((intreq) && (opna->s.irq != 0xff))
		{
			pic_setirq(opna->s.irq);
//			TRACEOUT(("fm int-A"));
		}

		set_fmtimeraevent(opna, NEVENT_RELATIVE);

		if ((opna->s.reg[0x27] & 0xc0) == 0x80)
		{
			opngen_csm(&opna->opngen);
		}
	}
}

/**
 * Timer-B event
 * @param[in] item The item of event
 */
void fmport_b(NEVENTITEM item)
{
	POPNA opna = (POPNA)item->userData;
	BOOL intreq = FALSE;
	
	if(!opna) return;

	if (item->flag & NEVENT_SETEVENT)
	{
		if(g_pcm86.irq==opna->s.irq){
			intreq = pcm86gen_intrq();
			if(!(opna->s.status & 0x02) && g_pcm86.irqflag){
				intreq = TRUE;
			}
		}
		if ((opna->s.reg[0x27] & 0x08) && !(opna->s.status & 0x02))
		{
			opna->s.status |= 0x02;
			intreq = TRUE;
		}
		if ((intreq) && (opna->s.irq != 0xff))
		{
			pic_setirq(opna->s.irq);
//			TRACEOUT(("fm int-B"));
		}

		set_fmtimerbevent(opna, NEVENT_RELATIVE);
	}
}

/**
 * Reset
 * @param[in] opna The instance
 * @param[in] nIrq DIPSW
 * @param[in] nTimerA The ID of Timer-A
 * @param[in] nTimerB The ID of Timer-B
 */
void opna_timer(POPNA opna, UINT nIrq, NEVENTID nTimerA, NEVENTID nTimerB)
{
	opna->s.intr = nIrq & 0xc0;
	if (nIrq & 0x10)
	{
		opna->s.irq = s_irqtable[nIrq >> 6];
	}

	g_nevent.item[nTimerA].userData = (INTPTR)opna;
	g_nevent.item[nTimerB].userData = (INTPTR)opna;

//	pic_registext(opna->s.irq);
}

/**
 * Write the register
 * @param[in] opna The instance
 * @param[in] cData The data
 */
void opna_settimer(POPNA opna, REG8 cData)
{
//	TRACEOUT(("fm 27 %x [%.4x:%.4x]", cData, CPU_CS, CPU_IP));
	int OPNAidx;
	int v_NEVENT_FMTIMERA, v_NEVENT_FMTIMERB;

	OPNAidx = (opna == &(g_opna[1])) ? 1 : 0;
	v_NEVENT_FMTIMERA = (OPNAidx==0) ? NEVENT_FMTIMERA : NEVENT_FMTIMER2A;
	v_NEVENT_FMTIMERB = (OPNAidx==0) ? NEVENT_FMTIMERB : NEVENT_FMTIMER2B;

	if(cData & 0x10){
		//nevent_reset((NEVENTID)v_NEVENT_FMTIMERA);
		opna->s.reg[0x27] &= ~0x10;
		opna->s.status &= ~0x01;
	}
	if(cData & 0x20){
		//nevent_reset((NEVENTID)v_NEVENT_FMTIMERB);
		opna->s.reg[0x27] &= ~0x20;
		opna->s.status &= ~0x02;
	}

	opna->s.status &= ~((cData & 0x30) >> 4);
	if (cData & 0x01)
	{
		if (!nevent_iswork((NEVENTID)v_NEVENT_FMTIMERA))
		{
			set_fmtimeraevent(opna, NEVENT_ABSOLUTE);
		}
	}
	else
	{
		nevent_reset((NEVENTID)v_NEVENT_FMTIMERA);
	}

	if (cData & 0x02)
	{
		if (!nevent_iswork((NEVENTID)v_NEVENT_FMTIMERB))
		{
			set_fmtimerbevent(opna, NEVENT_ABSOLUTE);
		}
	}
	else
	{
		nevent_reset((NEVENTID)v_NEVENT_FMTIMERB);
	}

	if ((!(cData & 0x03) || (cData & 0x30)) && (opna->s.irq != 0xff))
	{
		PCM86 pcm86 = &g_pcm86;
		if(pcm86->irq!=opna->s.irq || !pcm86->irqflag){
			pic_resetirq(opna->s.irq);
		}
	}
}
