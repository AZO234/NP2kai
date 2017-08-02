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


#ifdef __cplusplus
extern "C" {
#endif

void uPD4990_reset(const NP2CFG *pConfig);
void uPD4990_bind(void);

#ifdef SUPPORT_HRTIMER
extern void upd4990_hrtimer_start(void);
extern void upd4990_hrtimer_stop(void);
extern void upd4990_hrtimer_count(void);
#endif	/* SUPPORT_HRTIMER */

#ifdef __cplusplus
}
#endif

#endif	/* NP2_UPD4990_H */

