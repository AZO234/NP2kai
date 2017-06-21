/**
 * @file	amd98.c
 * @brief	Implementation of AMD-98
 */

#include "compiler.h"
#include "amd98.h"
#include <math.h>
#include "keystat.h"
#include "iocore.h"
#include "cbuscore.h"
#include "generic/keydisp.h"
#include "sound.h"
#include "sound/pcmmix.h"
#include "joymng.h"

	AMD98	g_amd98;

/* ないよりあったほーが良い程度のリズム… */
static struct
{
	PMIXHDR	hdr;
	PMIXTRK	trk[4];
	UINT	rate;
	UINT	enable;
} amd98r;

static	REG8	rapids = 0;

REG8 amd98_getjoy(UINT no)
{
	REG8	ret;

	rapids ^= 0xf0;
	ret = 0xff;
	if (no == 1)
	{
		ret &= (joymng_getstat() | (rapids & 0x30));
		if (np2cfg.KEY_MODE == 1)
		{
			ret &= keystat_getjoy();
		}
	}
	else
	{
		if (np2cfg.KEY_MODE == 2)
		{
			ret &= keystat_getjoy();
		}
	}
	if (np2cfg.BTN_RAPID)
	{
		ret |= rapids;
	}

	/* rapidと非rapidを合成 */
	ret &= ((ret >> 2) | (~0x30));

	if (np2cfg.BTN_MODE)
	{
		UINT8 bit1 = (ret & 0x20) >> 1;
		UINT8 bit2 = (ret & 0x10) << 1;
		ret = (ret & (~0x30)) | bit1 | bit2;
	}

	return ret;
}

static void pcmmake1(PMIXDAT *dat, UINT rate, int vol, double hz, double env)
{

	UINT	i;
	double	x;
	double	y;
	double	slast;
	double	s;
	double	v;
	UINT	size;
	SINT16	*ptr;

	x = 44100.0 * 2.0 * M_PI / ((double)rate * hz);
	y = 44100.0 / 256.0 / (double)rate;
	slast = 0.0;
	for (i = 0; i < rate; i++)
	{
		s = sin(x * (double)i);
		v = pow(env, (double)i * y) * (double)vol;
		if ((v < 128.0) && (slast < 0.0) && (s >= 0.0))
		{
			break;
		}
		slast = s;
	}
	size = i;
	if (!size)
	{
		return;
	}
	ptr = (SINT16 *)_MALLOC(size * sizeof(SINT16), "AMD98");
	if (ptr == NULL)
	{
		return;
	}
	for (i=0; i<size; i++)
	{
		s = sin(x * (double)i);
		v = pow(env, (double)i * y) * (double)vol;
		ptr[i] = (SINT16)(s * v);
	}
	dat->sample = ptr;
	dat->samples = size;
}

static void pcmmake2(PMIXDAT *dat, UINT rate, int vol, double hz, double env, double k)
{

	UINT	i;
	double	x;
	double	y;
	double	p;
	double	s;
	double	slast;
	double	v;
	UINT	size;
	SINT16	*ptr;

	x = 2.0 * M_PI * hz / (double)rate;
	y = 44100.0 / 256.0 / (double)rate;
	p = 0.0;
	slast = 0.0;
	for (i = 0; i < rate; i++)
	{
		p += x * pow(k, (double)i * y);
		s = sin(p);
		v = pow(env, (double)i * y) * (double)vol;
		if ((v < 128.0) && (slast < 0.0) && (s >= 0.0))
		{
			break;
		}
		slast = s;
	}
	size = i;
	if (!size)
	{
		return;
	}
	ptr = (SINT16 *)_MALLOC(size * sizeof(SINT16), "AMD98");
	if (ptr == NULL)
	{
		return;
	}
	p = 0.0;
	for (i = 0; i < size; i++)
	{
		p += x * pow(k, (double)i * y);
		s = sin(p);
		v = pow(env, (double)i * y) * (double)vol;
		ptr[i] = (SINT16)(s * v);
	}
	dat->sample = ptr;
	dat->samples = size;
}

/**
 * Initialize
 * @param[in] rate The sample rate, in samples per second (hertz)
 */
void amd98_initialize(UINT rate)
{
	memset(&amd98r, 0, sizeof(amd98r));
	amd98r.rate = rate;
}

/**
 * Deinitialize
 */
void amd98_deinitialize(void)
{
	int		i;
	void	*ptr;

	amd98r.hdr.enable = 0;
	for (i = 0; i < 4; i++)
	{
		ptr = amd98r.trk[i].data.sample;
		amd98r.trk[i].data.sample = NULL;
		if (ptr)
		{
			_MFREE(ptr);
		}
	}
}

static void amd98_rhythmload(void)
{
	UINT	i;

	if (!amd98r.hdr.enable)
	{
		TRACEOUT(("AMD98 Rhythm load"));
		amd98r.hdr.enable = 0x0f;
		/* bd */
		pcmmake1(&amd98r.trk[0].data, amd98r.rate,
							24000, 889.0476190476, 0.9446717478);
		/* lt */
		pcmmake2(&amd98r.trk[1].data, amd98r.rate,
							6400, 172.9411764706, 0.8665145391, 0.9960000000);
		/* ht */
		pcmmake2(&amd98r.trk[2].data, amd98r.rate,
							9600, 213.0000000000, 0.8665145391, 0.9960000000);
		/* sd */
		pcmmake1(&amd98r.trk[3].data, amd98r.rate,
							12000, 255.4400000000, 0.8538230481);
		for (i = 0; i < 4; i++)
		{
			amd98r.trk[i].flag = PMIXFLAG_L | PMIXFLAG_R;
			amd98r.trk[i].volume = 1 << 12;
		}
	}
}


/* ---- */

static void amd98_rhythm(UINT map)
{
	PMIXTRK	*trk;
	UINT	bit;

	map &= 0x0f;
	if (map == 0)
	{
		return;
	}
	sound_sync();
	trk = amd98r.trk;
	bit = 0x01;
	do
	{
		if ((map & bit) && (trk->data.sample))
		{
			trk->pcm = trk->data.sample;
			trk->remain = trk->data.samples;
			amd98r.hdr.playing |= bit;
		}
		trk++;
		bit <<= 1;
	} while (bit < 0x10);
}


/* ---- */

static void setamd98event(UINT32 cnt, NEVENTPOSITION absolute)
{
	if (cnt > 8)								/* 根拠なし*/
	{
		cnt *= pccore.multiple;
	}
	else
	{
		cnt = pccore.multiple << 16;
	}
	if (!(pccore.cpumode & CPUMODE_8MHZ))
	{
		cnt = cnt * 16 / 13;					// cnt * 2457600 / 1996800
	}
	nevent_set(NEVENT_MUSICGEN, cnt, amd98int, absolute);
}

/**
 * Interrupt
 * @param[in] item A pointer to an event-item structure
 */
void amd98int(NEVENTITEM item)
{
	PITCH	pitch;

	if (item->flag & NEVENT_SETEVENT)
	{
		pitch = pit.ch + 4;
		if ((pitch->ctrl & 0x0c) == 0x04)
		{
			/* レートジェネレータ */
			setamd98event(pitch->value, NEVENT_RELATIVE);
		}
	}
	pic_setirq(0x0d);
}


/* ---- I/O */

static void IOOUTCALL amd_od8(UINT port, REG8 dat)
{
	g_amd98.s.psg1reg = dat;
	(void)port;
}

static void IOOUTCALL amd_od9(UINT port, REG8 dat)
{
	g_amd98.s.psg2reg = dat;
	(void)port;
}

static void IOOUTCALL amd_oda(UINT port, REG8 dat)
{
	UINT	addr;

	addr = g_amd98.s.psg1reg;
	if (addr < 0x10)
	{
		psggen_setreg(&g_amd98.psg[0], addr, dat);
		keydisp_psg((UINT8 *)&g_amd98.psg[0].reg, addr);
	}
	(void)port;
}

static void IOOUTCALL amd_odb(UINT port, REG8 dat)
{
	UINT	addr;

	addr = g_amd98.s.psg2reg;
	if (addr < 0x0e)
	{
		psggen_setreg(&g_amd98.psg[1], addr, dat);
		keydisp_psg((UINT8 *)&g_amd98.psg[1].reg, addr);
	}
	else if (addr == 0x0f)
	{
		REG8 b;
		b = g_amd98.psg[1].reg.io2;
		if ((b & 1) > (dat & 1))
		{
			b &= 0xc2;
			if (b == 0x42)
			{
				g_amd98.s.psg3reg = g_amd98.psg[0].reg.io2;
			}
			else if (b == 0x40)
			{
				if (g_amd98.s.psg3reg < 0x0e)
				{
					psggen_setreg(&g_amd98.psg[2], g_amd98.s.psg3reg, g_amd98.psg[0].reg.io2);
					keydisp_psg((UINT8 *)&g_amd98.psg[2].reg, g_amd98.s.psg3reg);
				}
				else if (g_amd98.s.psg3reg == 0x0f)
				{
					amd98_rhythm(g_amd98.psg[0].reg.io2);
				}
			}
		}
		g_amd98.psg[1].reg.io2 = dat;
	}
	(void)port;
}

static void IOOUTCALL amd_odc(UINT port, REG8 dat)
{
	PITCH	pitch;

	pitch = pit.ch + 4;
	if (pit_setcount(pitch, dat))
	{
		return;
	}
	setamd98event(pitch->value, NEVENT_ABSOLUTE);
	(void)port;
}

static void IOOUTCALL amd_ode(UINT port, REG8 dat)
{
	pit_setflag(pit.ch + 4, dat);
	(void)port;
}

static REG8 IOINPCALL amd_ida(UINT port)
{
	UINT	addr;

	addr = g_amd98.s.psg1reg;
	if (addr < 0x0e)
	{
		return psggen_getreg(&g_amd98.psg[0], addr);
	}
	else if (addr == 0x0e)
	{
		return amd98_getjoy(1);
	}
	else if (addr == 0x0f)
	{
		return g_amd98.psg[0].reg.io2;
	}
	(void)port;
	return 0xff;
}

static REG8 IOINPCALL amd_idb(UINT port)
{
	UINT	addr;

	addr = g_amd98.s.psg2reg;
	if (addr < 0x0e)
	{
		return psggen_getreg(&g_amd98.psg[1], addr);
	}
	else if (addr == 0x0e)
	{
		return amd98_getjoy(2);
	}
	else if (addr == 0x0f)
	{
		return g_amd98.psg[1].reg.io2;
	}
	(void)port;
	return 0xff;
}

#if defined(TRACE)
static REG8 IOINPCALL amd_inp(UINT port)
{
	TRACEOUT(("amd inp - %.4x", port));
	return 0xff;
}
#endif

/* ---- Interface */

static void psgpanset(PSGGEN psg)
{
	psggen_setpan(psg, 0, 1);
	psggen_setpan(psg, 1, 0);
	psggen_setpan(psg, 2, 2);
}

/**
 * Reset
 * @param[in] pConfig A pointer to a configure structure
 */
void amd98_reset(const NP2CFG *pConfig)
{
	UINT i;

	memset(&g_amd98, 0, sizeof(g_amd98));

	for (i = 0; i < NELEMENTS(g_amd98.psg); i++)
	{
		psggen_reset(&g_amd98.psg[i]);
	}

	(void)pConfig;
}

/**
 * Bind
 */
void amd98_bind(void)
{
	UINT i;

	amd98_rhythmload();

	for (i = 0; i < NELEMENTS(g_amd98.psg); i++)
	{
		keydisp_bindpsg((UINT8 *)&g_amd98.psg[i].reg, 3993600);
		psgpanset(&g_amd98.psg[i]);
		psggen_restore(&g_amd98.psg[i]);
		sound_streamregist(&g_amd98.psg[i], (SOUNDCB)psggen_getpcm);
	}
	sound_streamregist(&amd98r, (SOUNDCB)pcmmix_getpcm);
	iocore_attachout(0xd8, amd_od8);
	iocore_attachout(0xd9, amd_od9);
	iocore_attachout(0xda, amd_oda);
	iocore_attachout(0xdb, amd_odb);
	iocore_attachout(0xdc, amd_odc);
	iocore_attachout(0xde, amd_ode);

	iocore_attachinp(0xda, amd_ida);
	iocore_attachinp(0xdb, amd_idb);
#if defined(TRACE)
	iocore_attachinp(0xd8, amd_inp);
	iocore_attachinp(0xd9, amd_inp);
	iocore_attachinp(0xdc, amd_inp);
	iocore_attachinp(0xde, amd_inp);
#endif
}

/**
 * state save
 */
int amd98_sfsave(STFLAGH sfh, const SFENTRY *tbl)
{
	int ret;
	UINT i;

	/* register */
	ret = statflag_write(sfh, &g_amd98.s, sizeof(g_amd98.s));

	/* psg */
	for (i = 0; i < NELEMENTS(g_amd98.psg); i++)
	{
		ret |= statflag_write(sfh, &g_amd98.psg[i].reg, sizeof(g_amd98.psg[i].reg));
	}
	return ret;
}

/**
 * state load
 */
int amd98_sfload(STFLAGH sfh, const SFENTRY *tbl)
{
	int ret;
	UINT i;

	/* register */
	ret = statflag_read(sfh, &g_amd98.s, sizeof(g_amd98.s));

	/* psg */
	for (i = 0; i < NELEMENTS(g_amd98.psg); i++)
	{
		ret |= statflag_read(sfh, &g_amd98.psg[i].reg, sizeof(g_amd98.psg[i].reg));
	}
	return ret;
}
