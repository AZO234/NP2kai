
#ifdef __cplusplus
extern "C" {
#endif

REG8 MEMCALL memems_rd8(UINT32 address);
REG16 MEMCALL memems_rd16(UINT32 address);
void MEMCALL memems_wr8(UINT32 address, REG8 value);
void MEMCALL memems_wr16(UINT32 address, REG16 value);

#ifdef __cplusplus
}
#endif

