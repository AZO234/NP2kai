#include <compiler.h>
#include <stdarg.h>
#include <stdio.h>
#include "trace.h"

#ifdef TRACE

void trace_init(void) {
}

void trace_term(void) {
}

void trace_fmt(const char *fmt, ...) {
	va_list ap;
	char buf[1024];

	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	fprintf(stderr, "%s\n", buf);
}

#endif
