/* === NP2 debug snapshot === (c) 2020 AZO */

#ifndef _DEBUGSNAPSHOT_H_
#define _DEBUGSNAPSHOT_H_

#include <compiler.h>
#include <pccore.h>
#include <np2.h>
#include <fdd/sxsi.h>
#include <diskimage/fddfile.h>

#ifdef __cplusplus
extern "C" {
#endif

UINT calc_sha1(UINT8 au8SHA1[20], const OEMCHAR* strFilepath);

typedef void* HCALCSHA1;
HCALCSHA1 calc_sha1_begin(void);
UINT calc_sha1_add(HCALCSHA1 hCalc, const UINT8* pu8Data, const UINT uLen);
UINT calc_sha1_end(HCALCSHA1 hCalc, UINT8 au8SHA1[20]);

#define DEBUGSS_DIRNAME  OEMTEXT("debugss")
#define DEBUGSS_FILENAME OEMTEXT("debugss")

extern UINT NP2_DebugSnapshot_Count;

typedef struct NP2_DebugSnapshot_t_ {
  OEMCHAR  strProgramType[32];
  OEMCHAR  strVersion[256];
  OEMCHAR  strStatePath[MAX_PATH];
  OEMCHAR  strBMPPath[MAX_PATH];
  UINT     uFDMount;
  UINT     auFDType[MAX_FDDFILE];
  OEMCHAR  astrFDImagePath[MAX_FDDFILE][MAX_PATH];
  UINT     auFDRO[MAX_FDDFILE];
  UINT8    auFDNum[MAX_FDDFILE];
  UINT8    auFDProtect[MAX_FDDFILE];
  union fdinfo auFDInfo[MAX_FDDFILE];
  UINT8    au8FDHash[MAX_FDDFILE][20];  // SHA1
  UINT     uHDCDMount;
  UINT     auHDCDType[SASIHDD_MAX + SCSIHDD_MAX];
  OEMCHAR  astrHDCDImagePath[SASIHDD_MAX + SCSIHDD_MAX][MAX_PATH];
  UINT8    au8CDHash[SASIHDD_MAX + SCSIHDD_MAX][20];  // SHA1
  NP2CFG   np2cfg;
  NP2OSCFG np2oscfg;
} NP2_DebugSnapshot_t;

int debugsnapshot_save(const UINT uNo);
int debugsnapshot_load(const UINT uNo);

#ifdef __cplusplus
}
#endif

#endif  // _DEBUGSNAPSHOT_H_

