#include	<compiler.h>
#include	<common/parts.h>
#include	<timemng.h>
#include	<pccore.h>
#include	<calendar.h>


	_CALENDAR	cal;


static const UINT8 days[12] = {	31, 28, 31, 30, 31, 30,
								31, 31, 30, 31, 30, 31};

static int isLeapYear(int year)
{
	if (year % 400 == 0)
		return 1;
	if (year % 100 == 0)
		return 0;
	if (year % 4 == 0)
		return 1;
	return 0;
}

static int daysInMonth(int year, int month)
{
	if (month == 2 && isLeapYear(year))
	{
		return 29;
	}

	return days[month - 1];
}

static SINT64 convertDateTimeToSeconds(const _SYSTIME *dt)
{
	int y, m;
	SINT64 total_seconds;
	SINT64 total_days = 0;

	for (y = 0; y < dt->year; y++)
	{
		total_days += isLeapYear(y) ? 366 : 365;
	}

	for (m = 1; m < dt->month; m++)
	{
		total_days += daysInMonth(dt->year, m);
	}

	total_days += (dt->day - 1);

	total_seconds = total_days * 86400LL;
	total_seconds += dt->hour * 3600LL;
	total_seconds += dt->minute * 60LL;
	total_seconds += dt->second;

	return total_seconds;
}

static void convertSecondsToDateTime(SINT64 total_seconds, _SYSTIME *dt)
{
	SINT64 total_days = total_seconds / 86400;
	SINT64 remaining_seconds = total_seconds % 86400;

	// 時・分・秒の計算
	dt->hour = remaining_seconds / 3600;
	remaining_seconds %= 3600;

	dt->minute = remaining_seconds / 60;
	dt->second = remaining_seconds % 60;

	// 年の計算
	dt->year = 0;
	while (1)
	{
		int days_in_year = isLeapYear(dt->year) ? 366 : 365;
		if (total_days >= days_in_year)
		{
			total_days -= days_in_year;
			(dt->year)++;
		}
		else
		{
			break;
		}
	}

	// 月の計算
	dt->month = 1;
	while (1)
	{
		int dim = daysInMonth(dt->year, dt->month);
		if (total_days >= dim)
		{
			total_days -= dim;
			(dt->month)++;
		}
		else
		{
			break;
		}
	}

	// 日の計算（1日始まりに直す）
	dt->day = (int)total_days + 1;
}

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

static SINT64 calctimediff(const _SYSTIME *time1, const _SYSTIME *time2) {
	return (SINT64)(convertDateTimeToSeconds(time1) - convertDateTimeToSeconds(time2));
}
static void addtimediff(_SYSTIME *timebase, const SINT64 totalseconds) {
	convertSecondsToDateTime(convertDateTimeToSeconds(timebase) + totalseconds, timebase);
}

// -----

void calendar_initialize(void) {

	timemng_gettime(&cal.dt);
	addtimediff(&cal.dt, np2cfg.cal_vofs); // 仮想カレンダオフセット適用
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

	_SYSTIME	realc;

	date2deg(&cal.dt, bcd);

	if (!np2cfg.calendar)
	{
		timemng_gettime(&realc);
		np2cfg.cal_vofs = calctimediff(&cal.dt, &realc); // 仮想カレンダオフセット記憶
	}
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

void calendar_getdt(_SYSTIME* dt) {

	if (!np2cfg.calendar) {
		*dt = cal.dt;
	}
	else {
		if (cal.realchg) {
			cal.realchg = 0;
			timemng_gettime(&cal.realc);
		}
		*dt = cal.realc;
	}
}
