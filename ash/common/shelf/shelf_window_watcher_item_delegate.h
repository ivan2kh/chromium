// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_COMMON_SHELF_SHELF_WINDOW_WATCHER_ITEM_DELEGATE_H_
#define ASH_COMMON_SHELF_SHELF_WINDOW_WATCHER_ITEM_DELEGATE_H_

#include "ash/public/interfaces/shelf.mojom.h"
#include "base/macros.h"

namespace ash {

class WmWindow;

// ShelfItemDelegate for the items created by ShelfWindowWatcher, for example:
// The Chrome OS settings window, task manager window, and panel windows.
class ShelfWindowWatcherItemDelegate : public mojom::ShelfItemDelegate {
 public:
  ShelfWindowWatcherItemDelegate(ShelfID id, WmWindow* window);
  ~ShelfWindowWatcherItemDelegate() override;

 private:
  // mojom::ShelfItemDelegate overrides:
  void ItemSelected(std::unique_ptr<ui::Event> event,
                    int64_t display_id,
                    ShelfLaunchSource source,
                    const ItemSelectedCallback& callback) override;
  void ExecuteCommand(uint32_t command_id, int32_t event_flags) override;
  void Close() override;

  ShelfID id_;
  // The window associated with this item. Not owned.
  WmWindow* window_;

  DISALLOW_COPY_AND_ASSIGN(ShelfWindowWatcherItemDelegate);
};

}  // namespace ash

#endif  // ASH_COMMON_SHELF_SHELF_WINDOW_WATCHER_ITEM_DELEGATE_H_
