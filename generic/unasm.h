
typedef struct {
const char	*mnemonic;
	char	operand[44];
} _UNASM, *UNASM;


#ifdef __cplusplus
extern "C" {
#endif

UINT unasm(UNASM r, const UINT8 *ptr, UINT leng, BOOL d, UINT32 addr);

#ifdef __cplusplus
}
#endif

