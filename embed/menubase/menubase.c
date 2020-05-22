#include	<compiler.h>
#include	<fontmng.h>
#include	<inputmng.h>
#include	<scrnmng.h>
#include	"taskmng.h"
#include	<embed/vramhdl.h>
#include	"menudeco.inc"
#include	<embed/menubase/menubase.h>


	VRAMHDL		menuvram;
	MENUBASE	menubase;


BRESULT menubase_create(void) {

	MENUBASE	*mb;

	mb = &menubase;
	mb->font = fontmng_create(MENU_FONTSIZE, FDAT_PROPORTIONAL, NULL);
	mb->font2 = fontmng_create(MENU_FONTSIZE, 0, NULL);
	menuicon_initialize();
	return(SUCCESS);
}

void menubase_destroy(void) {

	MENUBASE	*mb;

	menuicon_deinitialize();
	mb = &menubase;
	fontmng_destroy(mb->font2);
	fontmng_destroy(mb->font);
	ZeroMemory(mb, sizeof(MENUBASE));
}

BRESULT menubase_open(int num) {

	MENUBASE	*mb;
	SCRNMENU	smenu;
	VRAMHDL		hdl;

	mb = &menubase;
	menubase_close();

	if (scrnmng_entermenu(&smenu) != SUCCESS) {
		goto mbopn_err;
	}
	mb->width = smenu.width;
	mb->height = smenu.height;
	mb->bpp = smenu.bpp;
	hdl = vram_create(mb->width, mb->height, TRUE, smenu.bpp);
	menuvram = hdl;
	if (hdl == NULL) {
		goto mbopn_err;
	}
	unionrect_rst(&mb->rect);
	mb->num = num;
	return(SUCCESS);

mbopn_err:
	return(FAILURE);
}

void menubase_close(void) {

	MENUBASE	*mb;
	VRAMHDL		hdl;
	int			num;

	mb = &menubase;
	num = mb->num;
	if (num) {
		mb->num = 0;
		if (num == 1) {
			menusys_close();
		}
		else {
			menudlg_destroy();
		}
		hdl = menuvram;
		if (hdl) {
			menubase_draw(NULL, NULL);
			menuvram = NULL;
			vram_destroy(hdl);
		}
		scrnmng_leavemenu();
	}
}

BRESULT menubase_moving(int x, int y, int btn) {

	int		num;

	num = menubase.num;
	if (num == 1) {
		menusys_moving(x, y, btn);
	}
	else if (num) {
		menudlg_moving(x, y, btn);
	}
	return(SUCCESS);
}

BRESULT menubase_key(UINT key) {

	int		num;

	num = menubase.num;
	if (num == 1) {
		menusys_key(key);
	}
	return(SUCCESS);
}

void menubase_setrect(VRAMHDL vram, const RECT_T *rect) {

	RECT_T	rct;

	if (vram) {
		if (rect == NULL) {
			vram_getrect(vram, &rct);
		}
		else {
			rct.left = vram->posx + rect->left;
			rct.top = vram->posy + rect->top;
			rct.right = vram->posx + rect->right;
			rct.bottom = vram->posy + rect->bottom;
		}
		unionrect_add(&menubase.rect, &rct);
	}
}

void menubase_clrrect(VRAMHDL vram) {

	RECT_T	rct;

	if (vram) {
		vram_getrect(vram, &rct);
		vram_fillalpha(menuvram, &rct, 1);
		menubase_setrect(vram, NULL);
//		movieredraw = 1;
	}
}

void menubase_draw(void (*draw)(VRAMHDL dst, const RECT_T *rect, void *arg),
																void *arg) {

	MENUBASE	*mb;
const	RECT_T	*rect;

	mb = &menubase;
	if (mb->rect.type) {
		rect = unionrect_get(&mb->rect);
		if (draw) {
			draw(menuvram, rect, arg);
		}
		scrnmng_menudraw(rect);
		unionrect_rst(&mb->rect);
	} else {
		scrnmng_updatecursor();
	}
}


// ----

void menubase_proc(void) {
}

void menubase_modalproc(void) {

	while((taskmng_sleep(5)) && (menuvram != NULL)) {
	}
}

