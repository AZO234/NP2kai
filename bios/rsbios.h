// BIOS0C.CPP, BIOS19.CPP

enum {
	R_INT			= 0x00,
	R_BFLG			= 0x01,
	R_FLAG			= 0x02,
	R_CMD			= 0x03,
	R_STIME			= 0x04,
	R_RTIME			= 0x05,
	R_XOFF			= 0x06,
	R_XON			= 0x08,
	R_HEADP			= 0x0a,
	R_TAILP			= 0x0c,
	R_CNT			= 0x0e,
	R_PUTP			= 0x10,
	R_GETP			= 0x12,

	RINT_INT		= 0x80,

	RFLAG_INIT		= 0x80,
	RFLAG_BFULL		= 0x40,
	RFLAG_BOVF		= 0x20,
	RFLAG_XON		= 0x10,
	RFLAG_XOFF		= 0x08,

	RCMD_IR			= 0x40,
	RCMD_RTS		= 0x20,
	RCMD_ER			= 0x10,
	RCMD_SBRK		= 0x08,
	RCMD_RXE		= 0x04,
	RCMD_DTR		= 0x02,
	RCMD_TXEN		= 0x01,

	RSCODE_XON		= 0x11,
	RSCODE_XOFF		= 0x13,
	RSCODE_SO		= 0x0e,
	RSCODE_SI		= 0x0f
};

typedef struct {
	UINT8	INT;				// + 0
	UINT8	BFLG;				// + 1
	UINT8	FLAG;				// + 2
	UINT8	CMD;				// + 3
	UINT8	STIME;				// + 4
	UINT8	RTIME;				// + 5
	UINT8	XOFF[2];			// + 6
	UINT8	XON[2];				// + 8
	UINT8	HEADP[2];			// + a
	UINT8	TAILP[2];			// + c
	UINT8	CNT[2];				// + e
	UINT8	PUTP[2];			// +10
	UINT8	GETP[2];			// +12
} RSBIOS;

