// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_COMMON_SYSTEM_IME_TRAY_IME_CHROMEOS_H_
#define ASH_COMMON_SYSTEM_IME_TRAY_IME_CHROMEOS_H_

#include <stddef.h>

#include "ash/common/system/accessibility_observer.h"
#include "ash/common/system/chromeos/ime_menu/ime_list_view.h"
#include "ash/common/system/chromeos/virtual_keyboard/virtual_keyboard_observer.h"
#include "ash/common/system/ime/ime_observer.h"
#include "ash/common/system/tray/ime_info.h"
#include "ash/common/system/tray/system_tray_item.h"
#include "base/macros.h"

namespace ash {

namespace tray {
class IMEDefaultView;
class IMEDetailedView;
}

class TrayItemView;

class ASH_EXPORT TrayIME : public SystemTrayItem,
                           public IMEObserver,
                           public AccessibilityObserver,
                           public VirtualKeyboardObserver {
 public:
  explicit TrayIME(SystemTray* system_tray);
  ~TrayIME() override;

  // Overridden from VirtualKeyboardObserver.
  void OnKeyboardSuppressionChanged(bool suppressed) override;

  // Overridden from AccessibilityObserver:
  void OnAccessibilityModeChanged(
      AccessibilityNotificationVisibility notify) override;

 private:
  friend class TrayIMETest;

  // Repopulates the DefaultView and DetailedView.
  void Update();
  // Updates the System Tray label.
  void UpdateTrayLabel(const IMEInfo& info, size_t count);
  // Returns whether the virtual keyboard toggle should be shown in the
  // detailed view.
  bool ShouldShowKeyboardToggle();
  // Returns the appropriate label for the detailed view.
  base::string16 GetDefaultViewLabel(bool show_ime_label);

  // Overridden from SystemTrayItem.
  views::View* CreateTrayView(LoginStatus status) override;
  views::View* CreateDefaultView(LoginStatus status) override;
  views::View* CreateDetailedView(LoginStatus status) override;
  void DestroyTrayView() override;
  void DestroyDefaultView() override;
  void DestroyDetailedView() override;

  // Overridden from IMEObserver.
  void OnIMERefresh() override;
  void OnIMEMenuActivationChanged(bool is_active) override;

  // Returns true input methods are managed by policy.
  bool IsIMEManaged();

  // Whether the default view should be shown.
  bool ShouldDefaultViewBeVisible();

  // Decides if a tray icon should be shown.
  bool ShouldShowImeTrayItem(size_t ime_count);
  // Mandates behavior for the single IME case for the detailed IME list
  // sub-view.
  ImeListView::SingleImeBehavior GetSingleImeBehavior();

  TrayItemView* tray_label_;
  tray::IMEDefaultView* default_;
  tray::IMEDetailedView* detailed_;
  // Whether the virtual keyboard is suppressed.
  bool keyboard_suppressed_;
  // Cached IME info.
  IMEInfoList ime_list_;
  IMEInfo current_ime_;
  IMEPropertyInfoList property_list_;
  // If non-empty, a controlled-setting icon should be displayed with a tooltip
  // text defined by this string.
  base::string16 ime_managed_message_;

  // Whether the IME label and tray items should be visible.
  bool is_visible_;

  DISALLOW_COPY_AND_ASSIGN(TrayIME);
};

}  // namespace ash

#endif  // ASH_COMMON_SYSTEM_IME_TRAY_IME_CHROMEOS_H_
