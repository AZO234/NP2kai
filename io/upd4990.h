
#define UPD4990_REGLEN	8

typedef struct {
	UINT8	last;
	UINT8	cmd;
	UINT8	serial;
	UINT8	parallel;
	UINT8	reg[UPD4990_REGLEN];
	UINT	pos;
	UINT8	cdat;
	UINT8	regsft;
} _UPD4990, *UPD4990;


#ifdef __cplusplus
extern "C" {
#endif

void uPD4990_reset(const NP2CFG *pConfig);
void uPD4990_bind(void);

#ifdef __cplusplus
}
#endif

