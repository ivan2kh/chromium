// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_TEST_TEST_CONTENT_UTILITY_CLIENT_H_
#define EXTENSIONS_TEST_TEST_CONTENT_UTILITY_CLIENT_H_

#include "content/public/utility/content_utility_client.h"
#include "extensions/utility/utility_handler.h"

namespace extensions {

class TestContentUtilityClient : public content::ContentUtilityClient {
 public:
  TestContentUtilityClient();
  ~TestContentUtilityClient() override;

  // content::ContentUtilityClient:
  void UtilityThreadStarted() override;
  void ExposeInterfacesToBrowser(
      service_manager::InterfaceRegistry* registry) override;
  bool OnMessageReceived(const IPC::Message& message) override;

 private:
  UtilityHandler utility_handler_;
};

}  // namespace extensions

#endif  // EXTENSIONS_TEST_TEST_CONTENT_UTILITY_CLIENT_H_
