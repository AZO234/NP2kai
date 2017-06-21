
// テキストファイルの変換ルール

enum {
	TEXTCNV_DEFAULT	= 0,
	TEXTCNV_SJIS	= 1,
	TEXTCNV_EUC		= 2,
	TEXTCNV_UTF8	= 3,
	TEXTCNV_UCS2	= 4
};

enum {
	TEXTCNV_READ	= 0x01,
	TEXTCNV_WRITE	= 0x02
};

typedef UINT (*TCTOOEM)(OEMCHAR *dst, UINT dcnt, const void *src, UINT scnt);
typedef UINT (*TCFROMOEM)(void *dst, UINT dcnt, const OEMCHAR *src, UINT scnt);

typedef struct {
	UINT8		caps;
	UINT8		xendian;
	UINT8		width;
	UINT8		hdrsize;
	TCTOOEM		tooem;
	TCFROMOEM	fromoem;
} TCINF;


#ifdef __cplusplus
extern "C" {
#endif

UINT textcnv_getinfo(TCINF *inf, const UINT8 *hdr, UINT hdrsize);
void textcnv_swapendian16(void *buf, UINT leng);
void textcnv_swapendian32(void *buf, UINT leng);

#ifdef __cplusplus
}
#endif

