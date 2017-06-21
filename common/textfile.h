
typedef	void 		*TEXTFILEH;

#ifdef __cplusplus
extern "C" {
#endif

TEXTFILEH textfile_open(const OEMCHAR *filename, UINT buffersize);
TEXTFILEH textfile_create(const OEMCHAR *filename, UINT buffersize);
BRESULT textfile_read(TEXTFILEH fh, OEMCHAR *buffer, UINT size);
BRESULT textfile_write(TEXTFILEH fh, const OEMCHAR *buffer);
void textfile_close(TEXTFILEH fh);

#ifdef __cplusplus
}
#endif

