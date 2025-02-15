// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CSSContentDistributionValue_h
#define CSSContentDistributionValue_h

#include "core/css/CSSIdentifierValue.h"
#include "core/css/CSSValue.h"
#include "core/css/CSSValuePair.h"
#include "wtf/RefPtr.h"

namespace blink {

class CSSContentDistributionValue : public CSSValue {
 public:
  static CSSContentDistributionValue* create(CSSValueID distribution,
                                             CSSValueID position,
                                             CSSValueID overflow) {
    return new CSSContentDistributionValue(distribution, position, overflow);
  }
  ~CSSContentDistributionValue();

  // TODO(sashab): Make these return CSSValueIDs instead of CSSValues.
  CSSIdentifierValue* distribution() const {
    return CSSIdentifierValue::create(m_distribution);
  }

  CSSIdentifierValue* position() const {
    return CSSIdentifierValue::create(m_position);
  }

  CSSIdentifierValue* overflow() const {
    return CSSIdentifierValue::create(m_overflow);
  }

  String customCSSText() const;

  bool equals(const CSSContentDistributionValue&) const;

  DEFINE_INLINE_TRACE_AFTER_DISPATCH() {
    CSSValue::traceAfterDispatch(visitor);
  }

 private:
  CSSContentDistributionValue(CSSValueID distribution,
                              CSSValueID position,
                              CSSValueID overflow);

  CSSValueID m_distribution;
  CSSValueID m_position;
  CSSValueID m_overflow;
};

DEFINE_CSS_VALUE_TYPE_CASTS(CSSContentDistributionValue,
                            isContentDistributionValue());

}  // namespace blink

#endif  // CSSContentDistributionValue_h
