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
#include "hostdrv.h"
#include "ini.h"


static GtkWidget *config_dialog;
static GtkWidget *hostdrv_directory_entry;
static GtkWidget *hostdrv_enabled_checkbutton;
static GtkWidget *hostdrv_read_checkbutton;
static GtkWidget *hostdrv_write_checkbutton;
static GtkWidget *hostdrv_delete_checkbutton;

static TCHAR s_hostdrvdir[10][MAX_PATH] = {0};

static void
ok_button_clicked(GtkButton *b, gpointer d)
{
	const gchar *directory = gtk_entry_get_text(GTK_ENTRY(hostdrv_directory_entry));
	gint enabled = gtk_toggle_button_get_active(
	    GTK_TOGGLE_BUTTON(hostdrv_enabled_checkbutton));
	gint read = gtk_toggle_button_get_active(
	    GTK_TOGGLE_BUTTON(hostdrv_read_checkbutton));
	gint write = gtk_toggle_button_get_active(
	    GTK_TOGGLE_BUTTON(hostdrv_write_checkbutton));
	gint delete = gtk_toggle_button_get_active(
	    GTK_TOGGLE_BUTTON(hostdrv_delete_checkbutton));
	UINT renewal = FALSE;
	int i;
	UINT16 temp;

	if (enabled != np2cfg.hdrvenable) {
		np2cfg.hdrvenable = enabled;
		renewal = TRUE;
	}

	temp = 0;
	if(read)
		temp |= 1;
	if(write)
		temp |= 2;
	if(delete)
		temp |= 4;
	if (temp != np2cfg.hdrvacc) {
		np2cfg.hdrvacc = temp;
		renewal = TRUE;
	}

	if(strcmp(directory, np2cfg.hdrvroot) != 0) {
		strcpy(np2cfg.hdrvroot, directory);
		renewal = TRUE;
	}

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
directory_browse_button_clicked(GtkButton *b, gpointer d)
{

	GtkWidget *dialog;
	GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER;
	gint res;

	dialog = gtk_file_chooser_dialog_new ("Open File",
		                              GTK_WINDOW(config_dialog),
		                              action,
		                              _("_Cancel"),
		                              GTK_RESPONSE_CANCEL,
		                              _("_Open"),
		                              GTK_RESPONSE_ACCEPT,
		                              NULL);
	res = gtk_dialog_run (GTK_DIALOG (dialog));
	if (res == GTK_RESPONSE_ACCEPT)	{
		char *filename;
		GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);
		filename = gtk_file_chooser_get_filename (chooser);
		gtk_entry_set_text(GTK_ENTRY(hostdrv_directory_entry), filename);
	}
	gtk_widget_destroy (dialog);
}

void
create_hostdrv_dialog(void)
{
	GtkWidget *main_widget;
	GtkWidget *directory_label;
	GtkWidget *directory_hbox;
	GtkWidget *directory_browse_button;
	GtkWidget *permission_label;
	GtkWidget *confirm_widget;
	GtkWidget *ok_button;
	GtkWidget *cancel_button;

	uninstall_idle_process();

	config_dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(config_dialog), "Hostdrv option");
	gtk_window_set_position(GTK_WINDOW(config_dialog), GTK_WIN_POS_CENTER);
	gtk_window_set_modal(GTK_WINDOW(config_dialog), TRUE);
	gtk_window_set_resizable(GTK_WINDOW(config_dialog), FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(config_dialog), 5);

	g_signal_connect(G_OBJECT(config_dialog), "destroy",
	    G_CALLBACK(dialog_destroy), NULL);

	main_widget = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(main_widget);
	gtk_container_add(GTK_CONTAINER(config_dialog), main_widget);

	/* enabled */
	hostdrv_enabled_checkbutton = gtk_check_button_new_with_label("Enabled");
	gtk_widget_show(hostdrv_enabled_checkbutton);
	gtk_box_pack_start(GTK_BOX(main_widget), hostdrv_enabled_checkbutton, FALSE, FALSE, 1);
	if (np2cfg.hdrvenable) {
		g_signal_emit_by_name(G_OBJECT(hostdrv_enabled_checkbutton), "clicked");
	}

	/* Directory */
	directory_label = gtk_label_new("Shared Directory");
	gtk_widget_show(directory_label);
	gtk_box_pack_start(GTK_BOX(main_widget), directory_label, TRUE, FALSE, 0);
	gtk_misc_set_padding(GTK_MISC(directory_label), 5, 0);

	directory_hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(directory_hbox);
	gtk_box_pack_start(GTK_BOX(main_widget),directory_hbox, TRUE, TRUE, 2);

	hostdrv_directory_entry = gtk_entry_new();
	gtk_widget_show(hostdrv_directory_entry);
	gtk_box_pack_start(GTK_BOX(directory_hbox), hostdrv_directory_entry, FALSE, FALSE, 0);
	gtk_widget_set_size_request(hostdrv_directory_entry, 256, -1);
	gtk_entry_set_text(GTK_ENTRY(hostdrv_directory_entry), np2cfg.hdrvroot);

	directory_browse_button = gtk_button_new_with_label("Browse...");
	gtk_widget_show(directory_browse_button);
	gtk_box_pack_end(GTK_BOX(directory_hbox), directory_browse_button, FALSE, FALSE, 5);
	g_signal_connect_swapped(G_OBJECT(directory_browse_button), "clicked",
	    G_CALLBACK(directory_browse_button_clicked), NULL);

	/* Permission */
	permission_label = gtk_label_new("Permission");
	gtk_widget_show(permission_label);
	gtk_box_pack_start(GTK_BOX(main_widget), permission_label, TRUE, FALSE, 0);
	gtk_misc_set_padding(GTK_MISC(permission_label), 5, 0);

	hostdrv_read_checkbutton = gtk_check_button_new_with_label("Read");
	gtk_widget_show(hostdrv_read_checkbutton);
	gtk_box_pack_start(GTK_BOX(main_widget), hostdrv_read_checkbutton, FALSE, FALSE, 1);
	if (np2cfg.hdrvacc & 1) {
		g_signal_emit_by_name(G_OBJECT(hostdrv_read_checkbutton), "clicked");
	}

	hostdrv_write_checkbutton = gtk_check_button_new_with_label("Write");
	gtk_widget_show(hostdrv_write_checkbutton);
	gtk_box_pack_start(GTK_BOX(main_widget), hostdrv_write_checkbutton, FALSE, FALSE, 1);
	if (np2cfg.hdrvacc & 2) {
		g_signal_emit_by_name(G_OBJECT(hostdrv_write_checkbutton), "clicked");
	}

	hostdrv_delete_checkbutton = gtk_check_button_new_with_label("Delete");
	gtk_widget_show(hostdrv_delete_checkbutton);
	gtk_box_pack_start(GTK_BOX(main_widget), hostdrv_delete_checkbutton, FALSE, FALSE, 1);
	if (np2cfg.hdrvacc & 4) {
		g_signal_emit_by_name(G_OBJECT(hostdrv_delete_checkbutton), "clicked");
	}

	/*
	 * OK, Cancel button
	 */
	confirm_widget = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(confirm_widget);
	gtk_box_pack_start(GTK_BOX(main_widget),confirm_widget, TRUE, TRUE, 2);

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

/* title */
static const char s_hostdrvapp[] = "NP2 hostdrv";

/**
 * settings
 */
static const INITBL s_hostdrvini[] =
{
	{"HOSTDRV0",  INITYPE_STR,	s_hostdrvdir[0],	MAX_PATH},
	{"HOSTDRV1",  INITYPE_STR,	s_hostdrvdir[1],	MAX_PATH},
	{"HOSTDRV2",  INITYPE_STR,	s_hostdrvdir[2],	MAX_PATH},
	{"HOSTDRV3",  INITYPE_STR,	s_hostdrvdir[3],	MAX_PATH},
	{"HOSTDRV4",  INITYPE_STR,	s_hostdrvdir[4],	MAX_PATH},
	{"HOSTDRV5",  INITYPE_STR,	s_hostdrvdir[5],	MAX_PATH},
	{"HOSTDRV6",  INITYPE_STR,	s_hostdrvdir[6],	MAX_PATH},
	{"HOSTDRV7",  INITYPE_STR,	s_hostdrvdir[7],	MAX_PATH},
	{"HOSTDRV8",  INITYPE_STR,	s_hostdrvdir[8],	MAX_PATH},
	{"HOSTDRV9",  INITYPE_STR,	s_hostdrvdir[9],	MAX_PATH},
};

/**
 * read settings
 */
void hostdrv_readini()
{
	memset(&s_hostdrvdir, 0, sizeof(s_hostdrvdir));

	OEMCHAR szPath[MAX_PATH];
	milstr_ncpy(szPath, modulefile, sizeof(szPath));
	ini_read(szPath, s_hostdrvapp, s_hostdrvini, sizeof(s_hostdrvini) / sizeof(INITBL));
}

/**
 * write settings
 */
void hostdrv_writeini()
{
	if(!np2oscfg.readonly){
		char szPath[MAX_PATH];
		milstr_ncpy(szPath, modulefile, sizeof(szPath));
		ini_write(szPath, s_hostdrvapp, s_hostdrvini, sizeof(s_hostdrvini) / sizeof(INITBL), FALSE);
	}
}

/**
 * path to top
 */
void hostdrv_setcurrentpath(const char* newpath)
{
	int i;
	if(!newpath[0]) 
		return;
	for(i=0;i<sizeof(s_hostdrvini) / sizeof(INITBL);i++){
		if(_tcsicmp(s_hostdrvdir[i], newpath)==0){
			i++;
			break;
		}
	}
	for(i=i-1;i>=1;i--){
		strcpy(s_hostdrvdir[i], s_hostdrvdir[i-1]);
	}
	strcpy(s_hostdrvdir[0], newpath);
}
