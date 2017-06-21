
struct _np2infoex;
typedef struct	_np2infoex	NP2INFOEX;

struct _np2infoex {
	OEMCHAR	cr[4];
	BOOL	(*ext)(OEMCHAR *dst, const OEMCHAR *key, int maxlen,
														const NP2INFOEX *ex);
};


#ifdef __cplusplus
extern "C" {
#endif

void np2info(OEMCHAR *dst, const OEMCHAR *src, int maxlen,
														const NP2INFOEX *ex);

#ifdef __cplusplus
}
#endif

