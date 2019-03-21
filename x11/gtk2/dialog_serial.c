/*
 * Copyright (c) 2002-2004 NONAKA Kimihiro
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

#include "compiler.h"

#include "gtk2/xnp2.h"
#include "gtk2/gtk_menu.h"

#include "np2.h"
#include "dosio.h"
#include "ini.h"
#include "opngen.h"
#include "pccore.h"
#include "iocore.h"

#include "sysmng.h"


static const char *ch1_int_speed_str[] = {
	"Disable", "75", "150", "300", "600", "1200", "2400", "4800", "9600"
};

static const char *ch23_int_speed_str[] = {
	"75", "150", "300", "600", "1200", "2400", "4800", "9600", "19200"
};

static const char *ch1_int_stopbit_str[] = {
	"1", "1.5", "2"
};

static const char *ch1_int_parity_str[] = {
	"None", "Odd", "Even"
};

static const char *ch1_int_datasize_str[] = {
	"7", "8"
};

static const char *ch1_int_duplex_str[] = {
	"Full", "Half"
};

static const char *ch1_int_flowctrl_str[] = {
	"Disable", "Enable"
};

static const char *ch1_int_7bitkana_str[] = {
	"Disable", "Enable"
};

static const char *ch1_int_sendnl_str[] = {
	"CR", "CR+LF"
};

static const char *ch1_int_rcvnl_str[] = {
	"CR", "CR+LF"
};

static const char *ch1_int_jscode_str[] = {
	"KI:1B4Bh KO:1B48h", "KI:1A70h KO:1A71h"
};

static const char *ch1_int_rcvdel_str[] = {
	"Process", "Ignore"
};

static const char *ch2_int_int_str[] = {
	"0", "1", "2", "3"
};

static const char *ch3_int_int_str[] = {
	"0", "4", "5", "6"
};

static const char *ch23_int_mode_str[] = {
	"Sync(ST1)", "Sync(ST2)", "Async(TimeCount)", "Async(StartStop)"
};

static const char *ext_speed_str[] = {
	"110", "300", "1200", "2400", "4800", "9600", "19200", "38400", "57600", "115200"
};

static const char *ext_datasize_str[] = {
	"5", "6", "7", "8"
};

static const char *ext_parity_str[] = {
	"None", "Odd", "Even"
};

static const char *ext_stopbit_str[] = {
	"1", "1.5", "2"
};


static GtkWidget *ch1_int_speed_entry;
static GtkWidget *ch1_int_stopbit_entry;
static GtkWidget *ch1_int_parity_entry;
static GtkWidget *ch1_int_datasize_entry;
static GtkWidget *ch1_int_duplex_entry;
static GtkWidget *ch1_int_flowctrl_entry;
static GtkWidget *ch1_int_7bitkana_entry;
static GtkWidget *ch1_int_sendnl_entry;
static GtkWidget *ch1_int_rcvnl_entry;
static GtkWidget *ch1_int_jscode_entry;
static GtkWidget *ch1_int_rcvdel_entry;
static GtkWidget *ch1_ext_mout_entry;
static GtkWidget *ch1_ext_speed_combo;
static GtkWidget *ch1_ext_speed_entry;
static GtkWidget *ch1_ext_direct_checkbutton;
static GtkWidget *ch1_ext_datasize_combo;
static GtkWidget *ch1_ext_datasize_entry;
static GtkWidget *ch1_ext_parity_combo;
static GtkWidget *ch1_ext_parity_entry;
static GtkWidget *ch1_ext_stopbit_combo;
static GtkWidget *ch1_ext_stopbit_entry;
static GtkWidget *ch2_int_enable_checkbutton;
static GtkWidget *ch2_int_speed_entry;
static GtkWidget *ch2_int_int_entry;
static GtkWidget *ch2_int_mode_entry;
static GtkWidget *ch3_int_speed_entry;
static GtkWidget *ch3_int_int_entry;
static GtkWidget *ch3_int_mode_entry;
static GtkWidget *ch2_ext_mout_entry;
static GtkWidget *ch2_ext_direct_checkbutton;
static GtkWidget *ch2_ext_speed_combo;
static GtkWidget *ch2_ext_speed_entry;
static GtkWidget *ch2_ext_datasize_combo;
static GtkWidget *ch2_ext_datasize_entry;
static GtkWidget *ch2_ext_parity_combo;
static GtkWidget *ch2_ext_parity_entry;
static GtkWidget *ch2_ext_stopbit_combo;
static GtkWidget *ch2_ext_stopbit_entry;
static GtkWidget *ch3_ext_mout_entry;
static GtkWidget *ch3_ext_direct_checkbutton;
static GtkWidget *ch3_ext_speed_combo;
static GtkWidget *ch3_ext_speed_entry;
static GtkWidget *ch3_ext_datasize_combo;
static GtkWidget *ch3_ext_datasize_entry;
static GtkWidget *ch3_ext_parity_combo;
static GtkWidget *ch3_ext_parity_entry;
static GtkWidget *ch3_ext_stopbit_combo;
static GtkWidget *ch3_ext_stopbit_entry;


static void
ok_button_clicked(GtkButton *b, gpointer d)
{
	const gchar *text;
	UINT update = 0;
	int i, j;

	/* Ch.1 Speed */
	text = gtk_entry_get_text(GTK_ENTRY(ch1_int_speed_entry));
	for (i = 0; i < NELEMENTS(ch1_int_speed_str); i++) {
		if (strcmp(text, ch1_int_speed_str[i]) == 0) {
			if((np2cfg.memsw[1] & 0xF) != i) {
				np2cfg.memsw[1] &= ~0xF;
				np2cfg.memsw[1] |= i;
				update |= SYS_UPDATECFG;
			}
			break;
		}
	}

	/* Ch.1 DataSize */
	text = gtk_entry_get_text(GTK_ENTRY(ch1_int_datasize_entry));
	for (i = 0; i < NELEMENTS(ch1_int_datasize_str); i++) {
		if (strcmp(text, ch1_int_datasize_str[i]) == 0) {
			if((np2cfg.memsw[0] & 0xC) != (i << 2) + 2) {
				np2cfg.memsw[0] &= ~0xC;
				np2cfg.memsw[0] |= (i << 2) + 2;
				update |= SYS_UPDATECFG;
			}
			break;
		}
	}

	/* Ch.1 Parity */
	text = gtk_entry_get_text(GTK_ENTRY(ch1_int_parity_entry));
	for (i = 0; i < NELEMENTS(ch1_int_parity_str); i++) {
		if (strcmp(text, ch1_int_parity_str[i]) == 0) {
			switch(i) {
			case 0:
				j = 0;
				break;
			case 1:
				j = 1;
				break;
			case 2:
				j = 3;
				break;
			}
			if((np2cfg.memsw[0] & 0x30) != j << 4) {
				np2cfg.memsw[0] &= ~0x30;
				np2cfg.memsw[0] |= j << 4;
				update |= SYS_UPDATECFG;
			}
			break;
		}
	}

	/* Ch.1 StopBit */
	text = gtk_entry_get_text(GTK_ENTRY(ch1_int_stopbit_entry));
	for (i = 0; i < NELEMENTS(ch1_int_stopbit_str); i++) {
		if (strcmp(text, ch1_int_stopbit_str[i]) == 0) {
			if((np2cfg.memsw[0] & 0xC0) != (i << 6) + 1) {
				np2cfg.memsw[0] &= ~0xC0;
				np2cfg.memsw[0] |= (i << 6) + 1;
				update |= SYS_UPDATECFG;
			}
			break;
		}
	}

	/* Ch.1 FlowCtrl */
	text = gtk_entry_get_text(GTK_ENTRY(ch1_int_flowctrl_entry));
	for (i = 0; i < NELEMENTS(ch1_int_flowctrl_str); i++) {
		if (strcmp(text, ch1_int_flowctrl_str[i]) == 0) {
			if((np2cfg.memsw[0] & 0x1) != i) {
				np2cfg.memsw[0] &= ~0x1;
				np2cfg.memsw[0] |= i;
				update |= SYS_UPDATECFG;
			}
			break;
		}
	}

	/* Ch.1 Duplex */
	text = gtk_entry_get_text(GTK_ENTRY(ch1_int_duplex_entry));
	for (i = 0; i < NELEMENTS(ch1_int_duplex_str); i++) {
		if (strcmp(text, ch1_int_duplex_str[i]) == 0) {
			if((np2cfg.memsw[0] & 0x2) != i << 1) {
				np2cfg.memsw[0] &= ~0x2;
				np2cfg.memsw[0] |= i << 1;
				update |= SYS_UPDATECFG;
			}
			break;
		}
	}

	/* Ch.1 7bitKana */
	text = gtk_entry_get_text(GTK_ENTRY(ch1_int_7bitkana_entry));
	for (i = 0; i < NELEMENTS(ch1_int_7bitkana_str); i++) {
		if (strcmp(text, ch1_int_7bitkana_str[i]) == 0) {
			if((np2cfg.memsw[1] & 0x80) != i << 7) {
				np2cfg.memsw[1] &= ~0x80;
				np2cfg.memsw[1] |= i << 7;
				update |= SYS_UPDATECFG;
			}
			break;
		}
	}

	/* Ch.1 SendNl */
	text = gtk_entry_get_text(GTK_ENTRY(ch1_int_sendnl_entry));
	for (i = 0; i < NELEMENTS(ch1_int_sendnl_str); i++) {
		if (strcmp(text, ch1_int_sendnl_str[i]) == 0) {
			if((np2cfg.memsw[1] & 0x40) != i << 6) {
				np2cfg.memsw[1] &= ~0x40;
				np2cfg.memsw[1] |= i << 6;
				update |= SYS_UPDATECFG;
			}
			break;
		}
	}

	/* Ch.1 RcvNl */
	text = gtk_entry_get_text(GTK_ENTRY(ch1_int_rcvnl_entry));
	for (i = 0; i < NELEMENTS(ch1_int_rcvnl_str); i++) {
		if (strcmp(text, ch1_int_rcvnl_str[i]) == 0) {
			if((np2cfg.memsw[1] & 0x20) != i << 5) {
				np2cfg.memsw[1] &= ~0x20;
				np2cfg.memsw[1] |= i << 5;
				update |= SYS_UPDATECFG;
			}
			break;
		}
	}

	/* Ch.1 JSCode */
	text = gtk_entry_get_text(GTK_ENTRY(ch1_int_jscode_entry));
	for (i = 0; i < NELEMENTS(ch1_int_jscode_str); i++) {
		if (strcmp(text, ch1_int_jscode_str[i]) == 0) {
			if((np2cfg.memsw[1] & 0x10) != i << 4) {
				np2cfg.memsw[1] &= ~0x10;
				np2cfg.memsw[1] |= i << 4;
				update |= SYS_UPDATECFG;
			}
			break;
		}
	}

	/* Ch.1 RcvDel */
	text = gtk_entry_get_text(GTK_ENTRY(ch1_int_rcvdel_entry));
	for (i = 0; i < NELEMENTS(ch1_int_rcvdel_str); i++) {
		if (strcmp(text, ch1_int_rcvdel_str[i]) == 0) {
			if((np2cfg.memsw[2] & 0x80) != i << 7) {
				np2cfg.memsw[2] &= ~0x80;
				np2cfg.memsw[2] |= i << 7;
				update |= SYS_UPDATECFG;
			}
			break;
		}
	}

	/* Ch.1 MOut */
	text = gtk_entry_get_text(GTK_ENTRY(ch1_ext_mout_entry));
	if (strcmp(text, np2oscfg.com[0].mout) != 0) {
		milstr_ncpy(np2oscfg.com[0].mout, text, sizeof(np2oscfg.com[0].mout));
		update |= SYS_UPDATEOSCFG;
	}

	/* Ch.1 Direct */
	i = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ch1_ext_direct_checkbutton)) ? 1 : 0;
	if(np2oscfg.com[0].direct != i) {
		np2oscfg.com[0].direct = i;
		update |= SYS_UPDATEOSCFG;
	}

	/* Ch.1 Speed */
	text = gtk_entry_get_text(GTK_ENTRY(ch1_ext_speed_entry));
	i = atoi(text);
	if (np2oscfg.com[0].speed != i) {
		np2oscfg.com[0].speed = i;
		update |= SYS_UPDATEOSCFG;
	}
	
	/* Ch.1 DataSize */
	text = gtk_entry_get_text(GTK_ENTRY(ch1_ext_datasize_entry));
	for (i = 0; i < NELEMENTS(ext_datasize_str); i++) {
		if (strcmp(text, ext_datasize_str[i]) == 0) {
			if(((np2oscfg.com[0].param >> 2) & 3) != i) {
				np2oscfg.com[0].param &= ~0xC;
				np2oscfg.com[0].param |= i << 2;
				update |= SYS_UPDATEOSCFG;
			}
			break;
		}
	}

	/* Ch.1 Parity */
	text = gtk_entry_get_text(GTK_ENTRY(ch1_ext_parity_entry));
	for (i = 0; i < NELEMENTS(ext_parity_str); i++) {
		if (strcmp(text, ext_parity_str[i]) == 0) {
			switch(i) {
			case 1:
				if((np2oscfg.com[0].param & 0x30) != 0x10) {
					np2oscfg.com[0].param &= ~0x30;
					np2oscfg.com[0].param |= 0x10;
					update |= SYS_UPDATEOSCFG;
				}
				break;
			case 2:
				if((np2oscfg.com[0].param & 0x30) != 0x30) {
					np2oscfg.com[0].param |= 0x30;
					update |= SYS_UPDATEOSCFG;
				}
				break;
			default:
				if((np2oscfg.com[0].param & 0x30) != 0) {
					np2oscfg.com[0].param &= ~0x30;
					update |= SYS_UPDATEOSCFG;
				}
				break;
			}
			break;
		}
	}

	/* Ch.1 StopBit */
	text = gtk_entry_get_text(GTK_ENTRY(ch1_ext_stopbit_entry));
	for (i = 0; i < NELEMENTS(ext_stopbit_str); i++) {
		if (strcmp(text, ext_stopbit_str[i]) == 0) {
			switch(i) {
			case 1:
				if((np2oscfg.com[0].param & 0xC0) != 0x80) {
					np2oscfg.com[0].param &= ~0xC0;
					np2oscfg.com[0].param |= 0x80;
					update |= SYS_UPDATEOSCFG;
				}
				break;
			case 2:
				if((np2oscfg.com[0].param & 0xC0) != 0xC0) {
					np2oscfg.com[0].param |= 0xC0;
					update |= SYS_UPDATEOSCFG;
				}
				break;
			default:
				if((np2oscfg.com[0].param & 0xC0) != 0) {
					np2oscfg.com[0].param &= ~0xC0;
					update |= SYS_UPDATEOSCFG;
				}
				break;
			}
			break;
		}
	}

	/* Enable */
	i = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ch2_int_enable_checkbutton)) ? 1 : 0;
	if(np2cfg.pc9861enable != i) {
		np2cfg.pc9861enable = i;
		update |= SYS_UPDATECFG;
	}

	/* Ch.2 Mode */
	text = gtk_entry_get_text(GTK_ENTRY(ch2_int_mode_entry));
	for (i = 0; i < NELEMENTS(ch23_int_mode_str); i++) {
		if (strcmp(text, ch23_int_mode_str[i]) == 0) {
			if((np2cfg.pc9861sw[0] & 3) != i) {
				np2cfg.pc9861sw[0] &= ~3;
				np2cfg.pc9861sw[0] |= i;
				update |= SYS_UPDATECFG;
			}
			break;
		}
	}

	/* Ch.2 Speed */
	text = gtk_entry_get_text(GTK_ENTRY(ch2_int_speed_entry));
	for (i = 0; i < NELEMENTS(ch23_int_speed_str); i++) {
		if (strcmp(text, ch23_int_speed_str[i]) == 0) {
			if(!(np2cfg.pc9861sw[0] & 0x2)) {	// Sync
				j = 15 - i;
				if(j < 8) {
					j = 8;
				}
			} else {	// Async
				j = 12 - i;
			}
			if(((np2cfg.pc9861sw[0] >> 2) & 0xF) != j) {
				np2cfg.pc9861sw[0] &= ~0x3C;
				np2cfg.pc9861sw[0] |= j << 2;
				update |= SYS_UPDATECFG;
			}
			break;
		}
	}

	/* Ch.2 INT */
	text = gtk_entry_get_text(GTK_ENTRY(ch2_int_int_entry));
	for (i = 0; i < NELEMENTS(ch2_int_int_str); i++) {
		if (strcmp(text, ch2_int_int_str[i]) == 0) {
			switch(i) {
			case 0:
				j = 0;
				break;
			case 1:
				j = 2;
				break;
			case 2:
				j = 1;
				break;
			case 3:
				j = 3;
				break;
			}
			if((np2cfg.pc9861sw[1] & 3) != j) {
				np2cfg.pc9861sw[1] &= ~3;
				np2cfg.pc9861sw[1] |= j;
				update |= SYS_UPDATECFG;
			}
			break;
		}
	}

	/* Ch.3 Mode */
	text = gtk_entry_get_text(GTK_ENTRY(ch3_int_mode_entry));
	for (i = 0; i < NELEMENTS(ch23_int_mode_str); i++) {
		if (strcmp(text, ch23_int_mode_str[i]) == 0) {
			if((np2cfg.pc9861sw[2] & 3) != i) {
				np2cfg.pc9861sw[2] &= ~3;
				np2cfg.pc9861sw[2] |= i;
				update |= SYS_UPDATECFG;
			}
			break;
		}
	}

	/* Ch.3 Speed */
	text = gtk_entry_get_text(GTK_ENTRY(ch3_int_speed_entry));
	for (i = 0; i < NELEMENTS(ch23_int_speed_str); i++) {
		if (strcmp(text, ch23_int_speed_str[i]) == 0) {
			if(!(np2cfg.pc9861sw[0] & 0x2)) {	// Sync
				j = 15 - i;
				if(j < 8) {
					j = 8;
				}
			} else {	// Async
				j = 12 - i;
			}
			if(((np2cfg.pc9861sw[2] >> 2) & 0xF) != j) {
				np2cfg.pc9861sw[2] &= ~0x3C;
				np2cfg.pc9861sw[2] |= j << 2;
				update |= SYS_UPDATECFG;
			}
			break;
		}
	}

	/* Ch.3 INT */
	text = gtk_entry_get_text(GTK_ENTRY(ch3_int_int_entry));
	for (i = 0; i < NELEMENTS(ch3_int_int_str); i++) {
		if (strcmp(text, ch3_int_int_str[i]) == 0) {
			switch(i) {
			case 0:
				j = 0;
				break;
			case 1:
				j = 2;
				break;
			case 2:
				j = 1;
				break;
			case 3:
				j = 3;
				break;
			}
			if((np2cfg.pc9861sw[1] & 0xC) >> 2 != j) {
				np2cfg.pc9861sw[1] &= ~0xC;
				np2cfg.pc9861sw[1] |= j << 2;
				update |= SYS_UPDATECFG;
			}
			break;
		}
	}

	/* Ch.2 MOut */
	text = gtk_entry_get_text(GTK_ENTRY(ch2_ext_mout_entry));
	if (strcmp(text, np2oscfg.com[1].mout) != 0) {
		milstr_ncpy(np2oscfg.com[1].mout, text, sizeof(np2oscfg.com[1].mout));
		update |= SYS_UPDATEOSCFG;
	}

	/* Ch.2 Direct */
	i = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ch2_ext_direct_checkbutton)) ? 1 : 0;
	if(np2oscfg.com[1].direct != i) {
		np2oscfg.com[1].direct = i;
		update |= SYS_UPDATEOSCFG;
	}

	/* Ch.2 Speed */
	text = gtk_entry_get_text(GTK_ENTRY(ch2_ext_speed_entry));
	i = atoi(text);
	if (np2oscfg.com[1].speed != i) {
		np2oscfg.com[1].speed = i;
		update |= SYS_UPDATEOSCFG;
	}
	
	/* Ch.2 DataSize */
	text = gtk_entry_get_text(GTK_ENTRY(ch2_ext_datasize_entry));
	for (i = 0; i < NELEMENTS(ext_datasize_str); i++) {
		if (strcmp(text, ext_datasize_str[i]) == 0) {
			if(((np2oscfg.com[1].param >> 2) & 3) != i) {
				np2oscfg.com[1].param &= ~0xC;
				np2oscfg.com[1].param |= i << 2;
				update |= SYS_UPDATEOSCFG;
			}
			break;
		}
	}

	/* Ch.2 Parity */
	text = gtk_entry_get_text(GTK_ENTRY(ch2_ext_parity_entry));
	for (i = 0; i < NELEMENTS(ext_parity_str); i++) {
		if (strcmp(text, ext_parity_str[i]) == 0) {
			switch(i) {
			case 1:
				if((np2oscfg.com[1].param & 0x30) != 0x10) {
					np2oscfg.com[1].param &= ~0x30;
					np2oscfg.com[1].param |= 0x10;
					update |= SYS_UPDATEOSCFG;
				}
				break;
			case 2:
				if((np2oscfg.com[1].param & 0x30) != 0x30) {
					np2oscfg.com[1].param |= 0x30;
					update |= SYS_UPDATEOSCFG;
				}
				break;
			default:
				if((np2oscfg.com[1].param & 0x30) != 0) {
					np2oscfg.com[1].param &= ~0x30;
					update |= SYS_UPDATEOSCFG;
				}
				break;
			}
			break;
		}
	}

	/* Ch.2 StopBit */
	text = gtk_entry_get_text(GTK_ENTRY(ch2_ext_stopbit_entry));
	for (i = 0; i < NELEMENTS(ext_stopbit_str); i++) {
		if (strcmp(text, ext_stopbit_str[i]) == 0) {
			switch(i) {
			case 1:
				if((np2oscfg.com[1].param & 0xC0) != 0x80) {
					np2oscfg.com[1].param &= ~0xC0;
					np2oscfg.com[1].param |= 0x80;
					update |= SYS_UPDATEOSCFG;
				}
				break;
			case 2:
				if((np2oscfg.com[1].param & 0xC0) != 0xC0) {
					np2oscfg.com[1].param |= 0xC0;
					update |= SYS_UPDATEOSCFG;
				}
				break;
			default:
				if((np2oscfg.com[1].param & 0xC0) != 0) {
					np2oscfg.com[1].param &= ~0xC0;
					update |= SYS_UPDATEOSCFG;
				}
				break;
			}
			break;
		}
	}

	/* Ch.3 MOut */
	text = gtk_entry_get_text(GTK_ENTRY(ch3_ext_mout_entry));
	if (strcmp(text, np2oscfg.com[2].mout) != 0) {
		milstr_ncpy(np2oscfg.com[2].mout, text, sizeof(np2oscfg.com[2].mout));
		update |= SYS_UPDATEOSCFG;
	}

	/* Ch.3 Direct */
	i = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ch3_ext_direct_checkbutton)) ? 1 : 0;
	if(np2oscfg.com[2].direct != i) {
		np2oscfg.com[2].direct = i;
		update |= SYS_UPDATEOSCFG;
	}

	/* Ch.3 Speed */
	text = gtk_entry_get_text(GTK_ENTRY(ch2_ext_speed_entry));
	i = atoi(text);
	if (np2oscfg.com[2].speed != i) {
		np2oscfg.com[2].speed = i;
		update |= SYS_UPDATEOSCFG;
	}

	/* Ch.3 DataSize */
	text = gtk_entry_get_text(GTK_ENTRY(ch3_ext_datasize_entry));
	for (i = 0; i < NELEMENTS(ext_datasize_str); i++) {
		if (strcmp(text, ext_datasize_str[i]) == 0) {
			if(((np2oscfg.com[2].param >> 2) & 3) != i) {
				np2oscfg.com[2].param &= ~0xC;
				np2oscfg.com[2].param |= i << 2;
				update |= SYS_UPDATEOSCFG;
			}
			break;
		}
	}

	/* Ch.3 Parity */
	text = gtk_entry_get_text(GTK_ENTRY(ch3_ext_parity_entry));
	for (i = 0; i < NELEMENTS(ext_parity_str); i++) {
		if (strcmp(text, ext_parity_str[i]) == 0) {
			switch(i) {
			case 1:
				if((np2oscfg.com[2].param & 0x30) != 0x10) {
					np2oscfg.com[2].param &= ~0x30;
					np2oscfg.com[2].param |= 0x10;
					update |= SYS_UPDATEOSCFG;
				}
				break;
			case 2:
				if((np2oscfg.com[2].param & 0x30) != 0x30) {
					np2oscfg.com[2].param |= 0x30;
					update |= SYS_UPDATEOSCFG;
				}
				break;
			default:
				if((np2oscfg.com[2].param & 0x30) != 0) {
					np2oscfg.com[2].param &= ~0x30;
					update |= SYS_UPDATEOSCFG;
				}
				break;
			}
			break;
		}
	}

	/* Ch.3 StopBit */
	text = gtk_entry_get_text(GTK_ENTRY(ch3_ext_stopbit_entry));
	for (i = 0; i < NELEMENTS(ext_stopbit_str); i++) {
		if (strcmp(text, ext_stopbit_str[i]) == 0) {
			switch(i) {
			case 1:
				if((np2oscfg.com[2].param & 0xC0) != 0x80) {
					np2oscfg.com[2].param &= ~0xC0;
					np2oscfg.com[2].param |= 0x80;
					update |= SYS_UPDATEOSCFG;
				}
				break;
			case 2:
				if((np2oscfg.com[2].param & 0xC0) != 0xC0) {
					np2oscfg.com[2].param |= 0xC0;
					update |= SYS_UPDATEOSCFG;
				}
				break;
			default:
				if((np2oscfg.com[2].param & 0xC0) != 0) {
					np2oscfg.com[2].param &= ~0xC0;
					update |= SYS_UPDATEOSCFG;
				}
				break;
			}
			break;
		}
	}

	if (update) {
		sysmng_update(update);
	}

	gtk_widget_destroy((GtkWidget *)d);
}

static void
ch1_ext_direct_checkbutton_clicked(GtkButton *b, gpointer d)
{
	int i;

	i = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ch1_ext_direct_checkbutton)) ? 0 : 1;
	gtk_widget_set_sensitive(ch1_ext_speed_combo, i);
	gtk_widget_set_sensitive(ch1_ext_datasize_combo, i);
	gtk_widget_set_sensitive(ch1_ext_parity_combo, i);
	gtk_widget_set_sensitive(ch1_ext_stopbit_combo, i);
}

static void
ch2_ext_direct_checkbutton_clicked(GtkButton *b, gpointer d)
{
	int i;

	i = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ch2_ext_direct_checkbutton)) ? 0 : 1;
	gtk_widget_set_sensitive(ch2_ext_speed_combo, i);
	gtk_widget_set_sensitive(ch2_ext_datasize_combo, i);
	gtk_widget_set_sensitive(ch2_ext_parity_combo, i);
	gtk_widget_set_sensitive(ch2_ext_stopbit_combo, i);
}

static void
ch3_ext_direct_checkbutton_clicked(GtkButton *b, gpointer d)
{
	int i;

	i = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ch3_ext_direct_checkbutton)) ? 0 : 1;
	gtk_widget_set_sensitive(ch3_ext_speed_combo, i);
	gtk_widget_set_sensitive(ch3_ext_datasize_combo, i);
	gtk_widget_set_sensitive(ch3_ext_parity_combo, i);
	gtk_widget_set_sensitive(ch3_ext_stopbit_combo, i);
}

static void
dialog_destroy(GtkWidget *w, GtkWidget **wp)
{

	install_idle_process();
	gtk_widget_destroy(w);
}

static GtkWidget *
create_ch1_note(void)
{
	GtkWidget *root_widget;
	GtkWidget *table;
	GtkWidget *internal_frame;
	GtkWidget *internal_frame_vbox;
	GtkWidget *ch1_int_speed_hbox;
	GtkWidget *ch1_int_speed_label;
	GtkWidget *ch1_int_speed_combo;
	GtkWidget *ch1_int_stopbit_hbox;
	GtkWidget *ch1_int_stopbit_label;
	GtkWidget *ch1_int_stopbit_combo;
	GtkWidget *ch1_int_parity_hbox;
	GtkWidget *ch1_int_parity_label;
	GtkWidget *ch1_int_parity_combo;
	GtkWidget *ch1_int_datasize_hbox;
	GtkWidget *ch1_int_datasize_label;
	GtkWidget *ch1_int_datasize_combo;
	GtkWidget *ch1_int_flowctrl_hbox;
	GtkWidget *ch1_int_flowctrl_label;
	GtkWidget *ch1_int_flowctrl_combo;
	GtkWidget *ch1_int_duplex_hbox;
	GtkWidget *ch1_int_duplex_label;
	GtkWidget *ch1_int_duplex_combo;
	GtkWidget *ch1_int_7bitkana_hbox;
	GtkWidget *ch1_int_7bitkana_label;
	GtkWidget *ch1_int_7bitkana_combo;
	GtkWidget *ch1_int_sendnl_hbox;
	GtkWidget *ch1_int_sendnl_label;
	GtkWidget *ch1_int_sendnl_combo;
	GtkWidget *ch1_int_rcvnl_hbox;
	GtkWidget *ch1_int_rcvnl_label;
	GtkWidget *ch1_int_rcvnl_combo;
	GtkWidget *ch1_int_jscode_hbox;
	GtkWidget *ch1_int_jscode_label;
	GtkWidget *ch1_int_jscode_combo;
	GtkWidget *ch1_int_rcvdel_hbox;
	GtkWidget *ch1_int_rcvdel_label;
	GtkWidget *ch1_int_rcvdel_combo;
	GtkWidget *external_frame;
	GtkWidget *external_frame_vbox;
	GtkWidget *ch1_ext_mout_hbox;
	GtkWidget *ch1_ext_mout_label;
	GtkWidget *ch1_ext_speed_hbox;
	GtkWidget *ch1_ext_speed_label;
	GtkWidget *ch1_ext_datasize_hbox;
	GtkWidget *ch1_ext_datasize_label;
	GtkWidget *ch1_ext_parity_hbox;
	GtkWidget *ch1_ext_parity_label;
	GtkWidget *ch1_ext_stopbit_hbox;
	GtkWidget *ch1_ext_stopbit_label;
	int i;
	char str[32];

	root_widget = gtk_hbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(root_widget), 5);
	gtk_widget_show(root_widget);

	internal_frame = gtk_frame_new("Internal");
	gtk_container_set_border_width(GTK_CONTAINER(internal_frame), 2);
	gtk_widget_show(internal_frame);
	gtk_container_add(GTK_CONTAINER(root_widget), internal_frame);

	internal_frame_vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(internal_frame_vbox);
	gtk_container_add(GTK_CONTAINER(internal_frame), internal_frame_vbox);

	/* Ch.1 Speed */
	ch1_int_speed_hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(ch1_int_speed_hbox), 5);
	gtk_widget_show(ch1_int_speed_hbox);
	gtk_box_pack_start(GTK_BOX(internal_frame_vbox), ch1_int_speed_hbox, FALSE, FALSE, 0);

	ch1_int_speed_label = gtk_label_new("Speed");
	gtk_widget_show(ch1_int_speed_label);
	gtk_container_add(GTK_CONTAINER(ch1_int_speed_hbox), ch1_int_speed_label);

	ch1_int_speed_combo = gtk_combo_box_entry_new_text();
	gtk_widget_show(ch1_int_speed_combo);
	gtk_container_add(GTK_CONTAINER(ch1_int_speed_hbox), ch1_int_speed_combo);
	gtk_widget_set_size_request(ch1_int_speed_combo, 80, -1);
	for (i = 0; i < NELEMENTS(ch1_int_speed_str); i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(ch1_int_speed_combo), ch1_int_speed_str[i]);
	}
	ch1_int_speed_entry = gtk_bin_get_child(GTK_BIN(ch1_int_speed_combo));
	gtk_widget_show(ch1_int_speed_entry);
	gtk_editable_set_editable(GTK_EDITABLE(ch1_int_speed_entry), FALSE);
	i = np2cfg.memsw[1] & 0xF;
	if(i == 0 || i > 9) {
		gtk_entry_set_text(GTK_ENTRY(ch1_int_speed_entry), ch1_int_speed_str[0]);
	} else {
		gtk_entry_set_text(GTK_ENTRY(ch1_int_speed_entry), ch1_int_speed_str[i]);
	}

	/* Ch.1 DataSize */
	ch1_int_datasize_hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(ch1_int_datasize_hbox), 5);
	gtk_widget_show(ch1_int_datasize_hbox);
	gtk_box_pack_start(GTK_BOX(internal_frame_vbox), ch1_int_datasize_hbox, FALSE, FALSE, 0);

	ch1_int_datasize_label = gtk_label_new("DataSize");
	gtk_widget_show(ch1_int_datasize_label);
	gtk_container_add(GTK_CONTAINER(ch1_int_datasize_hbox), ch1_int_datasize_label);

	ch1_int_datasize_combo = gtk_combo_box_entry_new_text();
	gtk_widget_show(ch1_int_datasize_combo);
	gtk_container_add(GTK_CONTAINER(ch1_int_datasize_hbox), ch1_int_datasize_combo);
	gtk_widget_set_size_request(ch1_int_datasize_combo, 80, -1);
	for (i = 0; i < NELEMENTS(ch1_int_datasize_str); i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(ch1_int_datasize_combo), ch1_int_datasize_str[i]);
	}
	ch1_int_datasize_entry = gtk_bin_get_child(GTK_BIN(ch1_int_datasize_combo));
	gtk_widget_show(ch1_int_datasize_entry);
	gtk_editable_set_editable(GTK_EDITABLE(ch1_int_datasize_entry), FALSE);
	switch(np2cfg.memsw[0] & 0xC) {
	case 0xC:
		gtk_entry_set_text(GTK_ENTRY(ch1_int_datasize_entry), ch1_int_datasize_str[1]);
		break;
	default:
		gtk_entry_set_text(GTK_ENTRY(ch1_int_datasize_entry), ch1_int_datasize_str[0]);
		break;
	}

	/* Ch.1 Parity */
	ch1_int_parity_hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(ch1_int_parity_hbox), 5);
	gtk_widget_show(ch1_int_parity_hbox);
	gtk_box_pack_start(GTK_BOX(internal_frame_vbox), ch1_int_parity_hbox, FALSE, FALSE, 0);

	ch1_int_parity_label = gtk_label_new("Parity");
	gtk_widget_show(ch1_int_parity_label);
	gtk_container_add(GTK_CONTAINER(ch1_int_parity_hbox), ch1_int_parity_label);

	ch1_int_parity_combo = gtk_combo_box_entry_new_text();
	gtk_widget_show(ch1_int_parity_combo);
	gtk_container_add(GTK_CONTAINER(ch1_int_parity_hbox), ch1_int_parity_combo);
	gtk_widget_set_size_request(ch1_int_parity_combo, 80, -1);
	for (i = 0; i < NELEMENTS(ch1_int_parity_str); i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(ch1_int_parity_combo), ch1_int_parity_str[i]);
	}
	ch1_int_parity_entry = gtk_bin_get_child(GTK_BIN(ch1_int_parity_combo));
	gtk_widget_show(ch1_int_parity_entry);
	gtk_editable_set_editable(GTK_EDITABLE(ch1_int_parity_entry), FALSE);
	switch(np2cfg.memsw[0] & 0x30) {
	case 0x10:
		gtk_entry_set_text(GTK_ENTRY(ch1_int_parity_entry), ch1_int_parity_str[1]);
		break;
	case 0x30:
		gtk_entry_set_text(GTK_ENTRY(ch1_int_parity_entry), ch1_int_parity_str[2]);
		break;
	default:
		gtk_entry_set_text(GTK_ENTRY(ch1_int_parity_entry), ch1_int_parity_str[0]);
		break;
	}

	/* Ch.1 StopBit */
	ch1_int_stopbit_hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(ch1_int_stopbit_hbox), 5);
	gtk_widget_show(ch1_int_stopbit_hbox);
	gtk_box_pack_start(GTK_BOX(internal_frame_vbox), ch1_int_stopbit_hbox, FALSE, FALSE, 0);

	ch1_int_stopbit_label = gtk_label_new("StopBit");
	gtk_widget_show(ch1_int_stopbit_label);
	gtk_container_add(GTK_CONTAINER(ch1_int_stopbit_hbox), ch1_int_stopbit_label);

	ch1_int_stopbit_combo = gtk_combo_box_entry_new_text();
	gtk_widget_show(ch1_int_stopbit_combo);
	gtk_container_add(GTK_CONTAINER(ch1_int_stopbit_hbox), ch1_int_stopbit_combo);
	gtk_widget_set_size_request(ch1_int_stopbit_combo, 80, -1);
	for (i = 0; i < NELEMENTS(ch1_int_stopbit_str); i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(ch1_int_stopbit_combo), ch1_int_stopbit_str[i]);
	}
	ch1_int_stopbit_entry = gtk_bin_get_child(GTK_BIN(ch1_int_stopbit_combo));
	gtk_widget_show(ch1_int_stopbit_entry);
	gtk_editable_set_editable(GTK_EDITABLE(ch1_int_stopbit_entry), FALSE);
	switch(np2cfg.memsw[0] & 0xC0) {
	case 0x80:
		gtk_entry_set_text(GTK_ENTRY(ch1_int_stopbit_entry), ch1_int_stopbit_str[1]);
		break;
	case 0xC0:
		gtk_entry_set_text(GTK_ENTRY(ch1_int_stopbit_entry), ch1_int_stopbit_str[2]);
		break;
	default:
		gtk_entry_set_text(GTK_ENTRY(ch1_int_stopbit_entry), ch1_int_stopbit_str[0]);
		break;
	}

	/* Ch.1 FlowCtrl */
	ch1_int_flowctrl_hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(ch1_int_flowctrl_hbox), 5);
	gtk_widget_show(ch1_int_flowctrl_hbox);
	gtk_box_pack_start(GTK_BOX(internal_frame_vbox), ch1_int_flowctrl_hbox, FALSE, FALSE, 0);

	ch1_int_flowctrl_label = gtk_label_new("Flow Control");
	gtk_widget_show(ch1_int_flowctrl_label);
	gtk_container_add(GTK_CONTAINER(ch1_int_flowctrl_hbox), ch1_int_flowctrl_label);

	ch1_int_flowctrl_combo = gtk_combo_box_entry_new_text();
	gtk_widget_show(ch1_int_flowctrl_combo);
	gtk_container_add(GTK_CONTAINER(ch1_int_flowctrl_hbox), ch1_int_flowctrl_combo);
	gtk_widget_set_size_request(ch1_int_flowctrl_combo, 80, -1);
	for (i = 0; i < NELEMENTS(ch1_int_flowctrl_str); i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(ch1_int_flowctrl_combo), ch1_int_flowctrl_str[i]);
	}
	ch1_int_flowctrl_entry = gtk_bin_get_child(GTK_BIN(ch1_int_flowctrl_combo));
	gtk_widget_show(ch1_int_flowctrl_entry);
	gtk_editable_set_editable(GTK_EDITABLE(ch1_int_flowctrl_entry), FALSE);
	gtk_entry_set_text(GTK_ENTRY(ch1_int_flowctrl_entry), ch1_int_flowctrl_str[np2cfg.memsw[0] & 0x1]);

	/* Ch.1 Duplex */
	ch1_int_duplex_hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(ch1_int_duplex_hbox), 5);
	gtk_widget_show(ch1_int_duplex_hbox);
	gtk_box_pack_start(GTK_BOX(internal_frame_vbox), ch1_int_duplex_hbox, FALSE, FALSE, 0);

	ch1_int_duplex_label = gtk_label_new("Duplex");
	gtk_widget_show(ch1_int_duplex_label);
	gtk_container_add(GTK_CONTAINER(ch1_int_duplex_hbox), ch1_int_duplex_label);

	ch1_int_duplex_combo = gtk_combo_box_entry_new_text();
	gtk_widget_show(ch1_int_duplex_combo);
	gtk_container_add(GTK_CONTAINER(ch1_int_duplex_hbox), ch1_int_duplex_combo);
	gtk_widget_set_size_request(ch1_int_duplex_combo, 80, -1);
	for (i = 0; i < NELEMENTS(ch1_int_duplex_str); i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(ch1_int_duplex_combo), ch1_int_duplex_str[i]);
	}
	ch1_int_duplex_entry = gtk_bin_get_child(GTK_BIN(ch1_int_duplex_combo));
	gtk_widget_show(ch1_int_duplex_entry);
	gtk_editable_set_editable(GTK_EDITABLE(ch1_int_duplex_entry), FALSE);
	gtk_entry_set_text(GTK_ENTRY(ch1_int_duplex_entry), ch1_int_duplex_str[(np2cfg.memsw[0] & 0x2) >> 1]);

	/* Ch.1 7bitKana */
	ch1_int_7bitkana_hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(ch1_int_7bitkana_hbox), 5);
	gtk_widget_show(ch1_int_7bitkana_hbox);
	gtk_box_pack_start(GTK_BOX(internal_frame_vbox), ch1_int_7bitkana_hbox, FALSE, FALSE, 0);

	ch1_int_7bitkana_label = gtk_label_new("7bit Kana");
	gtk_widget_show(ch1_int_7bitkana_label);
	gtk_container_add(GTK_CONTAINER(ch1_int_7bitkana_hbox), ch1_int_7bitkana_label);

	ch1_int_7bitkana_combo = gtk_combo_box_entry_new_text();
	gtk_widget_show(ch1_int_7bitkana_combo);
	gtk_container_add(GTK_CONTAINER(ch1_int_7bitkana_hbox), ch1_int_7bitkana_combo);
	gtk_widget_set_size_request(ch1_int_7bitkana_combo, 80, -1);
	for (i = 0; i < NELEMENTS(ch1_int_7bitkana_str); i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(ch1_int_7bitkana_combo), ch1_int_7bitkana_str[i]);
	}
	ch1_int_7bitkana_entry = gtk_bin_get_child(GTK_BIN(ch1_int_7bitkana_combo));
	gtk_widget_show(ch1_int_7bitkana_entry);
	gtk_editable_set_editable(GTK_EDITABLE(ch1_int_7bitkana_entry), FALSE);
	gtk_entry_set_text(GTK_ENTRY(ch1_int_7bitkana_entry), ch1_int_7bitkana_str[(np2cfg.memsw[1] & 0x80) >> 7]);

	/* Ch.1 SendNl */
	ch1_int_sendnl_hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(ch1_int_sendnl_hbox), 5);
	gtk_widget_show(ch1_int_sendnl_hbox);
	gtk_box_pack_start(GTK_BOX(internal_frame_vbox), ch1_int_sendnl_hbox, FALSE, FALSE, 0);

	ch1_int_sendnl_label = gtk_label_new("Send NewLine");
	gtk_widget_show(ch1_int_sendnl_label);
	gtk_container_add(GTK_CONTAINER(ch1_int_sendnl_hbox), ch1_int_sendnl_label);

	ch1_int_sendnl_combo = gtk_combo_box_entry_new_text();
	gtk_widget_show(ch1_int_sendnl_combo);
	gtk_container_add(GTK_CONTAINER(ch1_int_sendnl_hbox), ch1_int_sendnl_combo);
	gtk_widget_set_size_request(ch1_int_sendnl_combo, 80, -1);
	for (i = 0; i < NELEMENTS(ch1_int_sendnl_str); i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(ch1_int_sendnl_combo), ch1_int_sendnl_str[i]);
	}
	ch1_int_sendnl_entry = gtk_bin_get_child(GTK_BIN(ch1_int_sendnl_combo));
	gtk_widget_show(ch1_int_sendnl_entry);
	gtk_editable_set_editable(GTK_EDITABLE(ch1_int_sendnl_entry), FALSE);
	gtk_entry_set_text(GTK_ENTRY(ch1_int_sendnl_entry), ch1_int_sendnl_str[(np2cfg.memsw[1] & 0x40) >> 6]);

	/* Ch.1 RcvNl */
	ch1_int_rcvnl_hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(ch1_int_rcvnl_hbox), 5);
	gtk_widget_show(ch1_int_rcvnl_hbox);
	gtk_box_pack_start(GTK_BOX(internal_frame_vbox), ch1_int_rcvnl_hbox, FALSE, FALSE, 0);

	ch1_int_rcvnl_label = gtk_label_new("Receive NewLine");
	gtk_widget_show(ch1_int_rcvnl_label);
	gtk_container_add(GTK_CONTAINER(ch1_int_rcvnl_hbox), ch1_int_rcvnl_label);

	ch1_int_rcvnl_combo = gtk_combo_box_entry_new_text();
	gtk_widget_show(ch1_int_rcvnl_combo);
	gtk_container_add(GTK_CONTAINER(ch1_int_rcvnl_hbox), ch1_int_rcvnl_combo);
	gtk_widget_set_size_request(ch1_int_rcvnl_combo, 80, -1);
	for (i = 0; i < NELEMENTS(ch1_int_rcvnl_str); i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(ch1_int_rcvnl_combo), ch1_int_rcvnl_str[i]);
	}
	ch1_int_rcvnl_entry = gtk_bin_get_child(GTK_BIN(ch1_int_rcvnl_combo));
	gtk_widget_show(ch1_int_rcvnl_entry);
	gtk_editable_set_editable(GTK_EDITABLE(ch1_int_rcvnl_entry), FALSE);
	gtk_entry_set_text(GTK_ENTRY(ch1_int_rcvnl_entry), ch1_int_rcvnl_str[(np2cfg.memsw[1] & 0x20) >> 5]);

	/* Ch.1 JSCode */
	ch1_int_jscode_hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(ch1_int_jscode_hbox), 5);
	gtk_widget_show(ch1_int_jscode_hbox);
	gtk_box_pack_start(GTK_BOX(internal_frame_vbox), ch1_int_jscode_hbox, FALSE, FALSE, 0);

	ch1_int_jscode_label = gtk_label_new("Japanese Shift Code");
	gtk_widget_show(ch1_int_jscode_label);
	gtk_container_add(GTK_CONTAINER(ch1_int_jscode_hbox), ch1_int_jscode_label);

	ch1_int_jscode_combo = gtk_combo_box_entry_new_text();
	gtk_widget_show(ch1_int_jscode_combo);
	gtk_container_add(GTK_CONTAINER(ch1_int_jscode_hbox), ch1_int_jscode_combo);
	gtk_widget_set_size_request(ch1_int_jscode_combo, 80, -1);
	for (i = 0; i < NELEMENTS(ch1_int_jscode_str); i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(ch1_int_jscode_combo), ch1_int_jscode_str[i]);
	}
	ch1_int_jscode_entry = gtk_bin_get_child(GTK_BIN(ch1_int_jscode_combo));
	gtk_widget_show(ch1_int_jscode_entry);
	gtk_editable_set_editable(GTK_EDITABLE(ch1_int_jscode_entry), FALSE);
	gtk_entry_set_text(GTK_ENTRY(ch1_int_jscode_entry), ch1_int_jscode_str[(np2cfg.memsw[1] & 0x10) >> 4]);

	/* Ch.1 RcvDel */
	ch1_int_rcvdel_hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(ch1_int_rcvdel_hbox), 5);
	gtk_widget_show(ch1_int_rcvdel_hbox);
	gtk_box_pack_start(GTK_BOX(internal_frame_vbox), ch1_int_rcvdel_hbox, FALSE, FALSE, 0);

	ch1_int_rcvdel_label = gtk_label_new("Receive Del");
	gtk_widget_show(ch1_int_rcvdel_label);
	gtk_container_add(GTK_CONTAINER(ch1_int_rcvdel_hbox), ch1_int_rcvdel_label);

	ch1_int_rcvdel_combo = gtk_combo_box_entry_new_text();
	gtk_widget_show(ch1_int_rcvdel_combo);
	gtk_container_add(GTK_CONTAINER(ch1_int_rcvdel_hbox), ch1_int_rcvdel_combo);
	gtk_widget_set_size_request(ch1_int_rcvdel_combo, 80, -1);
	for (i = 0; i < NELEMENTS(ch1_int_rcvdel_str); i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(ch1_int_rcvdel_combo), ch1_int_rcvdel_str[i]);
	}
	ch1_int_rcvdel_entry = gtk_bin_get_child(GTK_BIN(ch1_int_rcvdel_combo));
	gtk_widget_show(ch1_int_rcvdel_entry);
	gtk_editable_set_editable(GTK_EDITABLE(ch1_int_rcvdel_entry), FALSE);
	gtk_entry_set_text(GTK_ENTRY(ch1_int_rcvdel_entry), ch1_int_rcvdel_str[(np2cfg.memsw[2] & 0x80) >> 7]);

	external_frame = gtk_frame_new("External");
	gtk_container_set_border_width(GTK_CONTAINER(external_frame), 2);
	gtk_widget_show(external_frame);
	gtk_container_add(GTK_CONTAINER(root_widget), external_frame);

	external_frame_vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(external_frame_vbox);
	gtk_container_add(GTK_CONTAINER(external_frame), external_frame_vbox);

	ch1_ext_mout_hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(ch1_ext_mout_hbox), 5);
	gtk_widget_show(ch1_ext_mout_hbox);
	gtk_box_pack_start(GTK_BOX(external_frame_vbox), ch1_ext_mout_hbox, FALSE, FALSE, 0);

	/* Ch.1 MOut */
	ch1_ext_mout_label = gtk_label_new("Output");
	gtk_widget_show(ch1_ext_mout_label);
	gtk_container_add(GTK_CONTAINER(ch1_ext_mout_hbox), ch1_ext_mout_label);

	ch1_ext_mout_entry = gtk_entry_new();
	gtk_widget_show(ch1_ext_mout_entry);
	gtk_widget_set_size_request(ch1_ext_mout_entry, 80, -1);
	gtk_container_add(GTK_CONTAINER(ch1_ext_mout_hbox), ch1_ext_mout_entry);
	gtk_entry_set_text(GTK_ENTRY(ch1_ext_mout_entry), np2oscfg.com[0].mout);

	/* Ch.1 Direct */
	ch1_ext_direct_checkbutton = gtk_check_button_new_with_label("Direct");
	gtk_widget_show(ch1_ext_direct_checkbutton);
	gtk_container_add(GTK_CONTAINER(external_frame_vbox), ch1_ext_direct_checkbutton);
	g_signal_connect(G_OBJECT(ch1_ext_direct_checkbutton), "clicked",
		G_CALLBACK(ch1_ext_direct_checkbutton_clicked), NULL);
	if (np2oscfg.com[0].direct)
		g_signal_emit_by_name(G_OBJECT(ch1_ext_direct_checkbutton), "clicked");

	ch1_ext_speed_hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(ch1_ext_speed_hbox), 5);
	gtk_widget_show(ch1_ext_speed_hbox);
	gtk_box_pack_start(GTK_BOX(external_frame_vbox), ch1_ext_speed_hbox, FALSE, FALSE, 0);

	/* Ch.1 Speed */
	ch1_ext_speed_label = gtk_label_new("Speed");
	gtk_widget_show(ch1_ext_speed_label);
	gtk_container_add(GTK_CONTAINER(ch1_ext_speed_hbox), ch1_ext_speed_label);

	ch1_ext_speed_combo = gtk_combo_box_entry_new_text();
	gtk_widget_show(ch1_ext_speed_combo);
	gtk_container_add(GTK_CONTAINER(ch1_ext_speed_hbox), ch1_ext_speed_combo);
	gtk_widget_set_size_request(ch1_ext_speed_combo, 80, -1);
	for (i = 0; i < NELEMENTS(ext_speed_str); i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(ch1_ext_speed_combo), ext_speed_str[i]);
	}
	ch1_ext_speed_entry = gtk_bin_get_child(GTK_BIN(ch1_ext_speed_combo));
	gtk_widget_show(ch1_ext_speed_entry);
	gtk_editable_set_editable(GTK_EDITABLE(ch1_ext_speed_entry), FALSE);
	sprintf(str, "%d", np2oscfg.com[0].speed);
	gtk_entry_set_text(GTK_ENTRY(ch1_ext_speed_entry), str);
	if (np2oscfg.com[0].direct)
		gtk_widget_set_sensitive(ch1_ext_speed_combo, FALSE);

	ch1_ext_datasize_hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(ch1_ext_datasize_hbox), 5);
	gtk_widget_show(ch1_ext_datasize_hbox);
	gtk_box_pack_start(GTK_BOX(external_frame_vbox), ch1_ext_datasize_hbox, FALSE, FALSE, 0);

	/* Ch.1 DataSize */
	ch1_ext_datasize_label = gtk_label_new("DataSize");
	gtk_widget_show(ch1_ext_datasize_label);
	gtk_container_add(GTK_CONTAINER(ch1_ext_datasize_hbox), ch1_ext_datasize_label);

	ch1_ext_datasize_combo = gtk_combo_box_entry_new_text();
	gtk_widget_show(ch1_ext_datasize_combo);
	gtk_container_add(GTK_CONTAINER(ch1_ext_datasize_hbox), ch1_ext_datasize_combo);
	gtk_widget_set_size_request(ch1_ext_datasize_combo, 40, -1);
	for (i = 0; i < NELEMENTS(ext_datasize_str); i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(ch1_ext_datasize_combo), ext_datasize_str[i]);
	}
	ch1_ext_datasize_entry = gtk_bin_get_child(GTK_BIN(ch1_ext_datasize_combo));
	gtk_widget_show(ch1_ext_datasize_entry);
	gtk_editable_set_editable(GTK_EDITABLE(ch1_ext_datasize_entry), FALSE);
	gtk_entry_set_text(GTK_ENTRY(ch1_ext_datasize_entry), ext_datasize_str[(np2oscfg.com[0].param >> 2) & 3]);
	if (np2oscfg.com[0].direct)
		gtk_widget_set_sensitive(ch1_ext_datasize_combo, FALSE);

	ch1_ext_parity_hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(ch1_ext_parity_hbox), 5);
	gtk_widget_show(ch1_ext_parity_hbox);
	gtk_box_pack_start(GTK_BOX(external_frame_vbox), ch1_ext_parity_hbox, FALSE, FALSE, 0);

	/* Ch.1 Parity */
	ch1_ext_parity_label = gtk_label_new("Parity");
	gtk_widget_show(ch1_ext_parity_label);
	gtk_container_add(GTK_CONTAINER(ch1_ext_parity_hbox), ch1_ext_parity_label);

	ch1_ext_parity_combo = gtk_combo_box_entry_new_text();
	gtk_widget_show(ch1_ext_parity_combo);
	gtk_container_add(GTK_CONTAINER(ch1_ext_parity_hbox), ch1_ext_parity_combo);
	gtk_widget_set_size_request(ch1_ext_parity_combo, 80, -1);
	for (i = 0; i < NELEMENTS(ext_parity_str); i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(ch1_ext_parity_combo), ext_parity_str[i]);
	}
	ch1_ext_parity_entry = gtk_bin_get_child(GTK_BIN(ch1_ext_parity_combo));
	gtk_widget_show(ch1_ext_parity_entry);
	gtk_editable_set_editable(GTK_EDITABLE(ch1_ext_parity_entry), FALSE);
	switch(np2oscfg.com[0].param & 0x30) {
	case 0x10:
		gtk_entry_set_text(GTK_ENTRY(ch1_ext_parity_entry), "Odd");
		break;
	case 0x30:
		gtk_entry_set_text(GTK_ENTRY(ch1_ext_parity_entry), "Even");
		break;
	default:
		gtk_entry_set_text(GTK_ENTRY(ch1_ext_parity_entry), "None");
		break;
	}
	if (np2oscfg.com[0].direct)
		gtk_widget_set_sensitive(ch1_ext_parity_combo, FALSE);

	ch1_ext_stopbit_hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(ch1_ext_stopbit_hbox), 5);
	gtk_widget_show(ch1_ext_stopbit_hbox);
	gtk_box_pack_start(GTK_BOX(external_frame_vbox), ch1_ext_stopbit_hbox, FALSE, FALSE, 0);

	/* Ch.1 StopBit */
	ch1_ext_stopbit_label = gtk_label_new("StopBit");
	gtk_widget_show(ch1_ext_stopbit_label);
	gtk_container_add(GTK_CONTAINER(ch1_ext_stopbit_hbox), ch1_ext_stopbit_label);

	ch1_ext_stopbit_combo = gtk_combo_box_entry_new_text();
	gtk_widget_show(ch1_ext_stopbit_combo);
	gtk_container_add(GTK_CONTAINER(ch1_ext_stopbit_hbox), ch1_ext_stopbit_combo);
	gtk_widget_set_size_request(ch1_ext_stopbit_combo, 40, -1);
	for (i = 0; i < NELEMENTS(ext_stopbit_str); i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(ch1_ext_stopbit_combo), ext_stopbit_str[i]);
	}
	ch1_ext_stopbit_entry = gtk_bin_get_child(GTK_BIN(ch1_ext_stopbit_combo));
	gtk_widget_show(ch1_ext_stopbit_entry);
	gtk_editable_set_editable(GTK_EDITABLE(ch1_ext_stopbit_entry), FALSE);
	switch(np2oscfg.com[0].param & 0xC0) {
	case 0x80:
		gtk_entry_set_text(GTK_ENTRY(ch1_ext_stopbit_entry), "1.5");
		break;
	case 0xC0:
		gtk_entry_set_text(GTK_ENTRY(ch1_ext_stopbit_entry), "2");
		break;
	default:
		gtk_entry_set_text(GTK_ENTRY(ch1_ext_stopbit_entry), "1");
		break;
	}
	if (np2oscfg.com[0].direct)
		gtk_widget_set_sensitive(ch1_ext_stopbit_combo, FALSE);

	return root_widget;
}

static GtkWidget*
create_pc9861k_note(void)
{
	GtkWidget *root_widget;
	GtkWidget *table;
	GtkWidget *internal_frame;
	GtkWidget *internal_frame_vbox;
	GtkWidget *ch2_int_label;
	GtkWidget *ch2_int_speed_hbox;
	GtkWidget *ch2_int_speed_label;
	GtkWidget *ch2_int_speed_combo;
	GtkWidget *ch2_int_int_hbox;
	GtkWidget *ch2_int_int_label;
	GtkWidget *ch2_int_int_combo;
	GtkWidget *ch2_int_mode_hbox;
	GtkWidget *ch2_int_mode_label;
	GtkWidget *ch2_int_mode_combo;
	GtkWidget *ch3_int_label;
	GtkWidget *ch3_int_speed_hbox;
	GtkWidget *ch3_int_speed_label;
	GtkWidget *ch3_int_speed_combo;
	GtkWidget *ch3_int_int_hbox;
	GtkWidget *ch3_int_int_label;
	GtkWidget *ch3_int_int_combo;
	GtkWidget *ch3_int_mode_hbox;
	GtkWidget *ch3_int_mode_label;
	GtkWidget *ch3_int_mode_combo;
	int i;
	char str[10];

	root_widget = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(root_widget), 5);
	gtk_widget_show(root_widget);

	internal_frame = gtk_frame_new("Internal");
	gtk_container_set_border_width(GTK_CONTAINER(internal_frame), 2);
	gtk_widget_show(internal_frame);
	gtk_container_add(GTK_CONTAINER(root_widget), internal_frame);

	internal_frame_vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(internal_frame_vbox);
	gtk_container_add(GTK_CONTAINER(internal_frame), internal_frame_vbox);

	/* Enable */
	ch2_int_enable_checkbutton = gtk_check_button_new_with_label("Enable");
	gtk_widget_show(ch2_int_enable_checkbutton);
	gtk_box_pack_start(GTK_BOX(internal_frame_vbox), ch2_int_enable_checkbutton, FALSE, FALSE, 0);
	if (np2cfg.pc9861enable)
		g_signal_emit_by_name(G_OBJECT(ch2_int_enable_checkbutton), "clicked");

	table = gtk_table_new(4, 4, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 5);
	gtk_table_set_col_spacings(GTK_TABLE(table), 5);
	gtk_box_pack_start(GTK_BOX(internal_frame_vbox), table, FALSE, FALSE, 0);
	gtk_widget_show(table);

	ch2_int_label = gtk_label_new("Ch.2");
	gtk_widget_show(ch2_int_label);
	gtk_table_attach_defaults(GTK_TABLE(table), ch2_int_label, 0, 1, 0, 1);

	/* Ch.2 Speed */
	ch2_int_speed_hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(ch2_int_speed_hbox), 5);
	gtk_widget_show(ch2_int_speed_hbox);
	gtk_table_attach_defaults(GTK_TABLE(table), ch2_int_speed_hbox, 1, 2, 0, 1);

	ch2_int_speed_label = gtk_label_new("Speed");
	gtk_widget_show(ch2_int_speed_label);
	gtk_container_add(GTK_CONTAINER(ch2_int_speed_hbox), ch2_int_speed_label);

	ch2_int_speed_combo = gtk_combo_box_entry_new_text();
	gtk_widget_show(ch2_int_speed_combo);
	gtk_container_add(GTK_CONTAINER(ch2_int_speed_hbox), ch2_int_speed_combo);
	gtk_widget_set_size_request(ch2_int_speed_combo, 80, -1);
	for (i = 0; i < NELEMENTS(ch23_int_speed_str); i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(ch2_int_speed_combo), ch23_int_speed_str[i]);
	}
	ch2_int_speed_entry = gtk_bin_get_child(GTK_BIN(ch2_int_speed_combo));
	gtk_widget_show(ch2_int_speed_entry);
	gtk_editable_set_editable(GTK_EDITABLE(ch2_int_speed_entry), FALSE);
	if(!(np2cfg.pc9861sw[0] & 0x2)) {	// Sync
		gtk_entry_set_text(GTK_ENTRY(ch2_int_speed_entry), ch23_int_speed_str[7 - ((np2cfg.pc9861sw[0] >> 2) & 0x7) + 1]);
	} else {	// Async
		gtk_entry_set_text(GTK_ENTRY(ch2_int_speed_entry), ch23_int_speed_str[8 - (((np2cfg.pc9861sw[0] >> 2) & 0xF) - 4)]);
	}

	/* Ch.2 INT */
	ch2_int_int_hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(ch2_int_int_hbox), 5);
	gtk_widget_show(ch2_int_int_hbox);
	gtk_table_attach_defaults(GTK_TABLE(table), ch2_int_int_hbox, 2, 3, 0, 1);

	ch2_int_int_label = gtk_label_new("INT");
	gtk_widget_show(ch2_int_int_label);
	gtk_container_add(GTK_CONTAINER(ch2_int_int_hbox), ch2_int_int_label);

	ch2_int_int_combo = gtk_combo_box_entry_new_text();
	gtk_widget_show(ch2_int_int_combo);
	gtk_container_add(GTK_CONTAINER(ch2_int_int_hbox), ch2_int_int_combo);
	gtk_widget_set_size_request(ch2_int_int_combo, 40, -1);
	for (i = 0; i < NELEMENTS(ch2_int_int_str); i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(ch2_int_int_combo), ch2_int_int_str[i]);
	}
	ch2_int_int_entry = gtk_bin_get_child(GTK_BIN(ch2_int_int_combo));
	gtk_widget_show(ch2_int_int_entry);
	gtk_editable_set_editable(GTK_EDITABLE(ch2_int_int_entry), FALSE);
	switch(np2cfg.pc9861sw[1] & 0x03) {
	case 0:
		i = 0;
		break;
	case 1:
		i = 2;
		break;
	case 2:
		i = 1;
		break;
	case 3:
		i = 3;
		break;
	}
	gtk_entry_set_text(GTK_ENTRY(ch2_int_int_entry), ch2_int_int_str[i]);

	/* Ch.2 Mode */
	ch2_int_mode_hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(ch2_int_mode_hbox), 5);
	gtk_widget_show(ch2_int_mode_hbox);
	gtk_table_attach_defaults(GTK_TABLE(table), ch2_int_mode_hbox, 1, 2, 1, 2);

	ch2_int_mode_label = gtk_label_new("Mode");
	gtk_widget_show(ch2_int_mode_label);
	gtk_container_add(GTK_CONTAINER(ch2_int_mode_hbox), ch2_int_mode_label);

	ch2_int_mode_combo = gtk_combo_box_entry_new_text();
	gtk_widget_show(ch2_int_mode_combo);
	gtk_container_add(GTK_CONTAINER(ch2_int_mode_hbox), ch2_int_mode_combo);
	gtk_widget_set_size_request(ch2_int_mode_combo, 80, -1);
	for (i = 0; i < NELEMENTS(ch23_int_mode_str); i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(ch2_int_mode_combo), ch23_int_mode_str[i]);
	}
	ch2_int_mode_entry = gtk_bin_get_child(GTK_BIN(ch2_int_mode_combo));
	gtk_widget_show(ch2_int_mode_entry);
	gtk_editable_set_editable(GTK_EDITABLE(ch2_int_mode_entry), FALSE);
	gtk_entry_set_text(GTK_ENTRY(ch2_int_mode_entry), ch23_int_mode_str[np2cfg.pc9861sw[0] & 0x03]);

	ch3_int_label = gtk_label_new("Ch.3");
	gtk_widget_show(ch3_int_label);
	gtk_table_attach_defaults(GTK_TABLE(table), ch3_int_label, 0, 1, 2, 3);

	/* Ch.3 Speed */
	ch3_int_speed_hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(ch3_int_speed_hbox), 5);
	gtk_widget_show(ch3_int_speed_hbox);
	gtk_table_attach_defaults(GTK_TABLE(table), ch3_int_speed_hbox, 1, 2, 2, 3);

	ch3_int_speed_label = gtk_label_new("Speed");
	gtk_widget_show(ch3_int_speed_label);
	gtk_container_add(GTK_CONTAINER(ch3_int_speed_hbox), ch3_int_speed_label);

	ch3_int_speed_combo = gtk_combo_box_entry_new_text();
	gtk_widget_show(ch3_int_speed_combo);
	gtk_container_add(GTK_CONTAINER(ch3_int_speed_hbox), ch3_int_speed_combo);
	gtk_widget_set_size_request(ch3_int_speed_combo, 80, -1);
	for (i = 0; i < NELEMENTS(ch23_int_speed_str); i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(ch3_int_speed_combo), ch23_int_speed_str[i]);
	}
	ch3_int_speed_entry = gtk_bin_get_child(GTK_BIN(ch3_int_speed_combo));
	gtk_widget_show(ch3_int_speed_entry);
	gtk_editable_set_editable(GTK_EDITABLE(ch3_int_speed_entry), FALSE);
	if(!(np2cfg.pc9861sw[2] & 0x2)) {	// Sync
		gtk_entry_set_text(GTK_ENTRY(ch3_int_speed_entry), ch23_int_speed_str[7 - ((np2cfg.pc9861sw[2] >> 2) & 0x7) + 1]);
	} else {	// Async
		gtk_entry_set_text(GTK_ENTRY(ch3_int_speed_entry), ch23_int_speed_str[8 - (((np2cfg.pc9861sw[2] >> 2) & 0xF) - 4)]);
	}

	/* Ch.3 INT */
	ch3_int_int_hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(ch3_int_int_hbox), 5);
	gtk_widget_show(ch3_int_int_hbox);
	gtk_table_attach_defaults(GTK_TABLE(table), ch3_int_int_hbox, 2, 3, 2, 3);

	ch3_int_int_label = gtk_label_new("INT");
	gtk_widget_show(ch3_int_int_label);
	gtk_container_add(GTK_CONTAINER(ch3_int_int_hbox), ch3_int_int_label);

	ch3_int_int_combo = gtk_combo_box_entry_new_text();
	gtk_widget_show(ch3_int_int_combo);
	gtk_container_add(GTK_CONTAINER(ch3_int_int_hbox), ch3_int_int_combo);
	gtk_widget_set_size_request(ch3_int_int_combo, 40, -1);
	for (i = 0; i < NELEMENTS(ch3_int_int_str); i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(ch3_int_int_combo), ch3_int_int_str[i]);
	}
	ch3_int_int_entry = gtk_bin_get_child(GTK_BIN(ch3_int_int_combo));
	gtk_widget_show(ch3_int_int_entry);
	gtk_editable_set_editable(GTK_EDITABLE(ch3_int_int_entry), FALSE);
	switch((np2cfg.pc9861sw[1] & 0x0C) >> 2) {
	case 0:
		i = 0;
		break;
	case 1:
		i = 2;
		break;
	case 2:
		i = 1;
		break;
	case 3:
		i = 3;
		break;
	}
	gtk_entry_set_text(GTK_ENTRY(ch3_int_int_entry), ch3_int_int_str[i]);

	/* Ch.3 Mode */
	ch3_int_mode_hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(ch3_int_mode_hbox), 5);
	gtk_widget_show(ch3_int_mode_hbox);
	gtk_table_attach_defaults(GTK_TABLE(table), ch3_int_mode_hbox, 1, 2, 3, 4);

	ch3_int_mode_label = gtk_label_new("Mode");
	gtk_widget_show(ch3_int_mode_label);
	gtk_container_add(GTK_CONTAINER(ch3_int_mode_hbox), ch3_int_mode_label);

	ch3_int_mode_combo = gtk_combo_box_entry_new_text();
	gtk_widget_show(ch3_int_mode_combo);
	gtk_container_add(GTK_CONTAINER(ch3_int_mode_hbox), ch3_int_mode_combo);
	gtk_widget_set_size_request(ch3_int_mode_combo, 80, -1);
	for (i = 0; i < NELEMENTS(ch23_int_mode_str); i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(ch3_int_mode_combo), ch23_int_mode_str[i]);
	}
	ch3_int_mode_entry = gtk_bin_get_child(GTK_BIN(ch3_int_mode_combo));
	gtk_widget_show(ch3_int_mode_entry);
	gtk_editable_set_editable(GTK_EDITABLE(ch3_int_mode_entry), FALSE);
	gtk_entry_set_text(GTK_ENTRY(ch3_int_mode_entry), ch23_int_mode_str[np2cfg.pc9861sw[2] & 0x03]);

	return root_widget;
}

static GtkWidget *
create_ch2_note(void)
{
	GtkWidget *root_widget;
	GtkWidget *external_frame;
	GtkWidget *external_frame_vbox;
	GtkWidget *ch2_ext_mout_hbox;
	GtkWidget *ch2_ext_mout_label;
	GtkWidget *ch2_ext_speed_hbox;
	GtkWidget *ch2_ext_speed_label;
	GtkWidget *ch2_ext_datasize_hbox;
	GtkWidget *ch2_ext_datasize_label;
	GtkWidget *ch2_ext_parity_hbox;
	GtkWidget *ch2_ext_parity_label;
	GtkWidget *ch2_ext_stopbit_hbox;
	GtkWidget *ch2_ext_stopbit_label;
	int i;
	char str[32];

	root_widget = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(root_widget), 5);
	gtk_widget_show(root_widget);

	external_frame = gtk_frame_new("External");
	gtk_container_set_border_width(GTK_CONTAINER(external_frame), 2);
	gtk_widget_show(external_frame);
	gtk_container_add(GTK_CONTAINER(root_widget), external_frame);

	external_frame_vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(external_frame_vbox);
	gtk_container_add(GTK_CONTAINER(external_frame), external_frame_vbox);

	ch2_ext_mout_hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(ch2_ext_mout_hbox), 5);
	gtk_widget_show(ch2_ext_mout_hbox);
	gtk_box_pack_start(GTK_BOX(external_frame_vbox), ch2_ext_mout_hbox, FALSE, FALSE, 0);

	/* Ch.2 MOut */
	ch2_ext_mout_label = gtk_label_new("Output");
	gtk_widget_show(ch2_ext_mout_label);
	gtk_container_add(GTK_CONTAINER(ch2_ext_mout_hbox), ch2_ext_mout_label);

	ch2_ext_mout_entry = gtk_entry_new();
	gtk_widget_show(ch2_ext_mout_entry);
	gtk_widget_set_size_request(ch2_ext_mout_entry, 80, -1);
	gtk_container_add(GTK_CONTAINER(ch2_ext_mout_hbox), ch2_ext_mout_entry);
	gtk_entry_set_text(GTK_ENTRY(ch2_ext_mout_entry), np2oscfg.com[1].mout);

	/* Ch.2 Direct */
	ch2_ext_direct_checkbutton = gtk_check_button_new_with_label("Direct");
	gtk_widget_show(ch2_ext_direct_checkbutton);
	gtk_container_add(GTK_CONTAINER(external_frame_vbox), ch2_ext_direct_checkbutton);
	g_signal_connect(G_OBJECT(ch2_ext_direct_checkbutton), "clicked",
		G_CALLBACK(ch2_ext_direct_checkbutton_clicked), NULL);
	if (np2oscfg.com[1].direct)
		g_signal_emit_by_name(G_OBJECT(ch2_ext_direct_checkbutton), "clicked");

	ch2_ext_speed_hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(ch2_ext_speed_hbox), 5);
	gtk_widget_show(ch2_ext_speed_hbox);
	gtk_box_pack_start(GTK_BOX(external_frame_vbox), ch2_ext_speed_hbox, FALSE, FALSE, 0);

	/* Ch.2 Speed */
	ch2_ext_speed_label = gtk_label_new("Speed");
	gtk_widget_show(ch2_ext_speed_label);
	gtk_container_add(GTK_CONTAINER(ch2_ext_speed_hbox), ch2_ext_speed_label);

	ch2_ext_speed_combo = gtk_combo_box_entry_new_text();
	gtk_widget_show(ch2_ext_speed_combo);
	gtk_container_add(GTK_CONTAINER(ch2_ext_speed_hbox), ch2_ext_speed_combo);
	gtk_widget_set_size_request(ch2_ext_speed_combo, 80, -1);
	for (i = 0; i < NELEMENTS(ext_speed_str); i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(ch2_ext_speed_combo), ext_speed_str[i]);
	}
	ch2_ext_speed_entry = gtk_bin_get_child(GTK_BIN(ch2_ext_speed_combo));
	gtk_widget_show(ch2_ext_speed_entry);
	gtk_editable_set_editable(GTK_EDITABLE(ch2_ext_speed_entry), FALSE);
	sprintf(str, "%d", np2oscfg.com[1].speed);
	gtk_entry_set_text(GTK_ENTRY(ch2_ext_speed_entry), str);
	if (np2oscfg.com[1].direct)
		gtk_widget_set_sensitive(ch2_ext_speed_combo, FALSE);

	ch2_ext_datasize_hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(ch2_ext_datasize_hbox), 5);
	gtk_widget_show(ch2_ext_datasize_hbox);
	gtk_box_pack_start(GTK_BOX(external_frame_vbox), ch2_ext_datasize_hbox, FALSE, FALSE, 0);

	/* Ch.2 DataSize */
	ch2_ext_datasize_label = gtk_label_new("DataSize");
	gtk_widget_show(ch2_ext_datasize_label);
	gtk_container_add(GTK_CONTAINER(ch2_ext_datasize_hbox), ch2_ext_datasize_label);

	ch2_ext_datasize_combo = gtk_combo_box_entry_new_text();
	gtk_widget_show(ch2_ext_datasize_combo);
	gtk_container_add(GTK_CONTAINER(ch2_ext_datasize_hbox), ch2_ext_datasize_combo);
	gtk_widget_set_size_request(ch2_ext_datasize_combo, 40, -1);
	for (i = 0; i < NELEMENTS(ext_datasize_str); i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(ch2_ext_datasize_combo), ext_datasize_str[i]);
	}
	ch2_ext_datasize_entry = gtk_bin_get_child(GTK_BIN(ch2_ext_datasize_combo));
	gtk_widget_show(ch2_ext_datasize_entry);
	gtk_editable_set_editable(GTK_EDITABLE(ch2_ext_datasize_entry), FALSE);
	gtk_entry_set_text(GTK_ENTRY(ch2_ext_datasize_entry), ext_datasize_str[(np2oscfg.com[1].param >> 2) & 3]);
	if (np2oscfg.com[1].direct)
		gtk_widget_set_sensitive(ch2_ext_datasize_combo, FALSE);

	ch2_ext_parity_hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(ch2_ext_parity_hbox), 5);
	gtk_widget_show(ch2_ext_parity_hbox);
	gtk_box_pack_start(GTK_BOX(external_frame_vbox), ch2_ext_parity_hbox, FALSE, FALSE, 0);

	/* Ch.2 Parity */
	ch2_ext_parity_label = gtk_label_new("Parity");
	gtk_widget_show(ch2_ext_parity_label);
	gtk_container_add(GTK_CONTAINER(ch2_ext_parity_hbox), ch2_ext_parity_label);

	ch2_ext_parity_combo = gtk_combo_box_entry_new_text();
	gtk_widget_show(ch2_ext_parity_combo);
	gtk_container_add(GTK_CONTAINER(ch2_ext_parity_hbox), ch2_ext_parity_combo);
	gtk_widget_set_size_request(ch2_ext_parity_combo, 80, -1);
	for (i = 0; i < NELEMENTS(ext_parity_str); i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(ch2_ext_parity_combo), ext_parity_str[i]);
	}
	ch2_ext_parity_entry = gtk_bin_get_child(GTK_BIN(ch2_ext_parity_combo));
	gtk_widget_show(ch2_ext_parity_entry);
	gtk_editable_set_editable(GTK_EDITABLE(ch2_ext_parity_entry), FALSE);
	switch(np2oscfg.com[1].param & 0x30) {
	case 0x10:
		gtk_entry_set_text(GTK_ENTRY(ch2_ext_parity_entry), "Odd");
		break;
	case 0x30:
		gtk_entry_set_text(GTK_ENTRY(ch2_ext_parity_entry), "Even");
		break;
	default:
		gtk_entry_set_text(GTK_ENTRY(ch2_ext_parity_entry), "None");
		break;
	}
	if (np2oscfg.com[1].direct)
		gtk_widget_set_sensitive(ch2_ext_parity_combo, FALSE);

	ch2_ext_stopbit_hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(ch2_ext_stopbit_hbox), 5);
	gtk_widget_show(ch2_ext_stopbit_hbox);
	gtk_box_pack_start(GTK_BOX(external_frame_vbox), ch2_ext_stopbit_hbox, FALSE, FALSE, 0);

	/* Ch.2 StopBit */
	ch2_ext_stopbit_label = gtk_label_new("StopBit");
	gtk_widget_show(ch2_ext_stopbit_label);
	gtk_container_add(GTK_CONTAINER(ch2_ext_stopbit_hbox), ch2_ext_stopbit_label);

	ch2_ext_stopbit_combo = gtk_combo_box_entry_new_text();
	gtk_widget_show(ch2_ext_stopbit_combo);
	gtk_container_add(GTK_CONTAINER(ch2_ext_stopbit_hbox), ch2_ext_stopbit_combo);
	gtk_widget_set_size_request(ch2_ext_stopbit_combo, 40, -1);
	for (i = 0; i < NELEMENTS(ext_stopbit_str); i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(ch2_ext_stopbit_combo), ext_stopbit_str[i]);
	}
	ch2_ext_stopbit_entry = gtk_bin_get_child(GTK_BIN(ch2_ext_stopbit_combo));
	gtk_widget_show(ch2_ext_stopbit_entry);
	gtk_editable_set_editable(GTK_EDITABLE(ch2_ext_stopbit_entry), FALSE);
	switch(np2oscfg.com[1].param & 0xC0) {
	case 0x80:
		gtk_entry_set_text(GTK_ENTRY(ch2_ext_stopbit_entry), "1.5");
		break;
	case 0xC0:
		gtk_entry_set_text(GTK_ENTRY(ch2_ext_stopbit_entry), "2");
		break;
	default:
		gtk_entry_set_text(GTK_ENTRY(ch2_ext_stopbit_entry), "1");
		break;
	}
	if (np2oscfg.com[1].direct)
		gtk_widget_set_sensitive(ch2_ext_stopbit_combo, FALSE);

	return root_widget;
}

static GtkWidget *
create_ch3_note(void)
{
	GtkWidget *root_widget;
	GtkWidget *external_frame;
	GtkWidget *external_frame_vbox;
	GtkWidget *ch3_ext_mout_hbox;
	GtkWidget *ch3_ext_mout_label;
	GtkWidget *ch3_ext_speed_hbox;
	GtkWidget *ch3_ext_speed_label;
	GtkWidget *ch3_ext_datasize_hbox;
	GtkWidget *ch3_ext_datasize_label;
	GtkWidget *ch3_ext_parity_hbox;
	GtkWidget *ch3_ext_parity_label;
	GtkWidget *ch3_ext_stopbit_hbox;
	GtkWidget *ch3_ext_stopbit_label;
	int i;
	char str[32];

	root_widget = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(root_widget), 5);
	gtk_widget_show(root_widget);

	external_frame = gtk_frame_new("External");
	gtk_container_set_border_width(GTK_CONTAINER(external_frame), 2);
	gtk_widget_show(external_frame);
	gtk_container_add(GTK_CONTAINER(root_widget), external_frame);

	external_frame_vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(external_frame_vbox);
	gtk_container_add(GTK_CONTAINER(external_frame), external_frame_vbox);

	ch3_ext_mout_hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(ch3_ext_mout_hbox), 5);
	gtk_widget_show(ch3_ext_mout_hbox);
	gtk_box_pack_start(GTK_BOX(external_frame_vbox), ch3_ext_mout_hbox, FALSE, FALSE, 0);

	/* Ch.3 MOut */
	ch3_ext_mout_label = gtk_label_new("Output");
	gtk_widget_show(ch3_ext_mout_label);
	gtk_container_add(GTK_CONTAINER(ch3_ext_mout_hbox), ch3_ext_mout_label);

	ch3_ext_mout_entry = gtk_entry_new();
	gtk_widget_show(ch3_ext_mout_entry);
	gtk_widget_set_size_request(ch3_ext_mout_entry, 80, -1);
	gtk_container_add(GTK_CONTAINER(ch3_ext_mout_hbox), ch3_ext_mout_entry);
	gtk_entry_set_text(GTK_ENTRY(ch3_ext_mout_entry), np2oscfg.com[2].mout);

	/* Ch.3 Direct */
	ch3_ext_direct_checkbutton = gtk_check_button_new_with_label("Direct");
	gtk_widget_show(ch3_ext_direct_checkbutton);
	gtk_container_add(GTK_CONTAINER(external_frame_vbox), ch3_ext_direct_checkbutton);
	g_signal_connect(G_OBJECT(ch3_ext_direct_checkbutton), "clicked",
		G_CALLBACK(ch3_ext_direct_checkbutton_clicked), NULL);
	if (np2oscfg.com[2].direct)
		g_signal_emit_by_name(G_OBJECT(ch3_ext_direct_checkbutton), "clicked");

	ch3_ext_speed_hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(ch3_ext_speed_hbox), 5);
	gtk_widget_show(ch3_ext_speed_hbox);
	gtk_box_pack_start(GTK_BOX(external_frame_vbox), ch3_ext_speed_hbox, FALSE, FALSE, 0);

	/* Ch.3 Speed */
	ch3_ext_speed_label = gtk_label_new("Speed");
	gtk_widget_show(ch3_ext_speed_label);
	gtk_container_add(GTK_CONTAINER(ch3_ext_speed_hbox), ch3_ext_speed_label);

	ch3_ext_speed_combo = gtk_combo_box_entry_new_text();
	gtk_widget_show(ch3_ext_speed_combo);
	gtk_container_add(GTK_CONTAINER(ch3_ext_speed_hbox), ch3_ext_speed_combo);
	gtk_widget_set_size_request(ch3_ext_speed_combo, 80, -1);
	for (i = 0; i < NELEMENTS(ext_speed_str); i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(ch3_ext_speed_combo), ext_speed_str[i]);
	}
	ch3_ext_speed_entry = gtk_bin_get_child(GTK_BIN(ch3_ext_speed_combo));
	gtk_widget_show(ch3_ext_speed_entry);
	gtk_editable_set_editable(GTK_EDITABLE(ch3_ext_speed_entry), FALSE);
	sprintf(str, "%d", np2oscfg.com[2].speed);
	gtk_entry_set_text(GTK_ENTRY(ch3_ext_speed_entry), str);
	if (np2oscfg.com[2].direct)
		gtk_widget_set_sensitive(ch3_ext_speed_combo, FALSE);

	ch3_ext_datasize_hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(ch3_ext_datasize_hbox), 5);
	gtk_widget_show(ch3_ext_datasize_hbox);
	gtk_box_pack_start(GTK_BOX(external_frame_vbox), ch3_ext_datasize_hbox, FALSE, FALSE, 0);

	/* Ch.3 DataSize */
	ch3_ext_datasize_label = gtk_label_new("DataSize");
	gtk_widget_show(ch3_ext_datasize_label);
	gtk_container_add(GTK_CONTAINER(ch3_ext_datasize_hbox), ch3_ext_datasize_label);

	ch3_ext_datasize_combo = gtk_combo_box_entry_new_text();
	gtk_widget_show(ch3_ext_datasize_combo);
	gtk_container_add(GTK_CONTAINER(ch3_ext_datasize_hbox), ch3_ext_datasize_combo);
	gtk_widget_set_size_request(ch3_ext_datasize_combo, 40, -1);
	for (i = 0; i < NELEMENTS(ext_datasize_str); i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(ch3_ext_datasize_combo), ext_datasize_str[i]);
	}
	ch3_ext_datasize_entry = gtk_bin_get_child(GTK_BIN(ch3_ext_datasize_combo));
	gtk_widget_show(ch3_ext_datasize_entry);
	gtk_editable_set_editable(GTK_EDITABLE(ch3_ext_datasize_entry), FALSE);
	gtk_entry_set_text(GTK_ENTRY(ch3_ext_datasize_entry), ext_datasize_str[(np2oscfg.com[2].param >> 2) & 3]);
	if (np2oscfg.com[2].direct)
		gtk_widget_set_sensitive(ch3_ext_datasize_combo, FALSE);

	ch3_ext_parity_hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(ch3_ext_parity_hbox), 5);
	gtk_widget_show(ch3_ext_parity_hbox);
	gtk_box_pack_start(GTK_BOX(external_frame_vbox), ch3_ext_parity_hbox, FALSE, FALSE, 0);

	/* Ch.3 Parity */
	ch3_ext_parity_label = gtk_label_new("Parity");
	gtk_widget_show(ch3_ext_parity_label);
	gtk_container_add(GTK_CONTAINER(ch3_ext_parity_hbox), ch3_ext_parity_label);

	ch3_ext_parity_combo = gtk_combo_box_entry_new_text();
	gtk_widget_show(ch3_ext_parity_combo);
	gtk_container_add(GTK_CONTAINER(ch3_ext_parity_hbox), ch3_ext_parity_combo);
	gtk_widget_set_size_request(ch3_ext_parity_combo, 80, -1);
	for (i = 0; i < NELEMENTS(ext_parity_str); i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(ch3_ext_parity_combo), ext_parity_str[i]);
	}
	ch3_ext_parity_entry = gtk_bin_get_child(GTK_BIN(ch3_ext_parity_combo));
	gtk_widget_show(ch3_ext_parity_entry);
	gtk_editable_set_editable(GTK_EDITABLE(ch3_ext_parity_entry), FALSE);
	switch(np2oscfg.com[2].param & 0x30) {
	case 0x10:
		gtk_entry_set_text(GTK_ENTRY(ch3_ext_parity_entry), "Odd");
		break;
	case 0x30:
		gtk_entry_set_text(GTK_ENTRY(ch3_ext_parity_entry), "Even");
		break;
	default:
		gtk_entry_set_text(GTK_ENTRY(ch3_ext_parity_entry), "None");
		break;
	}
	if (np2oscfg.com[2].direct)
		gtk_widget_set_sensitive(ch3_ext_parity_combo, FALSE);

	ch3_ext_stopbit_hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(ch3_ext_stopbit_hbox), 5);
	gtk_widget_show(ch3_ext_stopbit_hbox);
	gtk_box_pack_start(GTK_BOX(external_frame_vbox), ch3_ext_stopbit_hbox, FALSE, FALSE, 0);

	/* Ch.3 StopBit */
	ch3_ext_stopbit_label = gtk_label_new("StopBit");
	gtk_widget_show(ch3_ext_stopbit_label);
	gtk_container_add(GTK_CONTAINER(ch3_ext_stopbit_hbox), ch3_ext_stopbit_label);

	ch3_ext_stopbit_combo = gtk_combo_box_entry_new_text();
	gtk_widget_show(ch3_ext_stopbit_combo);
	gtk_container_add(GTK_CONTAINER(ch3_ext_stopbit_hbox), ch3_ext_stopbit_combo);
	gtk_widget_set_size_request(ch3_ext_stopbit_combo, 40, -1);
	for (i = 0; i < NELEMENTS(ext_stopbit_str); i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(ch3_ext_stopbit_combo), ext_stopbit_str[i]);
	}
	ch3_ext_stopbit_entry = gtk_bin_get_child(GTK_BIN(ch3_ext_stopbit_combo));
	gtk_widget_show(ch3_ext_stopbit_entry);
	gtk_editable_set_editable(GTK_EDITABLE(ch3_ext_stopbit_entry), FALSE);
	switch(np2oscfg.com[2].param & 0xC0) {
	case 0x80:
		gtk_entry_set_text(GTK_ENTRY(ch3_ext_stopbit_entry), "1.5");
		break;
	case 0xC0:
		gtk_entry_set_text(GTK_ENTRY(ch3_ext_stopbit_entry), "2");
		break;
	default:
		gtk_entry_set_text(GTK_ENTRY(ch3_ext_stopbit_entry), "1");
		break;
	}
	if (np2oscfg.com[2].direct)
		gtk_widget_set_sensitive(ch3_ext_stopbit_combo, FALSE);

	return root_widget;
}

void
create_serial_dialog(void)
{
	GtkWidget *serial_dialog;
	GtkWidget *main_vbox;
	GtkWidget *notebook;
	GtkWidget *ch1_note;
	GtkWidget *pc9861k_note;
	GtkWidget *ch2_note;
	GtkWidget *ch3_note;
	GtkWidget *confirm_widget;
	GtkWidget *ok_button;
	GtkWidget *cancel_button;

	uninstall_idle_process();

	serial_dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(serial_dialog), "Serial option");
	gtk_window_set_position(GTK_WINDOW(serial_dialog), GTK_WIN_POS_CENTER);
	gtk_window_set_modal(GTK_WINDOW(serial_dialog), TRUE);
	gtk_window_set_resizable(GTK_WINDOW(serial_dialog), FALSE);

	g_signal_connect(G_OBJECT(serial_dialog), "destroy",
	    G_CALLBACK(dialog_destroy), NULL);

	main_vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(main_vbox);
	gtk_container_add(GTK_CONTAINER(serial_dialog), main_vbox);

	notebook = gtk_notebook_new();
	gtk_widget_show(notebook);
	gtk_box_pack_start(GTK_BOX(main_vbox), notebook, TRUE, TRUE, 0);

	/* "Ch.1" note */
	ch1_note = create_ch1_note();
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), ch1_note, gtk_label_new("Ch.1"));

	/* "PC-9861K" note */
	pc9861k_note = create_pc9861k_note();
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), pc9861k_note, gtk_label_new("PC-9861K"));

	/* "Ch.2" note */
	ch2_note = create_ch2_note();
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), ch2_note, gtk_label_new("Ch.2"));

	/* "Ch.3" note */
	ch3_note = create_ch3_note();
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), ch3_note, gtk_label_new("Ch.3"));

	/*
	 * OK, Cancel button
	 */
	confirm_widget = gtk_hbox_new(FALSE, 5);
	gtk_widget_show(confirm_widget);
	gtk_box_pack_start(GTK_BOX(main_vbox), confirm_widget, FALSE, FALSE, 5);

	cancel_button = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	gtk_widget_show(cancel_button);
	gtk_box_pack_end(GTK_BOX(confirm_widget), cancel_button, FALSE, FALSE, 0);
#if GTK_MAJOR_VERSION > 2 || (GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION >= 18)
	gtk_widget_set_can_default(cancel_button, TRUE);
#else
	GTK_WIDGET_SET_FLAGS(cancel_button, GTK_CAN_DEFAULT);
#endif
	g_signal_connect_swapped(G_OBJECT(cancel_button), "clicked",
	    G_CALLBACK(gtk_widget_destroy), G_OBJECT(serial_dialog));

	ok_button = gtk_button_new_from_stock(GTK_STOCK_OK);
	gtk_widget_show(ok_button);
	gtk_box_pack_end(GTK_BOX(confirm_widget), ok_button, FALSE, FALSE, 0);
#if GTK_MAJOR_VERSION > 2 || (GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION >= 18)
	gtk_widget_set_can_default(ok_button, TRUE);
	gtk_widget_has_default(ok_button);
#else
	GTK_WIDGET_SET_FLAGS(ok_button, GTK_CAN_DEFAULT);
	GTK_WIDGET_SET_FLAGS(ok_button, GTK_HAS_DEFAULT);
#endif
	g_signal_connect(G_OBJECT(ok_button), "clicked",
	    G_CALLBACK(ok_button_clicked), (gpointer)serial_dialog);
	gtk_widget_grab_default(ok_button);

	gtk_widget_show_all(serial_dialog);
}
