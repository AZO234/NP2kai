/**
 * @file	board14.h
 * @brief	Interface of PC-9801-14
 */

#pragma once

/**
 * @breif The sturecture of PC-9801-14
 */
struct musicgen_t
{
	UINT8	porta;
	UINT8	portb;
	UINT8	portc;
	UINT8	mask;
	UINT8	key[8];
	int		sync;
	int		ch;
};

typedef struct musicgen_t MUSICGEN;

#ifdef __cplusplus
extern "C"
{
#endif

extern	MUSICGEN	g_musicgen;

void musicgenint(NEVENTITEM item);
UINT board14_pitcount(void);

void board14_allkeymake(void);

void board14_reset(const NP2CFG *pConfig, BOOL bEnable);
void board14_bind(void);

#ifdef __cplusplus
}
#endif
