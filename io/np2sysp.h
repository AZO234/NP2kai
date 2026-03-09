
#define	NP2SYSP_BIT		4
#define	NP2SYSP_LEN		(1 << NP2SYSP_BIT)
#define	NP2SYSP_MASK	(NP2SYSP_LEN - 1)

typedef struct {
	char	substr[NP2SYSP_LEN];
	char	outstr[NP2SYSP_LEN];
	int		strpos;
	int		outpos;
	UINT32	outval;
	UINT32	inpval;
} _NP2SYSP, *NP2SYSP;


#ifdef __cplusplus
extern "C" {
#endif

void np2sysp_outstr(const void *arg1, long arg2);

void np2sysp_reset(const NP2CFG *pConfig);
void np2sysp_bind(void);

#ifdef __cplusplus
}
#endif

