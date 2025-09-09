
#ifdef __cplusplus
extern "C" {
#endif

REG8 MEMCALL memtram_rd8(UINT32 address);
REG16 MEMCALL memtram_rd16(UINT32 address);
UINT32 MEMCALL memtram_rd32(UINT32 address);
void MEMCALL memtram_wr8(UINT32 address, REG8 value);
void MEMCALL memtram_wr16(UINT32 address, REG16 value);
void MEMCALL memtram_wr32(UINT32 address, UINT32 value);

#ifdef __cplusplus
}
#endif

