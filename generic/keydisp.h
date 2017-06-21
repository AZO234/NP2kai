/**
 * @file	keydisp.h
 * @brief	Interface of the key display
 */

#pragma once

#if defined(SUPPORT_KEYDISP)

#include "cmndraw.h"

struct _cmnpalfn
{
	UINT8	(*get8)(struct _cmnpalfn *fn, UINT num);
	UINT32	(*get32)(struct _cmnpalfn *fn, UINT num);
	UINT16	(*cnv16)(struct _cmnpalfn *fn, RGB32 pal32);
	INTPTR	userdata;
};
typedef struct _cmnpalfn	CMNPALFN;

enum
{
	KEYDISP_MODENONE			= 0,
	KEYDISP_MODEFM,
	KEYDISP_MODEMIDI
};

enum
{
	KEYDISP_CHMAX		= 48,
};

enum
{
	KEYDISP_NOTEMAX		= 16,

	KEYDISP_KEYCX		= 28,
	KEYDISP_KEYCY		= 14,

	KEYDISP_LEVEL		= (1 << 4),
	KEYDISP_LEVEL_MAX	= KEYDISP_LEVEL - 1,

	KEYDISP_WIDTH		= 301,
	KEYDISP_HEIGHT		= (KEYDISP_KEYCY * KEYDISP_CHMAX) + 1,

	KEYDISP_DELAYEVENTS	= 2048,
};

enum
{
	KEYDISP_PALBG		= 0,
	KEYDISP_PALFG,
	KEYDISP_PALHIT,

	KEYDISP_PALS
};

enum
{
	KEYDISP_FLAGDRAW		= 0x01,
	KEYDISP_FLAGREDRAW		= 0x02,
	KEYDISP_FLAGSIZING		= 0x04
};


#ifdef __cplusplus
extern "C"
{
#endif

void keydisp_initialize(void);
void keydisp_setmode(UINT8 mode);
void keydisp_setpal(CMNPALFN *palfn);
void keydisp_setdelay(UINT8 frames);
UINT8 keydisp_process(UINT8 framepast);
void keydisp_getsize(int *width, int *height);
BOOL keydisp_paint(CMNVRAM *vram, BOOL redraw);

void keydisp_reset(void);
void keydisp_bindopna(const UINT8 *pcRegister, UINT nChannels, UINT nBaseClock);
void keydisp_bindpsg(const UINT8 *pcRegister, UINT nBaseClock);
void keydisp_bindopl3(const UINT8 *pcRegister, UINT nChannels, UINT nBaseClock);
void keydisp_opnakeyon(const UINT8 *pcRegister, REG8 cData);
void keydisp_psg(const UINT8 *pcRegister, UINT nAddress);
void keydisp_opl3keyon(const UINT8 *pcRegister, REG8 nChannelNum, REG8 cData);
void keydisp_midi(const UINT8 *msg);

#ifdef __cplusplus
}
#endif

#else

#define keydisp_draw(a)
#define keydisp_reset()
#define keydisp_bindopna(r, c, b)
#define keydisp_bindpsg(r, b)
#define keydisp_bindopl3(r, c, b)
#define keydisp_opnakeyon(r, d)
#define keydisp_psg(r, a)
#define keydisp_opl3keyon(r, c, d)
#define	keydisp_midi(a)

#endif
