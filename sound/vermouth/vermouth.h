
#ifndef __VERMOUTH_H
#define	__VERMOUTH_H

#ifndef VERMOUTHCL
#define	VERMOUTHCL
#endif

#ifndef VERMOUTH_EXPORTS
#define	VEXTERN
#define	VEXPORT		VERMOUTHCL
#else
#define	VEXTERN		__declspec(dllexport)
#define	VEXPORT		WINAPI
#endif

typedef struct {
	UINT	samprate;
} *MIDIMOD;

typedef struct {
	UINT	samprate;
	UINT	worksize;
} *MIDIHDL;

typedef struct {
	void	*userdata;
	UINT	totaltones;
	UINT	progress;
	UINT	bank;
	UINT	num;
} MIDIOUTLAEXPARAM;
typedef BRESULT (*FNMIDIOUTLAEXCB)(MIDIOUTLAEXPARAM *param);


#ifdef __cplusplus
extern "C" {
#endif

VEXTERN UINT VEXPORT midiout_getver(char *string, UINT leng);
VEXTERN MIDIHDL VEXPORT midiout_create(MIDIMOD mod, UINT worksize);
VEXTERN void VEXPORT midiout_destroy(MIDIHDL hdl);
VEXTERN void VEXPORT midiout_shortmsg(MIDIHDL hdl, UINT32 msg);
VEXTERN void VEXPORT midiout_longmsg(MIDIHDL hdl, const void *msg, UINT size);
VEXTERN const SINT32 * VEXPORT midiout_get(MIDIHDL hdl, UINT *samples);
VEXTERN UINT VEXPORT midiout_get16(MIDIHDL hdl, SINT16 *pcm, UINT size);
VEXTERN UINT VEXPORT midiout_get32(MIDIHDL hdl, SINT32 *pcm, UINT size);
VEXTERN void VEXPORT midiout_setgain(MIDIHDL hdl, int gain);
VEXTERN void VEXPORT midiout_setmoduleid(MIDIHDL hdl, UINT8 moduleid);
VEXTERN void VEXPORT midiout_setportb(MIDIHDL hdl, MIDIHDL portb);

VEXTERN MIDIMOD VEXPORT midimod_create(UINT samprate);
VEXTERN void VEXPORT midimod_destroy(MIDIMOD hdl);
VEXTERN void VEXPORT midimod_destroy(MIDIMOD hdl);
VEXTERN BRESULT VEXPORT midimod_cfgload(MIDIMOD mod, const OEMCHAR *filename);
VEXTERN void VEXPORT midimod_loadprogram(MIDIMOD hdl, UINT num);
VEXTERN void VEXPORT midimod_loadrhythm(MIDIMOD hdl, UINT num);
VEXTERN void VEXPORT midimod_loadgm(MIDIMOD hdl);
VEXTERN void VEXPORT midimod_loadall(MIDIMOD hdl);
VEXTERN int VEXPORT midimod_loadallex(MIDIMOD hdl, FNMIDIOUTLAEXCB cb, void *userdata);

#ifdef __cplusplus
}
#endif

#endif

