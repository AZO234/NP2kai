
#ifdef __cplusplus
extern "C" {
#endif

void cbuscore_reset(const NP2CFG *pConfig);
void cbuscore_bind(void);

void cbuscore_attachsndex(UINT port, const IOOUT *out, const IOINP *inp);
void cbuscore_detachsndex(UINT port);

#ifdef __cplusplus
}
#endif

