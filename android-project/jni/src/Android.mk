LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm

LOCAL_MODULE := main

SDL_PATH := ../SDL
NP2_PATH := $(LOCAL_PATH)/../np2

LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(SDL_PATH)/include \
	$(LOCAL_PATH) \
	$(NP2_PATH) \
	$(NP2_PATH)/cbus \
	$(NP2_PATH)/common \
	$(NP2_PATH)/embed \
	$(NP2_PATH)/embed/menu \
	$(NP2_PATH)/embed/menubase \
	$(NP2_PATH)/generic \
	$(NP2_PATH)/i286c \
	$(NP2_PATH)/io \
	$(NP2_PATH)/mem \
	$(NP2_PATH)/sdl2 \
	$(NP2_PATH)/sound \
	$(NP2_PATH)/vram

# Add your application source files here...
LOCAL_SRC_FILES := $(SDL_PATH)/src/main/android/SDL_android_main.c \
	main.c \
	$(subst $(LOCAL_PATH)/,, \
		$(NP2_PATH)/calendar.c \
		$(NP2_PATH)/debugsub.c \
		$(NP2_PATH)/keystat.c \
		$(NP2_PATH)/nevent.c \
		$(NP2_PATH)/pccore.c \
		$(NP2_PATH)/statsave.c \
		$(NP2_PATH)/timing.c \
		$(wildcard $(NP2_PATH)/bios/*.c) \
		$(wildcard $(NP2_PATH)/cbus/*.c) \
		$(wildcard $(NP2_PATH)/codecnv/*.c) \
		$(wildcard $(NP2_PATH)/common/*.c) \
		$(wildcard $(NP2_PATH)/embed/*.c) \
		$(wildcard $(NP2_PATH)/embed/menu/*.c) \
		$(wildcard $(NP2_PATH)/embed/menubase/*.c) \
		$(wildcard $(NP2_PATH)/fdd/*.c) \
		$(wildcard $(NP2_PATH)/font/*.c) \
		$(wildcard $(NP2_PATH)/generic/*.c) \
		$(wildcard $(NP2_PATH)/i286c/*.c) \
		$(wildcard $(NP2_PATH)/io/*.c) \
		$(wildcard $(NP2_PATH)/lio/*.c) \
		$(wildcard $(NP2_PATH)/mem/*.c) \
		$(wildcard $(NP2_PATH)/sdl2/*.c) \
		$(wildcard $(NP2_PATH)/sdl2/*.cpp) \
		$(wildcard $(NP2_PATH)/sound/*.c) \
		$(wildcard $(NP2_PATH)/sound/getsnd/*.c) \
		$(wildcard $(NP2_PATH)/sound/vermouth/*.c) \
		$(wildcard $(NP2_PATH)/vram/*.c) \
	)

LOCAL_SHARED_LIBRARIES := SDL2

LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -llog

include $(BUILD_SHARED_LIBRARY)
