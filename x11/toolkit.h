/*
 * Copyright (c) 2003 NONAKA Kimihiro
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef	NP2_X11_TOOLKIT_H__
#define	NP2_X11_TOOLKIT_H__

/* for toolkit_msgbox() */
#define	TK_MB_OK		(1U <<  0)
#define	TK_MB_CANCEL		(1U <<  1)
#define	TK_MB_YES		(1U <<  2)
#define	TK_MB_NO		(1U <<  3)
#define	TK_MB_OKCANCEL		(TK_MB_OK|TK_MB_CANCEL)
#define	TK_MB_YESNO		(TK_MB_YES|TK_MB_NO)
#define	TK_MB_BTN_MASK		(TK_MB_OK|TK_MB_CANCEL|TK_MB_YESNO)
#define	TK_MB_ICON_INFO		(1U << 16)
#define	TK_MB_ICON_WARNING	(1U << 17)
#define	TK_MB_ICON_ERROR	(1U << 18)
#define	TK_MB_ICON_QUESTION	(1U << 19)
#define	TK_MB_ICON_MASK		(TK_MB_ICON_INFO \
				 |TK_MB_ICON_WARNING \
				 |TK_MB_ICON_ERROR \
				 |TK_MB_ICON_QUESTION)

#include "gtk2/gtk_toolkit.h"

#define	toolkit_initialize()
#define	toolkit_terminate()
#define	toolkit_arginit(argcp, argvp)	gui_gtk_arginit(argcp, argvp)
#define	toolkit_widget_create()		gui_gtk_widget_create()
#define	toolkit_widget_show()		gui_gtk_widget_show()
#define	toolkit_widget_mainloop()	gui_gtk_widget_mainloop()
#define	toolkit_widget_quit()		gui_gtk_widget_quit()
#define	toolkit_event_process()		gui_gtk_event_process()
#define	toolkit_set_window_title(s)	gui_gtk_set_window_title(s)
#define	toolkit_messagebox(t,m)		gui_gtk_msgbox(t,m,TK_MB_OK|TK_MB_ICON_INFO)
#define	toolkit_msgbox(t,m,f)		gui_gtk_msgbox(t,m,f)

#endif	/* NP2_X11_TOOLKIT_H__ */
