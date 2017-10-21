#ifndef NP2_COMMON_H
#define NP2_COMMON_H

enum {
	SUCCESS		= 0,
	FAILURE		= 1
};

#ifndef BRESULT
#define	BRESULT		UINT
#endif

#ifndef INTPTR
#define	INTPTR		intptr_t
#endif
#ifndef UINTPTR
#define	UINTPTR		uintptr_t
#endif

#ifndef LOADINTELDWORD
#define	LOADINTELDWORD(a)		(((UINT32)(((UINT8*)(a))[0])) |				\
								((UINT32)(((UINT8*)(a))[1]) << 8) |			\
								((UINT32)(((UINT8*)(a))[2]) << 16) |		\
								((UINT32)(((UINT8*)(a))[3]) << 24))
#endif

#ifndef LOADINTELWORD
#define	LOADINTELWORD(a)		(((UINT16)((UINT8*)(a))[0]) | ((UINT16)(((UINT8*)(a))[1]) << 8))
#endif

#ifndef STOREINTELDWORD
#define	STOREINTELDWORD(a, b)	*(((UINT8*)(a))+0) = (UINT8)((b));		\
								*(((UINT8*)(a))+1) = (UINT8)((b)>>8);		\
								*(((UINT8*)(a))+2) = (UINT8)((b)>>16);	\
								*(((UINT8*)(a))+3) = (UINT8)((b)>>24)
#endif

#ifndef STOREINTELWORD
#define	STOREINTELWORD(a, b)	*(((UINT8*)(a))+0) = (UINT8)((b));			\
								*(((UINT8*)(a))+1) = (UINT8)((b)>>8)
#endif

/* Big Ending */

#ifndef LOADMOTOROLAQWORD
#define    LOADMOTOROLAQWORD(a)        (((UINT64)((UINT8*)(a))[7]) |             \
                               ((UINT64)(((UINT8*)(a))[6]) << 8) |         \
                               ((UINT64)(((UINT8*)(a))[5]) << 16) |        \
                               ((UINT64)(((UINT8*)(a))[4]) << 24) |        \
                               ((UINT64)(((UINT8*)(a))[3]) << 32) |        \
                               ((UINT64)(((UINT8*)(a))[2]) << 40) |        \
                               ((UINT64)(((UINT8*)(a))[1]) << 48) |        \
                               ((UINT64)(((UINT8*)(a))[0]) << 56))
#endif

#ifndef LOADMOTOROLADWORD
#define    LOADMOTOROLADWORD(a)        (((UINT32)((UINT8*)(a))[3]) |             \
                               ((UINT32)(((UINT8*)(a))[2]) << 8) |         \
                               ((UINT32)(((UINT8*)(a))[1]) << 16) |        \
                               ((UINT32)(((UINT8*)(a))[0]) << 24))
#endif

#ifndef LOADMOTOROLAWORD
#define    LOADMOTOROLAWORD(a)     (((UINT16)((UINT8*)(a))[1]) | ((UINT16)(((UINT8*)(a))[0]) << 8))
#endif

#ifndef STOREMOTOROLAQWORD
#define    STOREMOTOROLAQWORD(a, b)    *(((UINT8*)(a))+7) = (UINT8)((b));        \
                               *(((UINT8*)(a))+6) = (UINT8)((b)>>8);     \
                               *(((UINT8*)(a))+5) = (UINT8)((b)>>16);    \
                               *(((UINT8*)(a))+4) = (UINT8)((b)>>24);    \
                               *(((UINT8*)(a))+3) = (UINT8)((b)>>32);    \
                               *(((UINT8*)(a))+2) = (UINT8)((b)>>40);    \
                               *(((UINT8*)(a))+1) = (UINT8)((b)>>48);    \
                               *(((UINT8*)(a))+0) = (UINT8)((b)>>56)
#endif

#ifndef STOREMOTOROLADWORD
#define    STOREMOTOROLADWORD(a, b)    *(((UINT8*)(a))+3) = (UINT8)((b));        \
                               *(((UINT8*)(a))+2) = (UINT8)((b)>>8);     \
                               *(((UINT8*)(a))+1) = (UINT8)((b)>>16);    \
                               *(((UINT8*)(a))+0) = (UINT8)((b)>>24)
#endif

#ifndef STOREMOTOROLAWORD
#define    STOREMOTOROLAWORD(a, b) *(((UINT8*)(a))+1) = (UINT8)((b));            \
                               *(((UINT8*)(a))+0) = (UINT8)((b)>>8)
#endif

/* *** */

#ifndef	NELEMENTS
#define	NELEMENTS(a)	((int)(sizeof(a) / sizeof(a[0])))
#endif


// ---- Optimize Macros

#ifndef REG8
#define	REG8		UINT8
#endif
#ifndef REG16
#define	REG16		UINT16
#endif

#ifndef LOW8
#define	LOW8(a)					((UINT8)(a))
#endif
#ifndef LOW10
#define	LOW10(a)				((a) & 0x03ff)
#endif
#ifndef LOW11
#define	LOW11(a)				((a) & 0x07ff)
#endif
#ifndef LOW12
#define	LOW12(a)				((a) & 0x0fff)
#endif
#ifndef LOW14
#define	LOW14(a)				((a) & 0x3fff)
#endif
#ifndef LOW15
#define	LOW15(a)				((a) & 0x7fff)
#endif
#ifndef LOW16
#define	LOW16(a)				((UINT16)(a))
#endif
#ifndef HIGH16
#define	HIGH16(a)				(((UINT32)(a)) >> 16)
#endif


#ifndef OEMCHAR
#define	OEMCHAR					char
#endif
#ifndef OEMTEXT
#define	OEMTEXT(string)			string
#endif


#if !defined(RGB16)
#define	RGB16		UINT16
#endif

#if !defined(RGB32)
#if defined(BYTESEX_LITTLE)
typedef union {
	UINT32	d;
	struct {
		UINT8	b;
		UINT8	g;
		UINT8	r;
		UINT8	e;
	} p;
} RGB32;
#define	RGB32D(r, g, b)		(((r) << 16) + ((g) << 8) + ((b) << 0))
#elif defined(BYTESEX_BIG)
typedef union {
	UINT32	d;
	struct {
		UINT8	e;
		UINT8	r;
		UINT8	g;
		UINT8	b;
	} p;
} RGB32;
#define	RGB32D(r, g, b)		(((r) << 16) + ((g) << 8) + ((b) << 0))
#endif
#endif


#define	FTYPEID(a, b, c, d)	(((a) << 24) + ((b) << 16) + ((c) << 8) + (d))

enum {
	FTYPE_NONE		= 0,
	FTYPE_SMIL		= FTYPEID('S','M','I','L'),
	FTYPE_TEXT		= FTYPEID('T','E','X','T'),
	FTYPE_BMP		= FTYPEID('B','M','P',' '),
	FTYPE_GIF		= FTYPEID('G','I','F',' '),
	FTYPE_WAVE		= FTYPEID('W','A','V','E'),
	FTYPE_OGG		= FTYPEID('O','G','G',' '),
	FTYPE_MP3		= FTYPEID('M','P','3',' '),
	FTYPE_D88		= FTYPEID('.','D','8','8'),
	FTYPE_FDI		= FTYPEID('.','F','D','I'),
	FTYPE_BETA		= FTYPEID('B','E','T','A'),
	FTYPE_THD		= FTYPEID('.','T','H','D'),
	FTYPE_NHD		= FTYPEID('.','N','H','D'),
	FTYPE_HDI		= FTYPEID('.','H','D','I'),
	FTYPE_HDD		= FTYPEID('.','H','D','D'),
	FTYPE_S98		= FTYPEID('.','S','9','8'),
	FTYPE_MIMPI		= FTYPEID('M','I','M','P')
};


#if !defined(INLINE)
#define	INLINE
#endif
#if !defined(FASTCALL)
#define	FASTCALL
#endif

#endif	/* NP2_COMMON_H */
