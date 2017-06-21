/**
 * @file	tms3631c.c
 * @brief	Implementation of the TMS3631
 */

#include "compiler.h"
#include "tms3631.h"
#include <math.h>

	TMS3631CFG	tms3631cfg;

void tms3631_initialize(UINT rate)
{
	UINT i, j;
	double f;

	ZeroMemory(&tms3631cfg, sizeof(tms3631cfg));

	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 12; j++)
		{
			f = 440.0 * pow(2.0, (i - 3.0) + ((j - 9.0) / 12.0));
			f = f * TMS3631_MUL * (1 << (TMS3631_FREQ + 1)) / rate;
			tms3631cfg.freqtbl[(i * 16) + j + 1] = (UINT32)floor(f + 0.5);
		}
	}
}

void tms3631_setvol(const UINT8 *vol)
{
	UINT i;
	UINT j;
	SINT32 data;

	tms3631cfg.left = (vol[0] & 15) << 5;
	tms3631cfg.right = (vol[1] & 15) << 5;
	vol += 2;
	for (i = 0; i < 16; i++)
	{
		data = 0;
		for (j = 0; j < 4; j++)
		{
			data += (vol[j] & 15) * ((i & (1 << j)) ? 1 : -1);
		}
		tms3631cfg.feet[i] = data << 5;
	}
}


// ----

void tms3631_reset(TMS3631 tms)
{
	memset(tms, 0, sizeof(*tms));
}

void tms3631_setkey(TMS3631 tms, REG8 ch, REG8 key)
{
	tms->ch[ch & 7].freq = tms3631cfg.freqtbl[key & 0x3f];
}

void tms3631_setenable(TMS3631 tms, REG8 enable)
{
	tms->enable = enable;
}
