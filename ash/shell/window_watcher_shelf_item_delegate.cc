// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/shell/window_watcher_shelf_item_delegate.h"

#include "ash/shell/window_watcher.h"
#include "ash/wm/window_util.h"
#include "ui/aura/window.h"

namespace ash {
namespace shell {

WindowWatcherShelfItemDelegate::WindowWatcherShelfItemDelegate(
    ShelfID id,
    WindowWatcher* watcher)
    : id_(id), watcher_(watcher) {
  DCHECK_NE(id_, kInvalidShelfID);
  DCHECK(watcher_);
}

WindowWatcherShelfItemDelegate::~WindowWatcherShelfItemDelegate() {}

void WindowWatcherShelfItemDelegate::ItemSelected(
    std::unique_ptr<ui::Event> event,
    int64_t display_id,
    ShelfLaunchSource source,
    const ItemSelectedCallback& callback) {
  aura::Window* window = watcher_->GetWindowByID(id_);
  if (window->type() == ui::wm::WINDOW_TYPE_PANEL)
    wm::MoveWindowToDisplay(window, display_id);
  window->Show();
  wm::ActivateWindow(window);
  callback.Run(SHELF_ACTION_WINDOW_ACTIVATED, base::nullopt);
}

void WindowWatcherShelfItemDelegate::ExecuteCommand(uint32_t command_id,
                                                    int32_t event_flags) {
  // This delegate does not support showing an application menu.
  NOTIMPLEMENTED();
}

void WindowWatcherShelfItemDelegate::Close() {}

}  // namespace shell
}  // namespace ash
