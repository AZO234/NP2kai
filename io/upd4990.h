#ifndef NP2_UPD4990_H
#define NP2_UPD4990_H

#define UPD4990_REGLEN	8

typedef struct {
	UINT8	last;
	UINT8	cmd;
	UINT8	serial;
	UINT8	parallel;
	UINT8	reg[UPD4990_REGLEN];
	UINT	pos;
	UINT8	cdat;
	UINT8	regsft;
} _UPD4990, *UPD4990;

typedef struct {
	UINT32 hrtimerdiv; 
	UINT32 hrtimerclock; 
	UINT32 hrtimerclock32; 
	
	UINT32 clockcounter;
	UINT32 clockcounter32;
} _UPD4990HRT, *UPD4990HRT;


#ifdef __cplusplus
extern "C" {
#endif
	
#if defined(SUPPORT_HRTIMER)
void upd4990_hrtimer_count(void);
#endif

void uPD4990_reset(const NP2CFG *pConfig);
void uPD4990_bind(void);

#ifdef __cplusplus
}
#endif

#endif	/* NP2_UPD4990_H */

