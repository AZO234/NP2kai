
#pragma once

#ifndef MEMCALL
#define	MEMCALL
#endif


// 000000-0fffff ÉÅÉCÉìÉÅÉÇÉä
// 100000-10ffef HMA
// 10fff0-10ffff HIMEM
// 110000-193fff FONT-ROM/RAM
// 1a8000-1bffff VRAM1
// 1c0000-1c7fff ITF-ROM BAK
// 1c8000-1dffff EPSON RAM
// 1e0000-1e7fff VRAM1
// 1f8000-1fffff ITF-ROM

#define	USE_HIMEM		0x110000

enum {
	VRAM_STEP	= 0x100000,
	VRAM_B		= 0x0a8000,
	VRAM_R		= 0x0b0000,
	VRAM_G		= 0x0b8000,
	VRAM_E		= 0x0e0000,

	VRAM0_B		= VRAM_B,
	VRAM0_R		= VRAM_R,
	VRAM0_G		= VRAM_G,
	VRAM0_E		= VRAM_E,
	VRAM1_B		= (VRAM_STEP + VRAM_B),
	VRAM1_R		= (VRAM_STEP + VRAM_R),
	VRAM1_G		= (VRAM_STEP + VRAM_G),
	VRAM1_E		= (VRAM_STEP + VRAM_E),

	FONT_ADRS	= 0x110000,
	ITF_ADRS	= (VRAM_STEP + 0xf8000)
};

#define	VRAMADDRMASKEX(a)	((a) & (VRAM_STEP | 0x7fff))


#ifdef __cplusplus
extern "C" {
#endif

extern	UINT8	mem[0x200000];

void MEMCALL memm_arch(UINT type);
void MEMCALL memm_vram(UINT operate);

REG8 MEMCALL memp_read8(UINT32 address);
REG16 MEMCALL memp_read16(UINT32 address);
UINT32 MEMCALL memp_read32(UINT32 address);
void MEMCALL memp_write8(UINT32 address, REG8 value);
void MEMCALL memp_write16(UINT32 address, REG16 value);
void MEMCALL memp_write32(UINT32 address, UINT32 value);

void MEMCALL memp_reads(UINT32 address, void *dat, UINT leng);
void MEMCALL memp_writes(UINT32 address, const void *dat, UINT leng);

REG8 MEMCALL memr_read8(UINT seg, UINT off);
REG16 MEMCALL memr_read16(UINT seg, UINT off);
void MEMCALL memr_write8(UINT seg, UINT off, REG8 value);
void MEMCALL memr_write16(UINT seg, UINT off, REG16 value);
void MEMCALL memr_reads(UINT seg, UINT off, void *dat, UINT leng);
void MEMCALL memr_writes(UINT seg, UINT off, const void *dat, UINT leng);

#ifdef __cplusplus
}
#endif


// ---- Memory map

#define	MEMM_ARCH(t)		memm_arch(t)
#define	MEMM_VRAM(o)		memm_vram(o)


// ---- Physical Space (DMA)

#define	MEMP_READ8(addr)					\
			memp_read8((addr))
#define	MEMP_WRITE8(addr, dat)				\
			memp_write8((addr), (dat))


// ---- Logical Space (BIOS)

#define	MEML_READ8(addr)					\
			memp_read8((addr))
#define	MEML_READ16(addr)					\
			memp_read16((addr))
#define	MEML_WRITE8(addr, dat)				\
			memp_write8((addr), (dat))
#define	MEML_WRITE16(addr, dat)				\
			memp_write16((addr), (dat))
#define MEML_READS(addr, dat, leng)			\
			memp_reads((addr), (dat), (leng))
#define MEML_WRITES(addr, dat, leng)		\
			memp_writes((addr), (dat), (leng))

#define	MEMR_READ8(seg, off)				\
			memr_read8((seg), (off))
#define	MEMR_READ16(seg, off)				\
			memr_read16((seg), (off))
#define	MEMR_WRITE8(seg, off, dat)			\
			memr_write8((seg), (off), (dat))
#define	MEMR_WRITE16(seg, off, dat)			\
			memr_write16((seg), (off), (dat))
#define MEMR_READS(seg, off, dat, leng)		\
			memr_reads((seg), (off), (dat), (leng))
#define MEMR_WRITES(seg, off, dat, leng)	\
			memr_writes((seg), (off), (dat), (leng))

