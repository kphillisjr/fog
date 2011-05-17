// [Fog-G2d]
//
// [License]
// MIT, See COPYING file in package

// [Precompiled Headers]
#if defined(FOG_PRECOMP)
#include FOG_PRECOMP
#endif // FOG_PRECOMP

// [Dependencies]
#include <Fog/Core/Global/Assert.h>
#include <Fog/Core/Global/Internal_Core_p.h>
#include <Fog/G2d/Geometry/Shape.h>
#include <Fog/G2d/Geometry/Path.h>
#include <Fog/G2d/Geometry/PathTmp_p.h>
#include <Fog/G2d/Geometry/Transform.h>
#include <Fog/G2d/Global/Api.h>
#include <Fog/G2d/Global/Init_G2d_p.h>

namespace Fog {

// ============================================================================
// [Fog::Shape - GetBoundingBox]
// ============================================================================

template<typename NumT>
static err_t FOG_CDECL _G2d_ShapeT_getBoundingBox(uint32_t shapeType, const void* shapeData,
  NumT_(Box)* dst, const NumT_(Transform)* transform)
{
  NumT_(Point) tmp[8];
  uint32_t transformType = TRANSFORM_TYPE_IDENTITY;
  
  if (transform)
  {
    transformType = transform->getType();
    if (transformType == TRANSFORM_TYPE_DEGENERATE)
    {
      dst->reset();
      return ERR_GEOMETRY_DEGENERATE;
    }
  }
    
  switch (shapeType)
  {
    case SHAPE_TYPE_NONE:
      dst->reset();
      return ERR_GEOMETRY_NONE;

    case SHAPE_TYPE_LINE:
      *dst = reinterpret_cast<const NumT_(Line)*>(shapeData)->getBoundingBox();
      if (transform) transform->mapBox(*dst, *dst);
      return ERR_OK;

    case SHAPE_TYPE_QUAD:
      if (transformType != TRANSFORM_TYPE_IDENTITY)
      {
        if (transformType >= TRANSFORM_TYPE_PROJECTION)
        {
          static const uint8_t cmd[4] =
          { 
            PATH_CMD_MOVE_TO, 
            PATH_CMD_QUAD_TO, 
            PATH_CMD_QUAD_TO, 
            0xFF 
          };

          NumT_T1(PathTmp, 16) tmp;
          transform->mapPathData(tmp, cmd, reinterpret_cast<const NumT_(Point)*>(shapeData), 3);
          return tmp.getBoundingBox(*dst);
        }

        transform->_mapPoints(tmp, reinterpret_cast<const NumT_(Point)*>(shapeData), 3);
        shapeData = &tmp;
      }

      FOG_RETURN_ON_ERROR(reinterpret_cast<const NumT_(QuadCurve)*>(shapeData)->getBoundingBox(*dst));
      return ERR_OK;

    case SHAPE_TYPE_CUBIC:
      if (transformType != TRANSFORM_TYPE_IDENTITY)
      {
        if (transformType >= TRANSFORM_TYPE_PROJECTION)
        {
          static const uint8_t cmd[4] =
          {
            PATH_CMD_MOVE_TO, 
            PATH_CMD_CUBIC_TO, 
            PATH_CMD_CUBIC_TO, 
            PATH_CMD_CUBIC_TO
          };

          NumT_T1(PathTmp, 16) tmp;
          transform->mapPathData(tmp, cmd, reinterpret_cast<const NumT_(Point)*>(shapeData), 4);
          return tmp.getBoundingBox(*dst);
        }

        transform->_mapPoints(tmp, reinterpret_cast<const NumT_(Point)*>(shapeData), 4);
        shapeData = &tmp;
      }

      FOG_RETURN_ON_ERROR(reinterpret_cast<const NumT_(CubicCurve)*>(shapeData)->getBoundingBox(*dst));
      return ERR_OK;

    case SHAPE_TYPE_ARC:
      return reinterpret_cast<const NumT_(Arc)*>(shapeData)->_getBoundingBox(*dst, transform);

    case SHAPE_TYPE_RECT:
      *dst = *reinterpret_cast<const NumT_(Rect)*>(shapeData);
      if (transform) transform->mapBox(*dst, *dst);
      return ERR_OK;

    case SHAPE_TYPE_ROUND:
      return reinterpret_cast<const NumT_(Round)*>(shapeData)->_getBoundingBox(*dst, transform);

    case SHAPE_TYPE_CIRCLE:
      reinterpret_cast<const NumT_(Circle)*>(shapeData)->_getBoundingBox(*dst, transform);

    case SHAPE_TYPE_ELLIPSE:
      return reinterpret_cast<const NumT_(Ellipse)*>(shapeData)->_getBoundingBox(*dst, transform);

    case SHAPE_TYPE_CHORD:
      return reinterpret_cast<const NumT_(Chord)*>(shapeData)->_getBoundingBox(*dst, transform);

    case SHAPE_TYPE_PIE:
      return reinterpret_cast<const NumT_(Pie)*>(shapeData)->_getBoundingBox(*dst, transform);

    default:
      dst->reset();
      return ERR_RT_INVALID_ARGUMENT;
  }
}

// ============================================================================
// [Fog::Shape - HitTest]
// ============================================================================

template<typename NumT>
static bool FOG_CDECL _G2d_ShapeT_hitTest(uint32_t shapeType, const void* shapeData, const NumT_(Point)* pt)
{
  switch (shapeType)
  {
    case SHAPE_TYPE_NONE:
    case SHAPE_TYPE_LINE:
    case SHAPE_TYPE_QUAD:
    case SHAPE_TYPE_CUBIC:
    case SHAPE_TYPE_ARC:
      return false;

    case SHAPE_TYPE_RECT:
      return reinterpret_cast<const NumT_(Rect)*>(shapeData)->hitTest(*pt);

    case SHAPE_TYPE_ROUND:
      return reinterpret_cast<const NumT_(Round)*>(shapeData)->hitTest(*pt);

    case SHAPE_TYPE_CIRCLE:
      return reinterpret_cast<const NumT_(Circle)*>(shapeData)->hitTest(*pt);

    case SHAPE_TYPE_ELLIPSE:
      return reinterpret_cast<const NumT_(Ellipse)*>(shapeData)->hitTest(*pt);

    case SHAPE_TYPE_CHORD:
      return reinterpret_cast<const NumT_(Chord)*>(shapeData)->hitTest(*pt);

    case SHAPE_TYPE_PIE:
      return reinterpret_cast<const NumT_(Pie)*>(shapeData)->hitTest(*pt);

    default:
      return false;
  }
}

// ============================================================================
// [Fog::G2d - Library Initializers]
// ============================================================================

FOG_NO_EXPORT void _g2d_shape_init(void)
{
  _g2d.shapef.getBoundingBox = _G2d_ShapeT_getBoundingBox<float>;
  _g2d.shaped.getBoundingBox = _G2d_ShapeT_getBoundingBox<double>;

  _g2d.shapef.hitTest = _G2d_ShapeT_hitTest<float>;
  _g2d.shaped.hitTest = _G2d_ShapeT_hitTest<double>;
}

FOG_NO_EXPORT void _g2d_shape_fini(void)
{
}

} // Fog namespace
