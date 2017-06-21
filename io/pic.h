
typedef struct {
	UINT8	icw[4];

	UINT8	imr;			// ocw1
	UINT8	isr;
	UINT8	irr;
	UINT8	ocw3;

	UINT8	pry;
	UINT8	writeicw;
	UINT8	padding[2];
} _PICITEM, *PICITEM;


typedef struct {
	_PICITEM	pi[2];
} _PIC, *PIC;

enum {
	PIC_SYSTEMTIMER		= 0x01,
	PIC_KEYBOARD		= 0x02,
	PIC_CRTV			= 0x04,
	PIC_INT0			= 0x08,
	PIC_RS232C			= 0x10,
	PIC_INT1			= 0x20,
	PIC_INT2			= 0x40,
	PIC_SLAVE			= 0x80,

	PIC_PRINTER			= 0x01,
	PIC_INT3			= 0x02,
	PIC_INT41			= 0x04,
	PIC_INT42			= 0x08,
	PIC_INT5			= 0x10,
	PIC_INT6			= 0x20,
	PIC_NDP				= 0x40,

	IRQ_INT0			= 0x03,
	IRQ_INT1			= 0x05,
	IRQ_INT2			= 0x06,
	IRQ_INT3			= 0x09,
	IRQ_INT41			= 0x0a,
	IRQ_INT42			= 0x0b,
	IRQ_INT5			= 0x0c,
	IRQ_INT6			= 0x0d
};

#define PICEXISTINTR	((pic.pi[0].irr & (~pic.pi[0].imr)) ||		\
						(pic.pi[1].irr & (~pic.pi[1].imr)))


#ifdef __cplusplus
extern "C" {
#endif

void pic_irq(void);
void pic_setirq(REG8 irq);
void pic_resetirq(REG8 irq);

void picmask(NEVENTITEM item);

void pic_reset(const NP2CFG *pConfig);
void pic_bind(void);

#ifdef __cplusplus
}
#endif

