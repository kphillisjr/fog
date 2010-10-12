// [Fog-Core]
//
// [License]
// MIT, See COPYING file in package

// [Guard]
#ifndef _FOG_CORE_ASSERT_H
#define _FOG_CORE_ASSERT_H

// [Dependencies]
#include <Fog/Core/Build.h>

// ============================================================================
// [FOG_ASSERT]
// ============================================================================

//! @addtogroup Fog_Core_Debugging
//! @{

FOG_CAPI_EXTERN void FOG_NO_RETURN fog_assert_failure(const char* file, int line, const char* msg);

#if defined(FOG_DEBUG)

#define FOG_ASSERT(expression) \
  if (!(expression)) fog_assert_failure(__FILE__, __LINE__, #expression)

#define FOG_ASSERT_X(expression, msg) \
  if (!(expression)) fog_assert_failure(__FILE__, __LINE__, msg)

#define FOG_ASSERT_NOT_REACHED() \
  fog_assert_failure(__FILE__, __LINE__, NULL)

#else // !FOG_DEBUG

#define FOG_ASSERT(expression) FOG_NOP
#define FOG_ASSERT_X(expression, msg) FOG_NOP

#if defined(FOG_CC_MSVC)
#define FOG_ASSERT_NOT_REACHED() __assume(0)
#else
#define FOG_ASSERT_NOT_REACHED() FOG_NOP
#endif 

#endif // FOG_DEBUG

//! @}

// [Guard]
#endif // _FOG_CORE_ASSERT_H
