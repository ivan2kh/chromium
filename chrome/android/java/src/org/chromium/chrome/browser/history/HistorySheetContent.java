// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.history;

import android.support.v7.widget.Toolbar;
import android.view.View;

import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.toolbar.BottomToolbarPhone;
import org.chromium.chrome.browser.widget.bottomsheet.BottomSheet.BottomSheetContent;

/**
 * A {@link BottomSheetContent} holding a {@link HistoryManager} for display in the BottomSheet.
 */
public class HistorySheetContent implements BottomSheetContent {
    private final View mContentView;
    private final Toolbar mToolbarView;
    private HistoryManager mHistoryManager;

    /**
     * @param activity The activity displaying the history manager UI.
     */
    public HistorySheetContent(ChromeActivity activity) {
        mHistoryManager = new HistoryManager(activity, false);
        mContentView = mHistoryManager.getView();
        mToolbarView = mHistoryManager.detachToolbarView();
        ((BottomToolbarPhone) activity.getToolbarManager().getToolbar())
                .setOtherToolbarStyle(mToolbarView);
    }

    @Override
    public View getContentView() {
        return mContentView;
    }

    @Override
    public View getToolbarView() {
        return mToolbarView;
    }

    @Override
    public int getVerticalScrollOffset() {
        return mHistoryManager.getVerticalScrollOffset();
    }

    @Override
    public void destroy() {
        mHistoryManager.onDestroyed();
        mHistoryManager = null;
    }
}
