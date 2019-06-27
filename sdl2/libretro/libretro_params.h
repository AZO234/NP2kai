#ifndef LRPARAMS_
#define LRPARAMS_

#include "np2ver.h"

#define LR_SCREENWIDTH  1640
#define LR_SCREENHEIGHT 1024
#define LR_SCREENASPECT 4.0 / 3.0
#define LR_SCREENFPS    56.4

#define LR_SOUNDRATE    44100.0
//#define SNDSZ 735 //44100Hz/60fps=735 (sample/flame)
#define SNDSZ 782 //44100Hz/56.4fps=781.9 (sample/flame)

#define LR_CORENAME        "Neko Project II kai"
#define LR_LIBVERSION      NP2VER_CORE " " NP2VER_GIT
#define LR_VALIDFILEEXT    "d88|88d|d98|98d|fdi|xdf|hdm|dup|2hd|tfd|nfd|hd4|hd5|hd9|fdd|h01|hdb|ddb|dd6|dcp|dcu|flp|img|ima|bin|fim|thd|nhd|hdi|vhd|slh|hdn|m3u|cmd"
#define LR_NEEDFILEPATH    true
#define LR_BLOCKARCEXTRACT false
#define LR_REQUIRESROM     false

#endif
