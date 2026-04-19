
// どーでもいい通知系

enum {
	SYS_UPDATECFG		= 0x0001,
	SYS_UPDATEOSCFG		= 0x0002,
	SYS_UPDATECLOCK		= 0x0004,
	SYS_UPDATERATE		= 0x0008,
	SYS_UPDATESBUF		= 0x0010,
	SYS_UPDATEMIDI		= 0x0020,
	SYS_UPDATESBOARD	= 0x0040,
	SYS_UPDATEFDD		= 0x0080,
	SYS_UPDATEHDD		= 0x0100,
	SYS_UPDATEMEMORY	= 0x0200,
	SYS_UPDATESERIAL1	= 0x0400,

	SYS_UPDATESNDDEV	= 0x1000
};

enum {
	SYS_UPDATECAPTION_FDD	= 0x01,
	SYS_UPDATECAPTION_CLK	= 0x02,
	SYS_UPDATECAPTION_MISC	= 0x04,
	
	SYS_UPDATECAPTION_ALL	= 0xff,
};

#define FDDMENU_ITEMS_MAX	20

typedef struct {
	UINT8	showvolume;
	UINT8	showmousespeed;
} SYSMNGMISCINFO;

#ifdef __cplusplus
extern "C" {
#endif

extern	UINT	sys_updates;

extern	SYSMNGMISCINFO	sys_miscinfo;

#if 0
void sysmng_initialize(void);
void sysmng_update(UINT bitmap);
void sysmng_cpureset(void);
void sysmng_fddaccess(UINT8 drv);
void sysmng_hddaccess(UINT8 drv);
#else

// マクロ(単に関数コールしたくないだけ)
#define	sysmng_initialize()												\
			sys_updates = 0

#define	sysmng_update(a)												\
			sys_updates |= (a);											\
			if ((a) & SYS_UPDATEFDD) sysmng_updatecaption(SYS_UPDATECAPTION_FDD)

#define	sysmng_cpureset()												\
			sys_updates &= (SYS_UPDATECFG | SYS_UPDATEOSCFG);			\
			sysmng_workclockreset()

#define	sysmng_fddaccess(a)												\
			toolwin_fddaccess((a))

#define	sysmng_hddaccess(a)												\
			toolwin_hddaccess((a));

#endif


// ---- あとはOS依存部

void sysmng_workclockreset(void);
BOOL sysmng_workclockrenewal(void);
void sysmng_updatecaption(UINT8 flag);
void sysmng_requestupdatecaption(UINT8 flag);
void sysmng_requestupdatecheck(void);

void sysmng_findfile_Initialize(void);
void sysmng_findfile_Finalize(void);
OEMCHAR* sysmng_getfddlistitem(int drv, int index);
OEMCHAR* sysmng_getlastfddlistitem(int drv);

void toolwin_fddaccess(UINT8 drv);
void toolwin_hddaccess(UINT8 drv);


#ifdef __cplusplus
}
#endif

