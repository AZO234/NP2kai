#include	"compiler.h"
#include	"dosio.h"
#include	"textfile.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"keystat.h"
#include	"keystat.tbl"
#include	"softkbd.h"


typedef struct {
	UINT8	ref[0x80];
	UINT8	extkey;
	UINT8	mouselast;
	UINT8	padding;
	UINT8	d_up;
	UINT8	d_dn;
	UINT8	d_lt;
	UINT8	d_rt;
} KEYSTAT;

		NKEYTBL		nkeytbl;
		KEYCTRL		keyctrl;
static	KEYSTAT		keystat;


void keystat_initialize(void) {

	OEMCHAR	path[MAX_PATH];

	ZeroMemory(&keyctrl, sizeof(keyctrl));
	keyctrl.keyrep = 0x21;
	keyctrl.capsref = NKEYREF_NC;
	keyctrl.kanaref = NKEYREF_NC;

	ZeroMemory(&keystat, sizeof(keystat));
	FillMemory(keystat.ref, sizeof(keystat.ref), NKEYREF_NC);
	keystat_tblreset();
	getbiospath(path, OEMTEXT("key.txt"), NELEMENTS(path));
	keystat_tblload(path);
}

void keystat_tblreset(void) {

	UINT	i;

	ZeroMemory(&nkeytbl, sizeof(nkeytbl));
	for (i=0; i<0x80; i++) {
		nkeytbl.key[i].keys = 1;
		nkeytbl.key[i].key[0] = (UINT8)i;
	}
	for (i=0; i<0x10; i++) {
		nkeytbl.key[i+0x80].keys = 1;
		nkeytbl.key[i+0x80].key[0] = (UINT8)(i + 0xf0);
	}
}

void keystat_tblset(REG8 ref, const UINT8 *key, UINT cnt) {

	NKEYM	*nkey;

	if ((ref >= NKEY_USER) && (ref < (NKEY_USER + NKEY_USERKEYS))) {
		nkey = (NKEYM *)(nkeytbl.user + (ref - NKEY_USER));
		cnt = min(cnt, 15);
	}
	else if (ref < NKEY_SYSTEM) {
		nkey = (NKEYM *)(nkeytbl.key + ref);
		cnt = min(cnt, 3);
	}
	else {
		return;
	}
	nkey->keys = (UINT8)cnt;
	if (cnt) {
		CopyMemory(nkey->key, key, cnt);
	}
}


// ---- config...

static const OEMCHAR str_userkey1[] = OEMTEXT("userkey1");
static const OEMCHAR str_userkey2[] = OEMTEXT("userkey2");

static REG8 searchkeynum(const OEMCHAR *str, BOOL user) {

const KEYNAME	*n;
const KEYNAME	*nterm;

	n = s_keyname;
	nterm = s_keyname + NELEMENTS(s_keyname);
	while(n < nterm) {
		if (!milstr_cmp(n->str, str)) {
			return(n->num);
		}
		n++;
	}
	if (user) {
		if (!milstr_cmp(str_userkey1, str)) {
			return(NKEY_USER + 0);
		}
		if (!milstr_cmp(str_userkey2, str)) {
			return(NKEY_USER + 1);
		}
	}
	return(0xff);
}

void keystat_tblload(const OEMCHAR *filename) {

	TEXTFILEH	tfh;
	OEMCHAR		work[256];
	OEMCHAR		*p;
	OEMCHAR		*q;
	OEMCHAR		*r;
	UINT8		ref;
	UINT8		key[15];
	UINT		cnt;

	tfh = textfile_open(filename, 0x800);
	if (tfh == NULL) {
		goto kstbl_err;
	}
	while(textfile_read(tfh, work, NELEMENTS(work)) == SUCCESS) {
		p = milstr_nextword(work);
		q = milstr_chr(p, '\t');
		if (q == NULL) {
			q = milstr_chr(p, '=');
		}
		if (q == NULL) {
			continue;
		}
		*q++ = '\0';
		r = milstr_chr(p, ' ');
		if (r != NULL) {
			*r = '\0';
		}
		ref = searchkeynum(p, TRUE);
		if (ref == 0xff) {
			continue;
		}
		cnt = 0;
		while((q) && (cnt < 16)) {
			p = milstr_nextword(q);
			q = milstr_chr(p, ' ');
			if (q != NULL) {
				*q++ = '\0';
			}
			key[cnt] = searchkeynum(p, FALSE);
			if (key[cnt] != 0xff) {
				cnt++;
			}
		}
		keystat_tblset(ref, key, cnt);
	}
	textfile_close(tfh);

kstbl_err:
	return;
}


// ----

static REG8 getledstat(void) {

	REG8	ret;

	ret = 0;
	if (keyctrl.kanaref != NKEYREF_NC) {
		ret |= 8;
	}
	if (keyctrl.capsref != NKEYREF_NC) {
		ret |= 4;
	}
	return(ret);
}

static void reloadled(void) {

	keyctrl.kanaref = keystat.ref[0x72];
	keyctrl.capsref = keystat.ref[0x71];
#if defined(SUPPORT_SOFTKBD)
	softkbd_led(getledstat());
#endif
}

void keystat_ctrlreset(void) {

	keyctrl.reqparam = 0;
	keystat.ref[0x72] = keyctrl.kanaref;
	keystat.ref[0x71] = keyctrl.capsref;
#if defined(SUPPORT_SOFTKBD)
	softkbd_led(getledstat());
#endif
}

void keystat_ctrlsend(REG8 dat) {

	if (!keyctrl.reqparam) {
		keyctrl.mode = dat;
		switch(dat) {
#if defined(SUPPORT_PC9801_119)
			case 0x95:
#endif
			case 0x9c:
			case 0x9d:
				keyctrl.reqparam = 1;
				keyboard_ctrl(0xfa);
				break;

#if defined(SUPPORT_PC9801_119)
			case 0x96:
				keyboard_ctrl(0xfa);
				keyboard_ctrl(0xa0);
				keyboard_ctrl(0x83);
				break;
#endif

			case 0x9f:
				keyboard_ctrl(0xfa);
				keyboard_ctrl(0xa0);
				keyboard_ctrl(0x80);
				break;

			default:
				keyboard_ctrl(0xfc);
				break;
		}
	}
	else {
		switch(keyctrl.mode) {
#if defined(SUPPORT_PC9801_119)
			case 0x95:
				keyctrl.kbdtype = dat;
				keyboard_ctrl(0xfa);
				break;
#endif
			case 0x9c:
				keyboard_ctrl(0xfa);
				break;

			case 0x9d:
				if (dat == 0x60) {
					keyboard_ctrl(0xfa);
					keyboard_ctrl((REG8)(0x70 + getledstat()));
				}
				else if ((dat & 0xf0) == 0x70) {
					keyboard_ctrl(0xfa);
					keystat.ref[0x72] = (dat & 8)?NKEYREF_uPD8255:NKEYREF_NC;
					keystat.ref[0x71] = (dat & 4)?NKEYREF_uPD8255:NKEYREF_NC;
					reloadled();
				}
				break;
		}
		keyctrl.reqparam = 0;
	}
}


// ----

void keystat_down(const UINT8 *key, REG8 keys, REG8 ref) {

	UINT8	keydata;
	UINT8	keycode;
	REG8	data;

	while(keys--) {
		keydata = *key++;
		keycode = (keydata & 0x7f);
		if (keycode < 0x70) {
#if 1												// 05/02/04
			if (keystat.ref[keycode] != NKEYREF_NC) {
				if (!(kbexflag[keycode] & KBEX_NONREP)) {
					keyboard_send((REG8)(keycode + 0x80));
					keystat.ref[keycode] = NKEYREF_NC;
				}
			}
			if (keystat.ref[keycode] == NKEYREF_NC) {
				keyboard_send(keycode);
			}
#else
			if ((keystat.ref[keycode] == NKEYREF_NC) ||
				(!(kbexflag[keycode] & KBEX_NONREP))) {
				keyboard_send(keycode);
			}
#endif
			keystat.ref[keycode] = ref;
		}
		else {
#if defined(SUPPORT_PC9801_119)
			if (keyctrl.kbdtype != 0x03)
#endif
			{
				if (keycode == 0x7d) {
					keycode = 0x70;
				}
				else if (keycode >= 0x75) {
					continue;
				}
			}
			if ((np2cfg.XSHIFT) &&
				(((keycode == 0x70) && (np2cfg.XSHIFT & 1)) ||
				((keycode == 0x74) && (np2cfg.XSHIFT & 2)) ||
				((keycode == 0x73) && (np2cfg.XSHIFT & 4)))) {
				keydata |= 0x80;
			}
			if (!(keydata & 0x80)) {			// シフト
				if (keystat.ref[keycode] == NKEYREF_NC) {
					keystat.ref[keycode] = ref;
					keyboard_send(keycode);
				}
			}
			else {								// シフトメカニカル処理
				if (keystat.ref[keycode] == NKEYREF_NC) {
					keystat.ref[keycode] = ref;
					data = keycode;
				}
				else {
					keystat.ref[keycode] = NKEYREF_NC;
					data = (REG8)(keycode + 0x80);
				}
				keyboard_send(data);
			}
			if ((keycode == 0x71) || (keycode == 0x72)) {
				reloadled();
			}
		}
	}
}

void keystat_up(const UINT8 *key, REG8 keys, REG8 ref) {

	UINT8	keydata;
	UINT8	keycode;

	while(keys--) {
		keydata = *key++;
		keycode = (keydata & 0x7f);
		if (keycode < 0x70) {
			if (keystat.ref[keycode] == ref) {
				keystat.ref[keycode] = NKEYREF_NC;
				keyboard_send((REG8)(keycode + 0x80));
			}
		}
		else {
#if defined(SUPPORT_PC9801_119)
			if (keyctrl.kbdtype != 0x03)
#endif
			{
				if (keycode == 0x7d) {
					keycode = 0x70;
				}
				else if (keycode >= 0x75) {
					continue;
				}
			}
			if ((np2cfg.XSHIFT) &&
				(((keycode == 0x70) && (np2cfg.XSHIFT & 1)) ||
				((keycode == 0x74) && (np2cfg.XSHIFT & 2)) ||
				((keycode == 0x73) && (np2cfg.XSHIFT & 4)))) {
				keydata |= 0x80;
			}
			if (!(keydata & 0x80)) {			// シフト
				if (keystat.ref[keycode] != NKEYREF_NC) {
					keystat.ref[keycode] = NKEYREF_NC;
					keyboard_send((REG8)(keycode + 0x80));
					if ((keycode == 0x71) || (keycode == 0x72)) {
						reloadled();
					}
				}
			}
		}
	}
}

void keystat_resendstat(void) {

	REG8	i;

	for (i=0; i<0x80; i++) {
		if (keystat.ref[i] != NKEYREF_NC) {
			keyboard_send(i);
		}
	}
}


// ----

void keystat_keydown(REG8 ref) {

	UINT8	shift;
const NKEYM	*nkey;

	if ((ref >= NKEY_USER) && (ref < (NKEY_USER + NKEY_USERKEYS))) {
		nkey = (NKEYM *)(nkeytbl.user + (ref - NKEY_USER));
		keystat_down(nkey->key, nkey->keys, NKEYREF_USER);
	}
	else if (ref < NKEY_SYSTEM) {
		if (np2cfg.KEY_MODE) {
			shift = kbexflag[ref];
			if (shift & KBEX_JOYKEY) {
				keystat.extkey |= (1 << (shift & 7));
				return;
			}
		}
		nkey = (NKEYM *)(nkeytbl.key + ref);
		keystat_down(nkey->key, nkey->keys, ref);
	}
}

void keystat_keyup(REG8 ref) {

	UINT8	shift;
const NKEYM	*nkey;

	if ((ref >= NKEY_USER) && (ref < (NKEY_USER + NKEY_USERKEYS))) {
		nkey = (NKEYM *)(nkeytbl.user + (ref - NKEY_USER));
		keystat_up(nkey->key, nkey->keys, NKEYREF_USER);
	}
	else if (ref < NKEY_SYSTEM) {
		if (np2cfg.KEY_MODE) {
			shift = kbexflag[ref];
			if (shift & KBEX_JOYKEY) {
				keystat.extkey &= ~(1 << (shift & 7));
				return;
			}
		}
		nkey = (NKEYM *)(nkeytbl.key + ref);
		keystat_up(nkey->key, nkey->keys, ref);
	}
}

void keystat_releaseref(REG8 ref) {

	REG8	i;

	for (i=0; i<0x80; i++) {
		if (keystat.ref[i] == ref) {
			keystat.ref[i] = NKEYREF_NC;
			keyboard_send((REG8)(i + 0x80));
		}
	}
}

void keystat_resetjoykey(void) {

	REG8	i;

	keystat.extkey = 0;
	for (i=0; i<0x80; i++) {
		if (kbexflag[i] & KBEX_JOYKEY) {
			keystat_releaseref(i);
		}
	}
}


void keystat_releasekey(REG8 key) {

	key &= 0x7f;
	if ((key != 0x71) && (key != 0x72)) {
		if (keystat.ref[key] != NKEYREF_NC) {
			keystat.ref[key] = NKEYREF_NC;
			keyboard_send((REG8)(key + 0x80));
		}
	}
}

void keystat_allrelease(void) {

	REG8	i;

	for (i=0; i<0x80; i++) {
		keystat_releasekey(i);
	}
}


REG8 keystat_getjoy(void) {

	return(~keystat.extkey);
}

REG8 keystat_getmouse(SINT16 *x, SINT16 *y) {

	REG8	btn;
	UINT8	acc;
	SINT16	tmp;
	REG8	ret;

	btn = ~keystat.extkey;
	acc = btn | keystat.mouselast;
	keystat.mouselast = (UINT8)btn;
	tmp = 0;
	if (!(btn & 1)) {
		tmp -= mousedelta[keystat.d_up];
	}
	if (!(acc & 1)) {
		if (keystat.d_up < MOUSESTEPMAX) {
			keystat.d_up++;
		}
	}
	else {
		keystat.d_up = 0;
	}
	if (!(btn & 2)) {
		tmp += mousedelta[keystat.d_dn];
	}
	if (!(acc & 2)) {
		if (keystat.d_dn < MOUSESTEPMAX) {
			keystat.d_dn++;
		}
	}
	else {
		keystat.d_dn = 0;
	}
	*y += tmp;

	tmp = 0;
	if (!(btn & 4)) {
		tmp -= mousedelta[keystat.d_lt];
	}
	if (!(acc & 4)) {
		if (keystat.d_lt < MOUSESTEPMAX) {
			keystat.d_lt++;
		}
	}
	else {
		keystat.d_lt = 0;
	}
	if (!(btn & 8)) {
		tmp += mousedelta[keystat.d_rt];
	}
	if (!(acc & 8)) {
		if (keystat.d_rt < MOUSESTEPMAX) {
			keystat.d_rt++;
		}
	}
	else {
		keystat.d_rt = 0;
	}
	*x += tmp;

	ret = 0x5f;
	ret += (btn & 0x10) << 3;
	ret += (btn & 0x20);
	return(ret);
}


// ----

// キーコード変更

static REG8 cnvnewcode(REG8 oldcode) {

	switch(oldcode) {
		case 0x71:				// 通常caps
			return(0x81);

		case 0x72:				// 通常カナ
			return(0x82);

		case 0x79:				// メカニカルロックcaps
			return(0x71);

		case 0x7a:				// メカニカルロックcaps
			return(0x72);

		case 0x76:
			return(0x90);		// NKEY_USER + 0

		case 0x77:
			return(0x91);		// NKEY_USER + 1

		default:
			return(oldcode);
	}
}

void keystat_senddata(REG8 data) {

	REG8	keycode;

	keycode = cnvnewcode((REG8)(data & 0x7f));
	if (!(data & 0x80)) {
		keystat_keydown(keycode);
	}
	else {
		keystat_keyup(keycode);
	}
}

void keystat_forcerelease(REG8 data) {

	REG8	keycode;

	keycode = cnvnewcode((REG8)(data & 0x7f));
	keystat_releasekey(keycode);
}

