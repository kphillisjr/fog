// [Fog-Graphics]
//
// [License]
// MIT, See COPYING file in package

// [Guard]
#ifndef _FOG_GRAPHICS_PAINTENGINE_RASTERPAINTCALC_P_H
#define _FOG_GRAPHICS_PAINTENGINE_RASTERPAINTCALC_P_H

// [Dependencies]
#include <Fog/Core/Assert.h>
#include <Fog/Core/Atomic.h>
#include <Fog/Core/Static.h>
#include <Fog/Graphics/Constants.h>
#include <Fog/Graphics/Geometry.h>
#include <Fog/Graphics/Path.h>
#include <Fog/Graphics/PathStroker.h>
#include <Fog/Graphics/Transform.h>

#include <Fog/Graphics/PaintEngine/RasterPaintAction_p.h>
#include <Fog/Graphics/PaintEngine/RasterPaintBase_p.h>

namespace Fog {

//! @addtogroup Fog_Graphics_Private
//! @{

// ============================================================================
// [Forward Declarations]
// ============================================================================

struct RasterPaintCmd;
struct RasterPaintEngine;

// ============================================================================
// [Fog::RasterPaintCalc]
// ============================================================================

//! @internal
//!
//! @brief Calculation
//!
//! Calculation is complex action (usually sequental) that can be run only by
//! one thread. To improve performance ofsuch calculations raster paint engine
//! can distribute them to different threads.
struct FOG_HIDDEN RasterPaintCalc : public RasterPaintAction
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  FOG_INLINE RasterPaintCalc() { wakeUpWorkers.init(0); };
  FOG_INLINE ~RasterPaintCalc() {};

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! @brief Related command to this calculation.
  RasterPaintCmd* relatedTo;

  //! @brief Whether to wake up workers when calculation finishes (can by set
  //! by worker).
  Atomic<uint> wakeUpWorkers;
};

// ============================================================================
// [Fog::RasterPaintCalcPath]
// ============================================================================

//! @internal
struct FOG_HIDDEN RasterPaintCalcFillPath : public RasterPaintCalc
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  FOG_INLINE RasterPaintCalcFillPath() {};
  FOG_INLINE ~RasterPaintCalcFillPath() {};

  // --------------------------------------------------------------------------
  // [Methods]
  // --------------------------------------------------------------------------

  virtual void run(RasterPaintContext* ctx);
  virtual void release(RasterPaintContext* ctx);

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! @brief Path to process.
  Static<PathD> path;

  //! @brief Transformation matrix.
  Static<TransformD> transform;

  //! @brief Clip box.
  BoxI clipBox;

  //! @brief Transformation type (see @c RASTER_TRANSFORM_TYPE).
  uint32_t transformType;

  //! @brief Fill rule.
  uint32_t fillRule;

  //! @brief Approximation scale.
  double approximationScale;
};

// ============================================================================
// [Fog::RasterPaintCalcStrokePath]
// ============================================================================

//! @internal
struct FOG_HIDDEN RasterPaintCalcStrokePath : public RasterPaintCalc
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  FOG_INLINE RasterPaintCalcStrokePath() {};
  FOG_INLINE ~RasterPaintCalcStrokePath() {};

  // --------------------------------------------------------------------------
  // [Methods]
  // --------------------------------------------------------------------------

  virtual void run(RasterPaintContext* ctx);
  virtual void release(RasterPaintContext* ctx);

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! @brief Owner raster paint engine.
  RasterPaintEngine* engine;

  //! @brief Path to process.
  Static<PathD> path;

  //! @brief Transformation matrix.
  Static<TransformD> transform;

  //! @brief Clip box.
  BoxI clipBox;

  //! @brief Transformation type (see @c RASTER_TRANSFORM_TYPE).
  uint32_t transformType;

  //! @brief Stroker.
  Static<PathStroker> stroker;
};

//! @}

} // Fog namespace

// [Guard]
#endif // _FOG_GRAPHICS_PAINTENGINE_RASTERPAINTCALC_P_H
