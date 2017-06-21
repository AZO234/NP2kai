#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"memvram.h"
#include	"vram.h"


// ---- macros

#define	VRAMRD8(p, a) {												\
	CPU_REMCLOCK -= MEMWAIT_VRAM;									\
	return(mem[(a) + ((p) * VRAM_STEP)]);							\
}

#define VRAMRD16(p, a) {											\
	CPU_REMCLOCK -= MEMWAIT_VRAM;									\
	return(LOADINTELWORD(mem + (a) + ((p) * VRAM_STEP)));			\
}

#define VRAMWR8(p, a, v) {											\
	CPU_REMCLOCK -= MEMWAIT_VRAM;									\
	mem[(a) + ((p) * VRAM_STEP)] = (UINT8)(v);						\
	vramupdate[LOW15(a)] |= (1 << (p));								\
	gdcs.grphdisp |= (1 << (p));									\
}

#define VRAMWR16(p, a, v) {											\
	CPU_REMCLOCK -= MEMWAIT_VRAM;									\
	STOREINTELWORD(mem + (a) + ((p) * VRAM_STEP), (v));				\
	vramupdate[LOW15(a)] |= (1 << (p));								\
	vramupdate[LOW15((a) + 1)] |= (1 << (p));						\
	gdcs.grphdisp |= (1 << (p));									\
}


#define RMWWR8(p, a, v) {											\
	REG8	mask;													\
	UINT8	*vram;													\
	CPU_REMCLOCK -= MEMWAIT_GRCG;									\
	mask = ~value;													\
	(a) = LOW15((a));												\
	vramupdate[(a)] |= (1 << (p));									\
	gdcs.grphdisp |= (1 << (p));									\
	vram = mem + (a) + ((p) * VRAM_STEP);							\
	if (!(grcg.modereg & 1)) {										\
		vram[VRAM_B] &= mask;										\
		vram[VRAM_B] |= ((v) & grcg.tile[0].b[0]);					\
	}																\
	if (!(grcg.modereg & 2)) {										\
		vram[VRAM_R] &= mask;										\
		vram[VRAM_R] |= ((v) & grcg.tile[1].b[0]);					\
	}																\
	if (!(grcg.modereg & 4)) {										\
		vram[VRAM_G] &= mask;										\
		vram[VRAM_G] |= ((v) & grcg.tile[2].b[0]);					\
	}																\
	if (!(grcg.modereg & 8)) {										\
		vram[VRAM_E] &= mask;										\
		vram[VRAM_E] |= ((v) & grcg.tile[3].b[0]);					\
	}																\
}

#define RMWWR16(p, a, v) {											\
	UINT8	*vram;													\
	CPU_REMCLOCK -= MEMWAIT_GRCG;									\
	(a) = LOW15((a));												\
	vramupdate[(a) + 0] |= (1 << (p));								\
	vramupdate[(a) + 1] |= (1 << (p));								\
	gdcs.grphdisp |= (1 << (p));									\
	vram = mem + (a) + ((p) * VRAM_STEP);							\
	if (!(grcg.modereg & 1)) {										\
		UINT8 tmp;													\
		tmp = (UINT8)(v);											\
		vram[VRAM_B + 0] &= (~tmp);									\
		vram[VRAM_B + 0] |= (tmp & grcg.tile[0].b[0]);				\
		tmp = (UINT8)((v) >> 8);									\
		vram[VRAM_B + 1] &= (~tmp);									\
		vram[VRAM_B + 1] |= (tmp & grcg.tile[0].b[0]);				\
	}																\
	if (!(grcg.modereg & 2)) {										\
		UINT8 tmp;													\
		tmp = (UINT8)(v);											\
		vram[VRAM_R + 0] &= (~tmp);									\
		vram[VRAM_R + 0] |= (tmp & grcg.tile[1].b[0]);				\
		tmp = (UINT8)((v) >> 8);									\
		vram[VRAM_R + 1] &= (~tmp);									\
		vram[VRAM_R + 1] |= (tmp & grcg.tile[1].b[0]);				\
	}																\
	if (!(grcg.modereg & 4)) {										\
		UINT8 tmp;													\
		tmp = (UINT8)(v);											\
		vram[VRAM_G + 0] &= (~tmp);									\
		vram[VRAM_G + 0] |= (tmp & grcg.tile[2].b[0]);				\
		tmp = (UINT8)((v) >> 8);									\
		vram[VRAM_G + 1] &= (~tmp);									\
		vram[VRAM_G + 1] |= (tmp & grcg.tile[2].b[0]);				\
	}																\
	if (!(grcg.modereg & 8)) {										\
		UINT8 tmp;													\
		tmp = (UINT8)(v);											\
		vram[VRAM_E + 0] &= (~tmp);									\
		vram[VRAM_E + 0] |= (tmp & grcg.tile[3].b[0]);				\
		tmp = (UINT8)((v) >> 8);									\
		vram[VRAM_E + 1] &= (~tmp);									\
		vram[VRAM_E + 1] |= (tmp & grcg.tile[3].b[0]);				\
	}																\
}


#define TDWWR8(p, a, v) {											\
	UINT8	*vram;													\
	CPU_REMCLOCK -= MEMWAIT_GRCG;									\
	(a) = LOW15(a);													\
	vramupdate[(a)] |= (1 << (p));									\
	gdcs.grphdisp |= (1 << (p));									\
	vram = mem + (a) + ((p) * VRAM_STEP);							\
	if (!(grcg.modereg & 1)) {										\
		vram[VRAM_B] = grcg.tile[0].b[0];							\
	}																\
	if (!(grcg.modereg & 2)) {										\
		vram[VRAM_R] = grcg.tile[1].b[0];							\
	}																\
	if (!(grcg.modereg & 4)) {										\
		vram[VRAM_G] = grcg.tile[2].b[0];							\
	}																\
	if (!(grcg.modereg & 8)) {										\
		vram[VRAM_E] = grcg.tile[3].b[0];							\
	}																\
	(void)(v);														\
}

#define TDWWR16(p, a, v) {											\
	UINT8	*vram;													\
	CPU_REMCLOCK -= MEMWAIT_GRCG;									\
	(a) = LOW15(a);													\
	vramupdate[(a) + 0] |= (1 << (p));								\
	vramupdate[(a) + 1] |= (1 << (p));								\
	gdcs.grphdisp |= (1 << (p));									\
	vram = mem + (a) + ((p) * VRAM_STEP);							\
	if (!(grcg.modereg & 1)) {										\
		vram[VRAM_B + 0] = grcg.tile[0].b[0];						\
		vram[VRAM_B + 1] = grcg.tile[0].b[0];						\
	}																\
	if (!(grcg.modereg & 2)) {										\
		vram[VRAM_R + 0] = grcg.tile[1].b[0];						\
		vram[VRAM_R + 1] = grcg.tile[1].b[0];						\
	}																\
	if (!(grcg.modereg & 4)) {										\
		vram[VRAM_G + 0] = grcg.tile[2].b[0];						\
		vram[VRAM_G + 1] = grcg.tile[2].b[0];						\
	}																\
	if (!(grcg.modereg & 8)) {										\
		vram[VRAM_E + 0] = grcg.tile[3].b[0];						\
		vram[VRAM_E + 1] = grcg.tile[3].b[0];						\
	}																\
	(void)(v);														\
}


#define TCRRD8(p, a) {												\
const UINT8	*vram;													\
	REG8	ret;													\
	CPU_REMCLOCK -= MEMWAIT_GRCG;									\
	vram = mem + LOW15(a) + ((p) * VRAM_STEP);						\
	ret = 0;														\
	if (!(grcg.modereg & 1)) {										\
		ret |= vram[VRAM_B] ^ grcg.tile[0].b[0];					\
	}																\
	if (!(grcg.modereg & 2)) {										\
		ret |= vram[VRAM_R] ^ grcg.tile[1].b[0];					\
	}																\
	if (!(grcg.modereg & 4)) {										\
		ret |= vram[VRAM_G] ^ grcg.tile[2].b[0];					\
	}																\
	if (!(grcg.modereg & 8)) {										\
		ret |= vram[VRAM_E] ^ grcg.tile[3].b[0];					\
	}																\
	return(ret ^ 0xff);												\
}

#define TCRRD16(p, a) {												\
const UINT8	*vram;													\
	REG16	ret;													\
	CPU_REMCLOCK -= MEMWAIT_GRCG;									\
	ret = 0;														\
	vram = mem + LOW15(a) + ((p) * VRAM_STEP);						\
	if (!(grcg.modereg & 1)) {										\
		ret |= LOADINTELWORD(vram + VRAM_B) ^ grcg.tile[0].w;		\
	}																\
	if (!(grcg.modereg & 2)) {										\
		ret |= LOADINTELWORD(vram + VRAM_R) ^ grcg.tile[1].w;		\
	}																\
	if (!(grcg.modereg & 4)) {										\
		ret |= LOADINTELWORD(vram + VRAM_G) ^ grcg.tile[2].w;		\
	}																\
	if (!(grcg.modereg & 8)) {										\
		ret |= LOADINTELWORD(vram + VRAM_E) ^ grcg.tile[3].w;		\
	}																\
	return((UINT16)(~ret));											\
}





// ---- functions

REG8 MEMCALL memvram0_rd8(UINT32 address)	VRAMRD8(0, address)
REG8 MEMCALL memvram1_rd8(UINT32 address)	VRAMRD8(1, address)
REG16 MEMCALL memvram0_rd16(UINT32 address)	VRAMRD16(0, address)
REG16 MEMCALL memvram1_rd16(UINT32 address)	VRAMRD16(1, address)
void MEMCALL memvram0_wr8(UINT32 address, REG8 value)
											VRAMWR8(0, address, value)
void MEMCALL memvram1_wr8(UINT32 address, REG8 value)
											VRAMWR8(1, address, value)
void MEMCALL memvram0_wr16(UINT32 address, REG16 value)
											VRAMWR16(0, address, value)
void MEMCALL memvram1_wr16(UINT32 address, REG16 value)
											VRAMWR16(1, address, value)

REG8 MEMCALL memtcr0_rd8(UINT32 address)	TCRRD8(0, address)
REG8 MEMCALL memtcr1_rd8(UINT32 address)	TCRRD8(1, address)
REG16 MEMCALL memtcr0_rd16(UINT32 address)	TCRRD16(0, address)
REG16 MEMCALL memtcr1_rd16(UINT32 address)	TCRRD16(1, address)

void MEMCALL memrmw0_wr8(UINT32 address, REG8 value)
											RMWWR8(0, address, value)
void MEMCALL memrmw1_wr8(UINT32 address, REG8 value)
											RMWWR8(1, address, value)
void MEMCALL memrmw0_wr16(UINT32 address, REG16 value)
											RMWWR16(0, address, value)
void MEMCALL memrmw1_wr16(UINT32 address, REG16 value)
											RMWWR16(1, address, value)

void MEMCALL memtdw0_wr8(UINT32 address, REG8 value)
											TDWWR8(0, address, value)
void MEMCALL memtdw1_wr8(UINT32 address, REG8 value)
											TDWWR8(1, address, value)
void MEMCALL memtdw0_wr16(UINT32 address, REG16 value)
											TDWWR16(0, address, value)
void MEMCALL memtdw1_wr16(UINT32 address, REG16 value)
											TDWWR16(1, address, value)

