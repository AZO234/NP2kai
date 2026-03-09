
typedef struct {
	UINT	code;
	UINT	lr;
	UINT	line;
} _CGROM, *CGROM;

typedef struct {
	UINT32	low;
	UINT32	high;
	UINT8	writable;
} _CGWINDOW, *CGWINDOW;


#ifdef __cplusplus
extern "C" {
#endif

void cgrom_reset(const NP2CFG *pConfig);
void cgrom_bind(void);

#ifdef __cplusplus
}
#endif

