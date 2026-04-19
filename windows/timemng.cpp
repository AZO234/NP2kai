#include	"compiler.h"
#include	"timemng.h"


BRESULT timemng_gettime(_SYSTIME *systime) {

	GetLocalTime((SYSTEMTIME *)systime);
	return(SUCCESS);
}

