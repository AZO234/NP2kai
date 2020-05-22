/* === Debug snapshot data viewer === (C) AZO */

// Windows(MSVC):
//   cl debugsnapshot_viewer.c -DDBGSSVIEWER -I../ -Wall -DSUPPORT_IDEIO -DSUPPORT_SCSI -DSUPPORT_KAI_IMAGES -DSUPPORT_NVL_IMAGES -utf-8
// GCC(MinGW/Linux):
//   gcc debugsnapshot_viewer.c -DDBGSSVIEWER -I../ -Wall -Wextra -o debugsnapshot_viewer -DSUPPORT_IDEIO -DSUPPORT_SCSI -DSUPPORT_KAI_IMAGES -DSUPPORT_NVL_IMAGES

#include "debugsnapshot.h"

#if defined(_MSC_VER)
#include "codecnv/codecnv.h"
UINT16 u16Path[MAX_PATH];
#endif

NP2_DebugSnapshot_t tDebugSnapshot;

int main(int iArgc, char* astrArgv[]) {
  int iRes = 0;
  int i, j;
  FILE* f;

  if(iArgc < 2 || iArgc > 2) {
    printf("Usage: debugsnapshot_viewer <debugsnapshot.data>\n");
    iRes = 1;
  }

  if(!iRes) {
    f = fopen(astrArgv[1], "rb");
    if(!f) {
      printf("Error: couldn't open debugsnapshot.data\n");
      iRes = 2;
    }
  }
  if(!iRes) {
    fread(&tDebugSnapshot, sizeof(NP2_DebugSnapshot_t), 1, f);
    fclose(f);

    printf("NP2 program type: %s\n", tDebugSnapshot.strProgramType);
    printf("NP2 version: %s\n", tDebugSnapshot.strVersion);
#if defined(_MSC_VER)
    codecnv_utf8tousc2(u16Path, MAX_PATH, tDebugSnapshot.strStatePath, -1);
    wprintf("state file: %ls\n", tDebugSnapshot.strStatePath);
#else
    printf("state file: %s\n", tDebugSnapshot.strStatePath);
#endif
#if defined(_MSC_VER)
    codecnv_utf8tousc2(u16Path, MAX_PATH, tDebugSnapshot.strBMPPath, -1);
    wprintf("bitmap file: %ls\n", u16Path);
#else
    printf("bitmap file: %s\n", tDebugSnapshot.strBMPPath);
#endif
    printf("\n");

    printf("FDD mount flag: %X\n", tDebugSnapshot.uFDMount);
    for(i = 0; i < MAX_FDDFILE; i++) {
      if(tDebugSnapshot.uFDMount & (1 << i)) {
        printf("--- FDD %d info ---\n", i);
        printf("Type: %X\n", tDebugSnapshot.auFDType[i]);
#if defined(_MSC_VER)
        codecnv_utf8tousc2(u16Path, MAX_PATH, tDebugSnapshot.astrFDImagePath[i], -1);
        wprintf("image file: %ls\n", u16Path);
#else
        printf("image file: %s\n", tDebugSnapshot.astrFDImagePath[i]);
#endif
        printf("RO: %d\n", tDebugSnapshot.auFDRO[i]);
        printf("Num: %d\n", tDebugSnapshot.auFDNum[i]);
        printf("Protect: %d\n", tDebugSnapshot.auFDProtect[i]);
        printf("SHA-1: ");
        for(j = 0; j < 20; j++) {
          printf("%02x", tDebugSnapshot.au8FDHash[i][j]);
        }
        printf("\n");
      }
    }
    printf("\n");

    printf("HDCDD mount flag: %X\n", tDebugSnapshot.uHDCDMount);
    for(i = 0; i < SASIHDD_MAX + SCSIHDD_MAX; i++) {
      if(tDebugSnapshot.uHDCDMount & (1 << i)) {
        printf("--- HDCDD %d info ---\n", i);
        printf("Type: %X\n", tDebugSnapshot.auHDCDType[i]);
#if defined(_MSC_VER)
        codecnv_utf8tousc2(u16Path, MAX_PATH, tDebugSnapshot.astrHDCDImagePath[i], -1);
        wprintf("image file: %ls\n", u16Path);
#else
        printf("image file: %s\n", tDebugSnapshot.astrHDCDImagePath[i]);
#endif
        if(tDebugSnapshot.auHDCDType[i] == SXSIDEV_CDROM) {
          printf("SHA-1: ");
          for(j = 0; j < 20; j++) {
            printf("%02x", tDebugSnapshot.au8CDHash[i][j]);
          }
          printf("\n");
        }
      }
    }
  }

  return 0;
}



