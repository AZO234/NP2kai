
#ifdef __cplusplus
extern "C" {
#endif

BRESULT	fdd_set_dcp(FDDFILE fdd, FDDFUNC fdd_fn, const OEMCHAR *fname, int ro);

BRESULT	fdd_read_dcp(FDDFILE fdd);
BRESULT	fdd_write_dcp(FDDFILE fdd);

#ifdef __cplusplus
}
#endif

