
typedef struct {
	UINT8	porta;
	UINT8	portb;
	UINT8	portc;
	UINT8	mode;
} uPD8255;

enum {
	uPD8255_PORTCL	= 0x01,
	uPD8255_PORTB	= 0x02,
	uPD8255_GROUPB	= 0x04,
	uPD8255_PORTCH	= 0x08,
	uPD8255_PORTA	= 0x10,
	uPD8255_GROUPA	= 0x60,
	uPD8255_CTRL	= 0x80
};

