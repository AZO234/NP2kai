/*
 * Copyright (c) 2002-2003 NONAKA Kimihiro
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

#include "np2.h"
#include "pccore.h"

#include "sysmng.h"

#include "gtk2/xnp2.h"
#include "gtk2/gtk_menu.h"
#if defined(CPUCORE_IA32)
#include "i386c/ia32/cpu.h"
#endif


static const char *baseclock_str[] = {
	"1.9968MHz", "2.4576MHz"
};

static const char *clockmult_str[] = {
	"1", "2", "4", "5", "6", "8", "10", "12", "16", "20", "24", "30", "32", "34", "36", "40", "42", "52", "64", "76", "88", "100"
};

#if defined(CPUCORE_IA32)
static const char *cputype_str[] = {
	"(custom)", "Intel 80386", "Intel i486SX", "Intel i486DX", "Intel Pentium", "Intel MMX Pentium", "Intel Pentium Pro", "Intel Pentium II", "Intel Pentium III", "Intel Pentium M", "Intel Pentium 4", "AMD K6-2", "AMD K6-III", "AMD K7 Athlon", "AMD K7 Athlon XP", "Neko Processor II"
};
#endif

static const char *samplingrate_str[] = {
	"11025", "22050", "44100", "48000", "88200", "96000", "176400", "192000"
};

static const struct {
	const char*	label;
	const char*	arch;
} architecture[] = {
	{ "PC-9801VM", "VM" },
	{ "PC-9801VX", "VX" },
	{ "PC-286", "EPSON" },
};

static GtkWidget *baseclock_entry;
static GtkWidget *clockmult_entry;
static GtkWidget *samplingrate_entry;
static GtkWidget *cputype_entry;
static GtkWidget *buffer_entry;
static GtkWidget *always16bio_checkbutton;
#if defined(SUPPORT_RESUME)
static GtkWidget *resume_checkbutton;
#endif
#if defined(GCC_CPU_ARCH_IA32)
static GtkWidget *disablemmx_checkbutton;
#endif
static const char *arch;
static int rate;

static void
ok_button_clicked(GtkButton *b, gpointer d)
{
	const gchar *bufp = gtk_entry_get_text(GTK_ENTRY(buffer_entry));
	const gchar *base = gtk_entry_get_text(GTK_ENTRY(baseclock_entry));
	const gchar *multp = gtk_entry_get_text(GTK_ENTRY(clockmult_entry));
	const gchar *cputype = gtk_entry_get_text(GTK_ENTRY(cputype_entry));
#if defined(SUPPORT_RESUME)
	gint resume = gtk_toggle_button_get_active(
	    GTK_TOGGLE_BUTTON(resume_checkbutton));
#endif
#if defined(GCC_CPU_ARCH_IA32)
	gint disablemmx = gtk_toggle_button_get_active(
	    GTK_TOGGLE_BUTTON(disablemmx_checkbutton));
#endif
	gint always16bio = gtk_toggle_button_get_active(
	    GTK_TOGGLE_BUTTON(always16bio_checkbutton));
	guint bufsize;
	guint mult;
	UINT renewal = 0;
	int i;
	UINT16 always16bio_temp;

	if (strcmp(base, "1.9968MHz") == 0) {
		if (np2cfg.baseclock != PCBASECLOCK20) {
			np2cfg.baseclock = PCBASECLOCK20;
			renewal |= SYS_UPDATECFG|SYS_UPDATECLOCK;
		}
	} else {
		if (np2cfg.baseclock != PCBASECLOCK25) {
			np2cfg.baseclock = PCBASECLOCK25;
			renewal |= SYS_UPDATECFG|SYS_UPDATECLOCK;
		}
	}

	mult = milstr_solveINT(multp);
	switch (mult) {
	case 1: case 2: case 4: case 5: case 6: case 8: case 10: case 12:
	case 16: case 20: case 24: case 30: case 32: case 34: case 36: case 40: case 42:
	case 52: case 64: case 76: case 88: case 100:
		if (mult != np2cfg.multiple) {
			np2cfg.multiple = mult;
			renewal |= SYS_UPDATECFG|SYS_UPDATECLOCK;
		}
		break;
	}

#if defined(CPUCORE_IA32)
	for (i = 0; i < NELEMENTS(cputype_str); i++) {
		if(strcmp(cputype, cputype_str[i]) == 0) {
			switch(i) {
			case 0:
			case 1:
			case 2:
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
			case 9:
			case 10:
				SetCpuTypeIndex(i);
				break;
			case 11:
				SetCpuTypeIndex(15);
				break;
			case 12:
				SetCpuTypeIndex(16);
				break;
			case 13:
				SetCpuTypeIndex(17);
				break;
			case 14:
				SetCpuTypeIndex(18);
				break;
			case 15:
				SetCpuTypeIndex(255);
				break;
			}
			break;
		}
	}
#endif

	for (i = 0; i < NELEMENTS(architecture); i++) {
		if (strcmp(arch, architecture[i].arch) == 0) {
			milstr_ncpy(np2cfg.model, arch, sizeof(np2cfg.model));
			renewal |= SYS_UPDATECFG;
			break;
		}
	}
	if (i == NELEMENTS(architecture)) {
		milstr_ncpy(np2cfg.model, "VX", sizeof(np2cfg.model));
		renewal |= SYS_UPDATECFG;
	}

	if(always16bio) {
		always16bio_temp = 0xFF00;
	} else {
		always16bio_temp = 0;
	}
	if(np2cfg.sysiomsk != always16bio_temp) {
		np2cfg.sysiomsk = always16bio_temp;
		renewal |= SYS_UPDATECFG;
	}

	switch (rate) {
	case 11025:
	case 22050:
	case 44100:
	case 48000:
	case 88200:
	case 96000:
	case 176400:
	case 192000:
		if (rate != np2cfg.samplingrate) {
			np2cfg.samplingrate = rate;
			renewal |= SYS_UPDATECFG|SYS_UPDATERATE;
			soundrenewal = 1;
		}
		break;
	}

	bufsize = milstr_solveINT(bufp);
	if (bufsize < 20)
		bufsize = 20;
	else if (bufsize > 1000)
		bufsize = 1000;
	if (np2cfg.delayms != bufsize) {
		np2cfg.delayms = bufsize;
		renewal |= SYS_UPDATECFG|SYS_UPDATESBUF;
		soundrenewal = 1;
	}

#if defined(GCC_CPU_ARCH_IA32)
	if (!(mmxflag & MMXFLAG_NOTSUPPORT)) {
		disablemmx = disablemmx ? MMXFLAG_DISABLE : 0;
		if (np2oscfg.disablemmx != disablemmx) {
			np2oscfg.disablemmx = disablemmx;
			mmxflag &= ~MMXFLAG_DISABLE;
			mmxflag |= disablemmx;
			renewal |= SYS_UPDATEOSCFG;
		}
	}
#endif

#if defined(SUPPORT_RESUME)
	if (np2oscfg.resume != resume) {
		np2oscfg.resume = resume;
		renewal |= SYS_UPDATEOSCFG;
	}
#endif

	if (renewal) {
		sysmng_update(renewal);
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
arch_radiobutton_clicked(GtkButton *b, gpointer d)
{

	arch = (char *)d;
}

static void
clock_changed(GtkEditable *e, gpointer d)
{
	const gchar *base = gtk_entry_get_text(GTK_ENTRY(baseclock_entry));
	const gchar *multp = gtk_entry_get_text(GTK_ENTRY(clockmult_entry));
	guint mult = milstr_solveINT(multp);
	gchar buf[80];
	gint clk;

	if (base[0] == '1') {
		clk = PCBASECLOCK20 * mult;
	} else {
		clk = PCBASECLOCK25 * mult;
	}
	g_snprintf(buf, sizeof(buf), "%2d.%03d MHz",
	    clk / 1000000U, (clk / 1000) % 1000);
	gtk_label_set_text(GTK_LABEL((GtkWidget*)d), buf);
}

static void
samplingrate_changed(GtkEditable *e, gpointer d)
{
	rate = atoi(gtk_entry_get_text(GTK_ENTRY(samplingrate_entry)));
}

void
create_configure_dialog(void)
{
	GtkWidget *config_dialog;
	GtkWidget *main_widget;
	GtkWidget *cpu_hbox;
	GtkWidget *cpu_frame;
	GtkWidget *cpuframe_vbox;
	GtkWidget *cpuclock_hbox;
	GtkWidget *baseclock_combo;
	GtkWidget *rate_combo;
	GtkWidget *times_label;
	GtkWidget *realclock_label;
	GtkWidget *cputype_hbox;
	GtkWidget *cputype_label;
	GtkWidget *cputype_combo;
	GtkWidget *confirm_widget;
	GtkWidget *ok_button;
	GtkWidget *cancel_button;
	GtkWidget *arch_frame;
	GtkWidget *arch_vbox;
	GtkWidget *arch_hbox;
	GtkWidget *arch_radiobutton[NELEMENTS(architecture)];
	GtkWidget *always16bio_hbox;
	GtkWidget *sound_frame;
	GtkWidget *soundframe_vbox;
	GtkWidget *soundrate_hbox;
	GtkWidget *rate_label;
	GtkWidget *rate2_label;
	GtkWidget *samplingrate_combo;
	GtkWidget *soundbuffer_hbox;
	GtkWidget *buffer_label;
	GtkWidget *ms_label;
	gchar buf[8];
	int i, j;

	uninstall_idle_process();

	config_dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(config_dialog), "Configure");
	gtk_window_set_position(GTK_WINDOW(config_dialog), GTK_WIN_POS_CENTER);
	gtk_window_set_modal(GTK_WINDOW(config_dialog), TRUE);
	gtk_window_set_resizable(GTK_WINDOW(config_dialog), FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(config_dialog), 5);

	g_signal_connect(G_OBJECT(config_dialog), "destroy",
	    G_CALLBACK(dialog_destroy), NULL);

	main_widget = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(main_widget);
	gtk_container_add(GTK_CONTAINER(config_dialog), main_widget);

	/* CPU column */
	cpu_hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(cpu_hbox);
	gtk_box_pack_start(GTK_BOX(main_widget), cpu_hbox, TRUE, TRUE, 0);

	/*
	 * CPU frame
	 */
	cpu_frame = gtk_frame_new("CPU");
	gtk_widget_show(cpu_frame);
	gtk_box_pack_start(GTK_BOX(cpu_hbox), cpu_frame, TRUE, TRUE, 0);

	cpuframe_vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(cpuframe_vbox), 5);
	gtk_widget_show(cpuframe_vbox);
	gtk_container_add(GTK_CONTAINER(cpu_frame), cpuframe_vbox);

	/* cpu clock */
	cpuclock_hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(cpuclock_hbox);
	gtk_box_pack_start(GTK_BOX(cpuframe_vbox),cpuclock_hbox, TRUE, TRUE, 2);

	baseclock_combo = gtk_combo_box_entry_new_text();
	gtk_widget_show(baseclock_combo);
	gtk_box_pack_start(GTK_BOX(cpuclock_hbox), baseclock_combo, TRUE, FALSE, 0);
	gtk_widget_set_size_request(baseclock_combo, 128, -1);
	for (i = 0; i < NELEMENTS(baseclock_str); i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(baseclock_combo), baseclock_str[i]);
	}

	baseclock_entry = gtk_bin_get_child(GTK_BIN(baseclock_combo));
	gtk_widget_show(baseclock_entry);
	gtk_editable_set_editable(GTK_EDITABLE(baseclock_entry), FALSE);
	switch (np2cfg.baseclock) {
	default:
		np2cfg.baseclock = PCBASECLOCK25;
		sysmng_update(SYS_UPDATECFG|SYS_UPDATECLOCK);
		/*FALLTHROUGH*/
	case PCBASECLOCK25:
		gtk_entry_set_text(GTK_ENTRY(baseclock_entry),baseclock_str[1]);
		break;

	case PCBASECLOCK20:
		gtk_entry_set_text(GTK_ENTRY(baseclock_entry),baseclock_str[0]);
		break;
	}

	times_label = gtk_label_new("x");
	gtk_widget_show(times_label);
	gtk_box_pack_start(GTK_BOX(cpuclock_hbox), times_label, TRUE, FALSE, 0);
	gtk_misc_set_padding(GTK_MISC(times_label), 5, 0);

	rate_combo = gtk_combo_box_entry_new_text();
	gtk_widget_show(rate_combo);
	gtk_box_pack_start(GTK_BOX(cpuclock_hbox), rate_combo, TRUE, FALSE, 0);
	gtk_widget_set_size_request(rate_combo, 64, -1);
	for (i = 0; i < NELEMENTS(clockmult_str); i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(rate_combo), clockmult_str[i]);
	}

	clockmult_entry = gtk_bin_get_child(GTK_BIN(rate_combo));
	gtk_widget_show(clockmult_entry);
	gtk_editable_set_editable(GTK_EDITABLE(clockmult_entry), FALSE);
	switch (np2cfg.multiple) {
	case 1: case 2: case 4: case 5: case 6: case 8: case 10: case 12:
	case 16: case 20: case 24: case 30: case 36: case 40: case 42:
	case 52: case 64: case 76: case 88: case 100:
		g_snprintf(buf, sizeof(buf), "%d", np2cfg.multiple);
		gtk_entry_set_text(GTK_ENTRY(clockmult_entry), buf);
		break;

	default:
		gtk_entry_set_text(GTK_ENTRY(clockmult_entry), "4");
		break;
	}

	/* calculated cpu clock */
	realclock_label = gtk_label_new("MHz");
	gtk_widget_show(realclock_label);
	gtk_box_pack_start(GTK_BOX(cpuframe_vbox), realclock_label, FALSE, FALSE, 2);
	gtk_misc_set_alignment(GTK_MISC(realclock_label), 1.0, 0.5);

	g_signal_connect(G_OBJECT(baseclock_entry), "changed",
	  G_CALLBACK(clock_changed), (gpointer)realclock_label);
	g_signal_connect(G_OBJECT(clockmult_entry), "changed",
	  G_CALLBACK(clock_changed), (gpointer)realclock_label);
	clock_changed(NULL, realclock_label);

#if defined(CPUCORE_IA32)
	/* cpu type */
	cputype_hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(cputype_hbox);
	gtk_box_pack_start(GTK_BOX(cpuframe_vbox),cputype_hbox, TRUE, TRUE, 2);

	cputype_label = gtk_label_new("Type");
	gtk_widget_show(cputype_label);
	gtk_box_pack_start(GTK_BOX(cputype_hbox), cputype_label, FALSE, TRUE, 2);
	gtk_misc_set_alignment(GTK_MISC(cputype_label), 1.0, 0.5);

	cputype_combo = gtk_combo_box_entry_new_text();
	gtk_widget_show(cputype_combo);
	gtk_box_pack_start(GTK_BOX(cputype_hbox), cputype_combo, FALSE, TRUE, 0);
	gtk_widget_set_size_request(cputype_combo, 192, -1);
	for (i = 0; i < NELEMENTS(cputype_str); i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(cputype_combo), cputype_str[i]);
	}

	cputype_entry = gtk_bin_get_child(GTK_BIN(cputype_combo));
	gtk_widget_show(cputype_entry);
	gtk_editable_set_editable(GTK_EDITABLE(cputype_entry), FALSE);
	i = GetCpuTypeIndex();
	switch(i) {
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
		j = i;
		break;
	case 15:
		j = 11;
		break;
	case 16:
		j = 12;
		break;
	case 17:
		j = 13;
		break;
	case 18:
		j = 14;
		break;
	case 255:
		j = 15;
		break;
	default:
		j = 0;
	}
	gtk_entry_set_text(GTK_ENTRY(cputype_entry), cputype_str[j]);
#endif

	/* OK, Cancel button base widget */
	confirm_widget = gtk_vbutton_box_new();
	gtk_widget_show(confirm_widget);
	gtk_box_pack_start(GTK_BOX(cpu_hbox), confirm_widget, TRUE, TRUE, 0);
	gtk_button_box_set_layout(GTK_BUTTON_BOX(confirm_widget), GTK_BUTTONBOX_END);
	//gtk_button_box_set_spacing(GTK_BUTTON_BOX(confirm_widget), 0);

	/*
	 * Architecture frame
	 */
	arch_frame = gtk_frame_new("Architecture");
	gtk_widget_show(arch_frame);
	gtk_box_pack_start(GTK_BOX(main_widget), arch_frame, TRUE, TRUE, 0);

	/* architecture */
	arch_vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(arch_vbox), 5);
	gtk_widget_show(arch_vbox);
	gtk_container_add(GTK_CONTAINER(arch_frame), arch_vbox);

	arch_hbox = gtk_hbox_new(TRUE, 0);
	gtk_widget_show(arch_hbox);
	gtk_container_add(GTK_CONTAINER(arch_vbox), arch_hbox);

	for (i = 0; i < NELEMENTS(architecture); i++) {
		arch_radiobutton[i] = gtk_radio_button_new_with_label_from_widget(i > 0 ? GTK_RADIO_BUTTON(arch_radiobutton[i-1]) : NULL, architecture[i].label);
		gtk_widget_show(arch_radiobutton[i]);
		gtk_box_pack_start(GTK_BOX(arch_hbox), arch_radiobutton[i], FALSE, FALSE, 0);
#if GTK_MAJOR_VERSION > 2 || (GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION >= 18)
		gtk_widget_set_can_focus(arch_radiobutton[i], FALSE);
#else
		GTK_WIDGET_UNSET_FLAGS(rate_radiobutton[i], GTK_CAN_FOCUS);
#endif
		g_signal_connect(G_OBJECT(arch_radiobutton[i]), "clicked",
		    G_CALLBACK(arch_radiobutton_clicked), (gpointer)architecture[i].arch);
	}
	for (i = 0; i < NELEMENTS(architecture); i++) {
		if (strcmp(np2cfg.model, architecture[i].arch) == 0) {
			break;
		}
	}
	if (i == NELEMENTS(architecture)) {
		i = 1;
		milstr_ncpy(np2cfg.model, "VX", sizeof(np2cfg.model));
		sysmng_update(SYS_UPDATECFG);
	}
	g_signal_emit_by_name(G_OBJECT(arch_radiobutton[i]), "clicked");

	/* Always use 16bit I/O port addressing (PC-9821) */
	always16bio_hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(always16bio_hbox);
	gtk_container_add(GTK_CONTAINER(arch_vbox), always16bio_hbox);

	always16bio_checkbutton = gtk_check_button_new_with_label("Always use 16bit I/O port addressing (PC-9821)");
	gtk_widget_show(always16bio_checkbutton);
	gtk_box_pack_start(GTK_BOX(always16bio_hbox), always16bio_checkbutton, FALSE, FALSE, 1);
	if (np2cfg.sysiomsk == 0xFF00) {
		g_signal_emit_by_name(G_OBJECT(always16bio_checkbutton), "clicked");
	}

	/*
	 * Sound frame
	 */
	sound_frame = gtk_frame_new("Sound");
	gtk_widget_show(sound_frame);
	gtk_box_pack_start(GTK_BOX(main_widget), sound_frame, TRUE, TRUE, 0);

	soundframe_vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(soundframe_vbox), 5);
	gtk_widget_show(soundframe_vbox);
	gtk_container_add(GTK_CONTAINER(sound_frame), soundframe_vbox);

	/* sampling rate */
	soundrate_hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(soundrate_hbox);
	gtk_box_pack_start(GTK_BOX(soundframe_vbox), soundrate_hbox, FALSE, TRUE, 2);

	rate_label = gtk_label_new("Sampling Rate");
	gtk_widget_show(rate_label);
	gtk_box_pack_start(GTK_BOX(soundrate_hbox), rate_label, FALSE, TRUE, 1);
	gtk_widget_set_size_request(rate_label, 128, -1);

	samplingrate_combo = gtk_combo_box_entry_new_text();
	gtk_widget_show(samplingrate_combo);
	gtk_box_pack_start(GTK_BOX(soundrate_hbox), samplingrate_combo, FALSE, TRUE, 1);
	gtk_widget_set_size_request(samplingrate_combo, 96, -1);
	for (i = 0; i < NELEMENTS(samplingrate_str); i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(samplingrate_combo), samplingrate_str[i]);
	}

	samplingrate_entry = gtk_bin_get_child(GTK_BIN(samplingrate_combo));
	gtk_widget_show(samplingrate_entry);
	gtk_editable_set_editable(GTK_EDITABLE(samplingrate_entry), FALSE);
	switch (np2cfg.samplingrate) {
	case 11025:
		gtk_entry_set_text(GTK_ENTRY(samplingrate_entry),samplingrate_str[0]);
		break;
	case 22050:
		gtk_entry_set_text(GTK_ENTRY(samplingrate_entry),samplingrate_str[1]);
		break;
	case 44100:
		gtk_entry_set_text(GTK_ENTRY(samplingrate_entry),samplingrate_str[2]);
		break;
	case 48000:
		gtk_entry_set_text(GTK_ENTRY(samplingrate_entry),samplingrate_str[3]);
		break;
	case 88200:
		gtk_entry_set_text(GTK_ENTRY(samplingrate_entry),samplingrate_str[4]);
		break;
	case 96000:
		gtk_entry_set_text(GTK_ENTRY(samplingrate_entry),samplingrate_str[5]);
		break;
	case 176400:
		gtk_entry_set_text(GTK_ENTRY(samplingrate_entry),samplingrate_str[6]);
		break;
	case 192000:
		gtk_entry_set_text(GTK_ENTRY(samplingrate_entry),samplingrate_str[7]);
		break;
	default:
		np2cfg.samplingrate = 44100;
		sysmng_update(SYS_UPDATECFG|SYS_UPDATECLOCK);
		break;
	}
	g_signal_connect(G_OBJECT(samplingrate_entry), "changed",
	  G_CALLBACK(samplingrate_changed), (gpointer)realclock_label);

	rate2_label = gtk_label_new("kHz");
	gtk_widget_show(rate2_label);
	gtk_box_pack_start(GTK_BOX(soundrate_hbox), rate2_label, FALSE, TRUE, 1);
	gtk_widget_set_size_request(rate2_label, 32, -1);

	soundbuffer_hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(soundbuffer_hbox);
	gtk_box_pack_start(GTK_BOX(soundframe_vbox), soundbuffer_hbox, TRUE, TRUE, 2);

	/* buffer size */
	buffer_label = gtk_label_new("Buffer");
	gtk_widget_show(buffer_label);
	gtk_box_pack_start(GTK_BOX(soundbuffer_hbox), buffer_label, FALSE, FALSE, 0);
	gtk_widget_set_size_request(buffer_label, 96, -1);

	buffer_entry = gtk_entry_new();
	gtk_widget_show(buffer_entry);
	gtk_box_pack_start(GTK_BOX(soundbuffer_hbox), buffer_entry, FALSE, FALSE, 0);
	gtk_widget_set_size_request(buffer_entry, 48, -1);

	if (np2cfg.delayms >= 20 && np2cfg.delayms <= 1000) {
		g_snprintf(buf, sizeof(buf), "%d", np2cfg.delayms);
		gtk_entry_set_text(GTK_ENTRY(buffer_entry), buf);
	} else {
		gtk_entry_set_text(GTK_ENTRY(buffer_entry), "500");
		np2cfg.delayms = 500;
		sysmng_update(SYS_UPDATECFG|SYS_UPDATESBUF);
		soundrenewal = 1;
	}

	ms_label = gtk_label_new(" ms");
	gtk_widget_show(ms_label);
	gtk_box_pack_start(GTK_BOX(soundbuffer_hbox),ms_label, FALSE, FALSE, 0);

#if defined(SUPPORT_RESUME)
	/* resume */
	resume_checkbutton = gtk_check_button_new_with_label("Resume");
	gtk_widget_show(resume_checkbutton);
	gtk_box_pack_start(GTK_BOX(main_widget), resume_checkbutton, FALSE, FALSE, 1);
	if (np2oscfg.resume) {
		g_signal_emit_by_name(G_OBJECT(resume_checkbutton), "clicked");
	}
#endif

#if defined(GCC_CPU_ARCH_IA32)
	/* Disable MMX */
	disablemmx_checkbutton = gtk_check_button_new_with_label("Disable MMX");
	gtk_widget_show(disablemmx_checkbutton);
	gtk_box_pack_start(GTK_BOX(main_widget), disablemmx_checkbutton, FALSE, FALSE, 1);
	if (mmxflag & MMXFLAG_NOTSUPPORT) {
		gtk_widget_set_sensitive(disablemmx_checkbutton, FALSE);
	} else if (mmxflag & MMXFLAG_DISABLE) {
		g_signal_emit_by_name(G_OBJECT(disablemmx_checkbutton), "clicked");
	}
#endif

	/*
	 * OK, Cancel button
	 */
	ok_button = gtk_button_new_from_stock(GTK_STOCK_OK);
	gtk_widget_show(ok_button);
	gtk_container_add(GTK_CONTAINER(confirm_widget), ok_button);
#if GTK_MAJOR_VERSION > 2 || (GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION >= 18)
	gtk_widget_set_can_default(ok_button, TRUE);
	gtk_widget_has_default(ok_button);
#else
	GTK_WIDGET_SET_FLAGS(ok_button, GTK_CAN_DEFAULT);
	GTK_WIDGET_SET_FLAGS(ok_button, GTK_HAS_DEFAULT);
#endif
	g_signal_connect(G_OBJECT(ok_button), "clicked",
	    G_CALLBACK(ok_button_clicked), (gpointer)config_dialog);
	gtk_widget_grab_default(ok_button);

	cancel_button = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	gtk_widget_show(cancel_button);
	gtk_container_add(GTK_CONTAINER(confirm_widget), cancel_button);
#if GTK_MAJOR_VERSION > 2 || (GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION >= 18)
	gtk_widget_set_can_default(cancel_button, TRUE);
#else
	GTK_WIDGET_SET_FLAGS(cancel_button, GTK_CAN_DEFAULT);
#endif
	g_signal_connect_swapped(G_OBJECT(cancel_button), "clicked",
	    G_CALLBACK(gtk_widget_destroy), G_OBJECT(config_dialog));

	gtk_widget_show_all(config_dialog);
}
