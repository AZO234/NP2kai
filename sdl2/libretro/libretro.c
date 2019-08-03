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
char tmppath[4096];

static retro_log_printf_t log_cb = NULL;
static retro_video_refresh_t video_cb = NULL;
static retro_input_poll_t poll_cb = NULL;
retro_input_state_t input_cb = NULL;
retro_environment_t environ_cb = NULL;
extern struct retro_midi_interface *retro_midi_interface;

uint32_t   FrameBuffer[LR_SCREENWIDTH * LR_SCREENHEIGHT];

retro_audio_sample_batch_t audio_batch_cb = NULL;

static char CMDFILE[512];

bool did_reset, joy2key;
int lr_init = 0;

static char base_dir[MAX_PATH];

#ifdef _WIN32
static char slash = '\\';
#else
static char slash = '/';
#endif

static void update_variables(void);

/* media swap support */
struct retro_disk_control_callback dskcb;
static char disk_paths[50][MAX_PATH] = {0};
static unsigned drvno = 1;
static unsigned disk_index = 0;
static unsigned disk_images = 0;
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
   if(disk_index == disk_images)
   {
      //retroarch is trying to set "no disk in tray"
      return true;
   }

   update_variables();
   strcpy(np2cfg.fddfile[drvno], disk_paths[disk_index]);
   diskdrv_setfdd(drvno, disk_paths[disk_index], 0);
   return true;
}

unsigned getnumimages(){
   return disk_images;
}

bool addimageindex() {
   if (disk_images >= 50)
      return false;

   disk_images++;
   return true;
}

bool replacedsk(unsigned index,const struct retro_game_info *info){
   strcpy(disk_paths[index], info->path);
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

   environ_cb(RETRO_ENVIRONMENT_SET_DISK_CONTROL_INTERFACE,&dskcb);
}

void setnxtdskindex(void){
	if(disk_images > 2) {
		if(disk_index + 1 != disk_images) {
			setdskindex(disk_index + 1);
		} else {
			setdskindex(0);
		}
	}
}

void setpredskindex(void){
	if(disk_images > 2) {
		if(disk_index == 0) {
			setdskindex(disk_images - 1);
		} else {
			setdskindex(disk_index - 1);
		}
	}
}
/* end media swap support */

int loadcmdfile(char *argv)
{
   int res=0;

   FILE *fp = fopen(argv,"r");

   if( fp != NULL )
   {
      if ( fgets (CMDFILE , 512 , fp) != NULL )
         res=1;
      fclose (fp);
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

void parse_cmdline( const char *argv );

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

static bool read_m3u(const char *file)
{
   char line[MAX_PATH];
   char name[MAX_PATH];
   FILE *f = fopen(file, "r");

   if (!f)
      return false;

   while (fgets(line, sizeof(line), f) && disk_images < sizeof(disk_paths) / sizeof(disk_paths[0]))
   {
      if (line[0] == '#')
         continue;

      char *carriage_return = strchr(line, '\r');
      if (carriage_return)
         *carriage_return = '\0';

      char *newline = strchr(line, '\n');
      if (newline)
         *newline = '\0';

      // Remove any beginning and ending quotes as these can cause issues when feeding the paths into command line later
      if (line[0] == '"')
          memmove(line, line + 1, strlen(line));

      if (line[strlen(line) - 1] == '"')
          line[strlen(line) - 1]  = '\0';

      if (line[0] != '\0')
      {
         snprintf(name, sizeof(name), "%s%c%s", base_dir, slash, line);
         strcpy(disk_paths[disk_images], name);
         disk_images++;
      }
   }

   fclose(f);
   return (disk_images != 0);
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

int pre_main(const char *argv)
{
   int i=0;
   int Only1Arg;

   if (strlen(argv) > strlen("cmd"))
   {
      if( HandleExtension((char*)argv,"cmd") || HandleExtension((char*)argv,"CMD"))
         i=loadcmdfile((char*)argv);
      else if (HandleExtension((char*)argv, "m3u") || HandleExtension((char*)argv, "M3U"))
      {
         if (!read_m3u((char*)argv))
         {
            if (log_cb)
               log_cb(RETRO_LOG_ERROR, "%s\n", "[libretro]: failed to read m3u file ...");
            return false;
         }

         sprintf((char*)argv, "np2kai \"%s\"", disk_paths[0]);
         if(disk_images > 1)
         {
            sprintf((char*)argv, "%s \"%s\"", argv, disk_paths[1]);
         }
         disk_inserted = true;
         attach_disk_swap_interface();
      }
   }

   if(i==1)
      parse_cmdline(CMDFILE);
   else
      parse_cmdline(argv);

   Only1Arg = (strcmp(ARGUV[0],"np2kai") == 0) ? 0 : 1;

   for (i = 0; i<64; i++)
      xargv_cmd[i] = NULL;


   if(Only1Arg)
   {
      int cfgload=0;

      Add_Option("np2kai");

      if(cfgload==0)
      {

      }

      Add_Option(RPATH);
   }
   else
   { // Pass all cmdline args
      for(i = 0; i < ARGUC; i++)
         Add_Option(ARGUV[i]);
   }

   for (i = 0; i < PARAMCOUNT; i++)
   {
      xargv_cmd[i] = (char*)(XARGV[i]);
      printf("arg_%d:%s\n",i,xargv_cmd[i]);
   }

   dosio_init();
   file_setcd(tmppath);

   i=np2_main(PARAMCOUNT,( char **)xargv_cmd);

   xargv_cmd[PARAMCOUNT - 2] = NULL;

   return 0;
}

void parse_cmdline(const char *argv)
{
   char *p,*p2,*start_of_word;
   int c,c2;
   static char buffer[512*4];
   enum states { DULL, IN_WORD, IN_STRING } state = DULL;

   strcpy(buffer,argv);
   strcat(buffer," \0");

   for (p = buffer; *p != '\0'; p++)
   {
      c = (unsigned char) *p; /* convert to unsigned char for is* functions */
      switch (state)
      {
         case DULL: /* not in a word, not in a double quoted string */
            if (isspace(c)) /* still not in a word, so ignore this char */
               continue;
            /* not a space -- if it's a double quote we go to IN_STRING, else to IN_WORD */
            if (c == '"')
            {
               state = IN_STRING;
               start_of_word = p + 1; /* word starts at *next* char, not this one */
               continue;
            }
            state = IN_WORD;
            start_of_word = p; /* word starts here */
            continue;
         case IN_STRING:
            /* we're in a double quoted string, so keep going until we hit a close " */
            if (c == '"')
            {
               /* word goes from start_of_word to p-1 */
               //... do something with the word ...
               for (c2 = 0,p2 = start_of_word; p2 < p; p2++, c2++)
                  ARGUV[ARGUC][c2] = (unsigned char) *p2;
               ARGUC++;

               state = DULL; /* back to "not in word, not in string" state */
            }
            continue; /* either still IN_STRING or we handled the end above */
         case IN_WORD:
            /* we're in a word, so keep going until we get to a space */
            if (isspace(c))
            {
               /* word goes from start_of_word to p-1 */
               //... do something with the word ...
               for (c2 = 0,p2 = start_of_word; p2 <p; p2++,c2++)
                  ARGUV[ARGUC][c2] = (unsigned char) *p2;
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
	unsigned  short color;

	for(j=y;j<y+dy;j++){
		idx=0;
		for(i=x;i<x+dx;i++){
			if(cross[j-y][idx]=='.')DrawPointBmp(FrameBuffer,i,j,0xffffff);
			else if(cross[j-y][idx]=='X')DrawPointBmp(FrameBuffer,i,j,0);
			idx++;
		}
	}

}

static int lastx=320,lasty=240;
static int menukey=0;
static int menu_active=0;
static int mbL = 0, mbR = 0;
static bool joymouse;
static double joymouseaxel = 1.0;
static int joymousemovebtn = 0;
static int joymouseaxelratio = 10;
static bool joyNP2menu;
static int joyNP2menubtn;
static bool joyNP2menu_oldjoymouse;
static bool joyNP2menu_oldjoymouse;

bool mapkey_down[12];

static int joy2key_map[12] = { 
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

static uint16_t joy2key_map_arr[12] = { 
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

static uint16_t joy2key_map_kpad[12] = { 
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
   
void updateInput(){

   static int mposx=320,mposy=240;
   int w, h;

   scrnmng_getsize(&w, &h);

   poll_cb();

   joymng_sync();

   uint32_t i;
   
   if (joy2key)
   {
      for (i = 0; i < 12; i++)
      {
         if (input_cb(0, RETRO_DEVICE_JOYPAD, 0, joy2key_map[i]) && !mapkey_down[i] )
         {
            send_libretro_key_down(np2oscfg.lrjoybtn[i]);
            mapkey_down[i] = 1;
         }
         else if (!input_cb(0, RETRO_DEVICE_JOYPAD, 0, joy2key_map[i]) )
         {
            send_libretro_key_up(np2oscfg.lrjoybtn[i]);
            mapkey_down[i] = 0;
         }
      }
   }
   else
   {
      /* Keyboard Mapping */
      for (i=0; i < keys_needed; i++)
      {
         if (input_cb(0, RETRO_DEVICE_KEYBOARD, 0, keys_to_poll[i]))
            send_libretro_key_down(keys_to_poll[i]);
         else
            send_libretro_key_up(keys_to_poll[i]);
      }
   }

   if ((input_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_F12) ||
      (joyNP2menu && input_cb(0, RETRO_DEVICE_JOYPAD, 0, joyNP2menubtn)) ||
      input_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_MIDDLE)) && menukey==0){

      menukey=1;


      if (menuvram == NULL) {
         if (joyNP2menu && input_cb(0, RETRO_DEVICE_JOYPAD, 0, joyNP2menubtn)) {
            joyNP2menu_oldjoymouse = joymouse;
            joymouse = true;
         }
         sysmenu_menuopen(0, 0, 0);
         mposx=0;mposy=0;
         lastx=0;lasty=0;
         menu_active=1;
      } else {
         if (joyNP2menu && input_cb(0, RETRO_DEVICE_JOYPAD, 0, joyNP2menubtn)) {
            joymouse = joyNP2menu_oldjoymouse;
         }
         menubase_close();
         scrndraw_redraw();
         menu_active=0;
      }
   } else if ( !(input_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_F12) ||
      (joyNP2menu && input_cb(0, RETRO_DEVICE_JOYPAD, 0, joyNP2menubtn)) ||
      input_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_MIDDLE)) && menukey==1)
      menukey=0;

   if (!joymouse) {
      int mouse_x = input_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X);
      int mouse_y = input_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_Y);

      if (menuvram == NULL)
         mousemng_sync(mouse_x,mouse_y);

      mposx+=mouse_x;if(mposx<0)mposx=0;if(mposx>=w)mposx=w-1;
      mposy+=mouse_y;if(mposy<0)mposy=0;if(mposy>=h)mposy=h-1;

      if(lastx!=mposx || lasty!=mposy)
         if (menuvram != NULL)
            menubase_moving(mposx, mposy, 0);

      int mouse_l = input_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_LEFT);
      int mouse_r = input_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_RIGHT);

      if(mbL==0 && mouse_l)
      {
         mbL=1;
         if(menuvram == NULL)
         {
            mousemng_buttonevent(MOUSEMNG_LEFTDOWN);
         }
         else
         {
            menubase_moving(mposx, mposy, 1);
         }
      }
      else if(mbL==1 && !mouse_l)
      {
         mbL=0;
         if(menuvram == NULL)
         {
            mousemng_buttonevent(MOUSEMNG_LEFTUP);
         }
         else
         {
            menubase_moving(mposx, mposy, 2);
            scrndraw_redraw();
         }
      }

      if(mbR==0 && mouse_r)
      {
         mbR=1;
         if(menuvram == NULL)
         {
            mousemng_buttonevent(MOUSEMNG_RIGHTDOWN);
         }
      }
      else if(mbR==1 && !mouse_r)
      {
         mbR=0;
         if(menuvram == NULL)
         {
            mousemng_buttonevent(MOUSEMNG_RIGHTUP);
         }
      }
   } else {
      int mouse_x = 0;
      int mouse_y = 0;

      if((input_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP) ||
         input_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN) ||
         input_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT) ||
         input_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT)) && joymousemovebtn==0)
      {
         joymousemovebtn = 1;
         joymouseaxel = 1.0;
      } else if(!(input_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP) ||
         input_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN) ||
         input_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT) ||
         input_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT)) && joymousemovebtn==1)
      {
         joymousemovebtn = 0;
      }

      if(input_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R)) {
         if(joymouseaxelratio != 1) {
            joymouseaxel += 0.1 * joymouseaxelratio;
         }
      } else {
         joymouseaxel += 0.1;
      }

      if(input_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP) && joymousemovebtn == 1) {
         if(input_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT)) {
            mouse_x = 1.0 * -joymouseaxel;
            mouse_y = 1.0 * -joymouseaxel;
         } else if(input_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT)) {
            mouse_x = 1.0 * joymouseaxel;
            mouse_y = 1.0 * -joymouseaxel;
         } else {
            mouse_y = 1.0 * -joymouseaxel / 1.414;
         }
      } else if(input_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN) && joymousemovebtn == 1) {
         if(input_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT)) {
            mouse_x = 1.0 * -joymouseaxel;
            mouse_y = 1.0 * joymouseaxel;
         } else if(input_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT)) {
            mouse_x = 1.0 * joymouseaxel;
            mouse_y = 1.0 * joymouseaxel;
         } else {
            mouse_y = 1.0 * joymouseaxel / 1.414;
         }
      } else {
         if(input_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT) && joymousemovebtn == 1)
            mouse_x = 1.0 * -joymouseaxel / 1.414;
         else if(input_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT) && joymousemovebtn == 1)
            mouse_x = 1.0 * joymouseaxel / 1.414;
      }

      if (menuvram == NULL)
         mousemng_sync(mouse_x,mouse_y);

      mposx += mouse_x;
      if(mposx < 0)
         mposx = 0;
      if(mposx >= w)
         mposx = w - 1;

      mposy += mouse_y;
      if(mposy < 0)
         mposy = 0;
      if(mposy >= h)
         mposy = h - 1;

      if(lastx!=mposx || lasty!=mposy)
         if (menuvram != NULL)
            menubase_moving(mposx, mposy, 0);

      int mouse_l = input_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B);
      int mouse_r = input_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A);

      if(mbL==0 && mouse_l)
      {
         mbL=1;
         if(menuvram == NULL)
         {
            mousemng_buttonevent(MOUSEMNG_LEFTDOWN);
         }
         else
         {
            menubase_moving(mposx, mposy, 1);
         }
      }
      else if(mbL==1 && !mouse_l)
      {
         mbL=0;
         if(menuvram == NULL)
         {
            mousemng_buttonevent(MOUSEMNG_LEFTUP);
         }
         else
         {
            menubase_moving(mposx, mposy, 2);
            scrndraw_redraw();
         }
      }

      if(mbR==0 && mouse_r)
      {
         mbR=1;
         if(menuvram == NULL)
         {
            mousemng_buttonevent(MOUSEMNG_RIGHTDOWN);
         }
      }
      else if(mbR==1 && !mouse_r)
      {
         mbR=0;
         if(menuvram == NULL)
         {
            mousemng_buttonevent(MOUSEMNG_RIGHTUP);
         }
      }
   }

   lastx=mposx;lasty=mposy;
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
         np2cfg.SOUND_SW = 0x04;
      else if (strcmp(var.value, "PC9801-26K + 86") == 0)
         np2cfg.SOUND_SW = 0x06;
      else if (strcmp(var.value, "PC9801-86 + Chibi-oto") == 0)
         np2cfg.SOUND_SW = 0x14;
      else if (strcmp(var.value, "PC9801-118") == 0)
         np2cfg.SOUND_SW = 0x08;
      else if (strcmp(var.value, "PC9801-86 + Mate-X PCM(B460)") == 0)
         np2cfg.SOUND_SW = 0x64;
      else if (strcmp(var.value, "PC9801-86 + 118") == 0)
         np2cfg.SOUND_SW = 0x68;
      else if (strcmp(var.value, "Mate-X PCM(B460)") == 0)
         np2cfg.SOUND_SW = 0x60;
      else if (strcmp(var.value, "Speak Board") == 0)
         np2cfg.SOUND_SW = 0x20;
      else if (strcmp(var.value, "PC9801-86 + Speak Board") == 0)
         np2cfg.SOUND_SW = 0x24;
      else if (strcmp(var.value, "Spark Board") == 0)
         np2cfg.SOUND_SW = 0x40;
      else if (strcmp(var.value, "Sound Orchestra") == 0)
         np2cfg.SOUND_SW = 0x32;
      else if (strcmp(var.value, "Sound Orchestra-V") == 0)
         np2cfg.SOUND_SW = 0x82;
      else if (strcmp(var.value, "Sound Blaster 16") == 0)
         np2cfg.SOUND_SW = 0x41;
      else if (strcmp(var.value, "AMD-98") == 0)
         np2cfg.SOUND_SW = 0x80;
      else if (strcmp(var.value, "WaveStar") == 0)
         np2cfg.SOUND_SW = 0x70;
      else if (strcmp(var.value, "Otomi-chanx2") == 0)
         np2cfg.SOUND_SW = 0x30;
      else if (strcmp(var.value, "Otomi-chanx2 + 86") == 0)
         np2cfg.SOUND_SW = 0x50;
      else if (strcmp(var.value, "None") == 0)
         np2cfg.SOUND_SW = 0x00;
      else if (strcmp(var.value, "PC9801-14") == 0)
         np2cfg.SOUND_SW = 0x01;
      else if (strcmp(var.value, "PC9801-26K") == 0)
         np2cfg.SOUND_SW = 0x02;
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

   var.key = "np2kai_j2msuratio";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "up stop") == 0)
         joymouseaxelratio = 1;
      else if (strcmp(var.value, "x5") == 0)
         joymouseaxelratio = 5;
      else if (strcmp(var.value, "x20") == 0)
         joymouseaxelratio = 20;
      else
         joymouseaxelratio = 10;
   }

  var.key = "np2kai_joy2mousekey";
  var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "Mouse") == 0)
      {
         joymouse = true;
         joy2key = false;
      }
      else if (strcmp(var.value, "Arrows") == 0)
      {
         joymouse = false;
         joy2key = true;
         memcpy(np2oscfg.lrjoybtn, joy2key_map_arr, sizeof(uint16_t) * 12);
      }
      else if (strcmp(var.value, "Keypad") == 0)
      {
         joymouse = false;
         joy2key = true;
         memcpy(np2oscfg.lrjoybtn, joy2key_map_kpad, sizeof(uint16_t) * 12);
      }
      else if (strcmp(var.value, "Manual") == 0)
      {
         joymouse = false;
         joy2key = true;
      }
      else
      {
         joymouse = false;
         joy2key = false;
      }
   }

   var.key = "np2kai_joynp2menu";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "A") == 0)
      {
         joyNP2menu = true;
         joyNP2menubtn = RETRO_DEVICE_ID_JOYPAD_A;
      }
      else if (strcmp(var.value, "B") == 0)
      {
         joyNP2menu = true;
         joyNP2menubtn = RETRO_DEVICE_ID_JOYPAD_B;
      }
      else if (strcmp(var.value, "X") == 0)
      {
         joyNP2menu = true;
         joyNP2menubtn = RETRO_DEVICE_ID_JOYPAD_X;
      }
      else if (strcmp(var.value, "Y") == 0)
      {
         joyNP2menu = true;
         joyNP2menubtn = RETRO_DEVICE_ID_JOYPAD_Y;
      }
      else if (strcmp(var.value, "L") == 0)
      {
         joyNP2menu = true;
         joyNP2menubtn = RETRO_DEVICE_ID_JOYPAD_L;
      }
      else if (strcmp(var.value, "L2") == 0)
      {
         joyNP2menu = true;
         joyNP2menubtn = RETRO_DEVICE_ID_JOYPAD_L2;
      }
      else if (strcmp(var.value, "R") == 0)
      {
         joyNP2menu = true;
         joyNP2menubtn = RETRO_DEVICE_ID_JOYPAD_R;
      }
      else if (strcmp(var.value, "R2") == 0)
      {
         joyNP2menu = true;
         joyNP2menubtn = RETRO_DEVICE_ID_JOYPAD_R2;
      }
      else if (strcmp(var.value, "L3") == 0)
      {
         joyNP2menu = true;
         joyNP2menubtn = RETRO_DEVICE_ID_JOYPAD_L3;
      }
      else if (strcmp(var.value, "R3") == 0)
      {
         joyNP2menu = true;
         joyNP2menubtn = RETRO_DEVICE_ID_JOYPAD_R3;
      }
      else if (strcmp(var.value, "Start") == 0)
      {
         joyNP2menu = true;
         joyNP2menubtn = RETRO_DEVICE_ID_JOYPAD_START;
      }
      else if (strcmp(var.value, "Select") == 0)
      {
         joyNP2menu = true;
         joyNP2menubtn = RETRO_DEVICE_ID_JOYPAD_SELECT;
      }
      else
      {
         joyNP2menu = false;
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

   init_lr_key_to_pc98();
}

void retro_deinit(void)
{
   np2_end();
}

void retro_reset (void)
{
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
   char np2path[4096];
   bool load_floppy=false;
   bool worked = environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &syspath);
   if(!worked)abort();

   if(game != NULL)
      extract_directory(base_dir, game->path, sizeof(base_dir));

   strcpy(np2path, syspath);
   lr_init = 1;

#ifdef _WIN32
   strcat(np2path, "\\np2kai");
#else
   strcat(np2path, "/np2kai");
#endif

   sprintf(tmppath,"%s%c",np2path,G_DIR_SEPARATOR);

   np2cfg.delayms = 0;

   sprintf(np2cfg.fontfile,"%s%cfont.bmp",np2path,G_DIR_SEPARATOR);
   file_setcd(np2cfg.fontfile);
   sprintf(np2cfg.biospath,"%s%c",np2path,G_DIR_SEPARATOR);

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

