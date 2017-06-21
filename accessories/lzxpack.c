#include	"compiler.h"
#include	"dosio.h"
#include	"textout.h"

enum {
	_NOERROR		= 0,
	ERROR_INPUT,
	ERROR_OUTPUT,
	ERROR_MEMORY,
	ERROR_SYSTEM
};

typedef struct {
	BYTE	*ptr;
	int		size;
} _PACK, *PACK;

static int lzpacking(const BYTE *data, int datasize, PACK lz, int level) {

	int		maxlen;
	int		maxhis;
	BYTE	*ptr;
	BYTE	*ctrl;
	BYTE	bit;
	int		pos;
	int		back;
	int		rem;
const BYTE	*p;
const BYTE	*q;
	int		r;
	int		minback;
	int		minr;
	UINT	tmp;

	maxlen = 1 << level;
	maxhis = 1 << (16 - level);

	ptr = lz->ptr;
	ptr[0] = (BYTE)datasize;
	ptr[1] = (BYTE)(datasize >> 8);
	ptr[2] = (BYTE)(datasize >> 16);
	ptr[3] = (BYTE)level;
	ptr += 4;
	bit = 0;
	pos = 0;
	while(pos < datasize) {
		if (bit == 0) {
			bit = 0x80;
			ctrl = ptr;
			*ptr++ = 0x00;
		}
		back = min(pos, maxhis);
		rem = min(datasize - pos, maxlen);
		minback = back;
		minr = rem;
		while(back) {
			p = data - back;
			q = data;
			r = rem;
			while(r) {
				if (*p++ != *q++) {
					break;
				}
				r--;
			}
			if (minr >= r) {
				minr = r;
				minback = back;
			}
			back--;
		}
		r = rem - minr;
		if (r > 2) {
			data += r;
			pos += r;
			tmp = ((minback - 1) << level) | (r - 1);
			*ctrl |= bit;
			*ptr++ = (BYTE)(tmp >> 8);
			*ptr++ = (BYTE)tmp;
		}
		else {
			*ptr++ = *data++;
			pos++;
		}
		bit >>= 1;
	}
	lz->size = ptr - lz->ptr;
	return(lz->size);
}

static PACK lzpack(const BYTE *data, int datasize) {

	PACK	ret;
	int		memsize;
	int		i;
	int		level;
	int		minsize;

	memsize = (datasize + 7) / 8;
	memsize *= 9;
	memsize += 8;
	ret = (PACK)_MALLOC(sizeof(_PACK) + memsize, "LZ");
	if (ret == NULL) {
		goto lz_exit;
	}
	ret->ptr = (BYTE *)(ret + 1);

	level = 0;
	minsize = memsize;
	for (i=0; i<=16; i++) {
		lzpacking(data, datasize, ret, i);
		if (minsize > ret->size) {
			minsize = ret->size;
			level = i;
		}
		printf(".");
	}
	lzpacking(data, datasize, ret, level);

lz_exit:
	return(ret);
}


// ----

static void stringout(void *dst, const BYTE *ptr, UINT size) {

	int		step;
	char	work[16];

	step = 0;
	while(size--) {
		if (!step) {
			textout_write(dst, "\t\t");
		}
		SPRINTF(work, "0x%02x,", (BYTE)*ptr++);
		textout_write(dst, work);
		step++;
		if (step > 12) {
			textout_write(dst, "\n");
			step = 0;
		}
	}
	if (step) {
		textout_write(dst, "\n");
	}
}


// ----

static int packmain(const char *srcfile, const char *dstfile,
															const char *sym) {

	int		ret;
	FILEH	src;
	void	*dst;
	BYTE	*tmp;
	UINT	size;
	PACK	lz;
	char	work[256];

	ret = _NOERROR;
	printf("%s", srcfile);
	src = file_open(srcfile);
	if (src == FILEH_INVALID) {
		printf("... open error\n");
		ret = ERROR_INPUT;
		goto rf_err1;
	}
	size = file_getsize(src);
	if (size == 0) {
		printf("... size zero\n");
		ret = ERROR_INPUT;
		goto rf_err2;
	}
	tmp = (BYTE *)_MALLOC(size, filename);
	if (tmp == NULL) {
		printf("... memory error\n");
		ret = ERROR_MEMORY;
		goto rf_err2;
	}
	if (file_read(src, tmp, size) != size) {
		printf("... read error\n");
		ret = ERROR_INPUT;
		goto rf_err3;
	}

	dst = textout_open(dstfile, 256);

	lz = lzpack(tmp, size);
	printf("done\n");

	if (lz) {
		if (sym == NULL) {
			sym = srcfile;
		}
		SPRINTF(work, "static const BYTE %s[%d] = {\n", sym, lz->size);
		textout_write(dst, work);
		stringout(dst, lz->ptr, lz->size);
		textout_write(dst, "};\n");
		_MFREE(lz);
	}

	textout_close(dst);

rf_err3:
	_MFREE(tmp);

rf_err2:
	file_close(src);

rf_err1:
	return(ret);
}


// ----

static const char progorg[] = "lzxpack";

int main(int argc, char *argv[], char *envp[]) {

	int		ret;
const char	*prog;
const char	*src;
const char	*dst;
const char	*sym;
	int		pos;
const char	*p;

	dosio_init();

	ret = _NOERROR;
	prog = progorg;
	if (argc >= 1) {
		prog = argv[0];
	}
	src = NULL;
	dst = NULL;
	sym = NULL;
	pos = 1;
	while(pos < argc) {
		p = argv[pos++];
		if (p[0] == '-') {
			if (((p[1] == 'S') || (p[1] == 's')) && (sym == NULL)) {
				sym = p + 2;
			}
		}
		else if (src == NULL) {
			src = p;
		}
		else if (dst == NULL) {
			dst = p;
		}
	}

	if (src == NULL) {
		printf("%s: error: no input file\n", prog);
		ret = ERROR_SYSTEM;
		goto main_exit;
	}
	ret = packmain(src, dst, sym);

main_exit:
	dosio_term();
	return(ret);
}

