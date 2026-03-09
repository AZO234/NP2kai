
enum {
	SXSIMEDIA_SASITYPE	= 0x07,
	SXSIMEDIA_INVSASI	= 0x08
};

typedef struct {
	UINT8	sectors;
	UINT8	surfaces;
	UINT16	cylinders;
} SASIHDD;

typedef struct {
	UINT8	cylinders[2];
} THDHDR;

typedef struct {
	char	sig[16];
	char	comment[0x100];
	UINT8	headersize[4];
	UINT8	cylinders[4];
	UINT8	surfaces[2];
	UINT8	sectors[2];
	UINT8	sectorsize[2];
	UINT8	reserved[0xe2];
} NHDHDR;

typedef struct {
	UINT8	dummy[4];
	UINT8	hddtype[4];
	UINT8	headersize[4];
	UINT8	hddsize[4];
	UINT8	sectorsize[4];
	UINT8	sectors[4];
	UINT8	surfaces[4];
	UINT8	cylinders[4];
} HDIHDR;

typedef struct {
	char	sig[3];
	char	ver[4];
	char	delimita;
	char	comment[128];
	UINT8	padding1[4];
	UINT8	mbsize[2];
	UINT8	sectorsize[2];
	UINT8	sectors;
	UINT8	surfaces;
	UINT8	cylinders[2];
	UINT8	totals[4];
	UINT8	padding2[0x44];
} VHDHDR;

typedef struct {
	char	sig[4];
	UINT8   drvsize[8];
	UINT8	sectorsize[4];
	UINT8	cylinders[4];
	UINT8	surfaces[4];
	UINT8	sectors[4];
	UINT8	serialnum[20];
	UINT8	revision[8];
	char 	model[40];
} SLHHDR;


#ifdef __cplusplus
extern "C" {
#endif

extern const char sig_vhd[8];
extern const char sig_nhd[15];
extern const SASIHDD sasihdd[7];

BRESULT sxsihdd_open(SXSIDEV sxsi, const OEMCHAR *fname);

#ifdef __cplusplus
}
#endif


