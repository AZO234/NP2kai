#include	"compiler.h"

#ifndef NP2_MEMORY_ASM

#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"memtram.h"
#include	"memvram.h"
#include	"memegc.h"
#if defined(SUPPORT_PC9821)
#include	"memvga.h"
#endif
#include	"memems.h"
#include	"memepp.h"
#include	"vram.h"
#include	"font/font.h"


	UINT8	mem[0x200000];


typedef void (MEMCALL * MEM8WRITE)(UINT32 address, REG8 value);
typedef REG8 (MEMCALL * MEM8READ)(UINT32 address);
typedef void (MEMCALL * MEM16WRITE)(UINT32 address, REG16 value);
typedef REG16 (MEMCALL * MEM16READ)(UINT32 address);


// ---- MAIN

static REG8 MEMCALL memmain_rd8(UINT32 address) {

	return(mem[address]);
}

static REG16 MEMCALL memmain_rd16(UINT32 address) {

const UINT8	*ptr;

	ptr = mem + address;
	return(LOADINTELWORD(ptr));
}

static void MEMCALL memmain_wr8(UINT32 address, REG8 value) {

	mem[address] = (UINT8)value;
}

static void MEMCALL memmain_wr16(UINT32 address, REG16 value) {

	UINT8	*ptr;

	ptr = mem + address;
	STOREINTELWORD(ptr, value);
}


// ---- N/C

static REG8 MEMCALL memnc_rd8(UINT32 address) {

	(void)address;
	return(0xff);
}

static REG16 MEMCALL memnc_rd16(UINT32 address) {

	(void)address;
	return(0xffff);
}

static void MEMCALL memnc_wr8(UINT32 address, REG8 value) {

	(void)address;
	(void)value;
}

static void MEMCALL memnc_wr16(UINT32 address, REG16 value) {

	(void)address;
	(void)value;
}


// ---- memory 000000-0ffffff + 64KB

typedef struct {
	MEM8READ	rd8[0x22];
	MEM8WRITE	wr8[0x22];
	MEM16READ	rd16[0x22];
	MEM16WRITE	wr16[0x22];
} MEMFN0;

typedef struct {
	MEM8READ	brd8;		// E8000-F7FFF byte read
	MEM8READ	ird8;		// F8000-FFFFF byte read
	MEM8WRITE	bwr8;		// E8000-FFFFF byte write
	MEM16READ	brd16;		// E8000-F7FFF word read
	MEM16READ	ird16;		// F8000-FFFFF word read
	MEM16WRITE	bwr16;		// F8000-FFFFF word write
} MMAPTBL;

typedef struct {
	MEM8READ	rd8;
	MEM8WRITE	wr8;
	MEM16READ	rd16;
	MEM16WRITE	wr16;
} VACCTBL;

static MEMFN0 memfn0 = {
	   {memmain_rd8,	memmain_rd8,	memmain_rd8,	memmain_rd8,	// 00
		memmain_rd8,	memmain_rd8,	memmain_rd8,	memmain_rd8,	// 20
		memmain_rd8,	memmain_rd8,	memmain_rd8,	memmain_rd8,	// 40
		memmain_rd8,	memmain_rd8,	memmain_rd8,	memmain_rd8,	// 60
		memmain_rd8,	memmain_rd8,	memmain_rd8,	memmain_rd8,	// 80
		memtram_rd8,	memvram0_rd8,	memvram0_rd8,	memvram0_rd8,	// a0
		memems_rd8,		memems_rd8,		memmain_rd8,	memmain_rd8,	// c0
		memvram0_rd8,	memmain_rd8,	memmain_rd8,	memf800_rd8,	// e0
		memmain_rd8,	memmain_rd8},

	   {memmain_wr8,	memmain_wr8,	memmain_wr8,	memmain_wr8,	// 00
		memmain_wr8,	memmain_wr8,	memmain_wr8,	memmain_wr8,	// 20
		memmain_wr8,	memmain_wr8,	memmain_wr8,	memmain_wr8,	// 40
		memmain_wr8,	memmain_wr8,	memmain_wr8,	memmain_wr8,	// 60
		memmain_wr8,	memmain_wr8,	memmain_wr8,	memmain_wr8,	// 80
		memtram_wr8,	memvram0_wr8,	memvram0_wr8,	memvram0_wr8,	// a0
		memems_wr8,		memems_wr8,		memd000_wr8,	memd000_wr8,	// c0
		memvram0_wr8,	memnc_wr8,		memnc_wr8,		memnc_wr8,		// e0
		memmain_wr8,	memmain_wr8},

	   {memmain_rd16,	memmain_rd16,	memmain_rd16,	memmain_rd16,	// 00
		memmain_rd16,	memmain_rd16,	memmain_rd16,	memmain_rd16,	// 20
		memmain_rd16,	memmain_rd16,	memmain_rd16,	memmain_rd16,	// 40
		memmain_rd16,	memmain_rd16,	memmain_rd16,	memmain_rd16,	// 60
		memmain_rd16,	memmain_rd16,	memmain_rd16,	memmain_rd16,	// 80
		memtram_rd16,	memvram0_rd16,	memvram0_rd16,	memvram0_rd16,	// a0
		memems_rd16,	memems_rd16,	memmain_rd16,	memmain_rd16,	// c0
		memvram0_rd16,	memmain_rd16,	memmain_rd16,	memf800_rd16,	// e0
		memmain_rd16,	memmain_rd16},

	   {memmain_wr16,	memmain_wr16,	memmain_wr16,	memmain_wr16,	// 00
		memmain_wr16,	memmain_wr16,	memmain_wr16,	memmain_wr16,	// 20
		memmain_wr16,	memmain_wr16,	memmain_wr16,	memmain_wr16,	// 40
		memmain_wr16,	memmain_wr16,	memmain_wr16,	memmain_wr16,	// 60
		memmain_wr16,	memmain_wr16,	memmain_wr16,	memmain_wr16,	// 80
		memtram_wr16,	memvram0_wr16,	memvram0_wr16,	memvram0_wr16,	// a0
		memems_wr16,	memems_wr16,	memd000_wr16,	memd000_wr16,	// c0
		memvram0_wr16,	memnc_wr16,		memnc_wr16,		memnc_wr16,		// e0
		memmain_wr16,	memmain_wr16}};

static const MMAPTBL mmaptbl[2] = {
		   {memmain_rd8,	memf800_rd8,	memnc_wr8,
			memmain_rd16,	memf800_rd16,	memnc_wr16},
		   {memf800_rd8,	memf800_rd8,	memepson_wr8,
			memf800_rd16,	memf800_rd16,	memepson_wr16}};

static const VACCTBL vacctbl[0x10] = {
		{memvram0_rd8,	memvram0_wr8,	memvram0_rd16,	memvram0_wr16},	// 00
		{memvram1_rd8,	memvram1_wr8,	memvram1_rd16,	memvram1_wr16},
		{memvram0_rd8,	memvram0_wr8,	memvram0_rd16,	memvram0_wr16},
		{memvram1_rd8,	memvram1_wr8,	memvram1_rd16,	memvram1_wr16},
		{memvram0_rd8,	memvram0_wr8,	memvram0_rd16,	memvram0_wr16},	// 40
		{memvram1_rd8,	memvram1_wr8,	memvram1_rd16,	memvram1_wr16},
		{memvram0_rd8,	memvram0_wr8,	memvram0_rd16,	memvram0_wr16},
		{memvram1_rd8,	memvram1_wr8,	memvram1_rd16,	memvram1_wr16},
		{memtcr0_rd8,	memtdw0_wr8,	memtcr0_rd16,	memtdw0_wr16},	// 80
		{memtcr1_rd8,	memtdw1_wr8,	memtcr1_rd16,	memtdw1_wr16},
		{memegc_rd8,	memegc_wr8,		memegc_rd16,	memegc_wr16},
		{memegc_rd8,	memegc_wr8,		memegc_rd16,	memegc_wr16},
		{memvram0_rd8,	memrmw0_wr8,	memvram0_rd16,	memrmw0_wr16},	// c0
		{memvram1_rd8,	memrmw1_wr8,	memvram1_rd16,	memrmw1_wr16},
		{memegc_rd8,	memegc_wr8,		memegc_rd16,	memegc_wr16},
		{memegc_rd8,	memegc_wr8,		memegc_rd16,	memegc_wr16}};


void MEMCALL memm_arch(UINT type) {

const MMAPTBL	*mm;

	mm = mmaptbl + (type & 1);

	memfn0.rd8[0xe8000 >> 15] = mm->brd8;
	memfn0.rd8[0xf0000 >> 15] = mm->brd8;
	memfn0.rd8[0xf8000 >> 15] = mm->ird8;
	memfn0.wr8[0xe8000 >> 15] = mm->bwr8;
	memfn0.wr8[0xf0000 >> 15] = mm->bwr8;
	memfn0.wr8[0xf8000 >> 15] = mm->bwr8;

	memfn0.rd16[0xe8000 >> 15] = mm->brd16;
	memfn0.rd16[0xf0000 >> 15] = mm->brd16;
	memfn0.rd16[0xf8000 >> 15] = mm->ird16;
	memfn0.wr16[0xe8000 >> 15] = mm->bwr16;
	memfn0.wr16[0xf0000 >> 15] = mm->bwr16;
	memfn0.wr16[0xf8000 >> 15] = mm->bwr16;
}

void MEMCALL memm_vram(UINT func) {

const VACCTBL	*vacc;

#if defined(SUPPORT_PC9821)
	if (!(func & 0x20)) {
#endif	// defined(SUPPORT_PC9821)
		vacc = vacctbl + (func & 0x0f);

		memfn0.rd8[0xa8000 >> 15] = vacc->rd8;
		memfn0.rd8[0xb0000 >> 15] = vacc->rd8;
		memfn0.rd8[0xb8000 >> 15] = vacc->rd8;
		memfn0.rd8[0xe0000 >> 15] = vacc->rd8;

		memfn0.wr8[0xa8000 >> 15] = vacc->wr8;
		memfn0.wr8[0xb0000 >> 15] = vacc->wr8;
		memfn0.wr8[0xb8000 >> 15] = vacc->wr8;
		memfn0.wr8[0xe0000 >> 15] = vacc->wr8;

		memfn0.rd16[0xa8000 >> 15] = vacc->rd16;
		memfn0.rd16[0xb0000 >> 15] = vacc->rd16;
		memfn0.rd16[0xb8000 >> 15] = vacc->rd16;
		memfn0.rd16[0xe0000 >> 15] = vacc->rd16;

		memfn0.wr16[0xa8000 >> 15] = vacc->wr16;
		memfn0.wr16[0xb0000 >> 15] = vacc->wr16;
		memfn0.wr16[0xb8000 >> 15] = vacc->wr16;
		memfn0.wr16[0xe0000 >> 15] = vacc->wr16;

		if (!(func & (1 << VOPBIT_ANALOG))) {					// digital
			memfn0.rd8[0xe0000 >> 15] = memnc_rd8;
			memfn0.wr8[0xe0000 >> 15] = memnc_wr8;
			memfn0.rd16[0xe0000 >> 15] = memnc_rd16;
			memfn0.wr16[0xe0000 >> 15] = memnc_wr16;
		}
#if defined(SUPPORT_PC9821)
	}
	else {
		memfn0.rd8[0xa8000 >> 15] = memvga0_rd8;
		memfn0.rd8[0xb0000 >> 15] = memvga1_rd8;
		memfn0.rd8[0xb8000 >> 15] = memnc_rd8;
		memfn0.rd8[0xe0000 >> 15] = memvgaio_rd8;

		memfn0.wr8[0xa8000 >> 15] = memvga0_wr8;
		memfn0.wr8[0xb0000 >> 15] = memvga1_wr8;
		memfn0.wr8[0xb8000 >> 15] = memnc_wr8;
		memfn0.wr8[0xe0000 >> 15] = memvgaio_wr8;

		memfn0.rd16[0xa8000 >> 15] = memvga0_rd16;
		memfn0.rd16[0xb0000 >> 15] = memvga1_rd16;
		memfn0.rd16[0xb8000 >> 15] = memnc_rd16;
		memfn0.rd16[0xe0000 >> 15] = memvgaio_rd16;

		memfn0.wr16[0xa8000 >> 15] = memvga0_wr16;
		memfn0.wr16[0xb0000 >> 15] = memvga1_wr16;
		memfn0.wr16[0xb8000 >> 15] = memnc_wr16;
		memfn0.wr16[0xe0000 >> 15] = memvgaio_wr16;
	}
#endif	// defined(SUPPORT_PC9821)
}


// ---- memory f00000-fffffff

typedef struct {
	MEM8READ	rd8[8];
	MEM8WRITE	wr8[8];
	MEM16READ	rd16[8];
	MEM16WRITE	wr16[8];
} MEMFNF;


static REG8 MEMCALL memsys_rd8(UINT32 address) {

	address -= 0xf00000;
	return(memfn0.rd8[address >> 15](address));
}

static REG16 MEMCALL memsys_rd16(UINT32 address) {

	address -= 0xf00000;
	return(memfn0.rd16[address >> 15](address));
}

static void MEMCALL memsys_wr8(UINT32 address, REG8 value) {

	address -= 0xf00000;
	memfn0.wr8[address >> 15](address, value);
}

static void MEMCALL memsys_wr16(UINT32 address, REG16 value) {

	address -= 0xf00000;
	memfn0.wr16[address >> 15](address, value);
}

#if defined(SUPPORT_PC9821)
static const MEMFNF memfnf = {
	   {memvgaf_rd8,	memvgaf_rd8,	memvgaf_rd8,	memvgaf_rd8,
		memnc_rd8,		memsys_rd8,		memsys_rd8,		memsys_rd8},
	   {memvgaf_wr8,	memvgaf_wr8,	memvgaf_wr8,	memvgaf_wr8,
		memnc_wr8,		memsys_wr8,		memsys_wr8,		memsys_wr8},

	   {memvgaf_rd16,	memvgaf_rd16,	memvgaf_rd16,	memvgaf_rd16,
		memnc_rd16,		memsys_rd16,	memsys_rd16,	memsys_rd16},
	   {memvgaf_wr16,	memvgaf_wr16,	memvgaf_wr16,	memvgaf_wr16,
		memnc_wr16,		memsys_wr16,	memsys_wr16,	memsys_wr16}};
#else
static const MEMFNF memfnf = {
	   {memnc_rd8,		memnc_rd8,		memnc_rd8,		memnc_rd8,
		memnc_rd8,		memsys_rd8,		memsys_rd8,		memsys_rd8},
	   {memnc_wr8,		memnc_wr8,		memnc_wr8,		memnc_wr8,
		memnc_wr8,		memsys_wr8,		memsys_wr8,		memsys_wr8},

	   {memnc_rd16,		memnc_rd16,		memnc_rd16,		memnc_rd16,
		memnc_rd16,		memsys_rd16,	memsys_rd16,	memsys_rd16},
	   {memnc_wr16,		memnc_wr16,		memnc_wr16,		memnc_wr16,
		memnc_wr16,		memsys_wr16,	memsys_wr16,	memsys_wr16}};
#endif


// ----

REG8 MEMCALL memp_read8(UINT32 address) {

	if (address < I286_MEMREADMAX) {
		return(mem[address]);
	}
	else {
		address = address & CPU_ADRSMASK;
		if (address < USE_HIMEM) {
			return(memfn0.rd8[address >> 15](address));
		}
		else if (address < CPU_EXTLIMIT16) {
			return(CPU_EXTMEMBASE[address]);
		}
		else if (address < 0x00f00000) {
			return(0xff);
		}
		else if (address < 0x01000000) {
			return(memfnf.rd8[(address >> 17) & 7](address));
		}
#if defined(CPU_EXTLIMIT)
		else if (address < CPU_EXTLIMIT) {
			return(CPU_EXTMEMBASE[address]);
		}
#endif	// defined(CPU_EXTLIMIT)
#if defined(SUPPORT_PC9821)
		else if ((address >= 0xfff00000) && (address < 0xfff80000)) {
			return(memvgaf_rd8(address));
		}
#endif	// defined(SUPPORT_PC9821)
		else {
//			TRACEOUT(("out of mem (read8): %x", address));
			return(0xff);
		}
	}
}

REG16 MEMCALL memp_read16(UINT32 address) {

	REG16	ret;

	if (address < (I286_MEMREADMAX - 1)) {
		return(LOADINTELWORD(mem + address));
	}
	else if ((address + 1) & 0x7fff) {			// non 32kb boundary
		address = address & CPU_ADRSMASK;
		if (address < USE_HIMEM) {
			return(memfn0.rd16[address >> 15](address));
		}
		else if (address < CPU_EXTLIMIT16) {
			return(LOADINTELWORD(CPU_EXTMEMBASE + address));
		}
		else if (address < 0x00f00000) {
			return(0xffff);
		}
		else if (address < 0x01000000) {
			return(memfnf.rd16[(address >> 17) & 7](address));
		}
#if defined(CPU_EXTLIMIT)
		else if (address < CPU_EXTLIMIT) {
			return(LOADINTELWORD(CPU_EXTMEMBASE + address));
		}
#endif	// defined(CPU_EXTLIMIT)
#if defined(SUPPORT_PC9821)
		else if ((address >= 0xfff00000) && (address < 0xfff80000)) {
			return(memvgaf_rd16(address));
		}
#endif	// defined(SUPPORT_PC9821)
		else {
//			TRACEOUT(("out of mem (read16): %x", address));
			return(0xffff);
		}
	}
	else {
		ret = memp_read8(address + 0);
		ret += (REG16)(memp_read8(address + 1) << 8);
		return(ret);
	}
}

UINT32 MEMCALL memp_read32(UINT32 address) {

	UINT32	pos;
	UINT32	ret;

	if (address < (I286_MEMREADMAX - 3)) {
		return(LOADINTELDWORD(mem + address));
	}
	else if (address >= USE_HIMEM) {
		pos = address & CPU_ADRSMASK;
		if ((pos >= USE_HIMEM) && ((pos + 3) < CPU_EXTLIMIT16)) {
			return(LOADINTELDWORD(CPU_EXTMEMBASE + pos));
		}
	}
	if (!(address & 1)) {
		ret = memp_read16(address + 0);
		ret += (UINT32)memp_read16(address + 2) << 16;
	}
	else {
		ret = memp_read8(address + 0);
		ret += (UINT32)memp_read16(address + 1) << 8;
		ret += (UINT32)memp_read8(address + 3) << 24;
	}
	return(ret);
}

void MEMCALL memp_write8(UINT32 address, REG8 value) {

	if (address < I286_MEMWRITEMAX) {
		mem[address] = (UINT8)value;
	}
	else {
		address = address & CPU_ADRSMASK;
		if (address < USE_HIMEM) {
			memfn0.wr8[address >> 15](address, value);
		}
		else if (address < CPU_EXTLIMIT16) {
			CPU_EXTMEMBASE[address] = (UINT8)value;
		}
		else if (address < 0x00f00000) {
		}
		else if (address < 0x01000000) {
			memfnf.wr8[(address >> 17) & 7](address, value);
		}
#if defined(CPU_EXTLIMIT)
		else if (address < CPU_EXTLIMIT) {
			CPU_EXTMEMBASE[address] = (UINT8)value;
		}
#endif	// defined(CPU_EXTLIMIT)
#if defined(SUPPORT_PC9821)
		else if ((address >= 0xfff00000) && (address < 0xfff80000)) {
			memvgaf_wr8(address, value);
		}
#endif	// defined(SUPPORT_PC9821)
		else {
//			TRACEOUT(("out of mem (write8): %x", address));
		}
	}
}

void MEMCALL memp_write16(UINT32 address, REG16 value) {

	if (address < (I286_MEMWRITEMAX - 1)) {
		STOREINTELWORD(mem + address, value);
	}
	else if ((address + 1) & 0x7fff) {			// non 32kb boundary
		address = address & CPU_ADRSMASK;
		if (address < USE_HIMEM) {
			memfn0.wr16[address >> 15](address, value);
		}
		else if (address < CPU_EXTLIMIT16) {
			STOREINTELWORD(CPU_EXTMEMBASE + address, value);
		}
		else if (address < 0x00f00000) {
		}
		else if (address < 0x01000000) {
			memfnf.wr16[(address >> 17) & 7](address, value);
		}
#if defined(CPU_EXTLIMIT)
		else if (address < CPU_EXTLIMIT) {
			STOREINTELWORD(CPU_EXTMEMBASE + address, value);
		}
#endif	// defined(CPU_EXTLIMIT)
#if defined(SUPPORT_PC9821)
		else if ((address >= 0xfff00000) && (address < 0xfff80000)) {
			memvgaf_wr16(address, value);
		}
#endif	// defined(SUPPORT_PC9821)
		else {
//			TRACEOUT(("out of mem (write16): %x", address));
		}
	}
	else {
		memp_write8(address + 0, (UINT8)value);
		memp_write8(address + 1, (UINT8)(value >> 8));
	}
}

void MEMCALL memp_write32(UINT32 address, UINT32 value) {

	UINT32	pos;

	if (address < (I286_MEMWRITEMAX - 3)) {
		STOREINTELDWORD(mem + address, value);
		return;
	}
	else if (address >= USE_HIMEM) {
		pos = address & CPU_ADRSMASK;
		if ((pos >= USE_HIMEM) && ((pos + 3) < CPU_EXTLIMIT16)) {
			STOREINTELDWORD(CPU_EXTMEMBASE + pos, value);
			return;
		}
	}
	if (!(address & 1)) {
		memp_write16(address + 0, (UINT16)value);
		memp_write16(address + 2, (UINT16)(value >> 16));
	}
	else {
		memp_write8(address + 0, (UINT8)value);
		memp_write16(address + 1, (UINT16)(value >> 8));
		memp_write8(address + 3, (UINT8)(value >> 24));
	}
}


void MEMCALL memp_reads(UINT32 address, void *dat, UINT leng) {

	if ((address + leng) < I286_MEMREADMAX) {
		CopyMemory(dat, mem + address, leng);
	}
	else {
		UINT8 *out = (UINT8 *)dat;
		if (address < I286_MEMREADMAX) {
			CopyMemory(out, mem + address, I286_MEMREADMAX - address);
			out += I286_MEMREADMAX - address;
			leng -= I286_MEMREADMAX - address;
			address = I286_MEMREADMAX;
		}
		while(leng--) {
			*out++ = memp_read8(address++);
		}
	}
}

void MEMCALL memp_writes(UINT32 address, const void *dat, UINT leng) {

const UINT8	*out;

	if ((address + leng) < I286_MEMWRITEMAX) {
		CopyMemory(mem + address, dat, leng);
	}
	else {
		out = (UINT8 *)dat;
		if (address < I286_MEMWRITEMAX) {
			CopyMemory(mem + address, out, I286_MEMWRITEMAX - address);
			out += I286_MEMWRITEMAX - address;
			leng -= I286_MEMWRITEMAX - address;
			address = I286_MEMWRITEMAX;
		}
		while(leng--) {
			memp_write8(address++, *out++);
		}
	}
}


// ---- Logical Space (BIOS)

REG8 MEMCALL memr_read8(UINT seg, UINT off) {

	UINT32	address;

	address = (seg << 4) + LOW16(off);
	if (address < I286_MEMREADMAX) {
		return(mem[address]);
	}
	else {
		return(memp_read8(address));
	}
}

REG16 MEMCALL memr_read16(UINT seg, UINT off) {

	UINT32	address;

	address = (seg << 4) + LOW16(off);
	if (address < (I286_MEMREADMAX - 1)) {
		return(LOADINTELWORD(mem + address));
	}
	else {
		return(memp_read16(address));
	}
}

void MEMCALL memr_write8(UINT seg, UINT off, REG8 value) {

	UINT32	address;

	address = (seg << 4) + LOW16(off);
	if (address < I286_MEMWRITEMAX) {
		mem[address] = (UINT8)value;
	}
	else {
		memp_write8(address, value);
	}
}

void MEMCALL memr_write16(UINT seg, UINT off, REG16 value) {

	UINT32	address;

	address = (seg << 4) + LOW16(off);
	if (address < (I286_MEMWRITEMAX - 1)) {
		STOREINTELWORD(mem + address, value);
	}
	else {
		memp_write16(address, value);
	}
}

void MEMCALL memr_reads(UINT seg, UINT off, void *dat, UINT leng) {

	UINT8	*out;
	UINT32	adrs;
	UINT	size;

	out = (UINT8 *)dat;
	adrs = seg << 4;
	off = LOW16(off);
	if ((I286_MEMREADMAX >= 0x10000) &&
		(adrs < (I286_MEMREADMAX - 0x10000))) {
		if (leng) {
			size = 0x10000 - off;
			if (size >= leng) {
				CopyMemory(out, mem + adrs + off, leng);
				return;
			}
			CopyMemory(out, mem + adrs + off, size);
			out += size;
			leng -= size;
		}
		while(leng >= 0x10000) {
			CopyMemory(out, mem + adrs, 0x10000);
			out += 0x10000;
			leng -= 0x10000;
		}
		if (leng) {
			CopyMemory(out, mem + adrs, leng);
		}
	}
	else {
		while(leng--) {
			*out++ = memp_read8(adrs + off);
			off = LOW16(off + 1);
		}
	}
}

void MEMCALL memr_writes(UINT seg, UINT off, const void *dat, UINT leng) {

	UINT8	*out;
	UINT32	adrs;
	UINT	size;

	out = (UINT8 *)dat;
	adrs = seg << 4;
	off = LOW16(off);
	if ((I286_MEMWRITEMAX >= 0x10000) &&
		(adrs < (I286_MEMWRITEMAX - 0x10000))) {
		if (leng) {
			size = 0x10000 - off;
			if (size >= leng) {
				CopyMemory(mem + adrs + off, out, leng);
				return;
			}
			CopyMemory(mem + adrs + off, out, size);
			out += size;
			leng -= size;
		}
		while(leng >= 0x10000) {
			CopyMemory(mem + adrs, out, 0x10000);
			out += 0x10000;
			leng -= 0x10000;
		}
		if (leng) {
			CopyMemory(mem + adrs, out, leng);
		}
	}
	else {
		while(leng--) {
			memp_write8(adrs + off, *out++);
			off = LOW16(off + 1);
		}
	}
}

#endif

