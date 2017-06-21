
enum {
	CMD_RESET		= 0x00,
	CMD_SYNC_ON		= 0x0f,
	CMD_SYNC_OFF	= 0x0e,
	CMD_MASTER		= 0x6f,
	CMD_SLAVE		= 0x6e,

	CMD_START_		= 0x6b,
	CMD_START		= 0x0d,
	CMD_STOP_		= 0x05,
	CMD_STOP		= 0x0c,

	CMD_ZOOM		= 0x06,
	CMD_SCROLL		= 0x70,
	CMD_CSRW		= 0x49,
	CMD_CSRFORM		= 0x4b,
	CMD_PITCH		= 0x47,
	CMD_LPEN		= 0xc0,
	CMD_VECTW		= 0x4c,
	CMD_VECTE		= 0x6c,
	CMD_TEXTW		= 0x78,
	CMD_TEXTE		= 0x68,
	CMD_CSRR		= 0xe0,
	CMD_MASK		= 0x4a
};

enum {
	GDC_SYNC		= 0,								//  0
	GDC_ZOOM		= (GDC_SYNC+8),						//  8
	GDC_CSRFORM		= (GDC_ZOOM+1),						//  9
	GDC_SCROLL		= (GDC_CSRFORM+3),					// 12
	GDC_TEXTW		= (GDC_SCROLL+8),					// 20
	GDC_PITCH		= (GDC_TEXTW+8),					// 28
	GDC_LPEN		= (GDC_PITCH+1),					// 29
	GDC_VECTW		= (GDC_LPEN+3),						// 32
	GDC_CSRW		= (GDC_VECTW+11),					// 43
	GDC_MASK		= (GDC_CSRW+3),						// 46
	GDC_CSRR		= (GDC_MASK+2),						// 48
	GDC_WRITE		= (GDC_CSRR+5),						// 53
	GDC_CODE		= (GDC_WRITE+1),					// 54
	GDC_TERMDATA	= (GDC_CODE+2)
};

