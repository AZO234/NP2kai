#ifndef NP2_SDL2_DOSIO_H
#define NP2_SDL2_DOSIO_H

#if defined(__LIBRETRO__)
#include "file_stream.h"
typedef RFILE *				FILEH;
#else
typedef FILE *				FILEH;
#endif
#define	FILEH_INVALID		NULL

#if defined(WIN32)
#define	FLISTH				HANDLE
#define	FLISTH_INVALID		(INVALID_HANDLE_VALUE)
#else
#define	FLISTH				long
#define	FLISTH_INVALID		0
#endif

#define	FSEEK_SET	SEEK_SET
#define	FSEEK_CUR	SEEK_CUR
#define	FSEEK_END	SEEK_END

enum {
	FILEATTR_READONLY	= 0x01,
	FILEATTR_HIDDEN		= 0x02,
	FILEATTR_SYSTEM		= 0x04,
	FILEATTR_VOLUME		= 0x08,
	FILEATTR_DIRECTORY	= 0x10,
	FILEATTR_ARCHIVE	= 0x20,
	FILEATTR_NORMAL	= 0x80
};

enum {
	FLICAPS_SIZE		= 0x0001,
	FLICAPS_ATTR		= 0x0002,
	FLICAPS_DATE		= 0x0004,
	FLICAPS_TIME		= 0x0008
};

typedef struct {
	UINT16	year;		/* cx */
	UINT8	month;		/* dh */
	UINT8	day;		/* dl */
} DOSDATE;

typedef struct {
	UINT8	hour;		/* ch */
	UINT8	minute;		/* cl */
	UINT8	second;		/* dh */
} DOSTIME;

typedef struct {
	UINT	caps;
	UINT32	size;
	UINT32	attr;
	DOSDATE	date;
	DOSTIME	time;
	char	path[MAX_PATH];
} FLINFO;


#ifdef __cplusplus
extern "C" {
#endif

void dosio_init(void);
void dosio_term(void);

/* ファイル操作 */
FILEH file_open(const OEMCHAR *path);
FILEH file_open_rb(const OEMCHAR *path);
FILEH file_create(const OEMCHAR *path);
FILEPOS file_seek(FILEH handle, FILEPOS pointer, int method);
UINT file_read(FILEH handle, void *data, UINT length);
UINT file_write(FILEH handle, const void *data, UINT length);
short file_close(FILEH handle);
FILELEN file_getsize(FILEH handle);
short file_getdatetime(FILEH handle, DOSDATE *dosdate, DOSTIME *dostime);
short file_delete(const OEMCHAR *path);
short file_attr(const OEMCHAR *path);
short file_rename(const OEMCHAR *existpath, const OEMCHAR *newpath);
short file_dircreate(const OEMCHAR *path);
short file_dirdelete(const OEMCHAR *path);

/* カレントファイル操作 */
void file_setcd(const OEMCHAR *exepath);
OEMCHAR *file_getcd(const OEMCHAR *path);
FILEH file_open_c(const OEMCHAR *path);
FILEH file_open_rb_c(const OEMCHAR *path);
FILEH file_create_c(const OEMCHAR *path);
short file_delete_c(const OEMCHAR *path);
short file_attr_c(const OEMCHAR *path);

FLISTH file_list1st(const OEMCHAR *dir, FLINFO *fli);
BRESULT file_listnext(FLISTH hdl, FLINFO *fli);
void file_listclose(FLISTH hdl);

#define file_cpyname(p, n, m)	milstr_ncpy(p, n, m)
#define file_cmpname(p, n)		milstr_cmp(p, n)
void file_catname(OEMCHAR *path, const OEMCHAR *name, int maxlen);
OEMCHAR *file_getname(const OEMCHAR *path);
void file_cutname(OEMCHAR *path);
OEMCHAR *file_getext(const OEMCHAR *path);
void file_cutext(OEMCHAR *path);
void file_cutseparator(OEMCHAR *path);
void file_setseparator(OEMCHAR *path, int maxlen);

#ifdef	__cplusplus
}
#endif

#endif	/* NP2_SDL2_DOSIO_H */

