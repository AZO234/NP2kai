#ifndef LIBRETRO_CORE_OPTIONS_H__
#define LIBRETRO_CORE_OPTIONS_H__

#include <stdlib.h>
#include <string.h>

#include <libretro.h>
#include <retro_inline.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 ********************************
 * Core Option Definitions
 ********************************
*/

/* RETRO_LANGUAGE_ENGLISH */

/* Default language:
 * - All other languages must include the same keys and values
 * - Will be used as a fallback in the event that frontend language
 *   is not available
 * - Will be used as a fallback for any missing entries in
 *   frontend language definition */

struct retro_core_option_definition option_defs_us[] = {
   {
      "np2kai_drive",
      "Swap Disks on Drive",
      NULL,
      {
         { "FDD1", NULL },
         { "FDD2", NULL },
         { NULL, NULL},
      },
      "FDD2"
   },
   {
      "np2kai_keyboard",
      "Keyboard (Restart)",
      "Japanese or US keyboard type.",
      {
         { "Ja", NULL },
         { "Us", NULL },
         { NULL, NULL},
      },
      "Ja"
   },
   {
      "np2kai_model",
      "PC Model (Restart)",
      NULL,
      {

         { "PC-286", NULL },
         { "PC-9801VM", NULL },
         { "PC-9801VX", NULL },
         { NULL, NULL},
      },
      "PC-9801VX"
   },
   {
      "np2kai_clk_base",
      "CPU Base Clock (Restart)",
      NULL,
      {
         { "1.9968 MHz", NULL },
         { "2.4576 MHz", NULL },
         { NULL, NULL},
      },
      "2.4576 MHz"
   },
   {
      "np2kai_cpu_feature",
      "CPU Feature (Restart)",
      NULL,
      {
         { "(custom)", NULL },
         { "Intel 80386", NULL },
         { "Intel i486SX", NULL },
         { "Intel i486DX", NULL },
         { "Intel Pentium", NULL },
         { "Intel MMX Pentium", NULL },
         { "Intel Pentium Pro", NULL },
         { "Intel Pentium II", NULL },
         { "Intel Pentium III", NULL },
         { "Intel Pentium M", NULL },
         { "Intel Pentium 4", NULL },
         { "AMD K6-2", NULL },
         { "AMD K6-III", NULL },
         { "AMD K7 Athlon", NULL },
         { "AMD K7 Athlon XP", NULL },
         { "Neko Processor II", NULL },
         { NULL, NULL},
      },
      "Intel 80386"
   },
   {
      "np2kai_clk_mult",
      "CPU Clock Multiplier (Restart)",
      "Higher values require a fast machine. Can make some games run too fast.",
      {
         { "2", NULL },
         { "4", NULL },
         { "5", NULL },
         { "6", NULL },
         { "8", NULL },
         { "10", NULL },
         { "12", NULL },
         { "16", NULL },
         { "20", NULL },
         { "24", NULL },
         { "30", NULL },
         { "36", NULL },
         { "40", NULL },
         { "42", NULL },
         { "52", NULL },
         { "64", NULL },
         { "76", NULL },
         { "88", NULL },
         { "100", NULL },
         { NULL, NULL},
      },
      "4"
   },
#if defined(SUPPORT_ASYNC_CPU)
   {
      "np2kai_async_cpu",
      "Async CPU(experimental) (Restart)",
      NULL,
      {
         { "OFF", NULL },
         { "ON", NULL },
         { NULL, NULL},
      },
      "OFF"
   },
#endif
   {
      "np2kai_ExMemory",
      "RAM Size (Restart)",
      "Amount of memory the virtual machine can use. Save states size will grow accordingly.",
      {
         { "1", NULL },
         { "3", NULL },
         { "7", NULL },
         { "11", NULL },
         { "13", NULL },
#if defined(CPUCORE_IA32)
         { "16", NULL },
         { "32", NULL },
         { "64", NULL },
         { "120", NULL },
         { "230", NULL },
         { "512", NULL },
         { "1024", NULL },
#endif
         { NULL, NULL},
      },
      "3"
   },
   {
      "np2kai_FastMC",
      "Fast memcheck",
      "Do a faster memory checking at startup.",
      {
         { "OFF", NULL },
         { "ON", NULL },
         { NULL, NULL},
      },
      "OFF"
   },
   {
      "np2kai_uselasthddmount",
      "Use last HDD mount",
      "Last HDD mount at core start.",
      {
         { "OFF", NULL },
         { "ON", NULL },
         { NULL, NULL},
      },
      "OFF"
   },
   {
      "np2kai_gdc",
      "GDC",
      "Graphic Display Controller model.",
      {
         { "uPD7220", NULL },
         { "uPD72020", NULL },
         { NULL, NULL},
      },
      "uPD7220"
   },
   {
      "np2kai_skipline",
      "Skipline Revisions",
      "'Off' will show black scanlines in older games.",
      {
         { "Full 255 lines", NULL },
         { "OFF", NULL },
         { "ON", NULL },
         { NULL, NULL},
      },
      "Full 255 lines"
   },
   {
      "np2kai_realpal",
      "Real Palettes",
      NULL,
      {
         { "OFF", NULL },
         { "ON", NULL },
         { NULL, NULL},
      },
      "OFF"
   },
   {
      "np2kai_lcd",
      "LCD",
      NULL,
      {
         { "OFF", NULL },
         { "ON", NULL },
         { NULL, NULL},
      },
      "OFF"
   },
   {
      "np2kai_SNDboard",
      "Sound Board (Restart)",
      NULL,
      {
         { "PC9801-14", NULL },
         { "PC9801-86", NULL },
         { "PC9801-86 + 118(B460)", NULL },
         { "PC9801-86 + Mate-X PCM(B460)", NULL },
         { "PC9801-86 + Chibi-oto", NULL },
         { "PC9801-86 + Speak Board", NULL },
         { "PC9801-26K", NULL },
         { "PC9801-26K + 86", NULL },
         { "PC9801-118", NULL },
         { "Mate-X PCM", NULL },
         { "Chibi-oto", NULL },
         { "Speak Board", NULL },
         { "Spark Board", NULL },
         { "Sound Orchestra", NULL },
         { "Sound Orchestra-V", NULL },
         { "Little Orchestra L", NULL },
         { "Multimedia Orchestra", NULL },
#if defined(SUPPORT_SOUND_SB16)
         { "Sound Blaster 16", NULL },
         { "PC9801-86 + Sound Blaster 16", NULL },
         { "Mate-X PCM + Sound Blaster 16", NULL },
         { "PC9801-118 + Sound Blaster 16", NULL },
         { "PC9801-86 + Mate-X PCM(B460) + Sound Blaster 16", NULL },
         { "PC9801-86 + 118(B460) + Sound Blaster 16", NULL },
#endif
         { "AMD-98", NULL },
         { "WaveStar", NULL },
#if defined(SUPPORT_PX)
         { "Otomi-chanx2", NULL },
         { "Otomi-chanx2 + 86", NULL },
#endif
         { "None", NULL },
         { NULL, NULL},
      },
      "PC9801-86"
   },
   {
      "np2kai_118ROM",
      "enable 118 ROM",
      NULL,
      {
         { "OFF", NULL },
         { "ON", NULL },
         { NULL, NULL},
      },
      "ON"
   },
   {
      "np2kai_jast_snd",
      "JastSound",
      "Enable Jast Sound PCM device.",
      {
         { "OFF", NULL },
         { "ON", NULL },
         { NULL, NULL},
      },
      "OFF"
   },
   {
      "np2kai_xroll",
      "Swap PageUp/PageDown",
      NULL,
      {
         { "OFF", NULL },
         { "ON", NULL },
         { NULL, NULL},
      },
      "ON"
   },
   {
      "np2kai_usefmgen",
      "Sound Generator",
      "Use 'fmgen' for enhanced sound rendering.",
      {
         { "Default", NULL },
         { "fmgen", NULL },
         { NULL, NULL},
      },
      "fmgen"
   },
   {
      "np2kai_volume_M",
      "Volume Master",
      NULL,
      {
         { "0", NULL },
         { "5", NULL },
         { "10", NULL },
         { "15", NULL },
         { "20", NULL },
         { "25", NULL },
         { "30", NULL },
         { "35", NULL },
         { "40", NULL },
         { "45", NULL },
         { "50", NULL },
         { "55", NULL },
         { "60", NULL },
         { "65", NULL },
         { "70", NULL },
         { "75", NULL },
         { "80", NULL },
         { "85", NULL },
         { "90", NULL },
         { "95", NULL },
         { "100", NULL },
         { NULL, NULL},
      },
      "100"
   },
   {
      "np2kai_volume_F",
      "Volume FM",
      NULL,
      {
         { "0", NULL },
         { "4", NULL },
         { "8", NULL },
         { "12", NULL },
         { "16", NULL },
         { "20", NULL },
         { "24", NULL },
         { "28", NULL },
         { "32", NULL },
         { "36", NULL },
         { "40", NULL },
         { "44", NULL },
         { "48", NULL },
         { "52", NULL },
         { "56", NULL },
         { "60", NULL },
         { "64", NULL },
         { "68", NULL },
         { "72", NULL },
         { "76", NULL },
         { "80", NULL },
         { "84", NULL },
         { "88", NULL },
         { "92", NULL },
         { "96", NULL },
         { "100", NULL },
         { "104", NULL },
         { "108", NULL },
         { "112", NULL },
         { "116", NULL },
         { "120", NULL },
         { "124", NULL },
         { "128", NULL },
         { NULL, NULL},
      },
      "64"
   },
   {
      "np2kai_volume_S",
      "Volume SSG",
      NULL,
      {
         { "0", NULL },
         { "4", NULL },
         { "8", NULL },
         { "12", NULL },
         { "16", NULL },
         { "20", NULL },
         { "24", NULL },
         { "28", NULL },
         { "32", NULL },
         { "36", NULL },
         { "40", NULL },
         { "44", NULL },
         { "48", NULL },
         { "52", NULL },
         { "56", NULL },
         { "60", NULL },
         { "64", NULL },
         { "68", NULL },
         { "72", NULL },
         { "76", NULL },
         { "80", NULL },
         { "84", NULL },
         { "88", NULL },
         { "92", NULL },
         { "96", NULL },
         { "100", NULL },
         { "104", NULL },
         { "108", NULL },
         { "112", NULL },
         { "116", NULL },
         { "120", NULL },
         { "124", NULL },
         { "128", NULL },
         { NULL, NULL},
      },
      "28"
   },
   {
      "np2kai_volume_A",
      "Volume ADPCM",
      NULL,
      {
         { "0", NULL },
         { "4", NULL },
         { "8", NULL },
         { "12", NULL },
         { "16", NULL },
         { "20", NULL },
         { "24", NULL },
         { "28", NULL },
         { "32", NULL },
         { "36", NULL },
         { "40", NULL },
         { "44", NULL },
         { "48", NULL },
         { "52", NULL },
         { "56", NULL },
         { "60", NULL },
         { "64", NULL },
         { "68", NULL },
         { "72", NULL },
         { "76", NULL },
         { "80", NULL },
         { "84", NULL },
         { "88", NULL },
         { "92", NULL },
         { "96", NULL },
         { "100", NULL },
         { "104", NULL },
         { "108", NULL },
         { "112", NULL },
         { "116", NULL },
         { "120", NULL },
         { "124", NULL },
         { "128", NULL },
         { NULL, NULL},
      },
      "64"
   },
   {
      "np2kai_volume_P",
      "Volume PCM",
      NULL,
      {
         { "0", NULL },
         { "4", NULL },
         { "8", NULL },
         { "12", NULL },
         { "16", NULL },
         { "20", NULL },
         { "24", NULL },
         { "28", NULL },
         { "32", NULL },
         { "36", NULL },
         { "40", NULL },
         { "44", NULL },
         { "48", NULL },
         { "52", NULL },
         { "56", NULL },
         { "60", NULL },
         { "64", NULL },
         { "68", NULL },
         { "72", NULL },
         { "76", NULL },
         { "80", NULL },
         { "84", NULL },
         { "88", NULL },
         { "92", NULL },
         { "96", NULL },
         { "100", NULL },
         { "104", NULL },
         { "108", NULL },
         { "112", NULL },
         { "116", NULL },
         { "120", NULL },
         { "124", NULL },
         { "128", NULL },
         { NULL, NULL},
      },
      "92"
   },
   {
      "np2kai_volume_R",
      "Volume RHYTHM",
      NULL,
      {
         { "0", NULL },
         { "4", NULL },
         { "8", NULL },
         { "12", NULL },
         { "16", NULL },
         { "20", NULL },
         { "24", NULL },
         { "28", NULL },
         { "32", NULL },
         { "36", NULL },
         { "40", NULL },
         { "44", NULL },
         { "48", NULL },
         { "52", NULL },
         { "56", NULL },
         { "60", NULL },
         { "64", NULL },
         { "68", NULL },
         { "72", NULL },
         { "76", NULL },
         { "80", NULL },
         { "84", NULL },
         { "88", NULL },
         { "92", NULL },
         { "96", NULL },
         { "100", NULL },
         { "104", NULL },
         { "108", NULL },
         { "112", NULL },
         { "116", NULL },
         { "120", NULL },
         { "124", NULL },
         { "128", NULL },
         { NULL, NULL},
      },
      "64"
   },
   {
      "np2kai_volume_C",
      "Volume CD-DA",
      NULL,
      {
         { "0", NULL },
         { "8", NULL },
         { "16", NULL },
         { "24", NULL },
         { "32", NULL },
         { "40", NULL },
         { "48", NULL },
         { "56", NULL },
         { "64", NULL },
         { "72", NULL },
         { "80", NULL },
         { "88", NULL },
         { "96", NULL },
         { "104", NULL },
         { "112", NULL },
         { "120", NULL },
         { "128", NULL },
         { "136", NULL },
         { "144", NULL },
         { "154", NULL },
         { "160", NULL },
         { "168", NULL },
         { "196", NULL },
         { "184", NULL },
         { "192", NULL },
         { "200", NULL },
         { "208", NULL },
         { "216", NULL },
         { "224", NULL },
         { "232", NULL },
         { "240", NULL },
         { "248", NULL },
         { "255", NULL },
         { NULL, NULL},
      },
      "128"
   },
   {
      "np2kai_Seek_Snd",
      "Floppy Seek Sound",
      NULL,
      {
         { "OFF", NULL },
         { "ON", NULL },
         { NULL, NULL},
      },
      "OFF"
   },
   {
      "np2kai_Seek_Vol",
      "Volume Floppy Seek",
      NULL,
      {
         { "0", NULL },
         { "4", NULL },
         { "8", NULL },
         { "12", NULL },
         { "16", NULL },
         { "20", NULL },
         { "24", NULL },
         { "28", NULL },
         { "32", NULL },
         { "36", NULL },
         { "40", NULL },
         { "44", NULL },
         { "48", NULL },
         { "52", NULL },
         { "56", NULL },
         { "60", NULL },
         { "64", NULL },
         { "68", NULL },
         { "72", NULL },
         { "76", NULL },
         { "80", NULL },
         { "84", NULL },
         { "88", NULL },
         { "92", NULL },
         { "96", NULL },
         { "100", NULL },
         { "104", NULL },
         { "108", NULL },
         { "112", NULL },
         { "116", NULL },
         { "120", NULL },
         { "124", NULL },
         { "128", NULL },
         { NULL, NULL},
      },
      ""
   },
   {
      "np2kai_BEEP_vol",
      "Volume Beep",
      NULL,
      {
         { "0", NULL },
         { "1", NULL },
         { "2", NULL },
         { "3", NULL },
         { NULL, NULL},
      },
      "3"
   },
#if defined(SUPPORT_WAB)
   {
      "np2kai_CLGD_en",
      "Enable WAB (Restart App)",
      NULL,
      {
         { "OFF", NULL },
         { "ON", NULL },
         { NULL, NULL},
      },
      "OFF"
   },
   {
      "np2kai_CLGD_type",
      "WAB Type",
      NULL,
      {
         { "PC-9821Xe10,Xa7e,Xb10 built-in", NULL },
         { "PC-9821Bp,Bs,Be,Bf built-in", NULL },
         { "PC-9821Xe built-in", NULL },
         { "PC-9821Cb built-in", NULL },
         { "PC-9821Cf built-in", NULL },
         { "PC-9821Cb2 built-in", NULL },
         { "PC-9821Cx2 built-in", NULL },
         { "PC-9821 PCI CL-GD5446 built-in", NULL },
         { "MELCO WAB-S", NULL },
         { "MELCO WSN-A2F", NULL },
         { "MELCO WSN-A4F", NULL },
         { "I-O DATA GA-98NBI/C", NULL },
         { "I-O DATA GA-98NBII", NULL },
         { "I-O DATA GA-98NBIV", NULL },
         { "PC-9801-96(PC-9801B3-E02)", NULL },
         { "Auto Select(Xe10, GA-98NBI/C), PCI", NULL },
         { "Auto Select(Xe10, GA-98NBII), PCI", NULL },
         { "Auto Select(Xe10, GA-98NBIV), PCI", NULL },
         { "Auto Select(Xe10, WAB-S), PCI", NULL },
         { "Auto Select(Xe10, WSN-A2F), PCI", NULL },
         { "Auto Select(Xe10, WSN-A4F), PCI", NULL },
         { "Auto Select(Xe10, WAB-S)", NULL },
         { "Auto Select(Xe10, WSN-A2F)", NULL },
         { "Auto Select(Xe10, WSN-A4F)", NULL },
         { NULL, NULL},
      },
      "PC-9821Xe10,Xa7e,Xb10 built-in"
   },
   {
      "np2kai_CLGD_fc",
      "Use Fake Hardware Cursor",
      NULL,
      {
         { "OFF", NULL },
         { "ON", NULL },
         { NULL, NULL},
      },
      "OFF"
   },
#endif	/* defined(SUPPORT_WAB) */
#if defined(SUPPORT_PEGC)
   {
      "np2kai_PEGC",
      "Enable PEGC plane mode",
      NULL,
      {
         { "OFF", NULL },
         { "ON", NULL },
         { NULL, NULL},
      },
      "ON"
   },
#endif
#if defined(SUPPORT_PCI)
   {
      "np2kai_PCI_en",
      "Enable PCI (Restart App)",
      NULL,
      {
         { "OFF", NULL },
         { "ON", NULL },
         { NULL, NULL},
      },
      "OFF"
   },
   {
      "np2kai_PCI_type",
      "PCMC Type",
      NULL,
      {
         { "Intel 82434LX", NULL },
         { "Intel 82441FX", NULL },
         { "VLSI Wildcat", NULL },
         { NULL, NULL},
      },
      "Intel 82434LX"
   },
   {
      "np2kai_PCI_bios32",
      "Use BIOS32 (not recommended)",
      NULL,
      {
         { "OFF", NULL },
         { "ON", NULL },
         { NULL, NULL},
      },
      "OFF"
   },
#endif	/* defined(SUPPORT_PCI) */
   {
      "np2kai_usecdecc",
      "Use CD-ROM EDC/ECC Emulation",
      NULL,
      {
         { "OFF", NULL },
         { "ON", NULL },
         { NULL, NULL},
      },
      "ON"
   },
  {
    "np2kai_stick2mouse",
    "S2M(Joypad Analog Stick to Mouse) Mapping",
    "Emulate a mouse on your gamepad's analog stick.",
    {
      {"OFF", NULL},
      {"L-stick", NULL},
      {"R-stick", NULL},
      {NULL, NULL},
    },
    "R-stick"
  },
  {
    "np2kai_stick2mouse_shift",
    "S2M Click Shift Button Mapping",
    "Stick push shift to left->right click.",
    {
      {"OFF", NULL},
      {"L1", NULL},
      {"L2", NULL},
      {"R1", NULL},
      {"R2", NULL},
      {NULL, NULL},
    },
    "R1"
  },
  {
    "np2kai_joymode",
    "Joypad D-pad to Mouse/Keyboard/Joypad Mapping",
    "Emulate a keyboard/mouse/joypad on your gamepad. Map keyboard 'Arrows' or 'Keypad' on the D-pad.",
    {
      {"OFF", NULL},
      {"Mouse", NULL},
      {"Arrows", NULL},
      {"Arrows 3button", NULL},
      {"Keypad", NULL},
      {"Keypad 3button", NULL},
      {"Manual Keyboard", NULL},
      {"Atari Joypad", NULL},
      {NULL, NULL},
    },
    "OFF"
  },
  {
    "np2kai_joynp2menu",
    "Joypad to NP2 menu Mapping",
    "Select a gamepad button to open NP2 Menu.",
    {
      {"OFF", NULL},
      {"L1", NULL},
      {"L2", NULL},
      {"L3", NULL},
      {"R1", NULL},
      {"R2", NULL},
      {"R3", NULL},
      {"A", NULL},
      {"B", NULL},
      {"X", NULL},
      {"Y", NULL},
      {"Start", NULL},
      {"Select", NULL},
      {NULL, NULL},
    },
    "L2"
  },
  {NULL, NULL, NULL, {{0}}, NULL},
};

/* RETRO_LANGUAGE_JAPANESE */

struct retro_core_option_definition option_defs_ja[] = {
   {
      "np2kai_drive",
      "ディスク入れ替えドライブ",
      NULL,
      {
         { "FDD1", NULL },
         { "FDD2", NULL },
         { NULL, NULL},
      },
      "FDD2"
   },
   {
      "np2kai_keyboard",
      "キーボード形式 (要リスタート)",
      "日本語106、もしくは英語101",
      {
         { "Ja", NULL },
         { "Us", NULL },
         { NULL, NULL},
      },
      "Ja"
   },
   {
      "np2kai_model",
      "基本アーキテクチャ (要リスタート)",
      NULL,
      {

         { "PC-286", NULL },
         { "PC-9801VM", NULL },
         { "PC-9801VX", NULL },
         { NULL, NULL},
      },
      "PC-9801VX"
   },
   {
      "np2kai_clk_base",
      "CPUベースクロック (要リスタート)",
      NULL,
      {
         { "1.9968 MHz", NULL },
         { "2.4576 MHz", NULL },
         { NULL, NULL},
      },
      "2.4576 MHz"
   },
   {
      "np2kai_cpu_feature",
      "CPU仕様 (要リスタート)",
      NULL,
      {
         { "(custom)", NULL },
         { "Intel 80386", NULL },
         { "Intel i486SX", NULL },
         { "Intel i486DX", NULL },
         { "Intel Pentium", NULL },
         { "Intel MMX Pentium", NULL },
         { "Intel Pentium Pro", NULL },
         { "Intel Pentium II", NULL },
         { "Intel Pentium III", NULL },
         { "Intel Pentium M", NULL },
         { "Intel Pentium 4", NULL },
         { "AMD K6-2", NULL },
         { "AMD K6-III", NULL },
         { "AMD K7 Athlon", NULL },
         { "AMD K7 Athlon XP", NULL },
         { "Neko Processor II", NULL },
         { NULL, NULL},
      },
      "Intel 80386"
   },
   {
      "np2kai_clk_mult",
      "CPUクロック倍率 (要リスタート)",
      "動作が遅い場合は値を増やしてください。ゲームによってはオーバースピードになる可能性があります。",
      {
         { "2", NULL },
         { "4", NULL },
         { "5", NULL },
         { "6", NULL },
         { "8", NULL },
         { "10", NULL },
         { "12", NULL },
         { "16", NULL },
         { "20", NULL },
         { "24", NULL },
         { "30", NULL },
         { "36", NULL },
         { "40", NULL },
         { "42", NULL },
         { "52", NULL },
         { "64", NULL },
         { "76", NULL },
         { "88", NULL },
         { "100", NULL },
         { NULL, NULL},
      },
      "4"
   },
#if defined(SUPPORT_ASYNC_CPU)
   {
      "np2kai_async_cpu",
      "非同期CPU(experimental) (要リスタート)",
      "動機を待たずにCPUを先行処理させます。（プチフリする）",
      {
         { "OFF", NULL },
         { "ON", NULL },
         { NULL, NULL},
      },
      "OFF"
   },
#endif
   {
      "np2kai_ExMemory",
      "メモリサイズ (要リスタート)",
      "大きなシステムには大きなメモリが必要ですが、ステートセーブのサイズが大きくなります。",
      {
         { "1", NULL },
         { "3", NULL },
         { "7", NULL },
         { "11", NULL },
         { "13", NULL },
#if defined(CPUCORE_IA32)
         { "16", NULL },
         { "32", NULL },
         { "64", NULL },
         { "120", NULL },
         { "230", NULL },
         { "512", NULL },
         { "1024", NULL },
#endif
         { NULL, NULL},
      },
      "3"
   },
   {
      "np2kai_FastMC",
      "高速メモリチェック",
      "スタートアップ時のメモリチェックを素早くします。",
      {
         { "OFF", NULL },
         { "ON", NULL },
         { NULL, NULL},
      },
      "OFF"
   },
   {
      "np2kai_uselasthddmount",
      "前回のHDDマウントを使用",
      "前回のHDDをコア開始時にマウントします。",
      {
         { "OFF", NULL },
         { "ON", NULL },
         { NULL, NULL},
      },
      "OFF"
   },
   {
      "np2kai_gdc",
      "GDCタイプ",
      "Graphic Display Controller。",
      {
         { "uPD7220", NULL },
         { "uPD72020", NULL },
         { NULL, NULL},
      },
      "uPD7220"
   },
   {
      "np2kai_skipline",
      "Skipline Revisions",
      "'Off' will show black scanlines in older games.",
      {
         { "Full 255 lines", NULL },
         { "OFF", NULL },
         { "ON", NULL },
         { NULL, NULL},
      },
      "Full 255 lines"
   },
   {
      "np2kai_realpal",
      "擬似多色パレット",
      NULL,
      {
         { "OFF", NULL },
         { "ON", NULL },
         { NULL, NULL},
      },
      "OFF"
   },
   {
      "np2kai_lcd",
      "液晶ディスプレイ",
      NULL,
      {
         { "OFF", NULL },
         { "ON", NULL },
         { NULL, NULL},
      },
      "OFF"
   },
   {
      "np2kai_SNDboard",
      "サウンドボード (要リスタート)",
      NULL,
      {
         { "PC9801-14", NULL },
         { "PC9801-86", NULL },
         { "PC9801-86 + 118(B460)", NULL },
         { "PC9801-86 + Mate-X PCM(B460)", NULL },
         { "PC9801-86 + Chibi-oto", NULL },
         { "PC9801-86 + Speak Board", NULL },
         { "PC9801-26K", NULL },
         { "PC9801-26K + 86", NULL },
         { "PC9801-118", NULL },
         { "Mate-X PCM", NULL },
         { "Chibi-oto", NULL },
         { "Speak Board", NULL },
         { "Spark Board", NULL },
         { "Sound Orchestra", NULL },
         { "Sound Orchestra-V", NULL },
         { "Little Orchestra L", NULL },
         { "Multimedia Orchestra", NULL },
#if defined(SUPPORT_SOUND_SB16)
         { "Sound Blaster 16", NULL },
         { "PC9801-86 + Sound Blaster 16", NULL },
         { "Mate-X PCM + Sound Blaster 16", NULL },
         { "PC9801-118 + Sound Blaster 16", NULL },
         { "PC9801-86 + Mate-X PCM(B460) + Sound Blaster 16", NULL },
         { "PC9801-86 + 118(B460) + Sound Blaster 16", NULL },
#endif
         { "AMD-98", NULL },
         { "WaveStar", NULL },
#if defined(SUPPORT_PX)
         { "Otomi-chanx2", NULL },
         { "Otomi-chanx2 + 86", NULL },
#endif
         { "None", NULL },
         { NULL, NULL},
      },
      "PC9801-86"
   },
   {
      "np2kai_118ROM",
      "118用サウンドROM",
      NULL,
      {
         { "OFF", NULL },
         { "ON", NULL },
         { NULL, NULL},
      },
      "ON"
   },
   {
      "np2kai_jast_snd",
      "JastSound",
      "Enable Jast Sound PCM device.",
      {
         { "OFF", NULL },
         { "ON", NULL },
         { NULL, NULL},
      },
      "OFF"
   },
   {
      "np2kai_xroll",
      "PageUp/PageDownキーを入れ替え",
      NULL,
      {
         { "OFF", NULL },
         { "ON", NULL },
         { NULL, NULL},
      },
      "ON"
   },
   {
      "np2kai_usefmgen",
      "音源ジェネレータ （要リスタート）",
      "Defaultは猫OPNA、fmgenはより正確な音質になります。",
      {
         { "Default", NULL },
         { "fmgen", NULL },
         { NULL, NULL},
      },
      "fmgen"
   },
   {
      "np2kai_volume_M",
      "ボリューム： マスター",
      NULL,
      {
         { "0", NULL },
         { "5", NULL },
         { "10", NULL },
         { "15", NULL },
         { "20", NULL },
         { "25", NULL },
         { "30", NULL },
         { "35", NULL },
         { "40", NULL },
         { "45", NULL },
         { "50", NULL },
         { "55", NULL },
         { "60", NULL },
         { "65", NULL },
         { "70", NULL },
         { "75", NULL },
         { "80", NULL },
         { "85", NULL },
         { "90", NULL },
         { "95", NULL },
         { "100", NULL },
         { NULL, NULL},
      },
      "100"
   },
   {
      "np2kai_volume_F",
      "ボリューム： FM",
      NULL,
      {
         { "0", NULL },
         { "4", NULL },
         { "8", NULL },
         { "12", NULL },
         { "16", NULL },
         { "20", NULL },
         { "24", NULL },
         { "28", NULL },
         { "32", NULL },
         { "36", NULL },
         { "40", NULL },
         { "44", NULL },
         { "48", NULL },
         { "52", NULL },
         { "56", NULL },
         { "60", NULL },
         { "64", NULL },
         { "68", NULL },
         { "72", NULL },
         { "76", NULL },
         { "80", NULL },
         { "84", NULL },
         { "88", NULL },
         { "92", NULL },
         { "96", NULL },
         { "100", NULL },
         { "104", NULL },
         { "108", NULL },
         { "112", NULL },
         { "116", NULL },
         { "120", NULL },
         { "124", NULL },
         { "128", NULL },
         { NULL, NULL},
      },
      "64"
   },
   {
      "np2kai_volume_S",
      "ボリューム： SSG",
      NULL,
      {
         { "0", NULL },
         { "4", NULL },
         { "8", NULL },
         { "12", NULL },
         { "16", NULL },
         { "20", NULL },
         { "24", NULL },
         { "28", NULL },
         { "32", NULL },
         { "36", NULL },
         { "40", NULL },
         { "44", NULL },
         { "48", NULL },
         { "52", NULL },
         { "56", NULL },
         { "60", NULL },
         { "64", NULL },
         { "68", NULL },
         { "72", NULL },
         { "76", NULL },
         { "80", NULL },
         { "84", NULL },
         { "88", NULL },
         { "92", NULL },
         { "96", NULL },
         { "100", NULL },
         { "104", NULL },
         { "108", NULL },
         { "112", NULL },
         { "116", NULL },
         { "120", NULL },
         { "124", NULL },
         { "128", NULL },
         { NULL, NULL},
      },
      "28"
   },
   {
      "np2kai_volume_A",
      "ボリューム： ADPCM",
      NULL,
      {
         { "0", NULL },
         { "4", NULL },
         { "8", NULL },
         { "12", NULL },
         { "16", NULL },
         { "20", NULL },
         { "24", NULL },
         { "28", NULL },
         { "32", NULL },
         { "36", NULL },
         { "40", NULL },
         { "44", NULL },
         { "48", NULL },
         { "52", NULL },
         { "56", NULL },
         { "60", NULL },
         { "64", NULL },
         { "68", NULL },
         { "72", NULL },
         { "76", NULL },
         { "80", NULL },
         { "84", NULL },
         { "88", NULL },
         { "92", NULL },
         { "96", NULL },
         { "100", NULL },
         { "104", NULL },
         { "108", NULL },
         { "112", NULL },
         { "116", NULL },
         { "120", NULL },
         { "124", NULL },
         { "128", NULL },
         { NULL, NULL},
      },
      "64"
   },
   {
      "np2kai_volume_P",
      "ボリューム： PCM",
      NULL,
      {
         { "0", NULL },
         { "4", NULL },
         { "8", NULL },
         { "12", NULL },
         { "16", NULL },
         { "20", NULL },
         { "24", NULL },
         { "28", NULL },
         { "32", NULL },
         { "36", NULL },
         { "40", NULL },
         { "44", NULL },
         { "48", NULL },
         { "52", NULL },
         { "56", NULL },
         { "60", NULL },
         { "64", NULL },
         { "68", NULL },
         { "72", NULL },
         { "76", NULL },
         { "80", NULL },
         { "84", NULL },
         { "88", NULL },
         { "92", NULL },
         { "96", NULL },
         { "100", NULL },
         { "104", NULL },
         { "108", NULL },
         { "112", NULL },
         { "116", NULL },
         { "120", NULL },
         { "124", NULL },
         { "128", NULL },
         { NULL, NULL},
      },
      "90"
   },
   {
      "np2kai_volume_R",
      "ボリューム： リズム音源",
      NULL,
      {
         { "0", NULL },
         { "4", NULL },
         { "8", NULL },
         { "12", NULL },
         { "16", NULL },
         { "20", NULL },
         { "24", NULL },
         { "28", NULL },
         { "32", NULL },
         { "36", NULL },
         { "40", NULL },
         { "44", NULL },
         { "48", NULL },
         { "52", NULL },
         { "56", NULL },
         { "60", NULL },
         { "64", NULL },
         { "68", NULL },
         { "72", NULL },
         { "76", NULL },
         { "80", NULL },
         { "84", NULL },
         { "88", NULL },
         { "92", NULL },
         { "96", NULL },
         { "100", NULL },
         { "104", NULL },
         { "108", NULL },
         { "112", NULL },
         { "116", NULL },
         { "120", NULL },
         { "124", NULL },
         { "128", NULL },
         { NULL, NULL},
      },
      "64"
   },
   {
      "np2kai_volume_C",
      "ボリューム： CD-DA",
      NULL,
      {
         { "0", NULL },
         { "8", NULL },
         { "16", NULL },
         { "24", NULL },
         { "32", NULL },
         { "40", NULL },
         { "48", NULL },
         { "56", NULL },
         { "64", NULL },
         { "72", NULL },
         { "80", NULL },
         { "88", NULL },
         { "96", NULL },
         { "104", NULL },
         { "112", NULL },
         { "120", NULL },
         { "128", NULL },
         { "136", NULL },
         { "144", NULL },
         { "154", NULL },
         { "160", NULL },
         { "168", NULL },
         { "196", NULL },
         { "184", NULL },
         { "192", NULL },
         { "200", NULL },
         { "208", NULL },
         { "216", NULL },
         { "224", NULL },
         { "232", NULL },
         { "240", NULL },
         { "248", NULL },
         { "255", NULL },
         { NULL, NULL},
      },
      "128"
   },
   {
      "np2kai_Seek_Snd",
      "フロッピーシーク音",
      NULL,
      {
         { "OFF", NULL },
         { "ON", NULL },
         { NULL, NULL},
      },
      "OFF"
   },
   {
      "np2kai_Seek_Vol",
      "ボリューム： フロッピーシーク音",
      NULL,
      {
         { "0", NULL },
         { "4", NULL },
         { "8", NULL },
         { "12", NULL },
         { "16", NULL },
         { "20", NULL },
         { "24", NULL },
         { "28", NULL },
         { "32", NULL },
         { "36", NULL },
         { "40", NULL },
         { "44", NULL },
         { "48", NULL },
         { "52", NULL },
         { "56", NULL },
         { "60", NULL },
         { "64", NULL },
         { "68", NULL },
         { "72", NULL },
         { "76", NULL },
         { "80", NULL },
         { "84", NULL },
         { "88", NULL },
         { "92", NULL },
         { "96", NULL },
         { "100", NULL },
         { "104", NULL },
         { "108", NULL },
         { "112", NULL },
         { "116", NULL },
         { "120", NULL },
         { "124", NULL },
         { "128", NULL },
         { NULL, NULL},
      },
      ""
   },
   {
      "np2kai_BEEP_vol",
      "ボリューム： BEEP",
      NULL,
      {
         { "0", NULL },
         { "1", NULL },
         { "2", NULL },
         { "3", NULL },
         { NULL, NULL},
      },
      "3"
   },
#if defined(SUPPORT_WAB)
   {
      "np2kai_CLGD_en",
      "WAB有効 (要リスタート)",
      NULL,
      {
         { "OFF", NULL },
         { "ON", NULL },
         { NULL, NULL},
      },
      "OFF"
   },
   {
      "np2kai_CLGD_type",
      "WABタイプ",
      NULL,
      {
         { "PC-9821Xe10,Xa7e,Xb10 built-in", NULL },
         { "PC-9821Bp,Bs,Be,Bf built-in", NULL },
         { "PC-9821Xe built-in", NULL },
         { "PC-9821Cb built-in", NULL },
         { "PC-9821Cf built-in", NULL },
         { "PC-9821Cb2 built-in", NULL },
         { "PC-9821Cx2 built-in", NULL },
         { "PC-9821 PCI CL-GD5446 built-in", NULL },
         { "MELCO WAB-S", NULL },
         { "MELCO WSN-A2F", NULL },
         { "MELCO WSN-A4F", NULL },
         { "I-O DATA GA-98NBI/C", NULL },
         { "I-O DATA GA-98NBII", NULL },
         { "I-O DATA GA-98NBIV", NULL },
         { "PC-9801-96(PC-9801B3-E02)", NULL },
         { "Auto Select(Xe10, GA-98NBI/C), PCI", NULL },
         { "Auto Select(Xe10, GA-98NBII), PCI", NULL },
         { "Auto Select(Xe10, GA-98NBIV), PCI", NULL },
         { "Auto Select(Xe10, WAB-S), PCI", NULL },
         { "Auto Select(Xe10, WSN-A2F), PCI", NULL },
         { "Auto Select(Xe10, WSN-A4F), PCI", NULL },
         { "Auto Select(Xe10, WAB-S)", NULL },
         { "Auto Select(Xe10, WSN-A2F)", NULL },
         { "Auto Select(Xe10, WSN-A4F)", NULL },
         { NULL, NULL},
      },
      "PC-9821Xe10,Xa7e,Xb10 built-in"
   },
   {
      "np2kai_CLGD_fc",
      "WAB時のフェイクカーソル表示",
      NULL,
      {
         { "OFF", NULL },
         { "ON", NULL },
         { NULL, NULL},
      },
      "OFF"
   },
#endif	/* defined(SUPPORT_WAB) */
#if defined(SUPPORT_PEGC)
   {
      "np2kai_PEGC",
      "PEGCプレーンモード",
      NULL,
      {
         { "OFF", NULL },
         { "ON", NULL },
         { NULL, NULL},
      },
      "ON"
   },
#endif
#if defined(SUPPORT_PCI)
   {
      "np2kai_PCI_en",
      "PCIバス有効 (要リスタート)",
      NULL,
      {
         { "OFF", NULL },
         { "ON", NULL },
         { NULL, NULL},
      },
      "OFF"
   },
   {
      "np2kai_PCI_type",
      "PCMCタイプ",
      NULL,
      {
         { "Intel 82434LX", NULL },
         { "Intel 82441FX", NULL },
         { "VLSI Wildcat", NULL },
         { NULL, NULL},
      },
      "Intel 82434LX"
   },
   {
      "np2kai_PCI_bios32",
      "BIOS32使用 (オススメしません)",
      NULL,
      {
         { "OFF", NULL },
         { "ON", NULL },
         { NULL, NULL},
      },
      "OFF"
   },
#endif	/* defined(SUPPORT_PCI) */
   {
      "np2kai_usecdecc",
      "CD-ROM EDC/ECCエミュレーション",
      NULL,
      {
         { "OFF", NULL },
         { "ON", NULL },
         { NULL, NULL},
      },
      "ON"
   },
  {
    "np2kai_stick2mouse",
    "S2M(ジョイパッド アナログスティック->マウス マッピング",
    "ジョイパッドのアナログスティックをマウスに割り当てる。",
    {
      {"OFF", NULL},
      {"L-stick", NULL},
      {"R-stick", NULL},
      {NULL, NULL},
    },
    "R-stick"
  },
  {
    "np2kai_stick2mouse_shift",
    "S2M クリックシフトボタン マッピング",
    "スティック押し込みを左→右クリックにシフトするボタンを割り当てる。",
    {
      {"OFF", NULL},
      {"L1", NULL},
      {"L2", NULL},
      {"R1", NULL},
      {"R2", NULL},
      {NULL, NULL},
    },
    "R1"
  },
  {
    "np2kai_joymode",
    "ジョイパッド デジタルボタン マッピング",
    "ジョイパッドのデジタルボタンをキーボード/マウス/ジョイパッドの操作に割り当てる。",
    {
      {"OFF", NULL},
      {"Mouse", NULL},
      {"Arrows", NULL},
      {"Arrows 3button", NULL},
      {"Keypad", NULL},
      {"Keypad 3button", NULL},
      {"Manual Keyboard", NULL},
      {"Atari Joypad", NULL},
      {NULL, NULL},
    },
    "OFF"
  },
  {
    "np2kai_joynp2menu",
    "NP2メニュー表示ボタン設定",
    NULL,
    {
      {"OFF", NULL},
      {"L1", NULL},
      {"L2", NULL},
      {"L3", NULL},
      {"R1", NULL},
      {"R2", NULL},
      {"R3", NULL},
      {"A", NULL},
      {"B", NULL},
      {"X", NULL},
      {"Y", NULL},
      {"Start", NULL},
      {"Select", NULL},
      {NULL, NULL},
    },
    "L2"
  },
  {NULL, NULL, NULL, {{0}}, NULL},
};

/* RETRO_LANGUAGE_FRENCH */

/* RETRO_LANGUAGE_SPANISH */

/* RETRO_LANGUAGE_GERMAN */

/* RETRO_LANGUAGE_ITALIAN */

/* RETRO_LANGUAGE_DUTCH */

/* RETRO_LANGUAGE_PORTUGUESE_BRAZIL */

/* RETRO_LANGUAGE_PORTUGUESE_PORTUGAL */

/* RETRO_LANGUAGE_RUSSIAN */

/* RETRO_LANGUAGE_KOREAN */

/* RETRO_LANGUAGE_CHINESE_TRADITIONAL */

/* RETRO_LANGUAGE_CHINESE_SIMPLIFIED */

/* RETRO_LANGUAGE_ESPERANTO */

/* RETRO_LANGUAGE_POLISH */

/* RETRO_LANGUAGE_VIETNAMESE */

/* RETRO_LANGUAGE_ARABIC */

/* RETRO_LANGUAGE_GREEK */

/* RETRO_LANGUAGE_TURKISH */

/*
 ********************************
 * Language Mapping
 ********************************
*/

struct retro_core_option_definition *option_defs_intl[RETRO_LANGUAGE_LAST] = {
   option_defs_us, /* RETRO_LANGUAGE_ENGLISH */
   option_defs_ja, /* RETRO_LANGUAGE_JAPANESE */
   NULL,           /* RETRO_LANGUAGE_FRENCH */
   NULL,           /* RETRO_LANGUAGE_SPANISH */
   NULL,           /* RETRO_LANGUAGE_GERMAN */
   NULL,           /* RETRO_LANGUAGE_ITALIAN */
   NULL,           /* RETRO_LANGUAGE_DUTCH */
   NULL,           /* RETRO_LANGUAGE_PORTUGUESE_BRAZIL */
   NULL,           /* RETRO_LANGUAGE_PORTUGUESE_PORTUGAL */
   NULL,           /* RETRO_LANGUAGE_RUSSIAN */
   NULL,           /* RETRO_LANGUAGE_KOREAN */
   NULL,           /* RETRO_LANGUAGE_CHINESE_TRADITIONAL */
   NULL,           /* RETRO_LANGUAGE_CHINESE_SIMPLIFIED */
   NULL,           /* RETRO_LANGUAGE_ESPERANTO */
   NULL,           /* RETRO_LANGUAGE_POLISH */
   NULL,           /* RETRO_LANGUAGE_VIETNAMESE */
   NULL,           /* RETRO_LANGUAGE_ARABIC */
   NULL,           /* RETRO_LANGUAGE_GREEK */
   NULL,           /* RETRO_LANGUAGE_TURKISH */
};

/*
 ********************************
 * Functions
 ********************************
*/

/* Handles configuration/setting of core options.
 * Should only be called inside retro_set_environment().
 * > We place the function body in the header to avoid the
 *   necessity of adding more .c files (i.e. want this to
 *   be as painless as possible for core devs)
 */

static INLINE void libretro_set_core_options(retro_environment_t environ_cb)
{
   unsigned version = 0;

   if (!environ_cb)
      return;

   if (environ_cb(RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION, &version) && (version == 1))
   {
      struct retro_core_options_intl core_options_intl;
      unsigned language = 0;

      core_options_intl.us    = option_defs_us;
      core_options_intl.local = NULL;

      if (environ_cb(RETRO_ENVIRONMENT_GET_LANGUAGE, &language) &&
          (language < RETRO_LANGUAGE_LAST) && (language != RETRO_LANGUAGE_ENGLISH))
         core_options_intl.local = option_defs_intl[language];

      environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_INTL, &core_options_intl);
   }
   else
   {
      size_t i;
      size_t option_index              = 0;
      size_t num_options               = 0;
      struct retro_variable *variables = NULL;
      char **values_buf                = NULL;

      /* Determine number of options
       * > Note: We are going to skip a number of irrelevant
       *   core options when building the retro_variable array,
       *   but we'll allocate space for all of them. The difference
       *   in resource usage is negligible, and this allows us to
       *   keep the code 'cleaner' */
      while (true)
      {
         if (option_defs_us[num_options].key)
            num_options++;
         else
            break;
      }

      /* Allocate arrays */
      variables  = (struct retro_variable *)calloc(num_options + 1, sizeof(struct retro_variable));
      values_buf = (char **)calloc(num_options, sizeof(char *));

      if (!variables || !values_buf)
         goto error;

      /* Copy parameters from option_defs_us array */
      for (i = 0; i < num_options; i++)
      {
         const char *key                        = option_defs_us[i].key;
         const char *desc                       = option_defs_us[i].desc;
         const char *default_value              = option_defs_us[i].default_value;
         struct retro_core_option_value *values = option_defs_us[i].values;
         size_t buf_len                         = 3;
         size_t default_index                   = 0;

         values_buf[i] = NULL;

         if (desc)
         {
            size_t num_values = 0;

            /* Determine number of values */
            while (true)
            {
               if (values[num_values].value)
               {
                  /* Check if this is the default value */
                  if (default_value)
                     if (strcmp(values[num_values].value, default_value) == 0)
                        default_index = num_values;

                  buf_len += strlen(values[num_values].value);
                  num_values++;
               }
               else
                  break;
            }

            /* Build values string */
            if (num_values > 1)
            {
               size_t j;

               buf_len += num_values - 1;
               buf_len += strlen(desc);

               values_buf[i] = (char *)calloc(buf_len, sizeof(char));
               if (!values_buf[i])
                  goto error;

               strcpy(values_buf[i], desc);
               strcat(values_buf[i], "; ");

               /* Default value goes first */
               strcat(values_buf[i], values[default_index].value);

               /* Add remaining values */
               for (j = 0; j < num_values; j++)
               {
                  if (j != default_index)
                  {
                     strcat(values_buf[i], "|");
                     strcat(values_buf[i], values[j].value);
                  }
               }
            }
         }

         variables[option_index].key   = key;
         variables[option_index].value = values_buf[i];
         option_index++;
      }
      
      /* Set variables */
      environ_cb(RETRO_ENVIRONMENT_SET_VARIABLES, variables);

error:

      /* Clean up */
      if (values_buf)
      {
         for (i = 0; i < num_options; i++)
         {
            if (values_buf[i])
            {
               free(values_buf[i]);
               values_buf[i] = NULL;
            }
         }

         free(values_buf);
         values_buf = NULL;
      }

      if (variables)
      {
         free(variables);
         variables = NULL;
      }
   }
}

#ifdef __cplusplus
}
#endif

#endif
