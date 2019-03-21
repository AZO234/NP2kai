#include	"compiler.h"
#include	<stdarg.h>


int trace_flag = 0;

void trace_init(void) {
}

void trace_term(void) {
}

void trace_fmt(const char *fmt, ...) {

	va_list	ap;
	gchar buf[1024];

	va_start(ap, fmt);
	g_vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	g_printerr("%s\n", buf);
}
