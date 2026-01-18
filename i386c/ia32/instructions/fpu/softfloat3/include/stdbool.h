/* stdbool.h‚ª‚È‚¢ŠÂ‹«—p */

#pragma once

#ifndef STDBOOL_H
#define STDBOOL_H

#if !defined(__cplusplus)

#if defined(__has_include)
	#if __has_include(<stdbool.h>)
		#include <stdbool.h>
	#else
		typedef int bool;
		#define true 1
		#define false 0
		#define __bool_true_false_are_defined 1
	#endif
#else
	typedef int bool;
	#define true 1
	#define false 0
	#define __bool_true_false_are_defined 1
#endif

#endif

#endif
