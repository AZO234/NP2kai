
// Crosscheck for VC++

#define	SETFLAGS \
	__asm { cmp		CPU_OV, 0		} \
	__asm { setne	al				} \
	__asm { add		al, 7fh			} \
	__asm { mov		ah, CPU_FLAGL	} \
	__asm { sahf					}



#define	BYTE_SAR1(d, s) { \
	UINT8 _d = (s); \
	UINT8 _f, _ov; \
	SETFLAGS \
	__asm { sar		_d, 1		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_BYTE_SAR1(d, s); \
	if ((_d != (UINT8)(d)) || ((_f ^ CPU_FLAGL) & SZPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("sarb1 r=%x:%x f=%x:%x o=%d %d", _d, d, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define	WORD_SAR1(d, s) { \
	UINT16 _d = (s); \
	UINT8 _f, _ov; \
	SETFLAGS \
	__asm { sar		_d, 1		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_WORD_SAR1(d, s); \
	if ((_d != (UINT16)(d)) || ((_f ^ CPU_FLAGL) & SZPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("sarw1 r=%x:%x f=%x:%x o=%d %d", _d, d, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define	DWORD_SAR1(d, s) { \
	UINT32 _d = (s); \
	UINT8 _f, _ov; \
	SETFLAGS \
	__asm { sar		_d, 1		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_DWORD_SAR1(d, s); \
	if ((_d != (UINT32)(d)) || ((_f ^ CPU_FLAGL) & SZPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("sard1 r=%x:%x f=%x:%x o=%d %d", _d, d, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define	BYTE_SARCL(d, s, c) { \
	UINT8 _c = (c); \
	UINT8 _d = (s); \
	UINT8 _f, _ov; \
	SETFLAGS \
	__asm {	mov		cl, _c		} \
	__asm { sar		_d, cl		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_BYTE_SARCL(d, s, c); \
	if ((_d != (UINT8)(d)) || ((_f ^ CPU_FLAGL) & SZPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("sarb(%x) r=%x:%x f=%x:%x o=%d %d", _c, _d, d, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define	WORD_SARCL(d, s, c) { \
	UINT8 _c = (c); \
	UINT16 _d = (s); \
	UINT8 _f, _ov; \
	SETFLAGS \
	__asm {	mov		cl, _c		} \
	__asm { sar		_d, cl		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_WORD_SARCL(d, s, c); \
	if ((_d != (UINT16)(d)) || ((_f ^ CPU_FLAGL) & SZPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("sarw(%x) r=%x:%x f=%x:%x o=%d %d", _c, _d, d, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define	DWORD_SARCL(d, s, c) { \
	UINT8 _c = (c); \
	UINT32 _d = (s); \
	UINT8 _f, _ov; \
	SETFLAGS \
	__asm {	mov		cl, _c		} \
	__asm { sar		_d, cl		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_DWORD_SARCL(d, s, c); \
	if ((_d != (UINT32)(d)) || ((_f ^ CPU_FLAGL) & SZPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("sard(%x) r=%x:%x f=%x:%x o=%d %d", _c, _d, d, _f, CPU_FLAGL, _ov, CPU_OV)); \
}



#define	BYTE_SHL1(d, s) { \
	UINT8 _d = (s); \
	UINT8 _f, _ov; \
	SETFLAGS \
	__asm { shl		_d, 1		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_BYTE_SHL1(d, s); \
	if ((_d != (UINT8)(d)) || ((_f ^ CPU_FLAGL) & SZPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("shlb1 r=%x:%x f=%x:%x o=%d %d", _d, d, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define	WORD_SHL1(d, s) { \
	UINT16 _d = (s); \
	UINT8 _f, _ov; \
	SETFLAGS \
	__asm { shl		_d, 1		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_WORD_SHL1(d, s); \
	if ((_d != (UINT16)(d)) || ((_f ^ CPU_FLAGL) & SZPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("shlw1 r=%x:%x f=%x:%x o=%d %d", _d, d, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define	DWORD_SHL1(d, s) { \
	UINT32 _d = (s); \
	UINT8 _f, _ov; \
	SETFLAGS \
	__asm { shl		_d, 1		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_DWORD_SHL1(d, s); \
	if ((_d != (UINT32)(d)) || ((_f ^ CPU_FLAGL) & SZPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("shld1 r=%x:%x f=%x:%x o=%d %d", _d, d, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define	BYTE_SHLCL(d, s, c) { \
	UINT8 _c = (c); \
	UINT8 _d = (s); \
	UINT8 _f, _ov; \
	SETFLAGS \
	__asm {	mov		cl, _c		} \
	__asm { shl		_d, cl		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_BYTE_SHLCL(d, s, c); \
	if ((_d != (UINT8)(d)) || ((_f ^ CPU_FLAGL) & SZPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("shlb(%x) r=%x:%x f=%x:%x o=%d %d", _c, _d, d, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define	WORD_SHLCL(d, s, c) { \
	UINT8 _c = (c); \
	UINT16 _d = (s); \
	UINT8 _f, _ov; \
	SETFLAGS \
	__asm {	mov		cl, _c		} \
	__asm { shl		_d, cl		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_WORD_SHLCL(d, s, c); \
	if ((_d != (UINT16)(d)) || ((_f ^ CPU_FLAGL) & SZPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("shlw(%x) r=%x:%x f=%x:%x o=%d %d", _c, _d, d, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define	DWORD_SHLCL(d, s, c) { \
	UINT8 _c = (c); \
	UINT32 _d = (s); \
	UINT8 _f, _ov; \
	SETFLAGS \
	__asm {	mov		cl, _c		} \
	__asm { shl		_d, cl		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_DWORD_SHLCL(d, s, c); \
	if ((_d != (UINT32)(d)) || ((_f ^ CPU_FLAGL) & SZPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("shld(%x) r=%x:%x f=%x:%x o=%d %d", _c, _d, d, _f, CPU_FLAGL, _ov, CPU_OV)); \
}



#define	BYTE_SHR1(d, s) { \
	UINT8 _d = (s); \
	UINT8 _f, _ov; \
	SETFLAGS \
	__asm { shr		_d, 1		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_BYTE_SHR1(d, s); \
	if ((_d != (UINT8)(d)) || ((_f ^ CPU_FLAGL) & SZPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("shrb1 r=%x:%x f=%x:%x o=%d %d", _d, d, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define	WORD_SHR1(d, s) { \
	UINT16 _d = (s); \
	UINT8 _f, _ov; \
	SETFLAGS \
	__asm { shr		_d, 1		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_WORD_SHR1(d, s); \
	if ((_d != (UINT16)(d)) || ((_f ^ CPU_FLAGL) & SZPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("shrw1 r=%x:%x f=%x:%x o=%d %d", _d, d, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define	DWORD_SHR1(d, s) { \
	UINT32 _d = (s); \
	UINT8 _f, _ov; \
	SETFLAGS \
	__asm { shr		_d, 1		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_DWORD_SHR1(d, s); \
	if ((_d != (UINT32)(d)) || ((_f ^ CPU_FLAGL) & SZPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("shrd1 r=%x:%x f=%x:%x o=%d %d", _d, d, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define	BYTE_SHRCL(d, s, c) { \
	UINT8 _c = (c); \
	UINT8 _d = (s); \
	UINT8 _f, _ov; \
	SETFLAGS \
	__asm {	mov		cl, _c		} \
	__asm { shr		_d, cl		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_BYTE_SHRCL(d, s, c); \
	if ((_d != (UINT8)(d)) || ((_f ^ CPU_FLAGL) & SZPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("shrb(%x) r=%x:%x f=%x:%x o=%d %d", _c, _d, d, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define	WORD_SHRCL(d, s, c) { \
	UINT8 _c = (c); \
	UINT16 _d = (s); \
	UINT8 _f, _ov; \
	SETFLAGS \
	__asm {	mov		cl, _c		} \
	__asm { shr		_d, cl		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_WORD_SHRCL(d, s, c); \
	if ((_d != (UINT16)(d)) || ((_f ^ CPU_FLAGL) & SZPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("shrw(%x) r=%x:%x f=%x:%x o=%d %d", _c, _d, d, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define	DWORD_SHRCL(d, s, c) { \
	UINT8 _c = (c); \
	UINT32 _d = (s); \
	UINT8 _f, _ov; \
	SETFLAGS \
	__asm {	mov		cl, _c		} \
	__asm { shr		_d, cl		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_DWORD_SHRCL(d, s, c); \
	if ((_d != (UINT32)(d)) || ((_f ^ CPU_FLAGL) & SZPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("shrd(%x) r=%x:%x f=%x:%x o=%d %d", _c, _d, d, _f, CPU_FLAGL, _ov, CPU_OV)); \
}



#define	WORD_SHLD(d, s, c) { \
	UINT8 _c = (c); \
	UINT16 _s = (s); \
	UINT16 _d = (d); \
	UINT8 _f, _ov; \
	SETFLAGS \
	__asm {	mov		cl, _c		} \
	__asm {	mov		ax, _s		} \
	__asm { shld	_d, ax, cl	} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_WORD_SHLD(d, s, c); \
	if ((_d != (UINT16)(d)) || ((_f ^ CPU_FLAGL) & SZPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("shldw r=%x:%x f=%x:%x o=%d %d", _d, d, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define	DWORD_SHLD(d, s, c) { \
	UINT8 _c = (c); \
	UINT32 _s = (s); \
	UINT32 _d = (d); \
	UINT8 _f, _ov; \
	SETFLAGS \
	__asm {	mov		cl, _c		} \
	__asm {	mov		eax, _s		} \
	__asm { shld	_d, eax, cl	} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_DWORD_SHLD(d, s, c); \
	if ((_d != (UINT32)(d)) || ((_f ^ CPU_FLAGL) & SZPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("shldd r=%x:%x f=%x:%x o=%d %d", _d, d, _f, CPU_FLAGL, _ov, CPU_OV)); \
}



#define	WORD_SHRD(d, s, c) { \
	UINT8 _c = (c); \
	UINT16 _s = (s); \
	UINT16 _d = (d); \
	UINT8 _f, _ov; \
	SETFLAGS \
	__asm {	mov		cl, _c		} \
	__asm {	mov		ax, _s		} \
	__asm { shrd	_d, ax, cl	} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_WORD_SHRD(d, s, c); \
	if ((_d != (UINT16)(d)) || ((_f ^ CPU_FLAGL) & SZPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("shrdw r=%x:%x f=%x:%x o=%d %d", _d, d, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define	DWORD_SHRD(d, s, c) { \
	UINT8 _c = (c); \
	UINT32 _s = (s); \
	UINT32 _d = (d); \
	UINT8 _f, _ov; \
	SETFLAGS \
	__asm {	mov		cl, _c		} \
	__asm {	mov		eax, _s		} \
	__asm { shrd	_d, eax, cl	} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_DWORD_SHRD(d, s, c); \
	if ((_d != (UINT32)(d)) || ((_f ^ CPU_FLAGL) & SZPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("shrdd r=%x:%x f=%x:%x o=%d %d", _d, d, _f, CPU_FLAGL, _ov, CPU_OV)); \
}



#define	BYTE_RCL1(d, s) { \
	UINT8 _d = (s); \
	UINT8 _f, _ov; \
	SETFLAGS \
	__asm { rcl		_d, 1		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_BYTE_RCL1(d, s); \
	if ((_d != (UINT8)(d)) || ((_f ^ CPU_FLAGL) & SZPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("rclb1 r=%x:%x f=%x:%x o=%d %d", _d, d, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define	WORD_RCL1(d, s) { \
	UINT16 _d = (s); \
	UINT8 _f, _ov; \
	SETFLAGS \
	__asm { rcl		_d, 1		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_WORD_RCL1(d, s); \
	if ((_d != (UINT16)(d)) || ((_f ^ CPU_FLAGL) & SZPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("rclw1 r=%x:%x f=%x:%x o=%d %d", _d, d, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define	DWORD_RCL1(d, s) { \
	UINT32 _d = (s); \
	UINT8 _f, _ov; \
	SETFLAGS \
	__asm { rcl		_d, 1		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_DWORD_RCL1(d, s); \
	if ((_d != (UINT32)(d)) || ((_f ^ CPU_FLAGL) & SZPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("rcld1 r=%x:%x f=%x:%x o=%d %d", _d, d, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define	BYTE_RCLCL(d, s, c) { \
	UINT8 _c = (c); \
	UINT8 _d = (s); \
	UINT8 _f, _ov; \
	SETFLAGS \
	__asm {	mov		cl, _c		} \
	__asm { rcl		_d, cl		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_BYTE_RCLCL(d, s, c); \
	if ((_d != (UINT8)(d)) || ((_f ^ CPU_FLAGL) & SZPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("rclb(%x) r=%x:%x f=%x:%x o=%d %d", _c, _d, d, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define	WORD_RCLCL(d, s, c) { \
	UINT8 _c = (c); \
	UINT16 _d = (s); \
	UINT8 _f, _ov; \
	SETFLAGS \
	__asm {	mov		cl, _c		} \
	__asm { rcl		_d, cl		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_WORD_RCLCL(d, s, c); \
	if ((_d != (UINT16)(d)) || ((_f ^ CPU_FLAGL) & SZPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("rclw(%x) r=%x:%x f=%x:%x o=%d %d", _c, _d, d, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define	DWORD_RCLCL(d, s, c) { \
	UINT8 _c = (c); \
	UINT32 _d = (s); \
	UINT8 _f, _ov; \
	SETFLAGS \
	__asm {	mov		cl, _c		} \
	__asm { rcl		_d, cl		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_DWORD_RCLCL(d, s, c); \
	if ((_d != (UINT32)(d)) || ((_f ^ CPU_FLAGL) & SZPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("rcld(%x) r=%x:%x f=%x:%x o=%d %d", _c, _d, d, _f, CPU_FLAGL, _ov, CPU_OV)); \
}



#define	BYTE_RCR1(d, s) { \
	UINT8 _d = (s); \
	UINT8 _f, _ov; \
	SETFLAGS \
	__asm { rcr		_d, 1		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_BYTE_RCR1(d, s); \
	if ((_d != (UINT8)(d)) || ((_f ^ CPU_FLAGL) & SZPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("rcrb1 r=%x:%x f=%x:%x o=%d %d", _d, d, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define	WORD_RCR1(d, s) { \
	UINT16 _d = (s); \
	UINT8 _f, _ov; \
	SETFLAGS \
	__asm { rcr		_d, 1		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_WORD_RCR1(d, s); \
	if ((_d != (UINT16)(d)) || ((_f ^ CPU_FLAGL) & SZPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("rcrw1 r=%x:%x f=%x:%x o=%d %d", _d, d, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define	DWORD_RCR1(d, s) { \
	UINT32 _d = (s); \
	UINT8 _f, _ov; \
	SETFLAGS \
	__asm { rcr		_d, 1		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_DWORD_RCR1(d, s); \
	if ((_d != (UINT32)(d)) || ((_f ^ CPU_FLAGL) & SZPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("rcrd1 r=%x:%x f=%x:%x o=%d %d", _d, d, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define	BYTE_RCRCL(d, s, c) { \
	UINT8 _c = (c); \
	UINT8 _d = (s); \
	UINT8 _f, _ov; \
	SETFLAGS \
	__asm {	mov		cl, _c		} \
	__asm { rcr		_d, cl		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_BYTE_RCRCL(d, s, c); \
	if ((_d != (UINT8)(d)) || ((_f ^ CPU_FLAGL) & SZPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("rcrb(%x) r=%x:%x f=%x:%x o=%d %d", _c, _d, d, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define	WORD_RCRCL(d, s, c) { \
	UINT8 _c = (c); \
	UINT16 _d = (s); \
	UINT8 _f, _ov; \
	SETFLAGS \
	__asm {	mov		cl, _c		} \
	__asm { rcr		_d, cl		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_WORD_RCRCL(d, s, c); \
	if ((_d != (UINT16)(d)) || ((_f ^ CPU_FLAGL) & SZPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("rcrw(%x) r=%x:%x f=%x:%x o=%d %d", _c, _d, d, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define	DWORD_RCRCL(d, s, c) { \
	UINT8 _c = (c); \
	UINT32 _d = (s); \
	UINT8 _f, _ov; \
	SETFLAGS \
	__asm {	mov		cl, _c		} \
	__asm { rcr		_d, cl		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_DWORD_RCRCL(d, s, c); \
	if ((_d != (UINT32)(d)) || ((_f ^ CPU_FLAGL) & SZPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("rcrd(%x) r=%x:%x f=%x:%x o=%d %d", _c, _d, d, _f, CPU_FLAGL, _ov, CPU_OV)); \
}



#define	BYTE_ROL1(d, s) { \
	UINT8 _d = (s); \
	UINT8 _f, _ov; \
	SETFLAGS \
	__asm { rol		_d, 1		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_BYTE_ROL1(d, s); \
	if ((_d != (UINT8)(d)) || ((_f ^ CPU_FLAGL) & SZPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("rolb1 r=%x:%x f=%x:%x o=%d %d", _d, d, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define	WORD_ROL1(d, s) { \
	UINT16 _d = (s); \
	UINT8 _f, _ov; \
	SETFLAGS \
	__asm { rol		_d, 1		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_WORD_ROL1(d, s); \
	if ((_d != (UINT16)(d)) || ((_f ^ CPU_FLAGL) & SZPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("rolw1 r=%x:%x f=%x:%x o=%d %d", _d, d, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define	DWORD_ROL1(d, s) { \
	UINT32 _d = (s); \
	UINT8 _f, _ov; \
	SETFLAGS \
	__asm { rol		_d, 1		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_DWORD_ROL1(d, s); \
	if ((_d != (UINT32)(d)) || ((_f ^ CPU_FLAGL) & SZPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("rold1 r=%x:%x f=%x:%x o=%d %d", _d, d, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define	BYTE_ROLCL(d, s, c) { \
	UINT8 _c = (c); \
	UINT8 _d = (s); \
	UINT8 _f, _ov; \
	SETFLAGS \
	__asm {	mov		cl, _c		} \
	__asm { rol		_d, cl		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_BYTE_ROLCL(d, s, c); \
	if ((_d != (UINT8)(d)) || ((_f ^ CPU_FLAGL) & SZPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("rolb(%x) r=%x:%x f=%x:%x o=%d %d", _c, _d, d, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define	WORD_ROLCL(d, s, c) { \
	UINT8 _c = (c); \
	UINT16 _d = (s); \
	UINT8 _f, _ov; \
	SETFLAGS \
	__asm {	mov		cl, _c		} \
	__asm { rol		_d, cl		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_WORD_ROLCL(d, s, c); \
	if ((_d != (UINT16)(d)) || ((_f ^ CPU_FLAGL) & SZPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("rolw(%x) r=%x:%x f=%x:%x o=%d %d", _c, _d, d, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define	DWORD_ROLCL(d, s, c) { \
	UINT8 _c = (c); \
	UINT32 _d = (s); \
	UINT8 _f, _ov; \
	SETFLAGS \
	__asm {	mov		cl, _c		} \
	__asm { rol		_d, cl		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_DWORD_ROLCL(d, s, c); \
	if ((_d != (UINT32)(d)) || ((_f ^ CPU_FLAGL) & SZPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("rold(%x) r=%x:%x f=%x:%x o=%d %d", _c, _d, d, _f, CPU_FLAGL, _ov, CPU_OV)); \
}



#define	BYTE_ROR1(d, s) { \
	UINT8 _d = (s); \
	UINT8 _f, _ov; \
	SETFLAGS \
	__asm { ror		_d, 1		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_BYTE_ROR1(d, s); \
	if ((_d != (UINT8)(d)) || ((_f ^ CPU_FLAGL) & SZPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("rorb1 r=%x:%x f=%x:%x o=%d %d", _d, d, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define	WORD_ROR1(d, s) { \
	UINT16 _d = (s); \
	UINT8 _f, _ov; \
	SETFLAGS \
	__asm { ror		_d, 1		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_WORD_ROR1(d, s); \
	if ((_d != (UINT16)(d)) || ((_f ^ CPU_FLAGL) & SZPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("rorw1 r=%x:%x f=%x:%x o=%d %d", _d, d, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define	DWORD_ROR1(d, s) { \
	UINT32 _d = (s); \
	UINT8 _f, _ov; \
	SETFLAGS \
	__asm { ror		_d, 1		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_DWORD_ROR1(d, s); \
	if ((_d != (UINT32)(d)) || ((_f ^ CPU_FLAGL) & SZPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("rord1 r=%x:%x f=%x:%x o=%d %d", _d, d, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define	BYTE_RORCL(d, s, c) { \
	UINT8 _c = (c); \
	UINT8 _d = (s); \
	UINT8 _f, _ov; \
	SETFLAGS \
	__asm {	mov		cl, _c		} \
	__asm { ror		_d, cl		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_BYTE_RORCL(d, s, c); \
	if ((_d != (UINT8)(d)) || ((_f ^ CPU_FLAGL) & SZPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("rorb(%x) r=%x:%x f=%x:%x o=%d %d", _c, _d, d, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define	WORD_RORCL(d, s, c) { \
	UINT8 _c = (c); \
	UINT16 _d = (s); \
	UINT8 _f, _ov; \
	SETFLAGS \
	__asm {	mov		cl, _c		} \
	__asm { ror		_d, cl		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_WORD_RORCL(d, s, c); \
	if ((_d != (UINT16)(d)) || ((_f ^ CPU_FLAGL) & SZPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("rorw(%x) r=%x:%x f=%x:%x o=%d %d", _c, _d, d, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

#define	DWORD_RORCL(d, s, c) { \
	UINT8 _c = (c); \
	UINT32 _d = (s); \
	UINT8 _f, _ov; \
	SETFLAGS \
	__asm {	mov		cl, _c		} \
	__asm { ror		_d, cl		} \
	__asm { seto	_ov			} \
	__asm {	lahf				} \
	__asm {	mov		_f, ah		} \
	_DWORD_RORCL(d, s, c); \
	if ((_d != (UINT32)(d)) || ((_f ^ CPU_FLAGL) & SZPC_FLAG) || ((!_ov) != (!CPU_OV))) TRACEOUT(("rord(%x) r=%x:%x f=%x:%x o=%d %d", _c, _d, d, _f, CPU_FLAGL, _ov, CPU_OV)); \
}

