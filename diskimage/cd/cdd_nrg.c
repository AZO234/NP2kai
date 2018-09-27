#include	"compiler.h"
#include	"dosio.h"
#include	"textfile.h"
#include	"fdd/sxsi.h"

#ifdef SUPPORT_KAI_IMAGES

#include	"diskimage/cddfile.h"
#include	"diskimage/win9x/img_dosio.h"

static const UINT8 nrg_sig_new[4] = {'N','E','R','5'};
static const UINT8 nrg_sig_old[4] = {'N','E','R','O'};

//	手抜きで拙いBE->LE変換マクロ
#define	UINT8_FROM_BE(val)	\
	( val <<  4 ) | \
	( val >>  4 )
#define	UINT16_FROM_BE(val)	\
	( val <<  8 ) | \
	( val >>  8 & 0x00ff )
#define	UINT32_FROM_BE(val)	\
	( val << 24 ) | \
	( val <<  8 & 0x00ff0000 ) | \
	( val >>  8 & 0x0000ff00 ) | \
	( val >> 24 & 0x000000ff )
#define	UINT64_FROM_BE(val)	\
	( val << 56) | \
	( val << 40 & 0x00ff000000000000 ) | \
	( val << 24 & 0x0000ff0000000000 ) | \
	( val <<  8 & 0x000000ff00000000 ) | \
	( val >>  8 & 0x00000000ff000000 ) | \
	( val >> 24 & 0x0000000000ff0000 ) | \
	( val >> 40 & 0x000000000000ff00 ) | \
	( val >> 56 & 0x00000000000000ff )

#if defined(__GNUC__)
typedef struct {
	UINT8	adr_ctl;
	UINT8	track;
	UINT8	index;
	UINT8	__dummy1__;
	UINT32	start_sector;
} __attribute__ ((packed)) NRG_CUE_Block;

typedef struct {
    UINT32	__dummy1__;
    char	mcn[13];
    UINT8	__dummy2__;
    UINT8	_session_type;	/*	?	*/
    UINT8	_num_sessions;	/*	?	*/
    UINT8	first_track;
    UINT8	last_track;
} __attribute__ ((packed)) NRG_DAO_Header;	/*	length: 22 bytes	*/

//	バージョンによって構造体内のメンバのサイズがちげえ
typedef struct {
    char	isrc[12];
    UINT16	sector_size;
    UINT8	mode_code;
    UINT8	__dummy1__;
    UINT16	__dummy2__;
    /*	The following fields are 32-bit in old format and 64-bit in new format	*/
    UINT64	pregap_offset;	/*	Pregap offset in file		*/
    UINT64	start_offset;	/*	Track start offset in file	*/
    UINT64	end_offset;		/*	Track end offset			*/
} __attribute__ ((packed)) NRG_DAO_Block64;

typedef struct {
    char	isrc[12];
    UINT16	sector_size;
    UINT8	mode_code;
    UINT8	__dummy1__;
    UINT16	__dummy2__;
    /*	The following fields are 32-bit in old format and 64-bit in new format	*/
    UINT32	pregap_offset;	/*	Pregap offset in file		*/
    UINT32	start_offset;	/*	Track start offset in file	*/
    UINT32	end_offset;		/*	Track end offset			*/
} __attribute__ ((packed)) NRG_DAO_Block32;

typedef struct {
	char	*block_id;
	INT32	subblock_offset;
	INT32	subblock_length;
} __attribute__ ((packed)) NRG_BlockIDs;
#else
#pragma pack(push, 1)
typedef struct {
	UINT8	adr_ctl;
	UINT8	track;
	UINT8	index;
	UINT8	__dummy1__;
	UINT32	start_sector;
} NRG_CUE_Block;

typedef struct {
    UINT32	__dummy1__;
    char	mcn[13];
    UINT8	__dummy2__;
    UINT8	_session_type;	/*	?	*/
    UINT8	_num_sessions;	/*	?	*/
    UINT8	first_track;
    UINT8	last_track;
} NRG_DAO_Header;	/*	length: 22 bytes	*/

//	バージョンによって構造体内のメンバのサイズがちげえ
typedef struct {
    char	isrc[12];
    UINT16	sector_size;
    UINT8	mode_code;
    UINT8	__dummy1__;
    UINT16	__dummy2__;
    /*	The following fields are 32-bit in old format and 64-bit in new format	*/
    UINT64	pregap_offset;	/*	Pregap offset in file		*/
    UINT64	start_offset;	/*	Track start offset in file	*/
    UINT64	end_offset;		/*	Track end offset			*/
} NRG_DAO_Block64;

typedef struct {
    char	isrc[12];
    UINT16	sector_size;
    UINT8	mode_code;
    UINT8	__dummy1__;
    UINT16	__dummy2__;
    /*	The following fields are 32-bit in old format and 64-bit in new format	*/
    UINT32	pregap_offset;	/*	Pregap offset in file		*/
    UINT32	start_offset;	/*	Track start offset in file	*/
    UINT32	end_offset;		/*	Track end offset			*/
} NRG_DAO_Block32;

typedef struct {
	char	*block_id;
	INT32	subblock_offset;
	INT32	subblock_length;
} NRG_BlockIDs;
#pragma pack(pop)
#endif

static NRG_BlockIDs NRGBlockID[] = {
    { "CUEX", 0,  8  },
    { "CUES", 0,  8  },
    { "ETN2", 0,  32 },
    { "ETNF", 0,  20 },
    { "DAOX", 22, 42 },
    { "DAOI", 22, 30 },
    { "CDTX", 0,  0  },
    { "SINF", 0,  0  },
    { "MTYP", 0,  0  },
    { "END!", 0,  0  },
    { NULL,   0,  0  }
};


//	NRG読み込み
//	※手抜き実装なのでCUExブロックとDAOxブロックの間(？)にETNxブロックが
//	　存在しているイメージは読み込み失敗する
BRESULT opennrg(SXSIDEV sxsi, const OEMCHAR *fname) {

	_CDTRK	trk[99];
	UINT	index;
	FILEH	fh;
    FILELEN	filesize;
    UINT64	trailer_offset;
	UINT64	data_length;
	UINT8	sig[4];
	UINT8	nrg_ver;
	UINT	i;
	long	total_sec;
#ifdef	TOCLOGOUT
	OEMCHAR		logpath[MAX_PATH];
	OEMCHAR		logbuf[2048];
	TEXTFILEH	tfh;
#endif

	ZeroMemory(trk, sizeof(trk));
	index = 0;
	nrg_ver = 0;

	fh = file_open_rb(fname);
	if (fh == FILEH_INVALID) {
		goto opennrg_err2;
	}

	filesize = file_getsize(fh);
    file_seek(fh, -12, FSEEK_END);			//	末尾から12byte位置へ

	if (file_read(fh, sig, sizeof(sig)) != sizeof(sig)) {
		goto opennrg_err1;
	}

	if (!memcmp(sig, nrg_sig_new, sizeof(sig))) {
		/*	New format, 64-bit offset	*/
		UINT64	tmp_offset = 0;

		if (file_read(fh, &tmp_offset, sizeof(UINT64)) != sizeof(UINT64)) {
			goto opennrg_err1;
		}

		trailer_offset = UINT64_FROM_BE(tmp_offset);
        data_length = (filesize - 12) - trailer_offset;
	}
	else {
        /*	Try old format, with 32-bit offset	*/
	    file_seek(fh, -8, FSEEK_END);		//	末尾から8byte位置へ
		if (file_read(fh, sig, sizeof(sig)) != sizeof(sig)) {
			goto opennrg_err1;
		}

		if (!memcmp(sig, nrg_sig_old, sizeof(sig))) {
			UINT32	tmp_offset = 0;

			if (file_read(fh, &tmp_offset, sizeof(UINT32)) != sizeof(UINT32)) {
				goto opennrg_err1;
			}

			trailer_offset = UINT32_FROM_BE(tmp_offset);
			data_length = (filesize - 8) - trailer_offset;
			nrg_ver = 1;
		}
		else {
			/*	Unknown signature, can't handle the file	*/
			goto opennrg_err1;
		}
	}

    file_seek(fh, (FILEPOS)trailer_offset, FSEEK_SET);

	if (file_read(fh, sig, sizeof(sig)) != sizeof(sig)) {
		goto opennrg_err1;
	}
	//	CUEX、CUESブロック読み込み
	if (!memcmp(sig, NRGBlockID[0].block_id, sizeof(sig)) ||
		!memcmp(sig, NRGBlockID[1].block_id, sizeof(sig))) {

		UINT32	tmp_size;
		UINT32	blk_size;

		if (file_read(fh, &tmp_size, sizeof(UINT32)) != sizeof(UINT32)) {
			goto opennrg_err1;
		}
		blk_size = UINT32_FROM_BE(tmp_size);

		for (i = 0; i < blk_size/sizeof(NRG_CUE_Block); i++) {
			NRG_CUE_Block	NRG_CB;

			if (file_read(fh, &NRG_CB, sizeof(NRG_CUE_Block)) != sizeof(NRG_CUE_Block)) {
				goto opennrg_err1;
			}

			if (NRG_CB.index == 0x00 && NRG_CB.track >= 0x02) {
				//	モノによってトラック番号偽っているCD-ROMがあるっぽい？
//				trk[NRG_CB.track].pos0		= UINT32_FROM_BE(NRG_CB.start_sector);
				trk[index].pos0		= UINT32_FROM_BE(NRG_CB.start_sector);
			}
			if (NRG_CB.index == 0x01 && NRG_CB.track != 0xAA) {
				trk[index].adr_ctl	= UINT8_FROM_BE(NRG_CB.adr_ctl);
				//	モノによってトラック番号偽っているCD-ROMがあるっぽい？
//				trk[index].point	= NRG_CB.track;
				trk[index].point	= index+1;
				trk[index].pos		= UINT32_FROM_BE(NRG_CB.start_sector);
				index++;
			}
		}
	}
	else {
		//	最初がCUExブロック以外なら未対応
		goto opennrg_err1;
	}

	if (file_read(fh, sig, sizeof(sig)) != sizeof(sig)) {
		goto opennrg_err1;
	}
	//	DAOX、DAOIブロック読み込み
	if (!memcmp(sig, NRGBlockID[4].block_id, sizeof(sig)) ||
		!memcmp(sig, NRGBlockID[5].block_id, sizeof(sig))) {

		UINT32	tmp_size;
		UINT32	blk_size;
		NRG_DAO_Header	NRG_DH;

		if (file_read(fh, &tmp_size, sizeof(UINT32)) != sizeof(UINT32)) {
			goto opennrg_err1;
		}
		blk_size = UINT32_FROM_BE(tmp_size);

		if (file_read(fh, &NRG_DH, sizeof(NRG_DAO_Header)) != sizeof(NRG_DAO_Header)) {
			goto opennrg_err1;
		}

		blk_size -= sizeof(NRG_DAO_Header);
		if (nrg_ver == 0) {
			for (i = 0; i < blk_size/sizeof(NRG_DAO_Block64); i++) {
				NRG_DAO_Block64	NRG_DB;

				if (file_read(fh, &NRG_DB, sizeof(NRG_DAO_Block64)) != sizeof(NRG_DAO_Block64)) {
					goto opennrg_err1;
				}

				trk[i].sector_size		= UINT16_FROM_BE(NRG_DB.sector_size);
				trk[i].pregap_offset	= UINT64_FROM_BE(NRG_DB.pregap_offset);
				trk[i].start_offset		= UINT64_FROM_BE(NRG_DB.start_offset);
				trk[i].end_offset		= UINT64_FROM_BE(NRG_DB.end_offset);
			}
		}
		else {
			for (i = 0; i < blk_size/sizeof(NRG_DAO_Block32); i++) {
				NRG_DAO_Block32	NRG_DB;

				if (file_read(fh, &NRG_DB, sizeof(NRG_DAO_Block32)) != sizeof(NRG_DAO_Block32)) {
					goto opennrg_err1;
				}

				trk[i].sector_size		= UINT16_FROM_BE(NRG_DB.sector_size);
				trk[i].pregap_offset	= UINT32_FROM_BE(NRG_DB.pregap_offset);
				trk[i].start_offset		= UINT32_FROM_BE(NRG_DB.start_offset);
				trk[i].end_offset		= UINT32_FROM_BE(NRG_DB.end_offset);
			}
		}
	}
	else {
		//	２番目ががDAOxブロック以外なら未対応
		goto opennrg_err1;
	}

	if (index == 0) {
		goto opennrg_err1;
	}

	set_secread(sxsi, trk, index);

	total_sec = set_trkinfo(fh, trk, index, (FILELEN)trailer_offset);
	if (total_sec < 0) {
		goto opennrg_err1;
	}

	//	リードアウト？ポストギャップ？分150セクタ引く
	sxsi->totals = total_sec - 150;

	file_close(fh);

	return(setsxsidev(sxsi, fname, trk, index));

opennrg_err1:
	file_close(fh);

opennrg_err2:
	return(FAILURE);
}

#endif
