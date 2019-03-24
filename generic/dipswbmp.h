
#ifdef __cplusplus
extern "C"{
#endif


// それぞれ 4bit BMPが返る (メモリ解放を行なうこと)

UINT8 *dipswbmp_get9861(const UINT8 *s, const UINT8 *j);

UINT8 *dipswbmp_getsnd26(UINT8 cfg);
UINT8 *dipswbmp_getsnd86(UINT8 cfg);
UINT8 *dipswbmp_getsndspb(UINT8 cfg, UINT8 vrc);
UINT8 *dipswbmp_getmpu(UINT8 cfg);

#ifdef __cplusplus
}
#endif

