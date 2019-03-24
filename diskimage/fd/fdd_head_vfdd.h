enum {
	VFDD_TRKMAX		= 160,
	VFDD_SECMAX		= 26,
	VFDD_HEADERSIZE	= 220 + (12 * VFDD_TRKMAX * VFDD_SECMAX) + 32,
};

#if defined(__GNUC__)
//	仮想ＦＤＤヘッダー部
typedef struct {
	SINT8	verID[8];		//	バージョンを表すＩＤ
	SINT8	memo[128];		//	ディスクメモ
	SINT16	write_protect;	//	書き込み禁止
	SINT16	spdrv;			//	特殊読み込みドライブ
	SINT8	dmy[80];		//	予約領域
} __attribute__ ((packed)) _VFDD_HEAD, *VFDD_HEAD;

//	仮想ＦＤＤのＩＤデータ
typedef struct {
	UINT8	C;			//	Ｃ シリンダ番号
	UINT8	H;			//	Ｈ サーフェース番号
	UINT8	R;			//	Ｒ セクタ番号
	UINT8	N;			//	Ｎ セクタ長
	UINT8	D;			//	Ｄ データパターン
	UINT8	DDAM;		//	ＤＤＡＭ デリーテッドデータフラグ
	UINT8	flMF;		//	ＭＦ 倍密度フラグ
	UINT8	flHD;		//	２ＨＤフラグ
	SINT32	dataPoint;	//	データへのファイルポインタ
} __attribute__ ((packed)) _VFDD_ID, *VFDD_ID;

/*	特殊読み込み時のデータ	*/
typedef struct {
	SINT16	trk;		//	現在のトラック位置
	UINT16	iax;		//	入力レジスタの値
	UINT16	ibx;
	UINT16	icx;
	UINT16	idx;
	UINT16	oax;		//	出力レジスタの値
	UINT16	obx;
	UINT16	ocx;
	UINT16	odx;
	UINT16	ofl;
	SINT32	dataPoint;	//	データへのファイルポインタ
	SINT32	nextPoint;	//	次のデータへのファイルポインタ
	SINT16	count;		//	特殊読み込みカウンタ
	SINT16	neg_count;	//	カウント無視フラグ
} __attribute__ ((packed)) _VFDD_SP, *VFDD_SP;
#else
#pragma pack(push, 1)
//	仮想ＦＤＤヘッダー部
typedef struct {
	SINT8	verID[8];		//	バージョンを表すＩＤ
	SINT8	memo[128];		//	ディスクメモ
	SINT16	write_protect;	//	書き込み禁止
	SINT16	spdrv;			//	特殊読み込みドライブ
	SINT8	dmy[80];		//	予約領域
} _VFDD_HEAD, *VFDD_HEAD;

//	仮想ＦＤＤのＩＤデータ
typedef struct {
	UINT8	C;			//	Ｃ シリンダ番号
	UINT8	H;			//	Ｈ サーフェース番号
	UINT8	R;			//	Ｒ セクタ番号
	UINT8	N;			//	Ｎ セクタ長
	UINT8	D;			//	Ｄ データパターン
	UINT8	DDAM;		//	ＤＤＡＭ デリーテッドデータフラグ
	UINT8	flMF;		//	ＭＦ 倍密度フラグ
	UINT8	flHD;		//	２ＨＤフラグ
	SINT32	dataPoint;	//	データへのファイルポインタ
} _VFDD_ID, *VFDD_ID;

/*	特殊読み込み時のデータ	*/
typedef struct {
	SINT16	trk;		//	現在のトラック位置
	UINT16	iax;		//	入力レジスタの値
	UINT16	ibx;
	UINT16	icx;
	UINT16	idx;
	UINT16	oax;		//	出力レジスタの値
	UINT16	obx;
	UINT16	ocx;
	UINT16	odx;
	UINT16	ofl;
	SINT32	dataPoint;	//	データへのファイルポインタ
	SINT32	nextPoint;	//	次のデータへのファイルポインタ
	SINT16	count;		//	特殊読み込みカウンタ
	SINT16	neg_count;	//	カウント無視フラグ
} _VFDD_SP, *VFDD_SP;
#pragma pack(pop)
#endif

