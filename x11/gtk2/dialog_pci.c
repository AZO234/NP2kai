#include "compiler.h"

#include "gtk2/xnp2.h"
#include "gtk2/gtk_menu.h"

#include "np2.h"
#include "dosio.h"
#include "ini.h"
#include "pccore.h"
#include "iocore.h"

#include "sysmng.h"

#if defined(SUPPORT_PCI)

/*
 * General
 */

static const char *pci_pcmc_type_str[] = {
	"Intel 82434LX",
	"Intel 82441FX",
	"VLSI Wildcat",
};

static GtkWidget *pci_en_checkbutton;
static GtkWidget *pci_use32bios_checkbutton;
static GtkWidget *pci_pcmc_type_entry;


static void
ok_button_clicked(GtkButton *b, gpointer d)
{
	/* General */
	gint pci_en;
	gint pci_use32bios;
	const gchar *pci_pcmc_type;

	/* common */
	int i;
	int temp;
	BOOL renewal = FALSE;

	/* General */
	pci_en = gtk_toggle_button_get_active(
	    GTK_TOGGLE_BUTTON(pci_en_checkbutton));
	pci_use32bios = gtk_toggle_button_get_active(
	    GTK_TOGGLE_BUTTON(pci_use32bios_checkbutton));
	pci_pcmc_type = gtk_entry_get_text(GTK_ENTRY(pci_pcmc_type_entry));

	if (np2cfg.usepci != pci_en) {
		np2cfg.usepci = pci_en;
		renewal = TRUE;
	}
	if (np2cfg.pci_bios32 != pci_use32bios) {
		np2cfg.pci_bios32 = pci_use32bios;
		renewal = TRUE;
	}
	for (i = 0; i < NELEMENTS(pci_pcmc_type_str); i++) {
		if (strcmp(pci_pcmc_type, pci_pcmc_type_str[i]) == 0) {
			switch (i) {
			case 0:
				if(np2cfg.pci_pcmc != PCI_PCMC_82434LX) {
					np2cfg.pci_pcmc = PCI_PCMC_82434LX;
					renewal = TRUE;
				}
				break;
			case 1:
				if(np2cfg.pci_pcmc != PCI_PCMC_82441FX) {
					np2cfg.pci_pcmc = PCI_PCMC_82441FX;
					renewal = TRUE;
				}
				break;
			case 2:
				if(np2cfg.pci_pcmc != PCI_PCMC_WILDCAT) {
					np2cfg.pci_pcmc = PCI_PCMC_WILDCAT;
					renewal = TRUE;
				}
				break;
			}
			break;
		}
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

static GtkWidget *
create_general_note(void)
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
	pci_en_checkbutton = gtk_check_button_new_with_label("Enabled");
	gtk_widget_show(pci_en_checkbutton);
	gtk_container_add(GTK_CONTAINER(root_widget), pci_en_checkbutton);
	if (np2cfg.usepci)
		g_signal_emit_by_name(G_OBJECT(pci_en_checkbutton), "clicked");

	/* Type */
	type_hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(type_hbox);
	gtk_box_pack_start(GTK_BOX(root_widget), type_hbox, TRUE, TRUE, 0);

	type_label = gtk_label_new("PCMC");
	gtk_widget_show(type_label);
	gtk_container_add(GTK_CONTAINER(type_hbox), type_label);

	type_combo = gtk_combo_box_entry_new_text();
	gtk_widget_show(type_combo);
	gtk_container_add(GTK_CONTAINER(type_hbox), type_combo);
	gtk_widget_set_size_request(type_combo, 100, -1);
	for (i = 0; i < NELEMENTS(pci_pcmc_type_str); i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(type_combo), pci_pcmc_type_str[i]);
	}

	pci_pcmc_type_entry = gtk_bin_get_child(GTK_BIN(type_combo));
	gtk_widget_show(pci_pcmc_type_entry);
	gtk_editable_set_editable(GTK_EDITABLE(pci_pcmc_type_entry), FALSE);
	switch (np2cfg.pci_pcmc) {
	case PCI_PCMC_82434LX:
		gtk_entry_set_text(GTK_ENTRY(pci_pcmc_type_entry), pci_pcmc_type_str[0]);
		break;
	case PCI_PCMC_82441FX:
		gtk_entry_set_text(GTK_ENTRY(pci_pcmc_type_entry), pci_pcmc_type_str[1]);
		break;
	case PCI_PCMC_WILDCAT:
		gtk_entry_set_text(GTK_ENTRY(pci_pcmc_type_entry), pci_pcmc_type_str[2]);
		break;
	}

	/* Multi Window Mode */
	pci_use32bios_checkbutton = gtk_check_button_new_with_label("Use BIOS32 (not recommended)");
	gtk_widget_show(pci_use32bios_checkbutton);
	gtk_container_add(GTK_CONTAINER(root_widget), pci_use32bios_checkbutton);
	if (np2cfg.pci_bios32)
		g_signal_emit_by_name(G_OBJECT(pci_use32bios_checkbutton), "clicked");

	return root_widget;
}

void
create_pci_dialog(void)
{
	GtkWidget *pci_dialog;
	GtkWidget *main_vbox;
	GtkWidget *notebook;
	GtkWidget *general_note;
	GtkWidget *confirm_widget;
	GtkWidget *ok_button;
	GtkWidget *cancel_button;

	uninstall_idle_process();

	pci_dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(pci_dialog), "PCI option");
	gtk_window_set_position(GTK_WINDOW(pci_dialog), GTK_WIN_POS_CENTER);
	gtk_window_set_modal(GTK_WINDOW(pci_dialog), TRUE);
	gtk_window_set_resizable(GTK_WINDOW(pci_dialog), FALSE);

	g_signal_connect(G_OBJECT(pci_dialog), "destroy",
	    G_CALLBACK(dialog_destroy), NULL);

	main_vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(main_vbox);
	gtk_container_add(GTK_CONTAINER(pci_dialog), main_vbox);

	notebook = gtk_notebook_new();
	gtk_widget_show(notebook);
	gtk_box_pack_start(GTK_BOX(main_vbox), notebook, TRUE, TRUE, 0);

	/* "General" note */
	general_note = create_general_note();
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), general_note, gtk_label_new("General"));

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
	    G_CALLBACK(gtk_widget_destroy), G_OBJECT(pci_dialog));

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
	    G_CALLBACK(ok_button_clicked), (gpointer)pci_dialog);
	gtk_widget_grab_default(ok_button);

	gtk_widget_show_all(pci_dialog);
}
#endif

