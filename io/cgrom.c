#include	<compiler.h>
#include	<cpucore.h>
#include	<pccore.h>
#include	<io/iocore.h>
#include	<font/font.h>
#if defined(SUPPORT_TEXTHOOK)
#include	<codecnv/codecnv.h>
#endif


static void cgwindowset(CGROM cr) {

	UINT	low;
	UINT	high;
	int		code;

	cgwindow.writable &= ~1;
	low = 0x7fff0;
	if (grcg.chip >= 2) {
		if (!(cr->code & 0xff00)) {
			high = 0x80000 + (cr->code << 4);
			if (!(gdc.mode1 & 8)) {
				high += 0x2000;
			}
		}
		else {
			code = cr->code & 0x007f;
			high = (cr->code & 0x7f7f) << 4;
			if ((code >= 0x56) && (code < 0x58)) {
				cgwindow.writable |= 1;
				high += cr->lr;
			}
			else if ((code >= 0x09) && (code < 0x0c)) {				// ver0.78
				if (cr->lr) {
					high = low;
				}
			}
			else if (((code >= 0x0c) && (code < 0x10)) ||
				((code >= 0x58) && (code < 0x60))) {
				high += cr->lr;
			}
			else {
				low = high;
				high += 0x800;
			}
		}
	}
	else {
		high = low;
	}
	cgwindow.low = low;
	cgwindow.high = high;
}


// ---- I/O

// write charactor code low
static void IOOUTCALL cgrom_oa1(UINT port, REG8 dat) {

	CGROM	cr;

//	TRACEOUT(("%.4x:%.2x [%.4x:%.4x]", port, dat, CPU_CS, CPU_IP));
	hf_codeul |= 0x1;
	hf_count = 0;
	cr = &cgrom;
	cr->code = (dat << 8) | (cr->code & 0xff);
//#if defined(SUPPORT_TEXTHOOK)
//	if(np2cfg.usetexthook){
//		UINT16 SJis;
//		UINT8 th[3];
//		UINT16 thw[2];
//		thw[1]='¥0';
//		SJis = font_Jis2Sjis(((cr->code + 0x20) << 8) | (cr->code >> 8));
//		if(SJis){
//			th[0] = SJis >> 8; th[1] = SJis & 0x00ff; th[2] = '¥0';
//			codecnv_sjistoucs2(thw, 1, (const char*)th, 2);
//			font_outhooktest((wchar_t*)thw);
//		}
//	}
//#endif
	cgwindowset(cr);
	(void)port;
}

// write charactor code high
static void IOOUTCALL cgrom_oa3(UINT port, REG8 dat) {

	CGROM	cr;

//	TRACEOUT(("%.4x:%.2x [%.4x:%.4x]", port, dat, CPU_CS, CPU_IP));
	hf_codeul |= 0x2;
	hf_count = 0;
	cr = &cgrom;
	cr->code = (cr->code & 0xff00) | dat;
#if defined(SUPPORT_TEXTHOOK)
	if(np2cfg.usetexthook){
		UINT16 SJis;
		UINT8 th[3];
		UINT16 thw[2];
		thw[1]='¥0';
		SJis = font_Jis2Sjis(((cr->code + 0x20) << 8) | (cr->code >> 8));
		if(SJis){
			th[0] = SJis >> 8; th[1] = SJis & 0x00ff; th[2] = '¥0';
			codecnv_sjistoucs2(thw, 1, (const char*)th, 2);
			font_outhooktest((wchar_t*)thw);
		}
	}
#endif
	cgwindowset(cr);
	(void)port;
}

// write charactor line
static void IOOUTCALL cgrom_oa5(UINT port, REG8 dat) {

	CGROM	cr;

	cr = &cgrom;
	cr->line = dat & 0x1f;
	cr->lr = ((~dat) & 0x20) << 6;
#if defined(SUPPORT_TEXTHOOK)
	if(dat == 0 && np2cfg.usetexthook){
		UINT16 SJis;
		UINT8 th[3];
		UINT16 thw[2];
		thw[1]='¥0';
		SJis = font_Jis2Sjis(((cr->code + 0x20) << 8) | (cr->code >> 8));
		if(SJis){
			th[0] = SJis >> 8; th[1] = SJis & 0x00ff; th[2] = '¥0';
			codecnv_sjistoucs2(thw, 1, (const char*)th, 2);
			font_outhooktest((wchar_t*)thw);
		}
	}
#endif
	cgwindowset(cr);
	(void)port;
}

// CG write pattern
static void IOOUTCALL cgrom_oa9(UINT port, REG8 dat) {

	CGROM	cr;

	cr = &cgrom;
	if ((cr->code & 0x007e) == 0x0056) {
		fontrom[((cr->code & 0x7f7f) << 4) +
							cr->lr + (cr->line & 0x0f)] = (UINT8)dat;
		cgwindow.writable |= 0x80;
	}
	(void)port;
}

static REG8 IOINPCALL cgrom_ia9(UINT port) {

	CGROM	cr;
const UINT8	*ptr;
	int		type;

	cr = &cgrom;
	ptr = fontrom;
	type = cr->code & 0x00ff;
	if ((type >= 0x09) && (type < 0x0c)) {							// ver0.78
		if (!cr->lr) {
			if(hf_codeul && hf_count == 0) {
				hook_fontrom((cr->code & 0x7f7f) << 4);
				hf_codeul = 0;
				hf_count++;
			}
			ptr += (cr->code & 0x7f7f) << 4;
			return(ptr[cr->line & 0x0f]);
		}
	}
	else if (cr->code & 0xff00) {
		if(hf_codeul && hf_count == 0) {
			hook_fontrom((cr->code & 0x7f7f) << 4);
			hf_codeul = 0;
			hf_count++;
		}
		ptr += (cr->code & 0x7f7f) << 4;
		ptr += cr->lr;
		return(ptr[cr->line & 0x0f]);
	}
	else if (!(cr->line & 0x10)) {		// 半角
		if(hf_codeul && hf_count == 0) {
			hook_fontrom(0x80000 + (cr->code << 4));
			hf_codeul = 0;
			hf_count++;
		}
		ptr += 0x80000;
		ptr += cr->code << 4;
		return(ptr[cr->line]);
	}
	(void)port;
	return(0);
}


// ---- I/F

static const IOOUT cgromoa1[8] = {
					cgrom_oa1,	cgrom_oa3,	cgrom_oa5,	NULL,
					cgrom_oa9,	NULL,		NULL,		NULL};

static const IOINP cgromia1[8] = {
					NULL,		NULL,		NULL,		NULL,
					cgrom_ia9,	NULL,		NULL,		NULL};

void cgrom_reset(const NP2CFG *pConfig) {

	CGWINDOW	cgw;

	cgw = &cgwindow;
	ZeroMemory(cgw, sizeof(cgrom));
	cgw->low = 0x7fff0;
	cgw->high = 0x7fff0;
	cgw->writable = 0;

	(void)pConfig;
}

void cgrom_bind(void) {

	iocore_attachsysoutex(0x00a1, 0x0cf1, cgromoa1, 8);
	iocore_attachsysinpex(0x00a1, 0x0cf1, cgromia1, 8);
}

