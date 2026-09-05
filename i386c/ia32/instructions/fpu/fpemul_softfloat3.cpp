/*
 * Copyright (c) 2012 NONAKA Kimihiro
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * modified by SimK
 */

#include "compiler.h"

#if defined(USE_FPU) && defined(SUPPORT_FPU_SOFTFLOAT3)

#include <float.h>
#include <math.h>
#include "ia32/cpu.h"
#include "ia32/ia32.mcr"

#include "ia32/instructions/fpu/fp.h"
#include "ia32/instructions/fpu/fpumem.h"
#ifdef USE_SSE
#include "ia32/instructions/sse/sse.h"
#endif

// softfloat.hのインクルードはcpu.hにあります

 /*
 Short Real
	31: sign (符号)
 30-23: exp-8 (指数部: exponet)
 22-00: num-23 (小数部)

 Long Real
	63: sign
 62-52: exp-11
 51-00: num-52

 Temp Real
	79: sign
 78-64: exp-15
	63: 1(?)
 62-00: num-63

 --
 指数部:
 2 の 0 乗のとき 0111 1111 となる
 1000 0001: +2 乗
 1000 0000: +1 乗
 0111 1111:  0 乗
 0111 1110: -1 乗

 仮数部:
 2 を基数として整数部が一桁になるように正規化した数の 2 進数表現となる。
 正規化によって仮数部の最上位ビットは常に 1 になるので実際に用意しておく
 必要はなく、倍精度の 52 ビットであれば最上位の 1 を hidden bit にして
 含めなければ、53 ビット分の情報が含まれることになる。

 小数の二進数表現:
 0.1000    1/2         = 0.5
 0.0100    1/(2*2)     = 0.25
 0.0010    1/(2*2*2)   = 0.125
 0.0001    1/(2*2*2*2) = 0.0625
 */

#if 1
#undef	TRACEOUT
#define	TRACEOUT(s)	(void)(s)
#endif	/* 0 */

#define FPU_WORKCLOCK	6

#define PI		3.1415926535897932384626433832795
#define L2E		1.4426950408889634073599246810019
#define L2T		3.3219280948873623478703194294894
#define LN2		0.6931471805599453094172321214582
#define LG2		0.3010299956639811952137388947245

static INLINE void FPU_SetCW(UINT16 cword)
{
	FPU_CTRLWORD = cword & 0x7FFF;
	FPU_STAT.round = (FP_RND)((cword >> 10) & 3);
	switch (FPU_STAT.round) {
	case ROUND_Nearest:
		softfloat_roundingMode = softfloat_round_near_even;
		break;
	case ROUND_Down:
		softfloat_roundingMode = softfloat_round_min;
		break;
	case ROUND_Up:
		softfloat_roundingMode = softfloat_round_max;
		break;
	case ROUND_Chop:
		softfloat_roundingMode = softfloat_round_minMag;
		break;
	default:
		break;
	}
}

/* SoftFloat reports exceptions raised by the current host-side operation.
 * x87 exception flags are sticky, so each operation starts with an empty
 * SoftFloat flag set and merges only newly raised exceptions into the x87
 * status word when it completes. */
static INLINE void FPU_SoftFloatBegin(void)
{
	softfloat_exceptionFlags = 0;
}

static INLINE void FPU_SoftFloatEnd(void)
{
	const UINT8 flags = softfloat_exceptionFlags;
	const UINT16 result = (UINT16)(
		((flags & softfloat_flag_invalid) >> 4) |
		((flags & softfloat_flag_infinite) >> 1) |
		((flags & softfloat_flag_overflow) << 1) |
		((flags & softfloat_flag_underflow) << 3) |
		((flags & softfloat_flag_inexact) << 5));
	FPU_STATUSWORD |= result;
}

static INLINE sw_extFloat80_t c_float_to_floatx80(float value) {
	sw_float32_t f = *(sw_float32_t*)(&value);
	return f32_to_extF80(f);
}
static INLINE sw_extFloat80_t c_double_to_floatx80(double value) {
	sw_float64_t f = *(sw_float64_t*)(&value);
	return f64_to_extF80(f);
}
static INLINE sw_extFloat80_t int16_to_floatx80(SINT16 value) {
	return i32_to_extF80((SINT32)value);
}
static INLINE sw_extFloat80_t int32_to_floatx80(SINT32 value) {
	return i32_to_extF80(value);
}
static INLINE sw_extFloat80_t int64_to_floatx80(SINT64 value) {
	return i64_to_extF80(value);
}

static INLINE float floatx80_to_c_float(sw_extFloat80_t value) {
	sw_float32_t f = extF80_to_f32(value);
	return *(float*)(&f);
}
static INLINE double floatx80_to_c_double(sw_extFloat80_t value) {
	sw_float64_t f = extF80_to_f64(value);
	return *(double*)(&f);
}
static INLINE SINT16 floatx80_to_int16_np2(sw_extFloat80_t value) {
	return extF80_to_i16(value, softfloat_roundingMode, true);
}
static INLINE SINT32 floatx80_to_int32_np2(sw_extFloat80_t value) {
	return extF80_to_i32(value, softfloat_roundingMode, true);
}
static INLINE SINT64 floatx80_to_int64_np2(const sw_extFloat80_t value) {
	return extF80_to_i64(value, softfloat_roundingMode, true);
}
static INLINE SINT16 floatx80_to_int16(sw_extFloat80_t value) {
	return extF80_to_i16(value, softfloat_roundingMode, true);
}
static INLINE SINT32 floatx80_to_int32(sw_extFloat80_t value) {
	return extF80_to_i32(value, softfloat_roundingMode, true);
}
static INLINE SINT64 floatx80_to_int64(const sw_extFloat80_t value) {
	return extF80_to_i64(value, softfloat_roundingMode, true);
}
static INLINE SINT16 floatx80_to_int16_round_to_zero(sw_extFloat80_t value) {
	return extF80_to_i16(value, softfloat_round_minMag, true);
}
static INLINE SINT32 floatx80_to_int32_round_to_zero(sw_extFloat80_t value) {
	return extF80_to_i32(value, softfloat_round_minMag, true);
}
static INLINE SINT64 floatx80_to_int64_round_to_zero(const sw_extFloat80_t value) {
	return extF80_to_i64(value, softfloat_round_minMag, true);
}

static INLINE sw_extFloat80_t floatx80_round_to_int(const sw_extFloat80_t value) {
	return extF80_roundToInt(value, softfloat_roundingMode, true);
}

static INLINE sw_extFloat80_t floatx80_add(sw_extFloat80_t a, sw_extFloat80_t b) {
	return extF80_add(a, b);
}
static INLINE sw_extFloat80_t floatx80_sub(sw_extFloat80_t a, sw_extFloat80_t b) {
	return extF80_sub(a, b);
}
static INLINE sw_extFloat80_t floatx80_mul(sw_extFloat80_t a, sw_extFloat80_t b) {
	return extF80_mul(a, b);
}
static INLINE sw_extFloat80_t floatx80_div(sw_extFloat80_t a, sw_extFloat80_t b) {
	return extF80_div(a, b);
}
static INLINE bool floatx80_eq(sw_extFloat80_t a, sw_extFloat80_t b) {
	return extF80_eq(a, b);
}
static INLINE bool floatx80_lt(sw_extFloat80_t a, sw_extFloat80_t b) {
	return extF80_lt(a, b);
}
static INLINE bool floatx80_is_nan(sw_extFloat80_t a) {
	return extF80_isSignalingNaN(a);
}
static INLINE bool floatx80_is_inf(sw_extFloat80_t a) {
	uint16_t exponent = a.signExp & 0x7FFF;
	uint64_t significand = a.signif;
	return (exponent == 0x7FFF) && (significand == UINT64_C(0x8000000000000000));
}
static INLINE sw_extFloat80_t floatx80_sqrt(sw_extFloat80_t a) {
	return extF80_sqrt(a);
}



/*
 * FPU exception
 */

static INLINE void
fpu_check_NM_EXCEPTION(){
	// タスクスイッチまたはエミュレーション時にNM(デバイス使用不可例外)を発生させる
	if ((CPU_CR0 & (CPU_CR0_TS)) || (CPU_CR0 & CPU_CR0_EM)) {
		EXCEPTION(NM_EXCEPTION, 0);
	}
}
static INLINE void
fpu_check_NM_EXCEPTION2(){
	// タスクスイッチまたはエミュレーション時にNM(デバイス使用不可例外)を発生させる
	if ((CPU_CR0 & (CPU_CR0_TS)) || (CPU_CR0 & CPU_CR0_EM)) {
		EXCEPTION(NM_EXCEPTION, 0);
	}
}

static INLINE void fpu_checkexception() {
	if ((FPU_STATUSWORD & ~FPU_CTRLWORD) & 0x3F) {
		EXCEPTION(MF_EXCEPTION, 0);
	}
}


/*
 * FPU memory access function
 */

static void FPU_FLD80(UINT32 addr, UINT reg)
{
	*((REG80*)FPU_STAT.reg[reg].b) = fpu_memoryread_f(addr);
}

static void FPU_FLD_F32(UINT32 addr, UINT reg) {
	FPU_STAT.reg[reg].d = c_float_to_floatx80(fpu_memoryread_f32(addr));
}

static void FPU_FLD_F64(UINT32 addr, UINT reg) {
	FPU_STAT.reg[reg].d = c_double_to_floatx80(fpu_memoryread_f64(addr));
}

static void FPU_FLD_F80(UINT32 addr) {
	FPU_FLD80(addr, FPU_STAT_TOP);
}

static void FPU_FLD_I16(UINT32 addr, UINT reg) {
	FPU_STAT.reg[reg].d = int32_to_floatx80((SINT32)((SINT16)fpu_memoryread_w(addr)));
}

static void FPU_FLD_I32(UINT32 addr, UINT reg) {
	FPU_STAT.reg[reg].d = int32_to_floatx80((SINT32)fpu_memoryread_d(addr));
}

static void FPU_FLD_I64(UINT32 addr, UINT reg) {
	FPU_STAT.reg[reg].d = int64_to_floatx80((SINT64)fpu_memoryread_q(addr));
}

static void FPU_FBLD(UINT32 addr, UINT reg)
{
	int i;
	int tmp;

	REG80 bcdbuf;
	SINT64 val = 0;
	UINT8 in = 0;

	// 80bitまとめて読み取り
	bcdbuf = fpu_memoryread_f(addr);

	// 0〜8byte目の処理 BCD
	for (i = 8; i >= 0; i--) {
		in = bcdbuf.b[i];
		tmp = ((in >> 4) & 0xf) * 10 + (in & 0xf);
		val = val * 100 + tmp;
	}

	// 9byte目は符号のみ意味がある
	if (bcdbuf.b[9] & 0x80) {
		val = -val;
	}

	FPU_STAT.reg[reg].d = int64_to_floatx80(val);
}

static INLINE void FPU_FLD_F32_EA(UINT32 addr) {
	FPU_FLD_F32(addr, 8);
}
static INLINE void FPU_FLD_F64_EA(UINT32 addr) {
	FPU_FLD_F64(addr, 8);
}
static INLINE void FPU_FLD_I32_EA(UINT32 addr) {
	FPU_FLD_I32(addr, 8);
}
static INLINE void FPU_FLD_I16_EA(UINT32 addr) {
	FPU_FLD_I16(addr, 8);
}


static void FPU_ST80(UINT32 addr, UINT reg)
{
	fpu_memorywrite_f(addr, (REG80*)FPU_STAT.reg[reg].b);
}

static void FPU_FST_F32(UINT32 addr) {
	fpu_memorywrite_f32(addr, floatx80_to_c_float(FPU_STAT.reg[FPU_STAT_TOP].d));
}

static void FPU_FST_F64(UINT32 addr) {
	fpu_memorywrite_f64(addr, floatx80_to_c_double(FPU_STAT.reg[FPU_STAT_TOP].d));
}

static void FPU_FST_F80(UINT32 addr) {
	FPU_ST80(addr, FPU_STAT_TOP);
}

static INLINE void FPU_FST_I16(UINT32 addr) {
	FPU_SoftFloatBegin();
	fpu_memorywrite_w(addr, (UINT16)floatx80_to_int16_np2(FPU_STAT.reg[FPU_STAT_TOP].d));
	FPU_SoftFloatEnd();
}

static INLINE void FPU_FST_I32(UINT32 addr) {
	FPU_SoftFloatBegin();
	fpu_memorywrite_d(addr, (UINT32)floatx80_to_int32_np2(FPU_STAT.reg[FPU_STAT_TOP].d));
	FPU_SoftFloatEnd();
}

static INLINE void FPU_FST_I64(UINT32 addr) {
	FPU_SoftFloatBegin();
	fpu_memorywrite_q(addr, (UINT64)floatx80_to_int64_np2(FPU_STAT.reg[FPU_STAT_TOP].d));
	FPU_SoftFloatEnd();
}

static void FPU_FBST(UINT32 addr)
{
	SINT64 val;
	REG80 bcdbuf = { 0 };
	UINT i;

	uint_fast8_t oldrnd = softfloat_roundingMode;
	softfloat_roundingMode = softfloat_round_min;

	FPU_SoftFloatBegin();
	val = floatx80_to_int64(FPU_STAT.reg[FPU_STAT_TOP].d);

	// 9byte目は符号のみ意味がある
	if (val < 0)
	{
		bcdbuf.b[9] = 0x80;
		val = -val;
	}

	// 0〜8byte目の処理 BCD
	for (i = 0; i < 9; i++) {
		bcdbuf.b[i] = (UINT8)(val % 10);
		val /= 10;
		bcdbuf.b[i] |= (UINT8)(val % 10) << 4;
		val /= 10;
	}

	// 80bitまとめて書き込み
	fpu_memorywrite_f(addr, &bcdbuf);

	softfloat_roundingMode = oldrnd;
	FPU_SoftFloatEnd();
}


/*
 * FPU interface
 */

static void
FPU_FINIT(void)
{
	int i;
	FPU_SetCW(0x37F);
	FPU_STATUSWORD = 0;
	FPU_INSTPTR_SEG = 0;
	FPU_INSTPTR_OFFSET = 0;
	FPU_DATAPTR_SEG = 0;
	FPU_DATAPTR_OFFSET = 0;
	FPU_LASTINSTOP = 0;
	FPU_STAT_TOP=FP_TOP_GET();
	for(i=0;i<8;i++){
		// Emptyセットしてもレジスタの内容は消してはいけない
		FPU_STAT.tag[i] = TAG_Empty;
	}
	FPU_STAT.tag[8] = TAG_Valid; // dummy
	FPU_STAT.mmxenable = 0;
}
void SF_FPU_FINIT(void){
	int i;
	FPU_FINIT();
	for(i=0;i<8;i++){
		FPU_STAT.tag[i] = TAG_Empty;
		FPU_STAT.reg[i].l.ext = 0;
		FPU_STAT.reg[i].l.lower = 0;
		FPU_STAT.reg[i].l.upper = 0;
	}
}

static void FPU_FLDCW(UINT32 addr)
{
	UINT16 temp = cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, addr);
	FPU_SetCW(temp);
}

static UINT16 FPU_GetTag(void)
{
	UINT i;

	UINT16 tag = 0;
	for (i = 0; i < 8; i++)
		tag |= ((FPU_STAT.tag[i] & 3) << (2 * i));
	return tag;
}
/*
 * Record the instruction address and opcode fields exposed by x87 environment
 * save instructions.  CPU_PREV_EIP identifies the beginning of the current
 * instruction, including prefixes.  FPU_LASTINSTOP stores the D8..DF escape
 * number in bits 10..8 and the ModR/M byte in bits 7..0.
 */
static INLINE void FPU_RecordInstruction(UINT esc, UINT modrm)
{
	FPU_INSTPTR_SEG = CPU_CS;
	FPU_INSTPTR_OFFSET = CPU_PREV_EIP;
	FPU_LASTINSTOP = (UINT16)(((esc & 7) << 8) | (modrm & 0xff));
}

static INLINE int FPU_IsControlInstruction(UINT esc, UINT modrm)
{
	UINT idx = (modrm >> 3) & 7;
	UINT sub = modrm & 7;

	/* These D9 memory forms load or store x87 control/environment state. */
	if (esc == 1 && modrm < 0xc0 && idx >= 4)
		return 1;
	/* DB E0..E7 contains x87 control operations such as FCLEX and FINIT. */
	if (esc == 3 && modrm >= 0xc0 && idx == 4)
		return 1;
	/* These DD memory forms restore, save, or inspect x87 state. */
	if (esc == 5 && modrm < 0xc0 && (idx == 4 || idx == 6 || idx == 7))
		return 1;
	/* DF E0 transfers the status word to AX without a data operand. */
	if (esc == 7 && modrm >= 0xc0 && idx == 4 && sub == 0)
		return 1;
	return 0;
}

static INLINE void FPU_RecordInstructionIfNeeded(UINT esc, UINT modrm)
{
	if (!FPU_IsControlInstruction(esc, modrm))
		FPU_RecordInstruction(esc, modrm);
}

/* x87 control/state images use the current effective-address segment but do
 * not make that image address the saved x87 data-operand pointer. */
static INLINE UINT8 FPU_EnvRead8(UINT32 addr)
{
	return cpu_vmemoryread_b(CPU_INST_SEGREG_INDEX, addr);
}
static INLINE UINT16 FPU_EnvRead16(UINT32 addr)
{
	return cpu_vmemoryread_w(CPU_INST_SEGREG_INDEX, addr);
}
static INLINE UINT32 FPU_EnvRead32(UINT32 addr)
{
	return cpu_vmemoryread_d(CPU_INST_SEGREG_INDEX, addr);
}
static INLINE UINT64 FPU_EnvRead64(UINT32 addr)
{
	return cpu_vmemoryread_q(CPU_INST_SEGREG_INDEX, addr);
}
static INLINE void FPU_EnvWrite8(UINT32 addr, UINT8 value)
{
	cpu_vmemorywrite_b(CPU_INST_SEGREG_INDEX, addr, value);
}
static INLINE void FPU_EnvWrite16(UINT32 addr, UINT16 value)
{
	cpu_vmemorywrite_w(CPU_INST_SEGREG_INDEX, addr, value);
}
static INLINE void FPU_EnvWrite32(UINT32 addr, UINT32 value)
{
	cpu_vmemorywrite_d(CPU_INST_SEGREG_INDEX, addr, value);
}
static INLINE void FPU_EnvWrite64(UINT32 addr, UINT64 value)
{
	cpu_vmemorywrite_q(CPU_INST_SEGREG_INDEX, addr, value);
}

static INLINE UINT32 FPU_RealLinear(FPU_PTR p)
{
	return (((UINT32)p.seg) << 4) + p.offset;
}

static UINT8 FPU_GetTag8(void)
{
	UINT i;

	UINT8 tag = 0;
	for (i = 0; i < 8; i++)
		tag |= ((FPU_STAT.tag[i] == TAG_Empty ? 0 : 1) << (i));
	return tag;
}

static INLINE void FPU_SetTag(UINT16 tag)
{
	UINT i;

	for (i = 0; i < 8; i++) {
		FPU_STAT.tag[i] = (FP_TAG)((tag >> (2 * i)) & 3);

	}
}
static INLINE void FPU_SetTag8(UINT8 tag)
{
	UINT i;

	for (i = 0; i < 8; i++) {
		FPU_STAT.tag[i] = (((tag >> i) & 1) == 0 ? TAG_Empty : TAG_Valid);
	}
}

static void FPU_prepush(void) {
	FPU_STAT_TOP = (FPU_STAT_TOP - 1) & 7;
	if(FPU_STAT.tag[FPU_STAT_TOP] == TAG_Valid){
		FPU_STATUSWORD |= FP_C1_FLAG | FP_SF_FLAG | FP_IE_FLAG;
	}else{
		FPU_STATUSWORD &= ~FP_C1_FLAG;
	}
	FPU_STAT.tag[FPU_STAT_TOP] = TAG_Valid;
}
static void FPU_push(sw_extFloat80_t in) {
	FPU_prepush();
	FPU_STAT.reg[FPU_STAT_TOP].d = in;
}

static void FPU_pop(void) {
	if(FPU_STAT.tag[FPU_STAT_TOP] == TAG_Empty){
		FPU_STATUSWORD |= FP_SF_FLAG | FP_IE_FLAG;
	}
	FPU_STATUSWORD &= ~FP_C1_FLAG;
	FPU_STAT.tag[FPU_STAT_TOP] = TAG_Empty;
	FPU_STAT.mmxenable = 0;
	FPU_STAT_TOP = ((FPU_STAT_TOP + 1) & 7);
}

/*
 * FPU instruction
 */

 // レジスタ操作
static void FPU_FST(UINT st, UINT other) {
	FPU_STAT.tag[other] = FPU_STAT.tag[st];
	FPU_STAT.reg[other] = FPU_STAT.reg[st];
}
static void FPU_FXCH(UINT st, UINT other) {
	FP_TAG tag;
	FP_REG reg;

	tag = FPU_STAT.tag[other];
	reg = FPU_STAT.reg[other];
	FPU_STAT.tag[other] = FPU_STAT.tag[st];
	FPU_STAT.reg[other] = FPU_STAT.reg[st];
	FPU_STAT.tag[st] = tag;
	FPU_STAT.reg[st] = reg;
}
static void FPU_FLD1(void) {
	FPU_prepush();
	FPU_STAT.reg[FPU_STAT_TOP].d = c_double_to_floatx80(1.0);
}
static void FPU_FLDL2T(void) {
	FPU_prepush();
	FPU_STAT.reg[FPU_STAT_TOP].d = c_double_to_floatx80(L2T);
}
static void FPU_FLDL2E(void) {
	FPU_prepush();
	FPU_STAT.reg[FPU_STAT_TOP].d = c_double_to_floatx80(L2E);
}
static void FPU_FLDPI(void) {
	FPU_prepush();
	FPU_STAT.reg[FPU_STAT_TOP].d = c_double_to_floatx80(PI);
}
static void FPU_FLDLG2(void) {
	FPU_prepush();
	FPU_STAT.reg[FPU_STAT_TOP].d = c_double_to_floatx80(LG2);
}
static void FPU_FLDLN2(void) {
	FPU_prepush();
	FPU_STAT.reg[FPU_STAT_TOP].d = c_double_to_floatx80(LN2);
}
static void FPU_FLDZ(void) {
	FPU_prepush();
	FPU_STAT.reg[FPU_STAT_TOP].d = c_double_to_floatx80(0.0);
	FPU_STAT.tag[FPU_STAT_TOP] = TAG_Zero;
	FPU_STAT.mmxenable = 0;
}

// 四則演算
static INLINE void FPU_FADD(UINT op1, UINT op2) {
	FPU_SoftFloatBegin();
	FPU_STAT.reg[op1].d = floatx80_add(FPU_STAT.reg[op1].d, FPU_STAT.reg[op2].d);
	FPU_SoftFloatEnd();
	return;
}
static INLINE void FPU_FMUL(UINT st, UINT other) {
	FPU_SoftFloatBegin();
	FPU_STAT.reg[st].d = floatx80_mul(FPU_STAT.reg[st].d, FPU_STAT.reg[other].d);
	FPU_SoftFloatEnd();
	return;
}
static INLINE void FPU_FSUB(UINT st, UINT other) {
	FPU_SoftFloatBegin();
	FPU_STAT.reg[st].d = floatx80_sub(FPU_STAT.reg[st].d, FPU_STAT.reg[other].d);
	FPU_SoftFloatEnd();
	return;
}
static INLINE void FPU_FSUBR(UINT st, UINT other) {
	FPU_SoftFloatBegin();
	FPU_STAT.reg[st].d = floatx80_sub(FPU_STAT.reg[other].d, FPU_STAT.reg[st].d);
	FPU_SoftFloatEnd();
	return;
}
static INLINE void FPU_FDIV(UINT st, UINT other) {
	FPU_SoftFloatBegin();
	FPU_STAT.reg[st].d = floatx80_div(FPU_STAT.reg[st].d, FPU_STAT.reg[other].d);
	FPU_SoftFloatEnd();
	return;
}
static INLINE void FPU_FDIVR(UINT st, UINT other) {
	FPU_SoftFloatBegin();
	FPU_STAT.reg[st].d = floatx80_div(FPU_STAT.reg[other].d, FPU_STAT.reg[st].d);
	FPU_SoftFloatEnd();
	return;
}
static INLINE void FPU_FADD_EA(UINT op1) {
	FPU_FADD(op1, 8);
}
static INLINE void FPU_FMUL_EA(UINT op1) {
	FPU_FMUL(op1, 8);
}
static INLINE void FPU_FSUB_EA(UINT op1) {
	FPU_FSUB(op1, 8);
}
static INLINE void FPU_FSUBR_EA(UINT op1) {
	FPU_FSUBR(op1, 8);
}
static INLINE void FPU_FDIV_EA(UINT op1) {
	FPU_FDIV(op1, 8);
}
static INLINE void FPU_FDIVR_EA(UINT op1) {
	FPU_FDIVR(op1, 8);
}
static void FPU_FPREM(void) {
	sw_extFloat80_t val, div;
	SINT64 qint;

	FPU_SoftFloatBegin();
	val = FPU_STAT.reg[FPU_STAT_TOP].d;
	div = FPU_STAT.reg[FPU_ST(1)].d;
	qint = floatx80_to_int64_round_to_zero(floatx80_div(val, div)); // int(被除数 / 除数) = 商

	FPU_STAT.reg[FPU_STAT_TOP].d = floatx80_sub(val, floatx80_mul(int64_to_floatx80(qint), div)); // 被除数 - 商 x 除数 = 剰余
	FPU_STATUSWORD &= ~(FP_C0_FLAG | FP_C1_FLAG | FP_C2_FLAG | FP_C3_FLAG);
	if(qint & 4) FPU_STATUSWORD |= FP_C0_FLAG; // 商のbit2
	if(qint & 2) FPU_STATUSWORD |= FP_C3_FLAG; // 商のbit1
	if(qint & 1) FPU_STATUSWORD |= FP_C1_FLAG; // 商のbit0
	// C2クリアで完了扱い
	FPU_SoftFloatEnd();
}

static void FPU_FPREM1(void) {
	sw_extFloat80_t val, div, q;
	SINT64 qint;
	uint_fast8_t oldrnd = softfloat_roundingMode;

	// IEEE 754 剰余　商を最も近い整数値とする。余りが負値になることが有り得る

	FPU_SoftFloatBegin();
	val = FPU_STAT.reg[FPU_STAT_TOP].d;
	div = FPU_STAT.reg[FPU_ST(1)].d;
	q = floatx80_add(floatx80_div(val, div), c_double_to_floatx80(0.5)); // floor(値 + 0.5)で四捨五入 厳密には負値の境界で違うが微々たる差として気にしないことにする。
	softfloat_roundingMode = softfloat_round_min;
	qint = floatx80_to_int64(q); // 四捨五入(被除数 / 除数) = 最も整数に近い商

	FPU_STAT.reg[FPU_STAT_TOP].d = floatx80_sub(val, floatx80_mul(int64_to_floatx80(qint), div)); // 被除数 - 商 x 除数 = 剰余
	FPU_STATUSWORD &= ~(FP_C0_FLAG | FP_C1_FLAG | FP_C2_FLAG | FP_C3_FLAG);
	if(qint & 4) FPU_STATUSWORD |= FP_C0_FLAG; // 商のbit2
	if(qint & 2) FPU_STATUSWORD |= FP_C3_FLAG; // 商のbit1
	if(qint & 1) FPU_STATUSWORD |= FP_C1_FLAG; // 商のbit0
	// C2クリアで完了扱い
	softfloat_roundingMode = oldrnd;
	FPU_SoftFloatEnd();
}

// 数学関数
static void FPU_FSIN(void) {
	FPU_SoftFloatBegin();
	FPU_STAT.reg[FPU_STAT_TOP].d = c_double_to_floatx80(sin(floatx80_to_c_double(FPU_STAT.reg[FPU_STAT_TOP].d)));
	FPU_STATUSWORD &= ~FP_C2_FLAG;
	FPU_SoftFloatEnd();
	return;
}
static void FPU_FCOS(void) {
	FPU_SoftFloatBegin();
	FPU_STAT.reg[FPU_STAT_TOP].d = c_double_to_floatx80(cos(floatx80_to_c_double(FPU_STAT.reg[FPU_STAT_TOP].d)));
	FPU_STATUSWORD &= ~FP_C2_FLAG;
	FPU_SoftFloatEnd();
	return;
}
static void FPU_FSINCOS(void) {
	double temp;

	FPU_SoftFloatBegin();
	temp = floatx80_to_c_double(FPU_STAT.reg[FPU_STAT_TOP].d);
	FPU_STAT.reg[FPU_STAT_TOP].d = c_double_to_floatx80(sin(temp));
	FPU_push(c_double_to_floatx80(cos(temp)));
	FPU_STATUSWORD &= ~FP_C2_FLAG;
	FPU_SoftFloatEnd();
	return;
}
static void FPU_FPTAN(void) {
	FPU_SoftFloatBegin();
	FPU_STAT.reg[FPU_STAT_TOP].d = c_double_to_floatx80(tan(floatx80_to_c_double(FPU_STAT.reg[FPU_STAT_TOP].d)));
	FPU_push(c_double_to_floatx80(1.0));
	FPU_STATUSWORD &= ~FP_C2_FLAG;
	FPU_SoftFloatEnd();
	return;
}
static void FPU_FPATAN(void) {
	FPU_SoftFloatBegin();
	FPU_STAT.reg[FPU_ST(1)].d = c_double_to_floatx80(atan2(floatx80_to_c_double(FPU_STAT.reg[FPU_ST(1)].d), floatx80_to_c_double(FPU_STAT.reg[FPU_STAT_TOP].d)));
	FPU_pop();
	FPU_SoftFloatEnd();
	return;
}
static void FPU_FSQRT(void) {
	FPU_SoftFloatBegin();
	FPU_STAT.reg[FPU_STAT_TOP].d = floatx80_sqrt(FPU_STAT.reg[FPU_STAT_TOP].d);
	FPU_SoftFloatEnd();
	return;
}
static void FPU_FRNDINT(void) {
	FPU_SoftFloatBegin();
	FPU_STAT.reg[FPU_STAT_TOP].d = floatx80_round_to_int(FPU_STAT.reg[FPU_STAT_TOP].d);
	FPU_SoftFloatEnd();
}
static void FPU_F2XM1(void) {
	FPU_STAT.reg[FPU_STAT_TOP].d = c_double_to_floatx80(pow(2.0, floatx80_to_c_double(FPU_STAT.reg[FPU_STAT_TOP].d)) - 1);
}
static void FPU_FYL2X(void) {
	FPU_STAT.reg[FPU_ST(1)].d = floatx80_mul(FPU_STAT.reg[FPU_ST(1)].d, c_double_to_floatx80(log(floatx80_to_c_double(FPU_STAT.reg[FPU_STAT_TOP].d)) / log(2.0)));
	FPU_pop();
}
static void FPU_FYL2XP1(void) {
	FPU_STAT.reg[FPU_ST(1)].d = floatx80_mul(FPU_STAT.reg[FPU_ST(1)].d, c_double_to_floatx80(log(floatx80_to_c_double(FPU_STAT.reg[FPU_STAT_TOP].d) + 1.0) / log(2.0)));
	FPU_pop();
}
static void FPU_FSCALE(void) {
	FPU_STAT.reg[FPU_STAT_TOP].d = floatx80_mul(FPU_STAT.reg[FPU_STAT_TOP].d, c_double_to_floatx80(pow(2.0, floatx80_to_c_double(FPU_STAT.reg[FPU_ST(1)].d))));
}
static void FPU_FCHS(void) {
	FPU_STAT.reg[FPU_STAT_TOP].b[9] ^= 0x80;
}
static void FPU_FABS(void) {
	FPU_STAT.reg[FPU_STAT_TOP].b[9] &= ~0x80;
}

// 比較
static void FPU_FCOM(UINT st, UINT other) {
	FPU_STATUSWORD &= ~(FP_C0_FLAG | FP_C2_FLAG | FP_C3_FLAG);
	if (((FPU_STAT.tag[st] != TAG_Valid) && (FPU_STAT.tag[st] != TAG_Zero)) ||
		((FPU_STAT.tag[other] != TAG_Valid) && (FPU_STAT.tag[other] != TAG_Zero)) ||
		(floatx80_is_nan(FPU_STAT.reg[st].d) || floatx80_is_nan(FPU_STAT.reg[other].d))) {
		FPU_STATUSWORD |= FP_C3_FLAG|FP_C2_FLAG|FP_C0_FLAG;
	}
	else if (floatx80_eq(FPU_STAT.reg[st].d, FPU_STAT.reg[other].d)) {
		FPU_STATUSWORD |= FP_C3_FLAG;
	}
	else if (floatx80_lt(FPU_STAT.reg[st].d, FPU_STAT.reg[other].d)) {
		FPU_STATUSWORD |= FP_C0_FLAG;
	}
}
static void FPU_FCOMI(UINT st, UINT other) {
	CPU_FLAGL &= ~(Z_FLAG|P_FLAG|C_FLAG);
	if (((FPU_STAT.tag[st] != TAG_Valid) && (FPU_STAT.tag[st] != TAG_Zero)) ||
		((FPU_STAT.tag[other] != TAG_Valid) && (FPU_STAT.tag[other] != TAG_Zero)) ||
		(floatx80_is_nan(FPU_STAT.reg[st].d) || floatx80_is_nan(FPU_STAT.reg[other].d))) {
		CPU_FLAGL |= Z_FLAG|P_FLAG|C_FLAG;
	}
	else if (floatx80_eq(FPU_STAT.reg[st].d, FPU_STAT.reg[other].d)) {
		CPU_FLAGL |= Z_FLAG;
	}
	else if (floatx80_lt(FPU_STAT.reg[st].d, FPU_STAT.reg[other].d)) {
		CPU_FLAGL |= C_FLAG;
	}
}
static void FPU_FUCOM(UINT st, UINT other) {
	// 例外絡みの挙動が違うがほぼ同じとしてスルー
	FPU_FCOM(st, other);
}
static void FPU_FUCOMI(UINT st, UINT other) {
	// 例外絡みの挙動が違うがほぼ同じとしてスルー
	FPU_FCOMI(st, other);
}
static INLINE void FPU_FCOM_EA(UINT op1) {
	FPU_FCOM(op1, 8);
}
static void FPU_FTST(void) {
	FPU_STAT.reg[8].d = c_double_to_floatx80(0.0);
	FPU_FCOM(FPU_STAT_TOP, 8);
}

// 条件付きコピー
static void FPU_FCMOVB(UINT st, UINT other) {
	if (CPU_FLAGL & C_FLAG) {
		FPU_STAT.tag[st] = FPU_STAT.tag[other];
		FPU_STAT.reg[st] = FPU_STAT.reg[other];
	}
}
static void FPU_FCMOVE(UINT st, UINT other) {
	if (CPU_FLAGL & Z_FLAG) {
		FPU_STAT.tag[st] = FPU_STAT.tag[other];
		FPU_STAT.reg[st] = FPU_STAT.reg[other];
	}
}
static void FPU_FCMOVBE(UINT st, UINT other) {
	if (CPU_FLAGL & (C_FLAG | Z_FLAG)) {
		FPU_STAT.tag[st] = FPU_STAT.tag[other];
		FPU_STAT.reg[st] = FPU_STAT.reg[other];
	}
}
static void FPU_FCMOVU(UINT st, UINT other) {
	if (CPU_FLAGL & P_FLAG) {
		FPU_STAT.tag[st] = FPU_STAT.tag[other];
		FPU_STAT.reg[st] = FPU_STAT.reg[other];
	}
}
static void FPU_FCMOVNB(UINT st, UINT other) {
	if (!(CPU_FLAGL & C_FLAG)) {
		FPU_STAT.tag[st] = FPU_STAT.tag[other];
		FPU_STAT.reg[st] = FPU_STAT.reg[other];
	}
}
static void FPU_FCMOVNE(UINT st, UINT other) {
	if (!(CPU_FLAGL & Z_FLAG)) {
		FPU_STAT.tag[st] = FPU_STAT.tag[other];
		FPU_STAT.reg[st] = FPU_STAT.reg[other];
	}
}
static void FPU_FCMOVNBE(UINT st, UINT other) {
	if (!(CPU_FLAGL & (C_FLAG | Z_FLAG))) {
		FPU_STAT.tag[st] = FPU_STAT.tag[other];
		FPU_STAT.reg[st] = FPU_STAT.reg[other];
	}
}
static void FPU_FCMOVNU(UINT st, UINT other) {
	if (!(CPU_FLAGL & P_FLAG)) {
		FPU_STAT.tag[st] = FPU_STAT.tag[other];
		FPU_STAT.reg[st] = FPU_STAT.reg[other];
	}
}

// 浮動小数点数操作
static void FPU_FXAM(void) {
	FPU_STATUSWORD &= ~(FP_C0_FLAG | FP_C1_FLAG | FP_C2_FLAG | FP_C3_FLAG);
	if (FPU_STAT.reg[FPU_STAT_TOP].d.signExp & 0x8000) {
		FPU_STATUSWORD |= FP_C1_FLAG;
	}

	if (FPU_STAT.tag[FPU_STAT_TOP] == TAG_Empty) {
		FPU_STATUSWORD |= FP_C3_FLAG;
		FPU_STATUSWORD |= FP_C0_FLAG;
	}
	else if (floatx80_is_nan(FPU_STAT.reg[FPU_STAT_TOP].d)) {
		FPU_STATUSWORD |= FP_C0_FLAG;
	}
	else if (floatx80_is_inf(FPU_STAT.reg[FPU_STAT_TOP].d)) {
		FPU_STATUSWORD |= FP_C2_FLAG;
		FPU_STATUSWORD |= FP_C0_FLAG;
	}
	else if (floatx80_eq(FPU_STAT.reg[FPU_STAT_TOP].d, c_double_to_floatx80(0.0))) {
		FPU_STATUSWORD |= FP_C3_FLAG;
	}
	else {
		FPU_STATUSWORD |= FP_C2_FLAG;
	}
}

static void FPU_FXTRACT(void) {
	SINT32 expval;
	sw_extFloat80_t fracval;

	fracval = FPU_STAT.reg[FPU_STAT_TOP].d;
	expval = (SINT32)((UINT16)fracval.signExp & 0x7FFF) - 0x3FFF; // 指数部分を抽出、バイアス分を引く
	fracval.signExp = (SINT16)(((UINT16)fracval.signExp & 0x8000) | 0x3FFF); // 符号は残し、指数部分を0x3FFF（バイアス分=0）にして仮数だけにする
	FPU_STAT.reg[FPU_STAT_TOP].d = int64_to_floatx80(expval); // 指数の書き込み
	FPU_push(fracval); // 仮数のpush
}

// 環境ロード・ストア
static void FPU_FSTENV(UINT32 addr)
{
	const int protected_mode = ((CPU_CR0 & CPU_CR0_PE) != 0) && ((CPU_EFLAG & VM_FLAG) == 0);
	const FPU_PTR inst = FPU_INSTPTR;
	const FPU_PTR data = FPU_DATAPTR;
	const UINT16 lastop = (UINT16)(FPU_LASTINSTOP & 0x07ff);

	FP_TOP_SET(FPU_STAT_TOP);

	if (!CPU_INST_OP32) {
		FPU_EnvWrite16(addr + 0, FPU_CTRLWORD);
		FPU_EnvWrite16(addr + 2, FPU_STATUSWORD);
		FPU_EnvWrite16(addr + 4, FPU_GetTag());
		if (protected_mode) {
			/* m14byte protected-mode image: FIP:FCS and FDP:FDS are selectors:offsets. */
			FPU_EnvWrite16(addr + 6, (UINT16)inst.offset);
			FPU_EnvWrite16(addr + 8, inst.seg);
			FPU_EnvWrite16(addr + 10, (UINT16)data.offset);
			FPU_EnvWrite16(addr + 12, data.seg);
		} else {
			/* m14byte real/v86 image encodes 20-bit physical instruction/data addresses. */
			UINT32 ip = FPU_RealLinear(inst);
			UINT32 dp = FPU_RealLinear(data);
			FPU_EnvWrite16(addr + 6, (UINT16)ip);
			FPU_EnvWrite16(addr + 8,
			    (UINT16)((((ip >> 16) & 0x0f) << 12) | lastop));
			FPU_EnvWrite16(addr + 10, (UINT16)dp);
			FPU_EnvWrite16(addr + 12,
			    (UINT16)(((dp >> 16) & 0x0f) << 12));
		}
	} else {
		FPU_EnvWrite32(addr + 0, (UINT32)FPU_CTRLWORD);
		FPU_EnvWrite32(addr + 4, (UINT32)FPU_STATUSWORD);
		FPU_EnvWrite32(addr + 8, (UINT32)FPU_GetTag());
		if (protected_mode) {
			/* m28byte protected-mode image keeps 32-bit offsets and 16-bit selectors. */
			FPU_EnvWrite32(addr + 12, inst.offset);
			FPU_EnvWrite32(addr + 16,
			    ((UINT32)lastop << 16) | inst.seg);
			FPU_EnvWrite32(addr + 20, data.offset);
			FPU_EnvWrite32(addr + 24, (UINT32)data.seg);
		} else {
			/* m28byte real/v86 image encodes physical instruction/data addresses. */
			UINT32 ip = FPU_RealLinear(inst);
			UINT32 dp = FPU_RealLinear(data);
			FPU_EnvWrite32(addr + 12, ip & 0xffff);
			FPU_EnvWrite32(addr + 16,
			    ((ip >> 16) << 12) | lastop);
			FPU_EnvWrite32(addr + 20, dp & 0xffff);
			FPU_EnvWrite32(addr + 24, (dp >> 16) << 12);
		}
	}

	/* The stored image receives the old control word; the live x87 control
	 * word then has all exception-mask bits set. */
	FPU_SetCW((UINT16)(FPU_CTRLWORD | 0x003f));
	CPU_WORKCLOCK(60);
}

static void FPU_FLDENV(UINT32 addr)
{
	const int protected_mode = ((CPU_CR0 & CPU_CR0_PE) != 0) && ((CPU_EFLAG & VM_FLAG) == 0);

	if (!CPU_INST_OP32) {
		FPU_SetCW(FPU_EnvRead16(addr + 0));
		FPU_STATUSWORD = FPU_EnvRead16(addr + 2);
		FPU_SetTag(FPU_EnvRead16(addr + 4));
		if (protected_mode) {
			FPU_INSTPTR_OFFSET = FPU_EnvRead16(addr + 6);
			FPU_INSTPTR_SEG = FPU_EnvRead16(addr + 8);
			FPU_DATAPTR_OFFSET = FPU_EnvRead16(addr + 10);
			FPU_DATAPTR_SEG = FPU_EnvRead16(addr + 12);
		} else {
			UINT16 iplo = FPU_EnvRead16(addr + 6);
			UINT16 iphi_op = FPU_EnvRead16(addr + 8);
			UINT16 dplo = FPU_EnvRead16(addr + 10);
			UINT16 dphi = FPU_EnvRead16(addr + 12);
			FPU_INSTPTR_SEG = 0;
			FPU_INSTPTR_OFFSET = (UINT32)iplo |
			    ((UINT32)((iphi_op >> 12) & 0x0f) << 16);
			FPU_DATAPTR_SEG = 0;
			FPU_DATAPTR_OFFSET = (UINT32)dplo |
			    ((UINT32)((dphi >> 12) & 0x0f) << 16);
			FPU_LASTINSTOP = (UINT16)(iphi_op & 0x07ff);
		}
	} else {
		FPU_SetCW((UINT16)FPU_EnvRead32(addr + 0));
		FPU_STATUSWORD = (UINT16)FPU_EnvRead32(addr + 4);
		FPU_SetTag((UINT16)FPU_EnvRead32(addr + 8));
		if (protected_mode) {
			UINT32 cssel_op;
			FPU_INSTPTR_OFFSET = FPU_EnvRead32(addr + 12);
			cssel_op = FPU_EnvRead32(addr + 16);
			FPU_INSTPTR_SEG = (UINT16)cssel_op;
			FPU_LASTINSTOP = (UINT16)((cssel_op >> 16) & 0x07ff);
			FPU_DATAPTR_OFFSET = FPU_EnvRead32(addr + 20);
			FPU_DATAPTR_SEG = (UINT16)FPU_EnvRead32(addr + 24);
		} else {
			UINT32 iplo = FPU_EnvRead32(addr + 12);
			UINT32 iphi_op = FPU_EnvRead32(addr + 16);
			UINT32 dplo = FPU_EnvRead32(addr + 20);
			UINT32 dphi = FPU_EnvRead32(addr + 24);
			FPU_INSTPTR_SEG = 0;
			FPU_INSTPTR_OFFSET = (iplo & 0xffff) | ((iphi_op >> 12) << 16);
			FPU_DATAPTR_SEG = 0;
			FPU_DATAPTR_OFFSET = (dplo & 0xffff) | ((dphi >> 12) << 16);
			FPU_LASTINSTOP = (UINT16)(iphi_op & 0x07ff);
		}
	}
	FPU_STAT_TOP = FP_TOP_GET();
	CPU_WORKCLOCK(60);
}
static void FPU_FSAVE(UINT32 addr)
{
	UINT start;
	UINT i;

	FPU_FSTENV(addr);
	start = ((CPU_INST_OP32) ? 28 : 14);
	for (i = 0; i < 8; i++) {
		FPU_ST80(addr + start, FPU_ST(i));
		start += 10;
	}
	FPU_FINIT();
	CPU_WORKCLOCK(60);
}
static void FPU_FRSTOR(UINT32 addr)
{
	UINT start;
	UINT i;
	FPU_PTR inst;
	FPU_PTR data;
	UINT16 lastop;

	FPU_FLDENV(addr);
	inst = FPU_INSTPTR;
	data = FPU_DATAPTR;
	lastop = FPU_LASTINSTOP;
	start = ((CPU_INST_OP32) ? 28 : 14);
	for (i = 0; i < 8; i++) {
		FPU_FLD80(addr + start, FPU_ST(i));
		start += 10;
	}
	/* The register image is state restored by FRSTOR, so its memory reads do
	 * not replace the FDP contained in the restored environment. */
	FPU_INSTPTR = inst;
	FPU_DATAPTR = data;
	FPU_LASTINSTOP = lastop;
	CPU_WORKCLOCK(60);
}
static void FPU_FXSAVE(UINT32 addr) {
	UINT start;
	UINT i;
	const FPU_PTR inst = FPU_INSTPTR;
	const FPU_PTR data = FPU_DATAPTR;
	const UINT16 lastop = FPU_LASTINSTOP;

	FP_TOP_SET(FPU_STAT_TOP);
	FPU_EnvWrite16(addr + 0, FPU_CTRLWORD);
	FPU_EnvWrite16(addr + 2, FPU_STATUSWORD);
	FPU_EnvWrite8(addr + 4, FPU_GetTag8());
	FPU_EnvWrite8(addr + 5, 0);
	FPU_EnvWrite16(addr + 6, (UINT16)(lastop & 0x07ff));
	FPU_EnvWrite32(addr + 8, inst.offset);
	FPU_EnvWrite16(addr + 12, inst.seg);
	FPU_EnvWrite16(addr + 14, 0);
	FPU_EnvWrite32(addr + 16, data.offset);
	FPU_EnvWrite16(addr + 20, data.seg);
	FPU_EnvWrite16(addr + 22, 0);
#ifdef USE_SSE
	FPU_EnvWrite32(addr + 24, SSE_MXCSR);
#endif
	start = 32;
	for (i = 0; i < 8; i++) {
		FPU_ST80(addr + start, FPU_ST(i));
		start += 16;
	}
#ifdef USE_SSE
	start = 160;
	for (i = 0; i < 8; i++) {
		FPU_EnvWrite64(addr + start + 0, SSE_XMMREG(i).ul64[0]);
		FPU_EnvWrite64(addr + start + 8, SSE_XMMREG(i).ul64[1]);
		start += 16;
	}
#endif
	/* FXSAVE serializes the existing x87 pointers; its destination is not an
	 * x87 arithmetic/data operand and therefore does not replace FDP. */
	FPU_INSTPTR = inst;
	FPU_DATAPTR = data;
	FPU_LASTINSTOP = lastop;
}
static void FPU_FXRSTOR(UINT32 addr) {
	UINT start;
	UINT i;
	FPU_PTR inst;
	FPU_PTR data;
	UINT16 lastop;

	FPU_SetCW(FPU_EnvRead16(addr + 0));
	FPU_STATUSWORD = FPU_EnvRead16(addr + 2);
	FPU_SetTag8(FPU_EnvRead8(addr + 4));
	FPU_STAT_TOP = FP_TOP_GET();
	lastop = (UINT16)(FPU_EnvRead16(addr + 6) & 0x07ff);
	inst.offset = FPU_EnvRead32(addr + 8);
	inst.seg = FPU_EnvRead16(addr + 12);
	inst.pad = 0;
	data.offset = FPU_EnvRead32(addr + 16);
	data.seg = FPU_EnvRead16(addr + 20);
	data.pad = 0;
#ifdef USE_SSE
	SSE_MXCSR = FPU_EnvRead32(addr + 24);
#endif
	start = 32;
	for (i = 0; i < 8; i++) {
		FPU_FLD80(addr + start, FPU_ST(i));
		start += 16;
	}
#ifdef USE_SSE
	start = 160;
	for (i = 0; i < 8; i++) {
		SSE_XMMREG(i).ul64[0] = FPU_EnvRead64(addr + start + 0);
		SSE_XMMREG(i).ul64[1] = FPU_EnvRead64(addr + start + 8);
		start += 16;
	}
#endif
	FPU_INSTPTR = inst;
	FPU_DATAPTR = data;
	FPU_LASTINSTOP = lastop;
}
void SF_FPU_FXSAVERSTOR(void) {
	UINT32 op;
	UINT idx, sub;
	UINT32 maddr;

	CPU_WORKCLOCK(FPU_WORKCLOCK);
	GET_MODRM_PCBYTE((op));
	idx = (op >> 3) & 7;
	sub = (op & 7);

	switch (idx) {
	case 0: // FXSAVE
		fpu_check_NM_EXCEPTION2(); // XXX: 根拠無し
		maddr = calc_ea_dst(op);
		FPU_FXSAVE(maddr);
		break;
	case 1: // FXRSTOR
		fpu_check_NM_EXCEPTION2(); // XXX: 根拠無し
		maddr = calc_ea_dst(op);
		FPU_FXRSTOR(maddr);
		break;
#ifdef USE_SSE
	case 2: // LDMXCSR
		maddr = calc_ea_dst(op);
		SSE_LDMXCSR(maddr);
		break;
	case 3: // STMXCSR
		maddr = calc_ea_dst(op);
		SSE_STMXCSR(maddr);
		break;
	case 4: // SFENCE
		SSE_SFENCE();
		break;
	case 5: // LFENCE
		SSE_LFENCE();
		break;
	case 6: // MFENCE
		SSE_MFENCE();
		break;
	case 7: // CLFLUSH;
		SSE_CLFLUSH(op);
		break;
#endif
	default:
		ia32_panic("invalid opcode = %02x\n", op);
		break;
	}
}

static void EA_TREE(UINT op)
{
	UINT idx;

	idx = (op >> 3) & 7;
	
		switch (idx) {
		case 0:	/* FADD (単精度実数) */
			TRACEOUT(("FADD EA"));
			FPU_STATUSWORD &= ~FP_C1_FLAG;
			FPU_FADD_EA(FPU_STAT_TOP); 
			break;
		case 1:	/* FMUL (単精度実数) */
			TRACEOUT(("FMUL EA"));
			FPU_STATUSWORD &= ~FP_C1_FLAG;
			FPU_FMUL_EA(FPU_STAT_TOP);
			break;
		case 2:	/* FCOM (単精度実数) */
			TRACEOUT(("FCOM EA"));
			FPU_STATUSWORD &= ~FP_C1_FLAG;
			FPU_FCOM_EA(FPU_STAT_TOP);
			break;
		case 3:	/* FCOMP (単精度実数) */
			TRACEOUT(("FCOMP EA"));
			FPU_FCOM_EA(FPU_STAT_TOP);
			FPU_pop();
			break;
		case 4:	/* FSUB (単精度実数) */
			TRACEOUT(("FSUB EA"));
			FPU_STATUSWORD &= ~FP_C1_FLAG;
			FPU_FSUB_EA(FPU_STAT_TOP);
			break;
		case 5:	/* FSUBR (単精度実数) */
			TRACEOUT(("FSUBR EA"));
			FPU_STATUSWORD &= ~FP_C1_FLAG;
			FPU_FSUBR_EA(FPU_STAT_TOP);
			break;
		case 6:	/* FDIV (単精度実数) */
			TRACEOUT(("FDIV EA"));
			FPU_STATUSWORD &= ~FP_C1_FLAG;
			FPU_FDIV_EA(FPU_STAT_TOP);
			CPU_WORKCLOCK(30);
			break;
		case 7:	/* FDIVR (単精度実数) */
			TRACEOUT(("FDIVR EA"));
			FPU_STATUSWORD &= ~FP_C1_FLAG;
			FPU_FDIVR_EA(FPU_STAT_TOP);
			CPU_WORKCLOCK(30);
			break;
		default:
			break;
		}	
}

// d8
void
SF_ESC0(void)
{
	UINT32 op, madr;
	UINT idx, sub;

	CPU_WORKCLOCK(FPU_WORKCLOCK);
	GET_MODRM_PCBYTE(op);
	TRACEOUT(("use FPU d8 %.2x", op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	
	fpu_check_NM_EXCEPTION();
	fpu_checkexception();
	FPU_RecordInstructionIfNeeded(0, op);
	if (op >= 0xc0) {
		/* Fxxx ST(0), ST(i) */
		switch (idx) {
		case 0:	/* FADD */
			TRACEOUT(("FADD"));
			FPU_STATUSWORD &= ~FP_C1_FLAG;
			FPU_FADD(FPU_STAT_TOP,FPU_ST(sub));
			break;
		case 1:	/* FMUL */
			TRACEOUT(("FMUL"));
			FPU_STATUSWORD &= ~FP_C1_FLAG;
			FPU_FMUL(FPU_STAT_TOP,FPU_ST(sub));
			break;
		case 2:	/* FCOM */
			TRACEOUT(("FCOM"));
			FPU_STATUSWORD &= ~FP_C1_FLAG;
			FPU_FCOM(FPU_STAT_TOP,FPU_ST(sub));
			break;
		case 3:	/* FCOMP */
			TRACEOUT(("FCOMP"));
			FPU_STATUSWORD &= ~FP_C1_FLAG;
			FPU_FCOM(FPU_STAT_TOP,FPU_ST(sub));
			FPU_pop();
			break;
		case 4:	/* FSUB */
			TRACEOUT(("FSUB"));
			FPU_STATUSWORD &= ~FP_C1_FLAG;
			FPU_FSUB(FPU_STAT_TOP,FPU_ST(sub));
			break;
		case 5:	/* FSUBR */
			TRACEOUT(("FSUBR"));
			FPU_STATUSWORD &= ~FP_C1_FLAG;
			FPU_FSUBR(FPU_STAT_TOP,FPU_ST(sub));
			break;
		case 6:	/* FDIV */
			TRACEOUT(("FDIV"));
			FPU_STATUSWORD &= ~FP_C1_FLAG;
			FPU_FDIV(FPU_STAT_TOP,FPU_ST(sub));
			CPU_WORKCLOCK(30);
			break;
		case 7:	/* FDIVR */
			TRACEOUT(("FDIVR"));
			FPU_STATUSWORD &= ~FP_C1_FLAG;
			FPU_FDIVR(FPU_STAT_TOP,FPU_ST(sub));
			CPU_WORKCLOCK(30);
			break;
		}
	} else {
		madr = calc_ea_dst(op);
		FPU_FLD_F32_EA(madr);
		EA_TREE(op);
	}
}

// d9
void
SF_ESC1(void)
{
	UINT32 op, madr;
	UINT idx, sub;

	CPU_WORKCLOCK(FPU_WORKCLOCK);
	GET_MODRM_PCBYTE(op);
	TRACEOUT(("use FPU d9 %.2x", op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	
	fpu_check_NM_EXCEPTION();
	if(!(op < 0xc0 && idx>=4)){
		fpu_checkexception();
	}
	FPU_RecordInstructionIfNeeded(1, op);
	if (op >= 0xc0) 
	{
		switch (idx) {
		case 0: /* FLD ST(0), ST(i) */
			{
				UINT reg_from;
			
				TRACEOUT(("FLD STi"));
				reg_from = FPU_ST(sub);
				FPU_prepush();
				FPU_FST(reg_from, FPU_STAT_TOP);
			}
			break;

		case 1:	/* FXCH ST(0), ST(i) */
			TRACEOUT(("FXCH STi"));
			FPU_FXCH(FPU_STAT_TOP,FPU_ST(sub));
			break;

		case 2: /* FNOP */
			TRACEOUT(("FNOP"));
			break;

		case 3: /* FSTP STi */
			TRACEOUT(("FSTP STi"));
			FPU_FST(FPU_STAT_TOP,FPU_ST(sub));
			FPU_pop();
			break;
		
		case 4:
			switch (sub) {
			case 0x0:	/* FCHS */
				TRACEOUT(("FCHS"));
				FPU_STATUSWORD &= ~FP_C1_FLAG;
				FPU_FCHS();
				break;

			case 0x1:	/* FABS */
				TRACEOUT(("FABS"));
				FPU_STATUSWORD &= ~FP_C1_FLAG;
				FPU_FABS();
				break;

			case 0x2:  /* UNKNOWN */
			case 0x3:  /* ILLEGAL */
				break;

			case 0x4:	/* FTST */
				TRACEOUT(("FTST"));
				FPU_STATUSWORD &= ~FP_C1_FLAG;
				FPU_FTST();
				break;

			case 0x5:	/* FXAM */
				TRACEOUT(("FXAM"));
				FPU_FXAM();
				break;

			case 0x06:       /* FTSTP (cyrix)*/
			case 0x07:       /* UNKNOWN */
				break;
			}
			break;
			
		case 5:
			switch (sub) {
			case 0x0:	/* FLD1 */
				TRACEOUT(("FLD1"));
				FPU_FLD1();
				break;
				
			case 0x1:	/* FLDL2T */
				TRACEOUT(("FLDL2T"));
				FPU_FLDL2T();
				break;
				
			case 0x2:	/* FLDL2E */
				TRACEOUT(("FLDL2E"));
				FPU_FLDL2E();
				break;
				
			case 0x3:	/* FLDPI */
				TRACEOUT(("FLDPI"));
				FPU_FLDPI();
				break;
				
			case 0x4:	/* FLDLG2 */
				TRACEOUT(("FLDLG2"));
				FPU_FLDLG2();
				break;
				
			case 0x5:	/* FLDLN2 */
				TRACEOUT(("FLDLN2"));
				FPU_FLDLN2();
				break;
				
			case 0x6:	/* FLDZ */
				TRACEOUT(("FLDZ"));
				FPU_FLDZ();
				break;
				
			case 0x07: /* ILLEGAL */
				break;
			}
			break;

		case 6:
			switch (sub) {
			case 0x0:	/* F2XM1 */
				TRACEOUT(("F2XM1"));
				FPU_STATUSWORD &= ~FP_C1_FLAG;
				FPU_F2XM1();
				break;
				
			case 0x1:	/* FYL2X */
				TRACEOUT(("FYL2X"));
				FPU_STATUSWORD &= ~FP_C1_FLAG;
				FPU_FYL2X();
				break;
				
			case 0x2:	/* FPTAN */
				TRACEOUT(("FPTAN"));
				FPU_STATUSWORD &= ~FP_C1_FLAG;
				FPU_FPTAN();
				break;
				
			case 0x3:	/* FPATAN */
				TRACEOUT(("FPATAN"));
				FPU_STATUSWORD &= ~FP_C1_FLAG;
				FPU_FPATAN();
				break;
				
			case 0x4:	/* FXTRACT */
				TRACEOUT(("FXTRACT"));
				FPU_STATUSWORD &= ~FP_C1_FLAG;
				FPU_FXTRACT();
				break;
				
			case 0x5:	/* FPREM1 */
				TRACEOUT(("FPREM1"));
				FPU_FPREM1();
				break;
				
			case 0x6:	/* FDECSTP */
				TRACEOUT(("FDECSTP"));
				FPU_STATUSWORD &= ~FP_C1_FLAG;
				FPU_STAT_TOP = (FPU_STAT_TOP - 1) & 7;
				break;
				
			case 0x7:	/* FINCSTP */
				TRACEOUT(("FINCSTP"));
				FPU_STATUSWORD &= ~FP_C1_FLAG;
				FPU_STAT_TOP = (FPU_STAT_TOP + 1) & 7;
				break;
			}
			break;
			
		case 7:
			switch (sub) {
			case 0x0:	/* FPREM */
				TRACEOUT(("FPREM"));
				FPU_FPREM();
				break;
				
			case 0x1:	/* FYL2XP1 */
				TRACEOUT(("FYL2XP1"));
				FPU_FYL2XP1();
				break;
				
			case 0x2:	/* FSQRT */
				TRACEOUT(("FSQRT"));
				FPU_STATUSWORD &= ~FP_C1_FLAG;
				FPU_FSQRT();
				break;
				
			case 0x3:	/* FSINCOS */
				TRACEOUT(("FSINCOS"));
				FPU_STATUSWORD &= ~FP_C1_FLAG;
				FPU_FSINCOS();
				break;
				
			case 0x4:	/* FRNDINT */
				TRACEOUT(("FRNDINT"));
				FPU_STATUSWORD &= ~FP_C1_FLAG;
				FPU_FRNDINT();
				break;
				
			case 0x5:	/* FSCALE */
				TRACEOUT(("FSCALE"));
				FPU_STATUSWORD &= ~FP_C1_FLAG;
				FPU_FSCALE();
				break;
				
			case 0x6:	/* FSIN */
				TRACEOUT(("FSIN"));
				FPU_STATUSWORD &= ~FP_C1_FLAG;
				FPU_FSIN();				
				break;
				
			case 0x7:	/* FCOS */
				TRACEOUT(("FCOS"));
				FPU_STATUSWORD &= ~FP_C1_FLAG;
				FPU_FCOS();	
				break;
			}
			CPU_WORKCLOCK(50);
			break;

		default:
			ia32_panic("ESC1: invalid opcode = %02x\n", op);
			break;
		}
	} else {
		madr = calc_ea_dst(op);
		switch (idx) {
		case 0:	/* FLD (単精度実数) */
			TRACEOUT(("FLD float"));
			FPU_prepush();
			FPU_FLD_F32(madr,FPU_STAT_TOP);
			break;
			
		case 1:	/* UNKNOWN */
			break;

		case 2:	/* FST (単精度実数) */
			TRACEOUT(("FST float"));
			FPU_FST_F32(madr);
			FPU_STATUSWORD &= ~FP_C1_FLAG;
			break;

		case 3:	/* FSTP (単精度実数) */
			TRACEOUT(("FSTP float"));
			FPU_FST_F32(madr);
			FPU_pop();
			break;

		case 4:	/* FLDENV */
			TRACEOUT(("FLDENV"));
			FPU_FLDENV(madr);		
			break;

		case 5:	/* FLDCW */
			TRACEOUT(("FLDCW"));
			FPU_FLDCW(madr);
			break;

		case 6:	/* FSTENV */
			TRACEOUT(("FSTENV"));
			FPU_FSTENV(madr);
			break;

		case 7:	/* FSTCW */
			TRACEOUT(("FSTCW/FNSTCW"));
			FPU_EnvWrite16(madr, FPU_CTRLWORD);
			break;

		default:
			break;
		}
	}
}

// da
void
SF_ESC2(void)
{
	UINT32 op, madr;
	UINT idx, sub;

	CPU_WORKCLOCK(FPU_WORKCLOCK);
	GET_MODRM_PCBYTE(op);
	TRACEOUT(("use FPU da %.2x", op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	
	fpu_check_NM_EXCEPTION();
	fpu_checkexception();
	FPU_RecordInstructionIfNeeded(2, op);
	if (op >= 0xc0) {
		/* Fxxx ST(0), ST(i) */
		switch (idx) {
		case 0: /* FCMOVB */
			TRACEOUT(("ESC2: FCMOVB"));
			FPU_STATUSWORD &= ~FP_C1_FLAG;
			FPU_FCMOVB(FPU_STAT_TOP,FPU_ST(sub));
			break;
		case 1: /* FCMOVE */
			TRACEOUT(("ESC2: FCMOVE"));
			FPU_STATUSWORD &= ~FP_C1_FLAG;
			FPU_FCMOVE(FPU_STAT_TOP,FPU_ST(sub));
			break;
		case 2: /* FCMOVBE */
			TRACEOUT(("ESC2: FCMOVBE"));
			FPU_STATUSWORD &= ~FP_C1_FLAG;
			FPU_FCMOVBE(FPU_STAT_TOP,FPU_ST(sub));
			break;
		case 3: /* FCMOVU */
			TRACEOUT(("ESC2: FCMOVU"));
			FPU_STATUSWORD &= ~FP_C1_FLAG;
			FPU_FCMOVU(FPU_STAT_TOP,FPU_ST(sub));
			break;
		case 5:
			switch (sub) {
			case 1: /* FUCOMPP */
				TRACEOUT(("FUCOMPP"));
				FPU_FUCOM(FPU_STAT_TOP,FPU_ST(1));
				FPU_pop();
				FPU_pop();
				break;
				
			default:
				break;
			}
			break;
			
		default:
			break;
		}
	} else {
		madr = calc_ea_dst(op);
		FPU_FLD_I32_EA(madr);
		EA_TREE(op);
	}
}

// db
void
SF_ESC3(void)
{
	UINT32 op, madr;
	UINT idx, sub;

	CPU_WORKCLOCK(FPU_WORKCLOCK);
	GET_MODRM_PCBYTE(op);
	TRACEOUT(("use FPU db %.2x", op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	
	fpu_check_NM_EXCEPTION();
	if(!(op >= 0xc0 && idx==4)){
		fpu_checkexception();
	}
	FPU_RecordInstructionIfNeeded(3, op);
	if (op >= 0xc0) 
	{
		/* Fxxx ST(0), ST(i) */
		switch (idx) {
		case 0: /* FCMOVNB */
			TRACEOUT(("ESC3: FCMOVNB"));
			FPU_STATUSWORD &= ~FP_C1_FLAG;
			FPU_FCMOVNB(FPU_STAT_TOP,FPU_ST(sub));
			break;
		case 1: /* FCMOVNE */
			TRACEOUT(("ESC3: FCMOVNE"));
			FPU_STATUSWORD &= ~FP_C1_FLAG;
			FPU_FCMOVNE(FPU_STAT_TOP,FPU_ST(sub));
			break;
		case 2: /* FCMOVNBE */
			TRACEOUT(("ESC3: FCMOVNBE"));
			FPU_STATUSWORD &= ~FP_C1_FLAG;
			FPU_FCMOVNBE(FPU_STAT_TOP,FPU_ST(sub));
			break;
		case 3: /* FCMOVNU */
			TRACEOUT(("ESC3: FCMOVNU"));
			FPU_STATUSWORD &= ~FP_C1_FLAG;
			FPU_FCMOVNU(FPU_STAT_TOP,FPU_ST(sub));
			break;
		case 4:
			switch (sub) {
			case 0: /* FNENI */
			case 1: /* FNDIS */
				break;
				
			case 2: /* FCLEX */
				TRACEOUT(("FCLEX"));
				FPU_STATUSWORD &= 0x7f00;
				break;
				
			case 3: /* FNINIT/FINIT */
				TRACEOUT(("FNINIT/FINIT"));
				FPU_FINIT();
				break;
				
			case 4: /* FNSETPM */
			case 5: /* FRSTPM */
				// nop
				break;
				
			default:
				break;
			}
			break;
		case 5: /* FUCOMI */
			TRACEOUT(("ESC3: FUCOMI"));
			FPU_STATUSWORD &= ~FP_C1_FLAG;
			FPU_FUCOMI(FPU_STAT_TOP,FPU_ST(sub));
			break;
		case 6: /* FCOMI */
			TRACEOUT(("ESC3: FCOMI"));
			FPU_STATUSWORD &= ~FP_C1_FLAG;
			FPU_FCOMI(FPU_STAT_TOP,FPU_ST(sub));	
			break;
		default:
			break;
		}
	} else {
		madr = calc_ea_dst(op);
		switch (idx) {
		case 0:	/* FILD (DWORD) */
			TRACEOUT(("FILD"));
			FPU_prepush();
			FPU_FLD_I32(madr,FPU_STAT_TOP);
			break;
			
		case 1:	/* FISTTP (DWORD) */
			{
				uint_fast8_t oldrnd = softfloat_roundingMode;
				softfloat_roundingMode = softfloat_round_min;
				FPU_FST_I32(madr);
				softfloat_roundingMode = oldrnd;
			}
			FPU_pop();
			break;
			
		case 2:	/* FIST (DWORD) */
			TRACEOUT(("FIST"));
			FPU_FST_I32(madr);
			FPU_STATUSWORD &= ~FP_C1_FLAG;
			break;
			
		case 3:	/* FISTP (DWORD) */
			TRACEOUT(("FISTP"));
			FPU_FST_I32(madr);
			FPU_pop();
			break;
			
		case 5:	/* FLD (拡張実数) */
			TRACEOUT(("FLD 80 Bits Real"));
			FPU_prepush();
			FPU_FLD_F80(madr);
			break;
			
		case 7:	/* FSTP (拡張実数) */
			TRACEOUT(("FSTP 80 Bits Real"));
			FPU_FST_F80(madr);
			FPU_pop();
			break;

		default:
			break;
		}
	}
}

// dc
void
SF_ESC4(void)
{
	UINT32 op, madr;
	UINT idx, sub;

	CPU_WORKCLOCK(FPU_WORKCLOCK);
	GET_MODRM_PCBYTE(op);
	TRACEOUT(("use FPU dc %.2x", op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	
	fpu_check_NM_EXCEPTION();
	fpu_checkexception();
	FPU_RecordInstructionIfNeeded(4, op);
	if (op >= 0xc0) {
		/* Fxxx ST(i), ST(0) */
		switch (idx) {
		case 0:	/* FADD */
			TRACEOUT(("ESC4: FADD"));
			FPU_STATUSWORD &= ~FP_C1_FLAG;
			FPU_FADD(FPU_ST(sub),FPU_STAT_TOP);
			break;
		case 1:	/* FMUL */
			TRACEOUT(("ESC4: FMUL"));
			FPU_STATUSWORD &= ~FP_C1_FLAG;
			FPU_FMUL(FPU_ST(sub),FPU_STAT_TOP);
			break;
		case 2: /* FCOM */
			TRACEOUT(("ESC4: FCOM"));
			FPU_STATUSWORD &= ~FP_C1_FLAG;
			FPU_FCOM(FPU_STAT_TOP,FPU_ST(sub));			
			break;
		case 3: /* FCOMP */
			TRACEOUT(("ESC4: FCOMP"));
			FPU_FCOM(FPU_STAT_TOP,FPU_ST(sub));	
			FPU_pop();
			break;
		case 4:	/* FSUBR */
			TRACEOUT(("ESC4: FSUBR"));
			FPU_STATUSWORD &= ~FP_C1_FLAG;
			FPU_FSUBR(FPU_ST(sub),FPU_STAT_TOP);
			break;
		case 5:	/* FSUB */
			TRACEOUT(("ESC4: FSUB"));
			FPU_STATUSWORD &= ~FP_C1_FLAG;
			FPU_FSUB(FPU_ST(sub),FPU_STAT_TOP);
			break;
		case 6:	/* FDIVR */
			TRACEOUT(("ESC4: FDIVR"));
			FPU_STATUSWORD &= ~FP_C1_FLAG;
			FPU_FDIVR(FPU_ST(sub),FPU_STAT_TOP);
			break;
		case 7:	/* FDIV */
			TRACEOUT(("ESC4: FDIV"));
			FPU_STATUSWORD &= ~FP_C1_FLAG;
			FPU_FDIV(FPU_ST(sub),FPU_STAT_TOP);
			break;
		default:
			break;
		}
	} else {
		madr = calc_ea_dst(op);
		FPU_FLD_F64_EA(madr);
		EA_TREE(op);
	}
}

// dd
void
SF_ESC5(void)
{
	UINT32 op, madr;
	UINT idx, sub;

	CPU_WORKCLOCK(FPU_WORKCLOCK);
	GET_MODRM_PCBYTE(op);
	TRACEOUT(("use FPU dd %.2x", op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	fpu_check_NM_EXCEPTION();
	if(op >= 0xc0 || (idx!=4 && idx!=6 && idx!=7)){
		fpu_checkexception();
	}
	FPU_RecordInstructionIfNeeded(5, op);
	if (op >= 0xc0) {
		/* FUCOM ST(i), ST(0) */
		/* Fxxx ST(i) */
		switch (idx) {
		case 0:	/* FFREE */
			TRACEOUT(("FFREE"));
			FPU_STAT.tag[FPU_ST(sub)]=TAG_Empty;
			FPU_STAT.mmxenable = 0;
			break;
		case 1: /* FXCH */
			TRACEOUT(("FXCH"));
			FPU_FXCH(FPU_STAT_TOP,FPU_ST(sub));
			break;
		case 2:	/* FST */
			TRACEOUT(("FST"));
			FPU_FST(FPU_STAT_TOP,FPU_ST(sub));
			FPU_STATUSWORD &= ~FP_C1_FLAG;
			break;
		case 3:	/* FSTP */
			TRACEOUT(("FSTP"));
			FPU_FST(FPU_STAT_TOP,FPU_ST(sub));
			FPU_pop();
			break;
		case 4:	/* FUCOM */
			TRACEOUT(("FUCOM"));
			FPU_STATUSWORD &= ~FP_C1_FLAG;
			FPU_FUCOM(FPU_STAT_TOP,FPU_ST(sub));
			break;
		case 5:	/* FUCOMP */
			TRACEOUT(("FUCOMP"));
			FPU_FUCOM(FPU_STAT_TOP,FPU_ST(sub));
			FPU_pop();
			break;

		default:
			break;
		}
	} else {
		madr = calc_ea_dst(op);
		switch (idx) {
		case 0:	/* FLD (倍精度実数) */
			TRACEOUT(("FLD double real"));
			FPU_prepush();
			FPU_FLD_F64(madr,FPU_STAT_TOP);
			break;
		case 1:	/* FISTTP (QWORD) */
			{
				uint_fast8_t oldrnd = softfloat_roundingMode;
				softfloat_roundingMode = softfloat_round_min;
				FPU_FST_I64(madr);
				softfloat_roundingMode = oldrnd;
			}
			FPU_pop();
			break;
		case 2:	/* FST (倍精度実数) */
			TRACEOUT(("FST double real"));
			FPU_FST_F64(madr);
			FPU_STATUSWORD &= ~FP_C1_FLAG;
			break;
		case 3:	/* FSTP (倍精度実数) */
			TRACEOUT(("FSTP double real"));
			FPU_FST_F64(madr);
			FPU_pop();
			break;
		case 4:	/* FRSTOR */
			TRACEOUT(("FRSTOR"));
			FPU_FRSTOR(madr);
			break;
		case 6:	/* FSAVE */
			TRACEOUT(("FSAVE"));
			FPU_FSAVE(madr);
			break;

		case 7:	/* FSTSW */
			FP_TOP_SET(FPU_STAT_TOP);
			cpu_vmemorywrite_w(CPU_INST_SEGREG_INDEX, madr, FPU_STATUSWORD);
			break;

		default:
			break;
		}
	}
}

// de
void
SF_ESC6(void)
{
	UINT32 op, madr;
	UINT idx, sub;

	CPU_WORKCLOCK(FPU_WORKCLOCK);
	GET_MODRM_PCBYTE(op);
	TRACEOUT(("use FPU de %.2x", op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	
	fpu_check_NM_EXCEPTION();
	fpu_checkexception();
	FPU_RecordInstructionIfNeeded(6, op);
	if (op >= 0xc0) {
		/* Fxxx ST(i), ST(0) */
		switch (idx) {
		case 0:	/* FADDP */
			TRACEOUT(("FADDP"));
			FPU_FADD(FPU_ST(sub),FPU_STAT_TOP);
			break;
		case 1:	/* FMULP */
			TRACEOUT(("FMULP"));
			FPU_FMUL(FPU_ST(sub),FPU_STAT_TOP);
			break;
		case 2:	/* FCOMP5 */
			TRACEOUT(("FCOMP5"));
			FPU_FCOM(FPU_STAT_TOP,FPU_ST(sub));
			break;
		case 3: /* FCOMPP */
			TRACEOUT(("FCOMPP"));
			if(sub != 1) {
				return;
			}
			FPU_FCOM(FPU_STAT_TOP,FPU_ST(1));
			FPU_pop(); // 下コードと合わせて2回pop
			break;			
		case 4:	/* FSUBRP */
			TRACEOUT(("FSUBRP"));
			FPU_FSUBR(FPU_ST(sub),FPU_STAT_TOP);
			break;
		case 5:	/* FSUBP */
			TRACEOUT(("FSUBP"));
			FPU_FSUB(FPU_ST(sub),FPU_STAT_TOP);
			break;
		case 6:	/* FDIVRP */
			TRACEOUT(("FDIVRP"));
			FPU_FDIVR(FPU_ST(sub),FPU_STAT_TOP);
			if((FPU_STATUSWORD & ~FPU_CTRLWORD) & FP_ZE_FLAG){
				return; // POPしないようにする
			}
			break;
		case 7:	/* FDIVP */
			TRACEOUT(("FDIVP"));
			FPU_FDIV(FPU_ST(sub),FPU_STAT_TOP);
			if((FPU_STATUSWORD & ~FPU_CTRLWORD) & FP_ZE_FLAG){
				return; // POPしないようにする
			}
			break;
			/*FALLTHROUGH*/
		default:
			break;
		}
		FPU_pop();
	} else {
		madr = calc_ea_dst(op);
		FPU_FLD_I16_EA(madr);
		EA_TREE(op);
	}
}

// df
void
SF_ESC7(void)
{
	UINT32 op, madr;
	UINT idx, sub;

	CPU_WORKCLOCK(FPU_WORKCLOCK);
	GET_MODRM_PCBYTE(op);
	TRACEOUT(("use FPU df %.2x", op));
	idx = (op >> 3) & 7;
	sub = (op & 7);
	
	fpu_check_NM_EXCEPTION();
	if(!(op >= 0xc0 && idx==4 && sub==0)){
		fpu_checkexception();
	}
	FPU_RecordInstructionIfNeeded(7, op);
	if (op >= 0xc0) {
		/* Fxxx ST(0), ST(i) */
		switch (idx) {
		case 0: /* FFREEP */
			TRACEOUT(("FFREEP"));
			FPU_STAT.tag[FPU_ST(sub)]=TAG_Empty;
			FPU_STAT.mmxenable = 0;
			FPU_pop();
			break;
		case 1: /* FXCH */
			TRACEOUT(("FXCH"));
			FPU_STATUSWORD &= ~FP_C1_FLAG;
			FPU_FXCH(FPU_STAT_TOP,FPU_ST(sub));
			break;
		
		case 2:
		case 3: /* FSTP */
			TRACEOUT(("FSTP"));
			FPU_FST(FPU_STAT_TOP,FPU_ST(sub));
			FPU_pop();
			break;

		case 4:
			switch (sub)
			{
			case 0: /* FSTSW AX */
				TRACEOUT(("FSTSW AX"));
				FP_TOP_SET(FPU_STAT_TOP);
				CPU_AX = FPU_STATUSWORD;
				break;
				
			default:
				break;
			}
			break;
		case 5: /* FUCOMIP */
			TRACEOUT(("ESC7: FUCOMIP"));
			FPU_FUCOMI(FPU_STAT_TOP,FPU_ST(sub));	
			FPU_pop();
			break;
		case 6: /* FCOMIP */
			TRACEOUT(("ESC7: FCOMIP"));
			FPU_FCOMI(FPU_STAT_TOP,FPU_ST(sub));	
			FPU_pop();
			break;
		default:
			break;
		}
	} else {
		madr = calc_ea_dst(op);
		switch (idx) {
		case 0:	/* FILD (WORD) */
			TRACEOUT(("FILD SINT16"));
			FPU_prepush();
			FPU_FLD_I16(madr,FPU_STAT_TOP);
			break;
		case 1:	/* FISTTP (WORD) */
			{
				uint_fast8_t oldrnd = softfloat_roundingMode;
				softfloat_roundingMode = softfloat_round_min;
				FPU_FST_I16(madr);
				softfloat_roundingMode = oldrnd;
			}
			FPU_pop();
			break;
		case 2:	/* FIST (WORD) */
			TRACEOUT(("FIST SINT16"));
			FPU_FST_I16(madr);
			FPU_STATUSWORD &= ~FP_C1_FLAG;
			break;
		case 3:	/* FISTP (WORD) */
			TRACEOUT(("FISTP SINT16"));
			FPU_FST_I16(madr);
			FPU_pop();
			break;

		case 4:	/* FBLD (BCD) */
			TRACEOUT(("FBLD packed BCD"));
			FPU_prepush();
			FPU_FBLD(madr,FPU_STAT_TOP);
			break;

		case 5:	/* FILD (QWORD) */
			TRACEOUT(("FILD SINT64"));
			FPU_prepush();
			FPU_FLD_I64(madr,FPU_STAT_TOP);
			break;

		case 6:	/* FBSTP (BCD) */
			TRACEOUT(("FBSTP packed BCD"));
			FPU_FBST(madr);
			FPU_pop();
			break;

		case 7:	/* FISTP (QWORD) */
			TRACEOUT(("FISTP SINT64"));
			FPU_FST_I64(madr);
			FPU_pop();
			break;

		default:
			break;
		}
	}
}
#endif
