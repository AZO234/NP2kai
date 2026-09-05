/*============================================================================

This C header provides SoftFloat primitives implemented with Microsoft C/C++
compiler intrinsics.  The functions keep SoftFloat's zero-input semantics while
allowing normalized operands to use the host bit-scan instruction directly.

=============================================================================*/

#ifndef opts_MSVC_h
#define opts_MSVC_h 1

#if defined(INLINE) && (defined(_M_IX86) || defined(_M_X64))

#include <intrin.h>
#include <stdint.h>

INLINE uint_fast8_t softfloat_countLeadingZeros32(uint32_t a)
{
    unsigned long index;
    return _BitScanReverse(&index, (unsigned long)a) ? (uint_fast8_t)(31 - index) : 32;
}
#define softfloat_countLeadingZeros32 softfloat_countLeadingZeros32

INLINE uint_fast8_t softfloat_countLeadingZeros64(uint64_t a)
{
    unsigned long index;
#if defined(_M_X64)
    return _BitScanReverse64(&index, a) ? (uint_fast8_t)(63 - index) : 64;
#else
    const uint32_t high = (uint32_t)(a >> 32);
    if (_BitScanReverse(&index, (unsigned long)high)) {
        return (uint_fast8_t)(31 - index);
    }
    return _BitScanReverse(&index, (unsigned long)(uint32_t)a)
        ? (uint_fast8_t)(63 - index) : 64;
#endif
}
#define softfloat_countLeadingZeros64 softfloat_countLeadingZeros64

#endif

#endif
