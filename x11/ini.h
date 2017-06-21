#ifndef	NP2_X11_INI_H__
#define	NP2_X11_INI_H__

G_BEGIN_DECLS

enum {
	INITYPE_STR		= 0,
	INITYPE_BOOL,
	INITYPE_BITMAP,
	INITYPE_ARGS16,
	INITYPE_ARGH8,
	INITYPE_SINT8,
	INITYPE_SINT16,
	INITYPE_SINT32,
	INITYPE_UINT8,
	INITYPE_UINT16,
	INITYPE_UINT32,
	INITYPE_HEX8,
	INITYPE_HEX16,
	INITYPE_HEX32,
	INITYPE_BYTE3,
	INITYPE_KB,
	INITYPE_USER,
	INITYPE_SNDDRV,
	INITYPE_INTERP,
	INITYPE_MASK		= 0xff,

	INIFLAG_RO		= 0x0100,
	INIFLAG_MAX		= 0x0200,
	INIFLAG_AND		= 0x0400,
};

typedef struct {
	char	item[10];
	UINT16	itemtype;
	void	*value;
	UINT32	arg;
} INITBL;

void ini_read(const char *path, const char *title, INITBL *tbl, UINT count);
void ini_write(const char *path, const char *title, INITBL *tbl, UINT count, BOOL create);

void initload(void);
void initsave(void);

G_END_DECLS

#endif	/* NP2_X11_INI_H__ */
