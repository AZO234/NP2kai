
#if defined(X11) && (defined(i386) || defined(__i386__))
#define	INHIBIT_WORDP(m)	((m) >= (I286_MEMWRITEMAX - 1))
#elif (defined(ARM) || defined(X11)) && defined(BYTESEX_LITTLE)
#define	INHIBIT_WORDP(m)	(((m) & 1) || ((m) >= I286_MEMWRITEMAX))
#else
#define	INHIBIT_WORDP(m)	(1)
#endif

#define	__CBW(src)		(UINT16)((SINT8)(src))
#define	__CBD(src)		((SINT8)(src))
#define	WORD2LONG(src)	((SINT16)(src))


#define	SEGMENTPTR(s)	(((UINT16 *)&I286_SEGREG) + (s))

#define REAL_FLAGREG	(UINT16)((I286_FLAG & 0x7ff) | (I286_OV?O_FLAG:0))

#define	STRING_DIR		((I286_FLAG & D_FLAG)?-1:1)
#define	STRING_DIRx2	((I286_FLAG & D_FLAG)?-2:2)


// ---- flags

#if defined(I286C_TEST)

extern UINT8 BYTESZPF(UINT r);
extern UINT8 BYTESZPCF(UINT r);
#define	BYTESZPCF2(a)	BYTESZPCF((a) & 0x1ff)
extern UINT8 WORDSZPF(UINT32 r);
extern UINT8 WORDSZPCF(UINT32 r);

#elif !defined(MEMOPTIMIZE)

extern	UINT8	_szpflag16[0x10000];
#define	BYTESZPF(a)		(iflags[(a)])
#define	BYTESZPCF(a)	(iflags[(a)])
#define	BYTESZPCF2(a)	(iflags[(a) & 0x1ff])
#define	WORDSZPF(a)		(_szpflag16[(a)])
#define	WORDSZPCF(a)	(_szpflag16[LOW16(a)] + (((a) >> 16) & 1))

#else

#define	BYTESZPF(a)		(iflags[(a)])
#define	BYTESZPCF(a)	(iflags[(a)])
#define	BYTESZPCF2(a)	(iflags[(a) & 0x1ff])
#define	WORDSZPF(a)		((iflags[(a) & 0xff] & P_FLAG) + \
									(((a))?0:Z_FLAG) + (((a) >> 8) & S_FLAG))
#define	WORDSZPCF(a)	((iflags[(a) & 0xff] & P_FLAG) + \
							((LOW16(a))?0:Z_FLAG) + (((a) >> 8) & S_FLAG) + \
							(((a) >> 16) & 1))

#endif


// ---- reg position

#if !defined(MEMOPTIMIZE) || (MEMOPTIMIZE < 2)
extern	UINT8	*_reg8_b53[256];
extern	UINT8	*_reg8_b20[256];
#define	REG8_B53(op)		_reg8_b53[(op)]
#define	REG8_B20(op)		_reg8_b20[(op)]
#else
#if defined(BYTESEX_LITTLE)
#define	REG8_B53(op)		\
				(((UINT8 *)&I286_REG) + (((op) >> 2) & 6) + (((op) >> 5) & 1))
#define	REG8_B20(op)		\
				(((UINT8 *)&I286_REG) + (((op) & 3) * 2) + (((op) >> 2) & 1))
#else
#define	REG8_B53(op)		(((UINT8 *)&I286_REG) + (((op) >> 2) & 6) +	\
													((((op) >> 5) & 1) ^ 1))
#define	REG8_B20(op)		(((UINT8 *)&I286_REG) + (((op) & 3) * 2) +	\
													((((op) >> 2) & 1) ^ 1))
#endif
#endif

#if !defined(MEMOPTIMIZE) || (MEMOPTIMIZE < 2)
extern	UINT16	*_reg16_b53[256];
extern	UINT16	*_reg16_b20[256];
#define	REG16_B53(op)		_reg16_b53[(op)]
#define	REG16_B20(op)		_reg16_b20[(op)]
#else
#define	REG16_B53(op)		(((UINT16 *)&I286_REG) + (((op) >> 3) & 7))
#define	REG16_B20(op)		(((UINT16 *)&I286_REG) + ((op) & 7))
#endif


// ---- ea

#if !defined(MEMOPTIMIZE) || (MEMOPTIMIZE < 2)
typedef UINT32 (*CALCEA)(void);
typedef UINT16 (*CALCLEA)(void);
typedef UINT (*GETLEA)(UINT32 *seg);
extern	CALCEA	_calc_ea_dst[];
extern	CALCLEA	_calc_lea[];
extern	GETLEA	_get_ea[];
#define	CALC_EA(o)		(_calc_ea_dst[(o)]())
#define	CALC_LEA(o)		(_calc_lea[(o)]())
#define	GET_EA(o, s)	(_get_ea[(o)](s))
#else
extern UINT32 calc_ea_dst(UINT op);
extern UINT16 calc_lea(UINT op);
extern UINT calc_a(UINT op, UINT32 *seg);
#define	CALC_EA(o)		(calc_ea_dst(o))
#define	CALC_LEA(o)		(calc_lea(o))
#define	GET_EA(o, s)	(calc_a(o, s))
#endif


#define	SWAPBYTE(p, q) {											\
		REG8 tmp;													\
		tmp = (p);													\
		(p) = (q);													\
		(q) = tmp;													\
	}

#define	SWAPWORD(p, q) {											\
		REG16 tmp;													\
		tmp = (p);													\
		(p) = (q);													\
		(q) = tmp;													\
	}


#define	I286IRQCHECKTERM											\
		if (I286_REMCLOCK > 0) {									\
			I286_BASECLOCK -= I286_REMCLOCK;						\
			I286_REMCLOCK = 0;										\
		}


#define	REMOVE_PREFIX												\
		SS_FIX = SS_BASE;											\
		DS_FIX = DS_BASE;


#define	I286_WORKCLOCK(c)	I286_REMCLOCK -= (c)


#define	GET_PCBYTE(b)												\
		(b) = i286_memoryread(CS_BASE + I286_IP);					\
		I286_IP++;


#define	GET_PCBYTES(b)												\
		(b) = __CBW(i286_memoryread(CS_BASE + I286_IP));			\
		I286_IP++;


#define	GET_PCBYTESD(b)												\
		(b) = __CBD(i286_memoryread(CS_BASE + I286_IP));			\
		I286_IP++;


#define	GET_PCWORD(b)												\
		(b) = i286_memoryread_w(CS_BASE + I286_IP);					\
		I286_IP += 2;


#define	PREPART_EA_REG8(b, d_s)										\
		GET_PCBYTE((b))												\
		(d_s) = *(REG8_B53(b));


#define	PREPART_EA_REG8P(b, d_s)									\
		GET_PCBYTE((b))												\
		(d_s) = REG8_B53(b);


#define	PREPART_EA_REG16(b, d_s)									\
		GET_PCBYTE((b))												\
		(d_s) = *(REG16_B53(b));


#define	PREPART_EA_REG16P(b, d_s)									\
		GET_PCBYTE((b))												\
		(d_s) = REG16_B53(b);


#define PREPART_REG8_EA(b, s, d, regclk, memclk)					\
		GET_PCBYTE((b))												\
		if ((b) >= 0xc0) {											\
			I286_WORKCLOCK(regclk);									\
			(s) = *(REG8_B20(b));									\
		}															\
		else {														\
			I286_WORKCLOCK(memclk);									\
			(s) = i286_memoryread(CALC_EA(b));						\
		}															\
		(d) = REG8_B53(b);


#define	PREPART_REG16_EA(b, s, d, regclk, memclk)					\
		GET_PCBYTE(b)												\
		if (b >= 0xc0) {											\
			I286_WORKCLOCK(regclk);									\
			s = *(REG16_B20(b));									\
		}															\
		else {														\
			I286_WORKCLOCK(memclk);									\
			s = i286_memoryread_w(CALC_EA(b));						\
		}															\
		d = REG16_B53(b);


#define	ADDBYTE(r, d, s)											\
		(r) = (s) + (d);											\
		I286_OV = ((r) ^ (s)) & ((r) ^ (d)) & 0x80;					\
		I286_FLAGL = (UINT8)(((r) ^ (d) ^ (s)) & A_FLAG);			\
		I286_FLAGL |= BYTESZPCF(r);

#define	ADDWORD(r, d, s)											\
		(r) = (s) + (d);											\
		I286_OV = ((r) ^ (s)) & ((r) ^ (d)) & 0x8000;				\
		I286_FLAGL = (UINT8)(((r) ^ (d) ^ (s)) & A_FLAG);			\
		I286_FLAGL |= WORDSZPCF(r);


// flag no check
#define	ORBYTE(d, s)												\
		(d) |= (s);													\
		I286_OV = 0;												\
		I286_FLAGL = BYTESZPF(d);

#define	ORWORD(d, s)												\
		(d) |= (s);													\
		I286_OV = 0;												\
		I286_FLAGL = WORDSZPF(d);


#define	ADCBYTE(r, d, s) 											\
		(r) = (I286_FLAGL & 1) + (s) + (d);							\
		I286_OV = ((r) ^ (s)) & ((r) ^ (d)) & 0x80;					\
		I286_FLAGL = (UINT8)(((r) ^ (d) ^ (s)) & A_FLAG);			\
		I286_FLAGL |= BYTESZPCF(r);

#define	ADCWORD(r, d, s) 											\
		(r) = (I286_FLAGL & 1) + (s) + (d);							\
		I286_OV = ((r) ^ (s)) & ((r) ^ (d)) & 0x8000;				\
		I286_FLAGL = (UINT8)(((r) ^ (d) ^ (s)) & A_FLAG);			\
		I286_FLAGL |= WORDSZPCF(r);


// flag no check
#define	SBBBYTE(r, d, s) 											\
		(r) = (d) - (s) - (I286_FLAGL & 1);							\
		I286_OV = ((d) ^ (r)) & ((d) ^ (s)) & 0x80;					\
		I286_FLAGL = (UINT8)(((r) ^ (d) ^ (s)) & A_FLAG);			\
		I286_FLAGL |= BYTESZPCF2(r);

#define	SBBWORD(r, d, s) 											\
		(r) = (d) - (s) - (I286_FLAGL & 1);							\
		I286_OV = ((d) ^ (r)) & ((d) ^ (s)) & 0x8000;				\
		I286_FLAGL = (UINT8)(((r) ^ (d) ^ (s)) & A_FLAG);			\
		I286_FLAGL |= WORDSZPCF(r);


// flag no check
#define	ANDBYTE(d, s)												\
		(d) &= (s);													\
		I286_OV = 0;												\
		I286_FLAGL = BYTESZPF(d);

#define	ANDWORD(d, s)												\
		(d) &= (s);													\
		I286_OV = 0;												\
		I286_FLAGL = WORDSZPF(d);


// flag no check
#define	SUBBYTE(r, d, s) 											\
		(r) = (d) - (s);											\
		I286_OV = ((d) ^ (r)) & ((d) ^ (s)) & 0x80;					\
		I286_FLAGL = (UINT8)(((r) ^ (d) ^ (s)) & A_FLAG);			\
		I286_FLAGL |= BYTESZPCF2(r);

#define	SUBWORD(r, d, s) 											\
		(r) = (d) - (s);											\
		I286_OV = ((d) ^ (r)) & ((d) ^ (s)) & 0x8000;				\
		I286_FLAGL = ((r) ^ (d) ^ (s)) & A_FLAG;					\
		I286_FLAGL |= WORDSZPCF(r);


// flag no check
#define	XORBYTE(d, s)												\
		(d) ^= s;													\
		I286_OV = 0;												\
		I286_FLAGL = BYTESZPF(d);

#define	XORWORD(d, s)												\
		(d) ^= (s);													\
		I286_OV = 0;												\
		I286_FLAGL = WORDSZPF(d);


#define	NEGBYTE(d, s) 												\
		(d) = 0 - (s);												\
		I286_OV = ((d) & (s)) & 0x80;								\
		I286_FLAGL = (UINT8)(((d) ^ (s)) & A_FLAG);					\
		I286_FLAGL |= BYTESZPCF2(d);

#define	NEGWORD(d, s) 												\
		(d) = 0 - (s);												\
		I286_OV = ((d) & (s)) & 0x8000;								\
		I286_FLAGL = (UINT8)(((d) ^ (s)) & A_FLAG);					\
		I286_FLAGL |= WORDSZPCF(d);


#define	BYTE_MUL(r, d, s)											\
		I286_FLAGL &= (Z_FLAG | S_FLAG | A_FLAG | P_FLAG);			\
		(r) = (UINT8)(d) * (UINT8)(s);								\
		I286_OV = (r) >> 8;											\
		if (I286_OV) {												\
			I286_FLAGL |= C_FLAG;									\
		}

#define	WORD_MUL(r, d, s)											\
		I286_FLAGL &= (Z_FLAG | S_FLAG | A_FLAG | P_FLAG);			\
		(r) = (UINT16)(d) * (UINT16)(s);							\
		I286_OV = (r) >> 16;										\
		if (I286_OV) {												\
			I286_FLAGL |= C_FLAG;									\
		}


#define	BYTE_IMUL(r, d, s)											\
		I286_FLAGL &= (Z_FLAG | S_FLAG | A_FLAG | P_FLAG);			\
		(r) = (SINT8)(d) * (SINT8)(s);								\
		I286_OV = ((r) + 0x80) & 0xffffff00;						\
		if (I286_OV) {												\
			I286_FLAGL |= C_FLAG;									\
		}

#define	WORD_IMUL(r, d, s)											\
		I286_FLAGL &= (Z_FLAG | S_FLAG | A_FLAG | P_FLAG);			\
		(r) = (SINT16)(d) * (SINT16)(s);							\
		I286_OV = ((r) + 0x8000) & 0xffff0000;						\
		if (I286_OV) {												\
			I286_FLAGL |= C_FLAG;									\
		}


// flag no check
#define	INCBYTE(s) {												\
		UINT b = (s);												\
		(s)++;														\
		I286_OV = (s) & (b ^ (s)) & 0x80;							\
		I286_FLAGL &= C_FLAG;										\
		I286_FLAGL |= (UINT8)((b ^ (s)) & A_FLAG);					\
		I286_FLAGL |= BYTESZPF((UINT8)(s));							\
	}

#define	INCWORD(s) {												\
		UINT32 b = (s);												\
		(s)++;														\
		I286_OV = (s) & (b ^ (s)) & 0x8000;							\
		I286_FLAGL &= C_FLAG;										\
		I286_FLAGL |= (UINT8)((b ^ (s)) & A_FLAG);					\
		I286_FLAGL |= WORDSZPF((UINT16)(s));						\
	}


// flag no check
#define	DECBYTE(s) {												\
		UINT b = (s);												\
		b--;														\
		I286_OV = (s) & (b ^ (s)) & 0x80;							\
		I286_FLAGL &= C_FLAG;										\
		I286_FLAGL |= (UINT8)((b ^ (s)) & A_FLAG);					\
		I286_FLAGL |= BYTESZPF((UINT8)b);							\
		(s) = b;													\
	}

#define	DECWORD(s) {												\
		UINT32 b = (s);												\
		b--;														\
		I286_OV = (s) & (b ^ (s)) & 0x8000;							\
		I286_FLAGL &= C_FLAG;										\
		I286_FLAGL |= (UINT8)((b ^ (s)) & A_FLAG);					\
		I286_FLAGL |= WORDSZPF((UINT16)b);							\
		(s) = b;													\
	}


// flag no check
#define	INCWORD2(r, clock) {										\
		REG16 s = (r);												\
		REG16 d = (r);												\
		d++;														\
		(r) = (UINT16)d;											\
		I286_OV = d & (d ^ s) & 0x8000;								\
		I286_FLAGL &= C_FLAG;										\
		I286_FLAGL |= (UINT8)((d ^ s) & A_FLAG);					\
		I286_FLAGL |= WORDSZPF((UINT16)d);							\
		I286_WORKCLOCK(clock);										\
	}

#define	DECWORD2(r, clock) {										\
		REG16 s = (r);												\
		REG16 d = (r);												\
		d--;														\
		(r) = (UINT16)d;											\
		I286_OV = s & (d ^ s) & 0x8000;								\
		I286_FLAGL &= C_FLAG;										\
		I286_FLAGL |= (UINT8)((d ^ s) & A_FLAG);					\
		I286_FLAGL |= WORDSZPF((UINT16)d);							\
		I286_WORKCLOCK(clock);										\
	}


// ---- stack

#define	REGPUSH0(reg)												\
		I286_SP -= 2;												\
		i286_memorywrite_w(I286_SP + SS_BASE, reg);

#define	REGPOP0(reg) 												\
		reg = i286_memoryread_w(I286_SP + SS_BASE);					\
		I286_SP += 2;

#if (defined(ARM) || defined(X11)) && defined(BYTESEX_LITTLE)

#define	REGPUSH(reg, clock)	{										\
		UINT32 addr;												\
		I286_WORKCLOCK(clock);										\
		I286_SP -= 2;												\
		addr = I286_SP + SS_BASE;									\
		if (INHIBIT_WORDP(addr)) {									\
			i286_memorywrite_w(addr, reg);							\
		}															\
		else {														\
			*(UINT16 *)(mem + addr) = (reg);						\
		}															\
	}

#define	REGPOP(reg, clock) {										\
		UINT32 addr;												\
		I286_WORKCLOCK(clock);										\
		addr = I286_SP + SS_BASE;									\
		if (INHIBIT_WORDP(addr)) {									\
			(reg) = i286_memoryread_w(addr);						\
		}															\
		else {														\
			(reg) = *(UINT16 *)(mem + addr);						\
		}															\
		I286_SP += 2;												\
	}

#else

#define	REGPUSH(reg, clock)	{										\
		I286_WORKCLOCK(clock);										\
		I286_SP -= 2;												\
		i286_memorywrite_w(I286_SP + SS_BASE, reg);					\
	}

#define	REGPOP(reg, clock) {										\
		I286_WORKCLOCK(clock);										\
		reg = i286_memoryread_w(I286_SP + SS_BASE);					\
		I286_SP += 2;												\
	}

#endif

#define	SP_PUSH(reg, clock)	{										\
		REG16 sp = (reg);											\
		I286_SP -= 2;												\
		i286_memorywrite_w(I286_SP + SS_BASE, sp);					\
		I286_WORKCLOCK(clock);										\
	}

#define	SP_POP(reg, clock) {										\
		I286_WORKCLOCK(clock);										\
		reg = i286_memoryread_w(I286_SP + SS_BASE);					\
	}


#define	JMPSHORT(clock) {											\
		I286_WORKCLOCK(clock);										\
		I286_IP += __CBW(i286_memoryread(CS_BASE + I286_IP));		\
		I286_IP++;													\
	}


#define	JMPNOP(clock) {												\
		I286_WORKCLOCK(clock);										\
		I286_IP++;													\
	}


#define	MOVIMM8(reg) {												\
		I286_WORKCLOCK(2);											\
		GET_PCBYTE(reg)												\
	}


#define	MOVIMM16(reg) {												\
		I286_WORKCLOCK(2);											\
		GET_PCWORD(reg)												\
	}


#define	SEGSELECT(c)	((I286_MSW & MSW_PE)?i286c_selector(c):((c) << 4))

#define	INT_NUM(a, b)	i286c_intnum((a), (REG16)(b))

