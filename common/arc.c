#include	"compiler.h"
#include	"dosio.h"
#include	"arc.h"
#include	"arcunzip.h"
#if defined(OSLANG_UCS2) || defined(OSLANG_UTF8)
#include	"oemtext.h"
#endif


// ---- normal file (default)

typedef struct {
	_ARCFH	arcfh;
	FILEH	fh;
} PLAINFILE;

static UINT plainfile_read(ARCFH arcfh, void *buffer, UINT size) {

	return(file_read(((PLAINFILE *)arcfh)->fh, buffer, size));
}

static UINT plainfile_write(ARCFH arcfh, const void *buffer, UINT size) {

	return(file_write(((PLAINFILE *)arcfh)->fh, buffer, size));
}

static long plainfile_seek(ARCFH arcfh, long pos, UINT method) {

	return(file_seek(((PLAINFILE *)arcfh)->fh, pos, method));
}

static void plainfile_close(ARCFH arcfh) {

	file_close(((PLAINFILE *)arcfh)->fh);
	_MFREE(arcfh);
}

static ARCFH plainfile_regist(FILEH fh) {

	PLAINFILE	*ret;

	if (fh != FILEH_INVALID) {
		ret = (PLAINFILE *)_MALLOC(sizeof(PLAINFILE), "plainfile");
		if (ret != NULL) {
			ret->arcfh.arch = NULL;
			ret->arcfh.fileread = plainfile_read;
			ret->arcfh.filewrite = plainfile_write;
			ret->arcfh.fileseek = plainfile_seek;
			ret->arcfh.fileclose = plainfile_close;
			ret->fh = fh;
			return((ARCFH)ret);
		}
		file_close(fh);
	}
	return(NULL);
}


// ----

void arcfunc_lock(ARCH arch) {

	if (arch != NULL) {
		arch->locked++;
//		TRACEOUT(("arcfunc_lock %d", arch->locked));
	}
}

void arcfunc_unlock(ARCH arch) {

	if (arch != NULL) {
		arch->locked--;
//		TRACEOUT(("arcfunc_unlock %d", arch->locked));
		if (arch->locked == 0) {
			if (arch->deinitialize != NULL) {
				(*arch->deinitialize)(arch);
			}
		}
	}
}


// ----

ARCH arc_open(const OEMCHAR *path) {

	ARCH	ret;

	ret = arcunzip_open(path);
	if (ret) {
		file_cpyname(ret->path, path, NELEMENTS(ret->path));
		ret->locked = 1;
	}
	return(ret);
}

void arc_close(ARCH arch) {

	arcfunc_unlock(arch);
}


// ----

ARCDH arc_diropen(ARCH arch) {

	if ((arch != NULL) && (arch->diropen != NULL)) {
		return((*arch->diropen)(arch));
	}
	return(NULL);
}

BRESULT arc_dirread(ARCDH arcdh, OEMCHAR *fname, UINT size, ARCINF *inf) {

#if defined(OSLANG_UCS2) || defined(OSLANG_UTF8)
	BRESULT	ret;
	char	name[MAX_PATH];
#endif

	if ((arcdh != NULL) && (arcdh->dirread != NULL)) {
#if defined(OSLANG_UCS2) || defined(OSLANG_UTF8)
		ret = (arcdh->dirread)(arcdh, name, sizeof(name), inf);
		if ((ret == SUCCESS) && (fname != NULL) && (size != 0)) {
			oemtext_sjistooem(fname, size, name, (UINT)-1);
		}
		return(ret);
#else
		return((arcdh->dirread)(arcdh, fname, size, inf));
#endif
	}
	else {
		return(FAILURE);
	}
}

void arc_dirclose(ARCDH arcdh) {

	if ((arcdh != NULL) && (arcdh->dirclose != NULL)) {
		(*arcdh->dirclose)(arcdh);
	}
}


// ---

ARCFH arc_fileopen(ARCH arch, const OEMCHAR *fname) {

#if defined(OSLANG_UCS2) || defined(OSLANG_UTF8)
	char	name[MAX_PATH];
#endif

	if (arch != NULL) {
		if (arch->fileopen != NULL) {
#if defined(OSLANG_UCS2) || defined(OSLANG_UTF8)
			oemtext_oemtosjis(name, NELEMENTS(name), fname, (UINT)-1);
			return((*arch->fileopen)(arch, name));
#else
			return((*arch->fileopen)(arch, fname));
#endif
		}
	}
	return(NULL);
}

UINT arc_fileread(ARCFH arcfh, void *buffer, UINT size) {

	if ((arcfh != NULL) && (arcfh->fileread != NULL)) {
		return((*arcfh->fileread)(arcfh, buffer, size));
	}
	return(0);
}

UINT arc_filewrite(ARCFH arcfh, const void *buffer, UINT size) {

	if ((arcfh != NULL) && (arcfh->filewrite != NULL)) {
		return((*arcfh->filewrite)(arcfh, buffer, size));
	}
	return(0);
}

long arc_fileseek(ARCFH arcfh, long pos, UINT method) {

	if ((arcfh != NULL) && (arcfh->fileseek != NULL)) {
		return((*arcfh->fileseek)(arcfh, pos, method));
	}
	return(-1);
}

void arc_fileclose(ARCFH arcfh) {

	if ((arcfh != NULL) && (arcfh->fileclose != NULL)) {
		(*arcfh->fileclose)(arcfh);
	}
}


// ---- attr

SINT16 arc_attr(ARCH arch, const OEMCHAR *fname) {

#if defined(OSLANG_UCS2) || defined(OSLANG_UTF8)
	char	path[MAX_PATH];
#endif

	if ((arch != NULL) && (arch->fileattr != NULL)) {
#if defined(OSLANG_UCS2) || defined(OSLANG_UTF8)
		oemtext_oemtosjis(path, NELEMENTS(path), fname, (UINT)-1);
		return((*arch->fileattr)(arch, path));
#else
		return((*arch->fileattr)(arch, fname));
#endif
	}
	return(-1);
}


// ----

ARCFH arcex_fileopen(const OEMCHAR *fname) {

const OEMCHAR	*p;
	UINT		len;
	OEMCHAR		path[MAX_PATH];
	ARCFH		ret;
	ARCH		arch;

	p = milstr_chr(fname, '#');
	if (p == NULL) {
		return(plainfile_regist(file_open(fname)));
	}
	len = (UINT)(p - fname);
	if (len >= NELEMENTS(path)) {
		return(NULL);
	}
	CopyMemory(path, fname, len * sizeof(OEMCHAR));
	path[len] = '\0';
	fname = fname + len + 1;
	arch = arc_open(path);
	ret = arc_fileopen(arch, fname);
	arc_close(arch);
	return(ret);
}

ARCFH arcex_filecreate(const OEMCHAR *fname) {

	if (milstr_chr(fname, '#') == NULL) {
		return(plainfile_regist(file_create(fname)));
	}
	else {
		return(NULL);
	}
}

SINT16 arcex_attr(const OEMCHAR *fname) {

const OEMCHAR	*p;
	UINT		len;
	OEMCHAR		path[MAX_PATH];
	SINT16		ret;
	ARCH		arch;

	p = milstr_chr(fname, '#');
	if (p == NULL) {
		return(file_attr(fname));
	}
	len = (UINT)(p - fname);
	if (len >= NELEMENTS(path)) {
		return(-1);
	}
	CopyMemory(path, fname, len * sizeof(OEMCHAR));
	path[len] = '\0';
	fname = fname + len + 1;
	arch = arc_open(path);
	ret = arc_attr(arch, fname);
	arc_close(arch);
	return(ret);
}

