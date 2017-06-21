
enum {
	DLGMSG_CREATE	= 0,
	DLGMSG_COMMAND,
	DLGMSG_CLOSE,
	DLGMSG_DESTROY
};

enum {
	DMSG_SETHIDE		= 0,
	DMSG_GETHIDE,
	DMSG_SETENABLE,
	DMSG_GETENABLE,
	DMSG_SETVAL,
	DMSG_GETVAL,
	DMSG_SETVRAM,
	DMSG_SETTEXT,
	DMSG_SETICON,
	DMSG_ITEMAPPEND,
	DMSG_ITEMRESET,
	DMSG_ITEMSET,
	DMSG_ITEMSETEX,
	DMSG_SETLISTPOS,
	DMSG_SETRECT,
	DMSG_GETRECT,
	DMSG_SETFONT,
	DMSG_GETFONT
};

enum {
	MSS_TOP				= 0x0020,
	MSS_BOTH			= 0x0010,
	MSS_POSMASK			= 0x0030,
	MSS_VERT			= 0x0040,

	MSL_VERT			= 0x0010,

	MST_LEFT			= 0x0000,
	MST_CENTER			= 0x0010,
	MST_RIGHT			= 0x0020,
	MST_POSMASK			= 0x0030
};


enum {
	DLGTYPE_BASE	= 0,
	DLGTYPE_CLOSE,
	DLGTYPE_BUTTON,
	DLGTYPE_LIST,

	DLGTYPE_SLIDER,

	DLGTYPE_TABLIST,
	DLGTYPE_RADIO,
	DLGTYPE_CHECK,

	DLGTYPE_FRAME,
	DLGTYPE_EDIT,
	DLGTYPE_TEXT,
	DLGTYPE_ICON,
	DLGTYPE_VRAM,
	DLGTYPE_LINE,
	DLGTYPE_BOX,

// å›ä∑ópÅc
	DLGTYPE_LTEXT,
	DLGTYPE_CTEXT,
	DLGTYPE_RTEXT
};

typedef struct {
	int		type;
	MENUID	id;
	MENUFLG	flg;
const void	*arg;
	int		posx;
	int		posy;
	int		width;
	int		height;
} MENUPRM;

typedef struct {
	UINT16		pos;
	UINT16		icon;
const OEMCHAR	*str;
} ITEMEXPRM;

#define SLIDERPOS(a, b)		(((UINT16)a) | (((UINT16)b) << 16))


#ifdef __cplusplus
extern "C" {
#endif

BRESULT menudlg_create(int width, int height, const OEMCHAR *str,
								int (*proc)(int msg, MENUID id, long param));
void menudlg_destroy(void);

BRESULT menudlg_appends(const MENUPRM *res, int count);
BRESULT menudlg_append(int type, MENUID id, MENUFLG flg, const void *arg,
								int posx, int posy, int width, int height);

void menudlg_moving(int x, int y, int btn);

INTPTR menudlg_msg(int ctrl, MENUID id, INTPTR arg);
void menudlg_setpage(MENUID page);
void menudlg_disppagehidden(MENUID page, BOOL hidden);

#ifdef __cplusplus
}
#endif


// ---- MACRO

#define menudlg_sethide(id, hidden)		\
					menudlg_msg(DMSG_SETHIDE, (id), (hidden))
#define menudlg_setenable(id, enable)	\
					menudlg_msg(DMSG_SETENABLE, (id), (enable))
#define menudlg_setval(id, val)			\
					menudlg_msg(DMSG_SETVAL, (id), (UINT)(val))
#define menudlg_getval(id)				\
					((UINT)menudlg_msg(DMSG_GETVAL, (id), 0))
#define	menudlg_setvram(id, vram)		\
					((VRAMHDL)menudlg_msg(DMSG_SETVRAM, (id), (INTPTR)(vram)))
#define menudlg_settext(id, arg)		\
					menudlg_msg(DMSG_SETTEXT, (id), (INTPTR)(arg))
#define menudlg_seticon(id, val)		\
					menudlg_msg(DMSG_SETICON, (id), (val))
#define menudlg_itemappend(id, arg)		\
					menudlg_msg(DMSG_ITEMAPPEND, (id), (INTPTR)(arg))
#define menudlg_itemreset(id)			\
					menudlg_msg(DMSG_ITEMRESET, (id), 0)
#define menudlg_itemsetex(id, arg)		\
					menudlg_msg(DMSG_ITEMSETEX, (id), (INTPTR)(arg))
#define menudlg_setlistpos(id, num)		\
					menudlg_msg(DMSG_SETLISTPOS, (id), (num))
#define menudlg_setrect(id, rect)		\
					menudlg_msg(DMSG_SETRECT, (id), (INTPTR)(rect))
#define menudlg_getrect(id)				\
					((RECT_T *)menudlg_msg(DMSG_GETRECT, (id), 0))
#define menudlg_setfont(id, font)		\
					menudlg_msg(DMSG_SETFONT, (id), (INTPTR)(font))
#define menudlg_getfont(id)				\
					menudlg_msg(DMSG_GETFONT, (id), 0)
