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

#include "gtk2/xnp2.h"
#include "gtk2/gtk_menu.h"

#include "np2.h"
#include "dosio.h"
#include "pccore.h"
#include "iocore.h"

#include "palettes.h"
#include "scrndraw.h"

#include "sysmng.h"


/*
 * Video
 */
static GtkWidget *video_lcd_checkbutton;
static GtkWidget *video_lcd_reverse_checkbutton;
static GtkWidget *video_skipline_checkbutton;
static GObject *video_skipline_ratio_adj;

/*
 * Chip
 */
static GtkWidget *chip_enable_color16_checkbutton;
static gint chip_uPD72020;
static gint chip_gc_kind;

/*
 * Timing
 */
static const char *timing_waitclock_str[] = {
	"T-RAM", "V-RAM", "GRCG"
};

static GObject *timing_waitclock_adj[NELEMENTS(timing_waitclock_str)];
static GObject *timing_realpal_adj;


static void
ok_button_clicked(GtkButton *b, gpointer d)
{
	/* Video */
	gint video_lcd;
	gint video_lcdrev;
	gint video_skipline;
	guint video_skipline_ratio;

	/* Chip */
	gint chip_color16;

	/* Timing */
	guint timing_waitclock[NELEMENTS(timing_waitclock_str)];
	guint timing_realpal;

	/* common */
	BOOL renewal;
	int i;

	/* Video tab */
	video_lcd = gtk_toggle_button_get_active(
	    GTK_TOGGLE_BUTTON(video_lcd_checkbutton));
	video_lcdrev = gtk_toggle_button_get_active(
	    GTK_TOGGLE_BUTTON(video_lcd_reverse_checkbutton));
	video_skipline = gtk_toggle_button_get_active(
	    GTK_TOGGLE_BUTTON(video_skipline_checkbutton));
	video_skipline_ratio = (guint)gtk_adjustment_get_value(
	    GTK_ADJUSTMENT(video_skipline_ratio_adj));

	renewal = FALSE;
	if (np2cfg.skipline != video_skipline) {
		np2cfg.skipline = video_skipline;
		renewal = TRUE;
	}
	if (np2cfg.skiplight != video_skipline_ratio) {
		np2cfg.skiplight = video_skipline_ratio;
		renewal = TRUE;
	}
	if (renewal) {
		pal_makeskiptable();
	}
	if (video_lcd) {
		video_lcd |= video_lcdrev ? 2 : 0;
	}
	if (np2cfg.LCD_MODE != video_lcd) {
		np2cfg.LCD_MODE = video_lcd;
		pal_makelcdpal();
		renewal = TRUE;
	}

	if (renewal) {
		sysmng_update(SYS_UPDATECFG);
		scrndraw_redraw();
	}

	/* Chip tab */
	chip_color16 = gtk_toggle_button_get_active(
	    GTK_TOGGLE_BUTTON(chip_enable_color16_checkbutton));

	renewal = FALSE;
	if (np2cfg.uPD72020 != chip_uPD72020) {
		np2cfg.uPD72020 = chip_uPD72020;
		gdc_restorekacmode();
		gdcs.grphdisp |= GDCSCRN_ALLDRAW2;
		renewal = TRUE;
	}
	if ((np2cfg.grcg & 3) != chip_gc_kind) {
		np2cfg.grcg = (np2cfg.grcg & ~3) | chip_gc_kind;
		gdcs.grphdisp |= GDCSCRN_ALLDRAW2;
		renewal = TRUE;
	}
	if (np2cfg.color16 != chip_color16) {
		np2cfg.color16 = chip_color16;
		renewal = TRUE;
	}

	if (renewal) {
		sysmng_update(SYS_UPDATECFG);
	}

	/* Timing tab */
	timing_realpal = (guint)gtk_adjustment_get_value(
	    GTK_ADJUSTMENT(timing_realpal_adj)) + 32;

	renewal = FALSE;
	for (i =  0; i < NELEMENTS(timing_waitclock_str); i++) {
		timing_waitclock[i] = (guint)gtk_adjustment_get_value(
		    GTK_ADJUSTMENT(timing_waitclock_adj[i]));
		if (np2cfg.wait[i * 2] != timing_waitclock[i]) {
			np2cfg.wait[i * 2] = timing_waitclock[i];
			np2cfg.wait[i * 2 + 1] = 1;
			renewal = TRUE;
		}
	}
	if (np2cfg.realpal != timing_realpal) {
		np2cfg.realpal = timing_realpal;
		renewal = TRUE;
	}

	if (renewal) {
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
lcd_checkbutton_clicked(GtkCheckButton *b, gpointer d)
{

	gtk_widget_set_sensitive((GtkWidget *)d,
	    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(b)));
}

static void
uPD72020_radiobutton_clicked(GtkButton *b, gpointer d)
{

	chip_uPD72020 = GPOINTER_TO_UINT(d);
}

static void
gc_radiobutton_clicked(GtkButton *b, gpointer d)
{

	chip_gc_kind = GPOINTER_TO_UINT(d);
}

static GtkWidget*
create_video_note(void)
{
	GtkWidget *main_widget;
	GtkWidget *ratio_label;
	GtkWidget *ratio_scale;
	GtkWidget *ratio_hbox;

	main_widget = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(main_widget), 3);
	gtk_widget_show(main_widget);

	video_lcd_checkbutton =
	    gtk_check_button_new_with_label("Liquid Crystal Display");
	gtk_widget_show(video_lcd_checkbutton);
	gtk_box_pack_start(GTK_BOX(main_widget), video_lcd_checkbutton,
	    FALSE, FALSE, 0);
	if (np2cfg.LCD_MODE & 1) {
		g_signal_emit_by_name(G_OBJECT(video_lcd_checkbutton),
		    "clicked");
	}

	video_lcd_reverse_checkbutton =
	    gtk_check_button_new_with_label("Reverse");
	gtk_widget_show(video_lcd_reverse_checkbutton);
	gtk_box_pack_start(GTK_BOX(main_widget), video_lcd_reverse_checkbutton,
	    FALSE, FALSE, 0);
	gtk_container_set_border_width(
	    GTK_CONTAINER(video_lcd_reverse_checkbutton), 5);
	if (np2cfg.LCD_MODE & 1) {
		if (np2cfg.LCD_MODE & 2) {
			g_signal_emit_by_name(
			  G_OBJECT(video_lcd_reverse_checkbutton), "clicked");
		}
	} else {
		gtk_widget_set_sensitive(video_lcd_reverse_checkbutton, FALSE);
	}
	g_signal_connect(G_OBJECT(video_lcd_checkbutton), "clicked",
	    G_CALLBACK(lcd_checkbutton_clicked),
	    (gpointer)video_lcd_reverse_checkbutton);

	video_skipline_checkbutton =
	    gtk_check_button_new_with_label("Use skipline revisions");
	gtk_widget_show(video_skipline_checkbutton);
	gtk_box_pack_start(GTK_BOX(main_widget), video_skipline_checkbutton,
	    FALSE, FALSE, 0);
	if (np2cfg.skipline) {
		g_signal_emit_by_name(G_OBJECT(video_skipline_checkbutton),
		    "clicked");
	}

	ratio_hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(ratio_hbox);
	gtk_box_pack_start(GTK_BOX(main_widget), ratio_hbox, FALSE, FALSE, 0);

	ratio_label = gtk_label_new("Ratio");
	gtk_widget_show(ratio_label);
	gtk_box_pack_start(GTK_BOX(ratio_hbox), ratio_label, TRUE, TRUE, 0);

	video_skipline_ratio_adj = gtk_adjustment_new(np2cfg.skiplight,
	    0.0, 255.0, 1.0, 1.0, 0.0);
	ratio_scale = gtk_hscale_new(GTK_ADJUSTMENT(video_skipline_ratio_adj));
	gtk_scale_set_default_values(GTK_SCALE(ratio_scale));
	gtk_scale_set_digits(GTK_SCALE(ratio_scale), 0);
	gtk_box_pack_start(GTK_BOX(ratio_hbox), ratio_scale, TRUE, TRUE, 0);
	gtk_widget_show(ratio_scale);

	return main_widget;
}

static GtkWidget*
create_chip_note(void)
{
	static const char *gdc_str[] = { "uPD7220", "uPD72020" };
	static const char *gc_str[] = { "None", "GRCG", "GRCG+", "EGC" };
	GtkWidget *main_widget;
	GtkWidget *gdc_frame;
	GtkWidget *gdc_hbox;
	GtkWidget *upd72020_radiobutton[NELEMENTS(gdc_str)];
	GtkWidget *gc_frame;
	GtkWidget *gc_hbox;
	GtkWidget *gc_radiobutton[NELEMENTS(gc_str)];
	int i;

	main_widget = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(main_widget), 5);
	gtk_widget_show(main_widget);

	/*
	 * Graphic Display Contoller
	 */
	gdc_frame = gtk_frame_new("Graphic Display Controller");
	gtk_widget_show(gdc_frame);
	gtk_box_pack_start(GTK_BOX(main_widget), gdc_frame, TRUE, TRUE, 2);

	gdc_hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(gdc_hbox), 5);
	gtk_widget_show(gdc_hbox);
	gtk_container_add(GTK_CONTAINER(gdc_frame), gdc_hbox);

	for (i = 0; i < NELEMENTS(gdc_str); i++) {
		upd72020_radiobutton[i] =
		    gtk_radio_button_new_with_label_from_widget(
		      (i > 0) ? GTK_RADIO_BUTTON(upd72020_radiobutton[i-1])
		        : NULL, gdc_str[i]);
		gtk_widget_show(upd72020_radiobutton[i]);
		gtk_box_pack_start(GTK_BOX(gdc_hbox), upd72020_radiobutton[i],
		    TRUE, FALSE, 0);
		g_signal_connect(G_OBJECT(upd72020_radiobutton[i]), "clicked",
		    G_CALLBACK(uPD72020_radiobutton_clicked),
		    GUINT_TO_POINTER(i));
	}
	g_signal_emit_by_name(
	    G_OBJECT(upd72020_radiobutton[np2cfg.uPD72020 ? 1 : 0]),
	    "clicked");

	/*
	 * Graphic Charger
	 */
	gc_frame = gtk_frame_new("Graphic Charger");
	gtk_widget_show(gc_frame);
	gtk_box_pack_start(GTK_BOX(main_widget), gc_frame, TRUE, TRUE, 2);

	gc_hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(gc_hbox), 5);
	gtk_widget_show(gc_hbox);
	gtk_container_add(GTK_CONTAINER(gc_frame), gc_hbox);

	for (i = 0; i < NELEMENTS(gc_str); i++) {
		gc_radiobutton[i] =
		    gtk_radio_button_new_with_label_from_widget(
		      (i > 0) ? GTK_RADIO_BUTTON(gc_radiobutton[i-1]) : NULL,
		      gc_str[i]);
		gtk_widget_show(gc_radiobutton[i]);
		gtk_box_pack_start(GTK_BOX(gc_hbox), gc_radiobutton[i], TRUE,
		    FALSE, 0);
		g_signal_connect(G_OBJECT(gc_radiobutton[i]), "clicked",
		    G_CALLBACK(gc_radiobutton_clicked), GUINT_TO_POINTER(i));
	}
	g_signal_emit_by_name(G_OBJECT(gc_radiobutton[np2cfg.grcg & 3]),
	    "clicked");

	/*
	 * Use 16 colors
	 */
	chip_enable_color16_checkbutton =
	    gtk_check_button_new_with_label("Enable 16color (PC-9801-24)");
	gtk_widget_show(chip_enable_color16_checkbutton);
	gtk_box_pack_start(GTK_BOX(main_widget),
	    chip_enable_color16_checkbutton, FALSE, FALSE, 2);
	if (np2cfg.color16) {
		g_signal_emit_by_name(
		    G_OBJECT(chip_enable_color16_checkbutton), "clicked");
	}

	return main_widget;
}

static GtkWidget*
create_timing_note(void)
{
	GtkWidget *main_widget;
	struct {
		GtkWidget *name_label;
		GtkWidget *clock_scale;
		GtkWidget *clock_label;
	} waitclk[NELEMENTS(timing_waitclock_str)];
	GtkWidget *realpal_label;
	GtkWidget *realpal_scale;
	int i;

	main_widget = gtk_table_new(5, 5, FALSE);
	gtk_widget_show(main_widget);

	for (i = 0; i < NELEMENTS(timing_waitclock_str); i++) {
		waitclk[i].name_label = gtk_label_new(timing_waitclock_str[i]);
		gtk_widget_show(waitclk[i].name_label);
		gtk_table_attach(GTK_TABLE(main_widget), waitclk[i].name_label,
		    0, 1, i, i + 1,
		    GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 10, 0);

		timing_waitclock_adj[i] = gtk_adjustment_new(np2cfg.wait[i * 2], 0.0, 32.0, 1.0, 1.0, 0.0);
		waitclk[i].clock_scale = gtk_hscale_new(GTK_ADJUSTMENT(timing_waitclock_adj[i]));
		gtk_widget_show(waitclk[i].clock_scale);
		gtk_scale_set_default_values(GTK_SCALE(waitclk[i].clock_scale));
		gtk_scale_set_digits(GTK_SCALE(waitclk[i].clock_scale), 0);
		gtk_table_attach_defaults(GTK_TABLE(main_widget),
		    waitclk[i].clock_scale, 1, 4, i, i + 1);

		waitclk[i].clock_label = gtk_label_new("clock");
		gtk_misc_set_alignment(GTK_MISC(waitclk[i].clock_label),
		    0.1, 0.5);
		gtk_widget_show(waitclk[i].clock_label);
		gtk_table_attach_defaults(GTK_TABLE(main_widget),
		    waitclk[i].clock_label, 4, 5, i, i + 1);
	}

	realpal_label = gtk_label_new("RealPalettes Adjust");
	gtk_misc_set_alignment(GTK_MISC(realpal_label), 0.0, 1.0);
	gtk_widget_show(realpal_label);
	gtk_table_attach_defaults(GTK_TABLE(main_widget), realpal_label,
	     0, 5, 3, 4);

	timing_realpal_adj = gtk_adjustment_new(np2cfg.realpal - 32,
	    -32.0, 32.0, 1.0, 1.0, 0.0);
	realpal_scale = gtk_hscale_new(GTK_ADJUSTMENT(timing_realpal_adj));
	gtk_widget_show(realpal_scale);
	gtk_scale_set_default_values(GTK_SCALE(realpal_scale));
	gtk_scale_set_digits(GTK_SCALE(realpal_scale), 0);
	gtk_table_attach_defaults(GTK_TABLE(main_widget), realpal_scale,
	     1, 5, 4, 5);

	return main_widget;
}

void
create_screen_dialog(void)
{
	GtkWidget* screen_dialog;
	GtkWidget *main_widget;
	GtkWidget* screen_notebook;
	GtkWidget* video_note;
	GtkWidget* chip_note;
	GtkWidget* timing_note;
	GtkWidget *confirm_widget;
	GtkWidget *ok_button;
	GtkWidget *cancel_button;

	uninstall_idle_process();

	screen_dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(screen_dialog), "Screen option");
	gtk_window_set_position(GTK_WINDOW(screen_dialog), GTK_WIN_POS_CENTER);
	gtk_window_set_modal(GTK_WINDOW(screen_dialog), TRUE);
	gtk_window_set_resizable(GTK_WINDOW(screen_dialog), FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(screen_dialog), 5);

	g_signal_connect(G_OBJECT(screen_dialog), "destroy",
	    G_CALLBACK(dialog_destroy), NULL);

	main_widget = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(main_widget);
	gtk_container_add(GTK_CONTAINER(screen_dialog), main_widget);

	screen_notebook = gtk_notebook_new();
	gtk_widget_show(screen_notebook);
	gtk_box_pack_start(GTK_BOX(main_widget),screen_notebook, TRUE, TRUE, 0);

	/* "Video" note */
	video_note = create_video_note();
	gtk_notebook_append_page(GTK_NOTEBOOK(screen_notebook), video_note, gtk_label_new("Video"));

	/* "Chip" note */
	chip_note = create_chip_note();
	gtk_notebook_append_page(GTK_NOTEBOOK(screen_notebook), chip_note, gtk_label_new("Chip"));

	/* "Timing" note */
	timing_note = create_timing_note();
	gtk_notebook_append_page(GTK_NOTEBOOK(screen_notebook), timing_note, gtk_label_new("Timing"));

	/*
	 * OK, Cancel button
	 */
	confirm_widget = gtk_hbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(confirm_widget), 2);
	gtk_widget_show(confirm_widget);
	gtk_box_pack_start(GTK_BOX(main_widget), confirm_widget, TRUE, TRUE, 0);

	cancel_button = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	gtk_widget_show(cancel_button);
	gtk_box_pack_end(GTK_BOX(confirm_widget),cancel_button,FALSE, FALSE, 0);
#if GTK_MAJOR_VERSION > 2 || (GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION >= 18)
	gtk_widget_set_can_default(cancel_button, FALSE);
#else
	GTK_WIDGET_SET_FLAGS(cancel_button, GTK_CAN_DEFAULT);
#endif
	g_signal_connect_swapped(G_OBJECT(cancel_button), "clicked",
	    G_CALLBACK(gtk_widget_destroy), G_OBJECT(screen_dialog));

	ok_button = gtk_button_new_from_stock(GTK_STOCK_OK);
	gtk_widget_show(ok_button);
	gtk_box_pack_end(GTK_BOX(confirm_widget), ok_button, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(ok_button), "clicked",
	    G_CALLBACK(ok_button_clicked), (gpointer)screen_dialog);
#if GTK_MAJOR_VERSION > 2 || (GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION >= 18)
	gtk_widget_set_can_default(ok_button, TRUE);
	gtk_widget_has_default(ok_button);
#else
	GTK_WIDGET_SET_FLAGS(ok_button, GTK_CAN_DEFAULT);
	GTK_WIDGET_SET_FLAGS(ok_button, GTK_HAS_DEFAULT);
#endif
	gtk_widget_grab_default(ok_button);

	gtk_widget_show_all(screen_dialog);
}
