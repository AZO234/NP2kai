#ifndef LREXPORTS_
#define LREXPORTS_

#include <stdint.h>
#include "libretro.h"

extern uint16_t   FrameBuffer[];
extern retro_audio_sample_batch_t audio_batch_cb;
extern retro_log_printf_t log_cb;
#endif
