
/*============================================================================

This C header file is part of the SoftFloat IEEE Floating-Point Arithmetic
Package, Release 3e, by John R. Hauser.

Copyright 2011, 2012, 2013, 2014, 2015, 2016, 2017 The Regents of the
University of California.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice,
    this list of conditions, and the following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions, and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

 3. Neither the name of the University nor the names of its contributors may
    be used to endorse or promote products derived from this software without
    specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS "AS IS", AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, ARE
DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=============================================================================*/


/*============================================================================
| Note:  If SoftFloat is made available as a general library for programs to
| use, it is strongly recommended that a platform-specific version of this
| header, "softfloat.h", be created that folds in "softfloat_types.h" and that
| eliminates all dependencies on compile-time macros.
*============================================================================*/


#ifndef softfloat_h
#define softfloat_h 1

#include <stdbool.h>
#include <stdint.h>
#include "softfloat_types.h"

#ifndef THREAD_LOCAL
#define THREAD_LOCAL
#endif

/*----------------------------------------------------------------------------
| Software floating-point underflow tininess-detection mode.
*----------------------------------------------------------------------------*/
extern THREAD_LOCAL uint_fast8_t softfloat_detectTininess;
enum {
    softfloat_tininess_beforeRounding = 0,
    softfloat_tininess_afterRounding  = 1
};

/*----------------------------------------------------------------------------
| Software floating-point rounding mode.  (Mode "odd" is supported only if
| SoftFloat is compiled with macro 'SOFTFLOAT_ROUND_ODD' defined.)
*----------------------------------------------------------------------------*/
extern THREAD_LOCAL uint_fast8_t softfloat_roundingMode;
enum {
    softfloat_round_near_even   = 0,
    softfloat_round_minMag      = 1,
    softfloat_round_min         = 2,
    softfloat_round_max         = 3,
    softfloat_round_near_maxMag = 4,
    softfloat_round_odd         = 6
};

/*----------------------------------------------------------------------------
| Software floating-point exception flags.
*----------------------------------------------------------------------------*/
extern THREAD_LOCAL uint_fast8_t softfloat_exceptionFlags;
enum {
    softfloat_flag_inexact   =  1,
    softfloat_flag_underflow =  2,
    softfloat_flag_overflow  =  4,
    softfloat_flag_infinite  =  8,
    softfloat_flag_invalid   = 16
};

/*----------------------------------------------------------------------------
| Routine to raise any or all of the software floating-point exception flags.
*----------------------------------------------------------------------------*/
void softfloat_raiseFlags( uint_fast8_t );

/*----------------------------------------------------------------------------
| Integer-to-floating-point conversion routines.
*----------------------------------------------------------------------------*/
sw_float16_t ui32_to_f16( uint32_t );
sw_float32_t ui32_to_f32( uint32_t );
sw_float64_t ui32_to_f64( uint32_t );
#ifdef SOFTFLOAT_FAST_INT64
sw_extFloat80_t ui32_to_extF80( uint32_t );
sw_float128_t ui32_to_f128( uint32_t );
#endif
void ui32_to_extF80M( uint32_t, sw_extFloat80_t * );
void ui32_to_f128M( uint32_t, sw_float128_t * );
sw_float16_t ui64_to_f16( uint64_t );
sw_float32_t ui64_to_f32( uint64_t );
sw_float64_t ui64_to_f64( uint64_t );
#ifdef SOFTFLOAT_FAST_INT64
sw_extFloat80_t ui64_to_extF80( uint64_t );
sw_float128_t ui64_to_f128( uint64_t );
#endif
void ui64_to_extF80M( uint64_t, sw_extFloat80_t * );
void ui64_to_f128M( uint64_t, sw_float128_t * );
sw_float16_t i32_to_f16( int32_t );
sw_float32_t i32_to_f32( int32_t );
sw_float64_t i32_to_f64( int32_t );
#ifdef SOFTFLOAT_FAST_INT64
sw_extFloat80_t i32_to_extF80( int32_t );
sw_float128_t i32_to_f128( int32_t );
#endif
void i32_to_extF80M( int32_t, sw_extFloat80_t * );
void i32_to_f128M( int32_t, sw_float128_t * );
sw_float16_t i64_to_f16( int64_t );
sw_float32_t i64_to_f32( int64_t );
sw_float64_t i64_to_f64( int64_t );
#ifdef SOFTFLOAT_FAST_INT64
sw_extFloat80_t i64_to_extF80( int64_t );
sw_float128_t i64_to_f128( int64_t );
#endif
void i64_to_extF80M( int64_t, sw_extFloat80_t * );
void i64_to_f128M( int64_t, sw_float128_t * );

/*----------------------------------------------------------------------------
| 16-bit (half-precision) floating-point operations.
*----------------------------------------------------------------------------*/
uint_fast32_t f16_to_ui32( sw_float16_t, uint_fast8_t, bool );
uint_fast64_t f16_to_ui64( sw_float16_t, uint_fast8_t, bool );
int_fast32_t f16_to_i32( sw_float16_t, uint_fast8_t, bool );
int_fast64_t f16_to_i64( sw_float16_t, uint_fast8_t, bool );
uint_fast32_t f16_to_ui32_r_minMag( sw_float16_t, bool );
uint_fast64_t f16_to_ui64_r_minMag( sw_float16_t, bool );
int_fast32_t f16_to_i32_r_minMag( sw_float16_t, bool );
int_fast64_t f16_to_i64_r_minMag( sw_float16_t, bool );
sw_float32_t f16_to_f32( sw_float16_t );
sw_float64_t f16_to_f64( sw_float16_t );
#ifdef SOFTFLOAT_FAST_INT64
sw_extFloat80_t f16_to_extF80( sw_float16_t );
sw_float128_t f16_to_f128( sw_float16_t );
#endif
void f16_to_extF80M( sw_float16_t, sw_extFloat80_t * );
void f16_to_f128M( sw_float16_t, sw_float128_t * );
sw_float16_t f16_roundToInt( sw_float16_t, uint_fast8_t, bool );
sw_float16_t f16_add( sw_float16_t, sw_float16_t );
sw_float16_t f16_sub( sw_float16_t, sw_float16_t );
sw_float16_t f16_mul( sw_float16_t, sw_float16_t );
sw_float16_t f16_mulAdd( sw_float16_t, sw_float16_t, sw_float16_t );
sw_float16_t f16_div( sw_float16_t, sw_float16_t );
sw_float16_t f16_rem( sw_float16_t, sw_float16_t );
sw_float16_t f16_sqrt( sw_float16_t );
bool f16_eq( sw_float16_t, sw_float16_t );
bool f16_le( sw_float16_t, sw_float16_t );
bool f16_lt( sw_float16_t, sw_float16_t );
bool f16_eq_signaling( sw_float16_t, sw_float16_t );
bool f16_le_quiet( sw_float16_t, sw_float16_t );
bool f16_lt_quiet( sw_float16_t, sw_float16_t );
bool f16_isSignalingNaN( sw_float16_t );

/*----------------------------------------------------------------------------
| 32-bit (single-precision) floating-point operations.
*----------------------------------------------------------------------------*/
uint_fast32_t f32_to_ui32( sw_float32_t, uint_fast8_t, bool );
uint_fast64_t f32_to_ui64( sw_float32_t, uint_fast8_t, bool );
int_fast32_t f32_to_i32( sw_float32_t, uint_fast8_t, bool );
int_fast64_t f32_to_i64( sw_float32_t, uint_fast8_t, bool );
uint_fast32_t f32_to_ui32_r_minMag( sw_float32_t, bool );
uint_fast64_t f32_to_ui64_r_minMag( sw_float32_t, bool );
int_fast32_t f32_to_i32_r_minMag( sw_float32_t, bool );
int_fast64_t f32_to_i64_r_minMag( sw_float32_t, bool );
sw_float16_t f32_to_f16( sw_float32_t );
sw_float64_t f32_to_f64( sw_float32_t );
#ifdef SOFTFLOAT_FAST_INT64
sw_extFloat80_t f32_to_extF80( sw_float32_t );
sw_float128_t f32_to_f128( sw_float32_t );
#endif
void f32_to_extF80M( sw_float32_t, sw_extFloat80_t * );
void f32_to_f128M( sw_float32_t, sw_float128_t * );
sw_float32_t f32_roundToInt( sw_float32_t, uint_fast8_t, bool );
sw_float32_t f32_add( sw_float32_t, sw_float32_t );
sw_float32_t f32_sub( sw_float32_t, sw_float32_t );
sw_float32_t f32_mul( sw_float32_t, sw_float32_t );
sw_float32_t f32_mulAdd( sw_float32_t, sw_float32_t, sw_float32_t );
sw_float32_t f32_div( sw_float32_t, sw_float32_t );
sw_float32_t f32_rem( sw_float32_t, sw_float32_t );
sw_float32_t f32_sqrt( sw_float32_t );
bool f32_eq( sw_float32_t, sw_float32_t );
bool f32_le( sw_float32_t, sw_float32_t );
bool f32_lt( sw_float32_t, sw_float32_t );
bool f32_eq_signaling( sw_float32_t, sw_float32_t );
bool f32_le_quiet( sw_float32_t, sw_float32_t );
bool f32_lt_quiet( sw_float32_t, sw_float32_t );
bool f32_isSignalingNaN( sw_float32_t );

/*----------------------------------------------------------------------------
| 64-bit (double-precision) floating-point operations.
*----------------------------------------------------------------------------*/
uint_fast32_t f64_to_ui32( sw_float64_t, uint_fast8_t, bool );
uint_fast64_t f64_to_ui64( sw_float64_t, uint_fast8_t, bool );
int_fast32_t f64_to_i32( sw_float64_t, uint_fast8_t, bool );
int_fast64_t f64_to_i64( sw_float64_t, uint_fast8_t, bool );
uint_fast32_t f64_to_ui32_r_minMag( sw_float64_t, bool );
uint_fast64_t f64_to_ui64_r_minMag( sw_float64_t, bool );
int_fast32_t f64_to_i32_r_minMag( sw_float64_t, bool );
int_fast64_t f64_to_i64_r_minMag( sw_float64_t, bool );
sw_float16_t f64_to_f16( sw_float64_t );
sw_float32_t f64_to_f32( sw_float64_t );
#ifdef SOFTFLOAT_FAST_INT64
sw_extFloat80_t f64_to_extF80( sw_float64_t );
sw_float128_t f64_to_f128( sw_float64_t );
#endif
void f64_to_extF80M( sw_float64_t, sw_extFloat80_t * );
void f64_to_f128M( sw_float64_t, sw_float128_t * );
sw_float64_t f64_roundToInt( sw_float64_t, uint_fast8_t, bool );
sw_float64_t f64_add( sw_float64_t, sw_float64_t );
sw_float64_t f64_sub( sw_float64_t, sw_float64_t );
sw_float64_t f64_mul( sw_float64_t, sw_float64_t );
sw_float64_t f64_mulAdd( sw_float64_t, sw_float64_t, sw_float64_t );
sw_float64_t f64_div( sw_float64_t, sw_float64_t );
sw_float64_t f64_rem( sw_float64_t, sw_float64_t );
sw_float64_t f64_sqrt( sw_float64_t );
bool f64_eq( sw_float64_t, sw_float64_t );
bool f64_le( sw_float64_t, sw_float64_t );
bool f64_lt( sw_float64_t, sw_float64_t );
bool f64_eq_signaling( sw_float64_t, sw_float64_t );
bool f64_le_quiet( sw_float64_t, sw_float64_t );
bool f64_lt_quiet( sw_float64_t, sw_float64_t );
bool f64_isSignalingNaN( sw_float64_t );

/*----------------------------------------------------------------------------
| Rounding precision for 80-bit extended double-precision floating-point.
| Valid values are 32, 64, and 80.
*----------------------------------------------------------------------------*/
extern THREAD_LOCAL uint_fast8_t extF80_roundingPrecision;

/*----------------------------------------------------------------------------
| 80-bit extended double-precision floating-point operations.
*----------------------------------------------------------------------------*/
#ifdef SOFTFLOAT_FAST_INT64
uint_fast32_t extF80_to_ui32( sw_extFloat80_t, uint_fast8_t, bool );
uint_fast64_t extF80_to_ui64( sw_extFloat80_t, uint_fast8_t, bool );
int_fast32_t extF80_to_i32( sw_extFloat80_t, uint_fast8_t, bool );
int_fast64_t extF80_to_i64( sw_extFloat80_t, uint_fast8_t, bool );
uint_fast32_t extF80_to_ui32_r_minMag( sw_extFloat80_t, bool );
uint_fast64_t extF80_to_ui64_r_minMag( sw_extFloat80_t, bool );
int_fast32_t extF80_to_i32_r_minMag( sw_extFloat80_t, bool );
int_fast64_t extF80_to_i64_r_minMag( sw_extFloat80_t, bool );
sw_float16_t extF80_to_f16( sw_extFloat80_t );
sw_float32_t extF80_to_f32( sw_extFloat80_t );
sw_float64_t extF80_to_f64( sw_extFloat80_t );
sw_float128_t extF80_to_f128( sw_extFloat80_t );
sw_extFloat80_t extF80_roundToInt( sw_extFloat80_t, uint_fast8_t, bool );
sw_extFloat80_t extF80_add( sw_extFloat80_t, sw_extFloat80_t );
sw_extFloat80_t extF80_sub( sw_extFloat80_t, sw_extFloat80_t );
sw_extFloat80_t extF80_mul( sw_extFloat80_t, sw_extFloat80_t );
sw_extFloat80_t extF80_div( sw_extFloat80_t, sw_extFloat80_t );
sw_extFloat80_t extF80_rem( sw_extFloat80_t, sw_extFloat80_t );
sw_extFloat80_t extF80_sqrt( sw_extFloat80_t );
bool extF80_eq( sw_extFloat80_t, sw_extFloat80_t );
bool extF80_le( sw_extFloat80_t, sw_extFloat80_t );
bool extF80_lt( sw_extFloat80_t, sw_extFloat80_t );
bool extF80_eq_signaling( sw_extFloat80_t, sw_extFloat80_t );
bool extF80_le_quiet( sw_extFloat80_t, sw_extFloat80_t );
bool extF80_lt_quiet( sw_extFloat80_t, sw_extFloat80_t );
bool extF80_isSignalingNaN( sw_extFloat80_t );
#endif
uint_fast32_t extF80M_to_ui32( const sw_extFloat80_t *, uint_fast8_t, bool );
uint_fast64_t extF80M_to_ui64( const sw_extFloat80_t *, uint_fast8_t, bool );
int_fast32_t extF80M_to_i32( const sw_extFloat80_t *, uint_fast8_t, bool );
int_fast64_t extF80M_to_i64( const sw_extFloat80_t *, uint_fast8_t, bool );
uint_fast32_t extF80M_to_ui32_r_minMag( const sw_extFloat80_t *, bool );
uint_fast64_t extF80M_to_ui64_r_minMag( const sw_extFloat80_t *, bool );
int_fast32_t extF80M_to_i32_r_minMag( const sw_extFloat80_t *, bool );
int_fast64_t extF80M_to_i64_r_minMag( const sw_extFloat80_t *, bool );
sw_float16_t extF80M_to_f16( const sw_extFloat80_t * );
sw_float32_t extF80M_to_f32( const sw_extFloat80_t * );
sw_float64_t extF80M_to_f64( const sw_extFloat80_t * );
void extF80M_to_f128M( const sw_extFloat80_t *, sw_float128_t * );
void
 extF80M_roundToInt(
     const sw_extFloat80_t *, uint_fast8_t, bool, sw_extFloat80_t * );
void extF80M_add( const sw_extFloat80_t *, const sw_extFloat80_t *, sw_extFloat80_t * );
void extF80M_sub( const sw_extFloat80_t *, const sw_extFloat80_t *, sw_extFloat80_t * );
void extF80M_mul( const sw_extFloat80_t *, const sw_extFloat80_t *, sw_extFloat80_t * );
void extF80M_div( const sw_extFloat80_t *, const sw_extFloat80_t *, sw_extFloat80_t * );
void extF80M_rem( const sw_extFloat80_t *, const sw_extFloat80_t *, sw_extFloat80_t * );
void extF80M_sqrt( const sw_extFloat80_t *, sw_extFloat80_t * );
bool extF80M_eq( const sw_extFloat80_t *, const sw_extFloat80_t * );
bool extF80M_le( const sw_extFloat80_t *, const sw_extFloat80_t * );
bool extF80M_lt( const sw_extFloat80_t *, const sw_extFloat80_t * );
bool extF80M_eq_signaling( const sw_extFloat80_t *, const sw_extFloat80_t * );
bool extF80M_le_quiet( const sw_extFloat80_t *, const sw_extFloat80_t * );
bool extF80M_lt_quiet( const sw_extFloat80_t *, const sw_extFloat80_t * );
bool extF80M_isSignalingNaN( const sw_extFloat80_t * );

/*----------------------------------------------------------------------------
| 128-bit (quadruple-precision) floating-point operations.
*----------------------------------------------------------------------------*/
#ifdef SOFTFLOAT_FAST_INT64
uint_fast32_t f128_to_ui32( sw_float128_t, uint_fast8_t, bool );
uint_fast64_t f128_to_ui64( sw_float128_t, uint_fast8_t, bool );
int_fast32_t f128_to_i32( sw_float128_t, uint_fast8_t, bool );
int_fast64_t f128_to_i64( sw_float128_t, uint_fast8_t, bool );
uint_fast32_t f128_to_ui32_r_minMag( sw_float128_t, bool );
uint_fast64_t f128_to_ui64_r_minMag( sw_float128_t, bool );
int_fast32_t f128_to_i32_r_minMag( sw_float128_t, bool );
int_fast64_t f128_to_i64_r_minMag( sw_float128_t, bool );
sw_float16_t f128_to_f16( sw_float128_t );
sw_float32_t f128_to_f32( sw_float128_t );
sw_float64_t f128_to_f64( sw_float128_t );
sw_extFloat80_t f128_to_extF80( sw_float128_t );
sw_float128_t f128_roundToInt( sw_float128_t, uint_fast8_t, bool );
sw_float128_t f128_add( sw_float128_t, sw_float128_t );
sw_float128_t f128_sub( sw_float128_t, sw_float128_t );
sw_float128_t f128_mul( sw_float128_t, sw_float128_t );
sw_float128_t f128_mulAdd( sw_float128_t, sw_float128_t, sw_float128_t );
sw_float128_t f128_div( sw_float128_t, sw_float128_t );
sw_float128_t f128_rem( sw_float128_t, sw_float128_t );
sw_float128_t f128_sqrt( sw_float128_t );
bool f128_eq( sw_float128_t, sw_float128_t );
bool f128_le( sw_float128_t, sw_float128_t );
bool f128_lt( sw_float128_t, sw_float128_t );
bool f128_eq_signaling( sw_float128_t, sw_float128_t );
bool f128_le_quiet( sw_float128_t, sw_float128_t );
bool f128_lt_quiet( sw_float128_t, sw_float128_t );
bool f128_isSignalingNaN( sw_float128_t );
#endif
uint_fast32_t f128M_to_ui32( const sw_float128_t *, uint_fast8_t, bool );
uint_fast64_t f128M_to_ui64( const sw_float128_t *, uint_fast8_t, bool );
int_fast32_t f128M_to_i32( const sw_float128_t *, uint_fast8_t, bool );
int_fast64_t f128M_to_i64( const sw_float128_t *, uint_fast8_t, bool );
uint_fast32_t f128M_to_ui32_r_minMag( const sw_float128_t *, bool );
uint_fast64_t f128M_to_ui64_r_minMag( const sw_float128_t *, bool );
int_fast32_t f128M_to_i32_r_minMag( const sw_float128_t *, bool );
int_fast64_t f128M_to_i64_r_minMag( const sw_float128_t *, bool );
sw_float16_t f128M_to_f16( const sw_float128_t * );
sw_float32_t f128M_to_f32( const sw_float128_t * );
sw_float64_t f128M_to_f64( const sw_float128_t * );
void f128M_to_extF80M( const sw_float128_t *, sw_extFloat80_t * );
void f128M_roundToInt( const sw_float128_t *, uint_fast8_t, bool, sw_float128_t * );
void f128M_add( const sw_float128_t *, const sw_float128_t *, sw_float128_t * );
void f128M_sub( const sw_float128_t *, const sw_float128_t *, sw_float128_t * );
void f128M_mul( const sw_float128_t *, const sw_float128_t *, sw_float128_t * );
void
 f128M_mulAdd(
     const sw_float128_t *, const sw_float128_t *, const sw_float128_t *, sw_float128_t *
 );
void f128M_div( const sw_float128_t *, const sw_float128_t *, sw_float128_t * );
void f128M_rem( const sw_float128_t *, const sw_float128_t *, sw_float128_t * );
void f128M_sqrt( const sw_float128_t *, sw_float128_t * );
bool f128M_eq( const sw_float128_t *, const sw_float128_t * );
bool f128M_le( const sw_float128_t *, const sw_float128_t * );
bool f128M_lt( const sw_float128_t *, const sw_float128_t * );
bool f128M_eq_signaling( const sw_float128_t *, const sw_float128_t * );
bool f128M_le_quiet( const sw_float128_t *, const sw_float128_t * );
bool f128M_lt_quiet( const sw_float128_t *, const sw_float128_t * );
bool f128M_isSignalingNaN( const sw_float128_t * );

#endif

