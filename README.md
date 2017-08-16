Neko Project II 0.86 kai rev.9
===
Aug 17, 2017  

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

Build libretro port (Linux)
---

１． Build.  

	$ cd NP2kai/sdl2
	$ make -j4

２． 'np2_libretro.so' binary is outputed in NP2kai/sdl2  

Build libretro port (Windows)
---

１． Build by MSYS2 x86_64. (Reference follow URL)  

https://bot.libretro.com/docs/compilation/windows/

	$ cd NP2kai/sdl2
	$ make -j4

２． 'np2_libretro.dll' binary is outputed in NP2kai\sdl2  

about libretro port
---
BIOS files locate in "np2" directory at BIOS directory (configured by RetroArch).  
Configure file (np2.cfg) is made in "np2" BIOS directory.

NP2 menu is opened when F12 key or mouse middle button or joypad L2 button.  
NP2 menu can FDD/HDD swap.  

Mouse is cuptured (hidden/show toggle) by F11 key.

Mouse cursor is able to move with joypad when Joy2Mouse mode.  
Switch Joy2Mouse mode in config.  
D-UP/DOWN/LEFT/RIGHT: mouse move  
B button: left click  
A button: right click

Keyboard is able to control with joypad when Joy2Key mode.  
Switch Joy2Key mode in config.  
D-UP/DOWN/LEFT/RIGHT: allow key  
B button: Z key  
A button: X key  
X button: Space key  
Y button: left Ctrl key  
L button: Backspace key  
R button: right Shift key  
Select button: Escape key  
Start button: Return key

If you use 104 western keyboard,  
to input underscore(_), press Shift+right Ctrl.

BIOS files
---
* bios.rom
* FONT.ROM (big letter)
* itf.rom
* sound.rom
* (bios9821.rom or d8000.rom  But I never see good dump file.)
* 2608_bd.wav
* 2608_sd.wav
* 2608_top.wav
* 2608_hh.wav
* 2608_tom.wav
* 2608_rim.wav

Setting to RetroPie
---

１．Install Japanese font. (umefont need SDL2 port only)

    $ sudo apt-get install fonts-droid fonts-horai-umefont

２．Locate libretro & SDL2 port files.

    $ sudo mkdir /opt/retropie/libretrocores/lr-np2
    $ sudo cp np2_libretro.so /opt/retropie/libretrocores/lr-np2/
    $ sudo mkdir /opt/retropie/emulators/np2
    $ sudo cp np2_libretro.so /opt/retropie/emulators/np2/
    $ sudo touch /opt/retropie/emulators/np2/np2.cfg
    $ sudo chmod 666 /opt/retropie/emulators/np2/np2.cfg

３．Write & locate retroarch.cfg.

    $ sudo vi /opt/retropie/configs/pc98/retroarch.cfg

    # Settings made here will only override settings in the global retroarch.cfg if placed above the #include line
    
    input_remapping_directory = "/opt/retropie/configs/pc98/"
    
    #include "/opt/retropie/configs/all/retroarch.cfg"

４．Locate BIOS files.

BIOS files locate in "&tilde;/RetroPie/BIOS/np2/" directory.  
and "/opt/retropie/emulators/np2/" too.

５．Make shortcut to Japanese font. (SDL2 port only)

    $ sudo ln -s /usr/share/fonts/truetype/horai-umefont/ume-ugo4.ttf /opt/retropie/emulators/np2/default.ttf

６．Add "carbon-mod". (Japanese nize)
    $ git clone https://github.com/eagle0wl/es-theme-carbon.git
    $ sudo cp -r ./es-theme-carbon /etc/emulationstation/themes/carbon-mod

７．Add to "/etc/emulationstation/es_systems.cfg" writing.

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

８．Add to "/opt/retropie/configs/pc98/emulators.cfg" writing.

    $ cd /opt/retropie/configs
    $ sudo mkdir pc98
    $ cd pc98
    $ sudo nano emulators.cfg

    np2="/opt/retropie/emulators/np2/np21 %ROM%"
    lr-np2="/opt/retropie/emulators/retroarch/bin/retroarch -L /opt/retropie/libretrocores/lr-np2/np2_libretro.so --config /opt/retropie/configs/pc98/retroarch.cfg %ROM%"
    default="lr-np2"

９．Launch ES and set "CARBON-MOD" to "THEME-SET".

Point of tuning performance NP2kai
---
* CPU clock  
Change "CPU Clock Multiplyer".

* Memory size  
Change "RAM Size".

* Sound device  
26K: for old games.  
86: for newer games.

* Sound Generator
Normal: default generator.
fmgen: external generator.

Don't forget restart core.

About network
---
From rev.7 we can use network.

NIC is follow spec.  

MELCO LGY-98  
IRQ:6  
I/O:0x00D0

Release
---
* Aug 17, 2017 (rev.9)
	- Apply fmgen
* Aug 3, 2017 (rev.8)
	- Apply HRTIMER
	- [libretro] input underscore(_) for western keyboard
	- [libretro] Add Joy2Key (thanks Tetsuya79)
* Jul 24, 2017 (rev.7)
	- Apply network
* Jul 18, 2017 (rev.6)
* Jul 17, 2017
	- Apply HOSTDRV
	- [libretro] Add Joy2Mouse mode (switch at config menu)
* Jul 4, 2017
	- rename to 'kai'
* Jun 28, 2017
	- [libretro] Applicate to libretro port
* Jun 21, 2017
	- [X11] Applicate to X11 port
* Jun 20, 2017 (rev.6 beta)
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

