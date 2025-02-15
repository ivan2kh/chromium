// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/screen_pinning_controller.h"

#include <vector>

#include "ash/common/accelerators/accelerator_controller.h"
#include "ash/common/wm/window_state.h"
#include "ash/common/wm/wm_event.h"
#include "ash/common/wm_shell.h"
#include "ash/common/wm_window.h"
#include "ash/shell.h"
#include "ash/test/ash_test_base.h"
#include "ash/wm/window_util.h"
#include "base/stl_util.h"
#include "ui/aura/window.h"

namespace ash {
namespace {

int FindIndex(const std::vector<aura::Window*>& windows,
              const aura::Window* target) {
  auto iter = std::find(windows.begin(), windows.end(), target);
  return iter != windows.end() ? iter - windows.begin() : -1;
}

}  // namespace

using ScreenPinningControllerTest = test::AshTestBase;

TEST_F(ScreenPinningControllerTest, IsPinned) {
  aura::Window* w1 = CreateTestWindowInShellWithId(0);
  wm::ActivateWindow(w1);

  wm::PinWindow(w1, /* trusted */ false);
  EXPECT_TRUE(WmShell::Get()->IsPinned());
}

TEST_F(ScreenPinningControllerTest, OnlyOnePinnedWindow) {
  aura::Window* w1 = CreateTestWindowInShellWithId(0);
  aura::Window* w2 = CreateTestWindowInShellWithId(1);
  wm::ActivateWindow(w1);

  wm::PinWindow(w1, /* trusted */ false);
  EXPECT_TRUE(WmWindow::Get(w1)->GetWindowState()->IsPinned());
  EXPECT_FALSE(WmWindow::Get(w2)->GetWindowState()->IsPinned());

  // Prohibit to pin two (or more) windows.
  wm::PinWindow(w2, /* trusted */ false);
  EXPECT_TRUE(WmWindow::Get(w1)->GetWindowState()->IsPinned());
  EXPECT_FALSE(WmWindow::Get(w2)->GetWindowState()->IsPinned());
}

TEST_F(ScreenPinningControllerTest, FullscreenInPinnedMode) {
  aura::Window* w1 = CreateTestWindowInShellWithId(0);
  aura::Window* w2 = CreateTestWindowInShellWithId(1);
  wm::ActivateWindow(w1);

  wm::PinWindow(w1, /* trusted */ false);
  {
    // Window w1 should be in front of w2.
    std::vector<aura::Window*> siblings = w1->parent()->children();
    int index1 = FindIndex(siblings, w1);
    int index2 = FindIndex(siblings, w2);
    EXPECT_NE(-1, index1);
    EXPECT_NE(-1, index2);
    EXPECT_GT(index1, index2);
  }

  // Set w2 to fullscreen.
  {
    wm::ActivateWindow(w2);
    const wm::WMEvent event(wm::WM_EVENT_TOGGLE_FULLSCREEN);
    WmWindow::Get(w2)->GetWindowState()->OnWMEvent(&event);
  }
  {
    // Verify that w1 is still in front of w2.
    std::vector<aura::Window*> siblings = w1->parent()->children();
    int index1 = FindIndex(siblings, w1);
    int index2 = FindIndex(siblings, w2);
    EXPECT_NE(-1, index1);
    EXPECT_NE(-1, index2);
    EXPECT_GT(index1, index2);
  }

  // Unset w2's fullscreen.
  {
    wm::ActivateWindow(w2);
    const wm::WMEvent event(wm::WM_EVENT_TOGGLE_FULLSCREEN);
    WmWindow::Get(w2)->GetWindowState()->OnWMEvent(&event);
  }
  {
    // Verify that w1 is still in front of w2.
    std::vector<aura::Window*> siblings = w1->parent()->children();
    int index1 = FindIndex(siblings, w1);
    int index2 = FindIndex(siblings, w2);
    EXPECT_NE(-1, index1);
    EXPECT_NE(-1, index2);
    EXPECT_GT(index1, index2);
  }

  // Maximize w2.
  {
    wm::ActivateWindow(w2);
    const wm::WMEvent event(wm::WM_EVENT_TOGGLE_MAXIMIZE);
    WmWindow::Get(w2)->GetWindowState()->OnWMEvent(&event);
  }
  {
    // Verify that w1 is still in front of w2.
    std::vector<aura::Window*> siblings = w1->parent()->children();
    int index1 = FindIndex(siblings, w1);
    int index2 = FindIndex(siblings, w2);
    EXPECT_NE(-1, index1);
    EXPECT_NE(-1, index2);
    EXPECT_GT(index1, index2);
  }

  // Unset w2's maximize.
  {
    wm::ActivateWindow(w2);
    const wm::WMEvent event(wm::WM_EVENT_TOGGLE_MAXIMIZE);
    WmWindow::Get(w2)->GetWindowState()->OnWMEvent(&event);
  }
  {
    // Verify that w1 is still in front of w2.
    std::vector<aura::Window*> siblings = w1->parent()->children();
    int index1 = FindIndex(siblings, w1);
    int index2 = FindIndex(siblings, w2);
    EXPECT_NE(-1, index1);
    EXPECT_NE(-1, index2);
    EXPECT_GT(index1, index2);
  }

  // Restore w1.
  WmWindow::Get(w1)->GetWindowState()->Restore();

  // Now, fullscreen-ize w2 should put it in front of w1.
  {
    wm::ActivateWindow(w2);
    const wm::WMEvent event(wm::WM_EVENT_TOGGLE_FULLSCREEN);
    WmWindow::Get(w2)->GetWindowState()->OnWMEvent(&event);
  }
  {
    // Verify that w1 is still in front of w2.
    std::vector<aura::Window*> siblings = w1->parent()->children();
    int index1 = FindIndex(siblings, w1);
    int index2 = FindIndex(siblings, w2);
    EXPECT_NE(-1, index1);
    EXPECT_NE(-1, index2);
    EXPECT_GT(index2, index1);
  }
}

TEST_F(ScreenPinningControllerTest, TrustedPinnedWithAccelerator) {
  aura::Window* w1 = CreateTestWindowInShellWithId(0);
  wm::ActivateWindow(w1);

  wm::PinWindow(w1, /* trusted */ true);
  EXPECT_TRUE(WmShell::Get()->IsPinned());

  Shell::Get()->accelerator_controller()->PerformActionIfEnabled(UNPIN);
  // The UNPIN accelerator key is disabled for trusted pinned and the window
  // must be still pinned.
  EXPECT_TRUE(WmShell::Get()->IsPinned());
}

}  // namespace ash
