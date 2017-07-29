#include	"compiler.h"
#include	"strres.h"
#include	"textfile.h"
#include	"dosio.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"debugsub.h"


#if defined(MACOS)
#define	CRLITERAL	"\r"
#define	CRCONST		str_cr
#elif defined(X11)
#define	CRLITERAL	"\n"
#define	CRCONST		str_lf
#else
#define	CRLITERAL	"\r\n"
#define	CRCONST		str_crlf
#endif


static const OEMCHAR s_nv[] = OEMTEXT("NV");
static const OEMCHAR s_ov[] = OEMTEXT("OV");
static const OEMCHAR s_dn[] = OEMTEXT("DN");
static const OEMCHAR s_up[] = OEMTEXT("UP");
static const OEMCHAR s_di[] = OEMTEXT("DI");
static const OEMCHAR s_ei[] = OEMTEXT("EI");
static const OEMCHAR s_pl[] = OEMTEXT("PL");
static const OEMCHAR s_ng[] = OEMTEXT("NG");
static const OEMCHAR s_nz[] = OEMTEXT("NZ");
static const OEMCHAR s_zr[] = OEMTEXT("ZR");
static const OEMCHAR s_na[] = OEMTEXT("NA");
static const OEMCHAR s_ac[] = OEMTEXT("AC");
static const OEMCHAR s_po[] = OEMTEXT("PO");
static const OEMCHAR s_pe[] = OEMTEXT("PE");
static const OEMCHAR s_nc[] = OEMTEXT("NC");
static const OEMCHAR s_cy[] = OEMTEXT("CY");

static const OEMCHAR *flagstr[16][2] = {
				{NULL, NULL},		// 0x8000
				{NULL, NULL},		// 0x4000
				{NULL, NULL},		// 0x2000
				{NULL, NULL},		// 0x1000
				{s_nv, s_ov},		// 0x0800
				{s_dn, s_up},		// 0x0400
				{s_di, s_ei},		// 0x0200
				{NULL, NULL},		// 0x0100
				{s_pl, s_ng},		// 0x0080
				{s_nz, s_zr},		// 0x0040
				{NULL, NULL},		// 0x0020
				{s_na, s_ac},		// 0x0010
				{NULL, NULL},		// 0x0008
				{s_po, s_pe},		// 0x0004
				{NULL, NULL},		// 0x0002
				{s_nc, s_cy}};		// 0x0001

static const OEMCHAR file_i286reg[] = OEMTEXT("i286reg.%.3u");
static const OEMCHAR file_i286cs[] = OEMTEXT("i286_cs.%.3u");
static const OEMCHAR file_i286ds[] = OEMTEXT("i286_ds.%.3u");
static const OEMCHAR file_i286es[] = OEMTEXT("i286_es.%.3u");
static const OEMCHAR file_i286ss[] = OEMTEXT("i286_ss.%.3u");
static const OEMCHAR file_memorybin[] = OEMTEXT("memory.bin");

static const OEMCHAR str_register[] =								\
				OEMTEXT("AX=%.4x  BX=%.4x  CX=%.4x  DX=%.4x  ")		\
				OEMTEXT("SP=%.4x  BP=%.4x  SI=%.4x  DI=%.4x")		\
				OEMTEXT(CRLITERAL)									\
				OEMTEXT("DS=%.4x  ES=%.4x  SS=%.4x  CS=%.4x  ")		\
				OEMTEXT("IP=%.4x   ");
static const OEMCHAR str_picstat[] = 								\
				OEMTEXT(CRLITERAL)									\
				OEMTEXT("PIC0=%.2x:%.2x:%.2x")						\
				OEMTEXT(CRLITERAL)									\
				OEMTEXT("PIC1=%.2x:%.2x:%.2x")						\
				OEMTEXT(CRLITERAL)									\
				OEMTEXT("8255PORTC = %.2x / system-port = %.2x");


const OEMCHAR *debugsub_flags(UINT16 flag) {

static OEMCHAR	work[128];
	int			i;
	UINT16		bit;

	work[0] = 0;
	for (i=0, bit=0x8000; bit; i++, bit>>=1) {
		if (flagstr[i][0]) {
			if (flag & bit) {
				milstr_ncat(work, flagstr[i][1], NELEMENTS(work));
			}
			else {
				milstr_ncat(work, flagstr[i][0], NELEMENTS(work));
			}
			if (bit != 1) {
				milstr_ncat(work, str_space, NELEMENTS(work));
			}
		}
	}
	return(work);
}

const OEMCHAR *debugsub_regs(void) {

static OEMCHAR	work[256];

	OEMSPRINTF(work, str_register,	CPU_AX, CPU_BX, CPU_CX, CPU_DX,
									CPU_SP, CPU_BP, CPU_SI, CPU_DI,
									CPU_DS, CPU_ES, CPU_SS, CPU_CS, CPU_IP);
	milstr_ncat(work, debugsub_flags(CPU_FLAG), NELEMENTS(work));
	milstr_ncat(work, CRCONST, NELEMENTS(work));
	return(work);
}

static void writeseg(const OEMCHAR *fname, UINT32 addr, UINT limit) {

	FILEH	fh;
	UINT	size;
	UINT8	buf[0x400];								// Stack 0x1000 -> 0x400

	fh = file_create_c(fname);
	if (fh == FILEH_INVALID) {
		return;
	}
	limit = min(limit, 0xffff);
	limit++;
	while(limit) {
		size = min(limit, sizeof(buf));
		MEML_READS(addr, buf, size);
		file_write(fh, buf, size);
		addr += size;
		limit -= size;
	}
	file_close(fh);
}

void debugsub_status(void) {

static int		filenum = 0;
	TEXTFILEH	tfh;
	OEMCHAR		work[512];
const OEMCHAR	*p;

	OEMSPRINTF(work, file_i286reg, filenum);
	tfh = textfile_create(file_getcd(work), 0);
	if (tfh != NULL) {
		p = debugsub_regs();
		textfile_write(tfh, p);
		OEMSPRINTF(work, str_picstat,
								pic.pi[0].imr, pic.pi[0].irr, pic.pi[0].isr,
								pic.pi[1].imr, pic.pi[1].irr, pic.pi[1].isr,
								mouseif.upd8255.portc, sysport.c);
		textfile_write(tfh, work);
		textfile_close(tfh);
	}

	OEMSPRINTF(work, file_i286cs, filenum);
	writeseg(work, CS_BASE, 0xffff);
	OEMSPRINTF(work, file_i286ds, filenum);
	writeseg(work, DS_BASE, 0xffff);
	OEMSPRINTF(work, file_i286es, filenum);
	writeseg(work, ES_BASE, 0xffff);
	OEMSPRINTF(work, file_i286ss, filenum);
	writeseg(work, SS_BASE, 0xffff);
	filenum++;
}

void debugsub_memorydump(void) {

	FILEH	fh;
	int		i;

	fh = file_create_c(file_memorybin);
	if (fh != FILEH_INVALID) {
		for (i=0; i<34; i++)
//		for (i=0; i<64; i++)
		{
			file_write(fh, mem + i*0x8000, 0x8000);
		}
		file_close(fh);
	}
}

