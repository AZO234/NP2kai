
#if defined(__GNUC__)
typedef struct {
	UINT8	bfType[2];
	UINT8	bfSize[4];
	UINT8	bfReserved1[2];
	UINT8	bfReserved2[2];
	UINT8	bfOffBits[4];
} __attribute__ ((packed)) BMPFILE;
typedef struct {
	UINT8	biSize[4];
	UINT8	biWidth[4];
	UINT8	biHeight[4];
	UINT8	biPlanes[2];
	UINT8	biBitCount[2];
	UINT8	biCompression[4];
	UINT8	biSizeImage[4];
	UINT8	biXPelsPerMeter[4];
	UINT8	biYPelsPerMeter[4];
	UINT8	biClrUsed[4];
	UINT8	biClrImportant[4];
} __attribute__ ((packed)) BMPINFO;
#else
#pragma pack(push, 1)
typedef struct {
	UINT8	bfType[2];
	UINT8	bfSize[4];
	UINT8	bfReserved1[2];
	UINT8	bfReserved2[2];
	UINT8	bfOffBits[4];
} BMPFILE;
typedef struct {
	UINT8	biSize[4];
	UINT8	biWidth[4];
	UINT8	biHeight[4];
	UINT8	biPlanes[2];
	UINT8	biBitCount[2];
	UINT8	biCompression[4];
	UINT8	biSizeImage[4];
	UINT8	biXPelsPerMeter[4];
	UINT8	biYPelsPerMeter[4];
	UINT8	biClrUsed[4];
	UINT8	biClrImportant[4];
} BMPINFO;
#pragma pack(pop)
#endif

typedef struct {
	int		width;
	int		height;
	int		bpp;
} BMPDATA;


#ifdef __cplusplus
extern "C" {
#endif

UINT bmpdata_getalign(const BMPINFO *bi);
UINT bmpdata_getdatasize(const BMPINFO *bi);

UINT bmpdata_sethead(BMPFILE *bf, const BMPINFO *bi);
UINT bmpdata_setinfo(BMPINFO *bi, const BMPDATA *inf);
BRESULT bmpdata_getinfo(const BMPINFO *bi, BMPDATA *inf);

UINT8 *bmpdata_lzx(int level, int dstsize, const UINT8 *dat);
UINT8 *bmpdata_solvedata(const UINT8 *dat);

#ifdef __cplusplus
}
#endif

