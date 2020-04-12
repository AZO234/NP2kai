#ifndef WIN_HEADERS_H
#define WIN_HEADERS_H

#define STRICT
#define WIN32_LEAN_AND_MEAN

#include "compiler.h"
#include <cassert>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _MSC_VER
	#undef max
	#define max _MAX
	#undef min
	#define min _MIN
#endif

#ifdef __cplusplus
}
#endif

#endif	// WIN_HEADERS_H
