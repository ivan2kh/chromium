// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/properties/CSSPropertyAPIFontFamily.h"

#include "core/css/CSSValueList.h"
#include "core/css/properties/CSSPropertyFontUtils.h"

class CSSParserContext;

namespace blink {

const CSSValue* CSSPropertyAPIFontFamily::parseSingleValue(
    CSSParserTokenRange& range,
    const CSSParserContext*) {
  return CSSPropertyFontUtils::consumeFontFamily(range);
}

}  // namespace blink
