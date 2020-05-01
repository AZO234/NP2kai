#include <stdio.h>
#include <stdint.h>
#ifndef _MSC_VER
#include <stdbool.h>
#include <unistd.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <math.h>
#include <ctype.h>

#include "libretro.h"
#include "libretro_params.h"
#include "libretro_core_options.h"
#include "file_stream.h"

#include "compiler.h"//required to prevent missing type errors
#include "beep.h"
#include "cpucore.h"
#include "pccore.h"
#include "iocore.h"
#include "keystat.h"
#include "fddfile.h"
#include "newdisk.h"
#include "diskdrv.h"
#include "fontmng.h"
#include "kbdmng.h"
#include "ini.h"
#include "scrnmng.h"
#include "soundmng.h"
#include "sysmng.h"
#include "joymng.h"
#include "mousemng.h"
#include "font.h"
#include "kbtrans.h"
#include "vramhdl.h"
#include "menubase.h"
#include "palettes.h"
#include "sysmenu.h"
#include "scrndraw.h"
#include "milstr.h"
#include "strres.h"
#include "np2.h"
#include "fmboard.h"
#include "dosio.h"
#include "gdc.h"
#if defined(SUPPORT_FMGEN)
#include "fmgen_fmgwrap.h"
#endif	/* defined(SUPPORT_FMGEN) */
#if defined(SUPPORT_WAB)
#include "wab.h"
#include "cirrus_vga_extern.h"
#endif	/* defined(SUPPORT_WAB) */

extern void sdlaudio_callback(void *userdata, unsigned char *stream, int len);

signed short soundbuf[SNDSZ*2]; //16bit*2ch

char RPATH[512];
OEMCHAR tmppath[MAX_PATH];

static retro_log_printf_t log_cb = NULL;
static retro_video_refresh_t video_cb = NULL;
static retro_input_poll_t poll_cb = NULL;
retro_input_state_t input_cb = NULL;
retro_environment_t environ_cb = NULL;
extern struct retro_midi_interface *retro_midi_interface;

uint32_t   FrameBuffer[LR_SCREENWIDTH * LR_SCREENHEIGHT];

retro_audio_sample_batch_t audio_batch_cb = NULL;

static char CMDFILE[1024];

bool did_reset, joy2key;
int lr_init = 0;

char lr_game_base_dir[MAX_PATH];

int lr_uselasthddmount;

#ifdef _WIN32
static char slash = '\\';
#else
static char slash = '/';
#endif

static void update_variables(void);

/* media swap support */
struct retro_disk_control_callback dskcb;
extern char np2_main_disk_images_paths[50][MAX_PATH];
extern unsigned int np2_main_disk_images_count;
static unsigned drvno = 1;
static unsigned disk_index = 0;
static bool disk_inserted = false;
static unsigned int lastidx = 0;

//all the fake functions used to limit swapping to 1 disk drive
bool setdskeject(bool ejected){
   disk_inserted = !ejected;
   return true;
}

bool getdskeject(){
   return !disk_inserted;
}

unsigned getdskindex(){
   return disk_index;
}

bool setdskindex(unsigned index){
   disk_index = index;
   if(disk_index == np2_main_disk_images_count)
   {
      //retroarch is trying to set "no disk in tray"
      return true;
   }

   update_variables();
   strcpy(np2cfg.fddfile[drvno], np2_main_disk_images_paths[disk_index]);
   diskdrv_setfdd(drvno, np2_main_disk_images_paths[disk_index], 0);
   return true;
}

unsigned getnumimages(){
   return np2_main_disk_images_count;
}

bool addimageindex() {
   if (np2_main_disk_images_count >= 50)
      return false;

   np2_main_disk_images_count++;
   return true;
}

bool replacedsk(unsigned index,const struct retro_game_info *info){
   strcpy(np2_main_disk_images_paths[index], info->path);
   return true;
}

void attach_disk_swap_interface(){
   //these functions are unused
   dskcb.set_eject_state = setdskeject;
   dskcb.get_eject_state = getdskeject;
   dskcb.set_image_index = setdskindex;
   dskcb.get_image_index = getdskindex;
   dskcb.get_num_images  = getnumimages;
   dskcb.add_image_index = addimageindex;
   dskcb.replace_image_index = replacedsk;
   if(np2_main_disk_images_count) {
      disk_inserted = true;
   }

   environ_cb(RETRO_ENVIRONMENT_SET_DISK_CONTROL_INTERFACE,&dskcb);
}

void setnxtdskindex(void){
	if(np2_main_disk_images_count > 2) {
		if(disk_index + 1 != np2_main_disk_images_count) {
			setdskindex(disk_index + 1);
		} else {
			setdskindex(0);
		}
	}
}

void setpredskindex(void){
	if(np2_main_disk_images_count > 2) {
		if(disk_index == 0) {
			setdskindex(np2_main_disk_images_count - 1);
		} else {
			setdskindex(disk_index - 1);
		}
	}
}
/* end media swap support */

int loadcmdfile(char *argv)
{
  int res = 0;

  RFILE* fp = filestream_open(argv, RETRO_VFS_FILE_ACCESS_READ, RETRO_VFS_FILE_ACCESS_HINT_NONE);
  if(fp != NULL) {
    if(filestream_gets(fp, CMDFILE, 1024) != NULL) {
      res = 1;
    }
    filestream_close(fp);
  }

  return res;
}

int HandleExtension(char *path,char *ext)
{
   int len = strlen(path);

   if (len >= 4 &&
         path[len-4] == '.' &&
         path[len-3] == ext[0] &&
         path[len-2] == ext[1] &&
         path[len-1] == ext[2])
   {
      return 1;
   }

   return 0;
}
//Args for experimental_cmdline
static char ARGUV[64][1024];
static unsigned char ARGUC=0;

// Args for Core
static char XARGV[64][1024];
static const char* xargv_cmd[64];
int PARAMCOUNT=0;

extern int cmain(int argc, char *argv[]);

void parse_cmdline(const char *argv);

static void extract_directory(char *buf, const char *path, size_t size)
{
   char *base = NULL;

   strncpy(buf, path, size - 1);
   buf[size - 1] = '\0';

   base = strrchr(buf, '/');
   if (!base)
      base = strrchr(buf, '\\');

   if (base)
      *base = '\0';
   else
      buf[0] = '\0';
}

void Add_Option(const char* option)
{
   static int first=0;

   if(first==0)
   {
      PARAMCOUNT=0;
      first++;
   }

   sprintf(XARGV[PARAMCOUNT++],"%s",option);
}

int pre_main(const char *argv) {
  int i;

  CMDFILE[0] = '\0';
  if(strlen(argv) > strlen("cmd")) {
    if(HandleExtension((char*)argv, "cmd") || HandleExtension((char*)argv, "CMD")) {
      i = loadcmdfile((char*)argv);
    }
  }

  if(CMDFILE[0] == '\0') {
    milstr_ncpy(CMDFILE, "np2kai \"", 1024);
    milstr_ncat(CMDFILE, argv, 1024);
    milstr_ncat(CMDFILE, "\"", 1024);
  }
  parse_cmdline(CMDFILE);

   for (i = 0; i<64; i++)
      xargv_cmd[i] = NULL;

   for (i = 0; i < ARGUC; i++)
      Add_Option(ARGUV[i]);

   for (i = 0; i < PARAMCOUNT; i++)
   {
      xargv_cmd[i] = (char*)(XARGV[i]);
      printf("arg_%d:%s\n",i,xargv_cmd[i]);
      printf("argl_%d:%d\n",i,strlen(xargv_cmd[i]));
   }

   dosio_init();

   i=np2_main(PARAMCOUNT,( char **)xargv_cmd);

   xargv_cmd[PARAMCOUNT - 2] = NULL;

   return 0;
}

void parse_cmdline(const char *argv) {
  char *p, *p2, *start_of_word;
  int c, c2;
  static char buffer[512 * 4];
  enum states { DULL, IN_WORD, IN_STRING } state = DULL;

  strcpy(buffer, argv);
  strcat(buffer," \0");

  for(p = buffer; *p != '\0'; p++) {
    c = (unsigned char)*p; /* convert to unsigned char for is* functions */
    switch(state) {
    case DULL: /* not in a word, not in a double quoted string */
      if(c == ' ' || c == '\t' ||  c == '\r' ||  c == '\n') { /* still not in a word, so ignore this char */
        continue;
      }
      /* not a space -- if it's a double quote we go to IN_STRING, else to IN_WORD */
      if(c == '"') {
        state = IN_STRING;
        start_of_word = p + 1; /* word starts at *next* char, not this one */
        continue;
      }
      state = IN_WORD;
      start_of_word = p; /* word starts here */
      continue;

    case IN_STRING:
      /* we're in a double quoted string, so keep going until we hit a close " */
      if(c == '"') {
        /* word goes from start_of_word to p-1 */
        for(c2 = 0, p2 = start_of_word; p2 < p; p2++, c2++) {
          ARGUV[ARGUC][c2] = (unsigned char)*p2;
        }
        ARGUC++;
        state = DULL; /* back to "not in word, not in string" state */
      }
      continue; /* either still IN_STRING or we handled the end above */

    case IN_WORD:
      /* we're in a word, so keep going until we get to a space */
      if(c == ' ' || c == '\t' ||  c == '\r' ||  c == '\n') {
        /* word goes from start_of_word to p-1 */
        for(c2 = 0, p2 = start_of_word; p2 < p; p2++, c2++) {
          ARGUV[ARGUC][c2] = (unsigned char)*p2;
        }
        ARGUC++;
        state = DULL; /* back to "not in word, not in string" state */
      }
      continue; /* either still IN_WORD or we handled the end above */
    }
  }
}

static const char *cross[] = {
  "X                               ",
  "XX                              ",
  "X.X                             ",
  "X..X                            ",
  "X...X                           ",
  "X....X                          ",
  "X.....X                         ",
  "X......X                        ",
  "X.......X                       ",
  "X........X                      ",
  "X.....XXXXX                     ",
  "X..X..X                         ",
  "X.X X..X                        ",
  "XX  X..X                        ",
  "X    X..X                       ",
  "     X..X                       ",
  "      X..X                      ",
  "      X..X                      ",
  "       XX                       ",
  "                                ",
};

void DrawPointBmp(unsigned int *buffer,int x, int y, unsigned int color)
{
   int idx;
   int w, h;

   scrnmng_getsize(&w, &h);

   if(x>=0&&y>=0&&x<w&&y<h) {
      idx=x+y*w;
      if(draw32bit) {
         buffer[idx]=color;
      } else {
         ((unsigned short*)buffer)[idx]=(unsigned short)(color & 0xFFFF);
      }
   }
}


void draw_cross(int x,int y) {

	int i,j,idx;
	int dx=32,dy=20;

	for(j=y;j<y+dy;j++){
		idx=0;
		for(i=x;i<x+dx;i++){
			if(cross[j-y][idx]=='.')DrawPointBmp(FrameBuffer,i,j,0xffffff);
			else if(cross[j-y][idx]=='X')DrawPointBmp(FrameBuffer,i,j,0);
			idx++;
		}
	}

}

static enum {
  LR_NP2KAI_JOYMODE_NONE = 0,
  LR_NP2KAI_JOYMODE_KEY,
  LR_NP2KAI_JOYMODE_MOUSE,
  LR_NP2KAI_JOYMODE_ATARI,
  LR_NP2KAI_JOYMODE_SB
};
static int m_tJoyMode;
static int m_tJoyModeInt;
static BOOL m_bJoyModeChange;

static int lastx = 320, lasty = 240;
static int menukey = 0;
static int menu_active = 0;
static double j2m_axel = 1.0;
static int j2m_movebtn = 0;
static int j2m_l_down = 0, j2m_r_down = 0;
static BOOL joyNP2menu;
static int joyNP2menubtn;
static int s2m;
static int s2m_no;
static uint8_t s2m_shift;
static BOOL abKeyStat[0x200];

static int j2k_pad[12] = { 
   RETRO_DEVICE_ID_JOYPAD_UP,
   RETRO_DEVICE_ID_JOYPAD_DOWN,
   RETRO_DEVICE_ID_JOYPAD_LEFT,
   RETRO_DEVICE_ID_JOYPAD_RIGHT,
   RETRO_DEVICE_ID_JOYPAD_A,
   RETRO_DEVICE_ID_JOYPAD_B,
   RETRO_DEVICE_ID_JOYPAD_X,
   RETRO_DEVICE_ID_JOYPAD_Y,
   RETRO_DEVICE_ID_JOYPAD_L,
   RETRO_DEVICE_ID_JOYPAD_R,
   RETRO_DEVICE_ID_JOYPAD_SELECT,
   RETRO_DEVICE_ID_JOYPAD_START
};
static uint16_t j2k_key[12];
static uint16_t j2k_key_arrow[12] = { 
   RETROK_UP,
   RETROK_DOWN,
   RETROK_LEFT,
   RETROK_RIGHT,
   RETROK_x,
   RETROK_z,
   RETROK_SPACE,
   RETROK_LCTRL,
   RETROK_BACKSPACE,
   RETROK_RSHIFT,
   RETROK_ESCAPE,
   RETROK_RETURN
};
static uint16_t j2k_key_arrow3[12] = { 
   RETROK_UP,
   RETROK_DOWN,
   RETROK_LEFT,
   RETROK_RIGHT,
   RETROK_c,
   RETROK_x,
   RETROK_SPACE,
   RETROK_z,
   RETROK_BACKSPACE,
   RETROK_RSHIFT,
   RETROK_ESCAPE,
   RETROK_RETURN
};
static uint16_t j2k_key_kpad[12] = { 
   RETROK_KP8,
   RETROK_KP2,
   RETROK_KP4,
   RETROK_KP6,
   RETROK_x,
   RETROK_z,
   RETROK_SPACE,
   RETROK_LCTRL,
   RETROK_BACKSPACE,
   RETROK_RSHIFT,
   RETROK_ESCAPE,
   RETROK_RETURN
};
static uint16_t j2k_key_kpad3[12] = { 
   RETROK_KP8,
   RETROK_KP2,
   RETROK_KP4,
   RETROK_KP6,
   RETROK_c,
   RETROK_x,
   RETROK_SPACE,
   RETROK_z,
   RETROK_BACKSPACE,
   RETROK_RSHIFT,
   RETROK_ESCAPE,
   RETROK_RETURN
};

void resetInput(void) {
  int i;

  menukey = 0;
  j2m_l_down = 0;
  j2m_r_down = 0;
  for(i = 0; i < keys_needed; i++) {
    abKeyStat[i] = FALSE;
  }
  joymng_sync();
	reset_lrkey();
  keystat_allrelease();
}

void updateInput(){
  static int mposx = 320, mposy = 240;
  int w, h;
  uint32_t i;

  if(m_bJoyModeChange) {
    resetInput();
  }
  m_bJoyModeChange = FALSE;

  scrnmng_getsize(&w, &h);

  poll_cb();

  // --- input ATARI joypad
  if(m_tJoyMode == LR_NP2KAI_JOYMODE_ATARI) {
    joymng_sync();
  }

  // --- input NP2 menu
  int menu_key_f12 = input_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_F12);
  int menu_mouse_m = input_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_MIDDLE);
  int menu_joy_menu = input_cb(0, RETRO_DEVICE_JOYPAD, 0, joyNP2menubtn);

  if((menu_key_f12 || menu_joy_menu || menu_mouse_m) && menukey == 0) {
    menukey = 1;
    if(menuvram == NULL) {
      sysmenu_menuopen(0, 0, 0);
      mposx=0;mposy = 0;
      lastx=0;lasty = 0;
      menu_active = 1;
    } else {
      menubase_close();
      scrndraw_redraw();
      menu_active = 0;
    }
  } else if(!(menu_key_f12 || menu_joy_menu || menu_mouse_m) && menukey == 1) {
    menukey = 0;
  }

  // --- input key
  int input;

  // Joy2Key
  if(m_tJoyMode == LR_NP2KAI_JOYMODE_KEY) {
    for(i = 0; i < 12; i++) {
      input = input_cb(0, RETRO_DEVICE_JOYPAD, 0, j2k_pad[i]);
      if(input && !abKeyStat[j2k_key[i]]) {
        send_libretro_key_down(j2k_key[i]);
        abKeyStat[j2k_key[i]] = TRUE;
      } else if(!input && abKeyStat[j2k_key[i]]) {
        send_libretro_key_up(j2k_key[i]);
        abKeyStat[j2k_key[i]] = FALSE;
      }
    }
  // keyboard
  } else {
    for(i = 0; i < keys_needed; i++) {
      input = input_cb(0, RETRO_DEVICE_KEYBOARD, 0, keys_poll[i].lrkey);
      if(input && !abKeyStat[keys_poll[i].lrkey]) {
        send_libretro_key_down(keys_poll[i].lrkey);
        abKeyStat[keys_poll[i].lrkey] = TRUE;
      } else if(!input && abKeyStat[keys_poll[i].lrkey]) {
        send_libretro_key_up(keys_poll[i].lrkey);
        abKeyStat[keys_poll[i].lrkey] = FALSE;
      }
    }
  }

  // --- move mouse

  // mouse
  int mouse_x_device = input_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X);
  int mouse_y_device = input_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_Y);

  if(menuvram == NULL) {
    mousemng_sync(mouse_x_device, mouse_y_device);
  } else {
    mposx += mouse_x_device; if(mposx < 0) mposx = 0; if(mposx >= w) mposx = w - 1;
    mposy += mouse_y_device; if(mposy < 0) mposy = 0; if(mposy >= h) mposy = h - 1;
    if(lastx != mposx || lasty != mposy)
      menubase_moving(mposx, mposy, 0);
  }

  // Joy2Mouse
  if(m_tJoyMode == LR_NP2KAI_JOYMODE_MOUSE) {
    int j2m_move_x, j2m_move_y;
    int j2m_u, j2m_d, j2m_l, j2m_r;

    j2m_move_x = j2m_move_y = 0;

    j2m_u = input_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP);
    j2m_d = input_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN);
    j2m_l = input_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT);
    j2m_r = input_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT);

    if((j2m_u || j2m_d || j2m_l || j2m_r) && j2m_movebtn == 0) {
      j2m_movebtn = 1;
      j2m_axel = 1.0;
    } else if(!(j2m_u || j2m_d || j2m_l || j2m_r) && j2m_movebtn == 1) {
      j2m_movebtn = 0;
    }
    j2m_axel += 0.1;

    if(j2m_movebtn) {
      if(j2m_u) {
        if(j2m_l) {
          j2m_move_x = 1.0 * -j2m_axel;
          j2m_move_y = 1.0 * -j2m_axel;
        } else if(j2m_r) {
          j2m_move_x = 1.0 * j2m_axel;
          j2m_move_y = 1.0 * -j2m_axel;
        } else {
          j2m_move_y = 1.0 * -j2m_axel / 1.414;
        }
      } else if(j2m_d) {
        if(j2m_l) {
          j2m_move_x = 1.0 * -j2m_axel;
          j2m_move_y = 1.0 * j2m_axel;
        } else if(j2m_r) {
          j2m_move_x = 1.0 * j2m_axel;
          j2m_move_y = 1.0 * j2m_axel;
        } else {
          j2m_move_y = 1.0 * j2m_axel / 1.414;
        }
      } else {
        if(j2m_l)
          j2m_move_x = 1.0 * -j2m_axel / 1.414;
        else if(j2m_r)
          j2m_move_x = 1.0 * j2m_axel / 1.414;
      }
    }

    if(menuvram == NULL) {
      mousemng_sync(j2m_move_x, j2m_move_y);
    } else {
      mposx += j2m_move_x;
      if(mposx < 0)
        mposx = 0;
      if(mposx >= w)
        mposx = w - 1;
      mposy += j2m_move_y;
      if(mposy < 0)
        mposy = 0;
      if(mposy >= h)
        mposy = h - 1;
      if(lastx != mposx || lasty != mposy)
        if(menuvram != NULL)
          menubase_moving(mposx, mposy, 0);
    }
  }

  // Stick2Mouse
  if(s2m) {
    int use_stick;
    int16_t analog_x, analog_y;
    int mouse_move_x, mouse_move_y;

    if(s2m_no) {
      use_stick = RETRO_DEVICE_INDEX_ANALOG_RIGHT;
    } else {
      use_stick = RETRO_DEVICE_INDEX_ANALOG_LEFT;
    }

    analog_x = input_cb(0, RETRO_DEVICE_ANALOG, use_stick, RETRO_DEVICE_ID_ANALOG_X);
    analog_y = input_cb(0, RETRO_DEVICE_ANALOG, use_stick, RETRO_DEVICE_ID_ANALOG_Y);

    mouse_move_x = (int)(analog_x * ((float)10 / 0x10000));
    mouse_move_y = (int)(analog_y * ((float)10 / 0x10000));

    if(menuvram == NULL) {
      mousemng_sync(mouse_move_x, mouse_move_y);
    } else {
      mposx += mouse_move_x;
      if(mposx < 0)
        mposx = 0;
      if(mposx >= w)
        mposx = w - 1;
      mposy += mouse_move_y;
      if(mposy < 0)
        mposy = 0;
      if(mposy >= h)
        mposy = h - 1;
      if(lastx != mposx || lasty != mposy)
        menubase_moving(mposx, mposy, 0);
    }
  }

  lastx = mposx; lasty = mposy;

  // --- input mouse button

  // mouse
  int mouse_l_device = input_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_LEFT);
  int mouse_r_device = input_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_RIGHT);
  int mouse_l, mouse_r;

  mouse_l = mouse_r = 0;

  if(mouse_l_device) {
    mouse_l = 1;
  }
  if(mouse_r_device) {
    mouse_r = 1;
  }

  // joy2mouse
  if(m_tJoyMode == LR_NP2KAI_JOYMODE_MOUSE) {
      int j2m_a = input_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A);
      int j2m_b = input_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B);
      if(j2m_a) {
        mouse_r = 1;
      }
      if(j2m_b) {
        mouse_l = 1;
      }
  }

  // Stick2Mouse
  if(s2m) {
    int use_thumb;
    int16_t analog_thumb;
    int shift_btn = 0;

    if(s2m_no) {
      use_thumb = RETRO_DEVICE_ID_JOYPAD_R3;
    } else {
      use_thumb = RETRO_DEVICE_ID_JOYPAD_L3;
    }

    analog_thumb = input_cb(0, RETRO_DEVICE_JOYPAD, 0, use_thumb);
    if(s2m_shift != 0xFF) {
      shift_btn = input_cb(0, RETRO_DEVICE_JOYPAD, 0, s2m_shift);
    }

    if(analog_thumb && !shift_btn) {
      mouse_l = 1;
    } else if(analog_thumb && shift_btn) {
      mouse_r = 1;
    }
  }

  if(j2m_l_down == 0 && mouse_l) {
    j2m_l_down = 1;
    if(menuvram == NULL) {
      mousemng_buttonevent(MOUSEMNG_LEFTDOWN);
    } else {
      menubase_moving(mposx, mposy, 1);
      scrndraw_redraw();
    }
  } else if(j2m_l_down == 1 && !mouse_l) {
    j2m_l_down = 0;
    if(menuvram == NULL) {
      mousemng_buttonevent(MOUSEMNG_LEFTUP);
    } else {
      menubase_moving(mposx, mposy, 2);
      scrndraw_redraw();
    }
  }
  if(j2m_r_down == 0 && mouse_r) {
    j2m_r_down = 1;
    if(menuvram == NULL) {
      mousemng_buttonevent(MOUSEMNG_RIGHTDOWN);
      scrndraw_redraw();
    }
  } else if(j2m_r_down == 1 && !mouse_r) {
    j2m_r_down= 0;
    if(menuvram == NULL) {
      mousemng_buttonevent(MOUSEMNG_RIGHTUP);
      scrndraw_redraw();
    }
  }
}

void *retro_get_memory_data(unsigned type)
{
   if ( type == RETRO_MEMORY_SYSTEM_RAM )
      return CPU_EXTMEM;
   return NULL;
}
size_t retro_get_memory_size(unsigned type)
{
   if ( type == RETRO_MEMORY_SYSTEM_RAM )
      return CPU_EXTMEMSIZE;
   return 0;
}

//dummy functions
void retro_set_audio_sample(retro_audio_sample_t cb){}
bool retro_load_game_special(unsigned game_type, const struct retro_game_info *info, size_t num_info){return false;}
void retro_unload_game (void){}

void retro_set_video_refresh(retro_video_refresh_t cb)
{
   video_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb)
{
   poll_cb = cb;
}

void retro_set_input_state(retro_input_state_t cb)
{
   input_cb = cb;
}

void lowerstring(char* str)
{
   int i;
   for (i=0; str[i]; i++)
   {
      str[i] = tolower(str[i]);
   }
}

void retro_set_environment(retro_environment_t cb)
{
   struct retro_log_callback logging;
   BOOL allow_no_game = true;

   environ_cb = cb;

   //bool no_rom = !LR_REQUIRESROM;
   //environ_cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_rom);

   environ_cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &allow_no_game);

   if (environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logging))
      log_cb = logging.log;
   else
      log_cb = NULL;

   libretro_set_core_options(environ_cb);
}

static void update_variables(void)
{
   struct retro_variable var = {0};

   var.key = "np2kai_drive";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "FDD1") == 0)
         drvno = 0;
      else if (strcmp(var.value, "FDD2") == 0)
         drvno = 1;
   }

   var.key = "np2kai_keyboard";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "Us") == 0)
         np2oscfg.KEYBOARD = KEY_KEY101;
      else
         np2oscfg.KEYBOARD = KEY_KEY106;
      init_lrkey_to_pc98();
   }

   var.key = "np2kai_model";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "PC-9801VX") == 0)
         milstr_ncpy(np2cfg.model, str_VX, 3);
      else if (strcmp(var.value, "PC-286") == 0)
         milstr_ncpy(np2cfg.model, str_EPSON, 6);
      else if (strcmp(var.value, "PC-9801VM") == 0)
         milstr_ncpy(np2cfg.model, str_VM, 3);
   }

   var.key = "np2kai_clk_base";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "1.9968 MHz") == 0)
         np2cfg.baseclock = 1996800;
      else
         np2cfg.baseclock = 2457600;
   }

   var.key = "np2kai_cpu_feature";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      UINT cpu_index;

      if (strcmp(var.value, "Intel 80386") == 0)
         cpu_index = 1;
      else if (strcmp(var.value, "Intel i486SX") == 0)
         cpu_index = 2;
      else if (strcmp(var.value, "Intel i486DX") == 0)
         cpu_index = 3;
      else if (strcmp(var.value, "Intel Pentium") == 0)
         cpu_index = 4;
      else if (strcmp(var.value, "Intel MMX Pentium") == 0)
         cpu_index = 5;
      else if (strcmp(var.value, "Intel Pentium Pro") == 0)
         cpu_index = 6;
      else if (strcmp(var.value, "Intel Pentium II") == 0)
         cpu_index = 7;
      else if (strcmp(var.value, "Intel Pentium III") == 0)
         cpu_index = 8;
      else if (strcmp(var.value, "Intel Pentium M") == 0)
         cpu_index = 9;
      else if (strcmp(var.value, "Intel Pentium 4") == 0)
         cpu_index = 10;
      else if (strcmp(var.value, "AMD K6-2") == 0)
         cpu_index = 15;
      else if (strcmp(var.value, "AMD K6-III") == 0)
         cpu_index = 16;
      else if (strcmp(var.value, "AMD K7 Athlon") == 0)
         cpu_index = 17;
      else if (strcmp(var.value, "AMD K7 Athlon XP") == 0)
         cpu_index = 18;
      else if (strcmp(var.value, "Neko Processor II") == 0)
         cpu_index = 255;
      else
         cpu_index = 0;
      SetCpuTypeIndex(cpu_index);
   }

   var.key = "np2kai_clk_mult";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      np2cfg.multiple = atoi(var.value);
   }

#if defined(SUPPORT_ASYNC_CPU)
   var.key = "np2kai_async_cpu";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "OFF") == 0)
         np2cfg.asynccpu = 0;
      else
         np2cfg.asynccpu = 1;
   }
#endif

   var.key = "np2kai_ExMemory";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      np2cfg.EXTMEM = atoi(var.value);
   }

   var.key = "np2kai_uselasthddmount";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "OFF") == 0)
         lr_uselasthddmount = 0;
      else
         lr_uselasthddmount = 1;
   }

   var.key = "np2kai_FastMC";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "OFF") == 0)
         np2cfg.memcheckspeed = 1;
      else
         np2cfg.memcheckspeed = 8;
   }

   var.key = "np2kai_skipline";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "OFF") == 0)
         np2cfg.skipline = false;
      else if (strcmp(var.value, "ON") == 0)
         np2cfg.skipline = true;
      else if (strcmp(var.value, "Full 255 lines") == 0){
         np2cfg.skiplight = 255;
         np2cfg.skipline = true;
      }
      scrndraw_redraw();
   }

   var.key = "np2kai_realpal";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "OFF") == 0)
         np2cfg.RASTER = 0;
      else
         np2cfg.RASTER = 1;
   }

   var.key = "np2kai_SNDboard";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "PC9801-86") == 0)
         np2cfg.SOUND_SW = SOUNDID_PC_9801_86;
      else if (strcmp(var.value, "PC9801-26K + 86") == 0)
         np2cfg.SOUND_SW = SOUNDID_PC_9801_86_26K;
      else if (strcmp(var.value, "PC9801-86 + Chibi-oto") == 0)
         np2cfg.SOUND_SW = SOUNDID_PC_9801_14;
      else if (strcmp(var.value, "PC9801-118") == 0)
         np2cfg.SOUND_SW = SOUNDID_PC_9801_118;
      else if (strcmp(var.value, "PC9801-86 + Mate-X PCM(B460)") == 0)
         np2cfg.SOUND_SW = SOUNDID_PC_9801_86_WSS;
      else if (strcmp(var.value, "PC9801-86 + 118(B460)") == 0)
         np2cfg.SOUND_SW = SOUNDID_PC_9801_86_118;
      else if (strcmp(var.value, "Mate-X PCM") == 0)
         np2cfg.SOUND_SW = SOUNDID_MATE_X_PCM;
      else if (strcmp(var.value, "Speak Board") == 0)
         np2cfg.SOUND_SW = SOUNDID_SPEAKBOARD;
      else if (strcmp(var.value, "PC9801-86 + Speak Board") == 0)
         np2cfg.SOUND_SW = SOUNDID_86_SPEAKBOARD;
      else if (strcmp(var.value, "Spark Board") == 0)
         np2cfg.SOUND_SW = SOUNDID_SPARKBOARD;
      else if (strcmp(var.value, "Sound Orchestra") == 0)
         np2cfg.SOUND_SW = SOUNDID_SOUNDORCHESTRA;
      else if (strcmp(var.value, "Sound Orchestra-V") == 0)
         np2cfg.SOUND_SW = SOUNDID_SOUNDORCHESTRAV;
      else if (strcmp(var.value, "Little Orchestra L") == 0)
         np2cfg.SOUND_SW = SOUNDID_LITTLEORCHESTRAL;
      else if (strcmp(var.value, "Multimedia Orchestra") == 0)
         np2cfg.SOUND_SW = SOUNDID_MMORCHESTRA;
#if defined(SUPPORT_SOUND_SB16)
      else if (strcmp(var.value, "Sound Blaster 16") == 0)
         np2cfg.SOUND_SW = SOUNDID_SB16;
      else if (strcmp(var.value, "PC9801-86 + Sound Blaster 16") == 0)
         np2cfg.SOUND_SW = SOUNDID_PC_9801_86_SB16;
      else if (strcmp(var.value, "Mate-X PCM + Sound Blaster 16") == 0)
         np2cfg.SOUND_SW = SOUNDID_WSS_SB16;
      else if (strcmp(var.value, "PC9801-118 + Sound Blaster 16") == 0)
         np2cfg.SOUND_SW = SOUNDID_PC_9801_118_SB16;
      else if (strcmp(var.value, "PC9801-86 + Mate-X PCM(B460) + Sound Blaster 16") == 0)
         np2cfg.SOUND_SW = SOUNDID_PC_9801_86_WSS_SB16;
      else if (strcmp(var.value, "PC9801-86 + 118(B460) + Sound Blaster 16") == 0)
         np2cfg.SOUND_SW = SOUNDID_PC_9801_86_118_SB16;
#endif
      else if (strcmp(var.value, "AMD-98") == 0)
         np2cfg.SOUND_SW = SOUNDID_AMD98;
      else if (strcmp(var.value, "WaveStar") == 0)
         np2cfg.SOUND_SW = SOUNDID_WAVESTAR;
#if defined(SUPPORT_PX)
      else if (strcmp(var.value, "Otomi-chanx2") == 0)
         np2cfg.SOUND_SW = SOUNDID_PX1;
      else if (strcmp(var.value, "Otomi-chanx2 + 86") == 0)
         np2cfg.SOUND_SW = SOUNDID_PX2;
#endif
      else if (strcmp(var.value, "None") == 0)
         np2cfg.SOUND_SW = SOUNDID_NONE;
      else if (strcmp(var.value, "PC9801-14") == 0)
         np2cfg.SOUND_SW = SOUNDID_PC_9801_14;
      else if (strcmp(var.value, "PC9801-26K") == 0)
         np2cfg.SOUND_SW = SOUNDID_PC_9801_26K;
   }

   var.key = "np2kai_118ROM";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "OFF") == 0)
         np2cfg.snd118rom = 0;
      else if (strcmp(var.value, "ON") == 0)
         np2cfg.snd118rom = 1;
   }

   var.key = "np2kai_jast_snd";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "OFF") == 0)
         np2oscfg.jastsnd = 0;
      else if (strcmp(var.value, "ON") == 0)
         np2oscfg.jastsnd = 1;
   }

   var.key = "np2kai_usefmgen";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "Default") == 0)
         np2cfg.usefmgen = 0x00;
      else if (strcmp(var.value, "fmgen") == 0)
         np2cfg.usefmgen = 0x01;
   }

   var.key = "np2kai_xroll";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "OFF") == 0)
         np2oscfg.xrollkey = 0;
      else if (strcmp(var.value, "ON") == 0)
         np2oscfg.xrollkey = 1;
   }

   var.key = "np2kai_volume_M";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      np2cfg.vol_master = atoi(var.value);
   }

   var.key = "np2kai_volume_F";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      np2cfg.vol_fm = atoi(var.value);
      opngen_setvol(np2cfg.vol_fm);
      oplgen_setvol(np2cfg.vol_fm);
#if defined(SUPPORT_FMGEN)
      if(g_opna[0].fmgen)
         OPNA_SetVolumeFM(g_opna[0].fmgen, pow((double)np2cfg.vol_fm / 128, 0.12) * (20 + 192) - 192);
#endif	/* defined(SUPPORT_FMGEN) */
   }

   var.key = "np2kai_volume_S";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      np2cfg.vol_ssg = atoi(var.value);
      psggen_setvol(np2cfg.vol_ssg);
#if defined(SUPPORT_FMGEN)
      if(g_opna[0].fmgen)
         OPNA_SetVolumePSG(g_opna[0].fmgen, pow((double)np2cfg.vol_ssg / 128, 0.12) * (20 + 192) - 192);
#endif	/* defined(SUPPORT_FMGEN) */
   }

   var.key = "np2kai_volume_A";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      np2cfg.vol_adpcm = atoi(var.value);
      adpcm_setvol(np2cfg.vol_adpcm);
   }

   var.key = "np2kai_volume_P";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      np2cfg.vol_pcm = atoi(var.value);
      pcm86gen_setvol(np2cfg.vol_pcm);
#if defined(SUPPORT_FMGEN)
      if(g_opna[0].fmgen)
         OPNA_SetVolumeADPCM(g_opna[0].fmgen, pow((double)np2cfg.vol_pcm / 128, 0.12) * (20 + 192) - 192);
#endif	/* defined(SUPPORT_FMGEN) */
   }

   var.key = "np2kai_volume_R";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      np2cfg.vol_rhythm = atoi(var.value);
      rhythm_setvol(np2cfg.vol_rhythm);
#if defined(SUPPORT_FMGEN)
      if(g_opna[0].fmgen)
         OPNA_SetVolumeRhythmTotal(g_opna[0].fmgen, pow((double)np2cfg.vol_rhythm / 128, 0.12) * (20 + 192) - 192);
#endif	/* defined(SUPPORT_FMGEN) */
   }

   var.key = "np2kai_volume_C";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      np2cfg.davolume = atoi(var.value);
      rhythm_setvol(np2cfg.davolume);
   }

   var.key = "np2kai_Seek_Snd";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "OFF") == 0)
         np2cfg.MOTOR = false;
      else if (strcmp(var.value, "ON") == 0)
         np2cfg.MOTOR = true;
   }

   var.key = "np2kai_Seek_Vol";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      np2cfg.MOTORVOL = atoi(var.value);
   }

   var.key = "np2kai_BEEP_vol";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      np2cfg.BEEP_VOL = atoi(var.value);
      beep_setvol(np2cfg.BEEP_VOL);
   }

#if defined(SUPPORT_WAB)
   var.key = "np2kai_CLGD_en";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "ON") == 0)
         np2cfg.usegd5430 = 1;
      else
         np2cfg.usegd5430 = 0;
   }

   var.key = "np2kai_CLGD_type";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "PC-9821Xe10,Xa7e,Xb10 built-in") == 0)
         np2cfg.gd5430type = CIRRUS_98ID_Xe10;
      else if (strcmp(var.value, "PC-9821Bp,Bs,Be,Bf built-in") == 0)
         np2cfg.gd5430type = CIRRUS_98ID_Be;
      else if (strcmp(var.value, "PC-9821Xe built-in") == 0)
         np2cfg.gd5430type = CIRRUS_98ID_Xe;
      else if (strcmp(var.value, "PC-9821Cb built-in") == 0)
         np2cfg.gd5430type = CIRRUS_98ID_Cb;
      else if (strcmp(var.value, "PC-9821Cf built-in") == 0)
         np2cfg.gd5430type = CIRRUS_98ID_Cf;
      else if (strcmp(var.value, "PC-9821Cb2 built-in") == 0)
         np2cfg.gd5430type = CIRRUS_98ID_Cb2;
      else if (strcmp(var.value, "PC-9821Cx2 built-in") == 0)
         np2cfg.gd5430type = CIRRUS_98ID_Cx2;
      else if (strcmp(var.value, "PC-9821 PCI CL-GD5446 built-in") == 0)
         np2cfg.gd5430type = CIRRUS_98ID_PCI;
      else if (strcmp(var.value, "MELCO WAB-S") == 0)
         np2cfg.gd5430type = CIRRUS_98ID_WAB;
      else if (strcmp(var.value, "MELCO WSN-A2F") == 0)
         np2cfg.gd5430type = CIRRUS_98ID_WSN_A2F;
      else if (strcmp(var.value, "MELCO WSN-A4F") == 0)
         np2cfg.gd5430type = CIRRUS_98ID_WSN;
      else if (strcmp(var.value, "I-O DATA GA-98NBI/C") == 0)
         np2cfg.gd5430type = CIRRUS_98ID_GA98NBIC;
      else if (strcmp(var.value, "I-O DATA GA-98NBII") == 0)
         np2cfg.gd5430type = CIRRUS_98ID_GA98NBII;
      else if (strcmp(var.value, "I-O DATA GA-98NBIV") == 0)
         np2cfg.gd5430type = CIRRUS_98ID_GA98NBIV;
      else if (strcmp(var.value, "PC-9801-96(PC-9801B3-E02)") == 0)
         np2cfg.gd5430type = CIRRUS_98ID_96;
      else if (strcmp(var.value, "Auto Select(Xe10, GA-98NBI/C), PCI") == 0)
         np2cfg.gd5430type = CIRRUS_98ID_AUTO_XE_G1_PCI;
      else if (strcmp(var.value, "Auto Select(Xe10, GA-98NBII), PCI") == 0)
         np2cfg.gd5430type = CIRRUS_98ID_AUTO_XE_G2_PCI;
      else if (strcmp(var.value, "Auto Select(Xe10, GA-98NBIV), PCI") == 0)
         np2cfg.gd5430type = CIRRUS_98ID_AUTO_XE_G4_PCI;
      else if (strcmp(var.value, "Auto Select(Xe10, WAB-S), PCI") == 0)
         np2cfg.gd5430type = CIRRUS_98ID_AUTO_XE_WA_PCI;
      else if (strcmp(var.value, "Auto Select(Xe10, WSN-A2F), PCI") == 0)
         np2cfg.gd5430type = CIRRUS_98ID_AUTO_XE_WS_PCI;
      else if (strcmp(var.value, "Auto Select(Xe10, WSN-A4F), PCI") == 0)
         np2cfg.gd5430type = CIRRUS_98ID_AUTO_XE_W4_PCI;
      else if (strcmp(var.value, "Auto Select(Xe10, WAB-S)") == 0)
         np2cfg.gd5430type = CIRRUS_98ID_AUTO_XE10_WABS;
      else if (strcmp(var.value, "Auto Select(Xe10, WSN-A2F)") == 0)
         np2cfg.gd5430type = CIRRUS_98ID_AUTO_XE10_WSN2;
      else if (strcmp(var.value, "Auto Select(Xe10, WSN-A4F)") == 0)
         np2cfg.gd5430type = CIRRUS_98ID_AUTO_XE10_WSN4;
   }

   var.key = "np2kai_CLGD_fc";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "ON") == 0)
         np2cfg.gd5430fakecur = 1;
      else
         np2cfg.gd5430fakecur = 0;
   }
#endif	/* defined(SUPPORT_WAB) */

#if defined(SUPPORT_PEGC)
   var.key = "np2kai_PEGC";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "ON") == 0)
         np2cfg.usepegcplane = 1;
      else
         np2cfg.usepegcplane = 0;
   }
#endif

#if defined(SUPPORT_PCI)
   var.key = "np2kai_PCI_en";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "ON") == 0)
         np2cfg.usepci = 1;
      else
         np2cfg.usepci = 0;
   }

   var.key = "np2kai_PCI_type";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "Intel 82434LX") == 0)
         np2cfg.pci_pcmc = PCI_PCMC_82434LX;
      else if (strcmp(var.value, "Intel 82441FX") == 0)
         np2cfg.pci_pcmc = PCI_PCMC_82441FX;
      else if (strcmp(var.value, "VLSI Wildcat") == 0)
         np2cfg.pci_pcmc = PCI_PCMC_WILDCAT;
   }

   var.key = "np2kai_PCI_bios32";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "ON") == 0)
         np2cfg.pci_bios32 = 1;
      else
         np2cfg.pci_bios32 = 0;
   }
#endif	/* defined(SUPPORT_PCI) */

   var.key = "np2kai_usecdecc";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "ON") == 0)
         np2cfg.usecdecc = 1;
      else
         np2cfg.usecdecc = 0;
   }

  var.key = "np2kai_stick2mouse";
  var.value = NULL;
  if(environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
    if(strcmp(var.value, "L-stick") == 0) {
      s2m = true;
      s2m_no = 0;
    } else if(strcmp(var.value, "R-stick") == 0) {
      s2m = true;
      s2m_no = 1;
    } else {
      s2m = false;
    }
  }

  var.key = "np2kai_stick2mouse_shift";
  var.value = NULL;
  if(environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
    if(strcmp(var.value, "R1") == 0) {
      s2m_shift = RETRO_DEVICE_ID_JOYPAD_R;
    } else if(strcmp(var.value, "R2") == 0) {
      s2m_shift = RETRO_DEVICE_ID_JOYPAD_R2;
    } else if(strcmp(var.value, "L1") == 0) {
      s2m_shift = RETRO_DEVICE_ID_JOYPAD_L;
    } else if(strcmp(var.value, "L2") == 0) {
      s2m_shift = RETRO_DEVICE_ID_JOYPAD_L2;
    } else {
      s2m_shift = 0xFF;
    }
  }

  var.key = "np2kai_joymode";
  var.value = NULL;
  if(environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
    int preJoyModeInt = m_tJoyModeInt;
    if(strcmp(var.value, "Mouse") == 0) {
      m_tJoyMode = LR_NP2KAI_JOYMODE_MOUSE;
      m_tJoyModeInt = 1;
    } else if(strcmp(var.value, "Arrows") == 0) {
      m_tJoyMode = LR_NP2KAI_JOYMODE_KEY;
      memcpy(j2k_key, j2k_key_arrow, sizeof(uint16_t) * 12);
      m_tJoyModeInt = 2;
    } else if(strcmp(var.value, "Arrows 3button") == 0) {
      m_tJoyMode = LR_NP2KAI_JOYMODE_KEY;
      memcpy(j2k_key, j2k_key_arrow3, sizeof(uint16_t) * 12);
      m_tJoyModeInt = 3;
    } else if(strcmp(var.value, "Keypad") == 0) {
      m_tJoyMode = LR_NP2KAI_JOYMODE_KEY;
      memcpy(j2k_key, j2k_key_kpad, sizeof(uint16_t) * 12);
      m_tJoyModeInt = 4;
    } else if(strcmp(var.value, "Keypad 3button") == 0) {
      m_tJoyMode = LR_NP2KAI_JOYMODE_KEY;
      memcpy(j2k_key, j2k_key_kpad3, sizeof(uint16_t) * 12);
      m_tJoyModeInt = 5;
    } else if(strcmp(var.value, "Manual Keyboard") == 0) {
      m_tJoyMode = LR_NP2KAI_JOYMODE_KEY;
      memcpy(j2k_key, np2oscfg.lrjoybtn, sizeof(uint16_t) * 12);
      m_tJoyModeInt = 6;
    } else if(strcmp(var.value, "Atari Joypad") == 0) {
      m_tJoyMode = LR_NP2KAI_JOYMODE_ATARI;
      m_tJoyModeInt = 7;
    } else {
      m_tJoyMode = LR_NP2KAI_JOYMODE_NONE;
      m_tJoyModeInt = 0;
    }
    if(m_tJoyModeInt != preJoyModeInt) {
      m_bJoyModeChange = TRUE;
    }
  }

  var.key = "np2kai_joynp2menu";
  var.value = NULL;
  if(environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
    if(strcmp(var.value, "A") == 0) {
      joyNP2menu = TRUE;
      joyNP2menubtn = RETRO_DEVICE_ID_JOYPAD_A;
    } else if(strcmp(var.value, "B") == 0) {
      joyNP2menu = TRUE;
      joyNP2menubtn = RETRO_DEVICE_ID_JOYPAD_B;
    } else if(strcmp(var.value, "X") == 0) {
      joyNP2menu = TRUE;
      joyNP2menubtn = RETRO_DEVICE_ID_JOYPAD_X;
    } else if(strcmp(var.value, "Y") == 0) {
      joyNP2menu = TRUE;
      joyNP2menubtn = RETRO_DEVICE_ID_JOYPAD_Y;
    } else if(strcmp(var.value, "L1") == 0) {
      joyNP2menu = TRUE;
      joyNP2menubtn = RETRO_DEVICE_ID_JOYPAD_L;
    } else if(strcmp(var.value, "L2") == 0) {
      joyNP2menu = TRUE;
      joyNP2menubtn = RETRO_DEVICE_ID_JOYPAD_L2;
    } else if(strcmp(var.value, "R1") == 0) {
      joyNP2menu = TRUE;
      joyNP2menubtn = RETRO_DEVICE_ID_JOYPAD_R;
    } else if(strcmp(var.value, "R2") == 0) {
      joyNP2menu = TRUE;
      joyNP2menubtn = RETRO_DEVICE_ID_JOYPAD_R2;
    } else if(strcmp(var.value, "L3") == 0) {
      joyNP2menu = TRUE;
      joyNP2menubtn = RETRO_DEVICE_ID_JOYPAD_L3;
    } else if(strcmp(var.value, "R3") == 0) {
      joyNP2menu = TRUE;
      joyNP2menubtn = RETRO_DEVICE_ID_JOYPAD_R3;
    } else if(strcmp(var.value, "Start") == 0) {
      joyNP2menu = TRUE;
      joyNP2menubtn = RETRO_DEVICE_ID_JOYPAD_START;
    } else if(strcmp(var.value, "Select") == 0) {
      joyNP2menu = TRUE;
      joyNP2menubtn = RETRO_DEVICE_ID_JOYPAD_SELECT;
    } else {
      joyNP2menu = FALSE;
    }
  }

   var.key = "np2kai_lcd";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "OFF") == 0)
         np2cfg.LCD_MODE = 0;
      else
         np2cfg.LCD_MODE = 1;
      pal_makelcdpal();
      scrndraw_redraw();
   }

   var.key = "np2kai_gdc";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "uPD72020") == 0)
         np2cfg.uPD72020 = 1;
      else
         np2cfg.uPD72020 = 0;
      gdc_restorekacmode();
      gdcs.grphdisp |= GDCSCRN_ALLDRAW2;
   }

   sysmng_update(SYS_UPDATEOSCFG | SYS_UPDATECFG);

}

void retro_get_system_info(struct retro_system_info *info)
{
   memset(info, 0, sizeof(*info));
   info->need_fullpath    = LR_NEEDFILEPATH;
   info->valid_extensions = LR_VALIDFILEEXT;
   info->library_version  = LR_LIBVERSION;
   info->library_name     = LR_CORENAME;
   info->block_extract    = LR_BLOCKARCEXTRACT;
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
   int w, h;

   scrnmng_getsize(&w, &h);

   info->geometry.base_width   = w;
   info->geometry.base_height  = h;
   info->geometry.max_width    = w;
   info->geometry.max_height   = h;
   info->geometry.aspect_ratio = (double)w / h;
   info->timing.fps            = LR_SCREENFPS;
   info->timing.sample_rate    = LR_SOUNDRATE;
}

void retro_init (void)
{
   enum retro_pixel_format rgb;

   scrnmng_initialize();

   update_variables();

	struct retro_log_callback log;
	if (environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log))
		log_cb = log.log;
	else
		log_cb = NULL;

	if (log_cb)
		log_cb(RETRO_LOG_INFO, "Logger interface initialized\n");

	static struct retro_midi_interface midi_interface;
	if(environ_cb(RETRO_ENVIRONMENT_GET_MIDI_INTERFACE, &midi_interface))
		retro_midi_interface = &midi_interface;
	else
		retro_midi_interface = NULL;

	if (log_cb)
		log_cb(RETRO_LOG_INFO, "MIDI interface %s.\n",
			retro_midi_interface ? "initialized" : "unavailable\n");

#if defined(SUPPORT_CL_GD5430)
   draw32bit = np2cfg.usegd5430;
#endif
   if(draw32bit) {
      rgb = RETRO_PIXEL_FORMAT_XRGB8888;
   } else {
      rgb = RETRO_PIXEL_FORMAT_RGB565;
   }
   if(environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &rgb) && log_cb)
         log_cb(RETRO_LOG_INFO, "Frontend supports RGB565 (or XRGB8888).\n");
}

void retro_deinit(void)
{
   np2_end();
}

void retro_reset (void)
{
   if(menuvram) {
      menubase_close();
      menu_active = 0;
   }
   resetInput();
   pccore_reset();
   did_reset = true;
}
extern  void playretro();

static int firstcall=1;

void retro_run (void)
{
   if(firstcall)
   {
      pre_main(RPATH);
      update_variables();
      pccore_cfgupdate();
      pccore_reset();
      firstcall=0;
      printf("INIT done\n");
      return;
   }

   bool updated = false;
   int w, h;

   scrnmng_getsize(&w, &h);

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
   {
      update_variables();
   }

   if (did_reset){
      if(menuvram) {
         menubase_close();
         menu_active = 0;
      }
      resetInput();
      pccore_cfgupdate();
      pccore_reset();
      did_reset = false;
   }

   updateInput();

   if (menuvram != NULL){
      scrnmng_update();
      draw_cross(lastx,lasty);
   }
   else {
      //emulate 1 frame
      pccore_exec(true /*draw*/);
      sdlaudio_callback(NULL, NULL,SNDSZ*4);
   }

   if(draw32bit) {
      video_cb(FrameBuffer, w, h, w * 4/*Pitch*/);
   } else {
      video_cb(FrameBuffer, w, h, w * 2/*Pitch*/);
   }

    if (retro_midi_interface && retro_midi_interface->output_enabled())
        retro_midi_interface->flush();
}

void retro_cheat_reset(void)
{
   //no cheats on this core
} 

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
   //no cheats on this core
}

bool retro_load_game(const struct retro_game_info *game)
{
   //get system dir
   const char* syspath = 0;
   OEMCHAR np2path[MAX_PATH];
   bool load_floppy=false;
   bool worked = environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &syspath);
   if(!worked)abort();

   if(game != NULL)
      extract_directory(lr_game_base_dir, game->path, sizeof(lr_game_base_dir));

   milstr_ncpy(np2path, syspath, MAX_PATH);
   lr_init = 1;

#ifdef _WIN32
   milstr_ncat(np2path, OEMTEXT("\\np2kai"), MAX_PATH);
#else
   milstr_ncat(np2path, OEMTEXT("/np2kai"), MAX_PATH);
#endif

   OEMSNPRINTF(tmppath, MAX_PATH, OEMTEXT("%s%c"), np2path, OEMPATHDIVC);

   np2cfg.delayms = 0;

   OEMSNPRINTF(np2cfg.fontfile, MAX_PATH, OEMTEXT("%s%cfont.bmp"), np2path, OEMPATHDIVC);
   file_setcd(np2cfg.fontfile);
   OEMSNPRINTF(np2cfg.biospath, MAX_PATH, OEMTEXT("%s%c"), np2path, OEMPATHDIVC);

   if(game != NULL)
      strcpy(RPATH,game->path);
   else
      strcpy(RPATH,"");

   return true;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
   audio_batch_cb = cb;
}

unsigned retro_api_version(void)
{
   return RETRO_API_VERSION;
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
   (void)port;
   (void)device;
}

unsigned retro_get_region (void)
{
   return RETRO_REGION_NTSC;
}

#define RETRO_NP2_TEMPSTATE "temp_.sxx"

size_t retro_serialize_size(void)
{
   int ret;
   char	*path;
   size_t size;
   FILEH fh;

   path = file_getcd(RETRO_NP2_TEMPSTATE);
   ret = statsave_save(path);
   if(ret) {
      size = 0;
   } else {
      fh = file_open_rb(path);
      size = file_getsize(fh);
      file_close(fh);
   }
   file_delete(path);

   return size;
}

bool retro_serialize(void *data, size_t size)
{
   int ret;
   char	*path;
   FILEH fh;

   path = file_getcd(RETRO_NP2_TEMPSTATE);
   ret = statsave_save(path);
   if(ret) {
      file_delete(path);
      return false;
   }
   fh = file_open_rb(path);
   file_read(fh, data, size);
   file_close(fh);
   file_delete(path);
   return true;
}

bool retro_unserialize(const void *data, size_t size)
{
   int ret;
   char	*path;
   FILEH fh;

   if(size <= 0) {
      return false;
   }
   path = file_getcd(RETRO_NP2_TEMPSTATE);
   fh = file_create(path);
   file_write(fh, data, size);
   file_close(fh);
   statsave_load(path);
   file_delete(path);
   return true;
}

