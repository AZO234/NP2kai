#include	<compiler.h>
#include	<scrnmng.h>
#include	<vram/scrndraw.h>
#include	"sdraw.h"
#include	<vram/palettes.h>
#if defined(SUPPORT_VIDEOFILTER)
#include	<vram/videofilter.h>
#endif

#if defined(SUPPORT_VIDEOFILTER)
BOOL	bVFEnable;
BOOL	bVFImport;
#endif

#if !defined(NP2_SIZE_QVGA) || defined(SIZE_VGATEST)

#if defined(SUPPORT_VIDEOFILTER)
/* Fast inline VF pixel write per BPP. Reads pre-filtered RGB from sdraw->vfDest
 * directly instead of paying VideoFilter_PutDest()'s per-call overhead.
 * 8BPP variant intentionally a no-op: original PutDest had no case for BPP=1. */
#define VFPUTPIXEL_8(ptr, x, y)   ((void)0)
#define VFPUTPIXEL_16(ptr, x, y)  do { \
		uint32_t _vp = sdraw->vfDest[(y) * sdraw->vfW + (x)]; \
		*(UINT16 *)(ptr) = (UINT16)((((_vp) & 0xF8) >> 3) | (((_vp) & 0xFC00) >> 5) | (((_vp) & 0xF80000) >> 8)); \
	} while(0)
#define VFPUTPIXEL_24(ptr, x, y)  do { \
		uint32_t _vp = sdraw->vfDest[(y) * sdraw->vfW + (x)]; \
		(ptr)[RGB24_R] = (UINT8)(((_vp) >> 16) & 0xFF); \
		(ptr)[RGB24_G] = (UINT8)(((_vp) >> 8) & 0xFF); \
		(ptr)[RGB24_B] = (UINT8)((_vp) & 0xFF); \
	} while(0)
#define VFPUTPIXEL_32(ptr, x, y)  (*(UINT32 *)(ptr) = sdraw->vfDest[(y) * sdraw->vfW + (x)])
#else
#define VFPUTPIXEL_8(ptr, x, y)   ((void)0)
#define VFPUTPIXEL_16(ptr, x, y)  ((void)0)
#define VFPUTPIXEL_24(ptr, x, y)  ((void)0)
#define VFPUTPIXEL_32(ptr, x, y)  ((void)0)
#endif

#if defined(SUPPORT_8BPP)
#define	SDSYM(sym)				sdraw8##sym
#define	SDSETPIXEL(ptr, pal)	*(ptr) = (pal) + START_PAL
#define	VFPUTPIXEL(ptr, x, y)	VFPUTPIXEL_8(ptr, x, y)
#include	"sdraw.mcr"
#undef	SDSYM
#undef	SDSETPIXEL
#undef	VFPUTPIXEL
#endif

#if defined(SUPPORT_16BPP)
#define	SDSYM(sym)				sdraw16##sym
#define	SDSETPIXEL(ptr, pal)	*(UINT16 *)(ptr) = np2_pal16[(pal)]
#define	VFPUTPIXEL(ptr, x, y)	VFPUTPIXEL_16(ptr, x, y)
#include	"sdraw.mcr"
#include	"sdrawex.mcr"
#undef	SDSYM
#undef	SDSETPIXEL
#undef	VFPUTPIXEL
#endif

#if defined(SUPPORT_24BPP)
#define	SDSYM(sym)				sdraw24##sym
#define	SDSETPIXEL(ptr, pal)	(ptr)[RGB24_R] = np2_pal32[(pal)].p.r;	\
								(ptr)[RGB24_G] = np2_pal32[(pal)].p.g;	\
								(ptr)[RGB24_B] = np2_pal32[(pal)].p.b
#define	VFPUTPIXEL(ptr, x, y)	VFPUTPIXEL_24(ptr, x, y)
#include	"sdraw.mcr"
#include	"sdrawex.mcr"
#undef	SDSYM
#undef	SDSETPIXEL
#undef	VFPUTPIXEL
#endif

#if defined(SUPPORT_32BPP)
#define	SDSYM(sym)				sdraw32##sym
#define	SDSETPIXEL(ptr, pal)	*(UINT32 *)(ptr) = np2_pal32[(pal)].d
#define	VFPUTPIXEL(ptr, x, y)	VFPUTPIXEL_32(ptr, x, y)
#include	"sdraw.mcr"
#include	"sdrawex.mcr"
#undef	SDSYM
#undef	SDSETPIXEL
#undef	VFPUTPIXEL
#endif


// ----

static const SDRAWFN *tbl[] = {
#if defined(SUPPORT_8BPP)
			sdraw8p,
#else
			NULL,
#endif
#if defined(SUPPORT_16BPP)
			sdraw16p,
#else
			NULL,
#endif
#if defined(SUPPORT_24BPP)
			sdraw24p,
#else
			NULL,
#endif
#if defined(SUPPORT_32BPP)
			sdraw32p,
#else
			NULL,
#endif

#if defined(SUPPORT_NORMALDISP)
#if defined(SUPPORT_8BPP)
			sdraw8n,
#else
			NULL,
#endif
#if defined(SUPPORT_16BPP)
			sdraw16n,
#else
			NULL,
#endif
#if defined(SUPPORT_24BPP)
			sdraw24n,
#else
			NULL,
#endif
#if defined(SUPPORT_32BPP)
			sdraw32n,
#else
			NULL,
#endif
#endif
};

const SDRAWFN *sdraw_getproctbl(const SCRNSURF *surf) {

	int		proc;

	proc = ((surf->bpp >> 3) - 1) & 3;
#if defined(SUPPORT_NORMALDISP)
	if (surf->extend) {
		proc += 4;
	}
#endif
	return(tbl[proc]);
}


// ---- PC-9821

#if defined(SUPPORT_PC9821)

static const SDRAWFN *tblex[] = {
			NULL,
#if defined(SUPPORT_16BPP)
			sdraw16pex,
#else
			NULL,
#endif
#if defined(SUPPORT_24BPP)
			sdraw24pex,
#else
			NULL,
#endif
#if defined(SUPPORT_32BPP)
			sdraw32pex,
#else
			NULL,
#endif

#if defined(SUPPORT_NORMALDISP)
			NULL,
#if defined(SUPPORT_16BPP)
			sdraw16nex,
#else
			NULL,
#endif
#if defined(SUPPORT_24BPP)
			sdraw24nex,
#else
			NULL,
#endif
#if defined(SUPPORT_32BPP)
			sdraw32nex,
#else
			NULL,
#endif
#endif
};

const SDRAWFN *sdraw_getproctblex(const SCRNSURF *surf) {

	int		proc;

	proc = ((surf->bpp >> 3) - 1) & 3;
#if defined(SUPPORT_NORMALDISP)
	if (surf->extend) {
		proc += 4;
	}
#endif
	return(tblex[proc]);
}
#endif

#endif

