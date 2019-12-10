
#define	IDEIO_MULTIPLE_MAX	0
#define	IDEIO_BUFSIZE_MAX	4096

#define	IDEIO_MEDIA_EJECTABLE	(1 << 7)
#define	IDEIO_MEDIA_PREVENT		(1 << 6)
#define	IDEIO_MEDIA_PERSIST		(1 << 5)
//								(1 << 4)
#define	IDEIO_MEDIA_AUDIO		(1 << 3)
#define	IDEIO_MEDIA_DATA		(1 << 2)
#define	IDEIO_MEDIA_CHANGED		(1 << 1)
#define	IDEIO_MEDIA_LOADED		(1 << 0)

#define	IDEIO_MEDIA_COMBINE		(IDEIO_MEDIA_DATA|IDEIO_MEDIA_AUDIO)

typedef struct {
	UINT8	sxsidrv;

	UINT8	wp;
	UINT8	dr;
	UINT8	hd;
	UINT8	sc;
	UINT8	sn;
	UINT16	cy;

	UINT8	cmd;
	UINT8	status;
	UINT8	error;
	UINT8	ctrl;

	UINT8	device;
	UINT8	surfaces;
	UINT8	sectors;
	UINT8	bufdir;
	UINT8	buftc;

	// for ATA multiple mode
	UINT8	mulcnt;
	UINT8	multhr;
	UINT8	mulmode;

	// for ATAPI
	UINT8	media;
	UINT8	sk;		// sense key
	UINT16	asc;	// additional sense code (LSB) & qualifer (MSB)

	UINT32	sector;		// アクセスセクタ (LBA)
	UINT32	nsectors;	// 総セクタ数
	UINT16	secsize;	// セクタサイズ
	UINT16	dmy;

	// buffer management
	UINT	bufpos;
	UINT	bufsize;
	UINT8	buf[IDEIO_BUFSIZE_MAX];

	// audio
	UINT	daflag;
	UINT32	dacurpos;
	UINT32	dalength;
	UINT	dabufrem;
	UINT8	dabuf[2352];
	UINT8	davolume;
	UINT8	damsfbcd;
} _IDEDRV, *IDEDRV;

typedef struct {
	_IDEDRV	drv[2];
	UINT	drivesel;
} _IDEDEV, *IDEDEV;

typedef struct {
	UINT8	bank[2];
	UINT8	daplaying;
	UINT8	padding;
	UINT8   bios;
	OEMCHAR biosname[16];
	UINT32  rwait;
	UINT32  wwait;
	UINT32  mwait;
	_IDEDEV	dev[2];
} IDEIO;

enum {
	IDE_IRQ				= 0x09,

	IDETYPE_NONE		= 0,
	IDETYPE_HDD			= 1,
	IDETYPE_CDROM		= 2,

	IDEDIR_NONE			= 0,
	IDEDIR_OUT			= 1,
	IDEDIR_IN			= 2,

	IDETC_TRANSFEREND	= 0,
	IDETC_ATAPIREAD		= 1,

	IDETC_NOBIOS        = 0,
	IDETC_BIOS			= 1
};

// error
//  bit7: Bad Block detected
//  bit6: Data ECC error
//  bit4: ID Not Found
//  bit2: Aborted command
//  bit1: Track0 Error
//  bit0: Address Mark Not Found

enum {
	IDEERR_BBK			= 0x80,
	IDEERR_UNC			= 0x40,
	IDEERR_MCNG			= 0x20,
	IDEERR_IDNF			= 0x10,
	IDEERR_MCRQ			= 0x08,
	IDEERR_ABRT			= 0x04,
	IDEERR_TR0			= 0x02,
	IDEERR_AMNF			= 0x01
};

// interrupt reason (sector count)
//  bit7-3: tag
//  bit2: bus release
//  bit1: input/output
//  bit0: command/data
//
enum {
	IDEINTR_REL			= 0x04,
	IDEINTR_IO			= 0x02,	// 0: host->device, 1: device->host
	IDEINTR_CD			= 0x01	// 0: data, 1: command
};

// status
//  bit7: Busy
//  bit6: Drive Ready
//  bit5: Drive Write Fault
//  bit4: Drive Seek Complete
//  bit3: Data Request
//  bit2: Corrected data
//  bit1: Index
//  bit0: Error

enum {
	IDESTAT_BSY			= 0x80,
	IDESTAT_DRDY		= 0x40,
	IDESTAT_DWF			= 0x20,
	IDESTAT_DSC			= 0x10,
	IDESTAT_DRQ			= 0x08,
	IDESTAT_CORR		= 0x04,
	IDESTAT_INDX		= 0x02,
	IDESTAT_ERR			= 0x01,

	// for ATAPI PACKET
	IDESTAT_DMRD		= 0x20,		// DMA Ready
	IDESTAT_SERV		= 0x10,		// Service
	IDESTAT_CHK			= 0x01
};

// device/head
//  bit6: LBA (for read/write sector(s)/multiple)
//  bit4: master/slave device select

enum {
	IDEDEV_LBA			= 0x40,
	IDEDEV_DEV			= 0x10
};

// control
//  bit2: Software Reset
//  bit1: ~Interrupt Enable

enum {
	IDECTRL_SRST		= 0x04,
	IDECTRL_NIEN		= 0x02
};


#ifdef __cplusplus
extern "C" {
#endif

extern	IDEIO	ideio;

void IOOUTCALL ideio_w16(UINT port, REG16 value);
REG16 IOINPCALL ideio_r16(UINT port);
void IOOUTCALL ideio_w32(UINT port, UINT32 value);
UINT32 IOINPCALL ideio_r32(UINT port);

void ideio_initialize(void);
void ideio_deinitialize(void);
void ideio_basereset();
void ideio_reset(const NP2CFG *pConfig);
void ideio_bindCDDA(void);
void ideio_bind(void);
void ideio_notify(REG8 sxsidrv, UINT action);
void ideioint(NEVENTITEM item);
void ideio_mediachange(REG8 sxsidrv);

void ideio_setcursec(FILEPOS pos);

#ifdef __cplusplus
}
#endif

