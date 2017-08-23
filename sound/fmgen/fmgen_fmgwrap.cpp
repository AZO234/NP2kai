#include "compiler.h"

#include "fmgen_fmgwrap.h"
#include "fmgen_opm.h"
#include "fmgen_opna.h"
#include "fmgen_fmgen.h"

//	YM2203(OPN) ----------------------------------------------------
void*	OPN_Construct(void) { return new FM::OPN; }
void	OPN_Destruct(void* OPN) { if(OPN) delete (FM::OPN*)OPN; }

bool	OPN_Init(void* OPN, uint c, uint r, bool ip, const char* str) { return ((FM::OPN*)OPN)->Init(c, r, ip); }
bool	OPN_SetRate(void* OPN, uint c, uint r, bool b) { return ((FM::OPN*)OPN)->SetRate(c, r); }

void	OPN_SetVolumeFM(void* OPN, int db) { ((FM::OPN*)OPN)->SetVolumeFM(db); }
void	OPN_SetVolumePSG(void* OPN, int db) { ((FM::OPN*)OPN)->SetVolumePSG(db); }
void	OPN_SetLPFCutoff(void* OPN, uint freq) { ((FM::OPN*)OPN)->SetLPFCutoff(freq); }

bool	OPN_Count(void* OPN, int32 us) { ((FM::OPN*)OPN)->Count(us); }
int32	OPN_GetNextEvent(void* OPN) { ((FM::OPN*)OPN)->GetNextEvent(); }

void	OPN_Reset(void* OPN) { ((FM::OPN*)OPN)->Reset(); }
void 	SOUNDCALL OPN_Mix(void* OPN, int32* buffer, int nsamples) { ((FM::OPN*)OPN)->Mix(buffer, nsamples); }
void 	OPN_SetReg(void* OPN, uint addr, uint data) { ((FM::OPN*)OPN)->SetReg(addr, data); }
uint	OPN_GetReg(void* OPN, uint addr) { return ((FM::OPN*)OPN)->GetReg(addr); }

uint	OPN_ReadStatus(void* OPN) { return ((FM::OPN*)OPN)->ReadStatus(); }
uint	OPN_ReadStatusEx(void* OPN) { return ((FM::OPN*)OPN)->ReadStatusEx(); }

void	OPN_SetChannelMask(void* OPN, uint mask) { ((FM::OPN*)OPN)->SetChannelMask(mask); }

int	OPN_dbgGetOpOut(void* OPN, int c, int s) { return ((FM::OPN*)OPN)->dbgGetOpOut(c, s); }
int	OPN_dbgGetPGOut(void* OPN, int c, int s) { return ((FM::OPN*)OPN)->dbgGetPGOut(c, s); }
void* OPN_dbgGetCh(void* OPN, int c) { return ((FM::OPN*)OPN)->dbgGetCh(c); }

//	YM2608(OPNA) ---------------------------------------------------
void*	OPNA_Construct(void) { return new FM::OPNA; }
void	OPNA_Destruct(void* OPNA) { if(OPNA) delete (FM::OPNA*)OPNA; }

bool	OPNA_Init(void* OPNA, uint c, uint r, bool b, const char* str) { return ((FM::OPNA*)OPNA)->Init(c, r, b, str); }
bool	OPNA_LoadRhythmSample(void* OPNA, const char* str) { return ((FM::OPNA*)OPNA)->LoadRhythmSample(str); }

void	OPNA_SetVolumeFM(void* OPNA, int db) { ((FM::OPNA*)OPNA)->SetVolumeFM(db); }
void	OPNA_SetVolumePSG(void* OPNA, int db) { ((FM::OPNA*)OPNA)->SetVolumePSG(db); }
void	OPNA_SetLPFCutoff(void* OPNA, uint freq) { ((FM::OPNA*)OPNA)->SetLPFCutoff(freq); }

bool	OPNA_SetRate(void* OPNA, uint c, uint r, bool b) { return ((FM::OPNA*)OPNA)->SetRate(c, r, b); }
void 	SOUNDCALL OPNA_Mix(void* OPNA, int32* buffer, int nsamples) { ((FM::OPNA*)OPNA)->Mix(buffer, nsamples); }

bool	OPNA_Count(void* OPNA, int32 us) { ((FM::OPNA*)OPNA)->Count(us); }
int32	OPNA_GetNextEvent(void* OPNA) { ((FM::OPNA*)OPNA)->GetNextEvent(); }

void	OPNA_Reset(void* OPNA) { ((FM::OPNA*)OPNA)->Reset(); }
void 	OPNA_SetReg(void* OPNA, uint addr, uint data) { ((FM::OPNA*)OPNA)->SetReg(addr, data); }
uint	OPNA_GetReg(void* OPNA, uint addr) { return ((FM::OPNA*)OPNA)->GetReg(addr); }

uint	OPNA_ReadStatus(void* OPNA) { ((FM::OPNA*)OPNA)->ReadStatus(); }
uint	OPNA_ReadStatusEx(void* OPNA) { ((FM::OPNA*)OPNA)->ReadStatusEx(); }

void	OPNA_SetVolumeADPCM(void* OPNA, int db) { ((FM::OPNA*)OPNA)->SetVolumeADPCM(db); }
void	OPNA_SetVolumeRhythmTotal(void* OPNA, int db) { ((FM::OPNA*)OPNA)->SetVolumeRhythmTotal(db); }
void	OPNA_SetVolumeRhythm(void* OPNA, int index, int db) { ((FM::OPNA*)OPNA)->SetVolumeRhythm(index, db); }

uint8*	OPNA_GetADPCMBuffer(void* OPNA) { return ((FM::OPNA*)OPNA)->GetADPCMBuffer(); }

int	OPNA_dbgGetOpOut(void* OPNA, int c, int s) { return ((FM::OPNA*)OPNA)->dbgGetOpOut(c, s); }
int	OPNA_dbgGetPGOut(void* OPNA, int c, int s) { return ((FM::OPNA*)OPNA)->dbgGetPGOut(c, s); }
void* OPNA_dbgGetCh(void* OPNA, int c) { return ((FM::OPNA*)OPNA)->dbgGetCh(c); }

//	YM2151(OPM) ----------------------------------------------------
void*	OPM_Construct(void) { return new FM::OPM; }
void	OPM_Destruct(void* OPM) { if(OPM) delete (FM::OPM*)OPM; }

//bool	OPM_Init(void* OPM, uint c, uint r, bool ip) { return ((FM::OPM*)OPM)->Init(c, r, ip); }
//bool	OPM_SetRate(void* OPM, uint c, uint r, bool b) { return ((FM::OPM*)OPM)->SetRate(c, r, b); }
void	OPM_Reset(void* OPM) { ((FM::OPM*)OPM)->Reset(); }

bool	OPM_Count(void* OPM, int32 us) { ((FM::OPM*)OPM)->Count(us); }
int32	OPM_GetNextEvent(void* OPM) { ((FM::OPM*)OPM)->GetNextEvent(); }

void 	OPM_SetReg(void* OPM, uint addr, uint data) { ((FM::OPM*)OPM)->SetReg(addr, data); }
uint	OPM_ReadStatus(void* OPM) { return ((FM::OPM*)OPM)->ReadStatus(); }

void 	SOUNDCALL OPM_Mix(void* OPM, int32* buffer, int nsamples) { ((FM::OPM*)OPM)->Mix(buffer, nsamples); }

void	OPM_SetVolume(void* OPM, int db) { ((FM::OPM*)OPM)->SetVolume(db); }
void	OPM_SetChannelMask(void* OPM, uint mask) { ((FM::OPM*)OPM)->SetChannelMask(mask); }

