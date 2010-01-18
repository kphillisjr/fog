// [Fog/UI Library - Public API]
//
// [Licence]
// MIT, See COPYING file in package

// [Dependencies]
#include <Fog/Core/Application.h>
#include <Fog/Core/List.h>
#include <Fog/Core/Math.h>
#include <Fog/Graphics/Argb.h>
#include <Fog/Graphics/Geometry.h>
#include <Fog/Graphics/Font.h>
#include <Fog/Graphics/Image.h>
#include <Fog/Graphics/Painter.h>
#include <Fog/Graphics/Region.h>
#include <Fog/UI/Event.h>
#include <Fog/UI/Widget.h>

#include <Fog/UI/UISystem/Base.h>

FOG_IMPLEMENT_OBJECT(Fog::BaseUISystem)
FOG_IMPLEMENT_OBJECT(Fog::BaseUIWindow)

namespace Fog {

// ============================================================================
// [Helpers]
// ============================================================================

#define UI_SYSTEM() \
  reinterpret_cast<BaseUISystem*>(Application::getInstance()->getUiSystem())

static uint32_t fogButtonToId(uint32_t button)
{
  switch (button)
  {
    case BUTTON_LEFT  : return 0;
    case BUTTON_MIDDLE: return 1;
    case BUTTON_RIGHT : return 2;

    default:
      return BUTTON_INVALID;
  }
}

// ============================================================================
// [Fog::BaseUISystem - Construction / Destruction]
// ============================================================================

BaseUISystem::BaseUISystem()
{
  // Clear all
  memset(&_displayInfo, 0, sizeof(_displayInfo));
  memset(&_paletteInfo, 0, sizeof(_paletteInfo));
  memset(&_caretStatus, 0, sizeof(_caretStatus));
  memset(&_keyboardStatus, 0, sizeof(_keyboardStatus));
  memset(&_mouseStatus, 0, sizeof(_mouseStatus));
  memset(&_systemMouseStatus, 0, sizeof(_systemMouseStatus));

  _mouseStatus.position.set(INT_MIN, INT_MIN);
  _systemMouseStatus.position.set(INT_MIN, INT_MIN);

  _wheelLines          = 3;
  _repeatingDelay      = TimeDelta::fromMilliseconds(200);
  _repeatingInterval   = TimeDelta::fromMilliseconds(50);
  _doubleClickInterval = TimeDelta::fromMilliseconds(200);

  _buttonRepeat[0].addListener(EV_TIMER, this, &BaseUISystem::_onButtonRepeatTimeOut);
  _buttonRepeat[1].addListener(EV_TIMER, this, &BaseUISystem::_onButtonRepeatTimeOut);
  _buttonRepeat[2].addListener(EV_TIMER, this, &BaseUISystem::_onButtonRepeatTimeOut);

  memset(&_updateStatus, 0, sizeof(_updateStatus));
}

BaseUISystem::~BaseUISystem()
{
  if (_updateStatus.task) _updateStatus.task->cancel();
}

// ============================================================================
// [Fog::BaseUISystem - ID <-> UIWindow]
// ============================================================================

bool BaseUISystem::mapHandle(void* handle, UIWindow* native)
{
  return _widgetMapper.put(handle, native, true) == ERR_OK;
}

bool BaseUISystem::unmapHandle(void* handle)
{
  return _widgetMapper.remove(handle);
}

UIWindow* BaseUISystem::handleToNative(void* handle) const
{
  return _widgetMapper.value(handle, NULL);
}

// ============================================================================
// [Fog::BaseUISystem - Display]
// ============================================================================

err_t BaseUISystem::getDisplayInfo(DisplayInfo* out) const
{
  memcpy(out, &_displayInfo, sizeof(DisplayInfo));
  return ERR_OK;
}

err_t BaseUISystem::getPaletteInfo(PaletteInfo* out) const
{
  memcpy(out, &_paletteInfo, sizeof(PaletteInfo));
  return ERR_OK;
}

// ============================================================================
// [Fog::BaseUISystem - Caret]
// ============================================================================

err_t BaseUISystem::getCaretStatus(CaretStatus* out) const
{
  memcpy(out, &_caretStatus, sizeof(CaretStatus));
  return ERR_OK;
}

// ============================================================================
// [Fog::BaseUISystem - Keyboard]
// ============================================================================

err_t BaseUISystem::getKeyboardStatus(KeyboardStatus* out) const
{
  memcpy(out, &_keyboardStatus, sizeof(KeyboardStatus));
  return ERR_OK;
}

uint32_t BaseUISystem::getKeyboardModifiers() const
{
  return _keyboardStatus.modifiers;
}

// ============================================================================
// [Fog::BaseUISystem - Mouse]
// ============================================================================

err_t BaseUISystem::getMouseStatus(MouseStatus* out) const
{
  memcpy(out, &_mouseStatus, sizeof(MouseStatus));
  return ERR_OK;
}

err_t BaseUISystem::getSystemMouseStatus(SystemMouseStatus* out) const
{
  memcpy(out, &_systemMouseStatus, sizeof(SystemMouseStatus));
  return ERR_OK;
}

void BaseUISystem::invalidateMouseStatus()
{
  _mouseStatus.valid = false;
}

void BaseUISystem::updateMouseStatus()
{
}

void BaseUISystem::changeMouseStatus(Widget* w, const Point& pos)
{
  if (_mouseStatus.widget != w)
  {
    // Clear button press times (double click).
    _buttonTime[0].clear();
    _buttonTime[1].clear();
    _buttonTime[2].clear();

    Widget* before = _mouseStatus.widget;

    _mouseStatus.widget = w;
    _mouseStatus.position = pos;
    _mouseStatus.hover = true;
    _mouseStatus.buttons = 0;
    _mouseStatus.valid = true;

    // Send MouseOut to widget where mouse was before
    if (before)
    {
      MouseEvent e(EV_MOUSE_OUT);
      e._button = BUTTON_INVALID;
      e._modifiers = getKeyboardModifiers();
      e._position = pos;
      Widget::translateCoordinates(before, w, &e._position);
      before->sendEvent(&e);
    }

    {
      MouseEvent e(EV_MOUSE_IN);
      e._button = BUTTON_INVALID;
      e._modifiers = getKeyboardModifiers();
      e._position = pos;
      w->sendEvent(&e);
    }
  }
  else if (_mouseStatus.position != pos)
  {
    // Clear button press times (double click).
    _buttonTime[0].clear();
    _buttonTime[1].clear();
    _buttonTime[2].clear();

    uint32_t code = 0; // be quite
    uint32_t hoverChange;;

    hoverChange = (!!_mouseStatus.hover) |
                  ((!(pos.getX() < 0 || 
                      pos.getY() < 0 ||
                      pos.getX() >= w->getWidth() ||
                      pos.getY() >= w->getHeight())) << 1);

    enum HOVER_CHANGE
    {
      HOVER_CHANGE_OUTSIDE_MOVE = 0x00, // 00b
      HOVER_CHANGE_OUT          = 0x01, // 01b
      HOVER_CHANGE_IN           = 0x02, // 10b
      HOVER_CHANGE_INSIDE_MOVE  = 0x03  // 11b
    };

    switch (hoverChange)
    {
      case HOVER_CHANGE_OUTSIDE_MOVE: code = EV_MOUSE_MOVE; break;
      case HOVER_CHANGE_OUT:          code = EV_MOUSE_OUT ; break;
      case HOVER_CHANGE_IN:           code = EV_MOUSE_IN  ; break;
      case HOVER_CHANGE_INSIDE_MOVE:  code = EV_MOUSE_MOVE; break;
    }

    _mouseStatus.position = pos;
    _mouseStatus.hover = (hoverChange & 0x02) != 0;

    {
      MouseEvent e(code);
      e._button = BUTTON_INVALID;
      e._modifiers = getKeyboardModifiers();
      e._position = pos;
      e._isOutside = (hoverChange == 0x00);
      w->sendEvent(&e);
    }

    // Generate MouseMove events if generated event was MouseOut and widget is
    // in dragging state.
    if (_mouseStatus.buttons && code == EV_MOUSE_OUT)
    {
      MouseEvent e(EV_MOUSE_MOVE);
      e._button = BUTTON_INVALID;
      e._modifiers = getKeyboardModifiers();
      e._position = pos;
      e._isOutside = true;
      w->sendEvent(&e);
    }
  }
}

void BaseUISystem::clearSystemMouseStatus()
{
  _systemMouseStatus.uiWindow = NULL;
  _systemMouseStatus.position.set(INT_MIN, INT_MIN);
  _systemMouseStatus.hover = false;
  _systemMouseStatus.buttons = 0;
}

bool BaseUISystem::startButtonRepeat(uint32_t button, bool reset, TimeDelta delay, TimeDelta interval)
{
  uint32_t id = fogButtonToId(button);
  if (id == BUTTON_INVALID) return false;

  if (!_buttonRepeat[id].isRunning())
  {
    _buttonRepeat[id].setInterval(delay);
    _buttonRepeat[id].start();
    _buttonRepeatInterval[id] = interval;
  }
  else if (_buttonRepeat[id].isRunning() && reset)
  {
    _buttonRepeat[id].stop();
    _buttonRepeat[id].setInterval(interval);
    _buttonRepeat[id].start();
    _buttonRepeatInterval[id] = interval;
  }
  return true;
}

bool BaseUISystem::stopButtonRepeat(uint32_t button)
{
  uint32_t id = fogButtonToId(button);
  if (id == BUTTON_INVALID) return false;

  if (_buttonRepeat[id].isRunning())
  {
    _buttonRepeat[id].stop();
  }
  return true;
}

void BaseUISystem::clearButtonRepeat()
{
  for (sysuint_t i = 0; i < FOG_ARRAY_SIZE(_buttonRepeat); i++)
  {
    if (_buttonRepeat[i].isRunning()) _buttonRepeat[i].stop();
  }
}

// ============================================================================
// [Fog::BaseUISystem - Wheel]
// ============================================================================

int BaseUISystem::getWheelLines() const
{
  return _wheelLines;
}

void BaseUISystem::setWheelLines(int count)
{
  _wheelLines = count;
}

// ============================================================================
// [Fog::BaseUISystem - Timing]
// ============================================================================

TimeDelta BaseUISystem::getRepeatingDelay() const
{
  return _repeatingDelay;
}

TimeDelta BaseUISystem::getRepeatingInterval() const
{
  return _repeatingInterval;
}

TimeDelta BaseUISystem::getDoubleClickInterval() const
{
  return _doubleClickInterval;
}

// ============================================================================
// [Fog::BaseUISystem - Windowing System]
// ============================================================================

void BaseUISystem::dispatchEnabled(Widget* w, bool enabled)
{
  uint32_t state = w->getState();

  // Dispatch 'Enable'.
  if (enabled)
  {
    if (state == WIDGET_ENABLED) return;
    if (state == WIDGET_DISABLED_BY_PARENT && !w->isUIWindow()) return;

    w->_state = WIDGET_ENABLED;

    StateEvent e(EV_ENABLE);
    w->sendEvent(&e);

    FOG_WIDGET_TREE_ITERATOR(i1, w, true,
      // before traverse
      {
        // show only child that's hidden by parent
        if (child->getState() != WIDGET_DISABLED_BY_PARENT) FOG_WIDGET_TREE_ITERATOR_NEXT(i1);

        child->_state = WIDGET_ENABLED;
        child->sendEvent(&e);
      },
      // after traverse
      {
      }
    );

    w->update(Widget::UFlagUpdateAll);
  }
  // Dispatch 'Disable'.
  else
  {
    uint32_t state = w->getState();
    if (state == WIDGET_DISABLED) return;

    w->_state = WIDGET_DISABLED;

    //StateEvent e(toState == Widget::Disabled
    //  ? EvDisable
    //  : EvDisableByParent);
    StateEvent e(EV_DISABLE);
    w->sendEvent(&e);

    if (state != WIDGET_ENABLED) return;

    e._code = EV_DISABLE_BY_PARENT;

    FOG_WIDGET_TREE_ITERATOR(i2, w, true,
      // before traverse
      {
        // mark 'DisableByParent' all childs that's visible
        if (child->getState() != WIDGET_ENABLED)
        {
          FOG_WIDGET_TREE_ITERATOR_NEXT(i2);
        }
        else
        {
          child->_state = WIDGET_DISABLED_BY_PARENT;
          child->sendEvent(&e);
        }
      },
      // after traverse
      {
      }
    );

    w->update(Widget::UFlagUpdateAll);
  }
}

void BaseUISystem::dispatchVisibility(Widget* w, bool visible)
{
  uint32_t visibility = w->getVisibility();

  // Dispatch 'Show'.
  if (visible)
  {
    if (visibility == WIDGET_VISIBLE) return;
    if (visibility == WIDGET_HIDDEN_BY_PARENT && !w->isUIWindow()) return;

    w->_visibility = WIDGET_VISIBLE;

    VisibilityEvent e(EV_SHOW);
    w->sendEvent(&e);

    FOG_WIDGET_TREE_ITERATOR(i1, w, true,
      // before traverse
      {
        // show only child that's hidden by parent
        if (child->getVisibility() != WIDGET_HIDDEN_BY_PARENT) FOG_WIDGET_TREE_ITERATOR_NEXT(i1);

        child->_visibility = WIDGET_VISIBLE;
        w->sendEvent(&e);
      },
      // after traverse
      {}
    );

    w->update(Widget::UFlagUpdateAll);
  }
  // Dispatch 'Hidden'.
  else
  {
    if (visibility == WIDGET_HIDDEN) return;
    w->_visibility = WIDGET_HIDDEN;

    VisibilityEvent e(EV_HIDE);
    w->sendEvent(&e);

    if (visibility == WIDGET_VISIBLE)
    {
      e._code = EV_HIDE_BY_PARENT;

      FOG_WIDGET_TREE_ITERATOR(i2, w, true,
        // before traverse
        {
          // mark 'HiddenByParent' all childs that's visible
          if (child->getVisibility() != WIDGET_VISIBLE)
          {
            FOG_WIDGET_TREE_ITERATOR_NEXT(i2);
          }
          else
          {
            child->_visibility = WIDGET_HIDDEN_BY_PARENT;
            child->sendEvent(&e);
          }
        },
        // after traverse
        {
        }
      );

      Widget* p = w->getParent();
      if (p) p->update(Widget::UFlagUpdateAll);
    }
  }
}

void BaseUISystem::dispatchConfigure(Widget* w, const Rect& rect, bool changedOrientation)
{
  uint32_t changed = 0;

  if (w->getPosition() != rect.getPosition())
    changed |= ConfigureEvent::CHANGED_POSITION;

  if (w->getSize() != rect.getSize())
    changed |= ConfigureEvent::CHANGED_SIZE;

  if (changedOrientation)
    changed |= ConfigureEvent::CHANGED_ORIENTATION;

  if (!changed) return;

  ConfigureEvent e;
  e._rect = rect;
  e._changed = changed;

  w->_rect = rect;
  w->sendEvent(&e);

  Widget* p = w->getParent();
  if (p)
  {
    if (w->getVisibility() == WIDGET_VISIBLE) p->update(Widget::UFlagUpdateAll);
  }
  else if (changed & ConfigureEvent::CHANGED_SIZE)
  {
    w->update(Widget::UFlagUpdateAll);
  }
}

void BaseUISystem::widgetDestroyed(Widget* w)
{
  if (_mouseStatus.widget == w)
  {
    _mouseStatus.widget = NULL;
    _mouseStatus.position.set(INT_MIN, INT_MIN);
    _mouseStatus.hover = 0;
    _mouseStatus.valid = false;
  }

  if (_caretStatus.widget == w)
  {
    Memory::zero(&_caretStatus, sizeof(CaretStatus));
  }
}

// ============================================================================
// [Fog::BaseUISystem - Update]
// ============================================================================

// Working structure for parent and child, also it's record in stack for 
// traversing in widget-tree.
struct UpdateRec
{
  Box bounds;
  Box paintBounds;
  uint32_t uflags;
  uint32_t implicitFlags;
  uint32_t visible;
  uint32_t painted;
};

struct UpdateTask : public CancelableTask
{
  UpdateTask() : _valid(true) {}

  virtual void run()
  {
    if (!_valid) return;

    // Try to process all pending messages from event queue.
    // EventLoop::getCurrent()->runAllPending();
    // Do update.
    UI_SYSTEM()->doUpdate();
  }

  virtual void cancel()
  {
    _valid = false;
  }

private:
  bool _valid;
};

void BaseUISystem::update()
{
  if (!_updateStatus.scheduled)
  {
    _updateStatus.scheduled = true;
    _updateStatus.task = new(std::nothrow) UpdateTask();
    Application::getInstance()->getEventLoop()->postTask(_updateStatus.task);
  }
}

void BaseUISystem::doUpdate()
{
  _updateStatus.scheduled = false;
  _updateStatus.updating = true;
  _updateStatus.task = NULL;

  // Iterate over all dirty "UIWindow"s and clear the list.
  {
    List<BaseUIWindow*> dirty(_dirtyList);
    _dirtyList.free();

    List<BaseUIWindow*>::ConstIterator it(dirty);
    for (it.toStart(); it.isValid(); it.toNext())
    {
      UIWindow* uiWindow = it.value();
      if (uiWindow) doUpdateWindow(uiWindow);
    }
  }

  _updateStatus.updating = false;
}

void BaseUISystem::doUpdateWindow(UIWindow* window)
{
  window->_isDirty = false;

  // UIWindow widget.
  Widget* top = window->_widget;

  // Hidden windows won't be updated.
  if (!window->_visible) 
  {
    top->_uflags = 0;
    return;
  }

  // =======================================================
  // Local variables and initialization.
  // =======================================================

  // 10 seconds is extra buffer timeout
  TimeTicks now = TimeTicks::now() - TimeDelta::fromSeconds(10);

  // Painting.
  Painter painter;
  PaintEvent e;
  e._painter = &painter;

  // Temporary regions. We are using more regions, because region operations
  // are most effective by Region::op(dest, src1, src2) way. So it's safe to 
  // use different destination regions. Memory will be copied, but no memory
  // will be allocated that is the goal.
  TemporaryRegion<32> rtmp1;
  TemporaryRegion<32> rtmp2;
  TemporaryRegion<32> rtmp3;
  TemporaryRegion<64> blitRegion;

  // Some temporary data.
  Size topSize(top->getSize());
  Box  topBox(0, 0, (int)topSize.getWidth(), (int)topSize.getHeight());

  uint32_t uflags = top->_uflags;
  uint32_t implicitFlags = 0;
  bool blitFull = false;
  bool paintAll = false;

  // Paint helper variables.
  uint topBytesPerPixel;

  if ((int)window->_backingStore->getWidth()  < topSize.getWidth() ||
      (int)window->_backingStore->getHeight() < topSize.getHeight())
  {
    window->_backingStore->resize((uint)topSize.getWidth(), (uint)topSize.getHeight(), true);

    // We can omit updating here, because if window size was changed,
    // all uflags are already set.

    // It should already be set, but nobody knows...
    uflags |= Widget::UFlagUpdateAll;
  }
  else if ((
      (int)window->_backingStore->getWidth() != topSize.getWidth() ||
      (int)window->_backingStore->getHeight() != topSize.getHeight()) &&
      window->_backingStore->expired(now))
  {
    window->_backingStore->resize(topSize.getWidth(), topSize.getHeight(), false);
    uflags |= Widget::UFlagUpdateAll;
  }

  topBytesPerPixel = window->_backingStore->getDepth() >> 3;

  // =======================================================
  // Update top level widget.
  // =======================================================

  // if there is nothing to do, continue. It's checked here,
  // because next steps needs to create drawing context, etc...
  if ((uflags & (
      Widget::UFlagUpdate         |
      Widget::UFlagUpdateChild    |
      Widget::UFlagUpdateAll      |
      Widget::UFlagUpdateGeometry |
      Widget::UFlagRepaintWidget  |
      Widget::UFlagRepaintCaret   )) == 0)
  {
    return;
  }

  // We will call Painter::begin() here, because it will be shared
  // between all repaints. Also we tweak width and height, because if we
  // set abnormal large width or height (in cases that backing store is cached)
  // we can tell painter to use multithreading in small areas (that we don't
  // want).
  {
    ImageBuffer buffer;
    buffer.data = window->_backingStore->getPixels();
    buffer.width = Math::min(window->_backingStore->getWidth(), topSize.getWidth());
    buffer.height = Math::min(window->_backingStore->getHeight(), topSize.getHeight());
    buffer.stride = window->_backingStore->getStride();
    buffer.format = window->_backingStore->getFormat();

    // FIXME: Remove that hint, it's only for current testing.
    painter.begin(buffer, 0 | PAINTER_HINT_NO_MT);
  }

  if ((uflags & Widget::UFlagUpdateAll) != 0)
  {
    implicitFlags |=
      Widget::UFlagUpdate         |
      Widget::UFlagUpdateChild    |
      Widget::UFlagUpdateGeometry |
      Widget::UFlagRepaintWidget  ;
  }

  uflags |= implicitFlags;

  if ((uflags & Widget::UFlagRepaintWidget) != 0)
  {
    e._code = EV_PAINT;
    e._receiver = top;
    e._isParentPainted = 0;

    painter.setMetaVariables(
      Point(0, 0),
      TemporaryRegion<1>(Rect(0, 0, top->getWidth(), top->getHeight())),
      true,
      true);
    top->sendEvent(&e);

    uflags |=
      Widget::UFlagUpdateChild    ;
    implicitFlags |=
      Widget::UFlagUpdateChild    |
      Widget::UFlagRepaintWidget  ;

    blitFull = true;
    paintAll = true;
  }

  // =======================================================
  // Update children.
  // =======================================================

  if (top->hasChildren() && (uflags & Widget::UFlagUpdateChild) != 0)
  {
    LocalStack<1024> stack;
    err_t stackerr;

    // manual object iterator
    Widget* const* ocur;
    Widget* const* oend;

    // parent and child widgets
    Widget* parent;
    Widget* child;

    UpdateRec parentRec;
    UpdateRec childRec;

    parent = top;
    parentRec.bounds.set(0, 0, parent->getWidth(), parent->getHeight());
    parentRec.paintBounds.set(0, 0, parent->getWidth(), parent->getHeight());
    parentRec.uflags = uflags;
    parentRec.implicitFlags = implicitFlags;
    parentRec.visible = true;
    parentRec.painted = blitFull | paintAll;

__pushed:
    ocur = parent->_children.getData();
    oend = ocur + parent->_children.getLength();

    child = (Widget*)*ocur;
    for (;;)
    {
      if (child->getVisibility() != WIDGET_VISIBLE)
      {
        goto __next;
      }

      childRec.uflags = child->_uflags;
      childRec.implicitFlags = parentRec.implicitFlags;

      if ((childRec.uflags & Widget::UFlagUpdateAll) != 0)
      {
        childRec.implicitFlags |=
          Widget::UFlagUpdate         |
          Widget::UFlagUpdateChild    |
          Widget::UFlagUpdateGeometry |
          Widget::UFlagRepaintWidget  ;
      }
      childRec.uflags |= childRec.implicitFlags;

      if ((childRec.uflags & (
        Widget::UFlagUpdate         |
        Widget::UFlagUpdateChild    |
        Widget::UFlagUpdateGeometry |
        Widget::UFlagRepaintWidget  |
        Widget::UFlagRepaintCaret   |
        Widget::UFlagUpdateAll)) == 0)
      {
        goto __next;
      }

      childRec.bounds.x1 = parentRec.bounds.getX1() + child->getX1();
      childRec.bounds.y1 = parentRec.bounds.getY1() + child->getY1();
      childRec.bounds.x2 = childRec.bounds.getX1() + child->getWidth();
      childRec.bounds.y2 = childRec.bounds.getY1() + child->getHeight();

      childRec.bounds += parent->getOrigin();

      childRec.visible = Box::intersect(childRec.paintBounds, parentRec.paintBounds, childRec.bounds);
      childRec.painted = false;

      if (childRec.visible)
      {
        // paint client area / caret
        if ((childRec.uflags & (Widget::UFlagRepaintWidget  | Widget::UFlagRepaintCaret)) != 0)
        {
          e._code = EV_PAINT;
          e._receiver = child;
          e._isParentPainted = parentRec.painted | (!!(child->_uflags & Widget::UFlagPaintParentDone));

#if 0
          if (child->children().count() > 0 && child->children().getLength() <= 16)
          {
            rtmp2.set(childRec.paintBounds);
            TemporaryRegion<128> rtmp4;
            List<Widget*>::ConstIterator ci(child->children());
            int ox = childRec.bounds.x1() + child->origin().x();
            int oy = childRec.bounds.y1() + child->origin().y();

            for (ci.toStart(); ci.isValid(); ci.toNext())
            {
              Widget* cw = core_object_cast<Widget*>(ci.value());
              if (cw && cw->visibility() == Widget::Visible)
              {
                if ((cw->_uflags & Widget::UFlagPaintParentRequired))
                {
                  // this is simple optimization. If widget will call
                  // Widget::paintParent(), we will simply do it
                  // earlier.
                  cw->_uflags |= Widget::UFlagPaintParentDone;
                }
                else
                {
                  rtmp4.unite(Box(
                    cw->rect().x1() + ox,
                    cw->rect().y1() + oy,
                    cw->rect().x2() + ox,
                    cw->rect().y2() + oy));
                }
              }
            }
            Region::subtract(rtmp1, rtmp2, rtmp4);
          }
          else
          {
#endif
            rtmp1.set(childRec.paintBounds);
            childRec.painted = true;
#if 0
          }
#endif
          painter.setMetaVariables(
            Point(childRec.bounds.getX1(), childRec.bounds.getY1()),
            rtmp1,
            true,
            true);

          // FIXME: Repaint caret repaints whole control
          if ((childRec.uflags & (Widget::UFlagRepaintWidget | Widget::UFlagRepaintCaret)) != 0)
          {
            child->sendEvent(&e);
            //blitFull = true;
            if (!parentRec.painted) blitRegion.unite(rtmp1);
          }

          /*
          if (Application::caretStatus().widget == child &&
            (childRec.uflags & (Widget::UFlagRepaintWidget | Widget::UFlagRepaintCaret)) != 0)
          {
            theme->paintCaret(&painter, Application::caretStatus());
            //blitFull = true;
            if (!parentRec.painted) blitRegion.unite(rtmp1);
          }
          */

          parentRec.implicitFlags |=
            Widget::UFlagUpdate         |
            Widget::UFlagUpdateChild    |
            Widget::UFlagRepaintWidget  ;
          childRec.uflags |=
            Widget::UFlagUpdateChild    ;
          childRec.implicitFlags |=
            Widget::UFlagUpdate         |
            Widget::UFlagUpdateChild    |
            Widget::UFlagRepaintWidget  ;
        }
      }

      if ((childRec.uflags & Widget::UFlagUpdateChild) != 0 && child->_children.getLength())
      {
        stackerr = ERR_OK;
        stackerr |= stack.push(parent);
        stackerr |= stack.push(ocur);
        stackerr |= stack.push(oend);
        stackerr |= stack.push(parentRec);
        if (stackerr != ERR_OK) goto end;

        parent = child;
        parentRec = childRec;

        goto __pushed;
__pop:
        childRec = parentRec;
        child = parent;
        stack.pop(parentRec);
        stack.pop(oend);
        stack.pop(ocur);
        stack.pop(parent);
      }

      // clear update uflags
      child->_uflags &= ~(
        Widget::UFlagUpdate         |
        Widget::UFlagUpdateChild    |
        Widget::UFlagUpdateAll      |
        Widget::UFlagUpdateGeometry |
        Widget::UFlagRepaintWidget  |
        Widget::UFlagRepaintCaret   |
        // clear if this was set
        Widget::UFlagPaintParentDone);

__next:
      // go to next child or to parent
      if (++ocur == oend)
      {
        if (!stack.isEmpty())
          goto __pop;
        else
          break;
      }

      child = (Widget*)*ocur;
    }
  }

end:
  painter.end();

  // =======================================================
  // blit root widget content to screen
  // =======================================================

  const Box* rptr;
  sysuint_t rlen = 0;

  if (blitFull || window->_needBlit)
  {
    rptr = &topBox;
    rlen = 1;
  }
  else
  {
    rptr = blitRegion.getData();
    rlen = blitRegion.getLength();
  }

  if (rlen)
  {
    window->_backingStore->updateRects(rptr, rlen);
    doBlitWindow(window, rptr, rlen);
  }

  // Clear update flags.
  top->_uflags &= ~(
    Widget::UFlagUpdate         |
    Widget::UFlagUpdateChild    |
    Widget::UFlagUpdateAll      |
    Widget::UFlagUpdateGeometry |
    Widget::UFlagRepaintWidget  |
    Widget::UFlagRepaintCaret   |
    // clear if this was set
    Widget::UFlagPaintParentDone);

  // Clear blit flag.
  window->_needBlit = false;
}

// ============================================================================
// [Fog::BaseUISystem - Event Handlers]
// ============================================================================

void BaseUISystem::_onButtonRepeatTimeOut(TimerEvent* e)
{
  if (_systemMouseStatus.uiWindow == NULL) 
  {
    e->getTimer()->stop();
  }
  else
  {
    uint32_t id;
    for (id = 0; id != 3; id++)
    {
      if (e->getTimer() == &_buttonRepeat[id])
      {
        _buttonRepeat[id].setInterval(_buttonRepeatInterval[id]);
        _systemMouseStatus.uiWindow->onMousePress(1 << id, true);
        break;
      }
    }
  }
}

// ============================================================================
// [Fog::BaseUIWindow]
// ============================================================================

BaseUIWindow::BaseUIWindow(Widget* widget) : 
  UIWindow(widget),
  _windowRect(0, 0, 1, 1),
  _clientRect(0, 0, 1, 1),
  _sizeGranularity(1, 1)
{
}

BaseUIWindow::~BaseUIWindow()
{
  BaseUISystem* uiSystem = UI_SYSTEM();

  // Remove UIWindow from system mouse status.
  if (uiSystem->_systemMouseStatus.uiWindow == this)
  {
    uiSystem->clearSystemMouseStatus();
    uiSystem->clearButtonRepeat();
  }

  // Remove 'UIWindow's from dirty list.
  sysuint_t i = uiSystem->_dirtyList.indexOf(this);
  if (i != INVALID_INDEX) uiSystem->_dirtyList.set(i, NULL);
}

// ============================================================================
// [Fog::BaseUIWindow - Windowing System]
// ============================================================================

void BaseUIWindow::onEnabled(bool enabled)
{
  _enabled = enabled;
  UI_SYSTEM()->dispatchEnabled(_widget, enabled);
}

void BaseUIWindow::onVisibility(bool visible)
{
  _visible = visible;

  UI_SYSTEM()->dispatchVisibility(_widget, visible);
}

void BaseUIWindow::onConfigure(const Rect& windowRect, const Rect& clientRect)
{
  _windowRect = windowRect;
  _clientRect = clientRect;

  UI_SYSTEM()->dispatchConfigure(_widget, Rect(
    windowRect.getX(),
    windowRect.getY(),
    clientRect.getWidth(),
    clientRect.getHeight()), false);
}

void BaseUIWindow::onMouseHover(int x, int y)
{
  BaseUISystem* uiSystem = UI_SYSTEM();

  if (uiSystem->_systemMouseStatus.uiWindow != this)
  {
    uiSystem->_systemMouseStatus.uiWindow = this;
    uiSystem->_systemMouseStatus.buttons = 0;
  }
  uiSystem->_systemMouseStatus.position.set(x, y);
  uiSystem->_systemMouseStatus.hover = true;

  onMouseMove(x, y);
}

void BaseUIWindow::onMouseMove(int x, int y)
{
  BaseUISystem* uiSystem = UI_SYSTEM();
  Widget* w;

  if (uiSystem->_systemMouseStatus.uiWindow != this)
  {
    uiSystem->_systemMouseStatus.uiWindow = this;
    uiSystem->_systemMouseStatus.buttons = 0;
    uiSystem->_systemMouseStatus.hover = false;
  }
  uiSystem->_systemMouseStatus.position.set(x, y);

  Point p(x, y);

  // ----------------------------------
  // Grabbing mode
  // ----------------------------------

  if (uiSystem->_mouseStatus.buttons)
  {
    w = uiSystem->_mouseStatus.widget;
    p -= _widget->getOrigin();

    if (!Widget::translateCoordinates(w, _widget, &p))
    {
      FOG_ASSERT_NOT_REACHED();
    }

    uiSystem->changeMouseStatus(w, p);
    return;
  }

  // ----------------------------------
  // Motion mode
  // ----------------------------------

  // in motion mode, mouse should be in widget bounds (so check for it)
  if (x < 0 || y < 0 || x >= _widget->getWidth() || y >= _widget->getHeight())
  {
    return;
  }

  w = _widget;

__repeat:
  {
    // Add origin.
    p -= w->_origin;

    // Iterate over children and try to find child widget where  mouse
    // position is. Iteration must be done through end, becuase we want
    // widget with highest Z-Order to match mouse position first.
    List<Widget*>::ConstIterator it(w->_children);
    for (it.toEnd(); it.isValid(); it.toPrevious())
    {
      Widget* current = it.value();
      if (current->getVisibility() == WIDGET_VISIBLE && current->_rect.contains(p))
      {
        w = current;
        p -= w->getPosition();
        goto __repeat;
      }
    }

    uiSystem->changeMouseStatus(w, p);
  }
}

void BaseUIWindow::onMouseLeave(int x, int y)
{
  BaseUISystem* uiSystem = UI_SYSTEM();

  if (uiSystem->_systemMouseStatus.uiWindow == this)
  {
    Widget* w = uiSystem->_mouseStatus.widget;

    uiSystem->_mouseStatus.widget = NULL;
    uiSystem->_mouseStatus.position.set(-1, -1);;
    uiSystem->_mouseStatus.valid = 1;
    uiSystem->_mouseStatus.buttons = 0;
    uiSystem->_mouseStatus.hover = 0;

    uiSystem->clearSystemMouseStatus();
    uiSystem->clearButtonRepeat();

    if (w)
    {
      MouseEvent e(EV_MOUSE_OUT);
      e._position.set(-1, -1);

      w->sendEvent(&e);
    }
  }
}

void BaseUIWindow::onMousePress(uint32_t button, bool repeated)
{
  BaseUISystem* uiSystem = UI_SYSTEM();
  if (uiSystem->_systemMouseStatus.uiWindow != this) return;

  Widget* w = uiSystem->_mouseStatus.widget;
  if (!w) return;

  uiSystem->_systemMouseStatus.buttons |= button;
  uiSystem->_mouseStatus.buttons |= button;

  TimeTicks now = TimeTicks::now();

  if (!repeated) uiSystem->startButtonRepeat(
    button, true, uiSystem->_repeatingDelay, uiSystem->_repeatingInterval);

  MouseEvent e(EV_MOUSE_PRESS);
  e._button = button;
  e._modifiers = uiSystem->getKeyboardModifiers();
  e._position  = uiSystem->_mouseStatus.position;
  e._isOutside = ((e._position.x >= 0) &
                  (e._position.y >= 0) &
                  (e._position.x < w->getWidth()) &
                  (e._position.y < w->getHeight()));
  e._isRepeated = (repeated != 0);
  w->sendEvent(&e);

  uint32_t buttonId = fogButtonToId(button);
  if (repeated || buttonId == BUTTON_INVALID) return;

  if (!repeated && (now - uiSystem->_buttonTime[buttonId]) <= uiSystem->_doubleClickInterval)
  {
    uiSystem->_buttonTime[buttonId].clear();
    e._code = EV_DOUBLE_CLICK;
    w->sendEvent(&e);
  }
  else
  {
    uiSystem->_buttonTime[buttonId] = now;
  }
}

void BaseUIWindow::onMouseRelease(uint32_t button)
{
  BaseUISystem* uiSystem = UI_SYSTEM();
  if (uiSystem->_systemMouseStatus.uiWindow != this) return;

  uiSystem->_systemMouseStatus.buttons &= ~button;
  uiSystem->_mouseStatus.buttons &= ~button;

  uiSystem->stopButtonRepeat(button);

  Widget* w = uiSystem->_mouseStatus.widget;
  if (!w) return;

  bool lastButtonRelease = (uiSystem->_mouseStatus.buttons == 0);

  MouseEvent e(EV_MOUSE_RELEASE);
  e._button = button;
  e._modifiers = uiSystem->getKeyboardModifiers();
  e._position = uiSystem->_mouseStatus.position;
  e._isOutside = (
    e._position.x >= 0 &&
    e._position.y >= 0 &&
    e._position.x < w->getWidth() &&
    e._position.y < w->getHeight());
  w->sendEvent(&e);

  e._code = EV_CLICK;
  w->sendEvent(&e);

  if (lastButtonRelease)
  {
    onMouseMove(uiSystem->_systemMouseStatus.position.x,
                uiSystem->_systemMouseStatus.position.y);
  }
}

void BaseUIWindow::onMouseWheel(uint32_t wheel)
{
  BaseUISystem* uiSystem = UI_SYSTEM();
  if (uiSystem->_systemMouseStatus.uiWindow != this) return;

  Widget* w = uiSystem->_mouseStatus.widget;
  if (!w) return;

  MouseEvent e(EV_WHEEL);
  e._button = wheel;
  e._modifiers = uiSystem->getKeyboardModifiers();
  e._position = uiSystem->_mouseStatus.position;
  w->sendEvent(&e);
}

void BaseUIWindow::onFocus(bool focus)
{
  BaseUISystem* uiSystem = UI_SYSTEM();

  _hasFocus = focus;
  if (focus)
  {
    Widget* w = _widget->_findFocus();
    if (!w->_hasFocus)
    {
      FocusEvent e(EV_FOCUS_IN);
      _widget->_hasFocus = true;
      _widget->sendEvent(&e);
    }
  }
  else
  {
    uiSystem->_keyboardStatus.modifiers = 0;
  }
}

bool BaseUIWindow::onKeyPress(uint32_t key, uint32_t modifier, uint32_t systemCode, Char unicode)
{
  BaseUISystem* uiSystem = UI_SYSTEM();

  KeyEvent e(EV_KEY_PRESS);
  e._key = key;
  e._modifiers = uiSystem->getKeyboardModifiers();
  e._systemCode = systemCode;
  e._unicode = unicode;

  if (isShiftMod(uiSystem->_keyboardStatus.modifiers)) e._key |= KEY_SHIFT;
  if (isCtrlMod (uiSystem->_keyboardStatus.modifiers)) e._key |= KEY_CTRL;
  if (isAltMod  (uiSystem->_keyboardStatus.modifiers)) e._key |= KEY_ALT;

  // Set this status after modifiers check.
  uiSystem->_keyboardStatus.modifiers |= modifier;

  if ((e.getUnicode().ch() >= 1 && e.getUnicode().ch() <= 31) ||
      e.getUnicode().ch() == 127 /* DEL key */)
  {
    e._unicode._ch = 0;
  }

  _widget->_findFocus()->sendEvent(&e);
  return e.isAccepted();
}

bool BaseUIWindow::onKeyRelease(uint32_t key, uint32_t modifier, uint32_t systemCode, Char unicode)
{
  BaseUISystem* uiSystem = UI_SYSTEM();
  uiSystem->_keyboardStatus.modifiers &= ~modifier;

  KeyEvent e(EV_KEY_RELEASE);
  e._key = key;
  e._modifiers = uiSystem->getKeyboardModifiers();
  e._systemCode = systemCode;
  e._unicode._ch = 0;

  if (isShiftMod(uiSystem->_keyboardStatus.modifiers)) e._key |= KEY_SHIFT;
  if (isCtrlMod (uiSystem->_keyboardStatus.modifiers)) e._key |= KEY_CTRL;
  if (isAltMod  (uiSystem->_keyboardStatus.modifiers)) e._key |= KEY_ALT;

  _widget->_findFocus()->sendEvent(&e);
  return e.isAccepted();
}

void BaseUIWindow::clearFocus()
{
  Widget* w = _widget;

  // Need to clear focus
  while (w->_focusLink)
  {
    Widget* t = w->_focusLink;
    w->_focusLink = NULL;
    w = t;
  }

  if (w->_hasFocus)
  {
    w->_hasFocus = false;
    FocusEvent e(EV_FOCUS_OUT);
    w->sendEvent(&e);
  }
}

void BaseUIWindow::setFocus(Widget* w, uint32_t reason)
{
  if (_widget->_focusLink) clearFocus();

  Widget* t = w;
  while (t != _widget)
  {
    Widget* parent = t->getParent();
    parent->_focusLink = t;
    t = parent;
    FOG_ASSERT(t != NULL);
  }

  if (!_hasFocus) takeFocus();

  FocusEvent e(EV_FOCUS_IN, reason);
  w->_hasFocus = true;
  w->sendEvent(&e);
}

// ============================================================================
// [BaseUIWindow - Dirty]
// ============================================================================

void BaseUIWindow::setDirty()
{
  // Prevent setting isDirty flag more times. If it has been already set we just
  // do nothing.
  if (_isDirty) return;
  _isDirty = true;

  BaseUISystem* uiSystem = UI_SYSTEM();
  uiSystem->_dirtyList.append(this);
  uiSystem->update();
}

} // Fog namespace
