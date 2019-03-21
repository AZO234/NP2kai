
#ifdef SUPPORT_KAI_IMAGES

#define	MAX_FDDFILE		4							// ver0.31

#include	"diskimage/fd/fdd_head_d88.h"
#include	"diskimage/fd/fdd_head_dcp.h"	//	追加(Kai1)
#include	"diskimage/fd/fdd_head_nfd.h"	//	追加(Kai1)
#include	"diskimage/fd/fdd_head_vfdd.h"	//	追加(Kai1)

enum {
	FDDFILE_MF			= 0x80,
	FDDFILE_FM			= 0x20,

	FDDFILE_AM			= 0x08,
	FDDFILE_DDAM		= 0x02,

	FDDFILE_NONE		= 0x00,
	FDDCHECK_FM			= (FDDFILE_MF | FDDFILE_FM),
	FDDCHECK_MFM		= FDDFILE_MF,
	FDDCHECK_DAM		= FDDFILE_AM,
	FDDCHECK_DDAM		= (FDDFILE_AM | FDDFILE_DDAM),

	FDDWRITE_FM			= 0x40,
	FDDWRITE_DDAM		= 0x04
};

enum {
	DISKTYPE_NOTREADY	= 0,
	DISKTYPE_BETA,
	DISKTYPE_D88,
	DISKTYPE_DCP,		//	追加(Kai1)
	DISKTYPE_MAHALITO,	//	未実装
	DISKTYPE_NFD,		//	追加(Kai1)
	DISKTYPE_VFDD		//	追加(Kai1)
};

enum {
	DISKTYPE_2D			= 0,
	DISKTYPE_2DD,
	DISKTYPE_2HD
};

typedef struct {
	UINT32	headersize;
	UINT8	tracks;
	UINT8	sectors;
	UINT8	n;
	UINT8	disktype;
	UINT8	rpm;
} _XDFINFO, *XDFINFO;

typedef struct {
	UINT		fdtype_major;
	UINT		fdtype_minor;
	UINT32		fd_size;
	UINT32		ptr[D88_TRACKMAX];
	_D88HEAD	head;
} _D88INFO, *D88INFO;

//	追加(Kai1)
typedef struct {
	_XDFINFO	xdf;
	UINT32		ptr[DCP_TRACKMAX];
	_DCPHEAD	head;
} _DCPINFO, *DCPINFO, _BKDSKINFO, *BKDSKINFO;

typedef struct {
	_XDFINFO		xdf;
	UINT32		ptr[NFD_TRKMAX1][0xff];
	UINT32		tptr[NFD_TRKMAX1];
	UINT32		trksize[NFD_TRKMAX1];
	union {
		NFD_FILE_HEAD	r0;
		NFD_FILE_HEAD1	r1;
	} head;
} _NFDINFO, *NFDINFO;

typedef struct {
	_XDFINFO	xdf;
	SINT32		ptr[VFDD_TRKMAX][VFDD_SECMAX];
	_VFDD_HEAD	head;
	_VFDD_ID	id[VFDD_TRKMAX][VFDD_SECMAX];
	_VFDD_SP	sp_dmy;
} _VFDDINFO, *VFDDINFO;
//

typedef struct {
	OEMCHAR	fname[MAX_PATH];
	UINT	ftype;
	int		ro;
	UINT8	type;
	UINT8	num;
	UINT8	protect;
	union {
		_XDFINFO	xdf;
		_D88INFO	d88;
		_DCPINFO	dcp;	//	追加(Kai1)
		_BKDSKINFO	bkdsk;	//	追加(Kai1)
		_NFDINFO	nfd;	//	追加(Kai1)
		_VFDDINFO	vfdd;	//	追加(Kai1)
	} inf;
} _FDDFILE, *FDDFILE;

//	各種処理関数格納構造体(Kai1)
typedef struct {
	BRESULT	(*eject)(FDDFILE fdd);

	BRESULT	(*diskaccess)(FDDFILE fdd);
	BRESULT	(*seek)(FDDFILE fdd);
	BRESULT	(*seeksector)(FDDFILE fdd);
	BRESULT	(*readdiag)(FDDFILE fdd);
	BRESULT	(*read)(FDDFILE fdd);
	BRESULT	(*write)(FDDFILE fdd);
	BRESULT	(*readid)(FDDFILE fdd);
	BRESULT	(*writeid)(FDDFILE fdd);

	BRESULT	(*formatinit)(FDDFILE fdd);
	BRESULT	(*formating)(FDDFILE fdd, const UINT8 *ID);
	BOOL	(*isformating)(FDDFILE fdd);
	BOOL	fdcresult;
} _FDDFUNC, *FDDFUNC;
//

#ifdef __cplusplus
extern "C" {
#endif

extern	_FDDFILE	fddfile[MAX_FDDFILE];
extern	UINT8		fddlasterror;

// 起動時に一回だけ初期化
void fddfile_initialize(void);

void fddfile_reset2dmode(void);

OEMCHAR *fdd_diskname(REG8 drv);
OEMCHAR *fdd_getfileex(REG8 drv, UINT *ftype, int *ro);
BOOL fdd_diskready(REG8 drv);
BOOL fdd_diskprotect(REG8 drv);

BRESULT fdd_set(REG8 drv, const OEMCHAR *fname, UINT ftype, int ro);
BRESULT fdd_eject(REG8 drv);

//	未実装、未対応用ダミー関数群(Kai1)
BRESULT fdd_dummy_xxx(FDDFILE fdd);
BRESULT fdd_eject_xxx(FDDFILE fdd);
BRESULT fdd_formating_xxx(FDDFILE fdd, const UINT8 *ID);
BOOL fdd_isformating_xxx(FDDFILE fdd);
//
//	ベタ系イメージ用共通処理関数群(Kai1)
BRESULT fdd_diskaccess_common(FDDFILE fdd);
BRESULT fdd_seek_common(FDDFILE fdd);
BRESULT fdd_seeksector_common(FDDFILE fdd);
BRESULT fdd_readid_common(FDDFILE fdd);
//

BRESULT fdd_diskaccess(void);
BRESULT fdd_seek(void);
BRESULT fdd_seeksector(void);
BRESULT fdd_read(void);
BRESULT fdd_write(void);
BRESULT fdd_diagread(void);
BRESULT fdd_readid(void);
BRESULT fdd_writeid(void);

BRESULT fdd_formatinit(void);
BRESULT fdd_formating(const UINT8 *ID);
BOOL fdd_isformating(void);

BOOL fdd_fdcresult(void);	//	追加(Kai1)

#ifdef __cplusplus
}
#endif

#else
#define	MAX_FDDFILE		4							// ver0.31

#include	"fdd/d88head.h"

enum {
	FDDFILE_MF			= 0x80,
	FDDFILE_FM			= 0x20,

	FDDFILE_AM			= 0x08,
	FDDFILE_DDAM		= 0x02,

	FDDFILE_NONE		= 0x00,
	FDDCHECK_FM			= (FDDFILE_MF | FDDFILE_FM),
	FDDCHECK_MFM		= FDDFILE_MF,
	FDDCHECK_DAM		= FDDFILE_AM,
	FDDCHECK_DDAM		= (FDDFILE_AM | FDDFILE_DDAM),

	FDDWRITE_FM			= 0x40,
	FDDWRITE_DDAM		= 0x04
};

enum {
	DISKTYPE_NOTREADY	= 0,
	DISKTYPE_BETA,
	DISKTYPE_D88
};

enum {
	DISKTYPE_2D			= 0,
	DISKTYPE_2DD,
	DISKTYPE_2HD
};

typedef struct {
	UINT32	headersize;
	UINT8	tracks;
	UINT8	sectors;
	UINT8	n;
	UINT8	disktype;
	UINT8	rpm;
} _XDFINFO, *XDFINFO;

typedef struct {
	UINT		fdtype_major;
	UINT		fdtype_minor;
	UINT32		fd_size;
	UINT32		ptr[164];
	_D88HEAD	head;
} _D88INFO, *D88INFO;

typedef struct {
	OEMCHAR	fname[MAX_PATH];
	UINT	ftype;
	int		ro;
	UINT8	type;
	UINT8	num;
	UINT8	protect;
	union {
		_XDFINFO	xdf;
		_D88INFO	d88;
	} inf;
} _FDDFILE, *FDDFILE;


#ifdef __cplusplus
extern "C" {
#endif

extern	_FDDFILE	fddfile[MAX_FDDFILE];
extern	UINT8		fddlasterror;

// 起動時に一回だけ初期化
void fddfile_initialize(void);

void fddfile_reset2dmode(void);

OEMCHAR *fdd_diskname(REG8 drv);
OEMCHAR *fdd_getfileex(REG8 drv, UINT *ftype, int *ro);
BOOL fdd_diskready(REG8 drv);
BOOL fdd_diskprotect(REG8 drv);

BRESULT fdd_set(REG8 drv, const OEMCHAR *fname, UINT ftype, int ro);
BRESULT fdd_eject(REG8 drv);

BRESULT fdd_diskaccess(void);
BRESULT fdd_seek(void);
BRESULT fdd_seeksector(void);
BRESULT fdd_read(void);
BRESULT fdd_write(void);
BRESULT fdd_diagread(void);
BRESULT fdd_readid(void);
BRESULT fdd_writeid(void);

BRESULT fdd_formatinit(void);
BRESULT fdd_formating(const UINT8 *ID);
BOOL fdd_isformating(void);

#ifdef __cplusplus
}
#endif

#endif