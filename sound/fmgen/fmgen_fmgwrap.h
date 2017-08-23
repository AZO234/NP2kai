#ifndef FMGEN_FMGWRAP_H
#define FMGEN_FMGWRAP_H

#include "compiler.h"
#include "fmgen_types.h"

#ifdef __cplusplus
extern "C" {
#endif

//	YM2203(OPN) ----------------------------------------------------
void*	OPN_Construct(void);
void	OPN_Destruct(void* OPN);

bool	OPN_Init(void* OPN, uint c, uint r, bool b, const char* str);
bool	OPN_SetRate(void* OPN, uint c, uint r, bool b);

void	OPN_SetVolumeFM(void* OPN, int db);
void	OPN_SetVolumePSG(void* OPN, int db);
void	OPN_SetLPFCutoff(void* OPN, uint freq);

bool	OPN_Count(void* OPN, int32 us);
int32	OPN_GetNextEvent(void* OPN);

void	OPN_Reset(void* OPN);
void 	SOUNDCALL OPN_Mix(void* OPN, int32* buffer, int nsamples);
void 	OPN_SetReg(void* OPN, uint addr, uint data);
uint	OPN_GetReg(void* OPN, uint addr);

uint	OPN_ReadStatus(void* OPN);
uint	OPN_ReadStatusEx(void* OPN);

void	OPN_SetChannelMask(void* OPN, uint mask);

int	OPN_dbgGetOpOut(void* OPN, int c, int s);
int	OPN_dbgGetPGOut(void* OPN, int c, int s);
void* OPN_dbgGetCh(void* OPN, int c);

//	YM2608(OPNA) ---------------------------------------------------
void*	OPNA_Construct(void);
void	OPNA_Destruct(void* OPNA);

bool	OPNA_Init(void* OPNA, uint c, uint r, bool b, const char* str);
bool	OPNA_LoadRhythmSample(void* OPNA, const char* str);

void	OPNA_SetVolumeFM(void* OPNA, int db);
void	OPNA_SetVolumePSG(void* OPNA, int db);
void	OPNA_SetLPFCutoff(void* OPNA, uint freq);

bool	OPNA_SetRate(void* OPNA, uint c, uint r, bool b);
void 	SOUNDCALL OPNA_Mix(void* OPNA, int32* buffer, int nsamples);

bool	OPNA_Count(void* OPNA, int32 us);
int32	OPNA_GetNextEvent(void* OPNA);

void	OPNA_Reset(void* OPNA);
void 	OPNA_SetReg(void* OPNA, uint addr, uint data);
uint	OPNA_GetReg(void* OPNA, uint addr);

uint	OPNA_ReadStatus(void* OPNA);
uint	OPNA_ReadStatusEx(void* OPNA);

void	OPNA_SetVolumeADPCM(void* OPNA, int db);
void	OPNA_SetVolumeRhythmTotal(void* OPNA, int db);
void	OPNA_SetVolumeRhythm(void* OPNA, int index, int db);

uint8*	OPNA_GetADPCMBuffer(void* OPNA);

int	OPNA_dbgGetOpOut(void* OPNA, int c, int s);
int	OPNA_dbgGetPGOut(void* OPNA, int c, int s);
void* OPNA_dbgGetCh(void* OPNA, int c);

//	YM2151(OPM) ----------------------------------------------------
void*	OPM_Construct(void);
void	OPM_Destruct(void* OPM);

bool	OPM_Init(void* OPM, uint c, uint r, bool b);
bool	OPM_SetRate(void* OPM, uint c, uint r, bool b);
void	OPM_SetLPFCutoff(void* OPM, uint freq);
void	OPM_Reset(void* OPM);

bool	OPM_Count(void* OPM, int32 us);
int32	OPM_GetNextEvent(void* OPM);

void 	OPM_SetReg(void* OPM, uint addr, uint data);
uint	OPM_GetReg(void* OPM, uint addr);
uint	OPM_ReadStatus(void* OPM);

void 	SOUNDCALL  OPM_Mix(void* OPM, int32* buffer, int nsamples);

void	OPM_SetVolume(void* OPM, int db);
void	OPM_SetChannelMask(void* OPM, uint mask);

#ifdef __cplusplus
}
#endif

#endif	/* FMGEN_FMGWRAP_H */

