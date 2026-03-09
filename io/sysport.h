
typedef struct {
	UINT8	c;
} _SYSPORT, *SYSPORT;


#ifdef __cplusplus
extern "C" {
#endif

void systemport_reset(const NP2CFG *pConfig);
void systemport_bind(void);

#ifdef __cplusplus
}
#endif

