// [Fog/Cpu Library - C++ API]
//
// [Licence]
// MIT, See COPYING file in package

// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// [Guard]
#ifndef _FOG_CPU_INTRIN_3DNOW_H
#define _FOG_CPU_INTRIN_3DNOW_H

// [Dependencies]
#include <Fog/Build/Build.h>
#include <Fog/Cpu/Intrin_MMX.h>
#include <mm3dnow.h>

//! @addtogroup Fog_Core
//! @{

namespace Fog {

union FOG_ALIGN(8) mm3dnow_t
{
  __m64     m64;
  int64_t   sq[1];
  uint64_t  uq[1];
  int32_t   sd[2];
  uint32_t  ud[2];
  int16_t   sw[4];
  uint16_t  uw[4];
  int8_t    sb[8];
  uint8_t   ub[8];
  float     f[2];
};

}

//! @}

// [Guard]
#endif // _FOG_CPU_INTRIN_3DNOW_H
