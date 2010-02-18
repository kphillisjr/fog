// [Fog/Gui Library - Public API]
//
// [Licence]
// MIT, See COPYING file in package

// [Guard]
#ifndef _FOG_GUI_CHECKBOX_H
#define _FOG_GUI_CHECKBOX_H

// [Dependencies]
#include <Fog/Gui/ButtonBase.h>

//! @addtogroup Fog_Gui
//! @{

namespace Fog {

// ============================================================================
// [Fog::CheckBox]
// ============================================================================

struct FOG_API CheckBox : public ButtonBase
{
  FOG_DECLARE_OBJECT(CheckBox, ButtonBase)

  // [Construction / Destruction]

  CheckBox();
  virtual ~CheckBox();

  // [Event Handlers]

  // Fog::Widget Events.
  virtual void onClick(MouseEvent* e);
  virtual void onPaint(PaintEvent* e);
};

} // Fog namespace

//! @}

// [Guard]
#endif // _FOG_GUI_CHECKBOX_H
