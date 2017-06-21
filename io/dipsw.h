
#ifdef __cplusplus
extern "C" {
#endif

void IOOUTCALL dipsw_w8(UINT port, REG8 value);
REG8 IOINPCALL dipsw_r8(UINT port);

#ifdef __cplusplus
}
#endif

