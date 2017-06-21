#define		FILEH	short

#define		FSEEK_SET	0
#define		FSEEK_CUR	1
#define		FSEEK_END	2

extern	BYTE	CHICAGO;

typedef struct {
	WORD	attr;
	WORD	attr2;
	DWORD	ftCreate[2];
	DWORD	ftLastAccess[2];
	DWORD	ftLastWrite[2];
	DWORD	filesize;
	DWORD	filesize_high;
	DWORD	reserved1[2];
	BYTE	realname[260];
	BYTE	dosname[14];
	WORD	handle;
} FILEFIND32;


											// DOSIO:関数の準備
BYTE	dosio_init(void);
											// ファイル操作
FILEH	file_open(BYTE *filename);
FILEH	file_create(BYTE *filename);
long	file_seek(FILEH handle, long pointer, short mode);
WORD	file_lread(FILEH handle, void far *data, unsigned short length);
WORD	file_read(FILEH handle, void far *data, unsigned short length);
WORD	file_write(FILEH handle, void far *data, unsigned short length);
short	file_close(FILEH handle);
short	file_attr(BYTE *filename);
											// カレントファイル操作
void	file_setcd(BYTE *exename);
FILEH	file_open_c(BYTE *filename);
FILEH	file_create_c(BYTE *filename);
short	file_attr_c(BYTE *filename);

											// 日時の取得
void	dosdateset(BYTE *dat);
void	dostimeset(BYTE *dat);
											// 致命的エラーのフック
void	int24_set(void);
void	int24_reset(void);
											// ドライブのテキスト色取得
BYTE	check_drive(BYTE drive);
											// ファイル検索関数
BYTE	file_find1st(void *path, WORD attr, FILEFIND32 *dta);
BYTE	file_findnext(FILEFIND32 *dta);
void	file_findclose(void);


#define	EXNAME2LONG		0x8002
#define	EXNAME2SHORT	0x8001

void	file_exname(BYTE *src, BYTE *dst, WORD func);

WORD	sjis2jis(WORD sjis);
