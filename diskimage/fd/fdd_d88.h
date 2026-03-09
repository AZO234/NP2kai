#ifdef SUPPORT_KAI_IMAGES

#ifdef __cplusplus
extern "C" {
#endif

#if 1	//	関数名変更＆引数追加(k9)
BRESULT	fdd_set_d88(FDDFILE fdd, FDDFUNC fdd_fn, const OEMCHAR *fname, int ro);
BRESULT	fdd_eject_d88(FDDFILE fdd);

BRESULT	fdd_diskaccess_d88(FDDFILE fdd);
BRESULT	fdd_seek_d88(FDDFILE fdd);
BRESULT	fdd_seeksector_d88(FDDFILE fdd);
BRESULT	fdd_read_d88(FDDFILE fdd);
BRESULT	fdd_write_d88(FDDFILE fdd);
BRESULT	fdd_diagread_d88(FDDFILE fdd);
BRESULT	fdd_readid_d88(FDDFILE fdd);
BRESULT	fdd_writeid_d88(FDDFILE fdd);	//	未実装

BRESULT	fdd_formatinit_d88(FDDFILE fdd);
BRESULT	fdd_formating_d88(FDDFILE fdd, const UINT8 *ID);
BOOL	fdd_isformating_d88(FDDFILE fdd);
#else
BRESULT fddd88_set(FDDFILE fdd, const OEMCHAR *fname, int ro);
BRESULT fddd88_eject(FDDFILE fdd);

BRESULT fdd_diskaccess_d88(void);
BRESULT fdd_seek_d88(void);
BRESULT fdd_seeksector_d88(void);
BRESULT fdd_read_d88(void);
BRESULT fdd_write_d88(void);
BRESULT fdd_diagread_d88(void);
BRESULT fdd_readid_d88(void);
BRESULT fdd_writeid_d88(void);

BRESULT fdd_formatinit_d88(void);
BRESULT fdd_formating_d88(const UINT8 *ID);
BOOL fdd_isformating_d88(void);
#endif

#ifdef __cplusplus
}
#endif

#else
#ifdef __cplusplus
extern "C" {
#endif

BRESULT fddd88_set(FDDFILE fdd, const OEMCHAR *fname, int ro);
BRESULT fddd88_eject(FDDFILE fdd);

BRESULT fdd_diskaccess_d88(void);
BRESULT fdd_seek_d88(void);
BRESULT fdd_seeksector_d88(void);
BRESULT fdd_read_d88(void);
BRESULT fdd_write_d88(void);
BRESULT fdd_diagread_d88(void);
BRESULT fdd_readid_d88(void);
BRESULT fdd_writeid_d88(void);

BRESULT fdd_formatinit_d88(void);
BRESULT fdd_formating_d88(const UINT8 *ID);
BOOL fdd_isformating_d88(void);

#ifdef __cplusplus
}
#endif

#endif
