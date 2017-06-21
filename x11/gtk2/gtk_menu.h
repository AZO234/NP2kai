/*
 * Copyright (c) 2002-2003 NONAKA Kimihiro
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

#ifndef	NP2_GTK2_GTKMENU_H__
#define	NP2_GTK2_GTKMENU_H__

#include "gtk2/xnp2.h"

G_BEGIN_DECLS

GtkWidget *create_menu(void);
void xmenu_hide(void);
void xmenu_show(void);

void create_about_dialog(void);
void create_calendar_dialog(void);
void create_configure_dialog(void);
void create_midi_dialog(void);
void create_screen_dialog(void);
void create_sound_dialog(void);
void create_newdisk_fd_dialog(const gchar *filename);
void create_newdisk_hd_dialog(const gchar *filename, int kind);

typedef struct {
	GtkActionGroup	*action_group;
	GtkUIManager	*ui_manager;
} _MENU_HDL, *MENU_HDL;

void xmenu_toggle_item(MENU_HDL, const char *, BOOL);
void xmenu_toggle_menu(void);
void xmenu_select_screen(UINT8 mode);

G_END_DECLS

#endif	/* NP2_GTK2_GTKMENU_H__ */
