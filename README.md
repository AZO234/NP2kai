Neko Project II 0.86 SDL2 port rev.6
===
Jun 21, 2017  

Build SDL2 port
---

１． Install SDL2.  

	$ sudo apt-get install libsdl2-dev libsdl2-ttf-dev

２． Build.  

	$ cd NP2_SDL2/sdl2
	$ make -f makefile.unix

or

	$ make -f makefile21.unix

３． 'np2' or 'np21' binary is outputed in NP2_SDL2/bin  

Build X11 port
---

１． Install SDL2.  

	$ sudo apt-get install libsdl2-dev libsdl2-ttf-dev

２． Build.  

	$ cd NP2_SDL2/x11
	$ ./autogen.sh
	$ ./configure
	$ make

or

	$ ./configure --enable-ia32
	$ make

３． 'np2' or 'np21' binary is outputed in NP2_SDL2/x11  

Don't build other port. Maybe link errors occur.

Release
---
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

