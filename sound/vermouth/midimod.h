
struct _pathlist;
typedef	struct _pathlist	_PATHLIST;
typedef	struct _pathlist	*PATHLIST;

struct _pathlist {
	PATHLIST	next;
	OEMCHAR		path[MAX_PATH];
};

enum {
	TONECFG_EXIST		= 0x01,
	TONECFG_NOLOOP		= 0x02,
	TONECFG_NOENV		= 0x04,
	TONECFG_KEEPENV		= 0x08,
	TONECFG_NOTAIL		= 0x10,

	TONECFG_AUTOAMP		= -1,
	TONECFG_VARIABLE	= 0xff
};

typedef struct {
	OEMCHAR	*name;
	int		amp;
	UINT8	flag;
	UINT8	pan;
	UINT8	note;
} _TONECFG, *TONECFG;


#ifdef __cplusplus
extern "C" {
#endif

BRESULT VERMOUTHCL midimod_getfile(MIDIMOD mod, const OEMCHAR *filename,
													OEMCHAR *path, int size);
void VERMOUTHCL midimod_lock(MIDIMOD mod);
void VERMOUTHCL midimod_unlock(MIDIMOD mod);

#ifdef __cplusplus
}
#endif

