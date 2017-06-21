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

static int cnvmain(const char *srcfile, const char *dstfile,
															const char *sym) {

	int		ret;
	FILEH	fh;
	void	*dst;
	char	work[256];
	BYTE	buf[12];
	UINT	size;
	UINT	i;

	ret = _NOERROR;
	fh = file_open(srcfile);
	if (fh == FILEH_INVALID) {
		printf("... open error\n");
		ret = ERROR_INPUT;
		goto c_err1;
	}
	dst = textout_open(dstfile, 256);

	if (sym == NULL) {
		sym = srcfile;
	}
	SPRINTF(work, "static const unsigned char %s[] = {\n", sym);
	textout_write(dst, work);

	while(1) {
		size = file_read(fh, buf, 12);
		if (size == 0) {
			break;
		}
		for (i=0; i<size; i++) {
			SPRINTF(work + i*5, "0x%02x,", buf[i]);
		}
		textout_write(dst, "\t\t\t");
		textout_write(dst, work);
		textout_write(dst, "\n");
	}
	textout_write(dst, "};\n");

	textout_close(dst);

	file_close(fh);

c_err1:
	return(ret);
}


// ----

static const char progorg[] = "bin2txt";

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
	ret = cnvmain(src, dst, sym);

main_exit:
	dosio_term();
	return(ret);
}

