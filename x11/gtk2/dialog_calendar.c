/*
 * Copyright (c) 2003 NONAKA Kimihiro
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

#include <ctype.h>

#include "np2.h"
#include "pccore.h"

#include "sysmng.h"
#include "timemng.h"

#include "calendar.h"

#include "gtk2/xnp2.h"
#include "gtk2/gtk_menu.h"


static const char *calendar_kind_str[] = {
	"Real (localtime)", "Virtual Calendar"
};

static struct {
	GtkWidget	*entry;
	const UINT8	min;
	const UINT8	max;
} vircal[] = {
	{ NULL, 0x00, 0x99 },	/* year */
	{ NULL, 0x01, 0x12 },	/* month */
	{ NULL, 0x01, 0x31 },	/* day */
	{ NULL, 0x00, 0x23 },	/* hour */
	{ NULL, 0x00, 0x59 },	/* minute */
	{ NULL, 0x00, 0x59 },	/* second */
};

static GtkWidget *calendar_radiobutton[NELEMENTS(calendar_kind_str)];
static GtkWidget *now_button;
static UINT8 calendar_kind;


static UINT
getbcd(const char *str, size_t len)
{
	UINT val;
	UINT8 c;

	if (str[0] == '\0') {
		return 0xff;
	}

	val = 0;
	while (len--) {
		c = *str++;
		if (c == '\0')
			break;
		if (!isdigit(c))
			return 0xff;
		val <<= 4;
		val += c - '0';
	}
	return val;
}

static void
ok_button_clicked(GtkButton *b, gpointer d)
{
	UINT8 calendar_buf[8];
	const gchar *entryp;
	BOOL renewal;
	UINT8 val;
	int i;

	renewal = FALSE;
	if (np2cfg.calendar != calendar_kind) {
		renewal = TRUE;
	}
	if (!np2cfg.calendar) {
		memset(calendar_buf, 0, sizeof(calendar_buf));
		for (i = 0; i < NELEMENTS(vircal); i++) {
			entryp = gtk_entry_get_text(GTK_ENTRY(vircal[i].entry));
			val = getbcd(entryp, 2);
			if (val >= vircal[i].min && val <= vircal[i].max) {
				if (i == 1) {
					val = ((val & 0x10) * 10) + (val << 4);
				}
				calendar_buf[i] = (UINT8)val;
			} else {
				break;
			}
		}
		if (i == NELEMENTS(vircal)) {
			calendar_set(calendar_buf);
		} else {
			renewal = FALSE;
		}
	}

	if (renewal) {
		np2cfg.calendar = calendar_kind;
		sysmng_update(SYS_UPDATECFG);
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
set_virtual_calendar(BOOL virtual)
{
	UINT8 cbuf[8];
	char buf[8];
	int i;

	if (virtual) {
		calendar_getvir(cbuf);
	} else {
		calendar_getreal(cbuf);
	}
	for (i = 0; i < NELEMENTS(vircal); i++) {
		if (i != 1)
			g_snprintf(buf, sizeof(buf), "%02x", cbuf[i]);
		else
			g_snprintf(buf, sizeof(buf), "%02d", cbuf[i] >> 4);
		gtk_entry_set_text(GTK_ENTRY(vircal[i].entry), buf);
	}
}

static void
now_button_clicked(GtkButton *b, gpointer d)
{

	set_virtual_calendar(FALSE);
}

static void
calendar_radiobutton_clicked(GtkButton *b, gpointer d)
{
	gint virtual = GPOINTER_TO_UINT(d);
	int i;

	calendar_kind = virtual ? 0 : 1;

	for (i = 0; i < NELEMENTS(vircal); i++) {
		gtk_widget_set_sensitive(vircal[i].entry, virtual);
	}
	gtk_widget_set_sensitive(now_button, virtual);
}

void
create_calendar_dialog(void)
{
	GtkWidget *calendar_dialog;
	GtkWidget *main_widget;
	GtkWidget *calendar_widget;
	GtkWidget *date_sep_label[2];
	GtkWidget *time_sep_label[2];
	GtkWidget *ok_button;
	GtkWidget *cancel_button;
	int i;

	uninstall_idle_process();

	calendar_dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(calendar_dialog), "Calendar");
	gtk_window_set_position(GTK_WINDOW(calendar_dialog),GTK_WIN_POS_CENTER);
	gtk_window_set_modal(GTK_WINDOW(calendar_dialog), TRUE);
	gtk_window_set_resizable(GTK_WINDOW(calendar_dialog), FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(calendar_dialog), 5);

	g_signal_connect(G_OBJECT(calendar_dialog), "destroy",
	    G_CALLBACK(dialog_destroy), NULL);

	main_widget = gtk_table_new(4, 6, FALSE);
	gtk_widget_show(main_widget);
	gtk_container_add(GTK_CONTAINER(calendar_dialog), main_widget);

	/*
	 * calendar kind radiobutton
	 */
	for (i = 0; i < NELEMENTS(calendar_kind_str); i++) {
		calendar_radiobutton[i] =
		    gtk_radio_button_new_with_label_from_widget(
		      (i > 0) ? GTK_RADIO_BUTTON(calendar_radiobutton[i-1])
		        : NULL, calendar_kind_str[i]);
		gtk_widget_show(calendar_radiobutton[i]);
		gtk_table_attach_defaults(GTK_TABLE(main_widget),
		    calendar_radiobutton[i], 0, 3, i, i+1);
#if GTK_MAJOR_VERSION > 2 || (GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION >= 18)
		gtk_widget_set_can_focus(calendar_radiobutton[i], FALSE);
#else
		GTK_WIDGET_UNSET_FLAGS(calendar_radiobutton[i], GTK_CAN_FOCUS);
#endif
		g_signal_connect(G_OBJECT(calendar_radiobutton[i]),
		    "clicked", G_CALLBACK(calendar_radiobutton_clicked),
		    GUINT_TO_POINTER(i));
	}

	/*
	 * virtual calendar date/time
	 */
	calendar_widget = gtk_table_new(2, 7, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(calendar_widget), 5);
	gtk_table_set_col_spacings(GTK_TABLE(calendar_widget), 3);
	gtk_widget_show(calendar_widget);
	gtk_container_set_border_width(GTK_CONTAINER(calendar_widget), 2);
	gtk_table_attach_defaults(GTK_TABLE(main_widget), calendar_widget,
	    0, 3, 2, 4);

	for (i = 0; i < NELEMENTS(vircal); i++) {
		vircal[i].entry = gtk_entry_new();
		gtk_widget_show(vircal[i].entry);
		gtk_table_attach_defaults(GTK_TABLE(calendar_widget),
		    vircal[i].entry, (i%3)*2+1, (i%3)*2+2, i/3, i/3+1);
		gtk_entry_set_max_length(GTK_ENTRY(vircal[i].entry), 2);
		gtk_widget_set_size_request(vircal[i].entry, 24, -1);
	}
	for (i = 0; i < 2; i++) {
		date_sep_label[i] = gtk_label_new("/");
		gtk_widget_show(date_sep_label[i]);
		gtk_table_attach_defaults(GTK_TABLE(calendar_widget),
		    date_sep_label[i], i * 2 + 2, i * 2 + 3, 0, 1);

		time_sep_label[i] = gtk_label_new(":");
		gtk_widget_show(time_sep_label[i]);
		gtk_table_attach_defaults(GTK_TABLE(calendar_widget),
		    time_sep_label[i], i * 2 + 2, i * 2 + 3, 1, 2);
	}

	/*
	 * "now" button
	 */
	now_button = gtk_button_new_with_label("Now");
	gtk_widget_show(now_button);
	gtk_container_set_border_width(GTK_CONTAINER(now_button), 5);
	gtk_table_attach_defaults(GTK_TABLE(calendar_widget), now_button,
	    6, 7, 1, 2);
	g_signal_connect(G_OBJECT(now_button), "clicked",
	    G_CALLBACK(now_button_clicked), NULL);

	/* update to current state */
	set_virtual_calendar(TRUE);
	g_signal_emit_by_name(G_OBJECT(calendar_radiobutton[np2cfg.calendar ? 0 : 1]), "clicked");

	/*
	 * OK, Cancel button
	 */
	ok_button = gtk_button_new_from_stock(GTK_STOCK_OK);
	gtk_widget_show(ok_button);
	gtk_container_set_border_width(GTK_CONTAINER(ok_button), 2);
	gtk_table_attach_defaults(GTK_TABLE(main_widget),ok_button, 4, 6, 0, 1);
#if GTK_MAJOR_VERSION > 2 || (GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION >= 18)
	gtk_widget_set_can_default(ok_button, TRUE);
	gtk_widget_has_default(ok_button);
#else
	GTK_WIDGET_SET_FLAGS(ok_button, GTK_CAN_DEFAULT);
	GTK_WIDGET_SET_FLAGS(ok_button, GTK_HAS_DEFAULT);
#endif
	g_signal_connect(G_OBJECT(ok_button), "clicked",
	    G_CALLBACK(ok_button_clicked), (gpointer)calendar_dialog);
	gtk_widget_grab_default(ok_button);

	cancel_button = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	gtk_widget_show(cancel_button);
	gtk_container_set_border_width(GTK_CONTAINER(cancel_button), 3);
	gtk_table_attach_defaults(GTK_TABLE(main_widget), cancel_button,
	    4, 6, 1, 2);
	g_signal_connect_swapped(G_OBJECT(cancel_button), "clicked",
	    G_CALLBACK(gtk_widget_destroy), G_OBJECT(calendar_dialog));

	gtk_widget_show_all(calendar_dialog);
}
