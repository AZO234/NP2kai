# Neko Project II 0.86 kai
Apr 20, 2020<br>

NP2kai is PC-9801 series emulator<br>

![](https://img.shields.io/github/tag/AZO234/NP2kai.svg)

## Build and Install

### libretro core

<details><summary>
for Windows/Linux/macOS
</summary><div>

#### Install tools
1. MSYS2 64bit + 64bit console(Windows).<br>
2. Install compiler, etc.<br>

#### Build
1. Change directory to sdl2.<br>
```
$ cd NP2kai/sdl2
```
2. Make.<br>
```
$ make
```

#### Install binary
1. Install shared library(.dll or .so or .dylib) to libretro's core directory.<br>
2. Locate BIOS files to np2kai in libretro's BIOS directory (libretro/BIOS/np2kai).<br>
</div></details>

<details><summary>
for Android/iOS
</summary><div>

#### Install tools
1. MSYS2 64bit + 64bit console(Windows).<br>
2. Install Android Studio, and NDK. And PATH there.<br>
3. Clone libretro-super.<br>
```
$ git clone --depth 1 https://github.com/libretro/libretro-super.git
```

#### Build
1. Change directory to libretro-super.<br>
```
$ cd libretro-super
```
2. Fetch np2kai.<br>
```
$ ./libretro-fetsh.sh np2kai
```
2. Build.<br>
(Android)<br>
```
$ ./libretro-build-android-mk.sh np2kai
```
(iOS)<br>
```
$ ./libretro-build-ios.sh np2kai
```

#### Install binary
1. Install shared library(.so or .dylib) to libretro's core directory.<br>
2. Locate BIOS files to np2kai in libretro's BIOS directory (libretro/BIOS/np2kai).<br>
<br>
NP2 menu is shown F12 or mouse middle button or L2, to swap FDD/HDD diskimages.<br>
</div></details>

### Windows

You should [NP2fmgen](http://nenecchi.kirara.st/) or [NP21/W](https://sites.google.com/site/np21win/home), maybe.

<details><summary>
VisualStudio 2019
</summary><div>

#### Install tools
1. Install VisualStudio 2019.<br>
2. Install NASM and PATH it.<br>
3. Locate VST SDK to C:\VST_SDK and run copy_vst2_to_vst3_sdk.bat.<br>

#### Build
1. Open win9x/np2vs2019.<br>
2. Set Release/x64.<br>
3. Build projects.<br>
4. Output np2kai.exe(i286) and np21kai.exe(i386).<br>

#### Install binary
1. Locate .exe file anywhere.<br>
2. Locate BIOS files to .exe same filder.<br>
</div></details>

<details><summary>
SDL2
</summary><div>

#### Install tools
1. Install MSYS2 64bit.<br>
2. Run MSYS2 64bit console<br>
3. Run follow command.<br>
```
$ pacman -S git make mingw-w64-x86_64-toolchain mingw-w64-x86_64-ntldd mingw-w64-x86_64-SDL2 mingw-w64-x86_64-SDL2_ttf mingw-w64-x86_64-SDL2_mixer
```

#### Build
1. Change directory to sdl2.<br>
```
$ cd NP2kai/sdl2
```
2. Make.<br>
(i286)<br>
```
$ make -f Makefile.win
```
(i386)<br>
```
$ make -f Makefile21.win
```

#### Install binary
1. Install.<br>
```
$ make install
```
2. Locate BIOS files to ~/.config/np2kai(i286) or ~/.config/np21kai(i386).<br>
3. Run np2kai(i286) or np21kai(i386).<br>
<br>
- NP2 menu is shown F11 or mouse middle button, to swap FDD/HDD diskimages.<br>
</div></details>

<details><summary>
SDL1
</summary><div>

#### Install tools
1. Install MSYS2 64bit.<br>
2. Run MSYS2 64bit console<br>
3. Run follow command.<br>
```
$ pacman -S git make mingw-w64-x86_64-toolchain mingw-w64-x86_64-ntldd mingw-w64-x86_64-SDL mingw-w64-x86_64-SDL_ttf mingw-w64-x86_64-SDL_mixer
```

#### Build
1. Change directory to sdl2.<br>
```
$ cd NP2kai/sdl2
```
2. Make.<br>
(i286)<br>
Edit 'SDL_VERSION' in Makefile.win from 2 to 1.
```
$ make -f Makefile.win
```
(i386)<br>
Edit 'SDL_VERSION' in Makefile21.win from 2 to 1.
```
$ make -f Makefile21.win
```

#### Install binary
1. Install.<br>
```
$ make install
```
2. Locate BIOS files to ~/.config/np2kai(i286) or ~/.config/np21kai(i386).<br>
3. Run np2kai(i286) or np21kai(i386).<br>
<br>
- NP2 menu is shown F11 or mouse middle button, to swap FDD/HDD diskimages.<br>
</div></details>

### Linux

*temporary*<br>
It seems slow xnp2kai's dialog now, on Ubuntu GNOME.<br>
(Maybe GTK issue. No problem on Ubuntu MATE.)<br>
This issue is can aboid with follow command when starting<br>
```
$ dbus-launch --exit-with-session xnp2kai
```

<details><summary>
X11 with SDL2
</summary><div>

#### Install tools
1. Install SDL2, etc.<br>
```
$ sudo apt-get install automake git gtk+-2 build-essential libsdl2-dev libsdl2-ttf-dev libsdl2-mixer-dev libusb-1.0-0-dev
```

#### Build
1. Change directory to x11.<br>
```
$ cd NP2kai/x11
```
2. Run autotools script.<br>
```
$ ./autogen.sh
```
3. Run configure.<br>
(i286)<br>
```
$ ./configure
```
(i386)<br>
```
$ ./configure --enable-ia32
```
4. Make.<br>
```
$ make
```

#### Install binary
1. Install.<br>
```
$ sudo make install
```
2. Locate BIOS files to ~/.config/xnp2kai(i286) or ~/.config/xnp21kai(i386).<br>
3. Run xnp2kai(i286) or xnp21kai(i386).<br>
</div></details>

<details><summary>
X11 with SDL1
</summary><div>

#### Install tools
1. Install SDL, etc.<br>
```
$ sudo apt-get install automake git gtk+-2 build-essential libsdl1.2-dev libsdl-ttf2.0-dev libsdl-mixer1.2-dev libusb-1.0-0-dev
```

#### Build
1. Change directory to x11.<br>
```
$ cd NP2kai/x11
```
2. Run autotools script.<br>
```
$ ./autogen.sh
```
3. Run configure.<br>
(i286)<br>
```
$ ./configure --enable-sdl --enable-sdlmixer --enable-sdlttf --enable-sdl2=no --enable-sdl2mixer=no --enable-sdl2ttf=no
```
(i386)<br>
```
$ ./configure --enable-sdl --enable-sdlmixer --enable-sdlttf --enable-sdl2=no --enable-sdl2mixer=no --enable-sdl2ttf=no --enable-ia32
```
4. Make.<br>
```
$ make
```

#### Install binary
1. Install.<br>
```
$ sudo make install
```
2. Locate BIOS files to ~/.config/xnp2kai(i286) or ~/.config/xnp21kai(i386).<br>
3. Run xnp2kai(i286) or xnp21kai(i386).<br>
</div></details>

<details><summary>
SDL2
</summary><div>

#### Install tools
1. Install SDL2, etc.<br>
```
$ sudo apt-get install git build-essential libsdl2-dev libsdl2-ttf-dev libsdl2-mixer-dev
```

#### Build
1. Change directory to sdl2.<br>
```
$ cd NP2kai/sdl2
```
2. Make.<br>
(i286)<br>
```
$ make -f Makefile.unix
```
(i386)<br>
```
$ make -f Makefile21.unix
```

#### Install binary
1. Install.<br>
```
$ sudo make install
```
2. Locate BIOS files to ~/.config/np2kai(i286) or ~/.config/np21kai(i386).<br>
3. Run np2kai(i286) or np21kai(i386).<br>
<br>
- NP2 menu is shown F11 key or mouse middle button, to swap FDD/HDD diskimages.<br>
</div></details>

<details><summary>
SDL1
</summary><div>

#### Install tools
1. Install SDL2, etc.<br>
```
$ sudo apt-get install git build-essential libsdl1.2-dev libsdl-ttf2.0-dev libsdl-mixer1.2-dev
```

#### Build
1. Change directory to sdl2.<br>
```
$ cd NP2kai/sdl2
```
2. Make.<br>
(i286)<br>
Edit 'SDL_VERSION' in Makefile.unix from 2 to 1.
```
$ make -f Makefile.unix
```
(i386)<br>
Edit 'SDL_VERSION' in Makefile21.unix from 2 to 1.
```
$ make -f Makefile21.unix
```

#### Install binary
1. Install.<br>
```
$ sudo make install
```
2. Locate BIOS files to ~/.config/np2kai(i286) or ~/.config/np21kai(i386).<br>
3. Run np2kai(i286) or np21kai(i386).<br>
<br>
- NP2 menu is shown F11 key or mouse middle button, to swap FDD/HDD diskimages.<br>
</div></details>

### macOS

<details><summary>
SDL2
</summary><div>

#### Install tools
1. Install XCode.<br>
2. Install brew.<br>
3. Execute follow command.<br>
```
$ brew install sdl2 sdl2_mixer sdl2_ttf
```

#### Build
1. Change directory to sdl2.<br>
```
$ cd NP2kai/sdl2
```
2. Make.<br>
(i286)<br>
```
$ make -f Makefile.mac
```
(i386)<br>
```
$ make -f Makefile21.mac
```

#### Install binary
1. Install.<br>
```
$ make install
```
2. Locate BIOS files to ~/.config/np2kai(i286) or ~/.config/np21kai(i386).<br>
3. Run np2kai(i286) or np21kai(i386).<br>
<br>
- NP2 menu is shown F11 key or mouse middle button, to swap FDD/HDD diskimages.<br>
</div></details>

<details><summary>
SDL1
</summary><div>

#### Install tools
1. Install XCode.<br>
2. Install brew.<br>
3. Execute follow command.<br>
```
$ brew install sdl sdl_mixer sdl_ttf
```

#### Build
1. Change directory to sdl2.<br>
```
$ cd NP2kai/sdl2
```
2. Make.<br>
(i286)<br>
Edit 'SDL_VERSION' in Makefile.mac from 2 to 1.
```
$ make -f Makefile.mac
```
(i386)<br>
Edit 'SDL_VERSION' in Makefile21.mac from 2 to 1.
```
$ make -f Makefile21.mac
```

#### Install binary
1. Install.<br>
```
$ make install
```
2. Locate BIOS files to ~/.config/np2kai(i286) or ~/.config/np21kai(i386).<br>
3. Run np2kai(i286) or np21kai(i386).<br>
<br>
- NP2 menu is shown F11 key or mouse middle button, to swap FDD/HDD diskimages.<br>
</div></details>

### for Other

<details><summary>
GCW Zero (GCW0)
</summary><div>

#### Build
1. Change directory into sdl2.<br>
```
$ cd NP2kai/sdl2
```
2. Make.<br>
```
$ ODCONFIG=gcw0 make -f Makefile.opendingux
```
or
```
$ ODCONFIG=gcw0 make -f Makefile21.opendingux
```

#### Install binary
1. Transfer np2kai.opk or np21kai.opk to /media/apps/ .<br>
2. BIOS files should locate in ~/.config/np2kai (or np21kai).<br>
</div></details>

<details><summary>
Dingux series (RS90 GKD350H)
</summary><div>

#### Build
1. Change directory to sdl2.<br>
```
$ cd NP2kai/sdl2
```
2. Make.<br>
```
$ ODCONFIG=rs90 make -f Makefile.opendingux
```
or
```
$ ODCONFIG=rs90 make -f Makefile.opendingux
```

#### Install binary
1. Transfer np2kai.opk or np21kai.opk to /media/apps/ .<br>
2. BIOS files should locate in ~/.config/np2kai (or np21kai).<br>
</div></details>

## About libretro port

<details><summary>
BIOS files location
</summary><div>

- bios.rom<br>
- font.rom or font.bmp<br>
- itf.rom<br>
- sound.rom<br>
- (bios9821.rom or d8000.rom<br>But I never see good dump file.)<br>
- 2608_bd.wav<br>
- 2608_sd.wav<br>
- 2608_top.wav<br>
- 2608_hh.wav<br>
- 2608_tom.wav<br>
- 2608_rim.wav<br>

BIOS files locate in 'np2kai' directory at BIOS directory (configured by RetroArch).<br>
Configure file (np2kai.cfg) is made in 'np2kai' BIOS directory.<br>

To get BIOS files, you need actual PC-98 machine.<br>
Start MS-DOS on the actual PC-98 and execute the GETBIOS command<br>
(included in the np2tool/np2tool.zip floppy image)<br>
to create BIOS files.<br>

Rhythm sound files are dumped from PC-98 machine with FM sound gen.<br>
You can get sound files from [here](https://sites.google.com/site/ym2608rhythm/).
</div></details>

<details><summary>
Fonts
</summary><div>

NP2kai recommend font is using font.rom.<br>

### Windows

You can use 'MS Gothic'. To install add your windows,<br>
'install files Easten Asian languages' at 'language' option.<br>

After installation, 'msgothic.ttc' file's shortcut named 'default.ttf' put in BIOS directory.<br>
(Already exist 'font.tmp', delete this.)<br>
And start NP2kai.

### Linux

I recommend use 'Takao Gothic'.<br>
Install with follow command.
After installation, 'TakaoGothic.ttf' file's shortcut put in BIOS directory.<br>
```
sudo apt install 'fonts-takao-*'
```
and
```
ls -n /usr/share/fonts/truetype/takao-gothic/TakaoGothic.ttf BIOSdirectory/default.ttf'
```
Already exist 'font.tmp', delete this.<br>
And start NP2kai.<br>

You can use 'Noto sans mono CJK', 'MS Gothic'(Japanese) also.
</div></details>

<details><summary>
NP2 menu (different libretro menu)
</summary><div>

NP2 menu is shown F12 or mouse middle button or L2.<br>
NP2 menu can swap FDD/HDD diskimages.(Swapping HDD need reset.)<br>
</div></details>

<details><summary>
Mounting/Swaping Disk and HDD/CD mounting at start
</summary><div>

Using libretro contents .m3u file listed floppy disk images,<br>
You can use libretro swap interface.<br>
(This file must be wiritten in UTF-8.)<br>
```
1st.d88
2nd.d88
3rd.d88
```
1st image is mouted to FDD1, 2nd image is mouted to FDD2.<br>
(Not suitable when using only one FDD1 drive. Use NP2 menu.)<br>
To swap FDD2 imagefile, libretro menu durling run core,<br>
'Disk Control' -> 'Disk Cycle Tray Status' (eject) -> 'Disk Index' -> 'Disk Cycle Tray Status' (disk set)<br>
So, to swap FDD1 imagefile, libretro 'Option' menu -> 'Swap Disks on Drive' set 'FDD1'<br>

HDD/CD image can't be wiritten in .m3u file <br>
You can write to .cmd file commandline,<br>
(This file must be wiritten in UTF-8.)
```
np2kai fdilocation/aaa.fdi hdilocation/bbb.hdi isolocation/ccc.iso
```
'aaa.fdi' is mounted to FDD1,<br>
'bbb.hdi' is mounted to HDD1,<br>
'ccc.iso' is mounted to CD drive.<br>
(Determined by extension)<br>

.m3u files can written in .cmd file.<br>
Then, You can FDs+HD and FDs+CD contents file.
</div></details>

<details><summary>
Using mouse (Joypad mouse mode)
</summary><div>

Mouse cursor moving is always enable with mouse on PC.<br>

Mouse cursor moving and left-button be able to controled with joypad stick.<br>
Switch Stick2Mouse mode in config to 'L-stick' or 'R-stick(default)'.<br>
- Stick: mouse move<br>
- Thumb: mouse left button<br>
- ClickShift+Thumb: mouse right button<br>
ClickShift button is assigned to R1 default.<br>

To using digital pad, switch 'Joypad mode' in config to 'Mouse'.<br>
Mouse cursor is able to move with joypad's digital button also.<br>
- D-UP/DOWN/LEFT/RIGHT: mouse move<br>
- B button: mouse left button<br>
- A button: mouse right button<br>
- R button: mouse speed up durling hold<br>
</div></details>

<details><summary>
Using keyboard (Joypad Keyboard mode)
</summary><div>

Keyboard is able to control with joypad.<br>
Switch 'Joypad mode' in config to 'Arrows' or 'Keypad' (or 'Manual keyboard').<br>

- D-UP/DOWN/LEFT/RIGHT: Arrow key or Keypad(2468) key<br>

(No notation)
- B button: Z key<br>
- A button: X key<br>
- X button: Space key<br>
- Y button: left Ctrl key<br>
(3 button)
- B button: X key<br>
- A button: C key<br>
- X button: Space key<br>
- Y button: Z key<br>

- L button: Backspace key<br>
- R button: right Shift key<br>
- Select button: Escape key<br>
- Start button: Return key<br>

By setting 'Manual Kayboard', you can custom keycode for button.<br>
Change 'lrjoybtn' value in system/np2kai/np2kai.cfg.<br>
This value is little endian and 12 values ​​of 16bits(2Bytes) are arranged.<br>
Write the key code of RETROK (see libretro.h) to this value.<br>
The order is D-UP/DOWN/LEFT/RIGHT/A/B/X/Y/L/R/Select/Start.<br>
</div></details>

<details><summary>
Using ATARI joypad (Joypad ATARI mode)
</summary><div>

By setting 'ATARI joypad', you can use ATARI joypad port.

- A button: A button<br>
- B button: B button<br>
- X button: Rapid A button<br>
- Y button: Rapid B button<br>
</div></details>

<details><summary>
Tuning performance
</summary><div>

- CPU clock<br>
Change "CPU Clock Multiplyer".<br>
- Memory size<br>
Change "RAM Size".<br>
- Sound device<br>
26K: for old games.<br>
86: for newer games.<br>
- Sound Generator (to change need reset)<br>
fmgen: fmgen sound generator.<br>
Default: NP2's default sound generator.<br>
- How to set GDC 2.5MHz/5MHz?<br>
1. Press End key(assigned Help key) + reset<br>
2. Select 'ディップスイッチ２'(DIP switch 2)<br>
- How to key typing?<br>
There are two ways:<br>
1. map the 'enable hotkeys' hotkey in settings > input > input hotkey binds and RetroArch will stop listening for hotkeys unless/until you hold that button/key<br>
2. enable the "game focus mode" (mapped to scroll_lock by default) and it will send all of your inputs to the core instead of the frontend. However, some people have reported having trouble getting out of game focus mode.<br>
(Thanks hasenbanck)
</div></details>

## Keyboard mapping (libretro)

<details><summary>
Common
</summary><div>

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
</div></details>

<details><summary>
JP106 keyboard(default)
</summary><div>

|PC-98key|key|info|menu|
|:---:|:---:|:---:|:---:|
|￥ \|￥ \|don't come event|implemented|
|_ _||L2? Menu has open?|implemented|
</div></details>

<details><summary>
US101 keyboard
</summary><div>

|PC-98key|key|info|menu|
|:---:|:---:|:---:|:---:|
|2 "|2 @|two event 0x02,0x2A||
|￥ \|\\ \|||
|@ `|` ~|don't come event|implemented|
|; +|; :|||
|: *|' "|||
|_ _||can't push|implemented|
</div></details>

## Keyboard mapping (X11)

<details><summary>
Common
</summary><div>

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
</div></details>

<details><summary>
JP106 keyboard(default)
</summary><div>

All keys are used, OK.
</div></details>

<details><summary>
US101 keyboard
</summary><div>

|PC-98key|key|info|menu|
|:---:|:---:|:---:|:---:|
|2 "|2 @|||
|6 &|6 ^|||
|7 '|7 &|||
|8 (|8 *|||
|9 )|9 (|||
|0<br>|0 )|||
|- =|- _|||
|^ ~|= +|||
|￥ \|\\ \|||
|@ `|[ {|||
|[ {|] }|||
|; +|; :|||
|: *|' "|||
|] }|` ~|||
|_ _||can't push|implemented|
</div></details>

<details><summary>
Setting to RetroPie
</summary><div>

1. Install Japanese font. (umefont need SDL2 port only)<br>
```
$ sudo apt-get install fonts-droid fonts-horai-umefont
```
2. Locate libretro & SDL2 port files.<br>
```
$ sudo mkdir /opt/retropie/libretrocores/lr-np2kai
$ sudo cp np2kai_libretro.so /opt/retropie/libretrocores/lr-np2kai/
$ sudo mkdir /opt/retropie/emulators/np2kai
$ sudo cp np2kai /opt/retropie/emulators/np2kai/
$ sudo touch /opt/retropie/emulators/np2kai/np2kai.cfg
$ sudo chmod 666 /opt/retropie/emulators/np2kai/np2kai.cfg
```
3. Write & locate retroarch.cfg.<br>
```
$ sudo vi /opt/retropie/configs/pc98/retroarch.cfg`
```
Settings made here will only override settings in the global retroarch.cfg if placed above the #include line<br>
```
input_remapping_directory = "/opt/retropie/configs/pc98/"
#include "/opt/retropie/configs/all/retroarch.cfg"
```
4. Locate BIOS files.<br>
BIOS files locate in "&tilde;/RetroPie/BIOS/np2kai/" directory.<br>
and "/opt/retropie/emulators/np2kai/" too.<br>
5. Make shortcut to Japanese font. (SDL2 port only)<br>
```
$ sudo ln -s /usr/share/fonts/truetype/horai-umefont/ume-ugo4.ttf /opt/retropie/emulators/np2kai/default.ttf
```
6. Add "carbon-mod". (Japanese nize)<br>
```
$ git clone https://github.com/eagle0wl/es-theme-carbon.git`
$ sudo cp -r ./es-theme-carbon /etc/emulationstation/themes/carbon-mod
```
7. Add to "/etc/emulationstation/es_systems.cfg" writing.<br>
```
$ sudo nano /etc/emulationstation/es_systems.cfg`
```
```
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
```
8. Add to "/opt/retropie/configs/pc98/emulators.cfg" writing.<br>
```
$ cd /opt/retropie/configs
$ sudo mkdir pc98
$ cd pc98
$ sudo nano emulators.cfg
```
```
np2kai="/opt/retropie/emulators/np2kai %ROM%"
lr-np2kai="/opt/retropie/emulators/retroarch/bin/retroarch -L /opt/retropie/libretrocores/lr-np2kai/np2kai_libretro.so --config /opt/retropie/configs/pc98/retroarch.cfg %ROM%"
<br><br>default="lr-np2kai"
```
9. Launch ES and set "CARBON-MOD" to "THEME-SET".<br>
</div></details>

## Informaion

<details><summary>
Using CD-ROM drive
</summary><div>

To use CD drive with MS-DOS 6.2,<br>
write follow to CONFIG.SYS.<br>
```
LASTDRIVE=Z
DEVICE=A:￥DOS￥NECCDD.SYS /D:CD_101
```
And write follow to AUTOEXEC.BAT.<br>
```
A:￥DOS￥MSCDEX.EXE /D:CD_101 /L:Q
```
Then, you'll can use CD drive as Q drive.<br>
</div></details>

<details><summary>
How many files(0-15)?<br>
</summary><div>

This screen is boot as PC-98 ROM BASIC mode.<br>
You succeed to locate BIOS files.<br>
Your floppy/harddisk image isn't mount correctry.<br>
Check selecting image files and restart.<br>
</div></details>

<details><summary>
About PC-9801 floppy disk image
</summary><div>

NP2 is made according to PC-98 specifications.<br>

Floppy disk types are 720KB(2DD) and 1.23MB(2HD).<br>
The floppy drive differs from the one of the PC/AT on the hardware level.<br>
(There are a few floppies formatted to 1.44MB(2HD) using MS-DOS 6.2.)<br>

A common mistake is to create a floppy disk image with PC/AT.<br>
Some disk imager for PC/AT software is useful, but can't use PC-98 formatted floppy disk.<br>

To read data from PC-98 formatted floppy disk,<br>
You must use Win2000 older and '3-mode' floppy disk drive.<br>

Also, many PC-98 floppy disks are provided with powerful copy guard for DRM.<br>
If you are in the way, you should use WIZARD V3/V5.<br>
It is better to have no obstacle.<br>

And also, you may simply convert the image format.<br>

VFIC (Virtual Floppy Image Converter)<br>
https://www.vector.co.jp/soft/win95/util/se151106.html<br>

Virtual Floppy Drive<br>
https://sites.google.com/site/chitchatvmback/vfd<br>
</div></details>

<details><summary>
Hard disk image
</summary><div>

DiskExplorer is useful for hard disk image management.<br>
This software can access files in image nicely.<br>
Only FAT is supported, NTFS does not correspond.<br>

DiskExplorer<br>
https://hp.vector.co.jp/authors/VA013937/editdisk/index.html<br>
</div></details>

<details><summary>
To use libnvl.so functions (X11)
</summary><div>

http://euee.web.fc2.com/tool/nvl/np2kainvl.html<br>
Locate libnvl.so to /usr/local/lib/.<br>
Then we can use follow types HDD image files.<br>

- vmdk<br>
- dsk<br>
- vmdx<br>
- vdi<br>
- qcow<br>
- qcow2<br>
- hdd<br>
</div></details>

<details><summary>
Network Card
</summary><div>

NP2kai can use NIC that is follow spec.<br>

MELCO LGY-98<br>
IRQ:6<br>
I/O:0x00D0
</div></details>

<details><summary>
WAB (Window Accelerator Boards)
</summary><div>

NP2kai can use WAB (Window Accelerator Boards).<br>

To use WAB, enable WAB in menu and restart.<br>

WAB Type normally uses 'PC-9821Xe10,Xa7e,Xb10 built-in'.<br>

**640x480 256 color support for Windows 3.1**

1. Switch to directory 'A:￥WINDOWS', then run 'SETUP' command.<br>
2. Select display mode '640x480 256色 16ﾄﾞｯﾄ(9821ｼﾘｰｽﾞ対応)', or '640x480 256色 12ﾄﾞｯﾄ(9821ｼﾘｰｽﾞ対応)' for smaller system font, then complete the changes. You may need Windows 3.1 installation disks when applying changes for the display driver.<br>
3. Extract 'EGCN4.DRV' and 'PEGCV8.DRV' from 'MINI3.CAB' in Windows 98 CD.<br>
4. Copy extracted 'EGCN4.DRV' and 'PEGCV8.DRV' to 'A:￥WINDOWS￥SYSTEM' directory, so as to replace the original driver files from Windows 3.1 installation disk.<br>
5. Type 'win' command to check if the driver works well.<br>

**NOTE:** Do not run MS-DOS prompt with fullscreen mode, or your screen will get garbled when switching back to Windows environment.<br>

You can use WAB Type 'WAB-S', 'WSN', 'GA-98NB'.<br>

- WAB-S driver<br>
http://buffalo.jp/download/driver/multi/wab.html<br>

- WSN driver<br>
http://buffalo.jp/download/driver/multi/wgna_95.html<br>

- GA-98NB driver<br>
https://www.iodata.jp/lib/product/g/175_win95.htm<br>
</div></details>

<details><summary>
Hook fontrom (textize)
</summary><div>
Enable 'Hook fontrom' in menu,<br>
Hook to using fontrom and output text to 'hook_fontrom.txt' in BIOS directory.<br>
This function is disable at start NP2kai.<br>
</div></details>

#### MIDI sound (X11)

<details><summary>
External MIDI
</summary><div>

NP2kai can use external MIDI sound generator with UM-1.<br>

1. Connect UM-1 to USB<br>
2. Check you can see 'C4D0' by '$ ls /dev/snd' command<br>
3. Open xnp2kai<br>
- Select xnp2kai's menu 'Device' -> 'MIDI option...'<br>
- Set '/dev/snd/C4D0' to 'MIDI-OUT' in ’Device' frame<br>
- Select 'MIDI-OUT device' to 'MIDI-OUT' in ’Assign' frame<br>
- Press 'OK'<br>
4. I tried with Touhou 2 (set MIDI option), I can listen MIDI sound.<br>
</div></details>

<details><summary>
Timidity++ (software MIDI synthesizer)
</summary><div>

NP2kai can software synthesizer Timidity++ as Virtual MIDI.<br>

It seems that Timidity++ is incompatible with PulseAudio.<br>
By changing to ALSA output, I was able to play sound.<br>

1. Install Timidity++ and fluid-soundfont-gm<br>
```
$ sudo apt-get install timidity fluid-soundfont-gm
```
2. Edit timidity.cfg<br>
```
$ sudo nano /etc/timidity
```
```
#source /etc/timidity/freepats.cfg<br>
source /etc/timidity/fluidr3_gm.cfg
```
3. restart timidity<br>
```
$ sudo service timidity restart
```
4. 
```
$ aconnect -o
```
This time, you can see like Timidity port 128:0 to 128:3.<br>
5.
```
$ timidity -iA -B2,8 -Os &
```
Run timidity daemon output to ALSA.<br>
```
$ aconnect -o
```
This time, you can see like ALSAed Timidity port 129:0 to 129:3.<br>
6.
```
$ sudo modprobe snd-virmidi
```
Add virtual MIDI port module.<br>
```
$ aconnect -o
```
This time, you can see like VirMIDI 3-0 to 3-3 at 28:0 to 31:0.<br>
7.
```
$ ls /dev/snd
```
You can also see VirMIDI 3-0 to 3-3 at midiC3D0 to midiC3D3.<br>
8. Connect VirMIDI 3-0 and ALSAed Timidity port 0.<br>
```
$ aconnect 28:0 129:0
```
9. Finally set '/dev/snd/midiC3D0' to xnp2kai.<br>

Next boot computer, you command from 4.<br>
</div></details>

## Release
- May 10, 2020 (rev.22)<br>
  - merge NP21/W rev.73
- Apr 20, 2020<br>
  - mod OpenDingux
- Apr 19, 2020<br>
  - J2K/J2M -> JoypadMode
    - add ATARI joypad
- Apr 9, 2020<br>
  - hook fontrom (textize)
- Apr 5, 2020<br>
  - [Windows]
    - add send to SSTP(伺か,ukagaka) from xnp2
    - apply wide character (inner UTF-8)
  - add codecnv
    - UTF-32(UCS4)
- Apr 2, 2020<br>
  - reform compiler options
- Mar 31, 2020<br>
  - [libretro] fix input
    - mash trigger, too fast move
    - J2K 'Manual' setting lost
    - add S2M click shift (l to r) button (default R1)
- Mar 30, 2020<br>
  - add LittleOrchestraL, MultimediaOrchestra from np2s
- Mar 28, 2020<br>
  - Merge NP21/W rev.72<br>
  - [libretro] using lr file stream API
  - safe string function
  - np2min/np2max to MIN/MAX
  - common base compiler.h (compiler_base.h)
  - [X11] fix SUPPORT_PC9821
  - [SDL2] mod Windows file access
- Mar 13, 2020<br>
  - Merge NP21/W rev.71<br>
    - [libretro] add CPU feature<br>
    - fix Sound Blaster 16 (OPL3)<br>
    - GamePort on soundboards<br>
- Mar 6, 2020<br>
  - [SDL2/X11] fix default.ttf<br>
- Mar 2, 2020<br>
  - Using absolute/rerative path in .m3u and .cmd list file<br>
  - [libretro] not remember last HDD mount<br>
- Feb 18, 2020<br>
  - fix V30 and 286 flag register<br>
- Feb 4, 2020<br>
  - Merge NP21/W rev.70 strongly<br>
  - Merge NP21/W rev.70<br>
  - update libretro-common<br>
- Jan 29, 2020<br>
  - fix X11 no sound (please check 'sounddrv = SDL' in .config/xnp2kai/xnp2kairc)<br>
  - fix for GKD350H<br>
  - fix for GCW0<br>
- Jan 26, 2020<br>
  - mod mouse cursor moving.<br>
- Jan 15, 2020<br>
  - Support again SDL1<br>
  - fix bool
- Dec 10, 2019 (rev.21)<br>
  - Merge NP21/W 0.86 rev.69<br>
    - HAXM
- Nov 19, 2019<br>
  - fix Android makefile
  - merge yoshisuga/tvos THANKS!!
    - add libretro/tvOS port
  - merge part of swingflip/master THANKS!!
    - add xx Clasic port
  - merge part of yksoft1/emscripten THANKS VERY MUCH!!
  - [SDL2 & libretro]
    - fix WAB type value
    - mod to apply UTF-8
    - fix SUPPORT_LARGE_HDD
    - WinNT4/200 IDE Fix
  - fix Makefile21
- Jul 14, 2019 (rev.20)<br>
  - Merge NP21/W 0.86 rev.62-63<br>
- Jun 23, 2019<br>
  - modify default cfg/BIOS location (np2kai or 'np21kai')<br>
- Jun 21, 2019<br>
  - Fix SDL2 build and install<br>
  - Merge NP21/W 0.86 rev.57-61<br>
...<br>
- Jan 24, 2019<br>
  - Merge NP21/W 0.86 rev.56<br>
- Jan 13, 2019<br>
  - Merge NP21/W 0.86 rev.55<br>
- Jan 9, 2019<br>
  - Merge NP21/W 0.86 rev.53,54<br>
- Dec 22, 2018<br>
  - Merge NP21/W 0.86 rev.52<br>
- Dec 19, 2018<br>
  - Merge NP21/W 0.86 rev.51<br>
- Dec 16, 2018<br>
  - Fix WAB<br>
- Dec 14, 2018<br>
  - Merge NP21/W 0.86 rev.50<br>
- Dec 10, 2018 (rev.18)<br>
  - Merge NP21/W 0.86 rev.48,49<br>
- Nov 29, 2018<br>
  - Add MIDI support<br>
- Nov 25, 2018<br>
  - Merge NP21/W 0.86 rev.47<br>
- Oct 28, 2018<br>
  - Merge NP21/W 0.86 rev.46<br>
- Oct 14, 2018<br>
  - Merge NP21/W 0.86 rev.45<br>
- Sep 27, 2018<br>
  - Merge NP21/W 0.86 rev.44<br>
- Aug 22, 2018<br>
  - Apply for libnvl.so<br>
  - Merge NP21/W 0.86 rev.43<br>
- Jun 27, 2018 (rev.17)<br>
  - Merge NP21/W 0.86 rev.42<br>
- Jun 19, 2018<br>
  - Add Joy2Key manual mode<br>
  - Merge NP21/W 0.86 rev.41<br>
  - Read GP-IB BIOS.(not work)<br>
- Apr 26, 2018<br>
  - Add build for GCW Zero<br>
- Apr 2, 2018 (rev.16)<br>
  - Add WAB (and a little tune)<br>
- Mar 18, 2018<br>
  - Merge NP21/W 0.86 rev.40<br>
- Mar 9, 2018<br>
  - [X11] add UI<br>
  - [SDL2] add and fix UI<br>
- Mar 4, 2018<br>
  - refine keyboard map<br>
- Feb 28, 2018<br>
  - [SDL2] config file selectable by command line<br>
- Feb 20, 2018<br>
  - FONT.ROM/FONT.BMP can be loaded lower case.<br>
- Feb 19, 2018<br>
  - [libretro] Apply disk swap interface<br>
- Feb 17, 2018<br>
  - [X11] Mouse moving is more smopothly (Thanks frank-deng)<br>
  - Using *.img *.ima type floppy image (Thanks frank-deng)<br>
- Feb 14, 2018<br>
  - Fix parse CUE sheet (Thanks frank-deng)<br>
- Feb 6, 2018 (rev.15)<br>
  - NP2 namespace change to NP2kai<br>
  - [SDL2] Locate of config files is ~/.config/np2kai<br>
  - [X11] Locate of config files is ~/.config/xnp2kai<br>
- Feb 5, 2018<br>
  - Merge NP21/W 0.86 rev.38<br>
- Feb 4, 2018<br>
  - Add setting Joy to Mouse cursor speed up rasio<br>
- Dec 5, 2017<br>
  - Default GDC clock is 2.5MHz<br>
  - In Joy2Mousem, mouse speed up with R button<br>
- Nov 18, 2017<br>
  - Merge NP21/W 0.86 rev.37<br>
- Oct 26, 2017<br>
  - Apply to 1.44MB FDD floppy image file<br>
- Oct 21, 2017 (rev.14)<br>
  - Merge NP21/W 0.86 rev.36<br>
    - Mate-X PCM<br>
    - Sound Blaster 16<br>
    - OPL3 (MAME codes is GPL licence)<br>
    - Auto IDE BIOS<br>
- Oct 16, 2017 (rev.13)<br>
  - Refix BEEP PCM<br>
- Oct 2, 2017 (rev.13)<br>
  - remove CDDA mod<br>
- Sep 20, 2017<br>
  - [SDL2] Use SDL2 mixer<br>
- Sep 16, 2017<br>
  - Add RaSCSI hdd image file support<br>
- Sep 14, 2017 (rev.12)<br>
  - [libretro] (newest core binary is auto released by buildbot)<br>
  - Fix triple fault case<br>
- Sep 7, 2017<br>
  - Fix BEEP PCM<br>
- Aug 27, 2017 (rev.11)<br>
  - Merge NP21/w rev.35 beta2<br>
  - [libretro] state save/load<br>
- Aug 23, 2017 (rev.10)<br>
  - Merge NP21/w rev.35 beta1<br>
- Aug 22, 2017<br>
  - Merge 私家<br>
    Merged
    - AMD-98 Joyport<br>
    - S98V3<br>
    - Otomichanx2<br>
    - V30 patch<br>
    - VAEG fix<br>
    - CSM voice<br>
    *Couldn't merged*
    - LittleOrchestra<br>
    - MultimediaOrchestra<br>
    - WaveRec<br>
  - Merge kaiE<br>
    - force ROM to RAM
    - CDDA fix
    - Floppie fix
- Aug 21, 2017<br>
  - Apply libretro-super build<br>
- Aug 17, 2017 (rev.9)<br>
  - Apply fmgen<br>
- Aug 3, 2017 (rev.8)<br>
  - Apply HRTIMER<br>
  - [libretro] input underscore(_) for western keyboard<br>
  - [libretro] Add Joy2Key (thanks Tetsuya79)<br>
- Jul 24, 2017 (rev.7)<br>
  - Apply network<br>
- Jul 18, 2017 (rev.6)<br>
- Jul 17, 2017<br>
  - Apply HOSTDRV<br>
  - [libretro] Add Joy2Mouse mode (switch at config menu)<br>
- Jul 4, 2017<br>
  - rename to 'kai'<br>
- Jun 28, 2017<br>
  - [libretro] Applicate to libretro port<br>
- Jun 21, 2017<br>
  - [X11] Applicate to X11 port<br>
- Jun 20, 2017 (rev.6 beta)<br>
  - [NP21] fix for VGA<br>
- Jun 19, 2017<br>
  - [NP21] fix IA-32<br>
  - more memory size available<br>
- Jun 18, 2017<br>
  - more avilable FDD/HDD/CD-ROM image<br>
  - [NP21] FPU (fpemul_dosbox.c is GPL licence, others is MIT licence)<br>
- Jun 12, 2017<br>
  - COM<br>
  - MIDI<br>
  - JOYSTICK<br>
  - IDE (can't use CD-ROM yet)<br>
  - SDL_Keycode -&gt; SDL_Scancode<br>
  - Save BMP<br>
  - State Save<br>
- Jun 4, 2017<br>
  - [NP21] お察しください<br>
- Jun 1, 2017<br>
  - First release<br>

## ToDo

### SDL2
  - lvgl ?

### libretro
  - font.bmp

### Linux
  - GTK3 ?

### Windows
  - local UTF-8 to Wide conversion
  - build for Win9x
  - VST SDK 3
  - BMS dialog

## Reference

- Neko Project 2 (ねこープロジェクトII)<br>
http://www.yui.ne.jp/np2<br>
- NP2 for Raspberry Pi<br>
https://github.com/irori/np2pi/wiki/NP2-for-Raspberry-Pi<br>
- NP2 addon to RetroPie<br>
http://eagle0wl.hatenadiary.jp/entry/2016/10/07/213830<br>
- NP21/W<br>
https://sites.google.com/site/np21win/home<br>
- Neko Project 2 (PC98 emulator) port for libretro/RetroArch<br>
https://github.com/meepingsnesroms/libretro-meowPC98<br>

