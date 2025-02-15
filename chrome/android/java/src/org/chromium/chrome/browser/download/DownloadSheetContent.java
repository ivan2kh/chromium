// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download;

import android.app.Activity;
import android.support.v7.widget.Toolbar;
import android.view.View;

import org.chromium.base.ActivityState;
import org.chromium.base.ApplicationStatus;
import org.chromium.base.ApplicationStatus.ActivityStateListener;
import org.chromium.base.ThreadUtils;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.download.ui.DownloadManagerUi;
import org.chromium.chrome.browser.toolbar.BottomToolbarPhone;
import org.chromium.chrome.browser.widget.bottomsheet.BottomSheet.BottomSheetContent;

/**
 * A {@link BottomSheetContent} holding a {@link DownloadManagerUi} for display in the BottomSheet.
 */
public class DownloadSheetContent implements BottomSheetContent {
    private final View mContentView;
    private final Toolbar mToolbarView;
    private final ActivityStateListener mActivityStateListener;
    private DownloadManagerUi mDownloadManager;

    /**
     * @param activity The activity displaying the download manager UI.
     * @param isIncognito Whether the activity is currently displaying an incognito tab.
     */
    public DownloadSheetContent(ChromeActivity activity, final boolean isIncognito) {
        ThreadUtils.assertOnUiThread();

        mDownloadManager =
                new DownloadManagerUi(activity, isIncognito, activity.getComponentName(), false);
        mContentView = mDownloadManager.getView();
        mToolbarView = mDownloadManager.detachToolbarView();
        ((BottomToolbarPhone) activity.getToolbarManager().getToolbar())
                .setOtherToolbarStyle(mToolbarView);

        // #destroy() unregisters the ActivityStateListener to avoid checking for externally removed
        // downloads after the downloads UI is closed. This requires each download UI to have its
        // own ActivityStateListener. If multiple tabs are showing the downloads page, multiple
        // requests to check for externally removed downloads will be issued when the activity is
        // resumed.
        mActivityStateListener = new ActivityStateListener() {
            @Override
            public void onActivityStateChange(Activity activity, int newState) {
                if (newState == ActivityState.RESUMED) {
                    DownloadUtils.checkForExternallyRemovedDownloads(
                            mDownloadManager.getBackendProvider(), isIncognito);
                }
            }
        };
        ApplicationStatus.registerStateListenerForActivity(mActivityStateListener, activity);
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
        return mDownloadManager.getVerticalScrollOffset();
    }

    @Override
    public void destroy() {
        mDownloadManager.onDestroyed();
        mDownloadManager = null;
        ApplicationStatus.unregisterActivityStateListener(mActivityStateListener);
    }
}
