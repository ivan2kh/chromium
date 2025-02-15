// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CLEAN_CHROME_BROWSER_MODEL_BROWSER_WEB_STATE_LIST_DELEGATE_H_
#define IOS_CLEAN_CHROME_BROWSER_MODEL_BROWSER_WEB_STATE_LIST_DELEGATE_H_

#include "base/macros.h"
#import "ios/shared/chrome/browser/tabs/web_state_list_delegate.h"

class Browser;

// WebStateList delegate for the new architecture.
class BrowserWebStateListDelegate : public WebStateListDelegate {
 public:
  explicit BrowserWebStateListDelegate(Browser* browser);
  ~BrowserWebStateListDelegate() override;

  // WebStateListDelegate implementation.
  void WillAddWebState(web::WebState* web_state) override;

 private:
  Browser* browser_;

  DISALLOW_COPY_AND_ASSIGN(BrowserWebStateListDelegate);
};

#endif  // IOS_CLEAN_CHROME_BROWSER_MODEL_BROWSER_WEB_STATE_LIST_DELEGATE_H_
