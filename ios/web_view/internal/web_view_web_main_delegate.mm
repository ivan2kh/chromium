// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web_view/internal/web_view_web_main_delegate.h"

#import "base/mac/bundle_locations.h"
#include "base/memory/ptr_util.h"
#import "ios/web_view/internal/web_view_web_client.h"
#import "ios/web_view/public/cwv_delegate.h"
#import "ios/web_view/public/cwv_web_view.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace ios_web_view {

WebViewWebMainDelegate::WebViewWebMainDelegate(id<CWVDelegate> delegate)
    : delegate_(delegate) {}

WebViewWebMainDelegate::~WebViewWebMainDelegate() = default;

void WebViewWebMainDelegate::BasicStartupComplete() {
  base::mac::SetOverrideFrameworkBundle(
      [NSBundle bundleForClass:[CWVWebView class]]);
  web_client_ = base::MakeUnique<WebViewWebClient>(delegate_);
  web::SetWebClient(web_client_.get());
}

}  // namespace ios_web_view
