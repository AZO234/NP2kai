#include	<stdio.h>
#include	<string.h>
#include	"common.h"
#include	"dosio.h"

#define	MAXKEY		256
#define	MAXKEYSIZE	4096

	char	keywork[MAXKEYSIZE];
	char	*strkey[MAXKEY];
	char	key_hit[MAXKEY];
	char	key_byte[MAXKEY];

	int		keys;
	int		keypos;

static void milstr_textadj(char *str) {

	int				qout = 0;
	unsigned char	*p, *q;
	unsigned char	c;

	p = (unsigned char *)str;
	q = p;

	while(1) {
		if ((c = *p++) == '\0') {
			break;
		}
		if (!qout) {
			if (c <= ' ') {
				continue;
			}
			if (c == ';') {
				break;
			}
		}
		if (c == '\"') {
			if ((!qout) || (*p != '\"')) {
				qout ^= 1;
				continue;
			}
			p++;
		}
		*q++ = c;
	}
	*q = '\0';
}

// ---------------------------------------------------

char	nowsym[80] = "";
char	linestr[80] = "";
int		linestrp = 0;

void ldata_flash(void) {

	int		len;
	int		tabs;

	if (!linestrp) {
		return;
	}

	tabs = 1;
	len = strlen(nowsym);
	if (len < 15) {
		tabs += ((15 - len) >> 2);
	}
	while(tabs--) {
		nowsym[len++] = '\t';
	}
	nowsym[len] = '\0';
	printf(nowsym);
	puts(linestr);
	nowsym[0] = 0;
	linestrp = 0;
}

void ldata_set(BYTE dat) {

	if (	linestrp) {
		linestr[linestrp++] = ',';
	}
	else {
		strcpy(linestr, "db\t");
		linestrp = 3;
	}
	sprintf(linestr + linestrp, "0%02xh", dat);
	linestrp += 4;
	if (linestrp >= 58) {
		ldata_flash();
	}
}

void ldata_symbolset(char *sym) {

	strcpy(nowsym, sym);
}

// ---------------------------------------------------

int abstrcmp(BYTE *str) {

	int		i;

	for (i=0; i<keys; i++) {
		if (!strcmp(str, strkey[i])) {
			return(i+1);
		}
	}
	return(0);
}

int xstrcmp(BYTE *str) {

	int		i;
	BYTE	*p, *q;

	for (i=0; i<keys; i++) {
		p = str;
		q = strkey[i];
		while(1) {
			if (!(*q)) {
				return(i+1);
			}
			if (*p++ != *q++) {
				break;
			}
		}
	}
	return(0);
}



// ---------------------------------------------------

int packingout(BYTE *p) {

	BYTE	c;
	WORD	code;
	int		ret = 0;
	int		key;

	while(*p) {
		key = xstrcmp(p);
		if (key) {
			if (key < 32) {
				ldata_set((BYTE)key);
				ret++;
			}
			else if (key < (256+32)) {
				ldata_set(0x81);
				ldata_set((BYTE)(key - 32));
				ret += 2;
			}
			p += strlen(strkey[key-1]);
			key_hit[key-1]++;
			continue;
		}
		c = *p++;
		if (((c >= 0x80) && (c < 0xa0)) ||
			((c >= 0xe0) && (c < 0xfc))) {
			if (!p[0]) {
				break;
			}
			code = sjis2jis(((*p++) << 8) + c);
			if (code) {
				ldata_set((BYTE)(code | 0x80));
				ldata_set((BYTE)(code >> 8));
				ret += 2;
			}
		}
		else {
			if (c == '\\') {
				if (p[0] == '\\') {
					ldata_set('\\');
					ret++;
					p++;
				}
				else if (p[0] == 'n') {
					ldata_set(0x7f);
					ret++;
					p++;
				}
				else if (p[0] == 'z') {
					ldata_set(0x9f);
					ret++;
					p++;
				}
				else if (p[0] == 'x') {
					ldata_set(0x80);
					ldata_set(0x9b);
					ret+=2;
					p++;
				}
			}
			else {
				if (c & 0x80) {
					ldata_set(0x80);
					ret++;
				}
				ldata_set(c);
				ret++;
			}
		}
	}
	ldata_set(0);
	ldata_flash();
	return(ret + 1);
}

// ---------------------------------------------------

void keyinit(void) {

	memset(keywork, 0, sizeof(keywork));
	memset(key_hit, 0, sizeof(key_hit));
	memset(key_byte, 0, sizeof(key_byte));
	keys = 0;
	keypos = 0;
}

void checkkey(BYTE *str) {

	int		len;

	if ((str[0] != '#') || (str[1] != '=')) {
		return;
	}
	str += 2;
	if (keys >= MAXKEY) {
		return;
	}
	len = strlen(str) + 1;
	if ((keypos + len) > MAXKEYSIZE) {
		return;
	}
	strkey[keys] = keywork + keypos;
	memcpy(strkey[keys], str, len);
	key_byte[keys] = (BYTE)packingout(str);
	keypos += len;
	keys++;
}

void txtpacking(BYTE *str) {

	char	*p;
	int		keymatch;
	int		size;

	p = str;
	if ((str[0] == '#') && (str[1] == '=')) {
		return;
	}
	while(*p) {
		if (*p == '=') {
			break;
		}
		p++;
	}
	if (*p != '=') {
		return;
	}
	*p++ = '\0';
	ldata_symbolset(str);

	keymatch = abstrcmp(p);
	if (keymatch--) {
		size = 0;
		while(keymatch--) {
			size += (int)key_byte[keymatch];
		}
		sprintf(linestr, "equ\t\tKEYDATA + %d", size);
		linestrp = 1;
		ldata_flash();
	}
	else {
		packingout(p);
	}
}

// ----------------------------------------- メイン

void main(int argc, BYTE *argv[], BYTE *envp[]) {

	FILEH	fh;
	char	str[256];
	int		i;
	int		size;

	keyinit();

	if (argc < 2) {
		printf("入力名を指定して下さい.\n");
		return;
	}

	if ((fh = file_open(argv[1])) == -1) {
		printf("ファイルが見つかりません.\n");
		return;
	}

	puts("KEYDATA\tlabel\tbyte");

	while(file_lread(fh, str, 255)) {
		milstr_textadj(str);
		checkkey(str);
	}

	puts("\nKEYTABLE\tlabel\tword");
	size = 0;
	for (i=0; i<keys; i++) {
		printf("\t\t\t\tdw\toffset KEYDATA + %d\n", size);
		size += (int)key_byte[i];
	}

	file_seek(fh, 0, 0);

	puts("");

	while(file_lread(fh, str, 255)) {
		milstr_textadj(str);
		txtpacking(str);
	}
	file_close(fh);
}

