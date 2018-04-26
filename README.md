Neko Project II 0.86 kai rev.16
===
Apr 2, 2018  

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

or for GCW Zero (GCW0) game device only

	$ make -j4 -f Makefile21.gcw0



３． Install NP2kai.  

	$ sudo -f Makefile.unix install

or

	$ sudo -f Makefile21.unix install

or for GCW Zero (GCW0) game device only

	$ make -j4 -f Makefile21.gcw0 buildopk

	then transfer np2kai.opk to /media/apps/
	BIOS files should locate in ~/.config/np2kai

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

About libretro port
---
BIOS files locate in "np2kai" directory at BIOS directory (configured by RetroArch).  
Configure file (np2.cfg) is made in "np2kai" BIOS directory.

NP2 menu is opened when F12 key or mouse middle button or joypad L2 button.  
NP2 menu can swap FDD/HDD diskimages.  

If you run libretro by .m3u file include floppy disk images,  
F1 key menu durling play game, you can swap disk.  
[Disk Control] -> [Disk Cycle Tray Status] (eject) -> [Disk Index] -> [Disk Cycle Tray Status] (disk set)

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

[libretro] Keyboard
---
Common

|PC-98key|key|problem|menu|
|:---:|:---:|:---:|:---:|
|STOP|Pause|||
|COPY|PrintScreen|don't come event|implemented|
|v.f1||can't push||
|v.f2||can't push||
|v.f3||can't push||
|v.f4||can't push||
|v.f5||can't push||
|Kana||can't push|implemented|
|GRPH|RCtrl|||
|NFER|LAlt|||
|XFER|RAlt|||
|HOME/CLR|Home|||
|HELP|End|||
|KP=|KP=|can't push|implemented|
|KP.|KP.|don't come event|implemented|

JP106 keyboard(default)

|PC-98key|key|info|menu|
|:---:|:---:|:---:|:---:|
|￥ \||￥ \||don't come event|implemented|
|_ _||L2? Menu has open?|implemented|

US101 keyboard

|PC-98key|key|info|menu|
|:---:|:---:|:---:|:---:|
|2 "|2 @|two event 0x02,0x2A|
|￥ \||\\ \||||
|@ `|` ~|don't come event|implemented|
|; +|; :|||
|: *|' "|||
|_ _||can't push|implemented|

[X11] Keyboard
---
Common

|PC-98key|key|problem|menu|
|:---:|:---:|:---:|:---:|
|STOP|Pause|||
|COPY||can't push||
|v.f1||can't push||
|v.f2||can't push||
|v.f3||can't push||
|v.f4||can't push||
|v.f5||can't push||
|Kana||can't push||
|GRPH|RCtrl|||
|NFER|LAlt|||
|XFER|RAlt|||
|HOME/CLR|Home|||
|HELP|End|||
|KP=|KP=|can't push||

JP106 keyboard(default)

All keys OK.

US101 keyboard

|PC-98key|key|info|menu|
|:---:|:---:|:---:|:---:|
|2 "|2 @|||
|6 &|6 ^|||
|7 '|7 &|||
|8 (|8 *|||
|9 )|9 (|||
|0  |0 )|||
|- =|- _|||
|^ ~|= +|||
|￥ \||\\ \||||
|@ `|[ {|||
|[ {|] }|||
|; +|; :|||
|: *|' "|||
|] }|` ~|||
|_ _||can't push|implemented|

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
Default: NP2's default sound generator.  

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

* How many files(0-15)?

This screen is boot as PC-98 ROM BASIC mode.  
Your floppy/harddisk image isn't mount correctry.

Check selecting image files and restart.

* Floppy disk image

NP2 is made according to PC-98 specifications.  
Floppy disk types are 720KB(2DD) and 1.23MB(2HD).  
The floppy drive differs from the one of the PC/AT on the hardware level.  
(There are a few floppies formatted to 1.44MB(2HD) using MS-DOS 6.2.)

A common mistake is to create a floppy disk image with PC/AT.  
Some disk imager for PC/AT software is useful, but don't use PC-98 formatted floppy disk.

To read data from PC-98 formatted floppy disk,  
You must use Win2000 older and 3-mode floppy disk drive.

Also, many PC-98 floppy disks are provided with powerful copy guard for DRM.  
If you are in the way, you should use WIZARD V3/V5.  
It is better to have no obstacle.

And also, you may simply convert the image format.

VFIC (Virtual Floppy Image Converter)  
https://www.vector.co.jp/soft/win95/util/se151106.html

Virtual Floppy Drive  
https://sites.google.com/site/chitchatvmback/vfd

* Hard disk image

DiskExplorer is useful for hard disk image management.  
This software can access files in image nicely.  
Only FAT is supported, NTFS does not correspond.

DiskExplorer  
https://hp.vector.co.jp/authors/VA013937/editdisk/index.html

About network
---
From rev.7 we can use network.

NIC is follow spec.  

MELCO LGY-98  
IRQ:6  
I/O:0x00D0

[X11] To use external MIDI sound generator
---
We can use external MIDI sound generator with UM-1.

1. Connect UM-1 to USB
2. Check you can see 'C4D0' by '$ ls /dev/snd' command
3. Open xnp2kai
* Select xnp2kai's menu 'Device' -> 'MIDI option...'
* Set '/dev/snd/C4D0' to 'MIDI-OUT' in ’Device' frame
* Select 'MIDI-OUT device' to 'MIDI-OUT' in ’Assign' frame
* Press 'OK'
4. I tried with Touhou 2 (set MIDI option), I can listen MIDI sound.

[X11] To use software synthesizer timidity as Virtual MIDI
---
We can software synthesizer timidity as Virtual MIDI.

It seems that timidity is incompatible with PulseAudio.  
By changing to ALSA output, I was able to play sound.

１． Install timidity and fluid-soundfont-gm  

    $ sudo apt-get install timidity fluid-soundfont-gm

２． Edit timidity.cfg  

    $ sudo nano /etc/timidity  

    #source /etc/timidity/freepats.cfg  
    source /etc/timidity/fluidr3_gm.cfg

３． restart timidity  

    $ sudo service timidity restart

４．

    $ aconnect -o  
This time, you can see like Timidity port 128:0 to 128:3.

５．

    $ timidity -iA -B2,8 -Os &  

Drive timidity daemon output to ALSA.  

    $ aconnect -o  

This time, you can see like ALSAed Timidity port 129:0 to 129:3.

６．

    $ sudo modprobe snd-virmidi  

Add virtual MIDI port module.  

    $ aconnect -o  

This time, you can see like VirMIDI 3-0 to 3-3 at 28:0 to 31:0.

７．

    $ ls /dev/snd  

You can also see VirMIDI 3-0 to 3-3 at midiC3D0 to midiC3D3.

８． Connect VirMIDI 3-0 and ALSAed Timidity port 0.  

    $ aconnect 28:0 129:0  

９． Finally set '/dev/snd/midiC3D0' to XNP2.

Next boot computer, you command from 4.

WAB
---
From rev.16 we can use WAB (Window Accelerator Boards).

To use WAB, enable WAB and restart.

WAB Type normally uses 'PC-9821Xe10,Xa7e,Xb10 built-in'.

**640x480 256 color support for Windows 3.1**

1. Switch to directory `A:￥WINDOWS`, then run `SETUP` command.
2. Select display mode `640x480 256色 16ﾄﾞｯﾄ(9821ｼﾘｰｽﾞ対応)`, or `640x480 256色 12ﾄﾞｯﾄ(9821ｼﾘｰｽﾞ対応)` for smaller system font, then complete the changes. You may need Windows 3.1 installation disks when applying changes for the display driver.
3. Extract `EGCN4.DRV` and `PEGCV8.DRV` from `MINI3.CAB` in Windows 98 CD.
4. Copy extracted `EGCN4.DRV` and `PEGCV8.DRV` to `A:￥WINDOWS￥SYSTEM` directory, so as to replace the original driver files from Windows 3.1 installation disk.
5. Type `win` command to check if the driver works well.

**NOTE:** Do not run MS-DOS prompt with fullscreen mode, or your screen will get garbled when switching back to Windows environment.


Release
---
* Apr 2, 2018 (rev.16)
	- Add WAB (and a little tune)
* Mar 18, 2018
	- Merge NP21/W 0.86 rev.40
* Mar 9, 2018
	- [X11] add UI
	- [SDL2] add and fix UI
* Mar 4, 2018
	- refine keyboard map
* Feb 28, 2018
	- [SDL2] config file selectable by command line
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
