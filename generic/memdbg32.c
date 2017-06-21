#include	"compiler.h"

#if defined(CPUCORE_IA32) && defined(SUPPORT_MEMDBG32)

#include	"strres.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"memdbg32.h"


typedef struct {
	UINT	mode;
	int		width;
	int		height;
	int		bpp;
	CMNPAL	pal[MEMDBG32_PALS];
} MEMDBG32;

static	MEMDBG32	memdbg32;

static const char _mode0[] = "Real Mode";
static const char _mode1[] = "Protected Mode";
static const char _mode2[] = "Virtual86";
static const char *modestr[3] = {_mode0, _mode1, _mode2};
static const RGB32 md32pal[MEMDBG32_PALS] = {
			RGB32D(0x33, 0x33, 0x33),
			RGB32D(0x00, 0x00, 0x00),
			RGB32D(0xff, 0xaa, 0x00),
			RGB32D(0xff, 0x00, 0x00),
			RGB32D(0x11, 0x88, 0x11),
			RGB32D(0x00, 0xff, 0x00),
			RGB32D(0xff, 0xff, 0xff)};


void memdbg32_initialize(void) {

	ZeroMemory(&memdbg32, sizeof(memdbg32));
	memdbg32.width = (MEMDBG32_BLOCKW * 128) + 8;
	memdbg32.height = (MEMDBG32_BLOCKH * 2 * 16) + 8;
}

void memdbg32_getsize(int *width, int *height) {

	if (width) {
		*width = memdbg32.width;
	}
	if (height) {
		*height = memdbg32.height;
	}
}

REG8 memdbg32_process(void) {

	return(MEMDBG32_FLAGDRAW);
}

BOOL memdbg32_paint(CMNVRAM *vram, CMNPALCNV cnv, BOOL redraw) {

	UINT	mode;
	UINT8	use[16*256];
	UINT32	pd[1024];
	UINT	pdmax;
	UINT	i, j;
	UINT32	pde;
	UINT32	pdea;
	UINT32	pte;
	char	str[4];

	mode = 0;
	if (CPU_STAT_PM) {
		mode = 1;
	}
	if (CPU_STAT_VM86) {
		mode = 2;
	}
	if (memdbg32.mode != mode) {
		memdbg32.mode = mode;
		redraw = TRUE;
	}
	if ((!redraw) && (!CPU_STAT_PAGING)) {
		return(FALSE);
	}
	if (vram == NULL) {
		return(FALSE);
	}
	if ((memdbg32.bpp != vram->bpp) || (redraw)) {
		if (cnv == NULL) {
			return(FALSE);
		}
		(*cnv)(memdbg32.pal, md32pal, MEMDBG32_PALS, vram->bpp);
		memdbg32.bpp = vram->bpp;
	}

	cmndraw_fill(vram, 0, 0, memdbg32.width, memdbg32.height,
											memdbg32.pal[MEMDBG32_PALBDR]);
	ZeroMemory(use, sizeof(use));
	if (CPU_STAT_PAGING) {
		pdmax = 0;
		for (i=0; i<1024; i++) {
			pde = cpu_memoryread_d(CPU_STAT_PDE_BASE + (i * 4));
			if (pde & CPU_PDE_PRESENT) {
				for (j=0; j<pdmax; j++) {
					if (!((pde ^ pd[j]) & CPU_PDE_BASEADDR_MASK)) {
						break;
					}
				}
				if (j < pdmax) {
					pd[j] |= pde & CPU_PDE_ACCESS;
				}
				else {
					pd[pdmax++] = pde;
				}
			}
		}
		for (i=0; i<pdmax; i++) {
			pde = pd[i];
			pdea = pde & CPU_PDE_BASEADDR_MASK;
			for (j=0; j<1024; j++) {
				pte = cpu_memoryread_d(pdea + (j * 4));
				if ((pte & CPU_PTE_PRESENT) && (pte < 0x1000000)) {
					if ((pde & CPU_PDE_ACCESS) && (pte & CPU_PTE_ACCESS)) {
						use[pte >> 12] = MEMDBG32_PALPAGE1;
					}
					else if (!use[pte >> 12]) {
						use[pte >> 12] = MEMDBG32_PALPAGE0;
					}
				}
			}
		}
	}
	else {
		FillMemory(use, 256, MEMDBG32_PALREAL);
		FillMemory(use + (0xfa0000 >> 12), (0x60000 >> 12), MEMDBG32_PALREAL);
		if ((CPU_STAT_PM) && (pccore.extmem)) {
			FillMemory(use + 256, 256 * pccore.extmem, MEMDBG32_PALPM);
		}
	}
	for (i=0; i<32; i++) {
		for (j=0; j<128; j++) {
			cmndraw_fill(vram, 8 + j * MEMDBG32_BLOCKW, i * MEMDBG32_BLOCKH,
									MEMDBG32_BLOCKW - 1, MEMDBG32_BLOCKH - 1,
									memdbg32.pal[use[(i * 128) + j]]);
		}
	}
	for (i=0; i<16; i++) {
		SPRINTF(str, "%x", i);
		cmddraw_text8(vram, 0, i * MEMDBG32_BLOCKH * 2, str,
											memdbg32.pal[MEMDBG32_PALTXT]);
	}
	cmddraw_text8(vram, 0, memdbg32.height - 8, modestr[mode],
											memdbg32.pal[MEMDBG32_PALTXT]);
	return(TRUE);
}
#endif

