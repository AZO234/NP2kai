/**
 * @file boardws.h
 * @brief Q-Vision WaveStar sound board
 */
#pragma once

#include "pccore.h"

typedef struct {
	UINT8 fm_volume;
	UINT8 wss_mode;
	UINT8 unlock_index;
	UINT8 compat_status;
	UINT8 trap_4d2;
	UINT8 saved_pcm_irq;
	UINT8 saved_opna_irq;
	UINT8 pnp_config[0x100];

	UINT16 compat_port;
	UINT16 mpu_port;
	UINT16 fm_port;
	UINT16 diag_port;
	UINT8 irq;
	UINT8 mpu_irq;
	UINT8 dma;
	UINT8 pnp_enabled;
	UINT8 pcm86_enabled;
} WAVESTAR;

#ifdef __cplusplus
extern "C" {
#endif

extern WAVESTAR wavestar;

void boardws_reset(const NP2CFG *pConfig);
void boardws_bind(void);
void boardws_unbind(void);
UINT8 boardws_getfmvolume(void);
UINT8 boardws_getirq(void);
BOOL boardws_setirq(UINT8 irq);
UINT8 boardws_getmpuirq(void);
BOOL boardws_setmpuirq(UINT8 irq);

#ifdef __cplusplus
}
#endif
