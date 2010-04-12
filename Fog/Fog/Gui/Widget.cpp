// [Fog-Gui Library - Public API]
//
// [License]
// MIT, See COPYING file in package

// [Precompiled Headers]
#if defined(FOG_PRECOMP)
#include FOG_PRECOMP
#endif // FOG_PRECOMP

// [Dependencies]
#include <Fog/Core/Application.h>
#include <Fog/Core/List.h>
#include <Fog/Core/Static.h>
#include <Fog/Core/String.h>
#include <Fog/Graphics/Region.h>
#include <Fog/Gui/Constants.h>
#include <Fog/Gui/GuiEngine.h>
#include <Fog/Gui/Layout/Layout.h>
#include <Fog/Gui/Layout/LayoutItem.h>
#include <Fog/Gui/Widget_p.h>

FOG_IMPLEMENT_OBJECT(Fog::Widget)

namespace Fog {

// ============================================================================
// [Fog::Widget - Construction / Destruction]
// ============================================================================

Widget::Widget(uint32_t createFlags) :
  _parent(NULL),
  _guiWindow(NULL),
  _geometry(0, 0, 0, 0),
  _clientGeometry(0, 0, 0, 0),
  _origin(0, 0),
  _layout(NULL),
  _layoutPolicy(0),
  _hasHeightForWidth(false),
  _isLayoutDirty(true),
  _lastFocus(NULL),
  _focusLink(NULL),
  _uflags(0),
  _state(WIDGET_ENABLED),
  _visibility(WIDGET_HIDDEN),
  _focusPolicy(FOCUS_NONE),
  _hasFocus(false),
  _orientation(ORIENTATION_HORIZONTAL),
  _reserved(0),
  _tabOrder(0),
  _windowFlags(createFlags)
{
  _flags |= OBJ_IS_WIDGET;

  // TODO ?
  _focusLink = NULL;

  if ((createFlags & WINDOW_TYPE_FLAG) != 0)
  {
    createWindow(createFlags);
  }
}

Widget::~Widget()
{
  deleteLayout();
  if (_guiWindow) destroyWindow();
}

// ============================================================================
// [Fog::Widget - Hierarchy]
// ============================================================================

bool Widget::setParent(Widget* p)
{
  if (p) return p->add(this);

  if (_parent == NULL)
    return true;
  else
    return _parent->remove(this);
}

bool Widget::add(Widget* w)
{
  // First remove the element from it's parent
  Widget* p = w->_parent;
  if (p != NULL && p != this) 
  {
    // Remove element can fail, so we return false in that case
    if (!p->remove(w)) return false;
  }

  // Now element can be added, call virtual method
  return _add(_children.getLength(), w);
}

bool Widget::remove(Widget* w)
{
  Widget* p = w->_parent;
  if (p != this) return false;

  return _remove(_children.indexOf(w), w);
}

bool Widget::_add(sysuint_t index, Widget* w)
{
  FOG_ASSERT(index <= _children.getLength());

  _children.insert(index, w);
  w->_parent = this;

  return true;
}

bool Widget::_remove(sysuint_t index, Widget* w)
{
  FOG_ASSERT(index < _children.getLength());
  FOG_ASSERT(_children.at(index) == w);

  _children.removeAt(index);
  w->_parent = NULL;

  return true;
}

// ============================================================================
// [Fog::Widget - GuiWindow]
// ============================================================================

GuiWindow* Widget::getClosestGuiWindow() const
{
  Widget* w = const_cast<Widget*>(this);
  do {
    if (w->_guiWindow) return w->_guiWindow;
    w = w->_parent;
  } while (w);

  return NULL;
}

err_t Widget::createWindow(uint32_t createFlags)
{
  if (_guiWindow) return ERR_GUI_WINDOW_ALREADY_EXISTS;

  GuiEngine* ge = Application::getInstance()->getGuiEngine();
  if (ge == NULL) return ERR_GUI_NO_ENGINE;

  _guiWindow = ge->createGuiWindow(this);

  err_t err = _guiWindow->create(createFlags);
  if (err) destroyWindow();

  return err;
}

err_t Widget::destroyWindow()
{
  if (!_guiWindow) return ERR_RT_INVALID_HANDLE;

  delete _guiWindow;
  _guiWindow = NULL;
  return true;
}

String Widget::getWindowTitle() const
{
  String title;
  if (_guiWindow) _guiWindow->getTitle(title);
  return title;
}

err_t Widget::setWindowTitle(const String& title)
{
  if (!_guiWindow) return ERR_RT_INVALID_HANDLE;
  return _guiWindow->setTitle(title);
}

Image Widget::getWindowIcon() const
{
  Image i;
  if (_guiWindow) _guiWindow->getIcon(i);
  return i;
}

err_t Widget::setWindowIcon(const Image& icon)
{
  if (!_guiWindow) return ERR_RT_INVALID_HANDLE;
  return _guiWindow->setIcon(icon);
}

IntPoint Widget::getWindowGranularity() const
{
  IntPoint sz(0, 0);
  if (_guiWindow) _guiWindow->getSizeGranularity(sz);
  return sz;
}

err_t Widget::setWindowGranularity(const IntPoint& pt)
{
  if (!_guiWindow) return ERR_RT_INVALID_HANDLE;
  return _guiWindow->setSizeGranularity(pt);
}

// ============================================================================
// [Fog::Widget - Geometry]
// ============================================================================

void Widget::setGeometry(const IntRect& geometry)
{
  if (_geometry == geometry) return;

  if (_guiWindow)
  {
    _guiWindow->reconfigure(geometry);
  }
  else
  {
    GuiEngine* ge = Application::getInstance()->getGuiEngine();
    if (!ge) return;

    ge->dispatchConfigure(this, geometry, false);
  }
}

void Widget::setPosition(const IntPoint& pt)
{
  if (_geometry.getPosition() == pt) return;

  if (_guiWindow)
  {
    _guiWindow->move(pt);
  }
  else
  {
    GuiEngine* ge = Application::getInstance()->getGuiEngine();
    if (!ge) return;

    IntSize size = getSize();
    ge->dispatchConfigure(this, IntRect(pt.x, pt.y, size.w, size.h), false);
  }
}

void Widget::setSize(const IntSize& sz)
{
  if (_geometry.getSize() == sz) return;

  if (_guiWindow)
  {
    _guiWindow->resize(sz);
  }
  else
  {
    GuiEngine* ge = Application::getInstance()->getGuiEngine();
    if (!ge) return;

    ge->dispatchConfigure(this, IntRect(_geometry.x, _geometry.y, sz.w, sz.h), false);
  }
}

void Widget::setOrigin(const IntPoint& pt)
{
  if (_origin == pt) return;

  OriginEvent e;
  e._origin = pt;

  _origin = pt;
  sendEvent(&e);

  update(WIDGET_UPDATE_ALL);
}

bool Widget::worldToClient(IntPoint* coords) const
{
  Widget* w = const_cast<Widget*>(this);

  do {
    // TODO, disable origin here?
    // coords->translate(w->getOrigin());

    if (w->_guiWindow) return w->_guiWindow->worldToClient(coords);
    coords->translate(-(w->_geometry.x), -(w->_geometry.y));
    w = w->_parent;
  } while (w);

  return false;
}

bool Widget::clientToWorld(IntPoint* coords) const
{
  Widget* w = const_cast<Widget*>(this);

  do {
    coords->translate(w->getOrigin());
    if (w->_guiWindow) return w->_guiWindow->clientToWorld(coords);
    coords->translate(w->_geometry.x, w->_geometry.y);
    w = w->_parent;
  } while (w);

  return false;
}

bool Widget::translateCoordinates(Widget* to, Widget* from, IntPoint* coords)
{
  Widget* w;
  int x, y;

  if (to == from) return true;

  // Traverse 'from' -> 'to'.
  w = from;
  x = coords->x;
  y = coords->y;

  while (w->_parent)
  {
    x += w->_origin.x;
    y += w->_origin.y;
    x += w->_geometry.x;
    y += w->_geometry.y;

    w = w->_parent;

    if (w == to)
    {
      coords->set(x, y);
      return true;
    }
  }

  // Traverse 'to' -> 'from'.
  w = to;
  x = coords->x;
  y = coords->y;

  while (w->_parent)
  {
    x -= w->_origin.x;
    y -= w->_origin.y;
    x -= w->_geometry.x;
    y -= w->_geometry.y;

    w = w->_parent;

    if (w == from)
    {
      coords->set(x, y);
      return true;
    }
  }

  // These widgets are not in relationship.
  return false;
}

// ============================================================================
// [Fog::Widget - Hit Testing]
// ============================================================================

Widget* Widget::hitTest(const IntPoint& pt) const
{
  int x = pt.getX();
  int y = pt.getY();

  if (x < 0 || y < 0 || x > _geometry.w || y > _geometry.h) return NULL;

  List<Widget*>::ConstIterator it(_children);
  for (it.toEnd(); it.isValid(); it.toPrevious())
  {
    if (it.value()->_geometry.contains(pt)) return it.value();
  }

  return const_cast<Widget*>(this);
}

Widget* Widget::getChildAt(const IntPoint& pt, bool recursive) const
{
  int x = pt.getX();
  int y = pt.getY();

  if (x < 0 || y < 0 || x > _geometry.w || y > _geometry.h) return NULL;

  Widget* current = const_cast<Widget*>(this);

repeat:
  {
    List<Widget*>::ConstIterator it(current->_children);
    for (it.toEnd(); it.isValid(); it.toPrevious())
    {
      if (it.value()->_geometry.contains(pt))
      {
        current = it.value();
        x -= current->getX();
        y -= current->getY();
        if (current->hasChildren()) goto repeat;
        break;
      }
    }
  }

  return current;
}

// ============================================================================
// [Fog::Widget - Layout]
// ============================================================================

void Widget::setLayout(Layout* lay)
{
  if (lay->_parentItem == this) return;
  if (lay->_parentItem)
  {
    fog_stderr_msg("Fog::Widget", "setLayout", "Can't set layout that has already parent");
    return;
  }

  deleteLayout();

  if (lay)
  {
    _layout = lay;
    lay->_parentItem = this;

    LayoutEvent e(EVENT_LAYOUT_SET);
    this->sendEvent(&e);
    lay->sendEvent(&e);

    invalidateLayout();
  }
}

void Widget::deleteLayout()
{
  Layout* lay = takeLayout();
  if (lay) lay->destroy();
}

Layout* Widget::takeLayout()
{
  Layout* lay = _layout;

  if (lay)
  {
    lay->_parentItem = NULL;
    _layout = NULL;

    invalidateLayout();

    LayoutEvent e(EVENT_LAYOUT_REMOVE);
    lay->sendEvent(&e);
    this->sendEvent(&e);
  }

  return lay;
}

// ============================================================================
// [Layout Hints]
// ============================================================================

const LayoutHint& Widget::getLayoutHint() const
{
  return (_layout) ? _layout->getLayoutHint() : _layoutHint;
}

void Widget::setLayoutHint(const LayoutHint& layoutHint)
{
  // GUI TODO:
  if (_layout)
  {
    _layout->setLayoutHint(layoutHint);
  }
  else
  {
    if (_layoutHint == layoutHint) return;
    _layoutHint = layoutHint;
    invalidateLayout();
  }
}

const LayoutHint& Widget::getComputedLayoutHint() const
{
  return _layout ? _layout->getComputedLayoutHint() : _layoutHint;
}

void Widget::computeLayoutHint()
{
  if (_layout) _layout->computeLayoutHint();
}

// ============================================================================
// [Layout Policy]
// ============================================================================

uint32_t Widget::getLayoutPolicy() const
{
  return _layoutPolicy;
}

void Widget::setLayoutPolicy(uint32_t policy)
{
  if (_layoutPolicy == policy) return;

  _layoutPolicy = policy;
  invalidateLayout();
}

// ============================================================================
// [Layout Height For Width]
// ============================================================================

bool Widget::hasHeightForWidth() const
{
  return _hasHeightForWidth;
}

int Widget::getHeightForWidth(int width) const
{
  return -1;
}

// ============================================================================
// [Layout State]
// ============================================================================

bool Widget::isLayoutDirty() const
{
  return _isLayoutDirty;
}

void Widget::invalidateLayout() const
{
  // Don't invalidate more times, it can be time consuming.
  if (_isLayoutDirty) return;

  // Invalidate widget that has layout.
  Widget* w = const_cast<Widget*>(this);
  while (w->_layout == NULL)
  {
    w = w->_parent;
    if (w == NULL) return;
  }

  w->_isLayoutDirty = true;
  w->_layout->invalidateLayout();

  if (w->_parent && w->_parent->_isLayoutDirty == false) w->_parent->invalidateLayout();
}

// ============================================================================
// [Fog::Widget - State]
// ============================================================================

void Widget::setEnabled(bool val)
{
  if ( val && _state != WIDGET_DISABLED) return;
  if (!val && _state == WIDGET_DISABLED) return;

  if (_guiWindow)
  {
    if (val)
      _guiWindow->enable();
    else
      _guiWindow->disable();
  }
  else
  {
    GuiEngine* ge = Application::getInstance()->getGuiEngine();
    if (!ge) return;

    ge->dispatchEnabled(this, val);
  }
}

// ============================================================================
// [Fog::Widget - Visibility]
// ============================================================================

void Widget::setVisible(bool val)
{
  if ( val && _visibility != WIDGET_HIDDEN) return;
  if (!val && _visibility == WIDGET_HIDDEN) return;

  if (_guiWindow)
  {
    if (val)
      _guiWindow->show();
    else
      _guiWindow->hide();
  }
  else
  {
    GuiEngine* ge = Application::getInstance()->getGuiEngine();
    if (!ge) return;

    ge->dispatchVisibility(this, val);
  }
}

// ============================================================================
// [Fog::Widget - Window Style]
// ============================================================================

void Widget::setWindowFlags(uint32_t flags) 
{
    if(flags == _windowFlags) return;

    if(_guiWindow) 
    {
        _guiWindow->create(flags);
    }

    _windowFlags = flags;
}

void Widget::setWindowHints(uint32_t flags) 
{
  if(flags == getWindowHints()) return;

  //make sure to keep window type and to only update the hints
  flags = (_windowFlags & WINDOW_TYPE_FLAG) | (flags & WINDOW_HINTS_FLAG);

  if(_guiWindow) 
  {
    _guiWindow->create(flags);
  }

  _windowFlags = flags;
}

void Widget::changeFlag(uint32_t flag, bool set, bool update) {
  if(set) 
  {
    _windowFlags |= flag;
  }
  else 
  {
    _windowFlags &= ~flag;
  }

  if(_guiWindow && update) 
  {
    _guiWindow->create(_windowFlags);
  }
}

void Widget::setDragAble(bool drag, bool update) 
{
  if(drag == isDragAble()) 
    return;
  changeFlag(WINDOW_DRAGABLE,drag,update);
}

void Widget::setResizeAble(bool resize, bool update) 
{
  if(resize == isResizeAble()) 
    return;
  changeFlag(WINDOW_FIXED_SIZE,!resize,update);
}

// ============================================================================
// [Fog::Widget - Orientation]
// ============================================================================

void Widget::setOrientation(uint32_t val)
{
  if (orientation() == val) return;

  _orientation = val;

  GuiEngine* ge = Application::getInstance()->getGuiEngine();
  if (!ge) return;

  ge->dispatchConfigure(this, _geometry, true);
}

// ============================================================================
// [Fog::Widget - Caret]
// ============================================================================

#if 0
bool Widget::showCaret()
{
  return 0;
}

bool Widget::showCaret(const CaretProperties& properties)
{
  return 0;
}

bool Widget::hideCaret()
{
  return 0;
}
#endif

// ============================================================================
// [Fog::Widget - TabOrder]
// ============================================================================

void Widget::setTabOrder(int tabOrder)
{
  _tabOrder = tabOrder;
}

// ============================================================================
// [Fog::Widget - Focus]
// ============================================================================

void Widget::setFocusPolicy(uint32_t val)
{
  _focusPolicy = val;
}

Widget* Widget::getFocusableWidget(int focusable)
{
  return NULL;
}

void Widget::takeFocus(uint32_t reason)
{
  if (!hasFocus() && getVisibility() == WIDGET_VISIBLE && getState() == WIDGET_ENABLED)
  {
    // TODO:
    //Application::getInstance()->getGuiEngine()->dispatchTakeFocus(this, reason);
  }
}

void Widget::giveFocusNext(uint32_t reason)
{
  Widget* w = getFocusableWidget(FOCUSABLE_NEXT);
  if (w) w->takeFocus(reason);
}

void Widget::giveFocusPrevious(uint32_t reason)
{
  Widget* w = getFocusableWidget(FOCUSABLE_PREVIOUS);
  if (w) w->takeFocus(reason);
}

Widget* Widget::_findFocus() const
{
  Widget* w = const_cast<Widget*>(this);
  while (w->_focusLink) w = w->_focusLink;
  return w;
}

// ============================================================================
// [Fog::Widget - Font]
// ============================================================================

void Widget::setFont(const Font& font)
{
  _font = font;
  update(WIDGET_UPDATE_GEOMETRY | WIDGET_REPAINT_ALL);
}

// ============================================================================
// [Fog::Widget - Update]
// ============================================================================

void Widget::update(uint32_t updateFlags)
{
  uint32_t u = _uflags;
  //if ((u & updateFlags) == updateFlags || (u & WIDGET_UPDATE_ALL)) return;

  _uflags |= updateFlags | WIDGET_UPDATE_SOMETHING;

  if (u & WIDGET_UPDATE_SOMETHING) return;

  if (_guiWindow)
  {
    if (!_guiWindow->isDirty()) _guiWindow->setDirty();
    return;
  }

  // No UFlagUpdate nor GuiWindow, traverse all parents up to GuiWindow and
  // set UFlagUpdateChild to all parents.
  Widget* w = _parent;
  while (w && !(w->_uflags & (WIDGET_UPDATE_CHILD | WIDGET_UPDATE_ALL)))
  {
    w->_uflags |= WIDGET_UPDATE_CHILD;
    if (w->_guiWindow)
    {
      if (!w->_guiWindow->isDirty()) w->_guiWindow->setDirty();
      return;
    }
    w = w->_parent;
  }
}

// ============================================================================
// [Fog::Widget - Repaint]
// ============================================================================

void Widget::repaint(uint32_t repaintFlags)
{
  update(repaintFlags);
}

uint32_t Widget::getPaintHint() const
{
  return WIDGET_PAINT_SCREEN;
}

err_t Widget::getPropagatedRegion(Region* dst) const
{
  return dst->set(IntBox(0, 0, getWidth(), getHeight()));
}

// ============================================================================
// [Fog::Widget - Events]
// ============================================================================

void Widget::onChildAdd(ChildEvent* e)
{
}

void Widget::onChildRemove(ChildEvent* e)
{
}

void Widget::onEnable(StateEvent* e)
{
}

void Widget::onDisable(StateEvent* e)
{
}

void Widget::onShow(VisibilityEvent* e)
{
}

void Widget::onHide(VisibilityEvent* e)
{
}

void Widget::onConfigure(ConfigureEvent* e)
{
}

void Widget::onFocusIn(FocusEvent* e)
{
}

void Widget::onFocusOut(FocusEvent* e)
{
}

void Widget::onKeyPress(KeyEvent* e)
{
}

void Widget::onKeyRelease(KeyEvent* e)
{
}

void Widget::onMouseIn(MouseEvent* e)
{
}

void Widget::onMouseOut(MouseEvent* e)
{
}

void Widget::onMouseMove(MouseEvent* e)
{
}

void Widget::onMousePress(MouseEvent* e)
{
}

void Widget::onMouseRelease(MouseEvent* e)
{
}

void Widget::onClick(MouseEvent* e)
{
}

void Widget::onDoubleClick(MouseEvent* e)
{
}

void Widget::onWheel(MouseEvent* e)
{
}

void Widget::onSelection(SelectionEvent* e)
{
}

void Widget::onPaint(PaintEvent* e)
{
}

void Widget::onClose(CloseEvent* e)
{
}

void Widget::onThemeChange(ThemeEvent* e)
{
}

void Widget::onLayout(LayoutEvent* e)
{
}

} // Fog namespace
