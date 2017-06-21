
#ifndef SCRNCALL
#define	SCRNCALL
#endif

enum {
	SURFACE_WIDTH		= 640,
	SURFACE_HEIGHT		= 480,
	SURFACE_SIZE		= (SURFACE_WIDTH * SURFACE_HEIGHT),

	START_PALORG		= 0x0a,
	START_PAL			= 0x10
};


#ifdef __cplusplus
extern "C" {
#endif

extern	UINT8	renewal_line[SURFACE_HEIGHT];
extern	UINT8	np2_tram[SURFACE_SIZE];
extern	UINT8	np2_vram[2][SURFACE_SIZE];

void scrndraw_initialize(void);
void scrndraw_changepalette(void);
UINT8 scrndraw_draw(UINT8 update);
void scrndraw_redraw(void);

#ifdef __cplusplus
}
#endif

