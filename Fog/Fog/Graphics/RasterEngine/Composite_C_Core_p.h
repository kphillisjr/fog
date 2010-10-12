// [Fog-Graphics]
//
// [License]
// MIT, See COPYING file in package

// For some IDEs to enable code-assist.
#include <Fog/Core/Build.h>

#if defined(FOG_IDE)
#include <Fog/Graphics/RasterEngine/Defs_C_p.h>
#endif // FOG_IDE

namespace Fog {
namespace RasterEngine {

// ============================================================================
// [Fog::RasterEngine::C - Composite - NOP]
// ============================================================================

//! @internal
struct FOG_HIDDEN CompositeNopC
{
  static void FOG_FASTCALL cblit_full(
    uint8_t* dst, const RasterSolid* src, int w, const RasterClosure* closure)
  {
    FOG_UNUSED(dst);
    FOG_UNUSED(src);
    FOG_UNUSED(w);
    FOG_UNUSED(closure);
  }

  static void FOG_FASTCALL cblit_span(
    uint8_t* dst, const RasterSolid* src, const Span* span, const RasterClosure* closure)
  {
    FOG_UNUSED(dst);
    FOG_UNUSED(src);
    FOG_UNUSED(span);
    FOG_UNUSED(closure);
  }

  static void FOG_FASTCALL vblit_full(
    uint8_t* dst, const uint8_t* src, int w, const RasterClosure* closure)
  {
    FOG_UNUSED(dst);
    FOG_UNUSED(src);
    FOG_UNUSED(w);
    FOG_UNUSED(closure);
  }

  static void FOG_FASTCALL vblit_span(
    uint8_t* dst, const Span* span, const RasterClosure* closure)
  {
    FOG_UNUSED(dst);
    FOG_UNUSED(span);
    FOG_UNUSED(closure);
  }
};

// ============================================================================
// [Fog::RasterEngine::C - Composite - Helpers]
// ============================================================================

template<int CHARACTERISTICS>
struct CompositeHelpersC32
{
  static FOG_INLINE bool processDstPixel(uint32_t c0)
  {
    if (CHARACTERISTICS & OPERATOR_CHAR_NOP_IF_DST_A_ZERO)
      return !RasterUtil::isAlpha0x00_ARGB32(c0);
    else if (CHARACTERISTICS & OPERATOR_CHAR_NOP_IF_DST_A_FULL)
      return !RasterUtil::isAlpha0xFF_ARGB32(c0);
    else
      return true;
  }

  static FOG_INLINE bool processSrcPixel(uint32_t c0)
  {
    if (CHARACTERISTICS & OPERATOR_CHAR_NOP_IF_SRC_A_ZERO)
      return !RasterUtil::isAlpha0x00_ARGB32(c0);
    else if (CHARACTERISTICS & OPERATOR_CHAR_NOP_IF_SRC_A_FULL)
      return !RasterUtil::isAlpha0xFF_ARGB32(c0);
    else
      return true;
  }
};

// ============================================================================
// [Fog::RasterEngine::C - Composite - Base Funcs]
// ============================================================================

template<typename OP>
struct CompositeBaseFuncsC32
{
  // --------------------------------------------------------------------------
  // [CompositeBaseFuncsC32 - PRGB32 - CBlit]
  // --------------------------------------------------------------------------

  static void FOG_FASTCALL prgb32_cblit_full(
    uint8_t* dst, const RasterSolid* src, int w, const RasterClosure* closure)
  {
    FOG_ASSERT(w > 0);

    Face::b32_1x2 src0, src1;
    Face::b32_2x2Unpack0213(src0, src1, src->prgb);

    do {
      Face::b32_1x2 dst0, dst1;
      dst0 = READ_32(dst);

      if (OP::CHARACTERISTICS & (OPERATOR_CHAR_NOP_IF_DST_A_ZERO | OPERATOR_CHAR_NOP_IF_DST_A_FULL))
      {
        if (!CompositeHelpersC32<OP::CHARACTERISTICS>::processDstPixel(dst0))
          goto cMaskOpaqueSkip;
      }
      Face::b32_2x2Unpack0213(dst0, dst1, dst0);

      OP::prgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

      Face::b32_2x2Pack0213(dst0, dst0, dst1);
      ((uint32_t*)dst)[0] = dst0;

cMaskOpaqueSkip:
      dst += 4;
    } while (--w);
  }

  static void FOG_FASTCALL prgb32_cblit_span(
    uint8_t* dst, const RasterSolid* src, const Span* span, const RasterClosure* closure)
  {
    // [Begin] ----------------------------------------------------------------
    Face::b32_1x2 src0orig, src1orig;
    Face::b32_2x2Unpack0213(src0orig, src1orig, src->prgb);

    C_BLIT_SPAN8_BEGIN(4)
    // ------------------------------------------------------------------------

    // [CMask - Opaque] -------------------------------------------------------
    C_BLIT_SPAN8_CASE_CMASK_OPAQUE()
    {
      do {
        Face::b32_1x2 dst0, dst1;
        dst0 = READ_32(dst);

        if (OP::CHARACTERISTICS & (OPERATOR_CHAR_NOP_IF_DST_A_ZERO | OPERATOR_CHAR_NOP_IF_DST_A_FULL))
        {
          if (!CompositeHelpersC32<OP::CHARACTERISTICS>::processDstPixel(dst0))
            goto cMaskOpaqueSkip;
        }

        Face::b32_2x2Unpack0213(dst0, dst1, dst0);

        OP::prgb32_op_prgb32_32b(dst0, dst0, src0orig, dst1, dst1, src1orig);

        Face::b32_2x2Pack0213(dst0, dst0, dst1);
        ((uint32_t*)dst)[0] = dst0;

cMaskOpaqueSkip:
        dst += 4;
      } while (--w);
    }
    // ------------------------------------------------------------------------

    // [CMask - Alpha] --------------------------------------------------------
    C_BLIT_SPAN8_CASE_CMASK_ALPHA()
    {
      if (OP::CHARACTERISTICS & OPERATOR_CHAR_UNBOUND)
      {
        if (OP::CHARACTERISTICS & OPERATOR_CHAR_UNBOUND_MSK_IN)
        {
          Face::b32_1x2 src0, src1;
          Face::b32_2x2MulDiv255U(src0, src0orig, src1, src1orig, msk0);
          msk0 = 255 - msk0;

          do {
            Face::b32_1x2 dst0, dst1;
            Face::b32_1x2 dst0inv, dst1inv;

            dst0 = READ_32(dst);

            Face::b32_2x2Unpack0213(dst0, dst1, dst0);
            Face::b32_2x2MulDiv255U(dst0inv, dst0, dst1inv, dst1, msk0);
            Face::b32_2x2Pack0213(dst0inv, dst0inv, dst1inv);

            OP::prgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

            Face::b32_2x2Pack0213(dst0, dst0, dst1);
            ((uint32_t*)dst)[0] = dst0 + dst0inv;

            dst += 4;
          } while (--w);
        }
        else
        {
          do {
            Face::b32_1x2 dst0, dst1;
            Face::b32_1x2 dst0inv, dst1inv;

            dst0 = READ_32(dst);

            Face::b32_2x2Unpack0213(dst0, dst1, dst0);
            Face::b32_2x2MulDiv255U(dst0inv, dst0, dst1inv, dst1, 255 - msk0);
            Face::b32_2x2Pack0213(dst0inv, dst0inv, dst1inv);

            OP::prgb32_op_prgb32_32b(dst0, dst0, src0orig, dst1, dst1, src1orig);

            Face::b32_2x2MulDiv255U(dst0, dst0, dst1, dst1, msk0);
            Face::b32_2x2Pack0213(dst0, dst0, dst1);
            ((uint32_t*)dst)[0] = dst0 + dst0inv;

            dst += 4;
          } while (--w);
        }
      }
      else
      {
        Face::b32_1x2 src0, src1;
        Face::b32_2x2MulDiv255U(src0, src0orig, src1, src1orig, msk0);

        do {
          Face::b32_1x2 dst0, dst1;

          dst0 = READ_32(dst);

          if (OP::CHARACTERISTICS & (OPERATOR_CHAR_NOP_IF_DST_A_ZERO | OPERATOR_CHAR_NOP_IF_DST_A_FULL))
          {
            if (!CompositeHelpersC32<OP::CHARACTERISTICS>::processDstPixel(dst0))
              goto cMaskAlphaBoundSkip;
          }

          Face::b32_2x2Unpack0213(dst0, dst1, dst0);

          OP::prgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0;

cMaskAlphaBoundSkip:
          dst += 4;
        } while (--w);
      }
    }
    // ------------------------------------------------------------------------

    // [VMask - Alpha - Dense] ------------------------------------------------
    C_BLIT_SPAN8_CASE_VMASK_A_DENSE()
    {
      if (OP::CHARACTERISTICS & OPERATOR_CHAR_UNBOUND)
      {
        do {
          Face::b32_1x2 dst0, dst1;
          Face::b32_1x2 dst0inv, dst1inv;
          uint32_t msk0;

          msk0 = READ_8(msk);
          dst0 = READ_32(dst);

          Face::b32_2x2Unpack0213(dst0, dst1, dst0);
          Face::b32_2x2MulDiv255U(dst0inv, dst0, dst1inv, dst1, 255 - msk0);
          Face::b32_2x2Pack0213(dst0inv, dst0inv, dst1inv);

          OP::prgb32_op_prgb32_32b(dst0, dst0, src0orig, dst1, dst1, src1orig);

          Face::b32_2x2MulDiv255U(dst0, dst0, dst1, dst1, msk0);
          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0 + dst0inv;

          dst += 4;
          msk += 1;
        } while (--w);
      }
      else
      {
        do {
          Face::b32_1x2 dst0, dst1;
          Face::b32_1x2 src0, src1;
          uint32_t msk0;

          dst0 = READ_32(dst);

          if (OP::CHARACTERISTICS & (OPERATOR_CHAR_NOP_IF_DST_A_ZERO | OPERATOR_CHAR_NOP_IF_DST_A_FULL))
          {
            if (!CompositeHelpersC32<OP::CHARACTERISTICS>::processDstPixel(dst0))
              goto vMaskAlphaDenseBoundSkip;
          }

          msk0 = READ_8(msk);

          Face::b32_2x2Unpack0213(dst0, dst1, dst0);
          Face::b32_2x2MulDiv255U(src0, src0orig, src1, src1orig, msk0);

          OP::prgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0;

vMaskAlphaDenseBoundSkip:
          dst += 4;
          msk += 1;
        } while (--w);
      }
    }
    // ------------------------------------------------------------------------

    // [VMask - Alpha - Sparse] -----------------------------------------------
    C_BLIT_SPAN8_CASE_VMASK_A_SPARSE()
    {
      if (OP::CHARACTERISTICS & OPERATOR_CHAR_UNBOUND)
      {
        for (;;)
        {
          Face::b32_1x2 dst0, dst1;
          Face::b32_1x2 dst0inv, dst1inv;
          uint32_t msk0;

          msk0 = READ_8(msk);
          if (msk0 == 0x00) goto vMaskAlphaSparseUnBoundSkip;

          dst0 = READ_32(dst);
          if (msk0 == 0xFF) goto vMaskAlphaSparseUnBoundNoMask;

          Face::b32_2x2Unpack0213(dst0, dst1, dst0);
          Face::b32_2x2MulDiv255U(dst0inv, dst0, dst1inv, dst1, 255 - msk0);
          Face::b32_2x2Pack0213(dst0inv, dst0inv, dst1inv);

          OP::prgb32_op_prgb32_32b(dst0, dst0, src0orig, dst1, dst1, src1orig);

          Face::b32_2x2MulDiv255U(dst0, dst0, dst1, dst1, msk0);
          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0 + dst0inv;

vMaskAlphaSparseUnBoundSkip:
          dst += 4;
          msk += 1;

          if (--w) continue;
          break;

vMaskAlphaSparseUnBoundNoMask:
          Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));

          OP::prgb32_op_prgb32_32b(dst0, dst0, src0orig, dst1, dst1, src1orig);

          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0;

          dst += 4;
          msk += 1;

          if (--w) continue;
          break;
        }
      }
      else
      {
        for (;;)
        {
          Face::b32_1x2 dst0, dst1;
          Face::b32_1x2 src0, src1;
          uint32_t msk0;

          msk0 = READ_8(msk);
          if (msk0 == 0x00) goto vMaskAlphaSparseBoundSkip;
          if (msk0 == 0xFF) goto vMaskAlphaSparseBoundNoMask;

          dst0 = READ_32(dst);

          if (OP::CHARACTERISTICS & (OPERATOR_CHAR_NOP_IF_DST_A_ZERO | OPERATOR_CHAR_NOP_IF_DST_A_FULL))
          {
            if (!CompositeHelpersC32<OP::CHARACTERISTICS>::processDstPixel(dst0))
              goto vMaskAlphaSparseBoundSkip;
          }

          Face::b32_2x2Unpack0213(dst0, dst1, dst0);
          Face::b32_2x2MulDiv255U(src0, src0orig, src1, src1orig, msk0);

          OP::prgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0;

vMaskAlphaSparseBoundSkip:
          dst += 4;
          msk += 1;

          if (--w) continue;
          break;

vMaskAlphaSparseBoundNoMask:
          Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));

          OP::prgb32_op_prgb32_32b(dst0, dst0, src0orig, dst1, dst1, src1orig);

          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0;

          dst += 4;
          msk += 1;

          if (--w) continue;
          break;
        }
      }
    }
    // ------------------------------------------------------------------------

    // [VMask - ARGB - Dense] -------------------------------------------------
    C_BLIT_SPAN8_CASE_VMASK_ARGB_DENSE()
    {
      // TODO: VMASK-ARGB.
    }
    // ------------------------------------------------------------------------

    // [VMask - ARGB - Sparse] ------------------------------------------------
    C_BLIT_SPAN8_CASE_VMASK_ARGB_SPARSE()
    {
      // TODO: VMASK-ARGB.
    }
    // ------------------------------------------------------------------------

    // [End] ------------------------------------------------------------------
    C_BLIT_SPAN8_END()
    // ------------------------------------------------------------------------
  }

  // --------------------------------------------------------------------------
  // [CompositeBaseFuncsC32 - PRGB32 - VBlit - PRGB32]
  // --------------------------------------------------------------------------

  static void FOG_FASTCALL prgb32_vblit_prgb32_full(
    uint8_t* dst, const uint8_t* src, int w, const RasterClosure* closure)
  {
    FOG_ASSERT(w > 0);

    do {
      Face::b32_1x2 src0, src1;
      Face::b32_1x2 dst0, dst1;

      if (OP::CHARACTERISTICS & (OPERATOR_CHAR_NOP_IF_DST_A_ZERO | OPERATOR_CHAR_NOP_IF_DST_A_FULL))
      {
        dst0 = READ_32(dst);
        if (CompositeHelpersC32<OP::CHARACTERISTICS>::processDstPixel(dst0))
        {
          Face::b32_2x2Unpack0213(src0, src1, READ_32(src));
          Face::b32_2x2Unpack0213(dst0, dst1, dst0);

          OP::prgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0;
        }
      }
      else if (OP::CHARACTERISTICS & (OPERATOR_CHAR_NOP_IF_SRC_A_ZERO | OPERATOR_CHAR_NOP_IF_SRC_A_FULL))
      {
        src0 = READ_32(src);
        if (CompositeHelpersC32<OP::CHARACTERISTICS>::processSrcPixel(src0))
        {
          Face::b32_2x2Unpack0213(src0, src1, src0);
          Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));

          OP::prgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0;
        }
      }
      else
      {
        Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));
        Face::b32_2x2Unpack0213(src0, src1, READ_32(src));

        OP::prgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

        Face::b32_2x2Pack0213(dst0, dst0, dst1);
        ((uint32_t*)dst)[0] = dst0;
      }

      dst += 4;
      src += 4;
    } while (--w);
  }

  static void FOG_FASTCALL prgb32_vblit_prgb32_span(
    uint8_t* dst, const Span* span, const RasterClosure* closure)
  {
    // [Begin] ----------------------------------------------------------------
    V_BLIT_SPAN8_BEGIN(4)
    // ------------------------------------------------------------------------

    // [CMask - Opaque] -------------------------------------------------------
    V_BLIT_SPAN8_CASE_CMASK_OPAQUE()
    {
      do {
        Face::b32_1x2 src0, src1;
        Face::b32_1x2 dst0, dst1;

        if (OP::CHARACTERISTICS & (OPERATOR_CHAR_NOP_IF_DST_A_ZERO | OPERATOR_CHAR_NOP_IF_DST_A_FULL))
        {
          dst0 = READ_32(dst);
          if (CompositeHelpersC32<OP::CHARACTERISTICS>::processDstPixel(dst0))
          {
            Face::b32_2x2Unpack0213(src0, src1, READ_32(src));
            Face::b32_2x2Unpack0213(dst0, dst1, dst0);

            OP::prgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

            Face::b32_2x2Pack0213(dst0, dst0, dst1);
            ((uint32_t*)dst)[0] = dst0;
          }
        }
        else if (OP::CHARACTERISTICS & (OPERATOR_CHAR_NOP_IF_SRC_A_ZERO | OPERATOR_CHAR_NOP_IF_SRC_A_FULL))
        {
          src0 = READ_32(src);
          if (CompositeHelpersC32<OP::CHARACTERISTICS>::processSrcPixel(src0))
          {
            Face::b32_2x2Unpack0213(src0, src1, src0);
            Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));

            OP::prgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

            Face::b32_2x2Pack0213(dst0, dst0, dst1);
            ((uint32_t*)dst)[0] = dst0;
          }
        }
        else
        {
          Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));
          Face::b32_2x2Unpack0213(src0, src1, READ_32(src));

          OP::prgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0;
        }

        dst += 4;
        src += 4;
      } while (--w);
    }
    // ------------------------------------------------------------------------

    // [CMask - Alpha] --------------------------------------------------------
    V_BLIT_SPAN8_CASE_CMASK_ALPHA()
    {
      uint32_t msk0inv;
      if ((OP::CHARACTERISTICS & OPERATOR_CHAR_UNBOUND)) msk0inv = Face::b32_1x1Negate255(msk0);

      do {
        Face::b32_1x2 src0, src1;
        Face::b32_1x2 dst0, dst1;

        if ((OP::CHARACTERISTICS & OPERATOR_CHAR_UNBOUND) && OP::CHARACTERISTICS & (OPERATOR_CHAR_UNBOUND_MSK_IN))
        {
          Face::b32_1x2 dst0inv, dst1inv;

          Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));
          Face::b32_2x2MulDiv255U(dst0inv, dst0, dst1inv, dst1, msk0inv);
          Face::b32_2x2Pack0213(dst0inv, dst0inv, dst1inv);

          Face::b32_2x2Unpack0213(src0, src1, READ_32(src));
          Face::b32_2x2MulDiv255U(src0, src0, src1, src1, msk0);

          OP::prgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0 + dst0inv;
        }
        else if ((OP::CHARACTERISTICS & OPERATOR_CHAR_UNBOUND))
        {
          Face::b32_1x2 dst0inv, dst1inv;

          Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));
          Face::b32_2x2MulDiv255U(dst0inv, dst0, dst1inv, dst1, msk0inv);
          Face::b32_2x2Pack0213(dst0inv, dst0inv, dst1inv);

          Face::b32_2x2Unpack0213(src0, src1, READ_32(src));

          OP::prgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);
          Face::b32_2x2MulDiv255U(dst0, dst0, dst1, dst1, msk0);
          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0 + dst0inv;
        }
        else
        {
          Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));
          Face::b32_2x2Unpack0213(src0, src1, READ_32(src));

          Face::b32_2x2MulDiv255U(src0, src0, msk0, src1, src1, msk0);

          OP::prgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0;
        }

        dst += 4;
        src += 4;
      } while (--w);
    }
    // ------------------------------------------------------------------------

    // [VMask - Alpha - Dense] ------------------------------------------------
    V_BLIT_SPAN8_CASE_VMASK_A_DENSE()
    {
      do {
        Face::b32_1x2 src0, src1;
        Face::b32_1x2 dst0, dst1;
        uint32_t msk0 = READ_8(msk);

        if ((OP::CHARACTERISTICS & OPERATOR_CHAR_UNBOUND) && OP::CHARACTERISTICS & (OPERATOR_CHAR_UNBOUND_MSK_IN))
        {
          Face::b32_1x2 dst0inv, dst1inv;

          Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));
          Face::b32_2x2MulDiv255U(dst0inv, dst0, dst1inv, dst1, Face::b32_1x1Negate255(msk0));
          Face::b32_2x2Pack0213(dst0inv, dst0inv, dst1inv);

          Face::b32_2x2Unpack0213(src0, src1, READ_32(src));
          Face::b32_2x2MulDiv255U(src0, src0, src1, src1, msk0);

          OP::prgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0 + dst0inv;
        }
        else if ((OP::CHARACTERISTICS & OPERATOR_CHAR_UNBOUND))
        {
          Face::b32_1x2 dst0inv, dst1inv;

          Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));
          Face::b32_2x2MulDiv255U(dst0inv, dst0, dst1inv, dst1, Face::b32_1x1Negate255(msk0));
          Face::b32_2x2Pack0213(dst0inv, dst0inv, dst1inv);

          Face::b32_2x2Unpack0213(src0, src1, READ_32(src));

          OP::prgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);
          Face::b32_2x2MulDiv255U(dst0, dst0, dst1, dst1, msk0);
          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0 + dst0inv;
        }
        else
        {
          Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));
          Face::b32_2x2Unpack0213(src0, src1, READ_32(src));

          Face::b32_2x2MulDiv255U(src0, src0, msk0, src1, src1, msk0);

          OP::prgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0;
        }

        dst += 4;
        src += 4;
        msk += 1;
      } while (--w);
    }
    // ------------------------------------------------------------------------

    // [VMask - Alpha - Sparse] -----------------------------------------------
    V_BLIT_SPAN8_CASE_VMASK_A_SPARSE()
    {
      for (;;)
      {
        Face::b32_1x2 src0, src1;
        Face::b32_1x2 dst0, dst1;

        uint32_t msk0 = READ_8(msk);
        if (msk0 == 0x00) goto vMaskAlphaSparseSkip;

        dst0 = READ_32(dst);
        src0 = READ_32(src);

        Face::b32_2x2Unpack0213(dst0, dst1, dst0);
        Face::b32_2x2Unpack0213(src0, src1, src0);

        if (msk0 == 0xFF) goto vMaskAlphaSparseNoMask;

        if ((OP::CHARACTERISTICS & OPERATOR_CHAR_UNBOUND) && OP::CHARACTERISTICS & (OPERATOR_CHAR_UNBOUND_MSK_IN))
        {
          Face::b32_1x2 dst0inv, dst1inv;

          Face::b32_2x2MulDiv255U(dst0inv, dst0, dst1inv, dst1, Face::b32_1x1Negate255(msk0));
          Face::b32_2x2Pack0213(dst0inv, dst0inv, dst1inv);
          Face::b32_2x2MulDiv255U(src0, src0, src1, src1, msk0);

          OP::prgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0 + dst0inv;
        }
        else if ((OP::CHARACTERISTICS & OPERATOR_CHAR_UNBOUND))
        {
          Face::b32_1x2 dst0inv, dst1inv;

          Face::b32_2x2MulDiv255U(dst0inv, dst0, dst1inv, dst1, Face::b32_1x1Negate255(msk0));
          Face::b32_2x2Pack0213(dst0inv, dst0inv, dst1inv);

          OP::prgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);
          Face::b32_2x2MulDiv255U(dst0, dst0, dst1, dst1, msk0);
          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0 + dst0inv;
        }
        else
        {
          Face::b32_2x2MulDiv255U(src0, src0, msk0, src1, src1, msk0);

          OP::prgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0;
        }

vMaskAlphaSparseSkip:
        dst += 4;
        src += 4;
        msk += 1;

        if (--w) continue;
        break;

vMaskAlphaSparseNoMask:
        OP::prgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

        Face::b32_2x2Pack0213(dst0, dst0, dst1);
        ((uint32_t*)dst)[0] = dst0;

        dst += 4;
        src += 4;
        msk += 1;

        if (--w) continue;
        break;
      }
    }
    // ------------------------------------------------------------------------

    // [VMask - ARGB - Dense] -------------------------------------------------
    V_BLIT_SPAN8_CASE_VMASK_ARGB_DENSE()
    {
      // TODO: VMASK-ARGB.
    }
    // ------------------------------------------------------------------------

    // [VMask - ARGB - Sparse] ------------------------------------------------
    V_BLIT_SPAN8_CASE_VMASK_ARGB_SPARSE()
    {
      // TODO: VMASK-ARGB.
    }
    // ------------------------------------------------------------------------

    // [End] ------------------------------------------------------------------
    V_BLIT_SPAN8_END()
    // ------------------------------------------------------------------------
  }

  // --------------------------------------------------------------------------
  // [CompositeBaseFuncsC32 - PRGB32 - VBlit - ARGB32]
  // --------------------------------------------------------------------------

  static void FOG_FASTCALL prgb32_vblit_argb32_full(
    uint8_t* dst, const uint8_t* src, int w, const RasterClosure* closure)
  {
    FOG_ASSERT(w > 0);

    do {
      Face::b32_1x2 src0, src1;
      Face::b32_1x2 dst0, dst1;

      if (OP::CHARACTERISTICS & (OPERATOR_CHAR_NOP_IF_DST_A_ZERO | OPERATOR_CHAR_NOP_IF_DST_A_FULL))
      {
        dst0 = READ_32(dst);
        if (!CompositeHelpersC32<OP::CHARACTERISTICS>::processDstPixel(dst0))
          goto cMaskOpaqueSkip;

        Face::b32_2x2Unpack0213(src0, src1, READ_32(src));
        Face::b32_2x2Unpack0213(dst0, dst1, dst0);

        // Premultiply only if source colors are used.
        if (OP::CHARACTERISTICS & OPERATOR_CHAR_SRC_C_USED) Face::b32_2x2PremultiplyA(src0, src0, src1, src1);
        OP::prgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

        Face::b32_2x2Pack0213(dst0, dst0, dst1);
        ((uint32_t*)dst)[0] = dst0;
      }
      else if (OP::CHARACTERISTICS & (OPERATOR_CHAR_NOP_IF_SRC_A_ZERO | OPERATOR_CHAR_NOP_IF_SRC_A_FULL))
      {
        src0 = READ_32(src);
        if (!CompositeHelpersC32<OP::CHARACTERISTICS>::processSrcPixel(src0))
          goto cMaskOpaqueSkip;

        Face::b32_2x2Unpack0213(src0, src1, src0);
        Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));

        // Premultiply only if source colors are used.
        if (OP::CHARACTERISTICS & OPERATOR_CHAR_SRC_C_USED) Face::b32_2x2PremultiplyA(src0, src0, src1, src1);
        OP::prgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

        Face::b32_2x2Pack0213(dst0, dst0, dst1);
        ((uint32_t*)dst)[0] = dst0;
      }
      else
      {
        Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));
        Face::b32_2x2Unpack0213(src0, src1, READ_32(src));

        // Premultiply only if source colors are used.
        if (OP::CHARACTERISTICS & OPERATOR_CHAR_SRC_C_USED) Face::b32_2x2PremultiplyA(src0, src0, src1, src1);
        OP::prgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

        Face::b32_2x2Pack0213(dst0, dst0, dst1);
        ((uint32_t*)dst)[0] = dst0;
      }

cMaskOpaqueSkip:
      dst += 4;
      src += 4;
    } while (--w);
  }

  static void FOG_FASTCALL prgb32_vblit_argb32_span(
    uint8_t* dst, const Span* span, const RasterClosure* closure)
  {
    // [Begin] ----------------------------------------------------------------
    V_BLIT_SPAN8_BEGIN(4)
    // ------------------------------------------------------------------------

    // [CMask - Opaque] -------------------------------------------------------
    V_BLIT_SPAN8_CASE_CMASK_OPAQUE()
    {
      do {
        Face::b32_1x2 src0, src1;
        Face::b32_1x2 dst0, dst1;

        if (OP::CHARACTERISTICS & (OPERATOR_CHAR_NOP_IF_DST_A_ZERO | OPERATOR_CHAR_NOP_IF_DST_A_FULL))
        {
          dst0 = READ_32(dst);
          if (!CompositeHelpersC32<OP::CHARACTERISTICS>::processDstPixel(dst0))
            goto cMaskOpaqueSkip;

          Face::b32_2x2Unpack0213(src0, src1, READ_32(src));
          Face::b32_2x2Unpack0213(dst0, dst1, dst0);

          // Premultiply only if source colors are used.
          if (OP::CHARACTERISTICS & OPERATOR_CHAR_SRC_C_USED) Face::b32_2x2PremultiplyA(src0, src0, src1, src1);
          OP::prgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0;
        }
        else if (OP::CHARACTERISTICS & (OPERATOR_CHAR_NOP_IF_SRC_A_ZERO | OPERATOR_CHAR_NOP_IF_SRC_A_FULL))
        {
          src0 = READ_32(src);
          if (!CompositeHelpersC32<OP::CHARACTERISTICS>::processSrcPixel(src0))
            goto cMaskOpaqueSkip;

          Face::b32_2x2Unpack0213(src0, src1, src0);
          Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));

          // Premultiply only if source colors are used.
          if (OP::CHARACTERISTICS & OPERATOR_CHAR_SRC_C_USED) Face::b32_2x2PremultiplyA(src0, src0, src1, src1);
          OP::prgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0;
        }
        else
        {
          Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));
          Face::b32_2x2Unpack0213(src0, src1, READ_32(src));

          // Premultiply only if source colors are used.
          if (OP::CHARACTERISTICS & OPERATOR_CHAR_SRC_C_USED) Face::b32_2x2PremultiplyA(src0, src0, src1, src1);
          OP::prgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0;
        }

cMaskOpaqueSkip:
        dst += 4;
        src += 4;
      } while (--w);
    }
    // ------------------------------------------------------------------------

    // [CMask - Alpha] --------------------------------------------------------
    V_BLIT_SPAN8_CASE_CMASK_ALPHA()
    {
      uint32_t msk0inv;
      if ((OP::CHARACTERISTICS & OPERATOR_CHAR_UNBOUND)) msk0inv = Face::b32_1x1Negate255(msk0);

      do {
        Face::b32_1x2 src0, src1;
        Face::b32_1x2 dst0, dst1;

        if ((OP::CHARACTERISTICS & OPERATOR_CHAR_UNBOUND) && OP::CHARACTERISTICS & (OPERATOR_CHAR_UNBOUND_MSK_IN))
        {
          Face::b32_1x2 dst0inv, dst1inv;

          Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));
          Face::b32_2x2MulDiv255U(dst0inv, dst0, dst1inv, dst1, msk0inv);
          Face::b32_2x2Pack0213(dst0inv, dst0inv, dst1inv);

          Face::b32_2x2Unpack0213(src0, src1, READ_32(src));
          Face::b32_2x2PremultiplyU(src0, src0, src1, src1, Face::b32_1x1MulDiv255(Face::b32_1x2GetB1(src1), msk0));

          OP::prgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0 + dst0inv;
        }
        else if ((OP::CHARACTERISTICS & OPERATOR_CHAR_UNBOUND))
        {
          Face::b32_1x2 dst0inv, dst1inv;

          Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));
          Face::b32_2x2MulDiv255U(dst0inv, dst0, dst1inv, dst1, msk0inv);
          Face::b32_2x2Pack0213(dst0inv, dst0inv, dst1inv);

          Face::b32_2x2Unpack0213(src0, src1, READ_32(src));
          Face::b32_2x2PremultiplyA(src0, src0, src1, src1);

          OP::prgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);
          Face::b32_2x2MulDiv255U(dst0, dst0, dst1, dst1, msk0);
          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0 + dst0inv;
        }
        else
        {
          Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));
          Face::b32_2x2Unpack0213(src0, src1, READ_32(src));
          Face::b32_2x2PremultiplyU(src0, src0, src1, src1, Face::b32_1x1MulDiv255(Face::b32_1x2GetB1(src1), msk0));

          OP::prgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0;
        }

        dst += 4;
        src += 4;
      } while (--w);
    }
    // ------------------------------------------------------------------------

    // [VMask - Alpha - Dense] ------------------------------------------------
    V_BLIT_SPAN8_CASE_VMASK_A_DENSE()
    {
      do {
        Face::b32_1x2 src0, src1;
        Face::b32_1x2 dst0, dst1;
        uint32_t msk0 = READ_8(msk);

        if ((OP::CHARACTERISTICS & OPERATOR_CHAR_UNBOUND) && OP::CHARACTERISTICS & (OPERATOR_CHAR_UNBOUND_MSK_IN))
        {
          Face::b32_1x2 dst0inv, dst1inv;

          Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));
          Face::b32_2x2MulDiv255U(dst0inv, dst0, dst1inv, dst1, Face::b32_1x1Negate255(msk0));
          Face::b32_2x2Pack0213(dst0inv, dst0inv, dst1inv);

          Face::b32_2x2Unpack0213(src0, src1, READ_32(src));
          Face::b32_2x2PremultiplyU(src0, src0, src1, src1, Face::b32_1x1MulDiv255(Face::b32_1x2GetB1(src1), msk0));

          OP::prgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0 + dst0inv;
        }
        else if ((OP::CHARACTERISTICS & OPERATOR_CHAR_UNBOUND))
        {
          Face::b32_1x2 dst0inv, dst1inv;

          Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));
          Face::b32_2x2MulDiv255U(dst0inv, dst0, dst1inv, dst1, Face::b32_1x1Negate255(msk0));
          Face::b32_2x2Pack0213(dst0inv, dst0inv, dst1inv);

          Face::b32_2x2Unpack0213(src0, src1, READ_32(src));
          Face::b32_2x2PremultiplyA(src0, src0, src1, src1);

          OP::prgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);
          Face::b32_2x2MulDiv255U(dst0, dst0, dst1, dst1, msk0);
          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0 + dst0inv;
        }
        else
        {
          Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));
          Face::b32_2x2Unpack0213(src0, src1, READ_32(src));
          Face::b32_2x2PremultiplyU(src0, src0, src1, src1, Face::b32_1x1MulDiv255(Face::b32_1x2GetB1(src1), msk0));

          OP::prgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0;
        }

        dst += 4;
        src += 4;
        msk += 1;
      } while (--w);
    }
    // ------------------------------------------------------------------------

    // [VMask - Alpha - Sparse] -----------------------------------------------
    V_BLIT_SPAN8_CASE_VMASK_A_SPARSE()
    {
      for (;;)
      {
        Face::b32_1x2 src0, src1;
        Face::b32_1x2 dst0, dst1;

        uint32_t msk0 = READ_8(msk);
        if (msk0 == 0x00) goto vMaskAlphaSparseSkip;

        dst0 = READ_32(dst);
        src0 = READ_32(src);

        Face::b32_2x2Unpack0213(dst0, dst1, dst0);
        Face::b32_2x2Unpack0213(src0, src1, src0);

        if (msk0 == 0xFF) goto vMaskAlphaSparseNoMask;

        if ((OP::CHARACTERISTICS & OPERATOR_CHAR_UNBOUND) && OP::CHARACTERISTICS & (OPERATOR_CHAR_UNBOUND_MSK_IN))
        {
          Face::b32_1x2 dst0inv, dst1inv;

          Face::b32_2x2MulDiv255U(dst0inv, dst0, dst1inv, dst1, Face::b32_1x1Negate255(msk0));
          Face::b32_2x2Pack0213(dst0inv, dst0inv, dst1inv);
          Face::b32_2x2PremultiplyU(src0, src0, src1, src1, Face::b32_1x1MulDiv255(Face::b32_1x2GetB1(src1), msk0));

          OP::prgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0 + dst0inv;
        }
        else if ((OP::CHARACTERISTICS & OPERATOR_CHAR_UNBOUND))
        {
          Face::b32_1x2 dst0inv, dst1inv;

          Face::b32_2x2MulDiv255U(dst0inv, dst0, dst1inv, dst1, Face::b32_1x1Negate255(msk0));
          Face::b32_2x2Pack0213(dst0inv, dst0inv, dst1inv);
          Face::b32_2x2PremultiplyA(src0, src0, src1, src1);

          OP::prgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);
          Face::b32_2x2MulDiv255U(dst0, dst0, dst1, dst1, msk0);
          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0 + dst0inv;
        }
        else
        {
          Face::b32_2x2PremultiplyU(src0, src0, src1, src1, Face::b32_1x1MulDiv255(Face::b32_1x2GetB1(src1), msk0));

          OP::prgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0;
        }

vMaskAlphaSparseSkip:
        dst += 4;
        src += 4;
        msk += 1;

        if (--w) continue;
        break;

vMaskAlphaSparseNoMask:
        if (OP::CHARACTERISTICS & OPERATOR_CHAR_SRC_C_USED) Face::b32_2x2PremultiplyA(src0, src0, src1, src1);
        OP::prgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

        Face::b32_2x2Pack0213(dst0, dst0, dst1);
        ((uint32_t*)dst)[0] = dst0;

        dst += 4;
        src += 4;
        msk += 1;

        if (--w) continue;
        break;
      }
    }
    // ------------------------------------------------------------------------

    // [VMask - ARGB - Dense] -------------------------------------------------
    V_BLIT_SPAN8_CASE_VMASK_ARGB_DENSE()
    {
      // TODO: VMASK-ARGB.
    }
    // ------------------------------------------------------------------------

    // [VMask - ARGB - Sparse] ------------------------------------------------
    V_BLIT_SPAN8_CASE_VMASK_ARGB_SPARSE()
    {
      // TODO: VMASK-ARGB.
    }
    // ------------------------------------------------------------------------

    // [End] ------------------------------------------------------------------
    V_BLIT_SPAN8_END()
    // ------------------------------------------------------------------------
  }

  // --------------------------------------------------------------------------
  // [CompositeBaseFuncsC32 - PRGB32 - VBlit - XRGB32]
  // --------------------------------------------------------------------------

  static void FOG_FASTCALL prgb32_vblit_xrgb32_full(
    uint8_t* dst, const uint8_t* src, int w, const RasterClosure* closure)
  {
    FOG_ASSERT(w > 0);

    do {
      Face::b32_1x2 src0, src1;
      Face::b32_1x2 dst0, dst1;

      if (OP::CHARACTERISTICS & (OPERATOR_CHAR_NOP_IF_DST_A_ZERO | OPERATOR_CHAR_NOP_IF_DST_A_FULL))
      {
        dst0 = READ_32(dst);
        if (!CompositeHelpersC32<OP::CHARACTERISTICS>::processDstPixel(dst0))
          goto cMaskOpaqueSkip;

        Face::b32_2x2Unpack0213(src0, src1, READ_32(src));
        Face::b32_2x2Unpack0213(dst0, dst1, dst0);

        OP::prgb32_op_xrgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

        Face::b32_2x2Pack0213(dst0, dst0, dst1);
        ((uint32_t*)dst)[0] = dst0;
      }
      else
      {
        Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));
        Face::b32_2x2Unpack0213(src0, src1, READ_32(src));

        OP::prgb32_op_xrgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

        Face::b32_2x2Pack0213(dst0, dst0, dst1);
        ((uint32_t*)dst)[0] = dst0;
      }

cMaskOpaqueSkip:
      dst += 4;
      src += 4;
    } while (--w);
  }

  static void FOG_FASTCALL prgb32_vblit_xrgb32_span(
    uint8_t* dst, const Span* span, const RasterClosure* closure)
  {
    // [Begin] ----------------------------------------------------------------
    V_BLIT_SPAN8_BEGIN(4)
    // ------------------------------------------------------------------------

    // [CMask - Opaque] -------------------------------------------------------
    V_BLIT_SPAN8_CASE_CMASK_OPAQUE()
    {
      do {
        Face::b32_1x2 src0, src1;
        Face::b32_1x2 dst0, dst1;

        if (OP::CHARACTERISTICS & (OPERATOR_CHAR_NOP_IF_DST_A_ZERO | OPERATOR_CHAR_NOP_IF_DST_A_FULL))
        {
          dst0 = READ_32(dst);
          if (!CompositeHelpersC32<OP::CHARACTERISTICS>::processDstPixel(dst0))
            goto cMaskOpaqueSkip;

          Face::b32_2x2Unpack0213(src0, src1, READ_32(src));
          Face::b32_2x2Unpack0213(dst0, dst1, dst0);

          OP::prgb32_op_xrgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0;
        }
        else
        {
          Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));
          Face::b32_2x2Unpack0213(src0, src1, READ_32(src));

          OP::prgb32_op_xrgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0;
        }

cMaskOpaqueSkip:
        dst += 4;
        src += 4;
      } while (--w);
    }
    // ------------------------------------------------------------------------

    // [CMask - Alpha] --------------------------------------------------------
    V_BLIT_SPAN8_CASE_CMASK_ALPHA()
    {
      uint32_t msk0inv;
      if ((OP::CHARACTERISTICS & OPERATOR_CHAR_UNBOUND)) msk0inv = Face::b32_1x1Negate255(msk0);

      do {
        Face::b32_1x2 src0, src1;
        Face::b32_1x2 dst0, dst1;

        if ((OP::CHARACTERISTICS & OPERATOR_CHAR_UNBOUND) && OP::CHARACTERISTICS & (OPERATOR_CHAR_UNBOUND_MSK_IN))
        {
          Face::b32_1x2 dst0inv, dst1inv;

          Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));
          Face::b32_2x2MulDiv255U(dst0inv, dst0, dst1inv, dst1, msk0inv);
          Face::b32_2x2Pack0213(dst0inv, dst0inv, dst1inv);

          Face::b32_2x2Unpack0213(src0, src1, READ_32(src) | 0xFF000000);
          Face::b32_2x2MulDiv255U(src0, src0, src1, src1, msk0);

          OP::prgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0 + dst0inv;
        }
        else if ((OP::CHARACTERISTICS & OPERATOR_CHAR_UNBOUND))
        {
          Face::b32_1x2 dst0inv, dst1inv;

          Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));
          Face::b32_2x2MulDiv255U(dst0inv, dst0, dst1inv, dst1, msk0inv);
          Face::b32_2x2Pack0213(dst0inv, dst0inv, dst1inv);

          Face::b32_2x2Unpack0213(src0, src1, READ_32(src));

          OP::prgb32_op_xrgb32_32b(dst0, dst0, src0, dst1, dst1, src1);
          Face::b32_2x2MulDiv255U(dst0, dst0, dst1, dst1, msk0);
          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0 + dst0inv;
        }
        else
        {
          Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));
          Face::b32_2x2Unpack0213(src0, src1, READ_32(src) | 0xFF000000);

          Face::b32_2x2MulDiv255U(src0, src0, msk0, src1, src1, msk0);

          OP::prgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0;
        }

        dst += 4;
        src += 4;
      } while (--w);
    }
    // ------------------------------------------------------------------------

    // [VMask - Alpha - Dense] ------------------------------------------------
    V_BLIT_SPAN8_CASE_VMASK_A_DENSE()
    {
      do {
        Face::b32_1x2 src0, src1;
        Face::b32_1x2 dst0, dst1;
        uint32_t msk0 = READ_8(msk);

        if ((OP::CHARACTERISTICS & OPERATOR_CHAR_UNBOUND) && OP::CHARACTERISTICS & (OPERATOR_CHAR_UNBOUND_MSK_IN))
        {
          Face::b32_1x2 dst0inv, dst1inv;

          Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));
          Face::b32_2x2MulDiv255U(dst0inv, dst0, dst1inv, dst1, Face::b32_1x1Negate255(msk0));
          Face::b32_2x2Pack0213(dst0inv, dst0inv, dst1inv);

          Face::b32_2x2Unpack0213(src0, src1, READ_32(src) | 0xFF000000);
          Face::b32_2x2MulDiv255U(src0, src0, src1, src1, msk0);

          OP::prgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0 + dst0inv;
        }
        else if ((OP::CHARACTERISTICS & OPERATOR_CHAR_UNBOUND))
        {
          Face::b32_1x2 dst0inv, dst1inv;

          Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));
          Face::b32_2x2MulDiv255U(dst0inv, dst0, dst1inv, dst1, Face::b32_1x1Negate255(msk0));
          Face::b32_2x2Pack0213(dst0inv, dst0inv, dst1inv);

          Face::b32_2x2Unpack0213(src0, src1, READ_32(src));

          OP::prgb32_op_xrgb32_32b(dst0, dst0, src0, dst1, dst1, src1);
          Face::b32_2x2MulDiv255U(dst0, dst0, dst1, dst1, msk0);
          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0 + dst0inv;
        }
        else
        {
          Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));
          Face::b32_2x2Unpack0213(src0, src1, READ_32(src) | 0xFF000000);

          Face::b32_2x2MulDiv255U(src0, src0, msk0, src1, src1, msk0);

          OP::prgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0;
        }

        dst += 4;
        src += 4;
        msk += 1;
      } while (--w);
    }
    // ------------------------------------------------------------------------

    // [VMask - Alpha - Sparse] -----------------------------------------------
    V_BLIT_SPAN8_CASE_VMASK_A_SPARSE()
    {
      for (;;)
      {
        Face::b32_1x2 src0, src1;
        Face::b32_1x2 dst0, dst1;

        uint32_t msk0 = READ_8(msk);
        if (msk0 == 0x00) goto vMaskAlphaDenseSkip;

        Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));
        Face::b32_2x2Unpack0213(src0, src1, READ_32(src) | 0xFF000000);
        if (msk0 == 0xFF) goto vMaskAlphaDenseNoMask;

        if ((OP::CHARACTERISTICS & OPERATOR_CHAR_UNBOUND) && OP::CHARACTERISTICS & (OPERATOR_CHAR_UNBOUND_MSK_IN))
        {
          Face::b32_1x2 dst0inv, dst1inv;

          Face::b32_2x2MulDiv255U(dst0inv, dst0, dst1inv, dst1, Face::b32_1x1Negate255(msk0));
          Face::b32_2x2Pack0213(dst0inv, dst0inv, dst1inv);

          Face::b32_2x2MulDiv255U(src0, src0, src1, src1, msk0);

          OP::prgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0 + dst0inv;
        }
        else if ((OP::CHARACTERISTICS & OPERATOR_CHAR_UNBOUND))
        {
          Face::b32_1x2 dst0inv, dst1inv;

          Face::b32_2x2MulDiv255U(dst0inv, dst0, dst1inv, dst1, Face::b32_1x1Negate255(msk0));
          Face::b32_2x2Pack0213(dst0inv, dst0inv, dst1inv);

          OP::prgb32_op_xrgb32_32b(dst0, dst0, src0, dst1, dst1, src1);
          Face::b32_2x2MulDiv255U(dst0, dst0, dst1, dst1, msk0);
          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0 + dst0inv;
        }
        else
        {
          Face::b32_2x2MulDiv255U(src0, src0, msk0, src1, src1, msk0);

          OP::prgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0;
        }

vMaskAlphaDenseSkip:
        dst += 4;
        src += 4;
        msk += 1;

        if (--w) continue;
        break;

vMaskAlphaDenseNoMask:
        OP::prgb32_op_xrgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

        Face::b32_2x2Pack0213(dst0, dst0, dst1);
        ((uint32_t*)dst)[0] = dst0;

        dst += 4;
        src += 4;
        msk += 1;

        if (--w) continue;
        break;
      }
    }
    // ------------------------------------------------------------------------

    // [VMask - ARGB - Dense] -------------------------------------------------
    V_BLIT_SPAN8_CASE_VMASK_ARGB_DENSE()
    {
      // TODO: VMASK-ARGB.
    }
    // ------------------------------------------------------------------------

    // [VMask - ARGB - Sparse] ------------------------------------------------
    V_BLIT_SPAN8_CASE_VMASK_ARGB_SPARSE()
    {
      // TODO: VMASK-ARGB.
    }
    // ------------------------------------------------------------------------

    // [End] ------------------------------------------------------------------
    V_BLIT_SPAN8_END()
    // ------------------------------------------------------------------------
  }

  // --------------------------------------------------------------------------
  // [CompositeBaseFuncsC - XRGB32 - CBlit]
  // --------------------------------------------------------------------------

  static void FOG_FASTCALL xrgb32_cblit_full(
    uint8_t* dst, const RasterSolid* src, int w, const RasterClosure* closure)
  {
    FOG_ASSERT(w > 0);

    Face::b32_1x2 src0, src1;
    Face::b32_2x2Unpack0213(src0, src1, src->prgb);

    do {
      Face::b32_1x2 dst0, dst1;
      Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));

      OP::xrgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

      Face::b32_2x2Pack0213(dst0, dst0, dst1);
      ((uint32_t*)dst)[0] = dst0;

      dst += 4;
    } while (--w);
  }

  static void FOG_FASTCALL xrgb32_cblit_span(
    uint8_t* dst, const RasterSolid* src, const Span* span, const RasterClosure* closure)
  {
    // [Begin] ----------------------------------------------------------------
    Face::b32_1x2 src0orig, src1orig;
    Face::b32_2x2Unpack0213(src0orig, src1orig, src->prgb);

    C_BLIT_SPAN8_BEGIN(4)
    // ------------------------------------------------------------------------

    // [CMask - Opaque] -------------------------------------------------------
    C_BLIT_SPAN8_CASE_CMASK_OPAQUE()
    {
      do {
        Face::b32_1x2 dst0, dst1;
        Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));

        OP::xrgb32_op_prgb32_32b(dst0, dst0, src0orig, dst1, dst1, src1orig);

        Face::b32_2x2Pack0213(dst0, dst0, dst1);
        ((uint32_t*)dst)[0] = dst0;

        dst += 4;
      } while (--w);
    }
    // ------------------------------------------------------------------------

    // [CMask - Alpha] --------------------------------------------------------
    C_BLIT_SPAN8_CASE_CMASK_ALPHA()
    {
      if (OP::CHARACTERISTICS & OPERATOR_CHAR_UNBOUND)
      {
        if (OP::CHARACTERISTICS & OPERATOR_CHAR_UNBOUND_MSK_IN)
        {
          Face::b32_1x2 src0, src1;
          Face::b32_2x2MulDiv255U(src0, src0orig, src1, src1orig, msk0);
          msk0 = 255 - msk0;

          do {
            Face::b32_1x2 dst0, dst1;
            Face::b32_1x2 dst0inv, dst1inv;

            dst0 = READ_32(dst);

            Face::b32_2x2Unpack0213(dst0, dst1, dst0);
            Face::b32_2x2MulDiv255U(dst0inv, dst0, dst1inv, dst1, msk0);
            Face::b32_2x2Pack0213(dst0inv, dst0inv, dst1inv);

            OP::xrgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

            Face::b32_2x2Pack0213(dst0, dst0, dst1);
            ((uint32_t*)dst)[0] = dst0 + dst0inv;

            dst += 4;
          } while (--w);
        }
        else
        {
          do {
            Face::b32_1x2 dst0, dst1;
            Face::b32_1x2 dst0inv, dst1inv;

            dst0 = READ_32(dst);

            Face::b32_2x2Unpack0213(dst0, dst1, dst0);
            Face::b32_2x2MulDiv255U(dst0inv, dst0, dst1inv, dst1, 255 - msk0);
            Face::b32_2x2Pack0213(dst0inv, dst0inv, dst1inv);

            OP::xrgb32_op_prgb32_32b(dst0, dst0, src0orig, dst1, dst1, src1orig);

            Face::b32_2x2MulDiv255U(dst0, dst0, dst1, dst1, msk0);
            Face::b32_2x2Pack0213(dst0, dst0, dst1);
            ((uint32_t*)dst)[0] = dst0 + dst0inv;

            dst += 4;
          } while (--w);
        }
      }
      else
      {
        Face::b32_1x2 src0, src1;
        Face::b32_2x2MulDiv255U(src0, src0orig, src1, src1orig, msk0);

        do {
          Face::b32_1x2 dst0, dst1;

          dst0 = READ_32(dst);

          if (OP::CHARACTERISTICS & (OPERATOR_CHAR_NOP_IF_DST_A_ZERO | OPERATOR_CHAR_NOP_IF_DST_A_FULL))
          {
            if (CompositeHelpersC32<OP::CHARACTERISTICS>::processDstPixel(dst0))
            {
              Face::b32_2x2Unpack0213(dst0, dst1, dst0);

              OP::xrgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

              Face::b32_2x2Pack0213(dst0, dst0, dst1);
              ((uint32_t*)dst)[0] = dst0;
            }
          }
          else
          {
            Face::b32_2x2Unpack0213(dst0, dst1, dst0);

            OP::xrgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

            Face::b32_2x2Pack0213(dst0, dst0, dst1);
            ((uint32_t*)dst)[0] = dst0;
          }

          dst += 4;
        } while (--w);
      }
    }
    // ------------------------------------------------------------------------

    // [VMask - Alpha - Dense] ------------------------------------------------
    C_BLIT_SPAN8_CASE_VMASK_A_DENSE()
    {
      if (OP::CHARACTERISTICS & OPERATOR_CHAR_UNBOUND)
      {
        do {
          Face::b32_1x2 dst0, dst1;
          Face::b32_1x2 dst0inv, dst1inv;
          uint32_t msk0;

          msk0 = READ_8(msk);
          dst0 = READ_32(dst);

          Face::b32_2x2Unpack0213(dst0, dst1, dst0);
          Face::b32_2x2MulDiv255U(dst0inv, dst0, dst1inv, dst1, 255 - msk0);
          Face::b32_2x2Pack0213(dst0inv, dst0inv, dst1inv);

          OP::xrgb32_op_prgb32_32b(dst0, dst0, src0orig, dst1, dst1, src1orig);

          Face::b32_2x2MulDiv255U(dst0, dst0, dst1, dst1, msk0);
          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0 + dst0inv;

          dst += 4;
          msk += 1;
        } while (--w);
      }
      else
      {
        do {
          Face::b32_1x2 dst0, dst1;
          Face::b32_1x2 src0, src1;
          uint32_t msk0;

          dst0 = READ_32(dst);

          if (OP::CHARACTERISTICS & (OPERATOR_CHAR_NOP_IF_DST_A_ZERO | OPERATOR_CHAR_NOP_IF_DST_A_FULL))
          {
            if (!CompositeHelpersC32<OP::CHARACTERISTICS>::processDstPixel(dst0))
              goto vMaskAlphaDenseBoundSkip;
          }

          msk0 = READ_8(msk);

          Face::b32_2x2Unpack0213(dst0, dst1, dst0);
          Face::b32_2x2MulDiv255U(src0, src0orig, src1, src1orig, msk0);

          OP::xrgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0;

vMaskAlphaDenseBoundSkip:
          dst += 4;
          msk += 1;
        } while (--w);
      }
    }
    // ------------------------------------------------------------------------

    // [VMask - Alpha - Sparse] -----------------------------------------------
    C_BLIT_SPAN8_CASE_VMASK_A_SPARSE()
    {
      if (OP::CHARACTERISTICS & OPERATOR_CHAR_UNBOUND)
      {
        for (;;)
        {
          Face::b32_1x2 dst0, dst1;
          Face::b32_1x2 dst0inv, dst1inv;

          uint32_t msk0 = READ_8(msk);
          if (msk0 == 0x00) goto vMaskAlphaSparseUnBoundSkip;

          dst0 = READ_32(dst);
          Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));
          if (msk0 == 0xFF) goto vMaskAlphaSparseUnBoundNoMask;

          Face::b32_2x2MulDiv255U(dst0inv, dst0, dst1inv, dst1, 255 - msk0);
          Face::b32_2x2Pack0213(dst0inv, dst0inv, dst1inv);

          OP::xrgb32_op_prgb32_32b(dst0, dst0, src0orig, dst1, dst1, src1orig);

          Face::b32_2x2MulDiv255U(dst0, dst0, dst1, dst1, msk0);
          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0 + dst0inv;

vMaskAlphaSparseUnBoundSkip:
          dst += 4;
          msk += 1;

          if (--w) continue;
          break;

vMaskAlphaSparseUnBoundNoMask:
          OP::xrgb32_op_prgb32_32b(dst0, dst0, src0orig, dst1, dst1, src1orig);

          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0;

          dst += 4;
          msk += 1;

          if (--w) continue;
          break;
        }
      }
      else
      {
        for (;;)
        {
          Face::b32_1x2 dst0, dst1;
          Face::b32_1x2 src0, src1;

          uint32_t msk0 = READ_8(msk);
          if (msk0 == 0x00) goto vMaskAlphaSparseBoundSkip;

          dst0 = READ_32(dst);

          if (OP::CHARACTERISTICS & (OPERATOR_CHAR_NOP_IF_DST_A_ZERO | OPERATOR_CHAR_NOP_IF_DST_A_FULL))
          {
            if (!CompositeHelpersC32<OP::CHARACTERISTICS>::processDstPixel(dst0)) 
              goto vMaskAlphaSparseBoundSkip;
          }

          Face::b32_2x2Unpack0213(dst0, dst1, dst0);
          if (msk0 == 0xFF) goto vMaskAlphaSparseBoundNoMask;

          Face::b32_2x2MulDiv255U(src0, src0orig, src1, src1orig, msk0);

          OP::xrgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0;

vMaskAlphaSparseBoundSkip:
          dst += 4;
          msk += 1;

          if (--w) continue;
          break;

vMaskAlphaSparseBoundNoMask:
          OP::xrgb32_op_prgb32_32b(dst0, dst0, src0orig, dst1, dst1, src1orig);

          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0;

          dst += 4;
          msk += 1;

          if (--w) continue;
          break;
        }
      }
    }
    // ------------------------------------------------------------------------

    // [VMask - ARGB - Dense] -------------------------------------------------
    C_BLIT_SPAN8_CASE_VMASK_ARGB_DENSE()
    {
      // TODO: VMASK-ARGB.
    }
    // ------------------------------------------------------------------------

    // [VMask - ARGB - Sparse] ------------------------------------------------
    C_BLIT_SPAN8_CASE_VMASK_ARGB_SPARSE()
    {
      // TODO: VMASK-ARGB.
    }
    // ------------------------------------------------------------------------

    // [End] ------------------------------------------------------------------
    C_BLIT_SPAN8_END()
    // ------------------------------------------------------------------------
  }

  // --------------------------------------------------------------------------
  // [CompositeBaseFuncsC - XRGB32 - VBlit - PRGB32]
  // --------------------------------------------------------------------------

  static void FOG_FASTCALL xrgb32_vblit_prgb32_full(
    uint8_t* dst, const uint8_t* src, int w, const RasterClosure* closure)
  {
    FOG_ASSERT(w > 0);

    do {
      Face::b32_1x2 src0, src1;
      Face::b32_1x2 dst0, dst1;

      if (OP::CHARACTERISTICS & (OPERATOR_CHAR_NOP_IF_SRC_A_ZERO | OPERATOR_CHAR_NOP_IF_SRC_A_FULL))
      {
        src0 = READ_32(src);
        if (CompositeHelpersC32<OP::CHARACTERISTICS>::processSrcPixel(src0))
        {
          Face::b32_2x2Unpack0213(src0, src1, src0);
          Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));

          OP::xrgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0;
        }
      }
      else
      {
        Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));
        Face::b32_2x2Unpack0213(src0, src1, READ_32(src));

        OP::xrgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

        Face::b32_2x2Pack0213(dst0, dst0, dst1);
        ((uint32_t*)dst)[0] = dst0;
      }

      dst += 4;
      src += 4;
    } while (--w);
  }

  static void FOG_FASTCALL xrgb32_vblit_prgb32_span(
    uint8_t* dst, const Span* span, const RasterClosure* closure)
  {
    // [Begin] ----------------------------------------------------------------
    V_BLIT_SPAN8_BEGIN(4)
    // ------------------------------------------------------------------------

    // [CMask - Opaque] -------------------------------------------------------
    V_BLIT_SPAN8_CASE_CMASK_OPAQUE()
    {
      do {
        Face::b32_1x2 src0, src1;
        Face::b32_1x2 dst0, dst1;

        if (OP::CHARACTERISTICS & (OPERATOR_CHAR_NOP_IF_SRC_A_ZERO | OPERATOR_CHAR_NOP_IF_SRC_A_FULL))
        {
          src0 = READ_32(src);
          if (CompositeHelpersC32<OP::CHARACTERISTICS>::processSrcPixel(src0))
          {
            Face::b32_2x2Unpack0213(src0, src1, src0);
            Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));

            OP::xrgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

            Face::b32_2x2Pack0213(dst0, dst0, dst1);
            ((uint32_t*)dst)[0] = dst0;
          }
        }
        else
        {
          Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));
          Face::b32_2x2Unpack0213(src0, src1, READ_32(src));

          OP::xrgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0;
        }

        dst += 4;
        src += 4;
      } while (--w);
    }
    // ------------------------------------------------------------------------

    // [CMask - Alpha] --------------------------------------------------------
    V_BLIT_SPAN8_CASE_CMASK_ALPHA()
    {
      uint32_t msk0inv;
      if ((OP::CHARACTERISTICS & OPERATOR_CHAR_UNBOUND)) msk0inv = Face::b32_1x1Negate255(msk0);

      do {
        Face::b32_1x2 src0, src1;
        Face::b32_1x2 dst0, dst1;

        if ((OP::CHARACTERISTICS & OPERATOR_CHAR_UNBOUND) && OP::CHARACTERISTICS & (OPERATOR_CHAR_UNBOUND_MSK_IN))
        {
          Face::b32_1x2 dst0inv, dst1inv;

          Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));
          Face::b32_2x2MulDiv255U(dst0inv, dst0, dst1inv, dst1, msk0inv);
          Face::b32_2x2Pack0213(dst0inv, dst0inv, dst1inv);

          Face::b32_2x2Unpack0213(src0, src1, READ_32(src));
          Face::b32_2x2MulDiv255U(src0, src0, src1, src1, msk0);

          OP::xrgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0 + dst0inv;
        }
        else if ((OP::CHARACTERISTICS & OPERATOR_CHAR_UNBOUND))
        {
          Face::b32_1x2 dst0inv, dst1inv;

          Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));
          Face::b32_2x2MulDiv255U(dst0inv, dst0, dst1inv, dst1, msk0inv);
          Face::b32_2x2Pack0213(dst0inv, dst0inv, dst1inv);

          Face::b32_2x2Unpack0213(src0, src1, READ_32(src));

          OP::xrgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);
          Face::b32_2x2MulDiv255U(dst0, dst0, dst1, dst1, msk0);
          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0 + dst0inv;
        }
        else
        {
          Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));
          Face::b32_2x2Unpack0213(src0, src1, READ_32(src));

          Face::b32_2x2MulDiv255U(src0, src0, msk0, src1, src1, msk0);

          OP::xrgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0;
        }

        dst += 4;
        src += 4;
      } while (--w);
    }
    // ------------------------------------------------------------------------

    // [VMask - Alpha - Dense] ------------------------------------------------
    V_BLIT_SPAN8_CASE_VMASK_A_DENSE()
    {
      do {
        Face::b32_1x2 src0, src1;
        Face::b32_1x2 dst0, dst1;
        uint32_t msk0 = READ_8(msk);

        if ((OP::CHARACTERISTICS & OPERATOR_CHAR_UNBOUND) && OP::CHARACTERISTICS & (OPERATOR_CHAR_UNBOUND_MSK_IN))
        {
          Face::b32_1x2 dst0inv, dst1inv;

          Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));
          Face::b32_2x2MulDiv255U(dst0inv, dst0, dst1inv, dst1, Face::b32_1x1Negate255(msk0));
          Face::b32_2x2Pack0213(dst0inv, dst0inv, dst1inv);

          Face::b32_2x2Unpack0213(src0, src1, READ_32(src));
          Face::b32_2x2MulDiv255U(src0, src0, src1, src1, msk0);

          OP::xrgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0 + dst0inv;
        }
        else if ((OP::CHARACTERISTICS & OPERATOR_CHAR_UNBOUND))
        {
          Face::b32_1x2 dst0inv, dst1inv;

          Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));
          Face::b32_2x2MulDiv255U(dst0inv, dst0, dst1inv, dst1, Face::b32_1x1Negate255(msk0));
          Face::b32_2x2Pack0213(dst0inv, dst0inv, dst1inv);

          Face::b32_2x2Unpack0213(src0, src1, READ_32(src));

          OP::xrgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);
          Face::b32_2x2MulDiv255U(dst0, dst0, dst1, dst1, msk0);
          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0 + dst0inv;
        }
        else
        {
          Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));
          Face::b32_2x2Unpack0213(src0, src1, READ_32(src));

          Face::b32_2x2MulDiv255U(src0, src0, msk0, src1, src1, msk0);

          OP::xrgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0;
        }

        dst += 4;
        src += 4;
        msk += 1;
      } while (--w);
    }
    // ------------------------------------------------------------------------

    // [VMask - Alpha - Sparse] -----------------------------------------------
    V_BLIT_SPAN8_CASE_VMASK_A_SPARSE()
    {
      for (;;)
      {
        Face::b32_1x2 src0, src1;
        Face::b32_1x2 dst0, dst1;

        uint32_t msk0 = READ_8(msk);
        if (msk0 == 0x00) goto vMaskAlphaSparseSkip;

        Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));
        Face::b32_2x2Unpack0213(src0, src1, READ_32(src));
        if (msk0 == 0xFF) goto vMaskAlphaSparseNoMask;

        if ((OP::CHARACTERISTICS & OPERATOR_CHAR_UNBOUND) && OP::CHARACTERISTICS & (OPERATOR_CHAR_UNBOUND_MSK_IN))
        {
          Face::b32_1x2 dst0inv, dst1inv;

          Face::b32_2x2MulDiv255U(dst0inv, dst0, dst1inv, dst1, Face::b32_1x1Negate255(msk0));
          Face::b32_2x2Pack0213(dst0inv, dst0inv, dst1inv);

          Face::b32_2x2MulDiv255U(src0, src0, src1, src1, msk0);

          OP::xrgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0 + dst0inv;
        }
        else if ((OP::CHARACTERISTICS & OPERATOR_CHAR_UNBOUND))
        {
          Face::b32_1x2 dst0inv, dst1inv;

          Face::b32_2x2MulDiv255U(dst0inv, dst0, dst1inv, dst1, Face::b32_1x1Negate255(msk0));
          Face::b32_2x2Pack0213(dst0inv, dst0inv, dst1inv);

          OP::xrgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

          Face::b32_2x2MulDiv255U(dst0, dst0, dst1, dst1, msk0);
          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0 + dst0inv;
        }
        else
        {
          Face::b32_2x2MulDiv255U(src0, src0, msk0, src1, src1, msk0);

          OP::xrgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0;
        }

vMaskAlphaSparseSkip:
        dst += 4;
        src += 4;
        msk += 1;

        if (--w) continue;
        break;

vMaskAlphaSparseNoMask:
        OP::xrgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

        Face::b32_2x2Pack0213(dst0, dst0, dst1);
        ((uint32_t*)dst)[0] = dst0;

        dst += 4;
        src += 4;
        msk += 1;

        if (--w) continue;
        break;
      }
    }
    // ------------------------------------------------------------------------

    // [VMask - ARGB - Dense] -------------------------------------------------
    V_BLIT_SPAN8_CASE_VMASK_ARGB_DENSE()
    {
      // TODO: VMASK-ARGB.
    }
    // ------------------------------------------------------------------------

    // [VMask - ARGB - Sparse] ------------------------------------------------
    V_BLIT_SPAN8_CASE_VMASK_ARGB_SPARSE()
    {
      // TODO: VMASK-ARGB.
    }
    // ------------------------------------------------------------------------

    // [End] ------------------------------------------------------------------
    V_BLIT_SPAN8_END()
    // ------------------------------------------------------------------------
  }

  // --------------------------------------------------------------------------
  // [CompositeBaseFuncsC - XRGB32 - VBlit - ARGB32]
  // --------------------------------------------------------------------------

  static void FOG_FASTCALL xrgb32_vblit_argb32_full(
    uint8_t* dst, const uint8_t* src, int w, const RasterClosure* closure)
  {
    FOG_ASSERT(w > 0);

    do {
      Face::b32_1x2 src0, src1;
      Face::b32_1x2 dst0, dst1;

      if (OP::CHARACTERISTICS & (OPERATOR_CHAR_NOP_IF_SRC_A_ZERO | OPERATOR_CHAR_NOP_IF_SRC_A_FULL))
      {
        src0 = READ_32(src);
        if (CompositeHelpersC32<OP::CHARACTERISTICS>::processSrcPixel(src0))
        {
          Face::b32_2x2Unpack0213(src0, src1, src0);
          Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));

          // Premultiply only if source colors are used.
          if (OP::CHARACTERISTICS & OPERATOR_CHAR_SRC_C_USED) Face::b32_2x2PremultiplyA(src0, src0, src1, src1);

          OP::xrgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0;
        }
      }
      else
      {
        Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));
        Face::b32_2x2Unpack0213(src0, src1, READ_32(src));

        // Premultiply only if source colors are used.
        if (OP::CHARACTERISTICS & OPERATOR_CHAR_SRC_C_USED) Face::b32_2x2PremultiplyA(src0, src0, src1, src1);

        OP::xrgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

        Face::b32_2x2Pack0213(dst0, dst0, dst1);
        ((uint32_t*)dst)[0] = dst0;
      }

      dst += 4;
      src += 4;
    } while (--w);
  }

  static void FOG_FASTCALL xrgb32_vblit_argb32_span(
    uint8_t* dst, const Span* span, const RasterClosure* closure)
  {
    // [Begin] ----------------------------------------------------------------
    V_BLIT_SPAN8_BEGIN(4)
    // ------------------------------------------------------------------------

    // [CMask - Opaque] -------------------------------------------------------
    V_BLIT_SPAN8_CASE_CMASK_OPAQUE()
    {
      do {
        Face::b32_1x2 src0, src1;
        Face::b32_1x2 dst0, dst1;

        if (OP::CHARACTERISTICS & (OPERATOR_CHAR_NOP_IF_SRC_A_ZERO | OPERATOR_CHAR_NOP_IF_SRC_A_FULL))
        {
          src0 = READ_32(src);
          if (CompositeHelpersC32<OP::CHARACTERISTICS>::processSrcPixel(src0))
          {
            Face::b32_2x2Unpack0213(src0, src1, src0);
            Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));

            // Premultiply only if source colors are used.
            if (OP::CHARACTERISTICS & OPERATOR_CHAR_SRC_C_USED) Face::b32_2x2PremultiplyA(src0, src0, src1, src1);

            OP::xrgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

            Face::b32_2x2Pack0213(dst0, dst0, dst1);
            ((uint32_t*)dst)[0] = dst0;
          }
        }
        else
        {
          Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));
          Face::b32_2x2Unpack0213(src0, src1, READ_32(src));

          // Premultiply only if source colors are used.
          if (OP::CHARACTERISTICS & OPERATOR_CHAR_SRC_C_USED) Face::b32_2x2PremultiplyA(src0, src0, src1, src1);

          OP::xrgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0;
        }

        dst += 4;
        src += 4;
      } while (--w);
    }
    // ------------------------------------------------------------------------

    // [CMask - Alpha] --------------------------------------------------------
    V_BLIT_SPAN8_CASE_CMASK_ALPHA()
    {
      uint32_t msk0inv;
      if ((OP::CHARACTERISTICS & OPERATOR_CHAR_UNBOUND)) msk0inv = Face::b32_1x1Negate255(msk0);

      do {
        Face::b32_1x2 src0, src1;
        Face::b32_1x2 dst0, dst1;

        if ((OP::CHARACTERISTICS & OPERATOR_CHAR_UNBOUND) && OP::CHARACTERISTICS & (OPERATOR_CHAR_UNBOUND_MSK_IN))
        {
          Face::b32_1x2 dst0inv, dst1inv;

          Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));
          Face::b32_2x2MulDiv255U(dst0inv, dst0, dst1inv, dst1, msk0inv);
          Face::b32_2x2Pack0213(dst0inv, dst0inv, dst1inv);

          Face::b32_2x2Unpack0213(src0, src1, READ_32(src));
          Face::b32_2x2PremultiplyU(src0, src0, src1, src1, Face::b32_1x1MulDiv255(Face::b32_1x2GetB1(src1), msk0));

          OP::xrgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0 + dst0inv;
        }
        else if ((OP::CHARACTERISTICS & OPERATOR_CHAR_UNBOUND))
        {
          Face::b32_1x2 dst0inv, dst1inv;

          Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));
          Face::b32_2x2MulDiv255U(dst0inv, dst0, dst1inv, dst1, msk0inv);
          Face::b32_2x2Pack0213(dst0inv, dst0inv, dst1inv);

          Face::b32_2x2Unpack0213(src0, src1, READ_32(src));
          Face::b32_2x2PremultiplyA(src0, src0, src1, src1);

          OP::xrgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);
          Face::b32_2x2MulDiv255U(dst0, dst0, dst1, dst1, msk0);
          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0 + dst0inv;
        }
        else
        {
          Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));
          Face::b32_2x2Unpack0213(src0, src1, READ_32(src));
          Face::b32_2x2PremultiplyU(src0, src0, src1, src1, Face::b32_1x1MulDiv255(Face::b32_1x2GetB1(src1), msk0));

          OP::xrgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0;
        }

        dst += 4;
        src += 4;
      } while (--w);
    }
    // ------------------------------------------------------------------------

    // [VMask - Alpha - Dense] ------------------------------------------------
    V_BLIT_SPAN8_CASE_VMASK_A_DENSE()
    {
      do {
        Face::b32_1x2 src0, src1;
        Face::b32_1x2 dst0, dst1;
        uint32_t msk0 = READ_8(msk);

        if ((OP::CHARACTERISTICS & OPERATOR_CHAR_UNBOUND) && OP::CHARACTERISTICS & (OPERATOR_CHAR_UNBOUND_MSK_IN))
        {
          Face::b32_1x2 dst0inv, dst1inv;

          Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));
          Face::b32_2x2MulDiv255U(dst0inv, dst0, dst1inv, dst1, Face::b32_1x1Negate255(msk0));
          Face::b32_2x2Pack0213(dst0inv, dst0inv, dst1inv);

          Face::b32_2x2Unpack0213(src0, src1, READ_32(src));
          Face::b32_2x2PremultiplyU(src0, src0, src1, src1, Face::b32_1x1MulDiv255(Face::b32_1x2GetB1(src1), msk0));

          OP::xrgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0 + dst0inv;
        }
        else if ((OP::CHARACTERISTICS & OPERATOR_CHAR_UNBOUND))
        {
          Face::b32_1x2 dst0inv, dst1inv;

          Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));
          Face::b32_2x2MulDiv255U(dst0inv, dst0, dst1inv, dst1, Face::b32_1x1Negate255(msk0));
          Face::b32_2x2Pack0213(dst0inv, dst0inv, dst1inv);

          Face::b32_2x2Unpack0213(src0, src1, READ_32(src));
          Face::b32_2x2PremultiplyA(src0, src0, src1, src1);

          OP::xrgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);
          Face::b32_2x2MulDiv255U(dst0, dst0, dst1, dst1, msk0);
          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0 + dst0inv;
        }
        else
        {
          Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));
          Face::b32_2x2Unpack0213(src0, src1, READ_32(src));
          Face::b32_2x2PremultiplyU(src0, src0, src1, src1, Face::b32_1x1MulDiv255(Face::b32_1x2GetB1(src1), msk0));

          OP::xrgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0;
        }

        dst += 4;
        src += 4;
        msk += 1;
      } while (--w);
    }
    // ------------------------------------------------------------------------

    // [VMask - Alpha - Sparse] -----------------------------------------------
    V_BLIT_SPAN8_CASE_VMASK_A_SPARSE()
    {
      for (;;)
      {
        Face::b32_1x2 src0, src1;
        Face::b32_1x2 dst0, dst1;

        uint32_t msk0 = READ_8(msk);
        if (msk0 == 0x00) goto vMaskAlphaSparseSkip;

        Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));
        Face::b32_2x2Unpack0213(src0, src1, READ_32(src));
        if (msk0 == 0xFF) goto vMaskAlphaSparseSkip;

        if ((OP::CHARACTERISTICS & OPERATOR_CHAR_UNBOUND) && OP::CHARACTERISTICS & (OPERATOR_CHAR_UNBOUND_MSK_IN))
        {
          Face::b32_1x2 dst0inv, dst1inv;

          Face::b32_2x2MulDiv255U(dst0inv, dst0, dst1inv, dst1, Face::b32_1x1Negate255(msk0));
          Face::b32_2x2Pack0213(dst0inv, dst0inv, dst1inv);
          Face::b32_2x2PremultiplyU(src0, src0, src1, src1, Face::b32_1x1MulDiv255(Face::b32_1x2GetB1(src1), msk0));

          OP::xrgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0 + dst0inv;
        }
        else if ((OP::CHARACTERISTICS & OPERATOR_CHAR_UNBOUND))
        {
          Face::b32_1x2 dst0inv, dst1inv;

          Face::b32_2x2MulDiv255U(dst0inv, dst0, dst1inv, dst1, Face::b32_1x1Negate255(msk0));
          Face::b32_2x2Pack0213(dst0inv, dst0inv, dst1inv);
          Face::b32_2x2PremultiplyA(src0, src0, src1, src1);

          OP::xrgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);
          Face::b32_2x2MulDiv255U(dst0, dst0, dst1, dst1, msk0);
          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0 + dst0inv;
        }
        else
        {
          Face::b32_2x2PremultiplyU(src0, src0, src1, src1, Face::b32_1x1MulDiv255(Face::b32_1x2GetB1(src1), msk0));

          OP::xrgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0;
        }

vMaskAlphaSparseSkip:
        dst += 4;
        src += 4;
        msk += 1;

        if (--w) continue;
        break;

vMaskAlphaSparseNoMask:
        Face::b32_2x2PremultiplyA(src0, src0, src1, src1);
        OP::xrgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

        Face::b32_2x2Pack0213(dst0, dst0, dst1);
        ((uint32_t*)dst)[0] = dst0;

        if (--w) continue;
        break;
      }
    }
    // ------------------------------------------------------------------------

    // [VMask - ARGB - Dense] -------------------------------------------------
    V_BLIT_SPAN8_CASE_VMASK_ARGB_DENSE()
    {
      // TODO: VMASK-ARGB.
    }
    // ------------------------------------------------------------------------

    // [VMask - ARGB - Sparse] ------------------------------------------------
    V_BLIT_SPAN8_CASE_VMASK_ARGB_SPARSE()
    {
      // TODO: VMASK-ARGB.
    }
    // ------------------------------------------------------------------------

    // [End] ------------------------------------------------------------------
    V_BLIT_SPAN8_END()
    // ------------------------------------------------------------------------
  }

  // --------------------------------------------------------------------------
  // [CompositeBaseFuncsC - XRGB32 - VBlit - XRGB32]
  // --------------------------------------------------------------------------

  static void FOG_FASTCALL xrgb32_vblit_xrgb32_full(
    uint8_t* dst, const uint8_t* src, int w, const RasterClosure* closure)
  {
    FOG_ASSERT(w > 0);

    do {
      Face::b32_1x2 src0, src1;
      Face::b32_1x2 dst0, dst1;

      Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));
      Face::b32_2x2Unpack0213(src0, src1, READ_32(src));

      OP::xrgb32_op_xrgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

      Face::b32_2x2Pack0213(dst0, dst0, dst1);
      ((uint32_t*)dst)[0] = dst0;

      dst += 4;
      src += 4;
    } while (--w);
  }

  static void FOG_FASTCALL xrgb32_vblit_xrgb32_span(
    uint8_t* dst, const Span* span, const RasterClosure* closure)
  {
    // [Begin] ----------------------------------------------------------------
    V_BLIT_SPAN8_BEGIN(4)
    // ------------------------------------------------------------------------

    // [CMask - Opaque] -------------------------------------------------------
    V_BLIT_SPAN8_CASE_CMASK_OPAQUE()
    {
      do {
        Face::b32_1x2 src0, src1;
        Face::b32_1x2 dst0, dst1;

        Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));
        Face::b32_2x2Unpack0213(src0, src1, READ_32(src));

        OP::xrgb32_op_xrgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

        Face::b32_2x2Pack0213(dst0, dst0, dst1);
        ((uint32_t*)dst)[0] = dst0;

        dst += 4;
        src += 4;
      } while (--w);
    }
    // ------------------------------------------------------------------------

    // [CMask - Alpha] --------------------------------------------------------
    V_BLIT_SPAN8_CASE_CMASK_ALPHA()
    {
      uint32_t msk0inv;
      if ((OP::CHARACTERISTICS & OPERATOR_CHAR_UNBOUND)) msk0inv = Face::b32_1x1Negate255(msk0);

      do {
        Face::b32_1x2 src0, src1;
        Face::b32_1x2 dst0, dst1;

        if ((OP::CHARACTERISTICS & OPERATOR_CHAR_UNBOUND) && OP::CHARACTERISTICS & (OPERATOR_CHAR_UNBOUND_MSK_IN))
        {
          Face::b32_1x2 dst0inv, dst1inv;

          Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));
          Face::b32_2x2MulDiv255U(dst0inv, dst0, dst1inv, dst1, msk0inv);
          Face::b32_2x2Pack0213(dst0inv, dst0inv, dst1inv);

          Face::b32_2x2Unpack0213(src0, src1, READ_32(src) | 0xFF000000);
          Face::b32_2x2MulDiv255U(src0, src0, src1, src1, msk0);

          OP::xrgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0 + dst0inv;
        }
        else if ((OP::CHARACTERISTICS & OPERATOR_CHAR_UNBOUND))
        {
          Face::b32_1x2 dst0inv, dst1inv;

          Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));
          Face::b32_2x2MulDiv255U(dst0inv, dst0, dst1inv, dst1, msk0inv);
          Face::b32_2x2Pack0213(dst0inv, dst0inv, dst1inv);

          Face::b32_2x2Unpack0213(src0, src1, READ_32(src));

          OP::xrgb32_op_xrgb32_32b(dst0, dst0, src0, dst1, dst1, src1);
          Face::b32_2x2MulDiv255U(dst0, dst0, dst1, dst1, msk0);
          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0 + dst0inv;
        }
        else
        {
          Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));
          Face::b32_2x2Unpack0213(src0, src1, READ_32(src) | 0xFF000000);

          Face::b32_2x2MulDiv255U(src0, src0, msk0, src1, src1, msk0);

          OP::xrgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0;
        }

        dst += 4;
        src += 4;
      } while (--w);
    }
    // ------------------------------------------------------------------------

    // [VMask - Alpha - Dense] ------------------------------------------------
    V_BLIT_SPAN8_CASE_VMASK_A_DENSE()
    {
      do {
        Face::b32_1x2 src0, src1;
        Face::b32_1x2 dst0, dst1;
        uint32_t msk0 = READ_8(msk);

        if ((OP::CHARACTERISTICS & OPERATOR_CHAR_UNBOUND) && OP::CHARACTERISTICS & (OPERATOR_CHAR_UNBOUND_MSK_IN))
        {
          Face::b32_1x2 dst0inv, dst1inv;

          Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));
          Face::b32_2x2MulDiv255U(dst0inv, dst0, dst1inv, dst1, Face::b32_1x1Negate255(msk0));
          Face::b32_2x2Pack0213(dst0inv, dst0inv, dst1inv);

          Face::b32_2x2Unpack0213(src0, src1, READ_32(src) | 0xFF000000);
          Face::b32_2x2MulDiv255U(src0, src0, src1, src1, msk0);

          OP::xrgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0 + dst0inv;
        }
        else if ((OP::CHARACTERISTICS & OPERATOR_CHAR_UNBOUND))
        {
          Face::b32_1x2 dst0inv, dst1inv;

          Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));
          Face::b32_2x2MulDiv255U(dst0inv, dst0, dst1inv, dst1, Face::b32_1x1Negate255(msk0));
          Face::b32_2x2Pack0213(dst0inv, dst0inv, dst1inv);

          Face::b32_2x2Unpack0213(src0, src1, READ_32(src));

          OP::xrgb32_op_xrgb32_32b(dst0, dst0, src0, dst1, dst1, src1);
          Face::b32_2x2MulDiv255U(dst0, dst0, dst1, dst1, msk0);
          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0 + dst0inv;
        }
        else
        {
          Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));
          Face::b32_2x2Unpack0213(src0, src1, READ_32(src) | 0xFF000000);

          Face::b32_2x2MulDiv255U(src0, src0, msk0, src1, src1, msk0);

          OP::xrgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0;
        }

        dst += 4;
        src += 4;
        msk += 1;
      } while (--w);
    }
    // ------------------------------------------------------------------------

    // [VMask - Alpha - Sparse] -----------------------------------------------
    V_BLIT_SPAN8_CASE_VMASK_A_SPARSE()
    {
      for (;;)
      {
        Face::b32_1x2 src0, src1;
        Face::b32_1x2 dst0, dst1;

        uint32_t msk0 = READ_8(msk);
        if (msk0 == 0x00) goto vMaskAlphaSparseSkip;

        Face::b32_2x2Unpack0213(dst0, dst1, READ_32(dst));
        Face::b32_2x2Unpack0213(src0, src1, READ_32(src) | 0xFF000000);
        if (msk0 == 0xFF) goto vMaskAlphaSparseNoMask;

        if ((OP::CHARACTERISTICS & OPERATOR_CHAR_UNBOUND) && OP::CHARACTERISTICS & (OPERATOR_CHAR_UNBOUND_MSK_IN))
        {
          Face::b32_1x2 dst0inv, dst1inv;

          Face::b32_2x2MulDiv255U(dst0inv, dst0, dst1inv, dst1, Face::b32_1x1Negate255(msk0));
          Face::b32_2x2Pack0213(dst0inv, dst0inv, dst1inv);
          Face::b32_2x2MulDiv255U(src0, src0, src1, src1, msk0);

          OP::xrgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0 + dst0inv;
        }
        else if ((OP::CHARACTERISTICS & OPERATOR_CHAR_UNBOUND))
        {
          Face::b32_1x2 dst0inv, dst1inv;

          Face::b32_2x2MulDiv255U(dst0inv, dst0, dst1inv, dst1, Face::b32_1x1Negate255(msk0));
          Face::b32_2x2Pack0213(dst0inv, dst0inv, dst1inv);

          OP::xrgb32_op_xrgb32_32b(dst0, dst0, src0, dst1, dst1, src1);
          Face::b32_2x2MulDiv255U(dst0, dst0, dst1, dst1, msk0);
          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0 + dst0inv;
        }
        else
        {
          Face::b32_2x2MulDiv255U(src0, src0, msk0, src1, src1, msk0);

          OP::xrgb32_op_prgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

          Face::b32_2x2Pack0213(dst0, dst0, dst1);
          ((uint32_t*)dst)[0] = dst0;
        }

vMaskAlphaSparseSkip:
        dst += 4;
        src += 4;
        msk += 1;

        if (--w) continue;
        break;

vMaskAlphaSparseNoMask:
        dst += 4;
        src += 4;
        msk += 1;

        OP::xrgb32_op_xrgb32_32b(dst0, dst0, src0, dst1, dst1, src1);

        Face::b32_2x2Pack0213(dst0, dst0, dst1);
        ((uint32_t*)dst)[0] = dst0;

        if (--w) continue;
        break;
      }
    }
    // ------------------------------------------------------------------------

    // [VMask - ARGB - Dense] -------------------------------------------------
    V_BLIT_SPAN8_CASE_VMASK_ARGB_DENSE()
    {
      // TODO: VMASK-ARGB.
    }
    // ------------------------------------------------------------------------

    // [VMask - ARGB - Sparse] ------------------------------------------------
    V_BLIT_SPAN8_CASE_VMASK_ARGB_SPARSE()
    {
      // TODO: VMASK-ARGB.
    }
    // ------------------------------------------------------------------------

    // [End] ------------------------------------------------------------------
    V_BLIT_SPAN8_END()
    // ------------------------------------------------------------------------
  }
};

} // RasterEngine namespace
} // Fog namespace
