#include <compiler.h>

#include "gtk2/xnp2.h"
#include "gtk2/gtk_menu.h"

#include <np2.h>
#include <dosio.h>
#include <ini.h>
#include <pccore.h>
#include <io/iocore.h>

#include <sysmng.h>

/*
 * General
 */

static GtkWidget *powmng_checkbutton;


#if defined(SUPPORT_LGY98)
/*
 * LGY-98
 */

static const char *lgy98_ioport_str[] = {
	"00D0", "10D0", "20D0", "30D0", "40D0", "50D0", "60D0", "70D0"
};

static const char *lgy98_intr_str[] = {
	"INT0", "INT1", "INT2", "INT5"
};

static GtkWidget *lgy98_en_checkbutton;
static GtkWidget *lgy98_ioport_entry;
static GtkWidget *lgy98_int_entry;
#endif	/* SUPPORT_LGY98 */


static void
ok_button_clicked(GtkButton *b, gpointer d)
{
	/* General */
	gint en_powmng;

#if defined(SUPPORT_LGY98)
	/* LGY-98 */
	gint en_lgy98;
	const gchar *lgy98_ioport;
	const gchar *lgy98_intr;
#endif	/* SUPPORT_LGY98 */

	/* common */
	int i;
	int temp;
	BOOL renewal = FALSE;

	/* General */
	en_powmng = gtk_toggle_button_get_active(
	    GTK_TOGGLE_BUTTON(powmng_checkbutton));

#if defined(SUPPORT_LGY98)
	/* LGY-98 */
	en_lgy98 = gtk_toggle_button_get_active(
	    GTK_TOGGLE_BUTTON(lgy98_en_checkbutton));
	lgy98_ioport = gtk_entry_get_text(GTK_ENTRY(lgy98_ioport_entry));
	lgy98_intr = gtk_entry_get_text(GTK_ENTRY(lgy98_int_entry));
#endif	/* SUPPORT_LGY98 */

	if (np2cfg.np2netpmm != en_powmng) {
		np2cfg.np2netpmm = en_powmng;
		renewal = TRUE;
	}

#if defined(SUPPORT_LGY98)
	if (np2cfg.uselgy98 != en_lgy98) {
		np2cfg.uselgy98 = en_lgy98;
		renewal = TRUE;
	}
	temp = 0;
	for (i = 0; i < 4; i++) {
		if('0' <= lgy98_ioport[3 - i] && lgy98_ioport[3 - i] <= '9')
			temp += (lgy98_ioport[3 - i] - '0') * pow(16, i);
		else if('A' <= lgy98_ioport[3 - i] && lgy98_ioport[3 - i] <= 'F')
			temp += (lgy98_ioport[3 - i] - 'A' + 10) * pow(16, i);
	}
	if (np2cfg.lgy98io != temp) {
		np2cfg.lgy98io = temp;
		renewal = TRUE;
	}
	for (i = 0; i < NELEMENTS(lgy98_intr_str); i++) {
		if (strcmp(lgy98_intr, lgy98_intr_str[i]) == 0) {
			switch(i) {
			case 0:
				temp = 3;
				break;
			case 1:
				temp = 5;
				break;
			case 2:
				temp = 6;
				break;
			case 3:
				temp = 12;
				break;
			}
			if (np2cfg.lgy98irq != temp) {
				np2cfg.lgy98irq = temp;
				renewal = TRUE;
			}
			break;
		}
	}
#endif	/* SUPPORT_LGY98 */

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
	GtkWidget *table;

	root_widget = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(root_widget), 5);
	gtk_widget_show(root_widget);

	table = gtk_table_new(4, 1, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 5);
	gtk_table_set_col_spacings(GTK_TABLE(table), 5);
	gtk_box_pack_start(GTK_BOX(root_widget), table, FALSE, FALSE, 0);
	gtk_widget_show(table);

	/* Enable Power Management */
	powmng_checkbutton = gtk_check_button_new_with_label("Enable Power Management");
	gtk_widget_show(powmng_checkbutton);
	gtk_table_attach_defaults(GTK_TABLE(table), powmng_checkbutton, 2, 3, 0, 1);
	if (np2cfg.np2netpmm)
		g_signal_emit_by_name(G_OBJECT(powmng_checkbutton), "clicked");

	return root_widget;
}

#if defined(SUPPORT_LGY98)
static GtkWidget *
create_lgy98_note(void)
{
	GtkWidget *root_widget;
	GtkWidget *table;
	GtkWidget *ioport_label;
	GtkWidget *ioport_combo;
	GtkWidget *int_label;
	GtkWidget *int_combo;
	int i;
	char temp[16];

	root_widget = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(root_widget), 5);
	gtk_widget_show(root_widget);

	/* Enable */
	lgy98_en_checkbutton = gtk_check_button_new_with_label("Enable");
	gtk_widget_show(lgy98_en_checkbutton);
	gtk_container_add(GTK_CONTAINER(root_widget), lgy98_en_checkbutton);
	if (np2cfg.uselgy98)
		g_signal_emit_by_name(G_OBJECT(lgy98_en_checkbutton), "clicked");

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
	for (i = NELEMENTS(lgy98_ioport_str) - 1; i >= 0; i--) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(ioport_combo), lgy98_ioport_str[i]);
	}

	lgy98_ioport_entry = gtk_bin_get_child(GTK_BIN(ioport_combo));
	gtk_widget_show(lgy98_ioport_entry);
	gtk_editable_set_editable(GTK_EDITABLE(lgy98_ioport_entry), FALSE);
	sprintf(temp, "%04X", np2cfg.lgy98io);
	gtk_entry_set_text(GTK_ENTRY(lgy98_ioport_entry), temp);

	/* interrupt */
	int_label = gtk_label_new("Interrupt");
	gtk_widget_show(int_label);
	gtk_table_attach_defaults(GTK_TABLE(table), int_label, 2, 3, 0, 1);

	int_combo = gtk_combo_box_entry_new_text();
	gtk_widget_show(int_combo);
	gtk_table_attach_defaults(GTK_TABLE(table), int_combo, 3, 4, 0, 1);
	gtk_widget_set_size_request(int_combo, 80, -1);
	for (i = 0; i < NELEMENTS(lgy98_intr_str); i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(int_combo), lgy98_intr_str[i]);
	}

	lgy98_int_entry = gtk_bin_get_child(GTK_BIN(int_combo));
	gtk_widget_show(lgy98_int_entry);
	gtk_editable_set_editable(GTK_EDITABLE(lgy98_int_entry), FALSE);
	switch(np2cfg.lgy98irq) {
	case 3:
		i = 0;
		break;
	case 5:
		i = 1;
		break;
	case 6:
		i = 2;
		break;
	case 12:
		i = 3;
		break;
	}
	gtk_entry_set_text(GTK_ENTRY(lgy98_int_entry), lgy98_intr_str[i]);

	return root_widget;
}
#endif	/* defined(SUPPORT_LGY98) */

void
create_network_dialog(void)
{
	GtkWidget *network_dialog;
	GtkWidget *main_vbox;
	GtkWidget *notebook;
	GtkWidget *general_note;
#if defined(SUPPORT_LGY98)
	GtkWidget *lgy98_note;
#endif	/* SUPPORT_LGY98 */
	GtkWidget *confirm_widget;
	GtkWidget *ok_button;
	GtkWidget *cancel_button;

	uninstall_idle_process();

	network_dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(network_dialog), "Network option");
	gtk_window_set_position(GTK_WINDOW(network_dialog), GTK_WIN_POS_CENTER);
	gtk_window_set_modal(GTK_WINDOW(network_dialog), TRUE);
	gtk_window_set_resizable(GTK_WINDOW(network_dialog), FALSE);

	g_signal_connect(G_OBJECT(network_dialog), "destroy",
	    G_CALLBACK(dialog_destroy), NULL);

	main_vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(main_vbox);
	gtk_container_add(GTK_CONTAINER(network_dialog), main_vbox);

	notebook = gtk_notebook_new();
	gtk_widget_show(notebook);
	gtk_box_pack_start(GTK_BOX(main_vbox), notebook, TRUE, TRUE, 0);

	/* "General" note */
	general_note = create_general_note();
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), general_note, gtk_label_new("General"));

#if defined(SUPPORT_LGY98)
	/* "LGY-98" note */
	lgy98_note = create_lgy98_note();
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), lgy98_note, gtk_label_new("LGY-98"));
#endif	/* SUPPORT_LGY98 */

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
	    G_CALLBACK(gtk_widget_destroy), G_OBJECT(network_dialog));

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
	    G_CALLBACK(ok_button_clicked), (gpointer)network_dialog);
	gtk_widget_grab_default(ok_button);

	gtk_widget_show_all(network_dialog);
}
