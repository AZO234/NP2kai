
#ifdef __cplusplus
extern "C" {
#endif

BRESULT	fdd_set_nfd(FDDFILE fdd, FDDFUNC fdd_fn, const OEMCHAR *fname, int ro);

BRESULT fdd_seeksector_nfd(FDDFILE fdd);	//	追加(kaiE)
BRESULT	fdd_read_nfd(FDDFILE fdd);
BRESULT	fdd_write_nfd(FDDFILE fdd);
BRESULT fdd_readid_nfd(FDDFILE fdd);
BRESULT fdd_formatinit_nfd(FDDFILE fdd);	/* 170107 to support format command */

BRESULT fdd_seeksector_nfd1(FDDFILE fdd);	//	追加(kaiD)
BRESULT	fdd_read_nfd1(FDDFILE fdd);			//	追加(kaiD)
BRESULT	fdd_write_nfd1(FDDFILE fdd);		//	追加(kaiD)
BRESULT fdd_readid_nfd1(FDDFILE fdd);		//	追加(kaiD)

#ifdef __cplusplus
}
#endif

