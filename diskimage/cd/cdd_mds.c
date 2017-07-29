#include	"compiler.h"
#include	"dosio.h"
#include	"fdd/sxsi.h"

#ifdef SUPPORT_KAI_IMAGES

#include	"diskimage/cddfile.h"

#define	LOADINTELQWORD(a)		(*((UINT64 *)(a)))

static const OEMCHAR str_mdf[] = OEMTEXT(".mdf");

static const UINT8 mds_sig[16] =
						{'M','E','D','I','A',' ','D','E','S','C','R','I','P','T','O','R'};

#define	MDS_MEDIUM_CD			0x00	//	CD-ROM
#define	MDS_MEDIUM_CD_R			0x01	//	CD-R
#define	MDS_MEDIUM_CD_RW		0x02	//	CD-RW
#define	MDS_MEDIUM_DVD			0x10	//	DVD-ROM
#define	MDS_MEDIUM_DVD_MINUS_R	0x12	//	DVD-R
 
#define	MDS_TRACKMODE_UNKNOWN		0x00
#define	MDS_TRACKMODE_AUDIO			0xA9	//	sector size = 2352
#define	MDS_TRACKMODE_MODE1			0xAA	//	sector size = 2048
#define	MDS_TRACKMODE_MODE2			0xAB	//	sector size = 2336
#define	MDS_TRACKMODE_MODE2_FORM1	0xAC	//	sector size = 2048
#define	MDS_TRACKMODE_MODE2_FORM2	0xAD	//	sector size = 2324 (+4)

#define	MDS_SUBCHAN_NONE			0x00	//	no subchannel
#define	MDS_SUBCHAN_PW_INTERLEAVED	0x08	//	96-byte PW subchannel, interleaved

#define	MDS_POINT_TRACK_FIRST	0xA0	//	info about first track
#define	MDS_POINT_TRACK_LAST	0xA1	//	info about last track
#define	MDS_POINT_TRACK_LEADOUT	0xA2	//	info about lead-out

#if defined(__GNUC__)
typedef struct {
	UINT8	signature[16];			/*	0x0000	"MEDIA DESCRIPTOR"				*/
	UINT8	version[2];				/*	0x0010	Version ?						*/
	UINT16	medium_type;			/*	0x0012	Medium type						*/
	UINT16	num_sessions;			/*	0x0014	Number of sessions				*/
	UINT16	__dummy1__[2];			/*	0x0016	Wish I knew...					*/
	UINT16	bca_len;				/*	0x001A	Length of BCA data (DVD-ROM)	*/
	UINT32	__dummy2__[2];			/*	0x001C									*/
	UINT32	bca_data_offset;		/*	0x0026	Offset to BCA data (DVD-ROM)	*/
	UINT32	__dummy3__[6];			/*	0x002A	Probably more offsets			*/
	UINT32	disc_structures_offset;	/*	0x0042	Offset to disc structures		*/
	UINT32	__dummy4__[3];			/*	0x0046	Probably more offsets			*/
	UINT32	sessions_blocks_offset;	/*	0x0052	Offset to session blocks		*/
	UINT32	dpm_blocks_offset;		/*	0x0056	offset to DPM data blocks		*/
} __attribute__ ((packed)) MDS_Header;						/*	length: 88 bytes	*/

typedef struct {
	INT32	session_start;			/*	Session's start address							*/
	INT32	session_end;			/*	Session's end address							*/
	UINT16	session_number;			/*	(Unknown)										*/
	UINT8	num_all_blocks;			/*	Number of all data blocks.						*/
	UINT8	num_nontrack_blocks;	/*	Number of lead-in data blocks					*/
	UINT16	first_track;			/*	Total number of sessions in image?				*/
	UINT16	last_track;				/*	Number of regular track data blocks.			*/
	UINT32	__dummy2__;				/*	(unknown)										*/
	UINT32	tracks_blocks_offset;	/*	Offset of lead-in+regular track data blocks.	*/
} __attribute__ ((packed)) MDS_SessionBlock;					/*	length: 24 bytes */

typedef struct {
	UINT8	mode;					/*	Track mode								*/
	UINT8	subchannel;				/*	Subchannel mode							*/
	UINT8	adr_ctl;				/*	Adr/Ctl									*/
	UINT8	__dummy2__;				/*	Track flags?							*/
	UINT8	point;					/*	Track number. (>0x99 is lead-in track)	*/

	UINT32	__dummy3__;
	UINT8	min;					/*	Min											*/
	UINT8	sec;					/*	Sec											*/
	UINT8	frame;					/*	Frame										*/
	UINT32	extra_offset;			/*	Start offset of this track's extra block.	*/
	UINT16	sector_size;			/*	Sector size.								*/

	UINT8	__dummy4__[18];
	UINT32	start_sector;			/*	Track start sector (PLBA).	*/
	UINT64	start_offset;			/*	Track start offset.			*/
	UINT8	session;				/*	Session or index?			*/
	UINT8	__dummy5__[3];
	UINT32	footer_offset;			/*	Start offset of footer.		*/
	UINT8	__dummy6__[24];
} __attribute__ ((packed)) MDS_TrackBlock;					/*	length: 80 bytes	*/

typedef struct {
	UINT32 pregap;					/*	Number of sectors in pregap.	*/
	UINT32 length;					/*	Number of sectors in track.		*/
} __attribute__ ((packed)) MDS_TrackExtraBlock;				/*	length: 8 bytes	*/

typedef struct {
	UINT32	filename_offset;		/*	Start offset of image filename.						*/
	UINT32	widechar_filename;		/*	Seems to be set to 1 if widechar filename is used	*/
	UINT32	__dummy1__;
	UINT32	__dummy2__;
} __attribute__ ((packed)) MDS_Footer;						/*	length: 16 bytes	*/
#else
#pragma pack(push, 1)
typedef struct {
	UINT8	signature[16];			/*	0x0000	"MEDIA DESCRIPTOR"				*/
	UINT8	version[2];				/*	0x0010	Version ?						*/
	UINT16	medium_type;			/*	0x0012	Medium type						*/
	UINT16	num_sessions;			/*	0x0014	Number of sessions				*/
	UINT16	__dummy1__[2];			/*	0x0016	Wish I knew...					*/
	UINT16	bca_len;				/*	0x001A	Length of BCA data (DVD-ROM)	*/
	UINT32	__dummy2__[2];			/*	0x001C									*/
	UINT32	bca_data_offset;		/*	0x0026	Offset to BCA data (DVD-ROM)	*/
	UINT32	__dummy3__[6];			/*	0x002A	Probably more offsets			*/
	UINT32	disc_structures_offset;	/*	0x0042	Offset to disc structures		*/
	UINT32	__dummy4__[3];			/*	0x0046	Probably more offsets			*/
	UINT32	sessions_blocks_offset;	/*	0x0052	Offset to session blocks		*/
	UINT32	dpm_blocks_offset;		/*	0x0056	offset to DPM data blocks		*/
} MDS_Header;						/*	length: 88 bytes	*/

typedef struct {
	INT32	session_start;			/*	Session's start address							*/
	INT32	session_end;			/*	Session's end address							*/
	UINT16	session_number;			/*	(Unknown)										*/
	UINT8	num_all_blocks;			/*	Number of all data blocks.						*/
	UINT8	num_nontrack_blocks;	/*	Number of lead-in data blocks					*/
	UINT16	first_track;			/*	Total number of sessions in image?				*/
	UINT16	last_track;				/*	Number of regular track data blocks.			*/
	UINT32	__dummy2__;				/*	(unknown)										*/
	UINT32	tracks_blocks_offset;	/*	Offset of lead-in+regular track data blocks.	*/
} MDS_SessionBlock;					/*	length: 24 bytes */

typedef struct {
	UINT8	mode;					/*	Track mode								*/
	UINT8	subchannel;				/*	Subchannel mode							*/
	UINT8	adr_ctl;				/*	Adr/Ctl									*/
	UINT8	__dummy2__;				/*	Track flags?							*/
	UINT8	point;					/*	Track number. (>0x99 is lead-in track)	*/

	UINT32	__dummy3__;
	UINT8	min;					/*	Min											*/
	UINT8	sec;					/*	Sec											*/
	UINT8	frame;					/*	Frame										*/
	UINT32	extra_offset;			/*	Start offset of this track's extra block.	*/
	UINT16	sector_size;			/*	Sector size.								*/

	UINT8	__dummy4__[18];
	UINT32	start_sector;			/*	Track start sector (PLBA).	*/
	UINT64	start_offset;			/*	Track start offset.			*/
	UINT8	session;				/*	Session or index?			*/
	UINT8	__dummy5__[3];
	UINT32	footer_offset;			/*	Start offset of footer.		*/
	UINT8	__dummy6__[24];
} MDS_TrackBlock;					/*	length: 80 bytes	*/

typedef struct {
	UINT32 pregap;					/*	Number of sectors in pregap.	*/
	UINT32 length;					/*	Number of sectors in track.		*/
} MDS_TrackExtraBlock;				/*	length: 8 bytes	*/

typedef struct {
	UINT32	filename_offset;		/*	Start offset of image filename.						*/
	UINT32	widechar_filename;		/*	Seems to be set to 1 if widechar filename is used	*/
	UINT32	__dummy1__;
	UINT32	__dummy2__;
} MDS_Footer;						/*	length: 16 bytes	*/
#pragma pack(pop)
#endif

//	MDS読み込み
BRESULT openmds(SXSIDEV sxsi, const OEMCHAR *fname) {

	_CDTRK	trk[99];
	OEMCHAR	path[MAX_PATH];
	UINT	index;
	FILEH	fh;
	MDS_Header			MDS_H;
	MDS_SessionBlock	MDS_SB;
	MDS_TrackBlock		MDS_TB;
	MDS_TrackExtraBlock	MDS_TEB;
//	MDS_Footer			MDS_F;
	long	fpos;
	UINT	i;
	UINT32	ex_offset[99];
	UINT32	total_pregap;

	ZeroMemory(trk, sizeof(trk));
	path[0] = '\0';
	index = 0;

	//	イメージファイルの実体は手抜きで単一"*.mdf"固定
	file_cpyname(path, fname, NELEMENTS(path));
	file_cutext(path);
	file_catname(path, str_mdf, NELEMENTS(path));

	fh = file_open_rb(fname);
	if (fh == FILEH_INVALID) {
		goto openmds_err2;
	}

	//	Header読み込み
	if (file_read(fh, &MDS_H, sizeof(MDS_H)) != sizeof(MDS_H)) {
		goto openmds_err1;
	}
	if (memcmp(MDS_H.signature, mds_sig, 16)) {
		goto openmds_err1;
    }

	//	SessionBlockへシーク後、SessionBlock読み込み
	fpos = LOADINTELDWORD(&MDS_H.sessions_blocks_offset);
	if (file_seek(fh, fpos, FSEEK_SET) != fpos) {
		goto openmds_err1;
	}
	if (file_read(fh, &MDS_SB, sizeof(MDS_SB)) != sizeof(MDS_SB)) {
		goto openmds_err1;
	}

	//	そのすぐ後にTrackBlockが続いている前提で必要数分TrackBlock読み込み
	//	※MDS_SB.tracks_blocks_offsetに期待した値が入ってない…
	for (i = 0; i < MDS_SB.num_all_blocks; i++) {
		if (file_read(fh, &MDS_TB, sizeof(MDS_TB)) != sizeof(MDS_TB)) {
			goto openmds_err1;
		}

		//	MDS_TRACKMODE_AUDIO
		//	MDS_TRACKMODE_MODE1
		//	のみ、認識対象
		if (MDS_TB.mode == MDS_TRACKMODE_AUDIO || MDS_TB.mode == MDS_TRACKMODE_MODE1) {
			trk[index].adr_ctl		= MDS_TB.adr_ctl;
			trk[index].point		= MDS_TB.point;
			trk[index].pos			= ((MDS_TB.min * 60) + MDS_TB.sec) * 75 + MDS_TB.frame;
			trk[index].pos0			= 0;

			trk[index].sector_size	= LOADINTELWORD(&MDS_TB.sector_size);

			trk[index].start_sector	= LOADINTELDWORD(&MDS_TB.start_sector);
			trk[index].start_offset	= LOADINTELQWORD(&MDS_TB.start_offset);

			ex_offset[index] = LOADINTELDWORD(&MDS_TB.extra_offset);

			index++;
		}
	}

	//	TrackExtraBlockを読み込んでPREGAP分の補正
	total_pregap = 0;
	for (i = 0; i < index; i++) {
		if (ex_offset[i] != 0) {
			fpos = ex_offset[i];
			if (file_seek(fh, fpos, FSEEK_SET) != fpos) {
				goto openmds_err1;
			}
			if (file_read(fh, &MDS_TEB, sizeof(MDS_TEB)) != sizeof(MDS_TEB)) {
				goto openmds_err1;
			}
			total_pregap += LOADINTELDWORD(&MDS_TEB.pregap);
			trk[i].pos -= total_pregap;

			trk[i].pregap_sectors	= LOADINTELDWORD(&MDS_TEB.pregap);
			trk[i].track_sectors	= LOADINTELDWORD(&MDS_TEB.length);

			trk[i].pregap_sector	= trk[i].start_sector - trk[i].pregap_sectors;
			trk[i].end_sector		= trk[i].start_sector + trk[i].track_sectors - 1;

			trk[i].pregap_offset	= trk[i].start_offset;
			trk[i].end_offset		= trk[i].start_offset + (trk[i].track_sectors * trk[i].sector_size);
		}
	}

	if (index == 0) {
		goto openmds_err1;
	}

	set_secread(sxsi, trk, index);
	sxsi->totals = -1;

	file_close(fh);

	return(setsxsidev(sxsi, path, trk, index));

openmds_err1:
	file_close(fh);

openmds_err2:
	return(FAILURE);
}

#endif
