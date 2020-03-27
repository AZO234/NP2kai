#include "compiler.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#if defined(_WINDOWS)
#include "oemtext.h"
#include "codecnv/codecnv.h"
#endif
#include "dosio.h"
#if defined(__LIBRETRO__)
#include <retro_dirent.h>
#if !defined(_MSC_VER)
#include <unistd.h>
#endif
#else
#if defined(_WINDOWS)
#include <direct.h>
#else
#include <dirent.h>
#endif
#endif

#if defined(_WINDOWS)
static	OEMCHAR	curpath[MAX_PATH] = OEMTEXT(".\\");
#else
static	OEMCHAR	curpath[MAX_PATH] = OEMTEXT("./");
#endif
static	OEMCHAR	*curfilep = curpath + 2;

void
dosio_init(void)
{

	/* nothing to do */
}

void
dosio_term(void)
{

	/* nothing to do */
}

/* ファイル操作 */
FILEH file_open(const OEMCHAR *path) {

#if defined(__LIBRETRO__)
	return(filestream_open(path, RETRO_VFS_FILE_ACCESS_READ_WRITE, RETRO_VFS_FILE_ACCESS_HINT_NONE));
#elif defined(_WINDOWS)
	wchar_t	ucpath[MAX_PATH];
	MultiByteToWideChar(
		CP_UTF8,
		MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
		path,
		MAX_PATH,
		ucpath,
		sizeof(ucpath)
	);
	return(_wfopen(ucpath, "rb+"));
#else
	return(fopen(path, "rb+"));
#endif
}

FILEH file_open_rb(const OEMCHAR *path) {

#if defined(__LIBRETRO__)
	return(filestream_open(path, RETRO_VFS_FILE_ACCESS_READ, RETRO_VFS_FILE_ACCESS_HINT_NONE));
#elif defined(_WINDOWS)
	wchar_t	ucpath[MAX_PATH];
	MultiByteToWideChar(
		CP_UTF8,
		MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
		path,
		MAX_PATH,
		ucpath,
		sizeof(ucpath)
	);
	return(_wfopen(ucpath, "rb"));
#else
	return(fopen(path, "rb"));
#endif
}

FILEH file_create(const OEMCHAR *path) {

#if defined(__LIBRETRO__)
	return(filestream_open(path, RETRO_VFS_FILE_ACCESS_READ_WRITE, RETRO_VFS_FILE_ACCESS_HINT_NONE));
#elif defined(_WINDOWS)
	wchar_t	ucpath[MAX_PATH];
	MultiByteToWideChar(
		CP_UTF8,
		MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
		path,
		MAX_PATH,
		ucpath,
		sizeof(ucpath)
	);
	return(_wfopen(ucpath, "wb+"));
#else
	return(fopen(path, "wb+"));
#endif
}

FILEPOS file_seek(FILEH handle, FILEPOS pointer, int method) {
#if defined(SUPPORT_LARGE_HDD)
#if defined(__LIBRETRO__)
	int rmethod = 0;
	switch(method) {
	case FSEEK_SET:
		rmethod = RETRO_VFS_SEEK_POSITION_START;
		break;
	case FSEEK_CUR:
		rmethod = RETRO_VFS_SEEK_POSITION_CURRENT;
		break;
	case FSEEK_END:
		rmethod = RETRO_VFS_SEEK_POSITION_END;
		break;
	}
	filestream_seek(handle, pointer, rmethod);
	return(filestream_tell(handle));
#elif defined (__MINGW32__) 
	fseeko64(handle, pointer, method);
	return(ftello64(handle));
#elif defined (_MSC_VER)
	_fseeki64(handle, pointer, method);
	return(_ftelli64(handle));
#else
	fseeko(handle, pointer, method);
	return(ftello(handle));
#endif
#else
#if defined(__LIBRETRO__)
	filestream_seek(handle, pointer, method);
	return(filestream_tell(handle));
#else
	fseek(handle, pointer, method);
	return(ftell(handle));
#endif
#endif
}

UINT file_read(FILEH handle, void *data, UINT length) {

#if defined(__LIBRETRO__)
	return((UINT)filestream_read(handle, data, length));
#else
	return((UINT)fread(data, 1, length, handle));
#endif
}

UINT file_write(FILEH handle, const void *data, UINT length) {

#if defined(__LIBRETRO__)
	return((UINT)filestream_write(handle, data, length));
#else
	return((UINT)fwrite(data, 1, length, handle));
#endif
}

short file_close(FILEH handle) {

#if defined(__LIBRETRO__)
	filestream_close(handle);
#else
	fclose(handle);
#endif
	return(0);
}

FILELEN file_getsize(FILEH handle) {

#if defined(SUPPORT_LARGE_HDD)
#if defined(__LIBRETRO__)
	return (FILELEN)filestream_get_size(handle);
#elif defined(_WINDOWS)
	struct _stati64 sb;

	if (_fstati64(fileno(handle), &sb) == 0)
	{
		return (FILELEN)sb.st_size;
	}
#else
	struct stat sb;

	if (fstat(fileno(handle), &sb) == 0)
	{
		return (FILELEN)sb.st_size;
	}
#endif
#else
#if defined(__LIBRETRO__)
	return (FILELEN)filestream_get_size(handle);
#else
	struct stat sb;

	if (fstat(fileno(handle), &sb) == 0)
	{
		return (FILELEN)sb.st_size;
	}
#endif
#endif
	return(0);
}

short file_attr(const OEMCHAR *path) {

struct stat	sb;
	short	attr;

#if defined(_LIBRETRO__)
	if (path_is_directory(path)) {
		attr = FILEATTR_DIRECTORY;
	}
	else {
		attr = 0;
	}
#elif defined(_WINDOWS)
	wchar_t	ucpath[MAX_PATH];
	MultiByteToWideChar(
		CP_UTF8,
		MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
		path,
		MAX_PATH,
		ucpath,
		sizeof(ucpath)
	);
	if (_wstat(ucpath, &sb) == 0)
#else
	if (stat(path, &sb) == 0)
#endif
	{
#if defined(_WINDOWS)
		if (sb.st_mode & _S_IFDIR) {
			attr = FILEATTR_DIRECTORY;
		}
		else {
			attr = 0;
		}
		if (!(sb.st_mode & S_IWRITE)) {
			attr |= FILEATTR_READONLY;
		}
#else
		if (S_ISDIR(sb.st_mode)) {
			return(FILEATTR_DIRECTORY);
		}
		attr = 0;
		if (!(sb.st_mode & S_IWUSR)) {
			attr |= FILEATTR_READONLY;
		}
#endif
		return(attr);
	}
	return(-1);
}

static BRESULT cnv_sttime(time_t *t, DOSDATE *dosdate, DOSTIME *dostime) {

struct tm	*ftime;

	ftime = localtime(t);
	if (ftime == NULL) {
		return(FAILURE);
	}
	if (dosdate) {
		dosdate->year = ftime->tm_year + 1900;
		dosdate->month = ftime->tm_mon + 1;
		dosdate->day = ftime->tm_mday;
	}
	if (dostime) {
		dostime->hour = ftime->tm_hour;
		dostime->minute = ftime->tm_min;
		dostime->second = ftime->tm_sec;
	}
	return(SUCCESS);
}

short file_getdatetime(FILEH handle, DOSDATE *dosdate, DOSTIME *dostime) {

struct stat sb;

	if (fstat(fileno(handle), &sb) == 0) {
		if (cnv_sttime(&sb.st_mtime, dosdate, dostime) == SUCCESS) {
			return(0);
		}
	}
	return(-1);
}

short file_delete(const OEMCHAR *path) {

#if defined(_LIBRETRO__)
	return(filestream_delete(path))
#else
	return(remove(path));
#endif
}

short file_rename(const OEMCHAR *existpath, const OEMCHAR *newpath) {

#if defined(_LIBRETRO__)
	return((short)filestream_rename(existpath, newpath));
#else
	return((short)rename(existpath, newpath));
#endif
}

short file_dircreate(const OEMCHAR *path) {

#if defined(__LIBRETRO__)
	return((short)path_mkdir(path));
#elif defined(_WINDOWS)
	return((short)mkdir(path));
#else
	return((short)mkdir(path, 0777));
#endif
}

short file_dirdelete(const OEMCHAR *path) {

#if defined(__LIBRETRO__)
	return((short)rmdir(path));
#else
	return((short)rmdir(path));
#endif
}

/* カレントファイル操作 */
void file_setcd(const OEMCHAR *exepath) {

	file_cpyname(curpath, exepath, sizeof(curpath));
	curfilep = file_getname(curpath);
	*curfilep = '\0';
}

OEMCHAR *file_getcd(const OEMCHAR *path) {

	file_cpyname(curfilep, path, NELEMENTS(curpath) - (UINT)(curfilep - curpath));
	return(curpath);
}

FILEH file_open_c(const OEMCHAR *path) {

	file_cpyname(curfilep, path, NELEMENTS(curpath) - (UINT)(curfilep - curpath));
	return(file_open(curpath));
}

FILEH file_open_rb_c(const OEMCHAR *path) {

	file_cpyname(curfilep, path, NELEMENTS(curpath) - (UINT)(curfilep - curpath));
	return(file_open_rb(curpath));
}

FILEH file_create_c(const OEMCHAR *path) {

	file_cpyname(curfilep, path, NELEMENTS(curpath) - (UINT)(curfilep - curpath));
	return(file_create(curpath));
}

short file_delete_c(const OEMCHAR *path) {

	file_cpyname(curfilep, path, NELEMENTS(curpath) - (UINT)(curfilep - curpath));
	return(file_delete(curpath));
}

short file_attr_c(const OEMCHAR *path) {

	file_cpyname(curfilep, path, NELEMENTS(curpath) - (UINT)(curfilep - curpath));
	return(file_attr_c(curpath));
}

#if !defined(__LIBRETRO__) && defined(_WINDOWS)
static BRESULT cnvdatetime(FILETIME *file, DOSDATE *dosdate, DOSTIME *dostime) {

	FILETIME	localtime;
	SYSTEMTIME	systime;

	if ((FileTimeToLocalFileTime(file, &localtime) == 0) ||
		(FileTimeToSystemTime(&localtime, &systime) == 0)) {
		return(FAILURE);
	}
	if (dosdate) {
		dosdate->year = (UINT16)systime.wYear;
		dosdate->month = (UINT8)systime.wMonth;
		dosdate->day = (UINT8)systime.wDay;
	}
	if (dostime) {
		dostime->hour = (UINT8)systime.wHour;
		dostime->minute = (UINT8)systime.wMinute;
		dostime->second = (UINT8)systime.wSecond;
	}
	return(SUCCESS);
}

static BRESULT setflist(WIN32_FIND_DATA *w32fd, FLINFO *fli) {

	if ((w32fd->dwFileAttributes & FILEATTR_DIRECTORY) &&
		((!file_cmpname(w32fd->cFileName, ".")) ||
		(!file_cmpname(w32fd->cFileName, "..")))) {
		return(FAILURE);
	}
	fli->caps = FLICAPS_SIZE | FLICAPS_ATTR;
	fli->size = w32fd->nFileSizeLow;
	fli->attr = w32fd->dwFileAttributes;
	if (cnvdatetime(&w32fd->ftLastWriteTime, &fli->date, &fli->time)
																== SUCCESS) {
		fli->caps |= FLICAPS_DATE | FLICAPS_TIME;
	}
#if defined(OSLANG_UTF8)
	codecnv_sjistoutf8(fli->path, sizeof(fli->path),
												w32fd->cFileName, (UINT)-1);
#else
	file_cpyname(fli->path, w32fd->cFileName, sizeof(fli->path));
#endif
	return(SUCCESS);
}

FLISTH file_list1st(const OEMCHAR *dir, FLINFO *fli) {

	OEMCHAR			path[MAX_PATH];
	HANDLE			hdl;
	WIN32_FIND_DATA	w32fd;

	file_cpyname(path, dir, sizeof(path));
	file_setseparator(path, sizeof(path));
	file_catname(path, "*.*", sizeof(path));
	hdl = FindFirstFile(path, &w32fd);
	if (hdl != INVALID_HANDLE_VALUE) {
		do {
			if (setflist(&w32fd, fli) == SUCCESS) {
				return(hdl);
			}
		} while(FindNextFile(hdl, &w32fd));
		FindClose(hdl);
	}
	return(FLISTH_INVALID);
}

BRESULT file_listnext(FLISTH hdl, FLINFO *fli) {

	WIN32_FIND_DATA	w32fd;

	while(FindNextFile(hdl, &w32fd)) {
		if (setflist(&w32fd, fli) == SUCCESS) {
			return(SUCCESS);
		}
	}
	return(FAILURE);
}

void file_listclose(FLISTH hdl) {

	FindClose(hdl);
}
#else
FLISTH file_list1st(const OEMCHAR *dir, FLINFO *fli) {

#if defined(__LIBRETRO__)
	struct RDIR	*ret;

	ret = retro_opendir(dir);
#else
	DIR		*ret;

	ret = opendir(dir);
#endif
	if (ret == NULL) {
		goto ff1_err;
	}
	if (file_listnext((FLISTH)ret, fli) == SUCCESS) {
		return((FLISTH)ret);
	}
#if defined(__LIBRETRO__)
	retro_closedir(ret);
#else
	closedir(ret);
#endif

ff1_err:
	return(FLISTH_INVALID);
}

BRESULT file_listnext(FLISTH hdl, FLINFO *fli) {

#if defined(__LIBRETRO__)
int	de;
#else
struct dirent	*de;
#endif
struct stat		sb;

#if defined(__LIBRETRO__)
	de = retro_readdir((struct RDIR *)hdl);
	if (de == 0) {
#else
	de = readdir((DIR *)hdl);
	if (de == NULL) {
#endif
		return(FAILURE);
	}
	if (fli) {
		memset(fli, 0, sizeof(*fli));
		fli->caps = FLICAPS_ATTR;
#if defined(__LIBRETRO__)
		fli->attr = retro_dirent_is_dir((struct RDIR *)hdl, "") ? FILEATTR_DIRECTORY : 0;
#else
		fli->attr = (de->d_type & DT_DIR) ? FILEATTR_DIRECTORY : 0;
#endif

#if defined(__LIBRETRO__)
		milstr_ncpy(fli->path, retro_dirent_get_name((struct RDIR *)hdl), sizeof(fli->path));
#else
		if (stat(de->d_name, &sb) == 0) {
			fli->caps |= FLICAPS_SIZE;
			fli->size = (UINT)sb.st_size;
			if (S_ISDIR(sb.st_mode)) {
				fli->attr |= FILEATTR_DIRECTORY;
			}
			if (!(sb.st_mode & S_IWUSR)) {
				fli->attr |= FILEATTR_READONLY;
			}
			if (cnv_sttime(&sb.st_mtime, &fli->date, &fli->time) == SUCCESS) {
				fli->caps |= FLICAPS_DATE | FLICAPS_TIME;
			}
		}
		milstr_ncpy(fli->path, de->d_name, sizeof(fli->path));
#endif
	}
	return(SUCCESS);
}

void file_listclose(FLISTH hdl) {

#if defined(__LIBRETRO__)
	retro_closedir((struct RDIR *)hdl);
#else
	closedir((DIR *)hdl);
#endif
}
#endif

void file_catname(OEMCHAR *path, const OEMCHAR *name, int maxlen) {

	int		csize;

	while(maxlen > 0) {
		if (*path == '\0') {
			break;
		}
		path++;
		maxlen--;
	}
	file_cpyname(path, name, maxlen);
	while((csize = milstr_charsize(path)) != 0) {
#if defined(_WINDOWS)
		if ((csize == 1) && (*path == '\\')) {
			*path = '\\';
		}
#else
		if ((csize == 1) && (*path == '/')) {
			*path = '/';
		}
#endif
		path += csize;
	}
}

OEMCHAR *file_getname(const OEMCHAR *path) {

const OEMCHAR	*ret;
	int		csize;

	ret = path;
	while((csize = milstr_charsize(path)) != 0) {
#if defined(_WINDOWS)
		if ((csize == 1) && (*path == '\\')) {
#else
		if ((csize == 1) && (*path == '/')) {
#endif
			ret = path + 1;
		}
		path += csize;
	}
	return((OEMCHAR *)ret);
}

void file_cutname(OEMCHAR *path) {

	OEMCHAR	*p;

	p = file_getname(path);
	*p = '\0';
}

OEMCHAR *file_getext(const OEMCHAR *path) {

const OEMCHAR	*p;
const OEMCHAR	*q;

	p = file_getname(path);
	q = NULL;
	while(*p != '\0') {
		if (*p == '.') {
			q = p + 1;
		}
		p++;
	}
	if (q == NULL) {
		q = p;
	}
	return((OEMCHAR *)q);
}

void file_cutext(OEMCHAR *path) {

	OEMCHAR	*p;
	OEMCHAR	*q;

	p = file_getname(path);
	q = NULL;
	while(*p != '\0') {
		if (*p == '.') {
			q = p;
		}
		p++;
	}
	if (q != NULL) {
		*q = '\0';
	}
}

void file_cutseparator(OEMCHAR *path) {

	int		pos;

	pos = (int)strlen(path) - 1;
	if ((pos > 0) &&							// 2文字以上でー
#if defined(_WINDOWS)
		(path[pos] == '\\') &&					// ケツが \ でー
#else
		(path[pos] == '/') &&					// ケツが \ でー
#endif
		((pos != 1) || (path[0] != '.'))) {		// './' ではなかったら
		path[pos] = '\0';
	}
}

void file_setseparator(OEMCHAR *path, int maxlen) {

	int		pos;

	pos = (int)OEMSTRNLEN(path, maxlen);
#if defined(_WINDOWS)
	if ((pos) && (path[pos-1] != '\\') && ((pos + 2) < maxlen)) {
		path[pos++] = '\\';
#else	/* _WINDOWS */
	if ((pos) && (path[pos-1] != '/') && ((pos + 2) < maxlen)) {
		path[pos++] = '/';
#endif	/* _WINDOWS */
		path[pos] = '\0';
	}
}

