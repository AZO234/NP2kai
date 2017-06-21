#include	"compiler.h"
#include	"strres.h"
#include	"textfile.h"
#if defined(SUPPORT_TEXTCNV)
#include	"codecnv/textcnv.h"
#endif
#if defined(SUPPORT_ARC)
#include	"arc.h"
#else
#include	"dosio.h"
#endif


// ---- arc support?

#if defined(SUPPORT_ARC)
#define	_FILEH				ARCFH
#define	_FILEH_INVALID		NULL
#define	_FSEEK_SET			ARCSEEK_SET
#define _file_open_rb		arcex_fileopen
#define _file_create		arcex_filecreate
#define	_file_read			arc_fileread
#define	_file_write			arc_filewrite
#define	_file_seek			arc_fileseek
#define	_file_close			arc_fileclose
#else
#define	_FILEH				FILEH
#define	_FILEH_INVALID		FILEH_INVALID
#define	_FSEEK_SET			FSEEK_SET
#define _file_open_rb		file_open_rb
#define _file_create		file_create
#define	_file_read			file_read
#define	_file_write			file_write
#define	_file_seek			file_seek
#define	_file_close			file_close
#endif

enum {
	TFMODE_READ		= 0x01,
	TFMODE_WRITE	= 0x02
};

struct _textfile;
typedef struct _textfile		_TEXTFILE;
typedef struct _textfile		*TEXTFILE;

typedef BRESULT (*READFN)(TEXTFILE tf, void *buffer, UINT size);

struct _textfile {
	UINT8	mode;
	UINT8	width;
#if defined(SUPPORT_TEXTCNV)
	UINT8	access;
	UINT8	xendian;
#endif
	_FILEH	fh;
	long	fpos;
	UINT8	*buf;
	UINT	bufsize;
	UINT	bufpos;
	UINT	bufrem;
	READFN	readfn;
#if defined(SUPPORT_TEXTCNV)
	UINT8		*cnvbuf;
	UINT		cnvbufsize;
	TCTOOEM		tooem;
	TCFROMOEM	fromoem;
#endif
};


// ---- ReadA

static UINT readbufferA(TEXTFILE tf) {

	UINT	rsize;

	if (tf->bufrem == 0) {
		rsize = _file_read(tf->fh, tf->buf, tf->bufsize);
		rsize = rsize / sizeof(char);
		tf->fpos += rsize * sizeof(char);
		tf->bufpos = 0;
		tf->bufrem = rsize;
	}
	return(tf->bufrem);
}

static BRESULT readlineA(TEXTFILE tf, void *buffer, UINT size) {

	char	*dst;
	BOOL	crlf;
	BRESULT	ret;
	char	c;
const char	*src;
	UINT	pos;

	if (size == 0) {
		dst = NULL;
		size = 0;
	}
	else {
		dst = (char *)buffer;
		size--;
	}

	crlf = FALSE;
	ret = FAILURE;
	c = 0;
	do {
		if (readbufferA(tf) == 0) {
			break;
		}
		ret = SUCCESS;
		src = (char *)tf->buf;
		src += tf->bufpos;
		pos = 0;
		while(pos<tf->bufrem) {
			c = src[pos];
			pos++;
			if ((c == 0x0d) || (c == 0x0a)) {
				crlf = TRUE;
				break;
			}
			if (size) {
				size--;
				*dst++ = c;
			}
		}
		tf->bufpos += pos;
		tf->bufrem -= pos;
	} while(!crlf);
	if ((crlf) && (c == 0x0d)) {
		if (readbufferA(tf) != 0) {
			src = (char *)tf->buf;
			src += tf->bufpos;
			if (*src == 0x0a) {
				tf->bufpos++;
				tf->bufrem--;
			}
		}
	}
	if (dst) {
		*dst = '\0';
	}
	return(ret);
}


// ---- ReadW

static UINT readbufferW(TEXTFILE tf) {

	UINT	rsize;
	UINT8	*buf;

	if (tf->bufrem == 0) {
		buf = tf->buf;
		rsize = _file_read(tf->fh, buf, tf->bufsize);
		rsize = rsize / sizeof(UINT16);
		tf->fpos += rsize * sizeof(UINT16);
		tf->bufpos = 0;
		tf->bufrem = rsize;
#if defined(SUPPORT_TEXTCNV)
		if (tf->xendian) {
			textcnv_swapendian16(buf, rsize);
		}
#endif	// defined(SUPPORT_TEXTCNV)
	}
	return(tf->bufrem);
}

static BRESULT readlineW(TEXTFILE tf, void *buffer, UINT size) {

	UINT16		*dst;
	BOOL		crlf;
	BRESULT		ret;
	UINT16		c;
const UINT16	*src;
	UINT		pos;

	if (size == 0) {
		dst = NULL;
		size = 0;
	}
	else {
		dst = (UINT16 *)buffer;
		size--;
	}
	crlf = FALSE;
	ret = FAILURE;
	c = 0;
	do {
		if (readbufferW(tf) == 0) {
			break;
		}
		ret = SUCCESS;
		src = (UINT16 *)tf->buf;
		src += tf->bufpos;
		pos = 0;
		while(pos<tf->bufrem) {
			c = src[pos];
			pos++;
			if ((c == 0x0d) || (c == 0x0a)) {
				crlf = TRUE;
				break;
			}
			if (size) {
				size--;
				*dst++ = c;
			}
		}
		tf->bufpos += pos;
		tf->bufrem -= pos;
	} while(!crlf);
	if ((crlf) && (c == 0x0d)) {
		if (readbufferW(tf) != 0) {
			src = (UINT16 *)tf->buf;
			src += tf->bufpos;
			if (*src == 0x0a) {
				tf->bufpos++;
				tf->bufrem--;
			}
		}
	}
	if (dst) {
		*dst = '\0';
	}
	return(ret);
}


// ---- read with convert

#if defined(SUPPORT_TEXTCNV)
static BRESULT readlineAtoOEM(TEXTFILE tf, void *buffer, UINT size) {

	BRESULT	ret;

	ret = readlineA(tf, tf->cnvbuf, tf->cnvbufsize);
	if (ret == SUCCESS) {
		(tf->tooem)((OEMCHAR *)buffer, size, tf->cnvbuf, (UINT)-1);
	}
	return(ret);
}

static BRESULT readlineWtoOEM(TEXTFILE tf, void *buffer, UINT size) {

	BRESULT	ret;

	ret = readlineW(tf, tf->cnvbuf, tf->cnvbufsize / 2);
	if (ret == SUCCESS) {
		(tf->tooem)((OEMCHAR *)buffer, size, tf->cnvbuf, (UINT)-1);
	}
	return(ret);
}
#endif	// defined(SUPPORT_TEXTCNV)


// ---- write

static BRESULT flushwritebuffer(TEXTFILE tf) {

	UINT	size;
	UINT	wsize;

	if (tf->bufpos) {
		size = tf->bufpos * tf->width;
		wsize = _file_write(tf->fh, tf->buf, size);
		tf->fpos += wsize;
		if (wsize != size) {
			return(FAILURE);
		}
	}
	return(SUCCESS);
}

static BRESULT writebufferA(TEXTFILE tf, const void *buffer, UINT size) {

	BRESULT	ret;
const UINT8	*p;
	UINT	wsize;

	ret = SUCCESS;
	p = (UINT8 *)buffer;
	while(size) {
		wsize = min(size, tf->bufrem);
		if (wsize) {
			CopyMemory(tf->buf + tf->bufpos, p, wsize);
			p += wsize;
			size -= wsize;
			tf->bufpos += wsize;
			tf->bufrem -= wsize;
		}
		if (tf->bufrem == 0) {
			ret = flushwritebuffer(tf);
			tf->bufpos = 0;
			tf->bufrem = tf->bufsize / sizeof(char);
		}
	}
	return(ret);
}

static BRESULT writebufferW(TEXTFILE tf, const void *buffer, UINT size) {

	BRESULT	ret;
const UINT8	*p;
	UINT8	*q;
	UINT	wsize;

	ret = SUCCESS;
	p = (UINT8 *)buffer;
	while(size) {
		wsize = min(size, tf->bufrem);
		if (wsize) {
			q = tf->buf + (tf->bufpos * sizeof(UINT16));
			CopyMemory(q, p, wsize * sizeof(UINT16));
#if defined(SUPPORT_TEXTCNV)
			if (tf->xendian) {
				textcnv_swapendian16(q, wsize);
			}
#endif	// defined(SUPPORT_TEXTCNV)
			p += wsize * sizeof(UINT16);
			size -= wsize;
			tf->bufpos += wsize;
			tf->bufrem -= wsize;
		}
		if (tf->bufrem == 0) {
			ret = flushwritebuffer(tf);
			tf->bufpos = 0;
			tf->bufrem = tf->bufsize / sizeof(UINT16);
		}
	}
	return(ret);
}


// ----

static TEXTFILEH registfile(_FILEH fh, UINT buffersize,
											const UINT8 *hdr, UINT hdrsize) {

	UINT		cnvbufsize;
#if defined(SUPPORT_TEXTCNV)
	TCINF		inf;
#endif
	long		fpos;
	UINT8		width;
	READFN		readfn;
	TEXTFILE	ret;

	buffersize = buffersize & (~3);
	if (buffersize < 256) {
		buffersize = 256;
	}
	cnvbufsize = 0;
#if defined(SUPPORT_TEXTCNV)
	if (textcnv_getinfo(&inf, hdr, hdrsize) == 0) {
		return(NULL);
	}
	fpos = inf.hdrsize;
	width = inf.width;
	if (inf.width == 1) {
		readfn = (inf.tooem != NULL)?readlineAtoOEM:readlineA;
	}
	else if (inf.width == 2) {
		buffersize = buffersize * 2;
		readfn = (inf.tooem != NULL)?readlineWtoOEM:readlineW;
	}
	else {
		return(NULL);
	}
	if ((inf.tooem != NULL) || (inf.fromoem != NULL)) {
		cnvbufsize = buffersize;
	}
#else	// defined(SUPPORT_TEXTCNV)
	fpos = 0;
	width = 1;
	if ((hdrsize >= 3) &&
		(hdr[0] == 0xef) && (hdr[1] == 0xbb) && (hdr[2] == 0xbf)) {
		// UTF-8
		fpos = 3;
	}
	else if ((hdrsize >= 2) && (hdr[0] == 0xff) && (hdr[1] == 0xfe)) {
		// UCSLE
		fpos = 2;
		width = 2;
#if defined(BYTESEX_BIG)
		return(NULL);
#endif	// defined(BYTESEX_BIG)
	}
	else if ((hdrsize >= 2) && (hdr[0] == 0xfe) && (hdr[1] == 0xff)) {
		// UCS2BE
		fpos = 2;
		width = 2;
#if defined(BYTESEX_LITTLE)
		return(NULL);
#endif	// defined(BYTESEX_LITTLE)
	}
	if (width != sizeof(OEMCHAR)) {
		return(NULL);
	}
	buffersize = buffersize * sizeof(OEMCHAR);
	readfn = (width == 2)?readlineW:readlineA;
#endif	// defined(SUPPORT_TEXTCNV)

	ret = (TEXTFILE)_MALLOC(sizeof(_TEXTFILE) + buffersize + cnvbufsize,
																"TEXTFILE");
	if (ret == NULL) {
		return(NULL);
	}
	ZeroMemory(ret, sizeof(_TEXTFILE));
//	ret->mode = 0;
	ret->width = width;
	ret->fh = fh;
	ret->fpos = fpos;
	ret->buf = (UINT8 *)(ret + 1);
	ret->bufsize = buffersize;
	ret->readfn = readfn;
#if defined(SUPPORT_TEXTCNV)
	ret->access = inf.caps;
	ret->xendian = inf.xendian;
	ret->cnvbuf = ret->buf + buffersize;
	ret->cnvbufsize = cnvbufsize;
	ret->tooem = inf.tooem;
	ret->fromoem = inf.fromoem;
#endif	// defined(SUPPORT_TEXTCNV)
	return((TEXTFILEH)ret);
}

static BRESULT flushfile(TEXTFILE tf) {

	BRESULT	ret;
	long	fpos;
	UINT	size;
	UINT	wsize;

	ret = SUCCESS;
	if (tf->mode & TFMODE_READ) {
		fpos = tf->fpos - (tf->bufrem * tf->width);
		tf->fpos = _file_seek(tf->fh, fpos, _FSEEK_SET);
		if (tf->fpos != fpos) {
			ret = FAILURE;
		}
	}
	else if (tf->mode & TFMODE_WRITE) {
		if (tf->bufpos) {
			size = tf->bufpos * tf->width;
			wsize = _file_write(tf->fh, tf->buf, size);
			if (wsize != size) {
				ret = FAILURE;
			}
			tf->fpos += wsize;
		}
	}
	else {
		fpos = _file_seek(tf->fh, tf->fpos, _FSEEK_SET);
		if (tf->fpos != fpos) {
			ret = FAILURE;
		}
		tf->fpos = fpos;
	}
	tf->mode = 0;
	tf->bufpos = 0;
	tf->bufrem = 0;
	return(ret);
}


// ----

TEXTFILEH textfile_open(const OEMCHAR *filename, UINT buffersize) {

	_FILEH		fh;
	UINT8		hdr[4];
	UINT		hdrsize;
	TEXTFILEH	ret;

	fh = _file_open_rb(filename);
	if (fh == _FILEH_INVALID) {
		goto tfo_err;
	}
	hdrsize = _file_read(fh, hdr, sizeof(hdr));
	ret = registfile(fh, buffersize, hdr, hdrsize);
	if (ret) {
		return(ret);
	}
	_file_close(fh);

tfo_err:
	return(NULL);
}

TEXTFILEH textfile_create(const OEMCHAR *filename, UINT buffersize) {

	_FILEH		fh;
const UINT8		*hdr;
	UINT		hdrsize;
	TEXTFILEH	ret;

	fh = _file_create(filename);
	if (fh == _FILEH_INVALID) {
		goto tfc_err1;
	}
#if defined(OSLANG_UTF8)
	hdr = str_utf8;
	hdrsize = sizeof(str_utf8);
#elif defined(OSLANG_UCS2) 
	hdr = (UINT8 *)str_ucs2;
	hdrsize = sizeof(str_ucs2);
#else
	hdr = NULL;
	hdrsize = 0;
#endif
	if ((hdrsize) && (_file_write(fh, hdr, hdrsize) != hdrsize)) {
		goto tfc_err2;
	}
	ret = registfile(fh, buffersize, hdr, hdrsize);
	if (ret) {
		return(ret);
	}

tfc_err2:
	_file_close(fh);

tfc_err1:
	return(NULL);
}

BRESULT textfile_read(TEXTFILEH tfh, OEMCHAR *buffer, UINT size) {

	TEXTFILE	tf;

	tf = (TEXTFILE)tfh;
	if (tf == NULL) {
		return(FAILURE);
	}
#if defined(SUPPORT_TEXTCNV)
	if (!(tf->access & TEXTCNV_READ)) {
		return(FAILURE);
	}
#endif	// defined(SUPPORT_TEXTCNV)
	if (!(tf->mode & TFMODE_READ)) {
		flushfile(tf);
		tf->mode = TFMODE_READ;
	}
	return((tf->readfn)(tf, buffer, size));
}

BRESULT textfile_write(TEXTFILEH tfh, const OEMCHAR *buffer) {

	TEXTFILE	tf;
const void		*buf;
	UINT		leng;

	tf = (TEXTFILE)tfh;
	if (tf == NULL) {
		return(FAILURE);
	}
#if defined(SUPPORT_TEXTCNV)
	if (!(tf->access & TEXTCNV_WRITE)) {
		return(FAILURE);
	}
#endif	// defined(SUPPORT_TEXTCNV)
	if (!(tf->mode & TFMODE_WRITE)) {
		flushfile(tf);
		tf->mode = TFMODE_WRITE;
	}
	leng = (UINT)OEMSTRLEN(buffer);
#if defined(SUPPORT_TEXTCNV)
	if (tf->fromoem != NULL) {
		leng = (tf->fromoem)(tf->cnvbuf, tf->cnvbufsize / tf->width,
																buffer, leng);
		buf = tf->cnvbuf;
	}
	else {
		buf = buffer;
	}
#else	// defined(SUPPORT_TEXTCNV)
	buf = buffer;
#endif	// defined(SUPPORT_TEXTCNV)
	if (tf->width == 1) {
		return(writebufferA(tf, buf, leng));
	}
	else if (tf->width == 2) {
		return(writebufferW(tf, buf, leng));
	}
	else {
		return(FAILURE);
	}
}

void textfile_close(TEXTFILEH tfh) {

	TEXTFILE	tf;

	tf = (TEXTFILE)tfh;
	if (tf) {
		if (tf->mode & TFMODE_WRITE) {
			flushfile(tf);
		}
		_file_close(tf->fh);
		_MFREE(tf);
	}
}

