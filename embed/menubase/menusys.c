#include	"compiler.h"
#include	"fontmng.h"
#include	"inputmng.h"
#include	"vramhdl.h"
#include	"vrammix.h"
#include	"menudeco.inc"
#include	"menubase.h"


typedef struct _mhdl {
struct _mhdl	*chain;
struct _mhdl	*next;
struct _mhdl	*child;
	MENUID		id;
	MENUFLG		flag;
	RECT_T		rct;
	OEMCHAR		string[32];
} _MENUHDL, *MENUHDL;

typedef struct {
	VRAMHDL		vram;
	MENUHDL		menu;
	int			items;
	int			focus;
} _MSYSWND, *MSYSWND;

typedef struct {
	_MSYSWND	wnd[MENUSYS_MAX];
	LISTARRAY	res;
	MENUHDL		lastres;
	MENUHDL		root;
	UINT16		icon;
	UINT16		style;
	void		(*cmd)(MENUID id);
	int			depth;
	int			opened;
	int			lastdepth;
	int			lastpos;
	int			popupx;
	int			popupy;
	OEMCHAR		title[128];
} MENUSYS;


static MENUSYS	menusys;

#if defined(OSLANG_SJIS) && !defined(RESOURCE_US)
static const OEMCHAR str_sysr[] = 			// 元のサイズに戻す
			"\214\263\202\314\203\124\203\103\203\131\202\311" \
			"\226\337\202\267";
static const OEMCHAR str_sysm[] =			// 移動
			"\210\332\223\256";
static const OEMCHAR str_syss[] =			// サイズ変更
			"\203\124\203\103\203\131\225\317\215\130";
static const OEMCHAR str_sysn[] =			// 最小化
			"\215\305\217\254\211\273";
static const OEMCHAR str_sysx[] =			// 最大化
			"\215\305\221\345\211\273";
static const OEMCHAR str_sysc[] =			// 閉じる
			"\225\302\202\266\202\351";
#elif defined(OSLANG_EUC) && !defined(RESOURCE_US)
static const OEMCHAR str_sysr[] = 			// 元のサイズに戻す
			"\270\265\244\316\245\265\245\244\245\272\244\313" \
			"\314\341\244\271";
static const OEMCHAR str_sysm[] =			// 移動
			"\260\334\306\260";
static const OEMCHAR str_syss[] =			// サイズ変更
			"\245\265\245\244\245\272\312\321\271\271";
static const OEMCHAR str_sysn[] =			// 最小化
			"\272\307\276\256\262\275";
static const OEMCHAR str_sysx[] =			// 最大化
			"\272\307\302\347\262\275";
static const OEMCHAR str_sysc[] =			// 閉じる
			"\312\304\244\270\244\353";
#elif defined(OSLANG_UTF8) && !defined(RESOURCE_US)
static const OEMCHAR str_sysr[] = 			// 元のサイズに戻す
			"\345\205\203\343\201\256\343\202\265\343\202\244\343\202\272" \
			"\343\201\253\346\210\273\343\201\231";
static const OEMCHAR str_sysm[] =			// 移動
			"\347\247\273\345\213\225";
static const OEMCHAR str_syss[] =			// サイズ変更
			"\343\202\265\343\202\244\343\202\272\345\244\211\346\233\264";
static const OEMCHAR str_sysn[] =			// 最小化
			"\346\234\200\345\260\217\345\214\226";
static const OEMCHAR str_sysx[] =			// 最大化
			"\346\234\200\345\244\247\345\214\226";
static const OEMCHAR str_sysc[] =			// 閉じる
			"\351\226\211\343\201\230\343\202\213";
#else
static const OEMCHAR str_sysr[] = OEMTEXT("Restore");
static const OEMCHAR str_sysm[] = OEMTEXT("Move");
static const OEMCHAR str_syss[] = OEMTEXT("Size");
static const OEMCHAR str_sysn[] = OEMTEXT("Minimize");
static const OEMCHAR str_sysx[] = OEMTEXT("Maximize");
static const OEMCHAR str_sysc[] = OEMTEXT("Close");
#endif


static const MSYSITEM s_exit[] = {
		{str_sysr,			NULL,		0,				MENU_GRAY},
		{str_sysm,			NULL,		0,				MENU_GRAY},
		{str_syss,			NULL,		0,				MENU_GRAY},
#if defined(MENU_TASKMINIMIZE)
		{str_sysn,			NULL,		SID_MINIMIZE,	0},
#else
		{str_sysn,			NULL,		0,				MENU_GRAY},
#endif
		{str_sysx,			NULL,		0,				MENU_GRAY},
		{NULL,				NULL,		0,				MENU_SEPARATOR},
		{str_sysc,			NULL,		SID_CLOSE,		MENU_DELETED}};

static const MSYSITEM s_root[] = {
		{NULL,				s_exit,		0,				MENUS_SYSTEM},
#if defined(MENU_TASKMINIMIZE)
		{NULL,				NULL,		SID_MINIMIZE,	MENUS_MINIMIZE},
#endif
		{NULL,				NULL,		SID_CLOSE,		MENUS_CLOSE |
														MENU_DELETED}};


// ---- regist

static BOOL seaempty(void *vpItem, void *vpArg) {

	if (((MENUHDL)vpItem)->flag & MENU_DELETED) {
		return(TRUE);
	}
	(void)vpArg;
	return(FALSE);
}

static MENUHDL append1(MENUSYS *sys, const MSYSITEM *item) {

	MENUHDL		ret;
	_MENUHDL	hdl;

	ZeroMemory(&hdl, sizeof(hdl));
	hdl.id = item->id;
	hdl.flag = item->flag & (~MENU_DELETED);
	if (item->string) {
		milstr_ncpy(hdl.string, item->string, NELEMENTS(hdl.string));
	}
	ret = (MENUHDL)listarray_enum(sys->res, seaempty, NULL);
	if (ret) {
		*ret = hdl;
	}
	else {
		ret = (MENUHDL)listarray_append(sys->res, &hdl);
	}
	if (ret) {
		if (sys->lastres) {
			sys->lastres->chain = ret;
		}
		sys->lastres = ret;
	}
	return(ret);
}

static MENUHDL appends(MENUSYS *sys, const MSYSITEM *item) {

	MENUHDL		ret;
	MENUHDL		cur;

	ret = append1(sys, item);
	cur = ret;
	while(1) {
		if (cur == NULL) {
			goto ap_err;
		}
		if (item->child) {
			cur->child = appends(sys, item->child);
		}
		if (item->flag & MENU_DELETED) {
			break;
		}
		item++;
		cur->next = append1(sys, item);
		cur = cur->next;
	}
	return(ret);

ap_err:
	return(NULL);
}


// ----

static void draw(VRAMHDL dst, const RECT_T *rect, void *arg) {

	MENUSYS		*sys;
	int			cnt;
	MSYSWND		wnd;

	sys = &menusys;
	wnd = sys->wnd;
	cnt = sys->depth;
	while(cnt--) {
		vrammix_cpy2(dst, rect, wnd->vram, NULL, 2);
		wnd++;
	}
	(void)arg;
}

static MENUHDL getitem(MENUSYS *sys, int depth, int pos) {

	MENUHDL	ret;

	if ((unsigned int)depth >= (unsigned int)sys->depth) {
		goto gi_err;
	}
	ret = sys->wnd[depth].menu;
	while(ret) {
		if (!pos) {
			if (!(ret->flag & (MENU_DISABLE | MENU_SEPARATOR))) {
				return(ret);
			}
			else {
				break;
			}
		}
		pos--;
		ret = ret->next;
	}

gi_err:
	return(NULL);
}

static void wndclose(MENUSYS *sys, int depth) {

	MSYSWND		wnd;

	sys->depth = depth;
	wnd = sys->wnd + depth;
	while(depth < MENUSYS_MAX) {
		menubase_clrrect(wnd->vram);
		vram_destroy(wnd->vram);
		wnd->vram = NULL;
		wnd++;
		depth++;
	}
}

static void bitemdraw(VRAMHDL vram, MENUHDL menu, int flag) {

	void	*font;
	POINT_T	pt;
	UINT32	color;
	int		pos;
	int		menutype;

	font = menubase.font;
	menutype = menu->flag & MENUS_CTRLMASK;
	if (menutype == 0) {
		vram_filldat(vram, &menu->rct, menucolor[MVC_STATIC]);
		pos = 0;
		if (flag) {
			pos = 1;
		}
		if (!(menu->flag & MENU_GRAY)) {
			color = menucolor[MVC_TEXT];
		}
		else {
#if 0
			if (flag == 2) {
				flag = 0;
				pos = 0;
			}
#endif
			pt.x = menu->rct.left + pos + MENUSYS_SXSYS + MENU_DSTEXT;
			pt.y = menu->rct.top + pos + MENUSYS_SYSYS + MENU_DSTEXT;
			vrammix_text(vram, font, menu->string,
									menucolor[MVC_GRAYTEXT2], &pt, NULL);
			color = menucolor[MVC_GRAYTEXT1];
		}
		pt.x = menu->rct.left + pos + MENUSYS_SXSYS;
		pt.y = menu->rct.top + pos + MENUSYS_SYSYS;
		vrammix_text(vram, font, menu->string, color, &pt, NULL);
		if (flag) {
			menuvram_box(vram, &menu->rct,
								MVC2(MVC_SHADOW, MVC_HILIGHT), (flag==2));
		}
	}
}

enum {
	MEXIST_SYS		= 0x01,
	MEXIST_MINIMIZE	= 0x02,
	MEXIST_CLOSE	= 0x04,
	MEXIST_ITEM		= 0x08
};

static BRESULT wndopenbase(MENUSYS *sys) {

	MENUHDL menu;
	RECT_T	mrect;
	VRAMHDL	vram;
	int		items;
	int		posx;
	UINT	rootflg;
	int		menutype;
	int		height;
	POINT_T	pt;

	wndclose(sys, 0);

	rootflg = 0;
	menu = sys->root;
	while(menu) {					// メニュー内容を調べる。
		if (!(menu->flag & (MENU_DISABLE | MENU_SEPARATOR))) {
			switch(menu->flag & MENUS_CTRLMASK) {
				case MENUS_POPUP:
					break;

				case MENUS_SYSTEM:
					rootflg |= MEXIST_SYS;
					break;

				case MENUS_MINIMIZE:
					rootflg |= MEXIST_MINIMIZE;
					break;

				case MENUS_CLOSE:
					rootflg |= MEXIST_CLOSE;
					break;

				default:
					rootflg |= MEXIST_ITEM;
					break;
			}
		}
		menu = menu->next;
	}

	mrect.left = MENU_FBORDER + MENU_BORDER;
	mrect.top = MENU_FBORDER + MENU_BORDER;
	mrect.right = menubase.width - (MENU_FBORDER + MENU_BORDER);
	mrect.bottom = (MENU_FBORDER + MENU_BORDER) + MENUSYS_CYCAPTION;
	height = ((MENU_FBORDER + MENU_BORDER) * 2) + MENUSYS_CYCAPTION;
	if (rootflg & MEXIST_ITEM) {
		height += (MENUSYS_BCAPTION * 3) + MENUSYS_CYSYS;
		mrect.left += MENUSYS_BCAPTION;
		mrect.top += MENUSYS_BCAPTION;
		mrect.right -= MENUSYS_BCAPTION;
		mrect.bottom += MENUSYS_BCAPTION;
	}
	vram = menuvram_create(menubase.width, height, menubase.bpp);
	sys->wnd[0].vram = vram;
	if (vram == NULL) {
		goto wopn0_err;
	}
	if (sys->style & MENUSTYLE_BOTTOM) {
		vram->posy = max(0, menuvram->height - height);
	}
	menuvram_caption(vram, &mrect, sys->icon, sys->title);
	menubase_setrect(vram, NULL);
	menu = sys->root;
	sys->wnd[0].menu = menu;
	sys->wnd[0].focus = -1;
	sys->depth++;
	items = 0;
	posx = MENU_FBORDER + MENU_BORDER + MENUSYS_BCAPTION;
	while(menu) {
		if (!(menu->flag & (MENU_DISABLE | MENU_SEPARATOR))) {
			menutype = menu->flag & MENUS_CTRLMASK;
			if (menutype == MENUS_POPUP) {
			}
			else if (menutype == MENUS_SYSTEM) {
				menu->rct.left = mrect.left + MENU_PXCAPTION;
				menu->rct.right = menu->rct.left;
				menu->rct.top = mrect.top + MENU_PYCAPTION;
				menu->rct.bottom = menu->rct.top;
				if (sys->icon) {
					menu->rct.right += MENUSYS_SZICON;
					menu->rct.bottom += MENUSYS_SZICON;
				}
			}
			else if (menutype == MENUS_MINIMIZE) {
				menu->rct.right = mrect.right - MENU_PXCAPTION;
				if (rootflg & MEXIST_CLOSE) {
					menu->rct.right -= MENUSYS_CXCLOSE + (MENU_LINE * 2);
				}
				menu->rct.left = menu->rct.right - MENUSYS_CXCLOSE;
				menu->rct.top = mrect.top +
								((MENUSYS_CYCAPTION - MENUSYS_CYCLOSE) / 2);
				menu->rct.bottom = menu->rct.top + MENUSYS_CYCLOSE;
				menuvram_minimizebtn(vram, &menu->rct, 0);
			}
			else if (menutype == MENUS_CLOSE) {
				menu->rct.right = mrect.right - MENU_PXCAPTION;
				menu->rct.left = menu->rct.right - MENUSYS_CXCLOSE;
				menu->rct.top = mrect.top +
								((MENUSYS_CYCAPTION - MENUSYS_CYCLOSE) / 2);
				menu->rct.bottom = menu->rct.top + MENUSYS_CYCLOSE;
				menuvram_closebtn(vram, &menu->rct, 0);
			}
			else {
				menu->rct.left = posx;
				menu->rct.top = mrect.bottom + MENUSYS_BCAPTION;
				menu->rct.bottom = menu->rct.top + MENUSYS_CYSYS;
				fontmng_getsize(menubase.font, menu->string, &pt);
				posx += MENUSYS_SXSYS + pt.x + MENUSYS_LXSYS;
				if (posx >= (menubase.width -
						(MENU_FBORDER + MENU_BORDER + MENUSYS_BCAPTION))) {
					break;
				}
				menu->rct.right = posx;
				bitemdraw(vram, menu, 0);
			}
		}
		items++;
		menu = menu->next;
	}
	sys->wnd[0].items = items;
	return(SUCCESS);

wopn0_err:
	return(FAILURE);
}


// ----

static void citemdraw2(VRAMHDL vram, MENUHDL menu, UINT mvc, int pos) {

const MENURES2	*res;
	POINT_T		pt;

	res = menures_sys;
	if (menu->flag & MENU_CHECKED) {
		pt.x = menu->rct.left + MENUSYS_SXITEM + pos,
		pt.y = menu->rct.top + pos;
		menuvram_res3put(vram, res, &pt, mvc);
	}
	if (menu->child) {
		pt.x = menu->rct.right - MENUSYS_SXITEM - res[1].width + pos,
		pt.y = menu->rct.top + pos;
		menuvram_res3put(vram, res+1, &pt, mvc);
	}
}

static void citemdraw(VRAMHDL vram, MENUHDL menu, int flag) {

	POINT_T	pt;
	void	*font;
	int		left;
	int		right;
	int		top;
	UINT32	txtcol;

	vram_filldat(vram, &menu->rct, (flag != 0)?0x000080:0xc0c0c0);

	if (menu->flag & MENU_SEPARATOR) {
		left = menu->rct.left + MENUSYS_SXSEP;
		right = menu->rct.right - MENUSYS_LXSEP;
		top = menu->rct.top + MENUSYS_SYSEP;
		menuvram_linex(vram, left, top, right, MVC_SHADOW);
		menuvram_linex(vram, left, top + MENU_LINE, right, MVC_HILIGHT);
	}
	else {
		left = menu->rct.left + MENUSYS_SXITEM + MENUSYS_CXCHECK;
		top = menu->rct.top + MENUSYS_SYITEM;
		font = menubase.font;
		if (!(menu->flag & MENU_GRAY)) {
			txtcol = (flag != 0)?MVC_CURTEXT:MVC_TEXT;
		}
		else {
			if (flag == 0) {
				pt.x = left + MENU_DSTEXT;
				pt.y = top + MENU_DSTEXT;
				vrammix_text(vram, font, menu->string,
										menucolor[MVC_GRAYTEXT2], &pt, NULL);
				citemdraw2(vram, menu, MVC_GRAYTEXT2, 1);
			}
			txtcol = MVC_GRAYTEXT1;
		}
		pt.x = left;
		pt.y = top;
		vrammix_text(vram, font, menu->string, menucolor[txtcol], &pt, NULL);
		citemdraw2(vram, menu, txtcol, 0);
	}
}

static BRESULT childopn(MENUSYS *sys, int depth, int pos) {

	MENUHDL	menu;
	RECT_T	parent;
	int		dir;
	int		width;
	int		height;
	int		items;
	MSYSWND	wnd;
	int		drawitems;
	POINT_T	pt;

	menu = getitem(sys, depth, pos);
	if ((menu == NULL) || (menu->child == NULL)) {
		TRACEOUT(("child not found."));
		goto copn_err;
	}
	wnd = sys->wnd + depth;
	if ((menu->flag & MENUS_CTRLMASK) == MENUS_POPUP) {
		parent.left = sys->popupx;
		parent.top = max(sys->popupy, wnd->vram->height);
		parent.right = parent.left;
		parent.bottom = parent.top;
		dir = 0;
	}
	else {
		parent.left = wnd->vram->posx + menu->rct.left;
		parent.top = wnd->vram->posy + menu->rct.top;
		parent.right = wnd->vram->posx + menu->rct.right;
		parent.bottom = wnd->vram->posy + menu->rct.bottom;
		dir = depth + 1;
	}
	if (depth >= (MENUSYS_MAX - 1)) {
		TRACEOUT(("menu max."));
		goto copn_err;
	}
	wnd++;
	width = 0;
	height = (MENU_FBORDER + MENU_BORDER);
	items = 0;
	drawitems = 0;
	menu = menu->child;
	wnd->menu = menu;
	while(menu) {
		if (!(menu->flag & MENU_DISABLE)) {
			menu->rct.left = (MENU_FBORDER + MENU_BORDER);
			menu->rct.top = height;
			if (menu->flag & MENU_SEPARATOR) {
				if (height > (menubase.height - MENUSYS_CYSEP -
											(MENU_FBORDER + MENU_BORDER))) {
					break;
				}
				height += MENUSYS_CYSEP;
				menu->rct.bottom = height;
			}
			else {
				if (height > (menubase.height - MENUSYS_CYITEM -
											(MENU_FBORDER + MENU_BORDER))) {
					break;
				}
				height += MENUSYS_CYITEM;
				menu->rct.bottom = height;
				fontmng_getsize(menubase.font, menu->string, &pt);
				if (width < pt.x) {
					width = pt.x;
				}
			}
		}
		items++;
		menu = menu->next;
	}
	width += ((MENU_FBORDER + MENU_BORDER + MENUSYS_SXITEM) * 2) +
										MENUSYS_CXCHECK + MENUSYS_CXNEXT;
	if (width >= menubase.width) {
		width = menubase.width;
	}
	height += (MENU_FBORDER + MENU_BORDER);
	wnd->vram = menuvram_create(width, height, menubase.bpp);
	if (wnd->vram == NULL) {
		TRACEOUT(("sub menu vram couldn't create"));
		goto copn_err;
	}
	if (dir == 1) {
		if ((parent.top < height) ||
			(parent.bottom < (menubase.height - height))) {
			parent.top = parent.bottom;
		}
		else {
			parent.top -= height;
		}
	}
	else if (dir >= 2) {
		if ((parent.left < width) ||
			(parent.right < (menubase.width - width))) {
			parent.left = parent.right;
		}
		else {
			parent.left -= width;
		}
		if ((parent.top > (menubase.height - height)) &&
			(parent.bottom >= height)) {
			parent.top = parent.bottom - height;
		}
	}
	wnd->vram->posx = min(parent.left, menubase.width - width);
	wnd->vram->posy = min(parent.top, menubase.height - height);
	wnd->items = items;
	wnd->focus = -1;
	sys->depth++;
	menu = wnd->menu;
	drawitems = items;
	while(drawitems--) {
		if (!(menu->flag & MENU_DISABLE)) {
			menu->rct.right = width - (MENU_FBORDER + MENU_BORDER);
			citemdraw(wnd->vram, menu, 0);
		}
		menu = menu->next;
	}
	menubase_setrect(wnd->vram, NULL);
	return(SUCCESS);

copn_err:
	return(FAILURE);
}

static int openpopup(MENUSYS *sys) {

	MENUHDL menu;
	int		pos;

	if (sys->depth == 1) {
		pos = 0;
		menu = sys->root;
		while(menu) {
			if (!(menu->flag & (MENU_DISABLE | MENU_SEPARATOR))) {
				if ((menu->flag & MENUS_CTRLMASK) == MENUS_POPUP) {
					sys->wnd[0].focus = pos;
					childopn(sys, 0, pos);
					return(1);
				}
			}
			menu = menu->next;
			pos++;
		}
	}
	return(0);
}

static void itemdraw(MENUSYS *sys, int depth, int pos, int flag) {

	MENUHDL		menu;
	VRAMHDL	vram;
	void		(*drawfn)(VRAMHDL vram, MENUHDL menu, int flag);

	menu = getitem(sys, depth, pos);
	if (menu) {
		vram = sys->wnd[depth].vram;
		drawfn = (depth)?citemdraw:bitemdraw;
		drawfn(vram, menu, flag);
		menubase_setrect(vram, &menu->rct);
	}
}


// ----

typedef struct {
	int		depth;
	int		pos;
	MSYSWND	wnd;
	MENUHDL	menu;
} MENUPOS;

static void getposinfo(MENUSYS *sys, MENUPOS *pos, int x, int y) {

	RECT_T		rct;
	int			cnt;
	MSYSWND		wnd;
	MENUHDL		menu;

	cnt = sys->depth;
	wnd = sys->wnd + cnt;
	while(cnt--) {
		wnd--;
		if (wnd->vram) {
			vram_getrect(wnd->vram, &rct);
			if (rect_in(&rct, x, y)) {
				x -= wnd->vram->posx;
				y -= wnd->vram->posy;
				break;
			}
		}
	}
	if (cnt >= 0) {
		pos->depth = cnt;
		pos->wnd = wnd;
		menu = wnd->menu;
		cnt = 0;
		while((menu) && (cnt < wnd->items)) {
			if (!(menu->flag & (MENU_DISABLE | MENU_SEPARATOR))) {
				if (rect_in(&menu->rct, x, y)) {
					pos->pos = cnt;
					pos->menu = menu;
					return;
				}
			}
			cnt++;
			menu = menu->next;
		}
	}
	else {
		pos->depth = -1;
		pos->wnd = NULL;
	}
	pos->pos = -1;
	pos->menu = NULL;
}


// ----

static void defcmd(MENUID id) {

	(void)id;
}

BRESULT menusys_create(const MSYSITEM *item, void (*cmd)(MENUID id),
										UINT16 icon, const OEMCHAR *title) {

	MENUSYS		*ret;
	LISTARRAY	r;
	MENUHDL		hdl;

	ret = &menusys;
	ZeroMemory(ret, sizeof(MENUSYS));
	ret->icon = icon;
	if (cmd == NULL) {
		cmd = defcmd;
	}
	ret->cmd = cmd;
	if (title) {
		milstr_ncpy(ret->title, title, NELEMENTS(ret->title));
	}
	r = listarray_new(sizeof(_MENUHDL), 32);
	if (r == NULL) {
		goto mscre_err;
	}
	ret->res = r;
	hdl = appends(ret, s_root);
	if (hdl == NULL) {
		goto mscre_err;
	}
	ret->root = hdl;
	if (item) {
		while(hdl->next) {
			hdl = hdl->next;
		}
		hdl->next = appends(ret, item);
	}
	return(SUCCESS);

mscre_err:
	return(FAILURE);
}

void menusys_destroy(void) {

	MENUSYS	*sys;

	sys = &menusys;
	wndclose(sys, 0);
	if (sys->res) {
		listarray_destroy(sys->res);
	}
}

BRESULT menusys_open(int x, int y) {

	MENUSYS	*sys;

	sys = &menusys;

	if (menubase_open(1) != SUCCESS) {
		goto msopn_err;
	}
	sys->opened = 0;
	sys->lastdepth = -1;
	sys->lastpos = -1;
	if (wndopenbase(sys) != SUCCESS) {
		goto msopn_err;
	}
	sys->popupx = x;
	sys->popupy = y;
	sys->opened = openpopup(sys);
	menubase_draw(draw, sys);
	return(SUCCESS);

msopn_err:
	menubase_close();
	return(FAILURE);
}

void menusys_close(void) {

	MENUSYS	*sys;

	sys = &menusys;
	wndclose(sys, 0);
}

void menusys_moving(int x, int y, int btn) {

	MENUSYS	*sys;
	MENUPOS	cur;
	int		topwnd;

	sys = &menusys;
	getposinfo(sys, &cur, x, y);

	// メニューを閉じる～
	if (cur.depth < 0) {
		if (btn == 2) {
			menubase_close();
			return;
		}
	}
	topwnd = sys->depth - 1;
	if (cur.menu != NULL) {
		if (cur.wnd->focus != cur.pos) {
			if (sys->opened) {
				if (cur.depth != topwnd) {
					wndclose(sys, cur.depth + 1);
				}
				if ((!(cur.menu->flag & MENU_GRAY)) &&
					(cur.menu->child != NULL)) {
					childopn(sys, cur.depth, cur.pos);
				}
			}
			itemdraw(sys, cur.depth, cur.wnd->focus, 0);
			itemdraw(sys, cur.depth, cur.pos, 2 - sys->opened);
			cur.wnd->focus = cur.pos;
		}
		if (!(cur.menu->flag & MENU_GRAY)) {
			if (btn == 1) {
				if ((!sys->opened) && (cur.depth == 0) &&
					(cur.menu->child != NULL)) {
					wndclose(sys, 1);
					itemdraw(sys, 0, cur.pos, 1);
					childopn(sys, 0, cur.pos);
					sys->opened = 1;
				}
			}
			else if (btn == 2) {
				if ((cur.menu->id) && (!(cur.menu->flag & MENU_NOSEND))) {
					menubase_close();
					sys->cmd(cur.menu->id);
					return;
				}
			}
		}
	}
	else {
		if ((btn == 1) && (cur.depth == 0)) {
			wndclose(sys, 1);
			itemdraw(sys, 0, cur.wnd->focus, 0);
			sys->opened = openpopup(sys);
		}
		else if (cur.depth != topwnd) {
			cur.depth = topwnd;
			cur.pos = -1;
			cur.wnd = sys->wnd + cur.depth;
			if (cur.wnd->focus != cur.pos) {
				itemdraw(sys, cur.depth, cur.wnd->focus, 0);
				cur.wnd->focus = cur.pos;
			}
		}
	}
	menubase_draw(draw, sys);
}

static void focusmove(MENUSYS *sys, int depth, int dir) {

	MSYSWND	wnd;
	MENUHDL	menu;
	int		pos;
	MENUHDL	target;
	int		tarpos;

	wnd = sys->wnd + depth;
	target = NULL;
	tarpos = 0;
	pos = 0;
	menu = wnd->menu;
	while(menu) {
		if (pos == wnd->focus) {
			if ((dir < 0) && (target != NULL)) {
				break;
			}
		}
		else if (((menu->flag & MENUS_CTRLMASK) <= MENUS_SYSTEM) &&
				(!(menu->flag &
							(MENU_DISABLE | MENU_GRAY | MENU_SEPARATOR)))) {
			if (dir < 0) {
				target = menu;
				tarpos = pos;
			}
			else {
				if (pos < wnd->focus) {
					if (target == NULL) {
						target = menu;
						tarpos = pos;
					}
				}
				else {
					target = menu;
					tarpos = pos;
					break;
				}
			}
		}
		pos++;
		menu = menu->next;
	}
	if (target == NULL) {
		return;
	}
	itemdraw(sys, depth, wnd->focus, 0);
	itemdraw(sys, depth, tarpos, 2 - sys->opened);
	wnd->focus = tarpos;
//	TRACEOUT(("focus = %d", tarpos));
	if (depth == 0) {
		if (sys->opened) {
			wndclose(sys, 1);
			childopn(sys, 0, tarpos);
		}
	}
	else {
		if (depth != (sys->depth - 1)) {
			wndclose(sys, depth + 1);
		}
	}
}

static void focusenter(MENUSYS *sys, int depth, BOOL exec) {

	MENUHDL	menu;
	MSYSWND	wnd;

	wnd = sys->wnd + depth;
	menu = getitem(sys, depth, wnd->focus);
	if ((menu) && (!(menu->flag & MENU_GRAY)) &&
		(menu->child != NULL)) {
		if (depth == 0) {
			wndclose(sys, 1);
			itemdraw(sys, 0, wnd->focus, 1);
			sys->opened = 1;
		}
		childopn(sys, depth, wnd->focus);
	}
	else if (exec) {
		if ((menu) && (menu->id)) {
			menubase_close();
			sys->cmd(menu->id);
		}
	}
	else {
		focusmove(sys, 0, 1);
	}
}

void menusys_key(UINT key) {

	MENUSYS	*sys;
	int		topwnd;

	sys = &menusys;
	topwnd = sys->depth - 1;
	if (topwnd == 0) {
		if (key & KEY_LEFT) {
			focusmove(sys, 0, -1);
		}
		if (key & KEY_RIGHT) {
			focusmove(sys, 0, 1);
		}
		if (key & KEY_DOWN) {
			focusenter(sys, 0, FALSE);
		}
		if (key & KEY_ENTER) {
			focusenter(sys, 0, TRUE);
		}
	}
	else {
		if (key & KEY_UP) {
			focusmove(sys, topwnd, -1);
		}
		if (key & KEY_DOWN) {
			focusmove(sys, topwnd, 1);
		}
		if (key & KEY_LEFT) {
			if (topwnd >= 2) {
				wndclose(sys, topwnd);
			}
			else {
				focusmove(sys, 0, -1);
			}
		}
		if (key & KEY_RIGHT) {
			focusenter(sys, topwnd, FALSE);
		}
		if (key & KEY_ENTER) {
			focusenter(sys, topwnd, TRUE);
		}
	}
	menubase_draw(draw, sys);
}


// ----

typedef struct {
	MENUHDL		ret;
	MENUID		id;
} ITEMSEA;

static BOOL _itemsea(void *vpItem, void *vpArg) {

	if (((MENUHDL)vpItem)->id == ((ITEMSEA *)vpArg)->id) {
		((ITEMSEA *)vpArg)->ret = (MENUHDL)vpItem;
		return(TRUE);
	}
	return(FALSE);
}

static MENUHDL itemsea(MENUSYS *sys, MENUID id) {

	ITEMSEA		sea;

	sea.ret = NULL;
	sea.id = id;
	listarray_enum(sys->res, _itemsea, &sea);
	return(sea.ret);
}

static void menusys_setflag(MENUID id, MENUFLG flag, MENUFLG mask) {

	MENUSYS	*sys;
	MENUHDL	itm;
	int		depth;
	int		pos;
	int		focus;

	sys = &menusys;
	itm = itemsea(sys, id);
	if (itm == NULL) {
		goto mssf_end;
	}
	flag ^= itm->flag;
	flag &= mask;
	if (!flag) {
		goto mssf_end;
	}
	itm->flag ^= flag;

	// リドローが必要？
	depth = 0;
	while(depth < sys->depth) {
		itm = sys->wnd[depth].menu;
		pos = 0;
		while(itm) {
			if (itm->id == id) {
				if (!(itm->flag & (MENU_DISABLE | MENU_SEPARATOR))) {
					focus = 0;
					if (sys->wnd[depth].focus == pos) {
						focus = 2 - sys->opened;
					}
					itemdraw(sys, depth, pos, focus);
					menubase_draw(draw, sys);
					goto mssf_end;
				}
			}
			pos++;
			itm = itm->next;
		}
		depth++;
	}

mssf_end:
	return;
}

static void menusys_settxt(MENUID id, const OEMCHAR *arg) {

	MENUSYS	*sys;
	MENUHDL	itm;
	int		depth;
	int		pos;
	int		focus;

	sys = &menusys;
	itm = itemsea(sys, id);
	if (itm == NULL) {
		goto msst_end;
	}

	if (arg) {
		milstr_ncpy(itm->string, arg, NELEMENTS(itm->string));
	}
	else {
		itm->string[0] = '\0';
	}

	// リドローが必要？ (ToDo: 再オープンすべし)
	depth = 0;
	while(depth < sys->depth) {
		itm = sys->wnd[depth].menu;
		pos = 0;
		while(itm) {
			if (itm->id == id) {
				if (!(itm->flag & (MENU_DISABLE | MENU_SEPARATOR))) {
					focus = 0;
					if (sys->wnd[depth].focus == pos) {
						focus = 2 - sys->opened;
					}
					itemdraw(sys, depth, pos, focus);
					menubase_draw(draw, sys);
					goto msst_end;
				}
			}
			pos++;
			itm = itm->next;
		}
		depth++;
	}

msst_end:
	return;
}

INTPTR menusys_msg(int ctrl, MENUID id, INTPTR arg) {

	INTPTR	ret;
	MENUSYS	*sys;
	MENUHDL	itm;

	ret = 0;
	sys = &menusys;
	itm = itemsea(sys, id);
	if (itm == NULL) {
		goto msmsg_exit;
	}

	switch(ctrl) {
		case SMSG_SETHIDE:
			ret = (itm->flag & MENU_DISABLE) ? 1 : 0;
			menusys_setflag(id, (MENUFLG)((arg) ? MENU_DISABLE : 0), MENU_DISABLE);
			break;

		case SMSG_GETHIDE:
			ret = (itm->flag & MENU_DISABLE) ? 1 : 0;
			break;

		case SMSG_SETENABLE:
			ret = (itm->flag & MENU_GRAY) ? 0 : 1;
			menusys_setflag(id, (MENUFLG)((arg) ? 0 : MENU_GRAY), MENU_GRAY);
			break;

		case SMSG_GETENABLE:
			ret = (itm->flag & MENU_GRAY) ? 0 : 1;
			break;

		case SMSG_SETCHECK:
			ret = (itm->flag & MENU_CHECKED) ? 1 : 0;
			menusys_setflag(id, (MENUFLG)((arg) ? MENU_CHECKED : 0), MENU_CHECKED);
			break;

		case SMSG_GETCHECK:
			ret = (itm->flag & MENU_CHECKED) ? 1 : 0;
			break;

		case SMSG_SETTEXT:
			menusys_settxt(id, (OEMCHAR*)arg);
			break;
	}

msmsg_exit:
	return(ret);
}

void menusys_setstyle(UINT16 style) {

	menusys.style = style;
}

