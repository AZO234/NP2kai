#ifndef	NP2_X11_TOOLWIN_H__
#define	NP2_X11_TOOLWIN_H__

G_BEGIN_DECLS

enum {
	SKINMRU_MAX	= 4,
	FDDLIST_DRV	= 2,
	FDDLIST_MAX	= 8
};

typedef struct {
	int	insert;
	UINT	cnt;
	UINT	pos[FDDLIST_MAX];
	char	name[FDDLIST_MAX][MAX_PATH];
} TOOLFDD;

typedef struct {
	int	posx;
	int	posy;
	BOOL	type;
	TOOLFDD	fdd[FDDLIST_DRV];
	char	skin[MAX_PATH];
	char	skinmru[SKINMRU_MAX][MAX_PATH];
} NP2TOOL;

extern NP2TOOL np2tool;

#if !defined(SUPPORT_TOOLWINDOW)

#define	toolwin_create()
#define	toolwin_destroy()
#define	toolwin_setfdd(drv, name)
#define	toolwin_fddaccess(drv)
#define	toolwin_hddaccess(drv)
#define	toolwin_draw(frame)		(void)frame
#define	toolwin_readini()
#define	toolwin_writeini()

#else	/* !SUPPORT_TOOLWIN */

void toolwin_create(void);
void toolwin_destroy(void);

void toolwin_setfdd(UINT8 drv, const char *name);

void toolwin_fddaccess(UINT8 drv);
void toolwin_hddaccess(UINT8 drv);

void toolwin_draw(UINT8 frame);

void toolwin_readini(void);
void toolwin_writeini(void);

#endif	/* SUPPORT_TOOLWIN */

G_END_DECLS

#endif	/* NP2_X11_TOOLWIN_H__ */
