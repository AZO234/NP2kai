# On Windows, please run (emsdk dir)/emsdk_env.bat before make
# On Linux, please write 'source (emsdk dir)/emsdk_env.sh' in .profile or .pashrc

CC=emcc
CXX=em++
AR=emar

DEBUG ?= 0
SUPPORT_NP2_THREAD ?= 0
SUPPORT_NP2_TICKCOUNT ?= 0
SUPPORT_NET ?= 0
SUPPORT_ASYNC_CPU ?= 0
SDL_VERSION ?= 2
GIT_VERSION := "$(shell git rev-parse --short HEAD)"

ifeq ($(SDL_VERSION), 1)
SUPPORT_ASYNC_CPU ?=
else
SUPPORT_ASYNC_CPU ?= -DSUPPORT_ASYNC_CPU
endif
SDL_CFLAGS := -s USE_SDL=$(SDL_VERSION)
SDL_LIBS := -s USE_SDL=$(SDL_VERSION)

#WebAssembly seemed to be a bit faster than asm.js
WASM=1

#Set maximum RAM that Emscripten use (in bytes).
EMSCRIPTEN_TOTAL_MEMORY=67108864

TARGET_NAME := np21kai.bc

TARGET := $(TARGET_NAME)
CFLAGS += -DGIT_VERSION=\"$(GIT_VERSION)\"
CXXFLAGS += -DGIT_VERSION=\"$(GIT_VERSION)\"
fpic = -fPIC

ifeq ($(DEBUG), 1)
COMMONFLAGS += -O0 -g -DDEBUG -D_DEBUG -DTRACE
else
COMMONFLAGS += -O2 -DNDEBUG -D_NDEBUG
endif

CORE_DIR := ..
INCFLAGS :=
SOURCES_C :=
SOURCES_CXX :=

include Makefile.common

INCFLAGS := $(SDL_CFLAGS) $(INCFLAGS)

INCFLAGS += 	-I$(NP2_PATH)/i386c \
		-I$(NP2_PATH)/i386c/ia32 \
		-I$(NP2_PATH)/i386c/ia32/instructions \
		-I$(NP2_PATH)/i386c/ia32/instructions/fpu \
		-I$(NP2_PATH)/i386c/ia32/instructions/fpu/softfloat \
		-I$(NP2_PATH)/i386c/ia32/instructions/mmx \
		-I$(NP2_PATH)/i386c/ia32/instructions/sse \
		-I$(NP2_PATH)/sdl2/em
SOURCES_C += 	$(wildcard $(NP2_PATH)/i386c/*.c) \
		$(wildcard $(NP2_PATH)/i386c/ia32/*.c) \
		$(wildcard $(NP2_PATH)/i386c/ia32/instructions/*.c) \
		$(NP2_PATH)/i386c/ia32/instructions/fpu/fpdummy.c \
		$(NP2_PATH)/i386c/ia32/instructions/fpu/fpemul_dosbox.c \
		$(NP2_PATH)/i386c/ia32/instructions/fpu/fpemul_dosbox2.c \
		$(NP2_PATH)/i386c/ia32/instructions/fpu/fpemul_softfloat.c \
		$(wildcard $(NP2_PATH)/i386c/ia32/instructions/fpu/softfloat/*.c) \
		$(wildcard $(NP2_PATH)/i386c/ia32/instructions/mmx/*.c) \
		$(wildcard $(NP2_PATH)/i386c/ia32/instructions/sse/*.c) \
		$(wildcard $(NP2_PATH)/i386c/ia32/instructions/sse2/*.c) \
		$(wildcard $(NP2_PATH)/i386c/ia32/instructions/sse3/*.c) \
		$(NP2_PATH)/sdl2/em/main.c

ifeq ($(SDL_VERSION), 1)
	CFLAGS	+= $(NP2_PATH)/sdl2/em/SDL_mixer.c
endif

NP2SDLDEFINE := -DNP2_SDL2 -DUSE_SDLAUDIO

OBJECTS  = $(SOURCES_CXX:.cpp=.o) $(SOURCES_C:.c=.o)
CXXFLAGS += $(fpic) $(INCFLAGS) $(COMMONFLAGS) $(NP2DEFINE) $(NP21DEFINE) $(NP2SDLDEFINE)
CFLAGS   += $(fpic) $(INCFLAGS) $(COMMONFLAGS) $(NP2DEFINE) $(NP21DEFINE) $(NP2SDLDEFINE)
LDFLAGS  += $(fpic) -lm $(SDL_LIBS) -static

ifeq ($(EMULARITY), 1)
	CFLAGS	+= -DUSE_EMULARITY_NP2DIR
endif

all: $(TARGET)
$(TARGET): $(OBJECTS)
ifeq ($(STATIC_LINKING), 1)
	$(AR) rcs $@ $(OBJECTS)
else
	$(CXX) -o $@ $(OBJECTS) $(LDFLAGS)
endif

html: $(TARGET)
ifeq ($(PRELOAD), 1)
	$(CC) -O3 -s USE_SDL=$(SDL_VERSION) -s WASM=$(WASM) -s TOTAL_MEMORY=$(EMSCRIPTEN_TOTAL_MEMORY)  -s EMTERPRETIFY=1 -s EMTERPRETIFY_ASYNC=1 \
	-s EMTERPRETIFY_WHITELIST="['_np2_main','_np2exec','_taskmng_sleep']" -s BINARYEN_TRAP_MODE="js" --preload-file $(PREFILE) \
	$(TARGET) -o $(basename $(TARGET)).html
#	$(CC) -O3 -s USE_SDL=$(SDL_VERSION) -s WASM=$(WASM) -s TOTAL_MEMORY=$(EMSCRIPTEN_TOTAL_MEMORY)  -s EMTERPRETIFY=1 -s EMTERPRETIFY_ASYNC=1 \
#	-s EMTERPRETIFY_WHITELIST="['_np2_main','_np2exec','_taskmng_sleep']" --preload-file $(PREFILE) \
#	$(TARGET) -o $(basename $(TARGET)).html
else
	$(CC) -O3 -s USE_SDL=$(SDL_VERSION) -s WASM=$(WASM) -s TOTAL_MEMORY=$(EMSCRIPTEN_TOTAL_MEMORY)  -s EMTERPRETIFY=1 -s EMTERPRETIFY_ASYNC=1 \
	-s EMTERPRETIFY_WHITELIST="['_np2_main','_np2exec','_taskmng_sleep']" -s BINARYEN_TRAP_MODE="js" \
	$(TARGET) -o $(basename $(TARGET)).html 
#	$(CC) -O3 -s USE_SDL=$(SDL_VERSION) -s WASM=$(WASM) -s TOTAL_MEMORY=$(EMSCRIPTEN_TOTAL_MEMORY)  -s EMTERPRETIFY=1 -s EMTERPRETIFY_ASYNC=1 \
#	-s EMTERPRETIFY_WHITELIST="['_np2_main','_np2exec','_taskmng_sleep']" \
#	$(TARGET) -o $(basename $(TARGET)).html 
endif

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $^ -o $@


clean:
	rm -f $(OBJECTS) $(TARGET)
	rm -f $(basename $(TARGET)).html 
	rm -f $(basename $(TARGET)).js.orig.js
	rm -f $(basename $(TARGET)).js
	rm -f $(basename $(TARGET)).html.mem
	rm -f $(basename $(TARGET)).wasm 	
	rm -f $(basename $(TARGET)).data

cppcheck: compile_commands.json
	cppcheck --project=compile_commands.json --xml 2> cppcheck.xml

compile_commands.json:
	bear make -f Makefile21.em


install:
	strip $(TARGET)
	cp $(TARGET) /usr/local/bin/


uninstall:
	rm /usr/local/bin/$(TARGET)

