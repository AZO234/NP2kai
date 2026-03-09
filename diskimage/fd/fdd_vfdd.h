
#ifdef __cplusplus
extern "C" {
#endif

BRESULT	fdd_set_vfdd(FDDFILE fdd, FDDFUNC fdd_fn, const OEMCHAR *fname, int ro);

BRESULT	fdd_read_vfdd(FDDFILE fdd);
BRESULT	fdd_write_vfdd(FDDFILE fdd);
BRESULT fdd_readid_vfdd(FDDFILE fdd);

#ifdef __cplusplus
}
#endif

