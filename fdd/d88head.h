
enum {
	D88_TRACKMAX		= 164,
	D88_HEADERSIZE		= 0x20 + (D88_TRACKMAX * 4)
};


#if defined(__GNUC__)
// D88ヘッダ (size: 2b0h bytes)
typedef struct {
	UINT8	fd_name[17];		// Disk Name
	UINT8	reserved1[9]; 		// Reserved
	UINT8	protect;			// Write Protect bit:4
	UINT8	fd_type;			// Disk Format
	UINT8	fd_size[4];			// Disk Size
	UINT8	trackp[D88_TRACKMAX][4];
} __attribute__ ((packed)) _D88HEAD, *D88HEAD;

// D88セクタ (size: 16bytes)
typedef struct {
	UINT8	c;
	UINT8	h;
	UINT8	r;
	UINT8	n;
	UINT8	sectors[2];			// Sector Count
	UINT8	mfm_flg;			// sides
	UINT8	del_flg;			// DELETED DATA
	UINT8	stat;				// STATUS (FDC ret)
	UINT8	seektime;			// Seek Time
	UINT8	reserved[3];		// Reserved
	UINT8	rpm_flg;			// rpm			0:1.2  1:1.44
	UINT8	size[2];			// Sector Size
} __attribute__ ((packed)) _D88SEC, *D88SEC;
#else
#pragma pack(push, 1)
// D88ヘッダ (size: 2b0h bytes)
typedef struct {
	UINT8	fd_name[17];		// Disk Name
	UINT8	reserved1[9]; 		// Reserved
	UINT8	protect;			// Write Protect bit:4
	UINT8	fd_type;			// Disk Format
	UINT8	fd_size[4];			// Disk Size
	UINT8	trackp[D88_TRACKMAX][4];
} _D88HEAD, *D88HEAD;

// D88セクタ (size: 16bytes)
typedef struct {
	UINT8	c;
	UINT8	h;
	UINT8	r;
	UINT8	n;
	UINT8	sectors[2];			// Sector Count
	UINT8	mfm_flg;			// sides
	UINT8	del_flg;			// DELETED DATA
	UINT8	stat;				// STATUS (FDC ret)
	UINT8	seektime;			// Seek Time
	UINT8	reserved[3];		// Reserved
	UINT8	rpm_flg;			// rpm			0:1.2  1:1.44
	UINT8	size[2];			// Sector Size
} _D88SEC, *D88SEC;
#pragma pack(pop)
#endif

