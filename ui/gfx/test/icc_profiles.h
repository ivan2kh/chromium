// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/icc_profile.h"

namespace gfx {

ICCProfile ICCProfileForTestingAdobeRGB();
ICCProfile ICCProfileForTestingColorSpin();
ICCProfile ICCProfileForTestingGenericRGB();
ICCProfile ICCProfileForTestingSRGB();

// A profile that does not have an analytic transfer function.
ICCProfile ICCProfileForTestingNoAnalyticTrFn();

// A profile that is A2B only.
ICCProfile ICCProfileForTestingA2BOnly();

}  // namespace gfx
