
#ifdef __cplusplus
extern "C" {
#endif

#if (!defined(MEMOPTIMIZE)) || (MEMOPTIMIZE == 0)

extern	UINT32	grph_table[4*256*2];

#define GRPHDATASET(d, a) {							\
	UINT8 dat;										\
	UINT32 l32, r32;								\
	dat = mem[(a) + VRAM_B];						\
	l32 = grph_table[dat*2 + 0*0x200 + 0];			\
	r32 = grph_table[dat*2 + 0*0x200 + 1];			\
	dat = mem[(a) + VRAM_R];						\
	l32 += grph_table[dat*2 + 1*0x200 + 0];			\
	r32 += grph_table[dat*2 + 1*0x200 + 1];			\
	dat = mem[(a) + VRAM_G];						\
	l32 += grph_table[dat*2 + 2*0x200 + 0];			\
	r32 += grph_table[dat*2 + 2*0x200 + 1];			\
	dat = mem[(a) + VRAM_E];						\
	l32 += grph_table[dat*2 + 3*0x200 + 0];			\
	r32 += grph_table[dat*2 + 3*0x200 + 1];			\
	(d)[0] = l32;									\
	(d)[1] = r32;									\
}

#elif (MEMOPTIMIZE == 1)						// for Mac

extern	UINT32	grph_table1[256*2];

#define GRPHDATASET(d, a) {							\
	UINT8 dat;										\
	UINT32 l32, r32;								\
	dat = mem[(a) + VRAM_B];						\
	l32 = grph_table1[dat*2 + 0];					\
	r32 = grph_table1[dat*2 + 1];					\
	dat = mem[(a) + VRAM_R];						\
	l32 += grph_table1[dat*2 + 0] << 1;				\
	r32 += grph_table1[dat*2 + 1] << 1;				\
	dat = mem[(a) + VRAM_G];						\
	l32 += grph_table1[dat*2 + 0] << 2;				\
	r32 += grph_table1[dat*2 + 1] << 2;				\
	dat = mem[(a) + VRAM_E];						\
	l32 += grph_table1[dat*2 + 0] << 3;				\
	r32 += grph_table1[dat*2 + 1] << 3;				\
	(d)[0] = l32;									\
	(d)[1] = r32;									\
}

#else											// for ARM

extern	UINT32	grph_table0[16];

#define GRPHDATASET(d, a) {							\
	UINT8 dat;										\
	UINT32 l32, r32;								\
	dat = mem[(a) + VRAM_B];						\
	l32 = grph_table0[dat >> 4];					\
	r32 = grph_table0[dat & 15];					\
	dat = mem[(a) + VRAM_R];						\
	l32 += grph_table0[dat >> 4] << 1;				\
	r32 += grph_table0[dat & 15] << 1;				\
	dat = mem[(a) + VRAM_G];						\
	l32 += grph_table0[dat >> 4] << 2;				\
	r32 += grph_table0[dat & 15] << 2;				\
	dat = mem[(a) + VRAM_E];						\
	l32 += grph_table0[dat >> 4] << 3;				\
	r32 += grph_table0[dat & 15] << 3;				\
	(d)[0] = l32;									\
	(d)[1] = r32;									\
}

#endif

#ifdef __cplusplus
}
#endif

