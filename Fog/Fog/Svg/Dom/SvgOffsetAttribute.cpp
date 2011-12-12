// [Fog-Svg]
//
// [License]
// MIT, See COPYING file in package

// [Precompiled Headers]
#if defined(FOG_PRECOMP)
#include FOG_PRECOMP
#endif // FOG_PRECOMP

// [Dependencies]
#include <Fog/Core/Math/Math.h>
#include <Fog/Svg/Dom/SvgOffsetAttribute_p.h>

namespace Fog {

// ============================================================================
// [Fog::SvgOffsetAttribute]
// ============================================================================

SvgOffsetAttribute::SvgOffsetAttribute(XmlElement* element, const ManagedStringW& name, int offset) :
  XmlAttribute(element, name, offset)
{
  _offset = 0.0;
}

SvgOffsetAttribute::~SvgOffsetAttribute()
{
}

err_t SvgOffsetAttribute::setValue(const StringW& value)
{
  err_t err = _value.set(value);
  if (FOG_IS_ERROR(err)) return err;

  size_t end;
  if (value.parseReal(&_offset, CharW('.'), &end, NULL) == ERR_OK)
  {
    if (end < value.getLength() && value.getAt(end) == CharW('%')) _offset *= 0.01f;
    _offset = Math::bound<float>(_offset, 0.0f, 1.0f);
  }
  else
  {
    _offset = 0.0f;
  }

  return ERR_OK;
}

} // Fog namespace