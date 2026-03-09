
#ifdef SUPPORT_KAI_IMAGES

#ifdef __cplusplus
extern "C" {
#endif
	
#if 1	//	関数名変更＆引数追加(k9)
BRESULT	fdd_set_xdf(FDDFILE fdd, FDDFUNC fdd_fn, const OEMCHAR *fname, int ro);
BRESULT	fdd_set_fdi(FDDFILE fdd, FDDFUNC fdd_fn, const OEMCHAR *fname, int ro);

//BRESULT	fdd_diskaccess_xdf(FDDFILE fdd);
//BRESULT	fdd_seek_xdf(FDDFILE fdd);
//BRESULT	fdd_seeksector_xdf(FDDFILE fdd);
BRESULT	fdd_read_xdf(FDDFILE fdd);
BRESULT	fdd_write_xdf(FDDFILE fdd);
//BRESULT	fdd_readid_xdf(FDDFILE fdd);
BRESULT	fdd_formatinit_xdf(FDDFILE fdd);	/* 170107 to support format command */
#else
BRESULT fddxdf_set(FDDFILE fdd, const OEMCHAR *fname, int ro);
BRESULT fddxdf_setfdi(FDDFILE fdd, const OEMCHAR *fname, int ro);
BRESULT fddxdf_eject(FDDFILE fdd);

BRESULT fddxdf_diskaccess(FDDFILE fdd);
BRESULT fddxdf_seek(FDDFILE fdd);
BRESULT fddxdf_seeksector(FDDFILE fdd);
BRESULT fddxdf_read(FDDFILE fdd);
BRESULT fddxdf_write(FDDFILE fdd);
BRESULT fddxdf_readid(FDDFILE fdd);

#ifdef __cplusplus
}
#endif
#endif

#else
#ifdef __cplusplus
extern "C" {
#endif
	
BRESULT fddxdf_set(FDDFILE fdd, const OEMCHAR *fname, int ro);
BRESULT fddxdf_setfdi(FDDFILE fdd, const OEMCHAR *fname, int ro);
BRESULT fddxdf_eject(FDDFILE fdd);

BRESULT fddxdf_diskaccess(FDDFILE fdd);
BRESULT fddxdf_seek(FDDFILE fdd);
BRESULT fddxdf_seeksector(FDDFILE fdd);
BRESULT fddxdf_read(FDDFILE fdd);
BRESULT fddxdf_write(FDDFILE fdd);
BRESULT fddxdf_readid(FDDFILE fdd);

#ifdef __cplusplus
}
#endif
#endif


