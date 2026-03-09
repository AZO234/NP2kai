
#if defined(SUPPORT_PC9821)

#ifdef __cplusplus
extern "C" {
#endif

// PEGC 0F00000h-00F80000h Memory Access ?
REG8 MEMCALL memvgaf_rd8(UINT32 address);
void MEMCALL memvgaf_wr8(UINT32 address, REG8 value);
REG16 MEMCALL memvgaf_rd16(UINT32 address);
void MEMCALL memvgaf_wr16(UINT32 address, REG16 value);
UINT32 MEMCALL memvgaf_rd32(UINT32 address);
void MEMCALL memvgaf_wr32(UINT32 address, UINT32 value);

// PEGC memvga0:A8000h-AFFFFh, memvga1:B0000h-B7FFFh Bank(Packed-pixel Mode) or Plane Access(Plane Mode)
REG8 MEMCALL memvga0_rd8(UINT32 address);
REG8 MEMCALL memvga1_rd8(UINT32 address);
void MEMCALL memvga0_wr8(UINT32 address, REG8 value);
void MEMCALL memvga1_wr8(UINT32 address, REG8 value);
REG16 MEMCALL memvga0_rd16(UINT32 address);
REG16 MEMCALL memvga1_rd16(UINT32 address);
void MEMCALL memvga0_wr16(UINT32 address, REG16 value);
void MEMCALL memvga1_wr16(UINT32 address, REG16 value);
UINT32 MEMCALL memvga0_rd32(UINT32 address);
UINT32 MEMCALL memvga1_rd32(UINT32 address);
void MEMCALL memvga0_wr32(UINT32 address, UINT32 value);
void MEMCALL memvga1_wr32(UINT32 address, UINT32 value);

// PEGC E0000h-E7FFFh MMIO
REG8 MEMCALL memvgaio_rd8(UINT32 address);
void MEMCALL memvgaio_wr8(UINT32 address, REG8 value);
REG16 MEMCALL memvgaio_rd16(UINT32 address);
void MEMCALL memvgaio_wr16(UINT32 address, REG16 value);
UINT32 MEMCALL memvgaio_rd32(UINT32 address);
void MEMCALL memvgaio_wr32(UINT32 address, UINT32 value);

#ifdef __cplusplus
}
#endif

#endif

