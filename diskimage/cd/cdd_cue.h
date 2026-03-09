#ifdef __cplusplus
extern "C" {
#endif

extern const OEMCHAR str_cue[];
extern const OEMCHAR str_track[];
extern const OEMCHAR str_index[];

BRESULT opencue(SXSIDEV sxsi, const OEMCHAR *fname);

#ifdef __cplusplus
}
#endif
