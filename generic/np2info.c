#include	"compiler.h"
#include	"strres.h"
#include	"scrnmng.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"sound.h"
#include	"fmboard.h"
#include	"np2info.h"
#include "sound/soundrom.h"
#if defined(SUPPORT_IDEIO)
#include	"ideio.h"
#endif
#if defined(SUPPORT_GPIB)
#include	"gpibio.h"
#endif

static const OEMCHAR str_comma[] = OEMTEXT(", ");
static const OEMCHAR str_2halfMHz[] = OEMTEXT("2.5MHz");
#define str_5MHz	(str_2halfMHz + 2)
static const OEMCHAR str_8MHz[] = OEMTEXT("8MHz");
static const OEMCHAR str_notexist[] = OEMTEXT("not exist");
static const OEMCHAR str_disable[] = OEMTEXT("disable");

static const OEMCHAR str_cpu[] = OEMTEXT("8086-2\00070116\00080286\00080386\00080486\0Pentium\0PentiumPro");
static const OEMCHAR str_winclr[] = OEMTEXT("256-colors\00065536-colors\0full color\0true color");
static const OEMCHAR str_winmode[] = OEMTEXT(" (window)\0 (fullscreen)");
static const OEMCHAR str_grcgchip[] = OEMTEXT("\0GRCG \0GRCG CG-Window \0EGC CG-Window ");
static const OEMCHAR str_vrammode[] = OEMTEXT("Digital\0Analog\000256colors");
static const OEMCHAR str_vrampage[] = OEMTEXT(" page-0\0 page-1\0 page-all");
static const OEMCHAR str_chpan[] = OEMTEXT("none\0Mono-R\0Mono-L\0Stereo");
static const OEMCHAR str_fpu[] = OEMTEXT(" none\0 Berkeley SoftFloat 80bit Extended Precision FPU\0 64bit Double Precision FPU\0 64bit Double Precision FPU + INT64 Load/Store");
static const OEMCHAR str_simd_mmx[] = OEMTEXT("MMX ");
static const OEMCHAR str_simd_sse[] = OEMTEXT("SSE ");
static const OEMCHAR str_simd_sse2[] = OEMTEXT("SSE2 ");
static const OEMCHAR str_simd_sse3[] = OEMTEXT("SSE3 ");
static const OEMCHAR str_simd_ssse3[] = OEMTEXT("SSSE3 ");
static const OEMCHAR str_simd_sse4_1[] = OEMTEXT("SSE4.1 ");
static const OEMCHAR str_simd_sse4_2[] = OEMTEXT("SSE4.2 ");
static const OEMCHAR str_simd_3dnow[] = OEMTEXT("3DNow! ");
static const OEMCHAR str_simd_e3dnow[] = OEMTEXT("Enhanced 3DNow! ");

static const OEMCHAR str_clockfmt[] = OEMTEXT("%d.%1dMHz");
static const OEMCHAR str_memfmt[] = OEMTEXT("%3uKB");
static const OEMCHAR str_memfmt2[] = OEMTEXT("%3uKB + %uKB");
static const OEMCHAR str_memfmt3[] = OEMTEXT("%d.%1dMB");
static const OEMCHAR str_twidth[] = OEMTEXT("width-%u");
static const OEMCHAR str_dispclock[] = OEMTEXT("%u.%.2ukHz / %u.%uHz");

static const OEMCHAR str_pcm86a[] = OEMTEXT("   PCM: %dHz %dbit %s");
static const OEMCHAR str_pcm86b[] = OEMTEXT("        %d / %d / 32768");
static const OEMCHAR str_rhythm[] = OEMTEXT("BSCHTR");


// ---- common

static void info_ver(OEMCHAR *str, int maxlen, const NP2INFOEX *ex) {

	milstr_ncpy(str, np2version, maxlen);
	(void)ex;
}

static void info_cpu(OEMCHAR *str, int maxlen, const NP2INFOEX *ex) {

#if defined(CPUCORE_IA32)
#ifdef UNICODE
	MultiByteToWideChar(CP_ACP, 0, i386cpuid.cpu_brandstring, -1, str, maxlen);
#else
	milstr_ncpy(str, i386cpuid.cpu_brandstring, maxlen);
#endif
#else
	UINT	family;
#if defined(CPU_FAMILY)
	family = MIN(CPU_FAMILY, 6);
#else
	family = (CPU_TYPE & CPUTYPE_V30)?1:2;
#endif
	milstr_ncpy(str, milstr_list(str_cpu, family), maxlen);
	
#endif
	
	(void)ex;
}

static void info_clock(OEMCHAR *str, int maxlen, const NP2INFOEX *ex) {

	UINT32	clk;
	OEMCHAR	clockstr[64];

	clk = (pccore.realclock + 50000) / 100000;
	OEMSNPRINTF(clockstr, sizeof(clockstr), str_clockfmt, clk/10, clk % 10);
	milstr_ncpy(str, clockstr, maxlen);
	(void)ex;
}

static void info_base(OEMCHAR *str, int maxlen, const NP2INFOEX *ex) {

	milstr_ncpy(str,
				(pccore.cpumode & CPUMODE_8MHZ)?str_8MHz:str_5MHz, maxlen);
	(void)ex;
}

static void info_mem1(OEMCHAR *str, int maxlen, const NP2INFOEX *ex) {

	UINT	memsize;
	OEMCHAR	memstr[64];

	memsize = np2cfg.memsw[2] & 7;
	if (memsize < 6) {
		memsize = (memsize + 1) * 128;
	}
	else {
		memsize = 640;
	}
	if (pccore.extmem) {
		OEMSNPRINTF(memstr, sizeof(memstr), str_memfmt2, memsize, pccore.extmem * 1024);
	}
	else {
		OEMSNPRINTF(memstr, sizeof(memstr), str_memfmt, memsize);
	}
	milstr_ncpy(str, memstr, maxlen);
	(void)ex;
}

static void info_mem2(OEMCHAR *str, int maxlen, const NP2INFOEX *ex) {

	UINT	memsize;
	OEMCHAR	memstr[64];

	memsize = np2cfg.memsw[2] & 7;
	if (memsize < 6) {
		memsize = (memsize + 1) * 128;
	}
	else {
		memsize = 640;
	}
	memsize += pccore.extmem * 1024;
	OEMSNPRINTF(memstr, sizeof(memstr), str_memfmt, memsize);
	milstr_ncpy(str, memstr, maxlen);
	(void)ex;
}

static void info_mem3(OEMCHAR *str, int maxlen, const NP2INFOEX *ex) {

	UINT	memsize;
	OEMCHAR	memstr[64];

	memsize = np2cfg.memsw[2] & 7;
	if (memsize < 6) {
		memsize = (memsize + 1) * 128;
	}
	else {
		memsize = 640;
	}
	if (pccore.extmem > 1) {
		OEMSNPRINTF(memstr, sizeof(memstr), str_memfmt3, pccore.extmem, memsize / 100);
	}
	else {
		OEMSNPRINTF(memstr, sizeof(memstr), str_memfmt, memsize);
	}
	milstr_ncpy(str, memstr, maxlen);
	(void)ex;
}

static void info_gdc(OEMCHAR *str, int maxlen, const NP2INFOEX *ex) {

	milstr_ncpy(str, milstr_list(str_grcgchip, grcg.chip & 3), maxlen);
	milstr_ncat(str, str_2halfMHz + ((gdc.clock & 0x80)?2:0), maxlen);
	(void)ex;
}

static void info_gdc2(OEMCHAR *str, int maxlen, const NP2INFOEX *ex) {

	OEMCHAR	textstr[64];

	OEMSNPRINTF(textstr, sizeof(textstr), str_dispclock,
						gdc.hclock / 1000, (gdc.hclock / 10) % 100,
						gdc.vclock / 10, gdc.vclock % 10);
	milstr_ncpy(str, textstr, maxlen);
	(void)ex;
}

static void info_text(OEMCHAR *str, int maxlen, const NP2INFOEX *ex) {

const OEMCHAR	*p;
	OEMCHAR		textstr[128];

	if (!(gdcs.textdisp & GDCSCRN_ENABLE)) {
		p = str_disable;
	}
	else {
		OEMSNPRINTF(textstr, sizeof(textstr), str_twidth, ((gdc.mode1 & 0x4)?40:80));
		p = textstr;
	}
	milstr_ncpy(str, p, maxlen);
	(void)ex;
}

static void info_grph(OEMCHAR *str, int maxlen, const NP2INFOEX *ex) {

const OEMCHAR	*p;
	UINT		md;
	UINT		pg;
	OEMCHAR		work[32];

	if (!(gdcs.grphdisp & GDCSCRN_ENABLE)) {
		p = str_disable;
	}
	else {
		md = (gdc.analog & (1 << GDCANALOG_16))?1:0;
		pg = gdcs.access;
#if defined(SUPPORT_PC9821)
		if (gdc.analog & (1 << (GDCANALOG_256))) {
			md = 2;
			if (gdc.analog & (1 << (GDCANALOG_256E))) {
				pg = 2;
			}
		}
#endif
		milstr_ncpy(work, milstr_list(str_vrammode, md), NELEMENTS(work));
		milstr_ncat(work, milstr_list(str_vrampage, pg), NELEMENTS(work));
		p = work;
	}
	milstr_ncpy(str, p, maxlen);
	(void)ex;
}

static void info_sound(OEMCHAR *str, int maxlen, const NP2INFOEX *ex)
{
	const OEMCHAR *lpBoard;

	lpBoard = OEMTEXT("none");
	switch (g_nSoundID)
	{
		case SOUNDID_NONE:
			break;

		case SOUNDID_PC_9801_14:
			lpBoard = OEMTEXT("PC-9801-14");
			break;

		case SOUNDID_PC_9801_26K:
			lpBoard = OEMTEXT("PC-9801-26");
			break;

		case SOUNDID_PC_9801_86:
			lpBoard = OEMTEXT("PC-9801-86");
			break;

		case SOUNDID_PC_9801_86_26K:
			lpBoard = OEMTEXT("PC-9801-26 + 86");
			break;

		case SOUNDID_PC_9801_118:
			lpBoard = OEMTEXT("PC-9801-118");
			break;
			
		case SOUNDID_PC_9801_86_WSS:
			lpBoard = OEMTEXT("PC-9801-86 + Mate-X PCM");
			break;
			
		case SOUNDID_PC_9801_86_118:
			lpBoard = OEMTEXT("PC-9801-86 + PC-9801-118");
			break;
			
		case SOUNDID_MATE_X_PCM:
			lpBoard = OEMTEXT("Mate-X PCM");
			break;
			
		case SOUNDID_PC_9801_86_ADPCM:
			lpBoard = OEMTEXT("PC-9801-86 + Chibi-oto");
			break;

		case SOUNDID_WAVESTAR:
			lpBoard = OEMTEXT("Wave Star");
			break;

		case SOUNDID_SPEAKBOARD:
			lpBoard = OEMTEXT("Speak board");
			break;

		case SOUNDID_86_SPEAKBOARD:
			lpBoard = OEMTEXT("PC-9801-86 + Speak board");
			break;

		case SOUNDID_SPARKBOARD:
			lpBoard = OEMTEXT("Spark board");
			break;

		case SOUNDID_AMD98:
			lpBoard = OEMTEXT("AMD-98");
			break;

		case SOUNDID_SOUNDORCHESTRA:
			lpBoard = OEMTEXT("SOUND ORCHESTRA");
			break;

		case SOUNDID_SOUNDORCHESTRAV:
			lpBoard = OEMTEXT("SOUND ORCHESTRA-V");
			break;
			
		case SOUNDID_LITTLEORCHESTRAL:
			lpBoard = OEMTEXT("LITTLE ORCHESTRA L");
			break;

		case SOUNDID_MMORCHESTRA:
			lpBoard = OEMTEXT("MULTIMEDIA ORCHESTRA");
			break;
			
#if defined(SUPPORT_SOUND_SB16)
		case SOUNDID_SB16:
			lpBoard = OEMTEXT("Sound Blaster 16");
			break;
			
		case SOUNDID_PC_9801_86_SB16:
			lpBoard = OEMTEXT("PC-9801-86 + Sound Blaster 16");
			break;
			
		case SOUNDID_WSS_SB16:
			lpBoard = OEMTEXT("Mate-X PCM + Sound Blaster 16");
			break;
			
		case SOUNDID_PC_9801_86_WSS_SB16:
			lpBoard = OEMTEXT("PC-9801-86 + Mate-X PCM + Sound Blaster 16");
			break;
			
		case SOUNDID_PC_9801_118_SB16:
			lpBoard = OEMTEXT("PC-9801-118 + Sound Blaster 16");
			break;
			
		case SOUNDID_PC_9801_86_118_SB16:
			lpBoard = OEMTEXT("PC-9801-86 + PC-9801-118 + Sound Blaster 16");
			break;
#endif	// defined(SUPPORT_SOUND_SB16)

#if defined(SUPPORT_PX)
		case SOUNDID_PX1:
			lpBoard = OEMTEXT("Otomi-chanx2");
			break;

		case SOUNDID_PX2:
			lpBoard = OEMTEXT("Otomi-chanx2 + 86");
			break;
#endif	// defined(SUPPORT_PX)

		default:
			lpBoard = OEMTEXT("unknown");
			break;
	}
	milstr_ncpy(str, lpBoard, maxlen);
	(void)ex;
}

static void info_extsnd(OEMCHAR *str, int maxlen, const NP2INFOEX *ex) {

	OEMCHAR	buf[128];

	info_sound(str, maxlen, ex);
	if (g_nSoundID & 4) {
		milstr_ncat(str, ex->cr, maxlen);
		OEMSNPRINTF(buf, sizeof(buf), str_pcm86a,
							pcm86rate8[g_pcm86.fifo & 7] >> 3,
							(16 - ((g_pcm86.dactrl >> 3) & 8)),
							milstr_list(str_chpan, (g_pcm86.dactrl >> 4) & 3));
		milstr_ncat(str, buf, maxlen);
		milstr_ncat(str, ex->cr, maxlen);
		OEMSNPRINTF(buf, sizeof(buf), str_pcm86b, g_pcm86.virbuf, g_pcm86.fifosize);
		milstr_ncat(str, buf, maxlen);
	}
}

static void info_bios(OEMCHAR *str, int maxlen, const NP2INFOEX *ex) {

	str[0] = '\0';
	if (pccore.rom & PCROM_BIOS) {
		milstr_ncat(str, str_biosrom, maxlen);
	}
	if (soundrom.name[0]) {
		if (str[0]) {
			milstr_ncat(str, str_comma, maxlen);
		}
		milstr_ncat(str, soundrom.name, maxlen);
	}
#if defined(SUPPORT_IDEIO)
	if (ideio.bios) {
		if (str[0]) {
			milstr_ncat(str, str_comma, maxlen);
		}
		milstr_ncat(str, ideio.biosname, maxlen);
	}
#endif
#if defined(SUPPORT_PCI)
	if (pcidev.biosname[0]) {
		if (str[0]) {
			milstr_ncat(str, str_comma, maxlen);
		}
		milstr_ncat(str, pcidev.biosname, maxlen);
	}
#endif
#if defined(SUPPORT_GPIB)
	if (gpib.enable) {
		if (str[0]) {
			milstr_ncat(str, str_comma, maxlen);
		}
		milstr_ncat(str, OEMTEXT("gpib.rom"), maxlen);
	}
#endif
	if (str[0] == '\0') {
		milstr_ncat(str, str_notexist, maxlen);
	}
	(void)ex;
}

static void info_rhythm(OEMCHAR *str, int maxlen, const NP2INFOEX *ex) {

	OEMCHAR	rhythmstr[8];
	UINT	exist;
	UINT	i;

	exist = rhythm_getcaps();
	milstr_ncpy(rhythmstr, str_rhythm, NELEMENTS(rhythmstr));
	for (i=0; i<6; i++) {
		if (!(exist & (1 << i))) {
			rhythmstr[i] = '_';
		}
	}
	milstr_ncpy(str, rhythmstr, maxlen);
	(void)ex;
}

static void info_display(OEMCHAR *str, int maxlen, const NP2INFOEX *ex) {

	UINT	bpp;
	OEMCHAR buf[128] = {0};

	bpp = scrnmng_getbpp();
	milstr_ncpy(str, milstr_list(str_winclr, ((bpp >> 3) - 1) & 3), maxlen);
//	OEMSNPRINTF(buf, sizeof(buf), OEMTEXT(" %dx%d"), scrnmngp->width, scrnmngp->height);
//	milstr_ncat(str, buf, maxlen);
	milstr_ncat(str, milstr_list(str_winmode, (scrnmng_isfullscreen())?1:0),
																	maxlen);
	(void)ex;
}

static void info_fpu(OEMCHAR *str, int maxlen, const NP2INFOEX *ex) {

#if defined(CPUCORE_IA32)
	if(i386cpuid.cpu_feature & CPU_FEATURE_FPU){
		if(i386cpuid.fpu_type < 3){
			milstr_ncpy(str, milstr_list(str_fpu, 1+i386cpuid.fpu_type), maxlen);
		}else{
			milstr_ncpy(str, OEMTEXT(" unknown"), maxlen);
		}
	}else
#endif
	{
		milstr_ncpy(str, milstr_list(str_fpu, 0), maxlen);
	}
}

static void info_simd(OEMCHAR *str, int maxlen, const NP2INFOEX *ex) {

	int simdcount = 0;
	milstr_ncpy(str, OEMTEXT(" "), maxlen);
#if defined(CPUCORE_IA32)
	if(i386cpuid.cpu_feature & CPU_FEATURE_MMX){
		milstr_ncat(str, str_simd_mmx, maxlen);
		simdcount++;
	}
	if(i386cpuid.cpu_feature & CPU_FEATURE_SSE){
		milstr_ncat(str, str_simd_sse, maxlen);
		simdcount++;
	}
	if(i386cpuid.cpu_feature & CPU_FEATURE_SSE2){
		milstr_ncat(str, str_simd_sse2, maxlen);
		simdcount++;
	}
	if(i386cpuid.cpu_feature_ecx & CPU_FEATURE_ECX_SSE3){
		milstr_ncat(str, str_simd_sse3, maxlen);
		simdcount++;
	}
	if(i386cpuid.cpu_feature_ecx & CPU_FEATURE_ECX_SSSE3){
		milstr_ncat(str, str_simd_ssse3, maxlen);
		simdcount++;
	}
	if(i386cpuid.cpu_feature_ecx & CPU_FEATURE_ECX_SSE4_1){
		milstr_ncat(str, str_simd_sse4_1, maxlen);
		simdcount++;
	}
	if(i386cpuid.cpu_feature_ecx & CPU_FEATURE_ECX_SSE4_2){
		milstr_ncat(str, str_simd_sse4_2, maxlen);
		simdcount++;
	}
	if(i386cpuid.cpu_feature_ex & CPU_FEATURE_EX_3DNOW){
		milstr_ncat(str, str_simd_3dnow, maxlen);
		simdcount++;
	}
	if(i386cpuid.cpu_feature_ex & CPU_FEATURE_EX_E3DNOW){
		milstr_ncat(str, str_simd_e3dnow, maxlen);
		simdcount++;
	}
#endif
	if(simdcount==0){
		milstr_ncat(str, OEMTEXT("none"), maxlen);
	}
}


// ---- make string

typedef struct {
	OEMCHAR	key[8];
	void	(*proc)(OEMCHAR *str, int maxlen, const NP2INFOEX *ex);
} INFOPROC;

static const INFOPROC infoproc[] = {
			{OEMTEXT("VER"),	info_ver},
			{OEMTEXT("CPU"),	info_cpu},
			{OEMTEXT("CLOCK"),	info_clock},
			{OEMTEXT("BASE"),	info_base},
			{OEMTEXT("MEM1"),	info_mem1},
			{OEMTEXT("MEM2"),	info_mem2},
			{OEMTEXT("MEM3"),	info_mem3},
			{OEMTEXT("FPU"),	info_fpu},
			{OEMTEXT("SIMD"),	info_simd},
			{OEMTEXT("GDC"),	info_gdc},
			{OEMTEXT("GDC2"),	info_gdc2},
			{OEMTEXT("TEXT"),	info_text},
			{OEMTEXT("GRPH"),	info_grph},
			{OEMTEXT("SND"),	info_sound},
			{OEMTEXT("EXSND"),	info_extsnd},
			{OEMTEXT("BIOS"),	info_bios},
			{OEMTEXT("RHYTHM"),	info_rhythm},
			{OEMTEXT("DISP"),	info_display}};


static BOOL defext(OEMCHAR *dst, const OEMCHAR *key, int maxlen,
														const NP2INFOEX *ex) {

	milstr_ncpy(dst, key, maxlen);
	(void)ex;
	return(TRUE);
}

void np2info(OEMCHAR *dst, const OEMCHAR *src, int maxlen,
														const NP2INFOEX *ex) {

	NP2INFOEX	statex;
	OEMCHAR		c;
	UINT		leng;
	OEMCHAR		infwork[12];
const INFOPROC	*inf;
const INFOPROC	*infterm;

	if ((dst == NULL) || (maxlen <= 0) || (src == NULL)) {
		return;
	}
	if (ex == NULL) {
		milstr_ncpy(statex.cr, str_oscr, NELEMENTS(statex.cr));
		statex.ext = NULL;
	}
	else {
		statex = *ex;
	}
	if (statex.ext == NULL) {
		statex.ext = defext;
	}
	while(maxlen > 0) {
		c = *src++;
		if (c == '\0') {
			break;
		}
		else if (c == '\n') {
			milstr_ncpy(dst, statex.cr, maxlen);
		}
		else if (c != '%') {
			*dst++ = c;
			maxlen--;
			continue;
		}
		else if (*src == '%') {
			src++;
			*dst++ = c;
			maxlen--;
			continue;
		}
		else {
			leng = 0;
			while(1) {
				c = *src;
				if (c == '\0') {
					break;
				}
				src++;
				if (c == '%') {
					break;
				}
				if (leng < (NELEMENTS(infwork) - 1)) {
					infwork[leng++] = c;
				}
			}
			infwork[leng] = '\0';
			inf = infoproc;
			infterm = infoproc + NELEMENTS(infoproc);
			while(inf < infterm) {
				if (!milstr_cmp(infwork, inf->key)) {
					inf->proc(dst, maxlen, &statex);
					break;
				}
				inf++;
			}
			if (inf >= infterm) {
				if (!(*statex.ext)(dst, infwork, maxlen, &statex)) {
					continue;
				}
			}
		}
		leng = (UINT)OEMSTRLEN(dst);
		dst += leng;
		maxlen -= leng;
	}
	*dst = '\0';
}

