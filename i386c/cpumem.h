#ifndef	NP2_I386C_CPUMEM_H__
#define	NP2_I386C_CPUMEM_H__

#ifdef NP2_MEMORY_ASM			// アセンブラ版は 必ずfastcallで
#undef	MEMCALL
#define	MEMCALL	FASTCALL
#endif

#if !defined(MEMCALL)
#define	MEMCALL
#endif

// 000000-0fffff メインメモリ
// 100000-10ffef HMA
// 110000-193fff FONT-ROM/RAM
// 1a8000-1bffff VRAM1
// 1c0000-1c7fff ITF-ROM BAK
// 1c8000-1dffff EPSON RAM
// 1e0000-1e7fff VRAM1
// 1f8000-1fffff ITF-ROM

#define	CPU_MEMREADMAX	0x0a4000
#define	CPU_MEMWRITEMAX	0x0a0000

#define	VRAM_STEP	0x100000
#define	VRAM_B		0x0a8000
#define	VRAM_R		0x0b0000
#define	VRAM_G		0x0b8000
#define	VRAM_E		0x0e0000

#define	VRAMADDRMASKEX(a)	((a) & (VRAM_STEP | 0x7fff))

#define	VRAM0_B		VRAM_B
#define	VRAM0_R		VRAM_R
#define	VRAM0_G		VRAM_G
#define	VRAM0_E		VRAM_E
#define	VRAM1_B		(VRAM_STEP + VRAM_B)
#define	VRAM1_R		(VRAM_STEP + VRAM_R)
#define	VRAM1_G		(VRAM_STEP + VRAM_G)
#define	VRAM1_E		(VRAM_STEP + VRAM_E)

#define	FONT_ADRS	0x110000
#define	ITF_ADRS	0x1f8000

#define	USE_HIMEM		0x110000

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

REG8 MEMCALL meml_read8(UINT32 address);
REG16 MEMCALL meml_read16(UINT32 address);
UINT32 MEMCALL meml_read32(UINT32 address);
void MEMCALL meml_write8(UINT32 address, REG8 dat);
void MEMCALL meml_write16(UINT32 address, REG16 dat);
void MEMCALL meml_write32(UINT32 address, UINT32 dat);
void MEMCALL meml_reads(UINT32 address, void *dat, UINT leng);
void MEMCALL meml_writes(UINT32 address, const void *dat, UINT leng);

REG8 MEMCALL memr_read8(UINT seg, UINT off);
REG16 MEMCALL memr_read16(UINT seg, UINT off);
UINT32 MEMCALL memr_read32(UINT seg, UINT off);
void MEMCALL memr_write8(UINT seg, UINT off, REG8 dat);
void MEMCALL memr_write16(UINT seg, UINT off, REG16 dat);
void MEMCALL memr_write32(UINT seg, UINT off, UINT32 dat);
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
#define	MEMP_READ16(addr)					\
			memp_read16((addr))
#define	MEMP_READ32(addr)					\
			memp_read32((addr))
#define	MEMP_WRITE8(addr, dat)				\
			memp_write8((addr), (dat))
#define	MEMP_WRITE16(addr, dat)				\
			memp_write16((addr), (dat))
#define	MEMP_WRITE32(addr, dat)				\
			memp_write32((addr), (dat))
#define MEMP_READS(addr, dat, leng)			\
			memp_reads((addr), (dat), (leng))
#define MEMP_WRITES(addr, dat, leng)		\
			memp_writes((addr), (dat), (leng))


// ---- Logical Space (BIOS)

#define MEML_READ8(addr)					\
			meml_read8((addr))
#define MEML_READ16(addr)					\
			meml_read16((addr))
#define MEML_READ32(addr)					\
			meml_read32((addr))
#define MEML_WRITE8(addr, dat)				\
			meml_write8((addr), (dat))
#define MEML_WRITE16(addr, dat)				\
			meml_write16((addr), (dat))
#define MEML_WRITE32(addr, dat)				\
			meml_write32((addr), (dat))
#define MEML_READS(addr, dat, leng)			\
			meml_reads((addr), (dat), (leng))
#define MEML_WRITES(addr, dat, leng)		\
			meml_writes((addr), (dat), (leng))

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

#endif	/* !NP2_I386C_CPUMEM_H__ */
