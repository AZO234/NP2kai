
#ifdef __cplusplus
extern "C" {
#endif

enum {
	SXSIMEDIA_DATA = 0x10,
	SXSIMEDIA_AUDIO = 0x20
};

#ifdef SUPPORT_KAI_IMAGES

// WinDDKの構造体の名前と被っちゃったので TRACK -> TRACKTYPE に変更 np21w ver0.86 rev33
#define	TRACKTYPE_DATA	0x14
#define	TRACKTYPE_AUDIO	0x10

typedef struct {
	UINT8	adr_ctl;		//	Adr/Ctl
							//		ISO:0x14
							//		CUE:MODE1=0x14、MODE2=0x14、AUDIO=0x10
							//		CCD:
							//		CDM:
							//		MDS:MDS_TrackBlock.adr_ctl
	UINT8	point;			//	Track Number
							//		ISO:1
							//		CUE:TRACK ??=??
							//		CCD:
							//		CDM:
							//		MDS:MDS_TrackBlock.point
	UINT32	pos;			//	トラックのイメージファイル内での開始セクタ位置
							//		ISO:0
							//		CUE:INDEX 1
							//		CCD:
							//		CDM:
							//		MDS:((MDS_TrackBlock.min * 60) + MDS_TrackBlock.sec) * 75 + MDS_TrackBlock.frame
//	--------
	UINT32	pos0;			//	CUEシートの"INDEX 00"等で指定されたPREGAPのイメージ内での開始セクタ位置
							//		ISO:0
							//		CUE:INDEX 0
							//		CCD:
							//		CDM:
							//		MDS:0
	UINT32	str_sec;		//	トラックのイメージファイル上での開始セクタ位置
							//		ISO:
							//		CUE:
							//		CCD:
							//		CDM:
							//		MDS:
	UINT32	end_sec;		//	トラックのイメージファイル上での終了セクタ位置
							//		ISO:
							//		CUE:
							//		CCD:
							//		CDM:
							//		MDS:
	UINT32	sectors;		//	トラックのイメージファイル上でのセクタ数
							//		ISO:
							//		CUE:
							//		CCD:
							//		CDM:
							//		MDS:
//	--------
	UINT16	sector_size;	//	トラックのセクタサイズ
							//		ISO:2048 or 2352 or 2448(ファイルサイズを割って余りの出ない数値)
							//		CUE:MODE1/????=????、MODE2/????=????、AUDIO=2352
							//		CCD:2352(固定で正しい？)
							//		CDM:2352(固定で正しい？)
							//		MDS:MDS_TrackBlock.sector_size
							//		NRG:NRG_DAO_Block.sector_size

	//	CD上の各セクタ開始位置
	//	※イメージファイル上のPREGAPの扱いによって後述のイメージファイル上の各セクタ開始位置と
	//	　ずれた値になることもある
	UINT32	pregap_sector;	//	トラックのPREGAP開始セクタ位置
							//	※PREGAPが無い場合やPREGAPの実体が無い場合は
							//	　トラックのstart_sectorと同じ値
							//		ISO:0
							//		CUE:
							//		CCD:
							//		CDM:
							//		MDS:start_sector - pregap_sectors
	UINT32	start_sector;	//	トラックの開始セクタ位置
							//		ISO:0
							//		CUE:
							//		CCD:
							//		CDM:
							//		MDS:MDS_TrackBlock.start_sector(リトルエンディアン)
	UINT32	end_sector;		//	トラックの終了セクタ位置
							//		ISO:track_sectors - 1
							//		CUE:
							//		CCD:
							//		CDM:
							//		MDS:start_sector + track_sectors - 1

	//	イメージファイル上の各セクタ開始位置
	UINT32	img_pregap_sec;	//	トラックのPREGAP開始セクタ位置
							//	※PREGAPが無い場合やPREGAPの実体が無い場合は
							//	　トラックのstart_sectorと同じ値
							//		ISO:0
							//		CUE:
							//		CCD:
							//		CDM:
							//		MDS:
	UINT32	img_start_sec;	//	トラックの開始セクタ位置
							//		ISO:0
							//		CUE:
							//		CCD:
							//		CDM:
							//		MDS:
	UINT32	img_end_sec;	//	トラックの終了セクタ位置
							//		ISO:track_sectors - 1
							//		CUE:
							//		CCD:
							//		CDM:
							//		MDS:

	//	各セクタ開始位置のイメージファイル上でのoffset
	UINT64	pregap_offset;	//	イメージファイル上のトラックのPREGAPのoffset
							//	※通常は前トラックのend_offsetと同じ値
							//	※PREGAPが無い場合やPREGAPの実体が無い場合は
							//	　start_offsetと同じ値
							//		ISO:0
							//		CUE:
							//		CCD:
							//		CDM:
							//		MDS:start_offset
	UINT64	start_offset;	//	イメージファイル上のトラック開始位置のoffset
							//	※PREGAPが無い場合やPREGAPの実体が無い場合は
							//	　前トラックのend_offsetと同じ値
							//		ISO:0
							//		CUE:
							//		CCD:
							//		CDM:
							//		MDS:MDS_TrackBlock.start_offset(リトルエンディアン)
	UINT64	end_offset;		//	イメージファイル上のトラック終了位置のoffset
							//		ISO:track_sectors * sector_size
							//		CUE:
							//		CCD:
							//		CDM:
							//		MDS:start_offset + (track_sectors * sector_size)

	UINT32	pregap_sectors;	//	トラックのPREGAPのセクタ数
							//		ISO:0
							//		CUE:PREGAP
							//		CCD:
							//		CDM:
							//		MDS:MDS_TrackExtraBlock.pregap(リトルエンディアン)
	UINT32	track_sectors;	//	トラックのセクタ数
							//		ISO:ファイルサイズ / sector_size
							//		CUE:
							//		CCD:
							//		CDM:
							//		MDS:MDS_TrackExtraBlock.length(リトルエンディアン)
//	--------
} _CDTRK, *CDTRK;

#else
typedef struct {
	UINT8	type;
	UINT8	track;
	FILEPOS	pos;
} _CDTRK, *CDTRK;

#endif

BRESULT sxsicd_open(SXSIDEV sxsi, const OEMCHAR *fname);

CDTRK sxsicd_gettrk(SXSIDEV sxsi, UINT *tracks);
BRESULT sxsicd_readraw(SXSIDEV sxsi, FILEPOS pos, void *buf);

#ifdef __cplusplus
}
#endif

