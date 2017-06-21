#include	"compiler.h"
#include	"strres.h"
#include	"dosio.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"debugsub.h"

void debugwriteseg(const OEMCHAR *fname, const descriptor_t *sd,
												UINT32 addr, UINT32 size);
void debugpageptr(UINT32 addr);


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

static const OEMCHAR file_i386reg[] = OEMTEXT("i386reg.%.3u");
static const OEMCHAR file_i386cs[] = OEMTEXT("i386_cs.%.3u");
static const OEMCHAR file_i386ds[] = OEMTEXT("i386_ds.%.3u");
static const OEMCHAR file_i386es[] = OEMTEXT("i386_es.%.3u");
static const OEMCHAR file_i386ss[] = OEMTEXT("i386_ss.%.3u");
static const OEMCHAR file_memorybin[] = OEMTEXT("memory.bin");

static const OEMCHAR str_register[] =									\
					OEMTEXT("EAX=%.8x  EBX=%.8x  ECX=%.8x  EDX=%.8x")	\
					OEMTEXT(CRLITERAL)									\
					OEMTEXT("ESP=%.8x  EBP=%.8x  ESI=%.8x  EDI=%.8x")	\
					OEMTEXT(CRLITERAL)									\
					OEMTEXT("DS=%.4x  ES=%.4x  SS=%.4x  CS=%.4x  ")		\
					OEMTEXT("EIP=%.8x  ")								\
					OEMTEXT(CRLITERAL);
static const OEMCHAR str_picstat[] = 									\
					OEMTEXT("PIC0=%.2x:%.2x:%.2x")						\
					OEMTEXT(CRLITERAL)									\
					OEMTEXT("PIC1=%.2x:%.2x:%.2x")						\
					OEMTEXT(CRLITERAL)									\
					OEMTEXT("8255PORTC = %.2x / system-port = %.2x")	\
					OEMTEXT(CRLITERAL);


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

	OEMSPRINTF(work, str_register,	CPU_EAX, CPU_EBX, CPU_ECX, CPU_EDX,
									CPU_ESP, CPU_EBP, CPU_ESI, CPU_EDI,
									CPU_DS, CPU_ES, CPU_SS, CPU_CS, CPU_EIP);
	milstr_ncat(work, debugsub_flags(CPU_FLAG), NELEMENTS(work));
	milstr_ncat(work, CRCONST, NELEMENTS(work));
	return(work);
}

void debugwriteseg(const OEMCHAR *fname, const descriptor_t *sd,
												UINT32 addr, UINT32 size) {

	FILEH	fh;
	UINT8	buf[0x1000];
	UINT32	limit;

	limit = sd->u.seg.limit;
	if (limit <= addr) {
		return;
	}
	size = min(limit - addr, size - 1) + 1;
	fh = file_create_c(fname);
	if (fh == FILEH_INVALID) {
		return;
	}
	addr += sd->u.seg.segbase;
	while(size) {
		limit = min(size, sizeof(buf));
		MEML_READS(addr, buf, limit);
		file_write(fh, buf, limit);
		addr += limit;
		size -= limit;
	}
	file_close(fh);
}

void debugsub_status(void) {

static int		filenum = 0;
	FILEH		fh;
	OEMCHAR		work[512];
const OEMCHAR	*p;

	OEMSPRINTF(work, file_i386reg, filenum);
	fh = file_create_c(work);
	if (fh != FILEH_INVALID) {
		p = debugsub_regs();
		file_write(fh, p, OEMSTRLEN(p) * sizeof(OEMCHAR));
		OEMSPRINTF(work, str_picstat,
								pic.pi[0].imr, pic.pi[0].irr, pic.pi[0].isr,
								pic.pi[1].imr, pic.pi[1].irr, pic.pi[1].isr,
								mouseif.upd8255.portc, sysport.c);
		file_write(fh, work, OEMSTRLEN(work) * sizeof(OEMCHAR));

		OEMSPRINTF(work, OEMTEXT("CS = %.8x:%.8x") OEMTEXT(CRLITERAL),
						CPU_STAT_SREGBASE(CPU_CS_INDEX),
						CPU_STAT_SREGLIMIT(CPU_CS_INDEX));
		file_write(fh, work, OEMSTRLEN(work) * sizeof(OEMCHAR));

		file_close(fh);
	}

	OEMSPRINTF(work, file_i386cs, filenum);
	debugwriteseg(work, &CPU_STAT_SREG(CPU_CS_INDEX), CPU_EIP & 0xffff0000, 0x10000);
	OEMSPRINTF(work, file_i386ds, filenum);
	debugwriteseg(work, &CPU_STAT_SREG(CPU_DS_INDEX), 0, 0x10000);
	OEMSPRINTF(work, file_i386es, filenum);
	debugwriteseg(work, &CPU_STAT_SREG(CPU_ES_INDEX), 0, 0x10000);
	OEMSPRINTF(work, file_i386ss, filenum);
	debugwriteseg(work, &CPU_STAT_SREG(CPU_SS_INDEX), CPU_ESP & 0xffff0000, 0x10000);
	filenum++;
}

void debugsub_memorydump(void) {

	FILEH	fh;
	int		i;

	fh = file_create_c(file_memorybin);
	if (fh != FILEH_INVALID) {
		for (i=0; i<34; i++) {
			file_write(fh, mem + i*0x8000, 0x8000);
		}
		file_close(fh);
	}
}

void debugsub_memorydumpall(void) {

	FILEH	fh;

	fh = file_create_c(file_memorybin);
	if (fh != FILEH_INVALID) {
		file_write(fh, mem, 0x110000);
		if (CPU_EXTMEMSIZE > 0x10000) {
			file_write(fh, CPU_EXTMEM + 0x10000, CPU_EXTMEMSIZE - 0x10000);
		}
		file_close(fh);
	}
}


#if 0	// 俺用デバグ

void debugpageptr(UINT32 addr) {

	FILEH	fh;
	char	buf[256];
	UINT32	pde;
	UINT32	pte;
	UINT	i;
	UINT32	a;

	fh = file_create("page.txt");
	SPRINTF(buf, "CR3=%.8x\r\n", CPU_CR3);
	file_write(fh, buf, strlen(buf));
	for (i=0; i<1024; i++) {
		a = CPU_STAT_PDE_BASE + (i * 4);
		pde = cpu_memoryread_d(a);
		SPRINTF(buf, "%.8x=>%.8x [%.8x]\r\n", (i << 22), pde, a);
		file_write(fh, buf, strlen(buf));
	}
	addr >>= 22;
	pde = cpu_memoryread_d(CPU_STAT_PDE_BASE + (addr * 4));
	for (i=0; i<1024; i++) {
		a = (pde & CPU_PDE_BASEADDR_MASK) + (i * 4);
		pte = cpu_memoryread_d(a);
		SPRINTF(buf, "%.8x=>%.8x [%.8x]\r\n", (addr << 22) + (i << 12), pte, a);
		file_write(fh, buf, strlen(buf));
	}
	file_close(fh);
}

#endif

