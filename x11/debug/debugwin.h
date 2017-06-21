#if defined(SUPPORT_MEMDBG32)

G_BEGIN_DECLS

void debugwin_create(void);
void debugwin_destroy(void);
void debugwin_process(void);

G_END_DECLS

#else

#define	debugwin_create()
#define	debugwin_destroy()
#define	debugwin_process()

#endif
