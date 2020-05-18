cmake_minimum_required(VERSION 3.10)

project(
	NP2kai
	VERSION 0.86.0.22
	DESCRIPTION "NEC PC-98x1 and EPSON PC-x86 emulator"
	HOMEPAGE_URL "http://domisan.sakura.ne.jp/article/np2kai/np2kai.html"
	LANGUAGES C CXX
)

cmake_policy(SET CMP0076 NEW)

# Git
find_package(Git REQUIRED)

# NP2kai Git version
execute_process(COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0 OUTPUT_VARIABLE NP2kai_GIT_TAG OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD OUTPUT_VARIABLE NP2kai_GIT_HASH OUTPUT_STRIP_TRAILING_WHITESPACE)
message("*** NP2kai version is ${NP2kai_GIT_TAG} ${NP2kai_GIT_HASH}")

# NP2kai include dirs
list(APPEND NP2kai_common_include_dirs "${NP2kai_SOURCE_DIR}")

list(APPEND NP2kai_normal_include_dirs "i286c")

list(APPEND NP2kai_ia32_include_dirs "i386c" "i386c/ia32")

list(APPEND NP2kai_SDL_include_dirs "sdl")
list(APPEND NP2kai_SDL_Windows_dirs ${NP2kai_SDL_include_dirs} "sdl/windows")
list(APPEND NP2kai_SDL_UNIX_dirs ${NP2kai_SDL_include_dirs} "sdl/unix")
list(APPEND NP2kai_libretro_include_dirs ${NP2kai_SDL_include_dirs} "sdl/libretro")
list(APPEND NP2kai_Emscripten_include_dirs ${NP2kai_SDL_include_dirs} "sdl/em")
list(APPEND NP2kai_OpenDingux_include_dirs ${NP2kai_SDL_include_dirs} "sdl/opendingux")

list(APPEND NP2kai_X_include_dirs "x")

list(APPEND NP2kai_Windows_include_dirs "windows" "windows/misc")

# NP2kai sources
list(APPEND NP2kai_common_sources "calendar.c")
list(APPEND NP2kai_common_sources "keystat.c")
list(APPEND NP2kai_common_sources "nevent.c")
list(APPEND NP2kai_common_sources "pccore.c")
list(APPEND NP2kai_common_sources "statsave.c")
list(APPEND NP2kai_common_sources "timing.c")
file(GLOB_RECURSE NP2kai_temp "bios/*.c")
list(APPEND NP2kai_common_sources ${NP2kai_temp})
file(GLOB_RECURSE NP2kai_temp "cbus/*.c")
list(APPEND NP2kai_common_sources ${NP2kai_temp})
list(APPEND NP2kai_common_sources "codecnv/eucsjis.c")
list(APPEND NP2kai_common_sources "codecnv/eucucs2.c")
list(APPEND NP2kai_common_sources "codecnv/jisucs2.c")
list(APPEND NP2kai_common_sources "codecnv/sjiseuc.c")
list(APPEND NP2kai_common_sources "codecnv/sjisucs2.c")
list(APPEND NP2kai_common_sources "codecnv/tcswap16.c")
list(APPEND NP2kai_common_sources "codecnv/tcswap32.c")
list(APPEND NP2kai_common_sources "codecnv/ucs2sjis.c")
list(APPEND NP2kai_common_sources "codecnv/ucs2ucs4.c")
list(APPEND NP2kai_common_sources "codecnv/ucs2utf8.c")
list(APPEND NP2kai_common_sources "codecnv/ucs4ucs2.c")
list(APPEND NP2kai_common_sources "codecnv/ucs4utf8.c")
list(APPEND NP2kai_common_sources "codecnv/utf8ucs2.c")
list(APPEND NP2kai_common_sources "codecnv/utf8ucs4.c")
file(GLOB_RECURSE NP2kai_temp "common/*.c")
list(APPEND NP2kai_common_sources ${NP2kai_temp})
file(GLOB NP2kai_temp "diskimage/*.c")
list(APPEND NP2kai_common_sources ${NP2kai_temp})
list(APPEND NP2kai_common_sources "diskimage/cd/cdd_ccd.c")
list(APPEND NP2kai_common_sources "diskimage/cd/cdd_cue.c")
list(APPEND NP2kai_common_sources "diskimage/cd/cdd_iso.c")
list(APPEND NP2kai_common_sources "diskimage/cd/cdd_mds.c")
list(APPEND NP2kai_common_sources "diskimage/cd/cdd_nrg.c")
file(GLOB_RECURSE NP2kai_temp "diskimage/fd/*.c")
list(APPEND NP2kai_common_sources ${NP2kai_temp})
file(GLOB_RECURSE NP2kai_temp "fdd/*.c")
list(APPEND NP2kai_common_sources ${NP2kai_temp})
file(GLOB_RECURSE NP2kai_temp "font/*.c")
list(APPEND NP2kai_common_sources ${NP2kai_temp})
file(GLOB_RECURSE NP2kai_temp "generic/*.c")
list(APPEND NP2kai_common_sources ${NP2kai_temp})
file(GLOB_RECURSE NP2kai_temp "io/*.c")
list(APPEND NP2kai_common_sources ${NP2kai_temp})
file(GLOB_RECURSE NP2kai_temp "lio/*.c")
list(APPEND NP2kai_common_sources ${NP2kai_temp})
file(GLOB_RECURSE NP2kai_temp "mem/*.c")
list(APPEND NP2kai_common_sources ${NP2kai_temp})
list(APPEND NP2kai_common_sources "sound/adpcmc.c")
list(APPEND NP2kai_common_sources "sound/adpcmg.c")
list(APPEND NP2kai_common_sources "sound/beepc.c")
list(APPEND NP2kai_common_sources "sound/beepg.c")
list(APPEND NP2kai_common_sources "sound/cs4231c.c")
list(APPEND NP2kai_common_sources "sound/cs4231g.c")
list(APPEND NP2kai_common_sources "sound/fmboard.c")
list(APPEND NP2kai_common_sources "sound/oplgenc.c")
list(APPEND NP2kai_common_sources "sound/oplgeng.c")
list(APPEND NP2kai_common_sources "sound/opngenc.c")
list(APPEND NP2kai_common_sources "sound/opngeng.c")
list(APPEND NP2kai_common_sources "sound/opntimer.c")
list(APPEND NP2kai_common_sources "sound/pcm86c.c")
list(APPEND NP2kai_common_sources "sound/pcm86g.c")
list(APPEND NP2kai_common_sources "sound/pcmmix.c")
list(APPEND NP2kai_common_sources "sound/psggenc.c")
list(APPEND NP2kai_common_sources "sound/psggeng.c")
list(APPEND NP2kai_common_sources "sound/rhythmc.c")
list(APPEND NP2kai_common_sources "sound/s98.c")
list(APPEND NP2kai_common_sources "sound/sndcsec.c")
list(APPEND NP2kai_common_sources "sound/sound.c")
list(APPEND NP2kai_common_sources "sound/soundrom.c")
list(APPEND NP2kai_common_sources "sound/tms3631c.c")
list(APPEND NP2kai_common_sources "sound/tms3631g.c")
file(GLOB_RECURSE NP2kai_temp "sound/fmgen/*.c" "sound/fmgen/*.cpp")
list(APPEND NP2kai_common_sources ${NP2kai_temp})
file(GLOB_RECURSE NP2kai_temp "sound/getsnd/*.c")
list(APPEND NP2kai_common_sources ${NP2kai_temp})
file(GLOB_RECURSE NP2kai_temp "sound/mame/*.c")
list(APPEND NP2kai_common_sources ${NP2kai_temp})
file(GLOB_RECURSE NP2kai_temp "sound/vermouth/*.c")
list(APPEND NP2kai_common_sources ${NP2kai_temp})
file(GLOB_RECURSE NP2kai_temp "trap/*.c")
list(APPEND NP2kai_common_sources ${NP2kai_temp})
file(GLOB_RECURSE NP2kai_temp "vram/*.c")
list(APPEND NP2kai_common_sources ${NP2kai_temp})
file(GLOB_RECURSE NP2kai_temp "wab/*.c")
list(APPEND NP2kai_common_sources ${NP2kai_temp})

list(APPEND NP2kai_normal_sources "debugsub.c")
file(GLOB_RECURSE NP2kai_temp "i286c/*.c")
list(APPEND NP2kai_normal_sources ${NP2kai_temp})

option(BUILD_IA32 "Build IA-32" OFF)
list(APPEND NP2kai_ia32_sources "debugsub386.c")
file(GLOB NP2kai_temp "i386c/*.c")
list(APPEND NP2kai_ia32_sources ${NP2kai_temp})
file(GLOB NP2kai_temp "i386c/ia32/*.c")
list(APPEND NP2kai_ia32_sources ${NP2kai_temp})
file(GLOB NP2kai_temp "i386c/ia32/instructions/*.c")
list(APPEND NP2kai_ia32_sources ${NP2kai_temp})
list(APPEND NP2kai_ia32_sources "i386c/ia32/instructions/fpu/fpdummy.c")
list(APPEND NP2kai_ia32_sources "i386c/ia32/instructions/fpu/fpemul_dosbox.c")
list(APPEND NP2kai_ia32_sources "i386c/ia32/instructions/fpu/fpemul_dosbox2.c")
list(APPEND NP2kai_ia32_sources "i386c/ia32/instructions/fpu/fpemul_softfloat.c")
file(GLOB NP2kai_temp "i386c/ia32/instructions/fpu/softfloat/*.c")
list(APPEND NP2kai_ia32_sources ${NP2kai_temp})
list(APPEND NP2kai_ia32_sources "i386c/ia32/instructions/mmx/3dnow.c")
list(APPEND NP2kai_ia32_sources "i386c/ia32/instructions/mmx/mmx.c")
list(APPEND NP2kai_ia32_sources "i386c/ia32/instructions/sse/sse.c")
list(APPEND NP2kai_ia32_sources "i386c/ia32/instructions/sse2/sse2.c")
list(APPEND NP2kai_ia32_sources "i386c/ia32/instructions/sse3/sse3.c")

option(BUILD_HAXM "Build with HAXM" OFF)
file(GLOB_RECURSE NP2kai_temp "i386hax/*.c")
list(APPEND NP2kai_HAXM_sources ${NP2kai_temp})

file(GLOB NP2kai_temp "sdl/*.c")
list(APPEND NP2kai_SDL_sources ${NP2kai_temp})
file(GLOB_RECURSE NP2kai_temp "embed/*.c")
list(APPEND NP2kai_SDL_sources ${NP2kai_temp})
list(APPEND NP2kai_SDL_sources "codecnv/textcnv.c")
list(APPEND NP2kai_SDL_sources "sound/opl3.c")
list(APPEND NP2kai_SDL_sources "sound/opna.c")
list(APPEND NP2kai_SDL_Windows_sources "sdl/windows/main.c")
list(APPEND NP2kai_SDL_UNIX_sources "sdl/unix/main.c")
list(APPEND NP2kai_libretro_sources "sdl/libretro/libretro.c")
list(APPEND NP2kai_OpenDingux_sources "sdl/opendingux/main.c")

file(GLOB NP2kai_temp "x/*.c")
list(APPEND NP2kai_X_sources ${NP2kai_temp})
list(APPEND NP2kai_X_sources "x/gtk2/dialog_about.c")
list(APPEND NP2kai_X_sources "x/gtk2/dialog_calendar.c")
list(APPEND NP2kai_X_sources "x/gtk2/dialog_config.c")
list(APPEND NP2kai_X_sources "x/gtk2/dialog_hostdrv.c")
list(APPEND NP2kai_X_sources "x/gtk2/dialog_ide.c")
list(APPEND NP2kai_X_sources "x/gtk2/dialog_midi.c")
list(APPEND NP2kai_X_sources "x/gtk2/dialog_network.c")
list(APPEND NP2kai_X_sources "x/gtk2/dialog_newdisk.c")
list(APPEND NP2kai_X_sources "x/gtk2/dialog_pci.c")
list(APPEND NP2kai_X_sources "x/gtk2/dialog_screen.c")
list(APPEND NP2kai_X_sources "x/gtk2/dialog_serial.c")
list(APPEND NP2kai_X_sources "x/gtk2/dialog_sound.c")
list(APPEND NP2kai_X_sources "x/gtk2/dialog_wab.c")
list(APPEND NP2kai_X_sources "x/gtk2/gtk_drawmng.c")
list(APPEND NP2kai_X_sources "x/gtk2/gtk_mouse.c")
list(APPEND NP2kai_X_sources "x/gtk2/gtk_keyboard.c")
list(APPEND NP2kai_X_sources "x/gtk2/gtk_main.c")
list(APPEND NP2kai_X_sources "x/gtk2/gtk_menu.c")
list(APPEND NP2kai_X_sources "x/gtk2/gtk_mouse.c")
list(APPEND NP2kai_X_sources "x/gtk2/gtk_screen.c")
list(APPEND NP2kai_X_sources "x/gtk2/gtk_wrapper.c")
list(APPEND NP2kai_X_sources "x/gtk2/window_keydisp.c")
list(APPEND NP2kai_X_sources "x/gtk2/window_softkbd.c")
list(APPEND NP2kai_X_sources "codecnv/textcnv.c")
file(GLOB_RECURSE NP2kai_temp "x/misc/*.cpp")
list(APPEND NP2kai_X_sources ${NP2kai_temp})
file(GLOB_RECURSE NP2kai_temp "x/ext/*.cpp")
list(APPEND NP2kai_X_sources ${NP2kai_temp})

list(APPEND NP2kai_Windows_sources "diskimage/cd/cdd_real.c")
#file(GLOB_RECURSE NP2kai_temp "windows/*.c" "windows/*.cpp")
file(GLOB NP2kai_temp "windows/*.cpp")
list(APPEND NP2kai_Windows_sources ${NP2kai_temp})
file(GLOB NP2kai_temp "windows/commng/*.cpp")
list(APPEND NP2kai_Windows_sources ${NP2kai_temp})
file(GLOB NP2kai_temp "windows/commng/wintab/*.cpp")
list(APPEND NP2kai_Windows_sources ${NP2kai_temp})
file(GLOB NP2kai_temp "windows/debuguty/*.cpp")
list(APPEND NP2kai_Windows_sources ${NP2kai_temp})
file(GLOB NP2kai_temp "windows/dialog/*.c" "windows/dialog/*.cpp")
list(APPEND NP2kai_Windows_sources ${NP2kai_temp})
file(GLOB_RECURSE NP2kai_temp "windows/ext/*.cpp")
list(APPEND NP2kai_Windows_sources ${NP2kai_temp})
file(GLOB NP2kai_temp "windows/misc/*.cpp")
list(APPEND NP2kai_Windows_sources ${NP2kai_temp})
file(GLOB NP2kai_temp "windows/resources/*.cpp")
list(APPEND NP2kai_Windows_sources ${NP2kai_temp})
file(GLOB_RECURSE NP2kai_temp "windows/soundmng/*.cpp")
list(APPEND NP2kai_Windows_sources ${NP2kai_temp})
file(GLOB NP2kai_temp "windows/subwnd/*.cpp")
list(APPEND NP2kai_Windows_sources ${NP2kai_temp})
list(APPEND NP2kai_Windows_sources "windows/resources/1252/np2.rc")

# NP2kai definitions
list(APPEND NP2kai_common_definitions "BIOS_IO_EMULATION" "OSLANG_UTF8" "SUPPORT_8BPP" "SUPPORT_16BPP" "SUPPORT_24BPP" "SUPPORT_32BPP" "SUPPORT_CL_GD5430" "SUPPORT_CRT15KHZ" "SUPPORT_FAST_MEMORYCHECK" "SUPPORT_FMGEN" "SUPPORT_GPIB" "SUPPORT_HOSTDRV" "SUPPORT_HRTIMER" "SUPPORT_IDEIO" "SUPPORT_KAI_IMAGES" "SUPPORT_LARGE_HDD" "SUPPORT_NORMALDISP" "SUPPORT_PC9861K" "SUPPORT_PX" "SUPPORT_RESUME" "SUPPORT_RS232C_FIFO" "SUPPORT_S98" "SUPPORT_SCSI" "SUPPORT_SMPU98" "SUPPORT_SOFTKBD=0" "SUPPORT_SOUND_SB16" "SUPPORT_STATSAVE=10" "SUPPORT_UTF8" "SUPPORT_V30EXT" "SUPPORT_V30ORIGINAL" "SUPPORT_VPCVHD" "SUPPORT_WAB" "USE_MAME" "VAEG_FIX" "VERMOUTH_LIB")

list(APPEND NP2kai_extra_definitions "SUPPORT_SMPU98" "SUPPORT_NVL_IMAGES" "SUPPORT_SWSEEKSND" "SUPPORT_SWWABRLYSND")

list(APPEND NP2kai_ia32_definitions "CPUCORE_IA32" "IA32_PAGING_EACHSIZE" "IA32_REBOOT_ON_PANIC" "SUPPORT_CRT31KHZ" "SUPPORT_FPU_DOSBOX" "SUPPORT_FPU_DOSBOX2" "SUPPORT_FPU_SOFTFLOAT" "SUPPORT_GAMEPORT" "SUPPORT_MEMDBG32" "SUPPORT_LARGE_MEMORY" "SUPPORT_PC9801_119" "SUPPORT_PC9821" "SUPPORT_PCI" "SUPPORT_PEGC" "SUPPORT_SOUND_SB16" "SUPPORT_VGA_MODEX" "USE_3DNOW" "USE_FASTPAGING" "USE_FPU" "USE_MMX" "USE_SSE" "USE_SSE2" "USE_SSE3" "USE_TSC" "USE_VME")

list(APPEND NP2kai_SDL_definitions "NP2_SDL" "SUPPORT_BMS")
if(WIN32)
	list(APPEND NP2kai_LinkBreak_definitions "OSLINEBREAK_CRLF")
else()
	list(APPEND NP2kai_LinkBreak_definitions "OSLINEBREAK_LF")
endif()

list(APPEND NP2kai_X_definitions "NP2_X" "USE_RESOURCE_BMP" "SUPPORT_BMS")

list(APPEND NP2kai_Windows_definitions "NP2_WIN" "USE_RESOURCE_BMP")

# build type
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "RelWithDebInfo")
endif()
if(NOT EMSCRIPTEN)
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -Wall -DDEBUG -D_DEBUG -DTRACE")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -Wall -DNDEBUG -D_NDEBUG")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -Wall")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-Og -g -Wall -DNDEBUG -D_NDEBUG")
endif()
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")

# build option
option(BUILD_OPENDINGUX_GCW0 "Build OpenDingux GCW0 port" OFF)
option(BUILD_OPENDINGUX_RG350 "Build OpenDingux RG350 port" OFF)
option(BUILD_OPENDINGUX_RS90 "Build OpenDingux RS90 port" OFF)

if(EMSCRIPTEN)
elseif(BUILD_OPENDINGUX_GCW0)
	include("cmake/CrossOpenDinguxGCW0.cmake")
elseif(BUILD_OPENDINGUX_RG350)
	include("cmake/CrossOpenDinguxRG350.cmake")
elseif(BUILD_OPENDINGUX_RS90)
	include("cmake/CrossOpenDinguxRS90.cmake")
endif()

option(BUILD_I286 "Build normal(i286 older) NP2" OFF)
option(USE_SDL2 "Use SDL2" ON)
if(USE_SDL2)
	set(USE_SDL_VERSION 2)
else()
	set(USE_SDL_VERSION 1)
endif()
option(USE_SDL_MIXER "Use SDL_mixer" ON)
option(USE_SDL_TTF "Use SDL_ttf" ON)
if(NOT CMAKE_CROSSCOMPILING)
	if(MINGW OR MSYS OR CYGWIN OR CMAKE_HOST_APPLE)
		option(BUILD_SDL "Build SDL port" ON)
	else()
		option(BUILD_SDL "Build SDL port" OFF)
	endif()
	if(UNIX)
		option(BUILD_X "Build X port" ON)
	else()
		option(BUILD_X "Build X port" OFF)
	endif()
	option(BUILD_HAXM "Build HAXM port" OFF)

	option(USE_SDL "Use SDL" OFF)
	option(USE_HAXM "Use HAXM" OFF)

	option(USE_X "Use X" OFF)
	if(BUILD_X)
		set(USE_X ON)
	endif()
	option(USE_THREAD "Use Thread" ON)
	option(USE_USB "Use USB" ON)
	option(USE_VST3SDK "Use VST3SDK" OFF)
endif()


# function
option(USE_TICKCOUNT "Use TickCount" ON)
if(USE_TICKCOUNT)
	list(APPEND NP2kai_TickCount_definitions "SUPPORT_NP2_TICKCOUNT")
	list(APPEND NP2kai_TickCount_sources "np2_tickcount.c")
endif()

option(USE_NETWORK "Use Network" ON)
if(USE_NETWORK)
	list(APPEND NP2kai_Network_definitions "SUPPORT_NET" "SUPPORT_LGY98")
	file(GLOB_RECURSE NP2kai_temp "network/*.c")
	list(APPEND NP2kai_Network_sources ${NP2kai_temp})
endif()

option(USE_ASYNCCPU "Use AsyncCPU" OFF)
if(USE_ASYNCCPU)
	list(APPEND NP2kai_AsyncCPU_definitions "SUPPORT_ASYNC_CPU")
endif()

if(NOT CMAKE_CROSSCOMPILING)

	# math
	if(NOT WIN32)
		list(APPEND lib_math_libraries m)
	endif()

	# SDL
	find_package(SDL)
	if(USE_SDL AND USE_SDL_VERSION EQUAL 1)
		if(NOT SDL_FOUND)
			message(FATAL_ERROR "-- Not found SDL")
		endif()
	endif()
	find_path(SDL2_CMAKE "FindSDL2.cmake" "cmake/sdl2-cmake-scripts")
	if(NOT SDL2_CMAKE)
		execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive ${NP2kai_SOURCE_DIR}/cmake/sdl2-cmake-scripts)
	endif()
	set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${NP2kai_SOURCE_DIR}/cmake/sdl2-cmake-scripts")
	find_package(SDL2)
	if(USE_SDL AND USE_SDL_VERSION EQUAL 2)
		if(NOT SDL2_FOUND)
			message(FATAL_ERROR "-- Not found SDL2")
		endif()
	endif()
	if(USE_SDL AND (SDL_FOUND OR SDL2_FOUND))
		list(APPEND lib_SDL_definitions "SUPPORT_SDL" "SUPPORT_SDL_AUDIO")
	endif()

	# SDL_mixer
	find_package(SDL_mixer)
	if(USE_SDL_MIXER AND USE_SDL_VERSION EQUAL 1)
		if(NOT SDL_mixer_FOUND)
			message(FATAL_ERROR "-- Not found SDL_mixer")
		endif()
	endif()
	find_package(SDL2_mixer)
	if(USE_SDL_MIXER AND USE_SDL_VERSION EQUAL 2)
		if(NOT SDL2_mixer_FOUND)
			message(FATAL_ERROR "-- Not found SDL2_mixer")
		endif()
	endif()
	if(USE_SDL_MIXER AND (SDL_mixer_FOUND OR SDL2_mixer_FOUND))
		list(APPEND lib_SDL_mixer_definitions "SUPPORT_SDL_MIXER")
	endif()

	# SDL_ttf
	find_package(SDL_ttf)
	if(USE_SDL_TTF AND USE_SDL_VERSION EQUAL 1)
		if(NOT SDL_ttf_FOUND)
			message(FATAL_ERROR "-- Not found SDL_ttf")
		endif()
	endif()
	find_package(SDL2_ttf)
	if(USE_SDL_TTF AND USE_SDL_VERSION EQUAL 2)
		if(NOT SDL2_ttf_FOUND)
			message(FATAL_ERROR "-- Not found SDL2_ttf")
		endif()
	endif()
	if(USE_SDL_TTF AND (SDL_ttf_FOUND OR SDL2_ttf_FOUND))
		list(APPEND lib_SDL_ttf_definitions "SUPPORT_SDL_TTF")
	endif()

	# Thread
	if(WIN32)
		list(APPEND lib_Thread_libraries winmm)
	else()
		find_package(Threads)
		if(CMAKE_THREAD_LIBS_INIT)
			list(APPEND lib_Thread_libraries pthread)
		else()
			if(USE_THREAD)
				message(FATAL_ERROR "-- Not found phread")
			endif()
		endif()
	endif()
	if(USE_THREAD)
		list(APPEND lib_Thread_definitions "SUPPORT_NP2_THREAD" "SOUND_CRITICAL")
		list(APPEND NP2kai_Thread_sources "np2_thread.c")
	endif()

	# libusb
	if(NOT WIN32)
		find_path(LIBUSB_INCLUDE_DIR NAMES libusb.h PATH_SUFFIXES "include" "libusb" "libusb-1.0")
		find_library(LIBUSB_LIBRARY NAMES usb PATH_SUFFIXES "lib" "lib32" "lib64")
		if(NOT LIBUSB_INCLUDE_DIR STREQUAL "" AND NOT LIBUSB_LIBRARY STREQUAL "")
			message("-- Found libusb")
		else()
			if(USE_USB)
				message(FATAL_ERROR "-- Not found libusb")
			else()
				message("-- Not found libusb")
			endif()
		endif()
	endif()
	if(USE_USB)
		list(APPEND lib_usb_definitions "SUPPORT_USB")
		list(APPEND lib_usb_include_dirs ${LIBUSB_INCLUDE_DIR})
		list(APPEND lib_usb_libraries ${LIBUSB_LIBRARY})
	endif()

	# X
	if(USE_X)
		# X11
		find_package(X11 REQUIRED)

		# glib-2.0
		find_package(PkgConfig REQUIRED)
		pkg_check_modules(GLIB REQUIRED glib-2.0)
		find_path(lib_glib_include_dirs NAMES glib.h HINTS ${GLIB_INCLUDEDIR} ${GLIB_INCLUDE_DIRS})
		find_path(lib_glibconfig_include_dirs NAMES glibconfig.h HINTS ${GLIB_INCLUDEDIR} ${GLIB_INCLUDE_DIRS})
		find_library(lib_glib_libraries NAMES glib glib-2.0 PATH_SUFFIXES glib-2.0)
		if(lib_glib_include_dirs AND lib_glibconfig_include_dirs AND lib_glib_libraries)
			message("-- Found glib2.0")
		else()
			message(FATAL_ERROR "-- Not found glib2.0")
		endif()

		# GTK2
		find_package(GTK2 REQUIRED)
	endif()

	# HAXM
	if(USE_HAXM)
		if(NOT CMAKE_CROSSCOMPILING)
			if(WIN32)
				find_path(HAXM "HAX" "\\\\.\\")
			else()
				find_path(HAXM "HAX" "/dev")
			endif()
			if(NOT HAXM)
				message(FATAL_ERROR "-- Not found HAXM")
			else()
				message("-- Found HAXM")
			endif()
		endif()
		list(APPEND NP2kai_HAXM_definitions "SUPPORT_IA32_HAXM" "USE_CUSTOM_HOOKINST")
	endif()

	# VST3 SDK
	if(USE_VST3SDK)
		find_path(VST3SDK_CMAKE "README.md" "cmake/vst3_cmake")
		if(NOT VST3SDK)
			execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive ${NP2kai_SOURCE_DIR}/cmake/vst3_cmake)
		endif()
		set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${NP2kai_SOURCE_DIR}/cmake/vst3_cmake/modules")
#		set(SMTG_MYPLUGINS_SRC_PATH "sdl/---")
#		set(SMTG_MYPLUGINS_SRC_PATH "x/---")
#		set(SMTG_MYPLUGINS_SRC_PATH "windows/---")
		find_path(VST3SDK "CMakeLists.txt" "vst3sdk")
		if(NOT VST3SDK)
			execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive ${NP2kai_SOURCE_DIR}/vst3sdk)
		endif()
		add_subdirectory("vst3sdk")
		list(APPEND lib_vst3sdk_libraries "vst3sdk")
	endif()

	# WDK
	if(MSVC_VERSION)
		find_path(WDK_CMAKE "README.md" "cmake/FindWDK")
		if(NOT WDK_CMAKE)
			execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive ${NP2kai_SOURCE_DIR}/cmake/FindWDK)
		endif()
		set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${NP2kai_SOURCE_DIR}/cmake/FindWDK/cmake")
		find_package(WDK REQUIRED)
	endif()

endif()

if(CMAKE_CROSSCOMPILING)

	# Enscripten
	if(EMSCRIPTEN)
		if(USE_SDL_VERSION EQUAL 1 AND (USE_SDL_MIXER OR USE_SDL_TTL))
			message(FATAL_ERROR "-- Emscripten SDL1 NP2kai cannot be with SDL_mixer and SDL_ttf (-D USE_SDL_MIXER=OFF -D USE_SDL_TTF=OFF)")
		endif()

	# OpenDigux
	elseif(BUILD_OPENDINGUX_GCW0)
		find_path(gcw0toolchain "bin" "/opt/gcw0-toolchain/usr")
		if(gcw0toolchain)
			message("-- Found OpenDingux GCW0 toolchain")
		else()
			message(FATAL_ERROR "-- Not found OpenDingux GCW0 toolchain")
		endif()

	elseif(BUILD_OPENDINGUX_RG350)
#		find_path(rg350toolchain "bin" "/opt/rg350-toolchain/usr")
		set(rg350toolchain true)
		if(rg350toolchain)
			message("-- Found OpenDingux RG350 toolchain")
		else()
			message(FATAL_ERROR "-- Not found OpenDingux RG350 toolchain")
		endif()

	elseif(BUILD_OPENDINGUX_RS90)
		if(USE_SDL_VERSION EQUAL 2 OR USE_SDL_TTF)
			message(FATAL_ERROR "-- OpenDingux RS90 NP2kai cannot be with SDL2 and SDL_ttf (-D USE_SDL2=OFF -D USE_SDL_TTF=OFF)")
		endif()

		find_path(rs90toolchain "bin" "/opt/rs90-toolchain/usr")
		if(rs90toolchain)
			message("-- Found OpenDingux RS90 toolchain")
		else()
			message(FATAL_ERROR "-- Not found OpenDingux RS90 toolchain")
		endif()
	endif()

endif()

# target base
add_library(NP2kai_bbase INTERFACE)
target_compile_definitions(NP2kai_bbase INTERFACE ${NP2kai_common_definitions} "NP2KAI_GIT_TAG=\"${NP2kai_GIT_TAG}\"" "NP2KAI_GIT_HASH=\"${NP2kai_GIT_HASH}\"")
target_include_directories(NP2kai_bbase INTERFACE ${NP2kai_common_include_dirs})
target_sources(NP2kai_bbase INTERFACE ${NP2kai_common_sources})
if(WIN32)
	target_link_libraries(NP2kai_bbase INTERFACE)
else()
	target_link_libraries(NP2kai_bbase INTERFACE m)
endif()

add_library(NP2kai_base INTERFACE)
target_compile_definitions(NP2kai_base INTERFACE ${NP2kai_normal_definitions})
target_include_directories(NP2kai_base INTERFACE ${NP2kai_normal_include_dirs})
target_sources(NP2kai_base INTERFACE ${NP2kai_normal_sources})
target_link_libraries(NP2kai_base INTERFACE NP2kai_bbase)

add_library(NP21kai_base INTERFACE)
target_compile_definitions(NP21kai_base INTERFACE ${NP2kai_ia32_definitions})
target_include_directories(NP21kai_base INTERFACE ${NP2kai_ia32_include_dirs})
target_sources(NP21kai_base INTERFACE ${NP2kai_ia32_sources})
target_link_libraries(NP21kai_base INTERFACE NP2kai_bbase)

add_library(NP21kai_HAXM_base INTERFACE)
target_compile_definitions(NP21kai_HAXM_base INTERFACE ${NP2kai_HAXM_definitions})
target_sources(NP21kai_HAXM_base INTERFACE ${NP2kai_HAXM_sources})
target_link_libraries(NP21kai_HAXM_base INTERFACE NP21kai_base)

if(CMAKE_CROSSCOMPILING)

	if(EMSCRIPTEN)
		set(NP2kai_Emscripten_MinMemorySize 67108864)
		set(NP2kai_Emscripten_MaxMemorySize 1073741824)

		add_library(NP2kai_Emscripten_SDL1_base INTERFACE)
#		target_compile_definitions(NP2kai_Emscripten_SDL1_base INTERFACE ${NP2kai_SDL_definitions} "OSLINEBREAK_LF" "SUPPORT_SDL_AUDIO" "SUPPORT_SDL_MIXER" "SUPPORT_SDL_TTF" "SUPPORT_NP2_TICKCOUNT")
		target_compile_definitions(NP2kai_Emscripten_SDL1_base INTERFACE ${NP2kai_SDL_definitions} "OSLINEBREAK_LF" "SUPPORT_SDL_AUDIO" "SUPPORT_NP2_TICKCOUNT")
		target_include_directories(NP2kai_Emscripten_SDL1_base INTERFACE ${NP2kai_Emscripten_include_dirs})
		target_compile_options(NP2kai_Emscripten_SDL1_base INTERFACE "SHELL:-s USE_SDL=1" "SHELL:-s USE_SDL_MIXER=1" "SHELL:-s USE_SDL_TTF=1")
		target_link_options(NP2kai_Emscripten_SDL1_base INTERFACE "SHELL:-s INITIAL_MEMORY=${NP2kai_Emscripten_MinMemorySize}" "SHELL:-s MAXIMUM_MEMORY=${NP2kai_Emscripten_MaxMemorySize}" "SHELL:-s USE_SDL=1" "SHELL:-s USE_SDL_MIXER=1" "SHELL:-s USE_SDL_TTF=1" "SHELL:-s ASSERTIONS=1")
		target_link_libraries(NP2kai_Emscripten_SDL1_base INTERFACE m)
		target_sources(NP2kai_Emscripten_SDL1_base INTERFACE ${NP2kai_SDL_sources} "sdl/em/main.c" "sdl/em/SDL_mixer.c")

		add_library(NP2kai_Emscripten_SDL2_base INTERFACE)
		target_compile_definitions(NP2kai_Emscripten_SDL2_base INTERFACE ${NP2kai_SDL_definitions} "OSLINEBREAK_LF" "SUPPORT_SDL_AUDIO" "SUPPORT_SDL_MIXER" "SUPPORT_SDL_TTF" "SUPPORT_NP2_TICKCOUNT")
		target_include_directories(NP2kai_Emscripten_SDL2_base INTERFACE ${NP2kai_Emscripten_include_dirs})
		target_compile_options(NP2kai_Emscripten_SDL2_base INTERFACE "SHELL:-s USE_SDL=2" "SHELL:-s USE_SDL_MIXER=2" "SHELL:-s USE_SDL_TTF=2")
		target_link_options(NP2kai_Emscripten_SDL2_base INTERFACE "SHELL:-s INITIAL_MEMORY=${NP2kai_Emscripten_MinMemorySize}" "SHELL:-s MAXIMUM_MEMORY=${NP2kai_Emscripten_MaxMemorySize}" "SHELL:-s USE_SDL=2" "SHELL:-s USE_SDL_MIXER=2" "SHELL:-s USE_SDL_TTF=2" "SHELL:-s ASSERTIONS=1")
		target_link_libraries(NP2kai_Emscripten_SDL2_base INTERFACE m)
		target_sources(NP2kai_Emscripten_SDL2_base INTERFACE ${NP2kai_SDL_sources}  "sdl/em/main.c")
	endif()

	if(BUILD_OPENDINGUX_GCW0)
		add_library(NP2kai_gcw0_SDL1_base INTERFACE)
		target_compile_definitions(NP2kai_gcw0_SDL1_base INTERFACE ${NP2kai_SDL_definitions} "__OPENDINGUX__" "OSLINEBREAK_LF" "SUPPORT_SDL_AUDIO" "SUPPORT_SDL_MIXER" "SUPPORT_SDL_TTF" "SUPPORT_NP2_TICKCOUNT" "SUPPORT_NP2_THREAD" ${NP2kai_TickCount_definitions} ${NP2kai_Network_definitions})
		target_sources(NP2kai_gcw0_SDL1_base INTERFACE ${NP2kai_SDL_sources} ${NP2kai_OpenDingux_sources} "np2_thread.c" ${NP2kai_TickCount_sources} ${NP2kai_Network_sources})
		target_link_directories(NP2kai_gcw0_SDL1_base INTERFACE "/opt/gcw0-toolchain/usr/mipsel-gcw0-linux-uclibc/sysroot/lib")
		target_include_directories(NP2kai_gcw0_SDL1_base INTERFACE ${NP2kai_OpenDingux_include_dirs} "/opt/gcw0-toolchain/mipsel-gcw0-linux-uclibc/sysroot/usr/include" "/opt/gcw0-toolchain/usr/mipsel-gcw0-linux-uclibc/sysroot/usr/include/SDL")
		target_link_libraries(NP2kai_gcw0_SDL1_base INTERFACE m pthread SDL SDL_mixer SDL_ttf)

		add_library(NP2kai_gcw0_SDL2_base INTERFACE)
		target_compile_definitions(NP2kai_gcw0_SDL2_base INTERFACE ${NP2kai_SDL_definitions} "__OPENDINGUX__" "OSLINEBREAK_LF" "SUPPORT_SDL_AUDIO" "SUPPORT_SDL_MIXER" "SUPPORT_SDL_TTF" "SUPPORT_NP2_TICKCOUNT" "SUPPORT_NP2_THREAD" ${NP2kai_TickCount_definitions} ${NP2kai_Network_definitions})
		target_sources(NP2kai_gcw0_SDL2_base INTERFACE ${NP2kai_SDL_sources} ${NP2kai_OpenDingux_sources} "np2_thread.c" ${NP2kai_TickCount_sources} ${NP2kai_Network_sources})
		target_link_directories(NP2kai_gcw0_SDL2_base INTERFACE "/opt/gcw0-toolchain/usr/mipsel-gcw0-linux-uclibc/sysroot/lib")
		target_include_directories(NP2kai_gcw0_SDL2_base INTERFACE ${NP2kai_OpenDingux_include_dirs} "/opt/gcw0-toolchain/mipsel-gcw0-linux-uclibc/sysroot/usr/include" "/opt/gcw0-toolchain/usr/mipsel-gcw0-linux-uclibc/sysroot/usr/include/SDL2")
		target_link_libraries(NP2kai_gcw0_SDL2_base INTERFACE m pthread SDL2 SDL2_mixer SDL2_ttf)
	endif()

	if(BUILD_OPENDINGUX_RG350)
		add_library(NP2kai_rg350_SDL1_base INTERFACE)
		target_compile_definitions(NP2kai_rg350_SDL1_base INTERFACE ${NP2kai_SDL_definitions} "__OPENDINGUX__" "OSLINEBREAK_LF" "SUPPORT_SDL_AUDIO" "SUPPORT_SDL_MIXER" "SUPPORT_SDL_TTF" "SUPPORT_NP2_TICKCOUNT" "SUPPORT_NP2_THREAD" ${NP2kai_TickCount_definitions} ${NP2kai_Network_definitions})
		target_sources(NP2kai_rg350_SDL1_base INTERFACE ${NP2kai_SDL_sources} ${NP2kai_OpenDingux_sources} "np2_thread.c" ${NP2kai_TickCount_sources} ${NP2kai_Network_sources})
		target_link_directories(NP2kai_rg350_SDL1_base INTERFACE "/opt/rg350-toolchain/usr/mipsel-gcw0-linux-uclibc/sysroot/usr/lib")
		target_include_directories(NP2kai_rg350_SDL1_base INTERFACE ${NP2kai_OpenDingux_include_dirs} "/opt/rg350-toolchain/usr/mipsel-rg350-linux-uclibc/sysroot/usr/include" "/opt/rg350-toolchain/usr/mipsel-rg350-linux-uclibc/sysroot/usr/include/SDL")
		target_link_libraries(NP2kai_rg350_SDL1_base INTERFACE m pthread SDL SDL_mixer SDL_ttf)

		add_library(NP2kai_rg350_SDL2_base INTERFACE)
		target_compile_definitions(NP2kai_rg350_SDL2_base INTERFACE ${NP2kai_SDL_definitions} "__OPENDINGUX__" "OSLINEBREAK_LF" "SUPPORT_SDL_AUDIO" "SUPPORT_SDL_MIXER" "SUPPORT_SDL_TTF" "SUPPORT_NP2_TICKCOUNT" "SUPPORT_NP2_THREAD" ${NP2kai_TickCount_definitions} ${NP2kai_Network_definitions})
		target_sources(NP2kai_rg350_SDL2_base INTERFACE ${NP2kai_SDL_sources} ${NP2kai_OpenDingux_sources} "np2_thread.c" ${NP2kai_TickCount_sources} ${NP2kai_Network_sources})
		target_link_directories(NP2kai_rg350_SDL2_base INTERFACE "/opt/rg350-toolchain/usr/mipsel-gcw0-linux-uclibc/sysroot/usr/lib")
		target_include_directories(NP2kai_rg350_SDL2_base INTERFACE ${NP2kai_OpenDingux_include_dirs} "/opt/rg350-toolchain/usr/mipsel-rg350-linux-uclibc/sysroot/usr/include" "/opt/rg350-toolchain/usr/mipsel-rg350-linux-uclibc/sysroot/usr/include/SDL2")
		target_link_libraries(NP2kai_rg350_SDL2_base INTERFACE m pthread SDL2 SDL2_mixer SDL2_ttf)
	endif()

	if(BUILD_OPENDINGUX_RS90)
		add_library(NP2kai_rs90_SDL1_base INTERFACE)
#		target_compile_definitions(NP2kai_rs90_SDL1_base INTERFACE ${NP2kai_SDL_definitions} "__OPENDINGUX__" "OSLINEBREAK_LF" "SUPPORT_SDL_AUDIO" "SUPPORT_SDL_MIXER" "SUPPORT_SDL_TTF" "SUPPORT_NP2_TICKCOUNT" "SUPPORT_NP2_THREAD" ${NP2kai_TickCount_definitions} ${NP2kai_Network_definitions})
		target_compile_definitions(NP2kai_rs90_SDL1_base INTERFACE ${NP2kai_SDL_definitions} "__OPENDINGUX__" "OSLINEBREAK_LF" "SUPPORT_SDL_AUDIO" "SUPPORT_SDL_MIXER" "SUPPORT_NP2_TICKCOUNT" "SUPPORT_NP2_THREAD" ${NP2kai_TickCount_definitions} ${NP2kai_Network_definitions})
		target_include_directories(NP2kai_rs90_SDL1_base INTERFACE ${NP2kai_OpenDingux_include_dirs} "/opt/rs90-toolchain/mipsel-rs90-linux-musl/sysroot/usr/include" "/opt/rs90-toolchain/mipsel-rs90-linux-musl/sysroot/usr/include/SDL")
		target_sources(NP2kai_rs90_SDL1_base INTERFACE ${NP2kai_SDL_sources} ${NP2kai_OpenDingux_sources} "np2_thread.c" ${NP2kai_TickCount_sources} ${NP2kai_Network_sources})
		target_link_directories(NP2kai_rs90_SDL1_base INTERFACE "/opt/rs90-toolchain/mipsel-rs90-linux-musl/sysroot/lib")
		target_link_libraries(NP2kai_rs90_SDL1_base INTERFACE m pthread SDL SDL_mixer)

		add_library(NP2kai_rs90_base INTERFACE)
#		target_compile_definitions(NP2kai_rs90_base INTERFACE ${NP2kai_SDL_definitions} "__OPENDINGUX__" "OSLINEBREAK_LF" "SUPPORT_SDL_AUDIO" "SUPPORT_SDL_MIXER" "SUPPORT_SDL_TTF" "SUPPORT_NP2_TICKCOUNT" "SUPPORT_NP2_THREAD" ${NP2kai_TickCount_definitions} ${NP2kai_Network_definitions})
		target_compile_definitions(NP2kai_rs90_base INTERFACE ${NP2kai_SDL_definitions} "__OPENDINGUX__" "OSLINEBREAK_LF" "SUPPORT_SDL_AUDIO" "SUPPORT_SDL_MIXER" "SUPPORT_NP2_TICKCOUNT" "SUPPORT_NP2_THREAD" ${NP2kai_TickCount_definitions} ${NP2kai_Network_definitions})
		target_include_directories(NP2kai_rs90_base INTERFACE ${NP2kai_OpenDingux_include_dirs} "/opt/rs90-toolchain/mipsel-rs90-linux-musl/sysroot/usr/include" "/opt/rs90-toolchain/mipsel-rs90-linux-musl/sysroot/usr/include/SDL")
		target_sources(NP2kai_rs90_base INTERFACE ${NP2kai_SDL_sources} ${NP2kai_OpenDingux_sources} "np2_thread.c" ${NP2kai_TickCount_sources} ${NP2kai_Network_sources})
		target_link_directories(NP2kai_rs90_base INTERFACE "/opt/rs90-toolchain/mipsel-rs90-linux-musl/sysroot/lib")
		target_link_libraries(NP2kai_rs90_base INTERFACE m pthread SDL2 SDL2_mixer)
	endif()

else()

	if(MSVC_VERSION)
		add_library(NP2kai_Windows_base INTERFACE)
		target_compile_definitions(NP2kai_Windows_base INTERFACE ${NP2kai_Windows_definitions} ${NP2kai_extra_definitions} ${NP2kai_LinkBreak_definitions} ${NP2kai_TickCount_definitions} ${NP2kai_Network_definitions} ${lib_usb_definitions})
		target_include_directories(NP2kai_Windows_base INTERFACE ${NP2kai_Windows_include_dirs} ${lib_usb_include_dirs} ${lib_vst3sdk_include_dirs})
		target_sources(NP2kai_Windows_base INTERFACE ${NP2kai_Windows_sources} ${NP2kai_Thread_sources} ${NP2kai_TickCount_sources} ${NP2kai_Network_sources})
		if(CMAKE_BUILD_TYPE EQUAL "Debug")
			target_link_libraries(NP2kai_Windows_base INTERFACE ucrtd vcruntimed msvcrtd ${lib_Thread_libraries} ${lib_usb_libraries} ${lib_vst3sdk_libraries})
		else()
			target_link_libraries(NP2kai_Windows_base INTERFACE ucrt vcruntime msvcrt ${lib_Thread_libraries} ${lib_usb_libraries} ${lib_vst3sdk_libraries})
		endif()
	else()
		add_library(NP2kai_SDL_SDL1_base INTERFACE)
		target_sources(NP2kai_SDL_SDL1_base INTERFACE ${NP2kai_SDL_sources} ${NP2kai_Thread_sources} ${NP2kai_TickCount_sources} ${NP2kai_Network_sources})
		target_compile_definitions(NP2kai_SDL_SDL1_base INTERFACE ${NP2kai_SDL_definitions} ${lib_SDL_mixer_definitions} ${lib_SDL_ttf_definitions} ${NP2kai_extra_definitions} ${NP2kai_LinkBreak_definitions} ${NP2kai_TickCount_definitions} ${NP2kai_Network_definitions} ${SDL_DEFINE} ${SDL_MIXER_DEFINE} ${SDL_TTF_DEFINE} ${lib_usb_definitions})
		target_include_directories(NP2kai_SDL_SDL1_base INTERFACE ${NP2kai_SDL_UNIX_dirs} ${SDL_INCLUDE_DIR} ${SDL_MIXER_INCLUDE_DIR} ${SDL_TTF_INCLUDE_DIR} ${lib_usb_include_dirs} ${lib_vst3sdk_include_dirs})
		target_link_libraries(NP2kai_SDL_SDL1_base INTERFACE ${lib_math_libraries} ${SDL_LIBRARY} ${SDL_MIXER_LIBRARY} ${SDL_TTF_LIBRARY} ${lib_vst3sdk_libraries} ${lib_Thread_libraries} ${lib_usb_libraries} ${CMAKE_DL_LIBS})

		add_library(NP2kai_SDL_SDL2_base INTERFACE)
		target_sources(NP2kai_SDL_SDL2_base INTERFACE ${NP2kai_SDL_sources} ${NP2kai_Thread_sources} ${NP2kai_TickCount_sources} ${NP2kai_Network_sources})
		target_compile_definitions(NP2kai_SDL_SDL2_base INTERFACE ${NP2kai_SDL_definitions} ${lib_SDL_mixer_definitions} ${lib_SDL_ttf_definitions} ${NP2kai_extra_definitions} ${NP2kai_LinkBreak_definitions} ${NP2kai_TickCount_definitions} ${NP2kai_Network_definitions} ${SDL2_DEFINE} ${SDL2_MIXER_DEFINE} ${SDL2_TTF_DEFINE} ${lib_usb_definitions})
		target_include_directories(NP2kai_SDL_SDL2_base INTERFACE ${NP2kai_SDL_UNIX_dirs} ${SDL2_INCLUDE_DIR} ${SDL2_MIXER_INCLUDE_DIR} ${SDL2_TTF_INCLUDE_DIR} ${lib_usb_include_dirs} ${lib_vst3sdk_include_dirs})
		target_link_libraries(NP2kai_SDL_SDL2_base INTERFACE ${lib_math_libraries} ${SDL2_LIBRARY} ${SDL2_MIXER_LIBRARY} ${SDL2_TTF_LIBRARY} ${lib_vst3sdk_libraries} ${lib_Thread_libraries} ${lib_usb_libraries} ${CMAKE_DL_LIBS})

		add_library(NP2kai_X_SDL1_base INTERFACE)
		target_sources(NP2kai_X_SDL1_base INTERFACE ${NP2kai_X_sources} ${NP2kai_Thread_sources} ${NP2kai_TickCount_sources} ${NP2kai_Network_sources})
		target_compile_definitions(NP2kai_X_SDL1_base INTERFACE ${NP2kai_X_definitions} ${lib_SDL_definitions} ${lib_SDL_mixer_definitions} ${lib_SDL_ttf_definitions} ${NP2kai_extra_definitions} ${NP2kai_LinkBreak_definitions} ${NP2kai_TickCount_definitions} ${NP2kai_Network_definitions} ${GTK2_DEFINITIONS} ${SDL_DEFINE} ${SDL_MIXER_DEFINE} ${SDL_TTF_DEFINE} ${lib_usb_definitions} "SYSRESPATH=\"resources\"")
		target_include_directories(NP2kai_X_SDL1_base INTERFACE ${NP2kai_X_include_dirs} ${lib_glib_include_dirs} ${lib_glibconfig_include_dirs} ${GTK2_INCLUDE_DIRS} ${X11_INCLUDE_DIRS} ${SDL_INCLUDE_DIR} ${SDL_MIXER_INCLUDE_DIR} ${SDL_TTF_INCLUDE_DIR} ${lib_usb_include_dirs} ${lib_vst3sdk_include_dirs})
		target_link_libraries(NP2kai_X_SDL1_base INTERFACE ${lib_math_libraries} ${lib_glib_libraries} ${GTK2_LIBRARIES} ${X11_LIBRARIES} ${SDL_LIBRARY} ${SDL_MIXER_LIBRARY} ${SDL_TTF_LIBRARY} ${lib_Thread_libraries} ${lib_usb_libraries} ${lib_vst3sdk_libraries} ${CMAKE_DL_LIBS})

		add_library(NP2kai_X_SDL2_base INTERFACE)
		target_sources(NP2kai_X_SDL2_base INTERFACE ${NP2kai_X_sources} ${NP2kai_Thread_sources} ${NP2kai_TickCount_sources} ${NP2kai_Network_sources})
		target_compile_definitions(NP2kai_X_SDL2_base INTERFACE ${NP2kai_X_definitions} ${lib_SDL_definitions} ${lib_SDL_mixer_definitions} ${lib_SDL_ttf_definitions} ${NP2kai_extra_definitions} ${NP2kai_LinkBreak_definitions} ${NP2kai_TickCount_definitions} ${NP2kai_Network_definitions} ${GTK2_DEFINITIONS} ${SDL2_DEFINE} ${SDL2_MIXER_DEFINE} ${SDL2_TTF_DEFINE} ${lib_usb_definitions} "SYSRESPATH=\"resources\"")
		target_include_directories(NP2kai_X_SDL2_base INTERFACE ${NP2kai_X_include_dirs} ${lib_glib_include_dirs} ${lib_glibconfig_include_dirs} ${GTK2_INCLUDE_DIRS} ${X11_INCLUDE_DIRS} ${SDL2_INCLUDE_DIR} ${SDL2_MIXER_INCLUDE_DIR} ${SDL2_TTF_INCLUDE_DIR} ${lib_usb_include_dirs} ${lib_vst3sdk_include_dirs})
		target_link_libraries(NP2kai_X_SDL2_base INTERFACE ${lib_math_libraries} ${lib_glib_libraries} ${GTK2_LIBRARIES} ${X11_LIBRARIES} ${SDL2_LIBRARY} ${SDL2_MIXER_LIBRARY} ${SDL2_TTF_LIBRARY} ${lib_Thread_libraries} ${lib_usb_libraries} ${lib_vst3sdk_libraries} ${CMAKE_DL_LIBS})
	endif()

endif()

# target
if(CMAKE_CROSSCOMPILING)

	if(EMSCRIPTEN)
		set(CMAKE_EXECUTABLE_SUFFIX ".html")

		add_executable(emnp2kai_sdl1)
		target_link_options(emnp2kai_sdl1 PUBLIC "SHELL:-s INITIAL_MEMORY=${NP2kai_Emscripten_MinMemorySize}" "SHELL:-s MAXIMUM_MEMORY=33554432")
		target_link_libraries(emnp2kai_sdl1 PUBLIC NP2kai_base NP2kai_Emscripten_SDL1_base)

		add_executable(emnp21kai_sdl1)
		target_link_options(emnp21kai_sdl1 PUBLIC "SHELL:-s INITIAL_MEMORY=${NP2kai_Emscripten_MinMemorySize}" "SHELL:-s MAXIMUM_MEMORY=1073741824")
		target_link_libraries(emnp21kai_sdl1 PUBLIC NP21kai_base NP2kai_Emscripten_SDL1_base)

		add_executable(emnp2kai)
		target_link_options(emnp2kai PUBLIC "SHELL:-s INITIAL_MEMORY=${NP2kai_Emscripten_MinMemorySize}" "SHELL:-s MAXIMUM_MEMORY=33554432")
		target_link_libraries(emnp2kai PUBLIC NP2kai_base NP2kai_Emscripten_SDL2_base)

		add_executable(emnp21kai)
		target_link_options(emnp21kai PUBLIC "SHELL:-s INITIAL_MEMORY=${NP2kai_Emscripten_MinMemorySize}" "SHELL:-s MAXIMUM_MEMORY=1073741824")
		target_link_libraries(emnp21kai PUBLIC NP21kai_base NP2kai_Emscripten_SDL2_base)

		if(NOT BUILD_I286)
			set_target_properties(emnp2kai_sdl1 emnp2kai PROPERTIES EXCLUDE_FROM_ALL true)
		endif()
		if(USE_SDL_VERSION EQUAL 1)
			set_target_properties(emnp2kai emnp21kai PROPERTIES EXCLUDE_FROM_ALL true)
		else()
			set_target_properties(emnp2kai_sdl1 emnp21kai_sdl1 PROPERTIES EXCLUDE_FROM_ALL true)
		endif()
	endif()

	if(BUILD_OPENDINGUX_GCW0)
		set(CMAKE_EXECUTABLE_SUFFIX ".opk")

		add_executable(np2kai_gcw0_sdl1)
		target_link_libraries(np2kai_gcw0_sdl1 PUBLIC NP2kai_base NP2kai_gcw0_SDL1_base)
		add_custom_command(
			TARGET np2kai_gcw0_sdl1
			POST_BUILD
			COMMENT "Genarate .opk pack."
			COMMAND cp -r ${NP2kai_SOURCE_DIR}/sdl/opendingux/opk ${NP2kai_BINARY_DIR}/np2kai_gcw0_sdl1_opk
			COMMAND mv np2kai_gcw0_sdl1.opk ${NP2kai_BINARY_DIR}/np2kai_gcw0_sdl1_opk/
			COMMAND rm ${NP2kai_BINARY_DIR}/np2kai_gcw0_sdl1_opk/default_opendingux_np21kai_desktop
			COMMAND sed -i -e 's/Name=NP2kai/Name=NP2kai(SDL1)/g' ${NP2kai_BINARY_DIR}/np2kai_gcw0_sdl1_opk/default_opendingux_np2kai_desktop
			COMMAND sed -i -e 's/Exec=np2kai/Exec=np2kai_gcw0_sdl1/g' ${NP2kai_BINARY_DIR}/np2kai_gcw0_sdl1_opk/default_opendingux_np2kai_desktop
			COMMAND mv ${NP2kai_BINARY_DIR}/np2kai_gcw0_sdl1_opk/default_opendingux_np2kai_desktop ${NP2kai_BINARY_DIR}/np2kai_gcw0_sdl1_opk/default_opendingux_np2kai_gcw0_sdl1.desktop
			COMMAND mksquashfs ${NP2kai_BINARY_DIR}/np2kai_gcw0_sdl1_opk ${NP2kai_BINARY_DIR}/np2kai_gcw0_sdl1.opk -all-root -noappend -no-exports
			COMMAND rm -r ${NP2kai_BINARY_DIR}/np2kai_gcw0_sdl1_opk
		)

		add_executable(np21kai_gcw0_sdl1)
		target_link_libraries(np21kai_gcw0_sdl1 PUBLIC NP21kai_base NP2kai_gcw0_SDL1_base)
		add_custom_command(
			TARGET np21kai_gcw0_sdl1
			POST_BUILD
			COMMENT "Genarate .opk pack."
			COMMAND cp -r ${NP2kai_SOURCE_DIR}/sdl/opendingux/opk ${NP2kai_BINARY_DIR}/np21kai_gcw0_sdl1_opk
			COMMAND mv np21kai_gcw0_sdl1.opk ${NP2kai_BINARY_DIR}/np21kai_gcw0_sdl1_opk/
			COMMAND rm ${NP2kai_BINARY_DIR}/np21kai_gcw0_sdl1_opk/default_opendingux_np2kai_desktop
			COMMAND sed -i -e 's/Name=NP21kai/Name=NP21kai(SDL1)/g' ${NP2kai_BINARY_DIR}/np21kai_gcw0_sdl1_opk/default_opendingux_np21kai_desktop
			COMMAND sed -i -e 's/Exec=np21kai/Exec=np21kai_gcw0_sdl1/g' ${NP2kai_BINARY_DIR}/np21kai_gcw0_sdl1_opk/default_opendingux_np21kai_desktop
			COMMAND mv ${NP2kai_BINARY_DIR}/np21kai_gcw0_sdl1_opk/default_opendingux_np21kai_desktop ${NP2kai_BINARY_DIR}/np21kai_gcw0_sdl1_opk/default_opendingux_np21kai_gcw0_sdl1.desktop
			COMMAND mksquashfs ${NP2kai_BINARY_DIR}/np21kai_gcw0_sdl1_opk ${NP2kai_BINARY_DIR}/np21kai_gcw0_sdl1.opk -all-root -noappend -no-exports
			COMMAND rm -r ${NP2kai_BINARY_DIR}/np21kai_gcw0_sdl1_opk
		)

		add_executable(np2kai_gcw0)
		target_link_libraries(np2kai_gcw0 PUBLIC NP2kai_base NP2kai_gcw0_SDL2_base)
		add_custom_command(
			TARGET np2kai_gcw0
			POST_BUILD
			COMMENT "Genarate .opk pack."
			COMMAND cp -r ${NP2kai_SOURCE_DIR}/sdl/opendingux/opk ${NP2kai_BINARY_DIR}/np2kai_gcw0_opk
			COMMAND mv np2kai_gcw0.opk ${NP2kai_BINARY_DIR}/np2kai_gcw0_opk/
			COMMAND rm ${NP2kai_BINARY_DIR}/np2kai_gcw0_opk/default_opendingux_np21kai_desktop
			COMMAND sed -i -e 's/Exec=np2kai/Exec=np2kai_gcw0/g' ${NP2kai_BINARY_DIR}/np2kai_gcw0_opk/default_opendingux_np2kai_desktop
			COMMAND mv ${NP2kai_BINARY_DIR}/np2kai_gcw0_opk/default_opendingux_np2kai_desktop ${NP2kai_BINARY_DIR}/np2kai_gcw0_opk/default_opendingux_np2kai_gcw0.desktop
			COMMAND mksquashfs ${NP2kai_BINARY_DIR}/np2kai_gcw0_opk ${NP2kai_BINARY_DIR}/np2kai_gcw0.opk -all-root -noappend -no-exports
			COMMAND rm -r ${NP2kai_BINARY_DIR}/np2kai_gcw0_opk
		)

		add_executable(np21kai_gcw0)
		target_link_libraries(np21kai_gcw0 PUBLIC NP21kai_base NP2kai_gcw0_SDL2_base)
		add_custom_command(
			TARGET np21kai_gcw0
			POST_BUILD
			COMMENT "Genarate .opk pack."
			COMMAND cp -r ${NP2kai_SOURCE_DIR}/sdl/opendingux/opk ${NP2kai_BINARY_DIR}/np21kai_gcw0_opk
			COMMAND mv np21kai_gcw0.opk ${NP2kai_BINARY_DIR}/np21kai_gcw0_opk/
			COMMAND rm ${NP2kai_BINARY_DIR}/np21kai_gcw0_opk/default_opendingux_np2kai_desktop
			COMMAND sed -i -e 's/Exec=np21kai/Exec=np21kai_gcw0/g' ${NP2kai_BINARY_DIR}/np21kai_gcw0_opk/default_opendingux_np21kai_desktop
			COMMAND mv ${NP2kai_BINARY_DIR}/np21kai_gcw0_opk/default_opendingux_np21kai_desktop ${NP2kai_BINARY_DIR}/np21kai_gcw0_opk/default_opendingux_np21kai_gcw0.desktop
			COMMAND mksquashfs ${NP2kai_BINARY_DIR}/np21kai_gcw0_opk ${NP2kai_BINARY_DIR}/np21kai_gcw0.opk -all-root -noappend -no-exports
			COMMAND rm -r ${NP2kai_BINARY_DIR}/np21kai_gcw0_opk
		)

		if(NOT BUILD_I286)
			set_target_properties(np2kai_gcw0_sdl1 np2kai_gcw0 PROPERTIES EXCLUDE_FROM_ALL true)
		endif()
		if(USE_SDL_VERSION EQUAL 1)
			set_target_properties(np2kai_gcw0 np21kai_gcw0 PROPERTIES EXCLUDE_FROM_ALL true)
		else()
			set_target_properties(np2kai_gcw0_sdl1 np21kai_gcw0_sdl1 PROPERTIES EXCLUDE_FROM_ALL true)
		endif()
	endif()

	if(BUILD_OPENDINGUX_RG350)
		set(CMAKE_EXECUTABLE_SUFFIX ".opk")

		add_executable(np2kai_rg350_sdl1)
		target_link_libraries(np2kai_rg350_sdl1 PUBLIC NP2kai_base NP2kai_rg350_SDL1_base)
		add_custom_command(
			TARGET np2kai_rg350_sdl1
			POST_BUILD
			COMMENT "Genarate .opk pack."
			COMMAND cp -r ${NP2kai_SOURCE_DIR}/sdl/opendingux/opk ${NP2kai_BINARY_DIR}/np2kai_rg350_sdl1_opk
			COMMAND mv np2kai_rg350_sdl1.opk ${NP2kai_BINARY_DIR}/np2kai_rg350_sdl1_opk/
			COMMAND rm ${NP2kai_BINARY_DIR}/np2kai_rg350_sdl1_opk/default_opendingux_np21kai_desktop
			COMMAND sed -i -e 's/Name=NP2kai/Name=NP2kai(SDL1)/g' ${NP2kai_BINARY_DIR}/np2kai_rg350_sdl1_opk/default_opendingux_np2kai_desktop
			COMMAND sed -i -e 's/Exec=np2kai/Exec=np2kai_rg350_sdl1/g' ${NP2kai_BINARY_DIR}/np2kai_rg350_sdl1_opk/default_opendingux_np2kai_desktop
			COMMAND mv ${NP2kai_BINARY_DIR}/np2kai_rg350_sdl1_opk/default_opendingux_np2kai_desktop ${NP2kai_BINARY_DIR}/np2kai_rg350_sdl1_opk/default_opendingux_np2kai_rg350_sdl1.desktop
			COMMAND mksquashfs ${NP2kai_BINARY_DIR}/np2kai_rg350_sdl1_opk ${NP2kai_BINARY_DIR}/np2kai_rg350_sdl1.opk -all-root -noappend -no-exports
			COMMAND rm -r ${NP2kai_BINARY_DIR}/np2kai_rg350_sdl1_opk
		)

		add_executable(np21kai_rg350_sdl1)
		target_link_libraries(np21kai_rg350_sdl1 PUBLIC NP21kai_base NP2kai_rg350_SDL1_base)
		add_custom_command(
			TARGET np21kai_rg350_sdl1
			POST_BUILD
			COMMENT "Genarate .opk pack."
			COMMAND cp -r ${NP2kai_SOURCE_DIR}/sdl/opendingux/opk ${NP2kai_BINARY_DIR}/np21kai_rg350_sdl1_opk
			COMMAND mv np21kai_rg350_sdl1.opk ${NP2kai_BINARY_DIR}/np21kai_rg350_sdl1_opk/
			COMMAND rm ${NP2kai_BINARY_DIR}/np21kai_rg350_sdl1_opk/default_opendingux_np2kai_desktop
			COMMAND sed -i -e 's/Name=NP21kai/Name=NP21kai(SDL1)/g' ${NP2kai_BINARY_DIR}/np21kai_rg350_sdl1_opk/default_opendingux_np21kai_desktop
			COMMAND sed -i -e 's/Exec=np21kai/Exec=np21kai_rg350_sdl1/g' ${NP2kai_BINARY_DIR}/np21kai_rg350_sdl1_opk/default_opendingux_np21kai_desktop
			COMMAND mv ${NP2kai_BINARY_DIR}/np21kai_rg350_sdl1_opk/default_opendingux_np21kai_desktop ${NP2kai_BINARY_DIR}/np21kai_rg350_sdl1_opk/default_opendingux_np21kai_rg350_sdl1.desktop
			COMMAND mksquashfs ${NP2kai_BINARY_DIR}/np21kai_rg350_sdl1_opk ${NP2kai_BINARY_DIR}/np21kai_rg350_sdl1.opk -all-root -noappend -no-exports
			COMMAND rm -r ${NP2kai_BINARY_DIR}/np21kai_rg350_sdl1_opk
		)

		add_executable(np2kai_rg350)
		target_link_libraries(np2kai_rg350 PUBLIC NP2kai_base NP2kai_rg350_SDL2_base)
		add_custom_command(
			TARGET np2kai_rg350
			POST_BUILD
			COMMENT "Genarate .opk pack."
			COMMAND cp -r ${NP2kai_SOURCE_DIR}/sdl/opendingux/opk ${NP2kai_BINARY_DIR}/np2kai_rg350_opk
			COMMAND mv np2kai_rg350.opk ${NP2kai_BINARY_DIR}/np2kai_rg350_opk/
			COMMAND rm ${NP2kai_BINARY_DIR}/np2kai_rg350_opk/default_opendingux_np21kai_desktop
			COMMAND sed -i -e 's/Exec=np2kai/Exec=np2kai_rg350/g' ${NP2kai_BINARY_DIR}/np2kai_rg350_opk/default_opendingux_np2kai_desktop
			COMMAND mv ${NP2kai_BINARY_DIR}/np2kai_rg350_opk/default_opendingux_np2kai_desktop ${NP2kai_BINARY_DIR}/np2kai_rg350_opk/default_opendingux_np2kai_rg350.desktop
			COMMAND mksquashfs ${NP2kai_BINARY_DIR}/np2kai_rg350_opk ${NP2kai_BINARY_DIR}/np2kai_rg350.opk -all-root -noappend -no-exports
			COMMAND rm -r ${NP2kai_BINARY_DIR}/np2kai_rg350_opk
		)

		add_executable(np21kai_rg350)
		target_link_libraries(np21kai_rg350 PUBLIC NP21kai_base NP2kai_rg350_SDL2_base)
		add_custom_command(
			TARGET np21kai_rg350
			POST_BUILD
			COMMENT "Genarate .opk pack."
			COMMAND cp -r ${NP2kai_SOURCE_DIR}/sdl/opendingux/opk ${NP2kai_BINARY_DIR}/np21kai_rg350_opk
			COMMAND mv np21kai_rg350.opk ${NP2kai_BINARY_DIR}/np21kai_rg350_opk/
			COMMAND rm ${NP2kai_BINARY_DIR}/np21kai_rg350_opk/default_opendingux_np2kai_desktop
			COMMAND sed -i -e 's/Exec=np21kai/Exec=np21kai_rg350/g' ${NP2kai_BINARY_DIR}/np21kai_rg350_opk/default_opendingux_np21kai_desktop
			COMMAND mv ${NP2kai_BINARY_DIR}/np21kai_rg350_opk/default_opendingux_np21kai_desktop ${NP2kai_BINARY_DIR}/np21kai_rg350_opk/default_opendingux_np21kai_rg350.desktop
			COMMAND mksquashfs ${NP2kai_BINARY_DIR}/np21kai_rg350_opk ${NP2kai_BINARY_DIR}/np21kai_rg350.opk -all-root -noappend -no-exports
			COMMAND rm -r ${NP2kai_BINARY_DIR}/np21kai_rg350_opk
		)

		if(NOT BUILD_I286)
			set_target_properties(np2kai_rg350_sdl1 np2kai_rg350 PROPERTIES EXCLUDE_FROM_ALL true)
		endif()
		if(USE_SDL_VERSION EQUAL 1)
			set_target_properties(np2kai_rg350 np21kai_rg350 PROPERTIES EXCLUDE_FROM_ALL true)
		else()
			set_target_properties(np2kai_rg350_sdl1 np21kai_rg350_sdl1 PROPERTIES EXCLUDE_FROM_ALL true)
		endif()
	endif()

	if(BUILD_OPENDINGUX_RS90)
		set(CMAKE_EXECUTABLE_SUFFIX ".opk")

		add_executable(np2kai_rs90_sdl1)
		target_link_libraries(np2kai_rs90_sdl1 PUBLIC NP2kai_base NP2kai_rs90_SDL1_base)
		add_custom_command(
			TARGET np2kai_rs90_sdl1
			POST_BUILD
			COMMENT "Genarate .opk pack."
			COMMAND cp -r ${NP2kai_SOURCE_DIR}/sdl/opendingux/opk ${NP2kai_BINARY_DIR}/np2kai_rs90_sdl1_opk
			COMMAND mv np2kai_rs90_sdl1.opk ${NP2kai_BINARY_DIR}/np2kai_rs90_sdl1_opk/
			COMMAND rm ${NP2kai_BINARY_DIR}/np2kai_rs90_sdl1_opk/default_opendingux_np21kai_desktop
			COMMAND sed -i -e 's/Name=NP2kai/Name=NP2kai(SDL1)/g' ${NP2kai_BINARY_DIR}/np2kai_rs90_sdl1_opk/default_opendingux_np2kai_desktop
			COMMAND sed -i -e 's/Exec=np2kai/Exec=np2kai_rs90_sdl1/g' ${NP2kai_BINARY_DIR}/np2kai_rs90_sdl1_opk/default_opendingux_np2kai_desktop
			COMMAND mv ${NP2kai_BINARY_DIR}/np2kai_rs90_sdl1_opk/default_opendingux_np2kai_desktop ${NP2kai_BINARY_DIR}/np2kai_rs90_sdl1_opk/default_opendingux_np2kai_rs90_sdl1.desktop
			COMMAND mksquashfs ${NP2kai_BINARY_DIR}/np2kai_rs90_sdl1_opk ${NP2kai_BINARY_DIR}/np2kai_rs90_sdl1.opk -all-root -noappend -no-exports
			COMMAND rm -r ${NP2kai_BINARY_DIR}/np2kai_rs90_sdl1_opk
		)

		add_executable(np21kai_rs90_sdl1)
		target_link_libraries(np21kai_rs90_sdl1 PUBLIC NP21kai_base NP2kai_rs90_SDL1_base)
		add_custom_command(
			TARGET np21kai_rs90_sdl1
			POST_BUILD
			COMMENT "Genarate .opk pack."
			COMMAND cp -r ${NP2kai_SOURCE_DIR}/sdl/opendingux/opk ${NP2kai_BINARY_DIR}/np21kai_rs90_sdl1_opk
			COMMAND mv np21kai_rs90_sdl1.opk ${NP2kai_BINARY_DIR}/np21kai_rs90_sdl1_opk/
			COMMAND rm ${NP2kai_BINARY_DIR}/np21kai_rs90_sdl1_opk/default_opendingux_np2kai_desktop
			COMMAND sed -i -e 's/Name=NP21kai/Name=NP21kai(SDL1)/g' ${NP2kai_BINARY_DIR}/np21kai_rs90_sdl1_opk/default_opendingux_np21kai_desktop
			COMMAND sed -i -e 's/Exec=np21kai/Exec=np21kai_rs90_sdl1/g' ${NP2kai_BINARY_DIR}/np21kai_rs90_sdl1_opk/default_opendingux_np21kai_desktop
			COMMAND mv ${NP2kai_BINARY_DIR}/np21kai_rs90_sdl1_opk/default_opendingux_np21kai_desktop ${NP2kai_BINARY_DIR}/np21kai_rs90_sdl1_opk/default_opendingux_np21kai_rs90_sdl1.desktop
			COMMAND mksquashfs ${NP2kai_BINARY_DIR}/np21kai_rs90_sdl1_opk ${NP2kai_BINARY_DIR}/np21kai_rs90_sdl1.opk -all-root -noappend -no-exports
			COMMAND rm -r ${NP2kai_BINARY_DIR}/np21kai_rs90_sdl1_opk
		)

		add_executable(np2kai_rs90)
		target_link_libraries(np2kai_rs90 PUBLIC NP2kai_base NP2kai_rs90_base)
		add_custom_command(
			TARGET np2kai_rs90
			POST_BUILD
			COMMENT "Genarate .opk pack."
			COMMAND cp -r ${NP2kai_SOURCE_DIR}/sdl/opendingux/opk ${NP2kai_BINARY_DIR}/np2kai_rs90_opk
			COMMAND mv np2kai_rs90.opk ${NP2kai_BINARY_DIR}/np2kai_rs90_opk/
			COMMAND rm ${NP2kai_BINARY_DIR}/np2kai_rs90_opk/default_opendingux_np21kai_desktop
			COMMAND sed -i -e 's/Exec=np2kai/Exec=np2kai_rs90/g' ${NP2kai_BINARY_DIR}/np2kai_rs90_opk/default_opendingux_np2kai_desktop
			COMMAND mv ${NP2kai_BINARY_DIR}/np2kai_rs90_opk/default_opendingux_np2kai_desktop ${NP2kai_BINARY_DIR}/np2kai_rs90_opk/default_opendingux_np2kai_rs90.desktop
			COMMAND mksquashfs ${NP2kai_BINARY_DIR}/np2kai_rs90_opk ${NP2kai_BINARY_DIR}/np2kai_rs90.opk -all-root -noappend -no-exports
			COMMAND rm -r ${NP2kai_BINARY_DIR}/np2kai_rs90_opk
		)

		add_executable(np21kai_rs90_sdl1)
		target_link_libraries(np21kai_rs90 PUBLIC NP21kai_base NP2kai_rs90_base)
		add_custom_command(
			TARGET np21kai_rs90
			POST_BUILD
			COMMENT "Genarate .opk pack."
			COMMAND cp -r ${NP2kai_SOURCE_DIR}/sdl/opendingux/opk ${NP2kai_BINARY_DIR}/np21kai_rs90_opk
			COMMAND mv np21kai_rs90.opk ${NP2kai_BINARY_DIR}/np21kai_rs90_opk/
			COMMAND rm ${NP2kai_BINARY_DIR}/np21kai_rs90_opk/default_opendingux_np2kai_desktop
			COMMAND sed -i -e 's/Exec=np21kai/Exec=np21kai_rs90/g' ${NP2kai_BINARY_DIR}/np21kai_rs90_opk/default_opendingux_np21kai_desktop
			COMMAND mv ${NP2kai_BINARY_DIR}/np21kai_rs90_opk/default_opendingux_np21kai_desktop ${NP2kai_BINARY_DIR}/np21kai_rs90_opk/default_opendingux_np21kai_rs90.desktop
			COMMAND mksquashfs ${NP2kai_BINARY_DIR}/np21kai_rs90_opk ${NP2kai_BINARY_DIR}/np21kai_rs90.opk -all-root -noappend -no-exports
			COMMAND rm -r ${NP2kai_BINARY_DIR}/np21kai_rs90_opk
		)

		if(NOT BUILD_I286)
			set_target_properties(np2kai_rs90_sdl1 np2kai_rs90 PROPERTIES EXCLUDE_FROM_ALL true)
		endif()
		if(USE_SDL_VERSION EQUAL 1)
			set_target_properties(np2kai_rs90 np21kai_rs90 PROPERTIES EXCLUDE_FROM_ALL true)
		else()
			set_target_properties(np2kai_rs90_sdl1 np21kai_rs90_sdl1 PROPERTIES EXCLUDE_FROM_ALL true)
		endif()
	endif()

else()

	add_executable(sdlnp2kai_sdl1 WIN32 MACOSX_BUNDLE)
	target_link_libraries(sdlnp2kai_sdl1 NP2kai_base NP2kai_SDL_SDL1_base)
	if(WIN32)
		target_include_directories(sdlnp2kai_sdl1 PUBLIC ${NP2kai_SDL_Windows_include_dirs})
		target_sources(sdlnp2kai_sdl1 PUBLIC ${NP2kai_SDL_Windows_sources})
	else()
		target_include_directories(sdlnp2kai_sdl1 PUBLIC ${NP2kai_SDL_UNIX_include_dirs})
		target_sources(sdlnp2kai_sdl1 PUBLIC ${NP2kai_SDL_UNIX_sources})
	endif()
	install(TARGETS sdlnp2kai_sdl1)

	add_executable(sdlnp21kai_sdl1 WIN32 MACOSX_BUNDLE)
	target_link_libraries(sdlnp21kai_sdl1 NP21kai_base NP2kai_SDL_SDL1_base)
	if(WIN32)
		target_include_directories(sdlnp21kai_sdl1 PUBLIC ${NP2kai_SDL_Windows_include_dirs})
		target_sources(sdlnp21kai_sdl1 PUBLIC ${NP2kai_SDL_Windows_sources})
	else()
		target_include_directories(sdlnp21kai_sdl1 PUBLIC ${NP2kai_SDL_UNIX_include_dirs})
		target_sources(sdlnp21kai_sdl1 PUBLIC ${NP2kai_SDL_UNIX_sources})
	endif()
	install(TARGETS sdlnp21kai_sdl1)

	add_executable(sdlnp21kai_HAXM_sdl1 WIN32 MACOSX_BUNDLE)
	target_link_libraries(sdlnp21kai_HAXM_sdl1 NP21kai_HAXM_base NP2kai_SDL_SDL1_base)
	if(WIN32)
		target_include_directories(sdlnp21kai_HAXM_sdl1 PUBLIC ${NP2kai_SDL_Windows_include_dirs})
		target_sources(sdlnp21kai_HAXM_sdl1 PUBLIC ${NP2kai_SDL_Windows_sources})
	else()
		target_include_directories(sdlnp21kai_HAXM_sdl1 PUBLIC ${NP2kai_SDL_UNIX_include_dirs})
		target_sources(sdlnp21kai_HAXM_sdl1 PUBLIC ${NP2kai_SDL_UNIX_sources})
	endif()
	install(TARGETS sdlnp21kai_HAXM_sdl1)

	add_executable(sdlnp2kai WIN32 MACOSX_BUNDLE)
	target_link_libraries(sdlnp2kai NP2kai_base NP2kai_SDL_SDL2_base)
	if(WIN32)
		target_include_directories(sdlnp2kai PUBLIC ${NP2kai_SDL_Windows_include_dirs})
		target_sources(sdlnp2kai PUBLIC ${NP2kai_SDL_Windows_sources})
	else()
		target_include_directories(sdlnp2kai PUBLIC ${NP2kai_SDL_UNIX_include_dirs})
		target_sources(sdlnp2kai PUBLIC ${NP2kai_SDL_UNIX_sources})
	endif()
	install(TARGETS sdlnp2kai)

	add_executable(sdlnp21kai WIN32 MACOSX_BUNDLE)
	target_link_libraries(sdlnp21kai NP21kai_base NP2kai_SDL_SDL2_base)
	if(WIN32)
		target_include_directories(sdlnp21kai PUBLIC ${NP2kai_SDL_Windows_include_dirs})
		target_sources(sdlnp21kai PUBLIC ${NP2kai_SDL_Windows_sources})
	else()
		target_include_directories(sdlnp21kai PUBLIC ${NP2kai_SDL_UNIX_include_dirs})
		target_sources(sdlnp21kai PUBLIC ${NP2kai_SDL_UNIX_sources})
	endif()
	install(TARGETS sdlnp21kai)

	add_executable(sdlnp21kai_HAXM WIN32 MACOSX_BUNDLE)
	target_link_libraries(sdlnp21kai_HAXM NP21kai_HAXM_base NP2kai_SDL_SDL2_base)
	if(WIN32)
		target_include_directories(sdlnp21kai_HAXM PUBLIC ${NP2kai_SDL_Windows_include_dirs})
		target_sources(sdlnp21kai_HAXM PUBLIC ${NP2kai_SDL_Windows_sources})
	else()
		target_include_directories(sdlnp21kai_HAXM PUBLIC ${NP2kai_SDL_UNIX_include_dirs})
		target_sources(sdlnp21kai_HAXM PUBLIC ${NP2kai_SDL_UNIX_sources})
	endif()
	install(TARGETS sdlnp21kai_HAXM)

	set(X_MISC_INSTALL ON)

	add_executable(xnp2kai_sdl1)
	target_link_libraries(xnp2kai_sdl1 NP2kai_base NP2kai_X_SDL1_base)
	add_custom_command(
		TARGET xnp2kai_sdl1
		POST_BUILD
		COMMENT "Genarate xnp2kai man page."
		COMMAND cp ${NP2kai_SOURCE_DIR}/x/xnp2kai.man1 ${NP2kai_BINARY_DIR}/xnp2kai.1
		COMMAND cp ${NP2kai_SOURCE_DIR}/x/xnp2kai.jman1 ${NP2kai_BINARY_DIR}/xnp2kai.1j
	)
	install(TARGETS xnp2kai_sdl1)
	install(FILES ${NP2kai_BINARY_DIR}/xnp2kai.1 ${NP2kai_BINARY_DIR}/xnp2kai.1j TYPE MAN)
	install(FILES ${NP2kai_SOURCE_DIR}/x/resources/xnp2kai.desktop DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications)
	if(X_MISC_INSTALL)
		install(FILES ${NP2kai_SOURCE_DIR}/misc/np2.svg DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/scalable/apps/np2kai.svg)
		set(X_MISC_INSTALL OFF)
	endif()

	add_executable(xnp21kai_sdl1)
	target_link_libraries(xnp21kai_sdl1 NP21kai_base NP2kai_X_SDL1_base)
	add_custom_command(
		TARGET xnp21kai_sdl1
		POST_BUILD
		COMMENT "Genarate xnp21kai man page."
		COMMAND cp ${NP2kai_SOURCE_DIR}/x/xnp21kai.man1 ${NP2kai_BINARY_DIR}/xnp21kai.1
		COMMAND cp ${NP2kai_SOURCE_DIR}/x/xnp21kai.jman1 ${NP2kai_BINARY_DIR}/xnp21kai.1j
	)
	install(TARGETS xnp21kai_sdl1)
	install(FILES ${NP2kai_BINARY_DIR}/xnp21kai.1 ${NP2kai_BINARY_DIR}/xnp21kai.1j TYPE MAN)
	install(FILES ${NP2kai_SOURCE_DIR}/x/resources/xnp21kai.desktop DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications)
	if(X_MISC_INSTALL)
		install(FILES ${NP2kai_SOURCE_DIR}/misc/np2.svg DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/scalable/apps/np2kai.svg)
		set(X_MISC_INSTALL OFF)
	endif()

	add_executable(xnp21kai_HAXM_sdl1)
	target_link_libraries(xnp21kai_HAXM_sdl1 NP21kai_HAXM_base NP2kai_X_SDL1_base)
	install(TARGETS xnp21kai_HAXM_sdl1)
	install(FILES ${NP2kai_SOURCE_DIR}/x/resources/xnp21kai.desktop DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications)
	if(X_MISC_INSTALL)
		install(FILES ${NP2kai_SOURCE_DIR}/misc/np2.svg DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/scalable/apps/np2kai.svg)
		set(X_MISC_INSTALL OFF)
	endif()

	add_executable(xnp2kai)
	target_link_libraries(xnp2kai NP2kai_base NP2kai_X_SDL2_base)
	add_custom_command(
		TARGET xnp2kai
		POST_BUILD
		COMMENT "Genarate xnp2kai man page."
		COMMAND cp ${NP2kai_SOURCE_DIR}/x/xnp2kai.man1 ${NP2kai_BINARY_DIR}/xnp2kai.1
		COMMAND cp ${NP2kai_SOURCE_DIR}/x/xnp2kai.jman1 ${NP2kai_BINARY_DIR}/xnp2kai.1j
	)
	install(TARGETS xnp2kai)
	install(FILES ${NP2kai_BINARY_DIR}/xnp2kai.1 ${NP2kai_BINARY_DIR}/xnp2kai.1j TYPE MAN)
	install(FILES ${NP2kai_SOURCE_DIR}/x/resources/xnp2kai.desktop DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications)
	if(X_MISC_INSTALL)
		install(FILES ${NP2kai_SOURCE_DIR}/misc/np2.svg DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/scalable/apps/np2kai.svg)
		set(X_MISC_INSTALL OFF)
	endif()

	add_executable(xnp21kai)
	target_link_libraries(xnp21kai NP21kai_base NP2kai_X_SDL2_base)
	add_custom_command(
		TARGET xnp21kai
		POST_BUILD
		COMMENT "Genarate xnp21kai man page."
		COMMAND cp ${NP2kai_SOURCE_DIR}/x/xnp21kai.man1 ${NP2kai_BINARY_DIR}/xnp21kai.1
		COMMAND cp ${NP2kai_SOURCE_DIR}/x/xnp21kai.jman1 ${NP2kai_BINARY_DIR}/xnp21kai.1j
	)
	install(TARGETS xnp21kai)
	install(FILES ${NP2kai_BINARY_DIR}/xnp21kai.1 ${NP2kai_BINARY_DIR}/xnp21kai.1j TYPE MAN)
	install(FILES ${NP2kai_SOURCE_DIR}/x/resources/xnp21kai.desktop DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications)
	if(X_MISC_INSTALL)
		install(FILES ${NP2kai_SOURCE_DIR}/misc/np2.svg DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/scalable/apps/np2kai.svg)
		set(X_MISC_INSTALL OFF)
	endif()

	add_executable(xnp21kai_HAXM)
	target_link_libraries(xnp21kai_HAXM NP21kai_HAXM_base NP2kai_X_SDL2_base)
	install(TARGETS xnp21kai_HAXM)
	install(FILES ${NP2kai_SOURCE_DIR}/x/resources/xnp21kai.desktop DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications)
	if(X_MISC_INSTALL)
		install(FILES ${NP2kai_SOURCE_DIR}/misc/np2.svg DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/scalable/apps/np2kai.svg)
		set(X_MISC_INSTALL OFF)
	endif()

	if(NOT BUILD_I286)
		set_target_properties(sdlnp2kai_sdl1 sdlnp2kai xnp2kai_sdl1 xnp2kai PROPERTIES EXCLUDE_FROM_ALL true)
	endif()
	if(USE_SDL_VERSION EQUAL 1)
		set_target_properties(sdlnp2kai sdlnp21kai sdlnp21kai_HAXM xnp2kai xnp21kai xnp21kai_HAXM PROPERTIES EXCLUDE_FROM_ALL true)
	else()
		set_target_properties(sdlnp2kai_sdl1 sdlnp21kai_sdl1 sdlnp21kai_HAXM_sdl1 xnp2kai_sdl1 xnp21kai_sdl1 xnp21kai_HAXM_sdl1 PROPERTIES EXCLUDE_FROM_ALL true)
	endif()
	if(NOT BUILD_SDL)
		set_target_properties(sdlnp2kai_sdl1 sdlnp21kai_sdl1 sdlnp21kai_HAXM_sdl1 sdlnp2kai sdlnp21kai sdlnp21kai_HAXM PROPERTIES EXCLUDE_FROM_ALL true)
	endif()
	if(NOT BUILD_X)
		set_target_properties(xnp2kai_sdl1 xnp21kai_sdl1 xnp21kai_HAXM_sdl1 xnp2kai xnp21kai xnp21kai_HAXM PROPERTIES EXCLUDE_FROM_ALL true)
	endif()
	if(NOT BUILD_HAXM)
		set_target_properties(sdlnp21kai_HAXM_sdl1 sdlnp21kai_HAXM xnp21kai_HAXM_sdl1 xnp21kai_HAXM PROPERTIES EXCLUDE_FROM_ALL true)
	endif()

	if((UNIX OR MINGW OR MSYS OR CYGWIN OR CMAKE_HOST_APPLE) AND NOT BUILD_SDL AND NOT BUILD_X)
		message(FATAL_ERROR "Nothing build target.")
	endif()

	if(MSVC_VERSION)
		add_compile_options(/utf-8 /GS)

		add_executable(NP2kai_windows WIN32)
		target_link_libraries(NP2kai_windows NP2kai_base NP2kai_Windows_base)

		add_executable(NP21kai_windows WIN32)
		target_link_libraries(NP21kai_windows NP21kai_base NP2kai_Windows_base)

		add_executable(NP21kai_HAXM_windows WIN32)
		target_link_libraries(NP21kai_HAXM_windows NP21kai_HAXM_base NP2kai_Windows_base)

		add_library(NP2kai_windows_932 SHARED "windows/resources/932/np2jp.rc")
		add_library(NP2kai_windows_936 SHARED "windows/resources/936/np2.rc")
		add_library(NP2kai_windows_950 SHARED "windows/resources/950/np2.rc")
		add_library(NP2kai_windows_1252 SHARED "windows/resources/1252/np2.rc")

		if(NOT BUILD_I286)
			set_target_properties(NP2kai_windows PROPERTIES EXCLUDE_FROM_ALL true)
		endif()
		if(NOT BUILD_HAXM)
			set_target_properties(NP21kai_HAXM_windows PROPERTIES EXCLUDE_FROM_ALL true)
		endif()
	endif()

endif()

