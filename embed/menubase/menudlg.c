#include	"compiler.h"
#include	"strres.h"
#include	"fontmng.h"
#include	"vramhdl.h"
#include	"vrammix.h"
#include	"menudeco.inc"
#include	"menubase.h"


typedef struct _dprm {
struct _dprm	*next;
	UINT16		width;
	UINT16		num;
	VRAMHDL		icon;
	OEMCHAR		str[96];
} _DLGPRM, *DLGPRM;

#define	PRMNEXT_EMPTY	((DLGPRM)-1)

typedef struct {
	POINT_T		pt;
	void		*font;
} DLGTEXT;

typedef struct {
	void		*font;
	int			fontsize;
} DLGTAB;

typedef struct {
	void		*font;
	SINT16		fontsize;
	SINT16		scrollbar;
	SINT16		dispmax;
	SINT16		basepos;
} DLGLIST;

typedef struct {
	SINT16		minval;
	SINT16		maxval;
	int			pos;
	UINT8		type;
	UINT8		moving;
	UINT8		sldh;
	UINT8		sldv;
} DLGSLD;

typedef struct {
	VRAMHDL		vram;
} DLGVRAM;

typedef struct _ditem {
	int			type;
	MENUID		id;
	MENUFLG		flag;
	MENUID		page;
	MENUID		group;
	RECT_T		rect;
	DLGPRM		prm;
	int			prmcnt;
	int			val;
	VRAMHDL		vram;
	union {
		DLGTEXT		dt;
		DLGTAB		dtl;
		DLGLIST		dl;
		DLGSLD		ds;
		DLGVRAM		dv;
	} c;
} _DLGHDL, *DLGHDL;

typedef struct {
	VRAMHDL		vram;
	LISTARRAY	dlg;
	LISTARRAY	res;
	int			locked;
	int			closing;
	int			sx;
	int			sy;
	void		*font;
	MENUID		page;
	MENUID		group;
	int			(*proc)(int msg, MENUID id, long param);

	int			dragflg;
	int			btn;
	int			lastx;
	int			lasty;
	MENUID		lastid;
} _MENUDLG, *MENUDLG;


static	_MENUDLG	menudlg;

static void drawctrls(MENUDLG dlg, DLGHDL hdl);


// ----

static BOOL seaprmempty(void *vpItem, void *vpArg) {

	if (((DLGPRM)vpItem)->next == PRMNEXT_EMPTY) {
		menuicon_unlock(((DLGPRM)vpItem)->icon);
		((DLGPRM)vpItem)->icon = NULL;
		return(TRUE);
	}
	(void)vpArg;
	return(FALSE);
}

static DLGPRM resappend(MENUDLG dlg, const OEMCHAR *str) {

	DLGPRM	prm;

	prm = (DLGPRM)listarray_enum(dlg->res, seaprmempty, NULL);
	if (prm == NULL) {
		prm = (DLGPRM)listarray_append(dlg->res, NULL);
	}
	if (prm) {
		prm->next = NULL;
		prm->width = 0;
		prm->num = 0;
		prm->icon = NULL;
		prm->str[0] = '\0';
		if (str) {
			milstr_ncpy(prm->str, str, NELEMENTS(prm->str));
		}
	}
	return(prm);
}

static void resattachicon(MENUDLG dlg, DLGPRM prm, UINT16 icon,
													int width, int height) {

	if (prm) {
		menuicon_unlock(prm->icon);
		prm->num = icon;
		prm->icon = menuicon_lock(icon, width, height, dlg->vram->bpp);
	}
}

static DLGPRM ressea(DLGHDL hdl, int pos) {

	DLGPRM	prm;

	if (pos >= 0) {
		prm = hdl->prm;
		while(prm) {
			if (!pos) {
				return(prm);
			}
			pos--;
			prm = prm->next;
		}
	}
	return(NULL);
}

static BOOL dsbyid(void *vpItem, void *vpArg) {

	if (((DLGHDL)vpItem)->id == (MENUID)(unsigned long)vpArg) {
		return(TRUE);
	}
	return(FALSE);
}

static DLGHDL dlghdlsea(MENUDLG dlg, MENUID id) {

	return((DLGHDL)listarray_enum(dlg->dlg, dsbyid, (void *)(long)id));
}

static BRESULT gettextsz(DLGHDL hdl, POINT_T *sz) {

	DLGPRM	prm;

	prm = hdl->prm;
	if (prm == NULL) {
		goto gts_err;
	}
	*sz = hdl->c.dt.pt;
	if (prm->icon) {
		if (sz->x) {
#if defined(SIZE_QVGA)
			sz->x += 1;
#else
			sz->x += 2;
#endif
		}
		sz->x += sz->y;
	}
	return(SUCCESS);

gts_err:
	return(FAILURE);
}

static void getleft(POINT_T *pt, const RECT_T *rect, const POINT_T *sz) {

	pt->x = rect->left;
	pt->y = rect->top;
	(void)sz;
}

static void getcenter(POINT_T *pt, const RECT_T *rect, const POINT_T *sz) {

	pt->x = rect->left;
	pt->x += (rect->right - rect->left - sz->x) >> 1;
	pt->y = rect->top;
}

static void getright(POINT_T *pt, const RECT_T *rect, const POINT_T *sz) {

	pt->x = rect->right - sz->x - MENU_DSTEXT;
	pt->y = rect->top;
}

static void getmid(POINT_T *pt, const RECT_T *rect, const POINT_T *sz) {

	pt->x = rect->left;
	pt->x += (rect->right - rect->left - sz->x) >> 1;
	pt->y = rect->top;
	pt->y += (rect->bottom - rect->top - sz->y) >> 1;
}


static BRESULT _cre_settext(MENUDLG dlg, DLGHDL hdl, const void *arg) {

const OEMCHAR	*str;

	str = (OEMCHAR *)arg;
	if (str == NULL) {
		str = str_null;
	}
	hdl->prm = resappend(dlg, str);
	hdl->c.dt.font = dlg->font;
	fontmng_getsize(dlg->font, str, &hdl->c.dt.pt);
	return(SUCCESS);
}

static void dlg_text(MENUDLG dlg, DLGHDL hdl,
									const POINT_T *pt, const RECT_T *rect) {

	VRAMHDL		icon;
const OEMCHAR	*string;
	int			color;
	POINT_T		fp;
	POINT_T		p;

	if (hdl->prm == NULL) {
		return;
	}
	fp = *pt;
	icon = hdl->prm->icon;
	if (icon) {
		if (icon->alpha) {
			vramcpy_cpyex(dlg->vram, &fp, icon, NULL);
		}
		else {
			vramcpy_cpy(dlg->vram, &fp, icon, NULL);
		}
		fp.x += icon->width;
#if defined(SIZE_QVGA)
		fp.x += 1;
#else
		fp.x += 2;
#endif
	}
	string = hdl->prm->str;
	if (string) {
		if (!(hdl->flag & MENU_GRAY)) {
			color = MVC_TEXT;
		}
		else {
			p.x = fp.x + MENU_DSTEXT;
			p.y = fp.y + MENU_DSTEXT;
			vrammix_text(dlg->vram, hdl->c.dt.font, string,
										menucolor[MVC_GRAYTEXT2], &p, rect);
			color = MVC_GRAYTEXT1;
		}
		vrammix_text(dlg->vram, hdl->c.dt.font, string,
										menucolor[color], &fp, rect);
	}
}


// ---- base

static BRESULT dlgbase_create(MENUDLG dlg, DLGHDL hdl, const void *arg) {

	RECT_T		rct;

	rct.right = hdl->rect.right - hdl->rect.left -
										((MENU_FBORDER + MENU_BORDER) * 2);
	hdl->vram = vram_create(rct.right, MENUDLG_CYCAPTION, FALSE, menubase.bpp);
	if (hdl->vram == NULL) {
		goto dbcre_err;
	}
	hdl->vram->posx = (MENU_FBORDER + MENU_BORDER);
	hdl->vram->posy = (MENU_FBORDER + MENU_BORDER);
	rct.left = 0;
	rct.top = 0;
	rct.bottom = MENUDLG_CYCAPTION;
	menuvram_caption(hdl->vram, &rct, MICON_NULL, (OEMCHAR *)arg);
	return(SUCCESS);

dbcre_err:
	(void)dlg;
	return(FAILURE);
}


static void dlgbase_paint(MENUDLG dlg, DLGHDL hdl) {

	OEMCHAR	*title;

	title = NULL;
	if (hdl->prm) {
		title = hdl->prm->str;
	}
	menuvram_base(dlg->vram);
	vrammix_cpy(dlg->vram, NULL, hdl->vram, NULL);
	menubase_setrect(dlg->vram, NULL);
}


static void dlgbase_onclick(MENUDLG dlg, DLGHDL hdl, int x, int y) {

	RECT_T	rct;

	vram_getrect(hdl->vram, &rct);
	dlg->dragflg = rect_in(&rct, x, y);
	dlg->lastx = x;
	dlg->lasty = y;
}


static void dlgbase_move(MENUDLG dlg, DLGHDL hdl, int x, int y, int focus) {

	if (dlg->dragflg) {
		x -= dlg->lastx;
		y -= dlg->lasty;
		if ((x) || (y)) {
			menubase_clrrect(dlg->vram);
			dlg->vram->posx += x;
			dlg->vram->posy += y;
			menubase_setrect(dlg->vram, NULL);
		}
	}
	(void)hdl;
	(void)focus;
}


// ---- close

static void dlgclose_paint(MENUDLG dlg, DLGHDL hdl) {

	menuvram_closebtn(dlg->vram, &hdl->rect, hdl->val);
}


static void dlgclose_onclick(MENUDLG dlg, DLGHDL hdl, int x, int y) {

	hdl->val = 1;
	drawctrls(dlg, hdl);
	(void)x;
	(void)y;
}


static void dlgclose_move(MENUDLG dlg, DLGHDL hdl, int x, int y, int focus) {

	if (hdl->val != focus) {
		hdl->val = focus;
		drawctrls(dlg, hdl);
	}
	(void)x;
	(void)y;
}


static void dlgclose_rel(MENUDLG dlg, DLGHDL hdl, int focus) {

	if (focus) {
		dlg->proc(DLGMSG_CLOSE, 0, 0);
	}
	(void)hdl;
}


// ---- button

static void dlgbtn_paint(MENUDLG dlg, DLGHDL hdl) {

	POINT_T	sz;
	POINT_T	pt;
	UINT	c;

	vram_filldat(dlg->vram, &hdl->rect, menucolor[MVC_BTNFACE]);
	if (!hdl->val) {
		c = MVC4(MVC_HILIGHT, MVC_DARK, MVC_LIGHT, MVC_SHADOW);
	}
	else {
		c = MVC4(MVC_DARK, MVC_DARK, MVC_SHADOW, MVC_SHADOW);
	}
	menuvram_box2(dlg->vram, &hdl->rect, c);

	if (gettextsz(hdl, &sz) == SUCCESS) {
		getmid(&pt, &hdl->rect, &sz);
		if (hdl->val) {
			pt.x += MENU_DSTEXT;
			pt.y += MENU_DSTEXT;
		}
		dlg_text(dlg, hdl, &pt, &hdl->rect);
	}
}


static void dlgbtn_onclick(MENUDLG dlg, DLGHDL hdl, int x, int y) {

	hdl->val = 1;
	drawctrls(dlg, hdl);
	(void)x;
	(void)y;
}

static void dlgbtn_move(MENUDLG dlg, DLGHDL hdl, int x, int y, int focus) {

	if (hdl->val != focus) {
		hdl->val = focus;
		drawctrls(dlg, hdl);
	}
	(void)x;
	(void)y;
}

static void dlgbtn_rel(MENUDLG dlg, DLGHDL hdl, int focus) {

	if (hdl->val != 0) {
		hdl->val = 0;
		drawctrls(dlg, hdl);
	}
	if (focus) {
		dlg->proc(DLGMSG_COMMAND, hdl->id, 0);
	}
}


// ---- list

static void *dlglist_setfont(DLGHDL hdl, void *font) {
										// 後でスクロールバーの調整をすべし
	void	*ret;
	POINT_T	pt;

	ret = hdl->c.dl.font;
	hdl->c.dl.font = font;
	fontmng_getsize(font, mstr_fontcheck, &pt);
	if ((pt.y <= 0) || (pt.y >= 65536)) {
		pt.y = 16;
	}
	hdl->c.dl.fontsize = (SINT16)pt.y;
	hdl->c.dl.dispmax = (SINT16)(hdl->vram->height / pt.y);
	return(ret);
}

static void dlglist_reset(MENUDLG dlg, DLGHDL hdl) {

	DLGPRM	dp;
	DLGPRM	next;

	vram_filldat(hdl->vram, NULL, 0xffffff);
	dp = hdl->prm;
	while(dp) {
		next = dp->next;
		dp->next = PRMNEXT_EMPTY;
		dp = next;
	}
	hdl->prm = NULL;
	hdl->prmcnt = 0;
	hdl->val = -1;
	hdl->c.dl.scrollbar = 0;
	hdl->c.dl.basepos = 0;
}

static BRESULT dlglist_create(MENUDLG dlg, DLGHDL hdl, const void *arg) {

	int		width;
	int		height;

	width = hdl->rect.right - hdl->rect.left - (MENU_LINE * 4);
	height = hdl->rect.bottom - hdl->rect.top - (MENU_LINE * 4);
	hdl->vram = vram_create(width, height, FALSE, menubase.bpp);
	if (hdl->vram == NULL) {
		goto dlcre_err;
	}
	hdl->vram->posx = hdl->rect.left + (MENU_LINE * 2);
	hdl->vram->posy = hdl->rect.top + (MENU_LINE * 2);
	dlglist_setfont(hdl, dlg->font);
	dlglist_reset(dlg, hdl);
	return(SUCCESS);

dlcre_err:
	(void)dlg;
	(void)arg;
	return(FAILURE);
}

static void dlglist_paint(MENUDLG dlg, DLGHDL hdl) {

	menuvram_box2(dlg->vram, &hdl->rect,
						MVC4(MVC_SHADOW, MVC_HILIGHT, MVC_DARK, MVC_LIGHT));
	vrammix_cpy(dlg->vram, NULL, hdl->vram, NULL);
}

static void dlglist_drawitem(DLGHDL hdl, DLGPRM prm, int focus,
												POINT_T *pt, RECT_T *rct) {

	VRAMHDL	icon;
	POINT_T	fp;

	vram_filldat(hdl->vram, rct, menucolor[focus?MVC_CURBACK:MVC_HILIGHT]);
	fp.x = pt->x;
	fp.y = pt->y;
	icon = prm->icon;
	if (icon) {
		if (icon->alpha) {
			vramcpy_cpyex(hdl->vram, &fp, icon, NULL);
		}
		else {
			vramcpy_cpy(hdl->vram, &fp, icon, NULL);
		}
		fp.x += icon->width;
#if defined(SIZE_QVGA)
		fp.x += 1;
#else
		fp.x += 2;
#endif
	}
	vrammix_text(hdl->vram, hdl->c.dl.font, prm->str,
							menucolor[focus?MVC_CURTEXT:MVC_TEXT], &fp, rct);
}

static BOOL dlglist_drawsub(DLGHDL hdl, int pos, int focus) {

	DLGPRM	prm;
	POINT_T	pt;
	RECT_T	rct;

	prm = ressea(hdl, pos);
	if (prm == NULL) {
		return(FALSE);
	}
	pos -= hdl->c.dl.basepos;
	if (pos < 0) {
		return(FALSE);
	}
	pt.x = 0;
	pt.y = pos * hdl->c.dl.fontsize;
	if (pt.y >= hdl->vram->height) {
		return(FALSE);
	}
	rct.left = 0;
	rct.top = pt.y;
	rct.right = hdl->vram->width;
	if (hdl->prmcnt > hdl->c.dl.dispmax) {
		rct.right -= MENUDLG_CXVSCR;
	}
	rct.bottom = rct.top + hdl->c.dl.fontsize;
	dlglist_drawitem(hdl, prm, focus, &pt, &rct);
	return(TRUE);
}

static void dlglist_setbtn(DLGHDL hdl, int flg) {

	RECT_T		rct;
	POINT_T		pt;
	UINT		mvc4;
const MENURES2	*res;

	res = menures_scrbtn;
	rct.right = hdl->vram->width;
	rct.left = rct.right - MENUDLG_CXVSCR;
	if (!(flg & 2)) {
		rct.top = 0;
	}
	else {
		rct.top = hdl->vram->height - MENUDLG_CYVSCR;
		if (rct.top < MENUDLG_CYVSCR) {
			rct.top = MENUDLG_CYVSCR;
		}
		res++;
	}
	rct.bottom = rct.top + MENUDLG_CYVSCR;

	vram_filldat(hdl->vram, &rct, menucolor[MVC_BTNFACE]);
	if (flg & 1) {
		mvc4 = MVC4(MVC_SHADOW, MVC_SHADOW, MVC_LIGHT, MVC_LIGHT);
	}
	else {
		mvc4 = MVC4(MVC_LIGHT, MVC_DARK, MVC_HILIGHT, MVC_SHADOW);
	}
	menuvram_box2(hdl->vram, &rct, mvc4);
	pt.x = rct.left + (MENU_LINE * 2);
	pt.y = rct.top + (MENU_LINE * 2);
	if (flg & 1) {
		pt.x += MENU_DSTEXT;
		pt.y += MENU_DSTEXT;
	}
	menuvram_res3put(hdl->vram, res, &pt, MVC_TEXT);
}

static void dlglist_drawall(DLGHDL hdl) {

	DLGPRM	prm;
	POINT_T	pt;
	RECT_T	rct;
	int		pos;

	rct.left = 0;
	rct.top = 0 - (hdl->c.dl.basepos * hdl->c.dl.fontsize);
	rct.right = hdl->vram->width;
	if (hdl->prmcnt > hdl->c.dl.dispmax) {
		rct.right -= MENUDLG_CXVSCR;
	}

	prm = hdl->prm;
	pos = 0;
	while(prm) {
		if (rct.top >= hdl->vram->height) {
			break;
		}
		if (rct.top >= 0) {
			rct.bottom = rct.top + hdl->c.dl.fontsize;
			pt.x = 0;
			pt.y = rct.top;
			dlglist_drawitem(hdl, prm, (pos == hdl->val), &pt, &rct);
		}
		prm = prm->next;
		pos++;
		rct.top += hdl->c.dl.fontsize;
	}
	rct.bottom = hdl->vram->height;
	vram_filldat(hdl->vram, &rct, menucolor[MVC_HILIGHT]);
}

static int dlglist_barpos(DLGHDL hdl) {

	int		ret;

	ret = hdl->vram->height - (MENUDLG_CYVSCR * 2);
	ret -= hdl->c.dl.scrollbar;
	ret *= hdl->c.dl.basepos;
	ret /= (hdl->prmcnt - hdl->c.dl.dispmax);
	return(ret);
}

static void dlglist_drawbar(DLGHDL hdl) {

	RECT_T	rct;

	rct.right = hdl->vram->width;
	rct.left = rct.right - MENUDLG_CXVSCR;
	rct.top = MENUDLG_CYVSCR;
	rct.bottom = hdl->vram->height - MENUDLG_CYVSCR;
	vram_filldat(hdl->vram, &rct, menucolor[MVC_SCROLLBAR]);

	rct.top += dlglist_barpos(hdl);
	rct.bottom = rct.top + hdl->c.dl.scrollbar;
	vram_filldat(hdl->vram, &rct, menucolor[MVC_BTNFACE]);
	menuvram_box2(hdl->vram, &rct,
						MVC4(MVC_LIGHT, MVC_DARK, MVC_HILIGHT, MVC_SHADOW));
}

static BOOL dlglist_append(MENUDLG dlg, DLGHDL hdl, const OEMCHAR* arg) {

	BOOL	r;
	DLGPRM	*sto;
	int		barsize;

	r = FALSE;
	sto = &hdl->prm;
	while(*sto) {
		sto = &((*sto)->next);
	}
	*sto = resappend(dlg, arg);
	if (*sto) {
		r = dlglist_drawsub(hdl, hdl->prmcnt, FALSE);
		hdl->prmcnt++;
		if (hdl->prmcnt > hdl->c.dl.dispmax) {
			barsize = hdl->vram->height - (MENUDLG_CYVSCR * 2);
			if (barsize >= 8) {
				barsize *= hdl->c.dl.dispmax;
				barsize /= hdl->prmcnt;
				barsize = max(barsize, 6);
				if (!hdl->c.dl.scrollbar) {
					dlglist_drawall(hdl);
					dlglist_setbtn(hdl, 0);
					dlglist_setbtn(hdl, 2);
				}
				hdl->c.dl.scrollbar = barsize;
				dlglist_drawbar(hdl);
			}
		}
	}
	return(r);
}

static BOOL dlglist_setex(MENUDLG dlg, DLGHDL hdl, const ITEMEXPRM *arg) {

	DLGPRM	dp;
	UINT	cnt;

	if ((arg == NULL) || (arg->pos >= hdl->prmcnt)) {
		return(FALSE);
	}
	cnt = arg->pos;
	dp = hdl->prm;
	while((cnt) && (dp)) {
		cnt--;
		dp = dp->next;
	}
	if (dp == NULL) {
		return(FALSE);
	}
	resattachicon(dlg, dp, arg->icon, hdl->c.dl.fontsize,
											hdl->c.dl.fontsize);
	milstr_ncpy(dp->str, arg->str, NELEMENTS(dp->str));
	return(dlglist_drawsub(hdl, arg->pos, (arg->pos == hdl->val)));
}

static int dlglist_getpos(DLGHDL hdl, int y) {

	int		val;

	val = (y / hdl->c.dl.fontsize) + hdl->c.dl.basepos;
	if ((unsigned int)val < (unsigned int)hdl->prmcnt) {
		return(val);
	}
	else {
		return(-1);
	}
}

enum {
	DLCUR_OUT		= -1,
	DLCUR_INLIST	= 0,
	DLCUR_UP		= 1,
	DLCUR_INBAR		= 2,
	DLCUR_DOWN		= 3,
	DLCUR_PGUP		= 4,
	DLCUR_PGDN		= 5,
	DLCUR_INCUR		= 6
};

static int dlglist_getpc(DLGHDL hdl, int x, int y) {

	if ((unsigned int)x >= (unsigned int)hdl->vram->width) {
		goto dlgp_out;
	}
	if ((unsigned int)y >= (unsigned int)hdl->vram->height) {
		goto dlgp_out;
	}

	if ((hdl->prmcnt < hdl->c.dl.dispmax) ||
		(x < (hdl->vram->width - MENUDLG_CXVSCR))) {
		return(DLCUR_INLIST);
	}
	else if (y < MENUDLG_CYVSCR) {
		return(DLCUR_UP);
	}
	else if (y >= (hdl->vram->height - MENUDLG_CYVSCR)) {
		return(DLCUR_DOWN);
	}
	y -= MENUDLG_CYVSCR;
	y -= dlglist_barpos(hdl);
	if (y < 0) {
		return(DLCUR_PGUP);
	}
	else if (y < (int)hdl->c.dl.scrollbar) {
		return(DLCUR_INBAR);
	}
	else {
		return(DLCUR_PGDN);
	}

dlgp_out:
	return(DLCUR_OUT);
}

static void dlglist_setval(MENUDLG dlg, DLGHDL hdl, int val) {

	BOOL	r;

	if ((unsigned int)val >= (unsigned int)hdl->prmcnt) {
		val = -1;
	}
	if (val != hdl->val) {
		r = dlglist_drawsub(hdl, hdl->val, FALSE);
		r |= dlglist_drawsub(hdl, val, TRUE);
		hdl->val = val;
		if (r) {
			drawctrls(dlg, hdl);
		}
	}
}

static void dlglist_setbasepos(MENUDLG dlg, DLGHDL hdl, int pos) {

	int		displimit;

	if (pos < 0) {
		pos = 0;
	}
	else {
		displimit = hdl->prmcnt - hdl->c.dl.dispmax;
		if (displimit < 0) {
			displimit = 0;
		}
		if (pos > displimit) {
			pos = displimit;
		}
	}
	if (hdl->c.dl.basepos != pos) {
		hdl->c.dl.basepos = pos;
		dlglist_drawall(hdl);
		dlglist_drawbar(hdl);
	}
	(void)dlg;
}

static void dlglist_onclick(MENUDLG dlg, DLGHDL hdl, int x, int y) {

	int		flg;
	int		val;

	x -= (MENU_LINE * 2);
	y -= (MENU_LINE * 2);
	flg = dlglist_getpc(hdl, x, y);
	dlg->dragflg = flg;
	switch(flg) {
		case DLCUR_INLIST:
			val = dlglist_getpos(hdl, y);
			if ((val == hdl->val) && (val != -1)) {
				dlg->dragflg = DLCUR_INCUR;
			}
			dlglist_setval(dlg, hdl, val);
			dlg->proc(DLGMSG_COMMAND, hdl->id, 0);
			break;

		case 1:
		case 3:
			dlglist_setbtn(hdl, flg);
			dlglist_setbasepos(dlg, hdl, hdl->c.dl.basepos + flg - 2);
			drawctrls(dlg, hdl);
			break;

		case DLCUR_INBAR:
			y -= MENUDLG_CYVSCR;
			y -= dlglist_barpos(hdl);
			if ((unsigned int)y < (unsigned int)hdl->c.dl.scrollbar) {
				dlg->lasty = y;
			}
			else {
				dlg->lasty = -1;
			}
			break;

		case DLCUR_PGUP:
			dlglist_setbasepos(dlg, hdl, hdl->c.dl.basepos
														- hdl->c.dl.dispmax);
			drawctrls(dlg, hdl);
			break;

		case DLCUR_PGDN:
			dlglist_setbasepos(dlg, hdl, hdl->c.dl.basepos
														+ hdl->c.dl.dispmax);
			drawctrls(dlg, hdl);
			break;
	}
}

static void dlglist_move(MENUDLG dlg, DLGHDL hdl, int x, int y, int focus) {

	int		flg;
	int		val;
	int		height;

	x -= (MENU_LINE * 2);
	y -= (MENU_LINE * 2);
	flg = dlglist_getpc(hdl, x, y);
	switch(dlg->dragflg) {
		case DLCUR_INLIST:
		case DLCUR_INCUR:
			if (flg == DLCUR_INLIST) {
				val = dlglist_getpos(hdl, y);
				if (val != hdl->val) {
					dlg->dragflg = DLCUR_INLIST;
					dlglist_setval(dlg, hdl, val);
					dlg->proc(DLGMSG_COMMAND, hdl->id, 0);
				}
			}
			break;

		case 1:
		case 3:
			dlglist_setbtn(hdl, dlg->dragflg - ((dlg->dragflg == flg)?0:1));
			drawctrls(dlg, hdl);
			break;

		case DLCUR_INBAR:
			if (dlg->lasty >= 0) {
				y -= MENUDLG_CYVSCR;
				y -= dlg->lasty;
				height = hdl->vram->height - (MENUDLG_CYVSCR * 2);
				height -= hdl->c.dl.scrollbar;
				if (y < 0) {
					y = 0;
				}
				else if (y > height) {
					y = height;
				}
				y *= (hdl->prmcnt - hdl->c.dl.dispmax);
				y /= height;
				dlglist_setbasepos(dlg, hdl, y);
				drawctrls(dlg, hdl);
			}
			break;
	}
	(void)focus;
}

static void dlglist_rel(MENUDLG dlg, DLGHDL hdl, int focus) {

	switch(dlg->dragflg) {
		case 1:
		case 3:
			dlglist_setbtn(hdl, dlg->dragflg - 1);
			drawctrls(dlg, hdl);
			break;

		case DLCUR_INCUR:
			dlg->proc(DLGMSG_COMMAND, hdl->id, 1);
			break;
	}
	(void)focus;
}


// ---- slider

static void dlgslider_setflag(DLGHDL hdl) {

	int		size;
	UINT	type;

	if (!(hdl->flag & MSS_VERT)) {
		size = hdl->rect.bottom - hdl->rect.top;
	}
	else {
		size = hdl->rect.right - hdl->rect.left;
	}
	if (size < 13) {
		type = 0 + (9 << 8) + (5 << 16);
	}
	else if (size < 21) {
		type = 1 + (13 << 8) + (7 << 16);
	}
	else {
		type = 2 + (21 << 8) + (11 << 16);
	}
	hdl->c.ds.type = (UINT8)type;
	if (!(hdl->flag & MSS_VERT)) {
		hdl->c.ds.sldh = (UINT8)(type >> 16);
		hdl->c.ds.sldv = (UINT8)(type >> 8);
	}
	else {
		hdl->c.ds.sldh = (UINT8)(type >> 8);
		hdl->c.ds.sldv = (UINT8)(type >> 16);
	}
}

static int dlgslider_setpos(DLGHDL hdl, int val) {

	int		range;
	int		width;
	int		dir;

	range = hdl->c.ds.maxval - hdl->c.ds.minval;
	if (range) {
		dir = (range > 0)?1:-1;
		val -= hdl->c.ds.minval;
		val *= dir;
		range *= dir;
		if (val < 0) {
			val = 0;
		}
		else if (val >= range) {
			val = range;
		}
		hdl->val = hdl->c.ds.minval + (val * dir);
		if (!(hdl->flag & MSS_VERT)) {
			width = hdl->rect.right - hdl->rect.left;
			width -= hdl->c.ds.sldh;
		}
		else {
			width = hdl->rect.bottom - hdl->rect.top;
			width -= hdl->c.ds.sldv;
		}
		if ((width > 0) || (range)) {
			val *= width;
			val /= range;
		}
		else {
			val = 0;
		}
	}
	else {
		val = 0;
	}
	return(val);
}

static BRESULT dlgslider_create(MENUDLG dlg, DLGHDL hdl, const void *arg) {

	hdl->c.ds.minval = (SINT16)(long)arg;
	hdl->c.ds.maxval = (SINT16)((long)arg >> 16);
	hdl->c.ds.moving = 0;
	dlgslider_setflag(hdl);
	hdl->c.ds.pos = dlgslider_setpos(hdl, 0);
	(void)dlg;
	return(SUCCESS);
}

static void dlgslider_paint(MENUDLG dlg, DLGHDL hdl) {

	UINT		flag;
	int			ptr;
	RECT_U		rct;
	POINT_T		pt;
	MENURES2	src;

	flag = hdl->flag;
	switch(flag & MSS_POSMASK) {
		case MSS_BOTH:
			ptr = 1;
			break;
		case MSS_TOP:
			ptr = 2;
			break;
		default:
			ptr = 0;
			break;
	}
	vram_filldat(dlg->vram, &hdl->rect, menucolor[MVC_STATIC]);
	if (!(hdl->flag & MSS_VERT)) {
		rct.r.left = hdl->rect.left;
		rct.r.right = hdl->rect.right;
		rct.r.top = hdl->rect.top + ptr +
									(hdl->c.ds.sldv / 2) - (MENU_LINE * 2);
		rct.r.bottom = rct.r.top + (MENU_LINE * 4);
		menuvram_box2(dlg->vram, &rct.r,
						MVC4(MVC_SHADOW, MVC_HILIGHT, MVC_DARK, MVC_LIGHT));
		pt.x = hdl->rect.left + hdl->c.ds.pos;
		pt.y = hdl->rect.top;
	}
	else {
		rct.r.left = hdl->rect.left + ptr +
									(hdl->c.ds.sldh / 2) - (MENU_LINE * 2);
		rct.r.right = rct.r.left + (MENU_LINE * 4);
		rct.r.top = hdl->rect.top;
		rct.r.bottom = hdl->rect.bottom;
		menuvram_box2(dlg->vram, &rct.r,
						MVC4(MVC_SHADOW, MVC_HILIGHT, MVC_DARK, MVC_LIGHT));
		pt.x = hdl->rect.left;
		pt.y = hdl->rect.top + hdl->c.ds.pos;
		ptr += 3;
	}
	ptr *= 2;
	if ((hdl->flag & MENU_GRAY) || (hdl->c.ds.moving)) {
		ptr++;
	}
	src.width = hdl->c.ds.sldh;
	src.height = hdl->c.ds.sldv;
	src.pat = menures_slddat + menures_sldpos[hdl->c.ds.type][ptr];
	menuvram_res2put(dlg->vram, &src, &pt);
}

static void dlgslider_setval(MENUDLG dlg, DLGHDL hdl, int val) {

	int		pos;

	pos = dlgslider_setpos(hdl, val);
	if (hdl->c.ds.pos != pos) {
		hdl->c.ds.pos = pos;
		drawctrls(dlg, hdl);
	}
}

static void dlgslider_onclick(MENUDLG dlg, DLGHDL hdl, int x, int y) {

	int		width;
	int		range;
	int		dir;

	if (!(hdl->flag & MSS_VERT)) {
		width = hdl->c.ds.sldh;
	}
	else {
		width = hdl->c.ds.sldv;
		x = y;
	}
	x -= hdl->c.ds.pos;
	if ((x >= -1) && (x <= width)) {
		dlg->dragflg = x;
		hdl->c.ds.moving = 1;
		drawctrls(dlg, hdl);
	}
	else {
		dlg->dragflg = -1;
		dir = (x > 0)?1:0;
		range = hdl->c.ds.maxval - hdl->c.ds.minval;
		if (range < 0) {
			range = 0 - range;
			dir ^= 1;
		}
		if (range < 16) {
			range = 16;
		}
		range >>= 4;
		if (!dir) {
			range = 0 - range;
		}
		dlgslider_setval(dlg, hdl, hdl->val + range);
		dlg->proc(DLGMSG_COMMAND, hdl->id, 0);
	}
}

static void dlgslider_move(MENUDLG dlg, DLGHDL hdl, int x, int y, int focus) {

	int		range;
	int		width;
	int		dir;

	if (hdl->c.ds.moving) {
		range = hdl->c.ds.maxval - hdl->c.ds.minval;
		if (range) {
			dir = (range > 0)?1:-1;
			range *= dir;
			if (!(hdl->flag & MSS_VERT)) {
				width = hdl->rect.right - hdl->rect.left;
				width -= hdl->c.ds.sldh;
			}
			else {
				width = hdl->rect.bottom - hdl->rect.top;
				width -= hdl->c.ds.sldv;
				x = y;
			}
			x -= dlg->dragflg;
			if ((x < 0) || (width <= 0)) {
				x = 0;
			}
			else if (x >= width) {
				x = range;
			}
			else {
				x *= range;
				x += (width >> 1);
				x /= width;
			}
			x = hdl->c.ds.minval + (x * dir);
			dlgslider_setval(dlg, hdl, x);
			dlg->proc(DLGMSG_COMMAND, hdl->id, 0);
		}
	}
	(void)focus;
}


static void dlgslider_rel(MENUDLG dlg, DLGHDL hdl, int focus) {

	if (hdl->c.ds.moving) {
		hdl->c.ds.moving = 0;
		drawctrls(dlg, hdl);
	}
	(void)focus;
}


// ---- tablist

static void *dlgtablist_setfont(DLGHDL hdl, void *font) {

	void	*ret;
	POINT_T	pt;
	DLGPRM	prm;

	ret = hdl->c.dtl.font;
	hdl->c.dtl.font = font;
	fontmng_getsize(font, mstr_fontcheck, &pt);
	if ((pt.y <= 0) || (pt.y >= 65536)) {
		pt.y = 16;
	}
	hdl->c.dtl.fontsize = pt.y;
	prm = hdl->prm;
	while(prm) {
		fontmng_getsize(hdl->c.dtl.font, prm->str, &pt);
		prm->width = pt.x;
		prm = prm->next;
	}
	return(ret);
}

static BRESULT dlgtablist_create(MENUDLG dlg, DLGHDL hdl, const void *arg) {

	RECT_T	rct;

	rct.right = hdl->rect.right - hdl->rect.left;
	hdl->val = -1;
	dlgtablist_setfont(hdl, dlg->font);
	(void)arg;
	return(SUCCESS);
}

static void dlgtablist_paint(MENUDLG dlg, DLGHDL hdl) {

	VRAMHDL	dst;
	DLGPRM	prm;
	POINT_T	pt;
	RECT_T	rct;
	int		posx;
	int		lx;
	int		cnt;
	int		tabey;
	int		tabdy;

	dst = dlg->vram;
	rct = hdl->rect;
	vram_filldat(dst, &rct, menucolor[MVC_STATIC]);
	tabey = rct.top + hdl->c.dtl.fontsize +
							MENUDLG_SYTAB + MENUDLG_TYTAB + MENUDLG_EYTAB;
	rct.top = tabey;
	menuvram_box2(dst, &rct,
						MVC4(MVC_HILIGHT, MVC_DARK, MVC_LIGHT, MVC_SHADOW));

	posx = hdl->rect.left + (MENU_LINE * 2);
	prm = hdl->prm;
	cnt = hdl->val;
	while(prm) {
		if (cnt) {
			pt.x = posx;
			pt.y = hdl->rect.top + MENUDLG_SYTAB;
			menuvram_liney(dst, pt.x, pt.y + (MENU_LINE * 2),
														tabey, MVC_HILIGHT);
			pt.x += MENU_LINE;
			menuvram_liney(dst, pt.x, pt.y + MENU_LINE,
										pt.y + (MENU_LINE * 2), MVC_HILIGHT);
			menuvram_liney(dst, pt.x, pt.y + (MENU_LINE * 2),
														tabey, MVC_LIGHT);
			pt.x += MENU_LINE;
			lx = pt.x + prm->width + (MENUDLG_TXTAB * 2);
			menuvram_linex(dst, pt.x, pt.y, lx, MVC_HILIGHT);
			menuvram_linex(dst, pt.x, pt.y + MENU_LINE, lx, MVC_LIGHT);

			menuvram_liney(dst, lx, pt.y + MENU_LINE,
										pt.y + (MENU_LINE * 2), MVC_DARK);
			menuvram_liney(dst, lx, pt.y + (MENU_LINE * 2),
														tabey, MVC_SHADOW);
			lx++;
			menuvram_liney(dst, lx, pt.y + (MENU_LINE * 2),
														tabey, MVC_DARK);
			pt.x += MENUDLG_TXTAB;
			pt.y += MENUDLG_TYTAB;
			vrammix_text(dst, hdl->c.dtl.font, prm->str,
											menucolor[MVC_TEXT], &pt, NULL);
		}
		cnt--;
		posx += prm->width + (MENU_LINE * 4) + (MENUDLG_TXTAB) * 2;
		prm = prm->next;
	}

	posx = hdl->rect.left;
	prm = hdl->prm;
	cnt = hdl->val;
	while(prm) {
		if (!cnt) {
			pt.x = posx;
			pt.y = hdl->rect.top;
			if (posx == hdl->rect.left) {
				tabdy = tabey + 2;
			}
			else {
				tabdy = tabey + 1;
				menuvram_linex(dst, pt.x, tabdy,
										pt.x + (MENU_LINE * 2), MVC_STATIC);
			}
			menuvram_liney(dst, pt.x, pt.y + (MENU_LINE * 2),
														tabdy, MVC_HILIGHT);
			pt.x += MENU_LINE;
			menuvram_liney(dst, pt.x, pt.y + MENU_LINE,
										pt.y + (MENU_LINE * 2), MVC_HILIGHT);
			menuvram_liney(dst, pt.x, pt.y + (MENU_LINE * 2),
														tabdy, MVC_LIGHT);
			pt.x += MENU_LINE;
			lx = pt.x + prm->width + (MENU_LINE * 4) + (MENUDLG_TXTAB * 2);
			menuvram_linex(dst, pt.x, pt.y, lx, MVC_HILIGHT);
			menuvram_linex(dst, pt.x, pt.y + MENU_LINE, lx, MVC_LIGHT);
			menuvram_linex(dst, pt.x, tabey, lx, MVC_STATIC);
			menuvram_linex(dst, pt.x, tabey + MENU_LINE, lx, MVC_STATIC);
			tabdy = tabey + 1;
			menuvram_liney(dst, lx, pt.y + MENU_LINE,
										pt.y + (MENU_LINE * 2), MVC_DARK);
			menuvram_liney(dst, lx, pt.y + (MENU_LINE * 2),
														tabdy, MVC_SHADOW);
			lx++;
			menuvram_liney(dst, lx, pt.y + (MENU_LINE * 2),
														tabdy, MVC_DARK);
			pt.x += MENUDLG_TXTAB + (MENU_LINE * 2);
			pt.y += MENUDLG_TYTAB;
			vrammix_text(dst, hdl->c.dtl.font, prm->str,
											menucolor[MVC_TEXT], &pt, NULL);
			break;
		}
		cnt--;
		posx += prm->width + (MENU_LINE * 4) + (MENUDLG_TXTAB * 2);
		prm = prm->next;
	}
}

static void dlgtablist_setval(MENUDLG dlg, DLGHDL hdl, int val) {

	if (hdl->val != val) {
		hdl->val = val;
		drawctrls(dlg, hdl);
	}
}

static void dlgtablist_append(MENUDLG dlg, DLGHDL hdl, const OEMCHAR *arg) {

	DLGPRM	res;
	DLGPRM	*sto;
	POINT_T	pt;

	sto = &hdl->prm;
	while(*sto) {
		sto = &((*sto)->next);
	}
	res = resappend(dlg, arg);
	if (res) {
		*sto = res;
		fontmng_getsize(hdl->c.dtl.font, (OEMCHAR *)arg, &pt);
		res->width = pt.x;
		hdl->prmcnt++;
	}
}

static void dlgtablist_onclick(MENUDLG dlg, DLGHDL hdl, int x, int y) {

	DLGPRM	prm;
	int		pos;

	if (y < (hdl->c.dtl.fontsize +
							MENUDLG_SYTAB + MENUDLG_TYTAB + MENUDLG_EYTAB)) {
		pos = 0;
		prm = hdl->prm;
		while(prm) {
			x -= (MENU_LINE * 4);
			if (x < 0) {
				break;
			}
			x -= prm->width + (MENUDLG_TXTAB * 2);
			if (x < 0) {
				dlgtablist_setval(dlg, hdl, pos);
				dlg->proc(DLGMSG_COMMAND, hdl->id, 0);
				break;
			}
			pos++;
			prm = prm->next;
		}
	}
}


// ---- edit

static void dlgedit_paint(MENUDLG dlg, DLGHDL hdl) {

	RECT_T		rct;
	POINT_T		pt;
const OEMCHAR	*string;

	rct = hdl->rect;
	menuvram_box2(dlg->vram, &rct,
						MVC4(MVC_SHADOW, MVC_HILIGHT, MVC_DARK, MVC_LIGHT));
	rct.left += (MENU_LINE * 2);
	rct.top += (MENU_LINE * 2);
	rct.right -= (MENU_LINE * 2);
	rct.bottom -= (MENU_LINE * 2);
	vram_filldat(dlg->vram, &rct, menucolor[
							(hdl->flag & MENU_GRAY)?MVC_STATIC:MVC_HILIGHT]);
	if (hdl->prm == NULL) {
		goto dged_exit;
	}
	string = hdl->prm->str;
	if (string == NULL) {
		goto dged_exit;
	}
	pt.x = rct.left + MENU_LINE;
	pt.y = rct.top + MENU_LINE;
	vrammix_text(dlg->vram, hdl->c.dt.font, string,
											menucolor[MVC_TEXT], &pt, &rct);

dged_exit:
	return;
}


// ---- frame

static void dlgframe_paint(MENUDLG dlg, DLGHDL hdl) {

	RECT_T		rct;
	POINT_T		pt;

	rct.left = hdl->rect.left;
	rct.top = hdl->rect.top + MENUDLG_SYFRAME;
	rct.right = hdl->rect.right;
	rct.bottom = hdl->rect.bottom;
	menuvram_box2(dlg->vram, &rct,
					MVC4(MVC_SHADOW, MVC_HILIGHT, MVC_HILIGHT, MVC_SHADOW));
	rct.left += MENUDLG_SXFRAME;
	rct.top = hdl->rect.top;
	rct.right = rct.left + (MENUDLG_PXFRAME * 2) + hdl->c.dt.pt.x;
	rct.bottom = rct.top + hdl->c.dt.pt.y + MENU_DSTEXT;
	vram_filldat(dlg->vram, &rct, menucolor[MVC_STATIC]);
	if (hdl->prm) {
		pt.x = rct.left + MENUDLG_PXFRAME;
		pt.y = rct.top;
		dlg_text(dlg, hdl, &pt, &rct);
	}
}


// ---- radio

typedef struct {
	MENUDLG	dlg;
	MENUID	group;
} MDCB1;

static void dlgradio_paint(MENUDLG dlg, DLGHDL hdl) {

	POINT_T		pt;
const MENURES2	*src;
	int			pat;

	vram_filldat(dlg->vram, &hdl->rect, menucolor[MVC_STATIC]);
	pt.x = hdl->rect.left;
	pt.y = hdl->rect.top;
	src = menures_radio;
	pat = (hdl->flag & MENU_GRAY)?1:0;
	menuvram_res2put(dlg->vram, src + pat, &pt);
	if (hdl->val) {
		menuvram_res3put(dlg->vram, src + 2, &pt,
					(hdl->flag & MENU_GRAY)?MVC_GRAYTEXT1:MVC_TEXT);
	}
	pt.x += MENUDLG_SXRADIO;
	dlg_text(dlg, hdl, &pt, &hdl->rect);
}

static BOOL drsv_cb(void *vpItem, void *vpArg) {

	DLGHDL	item;

	item = (DLGHDL)vpItem;
	if ((item->type == DLGTYPE_RADIO) && (item->val) &&
		(item->group == ((MDCB1 *)vpArg)->group)) {
		item->val = 0;
		drawctrls(((MDCB1 *)vpArg)->dlg, item);
	}
	return(FALSE);
}

static void dlgradio_setval(MENUDLG dlg, DLGHDL hdl, int val) {

	MDCB1	mdcb;

	if (hdl->val != val) {
		if (val) {
			mdcb.dlg = dlg;
			mdcb.group = hdl->group;
			listarray_enum(dlg->dlg, drsv_cb, &mdcb);
		}
		hdl->val = val;
		drawctrls(dlg, hdl);
	}
}

static void dlgradio_onclick(MENUDLG dlg, DLGHDL hdl, int x, int y) {

	if (x < (hdl->c.dt.pt.x + MENUDLG_SXRADIO)) {
		dlgradio_setval(dlg, hdl, 1);
		dlg->proc(DLGMSG_COMMAND, hdl->id, 0);
	}
	(void)y;
}


// ---- check

static void dlgcheck_paint(MENUDLG dlg, DLGHDL hdl) {

	POINT_T	pt;
	RECT_T	rct;
	UINT32	basecol;
	UINT32	txtcol;

	vram_filldat(dlg->vram, &hdl->rect, menucolor[MVC_STATIC]);
	rct.left = hdl->rect.left;
	rct.top = hdl->rect.top;
	rct.right = rct.left + MENUDLG_CXCHECK;
	rct.bottom = rct.top + MENUDLG_CYCHECK;
	if (!(hdl->flag & MENU_GRAY)) {
		basecol = MVC_HILIGHT;
		txtcol = MVC_TEXT;
	}
	else {
		basecol = MVC_STATIC;
		txtcol = MVC_GRAYTEXT1;
	}
	vram_filldat(dlg->vram, &rct, menucolor[basecol]);
	menuvram_box2(dlg->vram, &rct,
						MVC4(MVC_SHADOW, MVC_HILIGHT, MVC_DARK, MVC_LIGHT));
	if (hdl->val) {
		pt.x = rct.left + (MENU_LINE * 2);
		pt.y = rct.top + (MENU_LINE * 2);
		menuvram_res3put(dlg->vram, &menures_check, &pt, txtcol);
	}
	pt.x = rct.left + MENUDLG_SXCHECK;
	pt.y = rct.top;
	dlg_text(dlg, hdl, &pt, &hdl->rect);
}

static void dlgcheck_setval(MENUDLG dlg, DLGHDL hdl, int val) {

	if (hdl->val != val) {
		hdl->val = val;
		drawctrls(dlg, hdl);
	}
}

static void dlgcheck_onclick(MENUDLG dlg, DLGHDL hdl, int x, int y) {

	if (x < (hdl->c.dt.pt.x + MENUDLG_SXCHECK)) {
		dlgcheck_setval(dlg, hdl, !hdl->val);
		dlg->proc(DLGMSG_COMMAND, hdl->id, 0);
	}
	(void)y;
}


// ---- text

static void dlgtext_paint(MENUDLG dlg, DLGHDL hdl) {

	POINT_T	sz;
	POINT_T	pt;
	void	(*getpt)(POINT_T *pt, const RECT_T *rect, const POINT_T *sz);

	vram_filldat(dlg->vram, &hdl->rect, menucolor[MVC_STATIC]);
	if (gettextsz(hdl, &sz) == SUCCESS) {
		switch(hdl->flag & MST_POSMASK) {
			case MST_LEFT:
			default:
				getpt = getleft;
				break;

			case MST_CENTER:
				getpt = getcenter;
				break;

			case MST_RIGHT:
				getpt = getright;
				break;
		}
		getpt(&pt, &hdl->rect, &sz);
		dlg_text(dlg, hdl, &pt, &hdl->rect);
	}
}

static void dlgtext_itemset(MENUDLG dlg, DLGHDL hdl, const OEMCHAR *str) {

	if (hdl->prm) {
		if (str == NULL) {
			str = str_null;
		}
		milstr_ncpy(hdl->prm->str, str, NELEMENTS(hdl->prm->str));
		fontmng_getsize(hdl->c.dt.font, str, &hdl->c.dt.pt);
	}
	(void)dlg;
}

static void dlgtext_iconset(MENUDLG dlg, DLGHDL hdl, UINT arg) {

	if (hdl->prm) {
		resattachicon(dlg, hdl->prm, (UINT16)arg, hdl->c.dt.pt.y, hdl->c.dt.pt.y);
	}
	(void)dlg;
}


// ---- icon/vram

static void iconpaint(MENUDLG dlg, DLGHDL hdl, VRAMHDL src) {

	RECT_U		r;
	UINT32		bgcol;

	r.p.x = hdl->rect.left;
	r.p.y = hdl->rect.top;
	bgcol = menucolor[MVC_STATIC];
	if (src) {
		if (src->alpha) {
			r.r.right = r.r.left + src->width;
			r.r.bottom = r.r.top + src->height;
			vram_filldat(dlg->vram, &r.r, bgcol);
			vramcpy_cpyex(dlg->vram, &r.p, src, NULL);
		}
		else {
			vramcpy_cpy(dlg->vram, &r.p, src, NULL);
		}
	}
	else {
		vram_filldat(dlg->vram, &hdl->rect, bgcol);
	}
}

static BRESULT dlgicon_create(MENUDLG dlg, DLGHDL hdl, const void *arg) {

	hdl->prm = resappend(dlg, NULL);
	resattachicon(dlg, hdl->prm, (UINT16)(long)arg,
		hdl->rect.right - hdl->rect.left, hdl->rect.bottom - hdl->rect.top);
	return(SUCCESS);
}

static void dlgicon_paint(MENUDLG dlg, DLGHDL hdl) {

	DLGPRM	prm;

	prm = hdl->prm;
	if (prm) {
		iconpaint(dlg, hdl, prm->icon);
	}
}

static BRESULT dlgvram_create(MENUDLG dlg, DLGHDL hdl, const void *arg) {

	hdl->c.dv.vram = (VRAMHDL)arg;
	(void)dlg;
	return(SUCCESS);
}

static void dlgvram_paint(MENUDLG dlg, DLGHDL hdl) {

	iconpaint(dlg, hdl, hdl->c.dv.vram);
}


// ---- line

static void dlgline_paint(MENUDLG dlg, DLGHDL hdl) {

	if (!(hdl->flag & MSL_VERT)) {
		menuvram_linex(dlg->vram, hdl->rect.left, hdl->rect.top,
											hdl->rect.right, MVC_SHADOW);
		menuvram_linex(dlg->vram, hdl->rect.left, hdl->rect.top + MENU_LINE,
											hdl->rect.right, MVC_HILIGHT);
	}
	else {
		menuvram_liney(dlg->vram, hdl->rect.left, hdl->rect.top,
											hdl->rect.bottom, MVC_SHADOW);
		menuvram_liney(dlg->vram, hdl->rect.left+MENU_LINE, hdl->rect.top,
											hdl->rect.bottom, MVC_HILIGHT);
	}
}


// ---- box

static void dlgbox_paint(MENUDLG dlg, DLGHDL hdl) {

	menuvram_box2(dlg->vram, &hdl->rect,
					MVC4(MVC_SHADOW, MVC_HILIGHT, MVC_HILIGHT, MVC_SHADOW));
}


// ---- procs

static BRESULT _cre(MENUDLG dlg, DLGHDL hdl, const void *arg) {

	(void)dlg;
	(void)hdl;
	(void)arg;
	return(SUCCESS);
}

#if 0		// not used
static void _paint(MENUDLG dlg, DLGHDL hdl) {

	(void)dlg;
	(void)hdl;
}
#endif

#if 0		// not used
static void _onclick(MENUDLG dlg, DLGHDL hdl, int x, int y) {

	(void)dlg;
	(void)hdl;
	(void)x;
	(void)y;
}
#endif

static void _setval(MENUDLG dlg, DLGHDL hdl, int val) {

	(void)dlg;
	(void)hdl;
	(void)val;
}

static void _moverel(MENUDLG dlg, DLGHDL hdl, int focus) {

	(void)dlg;
	(void)hdl;
	(void)focus;
}

typedef BRESULT (*DLGCRE)(MENUDLG dlg, DLGHDL hdl, const void *arg);
typedef void (*DLGPAINT)(MENUDLG dlg, DLGHDL hdl);
typedef void (*DLGSETVAL)(MENUDLG dlg, DLGHDL hdl, int val);
typedef void (*DLGCLICK)(MENUDLG dlg, DLGHDL hdl, int x, int y);
typedef void (*DLGMOV)(MENUDLG dlg, DLGHDL hdl, int x, int y, int focus);
typedef void (*DLGREL)(MENUDLG dlg, DLGHDL hdl, int focus);

static const DLGCRE dlgcre[] = {
		dlgbase_create,				// DLGTYPE_BASE
		_cre,						// DLGTYPE_CLOSE
		_cre_settext,				// DLGTYPE_BUTTON
		dlglist_create,				// DLGTYPE_LIST
		dlgslider_create,			// DLGTYPE_SLIDER
		dlgtablist_create,			// DLGTYPE_TABLIST
		_cre_settext,				// DLGTYPE_RADIO
		_cre_settext,				// DLGTYPE_CHECK
		_cre_settext,				// DLGTYPE_FRAME
		_cre_settext,				// DLGTYPE_EDIT
		_cre_settext,				// DLGTYPE_TEXT
		dlgicon_create,				// DLGTYPE_ICON
		dlgvram_create,				// DLGTYPE_VRAM
		_cre,						// DLGTYPE_LINE
		_cre						// DLGTYPE_BOX
};

static const DLGPAINT dlgpaint[] = {
		dlgbase_paint,				// DLGTYPE_BASE
		dlgclose_paint,				// DLGTYPE_CLOSE
		dlgbtn_paint,				// DLGTYPE_BUTTON
		dlglist_paint,				// DLGTYPE_LIST
		dlgslider_paint,			// DLGTYPE_SLIDER
		dlgtablist_paint,			// DLGTYPE_TABLIST
		dlgradio_paint,				// DLGTYPE_RADIO
		dlgcheck_paint,				// DLGTYPE_CHECK
		dlgframe_paint,				// DLGTYPE_FRAME
		dlgedit_paint,				// DLGTYPE_EDIT
		dlgtext_paint,				// DLGTYPE_TEXT
		dlgicon_paint,				// DLGTYPE_ICON
		dlgvram_paint,				// DLGTYPE_VRAM
		dlgline_paint,				// DLGTYPE_LINE
		dlgbox_paint				// DLGTYPE_BOX
};

static const DLGSETVAL dlgsetval[] = {
		_setval,					// DLGTYPE_BASE
		_setval,					// DLGTYPE_CLOSE
		_setval,					// DLGTYPE_BUTTON
		dlglist_setval,				// DLGTYPE_LIST
		dlgslider_setval,			// DLGTYPE_SLIDER
		dlgtablist_setval,			// DLGTYPE_TABLIST
		dlgradio_setval,			// DLGTYPE_RADIO
		dlgcheck_setval				// DLGTYPE_CHECK
};

static const DLGCLICK dlgclick[] = {
		dlgbase_onclick,			// DLGTYPE_BASE
		dlgclose_onclick,			// DLGTYPE_CLOSE
		dlgbtn_onclick,				// DLGTYPE_BUTTON
		dlglist_onclick,			// DLGTYPE_LIST
		dlgslider_onclick,			// DLGTYPE_SLIDER
		dlgtablist_onclick,			// DLGTYPE_TABLIST
		dlgradio_onclick,			// DLGTYPE_RADIO
		dlgcheck_onclick			// DLGTYPE_CHECK
};

static const DLGMOV dlgmov[] = {
		dlgbase_move,				// DLGTYPE_BASE
		dlgclose_move,				// DLGTYPE_CLOSE
		dlgbtn_move,				// DLGTYPE_BUTTON
		dlglist_move,				// DLGTYPE_LIST
		dlgslider_move				// DLGTYPE_SLIDER
};

static const DLGREL dlgrel[] = {
		_moverel,					// DLGTYPE_BASE
		dlgclose_rel,				// DLGTYPE_CLOSE
		dlgbtn_rel,					// DLGTYPE_BUTTON
		dlglist_rel,				// DLGTYPE_LIST
		dlgslider_rel				// DLGTYPE_SLIDER
};


// ---- draw

static void draw(VRAMHDL dst, const RECT_T *rect, void *arg) {

	MENUDLG		dlg;

	dlg = (MENUDLG)arg;
	vrammix_cpy2(dst, rect, dlg->vram, NULL, 2);
}


typedef struct {
	MENUDLG	dlg;
	DLGHDL	hdl;
	RECT_T	rect;
} MDCB2;

static BOOL dc_cb(void *vpItem, void *vpArg) {

	DLGHDL	hdl;
	MDCB2	*mdcb;

	hdl = (DLGHDL)vpItem;
	mdcb = (MDCB2 *)vpArg;
	if (hdl == mdcb->hdl) {
		mdcb->hdl = NULL;
	}
	if ((mdcb->hdl != NULL) || (hdl->flag & MENU_DISABLE)) {
		goto dccb_exit;
	}
	if (rect_isoverlap(&mdcb->rect, &hdl->rect)) {
		hdl->flag |= MENU_REDRAW;
	}

dccb_exit:
	return(FALSE);
}


static BOOL dc_cb2(void *vpItem, void *vpArg) {

	MENUDLG	dlg;
	DLGHDL	hdl;

	hdl = (DLGHDL)vpItem;
	dlg = (MENUDLG)vpArg;
	if (hdl->flag & MENU_REDRAW) {
		hdl->flag &= ~MENU_REDRAW;
		if ((!(hdl->flag & MENU_DISABLE)) &&
			((UINT)hdl->type < NELEMENTS(dlgpaint))) {
			dlgpaint[hdl->type](dlg, hdl);
			menubase_setrect(dlg->vram, &hdl->rect);
		}
	}
	return(FALSE);
}


static void drawctrls(MENUDLG dlg, DLGHDL hdl) {

	MDCB2	mdcb;

	if (hdl) {
		if (hdl->flag & MENU_DISABLE) {
			goto dcs_end;
		}
		mdcb.rect = hdl->rect;
	}
	else {
		mdcb.rect.left = 0;
		mdcb.rect.top = 0;
		mdcb.rect.right = dlg->vram->width;
		mdcb.rect.bottom = dlg->vram->height;
	}
	mdcb.dlg = dlg;
	mdcb.hdl = hdl;
	listarray_enum(dlg->dlg, dc_cb, &mdcb);
	if (!dlg->locked) {
		listarray_enum(dlg->dlg, dc_cb2, dlg);
		menubase_draw(draw, dlg);
	}

dcs_end:
	return;
}

static void drawlock(BOOL lock) {

	MENUDLG	dlg;

	dlg = &menudlg;
	if (lock) {
		dlg->locked++;
	}
	else {
		dlg->locked--;
		if (!dlg->locked) {
			listarray_enum(dlg->dlg, dc_cb2, dlg);
			menubase_draw(draw, dlg);
		}
	}
}


// ----

static int defproc(int msg, MENUID id, long param) {

	if (msg == DLGMSG_CLOSE) {
		menubase_close();
	}
	(void)id;
	(void)param;
	return(0);
}


BRESULT menudlg_create(int width, int height, const OEMCHAR *str,
								int (*proc)(int msg, MENUID id, long param)) {

	MENUBASE	*mb;
	MENUDLG		dlg;

	dlg = &menudlg;
	if (menubase_open(2) != SUCCESS) {
		goto mdcre_err;
	}
	ZeroMemory(dlg, sizeof(_MENUDLG));
	if ((width <= 0) || (height <= 0)) {
		goto mdcre_err;
	}
	width += (MENU_FBORDER + MENU_BORDER) * 2;
	height += ((MENU_FBORDER + MENU_BORDER) * 2) +
					MENUDLG_CYCAPTION + MENUDLG_BORDER;
	mb = &menubase;
	dlg->font = mb->font;
	dlg->vram = vram_create(width, height, FALSE, mb->bpp);
	if (dlg->vram == NULL) {
		goto mdcre_err;
	}
	dlg->vram->posx = (mb->width - width) >> 1;
	dlg->vram->posy = (mb->height - height) >> 1;
	dlg->dlg = listarray_new(sizeof(_DLGHDL), 32);
	if (dlg->dlg == NULL) {
		goto mdcre_err;
	}
	dlg->res = listarray_new(sizeof(_DLGPRM), 32);
	if (dlg->res == NULL) {
		goto mdcre_err;
	}
	if (menudlg_append(DLGTYPE_BASE, SID_CAPTION, 0, str,
										0, 0, width, height) != SUCCESS) {
		goto mdcre_err;
	}
	if (menudlg_append(DLGTYPE_CLOSE, SID_CLOSE, 0, NULL,
							width - (MENU_FBORDER + MENU_BORDER) -
									(MENUDLG_CXCLOSE + MENUDLG_PXCAPTION),
							(MENU_FBORDER + MENU_BORDER) +
								((MENUDLG_CYCAPTION - MENUDLG_CYCLOSE) / 2),
							MENUDLG_CXCLOSE, MENUDLG_CYCLOSE) != SUCCESS) {
		goto mdcre_err;
	}
	dlg->sx = (MENU_FBORDER + MENU_BORDER);
	dlg->sy = (MENU_FBORDER + MENU_BORDER) +
							(MENUDLG_CYCAPTION + MENUDLG_BORDER);
	if (proc == NULL) {
		proc = defproc;
	}
	dlg->proc = proc;
	dlg->locked = 0;
	drawlock(TRUE);
	proc(DLGMSG_CREATE, 0, 0);
	drawctrls(dlg, NULL);
	drawlock(FALSE);

	return(SUCCESS);

mdcre_err:
	menubase_close();
	return(FAILURE);
}


static BOOL mdds_cb(void *vpItem, void *vpArg) {

	vram_destroy(((DLGHDL)vpItem)->vram);
	(void)vpArg;
	return(FALSE);
}

static BOOL delicon(void *vpItem, void *vpArg) {

	menuicon_unlock(((DLGPRM)vpItem)->icon);
	(void)vpArg;
	return(FALSE);
}

void menudlg_destroy(void) {

	MENUDLG		dlg;

	dlg = &menudlg;

	if (dlg->closing) {
		return;
	}
	dlg->closing = 1;
	dlg->proc(DLGMSG_DESTROY, 0, 0);
	listarray_enum(dlg->dlg, mdds_cb, NULL);
	menubase_clrrect(dlg->vram);
	vram_destroy(dlg->vram);
	dlg->vram = NULL;
	listarray_destroy(dlg->dlg);
	dlg->dlg = NULL;
	listarray_enum(dlg->res, delicon, NULL);
	listarray_destroy(dlg->res);
	dlg->res = NULL;
}


// ----

BRESULT menudlg_appends(const MENUPRM *res, int count) {

	BRESULT	r;

	r = SUCCESS;
	while(count--) {
		r |= menudlg_append(res->type, res->id, res->flg, res->arg,
							res->posx, res->posy, res->width, res->height);
		res++;
	}
	return(r);
}

BRESULT menudlg_append(int type, MENUID id, MENUFLG flg, const void *arg,
								int posx, int posy, int width, int height) {

	MENUDLG		dlg;
	DLGHDL		hdl;
	_DLGHDL		dhdl;

	dlg = &menudlg;

	if (flg & MENU_TABSTOP) {
		dlg->group++;
	}
	switch(type) {
		case DLGTYPE_LTEXT:
			type = DLGTYPE_TEXT;
			flg &= ~MST_POSMASK;
			flg |= MST_LEFT;
			break;

		case DLGTYPE_CTEXT:
			type = DLGTYPE_TEXT;
			flg &= ~MST_POSMASK;
			flg |= MST_CENTER;
			break;

		case DLGTYPE_RTEXT:
			type = DLGTYPE_TEXT;
			flg &= ~MST_POSMASK;
			flg |= MST_RIGHT;
			break;
	}

	ZeroMemory(&dhdl, sizeof(dhdl));
	dhdl.type = type;
	dhdl.id = id;
	dhdl.flag = flg;
	dhdl.page = dlg->page;
	dhdl.group = dlg->group;
	dhdl.rect.left = dlg->sx + posx;
	dhdl.rect.top = dlg->sy + posy;
	dhdl.rect.right = dhdl.rect.left + width;
	dhdl.rect.bottom = dhdl.rect.top + height;
	dhdl.prm = NULL;
	dhdl.prmcnt = 0;
	dhdl.val = 0;
	if (((UINT)type >= NELEMENTS(dlgcre)) ||
		(dlgcre[type](dlg, &dhdl, arg))) {
		goto mda_err;
	}
	drawlock(TRUE);
	hdl = (DLGHDL)listarray_append(dlg->dlg, &dhdl);
	drawctrls(dlg, hdl);
	drawlock(FALSE);
	return(SUCCESS);

mda_err:
	return(FAILURE);
}


// ---- moving

typedef struct {
	int		x;
	int		y;
	DLGHDL	ret;
} MDCB3;

static BOOL hps_cb(void *vpItem, void *vpArg) {

	DLGHDL	hdl;
	MDCB3	*mdcb;

	hdl = (DLGHDL)vpItem;
	mdcb = (MDCB3 *)vpArg;
	if ((!(hdl->flag & (MENU_DISABLE | MENU_GRAY))) &&
		(rect_in(&hdl->rect, mdcb->x, mdcb->y))) {
		mdcb->ret = hdl;
	}
	return(FALSE);
}

static DLGHDL hdlpossea(MENUDLG dlg, int x, int y) {

	MDCB3	mdcb;

	mdcb.x = x;
	mdcb.y = y;
	mdcb.ret = NULL;
	listarray_enum(dlg->dlg, hps_cb, &mdcb);
	return(mdcb.ret);
}

void menudlg_moving(int x, int y, int btn) {

	MENUDLG	dlg;
	DLGHDL	hdl;
	int		focus;

	drawlock(TRUE);
	dlg = &menudlg;
	x -= dlg->vram->posx;
	y -= dlg->vram->posy;
	if (!dlg->btn) {
		if (btn == 1) {
			hdl = hdlpossea(dlg, x, y);
			if (hdl) {
				x -= hdl->rect.left;
				y -= hdl->rect.top;
				dlg->btn = 1;
				dlg->lastid = hdl->id;
				if ((UINT)hdl->type < NELEMENTS(dlgclick)) {
					dlgclick[hdl->type](dlg, hdl, x, y);
				}
			}
		}
	}
	else {
		hdl = dlghdlsea(dlg, dlg->lastid);
		if (hdl) {
			focus = rect_in(&hdl->rect, x, y);
			x -= hdl->rect.left;
			y -= hdl->rect.top;
			if ((UINT)hdl->type < NELEMENTS(dlgmov)) {
				dlgmov[hdl->type](dlg, hdl, x, y, focus);
			}
			if (btn == 2) {
				dlg->btn = 0;
				if ((UINT)hdl->type < NELEMENTS(dlgrel)) {
					dlgrel[hdl->type](dlg, hdl, focus);
				}
			}
		}
	}
	drawlock(FALSE);
}


// ---- ctrl

INTPTR menudlg_msg(int ctrl, MENUID id, INTPTR arg) {

	INTPTR	ret;
	MENUDLG	dlg;
	DLGHDL	hdl;
	int		flg;

	ret = 0;
	dlg = &menudlg;
	hdl = dlghdlsea(dlg, id);
	if (hdl == NULL) {
		goto mdm_exit;
	}
	drawlock(TRUE);
	switch(ctrl) {
		case DMSG_SETHIDE:
			ret = (hdl->flag & MENU_DISABLE) ? 1 : 0;
			flg = (arg) ? MENU_DISABLE : 0;
			if ((hdl->flag ^ flg) & MENU_DISABLE) {
				hdl->flag ^= MENU_DISABLE;
				if (flg) {
					drawctrls(dlg, NULL);
				}
				else {
					drawctrls(dlg, hdl);
				}
			}
			break;

		case DMSG_GETHIDE:
			ret = (hdl->flag & MENU_DISABLE) ? 1 : 0;
			break;

		case DMSG_SETENABLE:
			ret = (hdl->flag & MENU_GRAY) ? 0 : 1;
			flg = (arg) ? 0 : MENU_GRAY;
			if ((hdl->flag ^ flg) & MENU_GRAY) {
				hdl->flag ^= MENU_GRAY;
				drawctrls(dlg, hdl);
			}
			break;

		case DMSG_GETENABLE:
			ret = (hdl->flag & MENU_GRAY) ? 0 : 1;
			break;

		case DMSG_SETVAL:
			ret = hdl->val;
			if ((UINT)hdl->type < NELEMENTS(dlgsetval)) {
				dlgsetval[hdl->type](dlg, hdl, (int)arg);
			}
			break;

		case DMSG_GETVAL:
			ret = hdl->val;
			break;

		case DMSG_SETVRAM:
			if (hdl->type == DLGTYPE_VRAM) {
				ret = (INTPTR)hdl->c.dv.vram;
				hdl->c.dv.vram = (VRAMHDL)arg;
				drawctrls(dlg, hdl);
			}
			break;

		case DMSG_SETTEXT:
			switch(hdl->type) {
				case DLGTYPE_BUTTON:
				case DLGTYPE_RADIO:
				case DLGTYPE_CHECK:
				case DLGTYPE_EDIT:
				case DLGTYPE_TEXT:
					dlgtext_itemset(dlg, hdl, (OEMCHAR*)arg);
					drawctrls(dlg, hdl);
					break;
			}
			break;

		case DMSG_SETICON:
			switch(hdl->type) {
				case DLGTYPE_BUTTON:
				case DLGTYPE_RADIO:
				case DLGTYPE_CHECK:
				case DLGTYPE_EDIT:
				case DLGTYPE_TEXT:
					dlgtext_iconset(dlg, hdl, (UINT)arg);
					drawctrls(dlg, hdl);
					break;
			}
			break;

		case DMSG_ITEMAPPEND:
			switch(hdl->type) {
				case DLGTYPE_LIST:
					if (dlglist_append(dlg, hdl, (OEMCHAR*)arg)) {
						drawctrls(dlg, hdl);
					}
					break;

				case DLGTYPE_TABLIST:
					dlgtablist_append(dlg, hdl, (OEMCHAR*)arg);
					drawctrls(dlg, hdl);
					break;
			}
			break;

		case DMSG_ITEMRESET:
			if ((dlg->btn) && (dlg->lastid == hdl->id)) {
				dlg->btn = 0;
				if ((UINT)hdl->type < NELEMENTS(dlgrel)) {
					dlgrel[hdl->type](dlg, hdl, FALSE);
				}
			}
			if (hdl->type == DLGTYPE_LIST) {
				dlglist_reset(dlg, hdl);
				drawctrls(dlg, hdl);
			}
			break;

		case DMSG_ITEMSETEX:
			if (hdl->type == DLGTYPE_LIST) {
				if (dlglist_setex(dlg, hdl, (ITEMEXPRM *)arg)) {
					drawctrls(dlg, hdl);
				}
			}
			break;

		case DMSG_SETLISTPOS:
			if (hdl->type == DLGTYPE_LIST) {
				ret = hdl->c.dl.basepos;
				dlglist_setbasepos(dlg, hdl, (int)arg);
				drawctrls(dlg, hdl);
			}
			break;

		case DMSG_GETRECT:
			ret = (INTPTR)&hdl->rect;
			break;

		case DMSG_SETRECT:
			ret = (INTPTR)&hdl->rect;
			if ((hdl->type == DLGTYPE_TEXT) && (arg)) {
				drawctrls(dlg, hdl);
				hdl->rect = *(RECT_T *)arg;
				drawctrls(dlg, hdl);
			}
			break;

		case DMSG_SETFONT:
			if (hdl->type == DLGTYPE_LIST) {
				ret = (INTPTR)dlglist_setfont(hdl, (void*)arg);
				drawctrls(dlg, hdl);
			}
			else if (hdl->type == DLGTYPE_TABLIST) {
				ret = (INTPTR)dlgtablist_setfont(hdl, (void*)arg);
				drawctrls(dlg, hdl);
			}
			else if (hdl->type == DLGTYPE_TEXT) {
				ret = (INTPTR)hdl->c.dt.font;
				hdl->c.dt.font = (void*)arg;
				drawctrls(dlg, hdl);
			}
			break;

		case DMSG_GETFONT:
			if (hdl->type == DLGTYPE_LIST) {
				ret = (INTPTR)hdl->c.dl.font;
			}
			else if (hdl->type == DLGTYPE_TABLIST) {
				ret = (INTPTR)hdl->c.dtl.font;
			}
			else if (hdl->type == DLGTYPE_TEXT) {
				ret = (INTPTR)hdl->c.dt.font;
			}
			break;

	}
	drawlock(FALSE);

mdm_exit:
	return(ret);
}


// --- page

void menudlg_setpage(MENUID page) {

	MENUDLG	dlg;

	dlg = &menudlg;
	dlg->page = page;
}


typedef struct {
	MENUID	page;
	MENUFLG	flag;
} MDCB4;

static BOOL mddph_cb(void *vpItem, void *vpArg) {

	DLGHDL	hdl;
	MDCB4	*mdcb;

	hdl = (DLGHDL)vpItem;
	mdcb = (MDCB4 *)vpArg;
	if ((hdl->page == mdcb->page) &&
		((hdl->flag ^ mdcb->flag) & MENU_DISABLE)) {
		hdl->flag ^= MENU_DISABLE;
	}
	return(FALSE);
}

void menudlg_disppagehidden(MENUID page, BOOL hidden) {

	MENUDLG	dlg;
	MDCB4	mdcb;

	dlg = &menudlg;
	mdcb.page = page;
	mdcb.flag = (hidden)?MENU_DISABLE:0;
	listarray_enum(dlg->dlg, mddph_cb, &mdcb);
	drawlock(TRUE);
	drawctrls(dlg, NULL);
	drawlock(FALSE);
}

