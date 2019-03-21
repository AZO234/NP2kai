#ifndef HDD_VPCVHD_H
#define HDD_VPCVHD_H

#if defined(SUPPORT_LARGE_HDD)
typedef FILEPOS VPCVHD_FPOS;			// prepare for 64bits length
#else
typedef long VPCVHD_FPOS;
#endif

enum {
	VPCVHD_DISK_NONE = 0,
	VPCVHD_DISK_FIXED = 2,
	VPCVHD_DISK_DYNAMIC = 3,
	VPCVHD_DISK_DIFFERENCING = 4
};

typedef struct VPCVHDFOOTER {
	UINT8 Cookie[8];					// 'conectix'
	UINT8 Features[4];
	UINT8 FileFormatVersion[4];
	UINT8 DataOffset[8];
	UINT8 TimeStamp[4];
	UINT8 CreatorApplication[4];
	UINT8 CreatorVersion[4];
	UINT8 CreatorHostOS[4];
	UINT8 OriginalSize[8];
	UINT8 CurrentSize[8];
	UINT8 Cylinder[2];
	UINT8 Heads;
	UINT8 SectorsPerCylinder;
	UINT8 DiskType[4];
	UINT8 CheckSum[4];
	UINT8 UniqueID[16];
	UINT8 SavedState[1];
	UINT8 ReservedUpto512[427];			// note: up to 511bytes on some images
} VPCVHDFOOTER;

typedef struct VPCVHDDDH {
	UINT8 Cookie[8];					// 'cxsparse'
	UINT8 DataOffset[8];
	UINT8 TableOffset[8];
	UINT8 HeaderVersion[4];
	UINT8 MaxTableEntries[4];
	UINT8 BlockSize[4];
	UINT8 CheckSum[4];
	UINT8 ParentUniqueID[16];
	UINT8 ParentTimeStamp[4];
	UINT8 Reserved0[4];
	UINT8 ParentUnicodeName[512];		// UTF16BE
	UINT8 ParentLocatorEntries[24 * 8];
	UINT8 ReservedUpto1024[256];
} VPCVHDDDH;


#ifdef __cplusplus
extern "C" {
#endif

UINT32 vpc_calc_checksum(UINT8* buf, size_t size);
BRESULT sxsihdd_vpcvhd_mount(SXSIDEV sxsi, FILEH fh);


#ifdef __cplusplus
}
#endif


#endif