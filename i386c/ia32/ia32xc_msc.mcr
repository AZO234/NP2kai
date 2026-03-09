
// Crosscheck for VC++

#define BYTE_ADC(r, d, s) { \
	UINT8 _d = (d); \
	UINT8 _s = (s); \
	UINT8 _r; \
	UINT8 _f, _ov; \
	__asm { bt		CPU_FLAG, 0	} \
	__asm { mov		al, _d		} \
	__asm {	adc		al, _s		} \
	__asm { mov		_r, al		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_ADC_BYTE((r), (d), (s)); \
	if ((_r != (UINT8)(r)) || ((_f ^ CPU_FLAGL) & SZAPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("adcb r=%x:%x f=%x:%x o=%d %d", _r, r, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define WORD_ADC(r, d, s) { \
	UINT16 _d = (d); \
	UINT16 _s = (s); \
	UINT16 _r; \
	UINT8 _f, _ov; \
	__asm { bt		CPU_FLAG, 0	} \
	__asm { mov		ax, _d		} \
	__asm {	adc		ax, _s		} \
	__asm { mov		_r, ax		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_ADC_WORD((r), (d), (s)); \
	if ((_r != (UINT16)(r)) || ((_f ^ CPU_FLAGL) & SZAPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("adcw r=%x:%x f=%x:%x o=%d %d", _r, r, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define DWORD_ADC(r, d, s) { \
	UINT32 _d = (d); \
	UINT32 _s = (s); \
	UINT32 _r; \
	UINT8 _f, _ov; \
	__asm { bt		CPU_FLAG, 0	} \
	__asm { mov		eax, _d		} \
	__asm {	adc		eax, _s		} \
	__asm { mov		_r, eax		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_ADC_DWORD((r), (d), (s)); \
	if ((_r != (UINT32)(r)) || ((_f ^ CPU_FLAGL) & SZAPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("adcd r=%x:%x f=%x:%x o=%d %d", _r, r, _f, CPU_FLAGL, _ov, CPU_OV)); \
}



#define BYTE_ADD(r, d, s) { \
	UINT8 _d = (d); \
	UINT8 _s = (s); \
	UINT8 _r; \
	UINT8 _f, _ov; \
	__asm { mov		al, _d		} \
	__asm {	add		al, _s		} \
	__asm { mov		_r, al		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_ADD_BYTE((r), (d), (s)); \
	if ((_r != (UINT8)(r)) || ((_f ^ CPU_FLAGL) & SZAPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("addb r=%x:%x f=%x:%x o=%d %d", _r, r, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define WORD_ADD(r, d, s) { \
	UINT16 _d = (d); \
	UINT16 _s = (s); \
	UINT16 _r; \
	UINT8 _f, _ov; \
	__asm { mov		ax, _d		} \
	__asm {	add		ax, _s		} \
	__asm { mov		_r, ax		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_ADD_WORD((r), (d), (s)); \
	if ((_r != (UINT16)(r)) || ((_f ^ CPU_FLAGL) & SZAPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("addw r=%x:%x f=%x:%x o=%d %d", _r, r, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define DWORD_ADD(r, d, s) { \
	UINT32 _d = (d); \
	UINT32 _s = (s); \
	UINT32 _r; \
	UINT8 _f, _ov; \
	__asm { mov		eax, _d		} \
	__asm {	add		eax, _s		} \
	__asm { mov		_r, eax		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_ADD_DWORD((r), (d), (s)); \
	if ((_r != (UINT32)(r)) || ((_f ^ CPU_FLAGL) & SZAPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("addd r=%x:%x f=%x:%x o=%d %d", _r, r, _f, CPU_FLAGL, _ov, CPU_OV)); \
}



#define BYTE_AND(d, s) { \
	UINT8 _d = (d); \
	UINT8 _s = (s); \
	UINT8 _f, _ov; \
	__asm {	mov		al, _s		} \
	__asm { and		_d, al		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_AND_BYTE((d), (s)); \
	if ((_d != (UINT8)(d)) || ((_f ^ CPU_FLAGL) & SZAPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("andb r=%x:%x f=%x:%x o=%d %d", _d, d, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define WORD_AND(d, s) { \
	UINT16 _d = (d); \
	UINT16 _s = (s); \
	UINT8 _f, _ov; \
	__asm {	mov		ax, _s		} \
	__asm { and		_d, ax		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_AND_WORD((d), (s)); \
	if ((_d != (UINT16)(d)) || ((_f ^ CPU_FLAGL) & SZAPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("andw r=%x:%x f=%x:%x o=%d %d", _d, d, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define DWORD_AND(d, s) { \
	UINT32 _d = (d); \
	UINT32 _s = (s); \
	UINT8 _f, _ov; \
	__asm {	mov		eax, _s		} \
	__asm { and		_d, eax		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_AND_DWORD((d), (s)); \
	if ((_d != (UINT32)(d)) || ((_f ^ CPU_FLAGL) & SZAPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("andd r=%x:%x f=%x:%x o=%d %d", _d, d, _f, CPU_FLAGL, _ov, CPU_OV)); \
}



#define BYTE_DEC(s) { \
	UINT8 _s = (s); \
	UINT8 _f, _ov; \
	__asm { bt		CPU_FLAG, 0	} \
	__asm {	dec		_s			} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_BYTE_DEC((s)); \
	if ((_s != (UINT8)(s)) || ((_f ^ CPU_FLAGL) & SZAPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("decb r=%x:%x f=%x:%x o=%d %d", _s, s, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define WORD_DEC(s) { \
	UINT16 _s = (s); \
	UINT8 _f, _ov; \
	__asm { bt		CPU_FLAG, 0	} \
	__asm {	dec		_s			} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_WORD_DEC((s)); \
	if ((_s != (UINT16)(s)) || ((_f ^ CPU_FLAGL) & SZAPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("decd r=%x:%x f=%x:%x o=%d %d", _s, s, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define DWORD_DEC(s) { \
	UINT32 _s = (s); \
	UINT8 _f, _ov; \
	__asm { bt		CPU_FLAG, 0	} \
	__asm {	dec		_s			} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_DWORD_DEC((s)); \
	if ((_s != (UINT32)(s)) || ((_f ^ CPU_FLAGL) & SZAPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("decd r=%x:%x f=%x:%x o=%d %d", _s, s, _f, CPU_FLAGL, _ov, CPU_OV)); \
}



#define BYTE_INC(s) { \
	UINT8 _s = (s); \
	UINT8 _f, _ov; \
	__asm { bt		CPU_FLAG, 0	} \
	__asm {	inc		_s			} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_BYTE_INC((s)); \
	if ((_s != (UINT8)(s)) || ((_f ^ CPU_FLAGL) & SZAPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("incb r=%x:%x f=%x:%x o=%d %d", _s, s, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define WORD_INC(s) { \
	UINT16 _s = (s); \
	UINT8 _f, _ov; \
	__asm { bt		CPU_FLAG, 0	} \
	__asm {	inc		_s			} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_WORD_INC((s)); \
	if ((_s != (UINT16)(s)) || ((_f ^ CPU_FLAGL) & SZAPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("incd r=%x:%x f=%x:%x o=%d %d", _s, s, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define DWORD_INC(s) { \
	UINT32 _s = (s); \
	UINT8 _f, _ov; \
	__asm { bt		CPU_FLAG, 0	} \
	__asm {	inc		_s			} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_DWORD_INC((s)); \
	if ((_s != (UINT32)(s)) || ((_f ^ CPU_FLAGL) & SZAPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("incd r=%x:%x f=%x:%x o=%d %d", _s, s, _f, CPU_FLAGL, _ov, CPU_OV)); \
}



#define BYTE_NEG(d, s) { \
	UINT8 _d = (s); \
	UINT8 _f, _ov; \
	__asm {	neg		_d			} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_BYTE_NEG((d), (s)); \
	if ((_d != (UINT8)(d)) || ((_f ^ CPU_FLAGL) & SZAPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("negb r=%x:%x f=%x:%x o=%d %d", _d, d, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define WORD_NEG(d, s) { \
	UINT16 _d = (s); \
	UINT8 _f, _ov; \
	__asm {	neg		_d			} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_WORD_NEG((d), (s)); \
	if ((_d != (UINT16)(d)) || ((_f ^ CPU_FLAGL) & SZAPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("negw r=%x:%x f=%x:%x o=%d %d", _d, d, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define DWORD_NEG(d, s) { \
	UINT32 _d = (s); \
	UINT8 _f, _ov; \
	__asm {	neg		_d			} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_DWORD_NEG((d), (s)); \
	if ((_d != (UINT32)(d)) || ((_f ^ CPU_FLAGL) & SZAPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("negd r=%x:%x f=%x:%x o=%d %d", _d, d, _f, CPU_FLAGL, _ov, CPU_OV)); \
}



#define BYTE_OR(d, s) { \
	UINT8 _d = (d); \
	UINT8 _s = (s); \
	UINT8 _f, _ov; \
	__asm {	mov		al, _s		} \
	__asm { or		_d, al		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_OR_BYTE((d), (s)); \
	if ((_d != (UINT8)(d)) || ((_f ^ CPU_FLAGL) & SZAPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("orb r=%x:%x f=%x:%x o=%d %d", _d, d, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define WORD_OR(d, s) { \
	UINT16 _d = (d); \
	UINT16 _s = (s); \
	UINT8 _f, _ov; \
	__asm {	mov		ax, _s		} \
	__asm { or		_d, ax		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_OR_WORD((d), (s)); \
	if ((_d != (UINT16)(d)) || ((_f ^ CPU_FLAGL) & SZAPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("orw r=%x:%x f=%x:%x o=%d %d", _d, d, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define DWORD_OR(d, s) { \
	UINT32 _d = (d); \
	UINT32 _s = (s); \
	UINT8 _f, _ov; \
	__asm {	mov		eax, _s		} \
	__asm { or		_d, eax		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_OR_DWORD((d), (s)); \
	if ((_d != (UINT32)(d)) || ((_f ^ CPU_FLAGL) & SZAPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("ord r=%x:%x f=%x:%x o=%d %d", _d, d, _f, CPU_FLAGL, _ov, CPU_OV)); \
}



#define BYTE_SBB(r, d, s) { \
	UINT8 _d = (d); \
	UINT8 _s = (s); \
	UINT8 _r; \
	UINT8 _f, _ov; \
	__asm { bt		CPU_FLAG, 0	} \
	__asm { mov		al, _d		} \
	__asm {	sbb		al, _s		} \
	__asm { mov		_r, al		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_BYTE_SBB((r), (d), (s)); \
	if ((_r != (UINT8)(r)) || ((_f ^ CPU_FLAGL) & SZAPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("sbbb r=%x:%x f=%x:%x o=%d %d", _r, r, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define WORD_SBB(r, d, s) { \
	UINT16 _d = (d); \
	UINT16 _s = (s); \
	UINT16 _r; \
	UINT8 _f, _ov; \
	__asm { bt		CPU_FLAG, 0	} \
	__asm { mov		ax, _d		} \
	__asm {	sbb		ax, _s		} \
	__asm { mov		_r, ax		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_WORD_SBB((r), (d), (s)); \
	if ((_r != (UINT16)(r)) || ((_f ^ CPU_FLAGL) & SZAPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("sbbw r=%x:%x f=%x:%x o=%d %d", _r, r, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define DWORD_SBB(r, d, s) { \
	UINT32 _d = (d); \
	UINT32 _s = (s); \
	UINT32 _r; \
	UINT8 _f, _ov; \
	__asm { bt		CPU_FLAG, 0	} \
	__asm { mov		eax, _d		} \
	__asm {	sbb		eax, _s		} \
	__asm { mov		_r, eax		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_DWORD_SBB((r), (d), (s)); \
	if ((_r != (UINT32)(r)) || ((_f ^ CPU_FLAGL) & SZAPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("sbbd r=%x:%x f=%x:%x o=%d %d", _r, r, _f, CPU_FLAGL, _ov, CPU_OV)); \
}



#define BYTE_SUB(r, d, s) { \
	UINT8 _d = (d); \
	UINT8 _s = (s); \
	UINT8 _r; \
	UINT8 _f, _ov; \
	__asm { mov		al, _d		} \
	__asm {	sub		al, _s		} \
	__asm { mov		_r, al		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_BYTE_SUB((r), (d), (s)); \
	if ((_r != (UINT8)(r)) || ((_f ^ CPU_FLAGL) & SZAPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("subb r=%x:%x f=%x:%x o=%d %d", _r, r, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define WORD_SUB(r, d, s) { \
	UINT16 _d = (d); \
	UINT16 _s = (s); \
	UINT16 _r; \
	UINT8 _f, _ov; \
	__asm { mov		ax, _d		} \
	__asm {	sub		ax, _s		} \
	__asm { mov		_r, ax		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_WORD_SUB((r), (d), (s)); \
	if ((_r != (UINT16)(r)) || ((_f ^ CPU_FLAGL) & SZAPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("subw r=%x:%x f=%x:%x o=%d %d", _r, r, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define DWORD_SUB(r, d, s) { \
	UINT32 _d = (d); \
	UINT32 _s = (s); \
	UINT32 _r; \
	UINT8 _f, _ov; \
	__asm { mov		eax, _d		} \
	__asm {	sub		eax, _s		} \
	__asm { mov		_r, eax		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_DWORD_SUB((r), (d), (s)); \
	if ((_r != (UINT32)(r)) || ((_f ^ CPU_FLAGL) & SZAPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("subd r=%x:%x f=%x:%x o=%d %d", _r, r, _f, CPU_FLAGL, _ov, CPU_OV)); \
}



#define BYTE_XOR(d, s) { \
	UINT8 _d = (d); \
	UINT8 _s = (s); \
	UINT8 _f, _ov; \
	__asm {	mov		al, _s		} \
	__asm { xor		_d, al		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_BYTE_XOR((d), (s)); \
	if ((_d != (UINT8)(d)) || ((_f ^ CPU_FLAGL) & SZAPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("xorb r=%x:%x f=%x:%x o=%d %d", _d, d, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define WORD_XOR(d, s) { \
	UINT16 _d = (d); \
	UINT16 _s = (s); \
	UINT8 _f, _ov; \
	__asm {	mov		ax, _s		} \
	__asm { xor		_d, ax		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_WORD_XOR((d), (s)); \
	if ((_d != (UINT16)(d)) || ((_f ^ CPU_FLAGL) & SZAPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("xorw r=%x:%x f=%x:%x o=%d %d", _d, d, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define DWORD_XOR(d, s) { \
	UINT32 _d = (d); \
	UINT32 _s = (s); \
	UINT8 _f, _ov; \
	__asm {	mov		eax, _s		} \
	__asm { xor		_d, eax		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_DWORD_XOR((d), (s)); \
	if ((_d != (UINT32)(d)) || ((_f ^ CPU_FLAGL) & SZAPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("xord r=%x:%x f=%x:%x o=%d %d", _d, d, _f, CPU_FLAGL, _ov, CPU_OV)); \
}


// ----

#define BYTE_IMUL(r, d, s) { \
	UINT8 _d = (d); \
	UINT8 _s = (s); \
	UINT16 _r; \
	UINT8 _f, _ov; \
	__asm { bt		CPU_FLAG, 0	} \
	__asm { mov		al, _d		} \
	__asm {	imul	_s			} \
	__asm { mov		_r, ax		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_BYTE_IMUL((r), (d), (s)); \
	if ((_r != (UINT16)(r)) || ((_f ^ CPU_FLAGL) & C_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("imulb r=%x:%x f=%x:%x o=%d %d", _r, r, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define WORD_IMUL(r, d, s) { \
	UINT16 _d = (d); \
	UINT16 _s = (s); \
	UINT16 _rl, _rh; \
	UINT32 _r; \
	UINT8 _f, _ov; \
	__asm { bt		CPU_FLAG, 0	} \
	__asm { mov		ax, _d		} \
	__asm {	imul	_s			} \
	__asm { mov		_rl, ax		} \
	__asm { mov		_rh, dx		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_r = _rl + (_rh << 16); \
	_WORD_IMUL((r), (d), (s)); \
	if ((_r != (UINT32)(r)) || ((_f ^ CPU_FLAGL) & C_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("imulw r=%x:%x f=%x:%x o=%d %d", _r, r, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define DWORD_IMUL(r, d, s) { \
	UINT32 _d = (d); \
	UINT32 _s = (s); \
	UINT32 _rl, _rh; \
	UINT64 _r; \
	UINT8 _f, _ov; \
	__asm { bt		CPU_FLAG, 0	} \
	__asm { mov		eax, _d		} \
	__asm {	imul	_s			} \
	__asm { mov		_rl, eax	} \
	__asm { mov		_rh, edx	} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_r = (UINT64)_rl + ((UINT64)_rh << 32); \
	_DWORD_IMUL((r), (d), (s)); \
	if ((_r != (UINT64)(r)) || ((_f ^ CPU_FLAGL) & C_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("imuld r=%x%x:%x%x f=%x:%x o=%d %d", _r, r, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define BYTE_MUL(r, d, s) { \
	UINT8 _d = (d); \
	UINT8 _s = (s); \
	UINT16 _r; \
	UINT8 _f, _ov; \
	__asm { bt		CPU_FLAG, 0	} \
	__asm { mov		al, _d		} \
	__asm {	mul		_s			} \
	__asm { mov		_r, ax		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_BYTE_MUL((r), (d), (s)); \
	if ((_r != (UINT16)(r)) || ((_f ^ CPU_FLAGL) & C_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("mulb r=%x:%x f=%x:%x o=%d %d", _r, r, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define WORD_MUL(r, d, s) { \
	UINT16 _d = (d); \
	UINT16 _s = (s); \
	UINT16 _rl, _rh; \
	UINT32 _r; \
	UINT8 _f, _ov; \
	__asm { bt		CPU_FLAG, 0	} \
	__asm { mov		ax, _d		} \
	__asm {	mul		_s			} \
	__asm { mov		_rl, ax		} \
	__asm { mov		_rh, dx		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_r = _rl + (_rh << 16); \
	_WORD_MUL((r), (d), (s)); \
	if ((_r != (UINT32)(r)) || ((_f ^ CPU_FLAGL) & C_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("mulw r=%x:%x f=%x:%x o=%d %d", _r, r, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define DWORD_MUL(r, d, s) { \
	UINT32 _d = (d); \
	UINT32 _s = (s); \
	UINT32 _r, _o; \
	UINT8 _f, _ov; \
	__asm { bt		CPU_FLAG, 0	} \
	__asm { mov		eax, _d		} \
	__asm {	mul		_s			} \
	__asm { mov		_r, eax		} \
	__asm { mov		_o, edx		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_DWORD_MUL((r), (d), (s)); \
	if ((_r != (UINT32)(r)) || (_o != CPU_OV) || ((_f ^ CPU_FLAGL) & C_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("muld r=%x%x:%x%x f=%x:%x o=%d %d", _o, _r, CPU_OV, r, _f, CPU_FLAGL, _ov, CPU_OV)); \
}


// ----

#define AND_BYTE(d, s)		BYTE_AND(d, s)
#define AND_WORD(d, s)		WORD_AND(d, s)
#define AND_DWORD(d, s)		DWORD_AND(d, s)

#define	XC_STORE_FLAGL()

