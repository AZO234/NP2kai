/* === ISO-2022-JP (mod for PC98) and UTF-16(UCS2) code pair generator === (C) AZO */

// first install libnkf. https://github.com/AZO234/libnkf

// build: gcc jisutf16pairgen.c -lnkf -o jisutf16pairgen
// usage: ./jisutf16pairgen > jisutf16pair.txt

#include <stdio.h>
#include <stdint.h>
#include <libnkf.h>

int main(int iArgc, char* Argv[]) {
  uint16_t au16Sections_ASCII[][2] = {
    {0x0020, 0x007E},
// -> PC98
    {0x2921, 0x297E}
// <- PC98
  };
  uint16_t au16Sections_Katakana[][2] = {
    {0x0021, 0x005F},
// -> PC98
    {0x2A21, 0x2A7E}
// <- PC98
  };
  uint16_t au16Sections_Kanji[][2] = {
    {0x2121, 0x222E}, {0x223A, 0x2241}, {0x224A, 0x2250}, {0x225C, 0x226A},
    {0x2272, 0x2279}, {0x227E, 0x227E}, {0x2330, 0x2339}, {0x2341, 0x235A},
    {0x2361, 0x237A}, {0x2421, 0x2473}, {0x2521, 0x2576}, {0x2621, 0x2638},
    {0x2641, 0x2658}, {0x2721, 0x2741}, {0x2751, 0x2771}, //PC98 {0x2821, 0x2840},
// -> PC98
    {0x2B22, 0x2B6F}, {0x2C24, 0x2C6F},
// <- PC98
    {0x2D21, 0x2D3E}, {0x2D40 ,0x2D56}, {0x2D5F, 0x2D7C}, {0x3021, 0x4F53},
    {0x5021, 0x7426}, {0x7921, 0x7C6E}, {0x7C71, 0x7C7E}
  };

  uint8_t pc98;
  uint16_t j, uu;
  uint32_t a, b, l, n;
  char strJIS[6] = {0};
  char strJIS_ASCII[3]    = {0x1B, 0x28, 0x42};
  char strJIS_Katakana[3] = {0x1B, 0x28, 0x49};
  char strJIS_Kanji[3]    = {0x1B, 0x24, 0x42};
  char strOption8[]  = "-J -w8 -x";
  char strOption16[] = "-J -w16 -x";
  char* strUTF16;
  char* strUTF8;
  char* s;

  n = 0;
  for(a = 0; a < sizeof(au16Sections_ASCII) / (sizeof(uint16_t) * 2); a++) {
    for(j = au16Sections_ASCII[a][0]; j <= au16Sections_ASCII[a][1]; j++) {
      pc98 = 0;
      if(j >= 0x2921) {
        pc98 = 1;
      }
      strJIS[0] = strJIS_ASCII[0];
      strJIS[1] = strJIS_ASCII[1];
      strJIS[2] = strJIS_ASCII[2];
      if(!pc98) {
        strJIS[3] = j;
      } else {
        strJIS[3] = j & 0x7F;
      }
      strJIS[4] = '\0';
      l = 4;
      if(n % 4 == 0) {
        printf("\n ");
      }
      strUTF8  = nkf_convert(strJIS, l, strOption8, 9);
      printf(" /* %s */ ", strUTF8);
      free(strUTF8);
      strUTF16 = nkf_convert(strJIS, l, strOption16, 10);
      uu  = (uint16_t)((uint8_t*)strUTF16)[2] <<  8;
      uu += (uint16_t)((uint8_t*)strUTF16)[3]      ;
      printf("{0, 0x%04X, 0x%04X},", j, uu);
      free(strUTF16);
      n++;
    }
  }
  for(a = 0; a < sizeof(au16Sections_Katakana) / (sizeof(uint16_t) * 2); a++) {
    for(j = au16Sections_Katakana[a][0]; j <= au16Sections_Katakana[a][1]; j++) {
      pc98 = 0;
      if(j >= 0x2A21) {
        pc98 = 1;
      }
      strJIS[0] = strJIS_Katakana[0];
      strJIS[1] = strJIS_Katakana[1];
      strJIS[2] = strJIS_Katakana[2];
      if(!pc98) {
        strJIS[3] = j;
      } else {
        strJIS[3] = j & 0x7F;
      }
      strJIS[4] = '\0';
      l = 4;
      if(pc98 && strJIS[3] >= 0x60) {
        switch(strJIS[3]) {
        case 0x60:
          strJIS[0] = strJIS_Kanji[0];
          strJIS[1] = strJIS_Kanji[1];
          strJIS[2] = strJIS_Kanji[2];
          strJIS[3] = 0x25;
          strJIS[4] = 0x70;
          strJIS[5] = '\0';
          l = 5;
          break;
        case 0x61:
          strJIS[3] = 0x55;
          break;
        case 0x62:
          strJIS[0] = strJIS_Kanji[0];
          strJIS[1] = strJIS_Kanji[1];
          strJIS[2] = strJIS_Kanji[2];
          strJIS[3] = 0x25;
          strJIS[4] = 0x6E;
          strJIS[5] = '\0';
          l = 5;
          break;
        case 0x63:
          strJIS[3] = 0x29;
          break;
        case 0x64:
          strJIS[0] = strJIS_Kanji[0];
          strJIS[1] = strJIS_Kanji[1];
          strJIS[2] = strJIS_Kanji[2];
          strJIS[3] = 0x25;
          strJIS[4] = 0x76;
          strJIS[5] = '\0';
          l = 5;
          break;
        case 0x65:
          strJIS[3] = 0x33;
          strJIS[4] = 0x5E;
          strJIS[5] = '\0';
          l = 5;
          break;
        case 0x66:
          strJIS[3] = 0x36;
          strJIS[4] = 0x5E;
          strJIS[5] = '\0';
          l = 5;
          break;
        case 0x67:
          strJIS[3] = 0x37;
          strJIS[4] = 0x5E;
          strJIS[5] = '\0';
          l = 5;
          break;
        case 0x68:
          strJIS[3] = 0x38;
          strJIS[4] = 0x5E;
          strJIS[5] = '\0';
          l = 5;
          break;
        case 0x69:
          strJIS[3] = 0x39;
          strJIS[4] = 0x5E;
          strJIS[5] = '\0';
          l = 5;
          break;
        case 0x6A:
          strJIS[3] = 0x3A;
          strJIS[4] = 0x5E;
          strJIS[5] = '\0';
          l = 5;
          break;
        case 0x6B:
          strJIS[3] = 0x3B;
          strJIS[4] = 0x5E;
          strJIS[5] = '\0';
          l = 5;
          break;
        case 0x6C:
          strJIS[3] = 0x3C;
          strJIS[4] = 0x5E;
          strJIS[5] = '\0';
          l = 5;
          break;
        case 0x6D:
          strJIS[3] = 0x3D;
          strJIS[4] = 0x5E;
          strJIS[5] = '\0';
          l = 5;
          break;
        case 0x6E:
          strJIS[3] = 0x3E;
          strJIS[4] = 0x5E;
          strJIS[5] = '\0';
          l = 5;
          break;
        case 0x6F:
          strJIS[3] = 0x3F;
          strJIS[4] = 0x5E;
          strJIS[5] = '\0';
          l = 5;
          break;
        case 0x70:
          strJIS[3] = 0x40;
          strJIS[4] = 0x5E;
          strJIS[5] = '\0';
          l = 5;
          break;
        case 0x71:
          strJIS[3] = 0x41;
          strJIS[4] = 0x5E;
          strJIS[5] = '\0';
          l = 5;
          break;
        case 0x72:
          strJIS[3] = 0x42;
          strJIS[4] = 0x5E;
          strJIS[5] = '\0';
          l = 5;
          break;
        case 0x73:
          strJIS[3] = 0x43;
          strJIS[4] = 0x5E;
          strJIS[5] = '\0';
          l = 5;
          break;
        case 0x74:
          strJIS[3] = 0x44;
          strJIS[4] = 0x5E;
          strJIS[5] = '\0';
          l = 5;
          break;
        case 0x75:
          strJIS[3] = 0x4A;
          strJIS[4] = 0x5E;
          strJIS[5] = '\0';
          l = 5;
          break;
        case 0x76:
          strJIS[3] = 0x4A;
          strJIS[4] = 0x5F;
          strJIS[5] = '\0';
          l = 5;
          break;
        case 0x77:
          strJIS[3] = 0x4B;
          strJIS[4] = 0x5E;
          strJIS[5] = '\0';
          l = 5;
          break;
        case 0x78:
          strJIS[3] = 0x4B;
          strJIS[4] = 0x5F;
          strJIS[5] = '\0';
          l = 5;
          break;
        case 0x79:
          strJIS[3] = 0x4C;
          strJIS[4] = 0x5E;
          strJIS[5] = '\0';
          l = 5;
          break;
        case 0x7A:
          strJIS[3] = 0x4C;
          strJIS[4] = 0x5F;
          strJIS[5] = '\0';
          l = 5;
          break;
        case 0x7B:
          strJIS[3] = 0x4D;
          strJIS[4] = 0x5E;
          strJIS[5] = '\0';
          l = 5;
          break;
        case 0x7C:
          strJIS[3] = 0x4D;
          strJIS[4] = 0x5F;
          strJIS[5] = '\0';
          l = 5;
          break;
        case 0x7D:
          strJIS[3] = 0x4E;
          strJIS[4] = 0x5E;
          strJIS[5] = '\0';
          l = 5;
          break;
        case 0x7E:
          strJIS[3] = 0x4E;
          strJIS[4] = 0x5F;
          strJIS[5] = '\0';
          l = 5;
          break;
        }
      }
      if(n % 4 == 0) {
        printf("\n ");
      }
      strUTF8  = nkf_convert(strJIS, l, strOption8, 9);
      printf(" /* %s */ ", strUTF8);
      free(strUTF8);
      strUTF16 = nkf_convert(strJIS, l, strOption16, 10);
      uu  = (uint16_t)((uint8_t*)strUTF16)[2] <<  8;
      uu += (uint16_t)((uint8_t*)strUTF16)[3]      ;
      printf("{1, 0x%04X, 0x%04X},", j, uu);
      free(strUTF16);
      n++;
    }
  }
  for(a = 0; a < sizeof(au16Sections_Kanji) / (sizeof(uint16_t) * 2); a++) {
    for(j = au16Sections_Kanji[a][0]; j <= au16Sections_Kanji[a][1]; j++) {
      if(((j & 0xFF) >= 0x00 && (j & 0xFF) <= 0x20) || ((j & 0xFF) >= 0x7F && (j & 0xFF) <= 0xFF)) {
        continue;
      }
      pc98 = 0;
      if(j >= 0x2B22 && j <= 0x2C6F) {
        pc98 = 1;
      }
      strJIS[0] = strJIS_Kanji[0];
      strJIS[1] = strJIS_Kanji[1];
      strJIS[2] = strJIS_Kanji[2];
      strJIS[3] = (j >> 8) & 0xFF;
      strJIS[4] =  j       & 0xFF;
      strJIS[5] = '\0';
      l = 5;
      if(pc98) {
        switch(j) {
        case 0x2B22:
          strJIS[3] = 0x2D;
          strJIS[4] = 0x60;
          break;
        case 0x2B23:
          strJIS[3] = 0x2D;
          strJIS[4] = 0x61;
          break;
        case 0x2B24:
        case 0x2C24:
        case 0x2B28:
        case 0x2C28:
        case 0x2B2C:
        case 0x2C2C:
          strJIS[3] = 0x28;
          strJIS[4] = 0x21;
          break;
        case 0x2B25:
        case 0x2C25:
        case 0x2B29:
        case 0x2C29:
        case 0x2B2D:
        case 0x2C2D:
          strJIS[3] = 0x28;
          strJIS[4] = 0x2C;
          break;
        case 0x2B26:
        case 0x2C26:
        case 0x2B2A:
        case 0x2C2A:
        case 0x2B2E:
        case 0x2C2E:
          strJIS[3] = 0x28;
          strJIS[4] = 0x22;
          break;
        case 0x2B27:
        case 0x2C27:
        case 0x2B2B:
        case 0x2C2B:
        case 0x2B2F:
        case 0x2C2F:
          strJIS[3] = 0x28;
          strJIS[4] = 0x2D;
          break;
        case 0x2B30:
        case 0x2C30:
        case 0x2B31:
        case 0x2C31:
        case 0x2B32:
        case 0x2C32:
          strJIS[3] = 0x28;
          strJIS[4] = 0x23;
          break;
        case 0x2B33:
        case 0x2C33:
          strJIS[3] = 0x28;
          strJIS[4] = 0x2E;
          break;
        case 0x2B34:
        case 0x2C34:
        case 0x2B35:
        case 0x2C35:
        case 0x2B36:
        case 0x2C36:
          strJIS[3] = 0x28;
          strJIS[4] = 0x24;
          break;
        case 0x2B37:
        case 0x2C37:
          strJIS[3] = 0x28;
          strJIS[4] = 0x2F;
          break;
        case 0x2B38:
        case 0x2C38:
        case 0x2B39:
        case 0x2C39:
        case 0x2B3A:
        case 0x2C3A:
          strJIS[3] = 0x28;
          strJIS[4] = 0x26;
          break;
        case 0x2B3B:
        case 0x2C3B:
          strJIS[3] = 0x28;
          strJIS[4] = 0x31;
          break;
        case 0x2B3C:
        case 0x2C3C:
        case 0x2B3D:
        case 0x2C3D:
        case 0x2B3E:
        case 0x2C3E:
          strJIS[3] = 0x28;
          strJIS[4] = 0x25;
          break;
        case 0x2B3F:
        case 0x2C3F:
          strJIS[3] = 0x28;
          strJIS[4] = 0x30;
          break;
        case 0x2B40:
        case 0x2C40:
        case 0x2B41:
        case 0x2C41:
        case 0x2B42:
        case 0x2C42:
        case 0x2B43:
        case 0x2C43:
        case 0x2B45:
        case 0x2C45:
        case 0x2B46:
        case 0x2C46:
          strJIS[3] = 0x28;
          strJIS[4] = 0x27;
          break;
        case 0x2B44:
        case 0x2C44:
          strJIS[3] = 0x28;
          strJIS[4] = 0x37;
          break;
        case 0x2B47:
        case 0x2C47:
          strJIS[3] = 0x28;
          strJIS[4] = 0x32;
          break;
        case 0x2B48:
        case 0x2C48:
        case 0x2B49:
        case 0x2C49:
        case 0x2B4A:
        case 0x2C4A:
        case 0x2B4B:
        case 0x2C4B:
        case 0x2B4D:
        case 0x2C4D:
        case 0x2B4E:
        case 0x2C4E:
          strJIS[3] = 0x28;
          strJIS[4] = 0x29;
          break;
        case 0x2B4C:
        case 0x2C4C:
          strJIS[3] = 0x28;
          strJIS[4] = 0x39;
          break;
        case 0x2B4F:
        case 0x2C4F:
          strJIS[3] = 0x28;
          strJIS[4] = 0x34;
          break;
        case 0x2B50:
        case 0x2C50:
        case 0x2B51:
        case 0x2C51:
        case 0x2B52:
        case 0x2C52:
        case 0x2B53:
        case 0x2C53:
        case 0x2B55:
        case 0x2C55:
        case 0x2B56:
        case 0x2C56:
          strJIS[3] = 0x28;
          strJIS[4] = 0x28;
          break;
        case 0x2B54:
        case 0x2C54:
          strJIS[3] = 0x28;
          strJIS[4] = 0x38;
          break;
        case 0x2B57:
        case 0x2C57:
          strJIS[3] = 0x28;
          strJIS[4] = 0x33;
          break;
        case 0x2B58:
        case 0x2C58:
        case 0x2B59:
        case 0x2C59:
        case 0x2B5A:
        case 0x2C5A:
        case 0x2B5B:
        case 0x2C5B:
        case 0x2B5D:
        case 0x2C5D:
        case 0x2B5E:
        case 0x2C5E:
          strJIS[3] = 0x28;
          strJIS[4] = 0x2A;
          break;
        case 0x2B5C:
        case 0x2C5C:
          strJIS[3] = 0x28;
          strJIS[4] = 0x3A;
          break;
        case 0x2B5F:
        case 0x2C5F:
          strJIS[3] = 0x28;
          strJIS[4] = 0x35;
          break;
        case 0x2B60:
        case 0x2C60:
        case 0x2B61:
        case 0x2C61:
        case 0x2B62:
        case 0x2C62:
        case 0x2B63:
        case 0x2C63:
        case 0x2B64:
        case 0x2C64:
        case 0x2B65:
        case 0x2C65:
        case 0x2B66:
        case 0x2C66:
        case 0x2B67:
        case 0x2C67:
        case 0x2B68:
        case 0x2C68:
        case 0x2B69:
        case 0x2C69:
        case 0x2B6A:
        case 0x2C6A:
        case 0x2B6B:
        case 0x2C6B:
        case 0x2B6C:
        case 0x2C6C:
        case 0x2B6D:
        case 0x2C6D:
        case 0x2B6E:
        case 0x2C6E:
          strJIS[3] = 0x28;
          strJIS[4] = 0x2B;
          break;
        case 0x2B6F:
        case 0x2C6F:
          strJIS[3] = 0x28;
          strJIS[4] = 0x36;
          break;
        }
      }
      if(n % 4 == 0) {
        printf("\n ");
      }
      strUTF8  = nkf_convert(strJIS, l, strOption8, 9);
      printf(" /* %s */ ", strUTF8);
      free(strUTF8);
      strUTF16 = nkf_convert(strJIS, l, strOption16, 10);
      uu  = (uint16_t)((uint8_t*)strUTF16)[2] <<  8;
      uu += (uint16_t)((uint8_t*)strUTF16)[3]      ;
      printf("{2, 0x%04X, 0x%04X},", j, uu);
      free(strUTF16);
      n++;
    }
  }

  return 0;
}

