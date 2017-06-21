/*
 * Copyright (c) 2004 NONAKA Kimihiro
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
#include "pccore.h"

#include "fdd/fddfile.h"
#include "fdd/newdisk.h"


/*
 * create hard disk image
 */

static const OEMCHAR *str_hddsize = OEMTEXT(" HDD Size");

static gint
anex_newdisk_dialog(GtkWidget *dialog)
{
	static const int cnv[] = { 0, 1, 2, 3, 5, 6 };
	static const int hddsize[] = { 5, 10, 15, 20, 30, 40 };
	static int last = 0;

	char buf[32];
	GtkWidget *dialog_table;
	GtkWidget *button[NELEMENTS(hddsize)];
	GtkWidget *label;
	int i;

	/* dialog table */
	dialog_table = gtk_table_new(4, 2, FALSE);
	gtk_table_set_col_spacings(GTK_TABLE(dialog_table), 5);
#if GTK_MAJOR_VERSION > 2 || (GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION >= 14)
	gtk_container_add(
	    GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
	    dialog_table);
#else
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox),dialog_table);
#endif
	gtk_widget_show(dialog_table);

	/* "HDD Size" label */
	label = gtk_label_new(str_hddsize);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_table_attach(GTK_TABLE(dialog_table), label, 0, 1, 0, 1,
	    GTK_FILL|GTK_SHRINK, GTK_FILL|GTK_SHRINK,
	    0, 5);
	gtk_widget_show(label);

	/* HDD Size radio button */
	for (i = 0; i < NELEMENTS(hddsize); ++i) {
		g_snprintf(buf, sizeof(buf), "%dMB", hddsize[i]);
		button[i] = gtk_radio_button_new_with_label_from_widget(
		    (i > 0) ? GTK_RADIO_BUTTON(button[i-1]) : NULL, buf);
		gtk_widget_show(button[i]);
		gtk_table_attach_defaults(GTK_TABLE(dialog_table),
		    button[i], i % 2, (i % 2) + 1, (i / 2) + 1, (i / 2) + 2);
#if GTK_MAJOR_VERSION > 2 || (GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION >= 18)
		gtk_widget_set_can_focus(button[i], FALSE);
#else
		GTK_WIDGET_UNSET_FLAGS(button[i], GTK_CAN_FOCUS);
#endif
	}
	if (last >= NELEMENTS(hddsize)) {
		last = 0;
	}
	g_signal_emit_by_name(G_OBJECT(button[last]), "clicked");

	gtk_widget_show_all(dialog);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
		for (i = 0; i < NELEMENTS(hddsize); i++) {
			if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button[i]))) {
				last = i;
				return cnv[i];
			}
		}
	}
	return -1;
}

static gint
t98_newdisk_dialog(GtkWidget *dialog, const int kind)
{
	static const char *hddsizestr[] = {
		"20", "41", "65", "80", "128",
	};

	char buf[32];
	GtkWidget *dialog_table;
	GtkWidget *label;
	GtkWidget *hbox;
	GtkWidget *combo;
	GtkWidget *entry;
	const char *p;
	int hdsize;
	int minsize, maxsize;
	gint rv;
	int i;

	minsize = 5;
	switch (kind) {
	case 2:	/* THD */
		maxsize = 256;
		break;

	case 3:	/* NHD */
		maxsize = 512;
		break;

	default:
		return 0;
	}

	/* dialog table */
	dialog_table = gtk_table_new(2, 3, FALSE);
	gtk_table_set_col_spacings(GTK_TABLE(dialog_table), 5);
#if GTK_MAJOR_VERSION > 2 || (GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION >= 14)
	gtk_container_add(
	    GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
	    dialog_table);
#else
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox),dialog_table);
#endif
	gtk_widget_show(dialog_table);

	/* "HDD Size" label */
	label = gtk_label_new(str_hddsize);
	gtk_misc_set_alignment(GTK_MISC(label), 0.5, 0.5);
	gtk_table_attach_defaults(GTK_TABLE(dialog_table), label, 0, 1, 0, 1);
	gtk_widget_show(label);

	/* HDD Size */
	hbox = gtk_hbox_new(FALSE, 2);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 0);
	gtk_table_attach_defaults(GTK_TABLE(dialog_table), hbox, 1, 2, 0, 1);
	gtk_widget_show(hbox);

	combo = gtk_combo_box_entry_new_text();
	for (i = 0; i < NELEMENTS(hddsizestr); ++i) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(combo), hddsizestr[i]);
	}
	gtk_widget_set_size_request(combo, 60, -1);
	gtk_box_pack_start(GTK_BOX(hbox), combo, FALSE, FALSE, 5);
	gtk_widget_show(combo);

	entry = gtk_bin_get_child(GTK_BIN(combo));
	gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
	gtk_entry_set_max_length(GTK_ENTRY(entry), 3);
	gtk_entry_set_text(GTK_ENTRY(entry), "");
	gtk_widget_show(entry);

	/* "MB" label */
	label = gtk_label_new("MB");
	gtk_misc_set_alignment(GTK_MISC(label), 0.1, 0.5);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_widget_show(label);

	/* size label */
	g_snprintf(buf, sizeof(buf), "(%d-%dMB)", minsize, maxsize);
	label = gtk_label_new(buf);
	gtk_misc_set_alignment(GTK_MISC(label), 0.9, 0.5);
	gtk_table_attach_defaults(GTK_TABLE(dialog_table), label, 1, 2, 1, 2);
	gtk_widget_show(label);

	gtk_widget_show_all(dialog);

	for (;;) {
		rv = gtk_dialog_run(GTK_DIALOG(dialog));
		if (rv == GTK_RESPONSE_CANCEL) {
			hdsize = 0;
			break;
		}
		if (rv == GTK_RESPONSE_OK) {
			p = gtk_entry_get_text(GTK_ENTRY(entry));
			if (p && strlen(p) != 0) {
				hdsize = milstr_solveINT(p);
				if (hdsize >= minsize && hdsize <= maxsize) {
					return hdsize;
				}
				gtk_entry_set_text(GTK_ENTRY(entry), "");
			}
		}
	}
	return 0;
}

void
create_newdisk_hd_dialog(const char *filename, int kind)
{
	GtkWidget *dialog;
	gint hdsize;

	dialog = gtk_dialog_new_with_buttons("Create new hard disk image",
	    GTK_WINDOW(main_window),
	    GTK_DIALOG_DESTROY_WITH_PARENT|GTK_DIALOG_MODAL,
	    GTK_STOCK_OK, GTK_RESPONSE_OK,
	    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
	    NULL);
	if (dialog == NULL)
		return;

	switch (kind) {
	case 1:	/* HDI */
		hdsize = anex_newdisk_dialog(dialog);
		if (hdsize >= 0) {
			newdisk_hdi(filename, hdsize);
		}
		break;

	case 2:	/* THD */
		hdsize = t98_newdisk_dialog(dialog, kind);
		if (hdsize > 0) {
			newdisk_thd(filename, hdsize);
		}
		break;

	case 3:	/* NHD */
		hdsize = t98_newdisk_dialog(dialog, kind);
		if (hdsize > 0) {
			newdisk_nhd(filename, hdsize);
		}
		break;
	}
	gtk_widget_destroy(dialog);
}


/*
 * create floppy disk image
 */
#define	DISKNAME_LEN	16

void
create_newdisk_fd_dialog(const char *filename)
{
	static const struct {
		const char *str;
		REG8 fdtype;
	} disktype[] = {
		{ "2DD",   DISKTYPE_2DD << 4      },
		{ "2HD",   DISKTYPE_2HD << 4      },
		{ "1.44", (DISKTYPE_2HD << 4) + 1 },
	};
	static REG8 makefdtype = DISKTYPE_2HD << 4;

	GtkWidget *dialog;
	GtkWidget *dialog_table;
	GtkWidget *label;
	GtkWidget *entry;
	GtkWidget *hbox;
	GtkWidget *button[NELEMENTS(disktype)];
	const gchar *p;
	int ndisktype;
	int i;

	ndisktype = NELEMENTS(disktype);
	if (!np2cfg.usefd144) {
		ndisktype--;
	}

	dialog = gtk_dialog_new_with_buttons("Create new floppy disk image",
	    GTK_WINDOW(main_window),
	    GTK_DIALOG_DESTROY_WITH_PARENT|GTK_DIALOG_MODAL,
	    GTK_STOCK_OK, GTK_RESPONSE_OK,
	    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
	    NULL);
	if (dialog == NULL)
		return;

	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
	gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(dialog), 10);

	/* dialog table */
	dialog_table = gtk_table_new(2, 3, FALSE);
#if GTK_MAJOR_VERSION > 2 || (GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION >= 14)
	gtk_container_add(
	    GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
	    dialog_table);
#else
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox),dialog_table);
#endif
	gtk_table_set_col_spacings(GTK_TABLE(dialog_table), 5);
	gtk_widget_show(dialog_table);

	/* "Disk Label" label */
	label = gtk_label_new(" Disk Label");
	gtk_misc_set_alignment(GTK_MISC(label), 0.5, 0.5);
	gtk_table_attach_defaults(GTK_TABLE(dialog_table), label, 0, 1, 0, 1);
	gtk_widget_show(label);

	/* "Disk Label" text entry */
	entry = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(entry), DISKNAME_LEN);
	gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
	gtk_table_attach_defaults(GTK_TABLE(dialog_table), entry, 1, 2, 0, 1);
	gtk_widget_show(entry);

	/* "Disk Type" label */
	label = gtk_label_new(" Disk Type");
	gtk_misc_set_alignment(GTK_MISC(label), 0.5, 0.5);
	gtk_table_attach_defaults(GTK_TABLE(dialog_table), label, 0, 1, 1, 2);
	gtk_widget_show(label);

	/* "o 2DD  o 2HD  o 1.44" radio button */
	hbox = gtk_hbox_new(FALSE, 3);
	gtk_table_attach_defaults(GTK_TABLE(dialog_table), hbox, 1, 2, 1, 2);
	gtk_widget_show(hbox);
	for (i = 0; i < ndisktype; ++i) {
		button[i] = gtk_radio_button_new_with_label_from_widget(
		    (i > 0) ? GTK_RADIO_BUTTON(button[i-1]) : NULL,
		    disktype[i].str);
		gtk_widget_show(button[i]);
		gtk_box_pack_start(GTK_BOX(hbox), button[i], FALSE, FALSE, 1);
#if GTK_MAJOR_VERSION > 2 || (GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION >= 18)
		gtk_widget_set_can_focus(button[i], FALSE);
#else
		GTK_WIDGET_UNSET_FLAGS(button[i], GTK_CAN_FOCUS);
#endif
	}
	for (i = 0; i < ndisktype; ++i) {
		if (disktype[i].fdtype == makefdtype)
			break;
	}
	if (i == ndisktype) {
		i = (i <= 1) ? 0 : 1;	/* 2HD */
	}
	g_signal_emit_by_name(G_OBJECT(button[i]), "clicked");

	gtk_widget_show_all(dialog);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
		p = gtk_entry_get_text(GTK_ENTRY(entry));
		if (p == NULL)
			p = "";
		for (i = 0; i < NELEMENTS(disktype); i++) {
			if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button[i]))) {
				makefdtype = disktype[i].fdtype;
				newdisk_fdd(filename, makefdtype, p);
				break;
			}
		}
	}
	gtk_widget_destroy(dialog);
}
