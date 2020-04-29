GIT_VERSION := "$(shell git rev-parse --short HEAD)"
real_topsrcdir=	$(top_srcdir)/..

dist_pkgdata_DATA=	$(real_topsrcdir)/x11/resources/fddseek.wav \
			$(real_topsrcdir)/x11/resources/fddseek1.wav \
			$(real_topsrcdir)/x11/resources/relay1.wav
dist_pkgdata_DATA+=	$(real_topsrcdir)/np2tool/np2tool.d88

#EMUFM_SOURCES=	$(real_topsrcdir)/sound/opl3.c \
#		$(real_topsrcdir)/sound/opna.c

# CAVEAT:
# sccisoundinterfacemanager.cpp must be linked before externalchipmanager.cpp.
# Otherwise, it will take the curse of destructor calls the order of the
# global instances.
EXTFM_SOURCES=	$(real_topsrcdir)/x11/ext/opl3.cpp \
		$(real_topsrcdir)/x11/ext/opna.cpp \
		$(real_topsrcdir)/x11/ext/c86ctl/c86ctlc86box.cpp \
		$(real_topsrcdir)/x11/ext/c86ctl/c86ctlgimic.cpp \
		$(real_topsrcdir)/x11/ext/c86ctl/c86ctlif.cpp \
		$(real_topsrcdir)/x11/ext/c86ctl/c86ctlrealchipbase.cpp \
		$(real_topsrcdir)/x11/ext/scci/scciif.cpp \
		$(real_topsrcdir)/x11/ext/scci/sccisoundchip.cpp \
		$(real_topsrcdir)/x11/ext/scci/sccisoundinterface.cpp \
		$(real_topsrcdir)/x11/ext/scci/sccisoundinterfacemanager.cpp \
		$(real_topsrcdir)/x11/ext/scci/sccispfmlight.cpp \
		$(real_topsrcdir)/x11/ext/externalchipmanager.cpp \
		$(real_topsrcdir)/x11/ext/externalopl3.cpp \
		$(real_topsrcdir)/x11/ext/externalopm.cpp \
		$(real_topsrcdir)/x11/ext/externalopna.cpp \
		$(real_topsrcdir)/x11/ext/externalpsg.cpp

COMMON_SOURCES=	$(real_topsrcdir)/x11/main.c \
		$(real_topsrcdir)/x11/np2.c \
		$(real_topsrcdir)/x11/dosio.c \
		$(real_topsrcdir)/x11/ini.c \
		$(real_topsrcdir)/x11/oemtext.c \
		$(real_topsrcdir)/x11/trace.c \
		$(real_topsrcdir)/x11/kdispwin.c \
		$(real_topsrcdir)/x11/skbdwin.c \
		$(real_topsrcdir)/x11/cmmidi.c \
		$(real_topsrcdir)/x11/cmserial.c \
		$(real_topsrcdir)/x11/drawmng.c \
		$(real_topsrcdir)/x11/fontmng.c \
		$(real_topsrcdir)/x11/kbdmng.c \
		$(real_topsrcdir)/x11/commng.c \
		$(real_topsrcdir)/x11/joymng.c \
		$(real_topsrcdir)/x11/soundmng.c \
		$(real_topsrcdir)/x11/sysmng.c \
		$(real_topsrcdir)/x11/taskmng.c \
		$(real_topsrcdir)/x11/timemng.c \
		$(real_topsrcdir)/x11/gtk2/dialog_about.c \
		$(real_topsrcdir)/x11/gtk2/dialog_calendar.c \
		$(real_topsrcdir)/x11/gtk2/dialog_config.c \
		$(real_topsrcdir)/x11/gtk2/dialog_hostdrv.c \
		$(real_topsrcdir)/x11/gtk2/dialog_ide.c \
		$(real_topsrcdir)/x11/gtk2/dialog_midi.c \
		$(real_topsrcdir)/x11/gtk2/dialog_network.c \
		$(real_topsrcdir)/x11/gtk2/dialog_newdisk.c \
		$(real_topsrcdir)/x11/gtk2/dialog_pci.c \
		$(real_topsrcdir)/x11/gtk2/dialog_screen.c \
		$(real_topsrcdir)/x11/gtk2/dialog_serial.c \
		$(real_topsrcdir)/x11/gtk2/dialog_sound.c \
		$(real_topsrcdir)/x11/gtk2/dialog_wab.c \
		$(real_topsrcdir)/x11/gtk2/window_keydisp.c \
		$(real_topsrcdir)/x11/gtk2/window_softkbd.c \
		$(real_topsrcdir)/x11/gtk2/gtk_drawmng.c \
		$(real_topsrcdir)/x11/gtk2/gtk_screen.c \
		$(real_topsrcdir)/x11/gtk2/gtk_keyboard.c \
		$(real_topsrcdir)/x11/gtk2/gtk_menu.c \
		$(real_topsrcdir)/x11/gtk2/gtk_mouse.c \
		$(real_topsrcdir)/x11/gtk2/gtk_main.c \
		$(real_topsrcdir)/x11/gtk2/gtk_wrapper.c \
		$(real_topsrcdir)/x11/debug/viewmem.c \
		$(real_topsrcdir)/x11/misc/threadbase.cpp \
		$(real_topsrcdir)/x11/misc/tty.cpp \
		$(real_topsrcdir)/x11/misc/usbdev.cpp \
		\
		$(EXTFM_SOURCES) \
		\
		$(real_topsrcdir)/pccore.c \
		$(real_topsrcdir)/nevent.c \
		$(real_topsrcdir)/calendar.c \
		$(real_topsrcdir)/timing.c \
		$(real_topsrcdir)/statsave.c \
		$(real_topsrcdir)/keystat.c \
		$(real_topsrcdir)/np2_thread.c \
		$(real_topsrcdir)/np2_tickcount.c \
		\
		$(real_topsrcdir)/common/strres.c \
		$(real_topsrcdir)/common/milstr.c \
		$(real_topsrcdir)/common/_memory.c \
		$(real_topsrcdir)/common/textfile.c \
		$(real_topsrcdir)/common/profile.c \
		$(real_topsrcdir)/common/rect.c \
		$(real_topsrcdir)/common/lstarray.c \
		$(real_topsrcdir)/common/bmpdata.c \
		$(real_topsrcdir)/common/mimpidef.c \
		$(real_topsrcdir)/common/parts.c \
		$(real_topsrcdir)/common/wavefile.c \
		\
		$(real_topsrcdir)/codecnv/eucsjis.c \
		$(real_topsrcdir)/codecnv/eucucs2.c \
		$(real_topsrcdir)/codecnv/jisucs2.c \
		$(real_topsrcdir)/codecnv/sjiseuc.c \
		$(real_topsrcdir)/codecnv/sjisucs2.c \
		$(real_topsrcdir)/codecnv/tcswap16.c \
		$(real_topsrcdir)/codecnv/tcswap32.c \
		$(real_topsrcdir)/codecnv/textcnv.c \
		$(real_topsrcdir)/codecnv/ucs2sjis.c \
		$(real_topsrcdir)/codecnv/ucs2ucs4.c \
		$(real_topsrcdir)/codecnv/ucs2utf8.c \
		$(real_topsrcdir)/codecnv/ucs4ucs2.c \
		$(real_topsrcdir)/codecnv/ucs4utf8.c \
		$(real_topsrcdir)/codecnv/utf8ucs2.c \
		$(real_topsrcdir)/codecnv/utf8ucs4.c \
		\
		$(real_topsrcdir)/bios/bios.c \
		$(real_topsrcdir)/bios/bios09.c \
		$(real_topsrcdir)/bios/bios0c.c \
		$(real_topsrcdir)/bios/bios12.c \
		$(real_topsrcdir)/bios/bios13.c \
		$(real_topsrcdir)/bios/bios18.c \
		$(real_topsrcdir)/bios/bios19.c \
		$(real_topsrcdir)/bios/bios1a.c \
		$(real_topsrcdir)/bios/bios1b.c \
		$(real_topsrcdir)/bios/bios1c.c \
		$(real_topsrcdir)/bios/bios1f.c \
		$(real_topsrcdir)/bios/sxsibios.c \
		\
		$(real_topsrcdir)/cbus/cbuscore.c \
		$(real_topsrcdir)/cbus/pc9861k.c \
		$(real_topsrcdir)/cbus/mpu98ii.c \
		$(real_topsrcdir)/cbus/amd98.c \
		$(real_topsrcdir)/cbus/board118.c \
		$(real_topsrcdir)/cbus/board14.c \
		$(real_topsrcdir)/cbus/board26k.c \
		$(real_topsrcdir)/cbus/board86.c \
		$(real_topsrcdir)/cbus/boardpx.c \
		$(real_topsrcdir)/cbus/boardsb16.c \
		$(real_topsrcdir)/cbus/boardso.c \
		$(real_topsrcdir)/cbus/boardlol.c \
		$(real_topsrcdir)/cbus/boardmo.c \
		$(real_topsrcdir)/cbus/boardspb.c \
		$(real_topsrcdir)/cbus/boardx2.c \
		$(real_topsrcdir)/cbus/cs4231io.c \
		$(real_topsrcdir)/cbus/ct1741io.c \
		$(real_topsrcdir)/cbus/ct1745io.c \
		$(real_topsrcdir)/cbus/gpibio.c \
		$(real_topsrcdir)/cbus/pcm86io.c \
		$(real_topsrcdir)/cbus/sasiio.c \
		$(real_topsrcdir)/cbus/scsiio.c \
		$(real_topsrcdir)/cbus/scsicmd.c \
		$(real_topsrcdir)/cbus/ideio.c \
		$(real_topsrcdir)/cbus/atapicmd.c \
		$(real_topsrcdir)/cbus/smpu98.c \
		\
		$(real_topsrcdir)/fdd/diskdrv.c \
		$(real_topsrcdir)/fdd/newdisk.c \
		$(real_topsrcdir)/fdd/fdd_mtr.c \
		$(real_topsrcdir)/fdd/sxsi.c \
		$(real_topsrcdir)/fdd/sxsihdd.c \
		$(real_topsrcdir)/fdd/sxsicd.c \
		$(real_topsrcdir)/fdd/sxsihdd_nvl.c \
		\
		$(real_topsrcdir)/font/font.c \
		$(real_topsrcdir)/font/fontdata.c \
		$(real_topsrcdir)/font/fontmake.c \
		$(real_topsrcdir)/font/fontpc88.c \
		$(real_topsrcdir)/font/fontpc98.c \
		$(real_topsrcdir)/font/fontv98.c \
		$(real_topsrcdir)/font/fontfm7.c \
		$(real_topsrcdir)/font/fontx1.c \
		$(real_topsrcdir)/font/fontx68k.c \
		\
		$(real_topsrcdir)/generic/cmndraw.c \
		$(real_topsrcdir)/generic/keydisp.c \
		$(real_topsrcdir)/generic/cmjasts.c \
		$(real_topsrcdir)/generic/hostdrv.c \
		$(real_topsrcdir)/generic/hostdrvs.c \
		$(real_topsrcdir)/generic/unasm.c \
		$(real_topsrcdir)/generic/np2info.c \
		$(real_topsrcdir)/generic/memdbg32.c \
		$(real_topsrcdir)/generic/softkbd.c \
		\
		$(real_topsrcdir)/io/iocore.c \
		$(real_topsrcdir)/io/artic.c \
		$(real_topsrcdir)/io/bmsio.c \
		$(real_topsrcdir)/io/cgrom.c \
		$(real_topsrcdir)/io/cpuio.c \
		$(real_topsrcdir)/io/crtc.c \
		$(real_topsrcdir)/io/dipsw.c \
		$(real_topsrcdir)/io/dmac.c \
		$(real_topsrcdir)/io/egc.c \
		$(real_topsrcdir)/io/emsio.c \
		$(real_topsrcdir)/io/fdc.c \
		$(real_topsrcdir)/io/fdd320.c \
		$(real_topsrcdir)/io/gdc.c \
		$(real_topsrcdir)/io/gdc_sub.c \
		$(real_topsrcdir)/io/gdc_pset.c \
		$(real_topsrcdir)/io/mouseif.c \
		$(real_topsrcdir)/io/nmiio.c \
		$(real_topsrcdir)/io/np2sysp.c \
		$(real_topsrcdir)/io/pic.c \
		$(real_topsrcdir)/io/pit.c \
		$(real_topsrcdir)/io/printif.c \
		$(real_topsrcdir)/io/serial.c \
		$(real_topsrcdir)/io/sysport.c \
		$(real_topsrcdir)/io/upd4990.c \
		$(real_topsrcdir)/io/necio.c \
		$(real_topsrcdir)/io/epsonio.c \
		$(real_topsrcdir)/io/pcidev.c \
		$(real_topsrcdir)/io/pegc.c \
		$(real_topsrcdir)/io/pci/98graphbridge.c \
		$(real_topsrcdir)/io/pci/cbusbridge.c \
		\
		$(real_topsrcdir)/lio/lio.c \
		$(real_topsrcdir)/lio/gscreen.c \
		$(real_topsrcdir)/lio/gpset.c \
		$(real_topsrcdir)/lio/gline.c \
		$(real_topsrcdir)/lio/gcircle.c \
		$(real_topsrcdir)/lio/gput1.c \
		\
		$(real_topsrcdir)/mem/dmav30.c \
		$(real_topsrcdir)/mem/dmax86.c \
		$(real_topsrcdir)/mem/memegc.c \
		$(real_topsrcdir)/mem/memems.c \
		$(real_topsrcdir)/mem/memepp.c \
		$(real_topsrcdir)/mem/memtram.c \
		$(real_topsrcdir)/mem/memvga.c \
		$(real_topsrcdir)/mem/memvram.c \
		\
		$(real_topsrcdir)/sound/sound.c \
		$(real_topsrcdir)/sound/soundrom.c \
		$(real_topsrcdir)/sound/s98.c \
		$(real_topsrcdir)/sound/adpcmc.c \
		$(real_topsrcdir)/sound/adpcmg.c \
		$(real_topsrcdir)/sound/beepc.c \
		$(real_topsrcdir)/sound/beepg.c \
		$(real_topsrcdir)/sound/cs4231c.c \
		$(real_topsrcdir)/sound/cs4231g.c \
		$(real_topsrcdir)/sound/fmboard.c \
		$(real_topsrcdir)/sound/oplgenc.c \
		$(real_topsrcdir)/sound/oplgeng.c \
		$(real_topsrcdir)/sound/opngenc.c \
		$(real_topsrcdir)/sound/opngeng.c \
		$(real_topsrcdir)/sound/opntimer.c \
		$(real_topsrcdir)/sound/pcm86c.c \
		$(real_topsrcdir)/sound/pcm86g.c \
		$(real_topsrcdir)/sound/pcmmix.c \
		$(real_topsrcdir)/sound/psggenc.c \
		$(real_topsrcdir)/sound/psggeng.c \
		$(real_topsrcdir)/sound/rhythmc.c \
		$(real_topsrcdir)/sound/sndcsec.c \
		$(real_topsrcdir)/sound/tms3631c.c \
		$(real_topsrcdir)/sound/tms3631g.c \
		$(real_topsrcdir)/sound/getsnd/getsnd.c \
		$(real_topsrcdir)/sound/getsnd/getsmix.c \
		$(real_topsrcdir)/sound/getsnd/getmp3.c \
		$(real_topsrcdir)/sound/getsnd/getogg.c \
		$(real_topsrcdir)/sound/getsnd/getwave.c \
		$(real_topsrcdir)/sound/vermouth/midiout.c \
		$(real_topsrcdir)/sound/vermouth/midimod.c \
		$(real_topsrcdir)/sound/vermouth/midinst.c \
		$(real_topsrcdir)/sound/vermouth/midvoice.c \
		$(real_topsrcdir)/sound/vermouth/midtable.c \
		$(real_topsrcdir)/sound/fmgen/fmgen_file.cpp \
		$(real_topsrcdir)/sound/fmgen/fmgen_fmgen.cpp \
		$(real_topsrcdir)/sound/fmgen/fmgen_fmgwrap.cpp \
		$(real_topsrcdir)/sound/fmgen/fmgen_fmtimer.cpp \
		$(real_topsrcdir)/sound/fmgen/fmgen_opm.cpp \
		$(real_topsrcdir)/sound/fmgen/fmgen_opna.cpp \
		$(real_topsrcdir)/sound/fmgen/fmgen_psg.cpp \
		$(real_topsrcdir)/sound/mame/fmopl.c \
		$(real_topsrcdir)/sound/mame/ymdeltat.c \
		$(real_topsrcdir)/sound/mame/ymf262.c \
		\
		$(real_topsrcdir)/vram/vram.c \
		$(real_topsrcdir)/vram/scrndraw.c \
		$(real_topsrcdir)/vram/sdraw.c \
		$(real_topsrcdir)/vram/dispsync.c \
		$(real_topsrcdir)/vram/palettes.c \
		$(real_topsrcdir)/vram/maketext.c \
		$(real_topsrcdir)/vram/maketgrp.c \
		$(real_topsrcdir)/vram/makegrph.c \
		$(real_topsrcdir)/vram/makegrex.c \
		$(real_topsrcdir)/vram/scrnsave.c \
		\
		$(real_topsrcdir)/diskimage/cd/cdd_ccd.c \
		$(real_topsrcdir)/diskimage/cd/cdd_cue.c \
		$(real_topsrcdir)/diskimage/cd/cdd_iso.c \
		$(real_topsrcdir)/diskimage/cd/cdd_mds.c \
		$(real_topsrcdir)/diskimage/cd/cdd_nrg.c \
		$(real_topsrcdir)/diskimage/fd/fdd_bkdsk.c \
		$(real_topsrcdir)/diskimage/fd/fdd_d88.c \
		$(real_topsrcdir)/diskimage/fd/fdd_dcp.c \
		$(real_topsrcdir)/diskimage/fd/fdd_nfd.c \
		$(real_topsrcdir)/diskimage/fd/fdd_vfdd.c \
		$(real_topsrcdir)/diskimage/fd/fdd_xdf.c \
		$(real_topsrcdir)/diskimage/cddfile.c \
		$(real_topsrcdir)/diskimage/fddfile.c \
		$(real_topsrcdir)/diskimage/img_strres.c \
		\
		$(real_topsrcdir)/network/lgy98.c \
		$(real_topsrcdir)/network/net.c \
		\
		$(real_topsrcdir)/wab/cirrus_vga.c \
		$(real_topsrcdir)/wab/wab_rly.c \
		$(real_topsrcdir)/wab/wab.c

AM_CPPFLAGS=	-I$(real_topsrcdir) \
		-I$(real_topsrcdir)/x11 \
		-I$(real_topsrcdir)/x11/gtk2 \
		-I$(real_topsrcdir)/x11/debug \
		-I$(real_topsrcdir)/common \
		-I$(real_topsrcdir)/cbus \
		-I$(real_topsrcdir)/fdd \
		-I$(real_topsrcdir)/generic \
		-I$(real_topsrcdir)/io \
		-I$(real_topsrcdir)/io/pci \
		-I$(real_topsrcdir)/mem \
		-I$(real_topsrcdir)/network \
		-I$(real_topsrcdir)/sound \
		-I$(real_topsrcdir)/sound/fmgen \
		-I$(real_topsrcdir)/vram \
		-I$(real_topsrcdir)/wab \
		$(GTK_CFLAGS) $(GDK_CFLAGS) $(GDK_PIXBUF_CFLAGS) $(SDL_CFLAGS) $(LIBUSB1_CFLAGS) \
		$(X11_CFLAGS) $(XEXT_CFLAGS) \
		 -DGIT_VERSION=\"$(GIT_VERSION)\" -DNP2_X11 -DUSE_SDLAUDIO -DUSE_SDLMIXER -DBIOS_IO_EMULATION -DOSLANG_UTF8 -DOSLINEBREAK_LF -DSOUND_CRITICAL -DSUPPORT_16BPP -DSUPPORT_24BPP -DSUPPORT_32BPP -DSUPPORT_8BPP -DSUPPORT_BMS -DSUPPORT_CL_GD5430 -DSUPPORT_CRT15KHZ -DSUPPORT_FAST_MEMORYCHECK -DSUPPORT_FMGEN -DSUPPORT_GPIB -DSUPPORT_HOSTDRV -DSUPPORT_HRTIMER -DSUPPORT_IDEIO -DSUPPORT_KAI_IMAGES -DSUPPORT_LARGE_HDD -DSUPPORT_LGY98 -DSUPPORT_NET -DSUPPORT_NORMALDISP -DSUPPORT_NVL_IMAGES -DSUPPORT_PC9861K -DSUPPORT_PX -DSUPPORT_RESUME -DSUPPORT_RS232C_FIFO -DSUPPORT_S98 -DSUPPORT_SCSI -DSUPPORT_SMPU98 -DSUPPORT_SOFTKBD=0 -DSUPPORT_SOUND_SB16 -DSUPPORT_SWSEEKSND -DSUPPORT_SWWABRLYSND -DSUPPORT_STATSAVE=10 -DSUPPORT_UTF8 -DSUPPORT_V30EXT -DSUPPORT_V30ORIGINAL -DSUPPORT_VPCVHD -DSUPPORT_WAB -DUSE_MAME -DVAEG_FIX -DVERMOUTH_LIB -DSUPPORT_ASYNC_CPU -DSUPPORT_NP2_THREAD -DNP2_THREAD_POSIX -DSUPPORT_NP2_TICKCOUNT -DUSE_RESOURCE_BMP


if BUILD_ALL
AM_CPPFLAGS+=	-DX11_BUILD_ALL
endif

if HAVE_GCC
AM_CFLAGS=	-fsigned-char
AM_CFLAGS+=	-fno-strict-aliasing
AM_CXXFLAGS=	$(AM_CFLAGS)
AM_CXXFLAGS+=	-std=c++11
endif

LDADD=		$(GTK_LIBS) $(GDK_LIBS) $(GDK_PIXBUF_LIBS) $(SDL_LIBS) $(LIBUSB1_LIBS) \
		$(X11_LIBS) $(XEXT_LIBS) \
		$(X_LIBS) $(X_PRE_LIBS) $(XLIB) $(X_EXTRA_LIBS) -ldl -lpthread

DEFS=		@DEFS@ -DSYSRESPATH=\""$(pkgdatadir)"\"

I286C_SOURCES=	$(real_topsrcdir)/i286c/i286c.c \
		$(real_topsrcdir)/i286c/cpumem.c \
		$(real_topsrcdir)/i286c/i286c_mn.c \
		$(real_topsrcdir)/i286c/i286c_ea.c \
		$(real_topsrcdir)/i286c/i286c_0f.c \
		$(real_topsrcdir)/i286c/i286c_8x.c \
		$(real_topsrcdir)/i286c/i286c_sf.c \
		$(real_topsrcdir)/i286c/i286c_f6.c \
		$(real_topsrcdir)/i286c/i286c_fe.c \
		$(real_topsrcdir)/i286c/i286c_rp.c \
		$(real_topsrcdir)/i286c/v30patch.c \
		$(real_topsrcdir)/debugsub.c

I286C_CPPFLAGS=	-I$(real_topsrcdir)/i286c

IA32_SOURCES=	$(real_topsrcdir)/i386c/cpucore.c \
		$(real_topsrcdir)/i386c/cpumem.c \
		$(real_topsrcdir)/i386c/ia32/interface.c \
		$(real_topsrcdir)/i386c/ia32/ia32.c \
		$(real_topsrcdir)/i386c/ia32/cpu.c \
		$(real_topsrcdir)/i386c/ia32/cpu_io.c \
		$(real_topsrcdir)/i386c/ia32/cpu_mem.c \
		$(real_topsrcdir)/i386c/ia32/ctrlxfer.c \
		$(real_topsrcdir)/i386c/ia32/debug.c \
		$(real_topsrcdir)/i386c/ia32/disasm.c \
		$(real_topsrcdir)/i386c/ia32/exception.c \
		$(real_topsrcdir)/i386c/ia32/groups.c \
		$(real_topsrcdir)/i386c/ia32/inst_table.c \
		$(real_topsrcdir)/i386c/ia32/paging.c \
		$(real_topsrcdir)/i386c/ia32/resolve.c \
		$(real_topsrcdir)/i386c/ia32/segments.c \
		$(real_topsrcdir)/i386c/ia32/task.c \
		$(real_topsrcdir)/i386c/ia32/instructions/bin_arith.c \
		$(real_topsrcdir)/i386c/ia32/instructions/bit_byte.c \
		$(real_topsrcdir)/i386c/ia32/instructions/ctrl_trans.c \
		$(real_topsrcdir)/i386c/ia32/instructions/data_trans.c \
		$(real_topsrcdir)/i386c/ia32/instructions/dec_arith.c \
		$(real_topsrcdir)/i386c/ia32/instructions/flag_ctrl.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu.c \
		$(real_topsrcdir)/i386c/ia32/instructions/logic_arith.c \
		$(real_topsrcdir)/i386c/ia32/instructions/misc_inst.c \
		$(real_topsrcdir)/i386c/ia32/instructions/seg_reg.c \
		$(real_topsrcdir)/i386c/ia32/instructions/shift_rotate.c \
		$(real_topsrcdir)/i386c/ia32/instructions/string_inst.c \
		$(real_topsrcdir)/i386c/ia32/instructions/system_inst.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/fpdummy.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/fpemul_dosbox.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/fpemul_dosbox2.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/fpemul_softfloat.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_eq128.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_le128.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_lt128.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_shortShiftLeft128.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_shortShiftRight128.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_shortShiftRightJam64.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_shortShiftRightJam64Extra.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_shortShiftRightJam128.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_shortShiftRightJam128Extra.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_shiftRightJam32.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_shiftRightJam64.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_shiftRightJam64Extra.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_shiftRightJam128.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_shiftRightJam128Extra.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_shiftRightJam256M.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_countLeadingZeros8.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_countLeadingZeros16.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_countLeadingZeros32.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_countLeadingZeros64.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_add128.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_add256M.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_sub128.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_sub256M.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_mul64ByShifted32To128.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_mul64To128.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_mul128By32.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_mul128To256M.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_approxRecip_1Ks.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_approxRecip32_1.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_approxRecipSqrt_1Ks.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_approxRecipSqrt32_1.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_roundToUI32.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_roundToUI64.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_roundToI32.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_roundToI64.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_normSubnormalF16Sig.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_roundPackToF16.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_normRoundPackToF16.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_addMagsF16.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_subMagsF16.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_mulAddF16.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_normSubnormalF32Sig.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_roundPackToF32.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_normRoundPackToF32.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_addMagsF32.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_subMagsF32.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_mulAddF32.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_normSubnormalF64Sig.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_roundPackToF64.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_normRoundPackToF64.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_addMagsF64.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_subMagsF64.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_mulAddF64.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_normSubnormalExtF80Sig.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_roundPackToExtF80.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_normRoundPackToExtF80.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_addMagsExtF80.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_subMagsExtF80.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_normSubnormalF128Sig.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_roundPackToF128.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_normRoundPackToF128.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_addMagsF128.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_subMagsF128.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_mulAddF128.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/softfloat_state.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/ui32_to_f16.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/ui32_to_f32.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/ui32_to_f64.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/ui32_to_extF80.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/ui32_to_extF80M.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/ui32_to_f128.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/ui32_to_f128M.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/ui64_to_f16.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/ui64_to_f32.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/ui64_to_f64.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/ui64_to_extF80.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/ui64_to_extF80M.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/ui64_to_f128.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/ui64_to_f128M.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/i32_to_f16.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/i32_to_f32.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/i32_to_f64.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/i32_to_extF80.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/i32_to_extF80M.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/i32_to_f128.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/i32_to_f128M.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/i64_to_f16.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/i64_to_f32.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/i64_to_f64.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/i64_to_extF80.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/i64_to_extF80M.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/i64_to_f128.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/i64_to_f128M.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f16_to_ui32.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f16_to_ui64.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f16_to_i32.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f16_to_i64.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f16_to_ui32_r_minMag.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f16_to_ui64_r_minMag.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f16_to_i32_r_minMag.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f16_to_i64_r_minMag.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f16_to_f32.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f16_to_f64.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f16_to_extF80.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f16_to_extF80M.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f16_to_f128.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f16_to_f128M.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f16_roundToInt.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f16_add.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f16_sub.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f16_mul.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f16_mulAdd.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f16_div.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f16_rem.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f16_sqrt.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f16_eq.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f16_le.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f16_lt.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f16_eq_signaling.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f16_le_quiet.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f16_lt_quiet.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f16_isSignalingNaN.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f32_to_ui32.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f32_to_ui64.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f32_to_i32.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f32_to_i64.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f32_to_ui32_r_minMag.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f32_to_ui64_r_minMag.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f32_to_i32_r_minMag.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f32_to_i64_r_minMag.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f32_to_f16.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f32_to_f64.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f32_to_extF80.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f32_to_extF80M.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f32_to_f128.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f32_to_f128M.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f32_roundToInt.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f32_add.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f32_sub.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f32_mul.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f32_mulAdd.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f32_div.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f32_rem.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f32_sqrt.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f32_eq.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f32_le.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f32_lt.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f32_eq_signaling.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f32_le_quiet.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f32_lt_quiet.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f32_isSignalingNaN.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f64_to_ui32.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f64_to_ui64.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f64_to_i32.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f64_to_i64.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f64_to_ui32_r_minMag.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f64_to_ui64_r_minMag.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f64_to_i32_r_minMag.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f64_to_i64_r_minMag.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f64_to_f16.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f64_to_f32.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f64_to_extF80.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f64_to_extF80M.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f64_to_f128.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f64_to_f128M.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f64_roundToInt.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f64_add.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f64_sub.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f64_mul.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f64_mulAdd.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f64_div.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f64_rem.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f64_sqrt.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f64_eq.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f64_le.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f64_lt.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f64_eq_signaling.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f64_le_quiet.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f64_lt_quiet.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f64_isSignalingNaN.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/extF80_to_ui32.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/extF80_to_ui64.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/extF80_to_i32.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/extF80_to_i64.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/extF80_to_ui32_r_minMag.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/extF80_to_ui64_r_minMag.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/extF80_to_i32_r_minMag.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/extF80_to_i64_r_minMag.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/extF80_to_f16.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/extF80_to_f32.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/extF80_to_f64.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/extF80_to_f128.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/extF80_roundToInt.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/extF80_add.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/extF80_sub.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/extF80_mul.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/extF80_div.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/extF80_rem.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/extF80_sqrt.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/extF80_eq.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/extF80_le.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/extF80_lt.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/extF80_eq_signaling.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/extF80_le_quiet.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/extF80_lt_quiet.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/extF80_isSignalingNaN.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/extF80M_to_ui32.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/extF80M_to_ui64.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/extF80M_to_i32.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/extF80M_to_i64.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/extF80M_to_ui32_r_minMag.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/extF80M_to_ui64_r_minMag.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/extF80M_to_i32_r_minMag.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/extF80M_to_i64_r_minMag.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/extF80M_to_f16.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/extF80M_to_f32.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/extF80M_to_f64.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/extF80M_to_f128M.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/extF80M_roundToInt.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/extF80M_add.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/extF80M_sub.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/extF80M_mul.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/extF80M_div.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/extF80M_rem.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/extF80M_sqrt.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/extF80M_eq.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/extF80M_le.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/extF80M_lt.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/extF80M_eq_signaling.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/extF80M_le_quiet.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/extF80M_lt_quiet.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f128_to_ui32.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f128_to_ui64.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f128_to_i32.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f128_to_i64.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f128_to_ui32_r_minMag.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f128_to_ui64_r_minMag.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f128_to_i32_r_minMag.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f128_to_i64_r_minMag.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f128_to_f16.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f128_to_f32.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f128_to_extF80.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f128_to_f64.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f128_roundToInt.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f128_add.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f128_sub.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f128_mul.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f128_mulAdd.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f128_div.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f128_rem.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f128_sqrt.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f128_eq.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f128_le.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f128_lt.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f128_eq_signaling.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f128_le_quiet.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f128_lt_quiet.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f128_isSignalingNaN.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f128M_to_ui32.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f128M_to_ui64.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f128M_to_i32.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f128M_to_i64.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f128M_to_ui32_r_minMag.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f128M_to_ui64_r_minMag.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f128M_to_i32_r_minMag.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f128M_to_i64_r_minMag.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f128M_to_f16.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f128M_to_f32.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f128M_to_extF80M.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f128M_to_f64.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f128M_roundToInt.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f128M_add.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f128M_sub.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f128M_mul.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f128M_mulAdd.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f128M_div.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f128M_rem.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f128M_sqrt.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f128M_eq.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f128M_le.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f128M_lt.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f128M_eq_signaling.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f128M_le_quiet.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f128M_lt_quiet.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/softfloat_raiseFlags.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_f16UIToCommonNaN.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_commonNaNToF16UI.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_propagateNaNF16UI.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_f32UIToCommonNaN.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_commonNaNToF32UI.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_propagateNaNF32UI.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_f64UIToCommonNaN.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_commonNaNToF64UI.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_propagateNaNF64UI.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/extF80M_isSignalingNaN.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_extF80UIToCommonNaN.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_commonNaNToExtF80UI.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_propagateNaNExtF80UI.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/f128M_isSignalingNaN.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_f128UIToCommonNaN.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_commonNaNToF128UI.c \
		$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat/s_propagateNaNF128UI.c \
		$(real_topsrcdir)/i386c/ia32/instructions/mmx/3dnow.c \
		$(real_topsrcdir)/i386c/ia32/instructions/mmx/mmx.c \
		$(real_topsrcdir)/i386c/ia32/instructions/sse/sse.c \
		$(real_topsrcdir)/i386c/ia32/instructions/sse2/sse2.c \
		$(real_topsrcdir)/i386c/ia32/instructions/sse3/sse3.c \
		$(real_topsrcdir)/debugsub386.c

IA32_CPPFLAGS=	-I$(real_topsrcdir)/i386c \
		-I$(real_topsrcdir)/i386c/ia32 \
		-I$(real_topsrcdir)/i386c/ia32/instructions \
		-I$(real_topsrcdir)/i386c/ia32/instructions/fpu \
		-I$(real_topsrcdir)/i386c/ia32/instructions/fpu/softfloat \
		-I$(real_topsrcdir)/i386c/ia32/instructions/mmx \
		-I$(real_topsrcdir)/i386c/ia32/instructions/sse \
		-I$(real_topsrcdir)/i386c/ia32/instructions/sse2 \
		-I$(real_topsrcdir)/i386c/ia32/instructions/sse3 \
		-DCPUCORE_IA32 -DIA32_PAGING_EACHSIZE -DIA32_REBOOT_ON_PANIC -DSUPPORT_CRT31KHZ -DSUPPORT_FPU_DOSBOX -DSUPPORT_FPU_DOSBOX2 -DSUPPORT_FPU_SOFTFLOAT -DSUPPORT_GAMEPORT -DSUPPORT_MEMDBG32 -DSUPPORT_LARGE_MEMORY -DSUPPORT_PC9801_119 -DSUPPORT_PC9821 -DSUPPORT_PCI -DSUPPORT_PEGC -DSUPPORT_SOUND_SB16 -DSUPPORT_VGA_MODEX -DUSE_3DNOW -DUSE_FASTPAGING -DUSE_FPU -DUSE_MMX -DUSE_SSE -DUSE_SSE2 -DUSE_SSE3 -DUSE_TSC -DUSE_VME

HAXM_SOURCES=	$(real_topsrcdir)/i386hax/haxcore.c \
		$(real_topsrcdir)/i386hax/haxfunc.c
HAXM_CPPFLAGS=	-I$(real_topsrcdir)/i386hax -DSUPPORT_IA32_HAXM -DUSE_CUSTOM_HOOKINST

if BUILD_ALL
bin_PROGRAMS=	xnp2kai xnp21kai xnp21kai_haxm
nodist_man1_MANS= xnp2kai.1 xnp2kai.1j xnp21kai.1 xnp21kai.1j
xnp21kai_haxm_SOURCES=	$(COMMON_SOURCES) $(IA32_SOURCES) $(HAXM_SOURCES)
xnp21kai_haxm_CPPFLAGS=	$(AM_CPPFLAGS) $(IA32_CPPFLAGS) $(HAXM_CPPFLAGS)
xnp21kai_SOURCES=	$(COMMON_SOURCES) $(IA32_SOURCES)
xnp21kai_CPPFLAGS=	$(AM_CPPFLAGS) $(IA32_CPPFLAGS)
xnp2kai_SOURCES=	$(COMMON_SOURCES) $(I286C_SOURCES)
xnp2kai_CPPFLAGS=	$(AM_CPPFLAGS) $(I286C_CPPFLAGS)
else	# BUILD_ALL
if CPUCORE_IA32
if HAXM
bin_PROGRAMS=	xnp21kai_haxm
nodist_man1_MANS= xnp21kai.1 xnp21kai.1j
xnp21kai_haxm_SOURCES=	$(COMMON_SOURCES) $(IA32_SOURCES) $(HAXM_SOURCES)
xnp21kai_haxm_CPPFLAGS=	$(AM_CPPFLAGS) $(IA32_CPPFLAGS) $(HAXM_CPPFLAGS)
else	# HAXM
bin_PROGRAMS=	xnp21kai
nodist_man1_MANS= xnp21kai.1 xnp21kai.1j
xnp21kai_SOURCES=	$(COMMON_SOURCES) $(IA32_SOURCES)
xnp21kai_CPPFLAGS=	$(AM_CPPFLAGS) $(IA32_CPPFLAGS)
endif	# HAXM
else	# CPUCORE_IA32
bin_PROGRAMS=	xnp2kai
nodist_man1_MANS= xnp2kai.1 xnp2kai.1j
xnp2kai_SOURCES=	$(COMMON_SOURCES) $(I286C_SOURCES)
xnp2kai_CPPFLAGS=	$(AM_CPPFLAGS) $(I286C_CPPFLAGS)
endif	# CPUCORE_IA32
endif	# BUILD_ALL

CLEANFILES=	$(nodist_man1_MANS) make.log

cleandir:: distclean

.man1.1 .jman1.1j:
	@sed -e 's;@PKGDATADIR@;${DESTDIR}${pkgdatadir};' < $< > $@

cppcheck: compile_commands.json
	cppcheck --project=compile_commands.json --xml 2> cppcheck.xml

compile_commands.json:
	bear make -f Makefile

