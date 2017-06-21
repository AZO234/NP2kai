
// vermouth‚Ì‚İg—p‚ÌCOMMNG-MIDI
// ‚ ‚Ü‚è‚Éˆê‚·‚¬‚é‚ñ‚Å ŠÖ”–¼•Ï‚¦‚Ä‚±‚Á‚¿‚Ö


// ---- com manager midi for vermouth

#define	COMSIG_MIDI		0x4944494d

#ifdef __cplusplus
extern "C" {
#endif

void cmvermouth_initialize(void);
COMMNG cmvermouth_create(void);
#if defined(VERMOUTH_LIB)
void cmvermouth_load(UINT rate);
void cmvermouth_unload(void);
#endif

#ifdef __cplusplus
}
#endif

