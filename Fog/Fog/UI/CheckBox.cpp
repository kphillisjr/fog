
#include "ButtonBase.h"

// [Fog/UI Library - C++ API]
//
// [Licence]
// MIT, See COPYING file in package

// [Precompiled headers]
#if defined(FOG_PRECOMP)
#include FOG_PRECOMP
#endif // FOG_PRECOMP

// [Dependencies]
#include <Fog/Graphics/Painter.h>
#include <Fog/Graphics/PainterUtil.h>
#include <Fog/UI/CheckBox.h>

FOG_IMPLEMENT_OBJECT(Fog::CheckBox)

namespace Fog {

// ============================================================================
// [Fog::CheckBox]
// ============================================================================

CheckBox::CheckBox() :
  ButtonBase()
{
}

CheckBox::~CheckBox()
{
}

void CheckBox::onMouse(MouseEvent* e)
{
  switch (e->code())
  {
    case EvClick:
    {
      if (e->button() == ButtonLeft)
      {
        setChecked(_checked ^ 1);
      }
      break;
    }
  }

  base::onMouse(e);
}

void CheckBox::onPaint(PaintEvent* e)
{
  Painter* p = e->painter();

  Rect bounds(0, 0, width(), height());
  Rect chrect(1, (height() - 13) / 2, 13, 13);

  p->setSource(0xFF000000);
  p->drawRect(chrect);

  chrect.shrink(1);

  p->setSource(0xFFFFFFFF);
  p->fillRect(chrect);

  if (checked())
  {
    p->setSource(0xFF000000);
    Path path;
    double c = (double)(height() / 2);
    path.moveTo(3.5, c - 1.5);
    path.lineTo(6.5, c + 3.5);
    path.lineTo(11.5, c - 4.5);
    p->drawPath(path);
  }

  if (isDown())
  {
    Rect d = chrect;
    p->setSource(0x7F8FAFFF);
    p->drawRect(d);
    d.shrink(1);
    p->setSource(0x7F6F8FFF);
    p->drawRect(d);
  }

  bounds.shrink(1);
  bounds._x += 16;
  bounds._w -= 16;

  p->setSource(0xFF000000);
  p->drawText(bounds, _text, _font, TextAlignLeft);
}

} // Fog namespace
