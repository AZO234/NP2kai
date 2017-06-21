/*
 * Copyright (c) 2002-2004 NONAKA Kimihiro
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

#include "compiler.h"

#include "gtk2/xnp2.h"
#include "gtk2/gtk_menu.h"

#include "np2.h"
#include "dosio.h"
#include "ini.h"
#include "opngen.h"
#include "pccore.h"
#include "iocore.h"

#include "soundmng.h"
#include "sysmng.h"


/*
 * Mixer
 */
static const struct {
	const char	*name;
	UINT8		*valp;
	gfloat		min;
	gfloat		max;
} mixer_vol_tbl[] = {
	{ "FM",     &np2cfg.vol_fm,     0.0, 128.0 },
	{ "PSG",    &np2cfg.vol_ssg,    0.0, 128.0 },
	{ "ADPCM",  &np2cfg.vol_adpcm,  0.0, 128.0 },
	{ "PCM",    &np2cfg.vol_pcm,    0.0, 128.0 },
	{ "Rhythm", &np2cfg.vol_rhythm, 0.0, 128.0 },
};

static GObject *mixer_adj[NELEMENTS(mixer_vol_tbl)];


/*
 * PC-9801-14
 */

static const char *snd14_vol_str[] = {
	"left", "right", "f2", "f4", "f8", "f16"
};

static GObject *snd14_adj[NELEMENTS(snd14_vol_str)];


/*
 * PC-9801-26
 */

#define	SND26_SHIFT_IOPORT	4
#define	SND26_SHIFT_INTR	6
#define	SND26_SHIFT_ROMADDR	0

#define	SND26_MASK_IOPORT	0x10
#define	SND26_MASK_INTR		0xc0
#define	SND26_MASK_ROMADDR	0x07

#define	SND26_GET_IOPORT() \
	((np2cfg.snd26opt & SND26_MASK_IOPORT) >> SND26_SHIFT_IOPORT)
#define	SND26_GET_INTR() \
	((np2cfg.snd26opt & SND26_MASK_INTR) >> SND26_SHIFT_INTR)
#define	SND26_GET_ROMADDR()	snd26_get_romaddr()

#define	SND26_SET_IOPORT(v) \
	    (((v) << SND26_SHIFT_IOPORT) & SND26_MASK_IOPORT)
#define	SND26_SET_INTR(v) \
	    (((v) << SND26_SHIFT_INTR) & SND26_MASK_INTR)
#define	SND26_SET_ROMADDR(v)	snd26_set_romaddr(v)

static int
snd26_get_romaddr(void)
{
	int idx;

	idx = ((np2cfg.snd26opt & SND26_MASK_ROMADDR) >> SND26_SHIFT_ROMADDR);
	if (idx < 4)
		return idx;
	return 4;
}

static int
snd26_set_romaddr(int idx)
{

	if (idx < 4)
		return ((idx << SND26_SHIFT_ROMADDR) & SND26_MASK_ROMADDR);
	return ((4 << SND26_SHIFT_ROMADDR) & SND26_MASK_ROMADDR);
}

static const char *snd26_ioport_str[] = {
	"0288", "0188"
};

static const char *snd26_intr_str[] = {
	"INT0", "INT6", "INT4", "INT5"
};

static const char *snd26_romaddr_str[] = {
	"C8000", "CC000", "D0000", "D4000", "N/C"
};

static GtkWidget *snd26_ioport_entry;
static GtkWidget *snd26_int_entry;
static GtkWidget *snd26_romaddr_entry;


/*
 * PC-9801-86
 */

#define	SND86_SHIFT_IOPORT	0
#define	SND86_SHIFT_BIOSROM	1
#define	SND86_SHIFT_INTR	2
#define	SND86_SHIFT_INTERRUPT	4
#define	SND86_SHIFT_SOUNDID	5

#define	SND86_MASK_IOPORT	0x01
#define	SND86_MASK_BIOSROM	0x02
#define	SND86_MASK_INTR		0x0c
#define	SND86_MASK_INTERRUPT	0x10
#define	SND86_MASK_SOUNDID	0xe0

#define	SND86_GET_IOPORT() \
	((np2cfg.snd86opt & SND86_MASK_IOPORT) >> SND86_SHIFT_IOPORT)
#define	SND86_GET_BIOSROM() \
	((np2cfg.snd86opt & SND86_MASK_BIOSROM) >> SND86_SHIFT_BIOSROM)
#define	SND86_GET_INTR() \
	((np2cfg.snd86opt & SND86_MASK_INTR) >> SND86_SHIFT_INTR)
#define	SND86_GET_INTERRUPT() \
	((np2cfg.snd86opt & SND86_MASK_INTERRUPT) >> SND86_SHIFT_INTERRUPT)
#define	SND86_GET_SOUNDID() \
	((np2cfg.snd86opt & SND86_MASK_SOUNDID) >> SND86_SHIFT_SOUNDID)

#define	SND86_SET_IOPORT(v) \
	    (((v) << SND86_SHIFT_IOPORT) & SND86_MASK_IOPORT)
#define	SND86_SET_BIOSROM(v) \
	    (((v) << SND86_SHIFT_BIOSROM) & SND86_MASK_BIOSROM)
#define	SND86_SET_INTR(v) \
	    (((v) << SND86_SHIFT_INTR) & SND86_MASK_INTR)
#define	SND86_SET_INTERRUPT(v) \
	    (((v) << SND86_SHIFT_INTERRUPT) & SND86_MASK_INTERRUPT)
#define	SND86_SET_SOUNDID(v) \
	    (((v) << SND86_SHIFT_SOUNDID) & SND86_MASK_SOUNDID)

static const char *snd86_ioport_str[] = {
	"0288", "0188",
};

static const char *snd86_intr_str[] = {
	"INT0", "INT4", "INT6", "INT5",
};

static const char *snd86_soundid_str[] = {
	"7x", "6x", "5x", "4x", "3x", "2x", "1x", "0x",
};

static GtkWidget *snd86_ioport_entry;
static GtkWidget *snd86_int_entry;
static GtkWidget *snd86_soundid_entry;
static GtkWidget *snd86_int_checkbutton;
static GtkWidget *snd86_rom_checkbutton;


/*
 * Speak board
 */

#define	SPB_SHIFT_IOPORT	SND26_SHIFT_IOPORT
#define	SPB_SHIFT_INTR		SND26_SHIFT_INTR
#define	SPB_SHIFT_ROMADDR	SND26_SHIFT_ROMADDR

#define	SPB_MASK_IOPORT		SND26_MASK_IOPORT
#define	SPB_MASK_INTR		SND26_MASK_INTR	
#define	SPB_MASK_ROMADDR	SND26_MASK_ROMADDR

#define	SPB_GET_IOPORT()	SND26_GET_IOPORT()
#define	SPB_GET_INTR()		SND26_GET_INTR()
#define	SPB_GET_ROMADDR()	SND26_GET_ROMADDR()

#define	SPB_SET_IOPORT(v)	SND26_SET_IOPORT(v)
#define	SPB_SET_INTR(v)		SND26_SET_INTR(v)
#define	SPB_SET_ROMADDR(v)	SND26_SET_ROMADDR(v)

static const char *spb_ioport_str[] = {
	"0088", "0188"
};

#define	spb_intr_str	snd26_intr_str
#define	spb_romaddr_str	snd26_romaddr_str

static const char *spb_vr_channel_str[] = {
	"L", "R"
};

static GtkWidget *spb_ioport_entry;
static GtkWidget *spb_int_entry;
static GtkWidget *spb_romaddr_entry;
static GtkWidget *spb_vr_channel_checkbutton[2];
static GtkWidget *spb_reverse_channel_checkbutton;
static GObject *spb_vr_level_adj;


/*
 * JoyPad
 */

static const char *joypad_nodevice_str = "No device";
static const char *joypad_noconnect_str = "N/C";
static const char *joypad_num_str[256] = {
	"0",   "1",   "2",   "3",   "4",   "5",   "6",   "7", 
	"8",   "9",   "10",  "11",  "12",  "13",  "14",  "15", 
	"16",  "17",  "18",  "19",  "20",  "21",  "22",  "23", 
	"24",  "25",  "26",  "27",  "28",  "29",  "30",  "31", 
	"32",  "33",  "34",  "35",  "36",  "37",  "38",  "39", 
	"40",  "41",  "42",  "43",  "44",  "45",  "46",  "47", 
	"48",  "49",  "50",  "51",  "52",  "53",  "54",  "55", 
	"56",  "57",  "58",  "59",  "60",  "61",  "62",  "63", 
	"64",  "65",  "66",  "67",  "68",  "69",  "70",  "71", 
	"72",  "73",  "74",  "75",  "76",  "77",  "78",  "79", 
	"80",  "81",  "82",  "83",  "84",  "85",  "86",  "87", 
	"88",  "89",  "90",  "91",  "92",  "93",  "94",  "95", 
	"96",  "97",  "98",  "99",  "100", "101", "102", "103", 
	"104", "105", "106", "107", "108", "109", "110", "111", 
	"112", "113", "114", "115", "116", "117", "118", "119", 
	"120", "121", "122", "123", "124", "125", "126", "127", 
	"128", "129", "130", "131", "132", "133", "134", "135", 
	"136", "137", "138", "139", "140", "141", "142", "143", 
	"144", "145", "146", "147", "148", "149", "150", "151", 
	"152", "153", "154", "155", "156", "157", "158", "159", 
	"160", "161", "162", "163", "164", "165", "166", "167", 
	"168", "169", "170", "171", "172", "173", "174", "175", 
	"176", "177", "178", "179", "180", "181", "182", "183", 
	"184", "185", "186", "187", "188", "189", "190", "191", 
	"192", "193", "194", "195", "196", "197", "198", "199", 
	"200", "201", "202", "203", "204", "205", "206", "207", 
	"208", "209", "210", "211", "212", "213", "214", "215", 
	"216", "217", "218", "219", "220", "221", "222", "223", 
	"224", "225", "226", "227", "228", "229", "230", "231", 
	"232", "233", "234", "235", "236", "237", "238", "239", 
	"240", "241", "242", "243", "244", "245", "246", "247", 
	"248", "249", "250", "251", "252", "253", "254", "255", 
};

static joymng_devinfo_t **joypad_devlist;
static GtkWidget *joypad_use_checkbutton[1];
static GtkWidget *joypad_devlist_combo;
static GtkWidget *joypad_axis_combo[JOY_NAXIS];
static GtkWidget *joypad_button_combo[JOY_NBUTTON];
static char joypad_devname[MAX_PATH];
static UINT8 joypad_axis[JOY_NAXIS];
static UINT8 joypad_button[JOY_NBUTTON];


/*
 * Driver
 */

static const char *driver_name[SNDDRV_DRVMAX] = {
	"None",
	"SDL",
};

static int driver_snddrv;


static void
ok_button_clicked(GtkButton *b, gpointer d)
{
	/* mixer */
	guint mixer_vol[NELEMENTS(mixer_vol_tbl)];

	/* PC-9801-14 */
	guint snd14_vol14[NELEMENTS(snd14_vol_str)];

	/* PC-9801-26 */
	const gchar *snd26_ioport;
	const gchar *snd26_intr;
	const gchar *snd26_romaddr;
	UINT8 snd26opt, snd26opt_mask;

	/* PC-9801-86 */
	const gchar *snd86_ioport;
	const gchar *snd86_intr;
	const gchar *snd86_soundid;
	gint snd86_interrupt;
	gint snd86_biosrom;
	UINT8 snd86opt, snd86opt_mask;

	/* Speak board */
	const gchar *spb_ioport;
	const gchar *spb_intr;
	const gchar *spb_romaddr;
	UINT8 spb_vrc;
	UINT8 spb_vrl;
	UINT8 spb_x;
	UINT8 spbopt, spbopt_mask;

	/* JoyPad */
	UINT8 joypad[1];
	gint joypad_device_index;

	/* common */
	char buf[32];
	int i;
	BOOL renewal;

	/* Mixer */
	renewal = FALSE;
	for (i = 0; i < NELEMENTS(mixer_vol_tbl); i++) {
		mixer_vol[i] = (guint)gtk_adjustment_get_value(
		    GTK_ADJUSTMENT(mixer_adj[i]));
		if (*mixer_vol_tbl[i].valp != mixer_vol[i]) {
			*mixer_vol_tbl[i].valp = mixer_vol[i];
			renewal = TRUE;
		}
	}

	if (renewal) {
		sysmng_update(SYS_UPDATECFG);
	}

	/* PC-9801-14 */
	renewal = FALSE;
	for (i = 0; i < NELEMENTS(snd14_vol_str); i++) {
		snd14_vol14[i] = (guint)gtk_adjustment_get_value(
		    GTK_ADJUSTMENT(snd14_adj[i]));
		if (np2cfg.vol14[i] != snd14_vol14[i]) {
			np2cfg.vol14[i] = snd14_vol14[i];
			renewal = TRUE;
		}
	}

	if (renewal) {
		sysmng_update(SYS_UPDATECFG);
	}

	/* PC-9801-26 */
	snd26_ioport = gtk_entry_get_text(GTK_ENTRY(snd26_ioport_entry));
	snd26_intr = gtk_entry_get_text(GTK_ENTRY(snd26_int_entry));
	snd26_romaddr = gtk_entry_get_text(GTK_ENTRY(snd26_romaddr_entry));

	renewal = FALSE;
	snd26opt = snd26opt_mask = 0;
	for (i = 0; i < NELEMENTS(snd26_ioport_str); i++) {
		if (strcmp(snd26_ioport, snd26_ioport_str[i]) == 0) {
			if (SND26_GET_IOPORT() != i) {
				snd26opt |= SND26_SET_IOPORT(i);
				snd26opt_mask |= SND26_MASK_IOPORT;
				renewal = TRUE;
			}
			break;
		}
	}
	for (i = 0; i < NELEMENTS(snd26_intr_str); i++) {
		if (strcmp(snd26_intr, snd26_intr_str[i]) == 0) {
			if (SND26_GET_INTR() != i) {
				snd26opt |= SND26_SET_INTR(i);
				snd26opt_mask |= SND26_MASK_INTR;
				renewal = TRUE;
			}
			break;
		}
	}
	for (i = 0; i < NELEMENTS(snd26_romaddr_str); i++) {
		if (strcmp(snd26_romaddr, snd26_romaddr_str[i]) == 0) {
			if (SND26_GET_ROMADDR() != i) {
				snd26opt |= SND26_SET_ROMADDR(i);
				snd26opt_mask |= SND26_MASK_ROMADDR;
				renewal = TRUE;
			}
			break;
		}
	}

	if (renewal) {
		np2cfg.snd26opt &= ~snd26opt_mask;
		np2cfg.snd26opt |= snd26opt;
		sysmng_update(SYS_UPDATECFG);
	}

	/* PC-9801-86 */
	snd86_ioport = gtk_entry_get_text(GTK_ENTRY(snd86_ioport_entry));
	snd86_intr = gtk_entry_get_text(GTK_ENTRY(snd86_int_entry));
	snd86_soundid = gtk_entry_get_text(GTK_ENTRY(snd86_soundid_entry));
	snd86_interrupt = gtk_toggle_button_get_active(
	    GTK_TOGGLE_BUTTON(snd86_int_checkbutton));
	snd86_biosrom = gtk_toggle_button_get_active(
	    GTK_TOGGLE_BUTTON(snd86_rom_checkbutton));

	renewal = FALSE;
	snd86opt = snd86opt_mask = 0;
	for (i = 0; i < NELEMENTS(snd86_ioport_str); i++) {
		if (strcmp(snd86_ioport, snd86_ioport_str[i]) == 0) {
			if (SND86_GET_IOPORT() != i) {
				snd86opt |= SND86_SET_IOPORT(i);
				snd86opt_mask |= SND86_MASK_IOPORT;
				renewal = TRUE;
			}
			break;
		}
	}
	for (i = 0; i < NELEMENTS(snd86_intr_str); i++) {
		if (strcmp(snd86_intr, snd86_intr_str[i]) == 0) {
			if (SND86_GET_INTR() != i) {
				snd86opt |= SND86_SET_INTR(i);
				snd86opt_mask |= SND86_MASK_INTR;
				renewal = TRUE;
			}
			break;
		}
	}
	for (i = 0; i < NELEMENTS(snd86_soundid_str); i++) {
		if (strcmp(snd86_soundid, snd86_soundid_str[i]) == 0) {
			if (SND86_GET_SOUNDID() != i) {
				snd86opt |= SND86_SET_SOUNDID(i);
				snd86opt_mask |= SND86_MASK_SOUNDID;
				renewal = TRUE;
			}
			break;
		}
	}
	if (SND86_GET_INTERRUPT() != snd86_interrupt) {
		snd86opt |= SND86_SET_INTERRUPT(i);
		snd86opt_mask |= SND86_MASK_INTERRUPT;
		renewal = TRUE;
	}
	if (SND86_GET_BIOSROM() != snd86_biosrom) {
		snd86opt |= SND86_SET_BIOSROM(i);
		snd86opt_mask |= SND86_MASK_BIOSROM;
		renewal = TRUE;
	}

	if (renewal) {
		np2cfg.snd86opt &= ~snd86opt_mask;
		np2cfg.snd86opt |= snd86opt;
		sysmng_update(SYS_UPDATECFG);
	}

	/* Speak board */
	spb_ioport = gtk_entry_get_text(GTK_ENTRY(spb_ioport_entry));
	spb_intr = gtk_entry_get_text(GTK_ENTRY(spb_int_entry));
	spb_romaddr = gtk_entry_get_text(GTK_ENTRY(spb_romaddr_entry));
	spb_vrl = (UINT8)gtk_adjustment_get_value(
	    GTK_ADJUSTMENT(spb_vr_level_adj));
	spb_x = gtk_toggle_button_get_active(
	    GTK_TOGGLE_BUTTON(spb_reverse_channel_checkbutton));

	renewal = FALSE;
	spbopt = spbopt_mask = 0;
	for (i = 0; i < NELEMENTS(spb_ioport_str); i++) {
		if (strcmp(spb_ioport, spb_ioport_str[i]) == 0) {
			if (SPB_GET_IOPORT() != i) {
				snd86opt |= SPB_SET_IOPORT(i);
				snd86opt_mask |= SPB_MASK_IOPORT;
				renewal = TRUE;
			}
			break;
		}
	}
	for (i = 0; i < NELEMENTS(spb_intr_str); i++) {
		if (strcmp(spb_intr, spb_intr_str[i]) == 0) {
			if (SPB_GET_INTR() != i) {
				spbopt |= SPB_SET_INTR(i);
				spbopt_mask |= SPB_MASK_INTR;
				renewal = TRUE;
			}
			break;
		}
	}
	for (i = 0; i < NELEMENTS(spb_romaddr_str); i++) {
		if (strcmp(spb_romaddr, spb_romaddr_str[i]) == 0) {
			if (SPB_GET_ROMADDR() != i) {
				spbopt |= SPB_SET_ROMADDR(i);
				spbopt_mask |= SPB_MASK_ROMADDR;
				renewal = TRUE;
			}
			break;
		}
	}
	spb_vrc = 0;
	for (i = 0; i < NELEMENTS(spb_vr_channel_str); i++) {
		spb_vrc |= gtk_toggle_button_get_active(
		    GTK_TOGGLE_BUTTON(spb_vr_channel_checkbutton[i]))
		    ? (1 << i) : 0;
	}
	if (np2cfg.spb_vrc != spb_vrc) {
		np2cfg.spb_vrc = spb_vrc;
		renewal = TRUE;
	}
	if (np2cfg.spb_vrl != spb_vrl) {
		np2cfg.spb_vrl = spb_vrl;
		renewal = TRUE;
	}
	if (np2cfg.spb_x != spb_x) {
		np2cfg.spb_x = !np2cfg.spb_x;
		renewal = TRUE;
	}

	if (renewal) {
		np2cfg.spbopt &= ~spbopt_mask;
		np2cfg.spbopt |= spbopt;
		opngen_setVR(np2cfg.spb_vrc, np2cfg.spb_vrl);
		sysmng_update(SYS_UPDATEOSCFG);
	}

	/* JoyPad */
	if (!(np2oscfg.JOYPAD1 & 2)) {
		joypad[0] = gtk_toggle_button_get_active(
		    GTK_TOGGLE_BUTTON(joypad_use_checkbutton[0]));

		renewal = FALSE;
		if ((np2oscfg.JOYPAD1 ^ joypad[0]) & 1) {
			np2oscfg.JOYPAD1 = joypad[0];
			renewal = TRUE;
		}

		joypad_device_index = 0;
		if (joypad_devlist == NULL) {
			for (i = 0; joypad_devlist[i] != NULL; ++i) {
				if (strcmp(joypad_devname, joypad_devlist[i]->devname) == 0) {
					joypad_device_index = joypad_devlist[i]->devindex;
					break;
				}
			}
		}
		g_snprintf(buf, sizeof(buf), "%d", joypad_device_index);
		if (strcmp(np2oscfg.JOYDEV[0], buf) != 0) {
			milstr_ncpy(np2oscfg.JOYDEV[0], buf, sizeof(np2oscfg.JOYDEV[0]));
			renewal = TRUE;
		}

		for (i = 0; i < JOY_NAXIS; ++i) {
			if (np2oscfg.JOYAXISMAP[0][i] != joypad_axis[i]) {
				memcpy(np2oscfg.JOYAXISMAP[0], joypad_axis, sizeof(np2oscfg.JOYAXISMAP[0]));
				renewal = TRUE;
				break;
			}
		}

		for (i = 0; i < JOY_NBUTTON; ++i) {
			if (np2oscfg.JOYBTNMAP[0][i] != joypad_button[i]) {
				memcpy(np2oscfg.JOYBTNMAP[0], joypad_button, sizeof(np2oscfg.JOYBTNMAP[0]));
				renewal = TRUE;
				break;
			}
		}

		if (renewal) {
			joymng_initialize();
			sysmng_update(SYS_UPDATEOSCFG);
		}
	}

	/* Driver */
	renewal = FALSE;
	if (np2oscfg.snddrv != driver_snddrv) {
		np2oscfg.snddrv = driver_snddrv;
		renewal = TRUE;
	}

	if (renewal) {
		sysmng_update(SYS_UPDATEOSCFG);
		soundrenewal = 1;
	}

	gtk_widget_destroy((GtkWidget *)d);
}

static void
dialog_destroy(GtkWidget *w, GtkWidget **wp)
{

	install_idle_process();
	gtk_widget_destroy(w);
}

static void
mixer_default_button_clicked(GtkButton *b, gpointer d)
{
	int i;

	for (i = 0; i < NELEMENTS(mixer_vol_tbl); i++) {
		gtk_adjustment_set_value(GTK_ADJUSTMENT(mixer_adj[i]), 64.0);
	}
}

static void
snd14_default_button_clicked(GtkButton *b, gpointer d)
{
	static const gfloat defval[NELEMENTS(snd14_adj)] = {
		12.0, 12.0, 8.0, 6.0, 3.0, 12.0
	};
	int i;

	for (i = 0; i < NELEMENTS(snd14_adj); i++) {
		gtk_adjustment_set_value(GTK_ADJUSTMENT(snd14_adj[i]), defval[i]);
	}
}

static void
snd26_default_button_clicked(GtkButton *b, gpointer d)
{

	gtk_entry_set_text(GTK_ENTRY(snd26_ioport_entry), "0188");
	gtk_entry_set_text(GTK_ENTRY(snd26_int_entry), "INT5");
	gtk_entry_set_text(GTK_ENTRY(snd26_romaddr_entry), "CC000");
}

static void
snd86_default_button_clicked(GtkButton *b, gpointer d)
{

	gtk_entry_set_text(GTK_ENTRY(snd86_ioport_entry), "0188");
	gtk_entry_set_text(GTK_ENTRY(snd86_int_entry), "INT5");
	gtk_entry_set_text(GTK_ENTRY(snd86_soundid_entry), "4x");
	if (!gtk_toggle_button_get_active(
	    GTK_TOGGLE_BUTTON(snd86_int_checkbutton)))
		g_signal_emit_by_name(G_OBJECT(snd86_int_checkbutton),
		    "clicked");
	if (!gtk_toggle_button_get_active(
	    GTK_TOGGLE_BUTTON(snd86_rom_checkbutton)))
		g_signal_emit_by_name(G_OBJECT(snd86_rom_checkbutton),
		    "clicked");
}

static void
spb_default_button_clicked(GtkButton *b, gpointer d)
{
	int i;

	gtk_entry_set_text(GTK_ENTRY(spb_ioport_entry), "0188");
	gtk_entry_set_text(GTK_ENTRY(spb_int_entry), "INT5");
	gtk_entry_set_text(GTK_ENTRY(spb_romaddr_entry), "CC000");
	for (i = 0; i < NELEMENTS(spb_vr_channel_str); i++) {
		if (gtk_toggle_button_get_active(
		    GTK_TOGGLE_BUTTON(spb_vr_channel_checkbutton[i])))
			g_signal_emit_by_name(
			    G_OBJECT(spb_vr_channel_checkbutton[i]), "clicked");
	}
}

static void
driver_radiobutton_clicked(GtkButton *b, gpointer d)
{

	driver_snddrv = GPOINTER_TO_UINT(d);
}

static GtkWidget *
create_mixer_note(void)
{
	GtkWidget *root_widget;
	GtkWidget *table;
	GtkWidget *vol_label[NELEMENTS(snd14_adj)];
	GtkWidget *vol_hscale[NELEMENTS(snd14_adj)];
	GtkWidget *mixer_default_button;
	GtkWidget *hbox;
	int i;

	root_widget = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(root_widget), 5);
	gtk_widget_show(root_widget);

	table = gtk_table_new(5, 4, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 5);
	gtk_box_pack_start(GTK_BOX(root_widget), table, FALSE, FALSE, 0);
	gtk_widget_show(table);

	for (i = 0; i < NELEMENTS(mixer_vol_tbl); i++) {
		vol_label[i] = gtk_label_new(mixer_vol_tbl[i].name);
		gtk_table_attach_defaults(GTK_TABLE(table), vol_label[i],
		    0, 1, i, i+1);
		gtk_widget_show(vol_label[i]);

		mixer_adj[i] = gtk_adjustment_new(*mixer_vol_tbl[i].valp,
		    mixer_vol_tbl[i].min, mixer_vol_tbl[i].max, 1.0, 1.0, 0.0);
		vol_hscale[i] = gtk_hscale_new(GTK_ADJUSTMENT(mixer_adj[i]));
		gtk_widget_show(vol_hscale[i]);
		gtk_scale_set_value_pos(GTK_SCALE(vol_hscale[i]),GTK_POS_RIGHT);
		gtk_scale_set_digits(GTK_SCALE(vol_hscale[i]), 0);
		gtk_table_attach_defaults(GTK_TABLE(table), vol_hscale[i],
		    1, 4, i, i+1);
	}

	/* "Default" button */
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 5);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(root_widget), hbox, TRUE, FALSE, 0);

	mixer_default_button = gtk_button_new_with_label("Default");
	gtk_widget_show(mixer_default_button);
	gtk_box_pack_end(GTK_BOX(hbox), mixer_default_button, FALSE, FALSE, 5);
	g_signal_connect_swapped(G_OBJECT(mixer_default_button), "clicked",
	    G_CALLBACK(mixer_default_button_clicked), NULL);

	return root_widget;
}

static GtkWidget *
create_pc9801_14_note(void)
{
	GtkWidget *root_widget;
	GtkWidget *table;
	GtkWidget *label[NELEMENTS(snd14_vol_str)];
	GtkWidget *scale[NELEMENTS(snd14_vol_str)];
	GtkWidget *snd14_default_button;
	GtkWidget *hbox;
	int i;

	root_widget = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(root_widget), 5);
	gtk_widget_show(root_widget);

	table = gtk_table_new(6, 4, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 5);
	gtk_box_pack_start(GTK_BOX(root_widget), table, FALSE, FALSE, 0);
	gtk_widget_show(table);

	for (i = 0; i < NELEMENTS(snd14_vol_str); i++) {
		label[i] = gtk_label_new(snd14_vol_str[i]);
		gtk_widget_show(label[i]);
		gtk_table_attach_defaults(GTK_TABLE(table), label[i], 0, 1, i, i+1);

		snd14_adj[i] = gtk_adjustment_new(np2cfg.vol14[i], 0.0, 15.0, 1.0, 1.0, 0.0);
		scale[i] = gtk_hscale_new(GTK_ADJUSTMENT(snd14_adj[i]));
		gtk_scale_set_default_values(GTK_SCALE(scale[i]));
		gtk_scale_set_digits(GTK_SCALE(scale[i]), 0);
		gtk_widget_show(scale[i]);
		gtk_table_attach_defaults(GTK_TABLE(table), scale[i], 1, 4, i, i+1);
	}

	/* "Default" button */
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 5);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(root_widget), hbox, TRUE, FALSE, 0);

	snd14_default_button = gtk_button_new_with_label("Default");
	gtk_widget_show(snd14_default_button);
	gtk_box_pack_end(GTK_BOX(hbox), snd14_default_button, FALSE, FALSE, 5);
	g_signal_connect_swapped(G_OBJECT(snd14_default_button), "clicked",
	    G_CALLBACK(snd14_default_button_clicked), NULL);

	return root_widget;
}

static GtkWidget*
create_pc9801_26_note(void)
{
	GtkWidget *root_widget;
	GtkWidget *table;
	GtkWidget *ioport_label;
	GtkWidget *ioport_combo;
	GtkWidget *int_label;
	GtkWidget *int_combo;
	GtkWidget *romaddr_label;
	GtkWidget *romaddr_combo;
	GtkWidget *snd26_default_button;
	GtkWidget *hbox;
	int i;

	root_widget = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(root_widget), 5);
	gtk_widget_show(root_widget);

	table = gtk_table_new(2, 4, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 5);
	gtk_table_set_col_spacings(GTK_TABLE(table), 5);
	gtk_box_pack_start(GTK_BOX(root_widget), table, FALSE, FALSE, 0);
	gtk_widget_show(table);

	/* I/O port */
	ioport_label = gtk_label_new("I/O port");
	gtk_widget_show(ioport_label);
	gtk_table_attach_defaults(GTK_TABLE(table), ioport_label, 0, 1, 0, 1);

	ioport_combo = gtk_combo_box_entry_new_text();
	gtk_widget_show(ioport_combo);
	gtk_table_attach_defaults(GTK_TABLE(table), ioport_combo, 1, 2, 0, 1);
	gtk_widget_set_size_request(ioport_combo, 80, -1);
	for (i = NELEMENTS(snd26_ioport_str) - 1; i >= 0; i--) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(ioport_combo), snd26_ioport_str[i]);
	}

	snd26_ioport_entry = gtk_bin_get_child(GTK_BIN(ioport_combo));
	gtk_widget_show(snd26_ioport_entry);
	gtk_editable_set_editable(GTK_EDITABLE(snd26_ioport_entry), FALSE);
	gtk_entry_set_text(GTK_ENTRY(snd26_ioport_entry), snd26_ioport_str[SND26_GET_IOPORT()]);

	/* interrupt */
	int_label = gtk_label_new("Interrupt");
	gtk_widget_show(int_label);
	gtk_table_attach_defaults(GTK_TABLE(table), int_label, 2, 3, 0, 1);

	int_combo = gtk_combo_box_entry_new_text();
	gtk_widget_show(int_combo);
	gtk_table_attach_defaults(GTK_TABLE(table), int_combo, 3, 4, 0, 1);
	gtk_widget_set_size_request(int_combo, 80, -1);
	for (i = 0; i < NELEMENTS(snd26_intr_str); i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(int_combo), snd26_intr_str[i]);
	}

	snd26_int_entry = gtk_bin_get_child(GTK_BIN(int_combo));
	gtk_widget_show(snd26_int_entry);
	gtk_editable_set_editable(GTK_EDITABLE(snd26_int_entry), FALSE);
	gtk_entry_set_text(GTK_ENTRY(snd26_int_entry), snd26_intr_str[SND26_GET_INTR()]);

	/* ROM address */
	romaddr_label = gtk_label_new("ROM");
	gtk_widget_show(romaddr_label);
	gtk_table_attach_defaults(GTK_TABLE(table), romaddr_label, 0, 1, 1, 2);

	romaddr_combo = gtk_combo_box_entry_new_text();
	gtk_widget_show(romaddr_combo);
	gtk_table_attach_defaults(GTK_TABLE(table), romaddr_combo, 1, 2, 1, 2);
	gtk_widget_set_size_request(romaddr_combo, 80, -1);
	for (i = 0; i < NELEMENTS(snd26_romaddr_str); i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(romaddr_combo), snd26_romaddr_str[i]);
	}

	snd26_romaddr_entry = gtk_bin_get_child(GTK_BIN(romaddr_combo));
	gtk_widget_show(snd26_romaddr_entry);
	gtk_editable_set_editable(GTK_EDITABLE(snd26_romaddr_entry), FALSE);
	gtk_entry_set_text(GTK_ENTRY(snd26_romaddr_entry), snd26_romaddr_str[SND26_GET_ROMADDR()]);

	/* "Default" button */
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 5);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(root_widget), hbox, FALSE, FALSE, 0);

	snd26_default_button = gtk_button_new_with_label("Default");
	gtk_widget_show(snd26_default_button);
	gtk_box_pack_end(GTK_BOX(hbox), snd26_default_button, FALSE, FALSE, 5);
	g_signal_connect_swapped(G_OBJECT(snd26_default_button), "clicked",
	    G_CALLBACK(snd26_default_button_clicked), NULL);

	return root_widget;
}

static GtkWidget *
create_pc9801_86_note(void)
{
	GtkWidget *root_widget;
	GtkWidget *table;
	GtkWidget *ioport_label;
	GtkWidget *ioport_combo;
	GtkWidget *int_combo;
	GtkWidget *soundid_label;
	GtkWidget *soundid_combo;
	GtkWidget *snd86_default_button;
	GtkWidget *hbox;
	int i;

	root_widget = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(root_widget), 5);
	gtk_widget_show(root_widget);

	table = gtk_table_new(2, 5, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 5);
	gtk_table_set_col_spacings(GTK_TABLE(table), 5);
	gtk_box_pack_start(GTK_BOX(root_widget), table, FALSE, FALSE, 0);
	gtk_widget_show(table);

	/* I/O port */
	ioport_label = gtk_label_new("I/O port");
	gtk_widget_show(ioport_label);
	gtk_table_attach_defaults(GTK_TABLE(table), ioport_label, 0, 1, 0, 1);

	ioport_combo = gtk_combo_box_entry_new_text();
	gtk_widget_show(ioport_combo);
	gtk_table_attach_defaults(GTK_TABLE(table), ioport_combo, 1, 2, 0, 1);
	gtk_widget_set_size_request(ioport_combo, 80, -1);
	for (i = NELEMENTS(snd86_ioport_str) - 1; i >= 0; i--) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(ioport_combo), snd86_ioport_str[i]);
	}

	snd86_ioport_entry = gtk_bin_get_child(GTK_BIN(ioport_combo));
	gtk_widget_show(snd86_ioport_entry);
	gtk_editable_set_editable(GTK_EDITABLE(snd86_ioport_entry), FALSE);
	gtk_entry_set_text(GTK_ENTRY(snd86_ioport_entry), snd86_ioport_str[SND86_GET_IOPORT()]);

	/* interrupt */
	snd86_int_checkbutton = gtk_check_button_new_with_label("Interrupt");
	gtk_widget_show(snd86_int_checkbutton);
	gtk_table_attach_defaults(GTK_TABLE(table), snd86_int_checkbutton, 2, 3, 0, 1);
	if (SND86_GET_INTERRUPT())
		g_signal_emit_by_name(G_OBJECT(snd86_int_checkbutton), "clicked");

	int_combo = gtk_combo_box_entry_new_text();
	gtk_widget_show(int_combo);
	gtk_table_attach_defaults(GTK_TABLE(table), int_combo, 3, 4, 0, 1);
	gtk_widget_set_size_request(int_combo, 80, -1);
	for (i = 0; i < NELEMENTS(snd86_intr_str); i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(int_combo), snd86_intr_str[i]);
	}

	snd86_int_entry = gtk_bin_get_child(GTK_BIN(int_combo));
	gtk_widget_show(snd86_int_entry);
	gtk_editable_set_editable(GTK_EDITABLE(snd86_int_entry), FALSE);
	gtk_entry_set_text(GTK_ENTRY(snd86_int_entry), snd86_intr_str[SND86_GET_INTR()]);

	/* Sound ID */
	soundid_label = gtk_label_new("Sound ID");
	gtk_widget_show(soundid_label);
	gtk_table_attach_defaults(GTK_TABLE(table), soundid_label, 0, 1, 1, 2);

	soundid_combo = gtk_combo_box_entry_new_text();
	gtk_widget_show(soundid_combo);
	gtk_table_attach_defaults(GTK_TABLE(table), soundid_combo, 1, 2, 1, 2);
	gtk_widget_set_size_request(soundid_combo, 80, -1);
	for (i = NELEMENTS(snd86_soundid_str) - 1; i >= 0; i--) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(soundid_combo), snd86_soundid_str[i]);
	}

	snd86_soundid_entry = gtk_bin_get_child(GTK_BIN(soundid_combo));
	gtk_widget_show(snd86_soundid_entry);
	gtk_editable_set_editable(GTK_EDITABLE(snd86_soundid_entry), FALSE);
	gtk_entry_set_text(GTK_ENTRY(snd86_soundid_entry), snd86_soundid_str[SND86_GET_SOUNDID()]);

	/* ROM */
	snd86_rom_checkbutton = gtk_check_button_new_with_label("ROM");
	gtk_widget_show(snd86_rom_checkbutton);
	gtk_table_attach_defaults(GTK_TABLE(table), snd86_rom_checkbutton, 2, 3, 1, 2);
	if (SND86_GET_BIOSROM())
		g_signal_emit_by_name(G_OBJECT(snd86_rom_checkbutton), "clicked");

	/* "Default" button */
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 5);
	gtk_box_pack_start(GTK_BOX(root_widget), hbox, FALSE, FALSE, 0);

	snd86_default_button = gtk_button_new_with_label("Default");
	gtk_widget_show(snd86_default_button);
	gtk_box_pack_end(GTK_BOX(hbox), snd86_default_button, FALSE, FALSE, 5);
	g_signal_connect_swapped(G_OBJECT(snd86_default_button), "clicked",
	    G_CALLBACK(snd86_default_button_clicked), NULL);

	return root_widget;
}

static GtkWidget *
create_spb_note(void)
{
	GtkWidget *root_widget;
	GtkWidget *table;
	GtkWidget *ioport_label;
	GtkWidget *ioport_combo;
	GtkWidget *int_label;
	GtkWidget *int_combo;
	GtkWidget *romaddr_label;
	GtkWidget *romaddr_combo;
	GtkWidget *spb_default_button;
	GtkWidget *vr_label;
	GtkWidget *vr_level_label;
	GtkWidget *vr_level_scale;
	GtkWidget *hbox;
	int i;

	root_widget = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(root_widget), 5);
	gtk_widget_show(root_widget);

	table = gtk_table_new(2, 6, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 5);
	gtk_table_set_col_spacings(GTK_TABLE(table), 5);
	gtk_box_pack_start(GTK_BOX(root_widget), table, FALSE, FALSE, 0);
	gtk_widget_show(table);

	/* I/O port */
	ioport_label = gtk_label_new("I/O port");
	gtk_widget_show(ioport_label);
	gtk_table_attach_defaults(GTK_TABLE(table), ioport_label, 0, 1, 0, 1);

	ioport_combo = gtk_combo_box_entry_new_text();
	gtk_widget_show(ioport_combo);
	gtk_table_attach_defaults(GTK_TABLE(table), ioport_combo, 1, 3, 0, 1);
	gtk_widget_set_size_request(ioport_combo, 80, -1);
	for (i = NELEMENTS(spb_ioport_str) - 1; i >= 0; i--) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(ioport_combo), spb_ioport_str[i]);
	}

	spb_ioport_entry = gtk_bin_get_child(GTK_BIN(ioport_combo));
	gtk_widget_show(spb_ioport_entry);
	gtk_editable_set_editable(GTK_EDITABLE(spb_ioport_entry), FALSE);
	gtk_entry_set_text(GTK_ENTRY(spb_ioport_entry), spb_ioport_str[SPB_GET_IOPORT()]);

	/* interrupt */
	int_label = gtk_label_new("Interrupt");
	gtk_widget_show(int_label);
	gtk_table_attach_defaults(GTK_TABLE(table), int_label, 3, 4, 0, 1);

	int_combo = gtk_combo_box_entry_new_text();
	gtk_widget_show(int_combo);
	gtk_table_attach_defaults(GTK_TABLE(table), int_combo, 4, 6, 0, 1);
	gtk_widget_set_size_request(int_combo, 80, -1);
	for (i = 0; i < NELEMENTS(spb_intr_str); i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(int_combo), spb_intr_str[i]);
	}

	spb_int_entry = gtk_bin_get_child(GTK_BIN(int_combo));
	gtk_widget_show(spb_int_entry);
	gtk_editable_set_editable(GTK_EDITABLE(spb_int_entry), FALSE);
	gtk_entry_set_text(GTK_ENTRY(spb_int_entry), spb_intr_str[SPB_GET_INTR()]);

	/* ROM address */
	romaddr_label = gtk_label_new("ROM");
	gtk_widget_show(romaddr_label);
	gtk_table_attach_defaults(GTK_TABLE(table), romaddr_label, 0, 1, 1, 2);

	romaddr_combo = gtk_combo_box_entry_new_text();
	gtk_widget_show(romaddr_combo);
	gtk_table_attach_defaults(GTK_TABLE(table), romaddr_combo, 1, 3, 1, 2);
	gtk_widget_set_size_request(romaddr_combo, 80, -1);
	for (i = 0; i < NELEMENTS(spb_romaddr_str); i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(romaddr_combo), spb_romaddr_str[i]);
	}

	spb_romaddr_entry = gtk_bin_get_child(GTK_BIN(romaddr_combo));
	gtk_widget_show(spb_romaddr_entry);
	gtk_editable_set_editable(GTK_EDITABLE(spb_romaddr_entry), FALSE);
	gtk_entry_set_text(GTK_ENTRY(spb_romaddr_entry), spb_romaddr_str[SPB_GET_ROMADDR()]);

	/* VR */
	vr_label = gtk_label_new("VR");
	gtk_widget_show(vr_label);
	gtk_table_attach_defaults(GTK_TABLE(table), vr_label, 0, 1, 2, 3);

	for (i = 0; i < NELEMENTS(spb_vr_channel_str); i++) {
		spb_vr_channel_checkbutton[i] = gtk_check_button_new_with_label(spb_vr_channel_str[i]);
		gtk_widget_show(spb_vr_channel_checkbutton[i]);
		gtk_table_attach_defaults(GTK_TABLE(table), spb_vr_channel_checkbutton[i], i+1, i+2, 2, 3);
		if (np2cfg.spb_vrc & (1 << i))
			g_signal_emit_by_name(G_OBJECT(spb_vr_channel_checkbutton[i]), "clicked");
	}

	vr_level_label = gtk_label_new("level");
	gtk_widget_show(vr_level_label);
	gtk_table_attach_defaults(GTK_TABLE(table), vr_level_label, 3, 4, 2, 3);

	spb_vr_level_adj = gtk_adjustment_new(np2cfg.spb_vrl, 0.0, 24.0, 1.0, 1.0, 0.0);
	vr_level_scale = gtk_hscale_new(GTK_ADJUSTMENT(spb_vr_level_adj));
	gtk_scale_set_default_values(GTK_SCALE(vr_level_scale));
	gtk_scale_set_digits(GTK_SCALE(vr_level_scale), 0);
	gtk_scale_set_draw_value(GTK_SCALE(vr_level_scale), FALSE);
	gtk_widget_show(vr_level_scale);
	gtk_table_attach_defaults(GTK_TABLE(table), vr_level_scale, 4, 6, 2, 3);

	spb_reverse_channel_checkbutton = gtk_check_button_new_with_label("Reversed channel (SPB default)");
	gtk_widget_show(spb_reverse_channel_checkbutton);
	gtk_table_attach_defaults(GTK_TABLE(table), spb_reverse_channel_checkbutton, 0, 6, 3, 4);
	if (np2cfg.spb_x)
		g_signal_emit_by_name(G_OBJECT(spb_reverse_channel_checkbutton), "clicked");

	/* "Default" button */
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 5);
	gtk_box_pack_start(GTK_BOX(root_widget), hbox, FALSE, FALSE, 0);

	spb_default_button = gtk_button_new_with_label("Default");
	gtk_widget_show(spb_default_button);
	gtk_box_pack_end(GTK_BOX(hbox), spb_default_button, FALSE, FALSE, 5);
	g_signal_connect_swapped(G_OBJECT(spb_default_button), "clicked",
	    G_CALLBACK(spb_default_button_clicked), NULL);

	return root_widget;
}

static void
joypad_device_changed(GtkEditable *e, gpointer d)
{
	GtkWidget *axis_entry[JOY_NAXIS];
	GtkWidget *button_entry[JOY_NBUTTON];
	const gchar *dvname;
	int drv;
	int i, j;

	dvname = gtk_entry_get_text(GTK_ENTRY(e));
	if ((joypad_devlist == NULL)
	 || (dvname == NULL)
	 || (strcmp(dvname, joypad_nodevice_str) == 0)) {
		milstr_ncpy(joypad_devname, joypad_nodevice_str, sizeof(joypad_devname));
		return;
	}

	for (drv = 0; joypad_devlist[drv] != NULL; ++drv) {
		if (strcmp(dvname, joypad_devlist[drv]->devname) == 0) {
			break;
		}
	}
	if (joypad_devlist[drv] == NULL) {
		drv = 0;
		if (joypad_devlist[drv] == NULL) {
			milstr_ncpy(joypad_devname, joypad_nodevice_str, sizeof(joypad_devname));
			return;
		}
	}
	milstr_ncpy(joypad_devname, joypad_devlist[drv]->devname, sizeof(joypad_devname));

	/* Axis */
	for (i = 0; i < JOY_NAXIS; ++i) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(joypad_axis_combo[i]), joypad_noconnect_str);
		for (j = 0; j < joypad_devlist[drv]->naxis; j++) {
			gtk_combo_box_append_text(GTK_COMBO_BOX(joypad_axis_combo[i]), joypad_num_str[j]);
		}

		axis_entry[i] = gtk_bin_get_child(GTK_BIN(joypad_axis_combo[i]));
		gtk_widget_show(axis_entry[i]);
		gtk_editable_set_editable(GTK_EDITABLE(axis_entry[i]), FALSE);

		if (np2oscfg.JOYAXISMAP[0][i] < joypad_devlist[drv]->naxis) {
			gtk_entry_set_text(GTK_ENTRY(axis_entry[i]), joypad_num_str[np2oscfg.JOYAXISMAP[0][i]]);
		} else {
			gtk_entry_set_text(GTK_ENTRY(axis_entry[i]), joypad_noconnect_str);
		}
	}

	/* Button */
	for (i = 0; i < JOY_NBUTTON; ++i) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(joypad_button_combo[i]), joypad_noconnect_str);
		for (j = 0; j < joypad_devlist[drv]->nbutton; j++) {
			gtk_combo_box_append_text(GTK_COMBO_BOX(joypad_button_combo[i]), joypad_num_str[j]);
		}

		button_entry[i] = gtk_bin_get_child(GTK_BIN(joypad_button_combo[i]));
		gtk_widget_show(button_entry[i]);
		gtk_editable_set_editable(GTK_EDITABLE(button_entry[i]), FALSE);

		if (np2oscfg.JOYBTNMAP[0][i] < joypad_devlist[drv]->nbutton) {
			gtk_entry_set_text(GTK_ENTRY(button_entry[i]), joypad_num_str[np2oscfg.JOYBTNMAP[0][i]]);
		} else {
			gtk_entry_set_text(GTK_ENTRY(button_entry[i]), joypad_noconnect_str);
		}
	}
}

static void
joypad_axis_entry_changed(GtkEditable *e, gpointer d)
{
	const gchar *str = gtk_entry_get_text(GTK_ENTRY(e));
	UINT8 *p = (UINT8 *)d;

	if (strcmp(str, joypad_noconnect_str) == 0) {
		*p = JOY_AXIS_INVALID;
	} else {
		*p = milstr_solveINT(str);
	}
}

static void
joypad_button_entry_changed(GtkEditable *e, gpointer d)
{
	const gchar *str = gtk_entry_get_text(GTK_ENTRY(e));
	UINT8 *p = (UINT8 *)d;

	if (strcmp(str, joypad_noconnect_str) == 0) {
		*p = JOY_BUTTON_INVALID;
	} else {
		*p = milstr_solveINT(str);
	}
}

static GtkWidget *
create_joypad_note(void)
{
	char buf[32];
	GtkWidget *root_widget;
	GtkWidget *table;
	GtkWidget *devlist_label;
	GtkWidget *devlist_entry;
	GtkWidget *axis_label[JOY_NAXIS];
	GtkWidget *axis_entry[JOY_NAXIS];
	GtkWidget *button_label[JOY_NBUTTON];
	GtkWidget *button_entry[JOY_NBUTTON];
	int ndrv, drv;
	int i;

	root_widget = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(root_widget), 5);
	gtk_widget_show(root_widget);

	table = gtk_table_new(3, 8, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 3);
	gtk_table_set_col_spacings(GTK_TABLE(table), 5);
	gtk_box_pack_start(GTK_BOX(root_widget), table, FALSE, FALSE, 0);
	gtk_widget_show(table);

	/* Use JoyPad-1 */
	joypad_use_checkbutton[0] = gtk_check_button_new_with_label("Use JoyPad-1");
	gtk_widget_show(joypad_use_checkbutton[0]);
	gtk_table_attach_defaults(GTK_TABLE(table), joypad_use_checkbutton[0], 0, 3, 0, 1);

	/* Device */
	devlist_label = gtk_label_new("Device");
	gtk_widget_show(devlist_label);
	gtk_table_attach_defaults(GTK_TABLE(table), devlist_label, 0, 1, 1, 2);

	joypad_devlist_combo = gtk_combo_box_entry_new_text();
	gtk_widget_show(joypad_devlist_combo);
	gtk_table_attach_defaults(GTK_TABLE(table), joypad_devlist_combo, 1, 3, 1, 2);

	joypad_devlist = joymng_get_devinfo_list();
	if (joypad_devlist != NULL) {
		for (i = 0; joypad_devlist[i] != NULL; ++i) {
			gtk_combo_box_append_text(GTK_COMBO_BOX(joypad_devlist_combo), joypad_devlist[i]->devname);
		}
		ndrv = i;
	} else {
		gtk_combo_box_append_text(GTK_COMBO_BOX(joypad_devlist_combo), joypad_nodevice_str);
		ndrv = 0;
	}

	devlist_entry = gtk_bin_get_child(GTK_BIN(joypad_devlist_combo));
	gtk_widget_show(devlist_entry);
	gtk_editable_set_editable(GTK_EDITABLE(devlist_entry), FALSE);
	g_signal_connect(G_OBJECT(devlist_entry), "changed",
	    G_CALLBACK(joypad_device_changed), (gpointer)joypad_devlist_combo);

	/* Axis */
	for (i = 0; i < JOY_NAXIS; ++i) {
		g_snprintf(buf, sizeof(buf), "%c axis", 'X' + i);
		axis_label[i] = gtk_label_new(buf);
		gtk_widget_show(axis_label[i]);
		gtk_table_attach_defaults(GTK_TABLE(table), axis_label[i], 0, 1, 2+i, 3+i);

		joypad_axis_combo[i] = gtk_combo_box_entry_new_text();
		gtk_widget_show(joypad_axis_combo[i]);
		gtk_table_attach_defaults(GTK_TABLE(table), joypad_axis_combo[i], 1, 2, 2+i, 3+i);
		gtk_widget_set_size_request(joypad_axis_combo[i], 48, -1);

		gtk_combo_box_append_text(GTK_COMBO_BOX(joypad_axis_combo[i]), joypad_noconnect_str);

		axis_entry[i] = gtk_bin_get_child(GTK_BIN(joypad_axis_combo[i]));
		gtk_widget_show(axis_entry[i]);
		gtk_editable_set_editable(GTK_EDITABLE(axis_entry[i]), FALSE);
		gtk_entry_set_text(GTK_ENTRY(axis_entry[i]), joypad_noconnect_str);
		g_signal_connect(G_OBJECT(axis_entry[i]), "changed",
		    G_CALLBACK(joypad_axis_entry_changed),
		    (gpointer)(&joypad_axis[i]));
	}

	/* Button */
	for (i = 0; i < JOY_NBUTTON; ++i) {
		g_snprintf(buf, sizeof(buf), "%sButton%d", (i >= JOY_NBUTTON / 2) ? "Rapid " : "", i % 2);
		button_label[i] = gtk_label_new(buf);
		gtk_widget_show(button_label[i]);
		gtk_table_attach_defaults(GTK_TABLE(table), button_label[i], 0, 1, 4+i, 5+i);

		joypad_button_combo[i] = gtk_combo_box_entry_new_text();
		gtk_widget_show(joypad_button_combo[i]);
		gtk_table_attach_defaults(GTK_TABLE(table), joypad_button_combo[i], 1, 2, 4+i, 5+i);
		gtk_widget_set_size_request(joypad_button_combo[i], 48, -1);

		gtk_combo_box_append_text(GTK_COMBO_BOX(joypad_button_combo[i]), joypad_noconnect_str);

		button_entry[i] = gtk_bin_get_child(GTK_BIN(joypad_button_combo[i]));
		gtk_widget_show(button_entry[i]);
		gtk_editable_set_editable(GTK_EDITABLE(button_entry[i]), FALSE);
		gtk_entry_set_text(GTK_ENTRY(button_entry[i]), joypad_noconnect_str);
		g_signal_connect(G_OBJECT(button_entry[i]), "changed",
		    G_CALLBACK(joypad_button_entry_changed),
		    (gpointer)(&joypad_button[i]));
	}

	/* no joystick device */
	if ((np2oscfg.JOYPAD1 & 2) && (joypad_devlist == NULL)) {
		gtk_widget_set_sensitive(joypad_use_checkbutton[0], FALSE);
		gtk_widget_set_sensitive(joypad_devlist_combo, FALSE);
		for (i = 0; i < JOY_NAXIS; ++i) {
			gtk_widget_set_sensitive(joypad_axis_combo[i], FALSE);
		}
		for (i = 0; i < JOY_NBUTTON; ++i) {
			gtk_widget_set_sensitive(joypad_button_combo[i], FALSE);
		}
	}

	/* update status */
	if (joypad_devlist != NULL) {
		drv = milstr_solveINT(np2oscfg.JOYDEV[0]);
		if (drv < 0 || drv >= ndrv) {
			drv = 0;
		}
		gtk_entry_set_text(GTK_ENTRY(devlist_entry), joypad_devlist[drv]->devname);
	} else {
		gtk_entry_set_text(GTK_ENTRY(devlist_entry), joypad_nodevice_str);
	}
	if (np2oscfg.JOYPAD1 & 1) {
		g_signal_emit_by_name(G_OBJECT(joypad_use_checkbutton[0]), "clicked");
	}

	return root_widget;
}

static GtkWidget *
create_driver_note(void)
{
	GtkWidget *root_widget;
	GtkWidget *driver_frame;
	GtkWidget *driver_vbox;
	GtkWidget *driver_radiobutton[SNDDRV_DRVMAX];
	GtkWidget *snddrv_hbox;
	int i;

	root_widget = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(root_widget), 5);
	gtk_widget_show(root_widget);

	driver_frame = gtk_frame_new("Sound driver");
	gtk_widget_show(driver_frame);
	gtk_box_pack_start(GTK_BOX(root_widget), driver_frame, TRUE, TRUE, 0);

	driver_vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(driver_vbox), 5);
	gtk_widget_show(driver_vbox);
	gtk_container_add(GTK_CONTAINER(driver_frame), driver_vbox);

	for (i = 0; i < SNDDRV_DRVMAX; i++) {
		driver_radiobutton[i] = gtk_radio_button_new_with_label_from_widget(i > 0 ? GTK_RADIO_BUTTON(driver_radiobutton[i-1]) : NULL, driver_name[i]);
		gtk_widget_show(driver_radiobutton[i]);
		gtk_box_pack_start(GTK_BOX(driver_vbox), driver_radiobutton[i], TRUE, FALSE, 0);
		g_signal_connect(G_OBJECT(driver_radiobutton[i]), "clicked",
		    G_CALLBACK(driver_radiobutton_clicked), GINT_TO_POINTER(i));
	}
#if !defined(USE_SDLAUDIO) && !defined(USE_SDLMIXER)
	gtk_widget_set_sensitive(driver_radiobutton[SNDDRV_SDL], FALSE);
#endif

	switch (np2oscfg.snddrv) {
	case SNDDRV_NODRV:
#if defined(USE_SDLAUDIO) || defined(USE_SDLMIXER)
	case SNDDRV_SDL:
#endif
		g_signal_emit_by_name(G_OBJECT(driver_radiobutton[np2oscfg.snddrv]), "clicked");
		break;

#if !defined(USE_SDLAUDIO) && !defined(USE_SDLMIXER)
	case SNDDRV_SDL:
#endif
	case SNDDRV_DRVMAX:
	default:
		np2oscfg.snddrv = SNDDRV_NODRV;
		sysmng_update(SYS_UPDATEOSCFG);
		break;
	}

	snddrv_hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(snddrv_hbox), 5);
	gtk_widget_show(snddrv_hbox);
	gtk_box_pack_start(GTK_BOX(root_widget), snddrv_hbox, FALSE, FALSE, 0);

	return root_widget;
}

void
create_sound_dialog(void)
{
	GtkWidget *sound_dialog;
	GtkWidget *main_vbox;
	GtkWidget *notebook;
	GtkWidget *mixer_note;
	GtkWidget *pc9801_14_note;
	GtkWidget *pc9801_26_note;
	GtkWidget *pc9801_86_note;
	GtkWidget *spb_note;
	GtkWidget *joypad_note;
	GtkWidget *driver_note;
	GtkWidget *confirm_widget;
	GtkWidget *ok_button;
	GtkWidget *cancel_button;

	uninstall_idle_process();

	sound_dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(sound_dialog), "Sound option");
	gtk_window_set_position(GTK_WINDOW(sound_dialog), GTK_WIN_POS_CENTER);
	gtk_window_set_modal(GTK_WINDOW(sound_dialog), TRUE);
	gtk_window_set_resizable(GTK_WINDOW(sound_dialog), FALSE);

	g_signal_connect(G_OBJECT(sound_dialog), "destroy",
	    G_CALLBACK(dialog_destroy), NULL);

	main_vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(main_vbox);
	gtk_container_add(GTK_CONTAINER(sound_dialog), main_vbox);

	notebook = gtk_notebook_new();
	gtk_widget_show(notebook);
	gtk_box_pack_start(GTK_BOX(main_vbox), notebook, TRUE, TRUE, 0);

	/* "Mixer" note */
	mixer_note = create_mixer_note();
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), mixer_note, gtk_label_new("Mixer"));

	/* "PC-9801-14" note */
	pc9801_14_note = create_pc9801_14_note();
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), pc9801_14_note, gtk_label_new("PC-9801-14"));

	/* "26" note */
	pc9801_26_note = create_pc9801_26_note();
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), pc9801_26_note, gtk_label_new("26"));

	/* "86" note */
	pc9801_86_note = create_pc9801_86_note();
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), pc9801_86_note, gtk_label_new("86"));

	/* "SPB" note */
	spb_note = create_spb_note();
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), spb_note, gtk_label_new("SPB"));

	/* "JoyPad" note */
	joypad_note = create_joypad_note();
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), joypad_note, gtk_label_new("JoyPad"));

	/* "Driver" note */
	driver_note = create_driver_note();
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), driver_note, gtk_label_new("Driver"));

	/*
	 * OK, Cancel button
	 */
	confirm_widget = gtk_hbox_new(FALSE, 5);
	gtk_widget_show(confirm_widget);
	gtk_box_pack_start(GTK_BOX(main_vbox), confirm_widget, FALSE, FALSE, 5);

	cancel_button = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	gtk_widget_show(cancel_button);
	gtk_box_pack_end(GTK_BOX(confirm_widget), cancel_button, FALSE, FALSE, 0);
#if GTK_MAJOR_VERSION > 2 || (GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION >= 18)
	gtk_widget_set_can_default(cancel_button, TRUE);
#else
	GTK_WIDGET_SET_FLAGS(cancel_button, GTK_CAN_DEFAULT);
#endif
	g_signal_connect_swapped(G_OBJECT(cancel_button), "clicked",
	    G_CALLBACK(gtk_widget_destroy), G_OBJECT(sound_dialog));

	ok_button = gtk_button_new_from_stock(GTK_STOCK_OK);
	gtk_widget_show(ok_button);
	gtk_box_pack_end(GTK_BOX(confirm_widget), ok_button, FALSE, FALSE, 0);
#if GTK_MAJOR_VERSION > 2 || (GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION >= 18)
	gtk_widget_set_can_default(ok_button, TRUE);
	gtk_widget_has_default(ok_button);
#else
	GTK_WIDGET_SET_FLAGS(ok_button, GTK_CAN_DEFAULT);
	GTK_WIDGET_SET_FLAGS(ok_button, GTK_HAS_DEFAULT);
#endif
	g_signal_connect(G_OBJECT(ok_button), "clicked",
	    G_CALLBACK(ok_button_clicked), (gpointer)sound_dialog);
	gtk_widget_grab_default(ok_button);

	gtk_widget_show_all(sound_dialog);
}
