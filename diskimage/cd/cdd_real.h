#ifdef SUPPORT_PHYSICAL_CDDRV

#ifdef __cplusplus
extern "C" {
#endif

#if 0
#include	<api/ntddcdrm.h>
#else
	// DDK用意が面倒なので代替
#define IOCTL_CDROM_GET_DRIVE_GEOMETRY  CTL_CODE(FILE_DEVICE_CD_ROM, 0x0013, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_READ_TOC            CTL_CODE(FILE_DEVICE_CD_ROM, 0x0000, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_RAW_READ            CTL_CODE(FILE_DEVICE_CD_ROM, 0x000F, METHOD_OUT_DIRECT,  FILE_READ_ACCESS)

typedef struct _TRACK_DATA {
	UCHAR Reserved;
	UCHAR Control : 4;
	UCHAR Adr : 4;
	UCHAR TrackNumber;
	UCHAR Reserved1;
	UCHAR Address[4];
} TRACK_DATA, * PTRACK_DATA;

typedef struct _CDROM_TOC {
	UCHAR Length[2];
	UCHAR FirstTrack;
	UCHAR LastTrack;
	TRACK_DATA TrackData[100];
} CDROM_TOC, * PCDROM_TOC;

typedef enum _TRACK_MODE_TYPE {
	YellowMode2,
	XAForm2,
	CDDA,
} TRACK_MODE_TYPE, * PTRACK_MODE_TYPE;

typedef struct __RAW_READ_INFO {
	LARGE_INTEGER       DiskOffset;
	ULONG               SectorCount;
	TRACK_MODE_TYPE     TrackMode;
} RAW_READ_INFO, * PRAW_READ_INFO;

#endif

BRESULT openrealcdd(SXSIDEV sxsi, const OEMCHAR *fname);

#ifdef __cplusplus
}
#endif

#endif