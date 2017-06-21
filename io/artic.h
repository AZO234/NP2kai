
typedef struct {
	SINT32	lastclk2;
	SINT32	counter;
} _ARTIC, *ARTIC;


#ifdef __cplusplus
extern "C" {
#endif

void artic_callback(void);

void artic_reset(const NP2CFG *pConfig);
void artic_bind(void);
REG16 IOINPCALL artic_r16(UINT port);

#ifdef __cplusplus
}
#endif

