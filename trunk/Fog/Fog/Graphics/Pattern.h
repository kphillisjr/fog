// [Fog/Graphics Library - C++ API]
//
// [Licence]
// MIT, See COPYING file in package

// [Guard]
#ifndef _FOG_GRAPHICS_PATTERN_H
#define _FOG_GRAPHICS_PATTERN_H

// [Dependencies]
#include <Fog/Core/Static.h>
#include <Fog/Graphics/Argb.h>
#include <Fog/Graphics/Constants.h>
#include <Fog/Graphics/Geometry.h>
#include <Fog/Graphics/Image.h>
#include <Fog/Graphics/Matrix.h>

//! @addtogroup Fog_Graphics
//! @{

namespace Fog {

// ============================================================================
// [Fog::Pattern]
// ============================================================================

//! @brief Pattern can be used to define stroke or fill source when painting.
//!
//! Pattern is class that can be used to define these types of sources:
//! - Null - NOP (no source, no painting...).
//! - Solid color - Everything is painted by solid color.
//! - Texture - Raster texture is used as a source.
//! - Linear gradient - Linear gradient defined between two points.
//! - Radial gradient - Radial gradient defined by one circle, radius and focal point.
//! - Conical gradient - Conical gradient - atan() function is used.
struct FOG_API Pattern
{
  // [Data]

  struct FOG_API Data
  {
    // [Construction / Destruction]
    Data();
    Data(const Data& other);
    ~Data();

    // [Ref / Deref]

    FOG_INLINE Data* ref() const
    {
      refCount.inc();
      return const_cast<Data*>(this);
    }

    FOG_INLINE void deref()
    {
      if (refCount.deref()) delete this;
    }

    Data* copy();
    void deleteResources();

    // [Members]

    //! @brief Reference count.
    mutable Atomic<sysuint_t> refCount;

    //! @brief pattern type, see @c PATTERN_TYPE enum.
    int type;
    //! @brief Pattern spread, see @c SPREAD_TYPE enum.
    int spread;

    //! @brief Start and end points. These values have different meaning for
    //! each pattern type:
    //! - Null and Solid - Not used, should be zeros.
    //! - Texture - points[0] is texture offset (starting point).
    //! - Linear gradient - points[0 to 1] is start and end point.
    //! - Radial gradient - points[0] is circle center point, points[1] is focal point.
    //! - Conical gradient - points[0] is center point.
    PointD points[2];
    //! @brief Used only for PATTERN_RADIAL_GRADIENT - circle radius.
    double radius;

    //! @brief Pattern transformation matrix.
    Matrix matrix;

    //! @brief Embedded objects in pattern, this can be solid color, raster
    //! texture data and gradient stops.
    union Objects {
      Static<Argb> rgba;
      Static<Image> texture;
      Static<List<ArgbStop> > stops;
    } obj;
  };

  static Static<Data> sharedNull;

  // [Construction / Destruction]

  Pattern();
  Pattern(const Pattern& other);
  FOG_INLINE explicit Pattern(Data* d) : _d(d) {}
  ~Pattern();

  // [Implicit Sharing]

  FOG_INLINE sysuint_t refCount() const { return _d->refCount.get(); }
  FOG_INLINE sysuint_t isDetached() const { return _d->refCount.get() == 1; }

  FOG_INLINE err_t detach() { return (_d->refCount.get() > 1) ? _detach() : ERR_OK; }
  err_t _detach();

  void free();

  // [Type]

  FOG_INLINE int getType() const { return _d->type; }
  err_t setType(int type);

  FOG_INLINE bool isNull() const { return _d->type == PATTERN_NULL; }
  FOG_INLINE bool isSolid() const { return _d->type == PATTERN_SOLID; }
  FOG_INLINE bool isTexture() const { return _d->type == PATTERN_TEXTURE; }
  FOG_INLINE bool isGradient() const { return (_d->type & PATTERN_GRADIENT_MASK) == PATTERN_GRADIENT_MASK; }
  FOG_INLINE bool isLinearGradient() const { return _d->type == PATTERN_LINEAR_GRADIENT; }
  FOG_INLINE bool isRadialGradient() const { return _d->type == PATTERN_RADIAL_GRADIENT; }
  FOG_INLINE bool isConicalGradient() const { return _d->type == PATTERN_CONICAL_GRADIENT; }

  // [Null]

  void reset();

  // [Spread]

  FOG_INLINE int getSpread() const { return _d->spread; }
  err_t setSpread(int spread);

  // [Matrix]

  FOG_INLINE const Matrix& getMatrix() const { return _d->matrix; }
  err_t setMatrix(const Matrix& matrix);
  err_t resetMatrix();

  err_t translate(double x, double y);
  err_t rotate(double a);
  err_t scale(double s);
  err_t scale(double x, double y);
  err_t skew(double x, double y);
  err_t multiply(const Matrix& m, int order = MATRIX_PREPEND);

  // [Start Point / End Point]

  FOG_INLINE const PointD& getStartPoint() const { return _d->points[0]; }
  FOG_INLINE const PointD& getEndPoint() const { return _d->points[1]; }

  err_t setStartPoint(const Point& pt);
  err_t setStartPoint(const PointD& pt);

  err_t setEndPoint(const Point& pt);
  err_t setEndPoint(const PointD& pt);

  err_t setPoints(const Point& startPt, const Point& endPt);
  err_t setPoints(const PointD& startPt, const PointD& endPt);

  // [Solid]

  Argb getColor() const;
  err_t setColor(const Argb& rgba);

  // [Texture]

  Image getTexture() const;
  err_t setTexture(const Image& texture);

  // [Gradient]

  FOG_INLINE double getRadius() const { return _d->radius; }
  err_t setRadius(double r);

  List<ArgbStop> getStops() const;
  err_t setStops(const List<ArgbStop>& stops);

  err_t resetStops();
  err_t addStop(const ArgbStop& stop);

  // [Operator Overload]

  Pattern& operator=(const Pattern& other);
  Pattern& operator=(const Argb& rgba);

  // [Members]

  FOG_DECLARE_D(Data)
};

} // Fog namespace

//! @}

// [Guard]
#endif // _FOG_GRAPHICS_PATTERN_H
