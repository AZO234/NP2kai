#include	"compiler.h"
#include	"unasm.h"
#include	"unasmdef.tbl"
#include	"unasmstr.tbl"
#include	"unasmop3.tbl"
#include	"unasmop.tbl"
#include	"unasmop8.tbl"
#include	"unasmfpu.tbl"


static const char strhex[] = "0123456789abcdef";

static char *set_str(char *p, const char *string) {

	char	c;

	while(1) {
		c = *string++;
		if (c == '\0') {
			break;
		}
		*p++ = c;
	}
	return(p);
}

static char *set_reg(char *p, UINT reg) {

const char	*r;

	r = rstr.reg[0][reg];
	p[0] = r[0];
	p[1] = r[1];
	p += 2;
	if (r[2]) {
		*p++ = r[2];
		if (r[3]) {
			*p++ = r[3];
		}
	}
	return(p);
}

static char *set_shex(char *str, UINT32 value, UINT bit) {

	if (!(value & (1 << (bit - 1)))) {
		*str++ = '+';
	}
	else {
		*str++ = '-';
		value = 0 - value;
	}
	do {
		bit -= 4;
		*str++ = strhex[(value >> bit) & 15];
	} while(bit);
	return(str);
}

static char *set_hex(char *str, UINT32 value, UINT bit) {

	do {
		bit -= 4;
		*str++ = strhex[(value >> bit) & 15];
	} while(bit);
	return(str);
}

// ----

UINT unasm(UNASM r, const UINT8 *ptr, UINT leng, BOOL d, UINT32 addr) {

const UINT8	*org;
const UINT8	*term;
	UINT	flag;
	REG8	ope;
	UINT32	ctl;
	char	*p;
	UINT	type;
	UINT	mnemonic;
	UINT	regtype;
	UINT	f;
	_UNASM	una;

	if (r == NULL) {
		r = &una;
	}

	org = ptr;
	term = ptr + leng;
	flag = 0;
	if (ptr >= term) {
		return(0);
	}
	ope = *ptr++;
	ctl = optbl[ope];
	if (ctl & 1) {
		do {
			if (ptr >= term) {
				return(0);
			}
			ope = *ptr++;				// next

			type = (ctl >> 1) & 7;
			ctl >>= 4;
			switch(type) {
				case OPF_SOR:
					flag &= ~(UAFLAG_SOMASK << UAFLAG_SOR);
					flag += ctl << UAFLAG_SOR;
					ctl = optbl[ope];
					break;

				case OPF_REP:
					flag &= ~(UAFLAG_REPMASK << UAFLAG_REP);
					flag += ctl << UAFLAG_REP;
					ctl = optbl[ope];
					break;

				case OPF_OPE:
					flag |= 1 << UAFLAG_OPE;
					ctl = optbl[ope];
					break;

				case OPF_ADDR:
					flag |= 1 << UAFLAG_ADDR;
					ctl = optbl[ope];
					break;

				case OPF_286:
					ctl = op8tbl[ope];
					break;

				case OPF_OP3:
					ctl = op3tbl[ctl][(ope >> 3) & 7];
					break;

				case OPF_FPU:
					if (ope < 0xc0) {
						ctl = opefpu1[ctl][(ope >> 3) & 7];
					}
					else {
						ctl = opefpu2[ctl][ope - 0xc0];
					}
					break;
			}
		} while(ctl & 1);
	}
	if (d) {
		flag ^= (1 << UAFLAG_D) + (1 << UAFLAG_OPE) + (1 << UAFLAG_ADDR);
	}

	if (ctl & 0x10) {
		if (ptr >= term) {
			return(0);
		}
		ope = *ptr++;
	}
	if (!(ctl & 0x20)) {
		regtype = (flag << (3 - UAFLAG_OPE)) & 8;
	}
	else {
		regtype = RSTR_8 << 3;
		flag |= (1 << UAFLAG_8BIT);
	}

	mnemonic = (ctl >> 7) & 0x1ff;
	type = (ctl >> 16) & 0x3f;
	ctl >>= 22;
	p = r->operand;

opeana_st:
	switch(type) {
		case 0:
			goto opeana_ed;

		case OP_CL:
			p[0] = 'c';
			p[1] = 'l';
			p += 2;
			break;

		case OP_1:
			*p++ = '1';
			break;

		case OP_I8:
			if (ptr >= term) {
				return(0);
			}
			p = set_hex(p, *ptr++, 8);
			break;

		case OP_IS8:
			if (ptr >= term) {
				return(0);
			}
			p = set_shex(p, *ptr++, 8);
			break;

		case OP_I16:
			ptr += 2;
			if (ptr > term) {
				return(0);
			}
			p = set_hex(p, LOADINTELWORD(ptr - 2), 16);
			break;

		case OP_IMM:
			if (flag & (1 << UAFLAG_8BIT)) {
				if (ptr >= term) {
					return(0);
				}
				p = set_hex(p, *ptr++, 8);
			}
			else if (!(flag & (1 << UAFLAG_OPE))) {
				ptr += 2;
				if (ptr > term) {
					return(0);
				}
				p = set_hex(p, LOADINTELWORD(ptr - 2), 16);
			}
			else {
				ptr += 4;
				if (ptr > term) {
					return(0);
				}
				p = set_hex(p, LOADINTELDWORD(ptr - 4), 32);
			}
			break;

		case OP_EA:
			if (ope >= 0xc0) {
				p = set_reg(p, regtype + (ope & 7));
				break;
			}
opeana_ea:
			f = (flag >> UAFLAG_SOR) & UAFLAG_SOMASK;
			if (f) {
				p[0] = rstr.reg[RSTR_SEG][f - 1][0];
				p[1] = rstr.reg[RSTR_SEG][f - 1][1];
				p[2] = ':';
				p += 3;
			}
			*p++ = '[';
			if (!(flag & (1 << UAFLAG_ADDR))) {
				if ((ope & 0xc7) != 0x06) {
					p = set_str(p, rstr.lea[ope & 7]);
					switch(ope & 0xc0) {
						case 0x40:
							if (ptr >= term) {
								return(0);
							}
							p = set_shex(p, *ptr++, 8);
							break;

						case 0x80:
							ptr += 2;
							if (ptr > term) {
								return(0);
							}
							*p++ = '+';
							p = set_hex(p, LOADINTELWORD(ptr - 2), 16);
							break;
					}
				}
				else {
					ptr += 2;
					if (ptr > term) {
						return(0);
					}
					p = set_hex(p, LOADINTELWORD(ptr - 2), 16);
				}
			}
			else {
				if ((ope & 7) != 4) {
					f = ope & 0xc7;
				}
				else {
					if (ptr >= term) {
						return(0);
					}
					f = (ptr[0] >> 3) & 7;
					if (f != 4) {
						p = set_str(p, rstr.reg[RSTR_32][f]);
						if (ptr[0] & 0xc0) {
							p[0] = '*';
							p[1] = (char)('0' + (1 << (ptr[0] >> 6)));
							p += 2;
						}
						*p++ = '+';
					}
					f = (ope & 0xc0) + (ptr[0] & 7);
					ptr++;
				}
				if (f != 0x05) {
					p = set_str(p, rstr.reg[RSTR_32][f & 7]);
					switch(f & 0xc0) {
						case 0x40:
							if (ptr >= term) {
								return(0);
							}
							p = set_shex(p, *ptr++, 8);
							break;

						case 0x80:
							ptr += 4;
							if (ptr > term) {
								return(0);
							}
							*p++ = '+';
							p = set_hex(p, LOADINTELDWORD(ptr - 4), 32);
							break;
					}
				}
				else {
					ptr += 4;
					if (ptr > term) {
						return(0);
					}
					p = set_hex(p, LOADINTELDWORD(ptr - 4), 32);
				}
			}
			*p++ = ']';
			break;

		case OP_PEA:
			if (ope >= 0xc0) {
				p = set_reg(p, regtype + (ope & 7));
				break;
			}
			p = set_str(p, rstr.ptr[regtype >> 3]);
			goto opeana_ea;

		case OP_REG:
			p = set_reg(p, regtype + ((ope >> 3) & 7));
			break;

		case OP_SEG:
			p = set_str(p, rstr.reg[RSTR_SEG][(ope >> 3) & 7]);
			break;

		case OP_MEM:
			f = (flag >> UAFLAG_SOR) & UAFLAG_SOMASK;
			if (f) {
				p[0] = rstr.reg[RSTR_SEG][f - 1][0];
				p[1] = rstr.reg[RSTR_SEG][f - 1][1];
				p[2] = ':';
				p += 3;
			}
			*p++ = '[';
			if (!(flag & (1 << UAFLAG_D))) {
				ptr += 2;
				if (ptr > term) {
					return(0);
				}
				p = set_hex(p, LOADINTELWORD(ptr - 2), 16);
			}
			else {
				ptr += 4;
				if (ptr > term) {
					return(0);
				}
				p = set_hex(p, LOADINTELDWORD(ptr - 4), 32);
			}
			*p++ = ']';
			break;

		case OP_AX:
			p = set_reg(p, regtype);
			break;

		case OP_DX:
			p[0] = 'd';
			p[1] = 'x';
			p += 2;
			break;

		case OP_MR:
			p = set_reg(p, regtype + (ope & 7));
			break;

		case OP1_32:
			mnemonic += ((flag >> UAFLAG_OPE) & 1);
			break;

		case OP1_3:
			*p++ = '3';
			break;

		case OP1_STR:
			mnemonic += (regtype >> 3);
			f = (flag >> UAFLAG_REP) & UAFLAG_REPMASK;
			if (f) {
				p = set_str(p, rstr.ope[mnemonic]);
				mnemonic = RSTR_REP + (f - 1);
			}
			f = (flag >> UAFLAG_SOR) & UAFLAG_SOMASK;
			if (f) {
				p[0] = ' ';
				p[1] = rstr.reg[RSTR_SEG][f - 1][0];
				p[1] = rstr.reg[RSTR_SEG][f - 1][1];
				p[3] = ':';
				p += 4;
			}
			break;

		case OP1_JCXZ:
			mnemonic += ((flag >> UAFLAG_OPE) & 1);

		case OP1_SHORT:
			if (ptr >= term) {
				return(0);
			}
			addr += (SINT32)(SINT8)(*ptr++);
			addr += (int)(ptr - org);
			p = set_hex(p, addr, (flag & (1 << UAFLAG_D))?32:16);
			break;

		case OP1_NEAR:
			if (!(flag & (1 << UAFLAG_D))) {
				ptr += 2;
				if (ptr > term) {
					return(0);
				}
				addr += LOADINTELWORD(ptr - 2);
				addr += (int)(ptr - org);
				p = set_hex(p, addr, 16);
			}
			else {
				ptr += 4;
				if (ptr > term) {
					return(0);
				}
				addr += LOADINTELDWORD(ptr - 4);
				addr += (int)(ptr - org);
				p = set_hex(p, addr, 32);
			}
			break;

		case OP1_FAR:
			if (!(flag & (1 << UAFLAG_D))) {
				ptr += 4;
				if (ptr > term) {
					return(0);
				}
				p = set_hex(p, LOADINTELWORD(ptr - 2), 16);
				*p++ = ':';
				p = set_hex(p, LOADINTELWORD(ptr - 4), 16);
			}
			else {
				ptr += 6;
				if (ptr > term) {
					return(0);
				}
				p = set_hex(p, LOADINTELWORD(ptr - 2), 16);
				*p++ = ':';
				p = set_hex(p, LOADINTELDWORD(ptr - 6), 32);
			}
			break;

		case OP1_I10:
			if (ptr >= term) {
				return(0);
			}
			if (*ptr != 10) {
				p = set_hex(p, *ptr, 8);
			}
			ptr++;
			break;

		case OP1_SOR:
			f = (flag >> UAFLAG_SOR) & UAFLAG_SOMASK;
			if (f) {
				p[0] = rstr.reg[RSTR_SEG][f - 1][0];
				p[1] = rstr.reg[RSTR_SEG][f - 1][1];
				p[2] = ':';
				p += 3;
			}
			break;

		case OP1_FEA:
			if (ope >= 0xc0) {				// —L‚è“¾‚È‚¢‚¯‚Ç
				p = set_reg(p, regtype + (ope & 7));
				break;
			}
			p = set_str(p, "far ");
			goto opeana_ea;

		case OP1_REx:
			p = set_reg(p, (RSTR_32 << 3) + (ope & 7));
			p[0] = ',';
			p[1] = ' ';
			p[2] = (char)mnemonic;
			p[3] = 'r';
			p[4] = (char)('0' + ((ope >> 3) & 7));
			p += 5;
			mnemonic = RSTR_MOV;
			break;

		case OP1_ExR:
			p[0] = (char)mnemonic;
			p[1] = 'r';
			p[2] = (char)('0' + ((ope >> 3) & 7));
			p[3] = ',';
			p[4] = ' ';
			p += 5;
			p = set_reg(p, (RSTR_32 << 3) + (ope & 7));
			mnemonic = RSTR_MOV;
			break;

		case OP1_E2:
			p = set_str(p, "word ptr ");
			goto opeana_ea;

		case OP1_E4:
			p = set_str(p, "dword ptr ");
			goto opeana_ea;

		case OP1_E8:
			p = set_str(p, "qword ptr ");
			goto opeana_ea;

		case OP1_ET:
			p = set_str(p, "tword ptr ");
			goto opeana_ea;

		case OP1_ST:
			p[0] = 's';
			p[1] = 't';
			p[2] = '(';
			p[3] = (char)('0' + (ope & 7));
			p[4] = ')';
			p += 5;
			break;

		case OP1_ST1:
			p[0] = 's';
			p[1] = 't';
			p[2] = '(';
			p[3] = (char)('0' + (ope & 7));
			p[4] = ')';
			p[5] = ',';
			p[6] = ' ';
			p[7] = 's';
			p[8] = 't';
			p += 9;
			break;

		case OP1_ST2:
			p[0] = 's';
			p[1] = 't';
			p[2] = ',';
			p[3] = ' ';
			p[4] = 's';
			p[5] = 't';
			p[6] = '(';
			p[7] = (char)('0' + (ope & 7));
			p[8] = ')';
			p += 9;
			break;

		case OE_MX0:
			p = set_reg(p, regtype + ((ope >> 3) & 7));
			regtype = RSTR_8 << 3;
			break;

		case OE_MX1:
			p = set_reg(p, regtype + ((ope >> 3) & 7));
			regtype = RSTR_16 << 3;
			break;

		default:
			return(0);
	}
	type = ctl & 0x1f;
	ctl >>= 5;
	if (type) {
		p[0] = ',';
		p[1] = ' ';
		p += 2;
		goto opeana_st;
	}

opeana_ed:
	r->mnemonic = rstr.ope[mnemonic];
	p[0] = '\0';
	return((UINT)(ptr - org));
}

