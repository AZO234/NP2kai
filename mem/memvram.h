
#ifdef __cplusplus
extern "C" {
#endif

REG8 MEMCALL memvram0_rd8(UINT32 address);
REG8 MEMCALL memvram1_rd8(UINT32 address);
REG16 MEMCALL memvram0_rd16(UINT32 address);
REG16 MEMCALL memvram1_rd16(UINT32 address);
void MEMCALL memvram0_wr8(UINT32 address, REG8 value);
void MEMCALL memvram1_wr8(UINT32 address, REG8 value);
void MEMCALL memvram0_wr16(UINT32 address, REG16 value);
void MEMCALL memvram1_wr16(UINT32 address, REG16 value);

REG8 MEMCALL memtcr0_rd8(UINT32 address);
REG8 MEMCALL memtcr1_rd8(UINT32 address);
REG16 MEMCALL memtcr0_rd16(UINT32 address);
REG16 MEMCALL memtcr1_rd16(UINT32 address);

void MEMCALL memrmw0_wr8(UINT32 address, REG8 value);
void MEMCALL memrmw1_wr8(UINT32 address, REG8 value);
void MEMCALL memrmw0_wr16(UINT32 address, REG16 value);
void MEMCALL memrmw1_wr16(UINT32 address, REG16 value);

void MEMCALL memtdw0_wr8(UINT32 address, REG8 value);
void MEMCALL memtdw1_wr8(UINT32 address, REG8 value);
void MEMCALL memtdw0_wr16(UINT32 address, REG16 value);
void MEMCALL memtdw1_wr16(UINT32 address, REG16 value);

#ifdef __cplusplus
}
#endif

