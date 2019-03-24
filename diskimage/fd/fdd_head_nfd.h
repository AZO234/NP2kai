enum {
	NFD_TRKMAX		= 163,
	NFD_TRKMAX1		= 164,
	NFD_SECMAX		= 26,
	NFD_HEADERSIZE	= 288 + (16 * NFD_TRKMAX * NFD_SECMAX) + 0x10,
	NFD_HEADERSIZE1	= 288 + (16 * NFD_TRKMAX * NFD_SECMAX) + 0x10,
};

#if defined(__GNUC__)
//	セクタID
typedef struct {
	BYTE	C;									//	C（0xFFの時セクタ無し）
	BYTE	H;									//	H
	BYTE	R;									//	R
	BYTE	N;									//	N
	BYTE	flMFM;								//	0:FM / 1:MFM
	BYTE	flDDAM;								//	0:DAM / 1:DDAM
	BYTE	byStatus;							//	READ DATA(FDDBIOS)の結果
	BYTE	byST0;								//	READ DATA(FDDBIOS)の結果 ST0
	BYTE	byST1;								//	READ DATA(FDDBIOS)の結果 ST1
	BYTE	byST2;								//	READ DATA(FDDBIOS)の結果 ST2
	BYTE	byPDA;								//	FDDBIOSで使用するアドレス
	char	Reserve1[5];						//	予約
} __attribute__ ((packed)) NFD_SECT_ID, *LP_NFD_SECT_ID;

//	nfdヘッダ(r0)
typedef struct {
	char		szFileID[15];					//	識別ID "T98FDDIMAGE.R0"
	char		Reserve1[1];					//	予約
	char		szComment[0x100];				//	イメージコメント(ASCIIz)
	DWORD		dwHeadSize;						//	ヘッダ部のサイズ
	BYTE		flProtect;						//	0以外 : ライトプロテクト
	BYTE		byHead;							//	ヘッド数
	char		Reserve2[10];					//	予約
	NFD_SECT_ID	si[NFD_TRKMAX][NFD_SECMAX];		//	セクタID
	char		Reserve3[0x10];					//	予約
} __attribute__ ((packed)) NFD_FILE_HEAD, *LP_NFD_FILE_HEAD;

//	nfdヘッダ(r1)
typedef struct {
//	char	szFileID[sizeof(NFD_FILE_ID1)];		//	識別ID	"T98FDDIMAGE.R1"
//	char	Reserv1[0x10-sizeof(NFD_FILE_ID1)];	//	予備
	char	szFileID[15];						//	識別ID	"T98FDDIMAGE.R1"
	char	Reserv1[1];							//	予備
	char	szComment[0x100];					//	コメント
	DWORD	dwHeadSize;							//	ヘッダのサイズ
	BYTE	flProtect;							//	ライトプロテクト0以外
	BYTE	byHead;								//	ヘッド数	1-2
	char	Reserv2[0x10-4-1-1];				//	予備
	DWORD	dwTrackHead[NFD_TRKMAX1];			//	トラックID位置
	DWORD	dwAddInfo;							//	追加情報ヘッダのアドレス
	char	Reserv3[0x10-4];					//	予備
} __attribute__ ((packed)) NFD_FILE_HEAD1, *LP_NFD_FILE_HEAD1;

//	トラックID
typedef struct {
	WORD	wSector;							//	セクタID数
	WORD	wDiag;								//	特　殊ID数
	char	Reserv1[0x10-4];					//	予備
} __attribute__ ((packed)) NFD_TRACK_ID1, *LP_NFD_TRACK_ID1;

//	セクタ情報ヘッダ
typedef struct {
	BYTE	C;									//	C
	BYTE	H;									//	H
	BYTE	R;									//	R
	BYTE	N;									//	N
	BYTE	flMFM;								//	MFM(1)/FM(0)
	BYTE	flDDAM;								//	DDAM(1)/DAM(0)
	BYTE	byStatus;							//	READ DATA RESULT
	BYTE	bySTS0;								//	ST0
	BYTE	bySTS1;								//	ST1
	BYTE	bySTS2;								//	ST2
	BYTE	byRetry;							//	RetryDataなし(0)あり(1-)
	BYTE	byPDA;								//	PDA
	char	Reserv1[0x10-12];					//	予備
} __attribute__ ((packed)) NFD_SECT_ID1, *LP_NFD_SECT_ID1;

//	特殊読み込み情報ヘッダ
typedef struct {
	BYTE	Cmd;								//	Command
	BYTE	C;									//	C
	BYTE	H;									//	H
	BYTE	R;									//	R
	BYTE	N;									//	N
	BYTE	byStatus;							//	READ DATA RESULT
	BYTE	bySTS0;								//	ST0
	BYTE	bySTS1;								//	ST1
	BYTE	bySTS2;								//	ST2
	BYTE	byRetry;							//	RetryDataなし(0)あり(1-)
	DWORD	dwDataLen;							//	転送を行うデータサイズ
	BYTE	byPDA;								//	PDA
	char	Reserv1[0x10-15];					//	予備
} __attribute__ ((packed)) NFD_DIAG_ID1, *LP_NFD_DIAG_ID1;
#else
#pragma pack(push, 1)
//	セクタID
typedef struct {
	BYTE	C;									//	C（0xFFの時セクタ無し）
	BYTE	H;									//	H
	BYTE	R;									//	R
	BYTE	N;									//	N
	BYTE	flMFM;								//	0:FM / 1:MFM
	BYTE	flDDAM;								//	0:DAM / 1:DDAM
	BYTE	byStatus;							//	READ DATA(FDDBIOS)の結果
	BYTE	byST0;								//	READ DATA(FDDBIOS)の結果 ST0
	BYTE	byST1;								//	READ DATA(FDDBIOS)の結果 ST1
	BYTE	byST2;								//	READ DATA(FDDBIOS)の結果 ST2
	BYTE	byPDA;								//	FDDBIOSで使用するアドレス
	char	Reserve1[5];						//	予約
} NFD_SECT_ID, *LP_NFD_SECT_ID;

//	nfdヘッダ(r0)
typedef struct {
	char		szFileID[15];					//	識別ID	"T98FDDIMAGE.R0"
	char		Reserve1[1];					//	予約
	char		szComment[0x100];				//	イメージコメント(ASCIIz)
	DWORD		dwHeadSize;						//	ヘッダ部のサイズ
	BYTE		flProtect;						//	0以外 : ライトプロテクト
	BYTE		byHead;							//	ヘッド数
	char		Reserve2[10];					//	予約
	NFD_SECT_ID	si[NFD_TRKMAX][NFD_SECMAX];		//	セクタID
	char		Reserve3[0x10];					//	予約
} NFD_FILE_HEAD, *LP_NFD_FILE_HEAD;

//	nfdヘッダ(r1)
typedef struct {
//	char	szFileID[sizeof(NFD_FILE_ID1)];		//	識別ID	"T98FDDIMAGE.R1"
//	char	Reserv1[0x10-sizeof(NFD_FILE_ID1)];	//	予備
	char	szFileID[15];						//	識別ID	"T98FDDIMAGE.R1"
	char	Reserv1[1];							//	予備
	char	szComment[0x100];					//	コメント
	DWORD	dwHeadSize;							//	ヘッダのサイズ
	BYTE	flProtect;							//	ライトプロテクト0以外
	BYTE	byHead;								//	ヘッド数	1-2
	char	Reserv2[0x10-4-1-1];				//	予備
	DWORD	dwTrackHead[NFD_TRKMAX1];			//	トラックID位置
	DWORD	dwAddInfo;							//	追加情報ヘッダのアドレス
	char	Reserv3[0x10-4];					//	予備
} NFD_FILE_HEAD1, *LP_NFD_FILE_HEAD1;

//	トラックID
typedef struct {
	WORD	wSector;							//	セクタID数
	WORD	wDiag;								//	特　殊ID数
	char	Reserv1[0x10-4];					//	予備
} NFD_TRACK_ID1, *LP_NFD_TRACK_ID1;

//	セクタ情報ヘッダ
typedef struct {
	BYTE	C;									//	C
	BYTE	H;									//	H
	BYTE	R;									//	R
	BYTE	N;									//	N
	BYTE	flMFM;								//	MFM(1)/FM(0)
	BYTE	flDDAM;								//	DDAM(1)/DAM(0)
	BYTE	byStatus;							//	READ DATA RESULT
	BYTE	bySTS0;								//	ST0
	BYTE	bySTS1;								//	ST1
	BYTE	bySTS2;								//	ST2
	BYTE	byRetry;							//	RetryDataなし(0)あり(1-)
	BYTE	byPDA;								//	PDA
	char	Reserv1[0x10-12];					//	予備
} NFD_SECT_ID1, *LP_NFD_SECT_ID1;

//	特殊読み込み情報ヘッダ
typedef struct {
	BYTE	Cmd;								//	Command
	BYTE	C;									//	C
	BYTE	H;									//	H
	BYTE	R;									//	R
	BYTE	N;									//	N
	BYTE	byStatus;							//	READ DATA RESULT
	BYTE	bySTS0;								//	ST0
	BYTE	bySTS1;								//	ST1
	BYTE	bySTS2;								//	ST2
	BYTE	byRetry;							//	RetryDataなし(0)あり(1-)
	DWORD	dwDataLen;							//	転送を行うデータサイズ
	BYTE	byPDA;								//	PDA
	char	Reserv1[0x10-15];					//	予備
} NFD_DIAG_ID1, *LP_NFD_DIAG_ID1;
#pragma pack(pop)
#endif

