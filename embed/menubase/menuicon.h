
enum {
	MICON_NULL			= 0,
	MICON_STOP,
	MICON_QUESTION,
	MICON_EXCLAME,
	MICON_INFO,
	MICON_FOLDER,
	MICON_FOLDERPARENT,
	MICON_FILE,
	MICON_USER
};

#ifdef __cplusplus
extern "C" {
#endif

void menuicon_initialize(void);
void menuicon_deinitialize(void);
void menuicon_regist(UINT16 id, const MENURES *res);
VRAMHDL menuicon_lock(UINT16 id, int width, int height, int bpp);
void menuicon_unlock(VRAMHDL vram);

#ifdef __cplusplus
}
#endif

