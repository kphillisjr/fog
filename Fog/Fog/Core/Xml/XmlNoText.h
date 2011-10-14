// [Fog-Core]
//
// [License]
// MIT, See COPYING file in package

// [Guard]
#ifndef _FOG_CORE_XML_XMLNOTEXT_H
#define _FOG_CORE_XML_XMLNOTEXT_H

// [Dependencies]
#include <Fog/Core/Global/Global.h>
#include <Fog/Core/Tools/ManagedString.h>
#include <Fog/Core/Tools/Range.h>
#include <Fog/Core/Tools/String.h>
#include <Fog/Core/Xml/XmlElement.h>

namespace Fog {

//! @addtogroup Fog_Xml_Dom
//! @{

// ============================================================================
// [Fog::XmlNoText]
// ============================================================================

//! @brief Xml no text element (convenience, base class for others).
struct FOG_API XmlNoText : public XmlElement
{
  typedef XmlElement base;

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  XmlNoText(const ManagedStringW& tagName);
  virtual ~XmlNoText();

  // --------------------------------------------------------------------------
  // [Methods]
  // --------------------------------------------------------------------------

  virtual StringW getTextContent() const;
  virtual err_t setTextContent(const StringW& text);

private:
  _FOG_NO_COPY(XmlNoText)
};

} // Fog namespace

// [Guard]
#endif // _FOG_CORE_XML_XMLNOTEXT_H
