LOCAL_PATH := $(call my-dir)

CORE_DIR := $(LOCAL_PATH)/..

SUPPORT_NET := 1

include $(CORE_DIR)/sdl2/Makefile.common

INCFLAGS += 	-I$(NP2_PATH)/sdl2/libretro \
		-I$(NP2_PATH)/sdl2/libretro/libretro-common \
		-I$(NP2_PATH)/sdl2/libretro/libretro-common/include \
		-I$(NP2_PATH)/i386c \
		-I$(NP2_PATH)/i386c/ia32 \
		-I$(NP2_PATH)/i386c/ia32/instructions \
		-I$(NP2_PATH)/i386c/ia32/instructions/fpu \
		-I$(NP2_PATH)/i386c/ia32/instructions/fpu/softfloat \
		-I$(NP2_PATH)/i386c/ia32/instructions/mmx \
		-I$(NP2_PATH)/i386c/ia32/instructions/sse

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
		$(NP2_PATH)/sdl2/libretro/libretro-common/file/archive_file.c \
		$(NP2_PATH)/sdl2/libretro/libretro-common/file/config_file.c \
		$(NP2_PATH)/sdl2/libretro/libretro-common/file/config_file_userdata.c \
		$(NP2_PATH)/sdl2/libretro/libretro-common/file/file_path.c \
		$(NP2_PATH)/sdl2/libretro/libretro-common/file/retro_dirent.c \
		$(NP2_PATH)/sdl2/libretro/libretro-common/file/nbio/nbio_intf.c \
		$(NP2_PATH)/sdl2/libretro/libretro-common/file/nbio/nbio_linux.c \
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
		$(NP2_PATH)/sdl2/libretro/libretro-common/formats/libchdr/libchdr_zlib.c \
		$(NP2_PATH)/sdl2/libretro/libretro-common/formats/png/rpng.c \
		$(NP2_PATH)/sdl2/libretro/libretro-common/formats/png/rpng_encode.c \
		$(NP2_PATH)/sdl2/libretro/libretro-common/formats/tga/rtga.c \
		$(NP2_PATH)/sdl2/libretro/libretro-common/formats/wav/rwav.c \
		$(NP2_PATH)/sdl2/libretro/libretro-common/formats/xml/rxml.c \
		$(NP2_PATH)/sdl2/libretro/libretro-common/formats/image_texture.c \
		$(NP2_PATH)/sdl2/libretro/libretro-common/formats/image_transfer.c \
		$(NP2_PATH)/sdl2/libretro/libretro-common/lists/dir_list.c \
		$(NP2_PATH)/sdl2/libretro/libretro-common/lists/file_list.c \
		$(NP2_PATH)/sdl2/libretro/libretro-common/lists/string_list.c \
		$(NP2_PATH)/sdl2/libretro/libretro-common/lists/vector_list.c \
		$(NP2_PATH)/sdl2/libretro/libretro-common/rthreads/rsemaphore.c \
		$(NP2_PATH)/sdl2/libretro/libretro-common/rthreads/rthreads.c \
		$(NP2_PATH)/sdl2/libretro/libretro-common/streams/chd_stream.c \
		$(NP2_PATH)/sdl2/libretro/libretro-common/streams/file_stream.c \
		$(NP2_PATH)/sdl2/libretro/libretro-common/streams/file_stream_transforms.c \
		$(NP2_PATH)/sdl2/libretro/libretro-common/streams/interface_stream.c \
		$(NP2_PATH)/sdl2/libretro/libretro-common/streams/memory_stream.c \
		$(NP2_PATH)/sdl2/libretro/libretro-common/streams/stdin_stream.c \
		$(NP2_PATH)/sdl2/libretro/libretro-common/streams/trans_stream.c \
		$(NP2_PATH)/sdl2/libretro/libretro-common/streams/trans_stream_pipe.c \
		$(NP2_PATH)/sdl2/libretro/libretro-common/string/stdstring.c \
		$(NP2_PATH)/sdl2/libretro/libretro-common/vfs/vfs_implementation.c \
		$(NP2_PATH)/sdl2/libretro/libretro.c \
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

COREFLAGS := -D__LIBRETRO__ $(INCFLAGS) -DCPUCORE_IA32 -DSUPPORT_PC9821 -DUSE_FPU -DSUPPORT_LARGE_HDD -DSUPPORT_VPCVHD -DSUPPORT_KAI_IMAGES -DHOOK_SYSKEY -DALLOW_MULTIRUN -DSUPPORT_WAB -DSUPPORT_LARGE_MEMORY -DSUPPORT_CL_GD5430 -DUSE_MAME -DSUPPORT_FMGEN -DSUPPORT_SOUND_SB16 -DSUPPORT_FPU_DOSBOX -DSUPPORT_FPU_DOSBOX2 -DSUPPORT_FPU_SOFTFLOAT -DSUPPORT_FAST_MEMORYCHECK -DSUPPORT_PEGC -DSUPPORT_ASYNC_CPU -DSUPPORT_GPIB -DSUPPORT_PCI -DUSE_MMX -DUSE_3DNOW -DUSE_SSE -DUSE_SSE2 -DUSE_SSE3 -DUSE_TSC -DSUPPORT_NVL_IMAGES -DUSE_FASTPAGING -DUSE_VME -DBIOS_IO_EMULATION

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
