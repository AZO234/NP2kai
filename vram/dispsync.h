
typedef struct {
	UINT	text_vbp;
	UINT	textymax;
	UINT	grph_vbp;
	UINT	grphymax;

	UINT	scrnxpos;
	UINT	scrnxmax;
	UINT	scrnxextend;
	UINT	scrnymax;
	UINT32	textvad;
	UINT32	grphvad;

	UINT32	gdcwovrg; // 98標準グラフィックで画面幅80桁超えを認める
} DSYNC;


#ifdef __cplusplus
extern "C" {
#endif

extern	DSYNC	dsync;

void dispsync_initialize(void);
BOOL dispsync_renewalmode(void);
BOOL dispsync_renewalhorizontal(void);
BOOL dispsync_renewalvertical(void);
void dispsync_updateoverrange(void);

#ifdef __cplusplus
}
#endif

