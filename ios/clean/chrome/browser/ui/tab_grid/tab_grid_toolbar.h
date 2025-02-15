// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CLEAN_CHROME_BROWSER_UI_TAB_GRID_TAB_GRID_TOOLBAR_H_
#define IOS_CLEAN_CHROME_BROWSER_UI_TAB_GRID_TAB_GRID_TOOLBAR_H_

#import <UIKit/UIKit.h>

// The toolbar in the tab grid, which has a done button, incognito button, and
// an overflow menu button. The toolbar has an intrinsic height, and its buttons
// are anchored to the bottom of the toolbar, should the parent set a larger
// frame on the toolbar. The toolbar has a dark blur visual effect as the
// background.
@interface TabGridToolbar : UIView
@end

#endif  // IOS_CLEAN_CHROME_BROWSER_UI_TAB_GRID_TAB_GRID_TOOLBAR_H_
