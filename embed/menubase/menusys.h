
enum {
	MENUSYS_MAX			= 8
};

enum {
	SMSG_SETHIDE		= 0,
	SMSG_GETHIDE,
	SMSG_SETENABLE,
	SMSG_GETENABLE,
	SMSG_SETCHECK,
	SMSG_GETCHECK,
	SMSG_SETTEXT
};

enum {
	MENUS_POPUP			= 0x0010,
	MENUS_SYSTEM		= 0x0020,
	MENUS_MINIMIZE		= 0x0030,
	MENUS_CLOSE			= 0x0040,
	MENUS_CTRLMASK		= 0x0070
};

enum {
	MENUSTYLE_BOTTOM	= 0x0001
};

typedef struct _smi {
const OEMCHAR		*string;
const struct _smi	*child;
	MENUID			id;
	MENUFLG			flag;
} MSYSITEM;


#ifdef __cplusplus
extern "C" {
#endif

BRESULT menusys_create(const MSYSITEM *item, void (*cmd)(MENUID id),
										UINT16 icon, const OEMCHAR *title);
void menusys_destroy(void);

BRESULT menusys_open(int x, int y);
void menusys_close(void);

void menusys_moving(int x, int y, int btn);
void menusys_key(UINT key);

INTPTR menusys_msg(int ctrl, MENUID id, INTPTR arg);

void menusys_setstyle(UINT16 style);

#ifdef __cplusplus
}
#endif


// ---- MACRO

#define menusys_sethide(id, hide)		\
				menusys_msg(SMSG_SETHIDE, (id), (INTPTR)(hide))
#define menusys_gethide(id)				\
				((int)menusys_msg(SMSG_GETHIDE, (id), 0))

#define menusys_setenable(id, enable)	\
				menusys_msg(SMSG_SETENABLE, (id), (long)(enable))
#define menusys_getenable(id)			\
				((int)menusys_msg(SMSG_GETENABLE, (id), 0))

#define menusys_setcheck(id, checked)	\
				menusys_msg(SMSG_SETCHECK, (id), (long)(checked))
#define menusys_getcheck(id)			\
				((int)menusys_msg(SMSG_GETCHECK, (id), 0))

#define menusys_settext(id, str)		\
				menusys_msg(SMSG_SETTEXT, (id), (INTPTR)(str))

