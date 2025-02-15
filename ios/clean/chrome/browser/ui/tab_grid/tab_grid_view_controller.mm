// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// ======                        New Architecture                         =====
// =         This code is only used in the new iOS Chrome architecture.       =
// ============================================================================

#import "ios/clean/chrome/browser/ui/tab_grid/tab_grid_view_controller.h"

#include "base/mac/foundation_util.h"
#import "ios/chrome/browser/ui/tab_switcher/tab_switcher_panel_collection_view_layout.h"
#import "ios/chrome/browser/ui/tab_switcher/tab_switcher_panel_overlay_view.h"
#import "ios/clean/chrome/browser/ui/actions/settings_actions.h"
#import "ios/clean/chrome/browser/ui/actions/tab_grid_actions.h"
#import "ios/clean/chrome/browser/ui/commands/settings_commands.h"
#import "ios/clean/chrome/browser/ui/commands/tab_commands.h"
#import "ios/clean/chrome/browser/ui/commands/tab_grid_commands.h"
#import "ios/clean/chrome/browser/ui/tab_grid/mdc_floating_button+cr_tab_grid.h"
#import "ios/clean/chrome/browser/ui/tab_grid/tab_grid_collection_view_layout.h"
#import "ios/clean/chrome/browser/ui/tab_grid/tab_grid_tab_cell.h"
#import "ios/clean/chrome/browser/ui/tab_grid/tab_grid_toolbar.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface TabGridViewController ()<SettingsActions,
                                    TabGridActions,
                                    UICollectionViewDataSource,
                                    UICollectionViewDelegate,
                                    SessionCellDelegate>
@property(nonatomic, weak) UICollectionView* grid;
@property(nonatomic, weak) UIView* noTabsOverlay;
@property(nonatomic, weak) TabGridToolbar* toolbar;
@property(nonatomic, weak) MDCFloatingButton* floatingNewTabButton;
@end

@implementation TabGridViewController

@synthesize dataSource = _dataSource;
@synthesize settingsCommandHandler = _settingsCommandHandler;
@synthesize tabGridCommandHandler = _tabGridCommandHandler;
@synthesize tabCommandHandler = _tabCommandHandler;
@synthesize grid = _grid;
@synthesize noTabsOverlay = _noTabsOverlay;
@synthesize toolbar = _toolbar;
@synthesize floatingNewTabButton = _floatingNewTabButton;

- (void)viewDidLoad {
  TabGridCollectionViewLayout* layout =
      [[TabGridCollectionViewLayout alloc] init];
  UICollectionView* grid = [[UICollectionView alloc] initWithFrame:CGRectZero
                                              collectionViewLayout:layout];
  grid.translatesAutoresizingMaskIntoConstraints = NO;
  grid.backgroundColor = [UIColor blackColor];

  [self.view addSubview:grid];
  self.grid = grid;
  self.grid.dataSource = self;
  self.grid.delegate = self;
  [self.grid registerClass:[TabGridTabCell class]
      forCellWithReuseIdentifier:[TabGridTabCell identifier]];

  [NSLayoutConstraint activateConstraints:@[
    [self.grid.topAnchor constraintEqualToAnchor:self.view.topAnchor],
    [self.grid.bottomAnchor constraintEqualToAnchor:self.view.bottomAnchor],
    [self.grid.leadingAnchor constraintEqualToAnchor:self.view.leadingAnchor],
    [self.grid.trailingAnchor constraintEqualToAnchor:self.view.trailingAnchor],
  ]];

  TabGridToolbar* toolbar = [[TabGridToolbar alloc] init];
  self.toolbar = toolbar;
  [self.view addSubview:self.toolbar];
  self.toolbar.translatesAutoresizingMaskIntoConstraints = NO;
  [NSLayoutConstraint activateConstraints:@[
    [self.toolbar.topAnchor constraintEqualToAnchor:self.view.topAnchor],
    [self.toolbar.heightAnchor
        constraintEqualToAnchor:self.topLayoutGuide.heightAnchor
                       constant:self.toolbar.intrinsicContentSize.height],
    [self.toolbar.leadingAnchor
        constraintEqualToAnchor:self.view.leadingAnchor],
    [self.toolbar.trailingAnchor
        constraintEqualToAnchor:self.view.trailingAnchor]
  ]];
}

- (void)viewWillAppear:(BOOL)animated {
  MDCFloatingButton* floatingNewTabButton =
      [MDCFloatingButton cr_tabGridNewTabButton];
  self.floatingNewTabButton = floatingNewTabButton;
  [self.floatingNewTabButton
      setFrame:[MDCFloatingButton
                   cr_frameForTabGridNewTabButtonInRect:self.view.bounds]];
  [self.view addSubview:self.floatingNewTabButton];
}

- (void)viewDidLayoutSubviews {
  [super viewDidLayoutSubviews];
  self.grid.contentInset =
      UIEdgeInsetsMake(CGRectGetMaxY(self.toolbar.frame), 0, 0, 0);
}

- (UIStatusBarStyle)preferredStatusBarStyle {
  return UIStatusBarStyleLightContent;
}

#pragma mark - UICollectionViewDataSource methods

- (NSInteger)numberOfSectionsInCollectionView:
    (UICollectionView*)collectionView {
  return 1;
}

- (NSInteger)collectionView:(UICollectionView*)collectionView
     numberOfItemsInSection:(NSInteger)section {
  int items = [self.dataSource numberOfTabsInTabGrid];
  // HACK: Do not make showing noTabsOverlay a side effect of the dataSource
  // callback.
  if (items) {
    [self removeNoTabsOverlay];
  } else {
    [self showNoTabsOverlay];
  }
  return items;
}

- (UICollectionViewCell*)collectionView:(UICollectionView*)collectionView
                 cellForItemAtIndexPath:(nonnull NSIndexPath*)indexPath {
  TabGridTabCell* cell =
      base::mac::ObjCCastStrict<TabGridTabCell>([collectionView
          dequeueReusableCellWithReuseIdentifier:[TabGridTabCell identifier]
                                    forIndexPath:indexPath]);
  cell.delegate = self;
  [cell setSessionType:TabSwitcherSessionType::REGULAR_SESSION];
  DCHECK_LE(indexPath.item, INT_MAX);
  int item = static_cast<int>(indexPath.item);
  [cell setAppearanceForTabTitle:[self.dataSource titleAtIndex:item]
                         favicon:nil
                        cellSize:CGSizeZero];
  [cell setSelected:(indexPath.item == [self.dataSource indexOfActiveTab])];
  return cell;
}

#pragma mark - UICollectionViewDelegate methods

- (BOOL)collectionView:(UICollectionView*)collectionView
    shouldSelectItemAtIndexPath:(NSIndexPath*)indexPath {
  // Prevent user selection of items.
  return NO;
}

#pragma mark - ZoomTransitionDelegate methods

- (CGRect)rectForZoomWithKey:(NSObject*)key inView:(UIView*)view {
  NSIndexPath* cellPath = base::mac::ObjCCastStrict<NSIndexPath>(key);
  if (!key)
    return CGRectNull;
  UICollectionViewCell* cell = [self.grid cellForItemAtIndexPath:cellPath];
  return [view convertRect:cell.bounds fromView:cell];
}

#pragma mark - SettingsActions

- (void)showSettings:(id)sender {
  [self.settingsCommandHandler showSettings];
}

#pragma mark - TabGridActions

- (void)showTabGrid:(id)sender {
  [self.tabGridCommandHandler showTabGrid];
}

- (void)createNewTab:(id)sender {
  NSInteger index = [self.grid numberOfItemsInSection:0];
  NSIndexPath* indexPath = [NSIndexPath indexPathForItem:index inSection:0];
  auto updateBlock = ^{
    // Unselect current selected item.
    NSIndexPath* selectedIndexPath =
        [NSIndexPath indexPathForItem:[self.dataSource indexOfActiveTab]
                            inSection:0];
    [self.grid reloadItemsAtIndexPaths:@[ selectedIndexPath ]];

    // Create and show new tab.
    [self.tabCommandHandler createNewTabAtIndexPath:indexPath];
    [self.tabCommandHandler showTabAtIndexPath:indexPath];
    [self.grid insertItemsAtIndexPaths:@[ indexPath ]];
  };
  [self.grid performBatchUpdates:updateBlock completion:nil];
}

#pragma mark - SessionCellDelegate

- (TabSwitcherCache*)tabSwitcherCache {
  // PLACEHOLDER: return image cache.
  return nil;
}

- (void)cellPressed:(UICollectionViewCell*)cell {
  int selectedIndex = [self.dataSource indexOfActiveTab];
  NSIndexPath* newSelectedIndexPath = [self.grid indexPathForCell:cell];
  [self.tabCommandHandler showTabAtIndexPath:newSelectedIndexPath];
  if (selectedIndex == kTabGridDataSourceInvalidIndex) {
    [self.grid reloadItemsAtIndexPaths:@[ newSelectedIndexPath ]];
  } else if (newSelectedIndexPath.item != selectedIndex) {
    NSIndexPath* selectedIndexPath =
        [NSIndexPath indexPathForItem:selectedIndex inSection:0];
    [self.grid
        reloadItemsAtIndexPaths:@[ selectedIndexPath, newSelectedIndexPath ]];
  }
}

- (void)deleteButtonPressedForCell:(UICollectionViewCell*)cell {
  auto updateBlock = ^{
    NSIndexPath* indexPath = [self.grid indexPathForCell:cell];
    [self.tabCommandHandler closeTabAtIndexPath:indexPath];
    [self.grid deleteItemsAtIndexPaths:@[ indexPath ]];
  };
  [self.grid performBatchUpdates:updateBlock completion:nil];
}

#pragma mark - Private

// Shows an overlay covering the entire tab grid that informs the user that
// there are no tabs.
- (void)showNoTabsOverlay {
  // PLACEHOLDER: The new tab grid will have a completely different zero tab
  // overlay from the tab switcher. Also, the overlay will be above the recent
  // tabs section.
  TabSwitcherPanelOverlayView* overlayView =
      [[TabSwitcherPanelOverlayView alloc] initWithFrame:self.grid.bounds
                                            browserState:nil];
  overlayView.overlayType =
      TabSwitcherPanelOverlayType::OVERLAY_PANEL_USER_NO_OPEN_TABS;
  overlayView.autoresizingMask =
      UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  [self.grid addSubview:overlayView];
  self.noTabsOverlay = overlayView;
}

// Removes the noTabsOverlay covering the entire tab grid.
- (void)removeNoTabsOverlay {
  [self.noTabsOverlay removeFromSuperview];
}

@end
