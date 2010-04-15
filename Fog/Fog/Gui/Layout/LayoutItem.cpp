// [Fog-Gui Library - Public API]
//
// [License]
// MIT, See COPYING file in package

// [Precompiled Headers]
#if defined(FOG_PRECOMP)
#include FOG_PRECOMP
#endif // FOG_PRECOMP

// [Dependencies]
#include <Fog/Gui/Layout/LayoutItem.h>
#include <Fog/Gui/Widget.h>

FOG_IMPLEMENT_OBJECT(Fog::LayoutItem)

namespace Fog {

// ============================================================================
// [Fog::LayoutItem]
// ============================================================================

LayoutItem::LayoutItem(uint32_t alignment) : _alignment(alignment)
{

}

LayoutItem::~LayoutItem() 
{
}

IntSize LayoutItem::calculateMaximumSize(const IntSize& sizeHint, const IntSize& minSize, const IntSize& maxSize, const LayoutPolicy& sizePolicy, uint32_t align)
{
  if (align & ALIGNMENT_HORIZONTAL_MASK && align & ALIGNMENT_VERTICAL_MASK)
    return IntSize(WIDGET_MAX_SIZE, WIDGET_MAX_SIZE); //Max Layout-Size??
  IntSize s = maxSize;
  IntSize hint = sizeHint.expandedTo(minSize);
  if (s.getWidth() == WIDGET_MAX_SIZE && !(align & ALIGNMENT_HORIZONTAL_MASK))
    if (!(sizePolicy.getHorizontalPolicy() & LAYOUT_GROWING_WIDTH))
      s.setWidth(hint.getWidth());

  if (s.getHeight() == WIDGET_MAX_SIZE && !(align & ALIGNMENT_VERTICAL_MASK))
    if (!(sizePolicy.getVerticalPolicy() & LAYOUT_GROWING_WIDTH))
      s.setHeight(hint.getHeight());

  if (align & ALIGNMENT_HORIZONTAL_MASK)
    s.setWidth(WIDGET_MAX_SIZE);
  if (align & ALIGNMENT_VERTICAL_MASK)
    s.setHeight(WIDGET_MAX_SIZE);
  return s;
}

IntSize LayoutItem::calculateMaximumSize(const Widget *w) 
{
  return calculateMaximumSize(w->getSizeHint().expandedTo(w->getMinimumSizeHint()), w->getMinimumSize(), w->getMaximumSize(), w->getLayoutPolicy(), w->getLayoutAlignment());
}


IntSize LayoutItem::calculateMinimumSize(const IntSize& sizeHint, const IntSize& minSizeHint, const IntSize& minSize, const IntSize& maxSize, const LayoutPolicy& sizePolicy) {
  IntSize s(0, 0);

  if (sizePolicy.getHorizontalPolicy() != LAYOUT_IGNORE_WIDTH) {
    if (sizePolicy.getHorizontalPolicy() & LAYOUT_SHRINKING_WIDTH)
      s.setWidth(minSizeHint.getWidth());
    else
      s.setWidth(Math::max(sizeHint.getWidth(), minSizeHint.getWidth()));
  }

  if (sizePolicy.getVerticalPolicy() != LAYOUT_IGNORE_HEIGHT) {
    if (sizePolicy.getVerticalPolicy() & LAYOUT_SHRINKING_HEIGHT) {
      s.setHeight(minSizeHint.getHeight());
    } else {
      s.setHeight(Math::max(sizeHint.getHeight(), minSizeHint.getHeight()));
    }
  }

  s = s.boundedTo(maxSize);
  if (minSize.getWidth() > 0)
    s.setWidth(minSize.getWidth());
  if (minSize.getHeight() > 0)
    s.setHeight(minSize.getHeight());

  return s.expandedTo(IntSize(0,0));
}

IntSize LayoutItem::calculateMinimumSize(const Widget* w) {
  return calculateMinimumSize(w->getSizeHint(), w->getMinimumSizeHint(),w->getMinimumSize(), w->getMaximumSize(),w->getLayoutPolicy());
}

} // Fog namespace
