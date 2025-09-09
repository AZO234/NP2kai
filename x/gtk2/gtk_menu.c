/*
 * Copyright (c) 2004-2013 NONAKA Kimihiro
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <compiler.h>

#include <sys/stat.h>
#include <errno.h>

#include <np2.h>
#include <dosio.h>
#include <ini.h>
#include <pccore.h>
#include <io/iocore.h>
#include <debugsub.h>

#include <sound/beep.h>
#include <fdd/diskdrv.h>
#include <font/font.h>
#include <cbus/mpu98ii.h>
#include <cbus/pc9861k.h>
#include <sound/s98.h>
#include <vram/scrnsave.h>
#if defined(SUPPORT_VIDEOFILTER)
#include <vram/videofilter.h>
#endif
#include <fdd/newdisk.h>

#include <kdispwin.h>
#include <toolwin.h>
#include <debug/viewer.h>
#include <debug/debugwin.h>
#include <skbdwin.h>

#include <mousemng.h>
#include <scrnmng.h>
#include <sysmng.h>

#if defined(SUPPORT_DEBUGSS)
#include "debugsnapshot.h"
#endif

#if defined(SUPPORT_SMPU98)
#include <cbus/smpu98.h>
#endif

#include "gtk2/xnp2.h"
#include "gtk2/gtk_menu.h"
#include "gtk2/gtk_keyboard.h"

#ifdef SUPPORT_WAB
#include <wab/wab.h>
#include <wab/wabbmpsave.h>
#endif

#ifndef	NSTATSAVE
#define	NSTATSAVE	10
#endif

#ifdef SUPPORT_NVL_IMAGES
BOOL nvl_check();
#endif

void create_hostdrv_dialog(void);
void create_sound_dialog(void);
void create_midi_dialog(void);
void create_serial_dialog(void);
void create_ide_dialog(void);
void create_network_dialog(void);
void create_about_dialog(void);
void create_screen_dialog(void);
void create_pci_dialog(void);
void create_wab_dialog(void);

/* normal */
static void cb_bmpsave(GtkAction *action, gpointer user_data);
static void cb_change_font(GtkAction *action, gpointer user_data);
static void cb_diskeject(GtkAction *action, gpointer user_data);
static void cb_diskopen(GtkAction *action, gpointer user_data);
#if defined(SUPPORT_IDEIO)
static void cb_ataopen(GtkAction *action, gpointer user_data);
static void cb_ataremove(GtkAction *action, gpointer user_data);
static void cb_atapiopen(GtkAction *action, gpointer user_data);
static void cb_atapiremove(GtkAction *action, gpointer user_data);
#endif
static void cb_midipanic(GtkAction *action, gpointer user_data);
static void cb_newfdisk(GtkAction *action, gpointer user_data);
static void cb_newhdisk(GtkAction *action, gpointer user_data);
static void cb_reset(GtkAction *action, gpointer user_data);
#if !defined(SUPPORT_IDEIO)
static void cb_sasiopen(GtkAction *action, gpointer user_data);
static void cb_sasiremove(GtkAction *action, gpointer user_data);
#endif
#if defined(SUPPORT_STATSAVE)
static void cb_statsave(GtkAction *action, gpointer user_data);
static void cb_statload(GtkAction *action, gpointer user_data);
#endif
#if defined(SUPPORT_DEBUGSS)
static void cb_dbsssave(GtkAction *action, gpointer user_data);
static void cb_dbssload(GtkAction *action, gpointer user_data);
#endif
static void cb_sndus(GtkToggleAction *action, gpointer user_data);
static void cb_sndcad(GtkToggleAction *action, gpointer user_data);

static void cb_dialog(GtkAction *action, gpointer user_data);
static void cb_radio(GtkRadioAction *action, GtkRadioAction *current, gpointer user_data);

static GtkActionEntry menu_entries[] = {
/* Menu */
{ "EmulateMenu",  NULL, "Emulate",  NULL, NULL, NULL },
{ "FDDMenu",      NULL, "FDD",      NULL, NULL, NULL },
{ "HardDiskMenu", NULL, "HardDisk", NULL, NULL, NULL },
{ "ScreenMenu",   NULL, "Screen",   NULL, NULL, NULL },
{ "DeviceMenu",   NULL, "Device",   NULL, NULL, NULL },
{ "OtherMenu",    NULL, "Other",    NULL, NULL, NULL },
#if defined(SUPPORT_STATSAVE)
{ "StatMenu",     NULL, "Stat",     NULL, NULL, NULL },
#endif
#if defined(SUPPORT_DEBUGSS)
{ "DebugSSMenu",  NULL, "DebugSS",  NULL, NULL, NULL },
#endif

/* Submenu */
{ "NewDiskMenu",  NULL, "_New disk", NULL, NULL, NULL },
{ "Drive1Menu",   NULL, "Drive_1",   NULL, NULL, NULL },
{ "Drive2Menu",   NULL, "Drive_2",   NULL, NULL, NULL },
{ "Drive3Menu",   NULL, "Drive_3",   NULL, NULL, NULL },
{ "Drive4Menu",   NULL, "Drive_4",   NULL, NULL, NULL },
#if defined(SUPPORT_IDEIO)
{ "ATA00Menu",    NULL, "IDE0-_0",   NULL, NULL, NULL },
{ "ATA01Menu",    NULL, "IDE0-_1",   NULL, NULL, NULL },
{ "ATAPIMenu",    NULL, "_CD-ROM",   NULL, NULL, NULL },
#endif
{ "KeyboardMenu", NULL, "_Keyboard", NULL, NULL, NULL },
{ "MemoryMenu",   NULL, "M_emory",   NULL, NULL, NULL },
#if !defined(SUPPORT_IDEIO)
{ "SASI1Menu",    NULL, "SASI-_1",   NULL, NULL, NULL },
{ "SASI2Menu",    NULL, "SASI-_2",   NULL, NULL, NULL },
#endif
{ "ScrnSizeMenu", NULL, "Size",      NULL, NULL, NULL },
#if defined(SUPPORT_VIDEOFILTER)
{ "VF1Menu",      NULL, "Video filter", NULL, NULL, NULL },
#endif
{ "SoundMenu",    NULL, "_Sound",    NULL, NULL, NULL },
{ "FPUMenu",      NULL, "FPU",       NULL, NULL, NULL },

/* MenuItem */
{ "about",       NULL, "_About",            NULL, NULL, G_CALLBACK(cb_dialog) },
{ "bmpsave",     NULL, "_BMP save...",      NULL, NULL, G_CALLBACK(cb_bmpsave) },
{ "calendar",    NULL, "Ca_lendar...",      NULL, NULL, G_CALLBACK(cb_dialog) },
{ "configure",   NULL, "_Configure...",     NULL, NULL, G_CALLBACK(cb_dialog) },
{ "disk1eject",  NULL, "_Eject",            NULL, NULL, G_CALLBACK(cb_diskeject), },
{ "disk1open",   NULL, "_Open...",          NULL, NULL, G_CALLBACK(cb_diskopen), },
{ "disk2eject",  NULL, "_Eject",            NULL, NULL, G_CALLBACK(cb_diskeject), },
{ "disk2open",   NULL, "_Open...",          NULL, NULL, G_CALLBACK(cb_diskopen), },
{ "disk3eject",  NULL, "_Eject",            NULL, NULL, G_CALLBACK(cb_diskeject), },
{ "disk3open",   NULL, "_Open...",          NULL, NULL, G_CALLBACK(cb_diskopen), },
{ "disk4eject",  NULL, "_Eject",            NULL, NULL, G_CALLBACK(cb_diskeject), },
{ "disk4open",   NULL, "_Open...",          NULL, NULL, G_CALLBACK(cb_diskopen), },
{ "exit",        NULL, "E_xit",             NULL, NULL, G_CALLBACK(gtk_main_quit) },
//{ "font",        NULL, "_Font...",          NULL, NULL, G_CALLBACK(cb_change_font), },
{ "newfdisk",    NULL, "_Floppy disk image...", NULL, NULL, G_CALLBACK(cb_newfdisk) },
{ "newhdisk",    NULL, "_Hard disk image...",   NULL, NULL, G_CALLBACK(cb_newhdisk) },
#if defined(SUPPORT_IDEIO)
{ "ata00open",   NULL, "_Open...",          NULL, NULL, G_CALLBACK(cb_ataopen), },
{ "ata00remove", NULL, "_Remove",           NULL, NULL, G_CALLBACK(cb_ataremove), },
{ "ata01open",   NULL, "_Open...",          NULL, NULL, G_CALLBACK(cb_ataopen), },
{ "ata01remove", NULL, "_Remove",           NULL, NULL, G_CALLBACK(cb_ataremove), },
{ "atapiopen",   NULL, "_Open...",          NULL, NULL, G_CALLBACK(cb_atapiopen), },
{ "atapiremove", NULL, "_Remove",           NULL, NULL, G_CALLBACK(cb_atapiremove), },
#endif
{ "midiopt",     NULL, "MIDI _option...",   NULL, NULL, G_CALLBACK(cb_dialog) },
{ "midipanic",   NULL, "MIDI _panic",       NULL, NULL, G_CALLBACK(cb_midipanic) },
#if !defined(SUPPORT_IDEIO)
{ "sasi1open",   NULL, "_Open...",          NULL, NULL, G_CALLBACK(cb_sasiopen), },
{ "sasi1remove", NULL, "_Remove",           NULL, NULL, G_CALLBACK(cb_sasiremove), },
{ "sasi2open",   NULL, "_Open...",          NULL, NULL, G_CALLBACK(cb_sasiopen), },
{ "sasi2remove", NULL, "_Remove",           NULL, NULL, G_CALLBACK(cb_sasiremove), },
#endif
{ "ideopt",      NULL, "_IDE option...",    NULL, NULL, G_CALLBACK(cb_dialog) },
{ "screenopt",   NULL, "Screen _option...", NULL, NULL, G_CALLBACK(cb_dialog) },
{ "serialopt",   NULL, "Se_rial option...", NULL, NULL, G_CALLBACK(cb_dialog) },
{ "soundopt",    NULL, "So_und option...",  NULL, NULL, G_CALLBACK(cb_dialog) },
#if defined(SUPPORT_NET)
{ "networkopt",  NULL, "Network option...", NULL, NULL, G_CALLBACK(cb_dialog) },
#endif	/* SUPPORT_NET */
#if defined(SUPPORT_WAB)
{ "wabopt",      NULL, "Window Accelerator option...", NULL, NULL, G_CALLBACK(cb_dialog) },
#endif	/* SUPPORT_WAB */
#if defined(SUPPORT_PCI)
{ "pciopt",      NULL, "PCI option...",     NULL, NULL, G_CALLBACK(cb_dialog) },
#endif	/* SUPPORT_PCI */
#if defined(SUPPORT_HOSTDRV)
{ "hostdrvopt",  NULL, "Hostdrv option...", NULL, NULL, G_CALLBACK(cb_dialog) },
#endif	/* SUPPORT_HOSTDRV */
{ "reset",       NULL, "_Reset",            NULL, NULL, G_CALLBACK(cb_reset) },
#if defined(SUPPORT_STATSAVE)
{ "stat00save",  NULL, "Save 0",            NULL, NULL, G_CALLBACK(cb_statsave), },
{ "stat01save",  NULL, "Save 1",            NULL, NULL, G_CALLBACK(cb_statsave), },
{ "stat02save",  NULL, "Save 2",            NULL, NULL, G_CALLBACK(cb_statsave), },
{ "stat03save",  NULL, "Save 3",            NULL, NULL, G_CALLBACK(cb_statsave), },
{ "stat04save",  NULL, "Save 4",            NULL, NULL, G_CALLBACK(cb_statsave), },
{ "stat05save",  NULL, "Save 5",            NULL, NULL, G_CALLBACK(cb_statsave), },
{ "stat06save",  NULL, "Save 6",            NULL, NULL, G_CALLBACK(cb_statsave), },
{ "stat07save",  NULL, "Save 7",            NULL, NULL, G_CALLBACK(cb_statsave), },
{ "stat08save",  NULL, "Save 8",            NULL, NULL, G_CALLBACK(cb_statsave), },
{ "stat09save",  NULL, "Save 9",            NULL, NULL, G_CALLBACK(cb_statsave), },
{ "stat00load",  NULL, "Load 0",            NULL, NULL, G_CALLBACK(cb_statload), },
{ "stat01load",  NULL, "Load 1",            NULL, NULL, G_CALLBACK(cb_statload), },
{ "stat02load",  NULL, "Load 2",            NULL, NULL, G_CALLBACK(cb_statload), },
{ "stat03load",  NULL, "Load 3",            NULL, NULL, G_CALLBACK(cb_statload), },
{ "stat04load",  NULL, "Load 4",            NULL, NULL, G_CALLBACK(cb_statload), },
{ "stat05load",  NULL, "Load 5",            NULL, NULL, G_CALLBACK(cb_statload), },
{ "stat06load",  NULL, "Load 6",            NULL, NULL, G_CALLBACK(cb_statload), },
{ "stat07load",  NULL, "Load 7",            NULL, NULL, G_CALLBACK(cb_statload), },
{ "stat08load",  NULL, "Load 8",            NULL, NULL, G_CALLBACK(cb_statload), },
{ "stat09load",  NULL, "Load 9",            NULL, NULL, G_CALLBACK(cb_statload), },
#endif
#if defined(SUPPORT_DEBUGSS)
{ "dbss00save",  NULL, "Save debugss 0",    NULL, NULL, G_CALLBACK(cb_dbsssave), },
{ "dbss01save",  NULL, "Save debugss 1",    NULL, NULL, G_CALLBACK(cb_dbsssave), },
{ "dbss02save",  NULL, "Save debugss 2",    NULL, NULL, G_CALLBACK(cb_dbsssave), },
{ "dbss03save",  NULL, "Save debugss 3",    NULL, NULL, G_CALLBACK(cb_dbsssave), },
{ "dbss00load",  NULL, "Load debugss 0",    NULL, NULL, G_CALLBACK(cb_dbssload), },
{ "dbss01load",  NULL, "Load debugss 1",    NULL, NULL, G_CALLBACK(cb_dbssload), },
{ "dbss02load",  NULL, "Load debugss 2",    NULL, NULL, G_CALLBACK(cb_dbssload), },
{ "dbss03load",  NULL, "Load debugss 3",    NULL, NULL, G_CALLBACK(cb_dbssload), },
#endif
{ "sndus",       NULL, "Send underscore",   NULL, NULL, G_CALLBACK(cb_sndus), },
{ "sndcad",      NULL, "Send Ctrl+Alt+Del", NULL, NULL, G_CALLBACK(cb_sndcad), },
};
static const guint n_menu_entries = G_N_ELEMENTS(menu_entries);

/* Toggle */
static void cb_clockdisp(GtkToggleAction *action, gpointer user_data);
static void cb_dispvsync(GtkToggleAction *action, gpointer user_data);
static void cb_framedisp(GtkToggleAction *action, gpointer user_data);
static void cb_jastsound(GtkToggleAction *action, gpointer user_data);
static void cb_joyrapid(GtkToggleAction *action, gpointer user_data);
static void cb_joyreverse(GtkToggleAction *action, gpointer user_data);
static void cb_keydisplay(GtkToggleAction *action, gpointer user_data);
static void cb_mousemode(GtkToggleAction *action, gpointer user_data);
static void cb_mouserapid(GtkToggleAction *action, gpointer user_data);
static void cb_nowait(GtkToggleAction *action, gpointer user_data);
static void cb_asynccpu(GtkToggleAction *action, gpointer user_data);
#if defined(SUPPORT_VIDEOFILTER)
static void cb_vf1en(GtkToggleAction *action, gpointer user_data);
static void cb_vf1bo(GtkToggleAction *action, gpointer user_data);
#endif
static void cb_realpalettes(GtkToggleAction *action, gpointer user_data);
static void cb_s98logging(GtkToggleAction *action, gpointer user_data);
static void cb_seeksound(GtkToggleAction *action, gpointer user_data);
static void cb_softkeyboard(GtkToggleAction *action, gpointer user_data);
static void cb_toolwindow(GtkToggleAction *action, gpointer user_data);
static void cb_xctrlkey(GtkToggleAction *action, gpointer user_data);
static void cb_xgrphkey(GtkToggleAction *action, gpointer user_data);
static void cb_xshiftkey(GtkToggleAction *action, gpointer user_data);
static void cb_xrollkey(GtkToggleAction *action, gpointer user_data);
static void cb_itfwork(GtkToggleAction *action, gpointer user_data);
static void cb_fixmmtimer(GtkToggleAction *action, gpointer user_data);
static void cb_16mbmemchk(GtkToggleAction *action, gpointer user_data);
#if defined(SUPPORT_FAST_MEMORYCHECK)
static void cb_fastmemchk(GtkToggleAction *action, gpointer user_data);
#endif
#if defined(SUPPORT_FMGEN)
static void cb_fmgen(GtkToggleAction *action, gpointer user_data);
#endif
static void cb_hf_enable(GtkToggleAction *action, gpointer user_data);
#if defined(SUPPORT_DEBUGSS)
static void cb_en_dbss(GtkToggleAction *action, gpointer user_data);
#endif

static GtkToggleActionEntry togglemenu_entries[] = {
{ "clockdisp",    NULL, "_Clock disp",        NULL, NULL, G_CALLBACK(cb_clockdisp), FALSE },
{ "dispvsync",    NULL, "_Disp Vsync",        NULL, NULL, G_CALLBACK(cb_dispvsync), FALSE },
{ "framedisp",    NULL, "_Frame disp",        NULL, NULL, G_CALLBACK(cb_framedisp), FALSE },
{ "jastsound",    NULL, "_Jast sound",        NULL, NULL, G_CALLBACK(cb_jastsound), FALSE },
{ "joyrapid",     NULL, "Joy _rapid",         NULL, NULL, G_CALLBACK(cb_joyrapid), FALSE },
{ "joyreverse",   NULL, "Joy re_verse",       NULL, NULL, G_CALLBACK(cb_joyreverse), FALSE },
{ "keydisplay",   NULL, "Key display",        NULL, NULL, G_CALLBACK(cb_keydisplay), FALSE },
{ "mousemode",    NULL, "_Mouse mode",        NULL, NULL, G_CALLBACK(cb_mousemode), FALSE },
{ "mouserapid",   NULL, "_Mouse rapid",       NULL, NULL, G_CALLBACK(cb_mouserapid), FALSE },
{ "nowait",       NULL, "_No wait",           NULL, NULL, G_CALLBACK(cb_nowait), FALSE },
#if defined(SUPPORT_ASYNC_CPU)
{ "asynccpu",     NULL, "_Async CPU(experimental)", NULL, NULL, G_CALLBACK(cb_asynccpu), FALSE },
#endif
#if defined(SUPPORT_VIDEOFILTER)
{ "vf1en",        NULL, "Enable",             NULL, NULL, G_CALLBACK(cb_vf1en), FALSE },
{ "vf1bo",        NULL, "Use BMP only",       NULL, NULL, G_CALLBACK(cb_vf1bo), FALSE },
#endif
{ "realpalettes", NULL, "Real _palettes",     NULL, NULL, G_CALLBACK(cb_realpalettes), FALSE },
{ "s98logging",   NULL, "_S98 logging",       NULL, NULL, G_CALLBACK(cb_s98logging), FALSE },
{ "seeksound",    NULL, "_Seek sound",        NULL, NULL, G_CALLBACK(cb_seeksound), FALSE },
{ "softkeyboard", NULL, "S_oftware keyboard", NULL, NULL, G_CALLBACK(cb_softkeyboard), FALSE },
{ "toolwindow",   NULL, "_Tool window",       NULL, NULL, G_CALLBACK(cb_toolwindow), FALSE },
{ "xctrlkey",     NULL, "mechanical _CTRL",   NULL, NULL, G_CALLBACK(cb_xctrlkey), FALSE },
{ "xgrphkey",     NULL, "mechanical _GRPH",   NULL, NULL, G_CALLBACK(cb_xgrphkey), FALSE },
{ "xshiftkey",    NULL, "mechanical _SHIFT",  NULL, NULL, G_CALLBACK(cb_xshiftkey), FALSE },
{ "xrollkey",     NULL, "Swap PageUp/PageDown", NULL, NULL, G_CALLBACK(cb_xrollkey), FALSE },
{ "itfwork",      NULL, "ITF work",           NULL, NULL, G_CALLBACK(cb_itfwork), FALSE },
{ "fixmmtimer",   NULL, "Fix MMTimer",        NULL, NULL, G_CALLBACK(cb_fixmmtimer), FALSE },
{ "16mbmemchk",   NULL, "Skip over 16MB memcheck", NULL, NULL, G_CALLBACK(cb_16mbmemchk), FALSE },
#if defined(SUPPORT_FAST_MEMORYCHECK)
{ "fastmemchk",   NULL, "Fast memcheck", NULL, NULL, G_CALLBACK(cb_fastmemchk), FALSE },
#endif
#if defined(SUPPORT_FMGEN)
{ "fmgen",        NULL, "fmgen",              NULL, NULL, G_CALLBACK(cb_fmgen), FALSE },
#endif
{ "hf_enable",    NULL, "Fontrom hook",       NULL, NULL, G_CALLBACK(cb_hf_enable), FALSE },
#if defined(SUPPORT_DEBUGSS)
{ "en_dbss",      NULL, "Debug snapshot",       NULL, NULL, G_CALLBACK(cb_en_dbss), FALSE },
#endif
};
static const guint n_togglemenu_entries = G_N_ELEMENTS(togglemenu_entries);

/* Radio */
static GtkRadioActionEntry framerate_entries[] = {
{ "autoframe", NULL, "_Auto frame", NULL, NULL, 0 },
{ "fullframe", NULL, "_Full frame", NULL, NULL, 1 },
{ "1/2 frame", NULL, "1/_2 frame",  NULL, NULL, 2 },
{ "1/3 frame", NULL, "1/_3 frame",  NULL, NULL, 3 },
{ "1/4 frame", NULL, "1/_4 frame",  NULL, NULL, 4 },
};
static const guint n_framerate_entries = G_N_ELEMENTS(framerate_entries);

#if defined(SUPPORT_VIDEOFILTER)
static GtkRadioActionEntry vf1p_entries[] = {
{ "vf1p0", NULL, "Profile 0", NULL, NULL, 0 },
{ "vf1p1", NULL, "Profile 1", NULL, NULL, 1 },
{ "vf1p2", NULL, "Profile 2", NULL, NULL, 2 },
};
static const guint n_vf1p_entries = G_N_ELEMENTS(vf1p_entries);
#endif

static GtkRadioActionEntry kbtype_entries[] = {
{ "kb106", NULL, "JP Keyboard 106", NULL, NULL, 0 },
{ "kb101", NULL, "US Keyboard 101", NULL, NULL, 1 },
};
static const guint n_kbtype_entries = G_N_ELEMENTS(kbtype_entries);

static GtkRadioActionEntry joykey_entries[] = {
{ "keyboard", NULL, "_Keyboard", NULL, NULL, 0 },
{ "joykey1",  NULL, "Joykey-_1", NULL, NULL, 1 },
{ "joykey2",  NULL, "Joykey-_2", NULL, NULL, 2 },
};
static const guint n_joykey_entries = G_N_ELEMENTS(joykey_entries);

static GtkRadioActionEntry f11key_entries[] = {
{ "f11none", NULL, "F11 = None",        NULL, NULL, 0 },
{ "f11menu", NULL, "F11 = Menu toggle", NULL, NULL, 1 },
{ "f11fscr", NULL, "F11 = Full Screen", NULL, NULL, 2 },
};
static const guint n_f11key_entries = G_N_ELEMENTS(f11key_entries);

static GtkRadioActionEntry f12key_entries[] = {
{ "f12mouse", NULL, "F12 = _Mouse",     NULL, NULL, 0 },
{ "f12copy",  NULL, "F12 = Co_py",      NULL, NULL, 1 },
{ "f12stop",  NULL, "F12 = S_top",      NULL, NULL, 2 },
{ "f12help",  NULL, "F12 = _Help",      NULL, NULL, 7 },
{ "f12equal", NULL, "F12 = tenkey [=]", NULL, NULL, 4 },
{ "f12comma", NULL, "F12 = tenkey [,]", NULL, NULL, 3 },
};
static const guint n_f12key_entries = G_N_ELEMENTS(f12key_entries);

static GtkRadioActionEntry beepvol_entries[] = {
{ "beepoff",  NULL, "Beep _off",  NULL, NULL, 0 },
{ "beeplow",  NULL, "Beep _low",  NULL, NULL, 1 },
{ "beepmid",  NULL, "Beep _mid",  NULL, NULL, 2 },
{ "beephigh", NULL, "Beep _high", NULL, NULL, 3 },
};
static const guint n_beepvol_entries = G_N_ELEMENTS(beepvol_entries);

static GtkRadioActionEntry soundboard_entries[] = {
{ "disableboards",         NULL, "_Disable boards",                                  NULL, NULL, SOUNDID_NONE },
{ "pc-9801-14",            NULL, "PC-9801-_14",                                      NULL, NULL, SOUNDID_PC_9801_14 },
{ "pc-9801-26k",           NULL, "PC-9801-_26K",                                     NULL, NULL, SOUNDID_PC_9801_26K },
{ "pc-9801-86",            NULL, "PC-9801-8_6",                                      NULL, NULL, SOUNDID_PC_9801_86 },
{ "pc-9801-26k-86",        NULL, "PC-9801-26_K + 86",                                NULL, NULL, SOUNDID_PC_9801_26K },
{ "pc-9801-86-cb",         NULL, "PC-9801-86 + _Chibi-oto",                          NULL, NULL, SOUNDID_PC_9801_86_ADPCM },
{ "pc-9801-118",           NULL, "PC-9801-11_8",                                     NULL, NULL, SOUNDID_PC_9801_118 },
{ "pc-9801-86-mx",         NULL, "PC-9801-86 + Mate-X PCM(B460)",                    NULL, NULL, SOUNDID_PC_9801_86_WSS },
{ "pc-9801-86-118",        NULL, "PC-9801-86 + 118(B460)",                           NULL, NULL, SOUNDID_PC_9801_86_118 },
{ "pc-9801-mx",            NULL, "Mate-X PCM",                                       NULL, NULL, SOUNDID_MATE_X_PCM },
{ "speakboard",            NULL, "S_peak board",                                     NULL, NULL, SOUNDID_SPEAKBOARD },
{ "speakboard86",          NULL, "PC-9801-86 + Speak board",                         NULL, NULL, SOUNDID_86_SPEAKBOARD },
{ "sparkboard",            NULL, "Sp_ark board",                                     NULL, NULL, SOUNDID_SPARKBOARD },
{ "sndorchestra",          NULL, "Sound Orchestra",                                  NULL, NULL, SOUNDID_SOUNDORCHESTRA },
{ "sndorchestrav",         NULL, "Sound Orchestra-V",                                NULL, NULL, SOUNDID_SOUNDORCHESTRAV },
{ "littleorchestral",      NULL, "Little Orchestra L",                               NULL, NULL, SOUNDID_LITTLEORCHESTRAL },
{ "multiorchestra",        NULL, "Multimedia Orchestra",                             NULL, NULL, SOUNDID_MMORCHESTRA },
#if defined(SUPPORT_SOUND_SB16)
{ "sb16",	                 NULL, "Sound Blaster 16",                                 NULL, NULL, SOUNDID_SB16 },
{ "pc-9801-86-sb16",       NULL, "PC-9801-86 + Sound Blaster 16",                    NULL, NULL, SOUNDID_PC_9801_86_SB16 },
{ "pc-9801-mx-sb16",       NULL, "Mate-X PCM + Sound Blaster 16",                    NULL, NULL, SOUNDID_WSS_SB16 },
{ "pc-9801-118-sb16",      NULL, "PC-9801-118 + Sound Blaster 16",                   NULL, NULL, SOUNDID_PC_9801_118_SB16 },
{ "pc-9801-86-mx-sb16",    NULL, "PC-9801-86 + Mate-X PCM(B460) + Sound Blaster 16", NULL, NULL, SOUNDID_PC_9801_86_WSS_SB16 },
{ "pc-9801-86-118-sb16",   NULL, "PC-9801-86 + 118(B460) + Sound Blaster 16",        NULL, NULL, SOUNDID_PC_9801_86_118_SB16 },
#endif
{ "amd98",                 NULL, "_AMD98",                                           NULL, NULL, SOUNDID_AMD98 },
{ "wavestar",              NULL, "_WaveStar",                                        NULL, NULL, SOUNDID_WAVESTAR },
#if defined(SUPPORT_PX)
{ "px1",                   NULL, "Otomi-chanx2",                                     NULL, NULL, SOUNDID_PX1 },
{ "px2",                   NULL, "Otomi-chanx2 + 86",                                NULL, NULL, SOUNDID_PX2 },
#endif
};
static const guint n_soundboard_entries = G_N_ELEMENTS(soundboard_entries);

static GtkRadioActionEntry memory_entries[] = {
{ "640kb",  NULL, "64_0KB",  NULL, NULL, 0 },
{ "1.6mb",  NULL, "_1.6MB",  NULL, NULL, 1 },
{ "3.6mb",  NULL, "_3.6MB",  NULL, NULL, 3 },
{ "5.6mb",  NULL, "_5.6MB",  NULL, NULL, 5 },
{ "7.6mb",  NULL, "_7.6MB",  NULL, NULL, 7 },
{ "9.6mb",  NULL, "_9.6MB",  NULL, NULL, 9 },
{ "13.6mb", NULL, "13._6MB", NULL, NULL, 13 },
{ "16.6mb", NULL, "16.6MB", NULL, NULL, 16 },
{ "32.6mb", NULL, "32.6MB", NULL, NULL, 32 },
{ "64.6mb", NULL, "64.6MB", NULL, NULL, 64 },
{ "120.6mb", NULL, "120.6MB", NULL, NULL, 120 },
{ "230.6mb", NULL, "230.6MB", NULL, NULL, 230 },
#if defined(SUPPORT_LARGE_MEMORY)
{ "512.6mb", NULL, "512.6MB", NULL, NULL, 512 },
{ "1024.6mb", NULL, "1024.6MB", NULL, NULL, 1024 },
#endif
};
static const guint n_memory_entries = G_N_ELEMENTS(memory_entries);

static GtkRadioActionEntry screenmode_entries[] = {
{ "fullscreen",  NULL, "_Full screen", NULL, NULL, SCRNMODE_FULLSCREEN },
{ "windowmode",  NULL, "_Window",      NULL, NULL, 0 },
};
static const guint n_screenmode_entries = G_N_ELEMENTS(screenmode_entries);

static GtkRadioActionEntry rotate_entries[] = {
{ "normal",      NULL, "Nor_mal",       NULL, NULL, 0 },
{ "leftrotate",  NULL, "_Left rotate",  NULL, NULL, SCRNMODE_ROTATELEFT },
{ "rightrotate", NULL, "_Right rotate", NULL, NULL, SCRNMODE_ROTATERIGHT },
};
static const guint n_rotate_entries = G_N_ELEMENTS(rotate_entries);

static GtkRadioActionEntry screensize_entries[] = {
{ "320x200",  NULL, "320x200",  NULL, NULL, 4 },
{ "480x300",  NULL, "480x300",  NULL, NULL, 6 },
{ "640x400",  NULL, "640x400",  NULL, NULL, 8 },
{ "800x500",  NULL, "800x500",  NULL, NULL, 10 },
{ "960x600",  NULL, "960x600",  NULL, NULL, 12 },
{ "1280x800", NULL, "1280x800", NULL, NULL, 16 },
};
static const guint n_screensize_entries = G_N_ELEMENTS(screensize_entries);

static GtkRadioActionEntry fpu_entries[] = {
{ "fpu80",  NULL, "80bit Extended Precision FPU (recommended)",  NULL, NULL, 0 },
{ "fpu64",  NULL, "64bit Double Precision FPU",  NULL, NULL, 1 },
{ "fpuINT64",  NULL, "64bit Double Precision FPU + INT64 Load/Save",  NULL, NULL, 2 },
};
static const guint n_fpu_entries = G_N_ELEMENTS(fpu_entries);

static void cb_beepvol(gint idx);
static void cb_kbtype(gint idx);
static void cb_f11key(gint idx);
static void cb_f12key(gint idx);
#if defined(SUPPORT_VIDEOFILTER)
static void cb_vf1p(gint idx);
#endif
static void cb_framerate(gint idx);
static void cb_joykey(gint idx);
static void cb_memory(gint idx);
static void cb_rotate(gint idx);
static void cb_screenmode(gint idx);
static void cb_screensize(gint idx);
static void cb_soundboard(gint idx);
static void cb_fpu(gint idx);

static const struct {
	GtkRadioActionEntry	*entry;
	gint			count;
	void			(*func)(gint idx);
} radiomenu_entries[] = {
	{ beepvol_entries, G_N_ELEMENTS(beepvol_entries), cb_beepvol },
	{ kbtype_entries, G_N_ELEMENTS(kbtype_entries), cb_kbtype },
	{ f11key_entries, G_N_ELEMENTS(f11key_entries), cb_f11key },
	{ f12key_entries, G_N_ELEMENTS(f12key_entries), cb_f12key },
#if defined(SUPPORT_VIDEOFILTER)
	{ vf1p_entries, G_N_ELEMENTS(vf1p_entries), cb_vf1p },
#endif
	{ framerate_entries, G_N_ELEMENTS(framerate_entries), cb_framerate },
	{ joykey_entries, G_N_ELEMENTS(joykey_entries), cb_joykey },
	{ memory_entries, G_N_ELEMENTS(memory_entries), cb_memory },
	{ rotate_entries, G_N_ELEMENTS(rotate_entries), cb_rotate },
	{ screenmode_entries, G_N_ELEMENTS(screenmode_entries), cb_screenmode },
	{ screensize_entries, G_N_ELEMENTS(screensize_entries), cb_screensize },
	{ soundboard_entries, G_N_ELEMENTS(soundboard_entries), cb_soundboard },
	{ fpu_entries, G_N_ELEMENTS(fpu_entries), cb_fpu },
};
static const guint n_radiomenu_entries = G_N_ELEMENTS(radiomenu_entries);


static const gchar *ui_info =
"<ui>\n"
" <menubar name='MainMenu'>\n"
"  <menu name='Emulate' action='EmulateMenu'>\n"
"   <menuitem action='reset'/>\n"
"   <separator/>\n"
"   <menuitem action='configure'/>\n"
"   <menu name='NewDisk' action='NewDiskMenu'>\n"
"    <menuitem action='newfdisk'/>\n"
"    <menuitem action='newhdisk'/>\n"
"   </menu>\n"
//"   <menuitem action='font'/>\n"
"   <separator/>\n"
"   <menuitem action='exit'/>\n"
"  </menu>\n"
#if defined(SUPPORT_STATSAVE)
"  <menu name='Stat' action='StatMenu'>\n"
"  </menu>\n"
#endif
"  <menu name='FDD' action='FDDMenu'>\n"
"  </menu>\n"
"  <menu name='HardDisk' action='HardDiskMenu'>\n"
#if defined(SUPPORT_IDEIO)
"   <menu name='ATA00' action='ATA00Menu'>\n"
"    <menuitem action='ata00open'/>\n"
"    <menuitem action='ata00remove'/>\n"
"   </menu>\n"
"   <menu name='ATA01' action='ATA01Menu'>\n"
"    <menuitem action='ata01open'/>\n"
"    <menuitem action='ata01remove'/>\n"
"   </menu>\n"
"   <menu name='ATAPI' action='ATAPIMenu'>\n"
"    <menuitem action='atapiopen'/>\n"
"    <menuitem action='atapiremove'/>\n"
"   </menu>\n"
"   <separator/>\n"
"   <menuitem action='ideopt'/>\n"
#else	/* !SUPPORT_IDEIO */
"   <menu name='SASI1' action='SASI1Menu'>\n"
"    <menuitem action='sasi1open'/>\n"
"    <menuitem action='sasi1remove'/>\n"
"   </menu>\n"
"   <menu name='SASI2' action='SASI2Menu'>\n"
"    <menuitem action='sasi2open'/>\n"
"    <menuitem action='sasi2remove'/>\n"
"   </menu>\n"
#endif	/* SUPPORT_IDEIO */
"  </menu>\n"
"  <menu name='Screen' action='ScreenMenu'>\n"
"   <menuitem action='fullscreen'/>\n"
"   <menuitem action='windowmode'/>\n"
"   <separator/>\n"
"   <menuitem action='normal'/>\n"
"   <menuitem action='leftrotate'/>\n"
"   <menuitem action='rightrotate'/>\n"
"   <separator/>\n"
"   <menuitem action='dispvsync'/>\n"
"   <menuitem action='realpalettes'/>\n"
"   <menuitem action='nowait'/>\n"
#if defined(SUPPORT_ASYNC_CPU)
"   <menuitem action='asynccpu'/>\n"
#endif
"   <menuitem action='autoframe'/>\n"
"   <menuitem action='fullframe'/>\n"
"   <menuitem action='1/2 frame'/>\n"
"   <menuitem action='1/3 frame'/>\n"
"   <menuitem action='1/4 frame'/>\n"
#if defined(SUPPORT_SCREENSIZE)
"   <separator/>\n"
"   <menu name='Size' action='ScrnSizeMenu'>\n"
"    <menuitem action='320x200'/>\n"
"    <menuitem action='480x300'/>\n"
"    <menuitem action='640x400'/>\n"
"    <menuitem action='800x500'/>\n"
"    <menuitem action='960x600'/>\n"
"    <menuitem action='1280x800'/>\n"
"   </menu>\n"
#endif
#if defined(SUPPORT_VIDEOFILTER)
"   <separator/>\n"
"   <menu name='Video filter' action='VF1Menu'>\n"
"    <menuitem action='vf1en'/>\n"
"    <separator/>\n"
"    <menuitem action='vf1p0'/>\n"
"    <menuitem action='vf1p1'/>\n"
"    <menuitem action='vf1p2'/>\n"
"    <separator/>\n"
"    <menuitem action='vf1bo'/>\n"
"   </menu>\n"
#endif
"   <separator/>\n"
"   <menuitem action='screenopt'/>\n"
"  </menu>\n"
"  <menu name='Device' action='DeviceMenu'>\n"
"   <menu name='Keyboard' action='KeyboardMenu'>\n"
"    <menuitem action='kb106'/>\n"
"    <menuitem action='kb101'/>\n"
"    <separator/>\n"
"    <menuitem action='keyboard'/>\n"
"    <menuitem action='joykey1'/>\n"
"    <menuitem action='joykey2'/>\n"
"    <separator/>\n"
"    <menuitem action='xshiftkey'/>\n"
"    <menuitem action='xctrlkey'/>\n"
"    <menuitem action='xgrphkey'/>\n"
"    <menuitem action='xrollkey'/>\n"
"    <separator/>\n"
"    <menuitem action='f11none'/>\n"
"    <menuitem action='f11menu'/>\n"
"    <menuitem action='f11fscr'/>\n"
"    <separator/>\n"
"    <menuitem action='f12mouse'/>\n"
"    <menuitem action='f12copy'/>\n"
"    <menuitem action='f12stop'/>\n"
"    <menuitem action='f12help'/>\n"
"    <menuitem action='f12equal'/>\n"
"    <menuitem action='f12comma'/>\n"
"    <separator/>\n"
"    <menuitem action='sndus'/>\n"
"    <menuitem action='sndcad'/>\n"
"   </menu>\n"
"   <menu name='Sound' action='SoundMenu'>\n"
"    <menuitem action='beepoff'/>\n"
"    <menuitem action='beeplow'/>\n"
"    <menuitem action='beepmid'/>\n"
"    <menuitem action='beephigh'/>\n"
"    <separator/>\n"
"    <menuitem action='disableboards'/>\n"
"    <menuitem action='pc-9801-14'/>\n"
"    <menuitem action='pc-9801-26k'/>\n"
"    <menuitem action='pc-9801-86'/>\n"
"    <menuitem action='pc-9801-26k-86'/>\n"
"    <menuitem action='pc-9801-86-cb'/>\n"
"    <menuitem action='pc-9801-118'/>\n"
"    <menuitem action='pc-9801-86-mx'/>\n"
"    <menuitem action='pc-9801-mx'/>\n"
"    <menuitem action='speakboard'/>\n"
"    <menuitem action='speakboard86'/>\n"
"    <menuitem action='sparkboard'/>\n"
"    <menuitem action='sndorchestra'/>\n"
"    <menuitem action='sndorchestrav'/>\n"
"    <menuitem action='littleorchestral'/>\n"
"    <menuitem action='multiorchestra'/>\n"
#if defined(SUPPORT_SOUND_SB16)
"    <menuitem action='sb16'/>\n"
"    <menuitem action='pc-9801-86-sb16'/>\n"
"    <menuitem action='pc-9801-mx-sb16'/>\n"
"    <menuitem action='pc-9801-118-sb16'/>\n"
"    <menuitem action='pc-9801-86-mx-sb16'/>\n"
"    <menuitem action='pc-9801-86-118-sb16'/>\n"
#endif	/* SUPPORT_SOUND_SB16 */
"    <menuitem action='amd98'/>\n"
"    <menuitem action='wavestar'/>\n"
#if defined(SUPPORT_PX)
"    <menuitem action='px1'/>\n"
"    <menuitem action='px2'/>\n"
#endif	/* SUPPORT_PX */
"    <separator/>\n"
"    <menuitem action='jastsound'/>\n"
#if defined(SUPPORT_FMGEN)
"    <separator/>\n"
"    <menuitem action='fmgen'/>\n"
#endif	/* SUPPORT_FMGEN */
"    <separator/>\n"
"    <menuitem action='seeksound'/>\n"
"   </menu>\n"
"   <menu name='Memory' action='MemoryMenu'>\n"
"    <menuitem action='640kb'/>\n"
"    <menuitem action='1.6mb'/>\n"
"    <menuitem action='3.6mb'/>\n"
"    <menuitem action='5.6mb'/>\n"
"    <menuitem action='7.6mb'/>\n"
"    <menuitem action='9.6mb'/>\n"
"    <menuitem action='13.6mb'/>\n"
#if defined(CPUCORE_IA32)
"    <menuitem action='16.6mb'/>\n"
"    <menuitem action='32.6mb'/>\n"
"    <menuitem action='64.6mb'/>\n"
"    <menuitem action='120.6mb'/>\n"
"    <menuitem action='230.6mb'/>\n"
"    <menuitem action='512.6mb'/>\n"
"    <menuitem action='1024.6mb'/>\n"
#endif
"   </menu>\n"
"   <menu name='FPU' action='FPUMenu'>\n"
"    <menuitem action='fpu80'/>\n"
"    <menuitem action='fpu64'/>\n"
"    <menuitem action='fpuINT64'/>\n"
"   </menu>\n"
"   <menuitem action='mousemode'/>\n"
"   <separator/>\n"
"   <menuitem action='serialopt'/>\n"
"   <separator/>\n"
"   <menuitem action='midiopt'/>\n"
"   <menuitem action='midipanic'/>\n"
"   <separator/>\n"
"   <menuitem action='soundopt'/>\n"
#if defined(SUPPORT_NET)
"   <separator/>\n"
"   <menuitem action='networkopt'/>\n"
#endif	/* SUPPORT_NET */
#if defined(SUPPORT_WAB)
"   <separator/>\n"
"   <menuitem action='wabopt'/>\n"
#endif	/* SUPPORT_WAB */
#if defined(SUPPORT_PCI)
"   <separator/>\n"
"   <menuitem action='pciopt'/>\n"
#endif	/* SUPPORT_PCI */
#if defined(SUPPORT_HOSTDRV)
"   <separator/>\n"
"   <menuitem action='hostdrvopt'/>\n"
#endif	/* SUPPORT_HOSTDRV */
"  </menu>\n"
"  <menu name='Other' action='OtherMenu'>\n"
"   <menuitem action='bmpsave'/>\n"
"   <menuitem action='hf_enable'/>\n"
"   <menuitem action='s98logging'/>\n"
"   <menuitem action='calendar'/>\n"
"   <menuitem action='clockdisp'/>\n"
"   <menuitem action='framedisp'/>\n"
"   <menuitem action='joyreverse'/>\n"
"   <menuitem action='joyrapid'/>\n"
"   <menuitem action='mouserapid'/>\n"
"   <menuitem action='itfwork'/>\n"
//"   <menuitem action='fixmmtimer'/>\n"
//"   <menuitem action='16mbmemchk'/>\n"
#if defined(SUPPORT_FAST_MEMORYCHECK)
"   <menuitem action='fastmemchk'/>\n"
#endif
#if defined(SUPPORT_DEBUGSS)
"   <menuitem action='en_dbss'/>\n"
#endif
"   <separator/>\n"
"   <menuitem action='toolwindow'/>\n"
"   <menuitem action='keydisplay'/>\n"
"   <menuitem action='softkeyboard'/>\n"
"   <separator/>\n"
"   <menuitem action='about'/>\n"
"  </menu>\n"
#if defined(SUPPORT_DEBUGSS)
"  <menu name='DebugSS' action='DebugSSMenu'>\n"
"   <menuitem action='dbss00save'/>\n"
"   <menuitem action='dbss01save'/>\n"
"   <menuitem action='dbss02save'/>\n"
"   <menuitem action='dbss03save'/>\n"
"   <menuitem action='dbss00load'/>\n"
"   <menuitem action='dbss01load'/>\n"
"   <menuitem action='dbss02load'/>\n"
"   <menuitem action='dbss03load'/>\n"
"  </menu>\n"
#endif
" </menubar>\n"
"</ui>\n";

static _MENU_HDL menu_hdl;


/*
 * Menu utilities
 */
void
xmenu_toggle_item(MENU_HDL hdl, const char *name, BOOL onoff)
{
	GtkAction *action;
	gboolean b, f;

	if (hdl == NULL)
		hdl = &menu_hdl;

	action = gtk_action_group_get_action(hdl->action_group, name);
	if (action != NULL) {
		b = gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action));
		f = (b ? 1 : 0) ^ (onoff ? 1 : 0);
		if (f) {
			gtk_action_activate(action);
		}
	}
}

xmenu_visible_item(MENU_HDL hdl, const char *name, BOOL onoff)
{
	GtkAction *action;
	gboolean b, f;

	if (hdl == NULL)
		hdl = &menu_hdl;

	action = gtk_action_group_get_action(hdl->action_group, name);
	if (action != NULL) {
		b = gtk_action_get_visible(action);
		f = (b ? 1 : 0) ^ (onoff ? 1 : 0);
		if (f) {
			gtk_action_set_visible(action, onoff);
		}
	}
}

static void
xmenu_select_item_by_index(MENU_HDL hdl, GtkRadioActionEntry *entry, guint nentry, int newvalue)
{
	GtkAction *action;
	gint value;
	guint idx;

	if (hdl == NULL)
		hdl = &menu_hdl;

	for (idx = 0; idx < nentry; idx++) {
		if (entry[idx].value == newvalue)
			break;
	}
	if (idx == nentry)
		return;

	action = gtk_action_group_get_action(hdl->action_group, entry[idx].name);
	if (action != NULL) {
		value = (guint)gtk_radio_action_get_current_value(GTK_RADIO_ACTION(action));
		if (value != newvalue) {
			gtk_action_activate(action);
		}
	}
}

#define	xmenu_select_beepvol(v) \
	xmenu_select_item_by_index(NULL, beepvol_entries, n_beepvol_entries, v);
#define	xmenu_select_kbtype(v) \
	xmenu_select_item_by_index(NULL, kbtype_entries, n_kbtype_entries, v);
#define	xmenu_select_f11key(v) \
	xmenu_select_item_by_index(NULL, f11key_entries, n_f11key_entries, v);
#define	xmenu_select_f12key(v) \
	xmenu_select_item_by_index(NULL, f12key_entries, n_f12key_entries, v);
#if defined(SUPPORT_VIDEOFILTER)
#define	xmenu_select_vf1p(v) \
	xmenu_select_item_by_index(NULL, vf1p_entries, n_vf1p_entries, v);
#endif
#define	xmenu_select_framerate(v) \
	xmenu_select_item_by_index(NULL, framerate_entries, n_framerate_entries, v);
#define	xmenu_select_joykey(v) \
	xmenu_select_item_by_index(NULL, joykey_entries, n_joykey_entries, v);
#define	xmenu_select_memory(v) \
	xmenu_select_item_by_index(NULL, memory_entries, n_memory_entries, v);
#define	xmenu_select_rotate(v) \
	xmenu_select_item_by_index(NULL, rotate_entries, n_rotate_entries, v);
#define	xmenu_select_screenmode(v) \
	xmenu_select_item_by_index(NULL, screenmode_entries, n_screenmode_entries, v);
#define	xmenu_select_screensize(v) \
	xmenu_select_item_by_index(NULL, screensize_entries, n_screensize_entries, v);
#define	xmenu_select_soundboard(v) \
	xmenu_select_item_by_index(NULL, soundboard_entries, n_soundboard_entries, v);
#define	xmenu_select_fpu(v) \
	xmenu_select_item_by_index(NULL, fpu_entries, n_fpu_entries, v);


/*
 * Menu actions
 */
extern NP2CFG np2cfg_default;
extern NP2OSCFG np2oscfg_default;

static void
cb_bmpsave(GtkAction *action, gpointer user_data)
{
	GtkWidget *dialog = NULL;
	GtkFileFilter *filter;
	gchar *utf8, *path;
	SCRNSAVE bmp = NULL;

	uninstall_idle_process();

	bmp = scrnsave_create();
	if (bmp == NULL)
		goto end;

	dialog = gtk_file_chooser_dialog_new("Save as bitmap file",
	    GTK_WINDOW(main_window), GTK_FILE_CHOOSER_ACTION_SAVE, 
	    GTK_STOCK_SAVE, GTK_RESPONSE_OK,
	    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
	    NULL);
	if (dialog == NULL)
		goto end;

	gtk_file_chooser_set_show_hidden(GTK_FILE_CHOOSER(dialog), TRUE);
#if GTK_MAJOR_VERSION > 2 || (GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION >= 8)
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog),
	    TRUE);
#endif
	if (strlen(bmpfilefolder) == 0) {
		g_strlcpy(bmpfilefolder, modulefile, sizeof(bmpfilefolder));
		file_cutname(bmpfilefolder);
	}
	utf8 = g_filename_to_utf8(bmpfilefolder, -1, NULL, NULL, NULL);
	if (utf8) {
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), utf8);
		g_free(utf8);
	}

	filter = gtk_file_filter_new();
	if (filter) {
		gtk_file_filter_set_name(filter, "BMP Files");
		gtk_file_filter_add_pattern(filter, "*.[bB][mM][pP]");
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	}
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);
	filter = gtk_file_filter_new();
	if (filter) {
		gtk_file_filter_set_name(filter, "All files");
		gtk_file_filter_add_pattern(filter, "*");
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	}

	if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_OK)
		goto end;

	utf8 = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
	if (utf8) {
		path = g_filename_from_utf8(utf8, -1, NULL, NULL, NULL);
		if (path) {
			gchar *ext = file_getext(path);
			if (strlen(ext) != 3 || file_cmpname(ext, "bmp")) {
				gchar *tmp = g_strjoin(".", path, "bmp", NULL);
				g_free(path);
				path = tmp;
			}
			file_cpyname(bmpfilefolder, path, sizeof(bmpfilefolder));
			sysmng_update(SYS_UPDATEOSCFG);
#ifdef SUPPORT_WAB
			if(np2wab.relay){
				np2wab_writebmp(path);
			}else{
#endif
			scrnsave_writebmp(bmp, path, SCRNSAVE_AUTO);
#ifdef SUPPORT_WAB
			}
#endif
			g_free(path);
		}
		g_free(utf8);
	}

end:
	scrnsave_destroy(bmp);
	if (dialog)
		gtk_widget_destroy(dialog);
	install_idle_process();
}

static void
cb_change_font(GtkAction *action, gpointer user_data)
{
	GtkWidget *dialog = NULL;
	GtkFileFilter *filter;
	gchar *utf8, *path;
	struct stat sb;

	uninstall_idle_process();

	dialog = gtk_file_chooser_dialog_new("Open a font file",
	    GTK_WINDOW(main_window), GTK_FILE_CHOOSER_ACTION_OPEN, 
	    GTK_STOCK_OPEN, GTK_RESPONSE_OK,
	    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
	    NULL);
	if (dialog == NULL)
		goto end;

	gtk_file_chooser_set_show_hidden(GTK_FILE_CHOOSER(dialog), TRUE);
	utf8 = g_filename_to_utf8(np2cfg.fontfile, -1, NULL, NULL, NULL);
	if (utf8) {
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), utf8);
		g_free(utf8);
	}

	filter = gtk_file_filter_new();
	if (filter) {
		gtk_file_filter_set_name(filter, "Font bitmap");
		gtk_file_filter_add_pattern(filter, "*.[bB][mM][pP]");
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	}
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);
	filter = gtk_file_filter_new();
	if (filter) {
		gtk_file_filter_set_name(filter, "All files");
		gtk_file_filter_add_pattern(filter, "*");
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	}

	if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_OK)
		goto end;

	utf8 = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
	if (utf8) {
		path = g_filename_from_utf8(utf8, -1, NULL, NULL, NULL);
		if (path) {
			if ((stat(path, &sb) == 0) && S_ISREG(sb.st_mode) && (sb.st_mode & S_IRUSR)) {
				if (font_load(path, FALSE)) {
					gdcs.textdisp |= GDCSCRN_ALLDRAW2;
					file_cpyname(np2cfg.fontfile, path, sizeof(np2cfg.fontfile));
					sysmng_update(SYS_UPDATECFG);
				}
			}
			g_free(path);
		}
		g_free(utf8);
	}

end:
	if (dialog)
		gtk_widget_destroy(dialog);
	install_idle_process();
}

static void
cb_diskeject(GtkAction *action, gpointer user_data)
{
	const gchar *name = gtk_action_get_name(action);
	guint drive;

	/* name = "disk?eject" */
	if ((strlen(name) >= 5) && (g_ascii_isdigit(name[4]))) {
		drive = g_ascii_digit_value(name[4]) - 1;
		if (drive < 4) {
			diskdrv_setfdd(drive, NULL, FALSE);
			toolwin_setfdd(drive, NULL);
		}
	}
}

static void
cb_diskopen(GtkAction *action, gpointer user_data)
{
	GtkWidget *dialog = NULL;
	GtkFileFilter *filter;
	gchar *utf8, *path;
	struct stat sb;
	const gchar *name = gtk_action_get_name(action);
	guint drive;

	if ((strlen(name) < 5) || (!g_ascii_isdigit(name[4])))
		return;
	drive = g_ascii_digit_value(name[4]) - 1;

	uninstall_idle_process();

	dialog = gtk_file_chooser_dialog_new("Open a floppy disk image",
	    GTK_WINDOW(main_window), GTK_FILE_CHOOSER_ACTION_OPEN, 
	    GTK_STOCK_OPEN, GTK_RESPONSE_OK,
	    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
	    NULL);
	if (dialog == NULL)
		goto end;

	gtk_file_chooser_set_show_hidden(GTK_FILE_CHOOSER(dialog), TRUE);
	utf8 = g_filename_to_utf8(fddfolder, -1, NULL, NULL, NULL);
	if (utf8) {
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), utf8);
		g_free(utf8);
	}

	filter = gtk_file_filter_new();
	if (filter) {
		gtk_file_filter_set_name(filter, "D88 floppy image files");
		gtk_file_filter_add_pattern(filter, "*.[dD]88");
		gtk_file_filter_add_pattern(filter, "*.88[dD]");
		gtk_file_filter_add_pattern(filter, "*.[dD]98");
		gtk_file_filter_add_pattern(filter, "*.98[dD]");
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	}
	filter = gtk_file_filter_new();
	if (filter) {
		gtk_file_filter_set_name(filter, "Floppy disk image files");
		gtk_file_filter_add_pattern(filter, "*.[xX][dD][fF]");
		gtk_file_filter_add_pattern(filter, "*.[hH][dD][mM]");
		gtk_file_filter_add_pattern(filter, "*.[tT][fF][dD]");
		gtk_file_filter_add_pattern(filter, "*.[dD][uU][pP]");
		gtk_file_filter_add_pattern(filter, "*.2[hH][dD]");
		gtk_file_filter_add_pattern(filter, "*.[fF][dD][iI]");
		gtk_file_filter_add_pattern(filter, "*.[fF][sS]");
		gtk_file_filter_add_pattern(filter, "*.[fF][lL][pP]");
		gtk_file_filter_add_pattern(filter, "*.[nN][fF][dD]");
		gtk_file_filter_add_pattern(filter, "*.[hH][dD]4");
		gtk_file_filter_add_pattern(filter, "*.[hH][dD]5");
		gtk_file_filter_add_pattern(filter, "*.[hH][dD]9");
		gtk_file_filter_add_pattern(filter, "*.[fF][fF][dD]");
		gtk_file_filter_add_pattern(filter, "*.[hH]01");
		gtk_file_filter_add_pattern(filter, "*.[nN][dD][bB]");
		gtk_file_filter_add_pattern(filter, "*.[dD][dD][bB]");
		gtk_file_filter_add_pattern(filter, "*.[dD][dD]6");
		gtk_file_filter_add_pattern(filter, "*.[dD][cC][pP]");
		gtk_file_filter_add_pattern(filter, "*.[dD][cC][uU]");
		gtk_file_filter_add_pattern(filter, "*.[fF][lL][pP]");
		gtk_file_filter_add_pattern(filter, "*.[bB][iI][nN]");
		gtk_file_filter_add_pattern(filter, "*.[fF][iI][mM]");
		gtk_file_filter_add_pattern(filter, "*.[iI][mM][gG]");
		gtk_file_filter_add_pattern(filter, "*.[iI][mM][aA]");
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	}
	filter = gtk_file_filter_new();
	if (filter) {
		gtk_file_filter_set_name(filter, "All supported files");
		gtk_file_filter_add_pattern(filter, "*.[dD]88");
		gtk_file_filter_add_pattern(filter, "*.88[dD]");
		gtk_file_filter_add_pattern(filter, "*.[dD]98");
		gtk_file_filter_add_pattern(filter, "*.98[dD]");
		gtk_file_filter_add_pattern(filter, "*.[xX][dD][fF]");
		gtk_file_filter_add_pattern(filter, "*.[hH][dD][mM]");
		gtk_file_filter_add_pattern(filter, "*.[tT][fF][dD]");
		gtk_file_filter_add_pattern(filter, "*.[dD][uU][pP]");
		gtk_file_filter_add_pattern(filter, "*.2[hH][dD]");
		gtk_file_filter_add_pattern(filter, "*.[fF][dD][iI]");
		gtk_file_filter_add_pattern(filter, "*.[fF][sS]");
		gtk_file_filter_add_pattern(filter, "*.[fF][lL][pP]");
		gtk_file_filter_add_pattern(filter, "*.[nN][fF][dD]");
		gtk_file_filter_add_pattern(filter, "*.[hH][dD]4");
		gtk_file_filter_add_pattern(filter, "*.[hH][dD]5");
		gtk_file_filter_add_pattern(filter, "*.[hH][dD]9");
		gtk_file_filter_add_pattern(filter, "*.[fF][fF][dD]");
		gtk_file_filter_add_pattern(filter, "*.[hH]01");
		gtk_file_filter_add_pattern(filter, "*.[nN][dD][bB]");
		gtk_file_filter_add_pattern(filter, "*.[dD][dD][bB]");
		gtk_file_filter_add_pattern(filter, "*.[dD][dD]6");
		gtk_file_filter_add_pattern(filter, "*.[dD][cC][pP]");
		gtk_file_filter_add_pattern(filter, "*.[dD][cC][uU]");
		gtk_file_filter_add_pattern(filter, "*.[fF][lL][pP]");
		gtk_file_filter_add_pattern(filter, "*.[bB][iI][nN]");
		gtk_file_filter_add_pattern(filter, "*.[fF][iI][mM]");
		gtk_file_filter_add_pattern(filter, "*.[iI][mM][gG]");
		gtk_file_filter_add_pattern(filter, "*.[iI][mM][aA]");
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	}
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);
	filter = gtk_file_filter_new();
	if (filter) {
		gtk_file_filter_set_name(filter, "All files");
		gtk_file_filter_add_pattern(filter, "*");
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	}

	if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_OK)
		goto end;

	utf8 = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
	if (utf8) {
		path = g_filename_from_utf8(utf8, -1, NULL, NULL, NULL);
		if (path) {
			if ((stat(path, &sb) == 0) && S_ISREG(sb.st_mode) && (sb.st_mode & S_IRUSR)) {
				file_cpyname(fddfolder, path, sizeof(fddfolder));
				diskdrv_setfdd(drive, path, !(sb.st_mode & S_IWUSR));
				sysmng_update(SYS_UPDATEOSCFG);
			}
			g_free(path);
		}
		g_free(utf8);
	}

end:
	if (dialog)
		gtk_widget_destroy(dialog);
	install_idle_process();
}

#if defined(SUPPORT_IDEIO)
static void
cb_ataopen(GtkAction *action, gpointer user_data)
{
	GtkWidget *dialog = NULL;
	GtkFileFilter *filter;
	gchar *utf8, *path;
	struct stat sb;
	const gchar *name = gtk_action_get_name(action);
	guint channel, drive;

	/* "ata??open" */
	if ((strlen(name) < 5)
	 || (!g_ascii_isdigit(name[3]))
	 || (!g_ascii_isdigit(name[4]))) {
		return;
	}

	channel = g_ascii_digit_value(name[3]);
	drive = g_ascii_digit_value(name[4]);
	if (channel != 0 || drive >= 2)
		return;

	uninstall_idle_process();

	dialog = gtk_file_chooser_dialog_new("Open a IDE disk image",
	    GTK_WINDOW(main_window), GTK_FILE_CHOOSER_ACTION_OPEN, 
	    GTK_STOCK_OPEN, GTK_RESPONSE_OK,
	    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
	    NULL);
	if (dialog == NULL)
		goto end;

	gtk_file_chooser_set_show_hidden(GTK_FILE_CHOOSER(dialog), TRUE);
	utf8 = g_filename_to_utf8(hddfolder, -1, NULL, NULL, NULL);
	if (utf8) {
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), utf8);
		g_free(utf8);
	}

	filter = gtk_file_filter_new();
	if (filter) {
		gtk_file_filter_set_name(filter, "All files");
		gtk_file_filter_add_pattern(filter, "*");
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	}
	filter = gtk_file_filter_new();
	if (filter) {
		gtk_file_filter_set_name(filter, "IDE disk image files");
		gtk_file_filter_add_pattern(filter, "*.[tT][hH][dD]");
		gtk_file_filter_add_pattern(filter, "*.[hH][dD][iI]");
		gtk_file_filter_add_pattern(filter, "*.[nN][hH][dD]");
		gtk_file_filter_add_pattern(filter, "*.[vV][hH][dD]");
		gtk_file_filter_add_pattern(filter, "*.[sS][lL][hH]");
#ifdef SUPPORT_NVL_IMAGES
		if(nvl_check()) {
			gtk_file_filter_add_pattern(filter, "*.[vV][mM][dD][kK]");
			gtk_file_filter_add_pattern(filter, "*.[dD][sS][kK]");
			gtk_file_filter_add_pattern(filter, "*.[vV][mM][dD][xX]");
			gtk_file_filter_add_pattern(filter, "*.[vV][dD][iI]");
			gtk_file_filter_add_pattern(filter, "*.[qQ][cC][oO][wW]");
			gtk_file_filter_add_pattern(filter, "*.[qQ][cC][oO][wW]2");
			gtk_file_filter_add_pattern(filter, "*.[hH][dD][dD]");
		}
#endif	/* SUPPORT_NVL_IMAGES */
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	}
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_OK)
		goto end;

	utf8 = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
	if (utf8) {
		path = g_filename_from_utf8(utf8, -1, NULL, NULL, NULL);
		if (path) {
			if ((stat(path, &sb) == 0) && S_ISREG(sb.st_mode) && (sb.st_mode & S_IRUSR)) {
				file_cpyname(hddfolder, path, sizeof(hddfolder));
				diskdrv_setsxsi(2 * channel + drive, path);
				sysmng_update(SYS_UPDATEOSCFG);
			}
			g_free(path);
		}
		g_free(utf8);
	}

end:
	if (dialog)
		gtk_widget_destroy(dialog);
	install_idle_process();
}

static void
cb_ataremove(GtkAction *action, gpointer user_data)
{
	const gchar *name = gtk_action_get_name(GTK_ACTION(action));
	guint channel, drive;

	/* "ata??open" */
	if ((strlen(name) < 5)
	 || (!g_ascii_isdigit(name[3]))
	 || (!g_ascii_isdigit(name[4]))) {
		return;
	}

	channel = g_ascii_digit_value(name[3]);
	drive = g_ascii_digit_value(name[4]);
	if (channel == 0 && drive < 2) {
		if (2 * channel + drive < 4) {
			diskdrv_setsxsi(2 * channel + drive, NULL);
		}
	}
}

static void
cb_atapiopen(GtkAction *action, gpointer user_data)
{
	GtkWidget *dialog = NULL;
	GtkFileFilter *filter;
	gchar *utf8, *path;
	struct stat sb;

	uninstall_idle_process();

	dialog = gtk_file_chooser_dialog_new("Open a ATAPI CD-ROM image",
	    GTK_WINDOW(main_window), GTK_FILE_CHOOSER_ACTION_OPEN, 
	    GTK_STOCK_OPEN, GTK_RESPONSE_OK,
	    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
	    NULL);
	if (dialog == NULL)
		goto end;

	gtk_file_chooser_set_show_hidden(GTK_FILE_CHOOSER(dialog), TRUE);
	utf8 = g_filename_to_utf8(hddfolder, -1, NULL, NULL, NULL);
	if (utf8) {
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), utf8);
		g_free(utf8);
	}

	filter = gtk_file_filter_new();
	if (filter) {
		gtk_file_filter_set_name(filter, "All files");
		gtk_file_filter_add_pattern(filter, "*");
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	}
	filter = gtk_file_filter_new();
	if (filter) {
		gtk_file_filter_set_name(filter, "CD-ROM image files");
		gtk_file_filter_add_pattern(filter, "*.[iI][sS][oO]");
		gtk_file_filter_add_pattern(filter, "*.[cC][uU][eE]");
		gtk_file_filter_add_pattern(filter, "*.[cC][cC][dD]");
		gtk_file_filter_add_pattern(filter, "*.[cC][dD][mM]");
		gtk_file_filter_add_pattern(filter, "*.[mM][dD][sS]");
		gtk_file_filter_add_pattern(filter, "*.[nN][rR][gG]");
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	}
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_OK)
		goto end;

	utf8 = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
	if (utf8) {
		path = g_filename_from_utf8(utf8, -1, NULL, NULL, NULL);
		if (path) {
			if ((stat(path, &sb) == 0) && S_ISREG(sb.st_mode) && (sb.st_mode & S_IRUSR)) {
				file_cpyname(hddfolder, path, sizeof(hddfolder));
				diskdrv_setsxsi(0x02, path);
				sysmng_update(SYS_UPDATEOSCFG);
			}
			g_free(path);
		}
		g_free(utf8);
	}

end:
	if (dialog)
		gtk_widget_destroy(dialog);
	install_idle_process();
}

static void
cb_atapiremove(GtkAction *action, gpointer user_data)
{

	sxsi_devclose(0x02);
}
#endif	/* SUPPORT_IDEIO */

static void
cb_midipanic(GtkAction *action, gpointer user_data)
{

	rs232c_midipanic();
	mpu98ii_midipanic();
#if defined(SUPPORT_SMPU98)
	smpu98_midipanic();
#endif
	pc9861k_midipanic();
}

static void
cb_newfdisk(GtkAction *action, gpointer user_data)
{
	static const struct {
		const char *name;
		int         kind;
	} exttbl[] = {
		{ "d88", 0 },
		{ "88d", 0 },
		{ "d98", 0 },
		{ "98d", 0 },
		{ "hdm", 1 },
		{ "hd4", 2 },
	};
	static const char *extname[3] = { "d88", "hdm", "hd4" };
	GtkWidget *dialog = NULL;
	GtkFileFilter *f, *filter[3];
	gchar *utf8, *path, *tmp;
	const char *ext;
	int kind;
	int i;

	uninstall_idle_process();

	dialog = gtk_file_chooser_dialog_new("Create new floppy disk image file",
	    GTK_WINDOW(main_window), GTK_FILE_CHOOSER_ACTION_SAVE,
	    GTK_STOCK_SAVE, GTK_RESPONSE_OK,
	    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
	    NULL);
	if (dialog == NULL)
		goto end;

	gtk_file_chooser_set_show_hidden(GTK_FILE_CHOOSER(dialog), TRUE);
#if GTK_MAJOR_VERSION > 2 || (GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION >= 8)
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog),
	    TRUE);
#endif
	if (strlen(fddfolder) == 0) {
		g_strlcpy(fddfolder, modulefile, sizeof(fddfolder));
		file_cutname(fddfolder);
	}
	utf8 = g_filename_to_utf8(fddfolder, -1, NULL, NULL, NULL);
	if (utf8) {
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), utf8);
		g_free(utf8);
	}
	utf8 = g_filename_to_utf8("newdisk", -1, NULL, NULL, NULL);
	if (utf8) {
		gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), utf8);
		g_free(utf8);
	}

	filter[0] = gtk_file_filter_new();
	if (filter[0]) {
		gtk_file_filter_set_name(filter[0], "D88 image files (*.d88,*.d98,*.88d,*.98d)");
		gtk_file_filter_add_pattern(filter[0], "*.[dD]88");
		gtk_file_filter_add_pattern(filter[0], "*.88[dD]");
		gtk_file_filter_add_pattern(filter[0], "*.[dD]98");
		gtk_file_filter_add_pattern(filter[0], "*.98[dD]");
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter[0]);
	}
	filter[1] = gtk_file_filter_new();
	if (filter[1]) {
		gtk_file_filter_set_name(filter[1], "1.25MB raw image file (*.hdm)");
		gtk_file_filter_add_pattern(filter[1], "*.[hH][dD][mM]");
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter[1]);
	}
	filter[2] = gtk_file_filter_new();
	if (filter[2]) {
		gtk_file_filter_set_name(filter[2], "1.44MB raw image file (*.hd4)");
		gtk_file_filter_add_pattern(filter[2], "*.[hH][dD]4");
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter[2]);
	}
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter[0]);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_OK)
		goto end;

	utf8 = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
	if (utf8 == NULL)
		goto end;

	path = g_filename_from_utf8(utf8, -1, NULL, NULL, NULL);
	g_free(utf8);
	if (path == NULL)
		goto end;

	kind = -1;
	ext = file_getext(path);
	for (i = 0; i < NELEMENTS(exttbl); i++) {
		if (g_ascii_strcasecmp(ext, exttbl[i].name) == 0) {
			kind = exttbl[i].kind;
			break;
		}
	}
	if (i == NELEMENTS(exttbl)) {
		f = gtk_file_chooser_get_filter(GTK_FILE_CHOOSER(dialog));
		for (i = 0; i < NELEMENTS(filter); i++) {
			if (f == filter[i]) {
				kind = i;
				tmp = g_strjoin(".", path, extname[i], NULL);
				if (tmp) {
					g_free(path);
					path = tmp;
				}
				break;
			}
		}
	}

	/* XXX system has only one modal dialog? */
	gtk_widget_destroy(dialog);

	switch (kind) {
	case 0: /* D88 */
		create_newdisk_fd_dialog(path);
		break;

	case 1: /* HDM */
		newdisk_123mb_fdd(path);
		break;

	case 2: /* HD4 */
		newdisk_144mb_fdd(path);
		break;

	default:
		break;
	}
	g_free(path);

	install_idle_process();
	return;

end:
	if (dialog)
		gtk_widget_destroy(dialog);
	install_idle_process();
}

static void
cb_newhdisk(GtkAction *action, gpointer user_data)
{
	static const struct {
		const char *name;
		int         kind;
	} exttbl[] = {
		{ "nhd", 0 },
		{ "vhd", 1 },
		{ "hdi", 2 },
		{ "thd", 3 },
		{ "hdd", 4 },
#if defined(SUPPORT_SCSI)
		{ "hdn", 5 },
#endif	// defined(SUPPORT_SCSI)
	};
#if defined(SUPPORT_SCSI)
	static const char *extname[6] = { "nhd", "vhd", "hdi", "thd", "hdd", "hdn" };
	GtkFileFilter *f, *filter[6];
#else	// defined(SUPPORT_SCSI)
	static const char *extname[5] = { "nhd", "vhd", "hdi", "thd", "hdd" };
	GtkFileFilter *f, *filter[5];
#endif	// defined(SUPPORT_SCSI)
	GtkWidget *dialog = NULL;
	gchar *utf8, *path, *tmp;
	const char *ext;
	int kind;
	int i;

	uninstall_idle_process();

	dialog = gtk_file_chooser_dialog_new("Create new hard disk image file",
	    GTK_WINDOW(main_window), GTK_FILE_CHOOSER_ACTION_SAVE,
	    GTK_STOCK_SAVE, GTK_RESPONSE_OK,
	    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
	    NULL);
	if (dialog == NULL)
		goto end;

	gtk_file_chooser_set_show_hidden(GTK_FILE_CHOOSER(dialog), TRUE);
#if GTK_MAJOR_VERSION > 2 || (GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION >= 8)
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog),
	    TRUE);
#endif
	if (strlen(fddfolder) == 0) {
		g_strlcpy(fddfolder, modulefile, sizeof(fddfolder));
		file_cutname(fddfolder);
	}
	utf8 = g_filename_to_utf8(fddfolder, -1, NULL, NULL, NULL);
	if (utf8) {
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), utf8);
		g_free(utf8);
	}
	utf8 = g_filename_to_utf8("newdisk", -1, NULL, NULL, NULL);
	if (utf8) {
		gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), utf8);
		g_free(utf8);
	}

	filter[0] = gtk_file_filter_new();
	if (filter[0]) {
		gtk_file_filter_set_name(filter[0], "T98-Next harddisk image files (*.nhd)");
		gtk_file_filter_add_pattern(filter[0], "*.[nN][hH][dD]");
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter[0]);
	}
	filter[1] = gtk_file_filter_new();
	if (filter[1]) {
		gtk_file_filter_set_name(filter[1], "VirtualPC harddisk image files (*.vhd)");
		gtk_file_filter_add_pattern(filter[1], "*.[vV][hH][dD]");
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter[1]);
	}
	filter[2] = gtk_file_filter_new();
	if (filter[2]) {
		gtk_file_filter_set_name(filter[2], "Anex86 harddisk image files (*.hdi)");
		gtk_file_filter_add_pattern(filter[2], "*.[hH][dD][iI]");
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter[2]);
	}
	filter[3] = gtk_file_filter_new();
	if (filter[3]) {
		gtk_file_filter_set_name(filter[3], "T98 harddisk image files (*.thd)");
		gtk_file_filter_add_pattern(filter[3], "*.[tT][hH][dD]");
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter[3]);
	}
	filter[4] = gtk_file_filter_new();
	if (filter[4]) {
		gtk_file_filter_set_name(filter[4], "Virtual98 harddisk image files (*.hdd)");
		gtk_file_filter_add_pattern(filter[4], "*.[hH][hH][dD]");
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter[4]);
	}
#if defined(SUPPORT_SCSI)
	filter[5] = gtk_file_filter_new();
	if (filter[5]) {
		gtk_file_filter_set_name(filter[5], "RaSCSI harddisk image files (*.hdn)");
		gtk_file_filter_add_pattern(filter[5], "*.[hH][dD][nN]");
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter[5]);
	}
#endif	// defined(SUPPORT_SCSI)
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter[0]);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_OK)
		goto end;

	utf8 = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
	if (utf8 == NULL)
		goto end;

	path = g_filename_from_utf8(utf8, -1, NULL, NULL, NULL);
	g_free(utf8);
	if (path == NULL)
		goto end;

	kind = -1;
	ext = file_getext(path);
	for (i = 0; i < NELEMENTS(exttbl); i++) {
		if (g_ascii_strcasecmp(ext, exttbl[i].name) == 0) {
			kind = exttbl[i].kind;
			break;
		}
	}
	if (i == NELEMENTS(exttbl)) {
		f = gtk_file_chooser_get_filter(GTK_FILE_CHOOSER(dialog));
		for (i = 0; i < NELEMENTS(filter); i++) {
			if (f == filter[i]) {
				kind = i;
				tmp = g_strjoin(".", path, extname[i], NULL);
				if (tmp) {
					g_free(path);
					path = tmp;
				}
				break;
			}
		}
	}

	/* XXX system has only one modal dialog? */
	gtk_widget_destroy(dialog);

	create_newdisk_hd_dialog(path, kind);
	g_free(path);

	install_idle_process();
	return;

end:
	if (dialog)
		gtk_widget_destroy(dialog);
	install_idle_process();
}

static void
cb_reset(GtkAction *action, gpointer user_data)
{

	pccore_cfgupdate();
	if(nevent_iswork(NEVENT_CDWAIT)){
		nevent_forceexecute(NEVENT_CDWAIT);
	}
	pccore_reset();
}

#if !defined(SUPPORT_IDEIO)
static void
cb_sasiopen(GtkAction *action, gpointer user_data)
{
	GtkWidget *dialog = NULL;
	GtkFileFilter *filter;
	gchar *utf8, *path;
	struct stat sb;
	const gchar *name = gtk_action_get_name(action);
	guint drive;

	if ((strlen(name) < 5) || (!g_ascii_isdigit(name[4])))
		return;
	drive = g_ascii_digit_value(name[4]) - 1;

	uninstall_idle_process();

	dialog = gtk_file_chooser_dialog_new("Open a SASI disk image",
	    GTK_WINDOW(main_window), GTK_FILE_CHOOSER_ACTION_OPEN, 
	    GTK_STOCK_OPEN, GTK_RESPONSE_OK,
	    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
	    NULL);
	if (dialog == NULL)
		goto end;

	gtk_file_chooser_set_show_hidden(GTK_FILE_CHOOSER(dialog), TRUE);
	utf8 = g_filename_to_utf8(hddfolder, -1, NULL, NULL, NULL);
	if (utf8) {
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), utf8);
		g_free(utf8);
	}

	filter = gtk_file_filter_new();
	if (filter) {
		gtk_file_filter_set_name(filter, "All files");
		gtk_file_filter_add_pattern(filter, "*");
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	}
	filter = gtk_file_filter_new();
	if (filter) {
		gtk_file_filter_set_name(filter, "SASI disk image files");
		gtk_file_filter_add_pattern(filter, "*.[tT][hH][dD]");
		gtk_file_filter_add_pattern(filter, "*.[hH][dD][iI]");
		gtk_file_filter_add_pattern(filter, "*.[nN][hH][dD]");
		gtk_file_filter_add_pattern(filter, "*.[vV][hH][dD]");
		gtk_file_filter_add_pattern(filter, "*.[sS][lL][hH]");
#ifdef SUPPORT_NVL_IMAGES
		if(nvl_check()) {
			gtk_file_filter_add_pattern(filter, "*.[vV][mM][dD][kK]");
			gtk_file_filter_add_pattern(filter, "*.[dD][sS][kK]");
			gtk_file_filter_add_pattern(filter, "*.[vV][mM][dD][xX]");
			gtk_file_filter_add_pattern(filter, "*.[vV][dD][iI]");
			gtk_file_filter_add_pattern(filter, "*.[qQ][cC][oO][wW]");
			gtk_file_filter_add_pattern(filter, "*.[qQ][cC][oO][wW]2");
			gtk_file_filter_add_pattern(filter, "*.[hH][dD][dD]");
		}
#endif	/* SUPPORT_NVL_IMAGES */
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	}
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_OK)
		goto end;

	utf8 = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
	if (utf8) {
		path = g_filename_from_utf8(utf8, -1, NULL, NULL, NULL);
		if (path) {
			if ((stat(path, &sb) == 0) && S_ISREG(sb.st_mode) && (sb.st_mode & S_IRUSR)) {
				file_cpyname(hddfolder, path, sizeof(hddfolder));
				diskdrv_setsxsi(drive, path);
				sysmng_update(SYS_UPDATEOSCFG);
			}
			g_free(path);
		}
		g_free(utf8);
	}

end:
	if (dialog)
		gtk_widget_destroy(dialog);
	install_idle_process();
}

static void
cb_sasiremove(GtkAction *action, gpointer user_data)
{
	const gchar *name = gtk_action_get_name(GTK_ACTION(action));
	guint drive;

	/* name = "sasi?eject" */
	if ((strlen(name) >= 5) && (g_ascii_isdigit(name[4]))) {
		drive = g_ascii_digit_value(name[4]) - 1;
		if (drive < 2) {
			diskdrv_setsxsi(drive, NULL);
		}
	}
}
#endif	/* !SUPPORT_IDEIO */

#if defined(SUPPORT_STATSAVE)
static void
cb_statsave(GtkAction *action, gpointer user_data)
{
	const gchar *name = gtk_action_get_name(GTK_ACTION(action));
	char ext[4];
	guint n;

	/* name = "stat??save" */
	if ((strlen(name) >= 6)
	 && (g_ascii_isdigit(name[4]))
	 && (g_ascii_isdigit(name[5]))) {
		n = g_ascii_digit_value(name[4]) * 10;
		n += g_ascii_digit_value(name[5]);
		g_snprintf(ext, sizeof(ext), np2flagext, n);
		flagsave(ext);
	}
}

static void
cb_statload(GtkAction *action, gpointer user_data)
{
	const gchar *name = gtk_action_get_name(GTK_ACTION(action));
	char ext[4];
	guint n;

	/* name = "stat??load" */
	if ((strlen(name) >= 6)
	 && (g_ascii_isdigit(name[4]))
	 && (g_ascii_isdigit(name[5]))) {
		n = g_ascii_digit_value(name[4]) * 10;
		n += g_ascii_digit_value(name[5]);
		g_snprintf(ext, sizeof(ext), np2flagext, n);
		flagload(ext, "Status Load", TRUE);
	}
}
#endif

#if defined(SUPPORT_DEBUGSS)
static void
cb_dbsssave(GtkAction *action, gpointer user_data)
{
	const gchar *name = gtk_action_get_name(GTK_ACTION(action));
	guint n;

	/* name = "dbss??save" */
	if ((strlen(name) >= 6)
	 && (g_ascii_isdigit(name[4]))
	 && (g_ascii_isdigit(name[5]))) {
		n = g_ascii_digit_value(name[4]) * 10;
		n += g_ascii_digit_value(name[5]);
		debugsnapshot_save(n);
	}
}

static void
cb_dbssload(GtkAction *action, gpointer user_data)
{
	const gchar *name = gtk_action_get_name(GTK_ACTION(action));
	guint n;

	/* name = "dbss??load" */
	if ((strlen(name) >= 6)
	 && (g_ascii_isdigit(name[4]))
	 && (g_ascii_isdigit(name[5]))) {
		n = g_ascii_digit_value(name[4]) * 10;
		n += g_ascii_digit_value(name[5]);
		debugsnapshot_load(n);
	}
}
#endif

static void
cb_dialog(GtkAction *action, gpointer user_data)
{
	const gchar *name = gtk_action_get_name(action);

	if (g_ascii_strcasecmp(name, "configure") == 0) {
		create_configure_dialog();
#if defined(SUPPORT_HOSTDRV)
	} else if (g_ascii_strcasecmp(name, "hostdrvopt") == 0) {
		create_hostdrv_dialog();
#endif	/* SUPPORT_HOSTDRV */
#if defined(SUPPORT_IDEIO)
	} else if (g_ascii_strcasecmp(name, "ideopt") == 0) {
		create_ide_dialog();
#endif	/* SUPPORT_IDEIO */
	} else if (g_ascii_strcasecmp(name, "soundopt") == 0) {
		create_sound_dialog();
#if defined(SUPPORT_WAB)
	} else if (g_ascii_strcasecmp(name, "wabopt") == 0) {
		create_wab_dialog();
#endif	/* SUPPORT_WAB */
#if defined(SUPPORT_PCI)
	} else if (g_ascii_strcasecmp(name, "pciopt") == 0) {
		create_pci_dialog();
#endif	/* SUPPORT_PCI */
#if defined(SUPPORT_NET)
	} else if (g_ascii_strcasecmp(name, "networkopt") == 0) {
		create_network_dialog();
#endif	/* SUPPORT_NET */
	} else if (g_ascii_strcasecmp(name, "screenopt") == 0) {
		create_screen_dialog();
	} else if (g_ascii_strcasecmp(name, "midiopt") == 0) {
		create_midi_dialog();
	} else if (g_ascii_strcasecmp(name, "serialopt") == 0) {
		create_serial_dialog();
	} else if (g_ascii_strcasecmp(name, "calendar") == 0) {
		create_calendar_dialog();
	} else if (g_ascii_strcasecmp(name, "about") == 0) {
		create_about_dialog();
	}
}


/*
 * toggle item
 */
static void
cb_clockdisp(GtkToggleAction *action, gpointer user_data)
{
	gboolean b = gtk_toggle_action_get_active(action);
	gboolean f;

	f = (np2oscfg.DISPCLK & 1) ^ (b ? 1 : 0);
	if (f) {
		np2oscfg.DISPCLK ^= 1;
		sysmng_workclockrenewal();
		sysmng_updatecaption(3);
		sysmng_update(SYS_UPDATEOSCFG);
	}
}

static void
cb_dispvsync(GtkToggleAction *action, gpointer user_data)
{
	gboolean b = gtk_toggle_action_get_active(action);
	gboolean f;

	f = (np2cfg.DISPSYNC ? 1 : 0) ^ (b ? 1 : 0);
	if (f) {
		np2cfg.DISPSYNC = !np2cfg.DISPSYNC;
		sysmng_update(SYS_UPDATECFG);
	}
}

static void
cb_framedisp(GtkToggleAction *action, gpointer user_data)
{
	gboolean b = gtk_toggle_action_get_active(action);
	gboolean f;

	f = (np2oscfg.DISPCLK & 2) ^ (b ? 2 : 0);
	if (f) {
		np2oscfg.DISPCLK ^= 2;
		sysmng_workclockrenewal();
		sysmng_updatecaption(3);
		sysmng_update(SYS_UPDATEOSCFG);
	}
}

#if defined(SUPPORT_VIDEOFILTER)
static void
cb_vf1en(GtkToggleAction *action, gpointer user_data)
{
	gboolean b = gtk_toggle_action_get_active(action);
	gboolean f;

	f = (np2cfg.vf1_enable ? 1 : 0) ^ (b ? 1 : 0);
	if (f) {
		np2cfg.vf1_enable = !np2cfg.vf1_enable;
		VideoFilter_SetEnable(hVFMng1, np2cfg.vf1_enable);
		sysmng_update(SYS_UPDATECFG);
	}
}

static void
cb_vf1bo(GtkToggleAction *action, gpointer user_data)
{
	gboolean b = gtk_toggle_action_get_active(action);
	gboolean f;

	f = (np2cfg.vf1_bmponly ? 1 : 0) ^ (b ? 1 : 0);
	if (f) {
		np2cfg.vf1_bmponly = !np2cfg.vf1_bmponly;
		sysmng_update(SYS_UPDATECFG);
	}
}
#endif

static void
cb_jastsound(GtkToggleAction *action, gpointer user_data)
{
	gboolean b = gtk_toggle_action_get_active(action);
	gboolean f;

	f = (np2oscfg.jastsnd ? 1 : 0) ^ (b ? 1 : 0);
	if (f) {
		np2oscfg.jastsnd = !np2oscfg.jastsnd;
		sysmng_update(SYS_UPDATEOSCFG);
	}
}

static void
cb_joyrapid(GtkToggleAction *action, gpointer user_data)
{
	gboolean b = gtk_toggle_action_get_active(action);
	gboolean f;

	f = (np2cfg.BTN_RAPID ? 1 : 0) ^ (b ? 1 : 0);
	if (f) {
		np2cfg.BTN_RAPID = !np2cfg.BTN_RAPID;
		sysmng_update(SYS_UPDATECFG);
	}
}

static void
cb_joyreverse(GtkToggleAction *action, gpointer user_data)
{
	gboolean b = gtk_toggle_action_get_active(action);
	gboolean f;

	f = (np2cfg.BTN_MODE ? 1 : 0) ^ (b ? 1 : 0);
	if (f) {
		np2cfg.BTN_MODE = !np2cfg.BTN_MODE;
		sysmng_update(SYS_UPDATECFG);
	}
}

static void
cb_keydisplay(GtkToggleAction *action, gpointer user_data)
{
	gboolean b = gtk_toggle_action_get_active(action);
	gboolean f;

	f = (np2oscfg.keydisp ? 1 : 0) ^ (b ? 1 : 0);
	if (f) {
		np2oscfg.keydisp = !np2oscfg.keydisp;
		sysmng_update(SYS_UPDATEOSCFG);
		if (np2oscfg.keydisp) {
			kdispwin_create();
		} else {
			kdispwin_destroy();
		}
	}
}

static void
cb_mousemode(GtkToggleAction *action, gpointer user_data)
{
	gboolean b = gtk_toggle_action_get_active(action);
	gboolean f;

	f = (np2oscfg.MOUSE_SW ? 1 : 0) ^ (b ? 1 : 0);
	if (f) {
		mouse_running(MOUSE_XOR);
		np2oscfg.MOUSE_SW = !np2oscfg.MOUSE_SW;
		sysmng_update(SYS_UPDATEOSCFG);
	}
}

static void
cb_mouserapid(GtkToggleAction *action, gpointer user_data)
{
	gboolean b = gtk_toggle_action_get_active(action);
	gboolean f;

	f = (np2cfg.MOUSERAPID ? 1 : 0) ^ (b ? 1 : 0);
	if (f) {
		np2cfg.MOUSERAPID = !np2cfg.MOUSERAPID;
		sysmng_update(SYS_UPDATECFG);
	}
}

static void
cb_nowait(GtkToggleAction *action, gpointer user_data)
{
	gboolean b = gtk_toggle_action_get_active(action);
	gboolean f;

	f = (np2oscfg.NOWAIT ? 1 : 0) ^ (b ? 1 : 0);
	if (f) {
		np2oscfg.NOWAIT = !np2oscfg.NOWAIT;
		sysmng_update(SYS_UPDATEOSCFG);
	}
}

#if defined(SUPPORT_ASYNC_CPU)
static void
cb_asynccpu(GtkToggleAction *action, gpointer user_data)
{
	gboolean b = gtk_toggle_action_get_active(action);
	gboolean f;

	f = (np2cfg.asynccpu ? 1 : 0) ^ (b ? 1 : 0);
	if (f) {
		np2cfg.asynccpu = !np2cfg.asynccpu;
		sysmng_update(SYS_UPDATECFG);
	}
}
#endif

static void
cb_realpalettes(GtkToggleAction *action, gpointer user_data)
{
	gboolean b = gtk_toggle_action_get_active(action);
	gboolean f;

	f = (np2cfg.RASTER ? 1 : 0) ^ (b ? 1 : 0);
	if (f) {
		np2cfg.RASTER = !np2cfg.RASTER;
		sysmng_update(SYS_UPDATECFG);
	}
}

static void
cb_s98logging(GtkToggleAction *action, gpointer user_data)
{
	char work[MAX_PATH];
	char work2[64];
	gboolean b = gtk_toggle_action_get_active(action);
	gboolean f;

	f = (s98logging ? 1 : 0) ^ (b ? 1 : 0);
	if (f) {
		s98logging = !s98logging;
		if (s98logging) {
			file_cpyname(work, bmpfilefolder, sizeof(work));
			file_cutname(work);
			g_snprintf(work2, sizeof(work2), "np2_%04d.s98",
			    s98log_count);
			s98log_count = (s98log_count + 1) / 10000;
			file_catname(work, work2, sizeof(work));
			/* XXX file dialog */
		}
	}
}

static void
cb_seeksound(GtkToggleAction *action, gpointer user_data)
{
	gboolean b = gtk_toggle_action_get_active(action);
	gboolean f;

	f = (np2cfg.MOTOR ? 1 : 0) ^ (b ? 1 : 0);
	if (f) {
		np2cfg.MOTOR = !np2cfg.MOTOR;
		sysmng_update(SYS_UPDATECFG);
	}
}

static void
cb_softkeyboard(GtkToggleAction *action, gpointer user_data)
{
	gboolean b = gtk_toggle_action_get_active(action);
	gboolean f;

	f = (np2oscfg.softkbd ? 1 : 0) ^ (b ? 1 : 0);
	if (f) {
		np2oscfg.softkbd = !np2oscfg.softkbd;
		sysmng_update(SYS_UPDATEOSCFG);
		if (np2oscfg.softkbd) {
			skbdwin_create();
		} else {
			skbdwin_destroy();
		}
	}
}

static void
cb_toolwindow(GtkToggleAction *action, gpointer user_data)
{
	gboolean b = gtk_toggle_action_get_active(action);
	gboolean f;

	f = (np2oscfg.toolwin ? 1 : 0) ^ (b ? 1 : 0);
	if (f) {
		np2oscfg.toolwin = !np2oscfg.toolwin;
		sysmng_update(SYS_UPDATEOSCFG);
		if (np2oscfg.toolwin) {
			toolwin_create();
		} else {
			toolwin_destroy();
		}
	}
}

static void
cb_xctrlkey(GtkToggleAction *action, gpointer user_data)
{
	gboolean b = gtk_toggle_action_get_active(action);
	gboolean f;

	f = (np2cfg.XSHIFT & 2) ^ (b ? 2 : 0);
	if (f) {
		np2cfg.XSHIFT ^= 2;
		keystat_forcerelease(0x74);
		sysmng_update(SYS_UPDATECFG);
	}
}

static void
cb_xgrphkey(GtkToggleAction *action, gpointer user_data)
{
	gboolean b = gtk_toggle_action_get_active(action);
	gboolean f;

	f = (np2cfg.XSHIFT & 4) ^ (b ? 4 : 0);
	if (f) {
		np2cfg.XSHIFT ^= 4;
		keystat_forcerelease(0x73);
		sysmng_update(SYS_UPDATECFG);
	}
}

static void
cb_xshiftkey(GtkToggleAction *action, gpointer user_data)
{
	gboolean b = gtk_toggle_action_get_active(action);
	gboolean f;

	f = (np2cfg.XSHIFT & 1) ^ (b ? 1 : 0);
	if (f) {
		np2cfg.XSHIFT ^= 1;
		sysmng_update(SYS_UPDATECFG);
	}
}

static void
cb_xrollkey(GtkToggleAction *action, gpointer user_data)
{
	gboolean b = gtk_toggle_action_get_active(action);
	gboolean f;

	f = (np2oscfg.xrollkey ? 1 : 0) ^ (b ? 1 : 0);
	if (f) {
		np2oscfg.xrollkey = !np2oscfg.xrollkey;
		sysmng_update(SYS_UPDATEOSCFG);
	}
}

static void
cb_fixmmtimer(GtkToggleAction *action, gpointer user_data)
{
	gboolean b = gtk_toggle_action_get_active(action);
	gboolean f;

	f = (np2cfg.timerfix & 1) ^ (b ? 1 : 0);
	if (f) {
		np2cfg.timerfix ^= 1;
		sysmng_update(SYS_UPDATECFG);
	}
}

static void
cb_16mbmemchk(GtkToggleAction *action, gpointer user_data)
{
	gboolean b = gtk_toggle_action_get_active(action);
	gboolean f;

	f = (np2cfg.memchkmx == 15) ^ (b ? 1 : 0);
	if (f) {
		np2cfg.memchkmx = (b ? 15 : 0);
		sysmng_update(SYS_UPDATECFG);
	}
}

#if defined(SUPPORT_FAST_MEMORYCHECK)
static void
cb_fastmemchk(GtkToggleAction *action, gpointer user_data)
{
	gboolean b = gtk_toggle_action_get_active(action);
	gboolean f;

	f = (np2cfg.memcheckspeed > 1) ^ (b ? 1 : 0);
	if (f) {
		np2cfg.memcheckspeed = (b ? 8 : 1);
		sysmng_update(SYS_UPDATECFG);
	}
}
#endif

static void
cb_itfwork(GtkToggleAction *action, gpointer user_data)
{
	gboolean b = gtk_toggle_action_get_active(action);
	gboolean f;

	f = (np2cfg.ITF_WORK & 1) ^ (b ? 1 : 0);
	if (f) {
		np2cfg.ITF_WORK ^= 1;
		sysmng_update(SYS_UPDATECFG);
	}
}

#if defined(SUPPORT_FMGEN)
static void
cb_fmgen(GtkToggleAction *action, gpointer user_data)
{
	gboolean b = gtk_toggle_action_get_active(action);
	gboolean f;

	f = (np2cfg.usefmgen & 1) ^ (b ? 1 : 0);
	if (f) {
		np2cfg.usefmgen ^= 1;
		sysmng_update(SYS_UPDATECFG);
	}
}
#endif

static void
cb_hf_enable(GtkToggleAction *action, gpointer user_data) {
	gboolean b = gtk_toggle_action_get_active(action);
	gboolean f;

	f = (hf_enable & 1) ^ (b ? 1 : 0);
	if (f) {
		hf_enable ^= 1;
		if(hf_enable) {
			hook_fontrom_defenable();
		} else {
			hook_fontrom_defdisable();
		}
	}
}

#if defined(SUPPORT_DEBUGSS)
static void
cb_en_dbss(GtkToggleAction *action, gpointer user_data) {
	gboolean b = gtk_toggle_action_get_active(action);
	gboolean f;

	f = (np2cfg.debugss & 1) ^ (b ? 1 : 0);
	if (f) {
		np2cfg.debugss ^= 1;
		sysmng_update(SYS_UPDATECFG);
	}

	xmenu_visible_item(NULL, "DebugSSMenu", np2cfg.debugss);
}
#endif

static void
cb_sndus(GtkToggleAction *action, gpointer user_data)
{
	keystat_senddata(0x70);
	keystat_senddata(0x33);
	keystat_senddata(0x70 | 0x80);
	keystat_senddata(0x33 | 0x80);
}

static void
cb_sndcad(GtkToggleAction *action, gpointer user_data)
{
	keystat_senddata(0x73);
	keystat_senddata(0x74);
	keystat_senddata(0x39);
	keystat_senddata(0x73 | 0x80);
	keystat_senddata(0x74 | 0x80);
	keystat_senddata(0x39 | 0x80);
}

/*
 * radio item
 */
static void
cb_beepvol(gint idx)
{
	guint value;

	if (idx >= 0) {
		value = beepvol_entries[idx].value;
	} else {
		value = np2cfg_default.BEEP_VOL;
	}
	if (np2cfg.BEEP_VOL != value) {
		np2cfg.BEEP_VOL = value;
		beep_setvol(value);
		sysmng_update(SYS_UPDATECFG);
	}
}

static void
cb_kbtype(gint idx)
{
	guint value;

	if (idx >= 0) {
		value = kbtype_entries[idx].value;
	} else {
		value = np2oscfg.KEYBOARD;
	}
	if (np2oscfg.KEYBOARD != value) {
		np2oscfg.KEYBOARD = value;
		beep_setvol(value);
		sysmng_update(SYS_UPDATEOSCFG);
	}
}

static void
cb_f11key(gint idx)
{
	guint value;

	if (idx >= 0) {
		value = f11key_entries[idx].value;
	} else {
		value = np2oscfg_default.F11KEY;
	}
	if (np2oscfg.F11KEY != value) {
		np2oscfg.F11KEY = value;
		sysmng_update(SYS_UPDATEOSCFG);
	}
}

static void
cb_f12key(gint idx)
{
	guint value;

	if (idx >= 0) {
		value = f12key_entries[idx].value;
	} else {
		value = np2oscfg_default.F12KEY;
	}
	if (np2oscfg.F12KEY != value) {
		np2oscfg.F12KEY = value;
		kbdmng_resetf12();
		sysmng_update(SYS_UPDATEOSCFG);
	}
}

#if defined(SUPPORT_VIDEOFILTER)
static void
cb_vf1p(gint idx)
{
	if (np2cfg.vf1_pno != idx) {
		np2cfg.vf1_pno = idx;
		VideoFilter_SetProfileNo(hVFMng1, idx);
		sysmng_update(SYS_UPDATECFG);
	}
}
#endif

static void
cb_framerate(gint idx)
{
	guint value;

	if (idx >= 0) {
		value = framerate_entries[idx].value;
	} else {
		value = np2oscfg_default.DRAW_SKIP;
	}
	if (np2oscfg.DRAW_SKIP != value) {
		np2oscfg.DRAW_SKIP = value;
		sysmng_update(SYS_UPDATECFG);
	}
}

static void
cb_joykey(gint idx)
{
	guint value;

	if (idx >= 0) {
		value = joykey_entries[idx].value;
	} else {
		value = np2cfg_default.KEY_MODE;
	}
	if (np2cfg.KEY_MODE != value) {
		np2cfg.KEY_MODE = value;
		keystat_resetjoykey();
		sysmng_update(SYS_UPDATECFG);
	}
}

static void
cb_memory(gint idx)
{
	guint value;

	if (idx >= 0) {
		value = memory_entries[idx].value;
	} else {
		value = np2cfg_default.EXTMEM;
	}
	if (np2cfg.EXTMEM != value) {
		np2cfg.EXTMEM = value;
		sysmng_update(SYS_UPDATECFG);
	}
}

static void
cb_rotate(gint idx)
{
	guint value;

	if (idx >= 0) {
		value = rotate_entries[idx].value;
	} else {
		value = 0;
	}
	changescreen((scrnmode & ~SCRNMODE_ROTATEMASK) | value);
}

static void
cb_screenmode(gint idx)
{
	guint value;

	if (idx >= 0) {
		value = screenmode_entries[idx].value;
	} else {
		value = 0;
	}
	changescreen((scrnmode & ~SCRNMODE_FULLSCREEN) | value);
}

static void
cb_screensize(gint idx)
{
	guint value;

	if (idx >= 0) {
		value = screensize_entries[idx].value;
	} else {
		value = 0;
	}
	scrnmng_setmultiple(value);
}

static void
cb_soundboard(gint idx)
{
	guint value;

	if (idx >= 0) {
		value = soundboard_entries[idx].value;
	} else {
		value = np2cfg_default.SOUND_SW;
	}
	if (np2cfg.SOUND_SW != value) {
		np2cfg.SOUND_SW = value;
		sysmng_update(SYS_UPDATECFG);
	}
}

static void
cb_fpu(gint idx)
{
	guint value;

	if (idx >= 0) {
		value = fpu_entries[idx].value;
	} else {
		value = 0;
	}
	switch(value) {
	case 0:
		if (np2cfg.fpu_type != FPU_TYPE_SOFTFLOAT) {
			np2cfg.fpu_type = FPU_TYPE_SOFTFLOAT;
			sysmng_update(SYS_UPDATECFG);
		}
		break;
	case 1:
		if (np2cfg.fpu_type != FPU_TYPE_DOSBOX) {
			np2cfg.fpu_type = FPU_TYPE_DOSBOX;
			sysmng_update(SYS_UPDATECFG);
		}
		break;
	case 2:
		if (np2cfg.fpu_type != FPU_TYPE_DOSBOX2) {
			np2cfg.fpu_type = FPU_TYPE_DOSBOX2;
			sysmng_update(SYS_UPDATECFG);
		}
		break;
	}
}

static void
cb_radio(GtkRadioAction *action, GtkRadioAction *current, gpointer user_data)
{
	gint value = gtk_radio_action_get_current_value(action);
	guint menu_idx = (guint)GPOINTER_TO_INT(user_data);
	gint i;

	if (menu_idx < n_radiomenu_entries) {
		for (i = 0; i < radiomenu_entries[menu_idx].count; i++) {
			if (radiomenu_entries[menu_idx].entry[i].value == value)
				break;
		}
		if (i == radiomenu_entries[menu_idx].count) {
			i = -1;
		}
		if (radiomenu_entries[menu_idx].func) {
			(*radiomenu_entries[menu_idx].func)(i);
		}
	}
}


/*
 * create menubar
 */
static GtkWidget *menubar;
static guint menubar_timerid;

#define	EVENT_MASK	(GDK_ENTER_NOTIFY_MASK|GDK_LEAVE_NOTIFY_MASK)

static gboolean
menubar_timeout(gpointer p)
{

	if (menubar_timerid) {
		g_source_remove(menubar_timerid);
		menubar_timerid = 0;
	}

	if (scrnmode & SCRNMODE_FULLSCREEN) {
		xmenu_hide();
	}

	return TRUE;
}

/*
 - Signal: gboolean GtkWidget::enter_notify_event (GtkWidget *widget,
          GdkEventCrossing *event, gpointer user_data)
*/
static gboolean
enter_notify_evhandler(GtkWidget *w, GdkEventCrossing *ev, gpointer p)
{

	if (menubar_timerid) {
		g_source_remove(menubar_timerid);
		menubar_timerid = 0;
	}

	return TRUE;
}

/*
 - Signal: gboolean GtkWidget::leave_notify_event (GtkWidget *widget,
          GdkEventCrossing *event, gpointer user_data)
*/
static gboolean
leave_notify_evhandler(GtkWidget *w, GdkEventCrossing *ev, gpointer p)
{

	if (menubar_timerid) {
		g_source_remove(menubar_timerid);
		menubar_timerid = 0;
	}

	if (scrnmode & SCRNMODE_FULLSCREEN) {
		menubar_timerid = g_timeout_add(1000, menubar_timeout, NULL);
	}

	return TRUE;
}

#if defined(SUPPORT_STATSAVE)
static void
create_menu_statsave(GtkUIManager *ui_manager, int num)
{
	char *name, *action;
	guint id;
	int i;

	if (num <= 0)
		return;

	/* Save %d */
	for (i = 0; i < num; i++) {
		id = gtk_ui_manager_new_merge_id(ui_manager);
		name = g_strdup_printf("Save %d", i);
		action = g_strdup_printf("stat%02dsave", i);
		gtk_ui_manager_add_ui(ui_manager, id, "/MainMenu/Stat",
		    name, action, GTK_UI_MANAGER_MENUITEM, FALSE);
		g_free(action);
		g_free(name);
	}

	/* separator */
	id = gtk_ui_manager_new_merge_id(ui_manager);
	gtk_ui_manager_add_ui(ui_manager, id, "/MainMenu/Stat",
	    "", "", GTK_UI_MANAGER_SEPARATOR, FALSE);

	/* Load %d */
	for (i = 0; i < num; i++) {
		id = gtk_ui_manager_new_merge_id(ui_manager);
		name = g_strdup_printf("Load %d", i);
		action = g_strdup_printf("stat%02dload", i);
		gtk_ui_manager_add_ui(ui_manager, id, "/MainMenu/Stat",
		    name, action, GTK_UI_MANAGER_MENUITEM, FALSE);
		g_free(action);
		g_free(name);
	}
}
#endif

static void
equip_fddrive(GtkUIManager *ui_manager, guint no)
{
	char *path, *name, *action;
	guint id;

	if (no >= 4)
		return;
	no++;

	id = gtk_ui_manager_new_merge_id(ui_manager);
	name = g_strdup_printf("Drive%d", no);
	action = g_strdup_printf("Drive%dMenu", no);
	gtk_ui_manager_add_ui(ui_manager, id,
	    "/MainMenu/FDD", name, action, GTK_UI_MANAGER_MENU, FALSE);
	g_free(action);
	g_free(name);

	path = g_strdup_printf("/MainMenu/FDD/Drive%d", no);

	id = gtk_ui_manager_new_merge_id(ui_manager);
	name = g_strdup_printf("Drive%dOpen", no);
	action = g_strdup_printf("disk%dopen", no);
	gtk_ui_manager_add_ui(ui_manager, id,
	    path, name, action, GTK_UI_MANAGER_MENUITEM, FALSE);
	g_free(action);
	g_free(name);

	id = gtk_ui_manager_new_merge_id(ui_manager);
	name = g_strdup_printf("Drive%dEject", no);
	action = g_strdup_printf("disk%deject", no);
	gtk_ui_manager_add_ui(ui_manager, id,
	    path, name, action, GTK_UI_MANAGER_MENUITEM, FALSE);
	g_free(action);
	g_free(name);

	g_free(path);
}

GtkWidget *
create_menu(void)
{
	GError *err = NULL;
  GtkAction *action;
	gint rv;
	guint i;
	GtkWidget *child;

	menu_hdl.action_group = gtk_action_group_new("MenuActions");
	gtk_action_group_add_actions(menu_hdl.action_group,
	    menu_entries, n_menu_entries, NULL);
	gtk_action_group_add_toggle_actions(menu_hdl.action_group,
	    togglemenu_entries, n_togglemenu_entries, NULL);
	for (i = 0; i < n_radiomenu_entries; i++) {
		gtk_action_group_add_radio_actions(menu_hdl.action_group,
		    radiomenu_entries[i].entry, radiomenu_entries[i].count, 0,
		    G_CALLBACK(cb_radio), GINT_TO_POINTER(i));
	}

	menu_hdl.ui_manager = gtk_ui_manager_new();
	gtk_ui_manager_insert_action_group(menu_hdl.ui_manager,
	    menu_hdl.action_group, 0);

	gtk_window_add_accel_group(GTK_WINDOW(main_window),
	    gtk_ui_manager_get_accel_group(menu_hdl.ui_manager));

	rv = gtk_ui_manager_add_ui_from_string(menu_hdl.ui_manager,
	    ui_info, -1, &err);
	if (!rv) {
		g_message("create_menu: menu build failed: %s", err->message);
		g_error_free(err);
		return NULL;
	}

#if defined(SUPPORT_STATSAVE)
	if (np2oscfg.statsave) {
		create_menu_statsave(menu_hdl.ui_manager, NSTATSAVE);
	}
#endif

	if (np2cfg.fddequip) {
		for (i = 0; i < 4; i++) {
			if (np2cfg.fddequip & (1 << i)) {
				equip_fddrive(menu_hdl.ui_manager, i);
			}
		}
	}

	xmenu_toggle_item(NULL, "dispvsync", np2cfg.DISPSYNC);
	xmenu_toggle_item(NULL, "joyrapid", np2cfg.BTN_RAPID);
	xmenu_toggle_item(NULL, "joyreverse", np2cfg.BTN_MODE);
	xmenu_toggle_item(NULL, "mouserapid", np2cfg.MOUSERAPID);
	xmenu_toggle_item(NULL, "realpalettes", np2cfg.RASTER);
	xmenu_toggle_item(NULL, "seeksound", np2cfg.MOTOR);
	xmenu_toggle_item(NULL, "xctrlkey", np2cfg.XSHIFT & 2);
	xmenu_toggle_item(NULL, "xgrphkey", np2cfg.XSHIFT & 4);
	xmenu_toggle_item(NULL, "xshiftkey", np2cfg.XSHIFT & 1);
	xmenu_toggle_item(NULL, "xrollkey", np2oscfg.xrollkey);
	xmenu_toggle_item(NULL, "itfwork", np2cfg.ITF_WORK);
#if defined(SUPPORT_FAST_MEMORYCHECK)
	xmenu_toggle_item(NULL, "fastmemchk", np2cfg.memcheckspeed > 1);
#endif
	xmenu_toggle_item(NULL, "fixmmtimer", np2cfg.timerfix);
	xmenu_toggle_item(NULL, "16mbmemchk", np2cfg.memchkmx == 15);
#if defined(SUPPORT_FMGEN)
	xmenu_toggle_item(NULL, "fmgen", np2cfg.usefmgen & 1);
#endif
	xmenu_toggle_item(NULL, "hf_enable", hf_enable & 1);
	xmenu_toggle_item(NULL, "clockdisp", np2oscfg.DISPCLK & 1);
#if defined(SUPPORT_VIDEOFILTER)
	xmenu_toggle_item(NULL, "vf1en", np2cfg.vf1_enable);
	xmenu_toggle_item(NULL, "vf1bo", np2cfg.vf1_bmponly);
#endif
	xmenu_toggle_item(NULL, "framedisp", np2oscfg.DISPCLK & 2);
	xmenu_toggle_item(NULL, "jastsound", np2oscfg.jastsnd);
	xmenu_toggle_item(NULL, "keydisplay", np2oscfg.keydisp);
	xmenu_toggle_item(NULL, "mousemode", np2oscfg.MOUSE_SW);
	xmenu_toggle_item(NULL, "nowait", np2oscfg.NOWAIT);
#if defined(SUPPORT_ASYNC_CPU)
	xmenu_toggle_item(NULL, "asynccpu", np2cfg.asynccpu);
#endif
	xmenu_toggle_item(NULL, "softkeyboard", np2oscfg.softkbd);
	xmenu_toggle_item(NULL, "toolwindow", np2oscfg.toolwin);

#if defined(SUPPORT_DEBUGSS)
	xmenu_toggle_item(NULL, "en_dbss", np2cfg.debugss);
#endif

	xmenu_select_beepvol(np2cfg.BEEP_VOL);
	xmenu_select_kbtype(np2oscfg.KEYBOARD);
	xmenu_select_f11key(np2oscfg.F11KEY);
	xmenu_select_f12key(np2oscfg.F12KEY);
#if defined(SUPPORT_VIDEOFILTER)
	xmenu_select_vf1p(np2cfg.vf1_pno);
#endif
	xmenu_select_framerate(np2oscfg.DRAW_SKIP);
	xmenu_select_joykey(np2cfg.KEY_MODE);
	xmenu_select_memory(np2cfg.EXTMEM);
	xmenu_select_rotate(scrnmode & SCRNMODE_ROTATEMASK);
	xmenu_select_screenmode(scrnmode & SCRNMODE_FULLSCREEN);
	xmenu_select_screensize(SCREEN_DEFMUL);
	xmenu_select_soundboard(np2cfg.SOUND_SW);
	i = 0;
	switch(np2cfg.fpu_type) {
	case FPU_TYPE_SOFTFLOAT:
		i = 0;
		break;
	case FPU_TYPE_DOSBOX:
		i = 1;
		break;
	case FPU_TYPE_DOSBOX2:
		i = 2;
		break;
	}
	xmenu_select_fpu(i);

#if defined(SUPPORT_DEBUGSS)
	xmenu_visible_item(NULL, "DebugSSMenu", np2cfg.debugss);
#endif

	menubar = gtk_ui_manager_get_widget(menu_hdl.ui_manager, "/MainMenu");

	gtk_widget_add_events(menubar, EVENT_MASK);
	g_signal_connect(G_OBJECT(menubar), "enter_notify_event",
	            G_CALLBACK(enter_notify_evhandler), NULL);
	g_signal_connect(G_OBJECT(menubar), "leave_notify_event",
	            G_CALLBACK(leave_notify_evhandler), NULL);

	return menubar;
}

void
xmenu_hide(void)
{

	gtk_widget_hide(menubar);
}

void
xmenu_show(void)
{

	gtk_widget_show(menubar);
}

void
xmenu_toggle_menu(void)
{

	if (
#if GTK_MAJOR_VERSION > 2 || (GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION >= 18)
	    gtk_widget_get_visible(menubar)
#else
	    GTK_WIDGET_VISIBLE(menubar)
#endif
	)
		xmenu_hide();
	else
		xmenu_show();
}

void
xmenu_select_screen(UINT8 mode)
{

	xmenu_select_rotate(mode & SCRNMODE_ROTATEMASK);
	xmenu_select_screenmode(mode & SCRNMODE_FULLSCREEN);
}
