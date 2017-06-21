#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"


enum {
	PIC_OCW2_L		= 0x07,
	PIC_OCW2_EOI	= 0x20,
	PIC_OCW2_SL		= 0x40,
	PIC_OCW2_R		= 0x80,

	PIC_OCW3_RIS	= 0x01,
	PIC_OCW3_RR		= 0x02,
	PIC_OCW3_P		= 0x04,
	PIC_OCW3_SMM	= 0x20,
	PIC_OCW3_ESMM	= 0x40
};


static const _PICITEM def_master = {
								{0x11, 0x08, 0x80, 0x1d},
								0x7d, 0, 0, 0, 0, 0};

static const _PICITEM def_slave = {
								{0x11, 0x10, 0x07, 0x09},
								0x71, 0, 0, 0, 0, 0};


// ----

#if 0	// スレーブがおかしい…
void pic_irq(void) {

	PIC		p;
	REG8	mir;
	REG8	sir;
	REG8	dat;
	REG8	num;
	REG8	bit;
	REG8	slave;

	// 割込み許可？
	if (!CPU_isEI) {
		return;
	}
	p = &pic;

	mir = p->pi[0].irr & (~p->pi[0].imr);
	sir = p->pi[1].irr & (~p->pi[1].imr);
	if ((mir == 0) && (sir == 0)) {
		return;
	}

	slave = 1 << (p->pi[1].icw[2] & 7);
	dat = mir;
	if (sir) {
		dat |= slave & (~p->pi[0].imr);
	}
	if (!(p->pi[0].ocw3 & PIC_OCW3_SMM)) {
		dat |= p->pi[0].isr;
	}
	num = p->pi[0].pry;
	bit = 1 << num;
	while(!(dat & bit)) {
		num = (num + 1) & 7;
		bit = 1 << num;
	}
	if (p->pi[0].icw[2] & bit) {					// スレーヴ
		dat = sir;
		if (!(p->pi[1].ocw3 & PIC_OCW3_SMM)) {
			dat |= p->pi[1].isr;
		}
		num = p->pi[1].pry;
		bit = 1 << num;
		while(!(dat & bit)) {
			num = (num + 1) & 7;
			bit = 1 << num;
		}
		if (!(p->pi[1].isr & bit)) {
			p->pi[0].isr |= slave;
			p->pi[0].irr &= ~slave;
			p->pi[1].isr |= bit;
			p->pi[1].irr &= ~bit;
			TRACEOUT(("hardware-int %.2x", (p->pi[1].icw[1] & 0xf8) | num));
			CPU_INTERRUPT((REG8)((p->pi[1].icw[1] & 0xf8) | num), 0);
		}
	}
	else if (!(p->pi[0].isr & bit)) {				// マスター
		p->pi[0].isr |= bit;
		p->pi[0].irr &= ~bit;
		if (num == 0) {
			nevent_reset(NEVENT_PICMASK);
		}
		TRACEOUT(("hardware-int %.2x [%.4x:%.4x]", (p->pi[0].icw[1] & 0xf8) | num, CPU_CS, CPU_IP));
		CPU_INTERRUPT((REG8)((p->pi[0].icw[1] & 0xf8) | num), 0);
	}
}
#else
void pic_irq(void) {												// ver0.78

	PIC		p;
	REG8	mir;
	REG8	sir;
	REG8	num;
	REG8	bit;
	REG8	slave;

	// 割込み許可？
	if (!CPU_isEI) {
		return;
	}
	p = &pic;

	sir = p->pi[1].irr & (~p->pi[1].imr);
	slave = 1 << (p->pi[1].icw[2] & 7);
	mir = p->pi[0].irr;
	if (sir) {
		mir |= slave;
	}
	mir &= (~p->pi[0].imr);
	if (mir == 0) {
		return;
	}
	if (!(p->pi[0].ocw3 & PIC_OCW3_SMM)) {
		mir |= p->pi[0].isr;
	}
	num = p->pi[0].pry;
	bit = 1 << num;
	while(!(mir & bit)) {
		num = (num + 1) & 7;
		bit = 1 << num;
	}
	if (p->pi[0].icw[2] & bit) {					// スレーヴ
		if (sir == 0) {
			return;
		}
		if (!(p->pi[1].ocw3 & PIC_OCW3_SMM)) {
			sir |= p->pi[1].isr;
		}
		num = p->pi[1].pry;
		bit = 1 << num;
		while(!(sir & bit)) {
			num = (num + 1) & 7;
			bit = 1 << num;
		}
		if (!(p->pi[1].isr & bit)) {
			p->pi[0].isr |= slave;
			p->pi[0].irr &= ~slave;
			p->pi[1].isr |= bit;
			p->pi[1].irr &= ~bit;
//			TRACEOUT(("hardware-int %.2x", (p->pi[1].icw[1] & 0xf8) | num));
			CPU_INTERRUPT((REG8)((p->pi[1].icw[1] & 0xf8) | num), 0);
		}
	}
	else if (!(p->pi[0].isr & bit)) {				// マスター
		p->pi[0].isr |= bit;
		p->pi[0].irr &= ~bit;
		if (num == 0) {
			nevent_reset(NEVENT_PICMASK);
		}
//		TRACEOUT(("hardware-int %.2x [%.4x:%.4x]", (p->pi[0].icw[1] & 0xf8) | num, CPU_CS, CPU_IP));
		CPU_INTERRUPT((REG8)((p->pi[0].icw[1] & 0xf8) | num), 0);
	}
}
#endif


// 簡易モード(SYSTEM TIMERだけ)
void picmask(NEVENTITEM item) {

	PICITEM		pi;

	if (item->flag & NEVENT_SETEVENT) {
		pi = &pic.pi[0];
		pi->irr &= ~(pi->imr & PIC_SYSTEMTIMER);
	}
}

void pic_setirq(REG8 irq) {

	PICITEM	pi;
	REG8	bit;

	pi = pic.pi;
	bit = 1 << (irq & 7);
	if (!(irq & 8)) {
		pi[0].irr |= bit;
		if (pi[0].imr & bit) {
			if (bit & PIC_SYSTEMTIMER) {
				if ((pit.ch[0].ctrl & 0x0c) == 0x04) {
					SINT32 cnt;										// ver0.29
					if (pit.ch[0].value > 8) {
						cnt = pccore.multiple * pit.ch[0].value;
						cnt >>= 2;
					}
					else {
						cnt = pccore.multiple << (16 - 2);
					}
					nevent_set(NEVENT_PICMASK, cnt, picmask, NEVENT_ABSOLUTE);
				}
			}
		}
		if (pi[0].isr & bit) {
			if (bit & PIC_CRTV) {
				pi[0].irr &= ~PIC_CRTV;
				gdc.vsyncint = 1;
			}
		}
	}
	else {
		pi[1].irr |= bit;
	}
}

void pic_resetirq(REG8 irq) {

	PICITEM		pi;

	pi = pic.pi + ((irq >> 3) & 1);
	pi->irr &= ~(1 << (irq & 7));
}


// ---- I/O

static void IOOUTCALL pic_o00(UINT port, REG8 dat) {

	PICITEM	picp;
	REG8	level;
	UINT8	ocw3;

//	TRACEOUT(("pic %x %x", port, dat));
	picp = &pic.pi[(port >> 3) & 1];
	picp->writeicw = 0;
	switch(dat & 0x18) {
		case 0x00:						// ocw2
			if (dat & PIC_OCW2_SL) {
				level = dat & PIC_OCW2_L;
			}
			else {
				if (!picp->isr) {
					break;
				}
				level = picp->pry;
				while(!(picp->isr & (1 << level))) {
					level = (level + 1) & 7;
				}
			}
			if (dat & PIC_OCW2_R) {
				picp->pry = (level + 1) & 7;
			}
			if (dat & PIC_OCW2_EOI) {
				picp->isr &= ~(1 << level);
			}
			nevent_forceexit();				// mainloop exit
			break;

		case 0x08:							// ocw3
			ocw3 = picp->ocw3;
			if (!(dat & PIC_OCW3_RR)) {
				dat &= PIC_OCW3_RIS;
				dat |= (ocw3 & PIC_OCW3_RIS);
			}
			if (!(dat & PIC_OCW3_ESMM)) {
				dat &= ~PIC_OCW3_SMM;
				dat |= (ocw3 & PIC_OCW3_SMM);
			}
			picp->ocw3 = dat;
			break;

		default:
			picp->icw[0] = dat;
			picp->imr = 0;
			picp->irr = 0;
			picp->ocw3 = 0;
#if 0
			picp->levels = 0;
			picp->isr = 0;
#endif
			picp->pry = 0;
			picp->writeicw = 1;
			break;
	}
}

static void IOOUTCALL pic_o02(UINT port, REG8 dat) {

	PICITEM		picp;

//	TRACEOUT(("pic %x %x", port, dat));
	picp = &pic.pi[(port >> 3) & 1];
	if (!picp->writeicw) {
#if 1
		UINT8	set;
		set = picp->imr & (~dat);
		// リセットされたビットは割り込みある？
		if ((CPU_isDI) || (!(picp->irr & set))) {
			picp->imr = dat;
			return;
		}
#endif
		picp->imr = dat;
	}
	else {
		picp->icw[picp->writeicw] = dat;
		picp->writeicw++;
		if (picp->writeicw >= (3 + (picp->icw[0] & 1))) {
			picp->writeicw = 0;
		}
	}
	nevent_forceexit();
}

static REG8 IOINPCALL pic_i00(UINT port) {

	PICITEM		picp;

	picp = &pic.pi[(port >> 3) & 1];
	if (!(picp->ocw3 & PIC_OCW3_RIS)) {
		return(picp->irr);			// read irr
	}
	else {
		return(picp->isr);			// read isr
	}
}

static REG8 IOINPCALL pic_i02(UINT port) {

	PICITEM		picp;

	picp = &pic.pi[(port >> 3) & 1];
	return(picp->imr);
}


// ---- I/F

#if !defined(SUPPORT_PC9821)
static const IOOUT pico00[2] = {
					pic_o00,	pic_o02};

static const IOINP pici00[2] = {
					pic_i00,	pic_i02};
#else
static const IOOUT pico00[4] = {
					pic_o00,	pic_o02,	NULL,	NULL};

static const IOINP pici00[4] = {
					pic_i00,	pic_i02,	NULL,	NULL};
#endif

void pic_reset(const NP2CFG *pConfig) {

	pic.pi[0] = def_master;
	pic.pi[1] = def_slave;

	(void)pConfig;
}

void pic_bind(void) {

#if !defined(SUPPORT_PC9821)
	iocore_attachsysoutex(0x0000, 0x0cf1, pico00, 2);
	iocore_attachsysinpex(0x0000, 0x0cf1, pici00, 2);
#else
	iocore_attachsysoutex(0x0000, 0x0cf1, pico00, 4);
	iocore_attachsysinpex(0x0000, 0x0cf1, pici00, 4);
#endif
}

