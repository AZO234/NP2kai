
#ifdef __cplusplus
extern "C" {
#endif

void MEMCALL memd000_wr8(UINT32 address, REG8 value);
void MEMCALL memd000_wr16(UINT32 address, REG16 value);
void MEMCALL memd000_wr32(UINT32 address, UINT32 value);

REG8 MEMCALL memf800_rd8(UINT32 address);
REG16 MEMCALL memf800_rd16(UINT32 address);
UINT32 MEMCALL memf800_rd32(UINT32 address);

void MEMCALL memepson_wr8(UINT32 address, REG8 value);
void MEMCALL memepson_wr16(UINT32 address, REG16 value);
void MEMCALL memepson_wr32(UINT32 address, UINT32 value);

#ifdef __cplusplus
}
#endif

