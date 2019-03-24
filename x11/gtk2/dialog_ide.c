#include "compiler.h"

#include "np2.h"
#include "pccore.h"

#include "sysmng.h"

#include "gtk2/xnp2.h"
#include "gtk2/gtk_menu.h"


static const char *drivetype_str[] = {
	"None", "HDD", "CD-ROM"
};

static GtkWidget *drivetype_pm_entry;
static GtkWidget *drivetype_ps_entry;
static GtkWidget *drivetype_sm_entry;
static GtkWidget *drivetype_ss_entry;
static GtkWidget *interruptdelay_r_entry;
static GtkWidget *interruptdelay_w_entry;
static GtkWidget *useidebios_checkbutton;
static GtkWidget *autoidebios_checkbutton;
static GtkWidget *usecdecc_checkbutton;


static void
ok_button_clicked(GtkButton *b, gpointer d)
{
	const gchar *drivetype_pm = gtk_entry_get_text(GTK_ENTRY(drivetype_pm_entry));
	const gchar *drivetype_ps = gtk_entry_get_text(GTK_ENTRY(drivetype_ps_entry));
	const gchar *drivetype_sm = gtk_entry_get_text(GTK_ENTRY(drivetype_sm_entry));
	const gchar *drivetype_ss = gtk_entry_get_text(GTK_ENTRY(drivetype_ss_entry));
	const gchar *interruptdelay_r = gtk_entry_get_text(GTK_ENTRY(interruptdelay_r_entry));
	const gchar *interruptdelay_w = gtk_entry_get_text(GTK_ENTRY(interruptdelay_w_entry));
	gint useidebios = gtk_toggle_button_get_active(
	    GTK_TOGGLE_BUTTON(useidebios_checkbutton));
	gint autoidebios = gtk_toggle_button_get_active(
	    GTK_TOGGLE_BUTTON(autoidebios_checkbutton));
	gint usecdecc = gtk_toggle_button_get_active(
	    GTK_TOGGLE_BUTTON(usecdecc_checkbutton));
	int temp;
	int i;
	BOOL renewal = FALSE;

	for (i = 0; i < NELEMENTS(drivetype_str); i++) {
		if (strcmp(drivetype_pm, drivetype_str[i]) == 0) {
			if (np2cfg.idetype[0] != i) {
				np2cfg.idetype[0] = i;
				renewal = TRUE;
			}
			break;
		}
	}
	for (i = 0; i < NELEMENTS(drivetype_str); i++) {
		if (strcmp(drivetype_ps, drivetype_str[i]) == 0) {
			if (np2cfg.idetype[1] != i) {
				np2cfg.idetype[1] = i;
				renewal = TRUE;
			}
			break;
		}
	}
	for (i = 0; i < NELEMENTS(drivetype_str); i++) {
		if (strcmp(drivetype_sm, drivetype_str[i]) == 0) {
			if (np2cfg.idetype[2] != i) {
				np2cfg.idetype[2] = i;
				renewal = TRUE;
			}
			break;
		}
	}
	for (i = 0; i < NELEMENTS(drivetype_str); i++) {
		if (strcmp(drivetype_ss, drivetype_str[i]) == 0) {
			if (np2cfg.idetype[3] != i) {
				np2cfg.idetype[3] = i;
				renewal = TRUE;
			}
			break;
		}
	}

	temp = atoi(interruptdelay_r);
	if(temp < 0) temp = 0;
	if(temp > 100000000) temp = 100000000;
	if(np2cfg.iderwait != temp) {
		np2cfg.iderwait = temp;
		renewal = TRUE;
	}
	temp = atoi(interruptdelay_w);
	if(temp < 0) temp = 0;
	if(temp > 100000000) temp = 100000000;
	if(np2cfg.idewwait != temp) {
		np2cfg.idewwait = temp;
		renewal = TRUE;
	}

	if (np2cfg.idebios != useidebios) {
		np2cfg.idebios = useidebios;
		renewal = TRUE;
	}
	if (np2cfg.autoidebios != autoidebios) {
		np2cfg.autoidebios = autoidebios;
		renewal = TRUE;
	}
	if (np2cfg.usecdecc != usecdecc) {
		np2cfg.usecdecc = usecdecc;
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

void
create_ide_dialog(void)
{
	GtkWidget *ideconfig_dialog;
	GtkWidget *main_widget;
	GtkWidget *table;
	GtkWidget *primary_label;
	GtkWidget *secondary_label;
	GtkWidget *master_label;
	GtkWidget *drivetype_pm_combo;
	GtkWidget *drivetype_sm_combo;
	GtkWidget *slave_label;
	GtkWidget *drivetype_ps_combo;
	GtkWidget *drivetype_ss_combo;
	GtkWidget *interruptdelay_r_hbox;
	GtkWidget *interruptdelay_r_label;
	GtkWidget *interruptdelay_r_clock_label;
	GtkWidget *interruptdelay_w_hbox;
	GtkWidget *interruptdelay_w_label;
	GtkWidget *interruptdelay_w_clock_label;
	GtkWidget *check_confirm_hbox;
	GtkWidget *check_vbox;
	GtkWidget *ok_button;
	GtkWidget *cancel_button;
	char temp[16];
	int i;

	uninstall_idle_process();

	ideconfig_dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(ideconfig_dialog), "IDE Configure");
	gtk_window_set_position(GTK_WINDOW(ideconfig_dialog), GTK_WIN_POS_CENTER);
	gtk_window_set_modal(GTK_WINDOW(ideconfig_dialog), TRUE);
	gtk_window_set_resizable(GTK_WINDOW(ideconfig_dialog), FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(ideconfig_dialog), 5);

	g_signal_connect(G_OBJECT(ideconfig_dialog), "destroy",
	    G_CALLBACK(dialog_destroy), NULL);

	main_widget = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(main_widget);
	gtk_container_add(GTK_CONTAINER(ideconfig_dialog), main_widget);

	table = gtk_table_new(3, 3, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 5);
	gtk_table_set_col_spacings(GTK_TABLE(table), 5);
	gtk_box_pack_start(GTK_BOX(main_widget), table, FALSE, FALSE, 0);
	gtk_widget_show(table);

	/* Title */
	primary_label = gtk_label_new("Primary");
	gtk_widget_show(primary_label);
	gtk_table_attach_defaults(GTK_TABLE(table), primary_label, 1, 2, 0, 1);
	gtk_misc_set_padding(GTK_MISC(primary_label), 5, 0);

	secondary_label = gtk_label_new("Secondary");
	gtk_widget_show(secondary_label);
	gtk_table_attach_defaults(GTK_TABLE(table), secondary_label, 2, 3, 0, 1);
	gtk_misc_set_padding(GTK_MISC(secondary_label), 5, 0);

	/* Master */
	master_label = gtk_label_new("Master");
	gtk_widget_show(master_label);
	gtk_table_attach_defaults(GTK_TABLE(table), master_label, 0, 1, 1, 2);
	gtk_misc_set_padding(GTK_MISC(master_label), 5, 0);

	drivetype_pm_combo = gtk_combo_box_entry_new_text();
	gtk_widget_show(drivetype_pm_combo);
	gtk_table_attach_defaults(GTK_TABLE(table), drivetype_pm_combo, 1, 2, 1, 2);
	gtk_widget_set_size_request(drivetype_pm_combo, 96, -1);
	for (i = 0; i < NELEMENTS(drivetype_str); i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(drivetype_pm_combo), drivetype_str[i]);
	}
	drivetype_pm_entry = gtk_bin_get_child(GTK_BIN(drivetype_pm_combo));
	gtk_widget_show(drivetype_pm_entry);
	gtk_editable_set_editable(GTK_EDITABLE(drivetype_pm_entry), FALSE);
	gtk_entry_set_text(GTK_ENTRY(drivetype_pm_entry),drivetype_str[np2cfg.idetype[0]]);

	drivetype_sm_combo = gtk_combo_box_entry_new_text();
	gtk_widget_show(drivetype_sm_combo);
	gtk_table_attach_defaults(GTK_TABLE(table), drivetype_sm_combo, 2, 3, 1, 2);
	gtk_widget_set_size_request(drivetype_sm_combo, 96, -1);
	for (i = 0; i < NELEMENTS(drivetype_str); i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(drivetype_sm_combo), drivetype_str[i]);
	}
	drivetype_sm_entry = gtk_bin_get_child(GTK_BIN(drivetype_sm_combo));
	gtk_widget_show(drivetype_sm_entry);
	gtk_editable_set_editable(GTK_EDITABLE(drivetype_sm_entry), FALSE);
	gtk_entry_set_text(GTK_ENTRY(drivetype_sm_entry),drivetype_str[np2cfg.idetype[2]]);

	/* Slave */
	slave_label = gtk_label_new("Slave");
	gtk_widget_show(slave_label);
	gtk_table_attach_defaults(GTK_TABLE(table), slave_label, 0, 1, 2, 3);
	gtk_misc_set_padding(GTK_MISC(slave_label), 5, 0);

	drivetype_ps_combo = gtk_combo_box_entry_new_text();
	gtk_widget_show(drivetype_ps_combo);
	gtk_table_attach_defaults(GTK_TABLE(table), drivetype_ps_combo, 1, 2, 2, 3);
	gtk_widget_set_size_request(drivetype_ps_combo, 96, -1);
	for (i = 0; i < NELEMENTS(drivetype_str); i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(drivetype_ps_combo), drivetype_str[i]);
	}
	drivetype_ps_entry = gtk_bin_get_child(GTK_BIN(drivetype_ps_combo));
	gtk_widget_show(drivetype_ps_entry);
	gtk_editable_set_editable(GTK_EDITABLE(drivetype_ps_entry), FALSE);
	gtk_entry_set_text(GTK_ENTRY(drivetype_ps_entry),drivetype_str[np2cfg.idetype[1]]);

	drivetype_ss_combo = gtk_combo_box_entry_new_text();
	gtk_widget_show(drivetype_ss_combo);
	gtk_table_attach_defaults(GTK_TABLE(table), drivetype_ss_combo, 2, 3, 2, 3);
	gtk_widget_set_size_request(drivetype_ss_combo, 96, -1);
	for (i = 0; i < NELEMENTS(drivetype_str); i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(drivetype_ss_combo), drivetype_str[i]);
	}
	drivetype_ss_entry = gtk_bin_get_child(GTK_BIN(drivetype_ss_combo));
	gtk_widget_show(drivetype_ss_entry);
	gtk_editable_set_editable(GTK_EDITABLE(drivetype_ss_entry), FALSE);
	gtk_entry_set_text(GTK_ENTRY(drivetype_ss_entry),drivetype_str[np2cfg.idetype[3]]);

	/* Interrupt Delay(Read) */
	interruptdelay_r_hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(interruptdelay_r_hbox);
	gtk_box_pack_start(GTK_BOX(main_widget), interruptdelay_r_hbox, TRUE, TRUE, 0);

	interruptdelay_r_label = gtk_label_new("Interrupt Delay(Read)");
	gtk_widget_show(interruptdelay_r_label);
	gtk_box_pack_start(GTK_BOX(interruptdelay_r_hbox), interruptdelay_r_label, TRUE, FALSE, 0);
	gtk_misc_set_padding(GTK_MISC(interruptdelay_r_label), 5, 0);

	interruptdelay_r_entry = gtk_entry_new();
	gtk_widget_show(interruptdelay_r_entry);
	gtk_box_pack_start(GTK_BOX(interruptdelay_r_hbox), interruptdelay_r_entry, FALSE, FALSE, 0);
	gtk_widget_set_size_request(interruptdelay_r_entry, 128, -1);

	if (np2cfg.iderwait >= 0 && np2cfg.iderwait <= 100000000) {
		sprintf(temp, "%d", np2cfg.iderwait);
		gtk_entry_set_text(GTK_ENTRY(interruptdelay_r_entry), temp);
	} else {
		gtk_entry_set_text(GTK_ENTRY(interruptdelay_r_entry), "100000000");
		np2cfg.iderwait = 100000000;
		sysmng_update(SYS_UPDATECFG|SYS_UPDATESBUF);
	}

	interruptdelay_r_clock_label = gtk_label_new("clock");
	gtk_widget_show(interruptdelay_r_clock_label);
	gtk_box_pack_start(GTK_BOX(interruptdelay_r_hbox), interruptdelay_r_clock_label, TRUE, FALSE, 0);
	gtk_misc_set_padding(GTK_MISC(interruptdelay_r_clock_label), 5, 0);

	/* Interrupt Delay(Write) */
	interruptdelay_w_hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(interruptdelay_w_hbox);
	gtk_box_pack_start(GTK_BOX(main_widget), interruptdelay_w_hbox, TRUE, TRUE, 0);

	interruptdelay_w_label = gtk_label_new("Interrupt Delay(Write)");
	gtk_widget_show(interruptdelay_w_label);
	gtk_box_pack_start(GTK_BOX(interruptdelay_w_hbox), interruptdelay_w_label, TRUE, FALSE, 0);
	gtk_misc_set_padding(GTK_MISC(interruptdelay_w_label), 5, 0);

	interruptdelay_w_entry = gtk_entry_new();
	gtk_widget_show(interruptdelay_w_entry);
	gtk_box_pack_start(GTK_BOX(interruptdelay_w_hbox), interruptdelay_w_entry, FALSE, FALSE, 0);
	gtk_widget_set_size_request(interruptdelay_w_entry, 128, -1);

	if (np2cfg.idewwait >= 0 && np2cfg.idewwait <= 100000000) {
		sprintf(temp, "%d", np2cfg.idewwait);
		gtk_entry_set_text(GTK_ENTRY(interruptdelay_w_entry), temp);
	} else {
		gtk_entry_set_text(GTK_ENTRY(interruptdelay_w_entry), "100000000");
		np2cfg.idewwait = 100000000;
		sysmng_update(SYS_UPDATECFG|SYS_UPDATESBUF);
	}

	interruptdelay_w_clock_label = gtk_label_new("clock");
	gtk_widget_show(interruptdelay_w_clock_label);
	gtk_box_pack_start(GTK_BOX(interruptdelay_w_hbox), interruptdelay_w_clock_label, TRUE, FALSE, 0);
	gtk_misc_set_padding(GTK_MISC(interruptdelay_w_clock_label), 5, 0);

	/* Check & Confirm */
	check_vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(check_vbox);
	gtk_container_add(GTK_CONTAINER(main_widget), check_vbox);

	useidebios_checkbutton = gtk_check_button_new_with_label("Use IDE BIOS");
	gtk_widget_show(useidebios_checkbutton);
	gtk_box_pack_start(GTK_BOX(check_vbox), useidebios_checkbutton, FALSE, FALSE, 1);
	if (np2cfg.idebios) {
		g_signal_emit_by_name(G_OBJECT(useidebios_checkbutton), "clicked");
	}

	autoidebios_checkbutton = gtk_check_button_new_with_label("Auto IDE BIOS");
	gtk_widget_show(autoidebios_checkbutton);
	gtk_box_pack_start(GTK_BOX(check_vbox), autoidebios_checkbutton, FALSE, FALSE, 1);
	if (np2cfg.autoidebios) {
		g_signal_emit_by_name(G_OBJECT(autoidebios_checkbutton), "clicked");
	}

	usecdecc_checkbutton = gtk_check_button_new_with_label("Use CD-ROM EDC/ECC Emulation");
	gtk_widget_show(usecdecc_checkbutton);
	gtk_box_pack_start(GTK_BOX(check_vbox), usecdecc_checkbutton, FALSE, FALSE, 1);
	if (np2cfg.usecdecc) {
		g_signal_emit_by_name(G_OBJECT(usecdecc_checkbutton), "clicked");
	}

	/*
	 * OK, Cancel button
	 */
	check_confirm_hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(check_confirm_hbox);
	gtk_box_pack_start(GTK_BOX(main_widget), check_confirm_hbox, TRUE, TRUE, 0);

	ok_button = gtk_button_new_from_stock(GTK_STOCK_OK);
	gtk_widget_show(ok_button);
	gtk_container_add(GTK_CONTAINER(check_confirm_hbox), ok_button);
#if GTK_MAJOR_VERSION > 2 || (GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION >= 18)
	gtk_widget_set_can_default(ok_button, TRUE);
	gtk_widget_has_default(ok_button);
#else
	GTK_WIDGET_SET_FLAGS(ok_button, GTK_CAN_DEFAULT);
	GTK_WIDGET_SET_FLAGS(ok_button, GTK_HAS_DEFAULT);
#endif
	g_signal_connect(G_OBJECT(ok_button), "clicked",
	    G_CALLBACK(ok_button_clicked), (gpointer)ideconfig_dialog);
	gtk_widget_grab_default(ok_button);

	cancel_button = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	gtk_widget_show(cancel_button);
	gtk_container_add(GTK_CONTAINER(check_confirm_hbox), cancel_button);
#if GTK_MAJOR_VERSION > 2 || (GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION >= 18)
	gtk_widget_set_can_default(cancel_button, TRUE);
#else
	GTK_WIDGET_SET_FLAGS(cancel_button, GTK_CAN_DEFAULT);
#endif
	g_signal_connect_swapped(G_OBJECT(cancel_button), "clicked",
	    G_CALLBACK(gtk_widget_destroy), G_OBJECT(ideconfig_dialog));

	gtk_widget_show_all(ideconfig_dialog);
}
