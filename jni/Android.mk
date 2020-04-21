DEBUG ?= 0

SUPPORT_NP2_THREAD ?= 1
SUPPORT_NP2_TICKCOUNT ?= 1
SUPPORT_NET ?= 0
SUPPORT_ASYNC_CPU ?= 0
SUPPORT_DIRENT = 1

LOCAL_PATH := $(call my-dir)

CORE_DIR := $(LOCAL_PATH)/..

include $(CORE_DIR)/sdl2/Makefile.common

INCFLAGS += 	-I$(NP2_PATH)/sdl2/libretro \
		-I$(NP2_PATH)/sdl2/libretro/libretro-common/include \
		-I$(NP2_PATH)/sdl2/libretro/libretro-common/include/array \
		-I$(NP2_PATH)/sdl2/libretro/libretro-common/include/audio \
		-I$(NP2_PATH)/sdl2/libretro/libretro-common/include/audio/conversion \
		-I$(NP2_PATH)/sdl2/libretro/libretro-common/include/compat \
		-I$(NP2_PATH)/sdl2/libretro/libretro-common/include/dynamic \
		-I$(NP2_PATH)/sdl2/libretro/libretro-common/include/encodings \
		-I$(NP2_PATH)/sdl2/libretro/libretro-common/include/features \
		-I$(NP2_PATH)/sdl2/libretro/libretro-common/include/file \
		-I$(NP2_PATH)/sdl2/libretro/libretro-common/include/formats \
		-I$(NP2_PATH)/sdl2/libretro/libretro-common/include/gfx \
		-I$(NP2_PATH)/sdl2/libretro/libretro-common/include/gfx/math \
		-I$(NP2_PATH)/sdl2/libretro/libretro-common/include/gfx/scaler \
		-I$(NP2_PATH)/sdl2/libretro/libretro-common/include/glsm \
		-I$(NP2_PATH)/sdl2/libretro/libretro-common/include/glsym \
		-I$(NP2_PATH)/sdl2/libretro/libretro-common/include/glsym/switch \
		-I$(NP2_PATH)/sdl2/libretro/libretro-common/include/libchdr \
		-I$(NP2_PATH)/sdl2/libretro/libretro-common/include/lists \
		-I$(NP2_PATH)/sdl2/libretro/libretro-common/include/math \
		-I$(NP2_PATH)/sdl2/libretro/libretro-common/include/net \
		-I$(NP2_PATH)/sdl2/libretro/libretro-common/include/queues \
		-I$(NP2_PATH)/sdl2/libretro/libretro-common/include/rthreads \
		-I$(NP2_PATH)/sdl2/libretro/libretro-common/include/streams \
		-I$(NP2_PATH)/sdl2/libretro/libretro-common/include/string \
		-I$(NP2_PATH)/sdl2/libretro/libretro-common/include/utils \
		-I$(NP2_PATH)/sdl2/libretro/libretro-common/include/vfs \
		-I$(NP2_PATH)/sdl2/libretro/libretro-common/include/vulkan \
		-I$(NP2_PATH)/i386c \
		-I$(NP2_PATH)/i386c/ia32 \
		-I$(NP2_PATH)/i386c/ia32/instructions \
		-I$(NP2_PATH)/i386c/ia32/instructions/fpu

SOURCES_C += $(NP2_PATH)/sdl2/libretro/libretro.c \
		$(wildcard $(NP2_PATH)/i386c/*.c) \
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
		$(wildcard $(NP2_PATH)/i386c/ia32/instructions/sse3/*.c)

#		$(NP2_PATH)/sdl2/libretro/libretro-common/file/archive_file_zlib.c \
#		$(NP2_PATH)/sdl2/libretro/libretro-common/formats/libchdr/libchdr_flac.c \
#		$(NP2_PATH)/sdl2/libretro/libretro-common/formats/libchdr/libchdr_flac_codec.c \
#		$(NP2_PATH)/sdl2/libretro/libretro-common/streams/trans_stream_zlib.c

ifneq ($(STATIC_LINKING), 1)
	SOURCES_C += 	$(NP2_PATH)/sdl2/libretro/libretro-common/compat/compat_strcasestr.c \
			$(NP2_PATH)/sdl2/libretro/libretro-common/compat/compat_fnmatch.c \
			$(NP2_PATH)/sdl2/libretro/libretro-common/compat/compat_getopt.c \
			$(NP2_PATH)/sdl2/libretro/libretro-common/compat/compat_posix_string.c \
			$(NP2_PATH)/sdl2/libretro/libretro-common/compat/compat_snprintf.c \
			$(NP2_PATH)/sdl2/libretro/libretro-common/compat/compat_strl.c \
			$(NP2_PATH)/sdl2/libretro/libretro-common/compat/compat_vscprintf.c \
			$(NP2_PATH)/sdl2/libretro/libretro-common/compat/fopen_utf8.c \
			$(NP2_PATH)/sdl2/libretro/libretro-common/encodings/encoding_crc32.c \
			$(NP2_PATH)/sdl2/libretro/libretro-common/encodings/encoding_utf.c \
			$(NP2_PATH)/sdl2/libretro/libretro-common/features/features_cpu.c \
			$(NP2_PATH)/sdl2/libretro/libretro-common/file/config_file.c \
			$(NP2_PATH)/sdl2/libretro/libretro-common/file/config_file_userdata.c \
			$(NP2_PATH)/sdl2/libretro/libretro-common/file/file_path.c \
			$(NP2_PATH)/sdl2/libretro/libretro-common/file/file_path_io.c \
			$(NP2_PATH)/sdl2/libretro/libretro-common/file/retro_dirent.c \
			$(NP2_PATH)/sdl2/libretro/libretro-common/file/nbio/nbio_intf.c \
			$(NP2_PATH)/sdl2/libretro/libretro-common/file/nbio/nbio_orbis.c \
			$(NP2_PATH)/sdl2/libretro/libretro-common/file/nbio/nbio_stdio.c \
			$(NP2_PATH)/sdl2/libretro/libretro-common/file/nbio/nbio_unixmmap.c \
			$(NP2_PATH)/sdl2/libretro/libretro-common/file/nbio/nbio_windowsmmap.c \
			$(NP2_PATH)/sdl2/libretro/libretro-common/formats/bmp/rbmp.c \
			$(NP2_PATH)/sdl2/libretro/libretro-common/formats/bmp/rbmp_encode.c \
			$(NP2_PATH)/sdl2/libretro/libretro-common/formats/jpeg/rjpeg.c \
			$(NP2_PATH)/sdl2/libretro/libretro-common/formats/json/jsonsax.c \
			$(NP2_PATH)/sdl2/libretro/libretro-common/formats/json/jsonsax_full.c \
			$(NP2_PATH)/sdl2/libretro/libretro-common/formats/libchdr/libchdr_bitstream.c \
			$(NP2_PATH)/sdl2/libretro/libretro-common/formats/libchdr/libchdr_cdrom.c \
			$(NP2_PATH)/sdl2/libretro/libretro-common/formats/libchdr/libchdr_chd.c \
			$(NP2_PATH)/sdl2/libretro/libretro-common/formats/libchdr/libchdr_huffman.c \
			$(NP2_PATH)/sdl2/libretro/libretro-common/formats/png/rpng.c \
			$(NP2_PATH)/sdl2/libretro/libretro-common/formats/png/rpng_encode.c \
			$(NP2_PATH)/sdl2/libretro/libretro-common/formats/tga/rtga.c \
			$(NP2_PATH)/sdl2/libretro/libretro-common/formats/wav/rwav.c \
			$(NP2_PATH)/sdl2/libretro/libretro-common/formats/image_texture.c \
			$(NP2_PATH)/sdl2/libretro/libretro-common/formats/image_transfer.c \
			$(NP2_PATH)/sdl2/libretro/libretro-common/hash/rhash.c \
			$(NP2_PATH)/sdl2/libretro/libretro-common/lists/dir_list.c \
			$(NP2_PATH)/sdl2/libretro/libretro-common/lists/file_list.c \
			$(NP2_PATH)/sdl2/libretro/libretro-common/lists/string_list.c \
			$(NP2_PATH)/sdl2/libretro/libretro-common/lists/vector_list.c \
			$(NP2_PATH)/sdl2/libretro/libretro-common/streams/chd_stream.c \
			$(NP2_PATH)/sdl2/libretro/libretro-common/streams/file_stream.c \
			$(NP2_PATH)/sdl2/libretro/libretro-common/streams/file_stream_transforms.c \
			$(NP2_PATH)/sdl2/libretro/libretro-common/streams/interface_stream.c \
			$(NP2_PATH)/sdl2/libretro/libretro-common/streams/memory_stream.c \
			$(NP2_PATH)/sdl2/libretro/libretro-common/streams/stdin_stream.c \
			$(NP2_PATH)/sdl2/libretro/libretro-common/streams/trans_stream.c \
			$(NP2_PATH)/sdl2/libretro/libretro-common/streams/trans_stream_pipe.c \
			$(NP2_PATH)/sdl2/libretro/libretro-common/string/stdstring.c
endif

COREFLAGS := -D__LIBRETRO__ $(INCFLAGS) $(NP2DEFINE) $(NP21DEFINE) -DSUPPORT_NP2_TICKCOUNT

ifeq ($(SUPPORT_NP2_THREAD), 1)
	SOURCES_C += $(NP2_PATH)/sdl2/libretro/libretro-common/rthreads/rthreads.c \
		$(NP2_PATH)/sdl2/libretro/rsemaphore.c
endif
ifeq ($(SUPPORT_DIRENT), 1)
	SOURCES_C += $(NP2_PATH)/sdl2/libretro/libretro-common/vfs/vfs_implementation.c
endif

GIT_VERSION := " $(shell git rev-parse --short HEAD || echo unknown)"
ifneq ($(GIT_VERSION)," unknown")
	COREFLAGS += -DGIT_VERSION=\"$(GIT_VERSION)\"
endif
include $(CLEAR_VARS)
LOCAL_MODULE    := retro
LOCAL_SRC_FILES := $(SOURCES_C) $(SOURCES_CXX)
LOCAL_CFLAGS    := $(COREFLAGS)
LOCAL_CXXFLAGS  := $(COREFLAGS)
LOCAL_LDFLAGS   := -Wl,-version-script=$(CORE_DIR)/sdl2/link.T
include $(BUILD_SHARED_LIBRARY)
