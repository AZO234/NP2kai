# Neko Project II 0.86 kai
Nov 3, 2024<br>

NP2kai is PC-9801 series emulator<br>

![](https://img.shields.io/github/tag/AZO234/NP2kai.svg)

## Build and Install

### libretro core

<details><summary>
for Windows/Linux/macOS
</summary><div>

#### Install tools
1. MSYS2 64bit + 64bit console(Windows).
2. Install compiler, etc.

#### Build
1. Change directory to sdl.
```
$ cd NP2kai/sdl
```
2. Make.
```
$ make
```

#### Install binary
1. Install shared library(.dll or .so or .dylib) to libretro's cores directory (libretro/cores).
2. Locate BIOS files to np2kai in libretro's system directory (libretro/system/np2kai).
</div></details>

<details><summary>
for Android/iOS
</summary><div>

#### Install tools
1. MSYS2 64bit + 64bit console(Windows).
2. Install Android Studio, and NDK. And PATH there.
3. Clone libretro-super.
```
$ git clone --depth 1 https://github.com/libretro/libretro-super.git
```

#### Build
1. Change directory to libretro-super.
```
$ cd libretro-super
```
2. Fetch np2kai.
```
$ ./libretro-fetsh.sh np2kai
```
3. Build.
- Android
```
$ ./libretro-build-android-mk.sh np2kai
```
- iOS
```
$ ./libretro-build-ios.sh np2kai
```

#### Install binary
1. Install shared library(.so or .dylib) to libretro's cores directory (libretro/cores).
2. Locate BIOS files to np2kai in libretro's system directory (libretro/system/np2kai).

NP2 menu is shown F12 or mouse middle button or L2, to swap FDD/HDD diskimages.

On Android, Game Files are need to locate in '/storage/emulated/0/RetroArch' by access rights reason.
Game Files cannot locate on external storage.
</div></details>

### VisualStudio 2022

You should [NP2fmgen](http://nenecchi.kirara.st/) or [NP21/W](https://sites.google.com/site/np21win/home), maybe.

<details><summary>
VisualStudio 2022
</summary><div>

#### Install tools
1. Install VisualStudio 2022.
  - Desktop Development with C++
  - .NET Framework 4.8 SDK
  - C++ ATL
  - C++ MFC
  - Connecting USB Device
  - Windows 10 SDK
  - Windows Universal CRT
  - Graphics Debugger and GPU Profiler for DirectX
  - Developer Analytics Tools
  - Git for Windows
  - NuGet Package manager
  - GitHub extention for Visual Studio
  - C++ 2019 redistoributable package
  - C++ 2019 redistoributable package updater
  - MSBuild
  - CMake for Windows
  - IntelliCode
2. Install [WDK KMDF](https://docs.microsoft.com/ja-jp/windows-hardware/drivers/download-the-wdk)
3. Install [vcpkg](https://github.com/Microsoft/vcpkg)
4. Install packages with vcpkg.
  - OpenSSL
  - SDL2 SDL2_mixer SDL2_ttf
  - libusb
5. Install [Ninja](http://www.projectmanager.ninja/home.html)

#### Build
1. Start VisualStudio 2022 (and without code).
2. File -> Open -> CMake -> CMakeLists.txt in NP2kai directory.
3. Build -> Build all.
4. Output np21kai_windows.exe in out directory.

- CMake options of VisualStudio 2022 port (*=default)

|name|value|work|output|
|:---:|:---:|:---:|:---:|
|BUILD_I286|ON|Build i286|NP2kai_windows|
|BUILD_I286|OFF*|Build IA-32|NP21kai_windows|
|BUILD_HAXM|ON|Build IA-32 HAXM|NP21kai_HAXM_windows|

#### Install binary
1. Locate .exe file anywhere.
2. Locate BIOS files to .exe same filder.
</div></details>

### SDL

<details><summary>
SDL
</summary><div>

#### Install tools
- MSYS2
1. Install MSYS2 64bit.
2. Run MSYS2 64bit console.
3. Run follow command.
```
$ pacman -S git cmake make mingw-w64-x86_64-toolchain mingw-w64-x86_64-ntldd mingw-w64-x86_64-SDL2 mingw-w64-x86_64-SDL2_mixer mingw-w64-x86_64-SDL2_ttf mingw-w64-x86_64-SDL mingw-w64-x86_64-SDL_mixer mingw-w64-x86_64-SDL_ttf mingw-w64-x86_64-openssl mingw-w64-x86_64-libusb
```
- Linux
1. Run follow command.
```
$ sudo apt install git cmake ninja-build build-essential libsdl2-dev libsdl2-mixer-dev libsdl2-ttf-dev libsdl1.2-dev libsdl-mixer1.2-dev libsdl-ttf2.0-dev libusb-1.0-0-dev libssl-dev
```
- macOS
1. Install XCode and brew.
2. Run follow command.
```
$ sudo brew install cmake ninja sdl sdl_mixer sdl_ttf sdl2 sdl2_mixer sdl2_ttf libusb
```

#### Build
1. Change directory to NP2kai.
```
$ cd NP2kai
```
2. Make work directory, and step into there.
```
$ mkdir build
$ cd build
```
3. Generate Makefile.
```
$ cmake .. -D BUILD_SDL=ON
```
4. Make.
```
$ make -j
```

- CMake options of SDL port (*=default)

|name|value|work|output|
|:---:|:---:|:---:|:---:|
|BUILD_SDL|ON(*)|Build SDL port||
|USE_SDL2|ON*|Build with SDL2|sdlnp21kai|
|USE_SDL2|OFF|Build with SDL|sdlnp21kai_sdl1|
|USE_SDL_MIXER|ON*|Build with SDL_mixer or SDL2_mixer||
|USE_SDL_TTF|ON*|Build with SDL_ttf or SDL2_ttf||
|BUILD_I286|ON|Build i286|sdlnp2kai|
|BUILD_I286|OFF*|Build IA-32|sdlnp21kai|
|BUILD_HAXM|ON|Build IA-32 HAXM|sdlnp21kai_HAXM|

  - BUILD_SDL=ON default on macOS

#### Install binary
1. Install.
```
$ make install
```
2. Locate BIOS files to ~/.config/&lt;SDL NP2kai filename&gt;
3. Run SDL NP2kai.

- NP2 menu is shown F11 or mouse middle button, to swap FDD/HDD diskimages.
</div></details>


### X with GTK2 and SDL


#### Arch Linux

For the [latest release](https://github.com/AZO234/NP2kai/releases), a package can be found in the [AUR](https://aur.archlinux.org/packages/xnp2kai-azo234/)

Fonts are **NOT** included in the AUR package.

*temporary*<br>
It seems slow xnp2kai's dialog now, on Ubuntu GNOME.<br>
(Maybe GTK issue. No problem on Ubuntu MATE.)<br>
This issue is can aboid with follow command when starting
```
$ dbus-launch --exit-with-session xnp2kai
```

<details><summary>
X with GTK2 and SDL
</summary><div>

#### Install tools
1. Run follow command.

- Debian/Ubuntu series
```
$ sudo apt install git cmake ninja-build build-essential libx11-dev libglib2.0-dev libgtk2.0-dev libsdl2-dev libsdl2-mixer-dev libsdl2-ttf-dev libsdl1.2-dev libsdl-mixer1.2-dev libsdl-ttf2.0-dev  libusb-1.0-0-dev libfreetype-dev libfontconfig1-dev libssl-dev
```

- Fedora series
```
$ sudo dnf groupinstall "Development Tools"
$ sudo dnf install gcc-c++ cmake libusb-devel SDL-devel SDL_mixer-devel SDL_ttf-devel SDL2-devel SDL2_mixer-devel SDL2_ttf-devel gtk2-devel libX11-devel fontconfig-devel freetype-devel
```

#### Build
1. Change directory to NP2kai.
```
$ cd NP2kai
```
2. Make work directory, and step in.
```
$ mkdir build
$ cd build
```
3. Generate Makefile.
```
$ cmake .. -D BUILD_X=ON
```
4. Make.
```
$ make -j
```

- CMake options of X port (*=default)

|name|value|work|output|
|:---:|:---:|:---:|:---:|
|BUILD_X|ON(*)|Build X port||
|USE_SDL2|ON*|Build with SDL2|xnp21kai|
|USE_SDL2|OFF|Build with SDL|xnp21kai_sdl1|
|USE_SDL_MIXER|ON*|Build with SDL_mixer or SDL2_mixer||
|USE_SDL_TTF|ON*|Build with SDL_ttf or SDL2_ttf||
|BUILD_I286|ON|Build i286|xnp2kai|
|BUILD_I286|OFF*|Build IA-32|xnp21kai|
|BUILD_HAXM|ON|Build IA-32 HAXM|xnp21kai_HAXM|

  - BUILD_X=ON default on UNIX

#### Install binary
1. Install.
```
$ sudo make install
```
2. Locate BIOS files to ~/.config/<X NP2kai filename>
3. Run X NP2kai.

- NP2 menu is shown F11 key or mouse middle button, to swap FDD/HDD diskimages.
</div></details>

### Emscripten

<details><summary>
Emscripten
</summary><div>

#### Install tools
- MSYS2
1. Install [Emscripten](https://emscripten.org/).
2. Run MSYS2 64bit console
3. Run follow command.
```
$ pacman -S git cmake make
```
- Linux
1. Install [Emscripten](https://emscripten.org/).
2. Run follow command.
```
$ sudo apt install git cmake build-essential
```
- macOS
1. Install [Emscripten](https://emscripten.org/).
2. Install XCode and brew.
3. Run follow command.
```
$ sudo brew install cmake
```

#### Build
1. Change directory to NP2kai.
```
$ cd NP2kai
```
2. Make work directory, and step in.
```
$ mkdir build
$ cd build
```
3. Generate Makefile.
```
$ emcmake cmake ..
```
4. Make.
```
$ make -j
```

- CMake options of Emscripten port (*=default)

|name|value|work|output|
|:---:|:---:|:---:|:---:|
|USE_SDL2|ON*|Build with SDL2|emnp21kai.html|
|USE_SDL2|OFF|Build with SDL|emnp21kai_sdl1.html|
|USE_SDL_MIXER|ON*|Build with SDL2_mixer||
|USE_SDL_TTF|ON*|Build with SDL2_ttf||
|BUILD_I286|ON|Build i286|emnp2kai.html|
|BUILD_I286|OFF*|Build IA-32|emnp21kai.html|

  - Emscripten SDL1 port cannot be with SDL_mixer and SDL_ttf

#### Run
1. Run on emrun.
```
$ emrun <Emscripten NP2kai filename>.html
```
</div></details>

### OpenDingux

<details><summary>
OpenDingux
</summary><div>

#### Install tools
- GCW0
1. Install host toolchain to /opt/gcw0-toolchain with [buildroot](https://github.com/OpenDingux/buildroot).
- RG350
1. Install host toolchain to /opt/rg350-toolchain with [RG350_buildroot](https://github.com/tonyjih/RG350_buildroot).
2. [patch](https://raw.githubusercontent.com/AZO234/RAcross_linux/master/RG350_buildroot.patch).
- RS90
1. Install host toolchain to /opt/rs90-toolchain with [buildroot](https://github.com/OpenDingux/buildroot).

#### Build
1. Change directory to NP2kai.
```
$ cd NP2kai
```
2. Make work directory, and step in.
```
$ mkdir build
$ cd build
```
3. Generate Makefile.
- GCW0
```
$ cmake .. -D BUILD_OPENDINGUX_GCW0=ON
```
- RG350
```
$ cmake .. -D BUILD_OPENDINGUX_RG350=ON
```
- RS90
```
$ cmake .. -D BUILD_OPENDINGUX_RS90=ON
```
4. Make.
```
$ make -j
```

- CMake options of OpenDingux port (*=default)

|name|value|work|output|
|:---:|:---:|:---:|:---:|
|BUILD_OPENDINGUX_GCW0|ON|Build OpenDingux GCW0|np21kai_gcw0.opk|
|BUILD_OPENDINGUX_RG350|ON|Build OpenDingux RG350|np21kai_rg350.opk|
|BUILD_OPENDINGUX_RS90|ON|Build OpenDingux RS90|np21kai_rs90.opk|
|USE_SDL2|ON*|Build with SDL2|&lt;machine&gt;_np21kai.opk|
|USE_SDL2|OFF|Build with SDL|&lt;machine&gt;_np21kai_sdl1.opk|
|USE_SDL_MIXER|ON*|Build with SDL_mixer or SDL2_mixer||
|USE_SDL_TTF|ON*|Build with SDL_ttf or SDL2_ttf||
|BUILD_I286|ON|Build i286|np2kai_&lt;machine&gt;.opk|
|BUILD_I286|OFF*|Build IA-32|np21kai_&lt;machine&gt;.opk|

  - RS90 port cannot be with SDL2
  - RS90 port cannot be with SDL_ttf
</div></details>

## About libretro port

<details><summary>
BIOS files location
</summary><div>

- bios.rom
- font.rom or font.bmp
- itf.rom
- sound.rom
- (bios9821.rom or d8000.rom<br>But I never see good dump file.)
- 2608_bd.wav
- 2608_sd.wav
- 2608_top.wav
- 2608_hh.wav
- 2608_tom.wav
- 2608_rim.wav

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
ln -s /usr/share/fonts/truetype/takao-gothic/TakaoGothic.ttf BIOSdirectory/default.ttf'
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

Using .m3u file listed floppy disk images,<br>
You can use libretro swap interface.<br>
(This file must be wiritten in UTF-8.)<br>
(On libretro m3u file supported is not in core now.)
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

If mouse cannot use on a game,<br>
check mouse driver for the game or included in MS-DOS is loaded by CONFIG.SYS.<br>
(Or MS-DOS's mouse driver inhibit the game only mouse driver.)<br>
<code>DEVICE=A:&yen;DOS&yen;MOUSE.SYS</code>

Mouse cursor moving and left-button be able to controled with joypad stick.<br>
Switch Stick2Mouse mode in config to 'L-stick' or 'R-stick(default)'.<br>
- Stick: mouse move
- Thumb: mouse left button
- ClickShift+Thumb: mouse right button
ClickShift button is assigned to R1 default.<br>

To using digital pad, switch 'Joypad mode' in config to 'Mouse'.<br>
Mouse cursor is able to move with joypad's digital button also.<br>
- D-UP/DOWN/LEFT/RIGHT: mouse move
- B button: mouse left button
- A button: mouse right button
- R button: mouse speed up durling hold
</div></details>

<details><summary>
Using keyboard (Joypad Keyboard mode)
</summary><div>

Keyboard is able to control with joypad.<br>
Switch 'Joypad mode' in config to 'Arrows' or 'Keypad' (or 'Manual keyboard').<br>

- D-UP/DOWN/LEFT/RIGHT: Arrow key or Keypad(2468) key

(No notation)
- B button: Z key
- A button: X key
- X button: Space key
- Y button: left Ctrl key

(3 button)
- B button: X key
- A button: C key
- X button: Space key
- Y button: Z key
<br>

- L button: Backspace key
- R button: right Shift key
- Select button: Escape key
- Start button: Return key

By setting 'Manual Kayboard', you can custom keycode for button.<br>
Change 'lrjoybtn' value in system/np2kai/np2kai.cfg.<br>
This value is little endian and 12 values ​​of 16bits(2Bytes) are arranged.<br>
Write the key code of RETROK (see libretro.h) to this value.<br>
The order is D-UP/DOWN/LEFT/RIGHT/A/B/X/Y/L/R/Select/Start.
</div></details>

<details><summary>
Using ATARI joypad (Joypad ATARI mode)
</summary><div>

By setting 'ATARI joypad', you can use ATARI joypad port.

- A button: A button
- B button: B button
- X button: Rapid A button
- Y button: Rapid B button
</div></details>

<details><summary>
Tuning performance
</summary><div>

- CPU clock
  Change "CPU Clock Multiplyer".
- Memory size
  Change "RAM Size".
  - MS-DOS 5 or older : lower 16.6MB
  - MS-DOS 6 : lower 64.6MB
- Sound device
  - 26K: for old games.
  - 86: for newer games.
- Sound Generator (to change need reset)
  - fmgen: fmgen sound generator.
  - Default: NP2's default sound generator.
- How to set GDC 2.5MHz/5MHz?
1. Press End key(assigned Help key) + reset
2. Select 'ディップスイッチ２'(DIP switch 2)
- How to key typing?
  There are two ways:
  1. map the 'enable hotkeys' hotkey in settings > input > input hotkey binds and RetroArch will stop listening for hotkeys unless/until you hold that button/key
  2. enable the "game focus mode" (mapped to scroll_lock by default) and it will send all of your inputs to the core instead of the frontend. However, some people have reported having trouble getting out of game focus mode.
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

1. Install Japanese font. (umefont need SDL2 port only)
```
$ sudo apt-get install fonts-droid fonts-horai-umefont
```
2. Locate libretro & SDL2 port files.
```
$ sudo mkdir /opt/retropie/libretrocores/lr-np2kai
$ sudo cp np2kai_libretro.so /opt/retropie/libretrocores/lr-np2kai/
$ sudo mkdir /opt/retropie/emulators/np2kai
$ sudo cp np2kai /opt/retropie/emulators/np2kai/
$ sudo touch /opt/retropie/emulators/np2kai/np2kai.cfg
$ sudo chmod 666 /opt/retropie/emulators/np2kai/np2kai.cfg
```
3. Write & locate retroarch.cfg.
```
$ sudo vi /opt/retropie/configs/pc98/retroarch.cfg`
```
Settings made here will only override settings in the global retroarch.cfg if placed above the #include line
```
input_remapping_directory = "/opt/retropie/configs/pc98/"
#include "/opt/retropie/configs/all/retroarch.cfg"
```
4. Locate BIOS files.
  BIOS files locate in "&tilde;/RetroPie/BIOS/np2kai/" directory.<br>and "/opt/retropie/emulators/np2kai/" too.
5. Make shortcut to Japanese font. (SDL2 port only)
```
$ sudo ln -s /usr/share/fonts/truetype/horai-umefont/ume-ugo4.ttf /opt/retropie/emulators/np2kai/default.ttf
```
6. Add "carbon-mod". (Japanese nize)
```
$ git clone https://github.com/eagle0wl/es-theme-carbon.git`
$ sudo cp -r ./es-theme-carbon /etc/emulationstation/themes/carbon-mod
```
7. Add to "/etc/emulationstation/es_systems.cfg" writing.
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
8. Add to "/opt/retropie/configs/pc98/emulators.cfg" writing.
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
9. Launch ES and set "CARBON-MOD" to "THEME-SET".
</div></details>

## Informaion

<details><summary>
Key-repeat (libretro and SDL)
</summary><div>

To use Key-repeat, enable in menu.<br>
On default, Key-repeat's delay is 500ms, Key-repeat's interval is 50ms.<br>
</div></details>

<details><summary>
Using CD-ROM drive
</summary><div>

To use CD drive with MS-DOS 6.2,<br>
write follow to CONFIG.SYS.<br>
<code>LASTDRIVE=Z</code><br>
<code>DEVICE=A:&yen;DOS&yen;NECCDD.SYS /D:CD_101</code><br>
And write follow to AUTOEXEC.BAT.<br>
<code>A:&yen;DOS&yen;MSCDEX.EXE /D:CD_101 /L:Q</code><br>
Then, you'll can use CD drive as Q drive.
</div></details>

<details><summary>
How many files(0-15)?
</summary><div>

This screen is boot as PC-98 ROM BASIC mode.<br>
You succeed to locate BIOS files.<br>
Your floppy/harddisk image isn't mount correctry.<br>
Check selecting image files and restart.
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

- vmdk
- dsk
- vmdx
- vdi
- qcow
- qcow2
- hdd
</div></details>

<details><summary>
Text editor
</summary><div>

MS-DOS for PC-9801 include 'SEDIT.EXE' text editor.<br>
Also there is 'VZ Editor' product.
</div></details>

<details><summary>
About LHA(lzh) archived file
</summary><div>

File has extention '.lzh' is compressed file by [LHA](https://www.vector.co.jp/soft/dos/util/se002413.html).<br>
If to extract only, you can use [LHE](https://www.vector.co.jp/soft/dos/util/se017776.html).
</div></details>

<details><summary>
File manager
</summary><div>

To file management, you can use [FILMTN](https://www.vector.co.jp/soft/dos/util/se001385.html) and [LHMTN](https://www.vector.co.jp/soft/dos/util/se001396.html),<br>
or [FD](https://www.vector.co.jp/soft/dos/util/se000010.html).<br>
</div></details>

<details><summary>
Memory driver
</summary><div>

When start PC-98, memory amount is displaied.
```
MEMORY 640KB + 13312KB
```
640KB is conventional memory.<br>
(For example) 13312KB is extnded memory.

Extnded memory is use as XMS(eXtended Memory Specification)<br>
by HIMEM.SYS is written in CONFIG.SYS.<br>
<code>DEVICE=A:&yen;DOS&yen;HIMEM.SYS</code><br>
Drivers and daemons can be loaded on XMB.<br>
(But EMM386.EXE, SMARTDRV.EXE, NECCD*.SYS cannot be loaded on XMB.)<br>
<code>DEVICEHIGH=A:&yen;DOS&yen;MOUSE.SYS</code><br>
<code>DEVICEHIGH=A:&yen;DOS&yen;RAMDISK.SYS /X 1536</code><br>
8086 or V30 can use HMA(448KB XMS).<br>
i286 or later,<br>
MS-DOS 5 or older can use lower 16MB XMS.<br>
MS-DOS 6 can use lower 64MB XMS.

XMB can use as UMB(386KB), EMB by<br>
EMM386.EXE(old EMM386.SYS) is written in CONFIG.SYS.<br>
<code>DEVICE=A:&yen;DOS&yen;EMM386.EXE /P=64 /UMB /DPMI</code><br>
'/P=64' means using EMS 64page (1page=16KB).<br>
'/DPMI' means with DPMI support.

Normaly, MS-DOS is located on conventional memory.<br>
You can use XMB and UMB, DOS can be located on them,
```
DOS=HIGH,UMB
```

If you use upper 64MB XMB,<br>
you can use [VEM486](https://www.vector.co.jp/soft/dos/hardware/se025675.html) (deposit software)<br>
instead of HIMEM.SYS and EMM386.EXE.<br>
<code>DEVICE=A:&yen;VEM486&yen;VEM486.EXE</code>
</div></details>

<details><summary>
Running Turbo C++ 4.0
</summary><div>

To run Turbo C++ 4.0, use HIMEM.SYS only.
</div></details>

<details><summary>
Running NASM
</summary><div>

To run NASM, use DPMI(HIMEM.SYS + EMM386.EXE + DPMI option).<br>
And before run NASM, set swap follow command.<br>
<code>A:&yen;NASM&yen;CWSDPMI.EXE -S A:&yen;NASM&yen;CWSDPMI.SWP</code>

CWSDPMI.EXE is loaded on memory continuous.<br>
<code>A:&yen;NASM&yen;CWSDPMI.EXE -P -S A:&yen;NASM&yen;CWSDPMI.SWP</code><br>
To free<br>
<code>A:&yen;NASM&yen;CWSDPMI.EXE -U</code><br>
</div></details>

<details><summary>
Using linker
</summary><div>

I think better linker is genarate 16bit executable one.<br>
(Ex.LNK563)<br>
if Not careful, you can use MASM's linker.<br>
</div></details>

<details><summary>
About MS-C
</summary><div>

When MS-C ver.5 is released, users use MS-DOS 3.0.<br>
MS-C ver.6 is worked on Windows DOS prompt only.<br>
Then MS-C is unconvenience to MS-DOS.<br>
I think Turbo C++ 4.0 is used.
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

1. Switch to directory 'A:￥WINDOWS', then run 'SETUP' command.
2. Select display mode '640x480 256色 16ﾄﾞｯﾄ(9821ｼﾘｰｽﾞ対応)', or '640x480 256色 12ﾄﾞｯﾄ(9821ｼﾘｰｽﾞ対応)' for smaller system font, then complete the changes. You may need Windows 3.1 installation disks when applying changes for the display driver.
3. Extract 'EGCN4.DRV' and 'PEGCV8.DRV' from 'MINI3.CAB' in Windows 98 CD.
4. Copy extracted 'EGCN4.DRV' and 'PEGCV8.DRV' to 'A:￥WINDOWS￥SYSTEM' directory, so as to replace the original driver files from Windows 3.1 installation disk.
5. Type 'win' command to check if the driver works well.

**NOTE:** Do not run MS-DOS prompt with fullscreen mode, or your screen will get garbled when switching back to Windows environment.

You can use WAB Type 'WAB-S', 'WSN', 'GA-98NB'.<br>

- WAB-S driver
http://buffalo.jp/download/driver/multi/wab.html<br>

- WSN driver
http://buffalo.jp/download/driver/multi/wgna_95.html<br>

- GA-98NB driver
https://www.iodata.jp/lib/product/g/175_win95.htm<br>
</div></details>

<details><summary>
Hook fontrom (textize)
</summary><div>
Enable 'Hook fontrom' in menu,<br>
Hook to using fontrom and output text to 'hook_fontrom.txt' in BIOS directory.<br>
This function is disable at start NP2kai.<br>
</div></details>

<details><summary>
Debug snapshot
</summary><div>
Debug snapshot is 'save state' plus various information for debug.<br>
(version, SHA-1 hash of FDs and CDs, displaied image, state of machine.)
Those information files are saved into 'debugss' directory in BIOS directory.<br>
<br>
Take snapshot before and after the problem with reproducibility,<br>
This function is used for communication purposes.<br>
(Probably will be large file, so compression with ZIP<br>
and be careful hosting when reporting.)
</div></details>

<details><summary>
Video filter
</summary><div>
To enable and select profile, control by menu.<br>
Video filter1 have 3 profiles.<br>
<br>
A profile include 3 filters.<br>
Filters are applied in order.<br>
filter0 -&gt; filter1 -&gt; filter2<br>
<br>
Filter's parameters are set to 'vf1_p(profile no)_p(filter no)' to 8 params.

|Param No|Name|value|
|:---:|:---:|:---:|
|0|Enable|0:OFF/1:ON|
|1|Filter Type No|(follow table Filter Type no)|
|2|Param 0||
|3|Param 1||
|4|Param 2||
|5|Param 3||
|6|Param 4||
|7|Param 5||

|Filter Type No|Filter Name|Param 0|Param 1|Param 2|Param 3|Param 4|Param 5|
|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
|0|THRU|-|-|-|-|-|-|
|1|NP(Nega/Posi) invert|-|-|-|-|-|-|
|2|Depth down|0-7:downbits<br>(default 7)|-|-|-|-|-|
|3|Grey|0-8:Grey depth|0-359:H of white<br>(default 0)|0-255:S of white<br>(default 0)|0-255:V of white<br>(default 255)|-|-|
|4|V Gamma|1-255:Gamma*10<br>(default 10)|-|-|-|-|-|
|5|Rotate H|0-359:Rotate H<br>(default 0)|-|-|-|-|-|
|6|HSV smoothing|5-25:Radius*10<br>(default 5)|1/3/5:Sample count<br>(default 3)|0-180:Merge H diff<br>(default 30)|0-128:Merge S diff<br>(default 30)|0-128:Merge V diff<br>(default 90)|Weight 0:Same/1:Linear/2:Sign<br>(default 0)|
|7|RGB smoothing|5-25:Radius*10<br>(default 5)|1/3/5:Sample count<br>(default 3)|0-128:Merge R diff<br>(default 30)|0-128:Merge G diff<br>(default 30)|0-128:Merge B diff<br>(default 30)|Weight 0:Same/1:Linear/2:Sign<br>(default 0)|

HSV/RGB smoothing is heavy to work.<br>
</div></details>


#### MIDI sound (libretro)

<details><summary>
Common
</summary><div>

Set RetroArch's 'Setting' -&gt; 'Audio' -&gt; 'MIDI' -&gt; 'Output' -&gt; '(MIDI device)' in menu.
</div></details>

<details><summary>
Windows
</summary><div>

NP2kai can use 'Microsoft GS Wavetable Synth'.<br>
NP2kai can use external MIDI sound generator with UM-1(USB-MIDI interface).<br>
</div></details>

<details><summary>
Linux
</summary><div>

<details><summary>
External MIDI
</summary><div>

NP2kai can use external MIDI sound generator with UM-1(USB-MIDI interface).<br>
</div></details>

<details><summary>
Timidity++ (software MIDI synthesizer)
</summary><div>

NP2kai can software synthesizer Timidity++ as ALSA Virtual MIDI.<br>

1. Install Timidity++ and fluid-soundfont
```
$ sudo apt install timidity timidity-interfaces-extra fluid-soundfont-gm fluid-soundfont-gs
```
2. Edit timidity.cfg
```
$ sudo nano /etc/timidity/timidity.cfg
```
```
#source /etc/timidity/freepats.cfg<br>
source /etc/timidity/fluidr3_gm.cfg
```
3. Restart timidity
```
$ sudo service timidity restart
```
4. Run timidity daemon output to ALSA.
```
$ timidity -iA -B2,8 -Os &
```

5. It maybe able to select 'Timidity port 0' RetroArch's MIDI device.

Next boot computer, you command from 4.
You can write to .profile, but Timidity daemon spend a bit CPU performance.
</div></details>

</div></details>

#### MIDI sound (X11)

<details><summary>
Common
</summary><div>

NP2kai's MIDI setting is in 'Device' -&gt; 'MIDI option...'.

Set device file to 'Device'\'s 'MIDI-OUT'. (ex. /dev/snd/midiC0D0)<br>
And set 'Assign''s 'MIDI-OUT' to 'MIDI-OUT device'.
</div></details>

<details><summary>
External MIDI
</summary><div>

NP2kai can use external MIDI sound generator with UM-1(USB-MIDI interface).<br>

1. Connect UM-1 to USB
2. Check you can see 'midiC4D0' by '$ ls /dev/snd' command
3. Open xnp2kai and set MIDI device.

I tried with Touhou 2 (set MIDI option), I could listen MIDI sound.
</div></details>

<details><summary>
Timidity++ (software MIDI synthesizer)
</summary><div>

NP2kai can software synthesizer Timidity++ as Virtual MIDI.<br>
To using, necessaly setup Virtual MIDI Port module too.

1. Install Timidity++ and fluid-soundfont
```
$ sudo apt install timidity timidity-interfaces-extra fluid-soundfont-gm fluid-soundfont-gs
```
2. Edit timidity.cfg
```
$ sudo nano /etc/timidity/timidity.cfg
```
```
#source /etc/timidity/freepats.cfg<br>
source /etc/timidity/fluidr3_gm.cfg
```
3. Restart timidity
```
$ sudo service timidity restart
```
4. Run timidity daemon output to ALSA.
```
$ timidity -iA -B2,8 -Os &
```
You will see like ALSAed Timidity port 128:0 to 128:3.<br>
```
$ aconnect -o
```
5. Add virtual MIDI port module.
```
$ sudo modprobe snd-virmidi
```
(If you want to use snd-virmidi permanently, see [detail info](https://wiki.archlinux.org/index.php/Timidity%2B%2B).)
```
$ aconnect -o
```
You can see like VirMIDI 0-0 to 0-3 at 16:0 to 19:0.<br>
6. You can also see VirMIDI 0-0 to 0-3 as midiC0D0 to midiC0D3.<br>
```
$ ls /dev/snd
```
7. Connect VirMIDI 0-0 and ALSAed Timidity port 0.
```
$ aconnect 16:0 128:0
```
8. Finally set '/dev/snd/midiC0D0' to xnp2kai.

Next boot computer, you command from 4.
</div></details>

## Release
- Nov 3, 2024
  - merge NP21/W rev.92
- Oct 30, 2023
  - merge NP21/W rev.91
- Oct 24, 2023
  - merge NP21/W rev.90
- Feb 10, 2023
  - merge NP21/W rev.87,88
- Jly 31, 2022
  - merge NP21/W rev.85,86
    - (Exclusion is_nan, is_inf)
- Jan 23, 2021
  - merge NP21/W rev.84
  - [lr]remove m3u file support
- Jan 19, 2021
  - merge NP21/W rev.79
- Oct 22, 2020
  (Thanks to @miyamoto999)
  - Key-repeat
- Sep 28, 2020
  - merge NP21/W rev.78
- Sep 9, 2020
  - [SDL,lr] Fix GRPH and LWin keycode (thanks miyamoto999!)
- Sep 4, 2020
  - Apply to Nixpkgs package
- Aug 19, 2020
  - merge NP21/W rev.77
- Aug 12, 2020
  - fix mouse input off
- Jul 18, 2020
  - merge NP21/W rev.76
- Jul 14, 2020
  - add mouse input off
- Jun 23, 2020
  - merge NP21/W rev.75
- Jun 21, 2020
  - merge NP21/W rev.74
- Jun 15, 2020
  - [not lr] state save/load at first of main loop
- Jun 12, 2020
  - Video filter
- May 22, 2020
  - CMake
  - Emscripten
  - Debug snapshot
- May 10, 2020 (rev.22)
  - merge NP21/W rev.73
- Apr 20, 2020
- May 10, 2020 (rev.22)<br>
  - merge NP21/W rev.73
  - mod OpenDingux
- Apr 19, 2020
  - J2K/J2M -> JoypadMode
    - add ATARI joypad
- Apr 9, 2020
  - hook fontrom (textize)
- Apr 5, 2020
  - [Windows]
    - add send to SSTP(伺か,ukagaka) from xnp2
    - apply wide character (inner UTF-8)
  - add codecnv
    - UTF-32(UCS4)
- Apr 2, 2020
  - reform compiler options
- Mar 31, 2020
  - [libretro] fix input
    - mash trigger, too fast move
    - J2K 'Manual' setting lost
    - add S2M click shift (l to r) button (default R1)
- Mar 30, 2020
  - add LittleOrchestraL, MultimediaOrchestra from np2s
- Mar 28, 2020
  - Merge NP21/W rev.72
  - [libretro] using lr file stream API
  - safe string function
  - np2min/np2max to MIN/MAX
  - common base compiler.h (compiler_base.h)
  - [X11] fix SUPPORT_PC9821
  - [SDL2] mod Windows file access
- Mar 13, 2020
  - Merge NP21/W rev.71
    - [libretro] add CPU feature
    - fix Sound Blaster 16 (OPL3)
    - GamePort on soundboards
- Mar 6, 2020
  - [SDL2/X11] fix default.ttf
- Mar 2, 2020
  - Using absolute/rerative path in .m3u and .cmd list file
  - [libretro] not remember last HDD mount
- Feb 18, 2020
  - fix V30 and 286 flag register
- Feb 4, 2020
  - Merge NP21/W rev.70 strongly
  - Merge NP21/W rev.70
  - update libretro-common
- Jan 29, 2020
  - fix X11 no sound (please check 'sounddrv = SDL' in .config/xnp2kai/xnp2kairc)
  - fix for GKD350H
  - fix for GCW0
- Jan 26, 2020
  - mod mouse cursor moving.
- Jan 15, 2020
  - Support again SDL1
  - fix bool
- Dec 10, 2019 (rev.21)
  - Merge NP21/W 0.86 rev.69
    - HAXM
- Nov 19, 2019
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
- Jul 14, 2019 (rev.20)
  - Merge NP21/W 0.86 rev.62-63
- Jun 23, 2019
  - modify default cfg/BIOS location (np2kai or 'np21kai')
- Jun 21, 2019
  - Fix SDL2 build and install
  - Merge NP21/W 0.86 rev.57-61
...<br>
- Jan 24, 2019
  - Merge NP21/W 0.86 rev.56
- Jan 13, 2019
  - Merge NP21/W 0.86 rev.55
- Jan 9, 2019
  - Merge NP21/W 0.86 rev.53,54
- Dec 22, 2018
  - Merge NP21/W 0.86 rev.52
- Dec 19, 2018
  - Merge NP21/W 0.86 rev.51
- Dec 16, 2018
  - Fix WAB
- Dec 14, 2018
  - Merge NP21/W 0.86 rev.50
- Dec 10, 2018 (rev.18)
  - Merge NP21/W 0.86 rev.48,49
- Nov 29, 2018
  - Add MIDI support
- Nov 25, 2018
  - Merge NP21/W 0.86 rev.47
- Oct 28, 2018
  - Merge NP21/W 0.86 rev.46
- Oct 14, 2018
  - Merge NP21/W 0.86 rev.45
- Sep 27, 2018
  - Merge NP21/W 0.86 rev.44
- Aug 22, 2018
  - Apply for libnvl.so
  - Merge NP21/W 0.86 rev.43
- Jun 27, 2018 (rev.17)
  - Merge NP21/W 0.86 rev.42
- Jun 19, 2018
  - Add Joy2Key manual mode
  - Merge NP21/W 0.86 rev.41
  - Read GP-IB BIOS.(not work)
- Apr 26, 2018
  - Add build for GCW Zero
- Apr 2, 2018 (rev.16)
  - Add WAB (and a little tune)
- Mar 18, 2018
  - Merge NP21/W 0.86 rev.40
- Mar 9, 2018
  - [X11] add UI
  - [SDL2] add and fix UI
- Mar 4, 2018
  - refine keyboard map
- Feb 28, 2018
  - [SDL2] config file selectable by command line
- Feb 20, 2018
  - FONT.ROM/FONT.BMP can be loaded lower case.
- Feb 19, 2018
  - [libretro] Apply disk swap interface
- Feb 17, 2018
  - [X11] Mouse moving is more smopothly (Thanks frank-deng)
  - Using *.img *.ima type floppy image (Thanks frank-deng)
- Feb 14, 2018
  - Fix parse CUE sheet (Thanks frank-deng)
- Feb 6, 2018 (rev.15)
  - NP2 namespace change to NP2kai
  - [SDL2] Locate of config files is ~/.config/np2kai
  - [X11] Locate of config files is ~/.config/xnp2kai
- Feb 5, 2018
  - Merge NP21/W 0.86 rev.38
- Feb 4, 2018
  - Add setting Joy to Mouse cursor speed up rasio
- Dec 5, 2017
  - Default GDC clock is 2.5MHz
  - In Joy2Mousem, mouse speed up with R button
- Nov 18, 2017
  - Merge NP21/W 0.86 rev.37
- Oct 26, 2017
  - Apply to 1.44MB FDD floppy image file
- Oct 21, 2017 (rev.14)
  - Merge NP21/W 0.86 rev.36
    - Mate-X PCM
    - Sound Blaster 16
    - OPL3 (MAME codes is GPL licence)
    - Auto IDE BIOS
- Oct 16, 2017 (rev.13)
  - Refix BEEP PCM
- Oct 2, 2017 (rev.13)
  - remove CDDA mod
- Sep 20, 2017
  - [SDL2] Use SDL2 mixer
- Sep 16, 2017
  - Add RaSCSI hdd image file support
- Sep 14, 2017 (rev.12)
  - [libretro] (newest core binary is auto released by buildbot)
  - Fix triple fault case
- Sep 7, 2017
  - Fix BEEP PCM
- Aug 27, 2017 (rev.11)
  - Merge NP21/w rev.35 beta2
  - [libretro] state save/load
- Aug 23, 2017 (rev.10)
  - Merge NP21/w rev.35 beta1
- Aug 22, 2017
  - Merge 私家
    Merged
    - AMD-98 Joyport
    - S98V3
    - Otomichanx2
    - V30 patch
    - VAEG fix
    - CSM voice
    *Couldn't merged*
    - LittleOrchestra
    - MultimediaOrchestra
    - WaveRec
  - Merge kaiE
    - force ROM to RAM
    - CDDA fix
    - Floppie fix
- Aug 21, 2017
  - Apply libretro-super build
- Aug 17, 2017 (rev.9)
  - Apply fmgen
- Aug 3, 2017 (rev.8)
  - Apply HRTIMER
  - [libretro] input underscore(_) for western keyboard
  - [libretro] Add Joy2Key (thanks Tetsuya79)
- Jul 24, 2017 (rev.7)
  - Apply network
- Jul 18, 2017 (rev.6)
- Jul 17, 2017
  - Apply HOSTDRV
  - [libretro] Add Joy2Mouse mode (switch at config menu)
- Jul 4, 2017
  - rename to 'kai'
- Jun 28, 2017
  - [libretro] Applicate to libretro port
- Jun 21, 2017
  - [X11] Applicate to X11 port
- Jun 20, 2017 (rev.6 beta)
  - [NP21] fix for VGA
- Jun 19, 2017
  - [NP21] fix IA-32
  - more memory size available
- Jun 18, 2017
  - more avilable FDD/HDD/CD-ROM image
  - [NP21] FPU (fpemul_dosbox.c is GPL licence, others is MIT licence)
- Jun 12, 2017
  - COM
  - MIDI
  - JOYSTICK
  - IDE (can't use CD-ROM yet)
  - SDL_Keycode -&gt; SDL_Scancode
  - Save BMP
  - State Save
- Jun 4, 2017
  - [NP21] お察しください
- Jun 1, 2017
  - First release

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

- Neko Project 2 (ねこープロジェクトII)
http://www.yui.ne.jp/np2
- NP2 for Raspberry Pi
https://github.com/irori/np2pi/wiki/NP2-for-Raspberry-Pi
- NP2 addon to RetroPie
http://eagle0wl.hatenadiary.jp/entry/2016/10/07/213830
- NP21/W
https://sites.google.com/site/np21win/home
- Neko Project 2 (PC98 emulator) port for libretro/RetroArch
https://github.com/meepingsnesroms/libretro-meowPC98

