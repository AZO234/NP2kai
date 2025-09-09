#include	<compiler.h>
#include	<time.h>
#if !defined(_WIN32)
#include	<sys/time.h>
#include	<stdlib.h>
#endif
#include	<timemng.h>


BRESULT timemng_gettime(_SYSTIME *systime) {
#if defined(_WIN32)
	time_t microtime;
#else
	struct timeval microtime;
#endif
	struct tm *macrotime;

#if defined(_WIN32)
	time(&microtime);
	macrotime = localtime(&microtime);
#else
	gettimeofday(&microtime, NULL);
	macrotime = localtime(&microtime.tv_sec);
#endif
	if (macrotime != NULL) {
		systime->year = macrotime->tm_year + 1900;
		systime->month = macrotime->tm_mon + 1;
		systime->week = macrotime->tm_wday;
		systime->day = macrotime->tm_mday;
		systime->hour = macrotime->tm_hour;
		systime->minute = macrotime->tm_min;
		systime->second = macrotime->tm_sec;
#if defined(_WIN32)
		systime->milli = 0;
#else
		systime->milli = microtime.tv_usec / 1000;
#endif
		return(SUCCESS);
	}
	else {
		return(FAILURE);
	}
}

