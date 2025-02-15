// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_SHARED_CHROME_BROWSER_TABS_WEB_STATE_LIST_DELEGATE_H_
#define IOS_SHARED_CHROME_BROWSER_TABS_WEB_STATE_LIST_DELEGATE_H_

#include "base/macros.h"

namespace web {
class WebState;
}

// A delegate interface that the WebStateList uses to perform work that it
// cannot do itself such as attaching tab helpers, ...
class WebStateListDelegate {
 public:
  WebStateListDelegate() = default;
  virtual ~WebStateListDelegate() = default;

  // Notifies the delegate that the specified WebState will be added to the
  // WebStateList (via insertion/appending/replacing existing) and allows it
  // to do any preparation that it deems necessary.
  virtual void WillAddWebState(web::WebState* web_state) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(WebStateListDelegate);
};

#endif  // IOS_SHARED_CHROME_BROWSER_TABS_WEB_STATE_LIST_DELEGATE_H_
