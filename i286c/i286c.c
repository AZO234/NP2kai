#include	"compiler.h"
#include	"cpucore.h"
#include	"i286c.h"
#include	"v30patch.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"dmax86.h"
#include	"i286c.mcr"
#if defined(ENABLE_TRAP)
#include "trap/steptrap.h"
#endif
#if defined(SUPPORT_ASYNC_CPU)
#include "timing.h"
#include "nevent.h"
#include	"sound/sound.h"
#include	"sound/beep.h"
#include	"sound/fmboard.h"
#include	"sound/soundrom.h"
#include	"cbus/mpu98ii.h"
#endif



	I286CORE	i286core;

const UINT8 iflags[512] = {					// Z_FLAG, S_FLAG, P_FLAG
			0x44, 0x00, 0x00, 0x04, 0x00, 0x04, 0x04, 0x00,
			0x00, 0x04, 0x04, 0x00, 0x04, 0x00, 0x00, 0x04,
			0x00, 0x04, 0x04, 0x00, 0x04, 0x00, 0x00, 0x04,
			0x04, 0x00, 0x00, 0x04, 0x00, 0x04, 0x04, 0x00,
			0x00, 0x04, 0x04, 0x00, 0x04, 0x00, 0x00, 0x04,
			0x04, 0x00, 0x00, 0x04, 0x00, 0x04, 0x04, 0x00,
			0x04, 0x00, 0x00, 0x04, 0x00, 0x04, 0x04, 0x00,
			0x00, 0x04, 0x04, 0x00, 0x04, 0x00, 0x00, 0x04,
			0x00, 0x04, 0x04, 0x00, 0x04, 0x00, 0x00, 0x04,
			0x04, 0x00, 0x00, 0x04, 0x00, 0x04, 0x04, 0x00,
			0x04, 0x00, 0x00, 0x04, 0x00, 0x04, 0x04, 0x00,
			0x00, 0x04, 0x04, 0x00, 0x04, 0x00, 0x00, 0x04,
			0x04, 0x00, 0x00, 0x04, 0x00, 0x04, 0x04, 0x00,
			0x00, 0x04, 0x04, 0x00, 0x04, 0x00, 0x00, 0x04,
			0x00, 0x04, 0x04, 0x00, 0x04, 0x00, 0x00, 0x04,
			0x04, 0x00, 0x00, 0x04, 0x00, 0x04, 0x04, 0x00,
			0x80, 0x84, 0x84, 0x80, 0x84, 0x80, 0x80, 0x84,
			0x84, 0x80, 0x80, 0x84, 0x80, 0x84, 0x84, 0x80,
			0x84, 0x80, 0x80, 0x84, 0x80, 0x84, 0x84, 0x80,
			0x80, 0x84, 0x84, 0x80, 0x84, 0x80, 0x80, 0x84,
			0x84, 0x80, 0x80, 0x84, 0x80, 0x84, 0x84, 0x80,
			0x80, 0x84, 0x84, 0x80, 0x84, 0x80, 0x80, 0x84,
			0x80, 0x84, 0x84, 0x80, 0x84, 0x80, 0x80, 0x84,
			0x84, 0x80, 0x80, 0x84, 0x80, 0x84, 0x84, 0x80,
			0x84, 0x80, 0x80, 0x84, 0x80, 0x84, 0x84, 0x80,
			0x80, 0x84, 0x84, 0x80, 0x84, 0x80, 0x80, 0x84,
			0x80, 0x84, 0x84, 0x80, 0x84, 0x80, 0x80, 0x84,
			0x84, 0x80, 0x80, 0x84, 0x80, 0x84, 0x84, 0x80,
			0x80, 0x84, 0x84, 0x80, 0x84, 0x80, 0x80, 0x84,
			0x84, 0x80, 0x80, 0x84, 0x80, 0x84, 0x84, 0x80,
			0x84, 0x80, 0x80, 0x84, 0x80, 0x84, 0x84, 0x80,
			0x80, 0x84, 0x84, 0x80, 0x84, 0x80, 0x80, 0x84,
			0x45, 0x01, 0x01, 0x05, 0x01, 0x05, 0x05, 0x01,
			0x01, 0x05, 0x05, 0x01, 0x05, 0x01, 0x01, 0x05,
			0x01, 0x05, 0x05, 0x01, 0x05, 0x01, 0x01, 0x05,
			0x05, 0x01, 0x01, 0x05, 0x01, 0x05, 0x05, 0x01,
			0x01, 0x05, 0x05, 0x01, 0x05, 0x01, 0x01, 0x05,
			0x05, 0x01, 0x01, 0x05, 0x01, 0x05, 0x05, 0x01,
			0x05, 0x01, 0x01, 0x05, 0x01, 0x05, 0x05, 0x01,
			0x01, 0x05, 0x05, 0x01, 0x05, 0x01, 0x01, 0x05,
			0x01, 0x05, 0x05, 0x01, 0x05, 0x01, 0x01, 0x05,
			0x05, 0x01, 0x01, 0x05, 0x01, 0x05, 0x05, 0x01,
			0x05, 0x01, 0x01, 0x05, 0x01, 0x05, 0x05, 0x01,
			0x01, 0x05, 0x05, 0x01, 0x05, 0x01, 0x01, 0x05,
			0x05, 0x01, 0x01, 0x05, 0x01, 0x05, 0x05, 0x01,
			0x01, 0x05, 0x05, 0x01, 0x05, 0x01, 0x01, 0x05,
			0x01, 0x05, 0x05, 0x01, 0x05, 0x01, 0x01, 0x05,
			0x05, 0x01, 0x01, 0x05, 0x01, 0x05, 0x05, 0x01,
			0x81, 0x85, 0x85, 0x81, 0x85, 0x81, 0x81, 0x85,
			0x85, 0x81, 0x81, 0x85, 0x81, 0x85, 0x85, 0x81,
			0x85, 0x81, 0x81, 0x85, 0x81, 0x85, 0x85, 0x81,
			0x81, 0x85, 0x85, 0x81, 0x85, 0x81, 0x81, 0x85,
			0x85, 0x81, 0x81, 0x85, 0x81, 0x85, 0x85, 0x81,
			0x81, 0x85, 0x85, 0x81, 0x85, 0x81, 0x81, 0x85,
			0x81, 0x85, 0x85, 0x81, 0x85, 0x81, 0x81, 0x85,
			0x85, 0x81, 0x81, 0x85, 0x81, 0x85, 0x85, 0x81,
			0x85, 0x81, 0x81, 0x85, 0x81, 0x85, 0x85, 0x81,
			0x81, 0x85, 0x85, 0x81, 0x85, 0x81, 0x81, 0x85,
			0x81, 0x85, 0x85, 0x81, 0x85, 0x81, 0x81, 0x85,
			0x85, 0x81, 0x81, 0x85, 0x81, 0x85, 0x85, 0x81,
			0x81, 0x85, 0x85, 0x81, 0x85, 0x81, 0x81, 0x85,
			0x85, 0x81, 0x81, 0x85, 0x81, 0x85, 0x85, 0x81,
			0x85, 0x81, 0x81, 0x85, 0x81, 0x85, 0x85, 0x81,
			0x81, 0x85, 0x85, 0x81, 0x85, 0x81, 0x81, 0x85};


// ----

#if !defined(MEMOPTIMIZE)
	UINT8	_szpflag16[0x10000];
#endif

#if !defined(MEMOPTIMIZE) || (MEMOPTIMIZE < 2)
	UINT8	*_reg8_b53[256];
	UINT8	*_reg8_b20[256];
#endif
#if !defined(MEMOPTIMIZE) || (MEMOPTIMIZE < 2)
	UINT16	*_reg16_b53[256];
	UINT16	*_reg16_b20[256];
#endif

void i286c_initialize(void) {

#if !defined(MEMOPTIMIZE) || (MEMOPTIMIZE < 2)
	UINT	i;
#endif

#if !defined(MEMOPTIMIZE) || (MEMOPTIMIZE < 2)
	for (i=0; i<0x100; i++) {
		int pos;
#if defined(BYTESEX_LITTLE)
		pos = ((i & 0x20)?1:0);
#else
		pos = ((i & 0x20)?0:1);
#endif
		pos += ((i >> 3) & 3) * 2;
		_reg8_b53[i] = ((UINT8 *)&I286_REG) + pos;
#if defined(BYTESEX_LITTLE)
		pos = ((i & 0x4)?1:0);
#else
		pos = ((i & 0x4)?0:1);
#endif
		pos += (i & 3) * 2;
		_reg8_b20[i] = ((UINT8 *)&I286_REG) + pos;
#if !defined(MEMOPTIMIZE) || (MEMOPTIMIZE < 2)
		_reg16_b53[i] = ((UINT16 *)&I286_REG) + ((i >> 3) & 7);
		_reg16_b20[i] = ((UINT16 *)&I286_REG) + (i & 7);
#endif
	}
#endif

#if !defined(MEMOPTIMIZE)
	for (i=0; i<0x10000; i++) {
		REG8 f;
		UINT bit;
		f = P_FLAG;
		for (bit=0x80; bit; bit>>=1) {
			if (i & bit) {
				f ^= P_FLAG;
			}
		}
		if (!i) {
			f |= Z_FLAG;
		}
		if (i & 0x8000) {
			f |= S_FLAG;
		}
		_szpflag16[i] = f;
	}
#endif
#if !defined(MEMOPTIMIZE) || (MEMOPTIMIZE < 2)
	i286cea_initialize();
#endif
	if (CPU_TYPE == CPUTYPE_V30) {
		v30cinit();
	}
	ZeroMemory(&i286core, sizeof(i286core));
}

void i286c_deinitialize(void) {

	if (CPU_EXTMEM) {
		_MFREE(CPU_EXTMEM);
		CPU_EXTMEM = NULL;
		CPU_EXTMEMSIZE = 0;
	}
}

static void i286c_initreg(void) {

	I286_CS = 0xf000;
	CS_BASE = 0xf0000;
	I286_IP = 0xfff0;
	I286_ADRSMASK = 0xfffff;
}

#if defined(VAEG_FIX)
void i286c_reset(void) {
	ZeroMemory(&i286core.s, sizeof(i286core.s));
	if (CPU_TYPE == CPUTYPE_V30) {
		v30c_initreg();
	}
	else {
		i286c_initreg();
	}
}
#else
void i286c_reset(void) {
	ZeroMemory(&i286core.s, sizeof(i286core.s));
	i286c_initreg();
}
#endif

void i286c_shut(void) {

	ZeroMemory(&i286core.s, offsetof(I286STAT, cpu_type));
	i286c_initreg();
}

void i286c_setextsize(UINT32 size) {

	if (CPU_EXTMEMSIZE != size) {
		UINT8 *extmem;
		extmem = CPU_EXTMEM;
		if (extmem != NULL) {
			_MFREE(extmem);
			extmem = NULL;
		}
		if (size != 0) {
			extmem = (UINT8 *)_MALLOC(size + 16, "EXTMEM");
		}
		if (extmem != NULL) {
			CPU_EXTMEM = extmem;
			CPU_EXTMEMSIZE = size;
			CPU_EXTMEMBASE = CPU_EXTMEM - 0x100000;
			CPU_EXTLIMIT16 = MIN(size + 0x100000, 0xf00000);
#if defined(CPU_EXTLIMIT)
			CPU_EXTLIMIT = size + 0x100000;
#endif
		}
		else {
			CPU_EXTMEM = NULL;
			CPU_EXTMEMSIZE = 0;
			CPU_EXTMEMBASE = NULL;
			CPU_EXTLIMIT16 = 0;
#if defined(CPU_EXTLIMIT)
			CPU_EXTLIMIT = 0;
#endif
		}
	}
	i286core.e.ems[0] = mem + 0xc0000;
	i286core.e.ems[1] = mem + 0xc4000;
	i286core.e.ems[2] = mem + 0xc8000;
	i286core.e.ems[3] = mem + 0xcc000;
}

void i286c_setemm(UINT frame, UINT32 addr) {

	UINT8	*ptr;

	frame &= 3;
	if (addr < USE_HIMEM) {
		ptr = mem + addr;
	}
	else if ((addr - 0x100000 + 0x4000) <= CPU_EXTMEMSIZE) {
		ptr = CPU_EXTMEM + (addr - 0x100000);
	}
	else {
		ptr = mem + 0xc0000 + (frame << 14);
	}
	i286core.e.ems[frame] = ptr;
}


void CPUCALL i286c_intnum(UINT vect, REG16 IP) {

const UINT8	*ptr;

	if (vect < 0x10) TRACEOUT(("i286c_intnum - %.2x", vect));
	REGPUSH0(REAL_FLAGREG)
	REGPUSH0(I286_CS)
	REGPUSH0(IP)

	I286_FLAG &= ~(T_FLAG | I_FLAG);
	I286_TRAP = 0;

	ptr = mem + (vect * 4);
	I286_IP = LOADINTELWORD(ptr+0);				// real mode!
	I286_CS = LOADINTELWORD(ptr+2);				// real mode!
	CS_BASE = I286_CS << 4;
	I286_WORKCLOCK(20);
}

void CPUCALL i286c_interrupt(REG8 vect) {

	UINT	op;
const UINT8	*ptr;

	op = i286_memoryread(I286_IP + CS_BASE);
	if (op == 0xf4) {							// hlt
		I286_IP++;
	}
	REGPUSH0(REAL_FLAGREG)						// ここV30で辻褄が合わない
	REGPUSH0(I286_CS)
	REGPUSH0(I286_IP)

	I286_FLAG &= ~(T_FLAG | I_FLAG);
	I286_TRAP = 0;

	ptr = mem + (vect * 4);
	I286_IP = LOADINTELWORD(ptr + 0);			// real mode!
	I286_CS = LOADINTELWORD(ptr + 2);			// real mode!
	CS_BASE = I286_CS << 4;
	I286_WORKCLOCK(20);
}

void i286c(void) {

	UINT	opcode;

	if (I286_TRAP) {
		do {
#if defined(ENABLE_TRAP)
			steptrap(CPU_CS, CPU_IP);
#endif
			GET_PCBYTE(opcode);
			i286op[opcode]();
			if (I286_TRAP) {
				i286c_interrupt(1);
			}
			dmax86();
		} while(I286_REMCLOCK > 0);
	}
	else if (dmac.working) {
		do {
#if defined(ENABLE_TRAP)
			steptrap(CPU_CS, CPU_IP);
#endif
			GET_PCBYTE(opcode);
			i286op[opcode]();
			dmax86();
		} while(I286_REMCLOCK > 0);
	}
#if defined(SUPPORT_ASYNC_CPU)
	else if(np2cfg.asynccpu){
		int firstflag = 1;
		UINT timing;
		UINT lcflag = 0;
		SINT32 oldremclock = CPU_REMCLOCK;
		static int remclock_mul = 1000;
		int remclockb = 0;
		int remclkcnt = 0x100;
		int repflag = 0;
		static int latecount = 0;
		static int latecount2 = 0;
		static int hltflag = 0;
#define LATECOUNTER_THRESHOLD	6
#define LATECOUNTER_THRESHOLDM	6
		int realclock = 0;

		if(latecount2==0){
			if(latecount > 0){
				//latecount--;
			}else if (latecount < 0){
				latecount++;
			}
		}
		latecount2 = (latecount2+1) & 0x1fff;

		do {
#if defined(ENABLE_TRAP)
			steptrap(CPU_CS, CPU_IP);
#endif
			GET_PCBYTE(opcode);
			i286op[opcode]();
			
			// 非同期CPU処理
			realclock = 0;
			if(CPU_REMCLOCK >= 0 && !realclock && (remclkcnt > 0x7)){
				remclkcnt = 0;
				firstflag = 0;
				timing = timing_getcount_baseclock();
				if(timing!=0){
					if(!asynccpu_fastflag && !asynccpu_lateflag){
						if(remclock_mul < 100000) {
							latecount++;
							if(latecount > +LATECOUNTER_THRESHOLD){
								if(pccore.multiple > 2){
									if(pccore.multiple > 40){
										pccore.multiple-=3;
									}else if(pccore.multiple > 20){
										pccore.multiple-=2;
									}else{
										pccore.multiple-=1;
									}
									pccore.realclock = pccore.baseclock * pccore.multiple;
		
									sound_changeclock();
									beep_changeclock();
									mpu98ii_changeclock();
									keyboard_changeclock();
									mouseif_changeclock();
									gdc_updateclock();
								}

								latecount = 0;
							}
						}
						asynccpu_lateflag = 1;
					}
					CPU_REMCLOCK = 0;
					break;
				}else{
					if(!hltflag && !asynccpu_lateflag && g_nevent.item[NEVENT_FLAMES].proc==screendisp && g_nevent.item[NEVENT_FLAMES].clock <= CPU_BASECLOCK){
						//CPU_REMCLOCK = 10000;
						//oldremclock = CPU_REMCLOCK;
						if(!asynccpu_fastflag){
							latecount--;
							if(latecount < -LATECOUNTER_THRESHOLDM){
								if(pccore.multiple < np2cfg.multiple){
									pccore.multiple+=1;
									pccore.realclock = pccore.baseclock * pccore.multiple;
		
									sound_changeclock();
									beep_changeclock();
									mpu98ii_changeclock();
									keyboard_changeclock();
									mouseif_changeclock();
									gdc_updateclock();

									latecount = 0;
								}
							}
							asynccpu_fastflag = 1;
						}
					}
					firstflag = 1;
				}
			}
			remclkcnt++;
		} while(I286_REMCLOCK > 0);
	}
#endif
	else {
		do {
#if defined(ENABLE_TRAP)
			steptrap(CPU_CS, CPU_IP);
#endif
			GET_PCBYTE(opcode);
			i286op[opcode]();
		} while(I286_REMCLOCK > 0);
	}
}

void i286c_step(void) {

	UINT	opcode;

	I286_OV = I286_FLAG & O_FLAG;
	I286_FLAG &= ~(O_FLAG);

	GET_PCBYTE(opcode);
	i286op[opcode]();

	I286_FLAG &= ~(O_FLAG);
	if (I286_OV) {
		I286_FLAG |= (O_FLAG);
	}
	dmax86();
}


// ---- test

#if defined(I286C_TEST)
UINT8 BYTESZPF(UINT r) {

	if (r & (~0xff)) {
		TRACEOUT(("BYTESZPF bound error: %x", r));
	}
	return(iflags[r & 0xff]);
}

UINT8 BYTESZPCF(UINT r) {

	if (r & (~0x1ff)) {
		TRACEOUT(("BYTESZPCF bound error: %x", r));
	}
	return(iflags[r & 0x1ff]);
}

UINT8 WORDSZPF(UINT32 r) {

	UINT8	f1;
	UINT8	f2;

	if (r & (~0xffff)) {
		TRACEOUT(("WORDSZPF bound error: %x", r));
	}
	f1 = _szpflag16[r & 0xffff];
	f2 = iflags[r & 0xff] & P_FLAG;
	f2 += (r)?0:Z_FLAG;
	f2 += (r >> 8) & S_FLAG;
	if (f1 != f2) {
		TRACEOUT(("word flag error: %.2x %.2x", f1, f2));
	}
	return(f1);
}

UINT8 WORDSZPCF(UINT32 r) {

	UINT8	f1;
	UINT8	f2;

	if ((r & 0xffff0000) && (!(r & 0x00010000))) {
		TRACEOUT(("WORDSZPCF bound error: %x", r));
	}
	f1 = (r >> 16) & 1;
	f1 += _szpflag16[LOW16(r)];

	f2 = iflags[r & 0xff] & P_FLAG;
	f2 += (LOW16(r))?0:Z_FLAG;
	f2 += (r >> 8) & S_FLAG;
	f2 += (r >> 16) & 1;

	if (f1 != f2) {
		TRACEOUT(("word flag error: %.2x %.2x", f1, f2));
	}
	return(f1);
}
#endif

