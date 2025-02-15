// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CSSPropertyEquality_h
#define CSSPropertyEquality_h

#include "core/CSSPropertyNames.h"
#include "wtf/Allocator.h"
#include "wtf/text/AtomicString.h"

namespace blink {

class ComputedStyle;

class CSSPropertyEquality {
  STATIC_ONLY(CSSPropertyEquality);

 public:
  static bool propertiesEqual(CSSPropertyID,
                              const ComputedStyle&,
                              const ComputedStyle&);

  static bool registeredCustomPropertiesEqual(
      const WTF::AtomicString& propertyName,
      const ComputedStyle&,
      const ComputedStyle&);
};

}  // namespace blink

#endif  // CSSPropertyEquality_h
