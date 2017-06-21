#include	"compiler.h"
#include	"dosio.h"
#include	"textout.h"


typedef struct {
	FILEH	fh;
	int		pos;
	int		size;
} _TEXTOUT, *TEXTOUT;


void *textout_open(const char *filename, int bufsize) {

	TEXTOUT	to;
	FILEH	fh;

	to = NULL;
	if (filename == NULL) {
		goto toope_exit;
	}
	fh = file_create(filename);
	if (fh == NULL) {
		goto toope_exit;
	}
	bufsize = max(bufsize, 256);
	to = (TEXTOUT)_MALLOC(sizeof(_TEXTOUT) + bufsize, filename);
	if (to == NULL) {
		goto toope_exit;
	}
	to->fh = fh;
	to->pos = 0;
	to->size = bufsize;

toope_exit:
	return((void *)to);
}

static void writechar(TEXTOUT to, char c) {

	((BYTE *)(to + 1))[to->pos] = c;
	to->pos++;
	if (to->pos >= to->size) {
		to->pos = 0;
		file_write(to->fh, to + 1, to->size);
	}
}

void textout_write(void *hdl, const char *string) {

	TEXTOUT	to;
	char	c;

	if (string == NULL) {
		return;
	}
	to = (TEXTOUT)hdl;
	if (to == NULL) {
		printf("%s", string);
	}
	else {
		while(*string) {
			c = *string++;
#if defined(OSLINEBREAK_CR)
			if (c == '\n') {
				c = '\r';
			}
#elif defined(OSLINEBREAK_CRLF)
			if (c == '\n') {
				writechar(to, '\r');
			}
#endif
			writechar(to, c);
		}
	}
}

void textout_close(void *hdl) {

	TEXTOUT	to;

	to = (TEXTOUT)hdl;
	if (to) {
		if (to->pos) {
			file_write(to->fh, to + 1, to->pos);
		}
		file_close(to->fh);
		_MFREE(to);
	}
}

