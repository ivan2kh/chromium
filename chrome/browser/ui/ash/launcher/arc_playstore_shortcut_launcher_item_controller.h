// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ASH_LAUNCHER_ARC_PLAYSTORE_SHORTCUT_LAUNCHER_ITEM_CONTROLLER_H_
#define CHROME_BROWSER_UI_ASH_LAUNCHER_ARC_PLAYSTORE_SHORTCUT_LAUNCHER_ITEM_CONTROLLER_H_

#include <memory>

#include "base/macros.h"
#include "chrome/browser/ui/ash/launcher/app_shortcut_launcher_item_controller.h"

class ArcAppLauncher;
class ChromeLauncherController;

class ArcPlaystoreShortcutLauncherItemController
    : public AppShortcutLauncherItemController {
 public:
  explicit ArcPlaystoreShortcutLauncherItemController(
      ChromeLauncherController* controller);
  ~ArcPlaystoreShortcutLauncherItemController() override;

  // LauncherItemController overrides:
  void ItemSelected(std::unique_ptr<ui::Event> event,
                    int64_t display_id,
                    ash::ShelfLaunchSource source,
                    const ItemSelectedCallback& callback) override;

 private:
  std::unique_ptr<ArcAppLauncher> playstore_launcher_;

  DISALLOW_COPY_AND_ASSIGN(ArcPlaystoreShortcutLauncherItemController);
};

#endif  // CHROME_BROWSER_UI_ASH_LAUNCHER_ARC_PLAYSTORE_SHORTCUT_LAUNCHER_ITEM_CONTROLLER_H_
