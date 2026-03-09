/**
 * @file	lio.h
 * @brief	Interface of LIO
 */

#pragma once

enum {
	LIO_SEGMENT		= 0xf990,
	LIO_FONT		= 0x00a0
};

enum {
	LIO_SUCCESS		= 0,
	LIO_ILLEGALFUNC	= 5,
	LIO_OUTOFMEMORY	= 7
};

enum {
	LIODRAW_PMASK	= 0x03,
	LIODRAW_MONO	= 0x04,
	LIODRAW_UPPER	= 0x20,
	LIODRAW_4BPP	= 0x40
};

typedef struct {
	UINT8	scrnmode;
	UINT8	pos;
	UINT8	plane;
	UINT8	fgcolor;
	UINT8	bgcolor;
	UINT8	padding;
	UINT8	color[8];
	UINT8	viewx1[2];
	UINT8	viewy1[2];
	UINT8	viewx2[2];
	UINT8	viewy2[2];
	UINT8	disp;
	UINT8	access;
} LIOWORK;

typedef struct {
	SINT16	x1;
	SINT16	y1;
	SINT16	x2;
	SINT16	y2;
	UINT32	base;
	UINT8	flag;
	UINT8	palmax;
	UINT8	bank;
	UINT8	sbit;
} LIODRAW;


typedef struct {
	LIOWORK	work;
	UINT8	palmode;

	// ---- work
	UINT32	wait;
	LIODRAW	draw;
} _GLIO, *GLIO;


#ifdef __cplusplus
extern "C" {
#endif

extern const UINT32 lioplaneadrs[4];

void lio_initialize(void);
void bios_lio(REG8 cmd);

void lio_updatedraw(GLIO lio);
void lio_pset(const _GLIO *lio, SINT16 x, SINT16 y, REG8 pal);
void lio_line(const _GLIO *lio, SINT16 x1, SINT16 x2, SINT16 y, REG8 pal);

REG8 lio_ginit(GLIO lio);
REG8 lio_gscreen(GLIO lio);
REG8 lio_gview(GLIO lio);
REG8 lio_gcolor1(GLIO lio);
REG8 lio_gcolor2(GLIO lio);
REG8 lio_gcls(GLIO lio);
REG8 lio_gpset(GLIO lio);
REG8 lio_gline(GLIO lio);
REG8 lio_gcircle(GLIO lio);
REG8 lio_gpaint1(GLIO lio);
REG8 lio_gpaint2(GLIO lio);
REG8 lio_gget(GLIO lio);
REG8 lio_gput1(GLIO lio);
REG8 lio_gput2(GLIO lio);
REG8 lio_groll(GLIO lio);
REG8 lio_gpoint2(GLIO lio);
REG8 lio_gcopy(GLIO lio);

#ifdef __cplusplus
}
#endif

