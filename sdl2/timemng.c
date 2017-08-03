#include	"compiler.h"
#include	<time.h>
#include	"timemng.h"


BRESULT timemng_gettime(_SYSTIME *systime) {

	struct timeval microtime;
	struct tm *macrotime;

	gettimeofday(&microtime, NULL);
	macrotime = localtime(&microtime.tv_sec);
	if (macrotime != NULL) {
		systime->year = macrotime->tm_year + 1900;
		systime->month = macrotime->tm_mon + 1;
		systime->week = macrotime->tm_wday;
		systime->day = macrotime->tm_mday;
		systime->hour = macrotime->tm_hour;
		systime->minute = macrotime->tm_min;
		systime->second = macrotime->tm_sec;
		systime->milli = microtime.tv_usec / 1000;
		return(SUCCESS);
	}
	else {
		return(FAILURE);
	}
}

