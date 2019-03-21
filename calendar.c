#include	"compiler.h"
#include	"parts.h"
#include	"timemng.h"
#include	"pccore.h"
#include	"calendar.h"


	_CALENDAR	cal;


static const UINT8 days[12] = {	31, 28, 31, 30, 31, 30,
								31, 31, 30, 31, 30, 31};

static void secinc(_SYSTIME *dt) {

	UINT	month;
	UINT16	daylimit;

	dt->second++;
	if (dt->second < 60) {
		goto secinc_exit;
	}
	dt->second = 0;
	dt->minute++;
	if (dt->minute < 60) {
		goto secinc_exit;
	}
	dt->minute = 0;
	dt->hour++;
	if (dt->hour < 24) {
		goto secinc_exit;
	}
	dt->hour = 0;

	month = dt->month - 1;
	if (month < 12) {
		daylimit = days[month];
		if ((daylimit == 28) && (!(dt->year & 3))) {
			daylimit++;						// = 29;
		}
	}
	else {
		daylimit = 30;
	}
	dt->week = (UINT16)((dt->week + 1) % 7);
	dt->day++;
	if (dt->day <= daylimit) {
		goto secinc_exit;
	}
	dt->day = 1;
	dt->month++;
	if (dt->month <= 12) {
		goto secinc_exit;
	}
	dt->month = 1;
	dt->year++;

secinc_exit:
	return;
}

static void date2deg(_SYSTIME *t, const UINT8 *bcd) {

	UINT16	year;

	year = 1900 + AdjustBeforeDivision(bcd[0]);
	if (year < 1980) {
		year += 100;
	}
	t->year = (UINT16)year;
	t->week = (UINT16)((bcd[1]) & 0x0f);
	t->month = (UINT16)((bcd[1]) >> 4);
	t->day = (UINT16)AdjustBeforeDivision(bcd[2]);
	t->hour = (UINT16)AdjustBeforeDivision(bcd[3]);
	t->minute = (UINT16)AdjustBeforeDivision(bcd[4]);
	t->second = (UINT16)AdjustBeforeDivision(bcd[5]);
}

static void date2bcd(UINT8 *bcd, const _SYSTIME *t) {

	bcd[0] = AdjustAfterMultiply((UINT8)(t->year % 100));
	bcd[1] = (UINT8)((t->month << 4) + t->week);
	bcd[2] = AdjustAfterMultiply((UINT8)t->day);
	bcd[3] = AdjustAfterMultiply((UINT8)t->hour);
	bcd[4] = AdjustAfterMultiply((UINT8)t->minute);
	bcd[5] = AdjustAfterMultiply((UINT8)t->second);
}


// -----

void calendar_initialize(void) {

	timemng_gettime(&cal.dt);
	cal.steps = 0;
	cal.realchg = 1;
}

void calendar_inc(void) {

	cal.realchg = 1;

	// 56.4Hzだから…
	cal.steps += 10;
	if (cal.steps < 564) {
		return;
	}
	cal.steps -= 564;
	secinc(&cal.dt);
}

void calendar_set(const UINT8 *bcd) {

	date2deg(&cal.dt, bcd);
}

void calendar_getvir(UINT8 *bcd) {

	date2bcd(bcd, &cal.dt);
}

void calendar_getreal(UINT8 *bcd) {

	timemng_gettime(&cal.realc);
	date2bcd(bcd, &cal.realc);
}

void calendar_get(UINT8 *bcd) {

	if (!np2cfg.calendar) {
		date2bcd(bcd, &cal.dt);
	}
	else {
		if (cal.realchg) {
			cal.realchg = 0;
			timemng_gettime(&cal.realc);
		}
		date2bcd(bcd, &cal.realc);
	}
}

