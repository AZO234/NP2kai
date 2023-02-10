#include	<compiler.h>
#include	<common/bmpdata.h>
#include	<generic/dipswbmp.h>
#include	"dipswbmp.res"

#if defined(USE_RESOURCE_BMP)

typedef struct {
	BMPDATA	inf;
	UINT8	*ptr;
	int		yalign;
} DIPBMP;


static UINT8 *getbmp(const UINT8 *dat, DIPBMP *dipbmp) {

	BMPFILE	*ret;

	ret = (BMPFILE *)bmpdata_solvedata(dat);
	if (ret == NULL) {
		goto gb_err1;
	}
	if ((ret->bfType[0] != 'B') || (ret->bfType[1] != 'M')) {
		goto gb_err2;
	}
	if (bmpdata_getinfo((BMPINFO *)(ret + 1), &dipbmp->inf) != SUCCESS) {
		goto gb_err2;
	}
	dipbmp->yalign = bmpdata_getalign((BMPINFO *)(ret + 1));
	dipbmp->ptr = ((UINT8 *)ret) + (LOADINTELDWORD(ret->bfOffBits));
	if (dipbmp->inf.height < 0) {
		dipbmp->inf.height *= -1;
	}
	else {
		dipbmp->ptr += (dipbmp->inf.height - 1) * dipbmp->yalign;
		dipbmp->yalign *= -1;
	}
	return((UINT8 *)ret);

gb_err2:
	_MFREE(ret);

gb_err1:
	return(NULL);
}

static UINT8 *getbmpres(const OEMCHAR* resname, DIPBMP *dipbmp) {
	
#if defined(_WIN32)
	BMPFILE	*ret;
    HRSRC hRsrc;
	HANDLE hRes;
	LPVOID hBmpRes;
	int ressize;

	hRsrc = FindResource(NULL, resname, OEMTEXT("RAWBMP"));
	if (hRsrc == NULL) {
		goto gb_err1;
	}
	hRes = LoadResource(NULL, hRsrc);
	if (hRes == NULL) {
		goto gb_err1;
	}
	hBmpRes = LockResource(hRes);
	if (hBmpRes == NULL) {
		goto gb_err1;
	}
	ressize = SizeofResource(NULL, hRsrc);
	ret = (BMPFILE*)_MALLOC(ressize, "res");
	if (ret == NULL) {
		goto gb_err1;
	}
	memcpy(ret, hBmpRes, ressize);
	if ((ret->bfType[0] != 'B') || (ret->bfType[1] != 'M')) {
		goto gb_err2;
	}
	if (bmpdata_getinfo((BMPINFO *)(ret + 1), &dipbmp->inf) != SUCCESS) {
		goto gb_err2;
	}
	dipbmp->yalign = bmpdata_getalign((BMPINFO *)(ret + 1));
	dipbmp->ptr = ((UINT8 *)ret) + (LOADINTELDWORD(ret->bfOffBits));
	if (dipbmp->inf.height < 0) {
		dipbmp->inf.height *= -1;
	}
	else {
		dipbmp->ptr += (dipbmp->inf.height - 1) * dipbmp->yalign;
		dipbmp->yalign *= -1;
	}
	return((UINT8 *)ret);

gb_err2:
	_MFREE(ret);

gb_err1:
#endif
	return(NULL);
}

static void line4x(const DIPBMP *dipbmp, int x, int y, int l, UINT8 c) {

	UINT8	*ptr;

	ptr = dipbmp->ptr + (y * dipbmp->yalign);
	while(l--) {
		if (x & 1) {
			ptr[x/2] &= 0xf0;
			ptr[x/2] |= c;
		}
		else {
			ptr[x/2] &= 0x0f;
			ptr[x/2] |= (c << 4);
		}
		x++;
	}
}

static void line4y(const DIPBMP *dipbmp, int x, int y, int l, UINT8 c) {

	UINT8	*ptr;
	UINT8	mask;

	ptr = dipbmp->ptr + (x / 2) + (y * dipbmp->yalign);
	if (x & 1) {
		mask = 0xf0;
	}
	else {
		mask = 0x0f;
		c <<= 4;
	}
	while(l--) {
		*ptr &= mask;
		*ptr |= c;
		ptr += dipbmp->yalign;
	}
}


// ---- jumper

static void setjumperx(const DIPBMP *dipbmp, int x, int y) {

	int		i;

	x *= 9;
	y *= 9;
	for (i=0; i<2; i++) {
		line4x(dipbmp, x, y+0+i, 19, 0);
		line4x(dipbmp, x, y+8+i, 19, 0);
		line4y(dipbmp, x+ 0+i, y, 9, 0);
		line4y(dipbmp, x+17+i, y, 9, 0);
	}
}

static void setjumpery(const DIPBMP *dipbmp, int x, int y) {

	int		i;

	x *= 9;
	y *= 9;
	for (i=0; i<2; i++) {
		line4x(dipbmp, x, y+ 0+i, 9, 0);
		line4x(dipbmp, x, y+17+i, 9, 0);
		line4y(dipbmp, x+0+i, y, 19, 0);
		line4y(dipbmp, x+8+i, y, 19, 0);
	}
}

static void setjumperxex(const DIPBMP *dipbmp, int ofsx, int ofsy, int x, int y, UINT8 c) {

	int		i;

	x *= 9;
	y *= 9;
	x += ofsx;
	y += ofsy;
	for (i=0; i<2; i++) {
		line4x(dipbmp, x, y+0+i, 19, c);
		line4x(dipbmp, x, y+8+i, 19, c);
		line4y(dipbmp, x+ 0+i, y, 9, c);
		line4y(dipbmp, x+17+i, y, 9, c);
	}
}

static void setjumperyex(const DIPBMP *dipbmp, int ofsx, int ofsy, int x, int y, UINT8 c) {

	int		i;

	x *= 9;
	y *= 9;
	x += ofsx;
	y += ofsy;
	for (i=0; i<2; i++) {
		line4x(dipbmp, x, y+ 0+i, 9, c);
		line4x(dipbmp, x, y+17+i, 9, c);
		line4y(dipbmp, x+0+i, y, 19, c);
		line4y(dipbmp, x+8+i, y, 19, c);
	}
}


// ---- pc-9861k

static void setdip9861(const DIPBMP *dipbmp, const DIP9861 *pos, UINT8 cfg) {

	int		x;
	UINT	c;
	int		y;
	int		l;

	x = (pos->x * 9) + 1;
	c = pos->cnt;
	do {
		y = pos->y * 9 + ((cfg & 0x01)?5:9);
		l = 0;
		do {
			line4x(dipbmp, x, y + l, 7, 0);
		} while(++l < 3);
		x += 9;
		cfg >>= 1;
	} while(--c);
}

static void setjmp9861(const DIPBMP *dipbmp, const DIP9861 *pos, UINT8 cfg) {

	int		x;
	int		y;
	UINT	c;

	x = pos->x;
	y = pos->y;
	c = pos->cnt;
	do {
		if (cfg & 0x01) {
			setjumpery(dipbmp, x, y);
		}
		x++;
		cfg >>= 1;
	} while(--c);
}

UINT8 *dipswbmp_get9861(const UINT8 *s, const UINT8 *j) {

	UINT8	*ret;
	DIPBMP	dipbmp;
	int		i;

	ret = getbmp(bmp9861, &dipbmp);
	if (ret) {
		for (i=0; i<3; i++) {
			setdip9861(&dipbmp, dip9861s + i, s[i]);
		}
		for (i=0; i<6; i++) {
			setjmp9861(&dipbmp, dip9861j + i, j[i]);
		}
	}
	return(ret);
}


// ---- sound

static void setsnd26io(const DIPBMP *dipbmp, int px, int py, UINT8 cfg) {

	setjumpery(dipbmp, px + ((cfg & 0x10)?1:0), py);
}

static void setsnd26int(const DIPBMP *dipbmp, int px, int py, UINT8 cfg) {

	setjumperx(dipbmp, px + ((cfg & 0x80)?0:1), py);
	setjumperx(dipbmp, px + ((cfg & 0x40)?0:1), py + 1);
}

static void setsnd26rom(const DIPBMP *dipbmp, int px, int py, UINT8 cfg) {

	cfg &= 7;
	if (cfg >= 4) {
		cfg = 4;
	}
	setjumpery(dipbmp, px + cfg, py);
}

UINT8 *dipswbmp_getsnd26(UINT8 cfg) {

	UINT8	*ret;
	DIPBMP	dipbmp;

	ret = getbmp(bmp26, &dipbmp);
	if (ret) {
		setsnd26io(&dipbmp, 15, 1, cfg);
		setsnd26int(&dipbmp, 9, 1, cfg);
		setsnd26rom(&dipbmp, 2, 1, cfg);
	}
	return(ret);
}

UINT8 *dipswbmp_getsnd86(UINT8 cfg) {

	UINT8	*ret;
	DIPBMP	dipbmp;
	int		i;
	int		x;
	int		y;
	int		l;

	ret = getbmp(bmp86, &dipbmp);
	if (ret) {
		for (i=0; i<8; i++) {
			x = i * 8 + 17;
			y = (cfg & (1 << i))?16:9;
			l = 0;
			do {
				line4x(&dipbmp, x, y + l, 6, 3);
			} while(++l < 7);
		}
	}
	return(ret);
}

UINT8 *dipswbmp_getsndspb(UINT8 cfg, UINT8 vrc) {

	UINT8	*ret;
	DIPBMP	dipbmp;

	ret = getbmp(bmpspb, &dipbmp);
	if (ret) {
		setsnd26int(&dipbmp, 2, 1, cfg);
		setsnd26io(&dipbmp, 10, 1, cfg);
		setsnd26rom(&dipbmp, 14, 1, cfg);
		if (cfg & 0x20) {
			setjumpery(&dipbmp, 7, 1);
		}
		setjumperx(&dipbmp, ((vrc & 2)?21:22), 1);
		setjumperx(&dipbmp, ((vrc & 1)?21:22), 2);
	}
	return(ret);
}

UINT8 *dipswbmp_getmpu(UINT8 cfg) {

	UINT8	*ret;
	DIPBMP	dipbmp;
	int		i;
	int		x;
	int		y;
	int		l;

	ret = getbmp(bmpmpu, &dipbmp);
	if (ret) {
		for (i=0; i<4; i++) {
			x = (i + 2) * 9 + 1;
			y = (1 * 9) + ((cfg & (0x80 >> i))?5:9);
			l = 0;
			do {
				line4x(&dipbmp, x, y + l, 7, 2);
			} while(++l < 3);
		}
		setjumpery(&dipbmp, 9 + 3 - (cfg & 3), 1);
	}
	return(ret);
}
UINT8 *dipswbmp_getsmpu(UINT8 cfg) {

	UINT8	*ret;
	DIPBMP	dipbmp;
	int		i;
	int		x;
	int		y;
	int		l;

	ret = getbmp(bmpmpu, &dipbmp);
	if (ret) {
		for (i=0; i<4; i++) {
			x = (i + 2) * 9 + 1;
			y = (1 * 9) + ((cfg & (0x10 << i))?5:9); // 左右逆なだけっぽい
			l = 0;
			do {
				line4x(&dipbmp, x, y + l, 7, 2);
			} while(++l < 3);
		}
		setjumpery(&dipbmp, 9 + 3 - (cfg & 3), 1);
	}
	return(ret);
}

UINT8 *dipswbmp_getsnd118(UINT16 snd118io, UINT8 snd118dma, UINT8 snd118irqf, UINT8 snd118irqp, UINT8 snd118irqm, UINT8 snd118rom) {

	UINT8	*ret;
	DIPBMP	dipbmp;

	ret = getbmpres(OEMTEXT("JUMPER118"), &dipbmp);
	if (ret) {
		int jmpflag[2];
		setjumperyex(&dipbmp, 18, 9, 0, 0, 15); // #12 常時OFF どこにも影響しない？
		setjumperyex(&dipbmp, 18, 9, 1, 0, 15); // #11 YMF297の制御？
		setjumperyex(&dipbmp, 18, 9, 2, snd118irqm==0xff ? 1 : 0, 15); // #10
		
		// PCM INT (#8,#9)=(OFF,OFF):INT5(IRQ12), (OFF,ON):INT1(IRQ5), (ON,OFF):INT41(IRQ10), (ON,ON):INT0(IRQ3)
		switch(snd118irqp){
		case 12:
			jmpflag[0] = 0; jmpflag[1] = 0;
			break;
		case 5:
			jmpflag[0] = 0; jmpflag[1] = 1;
			break;
		case 10:
			jmpflag[0] = 1; jmpflag[1] = 0;
			break;
		case 3:
			jmpflag[0] = 1; jmpflag[1] = 1;
			break;
		default:
			jmpflag[0] = jmpflag[1] = 0;
			break;
		}
		setjumperyex(&dipbmp, 18, 9, 3, jmpflag[1], 15); // #9
		setjumperyex(&dipbmp, 18, 9, 4, jmpflag[0], 15); // #8

		setjumperyex(&dipbmp, 18, 9, 5, 0, 15); // #7  ON:DMA 2ch, OFF:DMA 1ch

		// FM INT (#5,#6)=(OFF,OFF):INT5(IRQ12), (OFF,ON):INT41(IRQ10), (ON,OFF):INT6(IRQ13), (ON,ON):INT0(IRQ3)
		switch(snd118irqf){
		case 12:
			jmpflag[0] = 0; jmpflag[1] = 0;
			break;
		case 10:
			jmpflag[0] = 0; jmpflag[1] = 1;
			break;
		case 13:
			jmpflag[0] = 1; jmpflag[1] = 0;
			break;
		case 3:
			jmpflag[0] = 1; jmpflag[1] = 1;
			break;
		default:
			jmpflag[0] = jmpflag[1] = 0;
			break;
		}
		setjumperyex(&dipbmp, 18, 9, 6, jmpflag[1], 15); // #6
		setjumperyex(&dipbmp, 18, 9, 7, jmpflag[0], 15); // #5

		setjumperyex(&dipbmp, 18, 9, 8,  snd118rom ? 0 : 1, 15); // #4  ON:Sound BIOS Disable, OFF:Sound BIOS Enable
		
		// (#2,#3)=(OFF,OFF):Normal, (OFF,ON):CS4231&OPNA Disable, (ON,OFF):CS4231 Disable, (ON,ON):禁止
		setjumperyex(&dipbmp, 18, 9, 9,  0, 15); // #3
		setjumperyex(&dipbmp, 18, 9, 10, 0, 15); // #2

		setjumperyex(&dipbmp, 18, 9, 11, 0, 15); // #1  ON:PnP Enable, OFF:PnP Disable
	}
	return(ret);
}

#endif  // defined(USE_RESOURCE_BMP)

