#include "compiler.h"

#include "drawmng.h"


void
drawmng_make16mask(PAL16MASK *pal16, UINT32 bmask, UINT32 rmask, UINT32 gmask)
{
	UINT8 sft;

	if (pal16 == NULL)
		return;

	sft = 0;
	while ((!(bmask & 0x80)) && (sft < 32)) {
		bmask <<= 1;
		sft++;
	}
	pal16->mask.p.b = (UINT8)bmask;
	pal16->r16b = sft;

	sft = 0;
	while ((rmask & 0xffffff00) && (sft < 32)) {
		rmask >>= 1;
		sft++;
	}
	pal16->mask.p.r = (UINT8)rmask;
	pal16->l16r = sft;

	sft = 0;
	while ((gmask & 0xffffff00) && (sft < 32)) {
		gmask >>= 1;
		sft++;
	}
	pal16->mask.p.g = (UINT8)gmask;
	pal16->l16g = sft;
}

RGB16
drawmng_makepal16(PAL16MASK *pal16, RGB32 pal32)
{
	RGB32 pal;

	pal.d = pal32.d & pal16->mask.d;
	return (RGB16)((pal.p.g << pal16->l16g) + (pal.p.r << pal16->l16r) + (pal.p.b >> pal16->r16b));
}
