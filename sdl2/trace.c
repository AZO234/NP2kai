#include	"compiler.h"
#include	<stdarg.h>
#if defined(WIN32) && defined(OSLANG_EUC)
#include	"codecnv/codecnv.h"
#endif

#ifdef TRACE
void trace_init(void) {
}

void trace_term(void) {
}

void trace_fmt(const char *fmt, ...) {

	va_list	ap;
	char	buf[1024];
#if defined(WIN32) && defined(OSLANG_EUC)
	char	sjis[1024];
#endif

	va_start(ap, fmt);
	vsprintf(buf, fmt, ap);
	va_end(ap);
#if defined(WIN32) && defined(OSLANG_EUC)
	codecnv_euctosjis(sjis, NELEMENTS(sjis), buf, (UINT)-1);
	printf("%s\n", sjis);
#else
	printf("%s\n", buf);
#endif
}
#endif

