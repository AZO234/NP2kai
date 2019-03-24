/**
 * @file	dmac.h
 * @brief	Interface of the DMA controller
 */

#pragma once

#include "pccore.h"

#ifndef DMACCALL
#define DMACCALL
#endif


enum {
	DMAEXT_START		= 0,
	DMAEXT_END		= 1,
	DMAEXT_BREAK		= 2,

	DMA_INITSIGNALONLY	= 1,

	DMADEV_NONE			= 0,
	DMADEV_2HD			= 1,
	DMADEV_2DD			= 2,
	DMADEV_SASI			= 3,
	DMADEV_SCSI			= 4,
	DMADEV_CS4231		= 5,
	DMADEV_CT1741		= 6
};

#if defined(BYTESEX_LITTLE)
enum {
	DMA16_LOW		= 0,
	DMA16_HIGH		= 1,
	DMA32_LOW		= 0,
	DMA32_HIGH		= 2
};
#elif defined(BYTESEX_BIG)
enum {
	DMA16_LOW		= 1,
	DMA16_HIGH		= 0,
	DMA32_LOW		= 2,
	DMA32_HIGH		= 0
};
#endif

typedef struct {
	void	(DMACCALL * outproc)(REG8 data);
	REG8	(DMACCALL * inproc)(void);
	REG8	(DMACCALL * extproc)(REG8 action);
} DMAPROC;

typedef struct {
	UINT32 startaddr;
	UINT32 lastaddr;
	UINT16 startcount;
	union {
		UINT8	b[4];
		UINT16	w[2];
		UINT32	d;
	} adrs;
	union {
		UINT8	b[2];
		UINT16	w;
	} leng;
	union {
		UINT8	b[2];
		UINT16	w;
	} adrsorg;
	union {
		UINT8	b[2];
		UINT16	w;
	} lengorg;
	UINT8	bound;
	UINT8	action;
	DMAPROC	proc;
	UINT8	mode;
	UINT8	sreq;
	UINT8	ready;
	UINT8	mask;
} _DMACH, *DMACH;

typedef struct {
	UINT8	device;
	UINT8	channel;
} DMADEV;

typedef struct {
	_DMACH	dmach[8];
	int		lh;
	UINT8	work;
	UINT8	working;
	UINT8	mask;
	UINT8	stat;
	UINT	devices;
	DMADEV	device[8];
} _DMAC, *DMAC;


#ifdef __cplusplus
extern "C" {
#endif

void DMACCALL dma_dummyout(REG8 data);
REG8 DMACCALL dma_dummyin(void);
REG8 DMACCALL dma_dummyproc(REG8 func);

void dmac_reset(const NP2CFG *pConfig);
void dmac_bind(void);
void dmac_extbind(void);

void dmac_check(void);
UINT dmac_getdatas(DMACH dmach, UINT8 *buf, UINT size);
void dmac_procset(void);
void dmac_attach(REG8 device, REG8 channel);
void dmac_detach(REG8 device);

#ifdef __cplusplus
}
#endif

