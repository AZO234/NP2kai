
typedef struct {
	int		enable;
} _NMIIO, *NMIIO;


#ifdef __cplusplus
extern "C" {
#endif

void nmiio_reset(const NP2CFG *pConfig);
void nmiio_bind(void);

#ifdef __cplusplus
}
#endif

