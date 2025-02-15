// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/common/shelf/shelf_controller.h"

#include "ash/common/shelf/app_list_shelf_item_delegate.h"
#include "ash/common/shelf/wm_shelf.h"
#include "ash/common/wm_shell.h"
#include "ash/common/wm_window.h"
#include "ash/public/interfaces/shelf.mojom.h"
#include "ash/root_window_controller.h"
#include "ash/shell.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/base/models/simple_menu_model.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/resources/grit/ui_resources.h"

namespace ash {

namespace {

// Returns an icon image from an SkBitmap, or the default shelf icon image if
// the bitmap is empty. Assumes the bitmap is a 1x icon.
// TODO(jamescook): Support other scale factors.
gfx::ImageSkia GetShelfIconFromBitmap(const SkBitmap& bitmap) {
  gfx::ImageSkia icon_image;
  if (!bitmap.isNull()) {
    icon_image = gfx::ImageSkia::CreateFrom1xBitmap(bitmap);
  } else {
    // Use default icon.
    ResourceBundle& rb = ResourceBundle::GetSharedInstance();
    icon_image = *rb.GetImageSkiaNamed(IDR_DEFAULT_FAVICON);
  }
  return icon_image;
}

// Returns the WmShelf instance for the display with the given |display_id|.
WmShelf* GetShelfForDisplay(int64_t display_id) {
  // The controller may be null for invalid ids or for displays being removed.
  RootWindowController* root_window_controller =
      Shell::GetRootWindowControllerWithDisplayId(display_id);
  return root_window_controller ? root_window_controller->GetShelf() : nullptr;
}

}  // namespace

ShelfController::ShelfController() {
  // Create the app list item in the shelf.
  AppListShelfItemDelegate::CreateAppListItemAndDelegate(&model_);
}

ShelfController::~ShelfController() {}

void ShelfController::BindRequest(mojom::ShelfControllerRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void ShelfController::NotifyShelfCreated(WmShelf* shelf) {
  // Notify observers, Chrome will set alignment and auto-hide from prefs.
  int64_t display_id = shelf->GetWindow()->GetDisplayNearestWindow().id();
  observers_.ForAllPtrs([display_id](mojom::ShelfObserver* observer) {
    observer->OnShelfCreated(display_id);
  });
}

void ShelfController::NotifyShelfAlignmentChanged(WmShelf* shelf) {
  ShelfAlignment alignment = shelf->alignment();
  int64_t display_id = shelf->GetWindow()->GetDisplayNearestWindow().id();
  observers_.ForAllPtrs(
      [alignment, display_id](mojom::ShelfObserver* observer) {
        observer->OnAlignmentChanged(alignment, display_id);
      });
}

void ShelfController::NotifyShelfAutoHideBehaviorChanged(WmShelf* shelf) {
  ShelfAutoHideBehavior behavior = shelf->auto_hide_behavior();
  int64_t display_id = shelf->GetWindow()->GetDisplayNearestWindow().id();
  observers_.ForAllPtrs([behavior, display_id](mojom::ShelfObserver* observer) {
    observer->OnAutoHideBehaviorChanged(behavior, display_id);
  });
}

void ShelfController::AddObserver(
    mojom::ShelfObserverAssociatedPtrInfo observer) {
  mojom::ShelfObserverAssociatedPtr observer_ptr;
  observer_ptr.Bind(std::move(observer));
  observers_.AddPtr(std::move(observer_ptr));
}

void ShelfController::SetAlignment(ShelfAlignment alignment,
                                   int64_t display_id) {
  WmShelf* shelf = GetShelfForDisplay(display_id);
  // TODO(jamescook): The initialization check should not be necessary, but
  // otherwise this wrongly tries to set the alignment on a secondary display
  // during login before the ShelfLockingManager and ShelfView are created.
  if (shelf && shelf->IsShelfInitialized())
    shelf->SetAlignment(alignment);
}

void ShelfController::SetAutoHideBehavior(ShelfAutoHideBehavior auto_hide,
                                          int64_t display_id) {
  WmShelf* shelf = GetShelfForDisplay(display_id);
  // TODO(jamescook): The initialization check should not be necessary, but
  // otherwise this wrongly tries to set auto-hide state on a secondary display
  // during login before the ShelfView is created.
  if (shelf && shelf->IsShelfInitialized())
    shelf->SetAutoHideBehavior(auto_hide);
}

void ShelfController::PinItem(
    const ShelfItem& item,
    mojom::ShelfItemDelegateAssociatedPtrInfo delegate) {
  NOTIMPLEMENTED();
}

void ShelfController::UnpinItem(const std::string& app_id) {
  NOTIMPLEMENTED();
}

void ShelfController::SetItemImage(const std::string& app_id,
                                   const SkBitmap& image) {
  if (!app_id_to_shelf_id_.count(app_id))
    return;
  ShelfID shelf_id = app_id_to_shelf_id_[app_id];
  int index = model_.ItemIndexByID(shelf_id);
  DCHECK_GE(index, 0);
  ShelfItem item = *model_.ItemByID(shelf_id);
  item.image = GetShelfIconFromBitmap(image);
  model_.Set(index, item);
}

}  // namespace ash
