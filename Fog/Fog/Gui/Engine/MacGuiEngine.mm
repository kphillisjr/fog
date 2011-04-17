// [Fog-Gui]
//
// [License]
// MIT, See COPYING file in package

#include <Fog/Core/Config/Config.h>
#if defined(FOG_OS_MAC)

// [Dependencies]
#include <Fog/G2d/Mac/MacUtil.h>
#include <Fog/Gui/Engine/MacGuiEngine.h>
#include <Fog/Gui/Widget/Widget.h>
#include <limits>

#include <IOKit/IOMessage.h>
#include <IOKit/pwr_mgt/IOPMLib.h>

#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>

FOG_IMPLEMENT_OBJECT(Fog::MacGuiEngine)
FOG_IMPLEMENT_OBJECT(Fog::MacGuiWindow)

// ==============================================================================
// [FogView]
// ==============================================================================

@interface FogView : NSView
{
@public
  CGImageRef image;
}
@property(assign) CGImageRef image;
@end

@implementation FogView 

@synthesize image;

- (void)drawRect:(NSRect)rect
{
  CGContextRef ctx = (CGContextRef)[[NSGraphicsContext currentContext] graphicsPort];
  CGContextDrawImage(ctx, NSRectToCGRect(rect), image);
}

@end

// ==============================================================================
// [FogWindow]
// ==============================================================================

@interface FogWindow : NSWindow
{
  Fog::MacGuiWindow* fogWindow;
}
- (FogView*) contentView;
@end

@implementation FogWindow 

- (id)init:(Fog::MacGuiWindow*)fogWindow_ frame:(NSRect)frame styleMask:(NSUInteger)style contentView:(NSView*)view
{
  if (self = [super initWithContentRect: frame
                              styleMask: style
                                backing: NSBackingStoreBuffered
                                  defer: NO])
  {
    [self setContentView:view];
    
    // initialize notifications
    NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
    
    [center addObserver:self selector:@selector(windowBecameKey:)
            name:NSWindowDidBecomeKeyNotification object:self];

    [center addObserver:self selector:@selector(windowMoved:)
            name:NSWindowDidMoveNotification object:self];
    
    [center addObserver:self selector:@selector(windowResized:)
            name:NSWindowDidResizeNotification object:self];
    
    fogWindow = fogWindow_;
  }
  return self;
}

- (void)windowBecameKey:(NSNotification*)notification
{
  using namespace Fog;
  
  fogWindow->onVisibility(WIDGET_VISIBLE);
}

- (void)windowMoved:(NSNotification*)notification
{
  using namespace Fog;
  
  NSRect wr = [self frame];
  NSRect cr = [self contentRectForFrameRect: wr];
  fogWindow->onGeometry(RectI(wr.origin.x, wr.origin.y, wr.size.width, wr.size.height),
                        RectI(cr.origin.x, cr.origin.y, cr.size.width, cr.size.height));
}

- (void)windowResized:(NSNotification*)notification
{
  using namespace Fog;
  
  NSRect wr = [self frame];
  NSRect cr = [self contentRectForFrameRect: wr];
  fogWindow->onGeometry(RectI(wr.origin.x, wr.origin.y, wr.size.width, wr.size.height),
                        RectI(cr.origin.x, cr.origin.y, cr.size.width, cr.size.height));
}

- (void)keyDown:(NSEvent*)event
{
  using namespace Fog;
  
  uint32_t ch = (uint32_t)[[event charactersIgnoringModifiers] characterAtIndex:0];
  fogWindow->onKeyPress(ch, 0, 0, (Char)ch);
}

- (void)keyUp:(NSEvent*)event
{
  using namespace Fog;
  
  uint32_t ch = (uint32_t)[[event charactersIgnoringModifiers] characterAtIndex:0];
  fogWindow->onKeyRelease(ch, 0, 0, (Char)ch);
}

- (void)rightMouseUp:(NSEvent*)event
{
  using namespace Fog;
  
  fogWindow->onMouseRelease(BUTTON_RIGHT);
}

- (void)rightMouseDown:(NSEvent*)event
{
  using namespace Fog;
  
  fogWindow->onMousePress(BUTTON_RIGHT, false);
}

- (void)mouseMoved:(NSEvent*)event
{
  NSPoint convertedPoint = [[self contentView] convertPoint:[event locationInWindow] fromView:nil];
  fogWindow->onMouseMove(convertedPoint.x, convertedPoint.y);
}

- (void)mouseUp:(NSEvent*)event
{
  using namespace Fog;
  
  fogWindow->onMouseRelease(BUTTON_LEFT);
}

- (void)mouseDown:(NSEvent*)event
{
  using namespace Fog;
  
  fogWindow->onMousePress(BUTTON_LEFT, false);
}

- (FogView*)contentView
{
	return (FogView*)[super contentView];
}

@end

// ==============================================================================
// [Application Menu]
// ==============================================================================

static NSString* getApplicationName()
{
  NSString* result;
  
  NSDictionary* dict = (NSDictionary*)CFBundleGetInfoDictionary(CFBundleGetMainBundle());
  if (dict) result = [dict objectForKey: @"CFBundleName"];

  if (![result length]) result = [[NSProcessInfo processInfo] processName];

  return result;
}

static void createApplicationMenu()
{
  // -----------------------------------------------------------------------------------
  // [Create Main Menu]
  // -----------------------------------------------------------------------------------
  {
    NSMenu* mainMenu = [[NSMenu alloc] initWithTitle:@"MainMenu"];
    
    // ---------------------------------------------------------------------------------
    // [Create Apple Menu]
    // ---------------------------------------------------------------------------------
    {
	    NSMenuItem* appleMenu = [mainMenu addItemWithTitle:@"Apple" action:NULL keyEquivalent:@""];
	    NSMenu* submenu = [[NSMenu alloc] initWithTitle:@"Apple"];

      NSString* applicationName = getApplicationName();
      NSMenuItem* menuItem;
      
      menuItem = [submenu addItemWithTitle:[NSString stringWithFormat:@"%@ %@", NSLocalizedString(@"About", nil), applicationName]
      							action:@selector(orderFrontStandardAboutPanel:)
      					 keyEquivalent:@""];
      [menuItem setTarget:NSApp];
      
      [submenu addItem:[NSMenuItem separatorItem]];
      
      menuItem = [submenu addItemWithTitle:[NSString stringWithFormat:@"%@ %@", NSLocalizedString(@"Hide", nil), applicationName]
      							action:@selector(hide:)
      					 keyEquivalent:@"h"];
      [menuItem setTarget:NSApp];
      
      menuItem = [submenu addItemWithTitle:NSLocalizedString(@"Hide Others", nil)
      							action:@selector(hideOtherApplications:)
      					 keyEquivalent:@"h"];
      [menuItem setKeyEquivalentModifierMask:NSCommandKeyMask | NSAlternateKeyMask];
      [menuItem setTarget:NSApp];
      
      menuItem = [submenu addItemWithTitle:NSLocalizedString(@"Show All", nil)
      							action:@selector(unhideAllApplications:)
      					 keyEquivalent:@""];
      [menuItem setTarget:NSApp];
      
      [submenu addItem:[NSMenuItem separatorItem]];
      
      menuItem = [submenu addItemWithTitle:[NSString stringWithFormat:@"%@ %@", NSLocalizedString(@"Quit", nil), applicationName]
      							action:@selector(terminate:)
      					 keyEquivalent:@"q"];
      [menuItem setTarget:NSApp];
	    
	    [NSApp performSelector:@selector(setAppleMenu:) withObject:submenu];
	    
	    [mainMenu setSubmenu:submenu forItem:appleMenu];
    }
	  
	  // ----------------------------------------------------------------------------------
	  // [Create Window Menu]
	  // ----------------------------------------------------------------------------------
	  {
	    NSMenuItem* windowMenu = [mainMenu addItemWithTitle:@"Window" action:NULL keyEquivalent:@""];
	    NSMenu* submenu = [[NSMenu alloc] initWithTitle:@"Window"];

    	NSMenuItem* menuItem;
    	menuItem = [submenu addItemWithTitle:NSLocalizedString(@"Minimize", nil)
    								action:@selector(performMinimize:)
    						 keyEquivalent:@"m"];

    	menuItem = [submenu addItemWithTitle:NSLocalizedString(@"Zoom", nil)
    								action:@selector(performZoom:)
    						 keyEquivalent:@""];

    	[submenu addItem:[NSMenuItem separatorItem]];

    	menuItem = [submenu addItemWithTitle:NSLocalizedString(@"Bring All to Front", nil)
    								action:@selector(arrangeInFront:)
    						 keyEquivalent:@""];

	    [mainMenu setSubmenu:submenu forItem:windowMenu];
	    [NSApp setWindowsMenu:submenu];
	    
	    [NSApp setMainMenu:mainMenu];
    }
	}
}

namespace Fog {

// =====================================================================================
// [MacGuiWindow]
// =====================================================================================

MacGuiEngine::MacGuiEngine() : 
 GuiEngine()
{
  ProcessSerialNumber psn;

  if (!GetCurrentProcess(&psn))
  {
      TransformProcessType(&psn, kProcessTransformToForegroundApplication);
      SetFrontProcess(&psn);
  }

  if (NSApp == nil)
  {
    [NSApplication sharedApplication];
    [NSApp finishLaunching];
  }

  createApplicationMenu();
}

MacGuiEngine::~MacGuiEngine()
{
  [NSApp release];
}

void MacGuiEngine::updateDisplayInfo()
{
  NSRect screenRect = [[[NSScreen screens] objectAtIndex: 0] frame];
  
  _displayInfo.width = screenRect.size.width;
  _displayInfo.height = screenRect.size.height;
  //display->is16bitSwapped = ?
  //display->[r,g,b]Mask = ?
}

GuiWindow* MacGuiEngine::createGuiWindow(Widget* widget)
{
  return fog_new MacGuiWindow(widget);
}

void MacGuiEngine::destroyGuiWindow(GuiWindow* native)
{
  fog_delete(native);
}

void MacGuiEngine::doBlitWindow(GuiWindow* native, const BoxI* rects, sysuint_t count)
{
  MacGuiBackBuffer* back = reinterpret_cast<MacGuiBackBuffer*>(native->getBackBuffer());
  back->updateRects([static_cast<MacGuiWindow*>(native)->window contentView], rects, count);
}

void MacGuiEngine::maximize(GuiWindow* window)
{
  NSWindow* win = static_cast<MacGuiWindow*>(window)->window;
  [win zoom: nil];
}

void MacGuiEngine::minimize(GuiWindow* window)
{
  NSWindow* win = static_cast<MacGuiWindow*>(window)->window;
  [win miniaturize: nil];
}

// ==============================================================================
// [MacGuiWindow]
// ==============================================================================
 
MacGuiWindow::MacGuiWindow(Widget* widget)
 : GuiWindow(widget)
{
  _backingStore = fog_new MacGuiBackBuffer();
}

MacGuiWindow::~MacGuiWindow()
{
  fog_delete(_backingStore);
}

err_t MacGuiWindow::create(uint32_t flags)
{
  NSUInteger styleMask = NSBorderlessWindowMask |
      ((flags & !WINDOW_FRAMELESS) ? NSTitledWindowMask : 0) |
      ((flags & WINDOW_CLOSE_BUTTON) ? NSClosableWindowMask : 0) |
      ((flags & WINDOW_MINIMIZE) ? NSMiniaturizableWindowMask : 0) |
      ((flags & WINDOW_MAXIMIZE) ? NSResizableWindowMask : 0);
  

  NSRect frame = NSMakeRect(_widget->getX(), _widget->getY(), 500, 500);    
  window = [[FogWindow alloc] init: this
                             frame: frame
                         styleMask: styleMask
                       contentView: [[FogView alloc] init]];

	return ERR_OK;
}

err_t MacGuiWindow::destroy()
{
  [window performClose:nil];
  return ERR_OK;
}

err_t MacGuiWindow::enable()
{
  if (window == nil) return ERR_RT_INVALID_HANDLE;
  
  // TODO
  return ERR_OK;
}

err_t MacGuiWindow::disable()
{
  if (window == nil) return ERR_RT_INVALID_HANDLE;
  
  // TODO
  return ERR_OK;
}

err_t MacGuiWindow::setPosition(const PointI& pt)
{
  if (window == nil) return ERR_RT_INVALID_HANDLE;
  
  [window setFrameTopLeftPoint: toNSPoint(pt)];

  return ERR_OK;
}

err_t MacGuiWindow::setSize(const SizeI& size)
{
  if (window == nil) return ERR_RT_INVALID_HANDLE;
  
  // TODO
  return ERR_OK;
}

err_t MacGuiWindow::setGeometry(const RectI& geometry)
{
  if (window == nil) return ERR_RT_INVALID_HANDLE;
  if (!geometry.isValid()) return ERR_RT_INVALID_ARGUMENT;
  
  [window setFrame: toNSRect(geometry) display: YES];

  return ERR_OK;
} 

err_t MacGuiWindow::takeFocus()
{
  if (window == nil) return ERR_RT_INVALID_HANDLE;
  
  // TODO how?
  return ERR_OK;
}

err_t MacGuiWindow::setTitle(const String& title)
{
  if (window == nil) return ERR_RT_INVALID_HANDLE;
  
  [window setTitle: toNSString(title)];
  return ERR_OK;
}

err_t MacGuiWindow::getTitle(String& title)
{
  if (window == nil) return ERR_RT_INVALID_HANDLE;
  
  title = fromNSString([window title]);
  return ERR_OK;
}

err_t MacGuiWindow::setIcon(const Image& icon)
{
  if (window == nil) return ERR_RT_INVALID_HANDLE;
  
  //[[window standardWindowButton:NSWindowDocumentIconButton] setImage:toNSImage(icon)];
  return ERR_OK;
}

err_t MacGuiWindow::getIcon(Image& icon)
{
  if (window == nil) return ERR_RT_INVALID_HANDLE;
  
  // TODO
  return ERR_OK;
}

err_t MacGuiWindow::show(uint32_t state)
{
  if (window == nil) return ERR_RT_INVALID_HANDLE;
  
  [window makeKeyAndOrderFront: nil];

  return ERR_OK;
}

err_t MacGuiWindow::hide()
{
  if (window == nil) return ERR_RT_INVALID_HANDLE;

  [window orderOut: nil];

  return ERR_OK;
}

// Z-Order

void MacGuiWindow::moveToTop(GuiWindow* w)
{
  // TODO
}

void MacGuiWindow::moveToBottom(GuiWindow* w)
{
  // TODO
}

void MacGuiWindow::setTransparency(float val)
{
  if (window == nil) return;

  [window setOpaque: NO];
  [window setAlphaValue: val];
}

err_t MacGuiWindow::setSizeGranularity(const PointI& pt)
{
  if (window == nil) return ERR_RT_INVALID_HANDLE;
  
  // TODO
  return ERR_OK;
}

err_t MacGuiWindow::getSizeGranularity(PointI& pt)
{
  if (window == nil) return ERR_RT_INVALID_HANDLE;
  
  // TODO
  return ERR_OK;
}

err_t MacGuiWindow::worldToClient(PointI* coords)
{
  if (window == nil) return ERR_RT_INVALID_HANDLE;
  
  for (int i=0; i<sizeof(coords)/sizeof(coords[0]); ++i)
  {
    NSPoint clientPt = [window convertScreenToBase: toNSPoint(coords[i])];
    coords[i].setX(clientPt.x);
    coords[i].setY(clientPt.y);
  }
  
  return ERR_OK;
}

err_t MacGuiWindow::clientToWorld(PointI* coords)
{
  if (window == nil) return ERR_RT_INVALID_HANDLE;
  
  for (int i=0; i<sizeof(coords)/sizeof(coords[0]); ++i)
  {
    NSPoint clientPt = [window convertBaseToScreen: toNSPoint(coords[i])];
    coords[i].setX(clientPt.x);
    coords[i].setY(clientPt.y);
  }

  return ERR_OK;
}

void MacGuiWindow::setOwner(GuiWindow* owner)
{
  if (window == nil) return;

  [window setParentWindow: static_cast<MacGuiWindow*>(owner)->window];
}

void MacGuiWindow::releaseOwner()
{
  if (window == nil) return;
  
  // TODO
}

// ==============================================================================
// [MacGuiBackBuffer]
// ==============================================================================

MacGuiBackBuffer::MacGuiBackBuffer()
 : GuiBackBuffer()
{
}

MacGuiBackBuffer::~MacGuiBackBuffer()
{
}

bool MacGuiBackBuffer::resize(int width, int height, bool cache)
{
  int targetWidth = width;
  int targetHeight = height;

  bool destroyImage = false;
  bool createImage = false;

  if (width == 0 || height == 0)
  {
    destroyImage = true;
  }
  else
  {
    if (cache)
    {
      if (width <= _cachedSize.w && height <= _cachedSize.h)
      {
        // Cached.
        _buffer.size.w = width;
        _buffer.size.h = height;
        return true;
      }

      // Don't create smaller buffer that previous!
      targetWidth  = Math::max<int>(width, _cachedSize.w);
      targetHeight = Math::max<int>(height, _cachedSize.h);

      // Cache using 128x128 blocks.
      targetWidth  = (targetWidth  + 127) & ~127;
      targetHeight = (targetHeight + 127) & ~127;
    }

    destroyImage = true;
    createImage = (targetWidth > 0 && targetHeight > 0);
  }

  // Destroy image buffer
  if (destroyImage)
  {
    Memory::free(_primaryPixels);
  }

  // Create image buffer.
  if (createImage)
  {
    _depth = [[NSScreen mainScreen] depth];
    _primaryStride = targetWidth * 4;
    
    _primaryPixels = (uint8_t*)Memory::alloc(_primaryStride * targetHeight);

    _buffer.stride = _primaryStride;
    _buffer.data = _primaryPixels;
    _buffer.size.w = _cachedSize.w = targetWidth;
    _buffer.size.h = _cachedSize.h = targetHeight;
    _buffer.format = IMAGE_FORMAT_PRGB32;

    // Secondary buffer not used on this platform.
    _secondaryPixels = NULL;
    _secondaryStride = 0;
  }
  
  return true;
}

void MacGuiBackBuffer::destroy()
{
  resize(0, 0, false);
}

void MacGuiBackBuffer::updateRects(const BoxI* rects, sysuint_t count)
{
}

void MacGuiBackBuffer::updateRects(FogView* view, const BoxI* rects, sysuint_t count)
{
  if (_buffer.size.w == 0 || _buffer.size.h == 0) return;
  
  int size = _primaryStride * _buffer.size.h;

  CGDataProviderRef provider = 
    CGDataProviderCreateWithData(NULL, _buffer.data, size, NULL);
  
  CGImageRef image = CGImageCreate(_buffer.size.w, _buffer.size.h, // size 
                                   8, 32, _buffer.stride, // bits/component, bits/pixel, bytes/line
                                    CGColorSpaceCreateDeviceRGB(), // colorspace
                                    0, // bitmap info
                                    provider,
                                    NULL,
                                    false,
                                    kCGRenderingIntentDefault);
  
  if (view.image) CGImageRelease(view.image);
  view.image = image;
  
  if (view.image == NULL)
  {
    Debug::failFunc("Fog::MacGuiBackBuffer", "updateRects", "Empty image");
  }

  for (sysint_t i=0; i<count; ++i)
  {
    [view drawRect: toNSRect(rects[i].toRectF())];
  }
}

// ==============================================================================
// [MacEventLoop]
// ==============================================================================

const CFTimeInterval kCFTimeIntervalMax = std::numeric_limits<CFTimeInterval>::max();

MacEventLoopBase::MacEventLoopBase() : 
 EventLoop(Ascii8("Gui.Mac"))
{
  _delayedWorkFireTime_ = kCFTimeIntervalMax;

  _runLoop = CFRunLoopGetCurrent();
  CFRetain(_runLoop);
  
  // Set a repeating timer with a preposterous firing time and interval.  The
  // timer will effectively never fire as-is.  The firing time will be adjusted
  // as needed when ScheduleDelayedWork is called.
  CFRunLoopTimerContext timerContext = CFRunLoopTimerContext();
  timerContext.info = this;
  _delayedWorkTimer = CFRunLoopTimerCreate(NULL,                // allocator
                                             kCFTimeIntervalMax,  // fire time
                                             kCFTimeIntervalMax,  // interval
                                             0,                   // flags
                                             0,                   // priority
                                             runDelayedWorkTimer,
                                             &timerContext);
  CFRunLoopAddTimer(_runLoop, _delayedWorkTimer, kCFRunLoopCommonModes);
  
  // run work has 1st priority
  CFRunLoopSourceContext sourceContext = CFRunLoopSourceContext();
  sourceContext.info = this;
  sourceContext.perform = runWorkSource;
  _workSource = CFRunLoopSourceCreate(NULL, 1, &sourceContext);
  CFRunLoopAddSource(_runLoop, _workSource, kCFRunLoopCommonModes);
  
  // run delayed work has 2nd priority
  sourceContext.perform = runDelayedWorkSource;
  _delayedWorkSource = CFRunLoopSourceCreate(NULL, 2, &sourceContext);
  CFRunLoopAddSource(_runLoop, _delayedWorkSource, kCFRunLoopCommonModes);
  
  // run idle work has 3rd priority
  sourceContext.perform = runIdleWorkSource;
  _idleWorkSource = CFRunLoopSourceCreate(NULL, 3, &sourceContext);
  CFRunLoopAddSource(_runLoop, _idleWorkSource, kCFRunLoopCommonModes);
}

MacEventLoopBase::~MacEventLoopBase()
{
  CFRunLoopRemoveSource(_runLoop, _idleWorkSource, kCFRunLoopCommonModes);
  CFRelease(_idleWorkSource);

  CFRunLoopRemoveSource(_runLoop, _delayedWorkSource, kCFRunLoopCommonModes);
  CFRelease(_delayedWorkSource);

  CFRunLoopRemoveSource(_runLoop, _workSource, kCFRunLoopCommonModes);
  CFRelease(_workSource);

  CFRunLoopRemoveTimer(_runLoop, _delayedWorkTimer, kCFRunLoopCommonModes);
  CFRelease(_delayedWorkTimer);

  CFRelease(_runLoop);
}

void MacEventLoopBase::_runInternal()
{
  CFRunLoopSourceSignal(_workSource);
  CFRunLoopSourceSignal(_delayedWorkSource);
  CFRunLoopSourceSignal(_idleWorkSource);
  
  _doRunInternal();
}

void MacEventLoopBase::_scheduleWork()
{
  CFRunLoopSourceSignal(_workSource);
  CFRunLoopWakeUp(_runLoop);
}

void MacEventLoopBase::_scheduleDelayedWork(const Time& delayedWorkTime)
{
  Time::Exploded now;
  delayedWorkTime.utcExplode(&now);
  double seconds = now.second + (static_cast<double>((delayedWorkTime.toInternalValue()) % Time::MicrosecondsPerSecond) /
                                                      Time::MicrosecondsPerSecond);
  CFGregorianDate gregorian = { now.year, now.month, now.dayOfMonth, now.hour,now.minute, seconds };
  _delayedWorkFireTime_ = CFGregorianDateGetAbsoluteTime(gregorian, NULL);

  CFRunLoopTimerSetNextFireDate(_delayedWorkTimer, _delayedWorkFireTime_);
}

void MacEventLoopBase::runDelayedWorkTimer(CFRunLoopTimerRef timer, void* info)
{
  MacEventLoopBase* self = static_cast<MacEventLoopBase*>(info);
  self->_delayedWorkFireTime_ = kCFTimeIntervalMax;
  CFRunLoopSourceSignal(self->_delayedWorkSource);
}

bool MacEventLoopBase::runWork()
{
  bool did_work = _doWork();
  if (did_work) CFRunLoopSourceSignal(_workSource);
  return did_work;
}

bool MacEventLoopBase::runDelayedWork()
{
  _doDelayedWork(&_delayedWorkTime);

  bool more_work = !_delayedWorkTime.isNull();
  if (!more_work) return false;

  TimeDelta delay = _delayedWorkTime - Time::now();
  if (delay > TimeDelta()) _scheduleDelayedWork(_delayedWorkTime);
  else CFRunLoopSourceSignal(_delayedWorkSource); // direct comeback!

  return true;
}

// Called by MacEventLoopBase::runIdleWorkSource.
bool MacEventLoopBase::runIdleWork()
{
  bool did_work = _doIdleWork();
  if (did_work) CFRunLoopSourceSignal(_idleWorkSource);
  return did_work;
}

void MacMainEventLoop::_doRunInternal()
{
  if (![NSApp isRunning])
  {
    [NSApp run];
  }
}

void MacMainEventLoop::quit()
{
  [NSApp terminate:nil];
}

} // namespace Fog


#endif // defined(FOG_OS_MAC)