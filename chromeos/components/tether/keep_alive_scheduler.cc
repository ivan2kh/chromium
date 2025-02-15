// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/tether/keep_alive_scheduler.h"

#include "base/bind.h"

namespace chromeos {

namespace tether {

// static
const uint32_t KeepAliveScheduler::kKeepAliveIntervalMinutes = 4;

KeepAliveScheduler::KeepAliveScheduler(ActiveHost* active_host,
                                       BleConnectionManager* connection_manager)
    : KeepAliveScheduler(active_host,
                         connection_manager,
                         base::MakeUnique<base::RepeatingTimer>()) {}

KeepAliveScheduler::KeepAliveScheduler(ActiveHost* active_host,
                                       BleConnectionManager* connection_manager,
                                       std::unique_ptr<base::Timer> timer)
    : active_host_(active_host),
      connection_manager_(connection_manager),
      timer_(std::move(timer)),
      weak_ptr_factory_(this) {
  active_host_->AddObserver(this);
}

KeepAliveScheduler::~KeepAliveScheduler() {
  active_host_->RemoveObserver(this);
}

void KeepAliveScheduler::OnActiveHostChanged(
    ActiveHost::ActiveHostStatus active_host_status,
    std::unique_ptr<cryptauth::RemoteDevice> active_host_device,
    const std::string& wifi_network_id) {
  if (active_host_status == ActiveHost::ActiveHostStatus::DISCONNECTED) {
    DCHECK(!active_host_device);
    DCHECK(wifi_network_id.empty());

    keep_alive_operation_.reset();
    active_host_device_.reset();
    timer_->Stop();
    return;
  }

  if (active_host_status == ActiveHost::ActiveHostStatus::CONNECTED) {
    DCHECK(active_host_device);
    active_host_device_ = std::move(active_host_device);
    timer_->Start(FROM_HERE,
                  base::TimeDelta::FromMinutes(kKeepAliveIntervalMinutes),
                  base::Bind(&KeepAliveScheduler::SendKeepAliveTickle,
                             weak_ptr_factory_.GetWeakPtr()));
    SendKeepAliveTickle();
  }
}

void KeepAliveScheduler::OnOperationFinished() {
  keep_alive_operation_->RemoveObserver(this);
  keep_alive_operation_.reset();
}

void KeepAliveScheduler::SendKeepAliveTickle() {
  DCHECK(active_host_device_);

  keep_alive_operation_ = KeepAliveOperation::Factory::NewInstance(
      *active_host_device_, connection_manager_);
  keep_alive_operation_->AddObserver(this);
  keep_alive_operation_->Initialize();
}

}  // namespace tether

}  // namespace chromeos
