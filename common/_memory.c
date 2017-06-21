#include	"compiler.h"


#define	MEMTBLMAX	256
#define	HDLTBLMAX	256


#if defined(MEMTRACE)

#include	"strres.h"
#include	"dosio.h"

#if defined(MACOS)
#define	CRLITERAL	"\r"
#elif defined(X11)
#define	CRLITERAL	"\n"
#else
#define	CRLITERAL	"\r\n"
#endif
static const char s_cr[] = CRLITERAL;

typedef struct {
	void	*hdl;
	UINT	size;
	char	name[24];
} _MEMTBL;

typedef struct {
	void	*hdl;
	char	name[28];
} _HDLTBL;

static _MEMTBL	memtbl[MEMTBLMAX];
static _HDLTBL	hdltbl[HDLTBLMAX];

static const char str_memhdr[] =											\
				"Handle   Size       Name" CRLITERAL						\
				"--------------------------------------------" CRLITERAL;

static const char str_hdlhdr[] =											\
				"Handle   Name" CRLITERAL									\
				"-------------------------------------" CRLITERAL;

static const char str_memused[] = "memused: %d" CRLITERAL;

void _meminit(void) {

	ZeroMemory(memtbl, sizeof(memtbl));
	ZeroMemory(hdltbl, sizeof(hdltbl));
}

void *_memalloc(int size, const char *name) {

	void	*ret;
	int		i;

	ret = malloc(size);
	if (ret) {
		for (i=0; i<MEMTBLMAX; i++) {
			if (memtbl[i].hdl == NULL) {
				memtbl[i].hdl = ret;
				memtbl[i].size = size;
				strcpy(memtbl[i].name, name);
				break;
			}
		}
	}
	return(ret);
}

void _memfree(void *hdl) {

	int		i;

	if (hdl) {
		for (i=0; i<MEMTBLMAX; i++) {
			if (memtbl[i].hdl == hdl) {
				memtbl[i].hdl = NULL;
				break;
			}
		}
		free(hdl);
	}
}

void _handle_append(void *hdl, const char *name) {

	int		i;

	if (hdl) {
		for (i=0; i<HDLTBLMAX; i++) {
			if (hdltbl[i].hdl == NULL) {
				hdltbl[i].hdl = hdl;
				strcpy(hdltbl[i].name, name);
				break;
			}
		}
	}
}

void _handle_remove(void *hdl) {

	if (hdl) {
		int		i;
		for (i=0; i<HDLTBLMAX; i++) {
			if (hdltbl[i].hdl == hdl) {
				hdltbl[i].hdl = NULL;
				break;
			}
		}
	}
}

void _memused(const OEMCHAR *filename) {

	int		i;
	FILEH	fh;
	int		memuses = 0;
	int		hdluses = 0;
	UINT8	memusebit[(MEMTBLMAX+7)/8];
	UINT8	hdlusebit[(HDLTBLMAX+7)/8];
	char	work[256];

	ZeroMemory(memusebit, sizeof(memusebit));
	ZeroMemory(hdlusebit, sizeof(hdlusebit));
	for (i=0; i<MEMTBLMAX; i++) {
		if (memtbl[i].hdl) {
			memusebit[i>>3] |= (UINT8)(0x80 >> (i & 7));
			memuses++;
		}
	}
	for (i=0; i<HDLTBLMAX; i++) {
		if (hdltbl[i].hdl) {
			hdlusebit[i>>3] |= (UINT8)(0x80 >> (i & 7));
			hdluses++;
		}
	}
	fh = file_create_c(filename);
	if (fh != FILEH_INVALID) {
		sprintf(work, "memused: %d\r\n");
		file_write(fh, work, strlen(work));
		if (memuses) {
			file_write(fh, str_memhdr, strlen(str_memhdr));
			for (i=0; i<MEMTBLMAX; i++) {
				if ((memusebit[i>>3] << (i & 7)) & 0x80) {
					sprintf(work, "%08lx %10u %s\r\n",
						(long)memtbl[i].hdl, memtbl[i].size, memtbl[i].name);
					file_write(fh, work, strlen(work));
				}
			}
			file_write(fh, s_cr, strlen(s_cr));
		}
		sprintf(work, "hdlused: %d\r\n", hdluses);
		file_write(fh, work, strlen(work));
		if (hdluses) {
			file_write(fh, str_hdlhdr, strlen(str_hdlhdr));
			for (i=0; i<HDLTBLMAX; i++) {
				if ((hdlusebit[i>>3] << (i & 7)) & 0x80) {
					sprintf(work, "%08lx %s\r\n",
									(long)hdltbl[i].hdl, hdltbl[i].name);
					file_write(fh, work, strlen(work));
				}
			}
			file_write(fh, s_cr, strlen(s_cr));
		}
		file_close(fh);
	}
}

#elif defined(MEMCHECK)

typedef struct {
	void	*hdl;
	UINT	size;
} _MEMTBL;

		BOOL	chgmemory;
		UINT	usedmemory;

static	_MEMTBL	memtbl[MEMTBLMAX];

void _meminit(void) {

	usedmemory = 0;
	chgmemory = FALSE;
	ZeroMemory(memtbl, sizeof(memtbl));
}

void *_memalloc(int size) {

	void	*ret;
	int		i;

	ret = malloc(size);
	if (ret) {
		for (i=0; i<MEMTBLMAX; i++) {
			if (memtbl[i].hdl == NULL) {
				memtbl[i].hdl = ret;
				memtbl[i].size = size;
				usedmemory += size;
				chgmemory = TRUE;
				break;
			}
		}
	}
	return(ret);
}

void _memfree(void *hdl) {

	int		i;

	if (hdl) {
		for (i=0; i<MEMTBLMAX; i++) {
			if (memtbl[i].hdl == hdl) {
				memtbl[i].hdl = NULL;
				usedmemory -= memtbl[i].size;
				chgmemory = TRUE;
				break;
			}
		}
		free(hdl);
	}
}

#endif

