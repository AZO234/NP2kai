
#ifndef TRACE

#define	TRACEINIT()
#define	TRACETERM()
#define	TRACEOUT(a)
#define	VERBOSE(a)

#else

#ifdef	__cplusplus
extern "C" {
#endif

void trace_init(void);
void trace_term(void);
void trace_fmt(const char *str, ...);

#define	TRACEINIT()		trace_init()
#define	TRACETERM()		trace_term()
#define	TRACEOUT(arg)	trace_fmt arg
#define	VERBOSE(arg)	trace_fmt arg

#ifdef	__cplusplus
};
#endif

#endif

