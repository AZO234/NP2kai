#include	"compiler.h"

#include	<time.h>

#include	"timemng.h"

BRESULT
timemng_gettime(_SYSTIME *systime)
{
	struct tm *now_time;
	time_t long_time;

	time(&long_time);
	now_time = localtime(&long_time);
	if (now_time != NULL) {
		systime->year = now_time->tm_year + 1900;
		systime->month = now_time->tm_mon + 1;
		systime->week = now_time->tm_wday;
		systime->day = now_time->tm_mday;
		systime->hour = now_time->tm_hour;
		systime->minute = now_time->tm_min;
		systime->second = now_time->tm_sec;
		systime->milli = 0;

		return SUCCESS;
	}
	return FAILURE;
}
