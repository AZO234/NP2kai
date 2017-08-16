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

#include "libretro.h"
#include "libretro_params.h"

#include "compiler.h"//required to prevent missing type errors
#include "pccore.h"
#include "keystat.h"
#include "fddfile.h"
#include "newdisk.h"
#include "diskdrv.h"
#include "fontmng.h"
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
#include "sysmenu.h"
#include "milstr.h"
#include "strres.h"
#include "np2.h"

signed short soundbuf[SNDSZ*2]; //16bit*2ch

char RPATH[512];
char tmppath[4096];

static retro_log_printf_t log_cb = NULL;
static retro_video_refresh_t video_cb = NULL;
static retro_input_poll_t poll_cb = NULL;
retro_input_state_t input_cb = NULL;
retro_environment_t environ_cb = NULL;

uint16_t   FrameBuffer[LR_SCREENWIDTH * LR_SCREENHEIGHT];
uint16_t   GuiBuffer[LR_SCREENWIDTH * LR_SCREENHEIGHT]; //menu surf
extern SCRNSURF	scrnsurf;

retro_audio_sample_batch_t audio_batch_cb = NULL;

static char CMDFILE[512];

bool did_reset, joy2key;

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

void Add_Option(const char* option)
{
   static int first=0;

   if(first==0)
   {
      PARAMCOUNT=0;
      first++;
   }

   sprintf(XARGV[PARAMCOUNT++],"%s\0",option);
}

int pre_main(const char *argv)
{
   int i=0;
   int Only1Arg;

   if (strlen(argv) > strlen("cmd"))
   {
      if( HandleExtension((char*)argv,"cmd") || HandleExtension((char*)argv,"CMD"))
         i=loadcmdfile((char*)argv);
   }

   if(i==1)
      parse_cmdline(CMDFILE);
   else
      parse_cmdline(argv);

   Only1Arg = (strcmp(ARGUV[0],"np2") == 0) ? 0 : 1;

   for (i = 0; i<64; i++)
      xargv_cmd[i] = NULL;


   if(Only1Arg)
   {
      int cfgload=0;

      Add_Option("np2");

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

void DrawPointBmp(unsigned short *buffer,int x, int y, unsigned short color)
{
   int idx;

   if(x>=0&&y>=0&&x<scrnsurf.width&&y<scrnsurf.height) {
      idx=x+y*scrnsurf.width;
      buffer[idx]=color;
   }
}


void draw_cross(int x,int y) {

	int i,j,idx;
	int dx=32,dy=20;
	unsigned  short color;

	for(j=y;j<y+dy;j++){
		idx=0;
		for(i=x;i<x+dx;i++){
			if(cross[j-y][idx]=='.')DrawPointBmp(FrameBuffer,i,j,0xffff);
			else if(cross[j-y][idx]=='X')DrawPointBmp(FrameBuffer,i,j,0);
			idx++;
		}
	}

}

static int lastx=320,lasty=240;
static menukey=0;
static menu_active=0;
static int mbL = 0, mbR = 0;
static bool joymouse;
static int joymousemovebtn = 0;
static double joymouseaxel = 1.0;

bool mapkey_down[12];

static int joy2key_map[12][2] = { 
   {RETRO_DEVICE_ID_JOYPAD_UP,     RETROK_UP},
   {RETRO_DEVICE_ID_JOYPAD_DOWN,   RETROK_DOWN},
   {RETRO_DEVICE_ID_JOYPAD_LEFT,   RETROK_LEFT},
   {RETRO_DEVICE_ID_JOYPAD_RIGHT,  RETROK_RIGHT},
   {RETRO_DEVICE_ID_JOYPAD_A,      RETROK_x},
   {RETRO_DEVICE_ID_JOYPAD_B,      RETROK_z},
   {RETRO_DEVICE_ID_JOYPAD_X,      RETROK_SPACE},
   {RETRO_DEVICE_ID_JOYPAD_Y,      RETROK_LCTRL},
   {RETRO_DEVICE_ID_JOYPAD_L,      RETROK_BACKSPACE},
   {RETRO_DEVICE_ID_JOYPAD_R,      RETROK_RSHIFT},
   {RETRO_DEVICE_ID_JOYPAD_SELECT, RETROK_ESCAPE},
   {RETRO_DEVICE_ID_JOYPAD_START,  RETROK_RETURN}
};
   
void updateInput(){

   static int mposx=LR_SCREENWIDTH/2,mposy=LR_SCREENHEIGHT/2;

   poll_cb();

   joymng_sync();

   uint32_t i;
   
   if (joy2key)
   {
      for (i = 0; i < 12; i++)
      {
         if (input_cb(0, RETRO_DEVICE_JOYPAD, 0, joy2key_map[i][0]) && !mapkey_down[i] )
         {
            send_libretro_key_down(joy2key_map[i][1]);
            mapkey_down[i] = 1;
         }
         else if (!input_cb(0, RETRO_DEVICE_JOYPAD, 0, joy2key_map[i][0]) )
         {
            send_libretro_key_up(joy2key_map[i][1]);
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
      input_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2) ||
      input_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_MIDDLE)) && menukey==0){

      menukey=1;

      if (menuvram == NULL) {
         memcpy(GuiBuffer,FrameBuffer,scrnsurf.width*scrnsurf.height*2);
         sysmenu_menuopen(0, 0, 0);
         mposx=0;mposy=0;
         lastx=0;lasty=0;
         menu_active=1;
      } else {
         menubase_close();
         scrndraw_redraw();
         menu_active=0;
      }
   } else if ( !(input_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_F12) ||
      input_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2) ||
      input_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_MIDDLE)) && menukey==1)
      menukey=0;

   if (!joymouse) {
      int mouse_x = input_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X);
      int mouse_y = input_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_Y);

      if (menuvram == NULL)
         mousemng_sync(mouse_x,mouse_y);

      mposx+=mouse_x;if(mposx<0)mposx=0;if(mposx>=scrnsurf.width)mposx=scrnsurf.width-1;
      mposy+=mouse_y;if(mposy<0)mposy=0;if(mposy>=scrnsurf.height)mposy=scrnsurf.height-1;

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
      joymouseaxel += 0.1;

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
      if(mposx >= scrnsurf.width)
         mposx = scrnsurf.width - 1;

      mposy += mouse_y;
      if(mposy < 0)
         mposy = 0;
      if(mposy >= scrnsurf.height)
         mposy = scrnsurf.height - 1;

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

//dummy functions
void *retro_get_memory_data(unsigned type){return NULL;}
size_t retro_get_memory_size(unsigned type){return 0;}
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

/* media support */
enum{
   MEDIA_TYPE_DISK = 0,
   MEDIA_TYPE_HD,
   MEDIA_TYPE_CD,
   MEDIA_TYPE_OTHER
};

void lowerstring(char* str)
{
   int i;
   for (i=0; str[i]; i++)
   {
      str[i] = tolower(str[i]);
   }
}

int getmediatype(const char* filename){
   char workram[4096/*max path name length for all existing OSes*/];
   strcpy(workram,filename);
   lowerstring(workram);
   const char* extension = workram + strlen(workram) - 4;

   if(strcasecmp(extension,".d88") == 0){
      return MEDIA_TYPE_DISK;
   }
   else if(strcasecmp(extension,".88d") == 0){
      return MEDIA_TYPE_DISK;
   }
   else if(strcasecmp(extension,".d98") == 0){
      return MEDIA_TYPE_DISK;
   }
   else if(strcasecmp(extension,".98d") == 0){
      return MEDIA_TYPE_DISK;
   }
   else if(strcasecmp(extension,".fdi") == 0){
      return MEDIA_TYPE_DISK;
   }
   else if(strcasecmp(extension,".xdf") == 0){
      return MEDIA_TYPE_DISK;
   }
   else if(strcasecmp(extension,".hdm") == 0){
      return MEDIA_TYPE_DISK;
   }
   else if(strcasecmp(extension,".dup") == 0){
      return MEDIA_TYPE_DISK;
   }
   else if(strcasecmp(extension,".2hd") == 0){
      return MEDIA_TYPE_DISK;
   }
   else if(strcasecmp(extension,".tfd") == 0){
      return MEDIA_TYPE_DISK;
   }
   else if(strcasecmp(extension,".nfd") == 0){
      return MEDIA_TYPE_DISK;
   }
   else if(strcasecmp(extension,".hd4") == 0){
      return MEDIA_TYPE_DISK;
   }
   else if(strcasecmp(extension,".hd5") == 0){
      return MEDIA_TYPE_DISK;
   }
   else if(strcasecmp(extension,".hd9") == 0){
      return MEDIA_TYPE_DISK;
   }
   else if(strcasecmp(extension,".fdd") == 0){
      return MEDIA_TYPE_DISK;
   }
   else if(strcasecmp(extension,".h01") == 0){
      return MEDIA_TYPE_DISK;
   }
   else if(strcasecmp(extension,".hdb") == 0){
      return MEDIA_TYPE_DISK;
   }
   else if(strcasecmp(extension,".ddb") == 0){
      return MEDIA_TYPE_DISK;
   }
   else if(strcasecmp(extension,".dd6") == 0){
      return MEDIA_TYPE_DISK;
   }
   else if(strcasecmp(extension,".dup") == 0){
      return MEDIA_TYPE_DISK;
   }
   else if(strcasecmp(extension,".flp") == 0){
      return MEDIA_TYPE_DISK;
   }
   else if(strcasecmp(extension,".bin") == 0){
      return MEDIA_TYPE_DISK;
   }
   else if(strcasecmp(extension,".fim") == 0){
      return MEDIA_TYPE_DISK;
   }
   else if(strcasecmp(extension,".thd") == 0){
      return MEDIA_TYPE_HD;
   }
   else if(strcasecmp(extension,".nhd") == 0){
      return MEDIA_TYPE_HD;
   }
   else if(strcasecmp(extension,".hdi") == 0){
      return MEDIA_TYPE_HD;
   }
   else if(strcasecmp(extension,".vhd") == 0){
      return MEDIA_TYPE_HD;
   }
   else if(strcasecmp(extension,".sln") == 0){
      return MEDIA_TYPE_HD;
   }
   else if(strcasecmp(extension,".iso") == 0){
      return MEDIA_TYPE_CD;
   }
   else if(strcasecmp(extension,".cue") == 0){
      return MEDIA_TYPE_CD;
   }
   else if(strcasecmp(extension,".ccd") == 0){
      return MEDIA_TYPE_CD;
   }
   else if(strcasecmp(extension,".cdm") == 0){
      return MEDIA_TYPE_CD;
   }
   else if(strcasecmp(extension,".mds") == 0){
      return MEDIA_TYPE_CD;
   }
   else if(strcasecmp(extension,".nrg") == 0){
      return MEDIA_TYPE_CD;
   }

   return MEDIA_TYPE_OTHER;
}
/* end media support */
/* media swap support */
struct retro_disk_control_callback dskcb;
bool diskinserted[5] = {true};
static unsigned drvno = 0;
static unsigned mediaidx = 0;

//all the fake functions used to limit swapping to 1 disk drive
bool setdskeject(bool ejected){
   if(ejected) {
      switch(drvno) {
      case 0:
         if(np2cfg.fddfile[0][0] != '\0') {
            diskdrv_setfdd(0, NULL, 0);
            return true;
         } else {
            return false;
         }
      case 1:
         if(np2cfg.fddfile[1][0] != '\0') {
            diskdrv_setfdd(1, NULL, 0);
            return true;
         } else {
            return false;
         }
         break;
      case 2:
         if(np2cfg.sasihdd[0][0] != '\0') {
            diskdrv_setsxsi(0x00, NULL);
            return true;
         } else {
            return false;
         }
         break;
      case 3:
         if(np2cfg.sasihdd[1][0] != '\0') {
            diskdrv_setsxsi(0x00, NULL);
            return true;
         } else {
            return false;
         }
         break;
      case 4:
         if(np2cfg.sasihdd[2][0] != '\0') {
            diskdrv_setsxsi(0x00, NULL);
            return true;
         } else {
            return false;
         }
         break;
      }
   }
   return false;
}

bool getdskeject(){
   return !diskinserted[drvno];
}

unsigned getdskindex(){
   return mediaidx;
}

bool setdskindex(unsigned index){
   mediaidx = index;
   return true;
}

unsigned getnumimages(){
   return mediaidx;
}

bool addimageindex() {
   mediaidx++;
   return true;
}



//this is the only real function,it will swap out the disk
bool replacedsk(unsigned index,const struct retro_game_info *info){
   switch(drvno) {
   case 0:
      if(getmediatype(info->path) != MEDIA_TYPE_DISK) return false;
      strcpy(np2cfg.fddfile[0], info->path);
      return true;
   case 1:
      if(getmediatype(info->path) != MEDIA_TYPE_DISK) return false;
      strcpy(np2cfg.fddfile[1], info->path);
      return true;
   case 2:
      if(getmediatype(info->path) != MEDIA_TYPE_HD) return false;
      strcpy(np2cfg.sasihdd[0], info->path);
      return true;
   case 3:
      if(getmediatype(info->path) != MEDIA_TYPE_HD) return false;
      strcpy(np2cfg.sasihdd[1], info->path);
      return true;
   case 4:
      if(getmediatype(info->path) != MEDIA_TYPE_CD) return false;
      strcpy(np2cfg.sasihdd[2], info->path);
      return true;
   }

   return false;
}

void attachdiskswapinterface(){
   //these functions are unused
   dskcb.set_eject_state = setdskeject;
   dskcb.get_eject_state = getdskeject;
   dskcb.set_image_index = setdskindex;
   dskcb.get_image_index = getdskindex;
   dskcb.get_num_images  = getnumimages;
   dskcb.add_image_index = addimageindex;

   //this is the only real function,it will swap out the disk
   dskcb.replace_image_index = replacedsk;

   environ_cb(RETRO_ENVIRONMENT_SET_DISK_CONTROL_INTERFACE,&dskcb);
}
/* end media swap support */

void retro_set_environment(retro_environment_t cb)
{
   struct retro_log_callback logging;
   BOOL allow_no_game = true;

   environ_cb = cb;

   //bool no_rom = !LR_REQUIRESROM;
   //environ_cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_rom);

   struct retro_variable variables[] = {
//      { "np2_drive" , "Drive; FDD1|FDD2|IDE1|IDE2|CD-ROM" },
      { "np2_model" , "PC Model (Restart); PC-9801VX|PC-286|PC-9801VM" },
      { "np2_clk_base" , "CPU Base Clock (Restart); 2.4576 MHz|1.9968 MHz" },
      { "np2_clk_mult" , "CPU Clock Multiplier (Restart); 4|5|6|8|10|12|16|20|24|30|36|40|42|1|2" },
      { "np2_ExMemory" , "RAM Size (Restart); 3|7|11|13|16|32|64|120|230|1" },
      { "np2_skipline" , "Skipline Revisions; Full 255 lines|ON|OFF" },
      { "np2_SNDboard" , "Sound Board (Restart); PC9801-26K|PC9801-86|PC9801-26K + 86|PC9801-86 + Chibi-oto|PC9801-118|Speak Board|Spark Board|Sound Orchestra|Sound Orchestra-V|AMD-98|None|PC9801-14" },
      { "np2_jast_snd" , "JastSound; OFF|ON" },
      { "np2_sndgen" , "Sound Generator; Normal|fmgen" },
      { "np2_volume_F" , "Volume FM; 64|68|72|76|80|84|88|92|96|100|104|108|112|116|120|124|128|0|4|8|12|16|20|24|28|32|36|40|44|48|52|56|60" },
      { "np2_volume_S" , "Volume SSG; 64|68|72|76|80|84|88|92|96|100|104|108|112|116|120|124|128|0|4|8|12|16|20|24|28|32|36|40|44|48|52|56|60" },
      { "np2_volume_A" , "Volume ADPCM; 64|68|72|76|80|84|88|92|96|100|104|108|112|116|120|124|128|0|4|8|12|16|20|24|28|32|36|40|44|48|52|56|60" },
      { "np2_volume_P" , "Volume PCM; 64|68|72|76|80|84|88|92|96|100|104|108|112|116|120|124|128|0|4|8|12|16|20|24|28|32|36|40|44|48|52|56|60" },
      { "np2_volume_R" , "Volume RHYTHM; 64|68|72|76|80|84|88|92|96|100|104|108|112|116|120|124|128|0|4|8|12|16|20|24|28|32|36|40|44|48|52|56|60" },
      { "np2_Seek_Snd" , "Floppy Seek Sound; OFF|ON" },
      { "np2_Seek_Vol" , "Volume Floppy Seek; 80|84|88|92|96|100|104|108|112|116|120|124|128|0|4|8|12|16|20|24|28|32|36|40|44|48|52|56|60|64|68|72|76" },
      { "np2_BEEP_vol" , "Volume Beep; 3|0|1|2" },
      { "np2_joy2mouse" , "Joypad to Mouse Mapping; OFF|ON" },
      { "np2_joy2key" , "Joypad to Keyboard Mapping; OFF|ON" },
      { NULL, NULL },
   };

   environ_cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &allow_no_game);

   if (environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logging))
      log_cb = logging.log;
   else
      log_cb = NULL;

   cb(RETRO_ENVIRONMENT_SET_VARIABLES, variables);
}

static void update_variables(void)
{
   struct retro_variable var = {0};

   var.key = "np2_drive";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "FDD1") == 0)
         drvno = 0;
      else if (strcmp(var.value, "FDD2") == 0)
         drvno = 1;
      else if (strcmp(var.value, "IDE1") == 0)
         drvno = 2;
      else if (strcmp(var.value, "IDE2") == 0)
         drvno = 3;
      else if (strcmp(var.value, "CD-ROM") == 0)
         drvno = 4;
   }

   var.key = "np2_model";
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

   var.key = "np2_clk_base";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "1.9968 MHz") == 0)
         np2cfg.baseclock = 1996800;
      else
         np2cfg.baseclock = 2457600;
   }

   var.key = "np2_clk_mult";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      np2cfg.multiple = atoi(var.value);
   }

   var.key = "np2_ExMemory";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      np2cfg.EXTMEM = atoi(var.value);
   }

   var.key = "np2_skipline";
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

   var.key = "np2_SNDboard";
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
      else if (strcmp(var.value, "Speak Board") == 0)
         np2cfg.SOUND_SW = 0x20;
      else if (strcmp(var.value, "Spark Board") == 0)
         np2cfg.SOUND_SW = 0x40;
      else if (strcmp(var.value, "Sound Orchestra") == 0)
         np2cfg.SOUND_SW = 0x32;
      else if (strcmp(var.value, "Sound Orchestra-V") == 0)
         np2cfg.SOUND_SW = 0x82;
      else if (strcmp(var.value, "AMD-98") == 0)
         np2cfg.SOUND_SW = 0x80;
      else if (strcmp(var.value, "None") == 0)
         np2cfg.SOUND_SW = 0x00;
      else if (strcmp(var.value, "PC9801-14") == 0)
         np2cfg.SOUND_SW = 0x01;
      else if (strcmp(var.value, "PC9801-26K") == 0)
         np2cfg.SOUND_SW = 0x02;
   }

   var.key = "np2_jast_snd";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "OFF") == 0)
         np2oscfg.jastsnd = 0;
      else if (strcmp(var.value, "ON") == 0)
         np2oscfg.jastsnd = 1;
   }

   var.key = "np2_sndgen";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "Normal") == 0)
         np2cfg.sndgen = 0x00;
      else if (strcmp(var.value, "fmgen") == 0)
         np2cfg.sndgen = 0x01;
   }

   var.key = "np2_volume_F";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      np2cfg.vol_fm = atoi(var.value);
      opngen_setvol(np2cfg.vol_fm);
      oplgen_setvol(np2cfg.vol_fm);
   }

   var.key = "np2_volume_S";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      np2cfg.vol_ssg = atoi(var.value);
      psggen_setvol(np2cfg.vol_ssg);
   }

   var.key = "np2_volume_A";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      np2cfg.vol_adpcm = atoi(var.value);
      adpcm_setvol(np2cfg.vol_adpcm);
   }

   var.key = "np2_volume_P";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      np2cfg.vol_pcm = atoi(var.value);
      pcm86gen_setvol(np2cfg.vol_pcm);
   }

   var.key = "np2_volume_R";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      np2cfg.vol_rhythm = atoi(var.value);
      rhythm_setvol(np2cfg.vol_rhythm);
   }

   var.key = "np2_Seek_Snd";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "OFF") == 0)
         np2cfg.MOTOR = false;
      else if (strcmp(var.value, "ON") == 0)
         np2cfg.MOTOR = true;
   }

   var.key = "np2_Seek_Vol";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      np2cfg.MOTORVOL = atoi(var.value);
   }

   var.key = "np2_BEEP_vol";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      np2cfg.BEEP_VOL = atoi(var.value);
      beep_setvol(np2cfg.BEEP_VOL);
   }

   var.key = "np2_joy2mouse";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "ON") == 0)
         joymouse = true;
      else
         joymouse = false;
   }

   var.key = "np2_joy2key";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "ON") == 0)
         joy2key = true;
      else
         joy2key = false;
   }

   initsave();

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
   info->geometry.base_width   = scrnsurf.width;
   info->geometry.base_height  = scrnsurf.height;
   info->geometry.max_width    = scrnsurf.width;
   info->geometry.max_height   = scrnsurf.height;
   info->geometry.aspect_ratio = (double)scrnsurf.width / scrnsurf.height;
   info->timing.fps            = LR_SCREENFPS;
   info->timing.sample_rate    = LR_SOUNDRATE;
}

void retro_init (void)
{
   enum retro_pixel_format rgb565;

   scrnsurf.width = 640;
   scrnsurf.height = 400;

   rgb565 = RETRO_PIXEL_FORMAT_RGB565;
   if(environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &rgb565) && log_cb)
         log_cb(RETRO_LOG_INFO, "Frontend supports RGB565 - will use that instead of XRGB1555.\n");

   update_variables();

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
      memcpy(FrameBuffer,GuiBuffer,scrnsurf.width*scrnsurf.height*2);
      draw_cross(lastx,lasty);
   }
   else {
      //emulate 1 frame
      pccore_exec(true /*draw*/);
      sound_play_cb(NULL, NULL,SNDSZ*4);
   }

   video_cb(FrameBuffer, scrnsurf.width, scrnsurf.height, scrnsurf.width * 2/*Pitch*/);
}

size_t retro_serialize_size (void)
{
   //no savestates on this core
   return 0;
}

bool retro_serialize(void *data, size_t size)
{
   //no savestates on this core
   return false;
}

bool retro_unserialize(const void * data, size_t size)
{
   //no savestates on this core
   return false;
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

   strcpy(np2path, syspath);

#ifdef _WIN32
   strcat(np2path, "\\np2");
#else
   strcat(np2path, "/np2");
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

   attachdiskswapinterface();

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
