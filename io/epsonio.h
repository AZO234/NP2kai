
typedef struct {
	UINT8	cpumode;
	UINT8	bankioen;
} _EPSONIO;


#ifdef __cplusplus
extern "C" {
#endif

void epsonio_reset(const NP2CFG *pConfig);
void epsonio_bind(void);

#ifdef __cplusplus
}
#endif

