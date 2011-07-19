// [Fog-Core]
//
// [License]
// MIT, See COPYING file in package

// [Precompiled Headers]
#if defined(FOG_PRECOMP)
#include FOG_PRECOMP
#endif // FOG_PRECOMP

// [Dependencies]
#include <Fog/Core/Global/Error.h>

// [Dependencies - Windows]
#if defined(FOG_OS_WINDOWS)
# include <windows.h>
#endif // FOG_OS_WINDOWS

namespace Fog {

// ============================================================================
// [Fog::Error - Error Translation]
// ============================================================================

String Error::getMessage(err_t code)
{
  String message;

  // TODO:

  return message;
}

err_t Error::registerErrorRange(err_t start, err_t end, TranslatorFn fn)
{
  // TODO:
  return ERR_RT_NOT_IMPLEMENTED;
}

err_t Error::unregisterErrorRange(err_t start, err_t end)
{
  // TODO:
  return ERR_RT_NOT_IMPLEMENTED;
}

// ============================================================================
// [Init / Fini]
// ============================================================================

FOG_NO_EXPORT void Error_init(void)
{
}

} // Fog namespace
