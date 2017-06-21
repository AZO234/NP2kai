
#ifdef __cplusplus
extern "C" {
#endif

void egcshift(void);

REG8 MEMCALL egc_readbyte(UINT32 addr);
void MEMCALL egc_writebyte(UINT32 addr, REG8 value);
REG16 MEMCALL egc_readword(UINT32 addr);
void MEMCALL egc_writeword(UINT32 addr, REG16 value);

REG8 MEMCALL memegc_rd8(UINT32 addr);
void MEMCALL memegc_wr8(UINT32 addr, REG8 value);
REG16 MEMCALL memegc_rd16(UINT32 addr);
void MEMCALL memegc_wr16(UINT32 addr, REG16 value);

#ifdef __cplusplus
}
#endif

