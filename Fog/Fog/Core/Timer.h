// [Fog/Core Library - C++ API]
//
// [Licence] 
// MIT, See COPYING file in package

// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// [Guard]
#ifndef _FOG_CORE_TIMER_H
#define _FOG_CORE_TIMER_H

// [Dependencies]
#include <Fog/Core/Object.h>
#include <Fog/Core/Task.h>
#include <Fog/Core/Time.h>

namespace Fog {

struct EventLoop;

// [Fog::Timer]

struct FOG_API Timer : public Object
{
  FOG_DECLARE_OBJECT(Timer, Object)

  Timer();
  virtual ~Timer();

  bool isRunning();

  bool start();
  void stop();

  FOG_INLINE TimeDelta interval() const
  { return _interval; }

  void setInterval(TimeDelta interval);

  virtual void onTimer(TimerEvent* e);

  fog_event_begin()
    fog_event(EvTimer, onTimer, TimerEvent, Override)
  fog_event_end()

protected:
  Task* _task;
  TimeDelta _interval;

private:
  friend struct TimerTask;
};

} // Fog namespace

#endif  // _FOG_TIMER_H
