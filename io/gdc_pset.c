#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"memegc.h"
#include	"gdc_sub.h"
#include	"gdc_pset.h"
#include	"vram.h"


static void MEMCALL _nop(GDCPSET pset, UINT addr, UINT bit) {

	(void)pset;
	(void)addr;
	(void)bit;
}

static void MEMCALL _replace0(GDCPSET pset, UINT addr, UINT bit) {

	vramupdate[addr] |= pset->update.b[0];
	pset->base.ptr[addr] &= ~(0x80 >> bit);
}

static void MEMCALL _replace1(GDCPSET pset, UINT addr, UINT bit) {

	vramupdate[addr] |= pset->update.b[0];
	pset->base.ptr[addr] |= (0x80 >> bit);
}

static void MEMCALL _complemnt(GDCPSET pset, UINT addr, UINT bit) {

	vramupdate[addr] |= pset->update.b[0];
	pset->base.ptr[addr] ^= (0x80 >> bit);
}

static void MEMCALL _clear(GDCPSET pset, UINT addr, UINT bit) {

	vramupdate[addr] |= pset->update.b[0];
	pset->base.ptr[addr] &= ~(0x80 >> bit);
}

static void MEMCALL _set(GDCPSET pset, UINT addr, UINT bit) {

	vramupdate[addr] |= pset->update.b[0];
	pset->base.ptr[addr] |= (0x80 >> bit);
}


// ---- grcg

static void MEMCALL withtdw(GDCPSET pset, UINT addr, UINT bit) {

	UINT8	*ptr;

	addr &= ~1;
	*(UINT16 *)(vramupdate + addr) |= pset->update.w;
	ptr = pset->base.ptr + addr;
	*(UINT16 *)(ptr + VRAM_B) = grcg.tile[0].w;
	*(UINT16 *)(ptr + VRAM_R) = grcg.tile[1].w;
	*(UINT16 *)(ptr + VRAM_G) = grcg.tile[2].w;
	*(UINT16 *)(ptr + VRAM_E) = grcg.tile[3].w;
	(void)bit;
}

static void MEMCALL withrmw(GDCPSET pset, UINT addr, UINT bit) {

	UINT8	*ptr;
	UINT8	data;
	UINT8	mask;

	vramupdate[addr] |= pset->update.b[0];
	ptr = pset->base.ptr + addr;
	data = (0x80 >> bit);
	mask = ~data;
	ptr[VRAM_B] &= mask;
	ptr[VRAM_B] |= data & grcg.tile[0].b[0];
	ptr[VRAM_R] &= mask;
	ptr[VRAM_R] |= data & grcg.tile[1].b[0];
	ptr[VRAM_G] &= mask;
	ptr[VRAM_G] |= data & grcg.tile[2].b[0];
	ptr[VRAM_E] &= mask;
	ptr[VRAM_E] |= data & grcg.tile[3].b[0];
}


// ---- egc

static void MEMCALL withegc(GDCPSET pset, UINT addr, UINT bit) {

	REG16	data;

	data = (0x80 >> bit);
	if (addr & 1) {
		addr &= ~1;
		data <<= 8;
	}
	egc_writeword(pset->base.addr + addr, data);
}


static const GDCPFN psettbl[4][2] = {
				{_replace0,	_replace1},
				{_nop,		_complemnt},
				{_nop,		_clear},
				{_nop,		_set}};


// ----

void MEMCALL gdcpset_prepare(GDCPSET pset, UINT32 csrw, REG16 pat, REG8 op) {

	UINT8	*base;
	UINT8	update;

	if (vramop.operate & (1 << VOPBIT_EGC)) {
		pset->func[0] = _nop;
		pset->func[1] = withegc;
		pset->base.addr = gdcplaneseg[(csrw >> 14) & 3];
	}
	else {
		base = mem;
		if (!gdcs.access) {
			update = 1;
		}
		else {
			base += VRAM_STEP;
			update = 2;
		}
		op &= 3;
		if (!(grcg.gdcwithgrcg & 0x8)) {
			pset->func[0] = psettbl[op][0];
			pset->func[1] = psettbl[op][1];
			pset->base.ptr = base + gdcplaneseg[(csrw >> 14) & 3];
		}
		else {
			pset->func[0] = _nop;
			pset->func[1] = (grcg.gdcwithgrcg & 0x4)?withrmw:withtdw;
			pset->base.ptr = base;
		}
		gdcs.grphdisp |= update;
		pset->update.b[0] = update;
		pset->update.b[1] = update;
	}
	pset->pattern = pat;
	pset->x = (UINT16)((((csrw & 0x3fff) % 40) << 4) + ((csrw >> 20) & 0x0f));
	pset->y = (UINT16)((csrw & 0x3fff) / 40);
	pset->dots = 0;
}

void MEMCALL gdcpset(GDCPSET pset, REG16 x, REG16 y) {

	UINT	dot;

	dot = pset->pattern & 1;
	pset->pattern = (pset->pattern >> 1) + (dot << 15);
	pset->dots++;
	x = LOW16(x);
	y = LOW16(y);
	if (y > 409) {
		return;
	}
	else if (y == 409) {
		if (x >= 384) {
			return;
		}
	}
	else {
		if (x >= 640) {
			return;
		}
	}
	(*pset->func[dot])(pset, (y * 80) + (x >> 3), x & 7);
}

