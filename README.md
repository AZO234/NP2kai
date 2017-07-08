Neko Project II 0.86 kai rev.6
===
Jul 9, 2017  

Build SDL2 port
---

１． Install SDL2.  

	$ sudo apt-get install libsdl2-dev libsdl2-ttf-dev

２． Build.  

	$ cd NP2_SDL2/sdl2
	$ make -j4 -f Makefile.unix

or

	$ make -j4 -f Makefile21.unix

３． 'np2' or 'np21' binary is outputed in NP2kai/sdl2  

BIOS files locate in same directory executable file.

Build X11 port
---

１． Install SDL2.  

	$ sudo apt-get install libsdl2-dev libsdl2-ttf-dev

２． Build.  

	$ cd NP2kai/x11
	$ ./autogen.sh
	$ ./configure
	$ make

or

	$ ./configure --enable-ia32
	$ make -j4

３． 'np2' or 'np21' binary is outputed in NP2kai/x11  

BIOS files locate in same directory executable file.

NP2 menu is opened when F11 key or mouse middle button.  
NP2 menu can FDD/HDD swap.

Build libretro port (RetroArch)
---

１． Build.  

	$ cd NP2kai/sdl2
	$ make -j4

２． 'np2_libretro.so' binary is outputed in NP2kai/sdl2  

BIOS files locate in "np2" directory at BIOS directory (configured by RetroArch).  
Configure file (np2.cfg) is made in "np2" BIOS directory.

NP2 menu is opened when F12 key or mouse middle button.  
NP2 menu can FDD/HDD swap.  
Mouse is cuptured (hidden/show toggle) by F11 key.


Don't build other port. Maybe link errors occur.


BIOS files
---
* bios.rom
* FONT.ROM (big letter)
* itf.rom
* 2608_bd.wav
* 2608_sd.wav
* 2608_top.wav
* 2608_hh.wav
* 2608_tom.wav
* 2608_rim.wav

Setting to RetroPie
---

１．locate libretro & SDL2 port files.

    $ sudo mkdir /opt/retropie/libretrocores/lr-np2
    $ sudo cp np2_libretro.so /opt/retropie/libretrocores/lr-np2/
    $ mkdir ~/RetroPie/np2
    $ sudo cp np21 ~/RetroPie/np2/

２．locate BIOS files.

BIOS files locate in "&tilda;/RetroPie/roms/pc98/np2/" directory.  
and "&tilda;/RetroPie/np2/" too.

３．Add "carbon-mod". (Japanese nize)

    $ git clone https://github.com/eagle0wl/es-theme-carbon.git
    $ sudo cp -r ./es-theme-carbon /etc/emulationstation/themes/carbon-mod

４．Add to "/etc/emulationstation/es_systems.cfg" writing.

    $ sudo nano /etc/emulationstation/es_systems.cfg

    ...
      </system>
      <system>
        <name>pc98</name>
        <fullname>PC-98</fullname>
        <path>/home/pi/RetroPie/roms/pc98</path>
        <extension>.d88 .88d .d98 .98d .fdi .xdf .hdm .dup .2hd .tfd .nfd .hd4 .hd5 .hd9 .fdd .h01 .hdb .ddb .dd6 .dcp .dcu .flp .bin .fim .D88 .88D .D98 .98D .FDI .XDF .HDM .DUP .2HD .TFD .NFD .HD4 .HD5 .HD9 .FDD .H01 .HDB .DDB .DD6 .DCP .DCU .FLP .BIN .FIM .thd .nhd .hdi .vhd .sln .THD .NHD .HDI .VHD .SLN</extension>
        <command>/opt/retropie/supplementary/runcommand/runcommand.sh 0 _SYS_ pc98 %ROM%</command>
        <platform>pc98</platform>
        <theme>pc98</theme>
        <directlaunch/>
      </system>
      <system>
    ...

５．Add to "/opt/retropie/configs/pc98/emulators.cfg" writing.

    $ cd /opt/retropie/configs
    $ sudo mkdir pc98
    $ cd pc98
    $ sudo nano emulators.cfg

    np2="~/RetroPie/np2/np21 %ROM%"
    lr-np2="/opt/retropie/emulators/retroarch/bin/retroarch -L /opt/retropie/libretrocores/lr-np2/np2_libretro.so --config /opt/retropie/configs/pc98/retroarch.cfg %ROM%"
    default="np2"

６．Launch ES and set "CARBON-MOD" to "THEME-SET".

Release
---
* Jul 4, 2017
	- rename to 'kai'
* Jun 28, 2017
	- [libretro] Applicate to libretro port
* Jun 21, 2017
	- [X11] Applicate to X11 port
* Jun 20, 2017 (rev.6)
	- [NP21] fix for VGA
* Jun 19, 2017
	- [NP21] fix IA-32
	- more memory size available
* Jun 18, 2017
	- more avilable FDD/HDD/CD-ROM image
	- [NP21] FPU (fpemul_dosbox.c is GPL licence, others is MIT licence)
* Jun 12, 2017
	- COM
	- MIDI
	- JOYSTICK
	- IDE (can't use CD-ROM yet)
	- SDL_Keycode -&gt; SDL_Scancode
	- Save BMP
	- State Save
* Jun 4, 2017  
	- [NP21] お察しください
* Jun 1, 2017  
	- First release

Reference
---
* http://www.yui.ne.jp/np2
* https://github.com/irori/np2pi/wiki/NP2-for-Raspberry-Pi
* http://eagle0wl.hatenadiary.jp/entry/2016/10/07/213830
* https://sites.google.com/site/np21win/home
* https://github.com/meepingsnesroms/libretro-meowPC98

