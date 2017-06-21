#include "compiler.h"

#if defined(SUPPORT_KEYDISP)

#include "np2.h"

#include "kdispwin.h"

#include "scrnmng.h"
#include "sysmng.h"

#include "gtk2/xnp2.h"
#include "gtk2/gtk_menu.h"
#include "gtk2/gtk_drawmng.h"


static UINT32 kdwinpal[KEYDISP_PALS] = {
	0x00000000, 0xffffffff, 0xf9ff0000
};

typedef struct {
	DRAWMNG_HDL	hdl;

	GtkWidget	*window;
	_MENU_HDL	menuhdl;
} KDWIN;

static KDWIN kdwin;

static void drawkeys(void);
static void setkeydispmode(UINT8 mode);
static UINT8 kdispwin_getmode(UINT8 cfgmode);


/*
 * keydisp widget
 */

static void
kdispwin_window_destroy(GtkWidget *w, gpointer p)
{

	if (kdwin.window)
		kdwin.window = NULL;
	drawmng_release(kdwin.hdl);
	kdwin.hdl = NULL;
}

#if 0
/*
 * Menu
 */
static void
close_window(gpointer data, guint action, GtkWidget *w)
{

	xmenu_toggle_item(kdwin.menuhdl, "keydisp", FALSE);
}

static void
change_module(gpointer data, guint action, GtkWidget *w)
{

	if (kdispcfg.mode != action) {
		kdispcfg.mode = action;
		sysmng_update(SYS_UPDATEOSCFG);
		setkeydispmode(kdispwin_getmode(kdispcfg.mode));
	}
}

static void
xmenu_select_module(UINT8 mode)
{
	static const char *name[] = {
		NULL,
		"/Module/FM",
		"/Module/MIDI",
	};

	if (mode < NELEMENTS(name) && name[mode]) {
		xmenu_select_item(&kdwin.menuhdl, name[mode]);
	}
}

#define f(f)	((GtkItemFactoryCallback)(f))
static GtkItemFactoryEntry menu_items[] = {
{ "/_Module",		NULL, NULL,             0,             "<Branch>"    },
{ "/Module/_FM",	NULL, f(change_module), KDISPCFG_FM,   "<RadioItem>" },
{ "/Module/_MIDI",	NULL, f(change_module), KDISPCFG_MIDI, "/Module/FM"  },
{ "/Module/sep",	NULL, NULL,             0,             "<Separator>" },
{ "/Module/_Close",	NULL, f(close_window),  0,             NULL          },
};
#undef	f

static GtkWidget *
create_kdispwin_menu(GtkWidget *parent)
{
	GtkAccelGroup *accel_group;
	GtkWidget *menubar;

	(void)parent;

	accel_group = gtk_accel_group_new();
	kdwin.menuhdl.item_factory = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<main>", accel_group);
	gtk_item_factory_create_items(kdwin.menuhdl.item_factory, NELEMENTS(menu_items), menu_items, NULL);

	menubar = gtk_item_factory_get_widget(kdwin.menuhdl.item_factory, "<main>");

#if defined(NOSOUND)
	gtk_widget_set_sensitive(gtk_item_factory_get_widget(kdwin.menuhdl.item_factory, "/Module/FM"), FALSE);
#endif

	xmenu_select_module(kdispwin_getmode(kdispcfg.mode));

	return menubar;
}
#endif

/*
 * signal
 */
static gint
kdispwin_expose(GtkWidget *w, GdkEventExpose *ev)
{

	if (ev->type == GDK_EXPOSE) {
		if (ev->count == 0) {
			drawkeys();
			return TRUE;
		}
	}
	return FALSE;
}


/*
 * keydisp local function
 */
static UINT8
getpal8(CMNPALFN *self, UINT num)
{

	if (num < KEYDISP_PALS) {
		return kdwinpal[num] >> 24;
	}
	return 0;
}

static UINT32
getpal32(CMNPALFN *self, UINT num)
{

	if (num < KEYDISP_PALS) {
		return kdwinpal[num] & 0xffffff;
	}
	return 0;
}

static UINT16
cnvpal16(CMNPALFN *self, RGB32 pal32)
{

	return (UINT16)drawmng_makepal16(&kdwin.hdl->pal16mask, pal32);
}

static UINT8
kdispwin_getmode(UINT8 cfgmode)
{

	switch (cfgmode) {
	default:
#if !defined(NOSOUND)
	case KDISPCFG_FM:
		return KEYDISP_MODEFM;
#endif

	case KDISPCFG_MIDI:
		return KEYDISP_MODEMIDI;
	}
}

static void
drawkeys(void)
{
	CMNVRAM *vram;

	vram = drawmng_surflock(kdwin.hdl);
	if (vram) {
		keydisp_paint(vram, TRUE);
		drawmng_surfunlock(kdwin.hdl);
		drawmng_blt(kdwin.hdl, NULL, NULL);
	}
}

static void
setkdwinsize(void)
{
	int width, height;

	keydisp_getsize(&width, &height);
	drawmng_set_size(kdwin.hdl, width, height);
}

static void
setkeydispmode(UINT8 mode)
{

	keydisp_setmode(mode);
}

/*
 * Interface
 */
void
kdispwin_create(void)
{
	GtkWidget *main_widget;
#if 0
	GtkWidget *kdispwin_menu;
#endif
	GtkWidget *da;
	CMNPALFN palfn;
	UINT8 mode;

	if (kdwin.window)
		return;

	kdwin.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(kdwin.window), "Key Display");
	gtk_window_set_resizable(GTK_WINDOW(kdwin.window), FALSE);
	g_signal_connect(G_OBJECT(kdwin.window), "destroy",
	    G_CALLBACK(kdispwin_window_destroy), NULL);
	gtk_widget_realize(kdwin.window);

	main_widget = gtk_vbox_new(FALSE, 2);
	gtk_widget_show(main_widget);
	gtk_container_add(GTK_CONTAINER(kdwin.window), main_widget);

#if 0
	kdispwin_menu = create_kdispwin_menu(kdwin.window);
	gtk_box_pack_start(GTK_BOX(main_widget), kdispwin_menu, FALSE, TRUE, 0);
	gtk_widget_show(kdispwin_menu);
#endif

	kdwin.hdl = drawmng_create(kdwin.window, KEYDISP_WIDTH, KEYDISP_HEIGHT);
	if (kdwin.hdl == NULL) {
		goto destroy;
	}

	da = GTK_WIDGET(drawmng_get_widget_handle(kdwin.hdl));
	gtk_box_pack_start(GTK_BOX(main_widget), da, FALSE, TRUE, 0);
	gtk_widget_show(da);
	g_signal_connect(G_OBJECT(da), "expose_event",
	    G_CALLBACK(kdispwin_expose), NULL);

	mode = kdispwin_getmode(kdispcfg.mode);
	setkeydispmode(mode);
	setkdwinsize();
	gtk_widget_show_all(kdwin.window);

	palfn.get8 = getpal8;
	palfn.get32 = getpal32;
	palfn.cnv16 = cnvpal16;
	keydisp_setpal(&palfn);
	drawmng_invalidate(kdwin.hdl, NULL);
	return;

destroy:
	gtk_widget_destroy(kdwin.window);
	kdwin.window = NULL;
//	xmenu_toggle_item(NULL, "keydisp", FALSE);
	sysmng_update(SYS_UPDATEOSCFG);
}

void
kdispwin_destroy(void)
{

	if (kdwin.window) {
		gtk_widget_destroy(kdwin.window);
		kdwin.window = NULL;
	}
}

void
kdispwin_draw(UINT8 cnt)
{
	UINT8 flag;

	if (kdwin.window) {
		if (cnt == 0) {
			cnt = 1;
		}
		flag = keydisp_process(cnt);
		if (flag & KEYDISP_FLAGSIZING) {
			setkdwinsize();
		}
		drawmng_invalidate(kdwin.hdl, NULL);
	}
}

#endif	/* SUPPORT_KEYDISP */
