#include	"compiler.h"
#include	"cpucore.h"
#include	"lio.h"


typedef struct {
	UINT8	x[2];
	UINT8	y[2];
	UINT8	pal;
} GPSET;

typedef struct {
	UINT8	x[2];
	UINT8	y[2];
} GPOINT2;


// ---- PSET

REG8 lio_gpset(GLIO lio) {

	GPSET	dat;
	SINT16	x;
	SINT16	y;

	lio_updatedraw(lio);
	MEMR_READS(CPU_DS, CPU_BX, &dat, sizeof(dat));
	x = (SINT16)LOADINTELWORD(dat.x);
	y = (SINT16)LOADINTELWORD(dat.y);
	if (dat.pal == 0xff) {
		if (CPU_AH == 1) {
			dat.pal = lio->work.fgcolor;
		}
		else {
			dat.pal = lio->work.bgcolor;
		}
	}
	lio_pset(lio, x, y, dat.pal);
	return(LIO_SUCCESS);
}


// ---- GPOINT2

REG8 lio_gpoint2(GLIO lio) {

	GPOINT2	dat;
	SINT16	x;
	SINT16	y;
	REG8	ret;
	UINT32	addr;
	UINT	sft;
	UINT	pl;
const UINT8	*ptr;

	lio_updatedraw(lio);
	MEMR_READS(CPU_DS, CPU_BX, &dat, sizeof(dat));
	x = (SINT16)LOADINTELWORD(dat.x);
	y = (SINT16)LOADINTELWORD(dat.y);
	if ((lio->draw.x1 > x) || (lio->draw.x2 < x) ||
		(lio->draw.y1 > y) || (lio->draw.y2 < y)) {
		ret = 0xff;
	}
	else {
		ret = 0;
		addr = (x >> 3) + (y * 80);
		if (lio->draw.flag & LIODRAW_UPPER) {
			addr += 16000;
		}
		addr += lio->draw.base;
		sft = (~x) & 7;
		if (!(lio->draw.flag & LIODRAW_MONO)) {
			for (pl=0; pl<3; pl++) {
				ptr = mem + addr + lioplaneadrs[pl];
				ret += (((*ptr) >> sft) & 1) << pl;
			}
			if (lio->draw.flag & LIODRAW_4BPP) {
				ptr = mem + addr + lioplaneadrs[3];
				ret += (((*ptr) >> sft) & 1) << 3;
			}
		}
		else {
			ptr = mem + addr + lioplaneadrs[lio->draw.flag & LIODRAW_PMASK];
			ret = ((*ptr) >> sft) & 1;
		}
	}
	CPU_AL = ret;
	return(LIO_SUCCESS);
}

