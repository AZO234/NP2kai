/* === dosio for wx port (POSIX) === */

#include <compiler.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <dosio.h>

static OEMCHAR  curpath[MAX_PATH] = OEMTEXT("./");
static OEMCHAR *curfilep = curpath + 2;

void dosio_init(void) {}
void dosio_term(void) {}

/* ---- file operations ---- */

FILEH file_open(const OEMCHAR *path)
{
	FILEH fh = fopen(path, "rb+");
	if (!fh) fh = fopen(path, "rb");
	return fh;
}
FILEH file_open_rb(const OEMCHAR *path)    { return fopen(path, "rb"); }
FILEH file_open_rw(const OEMCHAR *path)    { return fopen(path, "rb+"); }
FILEH file_create(const OEMCHAR *path)     { return fopen(path, "wb+"); }

FILEPOS file_seek(FILEH handle, FILEPOS pointer, int method)
{
	if (!handle) return -1;
	if (fseek((FILE *)handle, (long)pointer, method) != 0) return -1;
	return (FILEPOS)ftell((FILE *)handle);
}

UINT file_read(FILEH handle, void *data, UINT length)
{
	if (!handle) return 0;
	return (UINT)fread(data, 1, length, (FILE *)handle);
}

UINT file_write(FILEH handle, const void *data, UINT length)
{
	if (!handle) return 0;
	return (UINT)fwrite(data, 1, length, (FILE *)handle);
}

short file_close(FILEH handle)
{
	if (!handle) return 0;
	return (short)fclose((FILE *)handle);
}

FILELEN file_getsize(FILEH handle)
{
	if (!handle) return 0;
	long cur = ftell((FILE *)handle);
	fseek((FILE *)handle, 0, SEEK_END);
	long size = ftell((FILE *)handle);
	fseek((FILE *)handle, cur, SEEK_SET);
	return (FILELEN)size;
}

short file_getdatetime(FILEH handle, DOSDATE *dosdate, DOSTIME *dostime)
{
	if (!handle) return -1;
	int fd = fileno((FILE *)handle);
	struct stat st;
	if (fstat(fd, &st) != 0) return -1;
	struct tm *tm = localtime(&st.st_mtime);
	if (!tm) return -1;
	if (dosdate) {
		dosdate->year  = (UINT16)(tm->tm_year + 1900);
		dosdate->month = (UINT16)(tm->tm_mon + 1);
		dosdate->day   = (UINT16)tm->tm_mday;
	}
	if (dostime) {
		dostime->hour   = (UINT8)tm->tm_hour;
		dostime->minute = (UINT8)tm->tm_min;
		dostime->second = (UINT8)tm->tm_sec;
	}
	return 0;
}

short file_sync(FILEH handle)
{
	if (!handle) return -1;
	if (fflush((FILE *)handle) != 0) return -1;
	return (fsync(fileno((FILE *)handle)) == 0) ? 0 : -1;
}

short file_setsize(FILEH handle, FILELEN length)
{
	if (!handle) return -1;
	if (fflush((FILE *)handle) != 0) return -1;
	return (ftruncate(fileno((FILE *)handle), (off_t)length) == 0) ? 0 : -1;
}

short file_setdatetime(FILEH handle, const DOSDATE *dosdate, const DOSTIME *dostime)
{
	struct tm tmv;
	struct stat st;
	struct timeval tv[2];
	time_t mtime;

	if (!handle || !dosdate || !dostime) return -1;
	memset(&tmv, 0, sizeof(tmv));
	tmv.tm_year = (int)dosdate->year - 1900;
	tmv.tm_mon  = (int)dosdate->month - 1;
	tmv.tm_mday = dosdate->day;
	tmv.tm_hour = dostime->hour;
	tmv.tm_min  = dostime->minute;
	tmv.tm_sec  = dostime->second;
	tmv.tm_isdst = -1;
	mtime = mktime(&tmv);
	if (mtime == (time_t)-1 || fstat(fileno((FILE *)handle), &st) != 0) return -1;
	tv[0].tv_sec  = st.st_atime;
	tv[0].tv_usec = 0;
	tv[1].tv_sec  = mtime;
	tv[1].tv_usec = 0;
	return (futimes(fileno((FILE *)handle), tv) == 0) ? 0 : -1;
}

/* short filenames (8.3) have no POSIX equivalent */
BRESULT file_getshortname(const OEMCHAR *path, OEMCHAR *shortname, UINT cchShortName)
{
	(void)path;
	(void)shortname;
	(void)cchShortName;
	return FAILURE;
}

BOOL file_islink(const OEMCHAR *path)
{
	struct stat st;
	return (lstat(path, &st) == 0 && S_ISLNK(st.st_mode)) ? TRUE : FALSE;
}

BOOL file_infoislink(const FLINFO *fli, const OEMCHAR *path)
{
	(void)fli;
	return file_islink(path);
}

/* POSIX has no reliable way to detect that another process holds an
 * exclusive lock on a disk image unless NP2 itself takes one on open,
 * which it does not; always report "not locked" so callers fall back
 * to a generic open-error message. */
BOOL file_islocked(const OEMCHAR *path)
{
	(void)path;
	return FALSE;
}

BRESULT file_getinfo(const OEMCHAR *path, FLINFO *fli)
{
	struct stat st;

	if (stat(path, &st) != 0) return FAILURE;
	if (fli != NULL) {
		memset(fli, 0, sizeof(*fli));
		fli->caps = FLICAPS_SIZE | FLICAPS_ATTR | FLICAPS_DATE | FLICAPS_TIME;
		fli->size = (UINT32)st.st_size;
		if (S_ISDIR(st.st_mode)) fli->attr |= FILEATTR_DIRECTORY;
		if (!(st.st_mode & S_IWUSR)) fli->attr |= FILEATTR_READONLY;
		struct tm *tm = localtime(&st.st_mtime);
		if (tm) {
			fli->date.year  = (UINT16)(tm->tm_year + 1900);
			fli->date.month = (UINT8)(tm->tm_mon + 1);
			fli->date.day   = (UINT8)tm->tm_mday;
			fli->time.hour   = (UINT8)tm->tm_hour;
			fli->time.minute = (UINT8)tm->tm_min;
			fli->time.second = (UINT8)tm->tm_sec;
		}
		milstr_ncpy(fli->path, file_getname(path), sizeof(fli->path));
		fli->shortpath[0] = '\0';
	}
	return SUCCESS;
}

short file_delete(const OEMCHAR *path)   { return (short)remove(path); }
short file_attr(const OEMCHAR *path)
{
	struct stat st;
	if (stat(path, &st) != 0) return -1;
	short attr = 0;
	if (S_ISDIR(st.st_mode)) attr |= FILEATTR_DIRECTORY;
	if (!(st.st_mode & S_IWUSR)) attr |= FILEATTR_READONLY;
	return attr;
}
short file_rename(const OEMCHAR *existpath, const OEMCHAR *newpath)
{ return (short)rename(existpath, newpath); }
short file_dircreate(const OEMCHAR *path) { return (short)mkdir(path, 0755); }
short file_dirdelete(const OEMCHAR *path) { return (short)rmdir(path); }

/* ---- current-directory helpers ---- */

void file_setcd(const OEMCHAR *exepath)
{
	milstr_ncpy(curpath, exepath, MAX_PATH);
	/* ensure trailing slash */
	int len = (int)OEMSTRLEN(curpath);
	if (len > 0 && curpath[len-1] != '/') {
		curpath[len]   = '/';
		curpath[len+1] = '\0';
		len++;
	}
	curfilep = curpath + len;
}

OEMCHAR *file_getcd(const OEMCHAR *filename)
{
	static OEMCHAR buf[MAX_PATH];
	milstr_ncpy(buf, curpath, MAX_PATH);
	if (filename) milstr_ncat(buf, filename, MAX_PATH);
	return buf;
}

FILEH file_open_c(const OEMCHAR *path)
{
	return file_open(file_getcd(path));
}
FILEH file_open_rb_c(const OEMCHAR *path)
{
	return file_open_rb(file_getcd(path));
}
FILEH file_create_c(const OEMCHAR *path)
{
	return file_create(file_getcd(path));
}
short file_delete_c(const OEMCHAR *path)
{
	return file_delete(file_getcd(path));
}
short file_attr_c(const OEMCHAR *path)
{
	return file_attr(file_getcd(path));
}

/* ---- path utilities ---- */

void file_catname(OEMCHAR *path, const OEMCHAR *name, int maxlen)
{
	milstr_ncat(path, name, maxlen);
}

OEMCHAR *file_getname(const OEMCHAR *path)
{
	const OEMCHAR *p    = path;
	const OEMCHAR *last = path;
	while (*p) {
		if (*p == '/' || *p == '\\') last = p + 1;
		p++;
	}
	return (OEMCHAR *)last;
}

void file_cutname(OEMCHAR *path)
{
	OEMCHAR *p = file_getname(path);
	if (p > path) *(p - 1) = '\0';
	else *p = '\0';
}

OEMCHAR *file_getext(const OEMCHAR *path)
{
	const OEMCHAR *name = file_getname(path);
	const OEMCHAR *dot  = NULL;
	for (const OEMCHAR *p = name; *p; p++) {
		if (*p == '.') dot = p;
	}
	return dot ? (OEMCHAR *)(dot + 1) : (OEMCHAR *)(path + OEMSTRLEN(path));
}

void file_cutext(OEMCHAR *path)
{
	/* file_getext returns pointer past the dot; step back to the dot itself */
	OEMCHAR *ext = file_getext(path);
	if (ext > path && *(ext - 1) == '.') *(ext - 1) = '\0';
}

void file_cutseparator(OEMCHAR *path)
{
	int len = (int)OEMSTRLEN(path);
	if (len > 1 && (path[len-1] == '/' || path[len-1] == '\\'))
		path[len-1] = '\0';
}

void file_setseparator(OEMCHAR *path, int maxlen)
{
	int len = (int)OEMSTRLEN(path);
	if (len > 0 && path[len-1] != '/' && len < maxlen - 1) {
		path[len]   = '/';
		path[len+1] = '\0';
	}
}

/* ---- directory listing ---- */

typedef struct {
	DIR     *dir;
	OEMCHAR  basepath[MAX_PATH];
} _FLISTIMPL;

FLISTH file_list1st(const OEMCHAR *dir, FLINFO *fli)
{
	_FLISTIMPL *fh;
	DIR *d = opendir(dir && dir[0] ? dir : ".");
	if (!d) return FLISTH_INVALID;

	fh = (_FLISTIMPL *)calloc(1, sizeof(*fh));
	if (!fh) { closedir(d); return FLISTH_INVALID; }
	fh->dir = d;
	milstr_ncpy(fh->basepath, dir, MAX_PATH);
	file_setseparator(fh->basepath, MAX_PATH);

	if (file_listnext((FLISTH)fh, fli) != SUCCESS) {
		closedir(d);
		free(fh);
		return FLISTH_INVALID;
	}
	return (FLISTH)fh;
}

BRESULT file_listnext(FLISTH hdl, FLINFO *fli)
{
	_FLISTIMPL *fh = (_FLISTIMPL *)hdl;
	if (!fh || !fh->dir) return FAILURE;

	struct dirent *de;
	while ((de = readdir(fh->dir)) != NULL) {
		if (de->d_name[0] == '.') continue;
		milstr_ncpy(fli->path, de->d_name, MAX_PATH);
		fli->shortpath[0] = '\0';
		OEMCHAR fullpath[MAX_PATH];
		milstr_ncpy(fullpath, fh->basepath, MAX_PATH);
		milstr_ncat(fullpath, de->d_name, MAX_PATH);
		struct stat st;
		fli->caps = 0;
		if (stat(fullpath, &st) == 0) {
			fli->size  = (UINT32)st.st_size;
			fli->attr  = S_ISDIR(st.st_mode) ? FILEATTR_DIRECTORY : 0;
			fli->caps  = FLICAPS_SIZE | FLICAPS_ATTR;
		}
		return SUCCESS;
	}
	return FAILURE;
}

void file_listclose(FLISTH hdl)
{
	_FLISTIMPL *fh = (_FLISTIMPL *)hdl;
	if (fh) {
		if (fh->dir) closedir(fh->dir);
		free(fh);
	}
}
