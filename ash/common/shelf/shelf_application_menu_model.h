// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_COMMON_SHELF_SHELF_APPLICATION_MENU_MODEL_H_
#define ASH_COMMON_SHELF_SHELF_APPLICATION_MENU_MODEL_H_

#include <memory>
#include <vector>

#include "ash/ash_export.h"
#include "ash/public/interfaces/shelf.mojom.h"
#include "base/macros.h"
#include "ui/base/models/simple_menu_model.h"

namespace ash {

class ShelfApplicationMenuModelTestAPI;

// A menu model listing open applications associated with a shelf item. Layout:
// +---------------------------+
// |                           |
// |         Shelf Item Title  |
// |                           |
// |  [Icon] Menu Item Label   |
// |  [Icon] Menu Item Label   |
// |                           |
// +---------------------------+
class ASH_EXPORT ShelfApplicationMenuModel
    : public ui::SimpleMenuModel,
      public ui::SimpleMenuModel::Delegate {
 public:
  // Makes a menu with a |title|, separators, and |items| for |delegate|.
  // |delegate| may be null in unit tests that do not execute commands.
  ShelfApplicationMenuModel(const base::string16& title,
                            std::vector<mojom::MenuItemPtr> items,
                            mojom::ShelfItemDelegate* delegate);
  ~ShelfApplicationMenuModel() override;

  // ui::SimpleMenuModel::Delegate:
  bool IsCommandIdChecked(int command_id) const override;
  bool IsCommandIdEnabled(int command_id) const override;
  void ExecuteCommand(int command_id, int event_flags) override;

 private:
  friend class ShelfApplicationMenuModelTestAPI;

  // Records UMA metrics when a menu item is selected.
  void RecordMenuItemSelectedMetrics(int command_id,
                                     int num_menu_items_enabled);

  // The list of menu items as returned from the shelf item's controller.
  std::vector<mojom::MenuItemPtr> items_;

  // The shelf item delegate that created the menu and executes its commands.
  mojom::ShelfItemDelegate* delegate_;

  DISALLOW_COPY_AND_ASSIGN(ShelfApplicationMenuModel);
};

}  // namespace ash

#endif  // ASH_COMMON_SHELF_SHELF_APPLICATION_MENU_MODEL_H_
