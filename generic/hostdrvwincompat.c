#include "compiler.h"

#if (defined(SUPPORT_HOSTDRV9X) || defined(SUPPORT_HOSTDRVNT)) && !defined(WIN32) && !defined(_WIN32)

#include "hostdrvwincompat.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <time.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
#ifndef O_DIRECTORY
#define O_DIRECTORY 0
#endif

#define HD_PATHBUF (PATH_MAX * 2)
#define HD_HANDLE_FILE 1
#define HD_HANDLE_FIND 2
#define HD_EPOCH_DIFF 11644473600ULL
#define HD_TICKS_PER_SEC 10000000ULL

typedef struct _HDRVWIN_LOCK {
	uint64_t off;
	uint64_t len;
	struct _HDRVWIN_LOCK *next;
} HDRVWIN_LOCK;

typedef struct _HDRVWIN_ATTRMETA {
	char *path;
	DWORD attrs;
	struct _HDRVWIN_ATTRMETA *next;
} HDRVWIN_ATTRMETA;

typedef struct _HDRVWIN_HANDLE {
	int type;
	int fd;
	DWORD desiredAccess;
	DWORD shareMode;
	dev_t dev;
	ino_t ino;
	char *path;
	HDRVWIN_LOCK *locks;
	struct _HDRVWIN_HANDLE *next;
	struct {
		DIR *dir;
		char directory[HD_PATHBUF];
		WCHAR pattern[MAX_PATH];
		int single;
		int done;
	} find;
} HDRVWIN_HANDLE;

static DWORD s_lastError;
static HDRVWIN_HANDLE *s_handles;
static HDRVWIN_ATTRMETA *s_attrmeta;

static WCHAR hd_upper16(WCHAR c)
{
	if (c >= 'a' && c <= 'z') return (WCHAR)(c - ('a' - 'A'));
	return c;
}

size_t hd_wcslen(const WCHAR *s)
{
	const WCHAR *p = s;
	if (!s) return 0;
	while (*p) ++p;
	return (size_t)(p - s);
}

WCHAR *hd_wcscpy(WCHAR *d, const WCHAR *s)
{
	WCHAR *r = d;
	while ((*d++ = *s++) != 0) { }
	return r;
}

WCHAR *hd_wcsncpy(WCHAR *d, const WCHAR *s, size_t n)
{
	WCHAR *r = d;
	while (n && *s) { *d++ = *s++; --n; }
	while (n) { *d++ = 0; --n; }
	return r;
}

WCHAR *hd_wcscat(WCHAR *d, const WCHAR *s)
{
	WCHAR *r = d;
	while (*d) ++d;
	hd_wcscpy(d, s);
	return r;
}

int hd_wcscmp(const WCHAR *a, const WCHAR *b)
{
	while (*a && *a == *b) { ++a; ++b; }
	return (int)*a - (int)*b;
}

int hd_wcsncmp(const WCHAR *a, const WCHAR *b, size_t n)
{
	while (n && *a && *a == *b) { ++a; ++b; --n; }
	if (!n) return 0;
	return (int)*a - (int)*b;
}

int hd_wcsicmp(const WCHAR *a, const WCHAR *b)
{
	WCHAR ca, cb;
	do {
		ca = hd_upper16(*a++);
		cb = hd_upper16(*b++);
		if (ca != cb) return (int)ca - (int)cb;
	} while (ca);
	return 0;
}

int hd_wcsnicmp(const WCHAR *a, const WCHAR *b, size_t n)
{
	WCHAR ca, cb;
	while (n--) {
		ca = hd_upper16(*a++);
		cb = hd_upper16(*b++);
		if (ca != cb) return (int)ca - (int)cb;
		if (!ca) return 0;
	}
	return 0;
}

WCHAR *hd_wcschr(const WCHAR *s, WCHAR c)
{
	while (*s) {
		if (*s == c) return (WCHAR *)(uintptr_t)s;
		++s;
	}
	return (c == 0) ? (WCHAR *)(uintptr_t)s : NULL;
}

WCHAR *hd_wcsrchr(const WCHAR *s, WCHAR c)
{
	const WCHAR *last = NULL;
	do {
		if (*s == c) last = s;
	} while (*s++);
	return (WCHAR *)(uintptr_t)last;
}

DWORD GetLastError(void) { return s_lastError; }
void SetLastError(DWORD error) { s_lastError = error; }

static DWORD hd_errno_error(int e)
{
	switch (e) {
	case 0: return ERROR_SUCCESS;
	case ENOENT: return ERROR_FILE_NOT_FOUND;
	case ENOTDIR: return ERROR_PATH_NOT_FOUND;
	case EACCES: case EPERM: case EROFS: return ERROR_ACCESS_DENIED;
	case EMFILE: case ENFILE: return ERROR_TOO_MANY_OPEN_FILES;
	case ENOMEM: return ERROR_NOT_ENOUGH_MEMORY;
	case EEXIST: return ERROR_FILE_EXISTS;
	case ENOSPC: return ERROR_DISK_FULL;
	case ENAMETOOLONG: return ERROR_FILENAME_EXCED_RANGE;
	case ENOTEMPTY: return ERROR_DIR_NOT_EMPTY;
	case EXDEV: return ERROR_NOT_SAME_DEVICE;
	case EINVAL: return ERROR_INVALID_PARAMETER;
	default: return ERROR_INVALID_DATA;
	}
}

static int hd_u16_to_utf8(const WCHAR *src, char *dst, size_t dstSize)
{
	UINT n;
	if (!src || !dst || dstSize == 0) return 0;
	n = codecnv_ucs2toutf8(dst, (UINT)dstSize, (const UINT16 *)src, (UINT)-1);
	if (n == 0 || dst[dstSize - 1] != '\0') dst[dstSize - 1] = '\0';
	return dst[0] || src[0] == 0;
}

static int hd_utf8_to_u16(const char *src, WCHAR *dst, size_t dstChars)
{
	UINT n;
	if (!src || !dst || dstChars == 0) return 0;
	n = codecnv_utf8toucs2((UINT16 *)dst, (UINT)dstChars, src, (UINT)-1);
	if (n == 0) { dst[0] = 0; return 0; }
	dst[dstChars - 1] = 0;
	return 1;
}

static void hd_slashes_native(char *s)
{
	while (*s) { if (*s == '\\') *s = '/'; ++s; }
}

static void hd_slashes_windows(char *s)
{
	while (*s) { if (*s == '/') *s = '\\'; ++s; }
}

static int hd_ascii_icmp(const char *a, const char *b)
{
	unsigned char ca, cb;
	for (;;) {
		ca = (unsigned char)*a++;
		cb = (unsigned char)*b++;
		if (ca >= 'a' && ca <= 'z') ca -= 0x20;
		if (cb >= 'a' && cb <= 'z') cb -= 0x20;
		if (ca != cb) return (int)ca - (int)cb;
		if (!ca) return 0;
	}
}

static int hd_normalize_native(const char *src, char *dst, size_t dstSize)
{
	char tmp[HD_PATHBUF];
	char *parts[PATH_MAX / 2];
	size_t count = 0, i, out = 0;
	int absolute;
	char *p, *start;
	if (!src || !dst || dstSize == 0 || strlen(src) >= sizeof(tmp)) return 0;
	strcpy(tmp, src);
	hd_slashes_native(tmp);
	absolute = (tmp[0] == '/');
	p = tmp;
	while (*p) {
		while (*p == '/') ++p;
		if (!*p) break;
		start = p;
		while (*p && *p != '/') ++p;
		if (*p) *p++ = 0;
		if (!strcmp(start, ".")) continue;
		if (!strcmp(start, "..")) {
			if (count) --count;
			else if (!absolute) parts[count++] = start;
			continue;
		}
		parts[count++] = start;
	}
	if (absolute) {
		if (out + 1 >= dstSize) return 0;
		dst[out++] = '/';
	}
	for (i = 0; i < count; ++i) {
		size_t len = strlen(parts[i]);
		if (out && dst[out - 1] != '/') {
			if (out + 1 >= dstSize) return 0;
			dst[out++] = '/';
		}
		if (out + len >= dstSize) return 0;
		memcpy(dst + out, parts[i], len); out += len;
	}
	if (out == 0) {
		if (dstSize < 2) return 0;
		dst[out++] = absolute ? '/' : '.';
	}
	dst[out] = 0;
	return 1;
}

static int hd_win16_to_native_lex(const WCHAR *path, char *native, size_t nativeSize)
{
	char utf8[HD_PATHBUF];
	if (!hd_u16_to_utf8(path, utf8, sizeof(utf8))) return 0;
	hd_slashes_native(utf8);
	return hd_normalize_native(utf8, native, nativeSize);
}

/* Resolve an existing path with Windows-like case-insensitive fallback.
 * Exact host spelling wins.  If fallback finds more than one candidate, fail
 * rather than selecting an arbitrary file on a case-sensitive host FS. */
static int hd_resolve_native_existing_raw(const char *input, char *out, size_t outSize)
{
	char norm[HD_PATHBUF];
	char current[HD_PATHBUF];
	char work[HD_PATHBUF];
	char *p;
	if (!hd_normalize_native(input, norm, sizeof(norm))) return 0;
	if (!strcmp(norm, "/") || !strcmp(norm, ".")) {
		if (strlen(norm) >= outSize) return 0;
		strcpy(out, norm); return 1;
	}
	if (strlen(norm) >= sizeof(work)) return 0;
	strcpy(work, norm);
	if (norm[0] == '/') strcpy(current, "/"); else strcpy(current, ".");
	p = work + (norm[0] == '/' ? 1 : 0);
	while (*p) {
		char *slash = strchr(p, '/');
		char component[NAME_MAX + 1];
		char exact[HD_PATHBUF];
		struct stat st;
		size_t len = slash ? (size_t)(slash - p) : strlen(p);
		if (!len || len > NAME_MAX) { SetLastError(ERROR_FILENAME_EXCED_RANGE); return 0; }
		memcpy(component, p, len); component[len] = 0;
		if (!strcmp(current, "/")) snprintf(exact, sizeof(exact), "/%s", component);
		else snprintf(exact, sizeof(exact), "%s/%s", current, component);
		if (lstat(exact, &st) == 0) {
			if (strlen(exact) >= sizeof(current)) return 0;
			strcpy(current, exact);
		} else {
			DIR *d = opendir(current);
			struct dirent *de;
			char chosen[NAME_MAX + 1];
			int matches = 0;
			if (!d) { SetLastError(hd_errno_error(errno)); return 0; }
			chosen[0] = 0;
			while ((de = readdir(d)) != NULL) {
				if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) continue;
				if (hd_ascii_icmp(de->d_name, component) == 0) {
					++matches;
					if (matches == 1) {
						strncpy(chosen, de->d_name, sizeof(chosen) - 1);
						chosen[sizeof(chosen) - 1] = 0;
					}
				}
			}
			closedir(d);
			if (matches != 1) {
				/* A Windows namespace cannot normally contain two names that differ
				 * only by case.  On a case-sensitive host, never guess between them. */
				SetLastError(matches ? ERROR_FILE_EXISTS : ERROR_FILE_NOT_FOUND);
				return 0;
			}
			if (!strcmp(current, "/")) snprintf(exact, sizeof(exact), "/%s", chosen);
			else snprintf(exact, sizeof(exact), "%s/%s", current, chosen);
			if (strlen(exact) >= sizeof(current)) return 0;
			strcpy(current, exact);
		}
		if (!slash) break;
		p = slash + 1;
	}
	if (strlen(current) >= outSize) { SetLastError(ERROR_FILENAME_EXCED_RANGE); return 0; }
	strcpy(out, current);
	return 1;
}

static int hd_resolve_existing(const WCHAR *path, char *out, size_t outSize)
{
	char native[HD_PATHBUF];
	if (!hd_win16_to_native_lex(path, native, sizeof(native))) { SetLastError(ERROR_INVALID_NAME); return 0; }
	return hd_resolve_native_existing_raw(native, out, outSize);
}

static int hd_resolve_for_create(const WCHAR *path, char *out, size_t outSize)
{
	char native[HD_PATHBUF];
	char parent[HD_PATHBUF], resolvedParent[HD_PATHBUF];
	char *slash;
	const char *leaf;
	if (!hd_win16_to_native_lex(path, native, sizeof(native))) { SetLastError(ERROR_INVALID_NAME); return 0; }
	if (hd_resolve_native_existing_raw(native, out, outSize)) return 1;
	if (GetLastError() != ERROR_FILE_NOT_FOUND && GetLastError() != ERROR_PATH_NOT_FOUND) return 0;
	strncpy(parent, native, sizeof(parent) - 1); parent[sizeof(parent) - 1] = 0;
	slash = strrchr(parent, '/');
	if (!slash) { strcpy(parent, "."); leaf = native; }
	else {
		leaf = slash + 1;
		if (slash == parent) slash[1] = 0; else *slash = 0;
	}
	if (!*leaf || strchr(leaf, '/')) { SetLastError(ERROR_INVALID_NAME); return 0; }
	if (!hd_resolve_native_existing_raw(parent, resolvedParent, sizeof(resolvedParent))) {
		SetLastError(ERROR_PATH_NOT_FOUND); return 0;
	}
	if (!strcmp(resolvedParent, "/")) {
		if (snprintf(out, outSize, "/%s", leaf) >= (int)outSize) return 0;
	} else {
		if (snprintf(out, outSize, "%s/%s", resolvedParent, leaf) >= (int)outSize) return 0;
	}
	return 1;
}

int MultiByteToWideChar(UINT codePage, DWORD flags, const char *src, int srcLen,
	WCHAR *dst, int dstChars)
{
	UINT n;
	(void)flags;
	if (!src) { SetLastError(ERROR_INVALID_PARAMETER); return 0; }
	if (codePage == CP_OEMCP)
		n = codecnv_sjistoucs2((UINT16 *)dst, dst ? (UINT)dstChars : 0, src,
			(srcLen < 0) ? (UINT)-1 : (UINT)srcLen);
	else
		n = codecnv_utf8toucs2((UINT16 *)dst, dst ? (UINT)dstChars : 0, src,
			(srcLen < 0) ? (UINT)-1 : (UINT)srcLen);
	if (!n) SetLastError(ERROR_INVALID_DATA); else SetLastError(ERROR_SUCCESS);
	return (int)n;
}

int WideCharToMultiByte(UINT codePage, DWORD flags, const WCHAR *src, int srcLen,
	char *dst, int dstBytes, const char *defaultChar, BOOL *usedDefaultChar)
{
	UINT n;
	(void)flags; (void)defaultChar;
	if (usedDefaultChar) *usedDefaultChar = FALSE;
	if (!src) { SetLastError(ERROR_INVALID_PARAMETER); return 0; }
	if (codePage == CP_OEMCP)
		n = codecnv_ucs2tosjis(dst, dst ? (UINT)dstBytes : 0, (const UINT16 *)src,
			(srcLen < 0) ? (UINT)-1 : (UINT)srcLen);
	else
		n = codecnv_ucs2toutf8(dst, dst ? (UINT)dstBytes : 0, (const UINT16 *)src,
			(srcLen < 0) ? (UINT)-1 : (UINT)srcLen);
	if (!n) SetLastError(ERROR_INVALID_DATA); else SetLastError(ERROR_SUCCESS);
	return (int)n;
}

BOOL PathIsRelativeA(const char *path)
{
	if (!path || !path[0]) return TRUE;
	if (path[0] == '/' || path[0] == '\\') return FALSE;
	if (((path[0] >= 'A' && path[0] <= 'Z') || (path[0] >= 'a' && path[0] <= 'z')) && path[1] == ':') return FALSE;
	return TRUE;
}

DWORD GetFullPathNameA(const char *path, DWORD size, char *out, char **filePart)
{
	char native[HD_PATHBUF], joined[HD_PATHBUF], cwd[HD_PATHBUF];
	size_t len;
	if (!path || !out || !size) { SetLastError(ERROR_INVALID_PARAMETER); return 0; }
	strncpy(native, path, sizeof(native) - 1); native[sizeof(native) - 1] = 0;
	hd_slashes_native(native);
	if (PathIsRelativeA(native)) {
		if (!getcwd(cwd, sizeof(cwd))) { SetLastError(hd_errno_error(errno)); return 0; }
		if (snprintf(joined, sizeof(joined), "%s/%s", cwd, native) >= (int)sizeof(joined)) {
			SetLastError(ERROR_FILENAME_EXCED_RANGE); return 0;
		}
	} else {
		strncpy(joined, native, sizeof(joined) - 1); joined[sizeof(joined) - 1] = 0;
	}
	if (!hd_normalize_native(joined, native, sizeof(native))) { SetLastError(ERROR_FILENAME_EXCED_RANGE); return 0; }
	hd_slashes_windows(native);
	len = strlen(native);
	if (len + 1 > size) return (DWORD)(len + 1);
	strcpy(out, native);
	if (filePart) {
		char *a = strrchr(out, '\\');
		*filePart = a ? a + 1 : out;
	}
	SetLastError(ERROR_SUCCESS);
	return (DWORD)len;
}

static int hd_is_sep16(WCHAR c) { return c == '/' || c == '\\'; }

BOOL PathCanonicalizeW(WCHAR *out, const WCHAR *path)
{
	WCHAR comps[MAX_PATH][1]; /* only used to force bounds at compile time */
	WCHAR tmp[MAX_PATH];
	WCHAR *parts[MAX_PATH / 2];
	size_t count = 0, i, pos = 0;
	WCHAR *p;
	int absolute;
	(void)comps;
	if (!out || !path || hd_wcslen(path) >= MAX_PATH) return FALSE;
	hd_wcscpy(tmp, path);
	absolute = hd_is_sep16(tmp[0]);
	p = tmp;
	while (*p) {
		WCHAR *start;
		while (hd_is_sep16(*p)) ++p;
		if (!*p) break;
		start = p;
		while (*p && !hd_is_sep16(*p)) ++p;
		if (*p) *p++ = 0;
		if (!hd_wcscmp(start, HD_W("."))) continue;
		if (!hd_wcscmp(start, HD_W(".."))) { if (count) --count; continue; }
		parts[count++] = start;
	}
	if (absolute) out[pos++] = '\\';
	for (i = 0; i < count; ++i) {
		size_t l = hd_wcslen(parts[i]);
		if (pos && out[pos - 1] != '\\') out[pos++] = '\\';
		if (pos + l >= MAX_PATH) return FALSE;
		memcpy(out + pos, parts[i], l * sizeof(WCHAR)); pos += l;
	}
	if (!pos) out[pos++] = absolute ? '\\' : '.';
	out[pos] = 0;
	return TRUE;
}

BOOL PathCombineW(WCHAR *out, const WCHAR *dir, const WCHAR *file)
{
	WCHAR tmp[MAX_PATH];
	size_t dl, fl;
	if (!out || !dir || !file) return FALSE;
	if (hd_is_sep16(file[0]) || (file[0] && file[1] == ':')) return PathCanonicalizeW(out, file);
	dl = hd_wcslen(dir); fl = hd_wcslen(file);
	if (dl + fl + 2 >= MAX_PATH) return FALSE;
	hd_wcscpy(tmp, dir);
	if (dl && !hd_is_sep16(tmp[dl - 1])) { tmp[dl++] = '\\'; tmp[dl] = 0; }
	hd_wcscpy(tmp + dl, file);
	return PathCanonicalizeW(out, tmp);
}

BOOL PathRemoveFileSpecW(WCHAR *path)
{
	WCHAR *p, *q;
	if (!path || !*path) return FALSE;
	p = hd_wcsrchr(path, '\\'); q = hd_wcsrchr(path, '/');
	if (q && (!p || q > p)) p = q;
	if (!p) return FALSE;
	if (p == path) p[1] = 0; else *p = 0;
	return TRUE;
}

static int hd_wildmatch(const WCHAR *s, const WCHAR *p)
{
	if (!hd_wcscmp(p, HD_W("*.*"))) p = HD_W("*");
	while (*p) {
		if (*p == '*') {
			while (*p == '*') ++p;
			if (!*p) return 1;
			while (*s) { if (hd_wildmatch(s, p)) return 1; ++s; }
			return hd_wildmatch(s, p);
		}
		if (*p == '?') { if (!*s) return 0; ++p; ++s; continue; }
		if (hd_upper16(*p) != hd_upper16(*s)) return 0;
		++p; ++s;
	}
	return *s == 0;
}

BOOL PathMatchSpecW(const WCHAR *name, const WCHAR *pattern)
{
	return (name && pattern && hd_wildmatch(name, pattern)) ? TRUE : FALSE;
}

static HDRVWIN_ATTRMETA *hd_attr_find(const char *path)
{
	HDRVWIN_ATTRMETA *m;
	for (m = s_attrmeta; m; m = m->next) if (!strcmp(m->path, path)) return m;
	return NULL;
}

static void hd_attr_setmeta(const char *path, DWORD attrs)
{
	HDRVWIN_ATTRMETA *m = hd_attr_find(path);
	DWORD keep = attrs & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_TEMPORARY);
	if (!m) {
		m = (HDRVWIN_ATTRMETA *)calloc(1, sizeof(*m));
		if (!m) return;
		m->path = strdup(path);
		if (!m->path) { free(m); return; }
		m->next = s_attrmeta; s_attrmeta = m;
	}
	m->attrs = keep;
}

static void hd_attr_remove(const char *path)
{
	HDRVWIN_ATTRMETA **pp = &s_attrmeta;
	while (*pp) {
		if (!strcmp((*pp)->path, path)) {
			HDRVWIN_ATTRMETA *m = *pp; *pp = m->next; free(m->path); free(m); return;
		}
		pp = &(*pp)->next;
	}
}

static void hd_attr_move(const char *oldp, const char *newp)
{
	HDRVWIN_ATTRMETA *m = hd_attr_find(oldp);
	if (m) { char *n = strdup(newp); if (n) { free(m->path); m->path = n; } }
}

static void hd_unix_to_filetime(time_t sec, long nsec, FILETIME *ft)
{
	uint64_t ticks;
	if (!ft) return;
	if (sec < (time_t)-HD_EPOCH_DIFF) sec = (time_t)-HD_EPOCH_DIFF;
	ticks = ((uint64_t)((int64_t)sec + (int64_t)HD_EPOCH_DIFF) * HD_TICKS_PER_SEC) + (uint64_t)(nsec < 0 ? 0 : nsec / 100);
	ft->dwLowDateTime = (DWORD)ticks;
	ft->dwHighDateTime = (DWORD)(ticks >> 32);
}

static int64_t hd_filetime_seconds(const FILETIME *ft)
{
	uint64_t ticks = ((uint64_t)ft->dwHighDateTime << 32) | ft->dwLowDateTime;
	return (int64_t)(ticks / HD_TICKS_PER_SEC) - (int64_t)HD_EPOCH_DIFF;
}

static DWORD hd_attrs_from_stat(const char *path, const struct stat *st, int isLink)
{
	DWORD a = 0;
	const char *base = strrchr(path, '/');
	HDRVWIN_ATTRMETA *m;
	base = base ? base + 1 : path;
	if (S_ISDIR(st->st_mode)) a |= FILE_ATTRIBUTE_DIRECTORY;
	else a |= FILE_ATTRIBUTE_ARCHIVE;
	if (!(st->st_mode & S_IWUSR)) a |= FILE_ATTRIBUTE_READONLY;
	if (base[0] == '.' && base[1]) a |= FILE_ATTRIBUTE_HIDDEN;
	if (isLink) a |= FILE_ATTRIBUTE_REPARSE_POINT;
	m = hd_attr_find(path);
	if (m) a = (a & ~(FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_TEMPORARY)) | m->attrs;
	if (a == 0) a = FILE_ATTRIBUTE_NORMAL;
	return a;
}

static int hd_stat_path(const WCHAR *path, char *resolved, size_t resolvedSize, struct stat *st, int *isLink)
{
	struct stat lst;
	if (!hd_resolve_existing(path, resolved, resolvedSize)) return 0;
	if (lstat(resolved, &lst) != 0) { SetLastError(hd_errno_error(errno)); return 0; }
	if (isLink) *isLink = S_ISLNK(lst.st_mode) ? 1 : 0;
	if (stat(resolved, st) != 0) *st = lst;
	return 1;
}

DWORD GetFileAttributesW(const WCHAR *path)
{
	char resolved[HD_PATHBUF]; struct stat st; int link;
	if (!hd_stat_path(path, resolved, sizeof(resolved), &st, &link)) return INVALID_FILE_ATTRIBUTES;
	SetLastError(ERROR_SUCCESS);
	return hd_attrs_from_stat(resolved, &st, link);
}

BOOL GetFileAttributesExW(const WCHAR *path, int infoLevel, WIN32_FILE_ATTRIBUTE_DATA *data)
{
	char resolved[HD_PATHBUF]; struct stat st; int link;
	(void)infoLevel;
	if (!data || !hd_stat_path(path, resolved, sizeof(resolved), &st, &link)) return FALSE;
	memset(data, 0, sizeof(*data));
	data->dwFileAttributes = hd_attrs_from_stat(resolved, &st, link);
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
	hd_unix_to_filetime(st.st_birthtimespec.tv_sec, st.st_birthtimespec.tv_nsec, &data->ftCreationTime);
#else
	hd_unix_to_filetime(st.st_mtime, 0, &data->ftCreationTime);
#endif
	hd_unix_to_filetime(st.st_atime, 0, &data->ftLastAccessTime);
	hd_unix_to_filetime(st.st_mtime, 0, &data->ftLastWriteTime);
	data->nFileSizeLow = (DWORD)(uint64_t)st.st_size;
	data->nFileSizeHigh = (DWORD)((uint64_t)st.st_size >> 32);
	SetLastError(ERROR_SUCCESS); return TRUE;
}

BOOL SetFileAttributesW(const WCHAR *path, DWORD attributes)
{
	char resolved[HD_PATHBUF]; struct stat st; int link; mode_t mode;
	if (!hd_stat_path(path, resolved, sizeof(resolved), &st, &link)) return FALSE;
	mode = st.st_mode;
	if (attributes & FILE_ATTRIBUTE_READONLY) mode &= ~S_IWUSR; else mode |= S_IWUSR;
	if (chmod(resolved, mode) != 0) { SetLastError(hd_errno_error(errno)); return FALSE; }
	hd_attr_setmeta(resolved, attributes);
	SetLastError(ERROR_SUCCESS); return TRUE;
}

BOOL CreateDirectoryW(const WCHAR *path, void *securityAttributes)
{
	char native[HD_PATHBUF]; struct stat st;
	(void)securityAttributes;
	if (!hd_resolve_for_create(path, native, sizeof(native))) return FALSE;
	if (lstat(native, &st) == 0) { SetLastError(ERROR_ALREADY_EXISTS); return FALSE; }
	if (mkdir(native, 0777) != 0) { SetLastError(hd_errno_error(errno)); return FALSE; }
	SetLastError(ERROR_SUCCESS); return TRUE;
}

static int hd_same_object(HDRVWIN_HANDLE *h, const struct stat *st, const char *path)
{
	if (h->type != HD_HANDLE_FILE) return 0;
	if (st && h->dev == st->st_dev && h->ino == st->st_ino) return 1;
	return path && h->path && !strcmp(h->path, path);
}

static int hd_delete_share_ok(const struct stat *st, const char *path)
{
	HDRVWIN_HANDLE *h;
	for (h = s_handles; h; h = h->next)
		if (hd_same_object(h, st, path) && !(h->shareMode & FILE_SHARE_DELETE)) return 0;
	return 1;
}

BOOL RemoveDirectoryW(const WCHAR *path)
{
	char resolved[HD_PATHBUF]; struct stat st; int link;
	if (!hd_stat_path(path, resolved, sizeof(resolved), &st, &link)) return FALSE;
	if (!S_ISDIR(st.st_mode)) { SetLastError(ERROR_PATH_NOT_FOUND); return FALSE; }
	if (!hd_delete_share_ok(&st, resolved)) { SetLastError(ERROR_SHARING_VIOLATION); return FALSE; }
	if (rmdir(resolved) != 0) { SetLastError(hd_errno_error(errno)); return FALSE; }
	hd_attr_remove(resolved); SetLastError(ERROR_SUCCESS); return TRUE;
}

BOOL DeleteFileW(const WCHAR *path)
{
	char resolved[HD_PATHBUF]; struct stat st; int link;
	if (!hd_stat_path(path, resolved, sizeof(resolved), &st, &link)) return FALSE;
	if (S_ISDIR(st.st_mode)) { SetLastError(ERROR_ACCESS_DENIED); return FALSE; }
	if (!hd_delete_share_ok(&st, resolved)) { SetLastError(ERROR_SHARING_VIOLATION); return FALSE; }
	if (unlink(resolved) != 0) { SetLastError(hd_errno_error(errno)); return FALSE; }
	hd_attr_remove(resolved); SetLastError(ERROR_SUCCESS); return TRUE;
}

static int hd_copy_file(const char *src, const char *dst, int replace)
{
	int in = -1, out = -1; char buf[65536]; ssize_t r;
	struct stat st;
	if ((in = open(src, O_RDONLY)) < 0) return 0;
	if (fstat(in, &st) != 0) { close(in); return 0; }
	out = open(dst, O_WRONLY | O_CREAT | O_TRUNC | (replace ? 0 : O_EXCL), st.st_mode & 0777);
	if (out < 0) { close(in); return 0; }
	while ((r = read(in, buf, sizeof(buf))) > 0) {
		char *p = buf; ssize_t left = r;
		while (left > 0) { ssize_t w = write(out, p, (size_t)left); if (w <= 0) goto err; p += w; left -= w; }
	}
	if (r < 0) goto err;
	close(in); if (close(out) != 0) return 0; return 1;
err:
	close(in); close(out); unlink(dst); return 0;
}

/* Resolve the parent as Windows would, but preserve the requested leaf spelling.
 * This is required for case-only rename on case-sensitive host filesystems. */
static int hd_resolve_requested_leaf(const WCHAR *path, char *out, size_t outSize)
{
	char native[HD_PATHBUF], parent[HD_PATHBUF], resolvedParent[HD_PATHBUF];
	char *slash;
	const char *leaf;
	if (!hd_win16_to_native_lex(path, native, sizeof(native))) { SetLastError(ERROR_INVALID_NAME); return 0; }
	strncpy(parent, native, sizeof(parent) - 1); parent[sizeof(parent) - 1] = 0;
	slash = strrchr(parent, '/');
	if (!slash) { strcpy(parent, "."); leaf = native; }
	else {
		leaf = slash + 1;
		if (slash == parent) slash[1] = 0; else *slash = 0;
	}
	if (!*leaf || strchr(leaf, '/')) { SetLastError(ERROR_INVALID_NAME); return 0; }
	if (!hd_resolve_native_existing_raw(parent, resolvedParent, sizeof(resolvedParent))) {
		SetLastError(ERROR_PATH_NOT_FOUND); return 0;
	}
	if (!strcmp(resolvedParent, "/")) {
		if (snprintf(out, outSize, "/%s", leaf) >= (int)outSize) { SetLastError(ERROR_FILENAME_EXCED_RANGE); return 0; }
	} else {
		if (snprintf(out, outSize, "%s/%s", resolvedParent, leaf) >= (int)outSize) { SetLastError(ERROR_FILENAME_EXCED_RANGE); return 0; }
	}
	return 1;
}

static void hd_update_open_paths_after_rename(const struct stat *st, const char *newPath)
{
	HDRVWIN_HANDLE *h;
	for (h = s_handles; h; h = h->next) {
		char *copy;
		if (h->type != HD_HANDLE_FILE || h->dev != st->st_dev || h->ino != st->st_ino) continue;
		copy = strdup(newPath);
		if (copy) { free(h->path); h->path = copy; }
	}
}

BOOL MoveFileExW(const WCHAR *oldPath, const WCHAR *newPath, DWORD flags)
{
	char oldp[HD_PATHBUF], newp[HD_PATHBUF], requested[HD_PATHBUF], resolved[HD_PATHBUF];
	struct stat oldst, newst; int link;
	int newExists = 0;
	if (!hd_stat_path(oldPath, oldp, sizeof(oldp), &oldst, &link)) return FALSE;
	if (!hd_delete_share_ok(&oldst, oldp)) { SetLastError(ERROR_SHARING_VIOLATION); return FALSE; }
	if (!hd_resolve_requested_leaf(newPath, requested, sizeof(requested))) return FALSE;

	/* Prefer exact requested spelling.  Otherwise resolve case-insensitively. If
	 * that unique case-insensitive match is the source itself, keep the requested
	 * leaf so POSIX rename() performs the Windows-compatible case-only rename. */
	if (lstat(requested, &newst) == 0) {
		strncpy(newp, requested, sizeof(newp) - 1); newp[sizeof(newp) - 1] = 0;
		newExists = 1;
	} else if (hd_resolve_existing(newPath, resolved, sizeof(resolved))) {
		if (lstat(resolved, &newst) != 0) { SetLastError(hd_errno_error(errno)); return FALSE; }
		if (newst.st_dev == oldst.st_dev && newst.st_ino == oldst.st_ino && strcmp(resolved, requested) != 0) {
			strncpy(newp, requested, sizeof(newp) - 1); newp[sizeof(newp) - 1] = 0;
			newExists = 0;
		} else {
			strncpy(newp, resolved, sizeof(newp) - 1); newp[sizeof(newp) - 1] = 0;
			newExists = 1;
		}
	} else {
		if (GetLastError() != ERROR_FILE_NOT_FOUND && GetLastError() != ERROR_PATH_NOT_FOUND) return FALSE;
		strncpy(newp, requested, sizeof(newp) - 1); newp[sizeof(newp) - 1] = 0;
	}
	if (newExists) {
		if (newst.st_dev == oldst.st_dev && newst.st_ino == oldst.st_ino && !strcmp(oldp, newp)) {
			SetLastError(ERROR_ALREADY_EXISTS); return FALSE;
		}
		if (!(flags & MOVEFILE_REPLACE_EXISTING)) { SetLastError(ERROR_ALREADY_EXISTS); return FALSE; }
		if (!hd_delete_share_ok(&newst, newp)) { SetLastError(ERROR_SHARING_VIOLATION); return FALSE; }
	}
	if (rename(oldp, newp) == 0) {
		hd_attr_move(oldp, newp);
		hd_update_open_paths_after_rename(&oldst, newp);
		SetLastError(ERROR_SUCCESS); return TRUE;
	}
	if (errno == EXDEV && (flags & MOVEFILE_COPY_ALLOWED) && S_ISREG(oldst.st_mode)) {
		if (hd_copy_file(oldp, newp, (flags & MOVEFILE_REPLACE_EXISTING) != 0) && unlink(oldp) == 0) {
			hd_attr_move(oldp, newp); SetLastError(ERROR_SUCCESS); return TRUE;
		}
	}
	SetLastError(hd_errno_error(errno)); return FALSE;
}

BOOL MoveFileW(const WCHAR *oldPath, const WCHAR *newPath) { return MoveFileExW(oldPath, newPath, 0); }

static int hd_access_read(DWORD a) { return (a & (GENERIC_READ | GENERIC_EXECUTE)) != 0; }
static int hd_access_write(DWORD a) { return (a & GENERIC_WRITE) != 0; }
static int hd_access_delete(DWORD a) { return (a & DELETE) != 0; }

static int hd_share_compatible(const struct stat *st, const char *path, DWORD access, DWORD share)
{
	HDRVWIN_HANDLE *h;
	for (h = s_handles; h; h = h->next) {
		if (!hd_same_object(h, st, path)) continue;
		if (hd_access_read(access) && !(h->shareMode & FILE_SHARE_READ)) return 0;
		if (hd_access_write(access) && !(h->shareMode & FILE_SHARE_WRITE)) return 0;
		if (hd_access_delete(access) && !(h->shareMode & FILE_SHARE_DELETE)) return 0;
		if (hd_access_read(h->desiredAccess) && !(share & FILE_SHARE_READ)) return 0;
		if (hd_access_write(h->desiredAccess) && !(share & FILE_SHARE_WRITE)) return 0;
		if (hd_access_delete(h->desiredAccess) && !(share & FILE_SHARE_DELETE)) return 0;
	}
	return 1;
}

static void hd_register_handle(HDRVWIN_HANDLE *h) { h->next = s_handles; s_handles = h; }
static void hd_unregister_handle(HDRVWIN_HANDLE *h)
{
	HDRVWIN_HANDLE **pp = &s_handles;
	while (*pp) { if (*pp == h) { *pp = h->next; return; } pp = &(*pp)->next; }
}

HANDLE CreateFileW(const WCHAR *path, DWORD desiredAccess, DWORD shareMode,
	void *securityAttributes, DWORD creationDisposition, DWORD flagsAndAttributes,
	HANDLE templateFile)
{
	char native[HD_PATHBUF]; struct stat st; int exists, oflags, fd, isdir = 0;
	HDRVWIN_HANDLE *h; DWORD successError = ERROR_SUCCESS;
	(void)securityAttributes; (void)templateFile;
	if (!path) { SetLastError(ERROR_INVALID_PARAMETER); return INVALID_HANDLE_VALUE; }
	exists = hd_resolve_existing(path, native, sizeof(native));
	if (exists) {
		if (stat(native, &st) != 0) { SetLastError(hd_errno_error(errno)); return INVALID_HANDLE_VALUE; }
		isdir = S_ISDIR(st.st_mode);
	} else {
		if (creationDisposition == OPEN_EXISTING || creationDisposition == TRUNCATE_EXISTING) {
			if (GetLastError() == ERROR_FILE_NOT_FOUND) SetLastError(ERROR_FILE_NOT_FOUND);
			return INVALID_HANDLE_VALUE;
		}
		if (!hd_resolve_for_create(path, native, sizeof(native))) return INVALID_HANDLE_VALUE;
		memset(&st, 0, sizeof(st));
	}
	if (exists && creationDisposition == CREATE_NEW) { SetLastError(ERROR_FILE_EXISTS); return INVALID_HANDLE_VALUE; }
	if (isdir && !(flagsAndAttributes & FILE_FLAG_BACKUP_SEMANTICS)) { SetLastError(ERROR_ACCESS_DENIED); return INVALID_HANDLE_VALUE; }
	if (exists && !hd_share_compatible(&st, native, desiredAccess, shareMode)) { SetLastError(ERROR_SHARING_VIOLATION); return INVALID_HANDLE_VALUE; }

	if (isdir) {
		oflags = O_RDONLY | O_DIRECTORY;
	} else if (hd_access_write(desiredAccess) && hd_access_read(desiredAccess)) oflags = O_RDWR;
	else if (hd_access_write(desiredAccess)) oflags = O_WRONLY;
	else oflags = O_RDONLY;

	switch (creationDisposition) {
	case CREATE_NEW: oflags |= O_CREAT | O_EXCL; break;
	case CREATE_ALWAYS: oflags |= O_CREAT | O_TRUNC; if (exists) successError = ERROR_ALREADY_EXISTS; break;
	case OPEN_EXISTING: break;
	case OPEN_ALWAYS: oflags |= O_CREAT; if (exists) successError = ERROR_ALREADY_EXISTS; break;
	case TRUNCATE_EXISTING: oflags |= O_TRUNC; break;
	default: SetLastError(ERROR_INVALID_PARAMETER); return INVALID_HANDLE_VALUE;
	}
	fd = open(native, oflags, 0666);
	if (fd < 0) { SetLastError(hd_errno_error(errno)); return INVALID_HANDLE_VALUE; }
	if (fstat(fd, &st) != 0) { SetLastError(hd_errno_error(errno)); close(fd); return INVALID_HANDLE_VALUE; }
	if (!hd_share_compatible(&st, native, desiredAccess, shareMode)) { close(fd); SetLastError(ERROR_SHARING_VIOLATION); return INVALID_HANDLE_VALUE; }
	h = (HDRVWIN_HANDLE *)calloc(1, sizeof(*h));
	if (!h) { close(fd); SetLastError(ERROR_NOT_ENOUGH_MEMORY); return INVALID_HANDLE_VALUE; }
	h->type = HD_HANDLE_FILE; h->fd = fd; h->desiredAccess = desiredAccess; h->shareMode = shareMode;
	h->dev = st.st_dev; h->ino = st.st_ino; h->path = strdup(native);
	if (!h->path) { close(fd); free(h); SetLastError(ERROR_NOT_ENOUGH_MEMORY); return INVALID_HANDLE_VALUE; }
	hd_register_handle(h);
	if (!exists && (creationDisposition == CREATE_NEW || creationDisposition == CREATE_ALWAYS || creationDisposition == OPEN_ALWAYS))
		hd_attr_setmeta(native, flagsAndAttributes);
	SetLastError(successError); return h;
}

BOOL CloseHandle(HANDLE handle)
{
	HDRVWIN_HANDLE *h = (HDRVWIN_HANDLE *)handle; HDRVWIN_LOCK *l, *n;
	if (!h || h == INVALID_HANDLE_VALUE || h->type != HD_HANDLE_FILE) { SetLastError(ERROR_INVALID_HANDLE); return FALSE; }
	hd_unregister_handle(h);
	for (l = h->locks; l; l = n) { n = l->next; free(l); }
	if (h->fd >= 0) close(h->fd);
	free(h->path); free(h); SetLastError(ERROR_SUCCESS); return TRUE;
}

static int hd_range_overlap(uint64_t a, uint64_t al, uint64_t b, uint64_t bl);

static int hd_io_lock_ok(HDRVWIN_HANDLE *owner, uint64_t off, uint64_t len)
{
	HDRVWIN_HANDLE *h;
	HDRVWIN_LOCK *l;
	if (len == 0) return 1;
	for (h = s_handles; h; h = h->next) {
		if (h == owner || h->type != HD_HANDLE_FILE ||
			h->dev != owner->dev || h->ino != owner->ino) continue;
		for (l = h->locks; l; l = l->next) {
			if (hd_range_overlap(off, len, l->off, l->len)) return 0;
		}
	}
	return 1;
}

BOOL ReadFile(HANDLE handle, void *buffer, DWORD bytesToRead, DWORD *bytesRead, void *overlapped)
{
	HDRVWIN_HANDLE *h = (HDRVWIN_HANDLE *)handle; ssize_t r; off_t pos;
	(void)overlapped; if (bytesRead) *bytesRead = 0;
	if (!h || h == INVALID_HANDLE_VALUE || h->type != HD_HANDLE_FILE) { SetLastError(ERROR_INVALID_HANDLE); return FALSE; }
	pos = lseek(h->fd, 0, SEEK_CUR);
	if (pos == (off_t)-1) { SetLastError(hd_errno_error(errno)); return FALSE; }
	if (!hd_io_lock_ok(h, (uint64_t)pos, (uint64_t)bytesToRead)) { SetLastError(ERROR_LOCK_VIOLATION); return FALSE; }
	r = read(h->fd, buffer, bytesToRead);
	if (r < 0) { SetLastError(hd_errno_error(errno)); return FALSE; }
	if (bytesRead) *bytesRead = (DWORD)r; SetLastError(ERROR_SUCCESS); return TRUE;
}

BOOL WriteFile(HANDLE handle, const void *buffer, DWORD bytesToWrite, DWORD *bytesWritten, void *overlapped)
{
	HDRVWIN_HANDLE *h = (HDRVWIN_HANDLE *)handle; ssize_t r; off_t pos;
	(void)overlapped; if (bytesWritten) *bytesWritten = 0;
	if (!h || h == INVALID_HANDLE_VALUE || h->type != HD_HANDLE_FILE) { SetLastError(ERROR_INVALID_HANDLE); return FALSE; }
	pos = lseek(h->fd, 0, SEEK_CUR);
	if (pos == (off_t)-1) { SetLastError(hd_errno_error(errno)); return FALSE; }
	if (!hd_io_lock_ok(h, (uint64_t)pos, (uint64_t)bytesToWrite)) { SetLastError(ERROR_LOCK_VIOLATION); return FALSE; }
	r = write(h->fd, buffer, bytesToWrite);
	if (r < 0) { SetLastError(hd_errno_error(errno)); return FALSE; }
	if (bytesWritten) *bytesWritten = (DWORD)r; SetLastError(ERROR_SUCCESS); return TRUE;
}

static int hd_whence(DWORD m) { return m == FILE_BEGIN ? SEEK_SET : (m == FILE_CURRENT ? SEEK_CUR : (m == FILE_END ? SEEK_END : -1)); }

DWORD SetFilePointer(HANDLE handle, LONG distanceLow, LONG *distanceHigh, DWORD moveMethod)
{
	HDRVWIN_HANDLE *h = (HDRVWIN_HANDLE *)handle; int wh = hd_whence(moveMethod); int64_t d; off_t p;
	if (!h || h == INVALID_HANDLE_VALUE || h->type != HD_HANDLE_FILE || wh < 0) { SetLastError(ERROR_INVALID_PARAMETER); return INVALID_SET_FILE_POINTER; }
	d = (int64_t)(uint32_t)distanceLow;
	if (distanceHigh) d |= ((int64_t)*distanceHigh << 32); else d = (int32_t)distanceLow;
	p = lseek(h->fd, (off_t)d, wh);
	if (p == (off_t)-1) { SetLastError(hd_errno_error(errno)); return INVALID_SET_FILE_POINTER; }
	if (distanceHigh) *distanceHigh = (LONG)((uint64_t)p >> 32);
	SetLastError(ERROR_SUCCESS); return (DWORD)(uint64_t)p;
}

BOOL SetFilePointerEx(HANDLE handle, LARGE_INTEGER distance, LARGE_INTEGER *newPosition, DWORD moveMethod)
{
	HDRVWIN_HANDLE *h = (HDRVWIN_HANDLE *)handle; int wh = hd_whence(moveMethod); off_t p;
	if (!h || h == INVALID_HANDLE_VALUE || h->type != HD_HANDLE_FILE || wh < 0) { SetLastError(ERROR_INVALID_PARAMETER); return FALSE; }
	p = lseek(h->fd, (off_t)distance.QuadPart, wh);
	if (p == (off_t)-1) { SetLastError(hd_errno_error(errno)); return FALSE; }
	if (newPosition) newPosition->QuadPart = (int64_t)p; SetLastError(ERROR_SUCCESS); return TRUE;
}

BOOL SetEndOfFile(HANDLE handle)
{
	HDRVWIN_HANDLE *h = (HDRVWIN_HANDLE *)handle; off_t p;
	if (!h || h == INVALID_HANDLE_VALUE || h->type != HD_HANDLE_FILE) { SetLastError(ERROR_INVALID_HANDLE); return FALSE; }
	p = lseek(h->fd, 0, SEEK_CUR);
	if (p == (off_t)-1 || ftruncate(h->fd, p) != 0) { SetLastError(hd_errno_error(errno)); return FALSE; }
	SetLastError(ERROR_SUCCESS); return TRUE;
}

BOOL FlushFileBuffers(HANDLE handle)
{
	HDRVWIN_HANDLE *h = (HDRVWIN_HANDLE *)handle;
	if (!h || h == INVALID_HANDLE_VALUE || h->type != HD_HANDLE_FILE) { SetLastError(ERROR_INVALID_HANDLE); return FALSE; }
	if (fsync(h->fd) != 0) { SetLastError(hd_errno_error(errno)); return FALSE; }
	SetLastError(ERROR_SUCCESS); return TRUE;
}

static int hd_range_overlap(uint64_t a, uint64_t al, uint64_t b, uint64_t bl)
{
	uint64_t ae = al ? a + al : UINT64_MAX, be = bl ? b + bl : UINT64_MAX;
	if (ae < a) ae = UINT64_MAX; if (be < b) be = UINT64_MAX;
	return a < be && b < ae;
}

BOOL LockFile(HANDLE handle, DWORD offsetLow, DWORD offsetHigh, DWORD bytesLow, DWORD bytesHigh)
{
	HDRVWIN_HANDLE *h = (HDRVWIN_HANDLE *)handle, *o; HDRVWIN_LOCK *l, *n;
	uint64_t off = ((uint64_t)offsetHigh << 32) | offsetLow, len = ((uint64_t)bytesHigh << 32) | bytesLow;
	if (!h || h == INVALID_HANDLE_VALUE || h->type != HD_HANDLE_FILE) { SetLastError(ERROR_INVALID_HANDLE); return FALSE; }
	for (o = s_handles; o; o = o->next) if (o->type == HD_HANDLE_FILE && o->dev == h->dev && o->ino == h->ino)
		for (l = o->locks; l; l = l->next) if (hd_range_overlap(off, len, l->off, l->len)) { SetLastError(ERROR_LOCK_VIOLATION); return FALSE; }
	n = (HDRVWIN_LOCK *)malloc(sizeof(*n)); if (!n) { SetLastError(ERROR_NOT_ENOUGH_MEMORY); return FALSE; }
	n->off = off; n->len = len; n->next = h->locks; h->locks = n; SetLastError(ERROR_SUCCESS); return TRUE;
}

BOOL UnlockFile(HANDLE handle, DWORD offsetLow, DWORD offsetHigh, DWORD bytesLow, DWORD bytesHigh)
{
	HDRVWIN_HANDLE *h = (HDRVWIN_HANDLE *)handle; HDRVWIN_LOCK **pp, *l;
	uint64_t off = ((uint64_t)offsetHigh << 32) | offsetLow, len = ((uint64_t)bytesHigh << 32) | bytesLow;
	if (!h || h == INVALID_HANDLE_VALUE || h->type != HD_HANDLE_FILE) { SetLastError(ERROR_INVALID_HANDLE); return FALSE; }
	pp = &h->locks; while ((l = *pp) != NULL) { if (l->off == off && l->len == len) { *pp = l->next; free(l); SetLastError(ERROR_SUCCESS); return TRUE; } pp = &l->next; }
	SetLastError(ERROR_LOCK_VIOLATION); return FALSE;
}

BOOL GetFileInformationByHandle(HANDLE handle, BY_HANDLE_FILE_INFORMATION *info)
{
	HDRVWIN_HANDLE *h = (HDRVWIN_HANDLE *)handle; struct stat st;
	if (!h || h == INVALID_HANDLE_VALUE || h->type != HD_HANDLE_FILE || !info) { SetLastError(ERROR_INVALID_HANDLE); return FALSE; }
	if (fstat(h->fd, &st) != 0) { SetLastError(hd_errno_error(errno)); return FALSE; }
	memset(info, 0, sizeof(*info));
	info->dwFileAttributes = hd_attrs_from_stat(h->path ? h->path : "", &st, 0);
	hd_unix_to_filetime(st.st_mtime, 0, &info->ftCreationTime);
	hd_unix_to_filetime(st.st_atime, 0, &info->ftLastAccessTime);
	hd_unix_to_filetime(st.st_mtime, 0, &info->ftLastWriteTime);
	info->dwVolumeSerialNumber = (DWORD)(uint64_t)st.st_dev;
	info->nFileSizeLow = (DWORD)(uint64_t)st.st_size; info->nFileSizeHigh = (DWORD)((uint64_t)st.st_size >> 32);
	info->nNumberOfLinks = (DWORD)st.st_nlink;
	info->nFileIndexLow = (DWORD)(uint64_t)st.st_ino; info->nFileIndexHigh = (DWORD)((uint64_t)st.st_ino >> 32);
	SetLastError(ERROR_SUCCESS); return TRUE;
}

BOOL GetFileSizeEx(HANDLE handle, LARGE_INTEGER *size)
{
	HDRVWIN_HANDLE *h = (HDRVWIN_HANDLE *)handle; struct stat st;
	if (!h || h == INVALID_HANDLE_VALUE || h->type != HD_HANDLE_FILE || !size) { SetLastError(ERROR_INVALID_HANDLE); return FALSE; }
	if (fstat(h->fd, &st) != 0) { SetLastError(hd_errno_error(errno)); return FALSE; }
	size->QuadPart = (int64_t)st.st_size; SetLastError(ERROR_SUCCESS); return TRUE;
}

BOOL GetFileTime(HANDLE handle, FILETIME *creation, FILETIME *access, FILETIME *writeft)
{
	HDRVWIN_HANDLE *h = (HDRVWIN_HANDLE *)handle; struct stat st;
	if (!h || h == INVALID_HANDLE_VALUE || h->type != HD_HANDLE_FILE) { SetLastError(ERROR_INVALID_HANDLE); return FALSE; }
	if (fstat(h->fd, &st) != 0) { SetLastError(hd_errno_error(errno)); return FALSE; }
	if (creation) hd_unix_to_filetime(st.st_mtime, 0, creation);
	if (access) hd_unix_to_filetime(st.st_atime, 0, access);
	if (writeft) hd_unix_to_filetime(st.st_mtime, 0, writeft);
	SetLastError(ERROR_SUCCESS); return TRUE;
}

BOOL SetFileTime(HANDLE handle, const FILETIME *creation, const FILETIME *access, const FILETIME *writeft)
{
	HDRVWIN_HANDLE *h = (HDRVWIN_HANDLE *)handle; struct stat st; struct timespec ts[2];
	(void)creation;
	if (!h || h == INVALID_HANDLE_VALUE || h->type != HD_HANDLE_FILE) { SetLastError(ERROR_INVALID_HANDLE); return FALSE; }
	if (fstat(h->fd, &st) != 0) { SetLastError(hd_errno_error(errno)); return FALSE; }
	ts[0].tv_sec = access ? (time_t)hd_filetime_seconds(access) : st.st_atime; ts[0].tv_nsec = 0;
	ts[1].tv_sec = writeft ? (time_t)hd_filetime_seconds(writeft) : st.st_mtime; ts[1].tv_nsec = 0;
	if (futimens(h->fd, ts) != 0) { SetLastError(hd_errno_error(errno)); return FALSE; }
	SetLastError(ERROR_SUCCESS); return TRUE;
}

static int hd_fill_find_data_native(const char *path, const char *name, WIN32_FIND_DATAW *data)
{
	struct stat lst, st; int link;
	if (lstat(path, &lst) != 0) return 0;
	link = S_ISLNK(lst.st_mode);
	if (stat(path, &st) != 0) st = lst;
	memset(data, 0, sizeof(*data));
	data->dwFileAttributes = hd_attrs_from_stat(path, &st, link);
	hd_unix_to_filetime(st.st_mtime, 0, &data->ftCreationTime);
	hd_unix_to_filetime(st.st_atime, 0, &data->ftLastAccessTime);
	hd_unix_to_filetime(st.st_mtime, 0, &data->ftLastWriteTime);
	data->nFileSizeLow = (DWORD)(uint64_t)st.st_size; data->nFileSizeHigh = (DWORD)((uint64_t)st.st_size >> 32);
	if (!hd_utf8_to_u16(name, data->cFileName, _countof(data->cFileName))) return 0;
	data->cAlternateFileName[0] = 0;
	return 1;
}

static int hd_find_next_internal(HDRVWIN_HANDLE *h, WIN32_FIND_DATAW *data)
{
	struct dirent *de;
	if (h->find.single) {
		if (h->find.done) { SetLastError(ERROR_NO_MORE_FILES); return 0; }
		h->find.done = 1;
		return hd_fill_find_data_native(h->find.directory, strrchr(h->find.directory, '/') ? strrchr(h->find.directory, '/') + 1 : h->find.directory, data);
	}
	while ((de = readdir(h->find.dir)) != NULL) {
		WCHAR wname[MAX_PATH]; char full[HD_PATHBUF];
		if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) continue;
		if (!hd_utf8_to_u16(de->d_name, wname, _countof(wname))) continue;
		if (!hd_wildmatch(wname, h->find.pattern)) continue;
		if (!strcmp(h->find.directory, "/")) snprintf(full, sizeof(full), "/%s", de->d_name);
		else snprintf(full, sizeof(full), "%s/%s", h->find.directory, de->d_name);
		if (hd_fill_find_data_native(full, de->d_name, data)) { SetLastError(ERROR_SUCCESS); return 1; }
	}
	SetLastError(ERROR_NO_MORE_FILES); return 0;
}

HANDLE FindFirstFileW(const WCHAR *pattern, WIN32_FIND_DATAW *data)
{
	char native[HD_PATHBUF], parent[HD_PATHBUF], resolved[HD_PATHBUF]; char *slash; const char *leaf;
	WCHAR wleaf[MAX_PATH]; HDRVWIN_HANDLE *h; int wildcard;
	if (!pattern || !data || !hd_win16_to_native_lex(pattern, native, sizeof(native))) { SetLastError(ERROR_INVALID_NAME); return INVALID_HANDLE_VALUE; }
	slash = strrchr(native, '/');
	if (!slash) { strcpy(parent, "."); leaf = native; }
	else { leaf = slash + 1; strncpy(parent, native, sizeof(parent) - 1); parent[sizeof(parent) - 1] = 0; if (slash == native) parent[1] = 0; else parent[slash - native] = 0; }
	wildcard = strchr(leaf, '*') != NULL || strchr(leaf, '?') != NULL;
	h = (HDRVWIN_HANDLE *)calloc(1, sizeof(*h)); if (!h) { SetLastError(ERROR_NOT_ENOUGH_MEMORY); return INVALID_HANDLE_VALUE; }
	h->type = HD_HANDLE_FIND; h->fd = -1;
	if (!wildcard) {
		if (!hd_resolve_native_existing_raw(native, resolved, sizeof(resolved))) { free(h); return INVALID_HANDLE_VALUE; }
		strncpy(h->find.directory, resolved, sizeof(h->find.directory) - 1); h->find.single = 1; h->find.done = 0;
		if (!hd_find_next_internal(h, data)) { free(h); return INVALID_HANDLE_VALUE; }
	} else {
		if (!hd_resolve_native_existing_raw(parent, resolved, sizeof(resolved))) { free(h); return INVALID_HANDLE_VALUE; }
		if (!hd_utf8_to_u16(leaf, wleaf, _countof(wleaf))) { free(h); SetLastError(ERROR_INVALID_NAME); return INVALID_HANDLE_VALUE; }
		strncpy(h->find.directory, resolved, sizeof(h->find.directory) - 1); hd_wcsncpy(h->find.pattern, wleaf, _countof(h->find.pattern) - 1);
		h->find.dir = opendir(resolved); if (!h->find.dir) { SetLastError(hd_errno_error(errno)); free(h); return INVALID_HANDLE_VALUE; }
		if (!hd_find_next_internal(h, data)) { closedir(h->find.dir); free(h); return INVALID_HANDLE_VALUE; }
	}
	SetLastError(ERROR_SUCCESS); return h;
}

BOOL FindNextFileW(HANDLE find, WIN32_FIND_DATAW *data)
{
	HDRVWIN_HANDLE *h = (HDRVWIN_HANDLE *)find;
	if (!h || h == INVALID_HANDLE_VALUE || h->type != HD_HANDLE_FIND || !data) { SetLastError(ERROR_INVALID_HANDLE); return FALSE; }
	return hd_find_next_internal(h, data) ? TRUE : FALSE;
}

BOOL FindClose(HANDLE find)
{
	HDRVWIN_HANDLE *h = (HDRVWIN_HANDLE *)find;
	if (!h || h == INVALID_HANDLE_VALUE || h->type != HD_HANDLE_FIND) { SetLastError(ERROR_INVALID_HANDLE); return FALSE; }
	if (h->find.dir) closedir(h->find.dir); free(h); SetLastError(ERROR_SUCCESS); return TRUE;
}

BOOL GetDiskFreeSpaceExW(const WCHAR *root, ULARGE_INTEGER *freeAvail, ULARGE_INTEGER *total, ULARGE_INTEGER *totalFree)
{
	char resolved[HD_PATHBUF]; struct statvfs v; uint64_t fr;
	if (!hd_resolve_existing(root, resolved, sizeof(resolved))) return FALSE;
	if (statvfs(resolved, &v) != 0) { SetLastError(hd_errno_error(errno)); return FALSE; }
	fr = v.f_frsize ? v.f_frsize : v.f_bsize;
	if (freeAvail) freeAvail->QuadPart = (uint64_t)v.f_bavail * fr;
	if (total) total->QuadPart = (uint64_t)v.f_blocks * fr;
	if (totalFree) totalFree->QuadPart = (uint64_t)v.f_bfree * fr;
	SetLastError(ERROR_SUCCESS); return TRUE;
}

BOOL GetDiskFreeSpaceW(const WCHAR *root, DWORD *spc, DWORD *bps, DWORD *freec, DWORD *totalc)
{
	char resolved[HD_PATHBUF]; struct statvfs v; uint64_t fr, sectors, f, t;
	if (!hd_resolve_existing(root, resolved, sizeof(resolved))) return FALSE;
	if (statvfs(resolved, &v) != 0) { SetLastError(hd_errno_error(errno)); return FALSE; }
	fr = v.f_frsize ? v.f_frsize : v.f_bsize; sectors = (fr + 511) / 512; if (!sectors) sectors = 1;
	f = v.f_bavail; t = v.f_blocks;
	if (spc) *spc = (DWORD)(sectors > 0xffffffffULL ? 0xffffffffULL : sectors);
	if (bps) *bps = 512;
	if (freec) *freec = (DWORD)(f > 0xffffffffULL ? 0xffffffffULL : f);
	if (totalc) *totalc = (DWORD)(t > 0xffffffffULL ? 0xffffffffULL : t);
	SetLastError(ERROR_SUCCESS); return TRUE;
}

static int64_t hd_days_from_civil(int y, unsigned m, unsigned d)
{
	y -= m <= 2;
	{
		const int era = (y >= 0 ? y : y - 399) / 400;
		const unsigned yoe = (unsigned)(y - era * 400);
		const unsigned doy = (153 * (m + (m > 2 ? (unsigned)-3 : 9)) + 2) / 5 + d - 1;
		const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
		return (int64_t)era * 146097 + (int64_t)doe - 719468;
	}
}

static time_t hd_utc_fields_to_time(int y, int mon, int day, int hour, int min, int sec)
{
	return (time_t)(hd_days_from_civil(y, (unsigned)mon, (unsigned)day) * 86400 + hour * 3600 + min * 60 + sec);
}

BOOL FileTimeToDosDateTime(const FILETIME *ft, WORD *dosDate, WORD *dosTime)
{
	time_t t; struct tm tmv;
	if (!ft || !dosDate || !dosTime) return FALSE;
	t = (time_t)hd_filetime_seconds(ft); if (!gmtime_r(&t, &tmv)) return FALSE;
	if (tmv.tm_year + 1900 < 1980 || tmv.tm_year + 1900 > 2107) return FALSE;
	*dosDate = (WORD)(((tmv.tm_year + 1900 - 1980) << 9) | ((tmv.tm_mon + 1) << 5) | tmv.tm_mday);
	*dosTime = (WORD)((tmv.tm_hour << 11) | (tmv.tm_min << 5) | (tmv.tm_sec / 2)); return TRUE;
}

BOOL DosDateTimeToFileTime(WORD dosDate, WORD dosTime, FILETIME *ft)
{
	int y = 1980 + ((dosDate >> 9) & 0x7f), m = (dosDate >> 5) & 0x0f, d = dosDate & 0x1f;
	int hh = (dosTime >> 11) & 0x1f, mm = (dosTime >> 5) & 0x3f, ss = (dosTime & 0x1f) * 2;
	time_t t;
	if (!ft || m < 1 || m > 12 || d < 1 || d > 31 || hh > 23 || mm > 59 || ss > 59) return FALSE;
	t = hd_utc_fields_to_time(y, m, d, hh, mm, ss); hd_unix_to_filetime(t, 0, ft); return TRUE;
}

BOOL FileTimeToLocalFileTime(const FILETIME *ft, FILETIME *localft)
{
	time_t t; struct tm tmv; time_t synthetic;
	if (!ft || !localft) return FALSE;
	t = (time_t)hd_filetime_seconds(ft); if (!localtime_r(&t, &tmv)) return FALSE;
	synthetic = hd_utc_fields_to_time(tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday, tmv.tm_hour, tmv.tm_min, tmv.tm_sec);
	hd_unix_to_filetime(synthetic, 0, localft); return TRUE;
}

BOOL LocalFileTimeToFileTime(const FILETIME *localft, FILETIME *ft)
{
	time_t synthetic; struct tm tmv; time_t real;
	if (!localft || !ft) return FALSE;
	synthetic = (time_t)hd_filetime_seconds(localft); if (!gmtime_r(&synthetic, &tmv)) return FALSE;
	tmv.tm_isdst = -1; real = mktime(&tmv); if (real == (time_t)-1) return FALSE;
	hd_unix_to_filetime(real, 0, ft); return TRUE;
}

#endif
