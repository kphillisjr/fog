// [Fog-G2d]
//
// [License]
// MIT, See COPYING file in package

// [Guard]
#ifndef _FOG_G2D_SOURCE_COLORUTIL_H
#define _FOG_G2D_SOURCE_COLORUTIL_H

// [Dependencies]
#include <Fog/Core/Face/Face_C.h>
#include <Fog/Core/Math/Math.h>
#include <Fog/G2d/Global/Constants.h>
#include <Fog/G2d/Source/ColorBase.h>

#if defined(FOG_HARDCODE_SSE)
#include <Fog/Core/Face/Face_SSE.h>
#endif // FOG_HARDCODE

#if defined(FOG_HARDCODE_SSE2)
#include <Fog/Core/Face/Face_SSE2.h>
#endif // FOG_HARDCODE

namespace Fog {

//! @addtogroup Fog_G2d_Source
//! @{

// ============================================================================
// [Fog::ColorUtil]
// ============================================================================

struct FOG_NO_EXPORT ColorUtil
{
  // --------------------------------------------------------------------------
  // [Convert]
  // --------------------------------------------------------------------------

  static FOG_INLINE void argb32From64(ArgbBase32& argb32, const ArgbBase64& argb64)
  {
#if defined(FOG_HARDCODE_SSE2)
    // SSE2 Implementation.
    Face::m128i xmm0;
    Face::m128iLoad8(xmm0, &argb64);
    xmm0 = _mm_srli_epi16(xmm0, 8);
    xmm0 = _mm_packus_epi16(xmm0, xmm0);
    argb32.p32 = _mm_cvtsi128_si32(xmm0);
#elif FOG_ARCH_BITS >= 64
    // C Implementation (64-bit).
    uint32_t t0 = (uint32_t)(argb64._packed64 >> (56 - 24)) & 0xFF000000U;
    uint32_t t1 = (uint32_t)(argb64._packed64 >> (32 - 16)) & 0x00FF0000U;
    uint32_t t2 = (uint32_t)(argb64._packed64 >> (24 -  8)) & 0x0000FF00U;
    uint32_t t3 = (uint32_t)(argb64._packed64 >> ( 8 -  0)) & 0x000000FFU;

    t0 += t2;
    t1 += t3;

    argb32._packed32 = t0 + t1;
#else
    // C Implementation (32-bit).
    uint32_t t0 = ((argb64.a << 16) | argb64.g     ) & 0xFF00FF00;
    uint32_t t1 = ((argb64.r <<  8) | argb64.b >> 8) & 0x00FF00FF;
    argb32.u32 = t0 + t1;
#endif // FOG_HARDCODE
  }

  static FOG_INLINE void argb32FromF(ArgbBase32& argb32, const float* argbf)
  {
#if defined(FOG_HARDCODE_SSE2)
    Face::m128i xmm0;
    Face::m128f xmmf;

    Face::m128fLoad16u(xmmf, argbf);
    Face::m128fMulPS(xmmf, xmmf, FOG_SSE_GET_CONST_PS(m128f_4x_255));
    Face::m128iCvtPI32FromPS(xmm0, xmmf);
    Face::m128iSwapPI32(xmm0, xmm0);
    Face::m128iPackPU8FromPI32(xmm0, xmm0);

    Face::m128iStore4(&argb32.p32, xmm0);
#else
    argb32.u32 =
      ((uint32_t)((int)(argbf[0] * (255.0f)) ) << ARGB32_ASHIFT) |
      ((uint32_t)((int)(argbf[1] * (255.0f)) ) << ARGB32_RSHIFT) |
      ((uint32_t)((int)(argbf[2] * (255.0f)) ) << ARGB32_GSHIFT) |
      ((uint32_t)((int)(argbf[3] * (255.0f)) ) << ARGB32_BSHIFT) ;
#endif // FOG_HARDCODE
  }

  static FOG_INLINE void argb64From32(ArgbBase64& argb64, const ArgbBase32& argb32)
  {
#if defined(FOG_HARDCODE_SSE2)
    // SSE2 Implementation.
    Face::m128i xmm0;
    Face::m128iLoad4(xmm0, &argb32);
    Face::m128iUnpackPI16FromPI8Lo(xmm0, xmm0, xmm0);
#if defined(FOG_ARCH_X64)
    argb64._packed64 = _mm_cvtsi128_si64(xmm0);
#else
    Face::m128iStore8(&argb64.p64, xmm0);
#endif
#elif FOG_ARGB_BITS >= 64
    // C (64-bit) Implementation.
    uint32_t c0 = argb32._packed32;

    uint64_t t0 = ((uint64_t)(c0 & 0x0000FF00U) <<  8) | ((uint64_t)(c0 & 0x000000FFU)      );
    uint64_t t1 = ((uint64_t)(c0 & 0xFF000000U) << 24) | ((uint64_t)(c0 & 0x00FF0000U) << 16);

    t0 += t1;
    t0 *= 0x01010101U;
    argb64._packed64 = t0;
#else
    // C (32-bit) Implementation.
    uint32_t c0 = argb32.u32;

    uint32_t t0 = ((c0 & 0x0000FF00U) << 8) | ((c0 & 0x000000FFU)      );
    uint32_t t1 = ((c0 & 0xFF000000U) >> 8) | ((c0 & 0x00FF0000U) >> 16);

    t0 *= 0x0101U;
    t1 *= 0x0101U;

    argb64.u64 = (uint64_t)t0 + ((uint64_t)t1 << 32);
#endif // FOG_HARDCODE
  }

  static FOG_INLINE void argb64FromF(ArgbBase64& argb64, const float* argbf)
  {
#if defined(FOG_HARDCODE_SSE2)
    Face::m128i xmm0;
    Face::m128f xmmf;

    Face::m128fLoad16u(xmmf, argbf);
    Face::m128fMulPS(xmmf, xmmf, FOG_SSE_GET_CONST_PS(m128f_4x_65535));
    Face::m128iCvtPI32FromPS(xmm0, xmmf);
    Face::m128iSwapPI32(xmm0, xmm0);
    Face::m128iPackPU16FromPI32(xmm0, xmm0);

    Face::m128iStore8(&argb64.p64, xmm0);
#else
    argb64.u64 =
      ((uint64_t)((int)(argbf[0] * (65535.0f)) ) << ARGB64_ASHIFT) |
      ((uint64_t)((int)(argbf[1] * (65535.0f)) ) << ARGB64_RSHIFT) |
      ((uint64_t)((int)(argbf[2] * (65535.0f)) ) << ARGB64_GSHIFT) |
      ((uint64_t)((int)(argbf[3] * (65535.0f)) ) << ARGB64_BSHIFT) ;
#endif // FOG_HARDCODE
  }

  static FOG_INLINE void argbfFromArgb32(float* dst, const ArgbBase32& argb32)
  {
#if defined(FOG_HARDCODE_SSE2)
    Face::m128i xmm0;
    Face::m128f xmmf;

    Face::m128iLoad4(xmm0, &argb32.p32);
    Face::m128iUnpackPI32FromPI8Lo(xmm0, xmm0);
    Face::m128iSwapPI32(xmm0, xmm0);
    Face::m128fCvtPSFromPI32(xmmf, xmm0);
    Face::m128fMulPS(xmmf, xmmf, FOG_SSE_GET_CONST_PS(m128f_4x_1_div_255));

    Face::m128fStore16u(dst, xmmf);
#else
    uint32_t c0 = argb32.u32;

    dst[0] = (float)((c0 >> ARGB32_ASHIFT) & 0xFF) * (1.0f / 255.0f);
    dst[1] = (float)((c0 >> ARGB32_RSHIFT) & 0xFF) * (1.0f / 255.0f);
    dst[2] = (float)((c0 >> ARGB32_GSHIFT) & 0xFF) * (1.0f / 255.0f);
    dst[3] = (float)((c0 >> ARGB32_BSHIFT) & 0xFF) * (1.0f / 255.0f);
#endif // FOG_HARDCODE
  }

  static FOG_INLINE void argbfFromArgb64(float* dst,  const ArgbBase64& argb64)
  {
#if defined(FOG_HARDCODE_SSE2)
    Face::m128i xmm0;
    Face::m128f xmmf;

    Face::m128iLoad8(xmm0, &argb64.p64);
    Face::m128iUnpackPI32FromPI16Lo(xmm0, xmm0);
    Face::m128iSwapPI32(xmm0, xmm0);
    Face::m128fCvtPSFromPI32(xmmf, xmm0);
    Face::m128fMulPS(xmmf, xmmf, FOG_SSE_GET_CONST_PS(m128f_4x_1_div_65535));

    Face::m128fStore16u(dst, xmmf);
#else
    dst[0] = (float)(argb64.a) * (1.0f / 65535.0f);
    dst[1] = (float)(argb64.r) * (1.0f / 65535.0f);
    dst[2] = (float)(argb64.g) * (1.0f / 65535.0f);
    dst[3] = (float)(argb64.b) * (1.0f / 65535.0f);
#endif // FOG_HARDCODE
  }

  // --------------------------------------------------------------------------
  // [Combine]
  // --------------------------------------------------------------------------

  static FOG_INLINE uint32_t chalf32(uint32_t x32, uint32_t y32)
  {
    return (((x32 ^ y32) & 0xFEFEFEFEU) >> 1) + (x32 & y32);
  }

  static FOG_INLINE uint64_t chalf64(uint64_t x64, uint64_t y64)
  {
    return (((x64 ^ y64) & FOG_UINT64_C(0xFFFEFFFEFFFEFFFE)) >> 1) + (x64 & y64);
  }

  static FOG_INLINE uint32_t cadd32(uint32_t x32, uint32_t y32)
  {
#if defined(FOG_HARDCODE_SSE2)
    // SSE2 Implementation.
    __m128i xmm0 = _mm_cvtsi32_si128((int)(x32));
    __m128i xmm1 = _mm_cvtsi32_si128((int)(y32 & 0x00FFFFFFU));
    return _mm_cvtsi128_si32(_mm_adds_epu8(xmm0, xmm1));
#else
    // C Implementation.
    uint32_t t0 = (x32 & 0x00FF00FFU) + (y32 & 0x00FF00FFU);
    uint32_t t1 = (x32 & 0xFF00FF00U) + (y32 & 0x0000FF00U);

    if (t0 & 0xFF000000U) t0 |= 0x00FF0000U;
    if (t1 & 0x00FF0000U) t1 |= 0x0000FF00U;
    if (t0 & 0x0000FF00U) t0 |= 0x000000FFU;

    t0 &= 0x00FF00FFU;
    t1 &= 0xFF00FF00U;

    return t0 + t1;
#endif // FOG_HARDCODE
  }

  static FOG_INLINE uint32_t csub32(uint32_t x32, uint32_t y32)
  {
#if defined(FOG_HARDCODE_SSE2)
    // SSE2 Implementation.
    __m128i xmm0 = _mm_cvtsi32_si128((int)(x32));
    __m128i xmm1 = _mm_cvtsi32_si128((int)(y32 & 0x00FFFFFFU));
    return _mm_cvtsi128_si32(_mm_subs_epu8(xmm0, xmm1));
#else
    // C Implementation.
    uint32_t t0 = (x32 & 0x00FF0000U) - (y32 & 0x00FF0000U);
    uint32_t t1 = (x32 & 0x0000FF00U) - (y32 & 0x0000FF00U);
    uint32_t t2 = (x32 & 0x000000FFU) - (y32 & 0x000000FFU);

    if (t0 & 0xFF00FFFFU) t0 = 0;
    if (t1 & 0xFFFF00FFU) t1 = 0;
    if (t2 & 0xFFFFFF00U) t2 = 0;

    x32 &= 0xFF000000;
    t0 += t1;
    x32 += t2;
    x32 += t0;

    return x32;
#endif // FOG_HARDCODE
  }
};

//! @}

} // Fog namespace

// [Guard]
#endif // _FOG_G2D_SOURCE_COLORUTIL_H
