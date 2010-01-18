// [Fog/Core Library - Public API]
//
// [Licence]
// MIT, See COPYING file in package

// [Precompiled Headers]
#if defined(FOG_PRECOMP)
#include FOG_PRECOMP
#endif // FOG_PRECOMP

// [Dependencies]
#include <Fog/Core/Atomic.h>
#include <Fog/Core/Lazy.h>
#include <Fog/Core/Thread.h>

namespace Fog {

// ============================================================================
// [Fog::Lazy_Abstract]
// ============================================================================

Lazy_Abstract::Lazy_Abstract() : _ptr(NULL)
{
}

Lazy_Abstract::Lazy_Abstract(_DONT_INITIALIZE x)
{
  FOG_UNUSED(x);
}

Lazy_Abstract::~Lazy_Abstract()
{
  // Pointer should be NULL here
  FOG_ASSERT(_ptr == NULL);
}

void* Lazy_Abstract::_get()
{
  void* v = AtomicOperation<void*>::get(&_ptr);

  // Already created, just return it
  if (v != NULL && v != (void*)STATE_CREATING_NOW) return v;

  // Create instance
  if (AtomicOperation<void*>::cmpXchg(&_ptr, (void*)NULL, (void*)STATE_CREATING_NOW))
  {
    v = _create();
    AtomicOperation<void*>::set(&_ptr, v);
    return v;
  }

  // Race.
  // This is very rare situation, but it can happen!
  while ((v = AtomicOperation<void*>::get(&_ptr)) == (void*)STATE_CREATING_NOW)
  {
    Thread::_yield();
  }

  return v;
}

} // Fog namespace
