#ifdef __cplusplus
extern "C" {
#endif

extern const OEMCHAR str_ccd[];
extern const OEMCHAR str_cdm[];

BRESULT openccd(SXSIDEV sxsi, const OEMCHAR *fname);
//BRESULT opencdm(SXSIDEV sxsi, const OEMCHAR *fname);

#ifdef __cplusplus
}
#endif
