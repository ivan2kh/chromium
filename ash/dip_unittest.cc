// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>
#include <vector>

#include "ash/common/shelf/shelf_widget.h"
#include "ash/common/shelf/wm_shelf.h"
#include "ash/shell.h"
#include "ash/test/ash_test_base.h"
#include "ash/wm/window_properties.h"
#include "ash/wm/window_util.h"
#include "base/command_line.h"
#include "ui/aura/window.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/compositor/layer.h"
#include "ui/display/display.h"
#include "ui/display/manager/display_manager.h"
#include "ui/display/screen.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/widget/widget.h"
#include "ui/wm/core/shadow.h"
#include "ui/wm/core/shadow_controller.h"
#include "ui/wm/core/shadow_types.h"
#include "ui/wm/public/activation_client.h"

namespace ash {

using DIPTest = test::AshTestBase;

// Test if the WM sets correct work area under different density.
TEST_F(DIPTest, WorkArea) {
  UpdateDisplay("1000x900*1.0f");

  aura::Window* root = Shell::GetPrimaryRootWindow();
  const display::Display display =
      display::Screen::GetScreen()->GetDisplayNearestWindow(root);

  EXPECT_EQ("0,0 1000x900", display.bounds().ToString());
  gfx::Rect work_area = display.work_area();
  EXPECT_EQ(gfx::Rect(0, 0, 1000, 852).ToString(), work_area.ToString());
  EXPECT_EQ(gfx::Insets(0, 0, 48, 0).ToString(),
            display.bounds().InsetsFrom(work_area).ToString());

  UpdateDisplay("2000x1800*2.0f");
  display::Screen* screen = display::Screen::GetScreen();

  const display::Display display_2x = screen->GetDisplayNearestWindow(root);
  const display::ManagedDisplayInfo display_info_2x =
      Shell::GetInstance()->display_manager()->GetDisplayInfo(display_2x.id());

  // The |bounds_in_pixel()| should report bounds in pixel coordinate.
  EXPECT_EQ("1,1 2000x1800", display_info_2x.bounds_in_native().ToString());

  // Aura and views coordinates are in DIP, so they their bounds do not change.
  EXPECT_EQ("0,0 1000x900", display_2x.bounds().ToString());
  work_area = display_2x.work_area();
  EXPECT_EQ(gfx::Rect(0, 0, 1000, 852).ToString(), work_area.ToString());
  EXPECT_EQ(gfx::Insets(0, 0, 48, 0).ToString(),
            display_2x.bounds().InsetsFrom(work_area).ToString());

  // Sanity check if the workarea's inset hight is same as
  // the shelf's height.
  WmShelf* shelf = GetPrimaryShelf();
  EXPECT_EQ(display_2x.bounds().InsetsFrom(work_area).height(),
            shelf->shelf_widget()->GetNativeView()->layer()->bounds().height());
}

}  // namespace ash
