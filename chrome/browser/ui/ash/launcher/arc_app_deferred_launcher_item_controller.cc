// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/launcher/arc_app_deferred_launcher_item_controller.h"

#include <memory>

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/app_list/arc/arc_app_list_prefs.h"
#include "chrome/browser/ui/ash/launcher/arc_app_deferred_launcher_controller.h"
#include "chrome/browser/ui/ash/launcher/arc_app_window_launcher_controller.h"
#include "chrome/browser/ui/ash/launcher/chrome_launcher_controller.h"

ArcAppDeferredLauncherItemController::ArcAppDeferredLauncherItemController(
    const std::string& arc_app_id,
    ChromeLauncherController* controller,
    int event_flags,
    const base::WeakPtr<ArcAppDeferredLauncherController>& host)
    : LauncherItemController(arc_app_id, std::string(), controller),
      event_flags_(event_flags),
      host_(host),
      start_time_(base::Time::Now()) {}

ArcAppDeferredLauncherItemController::~ArcAppDeferredLauncherItemController() {
  if (host_)
    host_->Remove(app_id());
}

base::TimeDelta ArcAppDeferredLauncherItemController::GetActiveTime() const {
  return base::Time::Now() - start_time_;
}

void ArcAppDeferredLauncherItemController::ItemSelected(
    std::unique_ptr<ui::Event> event,
    int64_t display_id,
    ash::ShelfLaunchSource source,
    const ItemSelectedCallback& callback) {
  callback.Run(ash::SHELF_ACTION_NONE, base::nullopt);
}

void ArcAppDeferredLauncherItemController::ExecuteCommand(uint32_t command_id,
                                                          int32_t event_flags) {
  // This delegate does not support showing an application menu.
  NOTIMPLEMENTED();
}

void ArcAppDeferredLauncherItemController::Close() {
  if (host_)
    host_->Close(app_id());
}
