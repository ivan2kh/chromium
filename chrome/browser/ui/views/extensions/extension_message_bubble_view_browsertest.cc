// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/auto_reset.h"
#include "base/macros.h"
#include "chrome/browser/ui/extensions/extension_message_bubble_browsertest.h"
#include "chrome/browser/ui/test/test_browser_dialog.h"
#include "chrome/browser/ui/views/toolbar/toolbar_actions_bar_bubble_views.h"
#include "ui/base/ui_base_switches.h"
#include "ui/views/bubble/bubble_dialog_delegate.h"
#include "ui/views/controls/link.h"
#include "ui/views/controls/link_listener.h"
#include "ui/views/window/dialog_client_view.h"

namespace {

// Checks that the |bubble| is showing. Uses |reference_bounds| to ensure it is
// in approximately the correct position.
void CheckBubbleAgainstReferenceBounds(views::BubbleDialogDelegateView* bubble,
                                       const gfx::Rect& reference_bounds) {
  ASSERT_TRUE(bubble);

  // Do a rough check that the bubble is in the right place.
  gfx::Rect bubble_bounds = bubble->GetWidget()->GetWindowBoundsInScreen();
  // It should be below the reference view, but not too far below.
  EXPECT_GE(bubble_bounds.y(), reference_bounds.y());
  // The arrow should be poking into the anchor.
  const int kShadowWidth = 1;
  EXPECT_LE(bubble_bounds.y(), reference_bounds.bottom() + kShadowWidth);
  // The bubble should intersect the reference view somewhere along the x-axis.
  EXPECT_FALSE(bubble_bounds.x() > reference_bounds.right());
  EXPECT_FALSE(reference_bounds.x() > bubble_bounds.right());

  // And, of course, the bubble should be visible...
  EXPECT_TRUE(bubble->visible());
  // ... as should its Widget.
  EXPECT_TRUE(bubble->GetWidget()->IsVisible());
}

}  // namespace

class ExtensionMessageBubbleViewBrowserTest
    : public SupportsTestDialog<ExtensionMessageBubbleBrowserTest> {
 protected:
  ExtensionMessageBubbleViewBrowserTest() {}
  ~ExtensionMessageBubbleViewBrowserTest() override {}

  // ExtensionMessageBubbleBrowserTest:
  void SetUpCommandLine(base::CommandLine* command_line) override;

  // TestBrowserDialog:
  void ShowDialog(const std::string& name) override;

 private:
  // ExtensionMessageBubbleBrowserTest:
  void CheckBubbleNative(Browser* browser, AnchorPosition anchor) override;
  void CloseBubble(Browser* browser) override;
  void CloseBubbleNative(Browser* browser) override;
  void CheckBubbleIsNotPresentNative(Browser* browser) override;
  void ClickLearnMoreButton(Browser* browser) override;
  void ClickActionButton(Browser* browser) override;
  void ClickDismissButton(Browser* browser) override;

  // Whether to ignore requests from ExtensionMessageBubbleBrowserTest to
  // CloseBubble().
  bool block_close_ = false;

  DISALLOW_COPY_AND_ASSIGN(ExtensionMessageBubbleViewBrowserTest);
};

class ExtensionMessageBubbleViewBrowserTestLegacy
    : public ExtensionMessageBubbleViewBrowserTest {
 protected:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    ExtensionMessageBubbleViewBrowserTest::SetUpCommandLine(command_line);
    override_redesign_.reset();
    override_redesign_.reset(new extensions::FeatureSwitch::ScopedOverride(
        extensions::FeatureSwitch::extension_action_redesign(), false));
  }
};

void ExtensionMessageBubbleViewBrowserTest::SetUpCommandLine(
    base::CommandLine* command_line) {
  ExtensionMessageBubbleBrowserTest::SetUpCommandLine(command_line);
  // MD is required on Mac to get a Views bubble. On other platforms, it should
  // not affect the behavior of the bubble (just the appearance), so enable for
  // all platforms.
  command_line->AppendSwitch(switches::kExtendMdToSecondaryUi);
}

void ExtensionMessageBubbleViewBrowserTest::ShowDialog(
    const std::string& name) {
  // TODO(tapted): Add cases for all bubble types.
  EXPECT_EQ("devmode_warning", name);

  // When invoked this way, the dialog test harness must close the bubble.
  base::AutoReset<bool> guard(&block_close_, true);
  TestBubbleAnchoredToExtensionAction();
}

void ExtensionMessageBubbleViewBrowserTest::CheckBubbleNative(
    Browser* browser,
    AnchorPosition anchor) {
  gfx::Rect reference_bounds =
      GetAnchorReferenceBoundsForBrowser(browser, anchor);
  CheckBubbleAgainstReferenceBounds(GetViewsBubbleForBrowser(browser),
                                    reference_bounds);
}

void ExtensionMessageBubbleViewBrowserTest::CloseBubble(Browser* browser) {
  if (!block_close_)
    ExtensionMessageBubbleBrowserTest::CloseBubble(browser);
}

void ExtensionMessageBubbleViewBrowserTest::CloseBubbleNative(
    Browser* browser) {
  views::BubbleDialogDelegateView* bubble = GetViewsBubbleForBrowser(browser);
  ASSERT_TRUE(bubble);
  bubble->GetWidget()->Close();
  EXPECT_EQ(nullptr, GetViewsBubbleForBrowser(browser));
}

void ExtensionMessageBubbleViewBrowserTest::CheckBubbleIsNotPresentNative(
    Browser* browser) {
  EXPECT_EQ(nullptr, GetViewsBubbleForBrowser(browser));
}

void ExtensionMessageBubbleViewBrowserTest::ClickLearnMoreButton(
    Browser* browser) {
  ToolbarActionsBarBubbleViews* bubble = GetViewsBubbleForBrowser(browser);
  static_cast<views::LinkListener*>(bubble)->LinkClicked(
      const_cast<views::Link*>(bubble->learn_more_button()), 0);
}

void ExtensionMessageBubbleViewBrowserTest::ClickActionButton(
    Browser* browser) {
  ToolbarActionsBarBubbleViews* bubble = GetViewsBubbleForBrowser(browser);
  bubble->GetDialogClientView()->AcceptWindow();
}

void ExtensionMessageBubbleViewBrowserTest::ClickDismissButton(
    Browser* browser) {
  ToolbarActionsBarBubbleViews* bubble = GetViewsBubbleForBrowser(browser);
  bubble->GetDialogClientView()->CancelWindow();
}

IN_PROC_BROWSER_TEST_F(ExtensionMessageBubbleViewBrowserTest,
                       ExtensionBubbleAnchoredToExtensionAction) {
  TestBubbleAnchoredToExtensionAction();
}

IN_PROC_BROWSER_TEST_F(ExtensionMessageBubbleViewBrowserTestLegacy,
                       ExtensionBubbleAnchoredToAppMenu) {
  TestBubbleAnchoredToAppMenu();
}

IN_PROC_BROWSER_TEST_F(ExtensionMessageBubbleViewBrowserTestLegacy,
                       ExtensionBubbleAnchoredToAppMenuWithOtherAction) {
  TestBubbleAnchoredToAppMenuWithOtherAction();
}

IN_PROC_BROWSER_TEST_F(ExtensionMessageBubbleViewBrowserTest,
                       PRE_ExtensionBubbleShowsOnStartup) {
  PreBubbleShowsOnStartup();
}

IN_PROC_BROWSER_TEST_F(ExtensionMessageBubbleViewBrowserTest,
                       ExtensionBubbleShowsOnStartup) {
  TestBubbleShowsOnStartup();
}

IN_PROC_BROWSER_TEST_F(ExtensionMessageBubbleViewBrowserTest,
                       TestUninstallDangerousExtension) {
  TestUninstallDangerousExtension();
}

IN_PROC_BROWSER_TEST_F(ExtensionMessageBubbleViewBrowserTest,
                       TestDevModeBubbleIsntShownTwice) {
  TestDevModeBubbleIsntShownTwice();
}

// Tests for the extension bubble and settings overrides. These bubbles are
// currently only shown on Windows.
#if defined(OS_WIN)
IN_PROC_BROWSER_TEST_F(ExtensionMessageBubbleViewBrowserTest,
                       TestControlledNewTabPageMessageBubble) {
  TestControlledNewTabPageBubbleShown();
}

IN_PROC_BROWSER_TEST_F(ExtensionMessageBubbleViewBrowserTest,
                       TestControlledHomeMessageBubble) {
  TestControlledHomeBubbleShown();
}

IN_PROC_BROWSER_TEST_F(ExtensionMessageBubbleViewBrowserTest,
                       TestControlledSearchMessageBubble) {
  TestControlledSearchBubbleShown();
}

IN_PROC_BROWSER_TEST_F(ExtensionMessageBubbleViewBrowserTest,
                       PRE_TestControlledStartupMessageBubble) {
  PreTestControlledStartupBubbleShown();
}

IN_PROC_BROWSER_TEST_F(ExtensionMessageBubbleViewBrowserTest,
                       TestControlledStartupMessageBubble) {
  TestControlledStartupBubbleShown();
}

IN_PROC_BROWSER_TEST_F(ExtensionMessageBubbleViewBrowserTest,
                       PRE_TestControlledStartupNotShownOnRestart) {
  PreTestControlledStartupNotShownOnRestart();
}

IN_PROC_BROWSER_TEST_F(ExtensionMessageBubbleViewBrowserTest,
                       TestControlledStartupNotShownOnRestart) {
  TestControlledStartupNotShownOnRestart();
}

#endif  // defined(OS_WIN)

IN_PROC_BROWSER_TEST_F(ExtensionMessageBubbleViewBrowserTest,
                       TestBubbleWithMultipleWindows) {
  TestBubbleWithMultipleWindows();
}

// Crashes on Mac only.  http://crbug.com/702554
#if defined(OS_MACOSX)
#define MAYBE_TestClickingLearnMoreButton DISABLED_TestClickingLearnMoreButton
#else
#define MAYBE_TestClickingLearnMoreButton TestClickingLearnMoreButton
#endif
IN_PROC_BROWSER_TEST_F(ExtensionMessageBubbleViewBrowserTest,
                       MAYBE_TestClickingLearnMoreButton) {
  TestClickingLearnMoreButton();
}

IN_PROC_BROWSER_TEST_F(ExtensionMessageBubbleViewBrowserTest,
                       TestClickingActionButton) {
  TestClickingActionButton();
}

IN_PROC_BROWSER_TEST_F(ExtensionMessageBubbleViewBrowserTest,
                       TestClickingDismissButton) {
  TestClickingDismissButton();
}

// BrowserDialogTest for the warning bubble that appears at startup when there
// are extensions installed in developer mode. Can be invoked interactively with
// --gtest_filter=BrowserDialogTest.Invoke.
IN_PROC_BROWSER_TEST_F(ExtensionMessageBubbleViewBrowserTest,
                       InvokeDialog_devmode_warning) {
  RunDialog();
}
