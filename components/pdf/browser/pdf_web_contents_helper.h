// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PDF_BROWSER_PDF_WEB_CONTENTS_HELPER_H_
#define COMPONENTS_PDF_BROWSER_PDF_WEB_CONTENTS_HELPER_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "ipc/ipc_message.h"

namespace content {
class WebContents;
}

namespace pdf {

class PDFWebContentsHelperClient;

// Per-WebContents class to handle PDF messages.
class PDFWebContentsHelper
    : public content::WebContentsObserver,
      public content::WebContentsUserData<PDFWebContentsHelper> {
 public:
  static void CreateForWebContentsWithClient(
      content::WebContents* contents,
      std::unique_ptr<PDFWebContentsHelperClient> client);

 private:
  PDFWebContentsHelper(content::WebContents* web_contents,
                       std::unique_ptr<PDFWebContentsHelperClient> client);
  ~PDFWebContentsHelper() override;

  // content::WebContentsObserver overrides:
  bool OnMessageReceived(const IPC::Message& message,
                         content::RenderFrameHost* render_frame_host) override;

  // Message handlers.
  void OnHasUnsupportedFeature();
  void OnSaveURLAs(const GURL& url, const content::Referrer& referrer);
  void OnUpdateContentRestrictions(int content_restrictions);

  std::unique_ptr<PDFWebContentsHelperClient> client_;

  DISALLOW_COPY_AND_ASSIGN(PDFWebContentsHelper);
};

}  // namespace pdf

#endif  // COMPONENTS_PDF_BROWSER_PDF_WEB_CONTENTS_HELPER_H_
