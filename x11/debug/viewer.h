#ifndef	NP2_X11_VIEWER_H__
#define	NP2_X11_VIEWER_H__

#if defined(SUPPORT_VIEWER)

G_BEGIN_DECLS

#define	NP2VIEW_MAX	8

typedef struct {
	UINT8	vram;
	UINT8	itf;
	UINT8	A20;
} VIEWMEM_T;

enum {
	VIEWMODE_REG = 0,
	VIEWMODE_SEG,
	VIEWMODE_1MB,
	VIEWMODE_ASM,
	VIEWMODE_SND
};

enum {
	ALLOCTYPE_NONE = 0,
	ALLOCTYPE_REG,
	ALLOCTYPE_SEG,
	ALLOCTYPE_1MB,
	ALLOCTYPE_ASM,
	ALLOCTYPE_SND,

	ALOOCTYPE_ERROR = 0xffffffff
};

typedef struct {
	UINT32	type;
	UINT32	arg;
	UINT32	size;
	void	*ptr;
} VIEWMEMBUF;

typedef struct {
	void		*window;
	void		*widget;
	void		*vscr;
	void		*menu;
	void		*font;
	UINT32		index;
	UINT32		last;
	UINT8		fontsize;
	UINT8		enter;
	UINT8		pad[2];

	VIEWMEMBUF	buf1;
	VIEWMEMBUF	buf2;
	UINT32		pos;
	UINT32		maxline;
	UINT16		step;
	UINT16		mul;
	UINT8		alive;
	UINT8		type;
	UINT8		lock;
	UINT8		active;
	UINT16		seg;
	UINT16		off;
	VIEWMEM_T	dmem;
} NP2VIEW_T;

extern	NP2VIEW_T	np2view[NP2VIEW_MAX];

BOOL viewer_init(void);
void viewer_term(void);

void viewer_open(void);
void viewer_allclose(void);

void viewer_allreload(BOOL force);

G_END_DECLS

#else	/* SUPPORT_VIEWER */

#define viewer_init()
#define viewer_term()
#define viewer_open()
#define viewer_allclose()
#define viewer_allreload(v)

#endif	/* SUPPORT_VIEWER */

#endif	/* NP2_X11_VIEWER_H__ */
