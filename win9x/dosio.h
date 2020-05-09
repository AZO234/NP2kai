/**
 *	@file	dosio.h
 *	@brief	ファイル アクセス関数群の宣言およびインターフェイスの定義をします
 */

#pragma once

/*! コール規約 */
#define	DOSIOCALL	__stdcall


#define FILEH				HANDLE						/*!< ファイル ハンドル */
#define FILEH_INVALID		(INVALID_HANDLE_VALUE)		/*!< ファイル エラー値 */

#define FLISTH				HANDLE						/*!< ファイル検索ハンドル */
#define FLISTH_INVALID		(INVALID_HANDLE_VALUE)		/*!< ファイル検索エラー値 */

/**
 * ファイル ポインタ移動の開始点
 */
enum
{
	FSEEK_SET	= 0,				/*!< ファイルの先頭 */
	FSEEK_CUR	= 1,				/*!< 現在の位置 */
	FSEEK_END	= 2					/*!< ファイルの終わり */
};

/**
 * ファイル属性
 */
enum
{
	FILEATTR_READONLY	= 0x01,		/*!< 読み取り専用 */
	FILEATTR_HIDDEN		= 0x02,		/*!< 隠しファイル */
	FILEATTR_SYSTEM		= 0x04,		/*!< システム ファイル */
	FILEATTR_VOLUME		= 0x08,		/*!< ヴォリューム */
	FILEATTR_DIRECTORY	= 0x10,		/*!< ディレクトリ */
	FILEATTR_ARCHIVE	= 0x20,		/*!< アーカイブ ファイル */
	FILEATTR_NORMAL	= 0x80
};

/**
 * ファイル検索フラグ
 */
enum
{
	FLICAPS_SIZE		= 0x0001,	/*!< サイズ */
	FLICAPS_ATTR		= 0x0002,	/*!< 属性 */
	FLICAPS_DATE		= 0x0004,	/*!< 日付 */
	FLICAPS_TIME		= 0x0008	/*!< 時刻 */
};

/**
 * @brief DOSDATE 構造体
 */
struct _dosdate
{
	UINT16	year;			/*!< cx 年 */
	UINT8	month;			/*!< dh 月 */
	UINT8	day;			/*!< dl 日 */
};
typedef struct _dosdate		DOSDATE;		/*!< DOSDATE 定義 */

/**
 * @brief DOSTIME 構造体
 */
struct _dostime
{
	UINT8	hour;			/*!< ch 時 */
	UINT8	minute;			/*!< cl 分 */
	UINT8	second;			/*!< dh 秒 */
};
typedef struct _dostime		DOSTIME;		/*!< DOSTIME 定義 */

/**
 * @brief ファイル検索結果
 */
struct _flinfo
{
	UINT	caps;			/*!< フラグ */
	UINT32	size;			/*!< サイズ */
	UINT32	attr;			/*!< 属性 */
	DOSDATE	date;			/*!< 日付 */
	DOSTIME	time;			/*!< 時刻 */
	OEMCHAR	path[MAX_PATH];	/*!< ファイル名 */
};
typedef struct _flinfo		FLINFO;			/*!< FLINFO 定義 */

/* DOSIO:関数の準備 */
void dosio_init(void);
void dosio_term(void);

#ifdef __cplusplus
extern "C"
{
#endif

/* ファイル操作 */
FILEH DOSIOCALL file_open(const OEMCHAR* lpPathName);
FILEH DOSIOCALL file_open_rb(const OEMCHAR* lpPathName);
FILEH DOSIOCALL file_create(const OEMCHAR* lpPathName);
FILEPOS DOSIOCALL file_seek(FILEH hFile, FILEPOS pointer, int method);
UINT DOSIOCALL file_read(FILEH hFile, void *data, UINT length);
UINT DOSIOCALL file_write(FILEH hFile, const void *data, UINT length);
short DOSIOCALL file_close(FILEH hFile);
FILELEN DOSIOCALL file_getsize(FILEH hFile);
short DOSIOCALL file_getdatetime(FILEH hFile, DOSDATE* dosdate, DOSTIME* dostime);
short DOSIOCALL file_delete(const OEMCHAR* lpPathName);
short DOSIOCALL file_attr(const OEMCHAR* lpPathName);
short DOSIOCALL file_rename(const OEMCHAR* lpExistFile, const OEMCHAR* lpNewFile);
short DOSIOCALL file_dircreate(const OEMCHAR* lpPathName);
short DOSIOCALL file_dirdelete(const OEMCHAR* lpPathName);

/* カレントファイル操作 */
void DOSIOCALL file_setcd(const OEMCHAR* lpPathName);
OEMCHAR* DOSIOCALL file_getcd(const OEMCHAR* lpPathName);
FILEH DOSIOCALL file_open_c(const OEMCHAR* lpFilename);
FILEH DOSIOCALL file_open_rb_c(const OEMCHAR* lpFilename);
FILEH DOSIOCALL file_create_c(const OEMCHAR* lpFilename);
short DOSIOCALL file_delete_c(const OEMCHAR* lpFilename);
short DOSIOCALL file_attr_c(const OEMCHAR* lpFilename);

/* ファイル検索 */
FLISTH DOSIOCALL file_list1st(const OEMCHAR* lpPathName, FLINFO* fli);
BRESULT DOSIOCALL file_listnext(FLISTH hList, FLINFO* fli);
void DOSIOCALL file_listclose(FLISTH hList);

#define file_cpyname(a, b, c)	milstr_ncpy(a, b, c)		/*!< ファイル名コピー */
#define file_catname(a, b, c)	milstr_ncat(a, b, c)		/*!< ファイル名追加 */
#define file_cmpname(a, b)		milstr_cmp(a, b)			/*!< ファイル名比較 */
OEMCHAR* DOSIOCALL file_getname(const OEMCHAR* lpPathName);
void DOSIOCALL file_cutname(OEMCHAR* lpPathName);
OEMCHAR* DOSIOCALL file_getext(const OEMCHAR* lpPathName);
void DOSIOCALL file_cutext(OEMCHAR* lpPathName);
void DOSIOCALL file_cutseparator(OEMCHAR* lpPathName);
void DOSIOCALL file_setseparator(OEMCHAR* lpPathName, int cchPathName);

#ifdef __cplusplus
}
#endif

#define file_createex(p, t)		file_create(p)				/*!< ファイル作成 */
#define file_createex_c(p, t)	file_create_c(p)			/*!< ファイル作成 */
