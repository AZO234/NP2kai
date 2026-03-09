
enum {
	MIMPI_LA		= 0,
	MIMPI_PCM,
	MIMPI_GS,
	MIMPI_RHYTHM
};

typedef struct {
	UINT8	ch[16];
	UINT8	map[3][128];
	UINT8	bank[3][128];
} MIMPIDEF;


#ifdef __cplusplus
extern "C" {
#endif

BRESULT mimpidef_load(MIMPIDEF *def, const OEMCHAR *filename);

#ifdef __cplusplus
}
#endif

