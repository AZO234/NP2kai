#include	"compiler.h"
#include	"dosio.h"
#include	"arc.h"
#include	"arcunzip.h"
#if defined(SUPPORT_ZLIB)
#include	"zlib.h"
#endif


typedef struct {
	UINT8	disknum[2];
	UINT8	disknum_cd[2];
	UINT8	entrynum[2];
	UINT8	entrynum_cd[2];
	UINT8	catsize[4];
	UINT8	catfpos[4];
	UINT8	comsize[2];
} ZIPHDR;

typedef struct {
	UINT8	sig[4];
	UINT8	ver[2];
	UINT8	ver2[2];
	UINT8	flag[2];
	UINT8	compressmethod[2];
	UINT8	time[4];
	UINT8	crc[4];
	UINT8	compresssize[4];
	UINT8	originalsize[4];
	UINT8	filenamesize[2];
	UINT8	fileextrasize[2];
	UINT8	commentsize[2];
	UINT8	disknum[2];
	UINT8	internal[2];
	UINT8	external[4];
	UINT8	internalfpos[4];
} ZIPCAT;

typedef struct {
	UINT8	sig[4];
	UINT8	ver[2];
	UINT8	flag[2];
	UINT8	compressmethod[2];
	UINT8	time[4];
	UINT8	crc[4];
	UINT8	compresssize[4];
	UINT8	originalsize[4];
	UINT8	filenamesize[2];
	UINT8	fileextrasize[2];
} ZIPDAT;

typedef struct {
	UINT8	*ptr;
	UINT	rem;
} ZCATHDL;

typedef struct {
	_ARCH	arch;
	FILEH	fh;
	UINT	catsize;
} ZIPHDL;

typedef struct {
	_ARCDH	arcdh;
	ZCATHDL	zch;
} ZIPDIR;


// ---- cat

static void initializecat(ZIPHDL *hdl, ZCATHDL *zch) {

	zch->ptr = (UINT8 *)(hdl + 1);
	zch->rem = hdl->catsize;
}

static ZIPCAT *getcatnext(ZCATHDL *zch) {

	ZIPCAT	*ret;
	UINT	size;

	if (zch->rem < sizeof(ZIPCAT)) {
//		TRACEOUT(("rem: %d < sizeof(ZIPCAT)", zch->rem));
		goto gdn_err;
	}
	ret = (ZIPCAT *)zch->ptr;
	if ((ret->sig[0] != 0x50) || (ret->sig[1] != 0x4b) ||
		(ret->sig[2] != 0x01) || (ret->sig[3] != 0x02)) {
//		TRACEOUT(("hdr error"));
		goto gdn_err;
	}
	size = sizeof(ZIPCAT);
	size += LOADINTELWORD(ret->filenamesize);
	size += LOADINTELWORD(ret->fileextrasize);
	size += LOADINTELWORD(ret->commentsize);
	if (zch->rem < size) {
//		TRACEOUT(("rem: %d < catsize:%d", zch->rem, size));
		goto gdn_err;
	}
	zch->ptr += size;
	zch->rem -= size;
	return(ret);

gdn_err:
	return(NULL);
}

static UINT getcatfilename(const ZIPCAT *cat, char *filename, UINT size) {

	UINT	fnamesize;

	if (size == 0) {
		return(0);
	}
	size = size - 1;
	fnamesize = LOADINTELWORD(cat->filenamesize);
	size = min(size, fnamesize);
	if (size) {
		CopyMemory(filename, cat + 1, size);
	}
	filename[size] = '\0';
	return(size);
}

static void getcattime(const ZIPCAT *cat, ARCTIME *at) {

	UINT	val;

	if (at) {
		val = LOADINTELWORD(cat->time + 2);
		at->year = (UINT16)((val >> 9) + 1980);
		at->month = (UINT8)((val >> 5) & 0x0f);
		at->day = (UINT8)(val & 0x1f);
		val = LOADINTELWORD(cat->time + 0);
		at->hour = (UINT8)(val >> 11);
		at->minute = (UINT8)((val >> 5) & 0x3f);
		at->second = (UINT8)((val & 0x1f) << 1);
	}
}


// ---- method0

typedef struct {
	_ARCFH	arcfh;
	FILEH	fh;
	long	fposbase;
	UINT	pos;
	UINT	size;
} METHOD0;

static UINT method0_read(ARCFH arcfh, void *buffer, UINT size) {

	METHOD0	*m0;
	UINT	rsize;
	long	fpos;

	m0 = (METHOD0 *)arcfh;
	rsize = m0->size - m0->pos;
	rsize = min(rsize, size);
	if (rsize == 0) {
		return(0);
	}
	fpos = m0->fposbase + m0->pos;
	if (file_seek(m0->fh, fpos, FSEEK_SET) != fpos) {
		return(0);
	}
	rsize = file_read(m0->fh, buffer, rsize);
	m0->pos += rsize;
	return(rsize);
}

static long method0_seek(ARCFH arcfh, long pos, UINT method) {

	METHOD0	*m0;

	m0 = (METHOD0 *)arcfh;
	switch(method) {
		case ARCSEEK_SET:
		default:
			break;

		case ARCSEEK_CUR:
			pos += m0->pos;
			break;

		case ARCSEEK_END:
			pos += m0->size;
			break;
	}
	if (pos < 0) {
		pos = 0;
	}
	else if (pos > (long)m0->size) {
		pos = m0->size;
	}
	m0->pos = (UINT)pos;
	return(pos);
}

static void method0_close(ARCFH arcfh) {

	arcfunc_unlock(arcfh->arch);
	_MFREE(arcfh);
}

static ARCFH method0_open(ARCH arch, FILEH fh, long fpos, const ZIPDAT *zd) {

	UINT	size;
	METHOD0	*ret;

	if (memcmp(zd->compresssize, zd->originalsize, 4)) {
		return(NULL);
	}
	size = LOADINTELDWORD(zd->compresssize);
	ret = (METHOD0 *)_MALLOC(sizeof(METHOD0), arch->path);
	if (ret == NULL) {
		return(NULL);
	}
	arcfunc_lock(arch);
	ret->arcfh.arch = arch;
	ret->arcfh.fileread = method0_read;
	ret->arcfh.filewrite = NULL;
	ret->arcfh.fileseek = method0_seek;
	ret->arcfh.fileclose = method0_close;
	ret->fh = fh;
	ret->fposbase = fpos;
	ret->pos = 0;
	ret->size = size;
	return((ARCFH)ret);
}


// ---- method8

#if defined(SUPPORT_ZLIB)
typedef struct {
	_ARCFH		arcfh;
	FILEH		fh;
	long		fposbase;
	UINT		srcsize;
	UINT		srcpos;
	UINT		dstsize;
	UINT		dstpos;
	int			dstdiff;

	z_stream	stream;
	int			err;
	UINT		pos;
	UINT8		src[0x2000];
	UINT8		dst[0x2000];
} METHOD8;

static void method8start(METHOD8 *m8) {

	m8->srcpos = 0;
	m8->dstpos = 0;
	ZeroMemory(&m8->stream, sizeof(m8->stream));
	m8->err = inflateInit2(&m8->stream, -MAX_WBITS);
	m8->pos = 0;
	m8->stream.avail_in = 0;
	m8->stream.next_out = m8->dst;
	m8->stream.avail_out = NELEMENTS(m8->dst);
}

static UINT method8read(METHOD8 *m8, void *buffer, UINT size) {

	UINT	r;
	UINT8	*ptr;
	UINT	dstrem;
	long	fpos;

	r = m8->dstsize - m8->dstpos;
	size = min(size, r);
	ptr = (UINT8 *)buffer;
	while(size) {
		dstrem = NELEMENTS(m8->dst) - m8->stream.avail_out - m8->pos;
		r = min(dstrem, size);
		if (r) {
			if (buffer != NULL) {
				CopyMemory(ptr, m8->dst + m8->pos, r);
			}
			ptr += r;
			size -= r;
			m8->dstpos += r;
			m8->pos += r;
			dstrem -= r;
		}
		if (!size) {
			break;
		}
		if (m8->err != Z_OK) {
			break;
		}
//		TRACEOUT(("try decompress"));
		if (m8->stream.avail_in == 0) {
			r = m8->srcsize - m8->srcpos;
			r = min(r, NELEMENTS(m8->src));
			if (r == 0) {
				break;
			}
			fpos = m8->fposbase + m8->srcpos;
			if (file_seek(m8->fh, fpos, FSEEK_SET) != fpos) {
				break;
			}
			r = file_read(m8->fh, m8->src, r);
			if (r == 0) {
				break;
			}
//			TRACEOUT(("read buf %d [%.2x %.2x %.2x %.2x...]", r, m8->src[0], m8->src[1], m8->src[2], m8->src[3]));
			m8->srcpos += r;
			m8->stream.next_in = m8->src;
			m8->stream.avail_in = r;
		}
		m8->stream.next_out = m8->dst;
		m8->stream.avail_out = NELEMENTS(m8->dst);
		m8->err = inflate(&m8->stream, Z_SYNC_FLUSH);
		m8->pos = 0;
//		TRACEOUT(("inflate ret = %d", m8->err));
//		TRACEOUT(("avail_out = %d", m8->stream.avail_out));
	}
	return((UINT)(ptr - ((UINT8 *)buffer)));
}

static UINT method8_read(ARCFH arcfh, void *buffer, UINT size) {

	METHOD8	*m8;

	m8 = (METHOD8 *)arcfh;
	if (m8->dstdiff < 0) {
		inflateEnd(&m8->stream);
		method8start(m8);
		m8->dstdiff = 0 - m8->dstdiff;
	}
	if (m8->dstdiff) {
		method8read(m8, NULL, m8->dstdiff);
		m8->dstdiff = 0;
	}
	return(method8read(m8, buffer, size));
}

static long method8_seek(ARCFH arcfh, long pos, UINT method) {

	METHOD8	*m8;

	m8 = (METHOD8 *)arcfh;
	switch(method) {
		case ARCSEEK_SET:
		default:
			break;

		case ARCSEEK_CUR:
			pos += m8->dstpos + m8->dstdiff;
			break;

		case ARCSEEK_END:
			pos += m8->dstsize;
			break;
	}
	if (pos < 0) {
		pos = 0;
	}
	else if (pos > (long)m8->dstsize) {
		pos = m8->dstsize;
	}
	m8->dstdiff = (int)(pos - m8->dstpos);
	return(pos);
}

static void method8_close(ARCFH arcfh) {

	METHOD8	*m8;

	m8 = (METHOD8 *)arcfh;
	inflateEnd(&m8->stream);
	arcfunc_unlock(arcfh->arch);
	_MFREE(arcfh);
}

static ARCFH method8_open(ARCH arch, FILEH fh, long fpos, const ZIPDAT *zd) {

	METHOD8	*ret;

	ret = (METHOD8 *)_MALLOC(sizeof(METHOD8), arch->path);
	if (ret) {
		arcfunc_lock(arch);
		ret->arcfh.arch = arch;
		ret->arcfh.fileread = method8_read;
		ret->arcfh.filewrite = NULL;
		ret->arcfh.fileseek = method8_seek;
		ret->arcfh.fileclose = method8_close;
		ret->fh = fh;
		ret->fposbase = fpos;
		ret->srcsize = LOADINTELDWORD(zd->compresssize);
		ret->dstsize = LOADINTELDWORD(zd->originalsize);
		method8start(ret);
		ret->dstdiff = 0;
	}
	return((ARCFH)ret);
}
#endif


static ARCFH openzipfile(ARCH arch, const ZIPCAT *cat) {

	ZIPHDL	*hdl;
	UINT	method;
	long	fpos;
	UINT	size;
	FILEH	fh;
	ZIPDAT	zd;

	hdl = (ZIPHDL *)arch;
	method = LOADINTELWORD(cat->compressmethod);
//	TRACEOUT(("method = %d", method));

	fpos = LOADINTELDWORD(cat->internalfpos);
	size = LOADINTELDWORD(cat->compresssize);

//	TRACEOUT(("fpos = %d", fpos));
	fh = hdl->fh;
	if ((file_seek(fh, fpos, FSEEK_SET) != fpos) ||
		(file_read(fh, &zd, sizeof(zd)) != sizeof(zd))) {
		goto ozf_err;
	}

	if ((zd.sig[0] != 0x50) || (zd.sig[1] != 0x4b) ||
		(zd.sig[2] != 0x03) || (zd.sig[3] != 0x04)) {
		goto ozf_err;
	}
	if ((zd.compressmethod[0] != cat->compressmethod[0]) ||
		(zd.compressmethod[1] != cat->compressmethod[1])) {
		goto ozf_err;
	}
	if (!(zd.flag[0] & 8)) {
		if ((memcmp(zd.crc, cat->crc, 4)) ||
			(memcmp(zd.compresssize, cat->compresssize, 4)) ||
			(memcmp(zd.originalsize, cat->originalsize, 4))) {
			goto ozf_err;
		}
	}
	fpos += sizeof(zd);
	fpos += LOADINTELWORD(zd.filenamesize);
	fpos += LOADINTELWORD(zd.fileextrasize);

	if (method == 0) {
		return(method0_open(arch, fh, fpos, &zd));
	}
#if defined(SUPPORT_ZLIB)
	else if (method == Z_DEFLATED) {
		return(method8_open(arch, fh, fpos, &zd));
	}
#endif

ozf_err:
	return(NULL);
}


// ---- unzip dir

BRESULT dirread(ARCDH arcdh, char *fname, UINT size, ARCINF *inf) {

	ZIPDIR		*zipd;
const ZIPCAT	*cat;

	zipd = (ZIPDIR *)arcdh;
	cat = getcatnext(&zipd->zch);
	if (cat != NULL) {
		if ((fname != NULL) && (size != 0)) {
			getcatfilename(cat, fname, size);
		}
		if (inf) {
			inf->method = LOADINTELWORD(cat->compressmethod);
			inf->originalsize = LOADINTELDWORD(cat->originalsize);
			inf->compresssize = LOADINTELDWORD(cat->compresssize);
			getcattime(cat, &inf->time);
		}
		return(SUCCESS);
	}
	else {
		return(FAILURE);
	}
}

static void dirclose(ARCDH arcdh) {

	arcfunc_unlock(arcdh->arch);
	_MFREE(arcdh);
}

static ARCDH diropen(ARCH arch) {

	ZIPDIR	*ret;

	ret = (ZIPDIR *)_MALLOC(sizeof(ZIPDIR), arch->path);
	if (ret == NULL) {
		return(NULL);
	}
	arcfunc_lock(arch);
	ret->arcdh.arch = arch;
	ret->arcdh.dirread = dirread;
	ret->arcdh.dirclose = dirclose;
	initializecat((ZIPHDL *)arch, &ret->zch);
	return((ARCDH)ret);
}


// ---- unzip file

static ARCFH fileopen(ARCH arch, const char *name) {

	UINT		nameleng;
	ZCATHDL		zch;
const ZIPCAT	*cat;

	if (name == NULL) {
		return(NULL);
	}
//	TRACEOUT(("zipopen: %s", name));
	nameleng = (UINT)STRLEN(name);
	initializecat((ZIPHDL *)arch, &zch);
	while(1) {
		cat = getcatnext(&zch);
		if (cat == NULL) {
			return(NULL);
		}
		if ((LOADINTELWORD(cat->filenamesize) == nameleng) &&
			(!memcmp(cat + 1, name, nameleng))) {
			break;
		}
	}
	return(openzipfile(arch, cat));
}


// ---- unzip attr

static SINT16 fileattr(ARCH arch, const char *name) {

	UINT		nameleng;
	ZCATHDL		zch;
const ZIPCAT	*cat;

	if (name == NULL) {
		return(-1);
	}
	nameleng = (UINT)STRLEN(name);
	initializecat((ZIPHDL *)arch, &zch);
	while(1) {
		cat = getcatnext(&zch);
		if (cat == NULL) {
			return(-1);
		}
		if ((LOADINTELWORD(cat->filenamesize) == nameleng) &&
			(!memcmp(cat + 1, name, nameleng))) {
			break;
		}
	}
	return(0x21);
}


// ---- unzip open

static BRESULT getziphdrpos(FILEH fh, long *hdrpos) {

	long	fpos;
	UINT	bufrem;
	UINT8	buf[0x400];
	UINT	rsize;
	UINT	r;

	fpos = file_seek(fh, 0, FSEEK_END);
	bufrem = 0;
	while(fpos > 0) {
		rsize = NELEMENTS(buf) - bufrem;
		rsize = (UINT)(min(fpos, (long)rsize));
		fpos -= rsize;
		r = bufrem;
		while(r) {
			r--;
			buf[rsize + r] = buf[r];
		}
//		TRACEOUT(("read: %x %x", fpos, rsize));
		if ((file_seek(fh, fpos, FSEEK_SET) != fpos) ||
			(file_read(fh, buf, rsize) != rsize)) {
//			TRACEOUT(("seek error!"));
			break;
		}
		bufrem += rsize;
		while(bufrem >= 4) {
			if ((buf[bufrem-4] == 0x50) && (buf[bufrem-3] == 0x4b) &&
				(buf[bufrem-2] == 0x05) && (buf[bufrem-1] == 0x06)) {
				if (hdrpos) {
					*hdrpos = fpos + bufrem;
				}
				return(SUCCESS);
			}
			bufrem--;
		}
	}
	return(FAILURE);
}

static void deinitialize(ARCH arch) {

	file_close(((ZIPHDL *)arch)->fh);
	_MFREE(arch);
}

ARCH arcunzip_open(const OEMCHAR *path) {

	FILEH	fh;
	long	fpos;
	ZIPHDR	hdr;
	UINT	catsize;
	long	catfpos;
	ZIPHDL	*ret;

//	TRACEOUT(("open file: %s", path));
	fh = file_open_rb(path);
	if (fh == FILEH_INVALID) {
//		TRACEOUT(("file_open: error"));
		goto uzo_err1;
	}
	if (getziphdrpos(fh, &fpos) != SUCCESS) {
//		TRACEOUT(("getziphdrpos: error"));
		goto uzo_err2;
	}
//	TRACEOUT(("hdrpos = %d", fpos));
	if ((file_seek(fh, fpos, FSEEK_SET) != fpos) ||
		(file_read(fh, &hdr, sizeof(hdr)) != sizeof(hdr))) {
		goto uzo_err2;
	}
	if ((hdr.disknum[0] != 0) || (hdr.disknum[1] != 0) ||
		(hdr.disknum_cd[0] != 0) || (hdr.disknum_cd[1] != 0) ||
		(hdr.entrynum[0] != hdr.entrynum_cd[0]) ||
		(hdr.entrynum[1] != hdr.entrynum_cd[1])) {
		goto uzo_err2;
	}
	catsize = LOADINTELDWORD(hdr.catsize);
	catfpos = LOADINTELDWORD(hdr.catfpos);
	if ((catsize == 0) || (file_seek(fh, catfpos, FSEEK_SET) != catfpos)) {
		goto uzo_err2;
	}
	ret = (ZIPHDL *)_MALLOC(sizeof(ZIPHDL) + catsize, path);
	if (ret == NULL) {
		goto uzo_err2;
	}
	ZeroMemory(ret, sizeof(ZIPHDL));
	if (file_read(fh, ret + 1, catsize) != catsize) {
		goto uzo_err3;
	}
	ret->arch.diropen = diropen;
	ret->arch.fileopen = fileopen;
	ret->arch.fileattr = fileattr;
	ret->arch.deinitialize = deinitialize;
	ret->fh = fh;
	ret->catsize = catsize;
	return((ARCH)ret);

uzo_err3:
	_MFREE(ret);

uzo_err2:
	file_close(fh);

uzo_err1:
	return(NULL);
}

