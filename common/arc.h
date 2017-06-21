
enum {
	ARCSEEK_SET			= 0,
	ARCSEEK_CUR			= 1,
	ARCSEEK_END			= 2
};


typedef struct {
	UINT16	year;
	UINT8	month;
	UINT8	day;
	UINT8	hour;
	UINT8	minute;
	UINT8	second;
} ARCTIME;

typedef struct {
	UINT	method;
	UINT	originalsize;
	UINT	compresssize;
	ARCTIME	time;
} ARCINF;

struct _arcdh;
struct _arcfh;
struct _arch;
typedef struct _arcdh		_ARCDH;
typedef struct _arcdh		*ARCDH;
typedef struct _arcfh		_ARCFH;
typedef struct _arcfh		*ARCFH;
typedef struct _arch		_ARCH;
typedef struct _arch		*ARCH;

struct _arcdh {
	ARCH	arch;
	BRESULT (*dirread)(ARCDH arcdh, char *fname, UINT size, ARCINF *inf);
	void	(*dirclose)(ARCDH arcdh);
};

struct _arcfh {
	ARCH	arch;
	UINT	(*fileread)(ARCFH arcfh, void *buffer, UINT size);
	UINT	(*filewrite)(ARCFH arcfh, const void *buffer, UINT size);
	long	(*fileseek)(ARCFH arcfh, long pos, UINT method);
	void	(*fileclose)(ARCFH arcfh);
};

struct _arch {
	UINT	arctype;
	UINT	locked;
	ARCDH	(*diropen)(ARCH arch);
	ARCFH	(*fileopen)(ARCH arch, const char *name);
	SINT16	(*fileattr)(ARCH arch, const char *name);
	void	(*deinitialize)(ARCH arch);
	OEMCHAR	path[MAX_PATH];
};


#ifdef __cplusplus
extern "C" {
#endif

ARCH arc_open(const OEMCHAR *path);
void arc_close(ARCH arch);

ARCDH arc_diropen(ARCH arch);
BRESULT arc_dirread(ARCDH arcdh, OEMCHAR *fname, UINT size, ARCINF *inf);
void arc_dirclose(ARCDH arcdh);

SINT16 arc_attr(ARCH arch, const OEMCHAR *fname);

ARCFH arc_fileopen(ARCH arch, const OEMCHAR *fname);
UINT arc_fileread(ARCFH arcfh, void *buffer, UINT size);
UINT arc_filewrite(ARCFH arcfh, const void *buffer, UINT size);
long arc_fileseek(ARCFH arcfh, long pos, UINT method);
void arc_fileclose(ARCFH arcfh);

SINT16 arcex_attr(const OEMCHAR *fname);
ARCFH arcex_fileopen(const OEMCHAR *fname);
ARCFH arcex_filecreate(const OEMCHAR *fname);

void arcfunc_lock(ARCH arch);
void arcfunc_unlock(ARCH arch);

#ifdef __cplusplus
}
#endif

