
#ifndef VRAMCALL
#define	VRAMCALL
#endif


typedef struct {
	UINT	operate;
#if !defined(CPUSTRUC_MEMWAIT)
	UINT	tramwait;
	UINT	vramwait;
	UINT	grcgwait;
#endif
#if defined(SUPPORT_PC9821)
	UINT8	mio1[4];
	UINT8	mio2[0x40];
#endif
} _VRAMOP, *VRAMOP;

// operate:		bit0	access page
//				bit1	egc enable
//				bit2	grcg bit6
//				bit3	grcg bit7
//				bit4	analog enable
//				bit5	pc9821 vga

enum {
	VOPBIT_ACCESS	= 0,
	VOPBIT_EGC		= 1,
	VOPBIT_GRCG		= 2,
	VOPBIT_ANALOG	= 4,
	VOPBIT_VGA		= 5
};

//	VOP_ACCESSBIT	= 0x01,
//	VOP_EGCBIT		= 0x02,
//	VOP_GRCGBIT		= 0x0c,
//	VOP_ANALOGBIT	= 0x10,

//	VOP_ACCESSMASK	= ~(0x01),
//	VOP_EGCMASK		= ~(0x02),
//	VOP_GRCGMASK	= ~(0x0c),
//	VOP_ANALOGMASK	= ~(0x10)


#ifdef __cplusplus
extern "C" {
#endif

extern	_VRAMOP	vramop;
extern	UINT8	tramupdate[0x1000];
extern	UINT8	vramupdate[0x8000];
#if defined(SUPPORT_PC9821)
extern	UINT8	vramex[0x80000];
#endif

void vram_initialize(void);

#if !defined(CPUSTRUC_MEMWAIT)
#define	MEMWAIT_TRAM	vramop.tramwait
#define	MEMWAIT_VRAM	vramop.vramwait
#define	MEMWAIT_GRCG	vramop.grcgwait
#endif

#ifdef __cplusplus
}
#endif

