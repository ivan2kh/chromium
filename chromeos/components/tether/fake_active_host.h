// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_COMPONENTS_TETHER_FAKE_ACTIVE_HOST_H_
#define CHROMEOS_COMPONENTS_TETHER_FAKE_ACTIVE_HOST_H_

#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "chromeos/components/tether/active_host.h"

namespace chromeos {

namespace tether {

// Test double for ActiveHost.
class FakeActiveHost : public ActiveHost {
 public:
  FakeActiveHost();
  ~FakeActiveHost() override;

  // ActiveHost:
  void SetActiveHostDisconnected() override;
  void SetActiveHostConnecting(
      const std::string& active_host_device_id) override;
  void SetActiveHostConnected(const std::string& active_host_device_id,
                              const std::string& wifi_network_id) override;
  void GetActiveHost(
      const ActiveHost::ActiveHostCallback& active_host_callback) override;
  ActiveHostStatus GetActiveHostStatus() const override;
  std::string GetActiveHostDeviceId() const override;
  std::string GetWifiNetworkId() const override;

 private:
  void SetActiveHost(ActiveHostStatus active_host_status,
                     const std::string& active_host_device_id,
                     const std::string& wifi_network_id);

  ActiveHost::ActiveHostStatus active_host_status_;
  std::string active_host_device_id_;
  std::string wifi_network_id_;

  DISALLOW_COPY_AND_ASSIGN(FakeActiveHost);
};

}  // namespace tether

}  // namespace chromeos

#endif  // CHROMEOS_COMPONENTS_TETHER_FAKE_ACTIVE_HOST_H_
