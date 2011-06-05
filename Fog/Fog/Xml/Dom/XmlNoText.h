// [Fog-Xml]
//
// [License]
// MIT, See COPYING file in package

// [Guard]
#ifndef _FOG_XML_DOM_XMLNOTEXT_H
#define _FOG_XML_DOM_XMLNOTEXT_H

// [Dependencies]
#include <Fog/Core/Global/Class.h>
#include <Fog/Core/Tools/ManagedString.h>
#include <Fog/Core/Tools/Range.h>
#include <Fog/Core/Tools/String.h>
#include <Fog/Xml/Global/Constants.h>
#include <Fog/Xml/Dom/XmlElement.h>

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

  XmlNoText(const ManagedString& tagName);
  virtual ~XmlNoText();

  // --------------------------------------------------------------------------
  // [Methods]
  // --------------------------------------------------------------------------

  virtual String getTextContent() const;
  virtual err_t setTextContent(const String& text);

private:
  _FOG_CLASS_NO_COPY(XmlNoText)
};

} // Fog namespace

// [Guard]
#endif // _FOG_XML_DOM_XMLNOTEXT_H