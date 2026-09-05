/**
 * @file	ymzadpcmc.c
 * @brief	Implementation of the YMZ ADPCM
 */

#include "compiler.h"
#include "ymzadpcm.h"
#include "opngen.h"

	ADPCMCFG	ymzadpcmcfg;

#if 0
	static void trace_fmt_ex2(const char* fmt, ...)
	{
		char stmp[2048];
		va_list ap;
		va_start(ap, fmt);
		vsprintf(stmp, fmt, ap);
		strcat(stmp, "\n");
		va_end(ap);
		OutputDebugStringA(stmp);
	}
#define	TRACEOUT2(s)	trace_fmt_ex2 s
#else
#define	TRACEOUT2(s)	(void)s
#endif	/* 1 */

void ymzadpcm_initialize(UINT rate) {

	ymzadpcmcfg.rate = rate;
}

void ymzadpcm_setvol(UINT vol) {

	ymzadpcmcfg.vol = vol;
}

static void ymzadpcm_cpufifo_reset(ADPCM ad) {

	ad->cfifo.cpufiford = 0;
	ad->cfifo.cpufifowr = 0;
	ad->cfifo.cpufifocount = 0;
	ad->cpufifolow = 0;
	ad->cfifo.cpufifocur = 0;
}

static void ymzadpcm_cpuwrite(ADPCM ad, REG8 data) {

	if (ad->cfifo.cpufifocount < ADPCM_CPUFIFO_SIZE) {
		ad->cfifo.cpufifo[ad->cfifo.cpufifowr & ADPCM_CPUFIFO_MASK] = data;
		ad->cfifo.cpufifowr++;
		ad->cfifo.cpufifocount++;
	}
}

void ymzadpcm_reset(ADPCM ad) {

	memset(ad, 0, sizeof(*ad));
	ad->mask = 0;					// (UINT8)~0x1c;
	ad->delta = 127;
	STOREINTELWORD(ad->reg.stop, 0x0002);
	STOREINTELWORD(ad->reg.limit, 0xffff);
	ad->stop = 0x000060;
	ad->limit = 0x200000;
	ymzadpcm_update(ad);
}

void ymzadpcm_update(ADPCM ad) {

	UINT32	addr;

	if (ymzadpcmcfg.rate) {
		ad->base = ADTIMING * (OPNA_CLOCK / 72) / ymzadpcmcfg.rate;
	}
	addr = LOADINTELWORD(ad->reg.delta);
	addr = (addr * ad->base) >> 16;
	if (addr < 0x100) {
		addr = 0x100;
	}
	ad->step = addr;
	ad->pertim = (1 << (ADTIMING_BIT * 2)) / addr;
	ad->level = (ad->reg.level * ymzadpcmcfg.vol) >> 4;
}

void ymzadpcm_setreg(ADPCM ad, UINT reg, REG8 value) {

	UINT32	addr;

	sound_sync();
	((UINT8 *)(&ad->reg))[reg] = value;
	switch(reg) {
		case 0x00:								// control1
			if ((value & 0x80) && (!ad->play)) {
				ad->play = 0x20;
				ad->pos = ad->start;
				ad->cpustream = ((value & 0xe0) == 0x80);
				if (!ad->cpustream) {
					ad->pos = ad->start;
				}
				else {
					ymzadpcm_cpufifo_reset(ad);
				}
				ad->samp = 0;
				ad->delta = 127;
				ad->remain = 0;
				ad->out0 = 0;
				ad->out1 = 0;
				ad->fb = 0;
			}
			if (value & 1) {
				ad->play = 0;
				ad->cpustream = 0;
				ymzadpcm_cpufifo_reset(ad);
			}
			break;

		case 0x01:								// control2
			break;

		case 0x02:	case 0x03:					// start address
			addr = (LOADINTELWORD(ad->reg.start)) << 5;
			ad->pos = addr;
			ad->start = addr;
			break;

		case 0x04:	case 0x05:					// stop address
			addr = (LOADINTELWORD(ad->reg.stop) + 1) << 5;
			ad->stop = addr;
			break;

		case 0x08:								// data
			if ((ad->reg.ctrl1 & 0x60) == 0x60) {
				ymzadpcm_datawrite(ad, value);
			}
			else if ((ad->reg.ctrl1 & 0xe0) == 0x80) {
				ymzadpcm_cpuwrite(ad, value);
				//TRACEOUT2(("%02x", value));
			}
			break;

		case 0x09:	case 0x0a:					// delta
			addr = LOADINTELWORD(ad->reg.delta);
			addr = (addr * ad->base) >> 16;
			if (addr < 0x100) {
				addr = 0x100;
			}
			ad->step = addr;
			ad->pertim = (1 << (ADTIMING_BIT * 2)) / addr;
			break;

		case 0x0b:								// level
			ad->level = (value * ymzadpcmcfg.vol) >> 4;
			break;

		case 0x0c:	case 0x0d:					// limit address
			addr = (LOADINTELWORD(ad->reg.limit) + 1) << 5;
			ad->limit = addr;
			break;
			
		case 0x0e:								// DAC data
			ad->status |= 0x04;	// EOS
			break;

		case 0x10:								// flag
			if (value & 0x80) {
				ad->status = 0;
			}
			else {
				ad->mask = ~(value & 0x1f);
			}
			break;
	}
}

REG8 ymzadpcm_status(ADPCM ad) {
	
	REG8 brdy;

	brdy = 8;
	if (ad->cpustream) {
		brdy = (ad->cfifo.cpufifocount < (ADPCM_CPUFIFO_SIZE - 16)) ? 8 : 0;
	}
	return(((ad->status | brdy) & ad->mask) | ad->play);
}

