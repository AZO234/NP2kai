
#pragma once

#ifndef MEMCALL
#define	MEMCALL
#endif

#if defined(MEMORDERTYPE) && (MEMORDERTYPE != 0)
#error : MEMORDERTYPE != 0
#endif


// 000000-0fffff ÉÅÉCÉìÉÅÉÇÉä
// 100000-10ffef HMA
// 110000-193fff FONT-ROM/RAM
// 1a8000-1bffff VRAM1
// 1c0000-1c7fff ITF-ROM BAK
// 1c8000-1dffff EPSON RAM
// 1e0000-1e7fff VRAM1
// 1f8000-1fffff ITF-ROM

#define	USE_HIMEM	0x110000

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
	ITF_ADRS	= 0x1f8000
};

#define	VRAMADDRMASKEX(a)	((a) & (VRAM_STEP | 0x7fff))


#ifdef __cplusplus
extern "C" {
#endif

extern	UINT8	mem[0x200000];

void MEMCALL i286_memorymap(UINT type);
void MEMCALL i286_vram_dispatch(UINT operate);

UINT8 MEMCALL i286_memoryread(UINT32 address);
UINT16 MEMCALL i286_memoryread_w(UINT32 address);
void MEMCALL i286_memorywrite(UINT32 address, UINT8 value);
void MEMCALL i286_memorywrite_w(UINT32 address, UINT16 value);

UINT8 MEMCALL i286_membyte_read(UINT seg, UINT off);
UINT16 MEMCALL i286_memword_read(UINT seg, UINT off);
void MEMCALL i286_membyte_write(UINT seg, UINT off, UINT8 value);
void MEMCALL i286_memword_write(UINT seg, UINT off, UINT16 value);

void MEMCALL i286_memstr_read(UINT seg, UINT off, void *dat, UINT leng);
void MEMCALL i286_memstr_write(UINT seg, UINT off,
											const void *dat, UINT leng);

void MEMCALL i286_memx_read(UINT32 address, void *dat, UINT leng);
void MEMCALL i286_memx_write(UINT32 address, const void *dat, UINT leng);

#ifdef __cplusplus
}
#endif


// ---- Memory map

#define	MEMM_ARCH(t)		i286_memorymap(t)
#define	MEMM_VRAM(o)		i286_vram_dispatch(o)


// ---- Physical Space (DMA)

#define	MEMP_READ8(addr)					\
			i286_memoryread((addr))
#define	MEMP_WRITE8(addr, dat)				\
			i286_memorywrite((addr), (dat))


// ---- Logical Space (BIOS)

#define	MEML_READ8(addr)					\
			i286_memoryread((addr))
#define	MEML_READ16(addr)					\
			i286_memoryread_w((addr))
#define	MEML_WRITE8(addr, dat)				\
			i286_memorywrite((addr), (dat))
#define	MEML_WRITE16(addr, dat)				\
			i286_memorywrite_w((addr), (dat))
#define MEML_READS(addr, dat, leng)			\
			i286_memx_read((addr), (dat), (leng))
#define MEML_WRITES(addr, dat, leng)		\
			i286_memx_write((addr), (dat), (leng))

#define	MEMR_READ8(seg, off)				\
			i286_membyte_read((seg), (off))
#define	MEMR_READ16(seg, off)				\
			i286_memword_read((seg), (off))
#define	MEMR_WRITE8(seg, off, dat)			\
			i286_membyte_write((seg), (off), (dat))
#define	MEMR_WRITE16(seg, off, dat)			\
			i286_memword_write((seg), (off), (dat))
#define MEMR_READS(seg, off, dat, leng)		\
			i286_memstr_read((seg), (off), (dat), (leng))
#define MEMR_WRITES(seg, off, dat, leng)	\
			i286_memstr_write((seg), (off), (dat), (leng))

