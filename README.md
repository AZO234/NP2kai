Neko Project II 0.86 kai rev.15
===
Feb 6, 2018  

Build SDL2 port
---

１． Install SDL2.  

	(Linux)
	$ sudo apt-get install libsdl2-dev libsdl2-ttf-dev libsdl2-mixer-dev

or

	(Windows + MSYS2)
	$ pacman -S mingw-w64-x86_64-SDL2 mingw-w64-x86_64-SDL2_ttf mingw-w64-x86_64-SDL2_mixer

２． Build.  

	$ cd NP2kai/sdl2
	$ make -j4 -f Makefile.unix

or

	$ make -j4 -f Makefile21.unix

３． Install NP2kai.  

	$ sudo -f Makefile.unix install

or

	$ sudo -f Makefile21.unix install

４． Run NP2kai.  

	$ np2kai

or

	$ np21kai

BIOS files locate in ~/.config/np2kai .

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

３． Install NP2kai.  

	$ sudo make install

４． Run NP2kai.  

	$ xnp2kai

BIOS files locate in ~/.config/xnp2kai .

NP2 menu is opened when F11 key or mouse middle button.  
NP2 menu can swap FDD/HDD diskimages.

Build libretro port (Linux)
---

１． Build.  

	$ cd NP2kai/sdl2
	$ make -j4

２． 'np2kai_libretro.so' binary is outputed in NP2kai/sdl2  

Build libretro port (Windows)
---

１． Build by MSYS2 x86_64. (Reference follow URL)  

https://bot.libretro.com/docs/compilation/windows/

	$ cd NP2kai/sdl2
	$ make -j4

２． 'np2kai_libretro.dll' binary is outputed in NP2kai/sdl2  

Build libretro port (Mac)  * No confirmation to drive
---

１． Open terminal.

２． First 'git' to install XCode.

	$ git

３． Clone this repository

	$ git clone https://github.com/AZO234/NP2kai

４． Build.

	$ cd NP2kai/sdl2
	$ make -j4

５． 'np2kai_libretro.dylib' binary is outputed in NP2kai/sdl2  

Build with libretro-super  * No confirmation to drive
---

１． Open terminal.

２． Clone libretro-super repository.

	$ git clone --depth 1 https://github.com/libretro/libretro-super.git

３． CD to libretro-super, and clone this repository.

	$ cd libretro-super
	$ git clone --depth 1 https://github.com/AZO234/NP2kai libretro-np2kai

４． Build. (e.g. Android)

To build needs Android Studio, and NDK.  
https://developer.android.com/studio/index.html

	$ export PATH=~/Android/Sdk/ndk-bundle:$PATH
	$ chmod +x ./libretro-build-android-mk.sh
	$ ./libretro-build-android-mk.sh np2kai

'libnp2kai.so' binary is outputed in libretro-np2kai/libs  

４． Build. (e.g. iOS)

	$ ./libretro-build-ios.sh np2kai

'np2kai_libretro_ios.dylib' binary is outputed in libretro-np2kai  

about libretro port
---
BIOS files locate in "np2kai" directory at BIOS directory (configured by RetroArch).  
Configure file (np2.cfg) is made in "np2kai" BIOS directory.

NP2 menu is opened when F12 key or mouse middle button or joypad L2 button.  
NP2 menu can swap FDD/HDD diskimages.  

Mouse is cuptured (hidden/show toggle) by F11 key.

Mouse cursor is able to move with joypad when Joy2Mouse mode.  
Switch Joy2Mouse mode in config.  
D-UP/DOWN/LEFT/RIGHT: mouse move  
B button: left click  
A button: right click
R button: mouse speed up durling hold  

Keyboard is able to control with joypad when Joy2Key mode.  
Switch Joy2Key mode in config.  
D-UP/DOWN/LEFT/RIGHT: Arrow key or Keypad(2468) key 
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
* font.rom or font.bmp
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
        <extension>.d88 .88d .d98 .98d .fdi .xdf .hdm .dup .2hd .tfd .nfd .hd4 .hd5 .hd9 .fdd .h01 .hdb .ddb .dd6 .dcp .dcu .flp .bin .fim .img .ima .D88 .88D .D98 .98D .FDI .XDF .HDM .DUP .2HD .TFD .NFD .HD4 .HD5 .HD9 .FDD .H01 .HDB .DDB .DD6 .DCP .DCU .FLP .BIN .FIM .IMG .IMA .thd .nhd .hdi .vhd .sln .hdn .hdd .THD .NHD .HDI .VHD .SLN .HDN .HDD .m3u .M3U</extension>
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

fmgen: fmgen sound generator.
Default: default sound generator.  

Don't forget restart core.

* How to set GDC 2.5MHz/5MHz?  

1.Press End key(assigned Help key) + reset
2.Select 'ディップスイッチ２'(DIP switch 2)

* How to key typing?  

There are two ways:

1. map the 'enable hotkeys' hotkey in settings > input > input hotkey binds and RetroArch will stop listening for hotkeys unless/until you hold that button/key

2. enable the "game focus mode" (mapped to scroll_lock by default) and it will send all of your inputs to the core instead of the frontend. However, some people have reported having trouble getting out of game focus mode.

(Thanks hasenbanck)

* How to use CD drive with MS-DOS 6.2?  

Write follow to CONFIG.SYS. 

    LASTDRIVE=Z
    DEVICE=A:￥DOS￥NECCDD.SYS /D:CD_101

And write follow to AUTOEXEC.BAT. 

    A:￥DOS￥MSCDEX.EXE /D:CD_101 /L:Q

Then, you'll can use CD drive as Q drive.

About network
---
From rev.7 we can use network.

NIC is follow spec.  

MELCO LGY-98  
IRQ:6  
I/O:0x00D0

Release
---
* Feb 20, 2018
	- FONT.ROM/FONT.BMP can be loaded lower case.
* Feb 19, 2018
	- [libretro] Apply disk swap interface
* Feb 17, 2018
	- [X11] Mouse moving is more smopothly (Thanks frank-deng)
	- Using *.img *.ima type floppy image (Thanks frank-deng)
* Feb 14, 2018
	- Fix parse CUE sheet (Thanks frank-deng)
* Feb 6, 2018 (rev.15)
	- NP2 namespace change to NP2kai
	- [SDL2] Locate of config files is ~/.config/np2kai
	- [X11] Locate of config files is ~/.config/xnp2kai
* Feb 5, 2018
	- Merge NP21/W 0.86 rev.38
* Feb 4, 2018
	- Add setting Joy to Mouse cursor speed up rasio
* Dec 5, 2017
	- Default GDC clock is 2.5MHz
	- In Joy2Mousem, mouse speed up with R button
* Nov 18, 2017
	- Merge NP21/W 0.86 rev.37
* Oct 26, 2017
	- Apply to 1.44MB FDD floppy image file
* Oct 21, 2017 (rev.14)
	- Merge NP21/W 0.86 rev.36
		* Mate-X PCM  
		* Sound Blaster 16  
		* OPL3 (MAME codes is GPL licence)  
		* Auto IDE BIOS  
* Oct 16, 2017 (rev.13)
	- Refix BEEP PCM
* Oct 2, 2017 (rev.13)
	- remove CDDA mod
* Sep 20, 2017
	- [SDL2] Use SDL2 mixer
* Sep 16, 2017
	- Add RaSCSI hdd image file support
* Sep 14, 2017 (rev.12)
	- [libretro] (newest core binary is auto released by buildbot)
	- Fix triple fault case
* Sep 7, 2017
	- Fix BEEP PCM
* Aug 27, 2017 (rev.11)
	- Merge NP21/w rev.35 beta2
	- [libretro] state save/load
* Aug 23, 2017 (rev.10)
	- Merge NP21/w rev.35 beta1
* Aug 22, 2017
	- Merge 私家  
		Merged  
		* AMD-98 Joyport  
		* S98V3  
		* Otomichanx2  
		* V30 patch  
		* VAEG fix  
		* CSM voice  
		Couldn't merged  
		* LittleOrchestra  
		* MultimediaOrchestra  
		* WaveRec  
	- Merge kaiE
		* force ROM to RAM 
		* CDDA fix
		* Floppie fix
* Aug 21, 2017
	- Apply libretro-super build
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

