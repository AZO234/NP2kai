
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

/*----------------------------------------------------------------------------
| SoftFloat uses the native 64-bit integer path and inlines its small integer
| primitives aggressively.  These primitives dominate extended-precision add,
| divide, normalize, and conversion paths.
*----------------------------------------------------------------------------*/
#define LITTLEENDIAN 1
#define SOFTFLOAT_FAST_INT64
#define INLINE_LEVEL 5

/*----------------------------------------------------------------------------
| SoftFloat defines many small primitives in headers when INLINE_LEVEL enables
| them.  Internal linkage lets each translation unit inline those helpers while
| the separately compiled primitive sources keep their single external symbols.
*----------------------------------------------------------------------------*/
#ifndef INLINE
#if defined(_MSC_VER)
#pragma warning(disable: 4244)
#pragma warning(disable: 4245)
#define INLINE static __forceinline
#elif defined(__BORLANDC__)
#define INLINE static __inline
#elif defined(__GNUC__)
#define INLINE static __inline__ __attribute__((always_inline))
#else
#define INLINE static
#endif
#endif

/*----------------------------------------------------------------------------
| Use compiler bit-scan/count-leading-zero primitives when they are available.
*----------------------------------------------------------------------------*/
#if defined(_MSC_VER)
#include "opts-MSVC.h"
#else
#define SOFTFLOAT_BUILTIN_CLZ 1
#include "opts-GCC.h"
#endif
