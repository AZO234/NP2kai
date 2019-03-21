#include "compiler.h"

#include "gtk2/xnp2.h"
#include "gtk2/gtk_menu.h"

#include "np2.h"
#include "dosio.h"
#include "ini.h"
#include "pccore.h"
#include "iocore.h"

#include "sysmng.h"
#include "wab.h"
#if defined(SUPPORT_CL_GD5430)
#include "cirrus_vga_extern.h"
#endif	/* SUPPORT_CL_GD5430 */

#if defined(SUPPORT_WAB)

/*
 * General
 */

static GtkWidget *wab_useanalogswitchic_checkbutton;
static GtkWidget *wab_multiwindow_checkbutton;
static GtkWidget *wab_multithread_checkbutton;


#if defined(SUPPORT_CL_GD5430)
/*
 * CL_GD54xx
 */

static const char *cl_gd54xx_type_str[] = {
	"PC-9821Bp,Bs,Be,Bf built-in",
	"PC-9821Xe built-in",
	"PC-9821Cb built-in",
	"PC-9821Cf built-in",
	"PC-9821Xe10,Xa7e,Xb10 built-in",
	"PC-9821Cb2 built-in",
	"PC-9821Cx2 built-in",
#ifdef SUPPORT_PCI
	"PC-9821 PCI CL-GD5446 built-in",
#endif
	"MELCO WAB-S",
	"MELCO WSN-A2F",
	"MELCO WSN-A4F",
	"I-O DATA GA-98NBI/C",
	"I-O DATA GA-98NBII",
	"I-O DATA GA-98NBIV",
	"PC-9801-96(PC-9801B3-E02)",
#ifdef SUPPORT_PCI
	"Auto Select(Xe10, GA-98NBI/C), PCI",
	"Auto Select(Xe10, GA-98NBII), PCI",
	"Auto Select(Xe10, GA-98NBIV), PCI",
	"Auto Select(Xe10, WAB-S), PCI",
	"Auto Select(Xe10, WSN-A2F), PCI",
	"Auto Select(Xe10, WSN-A4F), PCI",
#endif
	"Auto Select(Xe10, WAB-S)",
	"Auto Select(Xe10, WSN-A2F)",
	"Auto Select(Xe10, WSN-A4F)",
};

static GtkWidget *cl_gd54xx_en_checkbutton;
static GtkWidget *cl_gd54xx_usefakehardwarecursor_checkbutton;
static GtkWidget *cl_gd54xx_type_entry;
#endif	/* SUPPORT_CL_GD5430 */


static void
ok_button_clicked(GtkButton *b, gpointer d)
{
	/* General */
	gint wab_useanalogswitchic;
	gint wab_multiwindow;
	gint wab_multithread;

#if defined(SUPPORT_CL_GD5430)
	/* CL_GD54xx */
	gint cl_gd54xx_en;
	gint cl_gd54xx_usefakehardwarecursor;
	const gchar *cl_gd54xx_type;
#endif	/* SUPPORT_CL_GD5430 */

	/* common */
	int i;
	int temp;
	BOOL renewal = FALSE;

	/* General */
	wab_useanalogswitchic = gtk_toggle_button_get_active(
	    GTK_TOGGLE_BUTTON(wab_useanalogswitchic_checkbutton));
	wab_multiwindow = gtk_toggle_button_get_active(
	    GTK_TOGGLE_BUTTON(wab_multiwindow_checkbutton));
	wab_multithread = gtk_toggle_button_get_active(
	    GTK_TOGGLE_BUTTON(wab_multithread_checkbutton));

#if defined(SUPPORT_CL_GD5430)
	/* CL_GD54xx */
	cl_gd54xx_en = gtk_toggle_button_get_active(
	    GTK_TOGGLE_BUTTON(cl_gd54xx_en_checkbutton));
	cl_gd54xx_usefakehardwarecursor = gtk_toggle_button_get_active(
	    GTK_TOGGLE_BUTTON(cl_gd54xx_usefakehardwarecursor_checkbutton));
	cl_gd54xx_type = gtk_entry_get_text(GTK_ENTRY(cl_gd54xx_type_entry));
#endif	/* SUPPORT_CL_GD5430 */

	if (np2cfg.wabasw != wab_useanalogswitchic) {
		np2cfg.wabasw = wab_useanalogswitchic;
		renewal = TRUE;
	}
	if (np2wabcfg.multiwindow != wab_multiwindow) {
		np2wabcfg.multiwindow = wab_multiwindow;
		renewal = TRUE;
	}
	if (np2wabcfg.multithread != wab_multithread) {
		np2wabcfg.multithread = wab_multithread;
		renewal = TRUE;
	}

#if defined(SUPPORT_CL_GD5430)
	if (np2cfg.usegd5430 != cl_gd54xx_en) {
		np2cfg.usegd5430 = cl_gd54xx_en;
		renewal = TRUE;
	}
	if (np2cfg.gd5430fakecur != cl_gd54xx_usefakehardwarecursor) {
		np2cfg.gd5430fakecur = cl_gd54xx_usefakehardwarecursor;
		renewal = TRUE;
	}
	for (i = 0; i < NELEMENTS(cl_gd54xx_type_str); i++) {
		if (strcmp(cl_gd54xx_type, cl_gd54xx_type_str[i]) == 0) {
			switch (i) {
			case 0:
				if(np2cfg.gd5430type != CIRRUS_98ID_Be) {
					np2cfg.gd5430type = CIRRUS_98ID_Be;
					renewal = TRUE;
				}
				break;
			case 1:
				if(np2cfg.gd5430type != CIRRUS_98ID_Xe) {
					np2cfg.gd5430type = CIRRUS_98ID_Xe;
					renewal = TRUE;
				}
				break;
			case 2:
				if(np2cfg.gd5430type != CIRRUS_98ID_Cb) {
					np2cfg.gd5430type = CIRRUS_98ID_Cb;
					renewal = TRUE;
				}
				break;
			case 3:
				if(np2cfg.gd5430type != CIRRUS_98ID_Cf) {
					np2cfg.gd5430type = CIRRUS_98ID_Cf;
					renewal = TRUE;
				}
				break;
			case 4:
				if(np2cfg.gd5430type != CIRRUS_98ID_Xe10) {
					np2cfg.gd5430type = CIRRUS_98ID_Xe10;
					renewal = TRUE;
				}
				break;
			case 5:
				if(np2cfg.gd5430type != CIRRUS_98ID_Cb2) {
					np2cfg.gd5430type = CIRRUS_98ID_Cb2;
					renewal = TRUE;
				}
				break;
			case 6:
				if(np2cfg.gd5430type != CIRRUS_98ID_Cx2) {
					np2cfg.gd5430type = CIRRUS_98ID_Cx2;
					renewal = TRUE;
				}
				break;
#ifdef SUPPORT_PCI
			case 7:
				if(np2cfg.gd5430type != CIRRUS_98ID_PCI) {
					np2cfg.gd5430type = CIRRUS_98ID_PCI;
					renewal = TRUE;
				}
				break;
			case 8:
				if(np2cfg.gd5430type != CIRRUS_98ID_WAB) {
					np2cfg.gd5430type = CIRRUS_98ID_WAB;
					renewal = TRUE;
				}
				break;
			case 9:
				if(np2cfg.gd5430type != CIRRUS_98ID_WSN_A2F) {
					np2cfg.gd5430type = CIRRUS_98ID_WSN_A2F;
					renewal = TRUE;
				}
				break;
			case 10:
				if(np2cfg.gd5430type != CIRRUS_98ID_WSN) {
					np2cfg.gd5430type = CIRRUS_98ID_WSN;
					renewal = TRUE;
				}
				break;
			case 11:
				if(np2cfg.gd5430type != CIRRUS_98ID_GA98NBIC) {
					np2cfg.gd5430type = CIRRUS_98ID_GA98NBIC;
					renewal = TRUE;
				}
				break;
			case 12:
				if(np2cfg.gd5430type != CIRRUS_98ID_GA98NBII) {
					np2cfg.gd5430type = CIRRUS_98ID_GA98NBII;
					renewal = TRUE;
				}
				break;
			case 13:
				if(np2cfg.gd5430type != CIRRUS_98ID_GA98NBIV) {
					np2cfg.gd5430type = CIRRUS_98ID_GA98NBIV;
					renewal = TRUE;
				}
				break;
			case 14:
				if(np2cfg.gd5430type != CIRRUS_98ID_96) {
					np2cfg.gd5430type = CIRRUS_98ID_96;
					renewal = TRUE;
				}
				break;
			case 15:
				if(np2cfg.gd5430type != CIRRUS_98ID_AUTO_XE_G1_PCI) {
					np2cfg.gd5430type = CIRRUS_98ID_AUTO_XE_G1_PCI;
					renewal = TRUE;
				}
				break;
			case 16:
				if(np2cfg.gd5430type != CIRRUS_98ID_AUTO_XE_G2_PCI) {
					np2cfg.gd5430type = CIRRUS_98ID_AUTO_XE_G2_PCI;
					renewal = TRUE;
				}
				break;
			case 17:
				if(np2cfg.gd5430type != CIRRUS_98ID_AUTO_XE_G4_PCI) {
					np2cfg.gd5430type = CIRRUS_98ID_AUTO_XE_G4_PCI;
					renewal = TRUE;
				}
				break;
			case 18:
				if(np2cfg.gd5430type != CIRRUS_98ID_AUTO_XE_WA_PCI) {
					np2cfg.gd5430type = CIRRUS_98ID_AUTO_XE_WA_PCI;
					renewal = TRUE;
				}
				break;
			case 19:
				if(np2cfg.gd5430type != CIRRUS_98ID_AUTO_XE_WS_PCI) {
					np2cfg.gd5430type = CIRRUS_98ID_AUTO_XE_WS_PCI;
					renewal = TRUE;
				}
				break;
			case 20:
				if(np2cfg.gd5430type != CIRRUS_98ID_AUTO_XE_W4_PCI) {
					np2cfg.gd5430type = CIRRUS_98ID_AUTO_XE_W4_PCI;
					renewal = TRUE;
				}
				break;
			case 21:
				if(np2cfg.gd5430type != CIRRUS_98ID_AUTO_XE10_WABS) {
					np2cfg.gd5430type = CIRRUS_98ID_AUTO_XE10_WABS;
					renewal = TRUE;
				}
				break;
			case 22:
				if(np2cfg.gd5430type != CIRRUS_98ID_AUTO_XE10_WSN2) {
					np2cfg.gd5430type = CIRRUS_98ID_AUTO_XE10_WSN2;
					renewal = TRUE;
				}
				break;
			case 23:
				if(np2cfg.gd5430type != CIRRUS_98ID_AUTO_XE10_WSN4) {
					np2cfg.gd5430type = CIRRUS_98ID_AUTO_XE10_WSN4;
					renewal = TRUE;
				}
				break;
#else
			case 7:
				if(np2cfg.gd5430type != CIRRUS_98ID_WAB) {
					np2cfg.gd5430type = CIRRUS_98ID_WAB;
					renewal = TRUE;
				}
				break;
			case 8:
				if(np2cfg.gd5430type != CIRRUS_98ID_WSN_A2F) {
					np2cfg.gd5430type = CIRRUS_98ID_WSN_A2F;
					renewal = TRUE;
				}
				break;
			case 9:
				if(np2cfg.gd5430type != CIRRUS_98ID_WSN) {
					np2cfg.gd5430type = CIRRUS_98ID_WSN;
					renewal = TRUE;
				}
				break;
			case 10:
				if(np2cfg.gd5430type != CIRRUS_98ID_GA98NBIC) {
					np2cfg.gd5430type = CIRRUS_98ID_GA98NBIC;
					renewal = TRUE;
				}
				break;
			case 11:
				if(np2cfg.gd5430type != CIRRUS_98ID_GA98NBII) {
					np2cfg.gd5430type = CIRRUS_98ID_GA98NBII;
					renewal = TRUE;
				}
				break;
			case 12:
				if(np2cfg.gd5430type != CIRRUS_98ID_GA98NBIV) {
					np2cfg.gd5430type = CIRRUS_98ID_GA98NBIV;
					renewal = TRUE;
				}
				break;
			case 13:
				if(np2cfg.gd5430type != CIRRUS_98ID_96) {
					np2cfg.gd5430type = CIRRUS_98ID_96;
					renewal = TRUE;
				}
				break;
			case 14:
				if(np2cfg.gd5430type != CIRRUS_98ID_AUTO_XE10_WABS) {
					np2cfg.gd5430type = CIRRUS_98ID_AUTO_XE10_WABS;
					renewal = TRUE;
				}
				break;
			case 15:
				if(np2cfg.gd5430type != CIRRUS_98ID_AUTO_XE10_WSN2) {
					np2cfg.gd5430type = CIRRUS_98ID_AUTO_XE10_WSN2;
					renewal = TRUE;
				}
				break;
			case 16:
				if(np2cfg.gd5430type != CIRRUS_98ID_AUTO_XE10_WSN4) {
					np2cfg.gd5430type = CIRRUS_98ID_AUTO_XE10_WSN4;
					renewal = TRUE;
				}
				break;
#endif
			}
			break;
		}
	}
#endif	/* SUPPORT_CL_GD5430 */

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

static GtkWidget *
create_general_note(void)
{
	GtkWidget *root_widget;

	root_widget = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(root_widget), 5);
	gtk_widget_show(root_widget);

	/* Use Analog Switch IC */
	wab_useanalogswitchic_checkbutton = gtk_check_button_new_with_label("Use Analog Switch IC (No relay sound)");
	gtk_widget_show(wab_useanalogswitchic_checkbutton);
	gtk_container_add(GTK_CONTAINER(root_widget), wab_useanalogswitchic_checkbutton);
	if (np2cfg.wabasw)
		g_signal_emit_by_name(G_OBJECT(wab_useanalogswitchic_checkbutton), "clicked");

	/* Multi Window Mode */
	wab_multiwindow_checkbutton = gtk_check_button_new_with_label("Multi Window Mode");
	gtk_widget_show(wab_multiwindow_checkbutton);
	gtk_container_add(GTK_CONTAINER(root_widget), wab_multiwindow_checkbutton);
	if (np2wabcfg.multiwindow)
		g_signal_emit_by_name(G_OBJECT(wab_multiwindow_checkbutton), "clicked");
gtk_widget_set_sensitive(wab_multiwindow_checkbutton, FALSE);

	/* Multi Thread Mode */
	wab_multithread_checkbutton = gtk_check_button_new_with_label("Multi Thread Mode");
	gtk_widget_show(wab_multithread_checkbutton);
	gtk_container_add(GTK_CONTAINER(root_widget), wab_multithread_checkbutton);
	if (np2wabcfg.multithread)
		g_signal_emit_by_name(G_OBJECT(wab_multithread_checkbutton), "clicked");
gtk_widget_set_sensitive(wab_multithread_checkbutton, FALSE);

	return root_widget;
}

#if defined(SUPPORT_CL_GD5430)
static GtkWidget *
create_cl_gd54xx_note(void)
{
	GtkWidget *root_widget;
	GtkWidget *type_hbox;
	GtkWidget *type_label;
	GtkWidget *type_combo;
	int i;

	root_widget = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(root_widget), 5);
	gtk_widget_show(root_widget);

	/* Enabled */
	cl_gd54xx_en_checkbutton = gtk_check_button_new_with_label("Enabled");
	gtk_widget_show(cl_gd54xx_en_checkbutton);
	gtk_container_add(GTK_CONTAINER(root_widget), cl_gd54xx_en_checkbutton);
	if (np2cfg.usegd5430)
		g_signal_emit_by_name(G_OBJECT(cl_gd54xx_en_checkbutton), "clicked");

	/* Type */
	type_hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(type_hbox);
	gtk_box_pack_start(GTK_BOX(root_widget), type_hbox, TRUE, TRUE, 0);

	type_label = gtk_label_new("Type");
	gtk_widget_show(type_label);
	gtk_container_add(GTK_CONTAINER(type_hbox), type_label);

	type_combo = gtk_combo_box_entry_new_text();
	gtk_widget_show(type_combo);
	gtk_container_add(GTK_CONTAINER(type_hbox), type_combo);
	gtk_widget_set_size_request(type_combo, 300, -1);
	for (i = 0; i < NELEMENTS(cl_gd54xx_type_str); i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(type_combo), cl_gd54xx_type_str[i]);
	}

	cl_gd54xx_type_entry = gtk_bin_get_child(GTK_BIN(type_combo));
	gtk_widget_show(cl_gd54xx_type_entry);
	gtk_editable_set_editable(GTK_EDITABLE(cl_gd54xx_type_entry), FALSE);
	switch (np2cfg.gd5430type) {
	case CIRRUS_98ID_Be:
		gtk_entry_set_text(GTK_ENTRY(cl_gd54xx_type_entry), cl_gd54xx_type_str[0]);
		break;
	case CIRRUS_98ID_Xe:
		gtk_entry_set_text(GTK_ENTRY(cl_gd54xx_type_entry), cl_gd54xx_type_str[1]);
		break;
	case CIRRUS_98ID_Cb:
		gtk_entry_set_text(GTK_ENTRY(cl_gd54xx_type_entry), cl_gd54xx_type_str[2]);
		break;
	case CIRRUS_98ID_Cf:
		gtk_entry_set_text(GTK_ENTRY(cl_gd54xx_type_entry), cl_gd54xx_type_str[3]);
		break;
	case CIRRUS_98ID_Xe10:
		gtk_entry_set_text(GTK_ENTRY(cl_gd54xx_type_entry), cl_gd54xx_type_str[4]);
		break;
	case CIRRUS_98ID_Cb2:
		gtk_entry_set_text(GTK_ENTRY(cl_gd54xx_type_entry), cl_gd54xx_type_str[5]);
		break;
	case CIRRUS_98ID_Cx2:
		gtk_entry_set_text(GTK_ENTRY(cl_gd54xx_type_entry), cl_gd54xx_type_str[6]);
		break;
#ifdef SUPPORT_PCI
	case CIRRUS_98ID_PCI:
		gtk_entry_set_text(GTK_ENTRY(cl_gd54xx_type_entry), cl_gd54xx_type_str[7]);
		break;
	case CIRRUS_98ID_WAB:
		gtk_entry_set_text(GTK_ENTRY(cl_gd54xx_type_entry), cl_gd54xx_type_str[8]);
		break;
	case CIRRUS_98ID_WSN_A2F:
		gtk_entry_set_text(GTK_ENTRY(cl_gd54xx_type_entry), cl_gd54xx_type_str[9]);
		break;
	case CIRRUS_98ID_WSN:
		gtk_entry_set_text(GTK_ENTRY(cl_gd54xx_type_entry), cl_gd54xx_type_str[10]);
		break;
	case CIRRUS_98ID_GA98NBIC:
		gtk_entry_set_text(GTK_ENTRY(cl_gd54xx_type_entry), cl_gd54xx_type_str[11]);
		break;
	case CIRRUS_98ID_GA98NBII:
		gtk_entry_set_text(GTK_ENTRY(cl_gd54xx_type_entry), cl_gd54xx_type_str[12]);
		break;
	case CIRRUS_98ID_GA98NBIV:
		gtk_entry_set_text(GTK_ENTRY(cl_gd54xx_type_entry), cl_gd54xx_type_str[13]);
		break;
	case CIRRUS_98ID_96:
		gtk_entry_set_text(GTK_ENTRY(cl_gd54xx_type_entry), cl_gd54xx_type_str[14]);
		break;
	case CIRRUS_98ID_AUTO_XE_G1_PCI:
		gtk_entry_set_text(GTK_ENTRY(cl_gd54xx_type_entry), cl_gd54xx_type_str[15]);
		break;
	case CIRRUS_98ID_AUTO_XE_G2_PCI:
		gtk_entry_set_text(GTK_ENTRY(cl_gd54xx_type_entry), cl_gd54xx_type_str[16]);
		break;
	case CIRRUS_98ID_AUTO_XE_G4_PCI:
		gtk_entry_set_text(GTK_ENTRY(cl_gd54xx_type_entry), cl_gd54xx_type_str[17]);
		break;
	case CIRRUS_98ID_AUTO_XE_WA_PCI:
		gtk_entry_set_text(GTK_ENTRY(cl_gd54xx_type_entry), cl_gd54xx_type_str[18]);
		break;
	case CIRRUS_98ID_AUTO_XE_WS_PCI:
		gtk_entry_set_text(GTK_ENTRY(cl_gd54xx_type_entry), cl_gd54xx_type_str[19]);
		break;
	case CIRRUS_98ID_AUTO_XE_W4_PCI:
		gtk_entry_set_text(GTK_ENTRY(cl_gd54xx_type_entry), cl_gd54xx_type_str[20]);
		break;
	case CIRRUS_98ID_AUTO_XE10_WABS:
		gtk_entry_set_text(GTK_ENTRY(cl_gd54xx_type_entry), cl_gd54xx_type_str[21]);
		break;
	case CIRRUS_98ID_AUTO_XE10_WSN2:
		gtk_entry_set_text(GTK_ENTRY(cl_gd54xx_type_entry), cl_gd54xx_type_str[22]);
		break;
	case CIRRUS_98ID_AUTO_XE10_WSN4:
		gtk_entry_set_text(GTK_ENTRY(cl_gd54xx_type_entry), cl_gd54xx_type_str[23]);
		break;
#else
	case CIRRUS_98ID_WAB:
		gtk_entry_set_text(GTK_ENTRY(cl_gd54xx_type_entry), cl_gd54xx_type_str[7]);
		break;
	case CIRRUS_98ID_WSN_A2F:
		gtk_entry_set_text(GTK_ENTRY(cl_gd54xx_type_entry), cl_gd54xx_type_str[8]);
		break;
	case CIRRUS_98ID_WSN:
		gtk_entry_set_text(GTK_ENTRY(cl_gd54xx_type_entry), cl_gd54xx_type_str[9]);
		break;
	case CIRRUS_98ID_GA98NBIC:
		gtk_entry_set_text(GTK_ENTRY(cl_gd54xx_type_entry), cl_gd54xx_type_str[10]);
		break;
	case CIRRUS_98ID_GA98NBII:
		gtk_entry_set_text(GTK_ENTRY(cl_gd54xx_type_entry), cl_gd54xx_type_str[11]);
		break;
	case CIRRUS_98ID_GA98NBIV:
		gtk_entry_set_text(GTK_ENTRY(cl_gd54xx_type_entry), cl_gd54xx_type_str[12]);
		break;
	case CIRRUS_98ID_96:
		gtk_entry_set_text(GTK_ENTRY(cl_gd54xx_type_entry), cl_gd54xx_type_str[13]);
		break;
	case CIRRUS_98ID_AUTO_XE10_WABS:
		gtk_entry_set_text(GTK_ENTRY(cl_gd54xx_type_entry), cl_gd54xx_type_str[14]);
		break;
	case CIRRUS_98ID_AUTO_XE10_WSN2:
		gtk_entry_set_text(GTK_ENTRY(cl_gd54xx_type_entry), cl_gd54xx_type_str[15]);
		break;
	case CIRRUS_98ID_AUTO_XE10_WSN4:
		gtk_entry_set_text(GTK_ENTRY(cl_gd54xx_type_entry), cl_gd54xx_type_str[16]);
		break;
#endif
	}

	/* Use Fake Hardware Cursor */
	cl_gd54xx_usefakehardwarecursor_checkbutton = gtk_check_button_new_with_label("Use Fake Hardware Cursor");
	gtk_widget_show(cl_gd54xx_usefakehardwarecursor_checkbutton);
	gtk_container_add(GTK_CONTAINER(root_widget), cl_gd54xx_usefakehardwarecursor_checkbutton);
	if (np2cfg.gd5430fakecur)
		g_signal_emit_by_name(G_OBJECT(cl_gd54xx_usefakehardwarecursor_checkbutton), "clicked");

	return root_widget;
}
#endif	/* defined(SUPPORT_CL_GD5430) */

void
create_wab_dialog(void)
{
	GtkWidget *wab_dialog;
	GtkWidget *main_vbox;
	GtkWidget *notebook;
	GtkWidget *general_note;
#if defined(SUPPORT_CL_GD5430)
	GtkWidget *cl_gd5430_note;
#endif	/* SUPPORT_CL_GD5430 */
	GtkWidget *confirm_widget;
	GtkWidget *ok_button;
	GtkWidget *cancel_button;

	uninstall_idle_process();

	wab_dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(wab_dialog), "Window Accelerator board option");
	gtk_window_set_position(GTK_WINDOW(wab_dialog), GTK_WIN_POS_CENTER);
	gtk_window_set_modal(GTK_WINDOW(wab_dialog), TRUE);
	gtk_window_set_resizable(GTK_WINDOW(wab_dialog), FALSE);

	g_signal_connect(G_OBJECT(wab_dialog), "destroy",
	    G_CALLBACK(dialog_destroy), NULL);

	main_vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(main_vbox);
	gtk_container_add(GTK_CONTAINER(wab_dialog), main_vbox);

	notebook = gtk_notebook_new();
	gtk_widget_show(notebook);
	gtk_box_pack_start(GTK_BOX(main_vbox), notebook, TRUE, TRUE, 0);

	/* "General" note */
	general_note = create_general_note();
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), general_note, gtk_label_new("General"));

#if defined(SUPPORT_CL_GD5430)
	/* "CL-GD54xx" note */
	cl_gd5430_note = create_cl_gd54xx_note();
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), cl_gd5430_note, gtk_label_new("CL-GD54xx"));
#endif	/* SUPPORT_CL_GD5430 */

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
	    G_CALLBACK(gtk_widget_destroy), G_OBJECT(wab_dialog));

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
	    G_CALLBACK(ok_button_clicked), (gpointer)wab_dialog);
	gtk_widget_grab_default(ok_button);

	gtk_widget_show_all(wab_dialog);
}
#endif

