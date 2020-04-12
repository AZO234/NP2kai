/* === Shift_JIS and UTF-16(UCS2) code pair generator === (C) AZO */

// first install libnkf. https://github.com/AZO234/libnkf

// build: gcc sjisutf16pairgen.c -lnkf -o sjisutf16pairgen
// usage: ./sjisutf16pairgen > sjisutf16pair.txt

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <libnkf.h>

int main(int iArgc, char* Argv[]) {
  uint16_t au16Sections[][2] = {
    {0x0020, 0x007E}, {0x00A1, 0x00DF},
    {0x8140, 0x81AC}, {0x81B8, 0x81BF}, {0x81C8, 0x81CE}, {0x81DA, 0x81E8},
    {0x81F0, 0x81F7}, {0x81FC, 0x81FC}, {0x824F, 0x8258}, {0x8260, 0x8279},
    {0x8281, 0x829A}, {0x829F, 0x82F1}, {0x8340, 0x8396}, {0x839F, 0x83B6},
    {0x83BF, 0x83D6}, {0x8440, 0x8460}, {0x8470, 0x8491}, {0x8470, 0x8491},
    {0x849F, 0x84BE}, {0x8740, 0x875D}, {0x875F, 0x8775}, {0x877E, 0x879C},
    {0x889F, 0x9872}, {0x989F, 0x9FFC}, {0xE040, 0xEAA4}, {0xED40, 0xEEEC},
    {0xEEEF, 0xEEFC},
  };

  int ul;
  uint16_t sj, uu0, uu1;
  uint32_t a, n;
  char strSJIS[3];
  char strOption8[]  = "-S -w8";
  char strOption16[] = "-S -w16";
  char* strUTF16;
  char* strUTF8;

  n = 0;
  for(a = 0; a < sizeof(au16Sections) / (sizeof(uint16_t) * 2); a++) {
    for(sj = au16Sections[a][0]; sj <= au16Sections[a][1]; sj++) {
      if(sj >= 0x100) {
        if(
          ( sj & 0xFF) == 0x7F ||
          ((sj & 0xFF) >= 0x00 && (sj & 0xFF) <= 0x3F) ||
          ((sj & 0xFF) >= 0xFD && (sj & 0xFF) <= 0xFF)
        ) {
          continue;
        }
      }
      if(n % 4 == 0) {
        printf("\n ");
      }
      if(sj < 0x0100) {
        strSJIS[0] = sj & 0xFF;
        strSJIS[1] = '\0';
      } else {
        strSJIS[0] = (sj >> 8) & 0xFF;
        strSJIS[1] =  sj       & 0xFF;
        strSJIS[2] = '\0';
      }
      strUTF8  = nkf_convert(strSJIS, strlen(strSJIS), strOption8,  strlen(strOption8 ));
      strUTF16 = nkf_convert(strSJIS, strlen(strSJIS), strOption16, strlen(strOption16));
      uu0  = (uint16_t)((uint8_t*)strUTF16)[0] <<  8;
      uu0 += (uint16_t)((uint8_t*)strUTF16)[1]      ;
      uu1 += (uint16_t)((uint8_t*)strUTF16)[2] <<  8;
      uu1 += (uint16_t)((uint8_t*)strUTF16)[3]      ;
      printf(" /* %s */ {0x%04X, 0x%04X, 0x%04X},", strUTF8, sj, uu0, uu1);
      free(strUTF8);
      free(strUTF16);
      n++;
    }
  }

  return 0;
}

