// [Fog/Gui Library - Public API]
//
// [Licence]
// MIT, See COPYING file in package

#include <Fog/Build/Build.h>

#if defined(FOG_UI_WINDOWS)

// [Dependencies]
#include <Fog/Core/Application.h>
#include <Fog/Gui/Constants.h>
#include <Fog/Gui/GuiEngine/Win.h>
#include <Fog/Gui/Widget.h>

// ----------------------------------------------------------------------------
#ifndef MSH_MOUSEWHEEL
# define MSH_MOUSEWHEEL L"MSWHEEL_ROLLMSG"
#endif

#ifndef WHEEL_DELTA
# define WHEEL_DELTA 120
#endif

#ifndef WM_MOUSEWHEEL
# define WM_MOUSEWHEEL (WM_MOUSELAST+1)
#endif

#ifndef MOUSEZ_CLASSNAME
# define MOUSEZ_CLASSNAME L"MouseZ"
#endif

#ifndef MOUSEZ_TITLE
# define MOUSEZ_TITLE L"Magellan MSWHEEL"
#endif

#define MSH_WHEELMODULE_CLASS (MOUSEZ_CLASSNAME)
#define MSH_WHEELMODULE_TITLE (MOUSEZ_TITLE)

#ifndef MSH_WHEELSUPPORT
// MSH_WHEELSUPPORT returns TRUE if wheel support is active, otherwise FALSE
# define MSH_WHEELSUPPORT L"MSH_WHEELSUPPORT_MSG"
#endif

#ifndef MSH_SCROLL_LINES
# define MSH_SCROLL_LINES L"MSH_SCROLL_LINES_MSG"
#endif
                                
#ifndef WHEEL_PAGESCROLL  
// MSH_SCROLL_LINES returns number of lines to scroll
# define WHEEL_PAGESCROLL (UINT_MAX)
#endif 

#ifndef SPI_SETWHEELSCROLLLINES
# define SPI_SETWHEELSCROLLLINES 105
#endif

// Keymasks
#ifndef REPEATED_KEYMASK
# define REPEATED_KEYMASK (1 << 30)
#endif

#ifndef EXTENDED_KEYMASK
# define EXTENDED_KEYMASK (1 << 24)
#endif
// ----------------------------------------------------------------------------

FOG_IMPLEMENT_OBJECT(Fog::WinGuiEngine)
FOG_IMPLEMENT_OBJECT(Fog::WinGuiWindow)

namespace Fog {

// ============================================================================
// [Helpers]
// ============================================================================

#define GUI_ENGINE() \
  reinterpret_cast<WinGuiEngine*>(Application::getInstance()->getGuiEngine())

static void hwndGetRect(HWND handle, Rect* out)
{
  RECT wr;
  RECT cr;

  GetWindowRect(handle, &wr);
  GetClientRect(handle, &cr);

  int cw = cr.right - cr.left;
  int ch = cr.bottom - cr.top;

  out->set(wr.left, wr.top, cw, ch);
}

static LRESULT CALLBACK hwndWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  WinUIWindow* window = 
    reinterpret_cast<WinUIWindow*>(
      GUI_ENGINE()->handleToNative((void*)hwnd));

  if (window)
    return window->onWinMsg(hwnd, message, wParam, lParam);
  else
    return DefWindowProc(hwnd, message, wParam, lParam);
}

// Returns count of lines to scroll when using mouse wheel, inspired in
// MSDN http://msdn2.microsoft.com/en-us/library/ms645602.aspx
static uint32_t getWheelLinesCount(void)
{
  UINT ucNumLines = 3;
  SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &ucNumLines, 0);
  return ucNumLines;
}

// ============================================================================
// [Fog::WinGuiEngine - Construction / Destruction]
// ============================================================================

WinGuiEngine::WinGuiEngine()
{
  memset(&_winKeymap, 0, sizeof(_winKeymap));

  // Initialize key translation tables.
  Fog::Memory::zero(&_winKeymap, sizeof(WinKeymap));

  _winKeymap.vk[VK_BACK] = KEY_BACKSPACE;
  _winKeymap.vk[VK_TAB] = KEY_TAB;
  _winKeymap.vk[VK_CLEAR] = KEY_CLEAR;
  _winKeymap.vk[VK_RETURN] = KEY_ENTER;
  _winKeymap.vk[VK_PAUSE] = KEY_PAUSE;
  _winKeymap.vk[VK_ESCAPE] = KEY_ESC;
  _winKeymap.vk[VK_SPACE] = KEY_SPACE;
  _winKeymap.vk[0xDE] = KEY_QUOTE;
  _winKeymap.vk[0xBC] = KEY_COMMA;
  _winKeymap.vk[0xBD] = KEY_MINUS;
  _winKeymap.vk[0xBE] = KEY_PERIOD;
  _winKeymap.vk[0xBF] = KEY_SLASH;
  _winKeymap.vk['0'] = KEY_0;
  _winKeymap.vk['1'] = KEY_1;
  _winKeymap.vk['2'] = KEY_2;
  _winKeymap.vk['3'] = KEY_3;
  _winKeymap.vk['4'] = KEY_4;
  _winKeymap.vk['5'] = KEY_5;
  _winKeymap.vk['6'] = KEY_6;
  _winKeymap.vk['7'] = KEY_7;
  _winKeymap.vk['8'] = KEY_8;
  _winKeymap.vk['9'] = KEY_9;
  _winKeymap.vk[0xBA] = KEY_SEMICOLON;
  _winKeymap.vk[0xBB] = KEY_EQUALS;
  _winKeymap.vk[0xDB] = KEY_LEFT_BRACE;
  _winKeymap.vk[0xDC] = KEY_BACKSLASH;
  _winKeymap.vk[0xE2] = KEY_LESS;
  _winKeymap.vk[0xDD] = KEY_RIGHT_BRACE;
  _winKeymap.vk[0xC0] = KEY_BACKQUOTE;
  _winKeymap.vk[0xDF] = KEY_BACKQUOTE;
  _winKeymap.vk['A'] = KEY_A;
  _winKeymap.vk['B'] = KEY_B;
  _winKeymap.vk['C'] = KEY_C;
  _winKeymap.vk['D'] = KEY_D;
  _winKeymap.vk['E'] = KEY_E;
  _winKeymap.vk['F'] = KEY_F;
  _winKeymap.vk['G'] = KEY_G;
  _winKeymap.vk['H'] = KEY_H;
  _winKeymap.vk['I'] = KEY_I;
  _winKeymap.vk['J'] = KEY_J;
  _winKeymap.vk['K'] = KEY_K;
  _winKeymap.vk['L'] = KEY_L;
  _winKeymap.vk['M'] = KEY_M;
  _winKeymap.vk['N'] = KEY_N;
  _winKeymap.vk['O'] = KEY_O;
  _winKeymap.vk['P'] = KEY_P;
  _winKeymap.vk['Q'] = KEY_Q;
  _winKeymap.vk['R'] = KEY_R;
  _winKeymap.vk['S'] = KEY_S;
  _winKeymap.vk['T'] = KEY_T;
  _winKeymap.vk['U'] = KEY_U;
  _winKeymap.vk['V'] = KEY_V;
  _winKeymap.vk['W'] = KEY_W;
  _winKeymap.vk['X'] = KEY_X;
  _winKeymap.vk['Y'] = KEY_Y;
  _winKeymap.vk['Z'] = KEY_Z;
  _winKeymap.vk[VK_DELETE] = KEY_DELETE;

  _winKeymap.vk[VK_NUMPAD0] = KEY_KP_0;
  _winKeymap.vk[VK_NUMPAD1] = KEY_KP_1;
  _winKeymap.vk[VK_NUMPAD2] = KEY_KP_2;
  _winKeymap.vk[VK_NUMPAD3] = KEY_KP_3;
  _winKeymap.vk[VK_NUMPAD4] = KEY_KP_4;
  _winKeymap.vk[VK_NUMPAD5] = KEY_KP_5;
  _winKeymap.vk[VK_NUMPAD6] = KEY_KP_6;
  _winKeymap.vk[VK_NUMPAD7] = KEY_KP_7;
  _winKeymap.vk[VK_NUMPAD8] = KEY_KP_8;
  _winKeymap.vk[VK_NUMPAD9] = KEY_KP_9;
  _winKeymap.vk[VK_DECIMAL] = KEY_KP_PERIOD;
  _winKeymap.vk[VK_DIVIDE] = KEY_KP_DIVIDE;
  _winKeymap.vk[VK_MULTIPLY] = KEY_KP_MULTIPLY;
  _winKeymap.vk[VK_SUBTRACT] = KEY_KP_MINUS;
  _winKeymap.vk[VK_ADD] = KEY_KP_PLUS;

  _winKeymap.vk[VK_UP] = KEY_UP;
  _winKeymap.vk[VK_DOWN] = KEY_DOWN;
  _winKeymap.vk[VK_RIGHT] = KEY_RIGHT;
  _winKeymap.vk[VK_LEFT] = KEY_LEFT;
  _winKeymap.vk[VK_INSERT] = KEY_INSERT;
  _winKeymap.vk[VK_HOME] = KEY_HOME;
  _winKeymap.vk[VK_END] = KEY_END;
  _winKeymap.vk[VK_PRIOR] = KEY_PAGE_UP;
  _winKeymap.vk[VK_NEXT] = KEY_PAGE_DOWN;

  _winKeymap.vk[VK_F1] = KEY_F1;
  _winKeymap.vk[VK_F2] = KEY_F2;
  _winKeymap.vk[VK_F3] = KEY_F3;
  _winKeymap.vk[VK_F4] = KEY_F4;
  _winKeymap.vk[VK_F5] = KEY_F5;
  _winKeymap.vk[VK_F6] = KEY_F6;
  _winKeymap.vk[VK_F7] = KEY_F7;
  _winKeymap.vk[VK_F8] = KEY_F8;
  _winKeymap.vk[VK_F9] = KEY_F9;
  _winKeymap.vk[VK_F10] = KEY_F10;
  _winKeymap.vk[VK_F11] = KEY_F11;
  _winKeymap.vk[VK_F12] = KEY_F12;
  _winKeymap.vk[VK_F13] = KEY_F13;
  _winKeymap.vk[VK_F14] = KEY_F14;
  _winKeymap.vk[VK_F15] = KEY_F15;

  _winKeymap.vk[VK_NUMLOCK] = KEY_NUM_LOCK;
  _winKeymap.vk[VK_CAPITAL] = KEY_CAPS_LOCK;
  _winKeymap.vk[VK_SCROLL] = KEY_SCROLL_LOCK;
  _winKeymap.vk[VK_RSHIFT] = KEY_RIGHT_SHIFT;
  _winKeymap.vk[VK_LSHIFT] = KEY_LEFT_SHIFT;
  _winKeymap.vk[VK_RCONTROL] = KEY_RIGHT_CTRL;
  _winKeymap.vk[VK_LCONTROL] = KEY_LEFT_CTRL;
  _winKeymap.vk[VK_RMENU] = KEY_RIGHT_ALT;
  _winKeymap.vk[VK_LMENU] = KEY_LEFT_ALT;
  _winKeymap.vk[VK_RWIN] = KEY_RIGHT_SUPER;
  _winKeymap.vk[VK_LWIN] = KEY_LEFT_SUPER;

  _winKeymap.vk[VK_HELP] = KEY_HELP;
  _winKeymap.vk[42 /*VK_PRINT*/] = KEY_PRINT;
  _winKeymap.vk[VK_SNAPSHOT] = KEY_PRINT;
  _winKeymap.vk[VK_CANCEL] = KEY_BREAK;
  _winKeymap.vk[VK_APPS] = KEY_MENU;

  uMSH_MOUSEWHEEL = RegisterWindowMessageW(MSH_MOUSEWHEEL);

  // Get Wheel lines count.
  _wheelLines = getWheelLinesCount();

  // Query timings.
  INT keyboardDelay;
  DWORD keyboardSpeed;
  DWORD dwTime;

  if (SystemParametersInfo(SPI_GETKEYBOARDDELAY, 0, &keyboardDelay, 0))
  {
    _repeatingDelay = TimeDelta::fromMilliseconds((int64_t)((0.25 * (double)(keyboardDelay + 1)) * 1000));
  }

  if (SystemParametersInfo(SPI_GETKEYBOARDSPEED, 0, &keyboardSpeed, 0))
  {
    _repeatingInterval = TimeDelta::fromMilliseconds((int64_t)((1 / (2.5 + (double)keyboardSpeed * 0.90)) * 1000));
  }

  if (SystemParametersInfo(SPI_SETDOUBLECLICKTIME, 0, &dwTime, 0))
  {
    _doubleClickInterval = TimeDelta::fromMilliseconds(dwTime);
  }

  HINSTANCE hInstance = (HINSTANCE)GetModuleHandleW(NULL);
  WNDCLASSEXW wc;

  // Init Window Class structure.
  wc.cbSize        = sizeof(WNDCLASSEXW);
  wc.style         = 0;
  wc.lpfnWndProc   = hwndWndProc;
  wc.cbClsExtra    = 0;
  wc.cbWndExtra    = 0;
  wc.hInstance     = hInstance;
  wc.hIcon         = LoadIconW(NULL, (LPCWSTR)IDI_APPLICATION);
  wc.hCursor       = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
  wc.hbrBackground = NULL;
  wc.lpszMenuName  = NULL;
  wc.lpszClassName = NULL;
  wc.hIconSm       = LoadIconW(NULL, (LPCWSTR)IDI_APPLICATION);

  // "Fog_Window" window class.
  wc.style         = CS_OWNDC;
  wc.lpszClassName = L"Fog_Window";
  if (!RegisterClassExW(&wc)) return;

  // "Fog_Popup" window class.
  wc.style         = CS_OWNDC;
  wc.lpszClassName = L"Fog_Popup";
  if (!RegisterClassExW(&wc)) return;

  updateDisplayInfo();
  _initialized = true;
}

WinGuiEngine::~WinGuiEngine()
{
  // Is safe to unregister window classes when we ends ?
  // HINSTANCE hInstance = (HINSTANCE)GetModuleHandleW(NULL);
  // UnregisterClassW(L"Fog_Window", hInstance);
  // UnregisterClassW(L"Fog_Popup", hInstance);
}

// ============================================================================
// [Fog::WinGuiEngine - Display]
// ============================================================================

void WinGuiEngine::updateDisplayInfo()
{
  int dibSize;
  LPBITMAPINFOHEADER dibHdr;
  HDC hdc;
  HBITMAP hbm;

  // Screen size
  _displayInfo.width  = ::GetSystemMetrics(SM_CXSCREEN);
  _displayInfo.height = ::GetSystemMetrics(SM_CYSCREEN);
  _displayInfo.is16BitSwapped = false;

  // Allocate enough space for a DIB header plus palette 
  // (for 8-bit modes) or bitfields (for 16 and 32-bit modes)
  dibSize = sizeof(BITMAPINFOHEADER) + 256 * sizeof (RGBQUAD);

  dibHdr = (LPBITMAPINFOHEADER)Memory::calloc(dibSize);
  if (!dibHdr) return;

  dibHdr->biSize = sizeof(BITMAPINFOHEADER);

  // Get a device-dependent bitmap that's compatible with the screen.
  hdc = GetDC(NULL);
  hbm = CreateCompatibleBitmap(hdc, 1, 1);

  // Convert the DDB to a DIB.  We need to call GetDIBits twice:
  // the first call just fills in the BITMAPINFOHEADER; the 
  // second fills in the bitfields or palette.
  GetDIBits(hdc, hbm, 0, 1, NULL, (LPBITMAPINFO)dibHdr, DIB_RGB_COLORS);
  GetDIBits(hdc, hbm, 0, 1, NULL, (LPBITMAPINFO)dibHdr, DIB_RGB_COLORS);
  DeleteObject(hbm);
  ReleaseDC(NULL, hdc);

  _displayInfo.depth = dibHdr->biBitCount;

  _paletteInfo.palCount = 0;
  memset(_paletteInfo.palConv, 0, sizeof(_paletteInfo.palConv));

  switch (dibHdr->biBitCount)
  {
    case 8:
      _paletteInfo.palCount = 256;
      break;
    case 16:
      if (dibHdr->biCompression == BI_BITFIELDS)
      {
        _displayInfo.rMask = ((DWORD*)((uint8_t*)dibHdr + dibHdr->biSize))[0];
        _displayInfo.gMask = ((DWORD*)((uint8_t*)dibHdr + dibHdr->biSize))[1];
        _displayInfo.bMask = ((DWORD*)((uint8_t*)dibHdr + dibHdr->biSize))[2];
      }
      else
      {
        // I don't know if this can happen...
        _displayInfo.rMask = 0x0000F800;
        _displayInfo.gMask = 0x000007E0;
        _displayInfo.bMask = 0x0000001F;
      }
      break;
    case 24:
      _displayInfo.rMask = 0x00FF0000;
      _displayInfo.gMask = 0x0000FF00;
      _displayInfo.bMask = 0x000000FF;
      break;
    case 32:
      _displayInfo.rMask = 0x00FF0000;
      _displayInfo.gMask = 0x0000FF00;
      _displayInfo.bMask = 0x000000FF;
      break;
    default:
      _displayInfo.depth = 0;
      _displayInfo.rMask = 0x00000000;
      _displayInfo.gMask = 0x00000000;
      _displayInfo.bMask = 0x00000000;
      break;
  }

  Memory::free(dibHdr);
}

// ============================================================================
// [Fog::WinGuiEngine - Update]
// ============================================================================

void WinGuiEngine::doBlitWindow(UIWindow* window, const Box* rects, sysuint_t count)
{
  HDC hdc = GetDC((HWND)window->getHandle());
  reinterpret_cast<WinUIBackingStore*>(window->_backingStore)->blitRects(hdc, rects, count);
  ReleaseDC((HWND)window->getHandle(), hdc);
}

// ============================================================================
// [Fog::WinGuiEngine - UIWindow]
// ============================================================================

UIWindow* WinGuiEngine::createUIWindow(Widget* widget)
{
  return new(std::nothrow) WinUIWindow(widget);
}

void WinGuiEngine::destroyUIWindow(UIWindow* native)
{
  delete native;
}

// ============================================================================
// [Fog::WinGuiEngine - Windows Specific]
// ============================================================================

uint32_t WinGuiEngine::winKeyToModifier(WPARAM* wParam, LPARAM lParam)
{
  uint32_t modifier = 0;

  switch (*wParam)
  {
    case VK_SHIFT:
    {
      if ((_keyboardStatus.modifiers & MODIFIER_LEFT_SHIFT) != 0 && !(GetKeyState(VK_LSHIFT) & 0x8000))
      {
        *wParam = VK_LSHIFT;
      } 
      else if ((_keyboardStatus.modifiers & MODIFIER_RIGHT_SHIFT) != 0 && !(GetKeyState(VK_RSHIFT) & 0x8000))
      {
        *wParam = VK_RSHIFT;
      } 
      else 
      {
        // Win9x
        int sc = HIWORD(lParam) & 0xFF;

        if (sc == 0x2A)
          *wParam = VK_LSHIFT;
        else if (sc == 0x36)
          *wParam = VK_RSHIFT;
        else
          *wParam = VK_LSHIFT;
      }
      break;
    }

    case VK_CONTROL:
      if (lParam & EXTENDED_KEYMASK)
        *wParam = VK_RCONTROL;
      else
        *wParam = VK_LCONTROL;
      break;

    case VK_MENU:
      if (lParam & EXTENDED_KEYMASK)
        *wParam = VK_RMENU;
      else
        *wParam = VK_LMENU;
      break;
  }

  switch (*wParam)
  {
    case VK_LSHIFT  : modifier = MODIFIER_LEFT_SHIFT ; break;
    case VK_RSHIFT  : modifier = MODIFIER_RIGHT_SHIFT; break;
    case VK_LCONTROL: modifier = MODIFIER_LEFT_CTRL  ; break;
    case VK_RCONTROL: modifier = MODIFIER_RIGHT_CTRL ; break;
    case VK_LMENU   : modifier = MODIFIER_LEFT_ALT   ; break;
    case VK_RMENU   : modifier = MODIFIER_RIGHT_ALT  ; break;

    case VK_CAPITAL : modifier = MODIFIER_CAPS       ; break;
    case VK_NUMLOCK : modifier = MODIFIER_NUM        ; break;
    case VK_INSERT  : modifier = MODIFIER_MODE       ; break;
  }

  return modifier;
}

uint32_t WinGuiEngine::winKeyToUnicode(UINT vKey, UINT scanCode)
{
  switch (vKey)
  {
    case VK_LEFT:
    case VK_RIGHT:
    case VK_UP:
    case VK_DOWN:
    case VK_TAB:
    case VK_ESCAPE:
    case VK_BACK:
    case VK_RETURN:
      return 0;
    default:
    {
      BYTE keystate[256];
      WCHAR unicode[2];

      GetKeyboardState(keystate);
      if (ToUnicode(vKey, scanCode, keystate, unicode, FOG_ARRAY_SIZE(unicode), 0) == 1 && unicode[0] > 31)
        return unicode[0];
      else
        return 0;
    }
  }
}

uint32_t WinGuiEngine::winKeyToFogKey(WPARAM vk, UINT scancode)
{
  uint32_t key;

  if ((vk == VK_RETURN) && (scancode & 0x100)) 
    key = KEY_KP_ENTER;
  else 
    key = _winKeymap.vk[vk];

  return key;
}

// ============================================================================
// [Fog::WinGuiEngine - Debug]
// ============================================================================

const char* WinGuiEngine::msgToStr(uint message)
{
  struct MsgTable
  {
    uint message;
    const char *str;
  };

  static const MsgTable msgTable[] = {
    {  WM_NULL, "WM_NULL"  },
    {  WM_CREATE, "WM_CREATE"  },
    {  WM_DESTROY, "WM_DESTROY"  },
    {  WM_MOVE, "WM_MOVE"  },
    {  WM_SIZE, "WM_SIZE"  },
    {  WM_ACTIVATE, "WM_ACTIVATE"  },
    {  WM_SETFOCUS, "WM_SETFOCUS"  },
    {  WM_KILLFOCUS, "WM_KILLFOCUS"  },
    {  WM_ENABLE, "WM_ENABLE"  },
    {  WM_SETREDRAW, "WM_SETREDRAW"  },
    {  WM_SETTEXT, "WM_SETTEXT"  },
    {  WM_GETTEXT, "WM_GETTEXT"  },
    {  WM_GETTEXTLENGTH, "WM_GETTEXTLENGTH"  },
    {  WM_PAINT, "WM_PAINT"  },
    {  WM_CLOSE, "WM_CLOSE"  },
    {  WM_QUERYENDSESSION, "WM_QUERYENDSESSION"  },
    {  WM_QUIT, "WM_QUIT"  },
    {  WM_QUERYOPEN, "WM_QUERYOPEN"  },
    {  WM_ERASEBKGND, "WM_ERASEBKGND"  },
    {  WM_SYSCOLORCHANGE, "WM_SYSCOLORCHANGE"  },
    {  WM_ENDSESSION, "WM_ENDSESSION"  },
    {  WM_SHOWWINDOW, "WM_SHOWWINDOW"  },
    {  WM_WININICHANGE, "WM_WININICHANGE"  },
    {  WM_DEVMODECHANGE, "WM_DEVMODECHANGE"  },
    {  WM_ACTIVATEAPP, "WM_ACTIVATEAPP"  },
    {  WM_FONTCHANGE, "WM_FONTCHANGE"  },
    {  WM_TIMECHANGE, "WM_TIMECHANGE"  },
    {  WM_CANCELMODE, "WM_CANCELMODE"  },
    {  WM_SETCURSOR, "WM_SETCURSOR"  },
    {  WM_MOUSEACTIVATE, "WM_MOUSEACTIVATE"  },
    {  WM_CHILDACTIVATE, "WM_CHILDACTIVATE"  },
    {  WM_QUEUESYNC, "WM_QUEUESYNC"  },
    {  WM_GETMINMAXINFO, "WM_GETMINMAXINFO"  },
    {  WM_PAINTICON, "WM_PAINTICON"  },
    {  WM_ICONERASEBKGND, "WM_ICONERASEBKGND"  },
    {  WM_NEXTDLGCTL, "WM_NEXTDLGCTL"  },
    {  WM_SPOOLERSTATUS, "WM_SPOOLERSTATUS"  },
    {  WM_DRAWITEM, "WM_DRAWITEM"  },
    {  WM_MEASUREITEM, "WM_MEASUREITEM"  },
    {  WM_DELETEITEM, "WM_DELETEITEM"  },
    {  WM_VKEYTOITEM, "WM_VKEYTOITEM"  },
    {  WM_CHARTOITEM, "WM_CHARTOITEM"  },
    {  WM_SETFONT, "WM_SETFONT"  },
    {  WM_GETFONT, "WM_GETFONT"  },
    {  WM_SETHOTKEY, "WM_SETHOTKEY"  },
    {  WM_GETHOTKEY, "WM_GETHOTKEY"  },
    {  WM_QUERYDRAGICON, "WM_QUERYDRAGICON"  },
    {  WM_COMPAREITEM, "WM_COMPAREITEM"  },
    {  WM_COMPACTING, "WM_COMPACTING"  },
    {  WM_WINDOWPOSCHANGING, "WM_WINDOWPOSCHANGING"  },
    {  WM_WINDOWPOSCHANGED, "WM_WINDOWPOSCHANGED"  },
    {  WM_POWER, "WM_POWER"  },
    {  WM_COPYDATA, "WM_COPYDATA"  },
    {  WM_CANCELJOURNAL, "WM_CANCELJOURNAL"  },
    {  WM_NCCREATE, "WM_NCCREATE"  },
    {  WM_NCDESTROY, "WM_NCDESTROY"  },
    {  WM_NCCALCSIZE, "WM_NCCALCSIZE"  },
    {  WM_NCHITTEST, "WM_NCHITTEST"  },
    {  WM_NCPAINT, "WM_NCPAINT"  },
#if defined(WM_SYNCPAINT)
    {  WM_SYNCPAINT, "WM_SYNCPAINT"  },
#endif
    {  WM_NCACTIVATE, "WM_NCACTIVATE"  },
    {  WM_GETDLGCODE, "WM_GETDLGCODE"  },
    {  WM_NCMOUSEMOVE, "WM_NCMOUSEMOVE"  },
    {  WM_NCLBUTTONDOWN, "WM_NCLBUTTONDOWN"  },
    {  WM_NCLBUTTONUP, "WM_NCLBUTTONUP"  },
    {  WM_NCLBUTTONDBLCLK, "WM_NCLBUTTONDBLCLK"  },
    {  WM_NCRBUTTONDOWN, "WM_NCRBUTTONDOWN"  },
    {  WM_NCRBUTTONUP, "WM_NCRBUTTONUP"  },
    {  WM_NCRBUTTONDBLCLK, "WM_NCRBUTTONDBLCLK"  },
    {  WM_NCMBUTTONDOWN, "WM_NCMBUTTONDOWN"  },
    {  WM_NCMBUTTONUP, "WM_NCMBUTTONUP"  },
    {  WM_NCMBUTTONDBLCLK, "WM_NCMBUTTONDBLCLK"  },
    {  WM_KEYFIRST, "WM_KEYFIRST"  },
    {  WM_KEYDOWN, "WM_KEYDOWN"  },
    {  WM_KEYUP, "WM_KEYUP"  },
    {  WM_CHAR, "WM_CHAR"  },
    {  WM_DEADCHAR, "WM_DEADCHAR"  },
    {  WM_SYSKEYDOWN, "WM_SYSKEYDOWN"  },
    {  WM_SYSKEYUP, "WM_SYSKEYUP"  },
    {  WM_SYSCHAR, "WM_SYSCHAR"  },
    {  WM_SYSDEADCHAR, "WM_SYSDEADCHAR"  },
    {  WM_KEYLAST, "WM_KEYLAST"  },
    {  WM_INITDIALOG, "WM_INITDIALOG"  },
    {  WM_COMMAND, "WM_COMMAND"  },
    {  WM_SYSCOMMAND, "WM_SYSCOMMAND"  },
    {  WM_TIMER, "WM_TIMER"  },
    {  WM_HSCROLL, "WM_HSCROLL"  },
    {  WM_VSCROLL, "WM_VSCROLL"  },
    {  WM_INITMENU, "WM_INITMENU"  },
    {  WM_INITMENUPOPUP, "WM_INITMENUPOPUP"  },
    {  WM_MENUSELECT, "WM_MENUSELECT"  },
    {  WM_MENUCHAR, "WM_MENUCHAR"  },
    {  WM_ENTERIDLE, "WM_ENTERIDLE"  },
    {  WM_CTLCOLORMSGBOX, "WM_CTLCOLORMSGBOX"  },
    {  WM_CTLCOLOREDIT, "WM_CTLCOLOREDIT"  },
    {  WM_CTLCOLORLISTBOX, "WM_CTLCOLORLISTBOX"  },
    {  WM_CTLCOLORBTN, "WM_CTLCOLORBTN"  },
    {  WM_CTLCOLORDLG, "WM_CTLCOLORDLG"  },
    {  WM_CTLCOLORSCROLLBAR, "WM_CTLCOLORSCROLLBAR"  },
    {  WM_CTLCOLORSTATIC, "WM_CTLCOLORSTATIC"  },
    {  WM_MOUSEFIRST, "WM_MOUSEFIRST"  },
    {  WM_MOUSEMOVE, "WM_MOUSEMOVE"  },
    {  WM_LBUTTONDOWN, "WM_LBUTTONDOWN"  },
    {  WM_LBUTTONUP, "WM_LBUTTONUP"  },
    {  WM_LBUTTONDBLCLK, "WM_LBUTTONDBLCLK"  },
    {  WM_RBUTTONDOWN, "WM_RBUTTONDOWN"  },
    {  WM_RBUTTONUP, "WM_RBUTTONUP"  },
    {  WM_RBUTTONDBLCLK, "WM_RBUTTONDBLCLK"  },
    {  WM_MBUTTONDOWN, "WM_MBUTTONDOWN"  },
    {  WM_MBUTTONUP, "WM_MBUTTONUP"  },
    {  WM_MBUTTONDBLCLK, "WM_MBUTTONDBLCLK"  },
    {  WM_MOUSELAST, "WM_MOUSELAST"  },
    {  WM_PARENTNOTIFY, "WM_PARENTNOTIFY"  },
    {  WM_ENTERMENULOOP, "WM_ENTERMENULOOP"  },
    {  WM_EXITMENULOOP, "WM_EXITMENULOOP"  },
    {  WM_MDICREATE, "WM_MDICREATE"  },
    {  WM_MDIDESTROY, "WM_MDIDESTROY"  },
    {  WM_MDIACTIVATE, "WM_MDIACTIVATE"  },
    {  WM_MDIRESTORE, "WM_MDIRESTORE"  },
    {  WM_MDINEXT, "WM_MDINEXT"  },
    {  WM_MDIMAXIMIZE, "WM_MDIMAXIMIZE"  },
    {  WM_MDITILE, "WM_MDITILE"  },
    {  WM_MDICASCADE, "WM_MDICASCADE"  },
    {  WM_MDIICONARRANGE, "WM_MDIICONARRANGE"  },
    {  WM_MDIGETACTIVE, "WM_MDIGETACTIVE"  },
    {  WM_MDISETMENU, "WM_MDISETMENU"  },
    {  WM_DROPFILES, "WM_DROPFILES"  },
    {  WM_MDIREFRESHMENU, "WM_MDIREFRESHMENU"  },
    {  WM_CUT, "WM_CUT"  },
    {  WM_COPY, "WM_COPY"  },
    {  WM_PASTE, "WM_PASTE"  },
    {  WM_CLEAR, "WM_CLEAR"  },
    {  WM_UNDO, "WM_UNDO"  },
    {  WM_RENDERFORMAT, "WM_RENDERFORMAT"  },
    {  WM_RENDERALLFORMATS, "WM_RENDERALLFORMATS"  },
    {  WM_DESTROYCLIPBOARD, "WM_DESTROYCLIPBOARD"  },
    {  WM_DRAWCLIPBOARD, "WM_DRAWCLIPBOARD"  },
    {  WM_PAINTCLIPBOARD, "WM_PAINTCLIPBOARD"  },
    {  WM_VSCROLLCLIPBOARD, "WM_VSCROLLCLIPBOARD"  },
    {  WM_SIZECLIPBOARD, "WM_SIZECLIPBOARD"  },
    {  WM_ASKCBFORMATNAME, "WM_ASKCBFORMATNAME"  },
    {  WM_CHANGECBCHAIN, "WM_CHANGECBCHAIN"  },
    {  WM_HSCROLLCLIPBOARD, "WM_HSCROLLCLIPBOARD"  },
    {  WM_QUERYNEWPALETTE, "WM_QUERYNEWPALETTE"  },
    {  WM_PALETTEISCHANGING, "WM_PALETTEISCHANGING"  },
    {  WM_PALETTECHANGED, "WM_PALETTECHANGED"  },
    {  WM_HOTKEY, "WM_HOTKEY"  },
    {  WM_PENWINFIRST, "WM_PENWINFIRST"  },
    {  WM_PENWINLAST, "WM_PENWINLAST"  }
#if defined(WM_SIZING)
    ,{  WM_SIZING, "WM_SIZING"  }
#endif
#if defined(WM_MOVING)
    ,{  WM_MOVING, "WM_MOVING"  }
#endif
#if defined(WM_ENTERSIZEMOVE)
    ,{  WM_ENTERSIZEMOVE, "WM_ENTERSIZEMOVE"  }
#endif
#if defined(WM_EXITSIZEMOVE)
    ,{  WM_EXITSIZEMOVE, "WM_EXITSIZEMOVE"  }
#endif
#if defined(WM_ASYNCIO)
    ,{  WM_ASYNCIO, "WM_ASYNCIO"  }
#endif
  };

  for (sysuint_t i = 0; i != FOG_ARRAY_SIZE(msgTable); i++)
  {
    if (msgTable[i].message == message) return msgTable[i].str;
  }

  return "Unknown";
}

// ============================================================================
// [Fog::WinUIWindow]
// ============================================================================

WinUIWindow::WinUIWindow(Widget* widget) : 
  BaseUIWindow(widget)
{
  _backingStore = new(std::nothrow) WinUIBackingStore();
}

WinUIWindow::~WinUIWindow()
{
  destroy();
  delete _backingStore;
}

err_t WinUIWindow::create(uint32_t flags)
{
  if (_handle) return ERR_GUI_WINDOW_ALREADY_EXISTS;

  // Create the Window (HWND is returned)
  DWORD dwStyle;
  DWORD dwStyleEx;
  int x;
  int y;
  WCHAR* wndClass;

  if (flags & CreatePopup)
  {
    dwStyle = WS_POPUP;
    dwStyleEx = WS_EX_TOOLWINDOW;
    x = -1;
    y = -1;
    wndClass = L"Fog_Popup";
  }
  else
  {
    dwStyle = WS_OVERLAPPEDWINDOW;
    dwStyleEx = WS_EX_CLIENTEDGE;
    x = CW_USEDEFAULT;
    y = CW_USEDEFAULT;
    wndClass = L"Fog_Window";
  }

  _handle = (void*)CreateWindowExW(
    dwStyleEx, wndClass, L"",
    dwStyle, x, y,
    1, 1,
    NULL, NULL, (HINSTANCE)GetModuleHandleW(NULL), NULL);

  if (_handle == NULL)
  {
    fog_stderr_msg("Fog::WinUIWindow", "create", "CreateWindowExW() failed.");
    goto fail;
  }

  // Create HWND <-> UIWindow* connection.
  GUI_ENGINE()->mapHandle(_handle, this);

  // Windows are enabled by default.
  _enabled = true;
  // Windows are not visible by default.
  _visible = false;
  // Need to blit window content.
  _needBlit = true;
  // Default focus is no focus, Windows will inform us if this gets changed.
  _hasFocus = false;

  // Get correct window and client rectangle.
  getWindowRect(&_windowRect, &_clientRect);

  return ERR_OK;

fail:
  return ERR_GUI_CANT_CREATE_UIWINDOW;
}

err_t WinUIWindow::destroy()
{
  if (!_handle) return ERR_RT_INVALID_HANDLE;

  // Destroy HWND <-> UIWindow* connection.
  GUI_ENGINE()->unmapHandle(_handle);

  // Destroy HWND.
  DestroyWindow((HWND)_handle);

  // Clear all variables.
  _handle = NULL;
  _hasFocus = false;
  _needBlit = false;

  return ERR_OK;
}

err_t WinUIWindow::enable()
{
  if (!_handle) return ERR_RT_INVALID_HANDLE;

  if (!_enabled) EnableWindow((HWND)_handle, TRUE);
  return ERR_OK;
}

err_t WinUIWindow::disable()
{
  if (!_handle) return ERR_RT_INVALID_HANDLE;

  if (_enabled) EnableWindow((HWND)_handle, FALSE);
  return ERR_OK;
}

err_t WinUIWindow::show()
{
  if (!_handle) return ERR_RT_INVALID_HANDLE;

  if (!_visible) ShowWindow((HWND)_handle, SW_SHOWNA);
  return ERR_OK;
}

err_t WinUIWindow::hide()
{
  if (!_handle) return ERR_RT_INVALID_HANDLE;

  if (_visible) ShowWindow((HWND)_handle, SW_HIDE);
  return ERR_OK;
}

err_t WinUIWindow::move(const Point& pt)
{
  if (!_handle) return ERR_RT_INVALID_HANDLE;

  if ((_windowRect.getX() != pt.getX()) | (_windowRect.getY1() != pt.getY()))
  {
    MoveWindow((HWND)_handle,
      pt.getX(),
      pt.getY(), 
      _windowRect.getWidth(),
      _windowRect.getHeight(),
      FALSE);
  }

  return ERR_OK;
}

err_t WinUIWindow::resize(const Size& size)
{
  if (!_handle) return ERR_RT_INVALID_HANDLE;
  if ((size.getWidth() <= 0) | (size.getHeight() <= 0)) return ERR_RT_INVALID_ARGUMENT;

  if (_windowRect.getSize() != size)
  {
    MoveWindow((HWND)_handle,
      _widget->getX(),
      _widget->getY(),
      size.getWidth(),
      size.getHeight(),
      TRUE);
  }

  return ERR_OK;
}

err_t WinUIWindow::reconfigure(const Rect& rect)
{
  if (!_handle) return ERR_RT_INVALID_HANDLE;
  if (!rect.isValid()) return ERR_RT_INVALID_ARGUMENT;

  if (_windowRect != rect)
  {
    MoveWindow((HWND)_handle, 
      rect.getX(),
      rect.getY(),
      rect.getWidth(),
      rect.getHeight(),
      TRUE);
  }

  return ERR_OK;
}

err_t WinUIWindow::takeFocus()
{
  if (!_handle) return ERR_RT_INVALID_HANDLE;

  SetFocus((HWND)_handle);
  return ERR_OK;
}

err_t WinUIWindow::setTitle(const String& title)
{
  if (!_handle) return ERR_RT_INVALID_HANDLE;

  SetWindowTextW((HWND)_handle, reinterpret_cast<const wchar_t*>(title.getData()));
  _title = title;

  return ERR_OK;
}

err_t WinUIWindow::getTitle(String& title)
{
  if (!_handle) return ERR_RT_INVALID_HANDLE;

  title = _title;
  return ERR_OK;
}

err_t WinUIWindow::setIcon(const Image& icon)
{
  if (!_handle) return ERR_RT_INVALID_HANDLE;

}

err_t WinUIWindow::getIcon(Image& icon)
{
  if (!_handle) return ERR_RT_INVALID_HANDLE;

}

err_t WinUIWindow::setSizeGranularity(const Point& pt)
{
  if (!_handle) return ERR_RT_INVALID_HANDLE;

  _sizeGranularity = pt;
  return ERR_OK;
}

err_t WinUIWindow::getSizeGranularity(Point& pt)
{
  if (!_handle) return ERR_RT_INVALID_HANDLE;

  pt = _sizeGranularity;
  return ERR_OK;
}

err_t WinUIWindow::worldToClient(Point* coords)
{
  if (!_handle) return ERR_RT_INVALID_HANDLE;

  return ScreenToClient((HWND)_handle, (POINT *)coords)
    ? (err_t)ERR_OK
    : (err_t)ERR_GUI_CANT_TRANSLETE_COORDINATES;
}

err_t WinUIWindow::clientToWorld(Point* coords)
{
  if (!_handle) return ERR_RT_INVALID_HANDLE;

  return ClientToScreen((HWND)_handle, (POINT *)coords) 
    ? (err_t)ERR_OK
    : (err_t)ERR_GUI_CANT_TRANSLETE_COORDINATES;
}

void WinUIWindow::onMousePress(uint32_t button, bool repeated)
{
  WinGuiEngine* uiSystem = GUI_ENGINE();
  if (uiSystem->_systemMouseStatus.uiWindow != this) return;

  if (uiSystem->_systemMouseStatus.buttons == 0 && !repeated)
  {
    SetCapture((HWND)_handle);
  }

  base::onMousePress(button, repeated);
}

void WinUIWindow::onMouseRelease(uint32_t button)
{
  WinGuiEngine* uiSystem = GUI_ENGINE();
  if (uiSystem->_systemMouseStatus.uiWindow != this) return;

  if ((uiSystem->_systemMouseStatus.buttons & ~button) == 0)
  {
    ReleaseCapture();
  }

  base::onMouseRelease(button);
}

LRESULT WinUIWindow::onWinMsg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  WinGuiEngine* uiSystem = GUI_ENGINE();

  switch (message)
  {
    case WM_SHOWWINDOW:
      onVisibility(wParam ? true : false);
      return 0;

    case WM_WINDOWPOSCHANGED:
    {
      Rect wr;
      Rect cr;
      getWindowRect(&wr, &cr);

      WINDOWPOS *pos = (WINDOWPOS*)lParam;
      wr.setX(pos->x);
      wr.setY(pos->y);

      onConfigure(wr, cr);
      return 0;
    }

    // case WM_GETMINMAXINFO:
    // {
    //   return 0;
    // }

    case WM_SIZE:
    {
      Rect wr;
      Rect cr;
      getWindowRect(&wr, &cr);

      onConfigure(wr, cr);
      return 0;
    }

    case WM_SETFOCUS:
      onFocus(true);
      return 0;

    case WM_KILLFOCUS:
      onFocus(false);
      return 0;

    case WM_MOUSEHOVER:
    case WM_MOUSEMOVE:
    {
      // Mouse tracking for WM_MOUSELEAVE message.
      TRACKMOUSEEVENT tme;
      tme.cbSize    = sizeof(TRACKMOUSEEVENT);
      tme.dwFlags   = TME_LEAVE;
      tme.hwndTrack = hwnd;
      TrackMouseEvent(&tme);

      int x = (int)(short)LOWORD(lParam);
      int y = (int)(short)HIWORD(lParam);

      // WM_MOUSEHOVER is not sent sometimes.
      if (uiSystem->_systemMouseStatus.uiWindow != this || 
         !uiSystem->_systemMouseStatus.hover)
      {
        onMouseHover(x, y);
      }
      else
      {
        onMouseMove(x, y);
      }

      return 0;
    }

    case WM_MOUSELEAVE:
    {
      if (uiSystem->_systemMouseStatus.uiWindow == this && 
         !uiSystem->_systemMouseStatus.buttons)
      {
        onMouseLeave(-1, -1);
      }
      return 0;
    }

    case WM_LBUTTONDOWN: onMousePress(BUTTON_LEFT  , false); return 0;
    case WM_RBUTTONDOWN: onMousePress(BUTTON_RIGHT , false); return 0;
    case WM_MBUTTONDOWN: onMousePress(BUTTON_MIDDLE, false); return 0;

    case WM_LBUTTONUP: onMouseRelease(BUTTON_LEFT  ); return 0;
    case WM_RBUTTONUP: onMouseRelease(BUTTON_RIGHT ); return 0;
    case WM_MBUTTONUP: onMouseRelease(BUTTON_MIDDLE); return 0;

    case WM_MOUSEWHEEL:
      // Typical for synaptics.
      if (uiSystem->_systemMouseStatus.uiWindow != this || 
         !uiSystem->_systemMouseStatus.hover)
      {
        POINT pt;
        DWORD cursorPos = GetMessagePos();
        pt.x = (int)(short)LOWORD(cursorPos);
        pt.y = (int)(short)HIWORD(cursorPos);
        ScreenToClient(hwnd, &pt);
        onMouseHover(pt.x, pt.y);
      }

      if ((int16_t)HIWORD(wParam) < 0)
      {
wheelDown:
        onMouseWheel(WHEEL_DOWN);
      }
      else
      {
wheelUp:
        onMouseWheel(WHEEL_UP);
      }
      return 0;

    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
    {
      uint32_t modifier = uiSystem->winKeyToModifier(&wParam, lParam);
      bool used = onKeyPress(
        uiSystem->winKeyToFogKey(wParam, HIWORD(lParam)),
        modifier,
        (uint32_t)wParam,
        Char(uiSystem->winKeyToUnicode(wParam, HIWORD(lParam)))
      );

      if (used) return 0;
      goto defWindowProc;
    }
    case WM_SYSKEYUP:
    case WM_KEYUP:
    {
      uint32_t modifier = uiSystem->winKeyToModifier(&wParam, lParam);
      bool used = onKeyRelease(
        uiSystem->winKeyToFogKey(wParam, HIWORD(lParam)),
        modifier,
        (uint32_t)wParam,
        Char(uiSystem->winKeyToUnicode(wParam, HIWORD(lParam)))
      );

      if (used) return 0;
      goto defWindowProc;
    }

    case WM_ERASEBKGND:
      return 1;

    case WM_PAINT:
    {
      RECT rect;
      PAINTSTRUCT ps;
      HDC hdc;

      GetClientRect(hwnd, &rect);
      hdc = BeginPaint(hwnd, &ps);

      if (!_isDirty)
      {
        BitBlt(
          hdc, 0, 0, rect.right-rect.left, rect.bottom-rect.top, 
          reinterpret_cast<WinUIBackingStore*>(_backingStore)->getHdc(), 0, 0, 
          SRCCOPY);
      }
      else
      {
        _needBlit = true;
      }

      EndPaint(hwnd, &ps);
      return 0;
    }

    case WM_CLOSE:
    {
      CloseEvent e;
      _widget->sendEvent(&e);
      return 0;
    }

    case WM_DESTROY:
      return 0;

    default:
      if (message == GUI_ENGINE()->uMSH_MOUSEWHEEL)
      {
        if ((int)wParam < 0) 
          goto wheelDown;
        else 
          goto wheelUp;
      }

defWindowProc:
      return DefWindowProc(hwnd, message, wParam, lParam);
  }
}

void WinUIWindow::getWindowRect(Rect* windowRect, Rect* clientRect)
{
  RECT wr;
  RECT cr;

  ::GetWindowRect((HWND)_handle, &wr);
  ::GetClientRect((HWND)_handle, &cr);

  windowRect->set(wr.left, wr.top, wr.right - wr.left, wr.bottom - wr.top);
  clientRect->set(cr.left, cr.top, cr.right - cr.left, cr.bottom - cr.top);
}

// ============================================================================
// [Fog::WinUIBackingStore]
// ============================================================================

WinUIBackingStore::WinUIBackingStore()
{
}

WinUIBackingStore::~WinUIBackingStore()
{
  destroy();
}

bool WinUIBackingStore::resize(int width, int height, bool cache)
{
  int targetWidth = width;
  int targetHeight = height;
  sysint_t targetSize;
  sysint_t targetStride;

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
      if (width <= _widthOrig && height <= _heightOrig)
      {
        // Cached, here can be debug counter to create
        // statistics about usability of that
        _width = width;
        _height = height;
        return true;
      }

      // Don't create smaller buffer that previous!
      targetWidth  = width;
      targetHeight = height;

      if (targetWidth  < _width)  targetWidth  = _width;
      if (targetHeight < _height) targetHeight = _height;

      targetWidth  = (targetWidth  + 127) & ~127;
      targetHeight = (targetHeight + 127) & ~127;
    }

    destroyImage = true;
    createImage = (targetWidth > 0 && targetHeight > 0);
  }

  // Destroy image buffer
  if (destroyImage)
  {
    switch (_type)
    {
      case TYPE_NONE:
        break;

      case TYPE_WIN_DIB:
        DeleteDC(_hdc);
        DeleteObject(_hBitmap);
        break;
    }
  }

  // Create image buffer.
  if (createImage)
  {
    int targetBPP = 32;

    // Define bitmap attributes.
    BITMAPINFO bmi;

    _hdc = CreateCompatibleDC(NULL);
    if (_hdc == NULL)
    {
      fog_stderr_msg("Fog::WinUIBackingStore", "resize", "CreateCompatibleDC failed, WinError: %u\n", GetLastError());
      goto fail;
    }

    Fog::Memory::zero(&bmi, sizeof(bmi));
    bmi.bmiHeader.biSize        = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth       = (int)targetWidth;
    bmi.bmiHeader.biHeight      = -((int)targetHeight);
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biBitCount    = targetBPP;
    bmi.bmiHeader.biCompression = BI_RGB;

    _pixelsPrimary = NULL;
    _hBitmap = CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, (void**)&_pixelsPrimary, NULL, 0);
    if (_hBitmap == NULL) 
    {
      fog_stderr_msg("Fog::WinUIBackingStore", "resize", "CreateDIBSection failed, request size %dx%d, WinError: %u\n", targetWidth, targetHeight, GetLastError());
      DeleteDC(_hdc);
      goto fail;
    }

    // Select bitmap to DC.
    SelectObject(_hdc, _hBitmap);

    // We need stride, windows should use 32 bit alignment, but be safe.
    DIBSECTION info;
    GetObjectW(_hBitmap, sizeof(DIBSECTION), &info);
    targetStride = info.dsBm.bmWidthBytes;
    targetSize = targetStride * targetHeight;
    _type = TYPE_WIN_DIB;

    if (_type != TYPE_NONE)
    {
      _createdTime = TimeTicks::now();
      _expireTime = _createdTime + TimeDelta::fromSeconds(15);

      _format = PIXEL_FORMAT_XRGB32;

      _stridePrimary = targetStride;
      _widthOrig = targetWidth;
      _heightOrig = targetHeight;

      // Extra buffer not needed.
      _pixelsSecondary = NULL;
      _strideSecondary = 0;

      _convertFunc = NULL;
      _convertDepth = 0;

      _pixels = _pixelsPrimary;
      _width = width;
      _height = height;
      _stride = _stridePrimary;
      return true;
    }
  }

fail:
  _clear();
  return false;
}

void WinUIBackingStore::destroy()
{
  resize(0, 0, false);
}

void WinUIBackingStore::updateRects(const Box* rects, sysuint_t count)
{
  // There is nothing to do (this is mainly for X11).
}

void WinUIBackingStore::blitRects(HDC target, const Box* rects, sysuint_t count)
{
  sysuint_t i;

  switch (getType())
  {
    case TYPE_NONE:
      break;

    case TYPE_WIN_DIB:
      for (i = 0; i != count; i++)
      {
        int x = rects[i].getX();
        int y = rects[i].getY();
        int w = rects[i].getWidth();
        int h = rects[i].getHeight();
        
        BitBlt(target, x, y, w, h, _hdc, x, y, SRCCOPY);
      }
      break;
  }
}

} // Fog namespace

#endif // FOG_OS_WINDOWS
