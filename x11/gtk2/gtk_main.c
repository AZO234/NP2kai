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

#include "np2.h"
#include "pccore.h"
#include "scrndraw.h"
#include "timing.h"

#include "toolkit.h"

#include "joymng.h"
#include "mousemng.h"
#include "scrnmng.h"
#include "soundmng.h"
#include "taskmng.h"

#include "gtk2/xnp2.h"
#include "gtk2/gtk_keyboard.h"
#include "gtk2/gtk_menu.h"

#include "resources/np2.xbm"


#define	APPNAME		"NP2"

#define	EVENT_MASK \
			(GDK_BUTTON1_MOTION_MASK	\
			 | GDK_BUTTON2_MOTION_MASK	\
			 | GDK_BUTTON3_MOTION_MASK	\
			 | GDK_POINTER_MOTION_MASK	\
			 | GDK_KEY_PRESS_MASK		\
			 | GDK_KEY_RELEASE_MASK		\
			 | GDK_BUTTON_PRESS_MASK	\
			 | GDK_BUTTON_RELEASE_MASK	\
			 | GDK_ENTER_NOTIFY_MASK	\
			 | GDK_LEAVE_NOTIFY_MASK	\
			 | GDK_EXPOSURE_MASK)

/*
 - Signal: gboolean GtkWidget::destroy_event (GtkWidget *widget,
          GdkEventAny *event, gpointer user_data)
*/
static gboolean
destroy_evhandler(GtkWidget *w, GdkEventAny *ev, gpointer p)
{

	toolkit_widget_quit();

	return TRUE;
}

/*
 - Signal: gboolean GtkWidget::configure_event (GtkWidget *widget,
          GdkEventConfigure *event, gpointer user_data)
*/
static gboolean
configure_evhandler(GtkWidget *w, GdkEventConfigure *ev, gpointer p)
{

	gdk_draw_rectangle(w->window, w->style->black_gc, TRUE,
	    0, 0, w->allocation.width, w->allocation.height);
	return TRUE;
}

/*
 - Signal: gboolean GtkWidget::expose_event (GtkWidget *widget,
          GdkEventExpose *event, gpointer user_data)
*/
static gboolean
expose_evhandler(GtkWidget *w, GdkEventExpose *ev, gpointer p)
{

	if (ev->count == 0) {
		scrndraw_redraw();
	}
	return TRUE;
}

/*
 - Signal: gboolean GtkWidget::key_press_event (GtkWidget *widget,
          GdkEventKey *event, gpointer user_data)
*/
static gboolean
key_press_evhandler(GtkWidget *w, GdkEventKey *ev, gpointer p)
{

	if (ev->keyval == GDK_KEY_F11) {
		if ((np2oscfg.F11KEY == 1) && (scrnmode & SCRNMODE_FULLSCREEN))
			xmenu_toggle_menu();
		else if (np2oscfg.F11KEY == 2)
			xmenu_select_screen(scrnmode ^ SCRNMODE_FULLSCREEN);
	} else if ((ev->keyval == GDK_KEY_F12) && (np2oscfg.F12KEY == 0))
		xmenu_toggle_item(NULL, "mousemode", !np2oscfg.MOUSE_SW);
	else
		gtkkbd_keydown(ev->keyval);
	return TRUE;
}

/*
 - Signal: gboolean GtkWidget::key_release_event (GtkWidget *widget,
          GdkEventKey *event, gpointer user_data)
*/
static gboolean
key_release_evhandler(GtkWidget *w, GdkEventKey *ev, gpointer p)
{

	if ((ev->keyval != GDK_KEY_F12) || (np2oscfg.F12KEY != 0))
		gtkkbd_keyup(ev->keyval);
	return TRUE;
}

/*
 - Signal: gboolean GtkWidget::button_press_event (GtkWidget *widget,
          GdkEventButton *event, gpointer user_data)
*/
static gboolean
button_press_evhandler(GtkWidget *w, GdkEventButton *ev, gpointer p)
{

	switch (ev->button) {
	case 1:
		mouse_btn(MOUSE_LEFTDOWN);
		break;

	case 2:
		xmenu_toggle_item(NULL, "mousemode", !np2oscfg.MOUSE_SW);
		break;

	case 3:
		mouse_btn(MOUSE_RIGHTDOWN);
		break;
	}
	return TRUE;
}

/*
 - Signal: gboolean GtkWidget::button_release_event (GtkWidget *widget,
          GdkEventButton *event, gpointer user_data)
*/
static gboolean
button_release_evhandler(GtkWidget *w, GdkEventButton *ev, gpointer p)
{

	switch (ev->button) {
	case 1:
		mouse_btn(MOUSE_LEFTUP);
		break;

	case 2:
		break;

	case 3:
		mouse_btn(MOUSE_RIGHTUP);
		break;
	}
	return TRUE;
}

/*
 - Signal: gboolean GtkWidget::motion_notify_event (GtkWidget *widget,
          GdkEventMotion *event, gpointer user_data)
*/
static gboolean
motion_notify_evhandler(GtkWidget *w, GdkEventMotion *ev, gpointer p)
{

	if ((scrnmode & SCRNMODE_FULLSCREEN) && (ev->y < 8.0))
		xmenu_show();

	return TRUE;
}


/*
 * misc
 */
static gint
main_loop_quit(gpointer p)
{

	scrnmng_fullscreen(0);

	return 0;
}

static void
set_icon_bitmap(GtkWidget *w)
{
	GdkPixmap *icon_pixmap;

	gdk_window_set_icon_name(w->window, APPNAME);
	icon_pixmap = gdk_bitmap_create_from_data(
	    w->window, np2_bits, np2_width, np2_height);
	gdk_window_set_icon(w->window, NULL, icon_pixmap, NULL);
}


/*
 * idle process
 */
static volatile int install_count = 0;
static guint idle_id;

void
install_idle_process(void)
{

	if (install_count++ == 0) {
		idle_id = g_idle_add((GSourceFunc)mainloop, NULL);
		soundmng_play();
	}
}

void
uninstall_idle_process(void)
{

	if (--install_count == 0) {
		soundmng_stop();
		g_source_remove(idle_id);
	}
}


/*
 * toolkit
 */
BRESULT
gui_gtk_arginit(int *argcp, char ***argvp)
{
	char buf[MAX_PATH];
	char *homeenv;

	gtk_set_locale();
	gtk_init(argcp, argvp);

	homeenv = getenv("HOME");
	if (homeenv) {
		g_snprintf(buf, sizeof(buf), "%s/.np2/gtkrc", homeenv);
		gtk_rc_add_default_file(buf);

		g_snprintf(buf, sizeof(buf), "%s/.np2/accels", homeenv);
		if (g_file_test(buf, G_FILE_TEST_IS_REGULAR))
			gtk_accel_map_load(buf);
	}

	return SUCCESS;
}

void
gui_gtk_widget_create(void)
{
	GtkWidget *main_vbox;
	GtkWidget *menubar;
	gchar *accel = NULL;
	gint root_x, root_y;

	main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
//	gtk_widget_set_double_buffered(GTK_WIDGET(main_window), FALSE);
	gtk_window_set_resizable(GTK_WINDOW(main_window), FALSE);
	gtk_window_set_title(GTK_WINDOW(main_window), np2oscfg.titles);
	gtk_widget_add_events(main_window, EVENT_MASK);

	main_vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(main_window), main_vbox);
	gtk_widget_show(main_vbox);

	menubar = create_menu();
	gtk_box_pack_start(GTK_BOX(main_vbox), menubar, FALSE, TRUE, 0);
	gtk_widget_show(menubar);

	drawarea = gtk_drawing_area_new();
//	gtk_widget_set_double_buffered(GTK_WIDGET(drawarea), FALSE);
	gtk_widget_set_size_request(GTK_WIDGET(drawarea), 640, 400);
	gtk_box_pack_end(GTK_BOX(main_vbox), drawarea, FALSE, TRUE, 0);
	gtk_widget_show(drawarea);

	g_object_get(gtk_widget_get_settings(main_window),
	    "gtk-menu-bar-accel", &accel, NULL);
	if (accel) {
		g_object_set(gtk_widget_get_settings(main_window),
		    "gtk-menu-bar-accel", "Menu", NULL);
		g_free(accel);
	}

	gtk_widget_realize(main_window);
	gdk_window_get_origin(main_window->window, &root_x, &root_y);
	gdk_window_reparent(main_window->window, NULL, root_x, root_y);
	set_icon_bitmap(main_window);

	g_signal_connect(G_OBJECT(main_window), "destroy", 
	    G_CALLBACK(destroy_evhandler), (gpointer)"WM destroy");
	g_signal_connect(G_OBJECT(main_window), "key_press_event",
	    G_CALLBACK(key_press_evhandler), NULL);
	g_signal_connect(G_OBJECT(main_window), "key_release_event",
	    G_CALLBACK(key_release_evhandler), NULL);
	g_signal_connect(G_OBJECT(main_window), "button_press_event",
	    G_CALLBACK(button_press_evhandler), NULL);
	g_signal_connect(G_OBJECT(main_window), "button_release_event",
	    G_CALLBACK(button_release_evhandler), NULL);
	g_signal_connect(G_OBJECT(main_window), "motion_notify_event",
	    G_CALLBACK(motion_notify_evhandler), NULL);

	g_signal_connect(G_OBJECT(drawarea), "configure_event",
	    G_CALLBACK(configure_evhandler), NULL);
	g_signal_connect(G_OBJECT(drawarea), "expose_event",
	    G_CALLBACK(expose_evhandler), NULL);
}

void
gui_gtk_widget_show(void)
{

	gtk_widget_show_all(main_window);
}

void
gui_gtk_widget_mainloop(void)
{

	install_idle_process();
	gtk_quit_add(1, main_loop_quit, NULL);
	gtk_main();
	uninstall_idle_process();
}

void
gui_gtk_widget_quit(void)
{

	taskmng_exit();
	gtk_main_quit();
}

void
gui_gtk_event_process(void)
{

	if (taskmng_isavail() && gdk_events_pending()) {
		gtk_main_iteration_do(FALSE);
	}
}

void
gui_gtk_set_window_title(const char* str)
{

	gtk_window_set_title(GTK_WINDOW(main_window), str);
}

int
gui_gtk_msgbox(const char *title, const char *msg, UINT flags)
{
	GtkWidget *dialog;
	GtkMessageType msgtype;
	GtkButtonsType btntype;
	int retval;
	int rv;

	uninstall_idle_process();

	switch (flags & TK_MB_BTN_MASK) {
	default:
		btntype = GTK_BUTTONS_OK;
		break;

	case TK_MB_OK:
		btntype = GTK_BUTTONS_OK;
		break;

	case TK_MB_CANCEL:
		btntype = GTK_BUTTONS_CANCEL;
		break;

	case TK_MB_OKCANCEL:
		btntype = GTK_BUTTONS_OK_CANCEL;
		break;

	case TK_MB_YESNO:
		btntype = GTK_BUTTONS_YES_NO;
		break;
	}

	if (flags & TK_MB_ICON_INFO) {
		msgtype = GTK_MESSAGE_INFO;
	} else if (flags & TK_MB_ICON_WARNING) {
		msgtype = GTK_MESSAGE_WARNING;
	} else if (flags & TK_MB_ICON_ERROR) {
		msgtype = GTK_MESSAGE_ERROR;
	} else if (flags & TK_MB_ICON_QUESTION) {
		msgtype = GTK_MESSAGE_QUESTION;
	} else {
		msgtype = GTK_MESSAGE_OTHER;
	}

	dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
	    GTK_DIALOG_DESTROY_WITH_PARENT|GTK_DIALOG_MODAL,
	    msgtype, btntype, "%s", msg);
	gtk_window_set_title(GTK_WINDOW(dialog), title);

	gtk_widget_show_all(dialog);

	rv = gtk_dialog_run(GTK_DIALOG(dialog));
	switch (rv) {
	case GTK_RESPONSE_OK:
		retval = TK_MB_OK;
		break;

	case GTK_RESPONSE_CANCEL:
		retval = TK_MB_CANCEL;
		break;

	case GTK_RESPONSE_YES:
		retval = TK_MB_YES;
		break;

	case GTK_RESPONSE_NO:
		retval = TK_MB_NO;
		break;

	default:
		retval = 0;	/* XXX */
		break;
	}

	gtk_widget_destroy(dialog);
	install_idle_process();

	return retval;
}
