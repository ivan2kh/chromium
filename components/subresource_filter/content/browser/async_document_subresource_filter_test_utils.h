// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SUBRESOURCE_FILTER_CONTENT_BROWSER_ASYNC_DOCUMENT_SUBRESOURCE_FILTER_TEST_UTILS_H_
#define COMPONENTS_SUBRESOURCE_FILTER_CONTENT_BROWSER_ASYNC_DOCUMENT_SUBRESOURCE_FILTER_TEST_UTILS_H_

#include "base/bind.h"
#include "base/callback_forward.h"
#include "base/macros.h"
#include "components/subresource_filter/core/common/activation_state.h"

namespace subresource_filter {
namespace testing {

// This test class is intended to be used in conjunction with an
// AsyncDocumentSubresourceFilter, and can be used to expect a certain
// activation result occured.
class TestActivationStateCallbackReceiver {
 public:
  TestActivationStateCallbackReceiver() = default;

  base::Callback<void(ActivationState)> GetCallback();
  void ExpectReceivedOnce(const ActivationState& expected_state) const;

 private:
  void Callback(ActivationState activation_state);

  ActivationState last_activation_state_;
  int callback_count_ = 0;

  DISALLOW_COPY_AND_ASSIGN(TestActivationStateCallbackReceiver);
};

}  // namespace testing
}  // namespace subresource_filter

#endif  // COMPONENTS_SUBRESOURCE_FILTER_CONTENT_BROWSER_ASYNC_DOCUMENT_SUBRESOURCE_FILTER_TEST_UTILS_H_
