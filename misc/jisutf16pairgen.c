/* === ISO-2022-JP and UTF-16(UCS2) code pair generator === (C) AZO */

// first install libnkf. https://github.com/AZO234/libnkf

// build: gcc jisutf16pairgen.c -lnkf -o jisutf16pairgen
// usage: ./jisutf16pairgen > jisutf16pair.txt

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <libnkf.h>

int main(int iArgc, char* Argv[]) {
  uint16_t au16Sections[][2] = {
//    {0x0020, 0x007E}, {0x00A1, 0x00DF},
    {0x0020, 0x007E},
    {0x2121, 0x222E}, {0x223A, 0x2241}, {0x224A, 0x2250}, {0x225C, 0x226A},
    {0x2272, 0x2279}, {0x227E, 0x227E}, {0x2330, 0x2339}, {0x2341, 0x235A},
    {0x2361, 0x237A}, {0x2421, 0x2473}, {0x2521, 0x2576}, {0x2621, 0x2638},
    {0x2641, 0x2658}, {0x2721, 0x2741}, {0x2751, 0x2771}, {0x2821, 0x2840},
    {0x2D21, 0x2D3E}, {0x2D40 ,0x2D56}, {0x2D5F, 0x2D7C}, {0x3021, 0x4F53},
    {0x5021, 0x7426}, {0x7921, 0x7C6E}, {0x7C71, 0x7C7E}
  };

  int ul;
  uint16_t j, uu0, uu1;
  uint32_t a, b, n;
  char strJIS[9] = {0x1B, 0x24, 0x42, 0x21, 0x21, 0x1B, 0x28, 0x42, '\0'};
  char strOption8[]  = "-J -w8";
  char strOption16[] = "-J -w16";
  char* strUTF16;
  char* strUTF8;
  char* s;

  n = 0;
  for(a = 0; a < sizeof(au16Sections) / (sizeof(uint16_t) * 2); a++) {
    for(j = au16Sections[a][0]; j <= au16Sections[a][1]; j++) {
      if(j < 0x80) {
        strJIS[0] = j;
        strJIS[1] = '\0';
      } else {
        strJIS[0] = 0x1B;
        strJIS[1] = 0x24;
        strJIS[2] = 0x42;
        s = &strJIS[3];
        if(j < 0x100) {
          *s++ =  j       & 0xFF;
        } else {
          if(
            ((j & 0xFF) >= 0x00 && (j & 0xFF) <= 0x20) ||
            ((j & 0xFF) >= 0x7F && (j & 0xFF) <= 0xFF)
          ) {
            continue;
          }
          *s++ = (j >> 8) & 0xFF;
          *s++ =  j       & 0xFF;
        }
        *s++ = 0x1B;
        *s++ = 0x28;
        *s++ = 0x42;
        *s++ = '\0';
      }
      if(n % 4 == 0) {
        printf("\n ");
      }
      strUTF8  = nkf_convert(strJIS, strlen(strJIS), strOption8,  strlen(strOption8 ));
      strUTF16 = nkf_convert(strJIS, strlen(strJIS), strOption16, strlen(strOption16));
      uu0  = (uint16_t)((uint8_t*)strUTF16)[0] <<  8;
      uu0 += (uint16_t)((uint8_t*)strUTF16)[1]      ;
      uu1 += (uint16_t)((uint8_t*)strUTF16)[2] <<  8;
      uu1 += (uint16_t)((uint8_t*)strUTF16)[3]      ;
      printf(" /* %s */ {0x%04X, 0x%04X, 0x%04X},", strUTF8, j, uu0, uu1);
      free(strUTF8);
      free(strUTF16);
      n++;
    }
  }

  return 0;
}

